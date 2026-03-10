#!/usr/bin/env python3
"""
generate_report.py — UrbanDrop Mini 1A  HTML dashboard

Reads all raw benchmark CSVs from results/raw/phase{1,2,3}_dev/ and
produces a single self-contained results/report.html.

Usage:
    python3 scripts/generate_report.py
    python3 scripts/generate_report.py --raw-dir results/raw --out results/report.html
"""

import argparse
import base64
import datetime
import glob
import io
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import pandas as pd

# ── Palette ───────────────────────────────────────────────────────────────────
MODE_COLORS = {
    "serial":             "#e85d2f",
    "parallel":           "#22c55e",
    "optimized":          "#8b5cf6",
    "optimized_serial":   "#a78bfa",
    "optimized_parallel": "#6d28d9",
}
THREAD_SHADES = ["#86efac","#4ade80","#22c55e","#15803d","#052e16"]
DEFAULT_COLOR = "#64748b"

DATASET_ORDER  = ["small", "medium", "large_dev"]
DATASET_LABELS = {"small": "10K rows", "medium": "100K rows", "large_dev": "1M rows"}
QUERY_LABELS   = {
    "ingest_only":           "Ingest Only",
    "speed_below":           "Speed Below",
    "time_window":           "Time Window",
    "borough_speed_below":   "Borough Speed",
    "top_n_slowest":         "Top-N Slowest",
    "summary":               "Summary",
}
MODE_LABELS = {
    "serial":             "Phase 1 — Serial",
    "parallel":           "Phase 2 — Parallel",
    "optimized":          "Phase 3 — Optimized",
    "optimized_serial":   "Phase 3 — Opt Serial",
    "optimized_parallel": "Phase 3 — Opt Parallel",
}

BG         = "#faf8f4"
CARD_BG    = "#ffffff"
GRID_COLOR = "#e8e3da"
TEXT_COLOR = "#1c1917"
MUTED      = "#78716c"


# ── Helpers ───────────────────────────────────────────────────────────────────
def fig_to_b64(fig) -> str:
    buf = io.BytesIO()
    fig.savefig(buf, format="png", dpi=150, bbox_inches="tight",
                facecolor=fig.get_facecolor())
    buf.seek(0)
    return base64.b64encode(buf.read()).decode()


def style_ax(ax, title="", xlabel="", ylabel=""):
    ax.set_facecolor(CARD_BG)
    ax.spines[["top","right"]].set_visible(False)
    ax.spines[["left","bottom"]].set_color(GRID_COLOR)
    ax.tick_params(colors=MUTED, labelsize=8)
    ax.yaxis.label.set_color(MUTED)
    ax.xaxis.label.set_color(MUTED)
    ax.grid(axis="y", color=GRID_COLOR, linewidth=0.8, zorder=0)
    ax.set_axisbelow(True)
    if title:
        ax.set_title(title, fontsize=9, color=TEXT_COLOR, pad=8, fontweight="semibold")
    if xlabel: ax.set_xlabel(xlabel, fontsize=8)
    if ylabel: ax.set_ylabel(ylabel, fontsize=8)


def mode_color(m: str) -> str:
    return MODE_COLORS.get(m, DEFAULT_COLOR)


def ql(q: str) -> str:
    return QUERY_LABELS.get(q, q)


def ml(m: str) -> str:
    return MODE_LABELS.get(m, m)


# Experiment definitions — dirs are relative to raw_dir
EXPERIMENTS = {
    "original": {
        "label": "Phase 1–3  ·  1M rows",
        "dirs":  ["phase1_dev", "phase2_dev", "phase3_dev"],
    },
    "par_ingest": {
        "label": "Phase 2–3  ·  Parallel Ingest  (1M)",
        "dirs":  ["phase2_par_ingest", "phase3_par_ingest"],
    },
    "full": {
        "label": "Phase 1–3  ·  Full Dataset  (46M rows)",
        "dirs":  ["phase1_full", "phase2_full", "phase3_full"],
        "dataset_labels": {"large_dev": "46M rows"},
    },
}


def _load_dirs(raw_dir: Path, dirs: list[str]) -> pd.DataFrame:
    frames = []
    for d in dirs:
        for f in glob.glob(str(raw_dir / d / "*.csv")):
            p = Path(f)
            if p.stem.startswith("batch_") or p.stem.startswith("subset_"):
                continue
            try:
                df = pd.read_csv(f)
                if "execution_mode" not in df.columns:
                    continue
                frames.append(df)
            except Exception as e:
                print(f"[warn] skipping {f}: {e}", file=sys.stderr)
    if not frames:
        return pd.DataFrame()
    combined = pd.concat(frames, ignore_index=True)
    for col, default in [("cpu_ms", float("nan")), ("peak_rss_kb", -1)]:
        if col not in combined.columns:
            combined[col] = default
    combined["run_number"]   = pd.to_numeric(combined["run_number"],   errors="coerce")
    combined["query_ms"]     = pd.to_numeric(combined["query_ms"],     errors="coerce")
    combined["ingest_ms"]    = pd.to_numeric(combined["ingest_ms"],    errors="coerce")
    combined["total_ms"]     = pd.to_numeric(combined["total_ms"],     errors="coerce")
    combined["cpu_ms"]       = pd.to_numeric(combined["cpu_ms"],       errors="coerce")
    combined["peak_rss_kb"]  = pd.to_numeric(combined["peak_rss_kb"],  errors="coerce")
    combined["rows_accepted"]= pd.to_numeric(combined["rows_accepted"], errors="coerce")
    combined["thread_count"] = pd.to_numeric(combined["thread_count"], errors="coerce").fillna(1).astype(int)
    return combined[combined["run_number"] >= 0].copy()


