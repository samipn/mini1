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
        description="Check Phase 3 optimized determinism/stability on subset data."
    )
    parser.add_argument("--binary", default="build/run_optimized", help="Path to run_optimized binary")
    parser.add_argument(
        "--dataset",
        default="data/subsets/i4gi-tjb9_subset_100000.csv",
        help="Subset CSV path for repeated determinism checks",
    )
    parser.add_argument(
        "--scenario-file",
        default="configs/phase3_dev_scenarios.conf",
        help="Scenario definition file",
    )
    parser.add_argument("--threads", default="1,2,4,8,16", help="Thread list for optimized parallel")
    parser.add_argument("--runs", type=int, default=5, help="Repeated runs per scenario")
    parser.add_argument(
        "--output-dir",
        default="results/raw/phase3_dev/stability",
        help="Directory for stability outputs",
    )
    return parser.parse_args()


def mean(values: List[float]) -> float:
    return sum(values) / len(values) if values else 0.0


def stddev(values: List[float]) -> float:
    if not values:
        return 0.0
    m = mean(values)
    return math.sqrt(sum((v - m) ** 2 for v in values) / len(values))


def evaluate_csv(path: Path) -> Dict[str, Dict[str, List[float]]]:
    by_thread: Dict[str, Dict[str, List[float]]] = {}
    with path.open("r", newline="", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        for row in reader:
            t = row["thread_count"]
            by_thread.setdefault(
                t,
                {"result_count": [], "avg_speed": [], "avg_travel": [], "query_ms": [], "serial_match": []},
            )
            by_thread[t]["result_count"].append(float(row["result_count"]))
            by_thread[t]["avg_speed"].append(float(row["average_speed_mph"]))
            by_thread[t]["avg_travel"].append(float(row["average_travel_time_seconds"]))
            by_thread[t]["query_ms"].append(float(row["query_ms"]))
            by_thread[t]["serial_match"].append(1.0 if row["serial_match"] == "true" else 0.0)
    return by_thread


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
        raise SystemExit(f"run_optimized binary not found: {binary}")
    if not dataset.is_file():
        raise SystemExit(f"dataset not found: {dataset}")

    scenarios = parse_scenarios(scenario_file)

    ts = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    summary_csv = output_dir / f"stability_summary_{ts}.csv"
    report_md = output_dir / f"stability_report_{ts}.md"

    rows_out: List[Dict[str, str]] = []
    has_failure = False

    for scenario in scenarios:
        serial_csv = output_dir / f"stability_{scenario.name}_optimized_serial_{ts}.csv"
        parallel_csv = output_dir / f"stability_{scenario.name}_optimized_parallel_{ts}.csv"
        serial_log = output_dir / f"stability_{scenario.name}_optimized_serial_{ts}.log"
        parallel_log = output_dir / f"stability_{scenario.name}_optimized_parallel_{ts}.log"

        serial_cmd = [
            str(binary),
            "--traffic",
            str(dataset),
            "--query",
            scenario.query_type,
            "--execution",
            "serial",
            "--benchmark-runs",
            str(args.runs),
            "--benchmark-out",
            str(serial_csv),
            "--validate-serial",
            *scenario.extra_args,
        ]
        serial_proc = subprocess.run(serial_cmd, capture_output=True, text=True, check=False)
        serial_log.write_text(serial_proc.stdout + "\n" + serial_proc.stderr, encoding="utf-8")

        if serial_proc.returncode != 0:
            has_failure = True
            rows_out.append(
                {
                    "scenario_name": scenario.name,
                    "mode": "optimized_serial",
                    "thread_count": "1",
                    "status": "FAIL",
                    "reason": "run_optimized serial returned nonzero",
                    "result_count_values": "",
                    "avg_speed_range": "",
                    "avg_travel_time_range": "",
                    "query_mean_ms": "",
                    "query_stddev_ms": "",
                    "serial_match_rate": "",
                    "csv_output": str(serial_csv),
                    "log_output": str(serial_log),
                }
            )
            continue

        parallel_cmd_base = [
            str(binary),
            "--traffic",
            str(dataset),
            "--query",
            scenario.query_type,
            "--execution",
            "parallel",
            "--benchmark-runs",
            str(args.runs),
            "--benchmark-out",
            str(parallel_csv),
            "--validate-serial",
            *scenario.extra_args,
        ]

        thread_values = [t.strip() for t in args.threads.split(",") if t.strip()]
        if parallel_csv.exists():
            parallel_csv.unlink()
        parallel_stdout = ""
        parallel_stderr = ""
        for idx, thread_count in enumerate(thread_values):
            cmd = parallel_cmd_base + ["--threads", thread_count]
            if idx > 0:
                cmd.append("--benchmark-append")
            proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
            parallel_stdout += proc.stdout + "\n"
            parallel_stderr += proc.stderr + "\n"
            if proc.returncode != 0:
                has_failure = True
                rows_out.append(
                    {
                        "scenario_name": scenario.name,
                        "mode": "optimized_parallel",
                        "thread_count": thread_count,
                        "status": "FAIL",
                        "reason": "run_optimized parallel returned nonzero",
                        "result_count_values": "",
                        "avg_speed_range": "",
                        "avg_travel_time_range": "",
                        "query_mean_ms": "",
                        "query_stddev_ms": "",
                        "serial_match_rate": "",
                        "csv_output": str(parallel_csv),
                        "log_output": str(parallel_log),
                    }
                )
        parallel_log.write_text(parallel_stdout + "\n" + parallel_stderr, encoding="utf-8")

        if not serial_csv.is_file() or not parallel_csv.is_file():
            has_failure = True
            continue

        for mode_name, csv_path in (("optimized_serial", serial_csv), ("optimized_parallel", parallel_csv)):
            by_thread = evaluate_csv(csv_path)
            for t in sorted(by_thread.keys(), key=lambda x: int(x)):
                m = by_thread[t]
                counts = sorted({int(v) for v in m["result_count"]})
                speed_min = min(m["avg_speed"])
                speed_max = max(m["avg_speed"])
                travel_min = min(m["avg_travel"])
                travel_max = max(m["avg_travel"])
                query_mean = mean(m["query_ms"])
                query_std = stddev(m["query_ms"])
                serial_match_rate = mean(m["serial_match"])

                status = "PASS"
                reason = "stable"
                if len(counts) != 1:
                    status = "FAIL"
                    reason = "result_count drift across repeated runs"
                    has_failure = True
                elif serial_match_rate < 1.0:
                    status = "FAIL"
                    reason = "serial parity mismatch"
                    has_failure = True

                rows_out.append(
                    {
                        "scenario_name": scenario.name,
                        "mode": mode_name,
                        "thread_count": t,
                        "status": status,
                        "reason": reason,
                        "result_count_values": "|".join(str(c) for c in counts),
                        "avg_speed_range": f"{speed_min:.9f}..{speed_max:.9f}",
                        "avg_travel_time_range": f"{travel_min:.9f}..{travel_max:.9f}",
                        "query_mean_ms": f"{query_mean:.6f}",
                        "query_stddev_ms": f"{query_std:.6f}",
                        "serial_match_rate": f"{serial_match_rate:.6f}",
                        "csv_output": str(csv_path),
                        "log_output": str(serial_log if mode_name == 'optimized_serial' else parallel_log),
                    }
                )

    with summary_csv.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(
            fh,
            fieldnames=[
                "scenario_name",
                "mode",
                "thread_count",
                "status",
                "reason",
                "result_count_values",
                "avg_speed_range",
                "avg_travel_time_range",
                "query_mean_ms",
                "query_stddev_ms",
                "serial_match_rate",
                "csv_output",
                "log_output",
            ],
        )
        writer.writeheader()
        writer.writerows(rows_out)

    with report_md.open("w", encoding="utf-8") as fh:
        fh.write("# Phase 3 Dev Stability Report\n\n")
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
                f"- `{row['scenario_name']}` `{row['mode']}` thread `{row['thread_count']}`: "
                f"`{row['status']}` ({row['reason']})\n"
            )
            fh.write(f"  - result_count values: `{row['result_count_values']}`\n")
            fh.write(f"  - avg_speed range: `{row['avg_speed_range']}`\n")
            fh.write(f"  - avg_travel_time range: `{row['avg_travel_time_range']}`\n")
            fh.write(f"  - query mean/stddev ms: `{row['query_mean_ms']}` / `{row['query_stddev_ms']}`\n")
            fh.write(f"  - serial_match_rate: `{row['serial_match_rate']}`\n")
            fh.write(f"  - csv: `{row['csv_output']}`\n")
            fh.write(f"  - log: `{row['log_output']}`\n")
        fh.write("\n")
        if has_failure:
            fh.write("Detected instability or mismatches in repeated optimized runs.\n")
        else:
            fh.write("No nondeterministic result corruption detected in optimized runs.\n")

    print(f"[phase3-stability] summary_csv={summary_csv}")
    print(f"[phase3-stability] report_md={report_md}")
    print(f"[phase3-stability] status={'FAIL' if has_failure else 'PASS'}")
    return 1 if has_failure else 0


if __name__ == "__main__":
    raise SystemExit(main())
