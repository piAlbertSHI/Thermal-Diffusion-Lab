#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

#include "diffusion/solver.hpp"

namespace {
using namespace diffusion;

Method parse_method(const std::string& value) {
    if (value == "forward-euler")
        return Method::forward_euler;
    if (value == "backward-euler")
        return Method::backward_euler;
    if (value == "crank-nicolson")
        return Method::crank_nicolson;
    throw std::invalid_argument("Unknown method: " + value);
}

Profile parse_profile(const std::string& value) {
    if (value == "step")
        return Profile::step;
    if (value == "cosine")
        return Profile::cosine;
    if (value == "pulse")
        return Profile::pulse;
    throw std::invalid_argument("Unknown profile: " + value);
}

std::size_t integer_option(const std::string& value, unsigned long maximum) {
    if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos) {
        throw std::invalid_argument("Expected a nonnegative integer: " + value);
    }
    const auto number = std::stoul(value);
    if (number > maximum)
        throw std::out_of_range("Integer exceeds the supported range.");
    return number;
}

double real_option(const std::string& value) {
    std::size_t read = 0;
    const double result = std::stod(value, &read);
    if (read != value.size() || !std::isfinite(result))
        throw std::invalid_argument("Expected a finite number.");
    return result;
}

void export_gallery(std::ostream& out) {
    const Grid grid;
    constexpr int frames = 41;
    const double dx = grid.length / grid.cells;
    out << std::setprecision(10) << "{\"schema\":1,\"cells\":" << grid.cells
        << ",\"length\":" << grid.length << ",\"diffusivity\":" << grid.diffusivity
        << ",\"frameDt\":" << 4 * dx * dx / grid.diffusivity << ",\"runs\":[";
    bool first = true;
    for (auto profile : {"step", "cosine", "pulse"}) {
        for (auto method : {"forward-euler", "backward-euler", "crank-nicolson"}) {
            for (double ratio : {0.25, 0.5, 1.0}) {
                const auto scheme = parse_method(method);
                if (scheme == Method::forward_euler && ratio > 0.5)
                    continue;
                Integrator solver(grid, scheme, ratio, initial_field(grid, parse_profile(profile)));
                if (!first)
                    out << ',';
                first = false;
                out << "\n{\"profile\":\"" << profile << "\",\"method\":\"" << method
                    << "\",\"ratio\":" << ratio << ",\"dt\":" << solver.time_step()
                    << ",\"frames\":[";
                for (int frame = 0; frame < frames; ++frame) {
                    if (frame > 0) {
                        out << ',';
                        solver.advance(static_cast<std::size_t>(4 / ratio));
                    }
                    out << '[';
                    for (std::size_t i = 0; i < grid.cells; ++i) {
                        if (i > 0)
                            out << ',';
                        out << solver.field()[i];
                    }
                    out << ']';
                }
                out << "]}";
            }
        }
    }
    out << "]}\n";
}

void usage() {
    std::cout
        << "diffusion_cli [--method forward-euler|backward-euler|crank-nicolson]\n"
           "  [--profile step|cosine|pulse] [--ratio R] [--cells N] [--steps N] [--output FILE]\n"
           "  [--probe X]   Output every time step at the nearest cell centre instead of a final "
           "profile.\n"
           "diffusion_cli --gallery FILE   Export the browser experiments.\n"
           "Grid defaults: L=20, diffusivity=1, 64 cells. CSV goes to stdout by default.\n";
}
}  // namespace

int main(int argc, char** argv) {
    using namespace diffusion;
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") {
            usage();
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--gallery") {
            std::ofstream gallery(argv[2], std::ios::binary);
            gallery.exceptions(std::ios::badbit | std::ios::failbit);
            export_gallery(gallery);
            gallery.close();
            return 0;
        }
        Grid grid;
        Method method = Method::crank_nicolson;
        Profile profile = Profile::step;
        double ratio = 0.5;
        std::size_t steps = 128;
        std::optional<double> probe;
        std::string filename;
        for (int i = 1; i < argc; i += 2) {
            const std::string key = argv[i];
            if (i + 1 >= argc)
                throw std::invalid_argument("Missing value for " + key);
            const std::string value = argv[i + 1];
            if (key == "--method")
                method = parse_method(value);
            else if (key == "--profile")
                profile = parse_profile(value);
            else if (key == "--ratio")
                ratio = real_option(value);
            else if (key == "--cells")
                grid.cells = integer_option(value, 1000000);
            else if (key == "--steps")
                steps = integer_option(value, 10000000);
            else if (key == "--probe")
                probe = real_option(value);
            else if (key == "--output")
                filename = value;
            else
                throw std::invalid_argument("Unknown option: " + key);
        }
        Integrator solver(grid, method, ratio, initial_field(grid, profile));
        if (probe && (*probe < 0 || *probe > grid.length))
            throw std::invalid_argument("Probe position must lie inside the rod.");
        std::ofstream file;
        if (!filename.empty()) {
            file.exceptions(std::ios::badbit | std::ios::failbit);
            file.open(filename);
        }
        std::ostream& out = filename.empty() ? std::cout : file;
        out << std::setprecision(15) << "x,time,temperature\n";
        auto write_sample = [&](std::size_t cell) {
            out << cell_center(grid, cell) << ',' << solver.time() << ',' << solver.field()[cell]
                << '\n';
        };
        if (probe) {
            const auto cell = std::min(grid.cells - 1,
                                       static_cast<std::size_t>(*probe / grid.length * grid.cells));
            write_sample(cell);
            for (std::size_t i = 0; i < steps; ++i) {
                solver.advance();
                write_sample(cell);
            }
        } else {
            solver.advance(steps);
            for (std::size_t cell = 0; cell < grid.cells; ++cell)
                write_sample(cell);
        }
        if (file.is_open())
            file.close();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "diffusion_cli: " << error.what() << '\n';
        return 2;
    }
}
