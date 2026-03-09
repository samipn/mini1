# Phase 2 Benchmark Consistency Analysis (2026-03-09)

## Inputs
- Dev manifest: `results/raw/phase2_dev/batch_20260309T082307Z_manifest.csv`
- Baseline manifest: `results/raw/phase2_baseline/batch_20260309T082645Z_manifest.csv`
- Stability summary: `results/raw/phase2_dev/stability/stability_summary_20260309T082534Z.csv`
- Validation summary: `results/raw/phase2_dev/validation/validation_20260309T082555Z.csv`
- Phase 2 dev summary: `results/tables/phase2_dev/phase2_dev_summary_20260309T082307Z.csv`
- Phase 2 baseline summary: `results/tables/phase2_baseline/phase2_baseline_summary_20260309T082645Z.csv`

## Objective Alignment
- Correctness parity objective: `PASS`
  - Validation rows: `60`
  - Validation failures: `0`
  - Serial/parallel parity and `serial_match` checks passed.
- Determinism/stability objective: `PASS`
  - Stability rows: `20`
  - Stability failures: `0`
- Repeatability objective: `PASS` (moderate jitter on some large runs)
  - Dev median total-time CV: `3.594%`
  - Baseline median total-time CV: `3.668%`
- Speedup objective: `PARTIAL` / scenario-dependent
  - Threads >=2 combinations with speedup >1.0: `24/48`
  - Best observed speedup: `1.919x` (`small`, `time_window_all`, `4` threads)
  - Worst observed speedup: `0.553x` (`large_dev`, `borough_manhattan_speed_15`, `4` threads)
  - 16-thread speedup mean: `1.053x` (min `0.564`, max `1.864`)

## Interpretation
- Phase 2 goals for correctness and deterministic behavior are satisfied on current artifacts.
- Performance gains are workload-dependent, which is expected for this stage:
  - some scans improve with parallelism;
  - some workloads remain overhead-limited or memory-bandwidth-limited.
- This behavior is acceptable for Phase 2 because the objective includes measuring both speedups and slowdowns with evidence.
