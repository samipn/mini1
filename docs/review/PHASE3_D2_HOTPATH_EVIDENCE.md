# Phase 3 D2 Hot-Path Evidence (Timing-Based)

Date: 2026-03-09 (UTC)
Scope: Phase 3 Deliverable 2 (`Profile or Identify Hot Paths`).

## Evidence Standard Used
This repository currently uses **timing-breakdown evidence** (not committed profiler traces) to identify and prioritize hotspots.

## Hot Paths Chosen
- `speed_below` full-scan filtering
- `time_window` full-scan filtering
- `borough_speed_below` combined filter
- full-dataset `summary` aggregation
- `top_n_slowest` recurring-link aggregation path

## Artifact Chain
- Scenario definitions:
  - `configs/phase3_dev_scenarios.conf`
- Repeatable benchmark runner collecting per-scenario/per-mode/per-thread timings:
  - `scripts/run_phase3_dev_benchmarks.sh`
- Raw batch artifacts (example):
  - `results/raw/phase3_dev/batch_20260309T061031Z_manifest.csv`
  - `results/raw/phase3_dev/batch_20260309T061031Z_records_per_second.csv`
- Summary table generation from manifest:
  - `scripts/summarize_phase3_dev.py`
  - `results/tables/phase3_dev/phase3_dev_summary_batch_20260309T050044Z_manifest.csv`

## Interpretation
These timing artifacts are sufficient to justify the chosen optimization targets by showing which scenario classes dominate query/runtime cost at larger subsets and across execution modes.

## Limitation
No committed `perf`/`callgrind` trace set is currently part of the Phase 3 artifact chain. If instruction-level attribution is required later, add profiler capture tooling as a follow-up task.
