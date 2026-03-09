# Phase 1 Detailed Review (D6-D10)

Date: 2026-03-09 (UTC)
Scope: Phase 1 Deliverables 6-10 only.

Reviewed deliverables:
- D6 Serial CLI runner
- D7 Timing and benchmark harness
- D8 Validation and sanity checks
- D9 Logging and notes for research use
- D10 Baseline benchmark scenarios

## Findings (ordered by severity)

1. High: current Phase 1 baseline artifacts do not meet D10 run-count expectations.
   - Behavior: latest Phase 1 dev manifest rows show `benchmark_runs=1` (smoke-only), while D10 requires 10+ runs per scenario for baseline comparability.
   - Impact: checked D10 status is stronger than current evidence set.
   - Evidence:
     - `docs/TODO_PHASE1.md:242`
     - `results/raw/phase1_dev/batch_20260309T061031Z_manifest.csv` (`benchmark_runs` column)
     - `results/tables/phase1_dev/phase1_dev_summary.csv` (`runs` column shows `1` in rows)

2. Medium: benchmark log entries for Phase 1 are stale versus current scenario/config state.
   - Behavior: `report/phase1_dev_benchmark_log.md` still documents older scenario names (`speed_below_8mph`, `top10_slowest`) and older branch/commit context, while current scenario config uses `speed_below_15`, `time_window_all`, `top_n_slowest`.
   - Impact: D9 traceability and interpretation quality are degraded.
   - Evidence:
     - `report/phase1_dev_benchmark_log.md:16`
     - `report/phase1_dev_benchmark_log.md:34`
     - `configs/phase1_dev_scenarios.csv:2`
     - `configs/phase1_dev_scenarios.csv:6`

3. Medium: benchmark-run quality floor is not enforced by runner.
   - Behavior: Phase 1 dev runner default is `RUNS=3` and accepts arbitrary values; no policy gate for deliverable-grade runs.
   - Impact: low-repeat batches can appear alongside baseline artifacts without downgrade labeling.
   - Evidence:
     - `scripts/run_phase1_dev_benchmarks.sh:12`
     - `scripts/run_phase1_dev_benchmarks.sh:69`

4. Low: Phase 1 benchmark script now mixes serial + parallel + optimized campaigns.
   - Behavior: `scripts/benchmark.sh` runs all three implementations, not just serial baseline paths.
   - Impact: operational confusion risk for “Phase 1 baseline only” execution unless workflow is explicitly separated.
   - Evidence:
     - `scripts/benchmark.sh:47`
     - `scripts/benchmark.sh:63`

## Memory / Leak Notes (D6-D10 scope)

- No direct memory leaks found in D6-D10 code paths (CLI/harness/notes/scripts reviewed).
- Main risks are evidence integrity and reproducibility rigor, not heap ownership.

## Recommended Remediation (D6-D10)

1. Add a deliverable-grade policy mode for Phase 1 baseline runs (`--runs >= 10`) and label smoke/dev runs distinctly.
2. Append fresh Phase 1 benchmark log entries aligned to current scenarios, branch, commit, and artifact timestamps.
3. Add freshness checks that flag stale Phase 1 summary tables relative to newest manifests.
4. Keep a clearly documented serial-only baseline command path (or dedicated script) for D10 reproducibility.
