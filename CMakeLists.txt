cmake_minimum_required(VERSION 3.20)
project(ThermalDiffusionLab LANGUAGES CXX)

add_library(diffusion src/solver.cpp)
target_include_directories(diffusion PUBLIC include)
target_compile_features(diffusion PUBLIC cxx_std_17)
set_target_properties(diffusion PROPERTIES CXX_EXTENSIONS OFF)

add_executable(diffusion_cli apps/diffusion_cli.cpp)
target_link_libraries(diffusion_cli PRIVATE diffusion)

include(CTest)
if(BUILD_TESTING)
  add_executable(diffusion_checks tests/solver_checks.cpp)
  target_link_libraries(diffusion_checks PRIVATE diffusion)
  foreach(check mass symmetry analytic convergence stability reference inputs)
    add_test(NAME diffusion_${check} COMMAND diffusion_checks ${check})
  endforeach()
endif()
