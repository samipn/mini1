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
    parser.add_argument(
        "--memory-csv",
        default="",
        help="Optional memory probe CSV from run_phase3_memory_probe.sh",
    )
    parser.add_argument(
        "--allow-stale-manifest",
        action="store_true",
        help="Allow summarizing a manifest that is not the latest batch manifest in its directory.",
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


def ensure_latest_manifest(manifest_path: Path, allow_stale: bool) -> None:
    if allow_stale:
        return
    candidates = sorted(manifest_path.parent.glob("batch_*_manifest.csv"))
    if not candidates:
        return
    latest = max(candidates, key=lambda p: p.name)
    if manifest_path.resolve() != latest.resolve():
        raise SystemExit(
            "Stale manifest selected. "
            f"Requested: {manifest_path.name}, latest: {latest.name}. "
            "Use the latest manifest or pass --allow-stale-manifest."
        )


def load_memory_rows(path: Path) -> List[Dict[str, str]]:
    if not path:
        return []
    if not path.is_file():
        raise SystemExit(f"Memory CSV not found: {path}")
    with path.open("r", newline="", encoding="utf-8") as fh:
        return list(csv.DictReader(fh))


def main() -> int:
    args = parse_args()
    manifest_path = Path(args.manifest)
    ensure_latest_manifest(manifest_path, args.allow_stale_manifest)
    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / args.output_file

    manifest_rows = load_manifest(manifest_path)
    memory_rows = load_memory_rows(Path(args.memory_csv)) if args.memory_csv else []

    grouped: Dict[Tuple[str, str, str, str, str], Dict[str, List[float]]] = defaultdict(
        lambda: {
            "ingest_ms": [],
            "query_ms": [],
            "total_ms": [],
            "validation_ms": [],
            "validation_ingest_ms": [],
            "rows_accepted": [],
            "result_count": [],
            "avg_speed": [],
            "avg_travel": [],
            "serial_match": [],
            "records_per_second": [],
            "validation_enabled": [],
        }
    )
    key_meta: Dict[Tuple[str, str, str, str, str], Dict[str, str]] = {}

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
                if key not in key_meta:
                    key_meta[key] = {
                        "batch_utc": entry.get("batch_utc", ""),
                        "git_branch": entry.get("git_branch", ""),
                        "git_commit": entry.get("git_commit", ""),
                        "evidence_tier": entry.get("evidence_tier", "unknown"),
                    }

                ingest_ms = float(row["ingest_ms"])
                query_ms = float(row["query_ms"])
                total_ms = float(row["total_ms"])
                rows_accepted = float(row["rows_accepted"])
                rps = rows_accepted / (total_ms / 1000.0) if total_ms > 0.0 else math.nan

                grouped[key]["ingest_ms"].append(ingest_ms)
                grouped[key]["query_ms"].append(query_ms)
                grouped[key]["total_ms"].append(total_ms)
                grouped[key]["validation_ms"].append(float(row.get("validation_ms", "0") or 0.0))
                grouped[key]["validation_ingest_ms"].append(
                    float(row.get("validation_ingest_ms", "0") or 0.0)
                )
                grouped[key]["rows_accepted"].append(rows_accepted)
                grouped[key]["result_count"].append(float(row["result_count"]))
                grouped[key]["avg_speed"].append(float(row["average_speed_mph"]))
                grouped[key]["avg_travel"].append(float(row["average_travel_time_seconds"]))
                grouped[key]["serial_match"].append(1.0 if row["serial_match"] == "true" else 0.0)
                grouped[key]["records_per_second"].append(rps)
                grouped[key]["validation_enabled"].append(
                    1.0 if row.get("serial_validation_enabled", "false") == "true" else 0.0
                )

    for (subset, scenario, mode, thread_count, _), metrics in grouped.items():
        mean_q = statistics.fmean(metrics["query_ms"])
        if mode == "serial" and thread_count == "1":
            serial_baseline_means[(subset, scenario)] = mean_q
        if mode == "parallel" and thread_count == args.parallel_thread_baseline:
            parallel_baseline_means[(subset, scenario)] = mean_q

    memory_index: Dict[Tuple[str, str, str, str, str], Dict[str, str]] = {}
    for row in memory_rows:
        mk = (
            row.get("batch_utc", "").strip(),
            row.get("git_commit", "").strip(),
            row.get("optimization_step", "").strip(),
            row.get("mode", "").strip(),
            row.get("thread_count", "").strip(),
        )
        memory_index[mk] = row

    with out_path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.writer(fh)
        writer.writerow(
            [
                "subset_label",
                "scenario_name",
                "mode",
                "thread_count",
                "optimization_step",
                "batch_utc",
                "git_branch",
                "git_commit",
                "evidence_tier",
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
                "validation_mean_ms",
                "validation_ingest_mean_ms",
                "validation_enabled_rate",
                "records_per_second_mean",
                "serial_match_rate",
                "result_count_mean",
                "avg_speed_mean",
                "avg_travel_time_mean",
                "speedup_vs_serial_query",
                "speedup_vs_parallel_query",
                "memory_max_rss_kb",
                "memory_exit_code",
                "memory_status",
            ]
        )

        for key in sorted(grouped.keys()):
            subset, scenario, mode, thread_count, optimization_step = key
            metrics = grouped[key]
            meta = key_meta.get(key, {})
            ingest = summarize(metrics["ingest_ms"])
            query = summarize(metrics["query_ms"])
            total = summarize(metrics["total_ms"])
            validation = summarize(metrics["validation_ms"])
            validation_ingest = summarize(metrics["validation_ingest_ms"])
            rps = summarize(metrics["records_per_second"])
            runs = len(metrics["query_ms"])

            serial_sp = speedup(serial_baseline_means.get((subset, scenario), math.nan), query[0])
            parallel_sp = speedup(parallel_baseline_means.get((subset, scenario), math.nan), query[0])

            mem_key = (
                meta.get("batch_utc", "").strip(),
                meta.get("git_commit", "").strip(),
                optimization_step.strip(),
                mode.strip(),
                thread_count.strip(),
            )
            mem_row = memory_index.get(mem_key, {})
            memory_max_rss = mem_row.get("max_rss_kb", "")
            memory_exit_code = mem_row.get("exit_code", "")
            memory_status = mem_row.get("status", "")

            row = [
                subset,
                scenario,
                mode,
                thread_count,
                optimization_step,
                meta.get("batch_utc", ""),
                meta.get("git_branch", ""),
                meta.get("git_commit", ""),
                meta.get("evidence_tier", "unknown"),
                runs,
            ]
            for stats in (ingest, query, total):
                row.extend([f"{value:.6f}" for value in stats])
            row.append(f"{validation[0]:.6f}")
            row.append(f"{validation_ingest[0]:.6f}")
            row.append(f"{statistics.fmean(metrics['validation_enabled']):.6f}")
            row.append(f"{rps[0]:.6f}")
            row.append(f"{statistics.fmean(metrics['serial_match']):.6f}")
            row.append(f"{statistics.fmean(metrics['result_count']):.6f}")
            row.append(f"{statistics.fmean(metrics['avg_speed']):.6f}")
            row.append(f"{statistics.fmean(metrics['avg_travel']):.6f}")
            row.append(f"{serial_sp:.6f}" if not math.isnan(serial_sp) else "nan")
            row.append(f"{parallel_sp:.6f}" if not math.isnan(parallel_sp) else "nan")
            row.append(memory_max_rss)
            row.append(memory_exit_code)
            row.append(memory_status)
            writer.writerow(row)

    print(f"[summarize_phase3_dev] wrote summary: {out_path}")
    print(f"[summarize_phase3_dev] groups: {len(grouped)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
