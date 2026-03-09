#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Dict, List, Tuple

from plot_common import utc_now_iso, write_chart_manifest, write_chart_markdown_index


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate simple Phase 2 dev benchmark graphs (SVG).")
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
        reader = csv.DictReader(fh)
        rows = list(reader)
    if not rows:
        raise SystemExit(f"Summary CSV has no rows: {path}")
    return rows


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


def normalize(values: List[float], low: float, high: float) -> List[float]:
    if not values:
        return []
    vmin = min(values)
    vmax = max(values)
    if abs(vmax - vmin) < 1e-12:
        return [0.5 * (low + high)] * len(values)
    return [high - ((v - vmin) / (vmax - vmin)) * (high - low) for v in values]


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

    # axes
    x0, y0 = left, top + plot_h
    x1, y1 = left + plot_w, top
    content.append(f'<line x1="{x0}" y1="{y0}" x2="{x1}" y2="{y0}" stroke="#333"/>\n')
    content.append(f'<line x1="{x0}" y1="{y0}" x2="{x0}" y2="{y1}" stroke="#333"/>\n')

    content.append(f'<text x="{width/2}" y="{height-20}" text-anchor="middle" font-size="14">{xlabel}</text>\n')
    content.append(f'<text x="20" y="{height/2}" text-anchor="middle" font-size="14" transform="rotate(-90 20 {height/2})">{ylabel}</text>\n')

    # x ticks
    n = len(xlabels)
    xs = []
    for i, label in enumerate(xlabels):
        x = left + (i / max(1, n - 1)) * plot_w if n > 1 else left + plot_w / 2
        xs.append(x)
        content.append(f'<line x1="{x:.2f}" y1="{y0}" x2="{x:.2f}" y2="{y0+6}" stroke="#333"/>\n')
        content.append(f'<text x="{x:.2f}" y="{y0+24}" text-anchor="middle" font-size="12">{label}</text>\n')

    all_vals = [v for _, vals, _ in series for v in vals]
    ys_norm = normalize(all_vals, top + 10, y0 - 10)
    idx = 0

    content.append(
        f'<rect x="{left}" y="{top+14}" width="{plot_w*0.64:.0f}" height="24" fill="#f7f7f7" stroke="#bbbbbb"/>\n'
    )
    content.append(
        f'<text x="{left+8}" y="{top+30}" font-size="11" fill="#333">What this tests: {what_tested}</text>\n'
    )

    legend_y = top + 42
    for name, vals, color in series:
        points = []
        for i, v in enumerate(vals):
            x = xs[i] if i < len(xs) else xs[-1]
            y = ys_norm[idx]
            idx += 1
            points.append((x, y))
        if points:
            pstr = " ".join(f"{x:.2f},{y:.2f}" for x, y in points)
            content.append(f'<polyline fill="none" stroke="{color}" stroke-width="2" points="{pstr}"/>\n')
            for x, y in points:
                content.append(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="3" fill="{color}"/>\n')

        content.append(f'<rect x="{width-220}" y="{legend_y}" width="14" height="14" fill="{color}"/>\n')
        content.append(f'<text x="{width-200}" y="{legend_y+12}" font-size="12">{name}</text>\n')
        legend_y += 20

    content.append(
        f'<text x="10" y="{height-8}" font-size="9" fill="#555">Source: {source_summary} | Generated: {generated_utc}</text>\n'
    )
    content.append(svg_footer())
    return "".join(content)


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
    left, right, top, bottom = 90, 40, 60, 120
    plot_w = width - left - right
    plot_h = height - top - bottom

    content = [svg_header(width, height)]
    content.append(f'<text x="{width/2}" y="28" text-anchor="middle" font-size="20">{title}</text>\n')
    content.append(f'<text x="{width/2}" y="48" text-anchor="middle" font-size="12" fill="#444">{subtitle}</text>\n')

    x0, y0 = left, top + plot_h
    content.append(f'<line x1="{x0}" y1="{y0}" x2="{x0+plot_w}" y2="{y0}" stroke="#333"/>\n')
    content.append(f'<line x1="{x0}" y1="{y0}" x2="{x0}" y2="{top}" stroke="#333"/>\n')
    content.append(f'<text x="{width/2}" y="{height-20}" text-anchor="middle" font-size="14">{xlabel}</text>\n')
    content.append(f'<text x="20" y="{height/2}" text-anchor="middle" font-size="14" transform="rotate(-90 20 {height/2})">{ylabel}</text>\n')

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
            f'<text x="{cat_left + (cat_width*0.35):.2f}" y="{y0+38}" text-anchor="middle" font-size="11" transform="rotate(20 {cat_left + (cat_width*0.35):.2f} {y0+38})">{cat}</text>\n'
        )
        for gi, (gname, vals, color) in enumerate(groups):
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
    for gname, _, color in groups:
        content.append(f'<rect x="{width-220}" y="{legend_y}" width="14" height="14" fill="{color}"/>\n')
        content.append(f'<text x="{width-200}" y="{legend_y+12}" font-size="12">{gname}</text>\n')
        legend_y += 20

    content.append(
        f'<text x="10" y="{height-8}" font-size="9" fill="#555">Source: {source_summary} | Generated: {generated_utc}</text>\n'
    )
    content.append(svg_footer())
    return "".join(content)


