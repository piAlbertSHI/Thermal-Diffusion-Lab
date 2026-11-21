# Thermal Diffusion Lab

[Open the interactive lab](https://pialbertshi.github.io/Thermal-Diffusion-Lab/) · [中文说明](README.zh-CN.md)

A hot and a cold half-rod meet at the midpoint. How fast does the jump disappear, and can a numerical method conserve heat as it smooths the interface? This C++17 project compares three time integrators for one-dimensional diffusion. The browser replays fields computed by the same solver that produces the command-line CSV.

[![Computed temperature profiles at four times and a space–time heat map for the insulated rod](docs/preview.png)](https://pialbertshi.github.io/Thermal-Diffusion-Lab/)

## Explore the experiment

| Try this in the lab | What to look for |
|---|---|
| Start with **Hot–cold interface**, then press **Play** | The jump broadens while the mean temperature stays at 5.5. Download a 64-cell profile at any frame. |
| Choose **Single cosine mode** | The dotted analytical curve is evaluated at the same cell centres and physical time. The readout gives the largest absolute difference. |
| Select **Forward Euler** and change the diffusion ratio to **1.00** | The solver's stability guard explains the missing run. Switch to **Backward Euler** to inspect this time step. |

The gallery contains 24 combinations of initial field, integrator and diffusion ratio, each with 41 snapshots. Their timestamps coincide so that changing schemes preserves the physical comparison time.

## Numerical method

For constant diffusivity α, the model is ∂T/∂t = α ∂²T/∂x² on a rod with zero heat flux at both ends. The default example uses length 20, diffusivity 1 and 64 equal cells in model units. Each cell exchanges heat with its neighbours. An outer face contributes zero flux, so internal exchanges cancel in the sum over cells.

| Time integrator | Time order | Implementation |
|---|---|---|
| Forward Euler | 1 | Explicit stencil with r = αΔt/Δx² ≤ 0.5 |
| Backward Euler | 1 | Tridiagonal solve evaluated at the new time |
| Crank–Nicolson | 2 | Average of the old and new diffusive fluxes |

Both implicit schemes factor the constant tridiagonal matrix once. Each time step uses O(N) work and storage. Unconditional linear stability still permits time-discretization error. Crank–Nicolson can retain oscillatory high-frequency components at large time steps.

## Verification

The smooth reference is T(x,t) = 5.5 + 4.5 cos(πx/L) exp(−απ²t/L²). This refinement check uses L = α = 1, r = 0.25 and final time 0.03125. The 32- and 64-cell runs end at exactly the same time. Errors are measured against the continuum solution at cell centres.

| Scheme | Maximum absolute error, 64 cells | Error at 32 cells / error at 64 |
|---|---:|---:|
| Forward Euler | 0.00010235 | 3.998 |
| Backward Euler | 0.00051144 | 3.991 |
| Crank–Nicolson | 0.00020462 | 3.994 |

Halving the cell width also quarters the time step in this experiment. The ratios measure the combined grid-and-time refinement. Seven native checks cover conservation, symmetry, the analytical reference, refinement, a discrete eigenmode, stability and input handling. CI regenerates all 62,976 browser temperature samples and compares them with the checked-in data.

## Build and run

Use a C++17 compiler and CMake 3.20 or newer. From the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/diffusion_cli --method crank-nicolson --profile step --ratio 0.5 --cells 64 --steps 128 --output profile.csv
./build/diffusion_cli --method crank-nicolson --steps 128 --probe 4 --output trace.csv
```

With Visual Studio on Windows the executable is `build\Release\diffusion_cli.exe`. `--steps` counts time steps. The CLI reports the physical time alongside the cell-centre coordinate and temperature. Run `--help` for all options.

The probe follows the cell nearest x = 4 (20% of the rod) from step zero through the final step. The browser offers a separate 41-frame probe export beneath the heat map.

To rebuild the web data and serve the lab locally:

```sh
./build/diffusion_cli --gallery docs/data/profiles.json
python -m http.server 8000 --directory docs
```

Open `http://localhost:8000`. The native solver has no Python dependency. The optional [figure script](tools/render_preview.py) uses Matplotlib to regenerate the figure above.

## Coursework and development

This began as a student numerical-methods project comparing explicit and implicit heat-equation solvers in C++14. The original split-temperature example and insulated rod motivated the current model. The revised implementation uses a common conservative spatial operator, adds Crank–Nicolson, and makes time, stability and numerical error observable. The [original source](coursework/original), [course report](coursework/heat-equation-report.docx) and [historical repository snapshot](https://github.com/piAlbertSHI/Thermal-Diffusion-Lab/tree/4530ba4a7494b1f4ff3b78ca107ece2a44bae7a0) preserve the submitted work.
