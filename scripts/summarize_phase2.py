#!/usr/bin/env python3
import csv
import math
import os
import statistics
import sys
from collections import defaultdict


def load_rows(path):
    rows = []
    with open(path, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            rows.append(row)
    return rows


def runtime_ms(row):
    return float(row["query_ms"])


def grouped_stats(values):
    if not values:
        return None
    return {
        "runs": len(values),
        "mean_ms": statistics.fmean(values),
        "median_ms": statistics.median(values),
        "min_ms": min(values),
        "max_ms": max(values),
        "stddev_ms": statistics.pstdev(values) if len(values) > 1 else 0.0,
    }


def main():
    if len(sys.argv) < 3:
        print("Usage: summarize_phase2.py <raw_dir> <output_csv>")
        return 2

    raw_dir = sys.argv[1]
    output_csv = sys.argv[2]

    serial_files = {
        "speed_below": "serial_speed_below.csv",
        "time_window": "serial_time_window.csv",
        "borough_speed_below": "serial_borough_speed_below.csv",
        "summary": "serial_summary.csv",
    }
    parallel_files = {
        "speed_below": "parallel_speed_below.csv",
        "time_window": "parallel_time_window.csv",
        "borough_speed_below": "parallel_borough_speed_below.csv",
        "summary": "parallel_summary.csv",
    }

    serial_means = {}
    summary_rows = []

    for scenario, name in serial_files.items():
        path = os.path.join(raw_dir, name)
        if not os.path.exists(path):
            continue
        values = [runtime_ms(row) for row in load_rows(path)]
        stats = grouped_stats(values)
        if not stats:
            continue
        serial_means[scenario] = stats["mean_ms"]
        summary_rows.append(
            {
                "scenario": scenario,
                "mode": "serial",
                "thread_count": 1,
                **stats,
                "speedup_vs_serial": 1.0,
            }
        )

    for scenario, name in parallel_files.items():
        path = os.path.join(raw_dir, name)
        if not os.path.exists(path):
            continue

        by_thread = defaultdict(list)
        for row in load_rows(path):
            by_thread[int(row["thread_count"])].append(runtime_ms(row))

        for thread_count in sorted(by_thread.keys()):
            stats = grouped_stats(by_thread[thread_count])
            if not stats:
                continue
            speedup = math.nan
            if scenario in serial_means and stats["mean_ms"] > 0:
                speedup = serial_means[scenario] / stats["mean_ms"]
            summary_rows.append(
                {
                    "scenario": scenario,
                    "mode": "parallel",
                    "thread_count": thread_count,
                    **stats,
                    "speedup_vs_serial": speedup,
                }
            )

    os.makedirs(os.path.dirname(output_csv), exist_ok=True)
    with open(output_csv, "w", newline="", encoding="utf-8") as f:
        fieldnames = [
            "scenario",
            "mode",
            "thread_count",
            "runs",
            "mean_ms",
            "median_ms",
            "min_ms",
            "max_ms",
            "stddev_ms",
            "speedup_vs_serial",
        ]
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for row in summary_rows:
            writer.writerow(row)

    print(f"Wrote phase2 summary: {output_csv}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
