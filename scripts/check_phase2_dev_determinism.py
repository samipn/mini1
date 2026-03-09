#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import datetime as dt
import math
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List


@dataclass
class Scenario:
    name: str
    query_type: str
    extra_args: List[str]


def parse_scenarios(path: Path) -> List[Scenario]:
    if not path.is_file():
        raise SystemExit(f"Scenario file not found: {path}")
    scenarios: List[Scenario] = []
    with path.open("r", newline="", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        for row in reader:
            args: List[str] = []
            if row.get("threshold", ""):
                args.extend(["--threshold", row["threshold"]])
            if row.get("start_epoch", ""):
                args.extend(["--start-epoch", row["start_epoch"]])
            if row.get("end_epoch", ""):
                args.extend(["--end-epoch", row["end_epoch"]])
            if row.get("borough", ""):
                args.extend(["--borough", row["borough"]])
            scenarios.append(Scenario(row["scenario_name"], row["query_type"], args))
    if not scenarios:
        raise SystemExit(f"No scenarios in {path}")
    return scenarios


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check Phase 2 dev parallel determinism/stability on subset data."
    )
    parser.add_argument("--binary", default="build/run_parallel", help="Path to run_parallel binary")
    parser.add_argument(
        "--dataset",
        default="data/subsets/i4gi-tjb9_subset_100000.csv",
        help="Subset CSV path for repeated determinism checks",
    )
    parser.add_argument(
        "--scenario-file",
        default="configs/phase2_dev_scenarios.conf",
        help="Scenario definition file",
    )
    parser.add_argument("--threads", default="1,2,4,8,16", help="Thread list")
    parser.add_argument("--runs", type=int, default=5, help="Repeated runs per scenario")
    parser.add_argument(
        "--output-dir",
        default="results/raw/phase2_dev/stability",
        help="Directory for stability outputs",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = Path.cwd()
    binary = (root / args.binary).resolve()
    dataset = (root / args.dataset).resolve()
    scenario_file = (root / args.scenario_file).resolve()
    output_dir = (root / args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    if args.runs < 2:
        raise SystemExit("--runs must be >= 2")
    if not binary.is_file():
        raise SystemExit(f"run_parallel binary not found: {binary}")
    if not dataset.is_file():
        raise SystemExit(f"dataset not found: {dataset}")

    scenarios = parse_scenarios(scenario_file)

    ts = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    summary_csv = output_dir / f"stability_summary_{ts}.csv"
    report_md = output_dir / f"stability_report_{ts}.md"

    rows_out: List[Dict[str, str]] = []
    has_failure = False

    for scenario in scenarios:
        out_csv = output_dir / f"stability_{scenario.name}_{ts}.csv"
        log_file = output_dir / f"stability_{scenario.name}_{ts}.log"

        cmd = [
            str(binary),
            "--traffic",
            str(dataset),
            "--query",
            scenario.query_type,
            "--thread-list",
            args.threads,
            "--benchmark-runs",
            str(args.runs),
            "--benchmark-out",
            str(out_csv),
            "--validate-serial",
            *scenario.extra_args,
        ]

        proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
        log_file.write_text(proc.stdout + "\n" + proc.stderr, encoding="utf-8")

        if proc.returncode != 0:
            has_failure = True
            rows_out.append(
                {
                    "scenario_name": scenario.name,
                    "thread_count": "n/a",
                    "status": "FAIL",
                    "reason": "run_parallel returned nonzero",
                    "result_count_values": "",
                    "avg_speed_range": "",
                    "avg_travel_time_range": "",
                    "query_mean_ms": "",
                    "query_stddev_ms": "",
                    "csv_output": str(out_csv),
                    "log_output": str(log_file),
                }
            )
            continue

        by_thread: Dict[str, Dict[str, List[float]]] = {}
        with out_csv.open("r", newline="", encoding="utf-8") as fh:
            reader = csv.DictReader(fh)
            for row in reader:
                t = row["thread_count"]
                if t not in by_thread:
                    by_thread[t] = {
                        "result_count": [],
                        "avg_speed": [],
                        "avg_travel": [],
                        "query_ms": [],
                    }
                by_thread[t]["result_count"].append(float(row["result_count"]))
                by_thread[t]["avg_speed"].append(float(row["average_speed_mph"]))
                by_thread[t]["avg_travel"].append(float(row["average_travel_time_seconds"]))
                by_thread[t]["query_ms"].append(float(row["query_ms"]))

        for t in sorted(by_thread.keys(), key=lambda x: int(x)):
            m = by_thread[t]
            counts = sorted({int(v) for v in m["result_count"]})
            avg_speed_min = min(m["avg_speed"])
            avg_speed_max = max(m["avg_speed"])
            avg_travel_min = min(m["avg_travel"])
            avg_travel_max = max(m["avg_travel"])
            query_mean = sum(m["query_ms"]) / len(m["query_ms"])
            q_variance = sum((v - query_mean) ** 2 for v in m["query_ms"]) / len(m["query_ms"])
            query_stddev = math.sqrt(q_variance)

            status = "PASS"
            reason = "stable"
            if len(counts) != 1:
                status = "FAIL"
                reason = "result_count drift across repeated runs"
                has_failure = True

            rows_out.append(
                {
                    "scenario_name": scenario.name,
                    "thread_count": t,
                    "status": status,
                    "reason": reason,
                    "result_count_values": "|".join(str(c) for c in counts),
                    "avg_speed_range": f"{avg_speed_min:.9f}..{avg_speed_max:.9f}",
                    "avg_travel_time_range": f"{avg_travel_min:.9f}..{avg_travel_max:.9f}",
                    "query_mean_ms": f"{query_mean:.6f}",
                    "query_stddev_ms": f"{query_stddev:.6f}",
                    "csv_output": str(out_csv),
                    "log_output": str(log_file),
                }
            )

    with summary_csv.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(
            fh,
            fieldnames=[
                "scenario_name",
                "thread_count",
                "status",
                "reason",
                "result_count_values",
                "avg_speed_range",
                "avg_travel_time_range",
                "query_mean_ms",
                "query_stddev_ms",
                "csv_output",
                "log_output",
            ],
        )
        writer.writeheader()
        writer.writerows(rows_out)

    with report_md.open("w", encoding="utf-8") as fh:
        fh.write("# Phase 2 Dev Stability Report\n\n")
        fh.write(f"- Batch UTC: `{ts}`\n")
        fh.write(f"- Binary: `{binary}`\n")
        fh.write(f"- Dataset: `{dataset}`\n")
        fh.write(f"- Threads: `{args.threads}`\n")
        fh.write(f"- Runs per scenario: `{args.runs}`\n")
        fh.write(f"- Summary CSV: `{summary_csv}`\n")
        fh.write(f"- Status: `{'FAIL' if has_failure else 'PASS'}`\n\n")
        fh.write("## Results\n")
        for row in rows_out:
            fh.write(
                f"- `{row['scenario_name']}` thread `{row['thread_count']}`: "
                f"`{row['status']}` ({row['reason']})\n"
            )
            fh.write(f"  - result_count values: `{row['result_count_values']}`\n")
            fh.write(f"  - avg_speed range: `{row['avg_speed_range']}`\n")
            fh.write(f"  - avg_travel_time range: `{row['avg_travel_time_range']}`\n")
            fh.write(
                f"  - query mean/stddev ms: `{row['query_mean_ms']}` / `{row['query_stddev_ms']}`\n"
            )
            fh.write(f"  - csv: `{row['csv_output']}`\n")
            fh.write(f"  - log: `{row['log_output']}`\n")
        fh.write("\n")
        if has_failure:
            fh.write("Detected instability or mismatches in repeated runs.\n")
        else:
            fh.write("No nondeterministic result corruption detected in this batch.\n")

    print(f"[phase2-stability] summary_csv={summary_csv}")
    print(f"[phase2-stability] report_md={report_md}")
    print(f"[phase2-stability] status={'FAIL' if has_failure else 'PASS'}")
    return 1 if has_failure else 0


if __name__ == "__main__":
    raise SystemExit(main())
