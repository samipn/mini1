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
- Full parallel benchmark CSV integration into `BenchmarkHarness` is still a later step (Phase 2 D12), so run-level thread data is currently captured in CLI logs rather than harness output files.

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
