#!/usr/bin/env python3
from __future__ import annotations

import csv
import datetime as dt
from pathlib import Path
from typing import Dict, Iterable, List


def utc_now_iso() -> str:
    return dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def save_matplotlib_figure(fig, output_dir: Path, stem: str, dpi: int = 150) -> List[Path]:
    ensure_dir(output_dir)
    outputs: List[Path] = []
    for ext in ("png", "svg"):
        out_path = output_dir / f"{stem}.{ext}"
        fig.savefig(out_path, dpi=dpi if ext == "png" else None)
        outputs.append(out_path)
    return outputs


def add_common_figure_text(
    fig,
    ax,
    subtitle: str,
    what_tested: str,
    source_summary: Path,
    generated_utc: str,
) -> None:
    # Keep headroom for subtitle and footers.
    ax.set_title(ax.get_title(), pad=26)
    fig.text(0.5, 0.93, subtitle, ha="center", va="center", fontsize=10, color="#444444")
    ax.text(
        0.01,
        0.99,
        f"What this tests: {what_tested}",
        transform=ax.transAxes,
        va="top",
        ha="left",
        fontsize=9,
        bbox={"boxstyle": "round,pad=0.3", "facecolor": "#f7f7f7", "edgecolor": "#bbbbbb"},
    )
    fig.text(
        0.01,
        0.01,
        f"Source: {source_summary} | Generated: {generated_utc}",
        ha="left",
        va="bottom",
        fontsize=8,
        color="#555555",
    )


def write_chart_manifest(path: Path, rows: Iterable[Dict[str, str]]) -> None:
    rows_list = list(rows)
    if not rows_list:
        return
    ensure_dir(path.parent)
    with path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(
            fh,
            fieldnames=[
                "chart_id",
                "file_path",
                "phase",
                "what_tested",
                "x_axis",
                "y_axis",
                "source_summary_csv",
                "generated_utc",
            ],
        )
        writer.writeheader()
        writer.writerows(rows_list)
