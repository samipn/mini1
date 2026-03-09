# Phase 3 Detailed Review (D11-D15)

Date: 2026-03-09 (UTC)
Scope: Phase 3 Deliverables 11-15 only.

Reviewed deliverables:
- D11 Extend benchmark harness for final comparisons
- D12 Benchmark each major optimization separately
- D13 Validate correctness against earlier phases
- D14 Summarize performance tradeoffs
- D15 Add final research notes

## Findings (ordered by severity)

1. High: D12 optimization-attribution is mostly label-based, not step-isolated by workflow.
   - Behavior: benchmark pipeline carries a single `optimization_step` label per batch, but there is no built-in orchestrated run that enforces/collects all required incremental states (`row baseline`, `columnar only`, `+encoding`, `+hot-loop`, `+parallel`) in one comparable pass.
   - Impact: attribution can be claimed without a reproducible, automated step-by-step evidence chain.
   - Evidence:
     - `scripts/run_phase3_dev_benchmarks.sh:16`
     - `scripts/run_phase3_dev_benchmarks.sh:153`
     - `scripts/run_phase3_baseline_benchmarks.sh:85`
     - `report/notes.md:370`

2. High: D12 “at least 10 runs per scenario” is not enforced and drifts in real batches.
   - Behavior: development runner defaults to `RUNS=3` and accepts arbitrary values; observed recent batch manifest includes `benchmark_runs=1`.
   - Impact: statistically weak measurements can still pass through the same summary/graph pipeline and be interpreted as deliverable evidence.
   - Evidence:
     - `scripts/run_phase3_dev_benchmarks.sh:14`
     - `scripts/run_phase3_dev_benchmarks.sh:295`
     - `results/raw/phase3_dev/batch_20260309T061031Z_manifest.csv` (`benchmark_runs` column shows `1`)

3. Medium: D11 benchmark harness CSV does not include memory metrics even when available.
   - Behavior: benchmark row schema records timing and validation fields, but no memory column; memory is collected separately via probe script with no direct per-run join key to harness rows.
   - Impact: final “all-three-phase direct comparison” CSVs are missing one dimension explicitly listed in D11 tasks.
   - Evidence:
     - `include/benchmark/BenchmarkHarness.hpp:52`
     - `src/benchmark/BenchmarkHarness.cpp:528`
     - `scripts/run_phase3_memory_probe.sh:97`

4. Medium: D15 Phase 3 benchmark notes are stale relative to current benchmark controls/artifacts.
   - Behavior: benchmark log still documents thread set `1,2,4,8` and earlier branch/commit context while active scripts and latest manifests use `1,2,4,8,16`.
   - Impact: report-readiness suffers because readers can misinterpret which thread-scaling evidence is current.
   - Evidence:
     - `report/phase3_dev_benchmark_log.md:24`
     - `scripts/run_phase3_dev_benchmarks.sh:15`
     - `results/raw/phase3_dev/batch_20260309T061031Z_manifest.csv` (thread list includes `1;2;4;8;16`)

## Memory / Leak Notes (D11-D15 scope)

- No direct memory leaks identified in D11-D15 harness/scripting code.
- Main risk in this scope is evidence integrity/comparability, not heap ownership.

## Recommended Remediation (D11-D15)

1. Add a single orchestrated “optimization attribution campaign” script that executes all required step states and writes one normalized comparison table.
2. Add a policy gate for D12 runs (`--runs >= 10`) in baseline/attribution modes, or clearly separate exploratory (`dev`) vs deliverable-grade runs in output schema.
3. Extend benchmark schema with optional memory columns (or strict join keys to memory probe rows) so D11 comparisons can include memory consistently.
4. Refresh Phase 3 benchmark log/notes to reflect current thread policy (including 16-thread runs), latest branch/commit references, and current artifact timestamps.
