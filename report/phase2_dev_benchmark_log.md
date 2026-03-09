# Phase 2 Development Benchmark Log

This log records development-stage (**pre-baseline**) subset benchmark batches
for Phase 2 serial-vs-parallel work.

## Batch: 2026-03-09T04:02:38Z
- Date/time (UTC): `2026-03-09T04:02:38Z`
- Git branch: `P2-D17-21`
- Commit hash: `aea201e`
- Subset sizes:
  - `small` (`10,000` rows)
  - `medium` (`100,000` rows)
  - `large_dev` (`1,000,000` rows)
- Scenarios run:
  - `speed_below_15`
  - `time_window_all`
  - `summary`
  - `borough_manhattan_speed_15`
- Thread counts tested:
  - `1`
  - `2`
  - `4`
  - `8`
- Raw outputs:
  - `results/raw/phase2_dev/batch_20260309T040238Z_manifest.csv`
  - `results/raw/phase2_dev/batch_20260309T040238Z_records_per_second.csv`
  - `results/raw/phase2_dev/{small,medium,large_dev}_*_{serial|parallel}_P2-D17-21_aea201e_20260309T040238Z.csv`
  - `results/raw/logs/phase2_dev_*_20260309T040238Z.log`
- Notable observations:
  - Serial-vs-parallel parity checks succeeded in the associated validation batch.
  - Runtime and throughput trends are consistent across subset sizes.
  - Thread scaling behavior is scenario-dependent and requires summary/plot analysis.
- Failures or anomalies:
  - No benchmark execution failures in this batch.
- Followed a code change:
  - Yes. This batch was run after adding the Phase 2 subset benchmark runner and validation helpers.

## Stability Batch: 2026-03-09T04:11:12Z
- Date/time (UTC): `2026-03-09T04:11:12Z`
- Git branch: `P2-D22-26`
- Commit hash: `0652f9b` (baseline at batch start)
- Dataset:
  - `medium` (`100,000` rows)
- Scenarios run:
  - `speed_below_15`
  - `time_window_all`
  - `summary`
  - `borough_manhattan_speed_15`
- Thread counts tested:
  - `1`
  - `2`
  - `4`
  - `8`
- Repeated runs:
  - `5` per scenario/thread configuration
- Raw outputs:
  - `results/raw/phase2_dev/stability/stability_summary_20260309T041112Z.csv`
  - `results/raw/phase2_dev/stability/stability_report_20260309T041112Z.md`
  - `results/raw/phase2_dev/stability/stability_*_20260309T041112Z.csv`
  - `results/raw/phase2_dev/stability/stability_*_20260309T041112Z.log`
- Notable observations:
  - Result counts remained stable for all scenarios and thread counts.
  - Aggregate values remained stable within expected floating-point precision.
  - Timing variance exists as expected but no determinism failures were observed.
- Failures or anomalies:
  - No determinism failures detected.
- Followed a code change:
  - Yes. This batch validated the new Phase 2 stability/determinism checker.
