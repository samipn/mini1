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
- [x] P1-D3 operational correctness: define and enforce loader output-vector reset semantics.
  Objective/result: loaders now replace caller vectors on each successful load, preventing accumulation across repeated invocations.
  Evidence: `src/io/GarageLoader.cpp`, `src/io/BuildingLoader.cpp`, `tests/test_support_loaders.cpp`.
- [x] P1-D6 hardening: prevent `run_serial` aborts on malformed numeric CLI inputs.
  Objective/result: replaced exception-throwing `stoull` parsing with guarded parse helper; invalid numeric args now return code 2.
  Evidence: `apps/run_serial.cpp` (`--progress-every`, `--sample`, `--top-n`, `--min-link-samples`, `--benchmark-runs`, `--expect-accepted`).
- [x] P1-D10 reproducibility: avoid overwrite-prone output naming in phase1 benchmark runner.
  Objective/result: per-scenario output naming now includes branch/commit/timestamp; manifest rows remain immutable across batches.
  Evidence: `scripts/run_phase1_dev_benchmarks.sh:173`.
- [x] P1-D1 build policy: apply strict warning configuration to app/test targets as well.
  Objective/result: warning policy helper is applied to all app and test executables in CMake.
  Evidence: `CMakeLists.txt` (`urbandrop_enable_warnings(...)` on app/test targets).
- [x] P1-D2/D5 API invariant: borough representation normalization for direct-record query correctness.
  Objective/result: direct inserts now normalize borough text into `borough_code` when missing, preserving query behavior.
  Evidence: `src/data_model/TrafficDataset.cpp`, `tests/test_query_apis.cpp`.
- [x] P1-D3 maintainability: remove loader parsing redundancy.
  Objective/result: centralized shared CSV parse/numeric/timestamp helper logic and switched `CSVReader`, `GarageLoader`, and `BuildingLoader` to shared helpers.
  Evidence: `include/io/CsvParseUtils.hpp`, `src/io/CsvParseUtils.cpp`, updated loader/reader sources.
- [x] P1 test depth: add explicit negative CLI tests for malformed numeric parameters.
  Objective/result: regression coverage added to assert graceful failure (exit code 2), no crash.
  Evidence: `tests/test_run_serial_benchmark_cli.cpp`.
- [x] P1-D10 evidence rigor: enforce deliverable-grade run count policy for baseline claims.
  Objective/result: baseline-grade Phase 1 batch generated with `runs=10` across small/medium/large-dev subsets using dedicated baseline runner.
  Evidence: `results/raw/phase1_baseline/batch_20260309T080046Z_manifest.csv`.
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
- [x] P2-D9 operational consistency: align default query behavior in `run_parallel`.
  Objective/result: default query now uses `summary`, so `run_parallel --traffic <path>` executes without requiring extra query-specific flags.
  Evidence: `apps/run_parallel.cpp` default `query_type`.
- [x] P2-D10 thread-list policy: reject `0` in explicit `--thread-list` entries.
  Objective/result: explicit `--thread-list` entries now require strictly positive thread counts; `0` is rejected with exit code 2.
  Evidence: `apps/run_parallel.cpp` (`ParseThreadList`), `tests/test_run_parallel_cli.cpp`.
- [ ] P2 maintainability: deduplicate CLI numeric parsing helpers.
  Objective: centralize parse helpers shared by serial/parallel/optimized runners to prevent divergence.
  Evidence: duplicated parse helper logic across app binaries.
- [x] P2-D2 build policy: apply strict warning policy to app/test targets in addition to `congestion_core`.
  Objective/result: warning policy helper is applied across app/test targets in CMake, not only core library target.
  Evidence: `CMakeLists.txt` (`urbandrop_enable_warnings(...)` for app/test targets).
- [x] P2-D4 runtime isolation: avoid process-global OpenMP thread mutation in query helpers.
  Objective/result: removed per-call `omp_set_num_threads` usage and switched to scoped `num_threads(...)` clauses in parallel regions.
  Evidence: `src/query/ParallelCongestionQuery.cpp`, `src/query/ParallelTrafficAggregator.cpp`.
