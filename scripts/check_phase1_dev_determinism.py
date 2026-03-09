#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import datetime as dt
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple


@dataclass
class Scenario:
    name: str
    query_type: str
    extra_args: List[str]


SCENARIOS: List[Scenario] = [
    Scenario("speed_below_8mph", "speed_below", ["--threshold", "8"]),
    Scenario(
        "time_window_full",
        "time_window",
        ["--start-epoch", "0", "--end-epoch", "4102444800"],
    ),
    Scenario(
        "borough_manhattan_speed_below_8mph",
        "borough_speed_below",
        ["--borough", "Manhattan", "--threshold", "8"],
    ),
    Scenario("top10_slowest", "top_n_slowest", ["--top-n", "10", "--min-link-samples", "1"]),
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Cross-check deterministic subset query behavior for Phase 1 dev."
    )
    parser.add_argument(
        "--binary",
        default="build/run_serial",
        help="Path to run_serial binary (default: build/run_serial).",
    )
    parser.add_argument(
        "--subsets-dir",
        default="data/subsets",
        help="Directory that contains subset CSV files.",
    )
    parser.add_argument(
        "--runs",
        type=int,
        default=3,
        help="Repeated runs per query scenario (default: 3).",
    )
    parser.add_argument(
        "--output-dir",
        default="results/raw/correctness",
        help="Directory for correctness outputs/reports.",
    )
    return parser.parse_args()


def first_match(subsets_dir: Path, pattern: str) -> Path:
    matches = sorted(subsets_dir.glob(pattern))
    if not matches:
        raise SystemExit(f"Missing subset file for pattern: {pattern}")
    return matches[0]


def load_rows(csv_path: Path) -> List[Dict[str, str]]:
    with csv_path.open("r", newline="", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        return list(reader)


def unique_int(rows: List[Dict[str, str]], key: str) -> Tuple[bool, List[int]]:
    values = sorted({int(r[key]) for r in rows})
    return len(values) == 1, values


def main() -> int:
    args = parse_args()
    root = Path.cwd()
    binary = (root / args.binary).resolve()
    subsets_dir = (root / args.subsets_dir).resolve()
    output_dir = (root / args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    if args.runs < 2:
        raise SystemExit("--runs must be >= 2 for consistency checking.")
    if not binary.is_file():
        raise SystemExit(f"run_serial binary not found: {binary}")
    if not subsets_dir.is_dir():
        raise SystemExit(f"subsets directory not found: {subsets_dir}")

    subset_map = {
        "small": first_match(subsets_dir, "*_subset_10000.csv"),
        "medium": first_match(subsets_dir, "*_subset_100000.csv"),
        "large_dev": first_match(subsets_dir, "*_subset_1000000.csv"),
    }

    ts = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    summary_csv = output_dir / f"determinism_summary_{ts}.csv"
    report_md = output_dir / f"determinism_report_{ts}.md"

    rows_out: List[Dict[str, str]] = []
    has_mismatch = False

    for subset_label, subset_path in subset_map.items():
        for scenario in SCENARIOS:
            csv_out = output_dir / f"determinism_{subset_label}_{scenario.name}_{ts}.csv"
            log_out = output_dir / f"determinism_{subset_label}_{scenario.name}_{ts}.log"

            cmd = [
                str(binary),
                "--traffic",
                str(subset_path),
                "--benchmark-runs",
                str(args.runs),
                "--dataset-label",
                subset_label,
                "--query",
                scenario.query_type,
                *scenario.extra_args,
                "--benchmark-out",
                str(csv_out),
            ]

            proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
            log_out.write_text(proc.stdout + "\n" + proc.stderr, encoding="utf-8")
            if proc.returncode != 0:
                has_mismatch = True
                rows_out.append(
                    {
                        "subset_label": subset_label,
                        "scenario_name": scenario.name,
                        "status": "FAIL",
                        "reason": "run_serial returned nonzero",
                        "result_counts": "",
                        "rows_read_values": "",
                        "rows_accepted_values": "",
                        "rows_rejected_values": "",
                        "csv_output": str(csv_out),
                        "log_output": str(log_out),
                    }
                )
                continue

            rows = load_rows(csv_out)
            if len(rows) != args.runs:
                has_mismatch = True

            result_ok, result_values = unique_int(rows, "result_count")
            read_ok, read_values = unique_int(rows, "rows_read")
            accepted_ok, accepted_values = unique_int(rows, "rows_accepted")
            rejected_ok, rejected_values = unique_int(rows, "rows_rejected")

            status = "PASS"
            reasons = []
            if len(rows) != args.runs:
                status = "FAIL"
                reasons.append(f"expected {args.runs} rows but found {len(rows)}")
            if not result_ok:
                status = "FAIL"
                reasons.append("result_count mismatch across runs")
            if not read_ok or not accepted_ok or not rejected_ok:
                status = "FAIL"
                reasons.append("ingest aggregate mismatch across runs")
            if status == "FAIL":
                has_mismatch = True

            rows_out.append(
                {
                    "subset_label": subset_label,
                    "scenario_name": scenario.name,
                    "status": status,
                    "reason": "; ".join(reasons) if reasons else "consistent",
                    "result_counts": "|".join(map(str, result_values)),
                    "rows_read_values": "|".join(map(str, read_values)),
                    "rows_accepted_values": "|".join(map(str, accepted_values)),
                    "rows_rejected_values": "|".join(map(str, rejected_values)),
                    "csv_output": str(csv_out),
                    "log_output": str(log_out),
                }
            )

    with summary_csv.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(
            fh,
            fieldnames=[
                "subset_label",
                "scenario_name",
                "status",
                "reason",
                "result_counts",
                "rows_read_values",
                "rows_accepted_values",
                "rows_rejected_values",
                "csv_output",
                "log_output",
            ],
        )
        writer.writeheader()
        writer.writerows(rows_out)

    with report_md.open("w", encoding="utf-8") as fh:
        fh.write("# Phase 1 Determinism Cross-Check Report\n\n")
        fh.write(f"- Batch UTC: `{ts}`\n")
        fh.write(f"- Binary: `{binary}`\n")
        fh.write(f"- Subsets dir: `{subsets_dir}`\n")
        fh.write(f"- Runs per scenario: `{args.runs}`\n")
        fh.write(f"- Summary CSV: `{summary_csv}`\n")
        fh.write(f"- Status: `{'FAIL' if has_mismatch else 'PASS'}`\n\n")
        fh.write("## Scenario Results\n")
        for row in rows_out:
            fh.write(
                f"- `{row['subset_label']}` / `{row['scenario_name']}`: "
                f"`{row['status']}` ({row['reason']})\n"
            )
            fh.write(f"  - result_count values: `{row['result_counts']}`\n")
            fh.write(
                "  - ingest aggregates: "
                f"rows_read=`{row['rows_read_values']}`, "
                f"rows_accepted=`{row['rows_accepted_values']}`, "
                f"rows_rejected=`{row['rows_rejected_values']}`\n"
            )
            fh.write(f"  - csv: `{row['csv_output']}`\n")
            fh.write(f"  - log: `{row['log_output']}`\n")
        fh.write("\n")
        if has_mismatch:
            fh.write(
                "Mismatches were detected. Keep corresponding CSV/log outputs for investigation.\n"
            )
        else:
            fh.write("No nondeterministic behavior detected in this batch.\n")

    print(f"[determinism] summary_csv={summary_csv}")
    print(f"[determinism] report_md={report_md}")
    print(f"[determinism] status={'FAIL' if has_mismatch else 'PASS'}")
    return 1 if has_mismatch else 0


if __name__ == "__main__":
    raise SystemExit(main())
