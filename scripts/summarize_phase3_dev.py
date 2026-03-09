#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import math
import statistics
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Tuple


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Summarize Phase 3 dev benchmark batch outputs.")
    parser.add_argument("--manifest", required=True, help="Batch manifest CSV from run_phase3_dev_benchmarks.sh")
    parser.add_argument("--output-dir", default="results/tables/phase3_dev", help="Output directory")
    parser.add_argument("--output-file", default="phase3_dev_summary.csv", help="Output CSV filename")
    parser.add_argument(
        "--parallel-thread-baseline",
        default="4",
        help="Parallel thread count baseline for speedup_vs_parallel",
    )
    return parser.parse_args()


def summarize(values: List[float]) -> Tuple[float, float, float, float, float]:
    mean_v = statistics.fmean(values)
    median_v = statistics.median(values)
    min_v = min(values)
    max_v = max(values)
    stddev_v = statistics.stdev(values) if len(values) >= 2 else 0.0
    return mean_v, median_v, min_v, max_v, stddev_v


def speedup(base_ms: float, candidate_ms: float) -> float:
    if candidate_ms <= 0.0:
        return math.nan
    return base_ms / candidate_ms


def load_manifest(path: Path) -> List[Dict[str, str]]:
    if not path.is_file():
        raise SystemExit(f"Manifest not found: {path}")
    with path.open("r", newline="", encoding="utf-8") as fh:
        rows = list(csv.DictReader(fh))
    if not rows:
        raise SystemExit(f"Manifest has no rows: {path}")
    return rows


def main() -> int:
    args = parse_args()
    manifest_path = Path(args.manifest)
    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / args.output_file

    manifest_rows = load_manifest(manifest_path)

    grouped: Dict[Tuple[str, str, str, str, str], Dict[str, List[float]]] = defaultdict(
        lambda: {
            "ingest_ms": [],
            "query_ms": [],
            "total_ms": [],
            "rows_accepted": [],
            "result_count": [],
            "avg_speed": [],
            "avg_travel": [],
            "serial_match": [],
            "records_per_second": [],
        }
    )

    serial_baseline_means: Dict[Tuple[str, str], float] = {}
    parallel_baseline_means: Dict[Tuple[str, str], float] = {}

    for entry in manifest_rows:
        output_csv = Path(entry["output_csv"])
        if not output_csv.is_file():
            raise SystemExit(f"Missing output CSV from manifest: {output_csv}")

        subset = entry["subset_label"].strip()
        scenario = entry["scenario_name"].strip()
        mode = entry["mode"].strip()
        optimization_step = entry.get("optimization_step", "unknown").strip() or "unknown"

        with output_csv.open("r", newline="", encoding="utf-8") as fh:
            for row in csv.DictReader(fh):
                thread_count = row["thread_count"].strip()
                key = (subset, scenario, mode, thread_count, optimization_step)

                ingest_ms = float(row["ingest_ms"])
                query_ms = float(row["query_ms"])
                total_ms = float(row["total_ms"])
                rows_accepted = float(row["rows_accepted"])
                rps = rows_accepted / (total_ms / 1000.0) if total_ms > 0.0 else math.nan

                grouped[key]["ingest_ms"].append(ingest_ms)
                grouped[key]["query_ms"].append(query_ms)
                grouped[key]["total_ms"].append(total_ms)
                grouped[key]["rows_accepted"].append(rows_accepted)
                grouped[key]["result_count"].append(float(row["result_count"]))
                grouped[key]["avg_speed"].append(float(row["average_speed_mph"]))
                grouped[key]["avg_travel"].append(float(row["average_travel_time_seconds"]))
                grouped[key]["serial_match"].append(1.0 if row["serial_match"] == "true" else 0.0)
                grouped[key]["records_per_second"].append(rps)

    for (subset, scenario, mode, thread_count, _), metrics in grouped.items():
        mean_q = statistics.fmean(metrics["query_ms"])
        if mode == "serial" and thread_count == "1":
            serial_baseline_means[(subset, scenario)] = mean_q
        if mode == "parallel" and thread_count == args.parallel_thread_baseline:
            parallel_baseline_means[(subset, scenario)] = mean_q

    with out_path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.writer(fh)
        writer.writerow(
            [
                "subset_label",
                "scenario_name",
                "mode",
                "thread_count",
                "optimization_step",
                "runs",
                "ingest_mean_ms",
                "ingest_median_ms",
                "ingest_min_ms",
                "ingest_max_ms",
                "ingest_stddev_ms",
                "query_mean_ms",
                "query_median_ms",
                "query_min_ms",
                "query_max_ms",
                "query_stddev_ms",
                "total_mean_ms",
                "total_median_ms",
                "total_min_ms",
                "total_max_ms",
                "total_stddev_ms",
                "records_per_second_mean",
                "serial_match_rate",
                "result_count_mean",
                "avg_speed_mean",
                "avg_travel_time_mean",
                "speedup_vs_serial_query",
                "speedup_vs_parallel_query",
            ]
        )

        for key in sorted(grouped.keys()):
            subset, scenario, mode, thread_count, optimization_step = key
            metrics = grouped[key]
            ingest = summarize(metrics["ingest_ms"])
            query = summarize(metrics["query_ms"])
            total = summarize(metrics["total_ms"])
            rps = summarize(metrics["records_per_second"])
            runs = len(metrics["query_ms"])

            serial_sp = speedup(serial_baseline_means.get((subset, scenario), math.nan), query[0])
            parallel_sp = speedup(parallel_baseline_means.get((subset, scenario), math.nan), query[0])

            row = [subset, scenario, mode, thread_count, optimization_step, runs]
            for stats in (ingest, query, total):
                row.extend([f"{value:.6f}" for value in stats])
            row.append(f"{rps[0]:.6f}")
            row.append(f"{statistics.fmean(metrics['serial_match']):.6f}")
            row.append(f"{statistics.fmean(metrics['result_count']):.6f}")
            row.append(f"{statistics.fmean(metrics['avg_speed']):.6f}")
            row.append(f"{statistics.fmean(metrics['avg_travel']):.6f}")
            row.append(f"{serial_sp:.6f}" if not math.isnan(serial_sp) else "nan")
            row.append(f"{parallel_sp:.6f}" if not math.isnan(parallel_sp) else "nan")
            writer.writerow(row)

    print(f"[summarize_phase3_dev] wrote summary: {out_path}")
    print(f"[summarize_phase3_dev] groups: {len(grouped)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