- [x] P2-D11/D13 evidence policy: separate dev batches (`runs=3`) from deliverable-grade scaling evidence (`runs>=10`).
  Objective/result: Phase 2 dev runner now enforces `runs>=3` (with explicit smoke override), and dedicated baseline runner enforces `runs>=10` in `results/raw/phase2_baseline`.
  Evidence: `scripts/run_phase2_dev_benchmarks.sh`, `scripts/run_phase2_baseline_benchmarks.sh`.
- [x] P2-D14 tooling clarity: deprecate or clearly scope legacy `summarize_phase2.py`.
  Objective/result: legacy summarizer now emits explicit deprecation guidance and points to manifest-driven `summarize_phase2_dev.py`.
  Evidence: `scripts/summarize_phase2.py`.
- [x] P2-D16 hardening: guard numeric parsing for `run_index_experiments --repeats`.
  Objective/result: replaced unguarded `std::stoull` with checked parsing; invalid values now return exit code 2.
  Evidence: `apps/run_index_experiments.cpp`, `tests/test_run_index_experiments_cli.cpp`.
- [x] P2-D18 robustness: replace fixed-column parsing in subset validation script with header-based extraction.
  Objective/result: subset validation now resolves benchmark fields by header names for both serial and parallel CSVs.
  Evidence: `scripts/run_phase2_subset_validation.sh`.
- [x] P2-D20/D26 path hygiene: enforce clearer dev-vs-baseline output destination policy for phase2 scripts.
  Objective/result: added dedicated baseline runner with default `results/raw/phase2_baseline` output and baseline run-count policy.
  Evidence: `scripts/run_phase2_baseline_benchmarks.sh`.
- [x] P2-D24 notes freshness: refresh Phase 2 benchmark log for current 16-thread policy and latest stability batch.
  Objective/result: appended new Phase 2 dev benchmark entry with explicit 16-thread scope and fresh manifest/summary/graph artifacts.
  Evidence: `report/phase2_dev_benchmark_log.md` (`2026-03-09T08:17:03Z` entry).
- [x] P2-D23 graph defaults: add all-thread comparison views (including 16-thread) by default.
  Objective/result: plotting now generates default all-thread serial-vs-parallel comparison charts using `1,2,4,8,16`.
  Evidence: `scripts/plot_phase2_dev.py` (`--parallel-threads` default and `serial_vs_parallel_all_threads_by_subset_*.svg` outputs).

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
- [x] P3-D5 correctness: reject unknown query types in `run_optimized` row-convert mode.
  Objective/result: unknown `--query` values now fail fast with exit code 2 before execution in both direct and row-convert paths.
  Evidence: `apps/run_optimized.cpp`, `tests/test_run_optimized_cli.cpp`.
- [x] P3-D3 robustness: make `TrafficColumns::AddRecord` exception-safe for SoA alignment.
  Objective/result: pre-reserve and upfront interning now prevent partial row append; dictionary/map updates are rollback-safe on interning failure.
  Evidence: `src/data_model/TrafficColumns.cpp`.
- [x] P3-D2 evidence: strengthen hotspot proof artifacts.
  Objective/result: clarified D2 as timing-based hotspot identification and added an explicit evidence map tying scenario config, raw manifests, and summaries.
  Evidence: `docs/TODO_PHASE3.md`, `docs/review/PHASE3_D2_HOTPATH_EVIDENCE.md`.
- [x] P3-D4 behavior consistency: define and enforce row-convert benchmarking semantics.
  Objective/result: row-convert mode now explicitly rejects benchmark-harness-only flags (`--benchmark-runs != 1`, `--benchmark-out`, `--benchmark-append`, `--validate-serial`, `--expect-accepted`) to prevent silent partial behavior.
  Evidence: `apps/run_optimized.cpp`, `tests/test_run_optimized_cli.cpp`.
- [x] P3-D6 hot-loop overhead: remove `std::function` predicate dispatch from optimized aggregation scans.
  Objective/result: aggregation helper is now templated predicate-based, removing type-erased per-row call overhead in hot scan loops.
  Evidence: `src/query/OptimizedTrafficAggregator.cpp`.
