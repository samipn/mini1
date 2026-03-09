# Phase 1 Notes

## Deliverables 1-10 (current)
- `D1` Build system established with CMake, strict warnings, and `run_serial`/`run_parallel`/`run_optimized` targets.
- `D2` Core data model added: `TrafficRecord`, `TrafficDataset`, `GarageRecord`, `BuildingRecord`, and `GeoPoint`.
- `D3` Serial traffic CSV ingestion implemented with configurable progress logging, row validation, rejection counters, and simple supporting loaders for garages/BES CSVs.
- `D4` Dataset container APIs expanded with direct indexed access (`At`) and iterator support for reusing a loaded dataset across multiple query runs.
- `D5` Query API layer implemented with `TimeWindowQuery`, `CongestionQuery`, and `TrafficAggregator` for time-window, threshold, borough, link-id/range, top-N slowest recurring links, and aggregate statistics.
- `D6` Serial CLI runner extended with query modes and query timing logs (`[query] start`, `[query] complete`, `elapsed_ms`) plus parameterized query arguments.
- `D7` Benchmark harness implemented (`BenchmarkHarness`) with repeated serial runs, ingest/query/total timing, accepted-row consistency checks, and CSV export metadata.
- `D8` Validation extended with accepted-row expectation checks (`--expect-accepted`) for both single runs and benchmark runs.
- `D9` Research notes expanded to include benchmark behavior, caveats, and dataset-specific issues.
- `D10` Baseline benchmark scenarios scripted in `scripts/benchmark.sh` and summarized with `scripts/plot_results.py`.

## Parsing assumptions
- Required traffic columns are `link_id`, `speed`, `travel_time`, and `data_as_of`.
- Accepted timestamp formats are `YYYY-MM-DDTHH:MM:SS[.sss][Z]` and `YYYY-MM-DD HH:MM:SS`.
- Rows with non-numeric `link_id`, `speed`, or `travel_time` are rejected as malformed.
- Rows with missing or unparseable timestamps are rejected and counted separately.

## Validation and quality flags
- `speed_mph < 0` or `speed_mph > 120` increments `suspicious_speed`.
- `travel_time_seconds < 0` increments `suspicious_travel_time` and the row is rejected by core-field validation.

## Test coverage (current)
- `test_traffic_dataset`: dataset container behavior (`Size`, `Empty`, counters).
- `test_csv_reader`: traffic CSV parsing, accepted/rejected row counters, quoted-field handling.
- `test_support_loaders`: garages/BES basic loader acceptance and rejection logic.
- `test_query_apis`: all D4/D5 query paths and aggregations on deterministic fixture data.
- Real-data sample fixture strategy:
  - tests share one reusable sample file generated from the first 1000 data lines of `/datasets/i4gi-tjb9.csv` (override with `URBANDROP_TRAFFIC_CSV`).
  - fixture file is cached in temp storage and reused across test binaries for faster repeated runs.
- `test_realdata_ingest`: real-data ingest-only validation on the shared 1000-line sample.
- `test_realdata_api`: real-data query API-only validation on the shared 1000-line sample.
- `test_realdata_cli`: real-data CLI-only validation on the shared 1000-line sample.
- `test_realdata_integrated`: end-to-end integrated validation (ingest + APIs + CLI) on the same shared sample.
- `test_benchmark_harness`: benchmark API-level repeated runs and CSV export format/row count.
- `test_run_serial_benchmark_cli`: benchmark CLI mode end-to-end with output CSV generation.

## Benchmark notes (2026-03-08, sample execution)
- Dataset used for recorded run: first 1000 data rows from `/datasets/i4gi-tjb9.csv` (saved to `/tmp/phase1_sample1000.csv` for benchmark command execution).
- Scenarios executed at 10 runs each:
  - ingest only
  - speed threshold
  - time window
  - borough + threshold
  - top-N slowest links
- Raw outputs written to:
  - `results/raw/serial_ingest_only.csv`
  - `results/raw/serial_speed_below.csv`
  - `results/raw/serial_time_window.csv`
  - `results/raw/serial_borough_speed_below.csv`
  - `results/raw/serial_top_n_slowest.csv`
- Summary written to:
  - `results/raw/serial_summary.csv`
- Observed runtime pattern on sample run:
  - ingest dominated total runtime for all scenarios
  - query cost was near-zero for basic filters and slightly higher for top-N grouping/sort

## Failed attempts / caveats
- Early TODO tracking and branch naming drift required correction (`P1-D7-F` selected as active branch for finishing phase).
- Integrated test strategy was initially monolithic; refactored into ingest/API/CLI sub-tests plus integrated test for better diagnosis and rerun speed.
- Benchmark values from a 1000-row sample are only functional baseline checks and are not representative of full-dataset production timing.

