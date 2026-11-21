#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include "diffusion/solver.hpp"

using namespace diffusion;

namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}

double cosine_error(std::size_t cells, Method scheme, double ratio, std::size_t steps) {
    Grid grid{cells, 1.0, 1.0};
    Integrator solver(grid, scheme, ratio, initial_field(grid, Profile::cosine));
    solver.advance(steps);
    double maximum = 0.0;
    for (std::size_t i = 0; i < cells; ++i) {
        maximum = std::max(maximum,
                           std::abs(solver.field()[i] -
                                    cosine_reference(grid, cell_center(grid, i), solver.time())));
    }
    return maximum;
}

template <class Function>
void rejects(Function operation) {
    try {
        operation();
    } catch (const std::invalid_argument&) {
        return;
    }
    throw std::runtime_error("Expected rejection.");
}
}  // namespace

int main(int argc, char** argv) {
    if (argc != 2)
        return 2;
    const std::string check = argv[1];
    try {
        Grid grid;
        const auto initial = initial_field(grid, Profile::step);
        for (auto method :
             {Method::forward_euler, Method::backward_euler, Method::crank_nicolson}) {
            Integrator solver(grid, method, 0.5, initial);
            if (check == "mass") {
                solver.advance(500);
                require(std::abs(mean_temperature(solver.field()) - 5.5) < 2e-12, "Mean drift.");
                require(
                    *std::min_element(solver.field().begin(), solver.field().end()) >= 1.0 - 1e-12,
                    "Minimum.");
                require(
                    *std::max_element(solver.field().begin(), solver.field().end()) <= 10.0 + 1e-12,
                    "Maximum.");
            } else if (check == "symmetry") {
                solver.advance(100);
                for (std::size_t i = 0; i < grid.cells; ++i)
                    require(std::abs(solver.field()[i] + solver.field()[grid.cells - 1 - i] - 11) <
                                2e-12,
                            "Reflection.");
            } else if (check == "analytic") {
                const double error = cosine_error(64, method, 0.25, 512);
                std::cout << "64 cells, t=0.03125: max absolute error " << error << '\n';
                require(error < 0.0012, "Continuum cosine error.");
            } else if (check == "convergence") {
                const double coarse = cosine_error(32, method, 0.25, 128);
                const double fine = cosine_error(64, method, 0.25, 512);
                std::cout << "32/64 cell error ratio: " << coarse / fine << '\n';
                require(coarse / fine > 3.8 && coarse / fine < 4.2, "Refinement rate.");
            } else if (check == "reference") {
                // The cell-centred no-flux cosine is also a discrete eigenvector.
                Grid small{9, 2.0, 0.7};
                auto start = initial_field(small, Profile::cosine);
                const double half_sine = std::sin(std::acos(-1.0) / (2 * small.cells));
                const double lambda = 4 * 0.3 * half_sine * half_sine;
                double amplification = 1 - lambda;
                if (method == Method::backward_euler)
                    amplification = 1 / (1 + lambda);
                if (method == Method::crank_nicolson)
                    amplification = (1 - lambda / 2) / (1 + lambda / 2);
                Integrator discrete(small, method, 0.3, start);
                discrete.advance(7);
                for (std::size_t i = 0; i < small.cells; ++i)
                    require(std::abs(discrete.field()[i] - 5.5 -
                                     (start[i] - 5.5) * std::pow(amplification, 7)) < 2e-13,
                            "Discrete mode.");
            } else if (check != "stability" && check != "inputs") {
                throw std::invalid_argument("Unknown check.");
            }
        }
        if (check == "stability") {
            rejects([&] { Integrator unstable(grid, Method::forward_euler, 0.5001, initial); });
            Integrator implicit(grid, Method::backward_euler, 10, initial);
            implicit.advance(8);
            require(std::abs(mean_temperature(implicit.field()) - 5.5) < 1e-12,
                    "Large implicit step.");
        }
        if (check == "inputs") {
            rejects([&] { Integrator bad(grid, Method::backward_euler, -1, initial); });
            rejects([&] { Integrator bad(grid, Method::backward_euler, 1, {0.0}); });
            rejects([&] { initial_field({1, 1, 1}, Profile::step); });
            rejects([&] {
                initial_field({10, 1, std::numeric_limits<double>::infinity()}, Profile::step);
            });
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << check << ": " << error.what() << '\n';
        return 1;
    }
}
