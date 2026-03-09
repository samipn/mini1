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
- [x] D3 ingestion pipeline reviewed and hardened.
  Objective/result: fixed repeated-load reset semantics in `CSVReader`, aligned `BuildingLoader` stats reset behavior, and added explicit parse includes; regression tests added in `test_csv_reader` and `test_support_loaders`.
- [x] D4 dataset container API reviewed.
  Objective/result: container APIs verified (`Size`, `Empty`, iterators, `At`) and repeat-load semantics now explicitly tested.
- [x] D5 query API reviewed.
  Objective/result: serial query coverage validated by `test_query_apis`, `test_run_serial_queries`, and `test_realdata_api`.
- [x] D6 serial CLI reviewed.
  Objective/result: CLI argument and benchmark path behavior validated by `test_run_serial_benchmark_cli` and integrated real-data CLI tests.
- [x] D7 benchmark harness review (serial path).
  Objective/result: serial benchmark schema/timing semantics validated by `test_benchmark_harness` and header-based script readers.
- [x] D8 validation/sanity checks reviewed.
  Objective/result: malformed/suspicious counter behavior validated in parser tests and real-data ingest checks.
- [x] D9 notes/research logging reviewed.
  Objective/result: `report/notes.md` claims cross-checked with generated artifacts and review audit docs.
- [x] D10 baseline benchmark scenarios reviewed.
  Objective/result: scenario completeness verified via `configs/phase1_dev_scenarios.csv` and smoke benchmark artifact generation.

#### Phase 1 Open Gaps (from deep review)
- [ ] P1-D3 operational correctness: define and enforce loader output-vector reset semantics.
  Objective: avoid silent duplicate accumulation when calling `GarageLoader::LoadCSV` / `BuildingLoader::LoadCSV` repeatedly with the same output vector.
  Evidence: both loaders reset stats but append to `out_records` without clearing.
- [x] P1-D6 hardening: prevent `run_serial` aborts on malformed numeric CLI inputs.
  Objective/result: replaced exception-throwing `stoull` parsing with guarded parse helper; invalid numeric args now return code 2.
  Evidence: `apps/run_serial.cpp` (`--progress-every`, `--sample`, `--top-n`, `--min-link-samples`, `--benchmark-runs`, `--expect-accepted`).
- [x] P1-D10 reproducibility: avoid overwrite-prone output naming in phase1 benchmark runner.
  Objective/result: per-scenario output naming now includes branch/commit/timestamp; manifest rows remain immutable across batches.
  Evidence: `scripts/run_phase1_dev_benchmarks.sh:173`.
- [ ] P1-D1 build policy: apply strict warning configuration to app/test targets as well.
  Objective: keep warning policy consistent across all compiled binaries, not only library objects.
  Evidence: warning flags are attached to `congestion_core` target but not explicitly to executable targets.
- [ ] P1-D2/D5 API invariant: borough representation normalization for direct-record query correctness.
  Objective: prevent false negatives in borough-filter query when records are inserted with `borough` text but unset `borough_code`.
  Evidence: `TrafficDataset::AddRecord` does not normalize borough fields; borough query prefers code for known borough strings.
- [x] P1-D3 maintainability: remove loader parsing redundancy.
  Objective/result: centralized shared CSV parse/numeric/timestamp helper logic and switched `CSVReader`, `GarageLoader`, and `BuildingLoader` to shared helpers.
  Evidence: `include/io/CsvParseUtils.hpp`, `src/io/CsvParseUtils.cpp`, updated loader/reader sources.
- [x] P1 test depth: add explicit negative CLI tests for malformed numeric parameters.
  Objective/result: regression coverage added to assert graceful failure (exit code 2), no crash.
  Evidence: `tests/test_run_serial_benchmark_cli.cpp`.
- [ ] P1-D10 evidence rigor: enforce deliverable-grade run count policy for baseline claims.
  Objective: prevent single-run/smoke batches from being treated as Phase 1 baseline evidence.
  Evidence: latest Phase 1 dev manifest/summary rows show `runs=1`.
- [x] P1-D9 notes freshness: keep Phase 1 benchmark log aligned with current scenario config and artifacts.
  Objective/result: appended latest `runs=3` Phase 1 dev batch entry with scenario names matching current config and current branch/commit metadata.
  Evidence: `report/phase1_dev_benchmark_log.md` (`2026-03-09T07:59:12Z` entry).
