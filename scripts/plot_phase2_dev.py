#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Dict, List, Tuple

from plot_common import (
    add_common_figure_text,
    save_matplotlib_figure,
    utc_now_iso,
    write_chart_manifest,
    write_chart_markdown_index,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate Phase 2 dev benchmark graphs.")
    parser.add_argument("--summary-csv", required=True, help="Summary CSV from summarize_phase2_dev.py")
    parser.add_argument("--output-dir", default="results/graphs/phase2_dev", help="Graph output directory")
    parser.add_argument("--parallel-thread", default="4", help="Parallel thread for comparison charts")
    parser.add_argument(
        "--parallel-threads",
        default="1,2,4,8,16",
        help="Comma-separated parallel thread counts for all-thread comparison charts",
    )
    parser.add_argument(
        "--allow-partial",
        action="store_true",
        help="Allow partial chart generation when some scenario/subset/mode combinations are missing.",
    )
    return parser.parse_args()


def load_rows(path: Path) -> List[Dict[str, str]]:
    if not path.is_file():
        raise SystemExit(f"Summary CSV not found: {path}")
    with path.open("r", newline="", encoding="utf-8") as fh:
        rows = list(csv.DictReader(fh))
    if not rows:
        raise SystemExit(f"Summary CSV has no rows: {path}")
    return rows


def row_lookup(rows: List[Dict[str, str]], **kwargs: str) -> Dict[str, str] | None:
    for row in rows:
        if all(row.get(k) == v for k, v in kwargs.items()):
            return row
    return None


def require_matplotlib():
    try:
        import matplotlib.pyplot as plt  # type: ignore
    except Exception as exc:
        raise SystemExit(
            "matplotlib is required to generate Phase 2 graphs.\n"
            "Install with: python3 -m pip install matplotlib"
        ) from exc
    return plt


def save_line_chart(
    plt,
    out_dir: Path,
    stem: str,
    title: str,
    subtitle: str,
    what_tested: str,
    xlabel: str,
    ylabel: str,
    xlabels: List[str],
    series: List[Tuple[str, List[float], str]],
    source_summary: Path,
    generated_utc: str,
) -> Path:
    fig, ax = plt.subplots(figsize=(12, 7))
    x = list(range(len(xlabels)))
    for name, vals, color in series:
        ax.plot(x[: len(vals)], vals, marker="o", linewidth=2.0, color=color, label=name)
    ax.set_xticks(x)
    ax.set_xticklabels(xlabels)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.grid(axis="y", linestyle="--", alpha=0.25)
    ax.legend(loc="best")
    add_common_figure_text(fig, ax, subtitle, what_tested, source_summary, generated_utc)
    fig.tight_layout(rect=(0.02, 0.04, 1.0, 0.90))
    paths = save_matplotlib_figure(fig, out_dir, stem, dpi=150)
    plt.close(fig)
    return next(p for p in paths if p.suffix == ".svg")


def save_grouped_bar_chart(
    plt,
    out_dir: Path,
    stem: str,
    title: str,
    subtitle: str,
    what_tested: str,
    xlabel: str,
    ylabel: str,
    categories: List[str],
    groups: List[Tuple[str, List[float], str]],
    source_summary: Path,
    generated_utc: str,
) -> Path:
    fig, ax = plt.subplots(figsize=(12, 7))
    n_cat = len(categories)
    n_grp = max(1, len(groups))
    x = list(range(n_cat))
    width = 0.8 / n_grp
    for i, (name, vals, color) in enumerate(groups):
        x_i = [v - 0.4 + (i + 0.5) * width for v in x]
        ax.bar(x_i[: len(vals)], vals, width=width, color=color, label=name)
    ax.set_xticks(x)
    ax.set_xticklabels(categories)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.grid(axis="y", linestyle="--", alpha=0.25)
    ax.legend(loc="best")
    add_common_figure_text(fig, ax, subtitle, what_tested, source_summary, generated_utc)
    fig.tight_layout(rect=(0.02, 0.04, 1.0, 0.90))
    paths = save_matplotlib_figure(fig, out_dir, stem, dpi=150)
    plt.close(fig)
    return next(p for p in paths if p.suffix == ".svg")


def main() -> int:
    args = parse_args()
    rows = load_rows(Path(args.summary_csv))
    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    plt = require_matplotlib()

    all_threads = [t.strip() for t in args.parallel_threads.split(",") if t.strip()]
    generated_utc = utc_now_iso()
    runs_values = sorted({r.get("runs", "").strip() for r in rows if r.get("runs", "").strip()})
    runs_text = ",".join(runs_values) if runs_values else "unknown"
    subtitle = f"Phase 2 Dev | runs={runs_text} | threads={','.join(all_threads)}"
    chart_manifest_rows: List[Dict[str, str]] = []

    subset_order = ["small", "medium", "large_dev"]
    subset_display = {"small": "10k", "medium": "100k", "large_dev": "1M"}
    colors = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b", "#17becf"]

    scenarios = sorted({r["scenario_name"] for r in rows})

    for scenario in scenarios:
        runtime_series = []
        speedup_series = []
        xlabels: List[str] = []
        for si, subset in enumerate(subset_order):
            subset_rows = [
                r
                for r in rows
                if r["scenario_name"] == scenario and r["subset_label"] == subset and r["mode"] == "parallel"
            ]
            subset_rows.sort(key=lambda r: int(r["thread_count"]))
            if not subset_rows:
                continue
            xlabels = [r["thread_count"] for r in subset_rows]
            runtime_vals = [float(r["query_mean_ms"]) for r in subset_rows]
            speedup_vals = [
                float(r["speedup_vs_serial_query"]) if r["speedup_vs_serial_query"] != "nan" else 0.0
                for r in subset_rows
            ]
            runtime_series.append((subset_display[subset], runtime_vals, colors[si % len(colors)]))
            speedup_series.append((subset_display[subset], speedup_vals, colors[si % len(colors)]))

        if runtime_series and xlabels:
            runtime_path = save_line_chart(
                plt,
                out_dir,
                f"runtime_vs_threads_{scenario}",
                f"Query Runtime vs Thread Count ({scenario})",
                subtitle,
                "Parallel query runtime scaling by thread count for each subset.",
                "Thread Count",
                "Mean Query Runtime (ms)",
                xlabels,
                runtime_series,
                Path(args.summary_csv),
                generated_utc,
            )
            chart_manifest_rows.append(
                {
                    "chart_id": f"phase2_runtime_vs_threads_{scenario}",
                    "file_path": str(runtime_path),
                    "phase": "phase2",
                    "what_tested": "Parallel runtime scaling by thread count.",
                    "x_axis": "Thread Count",
                    "y_axis": "Mean Query Runtime (ms)",
                    "source_summary_csv": str(Path(args.summary_csv)),
                    "generated_utc": generated_utc,
                }
            )

            speedup_path = save_line_chart(
                plt,
                out_dir,
                f"speedup_vs_threads_{scenario}",
                f"Speedup vs Thread Count ({scenario})",
                subtitle,
                "Parallel speedup relative to serial query runtime.",
                "Thread Count",
                "Speedup",
                xlabels,
                speedup_series,
                Path(args.summary_csv),
                generated_utc,
            )
            chart_manifest_rows.append(
                {
                    "chart_id": f"phase2_speedup_vs_threads_{scenario}",
                    "file_path": str(speedup_path),
                    "phase": "phase2",
                    "what_tested": "Speedup scaling by thread count.",
                    "x_axis": "Thread Count",
                    "y_axis": "Speedup (serial/parallel)",
                    "source_summary_csv": str(Path(args.summary_csv)),
                    "generated_utc": generated_utc,
                }
            )

    for scenario in scenarios:
        labels: List[str] = []
        serial_vals: List[float] = []
        parallel_vals: List[float] = []
        for subset in subset_order:
            serial_row = row_lookup(rows, scenario_name=scenario, subset_label=subset, mode="serial", thread_count="1")
            parallel_row = row_lookup(rows, scenario_name=scenario, subset_label=subset, mode="parallel", thread_count=args.parallel_thread)
            if serial_row is None or parallel_row is None:
                if args.allow_partial:
                    continue
                raise SystemExit(
                    f"Missing serial/parallel rows for scenario={scenario}, subset={subset}, thread={args.parallel_thread}."
                )
            labels.append(subset_display[subset])
            serial_vals.append(float(serial_row["query_mean_ms"]))
            parallel_vals.append(float(parallel_row["query_mean_ms"]))

        if labels:
            comp_path = save_grouped_bar_chart(
                plt,
                out_dir,
                f"serial_vs_parallel_by_subset_{scenario}",
                f"Serial vs Parallel Query Runtime by Subset ({scenario})",
                subtitle,
                "Direct runtime comparison between serial and selected parallel thread count.",
                "Subset Size",
                "Mean Query Runtime (ms)",
                labels,
                [("serial", serial_vals, "#1f77b4"), (f"parallel t={args.parallel_thread}", parallel_vals, "#ff7f0e")],
                Path(args.summary_csv),
                generated_utc,
            )
            chart_manifest_rows.append(
                {
                    "chart_id": f"phase2_serial_vs_parallel_subset_{scenario}",
                    "file_path": str(comp_path),
                    "phase": "phase2",
                    "what_tested": "Serial vs parallel runtime by subset.",
                    "x_axis": "Subset Size",
                    "y_axis": "Mean Query Runtime (ms)",
                    "source_summary_csv": str(Path(args.summary_csv)),
                    "generated_utc": generated_utc,
                }
            )

    for scenario in scenarios:
        labels: List[str] = []
        serial_vals: List[float] = []
        parallel_by_thread: Dict[str, List[float]] = {t: [] for t in all_threads}
        for subset in subset_order:
            serial_row = row_lookup(rows, scenario_name=scenario, subset_label=subset, mode="serial", thread_count="1")
            if serial_row is None:
                continue

            candidates: Dict[str, Dict[str, str]] = {}
            for t in all_threads:
                candidate = row_lookup(rows, scenario_name=scenario, subset_label=subset, mode="parallel", thread_count=t)
                if candidate is None:
                    if args.allow_partial:
                        candidates = {}
                        break
                    raise SystemExit(
                        f"Missing parallel row for scenario={scenario}, subset={subset}, thread={t}."
                    )
                candidates[t] = candidate

            if not candidates:
                continue

            labels.append(subset_display[subset])
            serial_vals.append(float(serial_row["query_mean_ms"]))
            for t in all_threads:
                parallel_by_thread[t].append(float(candidates[t]["query_mean_ms"]))

        if labels:
            groups: List[Tuple[str, List[float], str]] = [("serial", serial_vals, "#1f77b4")]
            for i, t in enumerate(all_threads):
                groups.append((f"parallel t={t}", parallel_by_thread[t], colors[(i + 1) % len(colors)]))
            all_threads_path = save_grouped_bar_chart(
                plt,
                out_dir,
                f"serial_vs_parallel_all_threads_by_subset_{scenario}",
                f"Serial vs Parallel (All Threads) by Subset ({scenario})",
                subtitle,
                "Runtime comparison across serial and all configured parallel thread counts.",
                "Subset Size",
                "Mean Query Runtime (ms)",
                labels,
                groups,
                Path(args.summary_csv),
                generated_utc,
            )
            chart_manifest_rows.append(
                {
                    "chart_id": f"phase2_all_threads_subset_{scenario}",
                    "file_path": str(all_threads_path),
                    "phase": "phase2",
                    "what_tested": "All-thread serial vs parallel subset comparison.",
                    "x_axis": "Subset Size",
                    "y_axis": "Mean Query Runtime (ms)",
                    "source_summary_csv": str(Path(args.summary_csv)),
                    "generated_utc": generated_utc,
                }
            )

    for subset in subset_order:
        labels: List[str] = []
        serial_vals: List[float] = []
        parallel_vals: List[float] = []
        for scenario in scenarios:
            serial_row = row_lookup(rows, scenario_name=scenario, subset_label=subset, mode="serial", thread_count="1")
            parallel_row = row_lookup(rows, scenario_name=scenario, subset_label=subset, mode="parallel", thread_count=args.parallel_thread)
            if serial_row is None or parallel_row is None:
                if args.allow_partial:
                    continue
                raise SystemExit(
                    f"Missing runtime-by-scenario rows for scenario={scenario}, subset={subset}, thread={args.parallel_thread}."
                )
            labels.append(scenario)
            serial_vals.append(float(serial_row["query_mean_ms"]))
            parallel_vals.append(float(parallel_row["query_mean_ms"]))

        if labels:
            by_scenario_path = save_grouped_bar_chart(
                plt,
                out_dir,
                f"runtime_by_scenario_{subset}",
                f"Runtime by Scenario ({subset_display[subset]})",
                subtitle,
                "Scenario-level runtime comparison between serial and selected parallel mode.",
                "Scenario",
                "Mean Query Runtime (ms)",
                labels,
                [("serial", serial_vals, "#1f77b4"), (f"parallel t={args.parallel_thread}", parallel_vals, "#ff7f0e")],
                Path(args.summary_csv),
                generated_utc,
            )
            chart_manifest_rows.append(
                {
                    "chart_id": f"phase2_runtime_by_scenario_{subset}",
                    "file_path": str(by_scenario_path),
                    "phase": "phase2",
                    "what_tested": "Scenario-level serial vs parallel runtime by subset.",
                    "x_axis": "Scenario",
                    "y_axis": "Mean Query Runtime (ms)",
                    "source_summary_csv": str(Path(args.summary_csv)),
                    "generated_utc": generated_utc,
                }
            )

    write_chart_manifest(out_dir / "chart_manifest.csv", chart_manifest_rows)
    write_chart_markdown_index(out_dir / "chart_index.md", chart_manifest_rows)
    print(f"[plot_phase2_dev] wrote SVG graphs to: {out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
