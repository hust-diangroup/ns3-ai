#!/usr/bin/env python3
"""System design figure for the ns3-ai-ntn module.

Shows the two processes (ns-3 in C++, the agent in Python), the
Boost.Interprocess shared-memory segment between them with the versioned
handshake, the Gym and message interfaces, the AI-RAN inference contract,
and the offline paths (synthetic Python environments, ONNX export).
House style: Deng et al., IEEE OJ-COMS 2026.

    python3 make_ns3_ai_ntn_design.py      # writes ns3_ai_ntn_design.png/.svg here
"""
import os

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

# ---- house palette (Deng et al. 2026, sampled) ---------------------------
SMO, NONRT, NEARRT, RT, NF = "#3a71ae", "#75bd42", "#ee822f", "#3d7199", "#91ace0"
DU_PALE, DU_EDGE, CREAM, PALE = "#b6c7ea", "#8fa9df", "#fef1c7", "#dae3f2"
GREEN_PALE, PANEL, NAVY, INK = "#acd78e", "#808080", "#2e54a1", "#1a1a1a"
L_ORAN, L_NEW, L_FH, FLOW, ATTACH = "#35b44a", "#e54c5e", "#4771c3", "#a6b4cc", "#ee822f"
ROLES = {
    "smo": (SMO, SMO, "white"), "nonrt": (NONRT, NONRT, "white"), "nearrt": (NEARRT, NEARRT, "white"),
    "rt": (RT, RT, "white"), "nf": (NF, NF, INK), "du": (DU_PALE, DU_EDGE, INK),
    "card": (CREAM, "#f3d98a", INK), "pale": (PALE, "#b9c8e3", INK), "green": (GREEN_PALE, "#8cc56a", INK),
}
LINES = {
    "a1": (L_NEW, "-", 1.6, "-|>"), "e2": (L_ORAN, "-", 1.6, "<|-|>"), "fh": (L_FH, "-", 1.8, "<|-|>"),
    "flow": (FLOW, "-", 4.0, "-|>"), "ctrl": (INK, (0, (4, 2)), 1.2, "-|>"), "attach": (ATTACH, "-", 1.4, "<|-|>"),
}


def setup():
    plt.rcParams.update({
        "font.family": "sans-serif",
        "font.sans-serif": ["Arial", "Liberation Sans", "Nimbus Sans", "DejaVu Sans"],
        "font.weight": "bold", "svg.fonttype": "none", "pdf.fonttype": 42,
    })


def box(ax, x, y, w, h, title, role, sub=None, dashed=False, fs=10.0, sfs=8.2):
    fc, ec, tc = ROLES[role]
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0,rounding_size=%g" % (0.10 * min(w, h)),
                                fc=fc, ec=NAVY if dashed else ec, lw=1.4 if dashed else 0.9,
                                ls=(0, (4, 2)) if dashed else "-", zorder=5))
    ty = y + h - 0.24 if sub else y + h / 2
    ax.text(x + w / 2, ty, title, ha="center", va="center", color=tc, fontsize=fs, weight="bold", zorder=6)
    if sub:
        ax.text(x + w / 2, (y + ty - 0.16) / 2, sub, ha="center", va="center", color=tc, fontsize=sfs,
                weight="normal", zorder=6, linespacing=1.25)


def panel(ax, x, y, w, h, title):
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="square,pad=0", fc="white", ec=PANEL, lw=0.9, zorder=1))
    ax.text(x + 0.10, y + h - 0.08, title, ha="left", va="top", fontsize=10.5, weight="bold", color=INK, zorder=2)


def route(ax, pts, kind, label=None, at=0, dx=0.06, dy=0.0, ha="left", va="center", fs=8.6):
    col, ls, lw, arr = LINES[kind]
    for a, b in zip(pts[:-2], pts[1:-1]):
        ax.plot([a[0], b[0]], [a[1], b[1]], color=col, lw=lw, ls=ls, zorder=3, solid_capstyle="butt")
    ax.add_patch(FancyArrowPatch(pts[-2], pts[-1], arrowstyle=arr, mutation_scale=11, color=col, lw=lw, ls=ls,
                                 shrinkA=0, shrinkB=0, zorder=3))
    if label:
        a, b = pts[at], pts[at + 1]
        ax.text((a[0] + b[0]) / 2 + dx, (a[1] + b[1]) / 2 + dy, label, fontsize=fs, weight="bold", color=INK,
                ha=ha, va=va, zorder=7)


def legend(ax, y, items):
    x = 0.25
    for kind, text, width in items:
        col, ls, lw, _ = LINES[kind]
        ax.plot([x, x + 0.42], [y, y], color=col, lw=lw + (0.3 if kind != "flow" else 0), ls=ls,
                solid_capstyle="butt")
        ax.text(x + 0.52, y, text, fontsize=9, weight="bold", va="center", ha="left", color=INK)
        x += 0.52 + width


