# Phase 1 Notes

## Deliverables 1-6 (current)
- `D1` Build system established with CMake, strict warnings, and `run_serial`/`run_parallel`/`run_optimized` targets.
- `D2` Core data model added: `TrafficRecord`, `TrafficDataset`, `GarageRecord`, `BuildingRecord`, and `GeoPoint`.
- `D3` Serial traffic CSV ingestion implemented with configurable progress logging, row validation, rejection counters, and simple supporting loaders for garages/BES CSVs.
- `D4` Dataset container APIs expanded with direct indexed access (`At`) and iterator support for reusing a loaded dataset across multiple query runs.
- `D5` Query API layer implemented with `TimeWindowQuery`, `CongestionQuery`, and `TrafficAggregator` for time-window, threshold, borough, link-id/range, top-N slowest recurring links, and aggregate statistics.
- `D6` Serial CLI runner extended with query modes and query timing logs (`[query] start`, `[query] complete`, `elapsed_ms`) plus parameterized query arguments.

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

## Open issues
- Benchmark harness integration and repeated-run result export are deferred to Deliverable 7+.
