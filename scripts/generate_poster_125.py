#!/usr/bin/env python3
"""
generate_poster.py — 1-page landscape research poster for UrbanDrop Mini 1A
Outputs: results/poster.pdf  (and poster.png for preview)
"""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch
import numpy as np
from pathlib import Path
import glob, pandas as pd, sys
import textwrap
import re
import math

# Global text scale for this variant.
FONT_SCALE = 1.65

# Findings column text flow tuning so spacing scales with FONT_SCALE.
FINDINGS_LINE_SPACING = 1.35
FINDINGS_Y_SCALE = 680.0


def fs(size: float) -> float:
    return size * FONT_SCALE


# Reflow findings body text so it better uses card width at different scales.
def reflow_findings_text(text: str, size: float, family: str | None, bold: bool) -> str:
    if family == "monospace" or bold:
        return text
    flat = " ".join(part.strip() for part in text.splitlines())
    # Wider wraps for smaller body text; slightly tighter for larger text.
    target = max(46, int(66 - (size - 7.0) * 3.5))
    return textwrap.fill(flat, width=target)


# ── Colors ──────────────────────────────────────────────────────────────────
C_DARK   = "#14171f"
C_RED    = "#e85d2f"
C_GREEN  = "#22c55e"
C_PURPLE = "#8b5cf6"
C_LPURP  = "#a78bfa"
C_DPURP  = "#6d28d9"
C_BG     = "#faf8f4"
C_CARD   = "#ffffff"
C_MUTED  = "#78716c"
C_GRID   = "#e8e3da"
C_TEXT   = "#1c1917"

QUERY_LABELS = {
    "speed_below":         "W1 Speed\nThreshold",
    "borough_speed_below": "W2 Borough\n+ Speed",
    "time_window":         "W3 Time\nWindow",
    "summary":             "W5 Summary",
}


def query_label(query_type: str) -> str:
    return QUERY_LABELS.get(query_type, query_type.replace("_", " ").title())


def mean_or_nan(series: pd.Series) -> float:
    if series is None:
        return float("nan")
    values = pd.to_numeric(series, errors="coerce").dropna()
    if values.empty:
        return float("nan")
    return float(values.mean())


def metric_mean(df: pd.DataFrame,
                execution_mode: str | None = None,
                query_type: str | None = None,
                thread_count: int | None = None,
                column: str = "query_ms") -> float:
    if df.empty or column not in df.columns:
        return float("nan")
    sub = df
    if execution_mode is not None and "execution_mode" in sub.columns:
        sub = sub[sub["execution_mode"] == execution_mode]
    if query_type is not None and "query_type" in sub.columns:
        sub = sub[sub["query_type"] == query_type]
    if thread_count is not None and "thread_count" in sub.columns:
        sub = sub[sub["thread_count"] == thread_count]
    if sub.empty:
        return float("nan")
    return mean_or_nan(sub[column])