def load_all_csvs(raw_dir: Path) -> pd.DataFrame:
    """Legacy single-frame loader used by existing chart functions."""
    return _load_dirs(raw_dir, EXPERIMENTS["original"]["dirs"])


def load_experiment_frames(raw_dir: Path) -> dict[str, pd.DataFrame]:
    """Returns {experiment_key: DataFrame} for each experiment."""
    result = {}
    for key, meta in EXPERIMENTS.items():
        df = _load_dirs(raw_dir, meta["dirs"])
        if not df.empty:
            result[key] = df
    return result


def query_rows(df: pd.DataFrame) -> pd.DataFrame:
    return df[df["query_type"] != "ingest_only"].copy()


# ── Charts ────────────────────────────────────────────────────────────────────
def chart_latency(df: pd.DataFrame, dataset: str) -> str | None:
    sub = query_rows(df[df["dataset_label"] == dataset])
    if sub.empty:
        return None
    modes   = [m for m in ["serial","parallel","optimized","optimized_serial","optimized_parallel"]
               if m in sub["execution_mode"].unique()]
    queries = sorted(sub["query_type"].unique())
    x = np.arange(len(queries))
    width = min(0.8 / max(len(modes), 1), 0.16)

    fig, ax = plt.subplots(figsize=(12, 4.5), facecolor=CARD_BG)
    for i, mode in enumerate(modes):
        ms = sub[sub["execution_mode"] == mode]
        means, stds = [], []
        for q in queries:
            v = ms[ms["query_type"] == q]["query_ms"]
            means.append(v.mean() if len(v) else 0)
            stds.append(v.std()   if len(v) else 0)
        offset = (i - len(modes)/2 + 0.5) * width
        bars = ax.bar(x + offset, means, width * 0.9,
                      label=ml(mode), color=mode_color(mode), alpha=0.92, zorder=3)
        ax.errorbar(x + offset, means, yerr=stds,
                    fmt="none", color="#555", capsize=2, linewidth=0.8, zorder=4)
        for bar, m in zip(bars, means):
            if m > 0.01:
                ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + max(means)*0.01,
                        f"{m:.2f}", ha="center", va="bottom", fontsize=6, color=TEXT_COLOR)

    ax.set_xticks(x)
    ax.set_xticklabels([ql(q) for q in queries], fontsize=9)
    style_ax(ax, title=f"QUERY LATENCY — {DATASET_LABELS.get(dataset, dataset)}  ·  mean (ms) ± stddev  ·  10 runs",
             ylabel="Query time (ms)")
    ax.legend(fontsize=8, framealpha=0.6)
    fig.tight_layout(pad=1.2)
    b64 = fig_to_b64(fig); plt.close(fig); return b64


def chart_speedup(df: pd.DataFrame, dataset: str) -> str | None:
    sub = query_rows(df[df["dataset_label"] == dataset])
    if sub.empty:
        return None
    serial_means = {}
    for q in sub["query_type"].unique():
        v = sub[(sub["execution_mode"] == "serial") & (sub["query_type"] == q)]["query_ms"]
        if len(v):
            serial_means[q] = v.mean()
    if not serial_means:
        return None

    compare_modes = [m for m in ["parallel","optimized","optimized_serial","optimized_parallel"]
                     if m in sub["execution_mode"].unique()]
    queries = sorted(serial_means.keys())
    x = np.arange(len(queries))
    width = min(0.8 / max(len(compare_modes), 1), 0.2)

    fig, ax = plt.subplots(figsize=(11, 4.5), facecolor=CARD_BG)
    for i, mode in enumerate(compare_modes):
        speedups = []
        for q in queries:
            v = sub[(sub["execution_mode"] == mode) & (sub["query_type"] == q)]["query_ms"]
            if len(v) and serial_means.get(q, 0) > 0:
                speedups.append(serial_means[q] / v.mean())
            else:
                speedups.append(0)
        offset = (i - len(compare_modes)/2 + 0.5) * width
        bars = ax.bar(x + offset, speedups, width*0.9,
                      label=ml(mode), color=mode_color(mode), alpha=0.92, zorder=3)
        for bar, s in zip(bars, speedups):
            if s > 0.05:
                ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.02,
                        f"{s:.2f}×", ha="center", va="bottom", fontsize=6.5, color=TEXT_COLOR)

    ax.axhline(1.0, color="#e85d2f", linewidth=1.2, linestyle="--", zorder=5, label="Serial baseline")
    ax.set_xticks(x)
    ax.set_xticklabels([ql(q) for q in queries], fontsize=9)
    style_ax(ax, title=f"SPEEDUP vs SERIAL — {DATASET_LABELS.get(dataset, dataset)}", ylabel="Speedup (×)")
    ax.legend(fontsize=8, framealpha=0.6)
    fig.tight_layout(pad=1.2)
    b64 = fig_to_b64(fig); plt.close(fig); return b64


