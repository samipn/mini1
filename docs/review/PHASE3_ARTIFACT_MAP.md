# Phase 3 Artifact Map

Purpose: define script-to-artifact flow so incomplete graph/table outputs can be debugged quickly.

## Raw Generation

1. `scripts/run_phase3_dev_benchmarks.sh`
   - Inputs: subset CSVs, `configs/phase3_dev_scenarios.conf`
   - Outputs (`results/raw/phase3_dev/`):
     - `subset_manifest_<ts>.csv`
     - `batch_<ts>_manifest.csv`
     - per-scenario benchmark CSVs
     - `batch_<ts>_records_per_second.csv`
     - `batch_<ts>_environment.csv`

2. Optional memory probe:
   - `scripts/run_phase3_memory_probe.sh`
   - Output: memory CSV under `results/raw/phase3_dev/memory/`

## Validation Gates

1. `scripts/check_phase3_dev_completeness.py --manifest <batch_manifest>`
   - Verifies required scenario/mode/thread combinations exist.
2. `scripts/check_phase3_dev_determinism.py ...`
   - Verifies correctness/stability expectations for repeat runs.

## Table/Graph Generation

1. `scripts/summarize_phase3_dev.py --manifest <batch_manifest>`
   - Output (`results/tables/phase3_dev/` by default):
     - `phase3_dev_summary.csv`
   - Includes timing split fields:
     - `query_mean_ms`
     - `validation_mean_ms`
     - `validation_ingest_mean_ms`
     - `validation_enabled_rate`

2. `scripts/plot_phase3_dev.py --summary-csv <summary_csv> [--memory-csv <memory_csv>]`
   - Output (`results/graphs/phase3_dev/` by default):
     - `runtime_by_subset_<scenario>.svg`
     - `optimized_runtime_vs_threads_<scenario>.svg`
     - `optimized_speedup_vs_threads_<scenario>.svg`
     - `memory_rss_by_mode.svg` (when memory CSV provided)

## Benchmark Caveat

- `query_ms` excludes serial parity-validation work.
- Validation overhead is emitted separately in `validation_ms` and `validation_ingest_ms`.
- For performance-only runs, use `scripts/run_phase3_dev_benchmarks.sh --no-validate-serial`.
- For correctness parity runs, use `--validate-serial` and analyze validation columns explicitly.