- [x] P1-D10 operational clarity: provide explicit serial-only baseline benchmark entrypoint.
  Objective/result: added dedicated serial baseline wrapper with baseline-grade run policy and explicit smoke override.
  Evidence: `scripts/run_phase1_baseline.sh`.
- [x] P1-D12 evidence integrity: enforce/verify subset-validation artifact presence.
  Objective/result: reran subset validation flow and refreshed timestamped logs for small/medium/large-dev datasets.
  Evidence: `results/raw/validation/validation_{small,medium,large_dev}_20260309T075859Z.log`.
- [x] P1-D13 scenario integrity: make determinism checks use the same scenario source as benchmark runs.
  Objective/result: determinism checker now loads scenarios from `configs/phase1_dev_scenarios.csv` (configurable via `--scenario-file`) instead of hardcoded scenario definitions.
  Evidence: `scripts/check_phase1_dev_determinism.py`.
- [x] P1-D14 policy guard: enforce minimum repeat count for deliverable-grade subset benchmark runs.
  Objective/result: `run_phase1_dev_benchmarks.sh` now enforces `--runs >= 3` by default with explicit smoke override via `--allow-under-min-runs`.
  Evidence: `scripts/run_phase1_dev_benchmarks.sh`.
- [x] P1-D15 evidence rigor: ensure latest subset timing artifacts satisfy minimum repeat expectations.
  Objective/result: generated fresh Phase 1 dev benchmark batch with `--runs 3` using the current scenario config.
  Evidence: `results/raw/phase1_dev/batch_20260309T075912Z_manifest.csv` (benchmark_runs=`3`).
- [x] P1-D16 summary robustness: handle dataset-label drift between runner outputs and summary defaults.
  Objective/result: summarizer now falls back to all discovered dataset labels when requested defaults are absent, so default invocation remains usable on smoke-labeled batches.
  Evidence: `scripts/summarize_phase1_dev.py`.
- [x] P1-D17 graph robustness: remove hardcoded subset/scenario assumptions in plotting.
  Objective/result: plotting now discovers dataset labels and query scenarios directly from summary CSV inputs, with optional `--dry-run` mapping validation.
  Evidence: `scripts/plot_phase1_dev.py`.
- [x] P1-D17 dependency clarity: provide explicit plotting dependency/install path.
  Objective/result: plotting dependency error message now includes explicit install commands and dry-run usage guidance.
  Evidence: `scripts/plot_phase1_dev.py`.
- [x] P1-D18 notes freshness: append benchmark-log entries for latest local Phase 1 dev artifacts.
  Objective/result: benchmark log now includes the `2026-03-09T06:10:31Z` smoke batch with current scenario names and artifact links.
  Evidence: `report/phase1_dev_benchmark_log.md`.
- [x] P1-D19 determinism depth: verify top-N payload/order stability, not only counts.
  Objective/result: determinism checker now computes and compares top-N rank/value signatures across repeated runs.
  Evidence: `scripts/check_phase1_dev_determinism.py` (`topn_signature_values` in summary/report output).
- [x] P1-D20 operational guardrails: enforce dev-vs-baseline output path separation in benchmark entrypoints.
  Objective/result: mixed benchmark entrypoint now rejects phase-scoped output dirs unless explicit override env var is set.
  Evidence: `scripts/benchmark.sh` (`URBANDROP_ALLOW_PHASE_DIRS=1` escape hatch).

### Phase 2 (D1-D15)
- [x] D1 baseline lock reviewed.
  Objective/result: baseline comparability validated through phase audits and preserved phase1/phase2 artifacts.
- [x] D2 parallel build support reviewed.
  Objective/result: OpenMP optional/required behavior verified in CMake and build/test runs.
- [x] D3 parallelization target selection reviewed.
  Objective/result: selected workloads align with scenario configs and benchmark logs.
- [x] D4 parallel infrastructure reviewed.
  Objective/result: thread-local/reduction patterns validated by correctness tests and determinism checks.
- [x] D5 speed-threshold parallel query reviewed.
  Objective/result: parity and scaling behavior validated via parallel correctness tests and thread sweeps.
- [x] D6 time-window parallel query reviewed.
  Objective/result: range correctness validated in API/CLI correctness suites.
- [x] D7 aggregation parallel query reviewed.
  Objective/result: aggregate parity validated with tolerance-aware comparison tests.
- [x] D8 top-N parallel strategy reviewed.
  Objective/result: ranking path verified through dedicated CLI/API tests and scenario artifacts.