def chart_thread_scaling(df: pd.DataFrame, dataset: str) -> str | None:
    sub = query_rows(df[(df["dataset_label"] == dataset) & (df["execution_mode"] == "parallel")])
    if sub.empty:
        return None
    queries = sorted(sub["query_type"].unique())
    thread_counts = sorted(sub["thread_count"].unique())
    if len(thread_counts) < 2:
        return None

    cols = min(len(queries), 3)
    rows = (len(queries) + cols - 1) // cols
    fig, axes = plt.subplots(rows, cols, figsize=(cols*4.5, rows*3.5), facecolor=BG)
    axes_flat = axes.flatten() if hasattr(axes, "flatten") else [axes]

    for idx, q in enumerate(queries):
        ax = axes_flat[idx]
        qsub = sub[sub["query_type"] == q]
        means = []
        for t in thread_counts:
            v = qsub[qsub["thread_count"] == t]["query_ms"]
            means.append(v.mean() if len(v) else float("nan"))
        ax.plot(thread_counts, means, marker="o", color=MODE_COLORS["parallel"],
                linewidth=2, markersize=5, zorder=3)
        ax.fill_between(thread_counts, means, alpha=0.12, color=MODE_COLORS["parallel"])
        for t, m in zip(thread_counts, means):
            if not np.isnan(m):
                ax.text(t, m + max(m for m in means if not np.isnan(m))*0.04,
                        f"{m:.1f}", ha="center", va="bottom", fontsize=7)
        style_ax(ax, title=ql(q), xlabel="Threads", ylabel="Query time (ms)")
        ax.set_xticks(thread_counts)

    for idx in range(len(queries), len(axes_flat)):
        axes_flat[idx].set_visible(False)

    fig.suptitle(f"THREAD SCALING — {DATASET_LABELS.get(dataset, dataset)}", fontsize=10,
                 color=TEXT_COLOR, fontweight="semibold", y=1.01)
    fig.tight_layout(pad=1.5)
    b64 = fig_to_b64(fig); plt.close(fig); return b64


def chart_ingest_scaling(df: pd.DataFrame) -> str | None:
    sub = df[df["execution_mode"] == "serial"].copy()
    if sub.empty:
        return None
    datasets = [d for d in DATASET_ORDER if d in sub["dataset_label"].unique()]
    if len(datasets) < 2:
        return None

    fig, ax = plt.subplots(figsize=(8, 4), facecolor=CARD_BG)
    means = []
    for ds in datasets:
        v = sub[sub["dataset_label"] == ds]["ingest_ms"]
        means.append(v.mean() if len(v) else 0)

    bars = ax.bar([DATASET_LABELS.get(d, d) for d in datasets], means,
                  color=MODE_COLORS["serial"], alpha=0.88, zorder=3, width=0.5)
    for bar, m in zip(bars, means):
        ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + max(means)*0.01,
                f"{m:.0f} ms", ha="center", va="bottom", fontsize=8, color=TEXT_COLOR)

    style_ax(ax, title="INGEST TIME vs DATASET SIZE  —  Serial baseline",
             ylabel="Ingest time (ms)")
    fig.tight_layout(pad=1.2)
    b64 = fig_to_b64(fig); plt.close(fig); return b64


def chart_memory(df: pd.DataFrame, dataset: str) -> str | None:
    sub = df[(df["dataset_label"] == dataset) & (df["peak_rss_kb"] > 0)]
    if sub.empty:
        return None
    modes = [m for m in ["serial","parallel","optimized","optimized_serial","optimized_parallel"]
             if m in sub["execution_mode"].unique()]

    fig, ax = plt.subplots(figsize=(8, 4), facecolor=CARD_BG)
    means = []
    for mode in modes:
        v = sub[sub["execution_mode"] == mode]["peak_rss_kb"]
        means.append(v.mean()/1024 if len(v) else 0)  # KB → MB

    bars = ax.bar(range(len(modes)), means,
                  color=[mode_color(m) for m in modes], alpha=0.88, zorder=3, width=0.5)
    for bar, m in zip(bars, means):
        if m > 0:
            ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + max(means)*0.01,
                    f"{m:.0f} MB", ha="center", va="bottom", fontsize=8, color=TEXT_COLOR)

    style_ax(ax, title=f"PEAK MEMORY (RSS) — {DATASET_LABELS.get(dataset, dataset)}",
             ylabel="Peak RSS (MB)")
    ax.set_xticks(range(len(modes)))
    ax.set_xticklabels([ml(m) for m in modes], rotation=15, ha="right", fontsize=8)
    fig.tight_layout(pad=1.2)
    b64 = fig_to_b64(fig); plt.close(fig); return b64