## Dataset-specific issues
- Real traffic CSV is extremely large (~15 GB locally), so fast iteration requires sampling for development tests.
- Borough-filter benchmark behavior depends on borough coverage in the selected sample or snapshot.
- Timestamp parsing accepts two normalized formats; malformed or missing timestamps are rejected and tracked.

## Open issues
- Full-dataset benchmark runs should be executed and archived before phase handoff if final report numbers require non-sampled measurements.

## Phase 2 Notes (D1-D5)
- Branch: `P2-D1-5`.
- Parallel model selected for Phase 2 first pass: **OpenMP**.
- Serial baseline preserved; ingestion remains serial for fair comparison focus.
- Implemented parallel query infrastructure:
  - `ParallelCongestionQuery` using OpenMP parallel-for reductions for count-based scans.
  - `ParallelTrafficAggregator` using OpenMP reductions for dataset-wide averages.
- Implemented query paths in this pass:
  - `speed_below` (parallelized)
  - `borough_speed_below` (parallelized)
  - `summary` aggregation (parallelized infrastructure support)
- Implemented parallel CLI:
  - `run_parallel` supports `--traffic`, `--query`, `--threshold`, `--borough`, `--threads`, `--benchmark-runs`, and `--validate-serial`.
  - CLI logs include OpenMP model and maximum runtime thread capacity.
- Correctness and testing:
  - `test_parallel_query_correctness` compares parallel vs serial across multiple thread counts.
  - `test_run_parallel_cli` validates end-to-end CLI behavior and serial-parallel parity.
  - Real-data smoke validation on first 1000 lines from `/datasets/i4gi-tjb9.csv` confirms `speed_below` parallel counts match serial counts for repeated runs.

## Phase 2 D1-D5 caveats
- Current parallel scan API returns counts rather than full materialized record vectors to keep first-pass thread safety simple.
- Time-window query parallelization was deferred at D1-D5 and is now implemented in D6-D10.

## Phase 2 Notes (D6-D10)
- Continued branch: `P2-D6-10`.
- OpenMP stack retained and extended (no fallback to `std::thread` in query execution paths).
- Added parallel time-window support:
  - count-based scan with OpenMP reduction
  - deterministic inclusive range comparisons
  - serial-vs-parallel parity checks in tests and CLI validation mode
- Added parallel aggregation expansions:
  - full dataset aggregation
  - speed-threshold conditional aggregation
  - borough+threshold conditional aggregation
  - time-window conditional aggregation
- Top-N decision/result for Phase 2:
  - implemented parallel top-N slowest recurring links using thread-local maps + merge + sort
  - validated output parity against serial top-N in tests and `--validate-serial`
- Extended `run_parallel` CLI:
  - new query modes: `time_window`, `top_n_slowest`
  - new args: `--start-epoch`, `--end-epoch`, `--top-n`, `--min-link-samples`, `--thread-list`
  - logs now include query start/end, elapsed time, dataset accepted row count, query type, and thread count
- Thread-count controls:
  - supports explicit `--threads` and deterministic `--thread-list` sweeps (for example `1,2,4,8`)
  - no auto-tuning between runs unless user requests thread `0` default behavior
  - thread count is emitted in benchmark-style run logs per query execution

## Phase 2 D6-D10 caveats
- Full parallel benchmark CSV integration into `BenchmarkHarness` was completed in D11-D15.

## Phase 2 Notes (D11-D15)
- Branch: `P2-D11-15`.
- Correctness validation coverage is now enforced in both test and benchmark paths:
  - `test_parallel_query_correctness` verifies serial-vs-parallel parity for speed threshold, borough+threshold, time window, summary aggregate, and top-N.
  - `BenchmarkHarness::RunParallel` can run serial validation back-to-back (`validate_parallel_against_serial`) and fail fast on mismatch.
- `BenchmarkHarness` was extended for parallel experiments:
  - execution mode metadata (`serial` or `parallel`)
  - explicit `thread_count` per run
  - serial-validation status (`serial_match`)
  - graph-friendly CSV output for both serial and parallel runs
- `run_parallel` benchmark mode now uses the shared harness and supports:
  - `--benchmark-out`, `--benchmark-append`, `--dataset-label`, `--expect-accepted`
  - thread sweep via `--thread-list`
  - validation gate via `--validate-serial`
- Benchmark pipeline updates:
  - `scripts/benchmark.sh` now runs serial+parallel scenario pairs with thread sweeps.
  - `scripts/summarize_phase2.py` computes grouped runtime stats and serial-relative speedup.
