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
    parser = argparse.ArgumentParser(description="Generate Phase 3 dev benchmark graphs.")
    parser.add_argument("--summary-csv", required=True, help="Summary CSV from summarize_phase3_dev.py")
    parser.add_argument("--output-dir", default="results/graphs/phase3_dev", help="Output graph directory")
    parser.add_argument("--parallel-thread", default="4", help="Parallel thread used for subset comparison")
    parser.add_argument("--optimized-thread", default="4", help="Optimized-parallel thread used for subset comparison")
    parser.add_argument(
        "--memory-csv",
        default="",
        help="Optional memory CSV from run_phase3_memory_probe.sh for memory plots",
    )
    parser.add_argument(
        "--memory-batch-utc",
        default="",
        help="Explicit batch_utc key used to select rows from --memory-csv.",
    )
    parser.add_argument(
        "--allow-partial",
        action="store_true",
        help="Allow partial chart generation when mode/subset/thread combinations are missing.",
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


def load_optional_rows(path: Path) -> List[Dict[str, str]]:
    if not path.is_file():
        return []
    with path.open("r", newline="", encoding="utf-8") as fh:
        return list(csv.DictReader(fh))


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
            "matplotlib is required to generate Phase 3 graphs.\n"
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

    generated_utc = utc_now_iso()
    runs_values = sorted({r.get("runs", "").strip() for r in rows if r.get("runs", "").strip()})
    runs_text = ",".join(runs_values) if runs_values else "unknown"
    subtitle = f"Phase 3 Dev | runs={runs_text} | parallel={args.parallel_thread} | optimized={args.optimized_thread}"
    chart_manifest_rows: List[Dict[str, str]] = []

    subset_order = ["small", "medium", "large_dev"]
    subset_display = {"small": "10k", "medium": "100k", "large_dev": "1M"}

    scenarios = sorted({r["scenario_name"] for r in rows})

    for scenario in scenarios:
        labels: List[str] = []
        serial_vals: List[float] = []
        parallel_vals: List[float] = []
        opt_serial_vals: List[float] = []
        opt_parallel_vals: List[float] = []

        for subset in subset_order:
            serial_row = row_lookup(rows, subset_label=subset, scenario_name=scenario, mode="serial", thread_count="1")
            parallel_row = row_lookup(rows, subset_label=subset, scenario_name=scenario, mode="parallel", thread_count=args.parallel_thread)
            opt_serial_row = row_lookup(rows, subset_label=subset, scenario_name=scenario, mode="optimized_serial", thread_count="1")
            opt_parallel_row = row_lookup(rows, subset_label=subset, scenario_name=scenario, mode="optimized_parallel", thread_count=args.optimized_thread)
            if not (serial_row and parallel_row and opt_serial_row and opt_parallel_row):
                if args.allow_partial:
                    continue
                raise SystemExit(
                    f"Missing mode rows for scenario={scenario}, subset={subset}, "
                    f"parallel_thread={args.parallel_thread}, optimized_thread={args.optimized_thread}."
                )

            labels.append(subset_display[subset])
            serial_vals.append(float(serial_row["query_mean_ms"]))
            parallel_vals.append(float(parallel_row["query_mean_ms"]))
            opt_serial_vals.append(float(opt_serial_row["query_mean_ms"]))
            opt_parallel_vals.append(float(opt_parallel_row["query_mean_ms"]))

        if labels:
            runtime_path = save_grouped_bar_chart(
                plt,
                out_dir,
                f"runtime_by_subset_{scenario}",
                f"Runtime by Subset ({scenario})",
                subtitle,
                "Relative runtime of serial/parallel/optimized execution modes by subset.",
                "Subset Size",
                "Mean Query Runtime (ms)",
                labels,
                [
                    ("serial", serial_vals, "#1f77b4"),
                    (f"parallel t={args.parallel_thread}", parallel_vals, "#ff7f0e"),
                    ("optimized serial", opt_serial_vals, "#2ca02c"),
                    (f"optimized parallel t={args.optimized_thread}", opt_parallel_vals, "#d62728"),
                ],
                Path(args.summary_csv),
                generated_utc,
            )
            chart_manifest_rows.append(
                {
                    "chart_id": f"phase3_runtime_by_subset_{scenario}",
                    "file_path": str(runtime_path),
                    "phase": "phase3",
                    "what_tested": "Mode runtime comparison by subset.",
                    "x_axis": "Subset Size",
                    "y_axis": "Mean Query Runtime (ms)",
                    "source_summary_csv": str(Path(args.summary_csv)),
                    "generated_utc": generated_utc,
                }
            )

    for scenario in scenarios:
        thread_rows = [
            r
            for r in rows
            if r["scenario_name"] == scenario and r["subset_label"] == "large_dev" and r["mode"] == "optimized_parallel"
        ]
        thread_rows.sort(key=lambda r: int(r["thread_count"]))
        if thread_rows:
            xlabels = [r["thread_count"] for r in thread_rows]
            runtimes = [float(r["query_mean_ms"]) for r in thread_rows]
            speedups = [float(r["speedup_vs_serial_query"]) if r["speedup_vs_serial_query"] != "nan" else 0.0 for r in thread_rows]

            opt_runtime_path = save_line_chart(
                plt,
                out_dir,
                f"optimized_runtime_vs_threads_{scenario}",
                f"Optimized Parallel Runtime vs Threads (large_dev, {scenario})",
                subtitle,
                "Optimized-parallel runtime scaling by thread count on large_dev.",
                "Thread Count",
                "Mean Query Runtime (ms)",
                xlabels,
                [("optimized_parallel", runtimes, "#d62728")],
                Path(args.summary_csv),
                generated_utc,
            )
            chart_manifest_rows.append(
                {
                    "chart_id": f"phase3_opt_runtime_vs_threads_{scenario}",
                    "file_path": str(opt_runtime_path),
                    "phase": "phase3",
                    "what_tested": "Optimized-parallel runtime scaling on large_dev.",
                    "x_axis": "Thread Count",
                    "y_axis": "Mean Query Runtime (ms)",
                    "source_summary_csv": str(Path(args.summary_csv)),
                    "generated_utc": generated_utc,
                }
            )

            opt_speedup_path = save_line_chart(
                plt,
                out_dir,
                f"optimized_speedup_vs_threads_{scenario}",
                f"Optimized Parallel Speedup vs Threads (large_dev, {scenario})",
                subtitle,
                "Optimized-parallel speedup relative to serial query runtime on large_dev.",
                "Thread Count",
                "Speedup vs serial",
                xlabels,
                [("optimized_parallel", speedups, "#2ca02c")],
                Path(args.summary_csv),
                generated_utc,
            )
            chart_manifest_rows.append(
                {
                    "chart_id": f"phase3_opt_speedup_vs_threads_{scenario}",
                    "file_path": str(opt_speedup_path),
                    "phase": "phase3",
                    "what_tested": "Optimized-parallel speedup scaling on large_dev.",
                    "x_axis": "Thread Count",
                    "y_axis": "Speedup (serial/optimized-parallel)",
                    "source_summary_csv": str(Path(args.summary_csv)),
                    "generated_utc": generated_utc,
                }
            )

    if args.memory_csv:
        memory_rows = load_optional_rows(Path(args.memory_csv))
        if memory_rows:
            selected_batch_utc = args.memory_batch_utc.strip()
            if not selected_batch_utc:
                batch_values = sorted(
                    {r.get("batch_utc", "").strip() for r in memory_rows if r.get("batch_utc", "").strip()}
                )
                if len(batch_values) == 1:
                    selected_batch_utc = batch_values[0]
                else:
                    raise SystemExit(
                        "Multiple batch_utc values present in memory CSV. "
                        "Pass --memory-batch-utc to select one explicitly."
                    )

            memory_rows = [r for r in memory_rows if r.get("batch_utc", "").strip() == selected_batch_utc]
            if not memory_rows:
                raise SystemExit(f"No memory rows found for batch_utc={selected_batch_utc} in {args.memory_csv}")

            mode_order = ["serial", "parallel", "optimized_serial", "optimized_parallel"]
            mode_labels = {
                "serial": "serial",
                "parallel": "parallel",
                "optimized_serial": "opt serial",
                "optimized_parallel": "opt parallel",
            }
            labels: List[str] = []
            rss_vals: List[float] = []
            for mode in mode_order:
                expected_thread = "1"
                if mode == "parallel":
                    expected_thread = args.parallel_thread
                elif mode == "optimized_parallel":
                    expected_thread = args.optimized_thread
                row = next(
                    (
                        r
                        for r in memory_rows
                        if r.get("mode") == mode and (r.get("thread_count", "").strip() == expected_thread)
                    ),
                    None,
                )
                if row is None:
                    continue
                labels.append(mode_labels[mode])
                rss_vals.append(float(row.get("max_rss_kb", "0") or 0.0))

            if labels:
                mem_path = save_grouped_bar_chart(
                    plt,
                    out_dir,
                    "memory_rss_by_mode",
                    "Peak RSS by Implementation Mode",
                    subtitle,
                    "Peak resident memory usage by execution mode.",
                    "Mode",
                    "Peak RSS (KB)",
                    labels,
                    [("max_rss_kb", rss_vals, "#6a3d9a")],
                    Path(args.summary_csv),
                    generated_utc,
                )
                chart_manifest_rows.append(
                    {
                        "chart_id": "phase3_memory_rss_by_mode",
                        "file_path": str(mem_path),
                        "phase": "phase3",
                        "what_tested": "Peak resident memory by mode.",
                        "x_axis": "Mode",
                        "y_axis": "Peak RSS (KB)",
                        "source_summary_csv": str(Path(args.summary_csv)),
                        "generated_utc": generated_utc,
                    }
                )

    write_chart_manifest(out_dir / "chart_manifest.csv", chart_manifest_rows)
    write_chart_markdown_index(out_dir / "chart_index.md", chart_manifest_rows)
    print(f"[plot_phase3_dev] wrote SVG graphs to: {out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
