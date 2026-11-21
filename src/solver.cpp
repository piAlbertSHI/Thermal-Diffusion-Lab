#include "diffusion/solver.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace diffusion {
namespace {

void validate_grid(const Grid& grid) {
    if (grid.cells < 2 || grid.cells > 1000000 || !std::isfinite(grid.length) || grid.length <= 0 ||
        !std::isfinite(grid.diffusivity) || grid.diffusivity <= 0) {
        throw std::invalid_argument(
            "Use 2 to 1000000 cells and positive finite length and diffusivity.");
    }
}

double theta(Method method) {
    switch (method) {
        case Method::forward_euler:
            return 0.0;
        case Method::backward_euler:
            return 1.0;
        case Method::crank_nicolson:
            return 0.5;
    }
    throw std::invalid_argument("Unknown time integrator.");
}

}  // namespace

double cell_center(const Grid& grid, std::size_t cell) {
    return (static_cast<double>(cell) + 0.5) * (grid.length / grid.cells);
}

std::vector<double> initial_field(const Grid& grid, Profile profile) {
    validate_grid(grid);
    std::vector<double> values(grid.cells);
    for (std::size_t i = 0; i < values.size(); ++i) {
        const double s = (static_cast<double>(i) + 0.5) / grid.cells;
        switch (profile) {
            case Profile::step:
                values[i] = s < 0.5 ? 1.0 : (s > 0.5 ? 10.0 : 5.5);
                break;
            case Profile::cosine:
                values[i] = cosine_reference(grid, cell_center(grid, i), 0);
                break;
            case Profile::pulse:
                values[i] = 1.0 + 9.0 * std::exp(-100.0 * (s - 0.5) * (s - 0.5));
                break;
            default:
                throw std::invalid_argument("Unknown initial condition.");
        }
    }
    return values;
}

double cosine_reference(const Grid& grid, double x, double time) {
    const double wavenumber = std::acos(-1.0) / grid.length;
    return 5.5 + 4.5 * std::cos(wavenumber * x) *
                     std::exp(-grid.diffusivity * wavenumber * wavenumber * time);
}

double mean_temperature(const std::vector<double>& field) {
    if (field.empty())
        throw std::invalid_argument("The temperature field is empty.");
    return std::accumulate(field.begin(), field.end(), 0.0) / field.size();
}

Integrator::Integrator(Grid grid, Method method, double ratio, std::vector<double> initial)
    : dt_(0), explicit_weight_(0), implicit_weight_(0), values_(std::move(initial)) {
    validate_grid(grid);
    if (values_.size() != grid.cells ||
        !std::all_of(values_.begin(), values_.end(), [](double v) { return std::isfinite(v); })) {
        throw std::invalid_argument("Provide one finite temperature per cell.");
    }
    if (!std::isfinite(ratio) || ratio <= 0)
        throw std::invalid_argument("Diffusion ratio must be positive and finite.");
    if (method == Method::forward_euler && ratio > 0.5) {
        throw std::invalid_argument("Forward Euler requires a diffusion ratio r <= 0.5.");
    }
    const double dx = grid.length / grid.cells;
    dt_ = ratio * dx * dx / grid.diffusivity;
    if (!std::isfinite(dt_) || dt_ <= 0 || ratio > std::numeric_limits<double>::max() / 4) {
        throw std::invalid_argument("Grid and ratio exceed floating-point range.");
    }
    const double weight = theta(method);
    explicit_weight_ = (1.0 - weight) * ratio;
    implicit_weight_ = weight * ratio;
    rhs_.resize(grid.cells);
    if (implicit_weight_ == 0.0)
        return;

    // Factor the tridiagonal matrix once. End cells have only one interior face.
    inverse_pivot_.resize(grid.cells);
    superdiagonal_.resize(grid.cells - 1);
    for (std::size_t i = 0; i < grid.cells; ++i) {
        const double faces = (i == 0 || i + 1 == grid.cells) ? 1.0 : 2.0;
        const double diagonal = 1.0 + faces * implicit_weight_;
        const double pivot =
            i == 0 ? diagonal : diagonal + implicit_weight_ * superdiagonal_[i - 1];
        if (!std::isfinite(pivot) || pivot <= 0)
            throw std::invalid_argument("Tridiagonal pivot exceeds numerical range.");
        inverse_pivot_[i] = 1.0 / pivot;
        if (i + 1 < grid.cells)
            superdiagonal_[i] = -implicit_weight_ * inverse_pivot_[i];
    }
}

void Integrator::advance(std::size_t steps) {
    if (steps > std::numeric_limits<std::size_t>::max() - step_count_) {
        throw std::overflow_error("Time-step counter overflow.");
    }
    const std::size_t count = values_.size();
    for (std::size_t step = 0; step < steps; ++step) {
        for (std::size_t i = 0; i < count; ++i) {
            const double left = i == 0 ? 0.0 : values_[i - 1] - values_[i];
            const double right = i + 1 == count ? 0.0 : values_[i + 1] - values_[i];
            rhs_[i] = values_[i] + explicit_weight_ * (left + right);
        }
        if (implicit_weight_ != 0.0) {
            rhs_[0] *= inverse_pivot_[0];
            for (std::size_t i = 1; i < count; ++i) {
                rhs_[i] = (rhs_[i] + implicit_weight_ * rhs_[i - 1]) * inverse_pivot_[i];
            }
            for (std::size_t i = count - 1; i > 0; --i)
                rhs_[i - 1] -= superdiagonal_[i - 1] * rhs_[i];
        }
        values_.swap(rhs_);
        ++step_count_;
    }
}
}  // namespace diffusion
