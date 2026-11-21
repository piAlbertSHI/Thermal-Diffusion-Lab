"""Regenerate the README figure from the C++ gallery. Requires matplotlib."""
import json
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
from matplotlib.ticker import MaxNLocator

root = Path(__file__).resolve().parents[1]
data = json.loads((root / "docs/data/profiles.json").read_text(encoding="utf-8"))
run = next(r for r in data["runs"] if (r["profile"], r["method"], r["ratio"]) == ("step", "crank-nicolson", 0.5))
x = [(i + 0.5) * data["length"] / data["cells"] for i in range(data["cells"])]
times = [i * data["frameDt"] for i in range(len(run["frames"]))]
background, panel, light = "#102d34", "#14333b", "#e6f1ec"
plt.rcParams.update({"font.family": "DejaVu Sans", "font.size": 10, "text.color": light,
                     "axes.labelcolor": light, "xtick.color": "#b8cecd", "ytick.color": "#b8cecd",
                     "axes.edgecolor": "#5c777b", "axes.facecolor": panel, "savefig.facecolor": background})
fig, (ax, heat) = plt.subplots(1, 2, figsize=(12, 4.9), gridspec_kw={"width_ratios": [1.2, 1]})
fig.patch.set_facecolor(background)
fig.subplots_adjust(left=.075, right=.935, bottom=.16, top=.72, wspace=.29)
fig.text(.073, .91, "Heat crossing an insulated rod", fontsize=23, weight="semibold")
fig.text(.075, .82, "Crank–Nicolson   ·   64 cells   ·   diffusion ratio 0.50", color="#b7d2d0", fontsize=12)
for index, color in zip([0, 4, 16, 40], ["#9fbfc7", "#76c3bd", "#ffcf88", "#ff9c65"]):
    ax.plot(x, run["frames"][index], color=color, lw=2.15, ls="--" if index == 0 else "-", label=f"t = {times[index]:.2f}")
ax.set(xlabel="Position, x", ylabel="Temperature, T", xlim=(0, data["length"]), ylim=(0.25, 10.75))
ax.grid(alpha=.15)
ax.legend(loc="upper left", frameon=False, fontsize=8.5, ncol=2)
ax.spines[["top","right"]].set_visible(False)
colors = LinearSegmentedColormap.from_list("thermal", ["#2b6698","#5ba0ac","#e7e3ad","#f09b4d","#c0592f"])
half_dt = data["frameDt"] / 2
im = heat.imshow(run["frames"], extent=[0,data["length"],-half_dt,times[-1]+half_dt],
                 origin="lower", aspect="auto", cmap=colors, interpolation="nearest", vmin=1, vmax=10)
heat.set(xlabel="Position, x", ylabel="Time, t", ylim=(0,times[-1]))
cb = fig.colorbar(im, ax=heat, fraction=.045, pad=.04)
cb.ax.set_title("T", color=light, fontsize=9, pad=5)
cb.locator = MaxNLocator(nbins=4)
cb.update_ticks()
fig.savefig(root / "docs/preview.png", dpi=155)