- [x] D9 parallel CLI reviewed.
  Objective/result: CLI validation and failure semantics covered by `test_run_parallel_cli`.
- [x] D10 thread-count controls reviewed.
  Objective/result: thread-list parsing and expected-line counts validated (including 16-thread scope).
- [x] D11 correctness validation mode reviewed.
  Objective/result: serial-vs-parallel validation pipeline passes (`check_phase2_dev_determinism.py`).
- [x] D12 benchmark harness extension reviewed (parallel path).
  Objective/result: schema-stable, header-based parsing enforced in phase2 scripts.
- [x] D13 scaling benchmark reviewed.
  Objective/result: required scenarios/threads confirmed by phase2 manifests and stability outputs.
- [x] D14 speedup/overhead analysis reviewed.
  Objective/result: summary tables and notes cross-checked against raw metrics.
- [x] D15 phase2 notes reviewed.
  Objective/result: notes and produced artifacts reconciled through audit docs.

#### Phase 2 Open Gaps (from deep review)
- [x] P2-D9 hardening: prevent `run_parallel` aborts on malformed numeric CLI inputs.
  Objective/result: replaced exception-throwing `stoull` parsing with guarded parse helper; invalid numeric args now return code 2.
  Evidence: `apps/run_parallel.cpp` (`--benchmark-runs`, `--expect-accepted`, `--top-n`, `--min-link-samples`).
- [x] P2 test depth: add negative CLI tests for malformed numeric parameters.
  Objective/result: added negative CLI regression assertions for malformed numeric arguments.
  Objective: assert graceful failure behavior for invalid numeric args in parallel CLI.
  Evidence: `tests/test_run_parallel_cli.cpp`.
- [ ] P2-D9 operational consistency: align default query behavior in `run_parallel`.
  Objective: make `run_parallel --traffic <path>` produce a useful default run without contradictory required flags.
  Evidence: `apps/run_parallel.cpp` default `query_type=speed_below` and threshold requirement check.
- [ ] P2-D10 thread-list policy: reject `0` in explicit `--thread-list` entries.
  Objective: enforce explicit positive thread counts and avoid ambiguous `threads=0` output semantics.
  Evidence: `apps/run_parallel.cpp` ParseThreadList currently allows `0`.
- [ ] P2 maintainability: deduplicate CLI numeric parsing helpers.
  Objective: centralize parse helpers shared by serial/parallel/optimized runners to prevent divergence.
  Evidence: duplicated parse helper logic across app binaries.
- [ ] P2-D2 build policy: apply strict warning policy to app/test targets in addition to `congestion_core`.
  Objective: keep warning enforcement consistent across all binaries participating in Phase 2 deliverables.
  Evidence: warning flags are currently scoped to `congestion_core` target options.
- [ ] P2-D4 runtime isolation: avoid process-global OpenMP thread mutation in query helpers.
  Objective: remove cross-call coupling from `omp_set_num_threads` and prefer scoped `num_threads(...)` usage.
  Evidence: `ParallelCongestionQuery` and `ParallelTrafficAggregator` set global OpenMP thread count per call.
- [ ] P2-D11/D13 evidence policy: separate dev batches (`runs=3`) from deliverable-grade scaling evidence (`runs>=10`).
  Objective: prevent ambiguity when comparing Phase 2 scaling claims across `phase2_dev` and `phase2` artifact families.
  Evidence: `phase2_dev` manifests use 3 runs; `results/raw/phase2` contains 10-run sample sets.
- [ ] P2-D14 tooling clarity: deprecate or clearly scope legacy `summarize_phase2.py`.
  Objective: avoid analysis drift from parallel maintenance of fixed-file and manifest-driven summary pipelines.
  Evidence: both `scripts/summarize_phase2.py` and `scripts/summarize_phase2_dev.py` are active.
- [ ] P2-D16 hardening: guard numeric parsing for `run_index_experiments --repeats`.
  Objective: invalid repeat values should return usage error, not process abort.
  Evidence: `apps/run_index_experiments.cpp` uses unguarded `std::stoull`.
- [ ] P2-D18 robustness: replace fixed-column parsing in subset validation script with header-based extraction.
  Objective: keep validation resilient to benchmark CSV schema changes.
  Evidence: `scripts/run_phase2_subset_validation.sh` uses positional fields (`$19/$21/$22/$6`).
