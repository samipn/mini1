#!/usr/bin/env python3
"""
Create deterministic CSV subsets for development benchmarking.

The script preserves the original header and writes the first N data rows for
each requested subset size.
"""

from __future__ import annotations

import argparse
import csv
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List


def parse_sizes(raw_sizes: List[str]) -> List[int]:
    sizes: List[int] = []
    for raw in raw_sizes:
        try:
            value = int(raw)
        except ValueError as exc:
            raise argparse.ArgumentTypeError(f"Invalid subset size: {raw}") from exc
        if value <= 0:
            raise argparse.ArgumentTypeError(
                f"Subset sizes must be positive integers, got: {value}"
            )
        sizes.append(value)
    return sorted(set(sizes))


def progress_log(rows_read: int, max_size: int) -> bool:
    if rows_read <= 10_000:
        return rows_read % 1_000 == 0
    if rows_read <= 100_000:
        return rows_read % 10_000 == 0
    return rows_read % 100_000 == 0 or rows_read == max_size


def open_subset_files(
    output_dir: Path, sizes: List[int], stem: str
) -> Dict[int, Dict[str, object]]:
    handles: Dict[int, Dict[str, object]] = {}
    for size in sizes:
        subset_path = output_dir / f"{stem}_subset_{size}.csv"
        fh = subset_path.open("w", newline="", encoding="utf-8")
        writer = csv.writer(fh)
        handles[size] = {
            "path": subset_path,
            "fh": fh,
            "writer": writer,
            "written_rows": 0,
        }
    return handles


def write_manifest(
    manifest_path: Path,
    source_file: Path,
    handles: Dict[int, Dict[str, object]],
    created_at_utc: str,
) -> None:
    with manifest_path.open("w", newline="", encoding="utf-8") as manifest_fh:
        writer = csv.writer(manifest_fh)
        writer.writerow(
            [
                "created_at_utc",
                "source_file",
                "subset_file",
                "requested_rows",
                "written_rows",
                "header_preserved",
            ]
        )
        for size in sorted(handles.keys()):
            entry = handles[size]
            subset_path = entry["path"]
            written_rows = entry["written_rows"]
            writer.writerow(
                [
                    created_at_utc,
                    str(source_file),
                    str(subset_path),
                    size,
                    written_rows,
                    "true",
                ]
            )


def run(input_path: Path, output_dir: Path, sizes: List[int]) -> int:
    if not input_path.exists():
        raise FileNotFoundError(f"Input file not found: {input_path}")
    if not input_path.is_file():
        raise ValueError(f"Input path is not a file: {input_path}")

    output_dir.mkdir(parents=True, exist_ok=True)

    max_size = max(sizes)
    stem = input_path.stem
    created_at_utc = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    print(f"[make_subsets] source: {input_path}")
    print(f"[make_subsets] output_dir: {output_dir}")
    print(f"[make_subsets] sizes: {sizes}")

    handles = open_subset_files(output_dir, sizes, stem)
    try:
        with input_path.open("r", newline="", encoding="utf-8") as in_fh:
            reader = csv.reader(in_fh)
            header = next(reader, None)
            if header is None:
                raise ValueError(f"Input CSV has no header: {input_path}")

            for entry in handles.values():
                writer = entry["writer"]
                writer.writerow(header)

            rows_read = 0
            for row in reader:
                rows_read += 1

                for size, entry in handles.items():
                    if rows_read <= size:
                        writer = entry["writer"]
                        writer.writerow(row)
                        entry["written_rows"] += 1

                if progress_log(rows_read, max_size):
                    print(f"[make_subsets] processed rows: {rows_read}")

                if rows_read >= max_size:
                    break

        manifest_path = output_dir / "manifest.csv"
        write_manifest(manifest_path, input_path, handles, created_at_utc)

        print("[make_subsets] completed.")
        for size in sizes:
            entry = handles[size]
            subset_path = entry["path"]
            written_rows = entry["written_rows"]
            print(
                f"[make_subsets] subset={size} rows={written_rows} file={subset_path}"
            )
        print(f"[make_subsets] manifest={manifest_path}")

    finally:
        for entry in handles.values():
            fh = entry["fh"]
            fh.close()

    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Create deterministic CSV subsets for Phase 1 development."
    )
    parser.add_argument(
        "--input",
        required=True,
        help="Path to source traffic CSV file.",
    )
    parser.add_argument(
        "--output-dir",
        default="data/subsets",
        help="Directory to write subset CSV files and manifest (default: data/subsets).",
    )
    parser.add_argument(
        "--sizes",
        nargs="+",
        default=["10000", "100000", "1000000"],
        help="Requested subset sizes as positive integers (default: 10000 100000 1000000).",
    )
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    sizes = parse_sizes(args.sizes)
    return run(Path(args.input), Path(args.output_dir), sizes)


if __name__ == "__main__":
    raise SystemExit(main())
