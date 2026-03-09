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
- [ ] Add build matrix notes (compiler, flags, OpenMP availability) in docs for reproducibility.

## 2) Algorithms and Runtime Behavior
- [ ] Re-run Phase 3 dev benchmarks without `--validate-serial` and regenerate summary/graphs.
- [ ] Keep a separate validation batch for parity checks only.
- [ ] Re-check optimized query scaling (1/2/4/8/16 threads) after timing-scope fix.
- [ ] Add thread-scaling plots for serial/parallel/optimized in one chart family per scenario.
- [ ] Add test case that fails if optimized `query_ms` unexpectedly includes serial validation ingest overhead.

## 3) Requirements -> Produced Outputs
- [ ] Align `docs/TODO_PHASE3.md` checkboxes with measured evidence only.
- [ ] Separate “implemented” vs “fully benchmark-validated” in Phase 3 TODO wording.
- [ ] Add a compact artifact map linking scripts -> generated CSV -> generated graph outputs.
- [ ] Confirm each claimed deliverable has a corresponding source file and artifact.

## 4) Missing Details
- [ ] Add memory-usage graph generation from `results/raw/phase3_dev/memory/*.csv`.
- [ ] Document benchmark caveat in one place: validation affects timing if enabled.
- [ ] Record exact machine/compiler config in benchmark logs for each batch.
- [ ] Add final baseline directory plan and script (`phase3_baseline`) distinct from `phase3_dev`.

## 5) Going Above and Beyond
- [ ] Add per-query profiler integration option (e.g., perf/callgrind wrapper script).
- [ ] Add confidence intervals / variability bands in summary tables.
- [ ] Add regression guardrails in CI for benchmark schema and artifact completeness.
- [x] Add benchmark integrity checker script to detect missing scenario/mode/thread combinations.

## Priority Queue
- [ ] P0: timing-scope fix in `BenchmarkHarness` + benchmark rerun + graph regeneration.
- [ ] P1: TODO/docs claim alignment and explicit benchmark methodology split.
- [ ] P2: memory graph support + richer statistical summaries.