- [ ] P2-D20/D26 path hygiene: enforce clearer dev-vs-baseline output destination policy for phase2 scripts.
  Objective: reduce accidental mixing between `phase2_dev`, `phase2`, and future `phase2_baseline` artifacts.
  Evidence: index experiment defaults to `results/raw/phase2`; mixed benchmark entrypoint defaults to generic `results/raw`.
- [ ] P2-D24 notes freshness: refresh Phase 2 benchmark log for current 16-thread policy and latest stability batch.
  Objective: keep report evidence synchronized with current thread scope and newest artifact timestamps.
  Evidence: `report/phase2_dev_benchmark_log.md` entries primarily document 1/2/4/8-thread runs.
- [ ] P2-D23 graph defaults: add all-thread comparison views (including 16-thread) by default.
  Objective: improve out-of-box visibility of full thread-scaling behavior in Phase 2 graphs.
  Evidence: `plot_phase2_dev.py` default comparative bars focus on one selected thread (`--parallel-thread 4`).

### Phase 3 (D1-D15)
- [x] D1 baseline freeze/comparison reviewed.
  Objective/result: `phase3_baseline` separation added and documented with runner + plan.
- [x] D2 hotspot identification reviewed.
  Objective/result: hotspot claims aligned with scenario/timing focus in notes and benchmark scripts.
- [x] D3 object-of-arrays layout reviewed.
  Objective/result: optimized layout and encoding paths verified by optimized correctness tests.
- [x] D4 conversion/direct-load path reviewed.
  Objective/result: row-convert and direct-load modes exercised and validated by CLI tests.
- [x] D5 optimized query APIs reviewed.
  Objective/result: parity verified against row baseline via optimized correctness suite.
- [x] D6 hot-loop overhead reduction reviewed.
  Objective/result: code-path changes inspected; semantics guarded by parity/determinism tests.
- [x] D7 memory usage evaluation reviewed.
  Objective/result: memory probe runner + memory artifact generation verified and plotted path available.
- [x] D8 optional query-support optimization reviewed.
  Objective/result: support/index experiment paths and tests present and validated.
- [x] D9 optimized + parallel integration reviewed.
  Objective/result: optimized parallel correctness/scaling paths validated at thread lists including 16.
- [x] D10 optimized CLI reviewed.
  Objective/result: execution mode/load mode behavior validated by `test_run_optimized_cli`.
- [x] D11 benchmark harness final-comparison reviewed.
  Objective/result: validation timing split and environment metadata emission verified.
- [x] D12 per-optimization benchmark attribution reviewed.
  Objective/result: optimization-step labeling and manifest lineage validated in phase3 scripts/docs.
- [x] D13 correctness validation reviewed.
  Objective/result: `check_phase3_dev_determinism.py` and completeness checks pass on subset validation.
- [x] D14 performance tradeoff analysis reviewed.
  Objective/result: summaries/notes cross-checked with generated raw and summary artifacts.
- [x] D15 final notes/report-readiness reviewed.
  Objective/result: review docs now provide auditable evidence chain for report and slide preparation.

#### Phase 3 Open Gaps (from deep review)
- [x] P3-D10 hardening: prevent `run_optimized` aborts on malformed numeric CLI inputs.
  Objective/result: replaced exception-throwing `stoull` parsing with guarded parse helper; invalid numeric args now return code 2.
  Evidence: `apps/run_optimized.cpp` (`--benchmark-runs`, `--expect-accepted`, `--top-n`, `--min-link-samples`).
- [x] P3 test depth: add negative CLI tests for malformed numeric parameters.
  Objective/result: added negative CLI regression assertions for malformed numeric arguments.
  Objective: assert graceful failure behavior for invalid numeric args in optimized CLI.
  Evidence: `tests/test_run_optimized_cli.cpp`.
- [ ] P3-D5 correctness: reject unknown query types in `run_optimized` row-convert mode.
  Objective: ensure unknown query types return nonzero and never silently report `result_count=0`.
  Evidence: `apps/run_optimized.cpp` row-convert branch currently falls through to success.
- [ ] P3-D3 robustness: make `TrafficColumns::AddRecord` exception-safe for SoA alignment.
  Objective: prevent partial-column append on interning/allocation failure to preserve index alignment invariants.
  Evidence: `src/data_model/TrafficColumns.cpp` pushes base columns before `InternLinkName`.
- [ ] P3-D2 evidence: strengthen hotspot proof artifacts.
  Objective: add profiler/timing artifact references for hotspot selection or adjust docs wording to “timing-based identification only”.
  Evidence: D2 TODO claim references profiling confirmation but profiler outputs are not currently part of artifact chain.
