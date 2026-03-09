# Phase 1 Development Benchmark Log

This log records development-stage (pre-baseline) subset benchmark batches for
traceability during Phase 1.

## Batch: 2026-03-08T23:27:58Z
- Date/time (UTC): `2026-03-08T23:27:58Z`
- Git branch: `P1-D15-16` (at execution time)
- Commit hash: `312af6a`
- Subset sizes:
  - `small` (`10,000` rows)
  - `medium` (`100,000` rows)
  - `large_dev` (`1,000,000` rows)
- Scenarios run:
  - `ingest_only`
  - `speed_below_8mph`
  - `time_window_full`
  - `borough_manhattan_speed_below_8mph`
  - `top10_slowest`
- Raw outputs:
  - `results/raw/phase1_dev/{small,medium,large_dev}_*.csv`
  - `results/raw/logs/phase1_dev_{small,medium,large_dev}_*_{timestamp}.log`
- Notable observations:
  - Runtime scales with subset size as expected.
  - Ingest time dominates total runtime for all subset sizes.
  - Query runtime remains small relative to ingest time.
- Failures or anomalies:
  - No runtime failures in this batch.
- Followed a code change:
  - Yes. This batch was used to validate the new repeatable subset benchmark runner flow.

## Batch: 2026-03-09T00:26:08Z
- Date/time (UTC): `2026-03-09T00:26:08Z`
- Git branch: `P1-D15-16`
- Commit hash: `294816e`
- Subset sizes:
  - `small` (`10,000` rows)
  - `medium` (`100,000` rows)
  - `large_dev` (`1,000,000` rows)
- Scenarios run:
  - `ingest_only`
  - `speed_below_8mph`
  - `time_window_full`
  - `borough_manhattan_speed_below_8mph`
  - `top10_slowest`
- Raw outputs:
  - `results/raw/phase1_dev/{small,medium,large_dev}_*.csv`
  - `results/raw/phase1_dev/batch_20260309T002608Z_manifest.csv`
  - `results/raw/phase1_dev/batch_20260309T002608Z_records_per_second.csv`
  - `results/raw/logs/phase1_dev_{small,medium,large_dev}_*_{timestamp}.log`
- Notable observations:
  - Batch metadata now captures branch + commit + scenario + subset labels.
  - Per-run records/second values are now generated for throughput analysis.
  - Runtime trend remains consistent with prior batch.
- Failures or anomalies:
  - No benchmark execution failures.
- Followed a code change:
  - Yes. This batch was run after adding batch manifest and records/second generation to the runner.
