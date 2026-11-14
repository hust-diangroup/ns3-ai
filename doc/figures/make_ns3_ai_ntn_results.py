#!/usr/bin/env python3
"""Data paths are relative to the toolkit workspace root (the folder that holds ns-3-dev/); pass a path on the command line to override.
Results figure for ns3-ai-ntn, from data stored in the tree.

(a) Gym interface against ns3-gym: CPU cycles per C++->Python and Python->C++
    transfer in RL-TCP, 6 seeds. Upstream ns3-ai benchmark, data in
    docs/benchmarking/gym-interface-data.m.
(b) Pure C++ (libtorch) against the message interface (PyTorch): CPU cycles per
    RL decision in RL-TCP, 6 seeds. Upstream benchmark, docs/benchmarking/pure-cpp-data.m.
(c) OranNtnGymHandover stepped on a measured NR NTN radio (oran-ntn-gym-handover-example,
    12 s, in-process mode, one run), parsed from the stored console log.

    python3 make_ns3_ai_ntn_results.py [GYM_RUN_LOG]
"""
import csv
import os
import re
import statistics as st
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
MOD = os.path.dirname(os.path.dirname(HERE))
BENCH = os.path.join(MOD, "docs", "benchmarking")
GYM_LOG = ("./ns-3-dev/toolkit-audit-2026-10-06/runs/"
           "A6-ai-twin-security/gym-inproc-12s/run.log")


def matrix(text, name):
    """two-column numeric matrix `name = [ a b; ... ];` from a MATLAB .m file"""
    body = re.search(r"%s\s*=\s*\[(.*?)\];" % re.escape(name), text, re.S).group(1)
    rows = [r.split() for r in body.replace(";", "\n").splitlines() if r.strip()]
    return [(float(a), float(b)) for a, b in rows]


