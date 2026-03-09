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

## Batch: 2026-03-09T06:10:31Z
- Date/time (UTC): `2026-03-09T06:10:31Z`
- Git branch: `P3-R1-INGEST-TEST-GRAPH`
- Commit hash: `b8b450b`
- Subset sizes:
  - `small_smoke` (`10,000` rows)
- Scenarios run:
  - `ingest_only`
  - `speed_below_15`
  - `time_window_all`
  - `borough_manhattan_speed_15`
  - `top_n_slowest`
  - `summary`
- Raw outputs:
  - `results/raw/phase1_dev/batch_20260309T061031Z_manifest.csv`
  - `results/raw/phase1_dev/batch_20260309T061031Z_records_per_second.csv`
  - `results/raw/phase1_dev/small_smoke_*.csv`
  - `results/raw/logs/phase1_dev_small_smoke_*_20260309T061031Z.log`
- Notable observations:
  - Scenario naming now aligns with `configs/phase1_dev_scenarios.csv`.
  - This batch is smoke-level (`benchmark_runs=1`) and should not be used as deliverable-grade timing evidence.
- Failures or anomalies:
  - No benchmark execution failures.
- Followed a code change:
  - Yes. This batch captured post-review smoke validation in the updated scenario namespace.

## Batch: 2026-03-09T07:59:12Z
- Date/time (UTC): `2026-03-09T07:59:12Z`
- Git branch: `P3-R1-INGEST-TEST-GRAPH`
- Commit hash: `f073e8b`
- Subset sizes:
  - `small_refresh` (`10,000` rows)
- Scenarios run:
  - `ingest_only`
  - `speed_below_15`
  - `time_window_all`
  - `borough_manhattan_speed_15`
  - `top_n_slowest`
  - `summary`
- Raw outputs:
  - `results/raw/phase1_dev/batch_20260309T075912Z_manifest.csv`
  - `results/raw/phase1_dev/batch_20260309T075912Z_records_per_second.csv`
  - `results/raw/phase1_dev/small_refresh_*.csv`
  - `results/raw/logs/phase1_dev_small_refresh_*_20260309T075912Z.log`
- Notable observations:
  - Batch used `--runs 3`, satisfying Phase 1 dev minimum repeat policy.
  - Scenario names and args match `configs/phase1_dev_scenarios.csv`.
  - Query runtimes remain small relative to ingest time on the 10k subset.
- Failures or anomalies:
  - No benchmark execution failures.
- Followed a code change:
  - Yes. This batch was used to refresh post-fix benchmark evidence against the current scenario config.