def chart_ingest_by_threads(df: pd.DataFrame) -> str | None:
    """Ingest time vs thread count for each dataset size (parallel ingest experiment)."""
    par = df[df["execution_mode"] == "parallel"].copy()
    if par.empty:
        return None
    datasets = [d for d in DATASET_ORDER if d in par["dataset_label"].unique()]
    if not datasets:
        return None
    thread_counts = sorted(par["thread_count"].unique())
    if len(thread_counts) < 2:
        return None

    fig, axes = plt.subplots(1, len(datasets), figsize=(5 * len(datasets), 4.5), facecolor=BG)
    if len(datasets) == 1:
        axes = [axes]

    for ax, ds in zip(axes, datasets):
        sub = par[par["dataset_label"] == ds]
        means = []
        for t in thread_counts:
            v = sub[sub["thread_count"] == t]["ingest_ms"]
            means.append(v.mean() if len(v) else float("nan"))

        # Also draw serial baseline (thread_count=1 serial)
        ser = df[(df["execution_mode"] == "serial") & (df["dataset_label"] == ds)]["ingest_ms"]
        ser_mean = ser.mean() if len(ser) else float("nan")

        ax.plot(thread_counts, means, marker="o", color=MODE_COLORS["parallel"],
                linewidth=2, markersize=5, zorder=3, label="Parallel ingest")
        ax.fill_between(thread_counts, means, alpha=0.12, color=MODE_COLORS["parallel"])
        if not np.isnan(ser_mean):
            ax.axhline(ser_mean, color=MODE_COLORS["serial"], linewidth=1.5,
                       linestyle="--", zorder=5, label=f"Serial ({ser_mean:.0f} ms)")
        for t, m in zip(thread_counts, means):
            if not np.isnan(m):
                ax.text(t, m + max(m for m in means if not np.isnan(m)) * 0.04,
                        f"{m:.0f}", ha="center", va="bottom", fontsize=7)
        style_ax(ax, title=DATASET_LABELS.get(ds, ds),
                 xlabel="Threads", ylabel="Ingest time (ms)")
        ax.set_xticks(thread_counts)
        ax.legend(fontsize=7, framealpha=0.6)

    fig.suptitle("INGEST TIME vs THREAD COUNT  —  mmap parallel loader",
                 fontsize=10, color=TEXT_COLOR, fontweight="semibold", y=1.02)
    fig.tight_layout(pad=1.5)
    b64 = fig_to_b64(fig); plt.close(fig); return b64


def chart_ingest_comparison(orig_df: pd.DataFrame, par_df: pd.DataFrame) -> str | None:
    """Side-by-side ingest speedup: serial (phase1) vs best parallel ingest at 8T."""
    if orig_df.empty or par_df.empty:
        return None
    datasets = [d for d in DATASET_ORDER
                if d in orig_df["dataset_label"].unique() and d in par_df["dataset_label"].unique()]
    if not datasets:
        return None

    x = np.arange(len(datasets))
    width = 0.28
    fig, ax = plt.subplots(figsize=(9, 4.5), facecolor=CARD_BG)

    orig_means, par_means_8t, par_means_best = [], [], []
    for ds in datasets:
        orig = orig_df[(orig_df["execution_mode"] == "serial") &
                       (orig_df["dataset_label"] == ds)]["ingest_ms"]
        par8 = par_df[(par_df["execution_mode"] == "parallel") &
                      (par_df["thread_count"] == 8) &
                      (par_df["dataset_label"] == ds)]["ingest_ms"]
        par_best_t = par_df[(par_df["execution_mode"] == "parallel") &
                            (par_df["dataset_label"] == ds)]
        best_mean = par_best_t.groupby("thread_count")["ingest_ms"].mean().min() if not par_best_t.empty else float("nan")
        orig_means.append(orig.mean() if len(orig) else float("nan"))
        par_means_8t.append(par8.mean() if len(par8) else float("nan"))
        par_means_best.append(best_mean)

    b1 = ax.bar(x - width, orig_means, width * 0.9,
                label="Serial ingest (Phase 1)", color=MODE_COLORS["serial"], alpha=0.88, zorder=3)
    b2 = ax.bar(x,          par_means_8t, width * 0.9,
                label="Parallel ingest (8T)", color=MODE_COLORS["parallel"], alpha=0.88, zorder=3)
    b3 = ax.bar(x + width, par_means_best, width * 0.9,
                label="Parallel ingest (best T)", color=MODE_COLORS["optimized_parallel"], alpha=0.88, zorder=3)

    for bars in [b1, b2, b3]:
        for bar in bars:
            h = bar.get_height()
            if h > 0 and not np.isnan(h):
                ax.text(bar.get_x() + bar.get_width() / 2, h + max(orig_means) * 0.01,
                        f"{h:.0f}", ha="center", va="bottom", fontsize=7, color=TEXT_COLOR)

    for i, (o, p8) in enumerate(zip(orig_means, par_means_8t)):
        if o > 0 and p8 > 0:
            sp = o / p8
            ax.text(x[i], max(orig_means) * 1.07, f"{sp:.1f}×",
                    ha="center", va="bottom", fontsize=8, color=MODE_COLORS["parallel"],
                    fontweight="bold")

    ax.set_xticks(x)
    ax.set_xticklabels([DATASET_LABELS.get(d, d) for d in datasets], fontsize=9)
    style_ax(ax, title="INGEST SPEEDUP: Serial vs Parallel mmap  —  wall-clock time",
             ylabel="Ingest time (ms)")
    ax.legend(fontsize=8, framealpha=0.6)
    fig.tight_layout(pad=1.2)
    b64 = fig_to_b64(fig); plt.close(fig); return b64


