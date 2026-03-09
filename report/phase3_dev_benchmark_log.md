# Phase 3 Development Benchmark Log

This log records development-stage (**pre-baseline**) subset benchmark batches for Phase 3 serial-vs-parallel-vs-optimized work.

## Current Policy Snapshot (2026-03-09 UTC)
- Source runner defaults:
  - `scripts/run_phase3_dev_benchmarks.sh`
- Current default thread list:
  - `1,2,4,8,16`
- Current scenario config:
  - `configs/phase3_dev_scenarios.conf`
  - includes `speed_below_15`, `time_window_all`, `summary`, `borough_manhattan_speed_15`, `top_5_slowest_links`
- Current reporting pipeline:
  - `scripts/run_phase3_dev_reporting.sh`
  - validates completeness, regenerates summary tables, regenerates graphs, and fails on missing graph artifacts
- Current baseline reporting guardrail:
  - `scripts/run_phase3_baseline_reporting.sh` (baseline-path guard by default)

## Latest Raw Batch Snapshot
- Latest manifest currently present:
  - `results/raw/phase3_dev/batch_20260309T061031Z_manifest.csv`
- Modes:
  - `serial`, `parallel`, `optimized_serial`, `optimized_parallel`
- Thread list values found in manifest:
  - `1` (serial/optimized_serial rows)
  - `1;2;4;8;16` (parallel/optimized_parallel rows)

## Batch: 2026-03-09T05:00:44Z
- Date/time (UTC): `2026-03-09T05:00:44Z`
- Git branch: `P3-D6-10`
- Commit hash at batch start: `fea67e7`
- Optimization step label: `soa_encoded_hotloop`
- Subset sizes:
  - `small` (`10,000` rows)
  - `medium` (`100,000` rows)
  - `large_dev` (`1,000,000` rows)
- Scenarios run:
  - `speed_below_15`
  - `time_window_all`
  - `summary`
  - `borough_manhattan_speed_15`
- Modes tested:
  - `serial`
  - `parallel`
  - `optimized_serial`
  - `optimized_parallel`
- Thread counts tested:
  - `1`
  - `2`
  - `4`
  - `8`
- Raw outputs:
  - `results/raw/phase3_dev/subset_manifest_20260309T050044Z.csv`
  - `results/raw/phase3_dev/batch_20260309T050044Z_manifest.csv`
  - `results/raw/phase3_dev/batch_20260309T050044Z_records_per_second.csv`
  - `results/raw/phase3_dev/{small,medium,large_dev}_*_{serial|parallel|optimized_serial|optimized_parallel}_P3-D6-10_fea67e7_20260309T050044Z.csv`
  - `results/raw/logs/phase3_dev_*_20260309T050044Z.log`
- Summary outputs:
  - `results/tables/phase3_dev/phase3_dev_summary_batch_20260309T050044Z_manifest.csv`
  - `results/graphs/phase3_dev/*.svg`
- Notable observations:
  - Cross-mode parity checks passed in the validation batch.
  - Optimized and optimized-parallel modes improved query-time behavior most on the larger subset scenarios.
  - Some small-subset scenarios remained ingest-dominated.
- Failures or anomalies:
  - No benchmark execution failures in this batch.
- Followed a code change:
  - Yes. This batch followed Phase 3 script/harness completion and optimized support/index integration.

## Stability Batch: 2026-03-09T05:06:29Z
- Date/time (UTC): `2026-03-09T05:06:29Z`
- Dataset:
  - `medium` (`100,000` rows)
- Threads:
  - `1,2,4,8`
- Repeated runs:
  - `5` per scenario/mode
- Artifacts:
  - `results/raw/phase3_dev/stability/stability_summary_20260309T050629Z.csv`
  - `results/raw/phase3_dev/stability/stability_report_20260309T050629Z.md`
- Status:
  - `PASS`

## Memory Probe: 2026-03-09T05:07:03Z
- Artifact:
  - `results/raw/phase3_dev/memory/memory_probe_20260309T050703Z.csv`
- Method:
  - `/usr/bin/time -v` with consistent summary-query workload across serial, parallel, optimized serial, and optimized parallel modes.