def compute_key_metrics(p1: pd.DataFrame, p2: pd.DataFrame, p3: pd.DataFrame) -> dict:
    metrics: dict[str, object] = {}

    # Ingest: prefer explicit ingest_only rows; fallback to mode-level ingest mean.
    serial_ingest_ms = metric_mean(p1, execution_mode="serial", query_type="ingest_only", column="ingest_ms")
    if math.isnan(serial_ingest_ms):
        serial_ingest_ms = metric_mean(p1, execution_mode="serial", column="ingest_ms")

    parallel_ingest_8_ms = metric_mean(
        p2, execution_mode="parallel", query_type="ingest_only", thread_count=8, column="ingest_ms"
    )
    if math.isnan(parallel_ingest_8_ms):
        parallel_ingest_8_ms = metric_mean(p2, execution_mode="parallel", thread_count=8, column="ingest_ms")
    if math.isnan(parallel_ingest_8_ms):
        parallel_ingest_8_ms = metric_mean(p2, execution_mode="parallel", column="ingest_ms")

    ingest_speedup_8 = (
        serial_ingest_ms / parallel_ingest_8_ms
        if serial_ingest_ms > 0 and parallel_ingest_8_ms > 0
        else float("nan")
    )

    # Query speedups.
    p1_serial = p1[(p1["execution_mode"] == "serial") & (p1["query_type"] != "ingest_only")].copy()
    p2_parallel = p2[(p2["execution_mode"] == "parallel") & (p2["query_type"] != "ingest_only")].copy()
    p3_opt_serial = p3[
        (p3["execution_mode"].isin(["optimized_serial", "optimized"])) &
        (p3["thread_count"] == 1) &
        (p3["query_type"] != "ingest_only")
    ].copy()
    p3_opt_8 = p3[
        (p3["execution_mode"].isin(["optimized_parallel", "optimized"])) &
        (p3["thread_count"] == 8) &
        (p3["query_type"] != "ingest_only")
    ].copy()

    layout_speedups: list[float] = []
    combined_speedups: list[tuple[str, float]] = []
    p2_best_per_query: list[float] = []

    query_set = sorted(set(p1_serial["query_type"]))
    for q in query_set:
        p1_q = mean_or_nan(p1_serial[p1_serial["query_type"] == q]["query_ms"])

        p3s_q = mean_or_nan(p3_opt_serial[p3_opt_serial["query_type"] == q]["query_ms"])
        if p1_q > 0 and p3s_q > 0:
            layout_speedups.append(p1_q / p3s_q)

        p3p_q = mean_or_nan(p3_opt_8[p3_opt_8["query_type"] == q]["query_ms"])
        if p1_q > 0 and p3p_q > 0:
            combined_speedups.append((q, p1_q / p3p_q))

        p2_q = p2_parallel[p2_parallel["query_type"] == q]
        if not p2_q.empty and p1_q > 0:
            best_q = float("nan")
            for tc in sorted(p2_q["thread_count"].dropna().astype(int).unique()):
                p2_tc_q = mean_or_nan(p2_q[p2_q["thread_count"] == tc]["query_ms"])
                if p2_tc_q > 0:
                    speed = p1_q / p2_tc_q
                    if math.isnan(best_q) or speed > best_q:
                        best_q = speed
            if not math.isnan(best_q):
                p2_best_per_query.append(best_q)

    best_query = "n/a"
    best_combined = float("nan")
    if combined_speedups:
        best_query, best_combined = max(combined_speedups, key=lambda item: item[1])

    # Memory reduction from peak RSS when available.
    p1_rss = metric_mean(p1, execution_mode="serial", query_type="summary", column="peak_rss_kb")
    if math.isnan(p1_rss):
        p1_rss = metric_mean(p1, execution_mode="serial", column="peak_rss_kb")
    p3_rss = metric_mean(p3, execution_mode="optimized_parallel", query_type="summary", thread_count=8, column="peak_rss_kb")
    if math.isnan(p3_rss):
        p3_rss = metric_mean(p3, execution_mode="optimized_parallel", thread_count=8, column="peak_rss_kb")
    mem_reduction_pct = (
        (p1_rss - p3_rss) / p1_rss * 100.0
        if p1_rss > 0 and p3_rss > 0
        else float("nan")
    )

    rows_accepted = metric_mean(p1, execution_mode="serial", query_type="summary", column="rows_accepted")
    if math.isnan(rows_accepted):
        rows_accepted = metric_mean(p1, execution_mode="serial", column="rows_accepted")

    metrics["serial_ingest_ms"] = serial_ingest_ms
    metrics["parallel_ingest_8_ms"] = parallel_ingest_8_ms
    metrics["ingest_speedup_8"] = ingest_speedup_8
    metrics["layout_range"] = (min(layout_speedups), max(layout_speedups)) if layout_speedups else (float("nan"), float("nan"))
    metrics["combined_range"] = (
        min([s for _, s in combined_speedups]),
        max([s for _, s in combined_speedups]),
    ) if combined_speedups else (float("nan"), float("nan"))
    metrics["p2_best_range"] = (min(p2_best_per_query), max(p2_best_per_query)) if p2_best_per_query else (float("nan"), float("nan"))
    metrics["best_query"] = best_query
    metrics["best_combined"] = best_combined
    metrics["memory_reduction_pct"] = mem_reduction_pct
    metrics["rows_accepted"] = rows_accepted
    return metrics


def fmt_secs(ms: float) -> str:
    return "n/a" if math.isnan(ms) else f"{ms / 1000.0:.1f} s"