def chart_cpu_efficiency(df: pd.DataFrame, dataset: str) -> str | None:
    sub = query_rows(df[(df["dataset_label"] == dataset) & (df["cpu_ms"] > 0) & (df["query_ms"] > 0)])
    if sub.empty:
        return None
    modes = [m for m in ["serial","parallel","optimized","optimized_serial","optimized_parallel"]
             if m in sub["execution_mode"].unique()]

    fig, ax = plt.subplots(figsize=(8, 4), facecolor=CARD_BG)
    ratios = []
    for mode in modes:
        v = sub[sub["execution_mode"] == mode]
        r = (v["cpu_ms"] / v["query_ms"]).mean() if len(v) else 0
        ratios.append(r)

    bars = ax.bar(range(len(modes)), ratios,
                  color=[mode_color(m) for m in modes], alpha=0.88, zorder=3, width=0.5)
    for bar, r in zip(bars, ratios):
        if r > 0:
            ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + max(ratios)*0.01,
                    f"{r:.2f}×", ha="center", va="bottom", fontsize=8, color=TEXT_COLOR)

    ax.axhline(1.0, color="#64748b", linestyle="--", linewidth=1, zorder=5)
    style_ax(ax, title=f"CPU EFFICIENCY — cpu_ms / query_ms  ({DATASET_LABELS.get(dataset, dataset)})",
             ylabel="CPU/Wall ratio")
    ax.set_xticks(range(len(modes)))
    ax.set_xticklabels([ml(m) for m in modes], rotation=15, ha="right", fontsize=8)
    fig.tight_layout(pad=1.2)
    b64 = fig_to_b64(fig); plt.close(fig); return b64


# ── KPIs ──────────────────────────────────────────────────────────────────────
def compute_kpis(df: pd.DataFrame) -> dict:
    kpis = {}
    ld = df[df["dataset_label"] == "large_dev"]
    if not ld.empty:
        kpis["rows"] = int(ld["rows_accepted"].max())
        rss = ld[ld["peak_rss_kb"] > 0]["peak_rss_kb"].max()
        kpis["peak_rss_mb"] = int(rss / 1024) if rss > 0 else None
    else:
        kpis["rows"] = int(df["rows_accepted"].max())
        kpis["peak_rss_mb"] = None

    # Best speedup (serial → optimized/parallel)
    qdf = query_rows(df[df["dataset_label"] == "large_dev"]) if not ld.empty else query_rows(df)
    best_sp, best_sp_q = 0.0, ""
    best_p2, best_p2_q = 0.0, ""
    for q in qdf["query_type"].unique():
        s = qdf[(qdf["execution_mode"] == "serial")   & (qdf["query_type"] == q)]["query_ms"].mean()
        o = qdf[(qdf["execution_mode"] == "optimized")& (qdf["query_type"] == q)]["query_ms"].mean()
        p = qdf[(qdf["execution_mode"] == "parallel") & (qdf["query_type"] == q)]["query_ms"].mean()
        if s and o and o > 0:
            sp = s / o
            if sp > best_sp: best_sp, best_sp_q = sp, q
        if s and p and p > 0:
            sp2 = s / p
            if sp2 > best_p2: best_p2, best_p2_q = sp2, q

    kpis["best_p3_speedup"] = round(best_sp, 2) if best_sp > 0 else None
    kpis["best_p3_query"]   = ql(best_sp_q)
    kpis["best_p2_speedup"] = round(best_p2, 2) if best_p2 > 0 else None
    kpis["best_p2_query"]   = ql(best_p2_q)
    kpis["n_queries"]       = df["query_type"].nunique()
    kpis["n_datasets"]      = df["dataset_label"].nunique()
    kpis["datasets"]        = [DATASET_LABELS.get(d, d) for d in DATASET_ORDER
                                if d in df["dataset_label"].unique()]
    return kpis


# ── Summary table ─────────────────────────────────────────────────────────────
def summary_table_html(df: pd.DataFrame, dataset: str) -> str:
    sub = query_rows(df[df["dataset_label"] == dataset])
    if sub.empty:
        return "<p style='color:#78716c'>No data</p>"
    modes   = [m for m in ["serial","parallel","optimized","optimized_serial","optimized_parallel"]
               if m in sub["execution_mode"].unique()]
    queries = sorted(sub["query_type"].unique())
    rows_html = ""
    for q in queries:
        serial_mean = sub[(sub["execution_mode"]=="serial") & (sub["query_type"]==q)]["query_ms"].mean()
        for mode in modes:
            v = sub[(sub["execution_mode"]==mode) & (sub["query_type"]==q)]["query_ms"]
            if v.empty: continue
            mean, std = v.mean(), v.std()
            speedup = f"{serial_mean/mean:.2f}×" if mode != "serial" and serial_mean > 0 and mean > 0 else "—"
            rows_html += f"""<tr>
                <td>{ml(mode)}</td><td>{ql(q)}</td>
                <td>{mean:.3f}</td><td>{std:.3f}</td><td>{speedup}</td>
            </tr>"""
    return f"""<table class="bench-table">
        <thead><tr>
            <th>Mode</th><th>Query</th>
            <th>Mean (ms)</th><th>Std (ms)</th><th>Speedup</th>
        </tr></thead>
        <tbody>{rows_html}</tbody>
    </table>"""