def main():
    log = sys.argv[1] if len(sys.argv) > 1 else GYM_LOG
    plt.rcParams.update({"font.family": "sans-serif",
                         "font.sans-serif": ["Arial", "Liberation Sans", "DejaVu Sans"],
                         "font.size": 10, "axes.linewidth": 0.8, "svg.fonttype": "none"})
    gym = open(os.path.join(BENCH, "gym-interface-data.m")).read()
    pc = open(os.path.join(BENCH, "pure-cpp-data.m")).read()
    c2p, p2c = matrix(gym, "cpp2py_mean"), matrix(gym, "py2cpp_mean")
    cyc = matrix(pc, "cycle_mean")
    summary = []

    fig, axs = plt.subplots(1, 3, figsize=(12.0, 3.9), gridspec_kw={"width_ratios": [1.0, 0.75, 1.35]})
    # (a)
    ax = axs[0]
    groups = [("C++ to Python", c2p), ("Python to C++", p2c)]
    for gi, (lab, rows) in enumerate(groups):
        for ti, (tool, col, hatch) in enumerate([("ns3-gym", "#bbbbbb", "//"), ("ns3-ai Gym", "#0072B2", "")]):
            vals = [r[ti] for r in rows]
            m, s = st.mean(vals), st.stdev(vals)
            ax.bar(gi + (ti - 0.5) * 0.36, m, 0.34, yerr=s, color=col, hatch=hatch, edgecolor="#333333", lw=0.6,
                   capsize=3, label=tool if gi == 0 else None)
            summary.append({"panel": "a", "case": "%s, %s" % (lab, tool), "seeds": len(vals),
                            "mean_cycles": round(m), "sd_across_seeds": round(s)})
        ratio = st.mean(r[0] for r in rows) / st.mean(r[1] for r in rows)
        ax.text(gi, st.mean(r[0] for r in rows) * 1.25, "%.0fx fewer" % ratio, ha="center", fontsize=9)
    ax.set_yscale("log")
    ax.set_ylim(1e3, 6e5)
    ax.set_xticks([0, 1])
    ax.set_xticklabels([g[0] for g in groups])
    ax.set_ylabel("CPU cycles per transfer")
    ax.set_title("(a) Gym transfer cost, RL-TCP", fontsize=10.5, loc="left")
    ax.legend(frameon=False, fontsize=9, loc="upper right")
    # (b)
    ax = axs[1]
    for ti, (tool, col, hatch) in enumerate([("Message interface\n(PyTorch)", "#bbbbbb", "//"),
                                             ("Pure C++\n(libtorch)", "#009E73", "")]):
        vals = [r[ti] for r in cyc]
        m, s = st.mean(vals), st.stdev(vals)
        ax.bar(ti, m / 1e3, 0.6, yerr=s / 1e3, color=col, hatch=hatch, edgecolor="#333333", lw=0.6, capsize=3)
        summary.append({"panel": "b", "case": tool.replace("\n", " "), "seeds": len(vals),
                        "mean_cycles": round(m), "sd_across_seeds": round(s)})
    ax.set_xticks([0, 1])
    ax.set_xticklabels(["Message\ninterface", "Pure C++\n(libtorch)"])
    ax.set_ylabel("CPU cycles per decision (x1000)")
    ax.set_title("(b) Decision cost, RL-TCP", fontsize=10.5, loc="left")
    # (c)
    ax = axs[2]
    pat = re.compile(r"^\s*([\d.]+)s\s+gym UE0 servingSINR=\s*(-?[\d.]+) dB\s+bestCand=\s*(-?[\d.]+) dB"
                     r"\s+action=(\d)\S*\s+reward=\s*(-?[\d.]+)")
    t, srv, cand, rew = [], [], [], []
    for line in open(log):
        mt = pat.match(line)
        if mt and float(mt.group(2)) != 0.0:  # the first step has no measurement yet
            t.append(float(mt.group(1)))
            srv.append(float(mt.group(2)))
            cand.append(float(mt.group(3)))
            rew.append(float(mt.group(5)))
    ax.plot(t, srv, "-o", color="#0072B2", ms=4, lw=1.5, label="Serving SINR (measured PHY)")
    ax.plot(t, cand, "--s", color="#E69F00", ms=4, lw=1.3, label="Best candidate (slant-range estimate)")
    ax.set_xlabel("Simulation time (s)")
    ax.set_ylabel("SINR (dB)")
    ax.set_ylim(0, 24)
    ax2 = ax.twinx()
    ax2.bar(t, rew, 0.35, color="#999999", alpha=0.55, label="Reward")
    ax2.set_ylabel("Reward")
    ax2.set_ylim(-10, 30)
    ax.set_zorder(ax2.get_zorder() + 1)
    ax.patch.set_visible(False)
    h1, l1 = ax.get_legend_handles_labels()
    h2, l2 = ax2.get_legend_handles_labels()
    ax.legend(h1 + h2, l1 + l2, frameon=False, fontsize=8.6, loc="lower right")
    ax.set_title("(c) OranNtnGymHandover on a measured radio", fontsize=10.5, loc="left")
    summary.append({"panel": "c", "case": "gym steps with a measurement", "seeds": 1, "mean_cycles": "",
                    "sd_across_seeds": "", "mean_serving_sinr_db": round(st.mean(srv), 2), "steps": len(t)})
    for a in axs:
        a.grid(True, axis="y", color="#d9d9d9", lw=0.6)
        a.spines["top"].set_visible(False)
    axs[0].spines["right"].set_visible(False)
    axs[1].spines["right"].set_visible(False)
    fig.tight_layout()
    for ext in ("png", "svg"):
        fig.savefig(os.path.join(HERE, "ns3_ai_ntn_results." + ext), dpi=200)
    keys = ["panel", "case", "seeds", "mean_cycles", "sd_across_seeds", "mean_serving_sinr_db", "steps"]
    with open(os.path.join(HERE, "ns3_ai_ntn_results_summary.csv"), "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=keys)
        w.writeheader()
        for r in summary:
            w.writerow(r)
    print("wrote ns3_ai_ntn_results.png/.svg and ns3_ai_ntn_results_summary.csv")


if __name__ == "__main__":
    main()