def fmt_x(value: float) -> str:
    return "n/a" if math.isnan(value) else f"{value:.1f}×"


def fmt_pct(value: float) -> str:
    return "n/a" if math.isnan(value) else f"{value:.0f}%"


def fmt_range_x(bounds: tuple[float, float]) -> str:
    lo, hi = bounds
    if math.isnan(lo) or math.isnan(hi):
        return "n/a"
    return f"{lo:.1f}× – {hi:.1f}×"

# ── Data loading ─────────────────────────────────────────────────────────────
def load_dir(raw: Path, d: str) -> pd.DataFrame:
    csv_paths = sorted(glob.glob(str(raw / d / "*.csv")))
    # Ignore metadata/manifest CSVs; keep only benchmark row CSVs.
    data_paths = []
    for f in csv_paths:
        p = Path(f)
        if p.stem.startswith("batch_") or p.stem.startswith("subset_"):
            continue
        data_paths.append(f)

    # Use only the newest timestamped batch present in this phase directory.
    # This avoids mixing old and new full runs in one poster.
    ts_re = re.compile(r"(\d{8}T\d{6}Z)")
    by_ts = {}
    no_ts = []
    for f in data_paths:
        m = ts_re.search(Path(f).name)
        if m:
            by_ts.setdefault(m.group(1), []).append(f)
        else:
            no_ts.append(f)
    if by_ts:
        latest_ts = max(by_ts.keys())
        candidate_paths = by_ts[latest_ts] + no_ts
    else:
        candidate_paths = data_paths

    frames = []
    for f in candidate_paths:
        try:
            df = pd.read_csv(f)
            if "execution_mode" in df.columns and "dataset_label" in df.columns:
                frames.append(df)
        except Exception:
            pass
    if not frames:
        return pd.DataFrame()
    df = pd.concat(frames, ignore_index=True)
    for c in ["query_ms", "ingest_ms", "thread_count", "rows_accepted"]:
        df[c] = pd.to_numeric(df.get(c), errors="coerce")
    df["thread_count"] = df["thread_count"].fillna(1).astype(int)

    # Full-run convention has historically used "large_dev" as the full dataset label.
    # Keep that preference, but fall back if a different label naming was used.
    for label in ("large_dev", "full", "full_dataset", "46M rows"):
        filtered = df[df["dataset_label"] == label].copy()
        if not filtered.empty:
            return filtered
    return df.copy()


# ── Style helpers ─────────────────────────────────────────────────────────────
def sax(ax, title="", xlabel="", ylabel="", title_size=7.5):
    ax.set_facecolor(C_CARD)
    ax.spines[["top", "right"]].set_visible(False)
    ax.spines[["left", "bottom"]].set_color(C_GRID)
    ax.tick_params(colors=C_MUTED, labelsize=fs(6.5))
    ax.yaxis.label.set_color(C_MUTED)
    ax.xaxis.label.set_color(C_MUTED)
    ax.grid(axis="y", color=C_GRID, linewidth=0.6, zorder=0)
    ax.set_axisbelow(True)
    if title:
        ax.set_title(title, fontsize=fs(title_size), color=C_TEXT,
                     pad=5, fontweight="bold")
    if xlabel:
        ax.set_xlabel(xlabel, fontsize=fs(6.5), color=C_MUTED)
    if ylabel:
        ax.set_ylabel(ylabel, fontsize=fs(6.5), color=C_MUTED)


def bar_val(ax, bars, fmt="{:.1f}", offset_frac=0.04, fontsize=5.5, rotation=0):
    ymax = max((b.get_height() for b in bars if b.get_height() > 0), default=1)
    for bar in bars:
        h = bar.get_height()
        if h > 0:
            ax.text(bar.get_x() + bar.get_width() / 2,
                    h + ymax * offset_frac,
                    fmt.format(h),
                    ha="center", va="bottom", fontsize=fs(fontsize),
                    color=C_TEXT, rotation=rotation)


