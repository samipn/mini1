# Phase 3 Detailed Review (D18-D24)

Date: 2026-03-09 (UTC)
Scope: Phase 3 Deliverables 18-24 only.

Reviewed deliverables:
- D18 Reuse/validate subset datasets
- D19 Cross-mode validation runs on subsets
- D20 Early optimized benchmark scenarios
- D21 Repeatable Phase 3 development benchmark runner
- D22 Preliminary optimized timing/comparison data
- D23 Preliminary memory-tradeoff data
- D24 Development summary tables

## Findings (ordered by severity)

1. High: D23 memory probe currently allows invalid runs to be recorded as normal benchmark evidence.
   - Behavior: probe executes with `/usr/bin/time ... || true`, then always appends CSV rows even if command failed.
   - Snapshot evidence: latest memory CSV includes command lines with `--traffic --help`, indicating invalid dataset argument flow still produced rows (`max_rss_kb` values and `0:00.00` elapsed).
   - Evidence:
     - `scripts/run_phase3_memory_probe.sh:106`
     - `scripts/run_phase3_memory_probe.sh:116`
     - `results/raw/phase3_dev/memory/memory_probe_20260309T060929Z.csv` (command column rows)

2. High: D24 summary artifacts are not automatically synchronized to latest D22 raw batch outputs.
   - Behavior: latest raw batch manifest is newer than the latest summary table; pipeline does not enforce summarize-after-batch or stale-summary detection.
   - Impact: tables/graphs can lag behind raw data while TODO claims remain checked.
   - Evidence:
     - latest raw: `results/raw/phase3_dev/batch_20260309T061031Z_manifest.csv`
     - latest summary: `results/tables/phase3_dev/phase3_dev_summary_batch_20260309T050044Z_manifest.csv`

3. Medium: D19 validation script is schema-fragile due fixed-column extraction.
   - Behavior: validation reads benchmark CSV values by hardcoded positional columns (`$19/$21/$22/$6`) rather than header names.
   - Impact: benchmark schema evolution can silently corrupt validation comparisons.
   - Evidence:
     - `scripts/run_phase3_subset_validation.sh:201`
     - `scripts/run_phase3_subset_validation.sh:228`
     - `scripts/run_phase3_subset_validation.sh:251`

4. Medium: D19 “top-N outputs if applicable” is not covered by current scenario set.
   - Behavior: Phase 3 validation scenarios include `speed_below`, `time_window`, `summary`, `borough_speed_below`, but no `top_n_slowest`.
   - Evidence:
     - `configs/phase3_dev_scenarios.conf:2`
     - `configs/phase3_dev_scenarios.conf:5`
   - Note: since top-N exists in optimized APIs/CLI, this is applicable for Phase 3 validation scope.

5. Medium: D22 run-count quality floor is not enforced in runner.
   - Behavior: runner default is `RUNS=3`, but accepts any value and recent manifest shows `benchmark_runs=1`.
   - Evidence:
     - `scripts/run_phase3_dev_benchmarks.sh:14`
     - `results/raw/phase3_dev/batch_20260309T061031Z_manifest.csv` (`benchmark_runs` column)

## Memory / Leak Notes (D18-D24 scope)

- No direct memory leaks found in D18-D24 scripts.
- Primary memory-risk issue is measurement integrity (failed or invalid workload probes being logged as successful measurements).

## Recommended Remediation (D18-D24)

1. Make memory probe failure-explicit (`exit_code/status` columns) and stop swallowing command failures.
2. Add freshness guard: if a newer raw batch exists than summary output, fail/report stale summary status.
3. Convert subset-validation CSV parsing to header-based extraction (same resilience pattern used in other benchmark scripts).
4. Add `top_n_slowest` scenario to Phase 3 validation/benchmark scenario config.
5. Add minimum run-count policy flags for deliverable-grade batches (separate from exploratory batches).
