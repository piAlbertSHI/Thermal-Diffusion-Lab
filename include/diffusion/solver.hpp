#pragma once

#include <cstddef>
#include <vector>

namespace diffusion {

enum class Method { forward_euler, backward_euler, crank_nicolson };
enum class Profile { step, cosine, pulse };

struct Grid {
    std::size_t cells = 64;
    double length = 20.0;
    double diffusivity = 1.0;
};

double cell_center(const Grid& grid, std::size_t cell);
std::vector<double> initial_field(const Grid& grid, Profile profile);
double cosine_reference(const Grid& grid, double x, double time);
double mean_temperature(const std::vector<double>& field);

// Cell-centred finite volumes. The two outer faces carry zero diffusive flux.
class Integrator {
   public:
    Integrator(Grid grid, Method method, double ratio, std::vector<double> initial);
    void advance(std::size_t steps = 1);
    const std::vector<double>& field() const {
        return values_;
    }
    double time() const {
        return static_cast<double>(step_count_) * dt_;
    }
    double time_step() const {
        return dt_;
    }

   private:
    double dt_;
    double explicit_weight_;
    double implicit_weight_;
    std::size_t step_count_ = 0;
    std::vector<double> values_;
    std::vector<double> rhs_;
    std::vector<double> inverse_pivot_;
    std::vector<double> superdiagonal_;
};

}  // namespace diffusion
