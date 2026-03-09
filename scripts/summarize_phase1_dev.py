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
    parser = argparse.ArgumentParser(
        description="Summarize Phase 1 dev benchmark CSV files."
    )
    parser.add_argument(
        "--input-dir",
        default="results/raw/phase1_dev",
        help="Directory containing raw benchmark CSV files.",
    )
    parser.add_argument(
        "--output-dir",
        default="results/tables/phase1_dev",
        help="Directory to write summary tables.",
    )
    parser.add_argument(
        "--output-file",
        default="phase1_dev_summary.csv",
        help="Output summary CSV file name.",
    )
    parser.add_argument(
        "--include-all-labels",
        action="store_true",
        help="Include every dataset_label found in raw files.",
    )
    parser.add_argument(
        "--labels",
        nargs="+",
        default=["small", "medium", "large_dev"],
        help="Dataset labels to include when --include-all-labels is not set.",
    )
    return parser.parse_args()


def summarize(values: List[float]) -> Tuple[float, float, float, float, float]:
    mean_v = statistics.fmean(values)
    median_v = statistics.median(values)
    min_v = min(values)
    max_v = max(values)
    stddev_v = statistics.stdev(values) if len(values) >= 2 else 0.0
    return mean_v, median_v, min_v, max_v, stddev_v


def main() -> int:
    args = parse_args()
    input_dir = Path(args.input_dir)
    output_dir = Path(args.output_dir)

    if not input_dir.is_dir():
        raise SystemExit(f"Input directory not found: {input_dir}")

    raw_files = sorted(input_dir.glob("*.csv"))
    raw_files = [p for p in raw_files if not p.name.startswith("batch_")]
    if not raw_files:
        raise SystemExit(f"No raw benchmark CSV files found in: {input_dir}")

    allowed_labels = set(args.labels)
    grouped: Dict[Tuple[str, str], Dict[str, List[float]]] = defaultdict(
        lambda: {
            "ingest_ms": [],
            "query_ms": [],
            "total_ms": [],
            "records_per_second": [],
            "runs": [],
        }
    )

    for csv_path in raw_files:
        with csv_path.open("r", newline="", encoding="utf-8") as fh:
            reader = csv.DictReader(fh)
            required = {
                "dataset_label",
                "query_type",
                "run_number",
                "ingest_ms",
                "query_ms",
                "total_ms",
                "rows_accepted",
            }
            missing = required.difference(set(reader.fieldnames or []))
            if missing:
                raise SystemExit(
                    f"Missing required columns in {csv_path.name}: {sorted(missing)}"
                )

            for row in reader:
                dataset_label = row["dataset_label"].strip()
                if not args.include_all_labels and dataset_label not in allowed_labels:
                    continue

                query_type = row["query_type"].strip()
                key = (dataset_label, query_type)

                ingest_ms = float(row["ingest_ms"])
                query_ms = float(row["query_ms"])
                total_ms = float(row["total_ms"])
                rows_accepted = float(row["rows_accepted"])
                rps = rows_accepted / (total_ms / 1000.0) if total_ms > 0 else math.nan

                grouped[key]["ingest_ms"].append(ingest_ms)
                grouped[key]["query_ms"].append(query_ms)
                grouped[key]["total_ms"].append(total_ms)
                grouped[key]["records_per_second"].append(rps)
                grouped[key]["runs"].append(float(row["run_number"]))

    if not grouped:
        raise SystemExit("No matching rows found after applying label filters.")

    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / args.output_file

    with out_path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.writer(fh)
        writer.writerow(
            [
                "dataset_label",
                "query_type",
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
                "records_per_second_median",
                "records_per_second_min",
                "records_per_second_max",
                "records_per_second_stddev",
            ]
        )

        for (dataset_label, query_type) in sorted(grouped.keys()):
            metrics = grouped[(dataset_label, query_type)]
            ingest_stats = summarize(metrics["ingest_ms"])
            query_stats = summarize(metrics["query_ms"])
            total_stats = summarize(metrics["total_ms"])
            rps_stats = summarize(metrics["records_per_second"])
            runs = len(metrics["total_ms"])

            row = [dataset_label, query_type, runs]
            for stats in (ingest_stats, query_stats, total_stats, rps_stats):
                row.extend([f"{v:.6f}" for v in stats])
            writer.writerow(row)

    print(f"[summarize_phase1_dev] wrote summary: {out_path}")
    print(f"[summarize_phase1_dev] groups: {len(grouped)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
