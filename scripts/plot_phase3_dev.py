#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Dict, List, Tuple

from plot_common import utc_now_iso, write_chart_manifest


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate simple Phase 3 dev benchmark graphs (SVG).")
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


def svg_header(width: int, height: int) -> str:
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}">\n'
        '<rect width="100%" height="100%" fill="#ffffff"/>\n'
    )


def svg_footer() -> str:
    return "</svg>\n"


def save_svg(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")


def grouped_bar_chart(
    title: str,
    subtitle: str,
    what_tested: str,
    xlabel: str,
    ylabel: str,
    categories: List[str],
    groups: List[Tuple[str, List[float], str]],
    source_summary: Path,
    generated_utc: str,
) -> str:
    width, height = 960, 540
    left, right, top, bottom = 90, 40, 60, 130
    plot_w = width - left - right
    plot_h = height - top - bottom

    content = [svg_header(width, height)]
    content.append(f'<text x="{width/2}" y="28" text-anchor="middle" font-size="20">{title}</text>\n')
    content.append(f'<text x="{width/2}" y="48" text-anchor="middle" font-size="12" fill="#444">{subtitle}</text>\n')

    x0, y0 = left, top + plot_h
    content.append(f'<line x1="{x0}" y1="{y0}" x2="{x0+plot_w}" y2="{y0}" stroke="#333"/>\n')
    content.append(f'<line x1="{x0}" y1="{y0}" x2="{x0}" y2="{top}" stroke="#333"/>\n')
    content.append(f'<text x="{width/2}" y="{height-20}" text-anchor="middle" font-size="14">{xlabel}</text>\n')
    content.append(
        f'<text x="20" y="{height/2}" text-anchor="middle" font-size="14" transform="rotate(-90 20 {height/2})">{ylabel}</text>\n'
    )

    all_vals = [v for _, vals, _ in groups for v in vals]
    vmax = max(all_vals) if all_vals else 1.0
    vmax = vmax if vmax > 0 else 1.0

    num_cat = len(categories)
    num_grp = len(groups)
    cat_width = plot_w / max(1, num_cat)
    bar_width = (cat_width * 0.7) / max(1, num_grp)

    for ci, cat in enumerate(categories):
        cat_left = left + ci * cat_width + (cat_width * 0.15)
        content.append(
            f'<text x="{cat_left + (cat_width*0.35):.2f}" y="{y0+36}" text-anchor="middle" font-size="11" transform="rotate(20 {cat_left + (cat_width*0.35):.2f} {y0+36})">{cat}</text>\n'
        )
        for gi, (_, vals, color) in enumerate(groups):
            if ci >= len(vals):
                continue
            val = vals[ci]
            h = (val / vmax) * (plot_h - 10)
            x = cat_left + gi * bar_width
            y = y0 - h
            content.append(f'<rect x="{x:.2f}" y="{y:.2f}" width="{bar_width-2:.2f}" height="{h:.2f}" fill="{color}"/>\n')

    content.append(
        f'<rect x="{left}" y="{top+14}" width="{plot_w*0.64:.0f}" height="24" fill="#f7f7f7" stroke="#bbbbbb"/>\n'
    )
    content.append(
        f'<text x="{left+8}" y="{top+30}" font-size="11" fill="#333">What this tests: {what_tested}</text>\n'
    )

    legend_y = top + 42
    for name, _, color in groups:
        content.append(f'<rect x="{width-260}" y="{legend_y}" width="14" height="14" fill="{color}"/>\n')
        content.append(f'<text x="{width-240}" y="{legend_y+12}" font-size="12">{name}</text>\n')
        legend_y += 20

    content.append(
        f'<text x="10" y="{height-8}" font-size="9" fill="#555">Source: {source_summary} | Generated: {generated_utc}</text>\n'
    )
    content.append(svg_footer())
    return "".join(content)


def line_chart(
    title: str,
    subtitle: str,
    what_tested: str,
    xlabel: str,
    ylabel: str,
    xlabels: List[str],
    series: List[Tuple[str, List[float], str]],
    source_summary: Path,
    generated_utc: str,
) -> str:
    width, height = 960, 540
    left, right, top, bottom = 90, 40, 60, 90
    plot_w = width - left - right
    plot_h = height - top - bottom

    content = [svg_header(width, height)]
    content.append(f'<text x="{width/2}" y="28" text-anchor="middle" font-size="20">{title}</text>\n')
    content.append(f'<text x="{width/2}" y="48" text-anchor="middle" font-size="12" fill="#444">{subtitle}</text>\n')

    x0, y0 = left, top + plot_h
    content.append(f'<line x1="{x0}" y1="{y0}" x2="{x0+plot_w}" y2="{y0}" stroke="#333"/>\n')
    content.append(f'<line x1="{x0}" y1="{y0}" x2="{x0}" y2="{top}" stroke="#333"/>\n')
    content.append(f'<text x="{width/2}" y="{height-20}" text-anchor="middle" font-size="14">{xlabel}</text>\n')
    content.append(
        f'<text x="20" y="{height/2}" text-anchor="middle" font-size="14" transform="rotate(-90 20 {height/2})">{ylabel}</text>\n'
    )

    xs = []
    for i, label in enumerate(xlabels):
        x = left + (i / max(1, len(xlabels) - 1)) * plot_w if len(xlabels) > 1 else left + plot_w / 2
        xs.append(x)
        content.append(f'<line x1="{x:.2f}" y1="{y0}" x2="{x:.2f}" y2="{y0+6}" stroke="#333"/>\n')
        content.append(f'<text x="{x:.2f}" y="{y0+24}" text-anchor="middle" font-size="12">{label}</text>\n')

    all_vals = [v for _, vals, _ in series for v in vals]
    vmin = min(all_vals) if all_vals else 0.0
    vmax = max(all_vals) if all_vals else 1.0
    if abs(vmax - vmin) < 1e-12:
        vmax = vmin + 1.0

    content.append(
        f'<rect x="{left}" y="{top+14}" width="{plot_w*0.64:.0f}" height="24" fill="#f7f7f7" stroke="#bbbbbb"/>\n'
    )
    content.append(
        f'<text x="{left+8}" y="{top+30}" font-size="11" fill="#333">What this tests: {what_tested}</text>\n'
    )

    legend_y = top + 42
    for name, vals, color in series:
        points: List[Tuple[float, float]] = []
        for i, v in enumerate(vals):
            x = xs[i]
            y = top + 10 + (1.0 - ((v - vmin) / (vmax - vmin))) * (plot_h - 20)
            points.append((x, y))
        if points:
            pstr = " ".join(f"{x:.2f},{y:.2f}" for x, y in points)
            content.append(f'<polyline fill="none" stroke="{color}" stroke-width="2" points="{pstr}"/>\n')
            for x, y in points:
                content.append(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="3" fill="{color}"/>\n')

        content.append(f'<rect x="{width-260}" y="{legend_y}" width="14" height="14" fill="{color}"/>\n')
        content.append(f'<text x="{width-240}" y="{legend_y+12}" font-size="12">{name}</text>\n')
        legend_y += 20

    content.append(
        f'<text x="10" y="{height-8}" font-size="9" fill="#555">Source: {source_summary} | Generated: {generated_utc}</text>\n'
    )
    content.append(svg_footer())
    return "".join(content)


def row_lookup(rows: List[Dict[str, str]], **kwargs: str) -> Dict[str, str] | None:
    for row in rows:
        if all(row.get(k) == v for k, v in kwargs.items()):
            return row
    return None


def main() -> int:
    args = parse_args()
    rows = load_rows(Path(args.summary_csv))
    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
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
            parallel_row = row_lookup(
                rows,
                subset_label=subset,
                scenario_name=scenario,
                mode="parallel",
                thread_count=args.parallel_thread,
            )
            opt_serial_row = row_lookup(
                rows,
                subset_label=subset,
                scenario_name=scenario,
                mode="optimized_serial",
                thread_count="1",
            )
            opt_parallel_row = row_lookup(
                rows,
                subset_label=subset,
                scenario_name=scenario,
                mode="optimized_parallel",
                thread_count=args.optimized_thread,
            )
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
            svg = grouped_bar_chart(
                title=f"Runtime by Subset ({scenario})",
                subtitle=subtitle,
                what_tested="Relative runtime of serial/parallel/optimized execution modes by subset.",
                xlabel="Subset Size",
                ylabel="Mean Query Runtime (ms)",
                categories=labels,
                groups=[
                    ("serial", serial_vals, "#1f77b4"),
                    (f"parallel t={args.parallel_thread}", parallel_vals, "#ff7f0e"),
                    ("optimized serial", opt_serial_vals, "#2ca02c"),
                    (f"optimized parallel t={args.optimized_thread}", opt_parallel_vals, "#d62728"),
                ],
                source_summary=Path(args.summary_csv),
                generated_utc=generated_utc,
            )
            runtime_path = out_dir / f"runtime_by_subset_{scenario}.svg"
            save_svg(runtime_path, svg)
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

            svg_runtime = line_chart(
                title=f"Optimized Parallel Runtime vs Threads (large_dev, {scenario})",
                subtitle=subtitle,
                what_tested="Optimized-parallel runtime scaling by thread count on large_dev.",
                xlabel="Thread Count",
                ylabel="Mean Query Runtime (ms)",
                xlabels=xlabels,
                series=[("optimized_parallel", runtimes, "#d62728")],
                source_summary=Path(args.summary_csv),
                generated_utc=generated_utc,
            )
            opt_runtime_path = out_dir / f"optimized_runtime_vs_threads_{scenario}.svg"
            save_svg(opt_runtime_path, svg_runtime)
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

            svg_speedup = line_chart(
                title=f"Optimized Parallel Speedup vs Threads (large_dev, {scenario})",
                subtitle=subtitle,
                what_tested="Optimized-parallel speedup relative to serial query runtime on large_dev.",
                xlabel="Thread Count",
                ylabel="Speedup vs serial",
                xlabels=xlabels,
                series=[("optimized_parallel", speedups, "#2ca02c")],
                source_summary=Path(args.summary_csv),
                generated_utc=generated_utc,
            )
            opt_speedup_path = out_dir / f"optimized_speedup_vs_threads_{scenario}.svg"
            save_svg(opt_speedup_path, svg_speedup)
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
                row = next((r for r in memory_rows if r.get("mode") == mode), None)
                if row is None:
                    continue
                labels.append(mode_labels[mode])
                rss_vals.append(float(row.get("max_rss_kb", "0") or 0.0))

            if labels:
                svg_mem = grouped_bar_chart(
                    title="Peak RSS by Implementation Mode",
                    subtitle=subtitle,
                    what_tested="Peak resident memory usage by execution mode.",
                    xlabel="Mode",
                    ylabel="Peak RSS (KB)",
                    categories=labels,
                    groups=[("max_rss_kb", rss_vals, "#6a3d9a")],
                    source_summary=Path(args.summary_csv),
                    generated_utc=generated_utc,
                )
                mem_path = out_dir / "memory_rss_by_mode.svg"
                save_svg(mem_path, svg_mem)
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
    print(f"[plot_phase3_dev] wrote SVG graphs to: {out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