- Development benchmark batch executed on `2026-03-09`:
  - dataset: first 1000 data rows from `/datasets/i4gi-tjb9.csv` (`/tmp/traffic_1000.csv`)
  - runs per scenario: `10`
  - thread counts: `1,2,4,8`
  - raw outputs: `results/raw/phase2/*.csv`
  - summary output: `results/tables/phase2/summary.csv`
- Observations from this batch (query-time focused):
  - no strong speedup on the 1000-row subset; ingest dominates end-to-end runtime.
  - most parallel query modes were slower than serial baseline at all tested thread counts.
  - occasional high-latency outliers appeared at high thread counts (especially 8 threads), indicating overhead/imbalance sensitivity on small workloads.
- Interpreted overhead causes:
  - thread startup/scheduling overhead dominates small scans.
  - reduction/merge overhead can exceed useful work at low record counts.
  - higher thread counts amplify jitter and synchronization overhead on this subset scale.
- Numerical behavior:
  - no material serial-vs-parallel aggregate mismatches observed in validation runs (floating-point tolerance `1e-9`).
- Ingestion parallelization status:
  - not attempted in this chunk; ingestion remains serial by design to preserve experiment focus.

## Phase 2 Notes (D17-D21)
- Branch: `P2-D17-21`.
- Date: `2026-03-09`.
- Scope completed:
  - subset dataset reuse/validation and manifest generation
  - serial-vs-parallel subset validation runs with aggregate parity reporting
  - repeatable subset benchmark scenarios + runner for serial/parallel pair execution
  - preliminary timing/scaling batch collection across required subset sizes and thread counts
- Added files:
  - `configs/phase2_dev_scenarios.conf`
  - `scripts/run_phase2_subset_validation.sh`
  - `scripts/run_phase2_dev_benchmarks.sh`
- Benchmark harness output was extended to include aggregate fields per run:
  - `has_aggregate`
  - `average_speed_mph`
  - `average_travel_time_seconds`
- Subset status:
  - In this workspace, prior Phase 1 subset files were missing.
  - Deterministic subsets were recreated from `/datasets/i4gi-tjb9.csv` at:
    - `data/subsets/i4gi-tjb9_subset_10000.csv`
    - `data/subsets/i4gi-tjb9_subset_100000.csv`
    - `data/subsets/i4gi-tjb9_subset_1000000.csv`
  - Source manifest: `data/subsets/manifest.csv`
- Phase 2 subset manifest/accessibility artifact:
  - `results/raw/phase2_dev/subset_manifest_20260309T040238Z.csv`
  - confirms expected row counts and serial/parallel runner accessibility.
- Validation batch artifact:
  - `results/raw/phase2_dev/validation/validation_20260309T035930Z.csv`
  - includes subset label, scenario, thread count, serial/parallel result counts, aggregate values, and `serial_match` status.
  - observed status in this run: all rows `ok`, no mismatches.
- Preliminary timing batch artifacts (`runs=3`, threads `1,2,4,8`):
  - manifest: `results/raw/phase2_dev/batch_20260309T040238Z_manifest.csv`
  - records/sec: `results/raw/phase2_dev/batch_20260309T040238Z_records_per_second.csv`
  - per-scenario raw files: `results/raw/phase2_dev/{small,medium,large_dev}_*_{serial|parallel}_P2-D17-21_aea201e_20260309T040238Z.csv`
  - logs: `results/raw/logs/phase2_dev_*_20260309T040238Z.log`
- Scenarios executed for each subset:
  - `speed_below_15`
  - `time_window_all`
  - `summary`
  - `borough_manhattan_speed_15`
- Notes:
  - Batch metadata now records branch, commit, subset label, scenario, and thread controls.
  - Query parity was enforced during parallel runs using `--validate-serial`.

## Phase 2 Notes (D22-D26)
- Branch: `P2-D22-26`.
- Added development summary + graph tooling for `phase2_dev` batches:
  - `scripts/summarize_phase2_dev.py`
  - `scripts/plot_phase2_dev.py` (dependency-free SVG output)
  - `scripts/check_phase2_dev_determinism.py`
- Generated summary table from latest batch manifest:
  - `results/tables/phase2_dev/phase2_dev_summary_20260309T040238Z.csv`
- Generated simple graphs:
  - `results/graphs/phase2_dev/runtime_vs_threads_*.svg`
  - `results/graphs/phase2_dev/speedup_vs_threads_*.svg`
  - `results/graphs/phase2_dev/serial_vs_parallel_by_subset_*.svg`
  - `results/graphs/phase2_dev/runtime_by_scenario_*.svg`
