# Thorough Code Review TODO

Purpose: track issues found during incremental repository review and convert them into actionable engineering tasks.

## Scope update
- [x] Include 16-thread benchmark/test iterations in default thread lists (`1,2,4,8,16`).
- [ ] Keep heavy benchmark execution deferred until review-pass instrumentation fixes are complete.

## 1) Code Setup
- [x] Split timing scopes in `BenchmarkHarness` so validation work is not included in `query_ms`.
- [x] Add explicit benchmark mode toggle: correctness-validation mode vs performance-only mode.
- [x] Add `validation_ms` and `validation_ingest_ms` columns to benchmark CSV output.
- [x] Update CMake to avoid making OpenMP a hard requirement for purely serial builds.
- [x] Add build matrix notes (compiler, flags, OpenMP availability) in docs for reproducibility.

## 2) Algorithms and Runtime Behavior
- [ ] Re-run Phase 3 dev benchmarks without `--validate-serial` and regenerate summary/graphs.
- [ ] Keep a separate validation batch for parity checks only.
- [ ] Re-check optimized query scaling (1/2/4/8/16 threads) after timing-scope fix.
- [ ] Add thread-scaling plots for serial/parallel/optimized in one chart family per scenario.
- [ ] Add test case that fails if optimized `query_ms` unexpectedly includes serial validation ingest overhead.

## 3) Requirements -> Produced Outputs
- [ ] Align `docs/TODO_PHASE3.md` checkboxes with measured evidence only.
- [ ] Separate “implemented” vs “fully benchmark-validated” in Phase 3 TODO wording.
- [x] Add a compact artifact map linking scripts -> generated CSV -> generated graph outputs.
- [ ] Confirm each claimed deliverable has a corresponding source file and artifact.
  Current evidence: `docs/review/PHASE_DELIVERABLE_AUDIT.md` (phase1 6/6, phase2 7/7, phase3 10/10).
- [ ] Reconcile checked path tokens in phase TODO docs with actual repo snapshot evidence.
  Current evidence: `docs/review/TODO_EVIDENCE_AUDIT.md` (phase1 16/16, phase2 13/13, phase3 14/14).

## 4) Missing Details
- [x] Add memory-usage graph generation from `results/raw/phase3_dev/memory/*.csv`.
- [x] Document benchmark caveat in one place: validation affects timing if enabled.
- [x] Record exact machine/compiler config in benchmark logs for each batch.
- [x] Add final baseline directory plan and script (`phase3_baseline`) distinct from `phase3_dev`.

## 5) Going Above and Beyond
- [ ] Add per-query profiler integration option (e.g., perf/callgrind wrapper script).
- [ ] Add confidence intervals / variability bands in summary tables.
- [ ] Add regression guardrails in CI for benchmark schema and artifact completeness.
- [x] Add benchmark integrity checker script to detect missing scenario/mode/thread combinations.

## Priority Queue
- [ ] P0: timing-scope fix in `BenchmarkHarness` + benchmark rerun + graph regeneration.
- [ ] P1: TODO/docs claim alignment and explicit benchmark methodology split.
- [ ] P2: memory graph support + richer statistical summaries.

## 6) Per-Deliverable Review Backlog

### Phase 1 (D1-D10)
- [x] D1 Build setup reviewed.
  Objective/result: verify CMake targets/warnings/build modes; no blocking defects found.
- [x] D2 core data structures reviewed.
  Objective/result: verify required records/fields exist and compile cleanly; no blocking defects found.
- [ ] D3 ingestion pipeline reviewed and hardening tasks identified.
  Objective: fix and test dataset/counter reset behavior for repeated loads in `CSVReader`; align `BuildingLoader` stats reset with `GarageLoader`; keep parse helpers self-contained with explicit includes.
- [ ] D4 dataset container API review.
  Objective: verify API contracts (`Size`, `Empty`, iterators, `At`) plus add tests for out-of-range, const-iteration stability, and ingestion/query separation assumptions.
- [ ] D5 query API review.
  Objective: validate serial query correctness against deterministic fixtures for all query types including edge ranges and top-N tie behavior.
- [ ] D6 serial CLI review.
  Objective: verify argument validation, error codes, and log/error stream consistency for invalid/missing input combinations.
- [ ] D7 benchmark harness review (serial path).
  Objective: verify serial CSV schema stability, run-count semantics, and no hidden validation overhead in serial timings.