# ── CSS ───────────────────────────────────────────────────────────────────────
CSS = """
* { box-sizing: border-box; margin: 0; padding: 0; }
body { font-family: -apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;
       background: #faf8f4; color: #1c1917; }
.topbar { background: #14171f; color: #fff; padding: 14px 28px;
          display: flex; align-items: center; gap: 16px;
          position: sticky; top: 0; z-index: 100; }
.topbar-title { font-size: 1.1rem; font-weight: 700; letter-spacing: 0.01em; }
.topbar-sub   { font-size: 0.78rem; color: #94a3b8; display:flex; gap:14px; }
.topbar-badge { margin-left: auto; background: #e85d2f; border-radius: 6px;
                padding: 4px 10px; font-size: 0.75rem; font-weight: 600; }
.container { max-width: 1280px; margin: 0 auto; padding: 24px 28px; }
.kpi-grid { display: grid; grid-template-columns: repeat(6,1fr); gap: 12px; margin-bottom: 28px; }
.kpi-card { background: #fff; border-radius: 12px; padding: 16px 14px;
            box-shadow: 0 1px 3px rgba(0,0,0,.06); }
.kpi-val  { font-size: 1.6rem; font-weight: 700; color: #1c1917; line-height: 1.1; }
.kpi-label{ font-size: 0.72rem; color: #78716c; margin-top: 4px; text-transform: uppercase;
            letter-spacing: 0.05em; }
.kpi-sub  { font-size: 0.7rem; color: #a8a29e; margin-top: 2px; }
.section-label { font-size: 0.7rem; font-weight: 600; letter-spacing: 0.08em;
                 text-transform: uppercase; color: #78716c; margin: 28px 0 10px; }
.chart-row-1 { display: grid; grid-template-columns: 1fr; gap: 16px; }
.chart-row-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; }
.chart-card  { background: #fff; border-radius: 12px; padding: 18px 20px;
               box-shadow: 0 1px 3px rgba(0,0,0,.06); }
.chart-card img { width: 100%; height: auto; display: block; }
.chart-card-title { font-size: 0.8rem; font-weight: 600; color: #44403c; margin-bottom: 4px; }
.chart-card-sub   { font-size: 0.72rem; color: #78716c; margin-bottom: 12px; }
.dataset-tabs { display: flex; gap: 8px; margin: 28px 0 12px; flex-wrap: wrap; }
.dataset-tab  { padding: 6px 16px; border-radius: 20px; font-size: 0.8rem; font-weight: 600;
                cursor: pointer; border: 2px solid transparent; }
.dataset-tab.active { background: #e85d2f; color: #fff; }
.dataset-tab:not(.active) { background: #fff; color: #44403c;
                             border-color: #e8e3da; }
.dataset-pane { display: none; }
.dataset-pane.active { display: block; }
.exp-tabs { display: flex; gap: 10px; margin: 24px 0 8px; flex-wrap: wrap; border-bottom: 2px solid #e8e3da; padding-bottom: 8px; }
.exp-tab  { padding: 8px 20px; border-radius: 8px 8px 0 0; font-size: 0.85rem; font-weight: 600;
            cursor: pointer; border: 2px solid #e8e3da; border-bottom: none; background: #f4f0e8; color: #44403c; }
.exp-tab.active { background: #14171f; color: #fff; border-color: #14171f; }
.exp-pane { display: none; }
.exp-pane.active { display: block; }
.bench-table { width: 100%; border-collapse: collapse; font-size: 0.8rem; }
.bench-table th { background: #f4f0e8; color: #44403c; font-weight: 600;
                  padding: 8px 12px; text-align: left; font-size: 0.72rem; }
.bench-table td { padding: 7px 12px; border-bottom: 1px solid #f0ece4; color: #1c1917; }
.bench-table tr:last-child td { border-bottom: none; }
.bench-table tr:hover td { background: #faf8f4; }
.exp-header { background: #fff; border-radius: 12px; padding: 16px 20px;
              margin-bottom: 20px; box-shadow: 0 1px 3px rgba(0,0,0,.06);
              display: flex; align-items: center; gap: 14px; }
.exp-dot { width: 12px; height: 12px; border-radius: 50%; background: #e85d2f; flex-shrink: 0; }
.exp-title { font-size: 1rem; font-weight: 700; color: #1c1917; }
.exp-sub   { font-size: 0.78rem; color: #78716c; margin-top: 2px; }
footer { text-align: center; padding: 32px 16px; font-size: 0.72rem; color: #a8a29e; }
@media (max-width: 900px) {
    .kpi-grid { grid-template-columns: repeat(3,1fr); }
    .chart-row-2 { grid-template-columns: 1fr; }
}
"""

TAB_JS = """
function switchDataset(id) {
    document.querySelectorAll('.dataset-pane').forEach(p => p.classList.remove('active'));
    document.querySelectorAll('.dataset-tab').forEach(t => t.classList.remove('active'));
    document.getElementById('pane-' + id).classList.add('active');
    document.getElementById('tab-' + id).classList.add('active');
}
function switchExp(id) {
    document.querySelectorAll('.exp-pane').forEach(p => p.classList.remove('active'));
    document.querySelectorAll('.exp-tab').forEach(t => t.classList.remove('active'));
    document.getElementById('exppane-' + id).classList.add('active');
    document.getElementById('exptab-' + id).classList.add('active');
}
"""