# ── Chart 1: Ingest speedup ───────────────────────────────────────────────────
def chart_ingest(ax, p2):
    threads = [1, 2, 4, 8, 16]
    times = []
    serial_t = p2[p2["execution_mode"] == "serial"]["ingest_ms"].mean()
    for tc in threads:
        filt = (p2["execution_mode"] == "parallel") & (p2["thread_count"] == tc)
        v = p2[filt]["ingest_ms"]
        times.append(v.mean() if len(v) else float("nan"))
    times[0] = serial_t   # 1T = serial

    speedups = [serial_t / t for t in times]
    ideal    = [float(tc) for tc in threads]

    color_bars = [C_RED if tc == 1 else C_GREEN for tc in threads]
    bars = ax.bar(range(len(threads)), speedups, color=color_bars,
                  alpha=0.88, zorder=3, width=0.55)
    ax.plot(range(len(threads)), ideal, color=C_MUTED, linewidth=1,
            linestyle="--", zorder=5, label="Ideal linear")
    bar_val(ax, bars, fmt="{:.1f}×")
    ax.set_xticks(range(len(threads)))
    ax.set_xticklabels([f"{t}T" for t in threads], fontsize=fs(6.5))
    sax(ax, title="Ingest Speedup vs Serial (46M rows)",
        xlabel="Threads", ylabel="Speedup (×)")
    ax.legend(fontsize=fs(5.5), framealpha=0.5)


# ── Chart 2: Query speedup Phase 2 vs Phase 1 ────────────────────────────────
def chart_p2_speedup(ax, p1q, p2q):
    queries = ["speed_below", "borough_speed_below", "time_window", "summary"]
    thread_sets = [2, 4, 8]
    x = np.arange(len(queries))
    width = 0.22
    shades = ["#86efac", "#22c55e", "#15803d"]

    for i, tc in enumerate(thread_sets):
        speedups = []
        for qt in queries:
            s = p1q[p1q["query_type"] == qt]["query_ms"].mean()
            p = p2q[(p2q["thread_count"] == tc) & (p2q["query_type"] == qt)]["query_ms"].mean()
            speedups.append(s / p if s > 0 and p > 0 else 0)
        offset = (i - 1) * width
        bars = ax.bar(x + offset, speedups, width * 0.9,
                      label=f"{tc}T", color=shades[i], alpha=0.9, zorder=3)
        for bar, s in zip(bars, speedups):
            if s > 0.05:
                ax.text(bar.get_x() + bar.get_width() / 2,
                        bar.get_height() + 0.03,
                        f"{s:.1f}×", ha="center", va="bottom",
                        fontsize=fs(5), color=C_TEXT)

    ax.axhline(1.0, color=C_RED, linewidth=1.2, linestyle="--",
               zorder=5, label="Phase 1 baseline")
    ax.set_xticks(x)
    ax.set_xticklabels([QUERY_LABELS[q] for q in queries], fontsize=fs(6))
    sax(ax, title="Phase 2 — Query Speedup vs Phase 1 Serial (AoS)",
        ylabel="Speedup (×)")
    ax.legend(fontsize=fs(5.5), framealpha=0.5, ncol=4)


# ── Chart 3: SoA layout gain ─────────────────────────────────────────────────
def chart_soa_gain(ax, p1q, p3q):
    queries = ["speed_below", "borough_speed_below", "time_window", "summary"]
    labels  = [QUERY_LABELS[q] for q in queries]
    x = np.arange(len(queries))
    width = 0.22

    configs = [
        ("AoS 1T\n(Phase 1)", "serial",  None,  1, C_RED,   p1q),
        ("SoA 1T",            "opt_ser",  None,  1, C_LPURP, p3q),
        ("SoA 8T",            "opt_par",  None,  8, C_DPURP, p3q),
    ]

    for i, (label, mode_key, _, tc, color, src) in enumerate(configs):
        means = []
        for qt in queries:
            if src is p1q:
                v = src[(src["execution_mode"] == "serial") &
                        (src["query_type"] == qt)]["query_ms"]
            else:
                v = src[(src["execution_mode"].isin(
                            ["optimized_serial", "optimized_parallel", "optimized"])) &
                        (src["thread_count"] == tc) &
                        (src["query_type"] == qt)]["query_ms"]
            means.append(v.mean() if len(v) else 0)

        offset = (i - 1) * width
        bars = ax.bar(x + offset, means, width * 0.9,
                      label=label, color=color, alpha=0.9, zorder=3)
        bar_val(ax, bars, fmt="{:.0f}", fontsize=5, offset_frac=0.02)

    ax.set_xticks(x)
    ax.set_xticklabels(labels, fontsize=fs(6))
    sax(ax, title="Phase 3 — Query Latency: AoS vs SoA (46M rows)",
        ylabel="Query time (ms)")
    ax.legend(fontsize=fs(5.5), framealpha=0.5, ncol=3)