- [ ] D8 validation/sanity checks review.
  Objective: ensure suspicious-row counters are internally consistent and covered by tests.
- [ ] D9 notes/research logging review.
  Objective: cross-check `report/notes.md` claims against code/artifacts and mark any unsupported claims.
- [ ] D10 baseline benchmark scenario review.
  Objective: verify scenario config completeness, reproducibility commands, and artifact naming consistency.

### Phase 2 (D1-D15)
- [ ] D1 baseline lock review.
  Objective: verify serial baseline comparability assumptions and immutable reference artifacts.
- [ ] D2 parallel build support review.
  Objective: verify OpenMP optional/required behavior across compilers and document fallback behavior.
- [ ] D3 parallelization target selection review.
  Objective: verify target workloads align with measured hot paths and notes.
- [ ] D4 parallel infrastructure review.
  Objective: verify thread-local/reduction design has no shared-state races by inspection + tests.
- [ ] D5 speed-threshold parallel query review.
  Objective: verify count parity and scaling behavior over thread list `1,2,4,8,16`.
- [ ] D6 time-window parallel query review.
  Objective: verify range-boundary correctness and parity under parallel execution.
- [ ] D7 aggregation parallel query review.
  Objective: verify floating-point reduction tolerance and deterministic acceptance bounds.
- [ ] D8 top-N parallel strategy review.
  Objective: verify ranking stability and merge logic; document complexity/perf tradeoffs clearly.
- [ ] D9 parallel CLI review.
  Objective: verify CLI thread controls, validation mode behavior, and nonzero-failure semantics.
- [ ] D10 thread-count controls review.
  Objective: verify thread-list parsing and per-thread artifact completeness checks.
- [ ] D11 correctness validation mode review.
  Objective: verify serial-vs-parallel parity pipeline catches mismatches and reports actionable diffs.
- [ ] D12 benchmark harness extension review (parallel path).
  Objective: verify phase2 CSV fields remain schema-stable and script consumers are resilient to schema drift.
- [ ] D13 scaling benchmark review.
  Objective: verify scenario coverage for serial vs parallel across required thread counts.
- [ ] D14 speedup/overhead analysis review.
  Objective: verify computed speedups and explanations are consistent with raw data.
- [ ] D15 phase2 notes review.
  Objective: verify notes reflect measured evidence only and identify unresolved bottlenecks.

### Phase 3 (D1-D15)
- [ ] D1 baseline freeze/comparison review.
  Objective: verify `phase3_dev` vs `phase3_baseline` separation and baseline script usage.
- [ ] D2 hotspot identification review.
  Objective: verify hotspot claims map to timing/profiling evidence.
- [ ] D3 object-of-arrays layout review.
  Objective: verify field alignment, encoding consistency, and counter integrity in optimized dataset.
- [ ] D4 conversion/direct-load path review.
  Objective: verify row->column parity and ingest correctness between load modes.
- [ ] D5 optimized query APIs review.
  Objective: verify optimized query parity vs serial baseline for all supported query modes.
- [ ] D6 hot-loop overhead reduction review.
  Objective: verify intended branch/string reductions exist and do not change semantics.
- [ ] D7 memory usage evaluation review.
  Objective: verify memory probe methodology and generated artifacts are reproducible and interpretable.
- [ ] D8 optional query-support optimization review.
  Objective: verify added index/lookup structures have measurable benefit and clear rollback criteria.
- [ ] D9 optimized + parallel integration review.
  Objective: verify optimized parallel scaling and correctness under `1,2,4,8,16` threads.
- [ ] D10 optimized CLI review.
  Objective: verify CLI execution modes, load modes, and benchmark flag interactions.
- [ ] D11 benchmark harness final-comparison review.
  Objective: verify validation timing split fields and environment metadata are consistently emitted.
- [ ] D12 per-optimization benchmark attribution review.
  Objective: verify each claimed optimization step has attributable before/after data.
- [ ] D13 correctness validation review.
  Objective: verify determinism/parity scripts detect and report mismatches across modes.
- [ ] D14 performance tradeoff analysis review.
  Objective: verify regressions, limits, and wins are all documented with data-backed explanations.
- [ ] D15 final notes/report-readiness review.
  Objective: verify final report inputs are evidence-backed and presentation-ready.
