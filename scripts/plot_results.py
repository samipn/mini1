#!/usr/bin/env python3
import csv
import os
import statistics
import sys
from glob import glob


def aggregate(values):
    if not values:
        return (0.0, 0.0, 0.0)
    mean = statistics.fmean(values)
    median = statistics.median(values)
    stdev = statistics.pstdev(values) if len(values) > 1 else 0.0
    return (mean, median, stdev)


def main() -> int:
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    in_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "results", "raw")
    out_csv = sys.argv[2] if len(sys.argv) > 2 else os.path.join(root, "results", "raw", "serial_summary.csv")

    files = sorted(glob(os.path.join(in_dir, "serial_*.csv")))
    if not files:
        print(f"No serial benchmark CSV files found in {in_dir}")
        return 1

    rows = []
    for path in files:
        with open(path, newline="", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            ingest = []
            query = []
            total = []
            query_type = ""
            dataset_label = ""
            runs = 0
            for r in reader:
                runs += 1
                query_type = r.get("query_type", "")
                dataset_label = r.get("dataset_label", "")
                ingest.append(float(r.get("ingest_ms", "0") or 0.0))
                query.append(float(r.get("query_ms", "0") or 0.0))
                total.append(float(r.get("total_ms", "0") or 0.0))

            ingest_mean, ingest_median, ingest_std = aggregate(ingest)
            query_mean, query_median, query_std = aggregate(query)
            total_mean, total_median, total_std = aggregate(total)

            rows.append(
                {
                    "file": os.path.basename(path),
                    "dataset_label": dataset_label,
                    "query_type": query_type,
                    "runs": runs,
                    "ingest_mean_ms": f"{ingest_mean:.3f}",
                    "ingest_median_ms": f"{ingest_median:.3f}",
                    "ingest_std_ms": f"{ingest_std:.3f}",
                    "query_mean_ms": f"{query_mean:.3f}",
                    "query_median_ms": f"{query_median:.3f}",
                    "query_std_ms": f"{query_std:.3f}",
                    "total_mean_ms": f"{total_mean:.3f}",
                    "total_median_ms": f"{total_median:.3f}",
                    "total_std_ms": f"{total_std:.3f}",
                }
            )

    os.makedirs(os.path.dirname(out_csv), exist_ok=True)
    with open(out_csv, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "file",
                "dataset_label",
                "query_type",
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
            ],
        )
        writer.writeheader()
        writer.writerows(rows)

    print(f"Wrote summary: {out_csv}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