# ── Chart 4: Total speedup bars ───────────────────────────────────────────────
def chart_total_speedup(ax, p1q, p3q):
    queries = ["speed_below", "borough_speed_below", "time_window", "summary"]
    speedups_p2 = []
    speedups_p3 = []

    for qt in queries:
        s = p1q[(p1q["execution_mode"] == "serial") &
                (p1q["query_type"] == qt)]["query_ms"].mean()
        # P3 at 8T
        p3 = p3q[(p3q["execution_mode"].isin(
                    ["optimized_parallel", "optimized"])) &
                 (p3q["thread_count"] == 8) &
                 (p3q["query_type"] == qt)]["query_ms"].mean()
        speedups_p3.append(s / p3 if s > 0 and p3 > 0 else 0)

    x = np.arange(len(queries))
    width = 0.45
    bars = ax.bar(x, speedups_p3, width,
                  color=[C_DPURP, C_PURPLE, C_LPURP, C_GREEN],
                  alpha=0.9, zorder=3)
    for bar, s in zip(bars, speedups_p3):
                ax.text(bar.get_x() + bar.get_width() / 2,
                        bar.get_height() + 0.2,
                        f"{s:.1f}×", ha="center", va="bottom",
                        fontsize=fs(8), color=C_TEXT, fontweight="bold")

    ax.axhline(1.0, color=C_RED, linewidth=1.2, linestyle="--",
               zorder=5)
    ax.set_xticks(x)
    ax.set_xticklabels([QUERY_LABELS[q] for q in queries], fontsize=fs(7))
    sax(ax, title="Total Speedup: Phase 3 (SoA + 8T) vs Phase 1 Serial",
        ylabel="Speedup vs Phase 1 (×)", title_size=8)


# ── KPI box ───────────────────────────────────────────────────────────────────
def draw_kpi(fig, rect, val, label, color=C_RED):
    """Draw a simple KPI card at normalized fig coords rect=(x,y,w,h)."""
    ax = fig.add_axes(rect)
    ax.set_xlim(0, 1); ax.set_ylim(0, 1)
    ax.axis("off")
    ax.add_patch(FancyBboxPatch((0, 0), 1, 1,
                                boxstyle="round,pad=0.05",
                                facecolor=C_CARD, edgecolor=C_GRID,
                                linewidth=0.6, zorder=1))
    ax.text(0.5, 0.62, val,  ha="center", va="center",
            fontsize=fs(14), fontweight="bold", color=color, zorder=2)
    ax.text(0.5, 0.22, label, ha="center", va="center",
            fontsize=fs(6), color=C_MUTED, zorder=2)


