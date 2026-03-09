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
- [x] P1-D6 hardening: prevent `run_serial` aborts on malformed numeric CLI inputs.
  Objective/result: replaced exception-throwing `stoull` parsing with guarded parse helper; invalid numeric args now return code 2.
  Evidence: `apps/run_serial.cpp` (`--progress-every`, `--sample`, `--top-n`, `--min-link-samples`, `--benchmark-runs`, `--expect-accepted`).
- [x] P1-D10 reproducibility: avoid overwrite-prone output naming in phase1 benchmark runner.
  Objective/result: per-scenario output naming now includes branch/commit/timestamp; manifest rows remain immutable across batches.
  Evidence: `scripts/run_phase1_dev_benchmarks.sh:173`.
- [ ] P1-D3 maintainability: remove loader parsing redundancy.
  Objective: centralize shared CSV parse/timestamp helper logic used by `CSVReader`, `GarageLoader`, and `BuildingLoader`.
  Evidence: duplicated helpers across `src/io/CSVReader.cpp`, `src/io/GarageLoader.cpp`, `src/io/BuildingLoader.cpp`.
- [x] P1 test depth: add explicit negative CLI tests for malformed numeric parameters.
  Objective/result: regression coverage added to assert graceful failure (exit code 2), no crash.
  Evidence: `tests/test_run_serial_benchmark_cli.cpp`.

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