- Stability/determinism batch executed (medium subset, runs=5, threads=1,2,4,8):
  - `results/raw/phase2_dev/stability/stability_summary_20260309T041112Z.csv`
  - `results/raw/phase2_dev/stability/stability_report_20260309T041112Z.md`
  - Result: `PASS` (no result-count drift; aggregate ranges stable within expected floating-point precision).
- Added dedicated Phase 2 dev benchmark log:
  - `report/phase2_dev_benchmark_log.md`

## Phase 2 Notes (Chunk 4: Indexed Joins + Boost Evaluation)
- Branch: `P2-D27-30`.
- Added optional index/materialization infrastructure:
  - `include/query/CrossDatasetIndex.hpp`
  - `src/query/CrossDatasetIndex.cpp`
  - supports optional link-id count index (`traffic link_id -> count`)
  - supports BBL indexes (`garage bbl -> count`, `building bbl -> count`)
  - supports materialized shared-BBL relationship rows and borough rollups
- Added experiment runner:
  - `apps/run_index_experiments.cpp`
  - `scripts/run_phase2_index_experiments.sh`
  - compares baseline repeated scans vs indexed lookups and writes benchmark CSV rows
- Added correctness test:
  - `tests/test_cross_dataset_index.cpp`
- CMake integration:
  - `run_index_experiments` executable
  - `test_cross_dataset_index` in CTest
- Bench result sample (subset + synthetic garage/BES fixture):
  - output: `results/raw/phase2/index_lookup_benchmark.csv`
  - observed on this run:
    - baseline repeated link-id scans were slower than indexed link-id lookups
    - index build cost was measurable but small relative to repeated scan overhead
- Selective Boost evaluation:
  - measured optional prototype timings when headers are available for:
    - `boost::container::flat_map`
    - `boost::unordered::unordered_flat_map`
    - `boost::dynamic_bitset`
  - fallback behavior preserves STL-only execution when Boost components are unavailable.
- Keep/defer decisions:
  - Kept in Phase 2:
    - optional index prototypes
    - benchmarkable scan-vs-index comparison path
    - lightweight materialized relationship statistics
  - Deferred to Phase 3:
    - making indexed/materialized paths default for all query flows
    - deeper memory-layout/ownership redesign and high-cardinality compaction
    - broad replacement of STL maps in core runtime paths based solely on micro-benchs

## Phase 2 Notes (Chunk 5: Finalization)
- Branch: `P2-D31-F`.
- Closeout actions:
  - normalized remaining Phase 2 TODO parent checklist items so completion status aligns with implemented work.
  - marked Phase 2 Definition-of-Done checklist as complete.
  - kept explicit non-goals and not-applicable alternatives (`std::thread` path) unselected by design.
- Final verification pass requirements:
  - clean rebuild from `build/`
  - full CTest suite pass
  - Phase 2 dev artifacts remain organized under `phase2_dev` paths and separated from final baseline paths.
- Verification executed:
  - clean rebuild completed (`rm -r build`, reconfigure, rebuild)
  - CTest result: `13/13` tests passed
  - regenerated Phase 2 summary artifacts:
    - `results/tables/phase2/summary.csv`
    - `results/tables/phase2_dev/phase2_dev_summary_20260309T040238Z.csv`
    - `results/graphs/phase2_dev/*.svg`

## Pre-baseline classification (D20)
- All subset benchmark artifacts under:
  - `results/raw/phase1_dev/`
  - `results/tables/phase1_dev/`
  - `results/graphs/phase1_dev/`
  are classified as **development-stage (pre-baseline)**.
- These outputs are useful for:
  - validating benchmark workflow stability,
  - checking deterministic behavior,
  - and observing early scaling trends.
- They are **not** the final official Phase 1 comparison data.
- Final official full-dataset baseline outputs are reserved for:
  - `results/raw/phase1_baseline/`
  - `results/tables/phase1_baseline/`
  - `results/graphs/phase1_baseline/`

## Pre-baseline classification (Phase 2)
- All subset benchmark artifacts under:
  - `results/raw/phase2_dev/`
  - `results/tables/phase2_dev/`
  - `results/graphs/phase2_dev/`
  are classified as **development-stage (pre-baseline)**.
- These outputs are not final official Phase 2 baseline comparisons.
- Final official full-dataset Phase 2 comparison outputs are reserved for:
  - `results/raw/phase2_baseline/`
  - `results/tables/phase2_baseline/`
  - `results/graphs/phase2_baseline/`