def main() -> int:
    args = parse_args()
    rows = load_rows(Path(args.summary_csv))
    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    all_threads = [t.strip() for t in args.parallel_threads.split(",") if t.strip()]
    generated_utc = utc_now_iso()
    runs_values = sorted({r.get("runs", "").strip() for r in rows if r.get("runs", "").strip()})
    runs_text = ",".join(runs_values) if runs_values else "unknown"
    subtitle = f"Phase 2 Dev | runs={runs_text} | threads={','.join(all_threads)}"
    chart_manifest_rows: List[Dict[str, str]] = []

    subset_order = ["small", "medium", "large_dev"]
    subset_display = {"small": "10k", "medium": "100k", "large_dev": "1M"}
    colors = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b"]

    scenarios = sorted({r["scenario_name"] for r in rows})

    # Runtime vs thread count + speedup vs thread count
    for scenario in scenarios:
        runtime_series = []
        speedup_series = []
        for si, subset in enumerate(subset_order):
            subset_rows = [
                r for r in rows
                if r["scenario_name"] == scenario and r["subset_label"] == subset and r["mode"] == "parallel"
            ]
            subset_rows.sort(key=lambda r: int(r["thread_count"]))
            if not subset_rows:
                continue
            xlabels = [r["thread_count"] for r in subset_rows]
            runtime_vals = [float(r["query_mean_ms"]) for r in subset_rows]
            speedup_vals = [float(r["speedup_vs_serial_query"]) if r["speedup_vs_serial_query"] != "nan" else 0.0 for r in subset_rows]
            runtime_series.append((subset_display[subset], runtime_vals, colors[si % len(colors)]))
            speedup_series.append((subset_display[subset], speedup_vals, colors[si % len(colors)]))

        if runtime_series:
            svg = line_chart(
                title=f"Query Runtime vs Thread Count ({scenario})",
                subtitle=subtitle,
                what_tested="Parallel query runtime scaling by thread count for each subset.",
                xlabel="Thread Count",
                ylabel="Mean Query Runtime (ms)",
                xlabels=xlabels,
                series=runtime_series,
                source_summary=Path(args.summary_csv),
                generated_utc=generated_utc,
            )
            runtime_path = out_dir / f"runtime_vs_threads_{scenario}.svg"
            save_svg(runtime_path, svg)
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

            svg2 = line_chart(
                title=f"Speedup vs Thread Count ({scenario})",
                subtitle=subtitle,
                what_tested="Parallel speedup relative to serial query runtime.",
                xlabel="Thread Count",
                ylabel="Speedup",
                xlabels=xlabels,
                series=speedup_series,
                source_summary=Path(args.summary_csv),
                generated_utc=generated_utc,
            )
            speedup_path = out_dir / f"speedup_vs_threads_{scenario}.svg"
            save_svg(speedup_path, svg2)
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

    # Serial vs parallel by subset
    for scenario in scenarios:
        labels: List[str] = []
        serial_vals: List[float] = []
        parallel_vals: List[float] = []
        for subset in subset_order:
            serial_row = next((r for r in rows if r["scenario_name"] == scenario and r["subset_label"] == subset and r["mode"] == "serial" and r["thread_count"] == "1"), None)
            parallel_row = next((r for r in rows if r["scenario_name"] == scenario and r["subset_label"] == subset and r["mode"] == "parallel" and r["thread_count"] == args.parallel_thread), None)
            if serial_row is None or parallel_row is None:
                if args.allow_partial:
                    continue
                raise SystemExit(
                    f"Missing serial/parallel rows for scenario={scenario}, subset={subset}, "
                    f"thread={args.parallel_thread}."
                )
            labels.append(subset_display[subset])
            serial_vals.append(float(serial_row["query_mean_ms"]))
            parallel_vals.append(float(parallel_row["query_mean_ms"]))
        if labels:
            svg = grouped_bar_chart(
                title=f"Serial vs Parallel Query Runtime by Subset ({scenario})",
                subtitle=subtitle,
                what_tested="Direct runtime comparison between serial and selected parallel thread count.",
                xlabel="Subset Size",
                ylabel="Mean Query Runtime (ms)",
                categories=labels,
                groups=[("serial", serial_vals, "#1f77b4"), (f"parallel t={args.parallel_thread}", parallel_vals, "#ff7f0e")],
                source_summary=Path(args.summary_csv),
                generated_utc=generated_utc,
            )
            comp_path = out_dir / f"serial_vs_parallel_by_subset_{scenario}.svg"
            save_svg(comp_path, svg)
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

    # Serial + all configured parallel threads by subset (default includes 16-thread view)
    for scenario in scenarios:
        labels: List[str] = []
        serial_vals: List[float] = []
        parallel_by_thread: Dict[str, List[float]] = {t: [] for t in all_threads}
        for subset in subset_order:
            serial_row = next(
                (
                    r
                    for r in rows
                    if r["scenario_name"] == scenario
                    and r["subset_label"] == subset
                    and r["mode"] == "serial"
                    and r["thread_count"] == "1"
                ),
                None,
            )
            if serial_row is None:
                continue

            candidate_rows: Dict[str, Dict[str, str]] = {}
            for t in all_threads:
                candidate = next(
                    (
                        r
                        for r in rows
                        if r["scenario_name"] == scenario
                        and r["subset_label"] == subset
                        and r["mode"] == "parallel"
                        and r["thread_count"] == t
                    ),
                    None,
                )
                if candidate is None:
                    if args.allow_partial:
                        candidate_rows = {}
                        break
                    raise SystemExit(
                        f"Missing parallel row for scenario={scenario}, subset={subset}, thread={t}."
                    )
                candidate_rows[t] = candidate

            if not candidate_rows:
                continue

            labels.append(subset_display[subset])
            serial_vals.append(float(serial_row["query_mean_ms"]))
            for t in all_threads:
                parallel_by_thread[t].append(float(candidate_rows[t]["query_mean_ms"]))

        if labels:
            groups = [("serial", serial_vals, "#1f77b4")]
            palette = ["#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b", "#17becf"]
            for i, t in enumerate(all_threads):
                groups.append((f"parallel t={t}", parallel_by_thread[t], palette[i % len(palette)]))
            svg = grouped_bar_chart(
                title=f"Serial vs Parallel (All Threads) by Subset ({scenario})",
                subtitle=subtitle,
                what_tested="Runtime comparison across serial and all configured parallel thread counts.",
                xlabel="Subset Size",
                ylabel="Mean Query Runtime (ms)",
                categories=labels,
                groups=groups,
                source_summary=Path(args.summary_csv),
                generated_utc=generated_utc,
            )
            all_threads_path = out_dir / f"serial_vs_parallel_all_threads_by_subset_{scenario}.svg"
            save_svg(all_threads_path, svg)
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

    # Runtime by scenario per subset
    for subset in subset_order:
        labels: List[str] = []
        serial_vals: List[float] = []
        parallel_vals: List[float] = []
        for scenario in scenarios:
            serial_row = next((r for r in rows if r["scenario_name"] == scenario and r["subset_label"] == subset and r["mode"] == "serial" and r["thread_count"] == "1"), None)
            parallel_row = next((r for r in rows if r["scenario_name"] == scenario and r["subset_label"] == subset and r["mode"] == "parallel" and r["thread_count"] == args.parallel_thread), None)
            if serial_row is None or parallel_row is None:
                if args.allow_partial:
                    continue
                raise SystemExit(
                    f"Missing runtime-by-scenario rows for scenario={scenario}, subset={subset}, "
                    f"thread={args.parallel_thread}."
                )
            labels.append(scenario)
            serial_vals.append(float(serial_row["query_mean_ms"]))
            parallel_vals.append(float(parallel_row["query_mean_ms"]))
        if labels:
            svg = grouped_bar_chart(
                title=f"Runtime by Scenario ({subset_display[subset]})",
                subtitle=subtitle,
                what_tested="Scenario-level runtime comparison between serial and selected parallel mode.",
                xlabel="Scenario",
                ylabel="Mean Query Runtime (ms)",
                categories=labels,
                groups=[("serial", serial_vals, "#1f77b4"), (f"parallel t={args.parallel_thread}", parallel_vals, "#ff7f0e")],
                source_summary=Path(args.summary_csv),
                generated_utc=generated_utc,
            )
            by_scenario_path = out_dir / f"runtime_by_scenario_{subset}.svg"
            save_svg(by_scenario_path, svg)
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