# ── HTML rendering ─────────────────────────────────────────────────────────────
def kpi_card(val, label, sub="") -> str:
    return f"""<div class="kpi-card">
        <div class="kpi-val">{val}</div>
        <div class="kpi-label">{label}</div>
        {"<div class='kpi-sub'>"+sub+"</div>" if sub else ""}
    </div>"""


def img_card(b64, title="", sub="") -> str:
    if not b64:
        return f"""<div class="chart-card">
            <div class="chart-card-title">{title}</div>
            <div class="chart-card-sub" style="padding:32px;text-align:center;color:#a8a29e">
                No data available
            </div>
        </div>"""
    return f"""<div class="chart-card">
        {"<div class='chart-card-title'>"+title+"</div>" if title else ""}
        {"<div class='chart-card-sub'>"+sub+"</div>" if sub else ""}
        <img src="data:image/png;base64,{b64}" alt="{title}">
    </div>"""


def render_dataset_pane(df, dataset, first=False) -> str:
    lat  = chart_latency(df, dataset)
    spd  = chart_speedup(df, dataset)
    thr  = chart_thread_scaling(df, dataset)
    mem  = chart_memory(df, dataset)
    cpu  = chart_cpu_efficiency(df, dataset)
    tbl  = summary_table_html(df, dataset)
    label = DATASET_LABELS.get(dataset, dataset)

    return f"""
    <div class="dataset-pane {'active' if first else ''}" id="pane-{dataset}">
        <div class="section-label">Query Latency</div>
        <div class="chart-row-1">
            {img_card(lat, f"Query latency — {label}", "mean ± stddev, 10 runs each")}
        </div>
        <div class="section-label">Speedup &amp; Thread Scaling</div>
        <div class="chart-row-2">
            {img_card(spd, "Speedup vs Serial")}
            {img_card(thr, "Thread Scaling — Phase 2 Parallel")}
        </div>
        <div class="section-label">Memory &amp; CPU Efficiency</div>
        <div class="chart-row-2">
            {img_card(mem, "Peak RSS by Mode")}
            {img_card(cpu, "CPU / Wall-clock Ratio")}
        </div>
        <div class="section-label">Full Benchmark Results — {label}</div>
        <div class="chart-card">{tbl}</div>
    </div>"""


def render_exp_pane(exp_key: str, df: pd.DataFrame, orig_df: pd.DataFrame, first: bool) -> str:
    """Render one experiment tab pane with dataset sub-tabs."""
    exp_meta = EXPERIMENTS.get(exp_key, {})
    ds_labels_override = exp_meta.get("dataset_labels", {})

    def _dslabel(ds):
        return ds_labels_override.get(ds, DATASET_LABELS.get(ds, ds))

    datasets = [d for d in DATASET_ORDER if d in df["dataset_label"].unique()]
    ingest_b64 = chart_ingest_scaling(df)

    # For par_ingest experiment: add ingest-by-threads charts
    extra_ingest = ""
    if exp_key == "par_ingest":
        by_threads = chart_ingest_by_threads(df)
        compare    = chart_ingest_comparison(orig_df, df)
        extra_ingest = f"""
        <div class="section-label">Parallel Ingest Speedup</div>
        <div class="chart-row-2">
            {img_card(compare, "Ingest Time: Serial vs Parallel mmap",
                      "Orange number = speedup at 8 threads")}
            {img_card(by_threads, "Ingest Time vs Thread Count",
                      "Dashed line = serial baseline")}
        </div>"""

    tab_buttons = "".join(
        f'<div class="dataset-tab {"active" if i==0 else ""}" '
        f'id="tab-{exp_key}-{ds}" onclick="switchDataset(\'{exp_key}-{ds}\')">'
        f'{_dslabel(ds)}</div>'
        for i, ds in enumerate(datasets)
    )

    def _pane(ds, i):
        lat = chart_latency(df, ds)
        spd = chart_speedup(df, ds)
        thr = chart_thread_scaling(df, ds)
        mem = chart_memory(df, ds)
        cpu = chart_cpu_efficiency(df, ds)
        tbl = summary_table_html(df, ds)
        label = _dslabel(ds)
        return f"""
        <div class="dataset-pane {'active' if i==0 else ''}" id="pane-{exp_key}-{ds}">
            <div class="section-label">Query Latency</div>
            <div class="chart-row-1">{img_card(lat, f"Query latency — {label}", "mean ± stddev")}</div>
            <div class="section-label">Speedup &amp; Thread Scaling</div>
            <div class="chart-row-2">
                {img_card(spd, "Speedup vs Serial")}
                {img_card(thr, "Thread Scaling — Parallel Queries")}
            </div>
            <div class="section-label">Memory &amp; CPU</div>
            <div class="chart-row-2">
                {img_card(mem, "Peak RSS by Mode")}
                {img_card(cpu, "CPU / Wall-clock Ratio")}
            </div>
            <div class="section-label">Full Benchmark Results — {label}</div>
            <div class="chart-card">{tbl}</div>
        </div>"""

    panes = "".join(_pane(ds, i) for i, ds in enumerate(datasets))

    return f"""
    <div class="exp-pane {'active' if first else ''}" id="exppane-{exp_key}">
        {extra_ingest}
        <div class="section-label">Ingest Time vs Dataset Size</div>
        <div class="chart-row-1">{img_card(ingest_b64, "Ingest Time vs Dataset Size", "Serial baseline")}</div>
        <div class="dataset-tabs">{tab_buttons}</div>
        {panes}
    </div>"""