- [ ] P3-D4 behavior consistency: define and enforce row-convert benchmarking semantics.
  Objective: either route row-convert through benchmark harness for repeated runs/CSV output/validation, or document explicit limitations.
  Evidence: `apps/run_optimized.cpp` direct mode uses harness; row-convert mode does not.
- [ ] P3-D6 hot-loop overhead: remove `std::function` predicate dispatch from optimized aggregation scans.
  Objective: avoid per-row type-erased predicate calls in optimized query loops and enforce inlined/specialized hot paths.
  Evidence: `src/query/OptimizedTrafficAggregator.cpp` uses `std::function<bool(std::size_t)>` in `SummarizeConditional`.
- [ ] P3-D7 measurement integrity: memory probe should surface command failures explicitly.
  Objective: stop swallowing probe command failures (or record `exit_code/status`) so memory artifacts cannot silently include failed runs.
  Evidence: `scripts/run_phase3_memory_probe.sh` runs probes with `/usr/bin/time ... || true` and always appends CSV rows.
- [ ] P3-D8 hardening/tests: guard `run_optimized_support_experiments` numeric parsing and add negative test coverage.
  Objective: invalid `--repeats` should return usage error instead of process abort.
  Evidence: `apps/run_optimized_support_experiments.cpp` uses unguarded `std::stoull` for `--repeats`.
- [ ] P3-D12 attribution rigor: add automated step-isolated benchmark campaign.
  Objective: generate reproducible side-by-side results for each optimization stage (row baseline, columnar only, +encoding, +hot-loop, +parallel) instead of relying on a single step label per batch.
  Evidence: `run_phase3_dev_benchmarks.sh` carries one `optimization_step` label per run set.
- [ ] P3-D12 measurement policy: enforce deliverable-grade run counts for attribution/final tables.
  Objective: prevent low-repeat batches from being mixed into deliverable evidence without explicit downgrade label.
  Evidence: `run_phase3_dev_benchmarks.sh` default `RUNS=3`; observed manifest batches with `benchmark_runs=1`.
- [ ] P3-D11 benchmark completeness: connect memory metrics directly to benchmark rows.
  Objective: include memory usage in comparable benchmark schema (or provide deterministic join keys with memory probe outputs).
  Evidence: Benchmark CSV header lacks memory fields while memory probe writes separate CSV.
- [ ] P3-D15 notes freshness: sync benchmark log metadata with current thread policy/artifacts.
  Objective: keep Phase 3 report evidence aligned with latest branch/commit/thread configurations (including 16-thread runs).
  Evidence: `report/phase3_dev_benchmark_log.md` thread list is stale vs current batch manifests.
- [ ] P3-D19 robustness: replace fixed-column CSV extraction in subset validation with header-based parsing.
  Objective: prevent validation drift when benchmark CSV schema changes.
  Evidence: `run_phase3_subset_validation.sh` uses positional fields (`$19/$21/$22/$6`).
- [ ] P3-D19 coverage: include `top_n_slowest` scenario in Phase 3 validation suite.
  Objective: ensure “top-N outputs if applicable” is exercised in cross-mode subset validation.
  Evidence: `configs/phase3_dev_scenarios.conf` currently excludes top-N scenarios.
- [ ] P3-D24 freshness gate: enforce summary/graph generation against latest raw batch.
  Objective: fail fast when summary tables/graphs lag behind newest manifests.
  Evidence: latest summary CSV timestamp is older than latest batch manifest.
- [ ] P3-D25 artifact presence: ensure graph outputs are generated and verified after each benchmark summary run.
  Objective: maintain concrete graph evidence instead of stale TODO completion state.
  Evidence: `results/graphs/phase3_dev/` currently contains only `.gitkeep`.
- [ ] P3-D25 memory plotting correctness: select memory rows by explicit batch key.
  Objective: avoid mixing unmatched memory rows when plotting from multi-row memory CSV inputs.
  Evidence: `plot_phase3_dev.py` currently picks first row per mode via `next(...)`.
- [ ] P3-D29 ergonomics: add baseline-specific summary/completeness wrappers or strict path guards.
  Objective: reduce operator-error risk between `phase3_dev` and `phase3_baseline` output paths.
  Evidence: baseline flow still reuses `phase3_dev`-named tooling with caller-managed output paths.
