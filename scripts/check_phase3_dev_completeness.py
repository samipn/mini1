#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Dict, List, Set, Tuple


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Check completeness of Phase 3 dev benchmark batch outputs.")
    parser.add_argument("--manifest", required=True, help="Batch manifest CSV")
    parser.add_argument(
        "--threads",
        default="1,2,4,8,16",
        help="Expected thread list for parallel modes",
    )
    return parser.parse_args()


def load_manifest(path: Path) -> List[Dict[str, str]]:
    if not path.is_file():
        raise SystemExit(f"Manifest not found: {path}")
    with path.open("r", newline="", encoding="utf-8") as fh:
        rows = list(csv.DictReader(fh))
    if not rows:
        raise SystemExit(f"Manifest has no rows: {path}")
    return rows


def parse_threads(value: str) -> List[str]:
    return [t.strip() for t in value.split(",") if t.strip()]


def count_rows(path: Path) -> int:
    with path.open("r", newline="", encoding="utf-8") as fh:
        return sum(1 for _ in csv.DictReader(fh))


def thread_set_from_csv(path: Path) -> Set[str]:
    with path.open("r", newline="", encoding="utf-8") as fh:
        return {row.get("thread_count", "") for row in csv.DictReader(fh)}


def main() -> int:
    args = parse_args()
    manifest = load_manifest(Path(args.manifest))
    expected_threads = parse_threads(args.threads)

    failures: List[str] = []
    seen_keys: Set[Tuple[str, str, str]] = set()

    for row in manifest:
        subset = row["subset_label"].strip()
        scenario = row["scenario_name"].strip()
        mode = row["mode"].strip()
        runs = int(row["benchmark_runs"])
        csv_path = Path(row["output_csv"])

        key = (subset, scenario, mode)
        if key in seen_keys:
            continue
        seen_keys.add(key)

        if not csv_path.is_file():
            failures.append(f"missing csv: {csv_path}")
            continue

        rows = count_rows(csv_path)
        threads_present = thread_set_from_csv(csv_path)

        if mode in {"parallel", "optimized_parallel"}:
            expected_rows = runs * len(expected_threads)
            if rows != expected_rows:
                failures.append(
                    f"row_count mismatch for {subset}/{scenario}/{mode}: expected {expected_rows}, got {rows}"
                )
            missing_threads = [t for t in expected_threads if t not in threads_present]
            if missing_threads:
                failures.append(
                    f"missing threads for {subset}/{scenario}/{mode}: {','.join(missing_threads)}"
                )
        else:
            if rows != runs:
                failures.append(
                    f"row_count mismatch for {subset}/{scenario}/{mode}: expected {runs}, got {rows}"
                )
            if threads_present and threads_present != {"1"}:
                failures.append(
                    f"unexpected thread_count values for {subset}/{scenario}/{mode}: {sorted(threads_present)}"
                )

    if failures:
        print("[phase3-completeness] FAIL")
        for item in failures:
            print(f"- {item}")
        return 1

    print("[phase3-completeness] PASS")
    print(f"checked groups: {len(seen_keys)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