def main():
    setup()
    W, H = 12.0, 6.9
    fig = plt.figure(figsize=(W, H))
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_xlim(0, W)
    ax.set_ylim(0, H)
    ax.axis("off")
    legend(ax, 6.65, [("fh", "Shared-memory exchange", 2.05), ("e2", "Gymnasium step API", 1.75),
                      ("flow", "Inference request / reply", 2.10), ("ctrl", "Control / offline path", 1.9)])

    ty = [5.00, 3.80, 2.90, 2.00]          # row bottoms
    th = [0.85, 0.95, 0.65, 0.72]          # row heights
    panel(ax, 0.15, 1.85, 3.75, 4.55, "ns-3 process (C++)")
    panel(ax, 4.15, 1.85, 3.70, 4.55, "Bridge between the processes")
    panel(ax, 8.10, 1.85, 3.75, 4.55, "Agent side (Python)")
    xl, xm, xr, bw = 0.35, 4.35, 8.30, 3.35

    box(ax, xl, ty[0], bw, th[0], "Scenario", "nf", "e.g. OranNtnGymHandover, rl-tcp", fs=9.6)
    box(ax, xl, ty[1], bw, th[1], "OpenGymInterface", "rt", "Notify() each step; transition\nlatched in GetObservation", fs=9.6)
    box(ax, xl, ty[2], bw, th[2], "Ns3AiMsgInterface", "rt", fs=9.6)
    box(ax, xl, ty[3], bw, th[3], "AiranInferenceClient", "du", fs=9.6)

    box(ax, xm, ty[0], bw, th[0], "Handshake", "card", "protocol + module version,\nFNV-1a schema digest", fs=9.6)
    box(ax, xm, ty[1], bw, th[1], "Shared-memory segment", "pale", "Boost.Interprocess: protobuf obs,\naction, reward; semaphores; RAII", fs=9.6)
    box(ax, xm, ty[2], bw, th[2], "Struct / vector buffers", "pale", fs=9.6)
    box(ax, xm, ty[3], bw, th[3], "InferenceChannel", "card", "in-process FIFO or length-prefixed TCP", fs=9.6, sfs=7.8)

    box(ax, xr, ty[0], bw, th[0], "Agent", "smo", "SB3 PPO / SAC, N independent\nenv copies (no central critic)", fs=9.6)
    box(ax, xr, ty[1], bw, th[1], "Gymnasium 1.0 Env", "nonrt", "ns3ai_gym_env: reset(), step()\nmismatch is an error, not data", fs=9.6)
    box(ax, xr, ty[2], bw, th[2], "pybind11 message module", "green", fs=9.6)
    box(ax, xr, ty[3], bw, th[3], "AiranInferenceServer (C++)", "du", "Triton config parser, mock runtimes", fs=9.6, sfs=7.8)

    def mid(i):
        return ty[i] + th[i] / 2

    route(ax, [(2.02, ty[0]), (2.02, ty[1] + th[1])], "ctrl", "step", dx=0.08)
    route(ax, [(xl + bw, mid(1)), (xm, mid(1))], "fh")
    route(ax, [(xm + bw, mid(1)), (xr, mid(1))], "fh")
    route(ax, [(xl + bw, mid(2)), (xm, mid(2))], "fh")
    route(ax, [(xm + bw, mid(2)), (xr, mid(2))], "fh")
    route(ax, [(xr + bw / 2, ty[1] + th[1]), (xr + bw / 2, ty[0])], "e2", "action / obs", dx=0.08)
    route(ax, [(xm + bw / 2, ty[0]), (xm + bw / 2, ty[1] + th[1])], "ctrl", "checked first", dx=0.08)
    route(ax, [(xl + bw, mid(3)), (xm, mid(3))], "flow")
    route(ax, [(xm + bw, mid(3)), (xr, mid(3))], "flow")
    panel(ax, 0.15, 0.25, 11.70, 1.35, "Offline and optional paths")
    box(ax, 0.35, 0.40, 5.55, 0.80, "python_utils/ns3_ai_ntn", "pale",
        "HandoverEnv, BeamMgmtEnv, SliceEnv, PowerCtrlEnv: synthetic, do not boot ns-3", dashed=True, fs=9.6)
    box(ax, 6.10, 0.40, 5.55, 0.80, "ONNX export to oran-ntn", "pale",
        "OranNtnOnnxXapp infers if ONNX Runtime is present; no trained model ships", dashed=True, fs=9.6)

    fig.text(0.995, 0.006, "Muhammad Uzair, Independent Researcher", ha="right", va="bottom", fontsize=7,
             weight="normal", color="#666666")
    here = os.path.dirname(os.path.abspath(__file__))
    for ext in ("png", "svg"):
        fig.savefig(os.path.join(here, "ns3_ai_ntn_design." + ext), dpi=200)
    print("wrote ns3_ai_ntn_design.png/.svg")


if __name__ == "__main__":
    main()
