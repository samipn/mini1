# Phase 3 Detailed Review (D25-D29)

Date: 2026-03-09 (UTC)
Scope: Phase 3 Deliverables 25-29 only.

Reviewed deliverables:
- D25 Development graphs
- D26 Development benchmark log
- D27 Stability checks for optimized runs
- D28 Optimization-step attribution
- D29 Pre-baseline labeling/separation

## Findings (ordered by severity)

1. High: D25 graph evidence is currently missing in working artifact snapshot.
   - Behavior: `results/graphs/phase3_dev/` contains only `.gitkeep` at review time.
   - Impact: checked TODO claims for graph generation are not supported by current local artifact set.
   - Evidence:
     - directory listing: `results/graphs/phase3_dev/` (only `.gitkeep`)
     - graph writer target path in script: `scripts/plot_phase3_dev.py:13`

2. Medium: D25 memory graph selection can be incorrect when memory CSV has multiple rows/batches.
   - Behavior: graph script selects the first row by mode (`next(...)`) from the provided CSV, not necessarily latest batch or specific query; can mix unmatched measurements.
   - Evidence:
     - `scripts/plot_phase3_dev.py:285`
     - `scripts/plot_phase3_dev.py:289`

3. Medium: D26 benchmark log is stale versus current thread policy and latest Phase 3 batch.
   - Behavior: benchmark log still records threads `1,2,4,8` and older branch/commit entries, while latest manifests include `1,2,4,8,16` and newer branch/commit.
   - Evidence:
     - `report/phase3_dev_benchmark_log.md:24`
     - `report/phase3_dev_benchmark_log.md:7`
     - `results/raw/phase3_dev/batch_20260309T061031Z_manifest.csv` (`thread_list` includes `1;2;4;8;16`)

4. Medium: D28 attribution remains metadata labeling, not step-isolated reproducible campaign.
   - Behavior: `optimization_step` exists in manifests/summaries, but workflow does not enforce isolated step-by-step benchmark execution in a single campaign matrix.
   - Evidence:
     - `scripts/run_phase3_dev_benchmarks.sh:16`
     - `scripts/run_phase3_dev_benchmarks.sh:153`
     - `scripts/summarize_phase3_dev.py:88`

5. Low: D29 separation is conceptually strong but operationally shares generic dev tooling names.
   - Behavior: baseline plan uses `run_phase3_baseline_benchmarks.sh`, but summary/completeness scripts still use `phase3_dev` naming and rely on caller-provided paths.
   - Impact: workable, but easier to misroute outputs by operator error.
   - Evidence:
     - `docs/review/PHASE3_BASELINE_PLAN.md:33`
     - `docs/review/PHASE3_BASELINE_PLAN.md:50`
     - `scripts/summarize_phase3_dev.py:14`
     - `scripts/check_phase3_dev_completeness.py:11`

## Memory / Leak Notes (D25-D29 scope)

- No memory leak findings in D25-D29 scripts.
- Graph/report fidelity and attribution traceability are the key risks in this scope.

## Recommended Remediation (D25-D29)

1. Regenerate and verify phase3-dev graph set after latest summary generation, then append evidence links in review log.
2. Update plotting logic to filter memory rows by selected batch timestamp (or latest batch) before plotting.
3. Append fresh D26 log entries after each benchmark batch with 16-thread runs included.
4. Add dedicated attribution campaign runner for D28 that emits one table across all optimization stages.
5. Consider thin baseline-specific wrappers for summary/completeness naming to reduce D29 operator error.