# ── Main poster layout ────────────────────────────────────────────────────────
def make_poster(raw_dir: Path, out_path: Path):
    p1 = load_dir(raw_dir, "phase1_full")
    p2 = load_dir(raw_dir, "phase2_full")
    p3 = load_dir(raw_dir, "phase3_full")

    if p1.empty or p2.empty or p3.empty:
        print("ERROR: missing phase data", file=sys.stderr)
        sys.exit(1)

    p1q = p1[p1["query_type"] != "ingest_only"].copy()
    p2q = p2[p2["query_type"] != "ingest_only"].copy()
    p3q = p3[p3["query_type"] != "ingest_only"].copy()
    metrics = compute_key_metrics(p1, p2, p3)

    # ── Canvas ────────────────────────────────────────────────────────────────
    fig = plt.figure(figsize=(22, 13), facecolor=C_BG)

    # ── Header banner ─────────────────────────────────────────────────────────
    header = fig.add_axes([0, 0.905, 1, 0.095])
    header.set_xlim(0, 1); header.set_ylim(0, 1)
    header.axis("off")
    header.add_patch(plt.Rectangle((0, 0), 1, 1, facecolor=C_DARK, zorder=0))
    header.text(0.012, 0.60,
                "Memory Overload: Data Layout & Parallel Processing in Large-Scale Traffic Analysis",
                ha="left", va="center", fontsize=fs(14), fontweight="bold",
                color="white", zorder=1)
    header.text(0.012, 0.18,
                "NYC DOT Real-Time Traffic Speed Feed  ·  46.2M records  ·  "
                "C++17 + OpenMP  ·  3-Phase AoS → SoA Benchmark  ·  CMPE 275 Mini 1A",
                ha="left", va="center", fontsize=fs(8), color="#94a3b8", zorder=1)
    header.add_patch(plt.Rectangle((0.88, 0.1), 0.11, 0.8,
                                   facecolor=C_RED, zorder=1, alpha=0.9))
    header.text(0.935, 0.55, "System 1",
                ha="center", va="center", fontsize=fs(9),
                fontweight="bold", color="white", zorder=2)

    # ── KPI strip ─────────────────────────────────────────────────────────────
    kpis = [
        (f"{int(metrics['rows_accepted'])/1_000_000:.1f}M" if not math.isnan(metrics["rows_accepted"]) else "n/a",
         "Records\nProcessed", C_RED),
        (fmt_secs(metrics["serial_ingest_ms"]), "Serial Ingest\n(Phase 1)", C_MUTED),
        (fmt_secs(metrics["parallel_ingest_8_ms"]), "Parallel Ingest\n(8T)", C_GREEN),
        (fmt_x(metrics["ingest_speedup_8"]), "Ingest\nSpeedup", C_GREEN),
        (fmt_x(metrics["best_combined"]), f"Best Query\nSpeedup ({query_label(str(metrics['best_query']))})", C_DPURP),
        (fmt_pct(metrics["memory_reduction_pct"]), "Memory\nReduction (SoA)", C_PURPLE),
    ]
    kpi_w, kpi_h = 0.138, 0.077
    kpi_y = 0.815
    kpi_gap = 0.004
    kpi_start = 0.012
    for i, (val, label, color) in enumerate(kpis):
        x = kpi_start + i * (kpi_w + kpi_gap)
        draw_kpi(fig, [x, kpi_y, kpi_w, kpi_h], val, label, color)

    # ── Chart grid ────────────────────────────────────────────────────────────
    # Two rows of two charts, plus left text column
    gs = gridspec.GridSpec(
        2, 3,
        left=0.012, right=0.99,
        top=0.785, bottom=0.085,
        wspace=0.16, hspace=0.20,
        width_ratios=[1.0, 1.0, 1.0],
    )

    ax_ingest  = fig.add_subplot(gs[0, 0])
    ax_p2_spd  = fig.add_subplot(gs[0, 1])
    ax_soa     = fig.add_subplot(gs[1, 0])
    ax_total   = fig.add_subplot(gs[1, 1])

    chart_ingest(ax_ingest, p2)
    chart_p2_speedup(ax_p2_spd, p1q, p2q)
    chart_soa_gain(ax_soa, p1q, p3q)
    chart_total_speedup(ax_total, p1q, p3q)

    # ── Right column: findings text ───────────────────────────────────────────
    ax_text = fig.add_subplot(gs[:, 2])
    ax_text.axis("off")
    ax_text.set_facecolor(C_CARD)

    # Draw a card background
    ax_text.add_patch(FancyBboxPatch((0, 0), 1, 1,
                                    boxstyle="round,pad=0.02",
                                    facecolor=C_CARD, edgecolor=C_GRID,
                                    linewidth=0.8, transform=ax_text.transAxes,
                                    zorder=0, clip_on=False))

    findings = [
        ("KEY FINDINGS", None, 10, True, C_RED),
        ("", None, 4, False, C_TEXT),

        ("① Ingest Parallelism Critical at Scale", None, 9, True, C_DARK),
        (f"Serial CSV parsing dominates total runtime at full scale: "
         f"{fmt_secs(metrics['serial_ingest_ms'])} ingest time versus sub-second queries. "
         f"Parallel ingest at 8 threads reduces this to {fmt_secs(metrics['parallel_ingest_8_ms'])}, "
         f"which is a {fmt_x(metrics['ingest_speedup_8'])} speedup.", None, 8.5, False, C_MUTED),
        ("", None, 4, False, C_TEXT),

        ("② SoA Layout Dominates Query Performance", None, 9, True, C_DARK),
        (f"AoS scans full records, while SoA streams only required columns. "
         f"From the current raw batch, layout-only speedup (Phase 3 serial vs Phase 1 serial) is "
         f"{fmt_range_x(metrics['layout_range'])}.", None, 8.5, False, C_MUTED),
        ("", None, 4, False, C_TEXT),

        (f"③ Combined: Up to {fmt_x(metrics['best_combined'])} Total Speedup", None, 9, True, C_DARK),
        (f"SoA + 8-thread parallelism achieves {fmt_range_x(metrics['combined_range'])} over the serial AoS baseline. "
         f"Best query in this run is {query_label(str(metrics['best_query']))} at {fmt_x(metrics['best_combined'])}.",
         None, 8.5, False, C_MUTED),
        ("", None, 4, False, C_TEXT),

        ("④ Phase Comparison Summary", None, 9, True, C_DARK),
        (
            "Phase 1  Serial AoS        1.0×  baseline\n"
            f"Phase 2  Parallel AoS      {fmt_range_x(metrics['p2_best_range'])}  query\n"
            f"          Parallel ingest   {fmt_x(metrics['ingest_speedup_8'])}\n"
            f"Phase 3  SoA serial        {fmt_range_x(metrics['layout_range'])}  layout\n"
            f"          SoA + 8T          {fmt_range_x(metrics['combined_range'])}  total",
            "monospace", 6.8, False, C_DARK
        ),
        ("", None, 4, False, C_TEXT),

        ("Dataset", None, 8, True, C_DARK),
        ("NYC DOT Real-Time Traffic Speed Feed\n"
         "46,264,669 records  ·  19 GB raw CSV\n"
         "16-core Intel i9-12900  ·  94 GB RAM\n"
         "GCC -O2 -fopenmp  ·  Linux 6.8", None, 8.5, False, C_MUTED),
    ]

    y = 0.97
    for (text, family, size, bold, color) in findings:
        if text == "":
            y -= fs(size) / 260
            continue
        render_text = reflow_findings_text(text, size, family, bold)
        weight = "bold" if bold else "normal"
        ff = family if family else "sans-serif"
        ax_text.text(0.03, y, render_text,
                     transform=ax_text.transAxes,
                     ha="left", va="top",
                     fontsize=fs(size), fontweight=weight,
                     color=color, fontfamily=ff,
                     linespacing=FINDINGS_LINE_SPACING)
        # rough line height estimate
        n_lines = render_text.count("\n") + 1
        y -= (fs(size) * n_lines * FINDINGS_LINE_SPACING + fs(2)) / FINDINGS_Y_SCALE

    # ── Footer ────────────────────────────────────────────────────────────────
    footer = fig.add_axes([0, 0, 1, 0.055])
    footer.set_xlim(0, 1); footer.set_ylim(0, 1)
    footer.axis("off")
    footer.add_patch(plt.Rectangle((0, 0), 1, 1,
                                   facecolor="#f4f0e8", zorder=0))
    footer.text(
        0.012, 0.5,
        "Phase 1 — Serial AoS (run_serial)   ·   "
        "Phase 2 — OpenMP Parallel AoS (run_parallel)  ·  parallel mmap ingestion   ·   "
        "Phase 3 — SoA + OpenMP (run_optimized)   ·   "
        "5 runs/scenario  ·  threads: 1,2,4,8,16   ·   dataset: i4gi-tjb9",
        ha="left", va="center", fontsize=fs(6.5), color=C_MUTED)

    # ── Save ──────────────────────────────────────────────────────────────────
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(str(out_path).replace(".pdf", ".png"),
                dpi=150, bbox_inches="tight", facecolor=C_BG)
    fig.savefig(str(out_path),
                dpi=200, bbox_inches="tight", facecolor=C_BG)
    plt.close(fig)
    print(f"[poster] wrote {out_path}")
    print(f"[poster] wrote {str(out_path).replace('.pdf','.png')}")


if __name__ == "__main__":
    repo = Path(__file__).parent.parent
    make_poster(
        raw_dir=repo / "results" / "raw",
        out_path=repo / "results" / "poster.pdf",
    )