- [x] P3-D7 measurement integrity: memory probe should surface command failures explicitly.
  Objective/result: memory probe now records `exit_code` and `status`, emits `log_file`, and prints failure diagnostics while still preserving row-level artifacts.
  Evidence: `scripts/run_phase3_memory_probe.sh`.
- [x] P3-D8 hardening/tests: guard `run_optimized_support_experiments` numeric parsing and add negative test coverage.
  Objective/result: invalid/zero `--repeats` and unknown args now return exit code 2; negative CLI test coverage added.
  Evidence: `apps/run_optimized_support_experiments.cpp`, `tests/test_run_optimized_support_experiments_cli.cpp`, `CMakeLists.txt`.
- [x] P3-D12 attribution rigor: add automated step-isolated benchmark campaign.
  Objective/result: added scripted step-matrix runner that executes labeled baseline/dev step batches and records campaign-level artifact links for side-by-side analysis.
  Evidence: `scripts/run_phase3_step_matrix.sh`.
- [x] P3-D12 measurement policy: enforce deliverable-grade run counts for attribution/final tables.
  Objective/result: benchmark runner now rejects `--runs < 3` unless `--allow-low-runs` is explicitly provided; manifests/environment capture `evidence_tier` (`deliverable` vs `exploratory`).
  Evidence: `scripts/run_phase3_dev_benchmarks.sh`, `tests/test_phase3_benchmark_policy_cli.cpp`.
- [x] P3-D11 benchmark completeness: connect memory metrics directly to benchmark rows.
  Objective/result: summary pipeline now supports deterministic memory joins keyed by `(batch_utc, git_commit, optimization_step, mode, thread_count)` and emits memory columns in Phase 3 summary output.
  Evidence: `scripts/run_phase3_memory_probe.sh`, `scripts/summarize_phase3_dev.py`.
- [x] P3-D15 notes freshness: sync benchmark log metadata with current thread policy/artifacts.
  Objective/result: refreshed Phase 3 benchmark log with current thread/scenario/reporting policy snapshot and latest raw batch metadata including 16-thread coverage.
  Evidence: `report/phase3_dev_benchmark_log.md`.
- [x] P3-D19 robustness: replace fixed-column CSV extraction in subset validation with header-based parsing.
  Objective/result: subset validation now extracts fields by header names (including per-thread lookup), eliminating positional coupling.
  Evidence: `scripts/run_phase3_subset_validation.sh`.
- [x] P3-D19 coverage: include `top_n_slowest` scenario in Phase 3 validation suite.
  Objective/result: added top-N scenario to Phase 3 scenario config and extended scenario-argument plumbing (`top_n`, `min_link_samples`) in benchmark and validation scripts.
  Evidence: `configs/phase3_dev_scenarios.conf`, `scripts/run_phase3_dev_benchmarks.sh`, `scripts/run_phase3_subset_validation.sh`.
- [x] P3-D24 freshness gate: enforce summary/graph generation against latest raw batch.
  Objective/result: summary generation now fails by default when `--manifest` is not the latest `batch_*_manifest.csv` in the raw directory (opt-out via `--allow-stale-manifest`).
  Evidence: `scripts/summarize_phase3_dev.py`, `tests/test_phase3_reporting_guards.cpp`.
- [x] P3-D25 artifact presence: ensure graph outputs are generated and verified after each benchmark summary run.
  Objective/result: added reporting pipeline script that runs completeness+summary+plot and fails if chart manifest or SVG outputs are missing.
  Evidence: `scripts/run_phase3_dev_reporting.sh`.
- [x] P3-D25 memory plotting correctness: select memory rows by explicit batch key.
  Objective/result: plotting now requires deterministic memory-batch selection when multiple `batch_utc` values exist and filters rows by both batch key and mode/thread.
  Evidence: `scripts/plot_phase3_dev.py`, `tests/test_phase3_reporting_guards.cpp`.
- [x] P3-D29 ergonomics: add baseline-specific summary/completeness wrappers or strict path guards.
  Objective/result: added baseline reporting wrapper with baseline-path guard and baseline output defaults to reduce dev/baseline mix-ups.
  Evidence: `scripts/run_phase3_baseline_reporting.sh`, `tests/test_phase3_workflow_cli.cpp`.