def render_html(df: pd.DataFrame, title: str, source: str,
                exp_frames: dict | None = None) -> str:
    kpis      = compute_kpis(df)
    generated = datetime.datetime.now().strftime("%Y-%m-%d %H:%M")

    kpi_html = "".join([
        kpi_card(f"{kpis['rows']/1e6:.2f}M" if kpis.get("rows") else "—",
                 "Records Processed", "large_dev subset"),
        kpi_card(f"{kpis['peak_rss_mb']} MB" if kpis.get("peak_rss_mb") else "—",
                 "Peak RSS", "physical memory"),
        kpi_card(f"{kpis['best_p2_speedup']}×" if kpis.get("best_p2_speedup") else "—",
                 "Phase 2 Best Speedup", kpis.get("best_p2_query","")),
        kpi_card(f"{kpis['best_p3_speedup']}×" if kpis.get("best_p3_speedup") else "—",
                 "Phase 3 Best Speedup", kpis.get("best_p3_query","")),
        kpi_card(str(kpis.get("n_queries","—")), "Query Types", "across all phases"),
        kpi_card(str(kpis.get("n_datasets","—")), "Dataset Sizes",
                 " · ".join(kpis.get("datasets",[]))),
    ])

    # Build experiment tabs
    if exp_frames is None:
        exp_frames = {"original": df}

    orig_df = exp_frames.get("original", df)
    exp_keys = [k for k in EXPERIMENTS if k in exp_frames]
    if not exp_keys:
        exp_keys = list(exp_frames.keys())

    exp_tab_buttons = "".join(
        f'<div class="exp-tab {"active" if i==0 else ""}" '
        f'id="exptab-{k}" onclick="switchExp(\'{k}\')">'
        f'{EXPERIMENTS.get(k, {}).get("label", k)}</div>'
        for i, k in enumerate(exp_keys)
    )
    exp_panes = "".join(
        render_exp_pane(k, exp_frames[k], orig_df, first=(i==0))
        for i, k in enumerate(exp_keys)
    )

    return f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>{title}</title>
<style>{CSS}</style>
</head>
<body>
<div class="topbar">
    <div style="display:flex;align-items:center;gap:14px;min-width:0">
        <div class="topbar-title">UrbanDrop — Mini 1A  Benchmark Dashboard</div>
        <div class="topbar-sub">
            <span>NYC DOT Traffic Speed Dataset</span>
            <span>3-Phase C++ Performance Research</span>
        </div>
    </div>
    <div class="topbar-badge">System 1</div>
</div>
<div class="container">
    <div class="exp-header">
        <div class="exp-dot"></div>
        <div>
            <div class="exp-title">Congestion Intelligence Engine</div>
            <div class="exp-sub">
                Serial AoS (Phase 1) → OpenMP Parallel (Phase 2) → SoA + OpenMP (Phase 3)
                &nbsp;·&nbsp; NYC DOT Traffic Speed data
                &nbsp;·&nbsp; Benchmarked: {generated}
            </div>
        </div>
    </div>

    <div class="section-label">Key Metrics</div>
    <div class="kpi-grid">{kpi_html}</div>

    <div class="section-label">Experiments</div>
    <div class="exp-tabs">{exp_tab_buttons}</div>
    {exp_panes}
</div>
<footer>
    Generated {generated} &nbsp;·&nbsp; Source: {source}
</footer>
<script>{TAB_JS}</script>
</body>
</html>"""


# ── Entry point ────────────────────────────────────────────────────────────────
def main():
    repo_root = Path(__file__).parent.parent
    p = argparse.ArgumentParser(description="UrbanDrop Mini 1A — HTML benchmark report")
    p.add_argument("--raw-dir", default=str(repo_root / "results" / "raw"),
                   help="Directory containing phase{1,2,3}_dev subdirs")
    p.add_argument("--out",     default=str(repo_root / "results" / "report.html"))
    p.add_argument("--title",   default="UrbanDrop Mini 1A — Benchmark Report")
    args = p.parse_args()

    raw_dir = Path(args.raw_dir)
    print(f"[generate_report] loading CSVs from {raw_dir}")
    exp_frames = load_experiment_frames(raw_dir)
    if not exp_frames:
        print("[generate_report] ERROR: no benchmark CSVs found", file=sys.stderr)
        sys.exit(1)

    # Use "original" as primary frame for KPI cards; fall back to first available
    _orig = exp_frames.get("original")
    df = _orig if (_orig is not None and not _orig.empty) else next(iter(exp_frames.values()))
    print(f"[generate_report] loaded experiments: {list(exp_frames.keys())}")
    print(f"[generate_report] primary frame: {len(df):,} rows across modes: "
          f"{sorted(df['execution_mode'].unique())}")
    print(f"[generate_report] datasets: {sorted(df['dataset_label'].unique())}")
    print(f"[generate_report] query types: {sorted(df['query_type'].unique())}")

    html = render_html(df, args.title, str(raw_dir), exp_frames=exp_frames)

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(html, encoding="utf-8")
    size_kb = out_path.stat().st_size // 1024
    print(f"[generate_report] wrote {out_path}  ({size_kb} KB)")


if __name__ == "__main__":
    main()
