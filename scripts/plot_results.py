#!/usr/bin/env python3
from __future__ import annotations

import csv
import math
import os
import statistics
import sys
from collections import defaultdict
from glob import glob
from typing import Dict, List, Tuple


def aggregate(values: List[float]) -> Tuple[float, float, float]:
    if not values:
        return (0.0, 0.0, 0.0)
    mean = statistics.fmean(values)
    median = statistics.median(values)
    stdev = statistics.pstdev(values) if len(values) > 1 else 0.0
    return (mean, median, stdev)


def main() -> int:
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    in_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "results", "raw")
    out_csv = (
        sys.argv[2]
        if len(sys.argv) > 2
        else os.path.join(root, "results", "tables", "benchmark_summary.csv")
    )

    files = sorted(glob(os.path.join(in_dir, "*.csv")))
    if not files:
        print(f"No benchmark CSV files found in {in_dir}")
        return 1

    grouped: Dict[Tuple[str, str, str], Dict[str, List[float]]] = defaultdict(
        lambda: {"ingest": [], "query": [], "total": []}
    )

    for path in files:
        try:
            with open(path, newline="", encoding="utf-8") as f:
                reader = csv.DictReader(f)
                if not reader.fieldnames:
                    continue
                required = {"execution_mode", "query_type", "thread_count", "ingest_ms", "query_ms", "total_ms"}
                if not required.issubset(set(reader.fieldnames)):
                    continue
                for row in reader:
                    mode = row.get("execution_mode", "unknown")
                    query_type = row.get("query_type", "unknown")
                    thread = row.get("thread_count", "1")
                    key = (mode, query_type, thread)
                    grouped[key]["ingest"].append(float(row.get("ingest_ms", "0") or 0.0))
                    grouped[key]["query"].append(float(row.get("query_ms", "0") or 0.0))
                    grouped[key]["total"].append(float(row.get("total_ms", "0") or 0.0))
        except Exception:
            continue

    if not grouped:
        print(f"No benchmark rows with expected schema found in {in_dir}")
        return 1

    serial_query_means: Dict[str, float] = {}
    for (mode, query_type, thread), metrics in grouped.items():
        if mode == "serial" and thread == "1":
            serial_query_means[query_type] = statistics.fmean(metrics["query"])

    os.makedirs(os.path.dirname(out_csv), exist_ok=True)
    with open(out_csv, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(
            [
                "execution_mode",
                "query_type",
                "thread_count",
                "runs",
                "ingest_mean_ms",
                "ingest_median_ms",
                "ingest_std_ms",
                "query_mean_ms",
                "query_median_ms",
                "query_std_ms",
                "total_mean_ms",
                "total_median_ms",
                "total_std_ms",
                "speedup_vs_serial_query",
            ]
        )

        for (mode, query_type, thread) in sorted(grouped.keys()):
            metrics = grouped[(mode, query_type, thread)]
            ingest_mean, ingest_median, ingest_std = aggregate(metrics["ingest"])
            query_mean, query_median, query_std = aggregate(metrics["query"])
            total_mean, total_median, total_std = aggregate(metrics["total"])
            runs = len(metrics["query"])

            serial_mean = serial_query_means.get(query_type, math.nan)
            speedup = (serial_mean / query_mean) if query_mean > 0 and not math.isnan(serial_mean) else math.nan

            writer.writerow(
                [
                    mode,
                    query_type,
                    thread,
                    runs,
                    f"{ingest_mean:.3f}",
                    f"{ingest_median:.3f}",
                    f"{ingest_std:.3f}",
                    f"{query_mean:.3f}",
                    f"{query_median:.3f}",
                    f"{query_std:.3f}",
                    f"{total_mean:.3f}",
                    f"{total_median:.3f}",
                    f"{total_std:.3f}",
                    f"{speedup:.3f}" if not math.isnan(speedup) else "nan",
                ]
            )

    print(f"Wrote summary: {out_csv}")
    print(f"Grouped rows: {len(grouped)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
