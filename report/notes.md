# Phase 1 Notes

## Deliverables 1-3 (current)
- `D1` Build system established with CMake, strict warnings, and `run_serial`/`run_parallel`/`run_optimized` targets.
- `D2` Core data model added: `TrafficRecord`, `TrafficDataset`, `GarageRecord`, `BuildingRecord`, and `GeoPoint`.
- `D3` Serial traffic CSV ingestion implemented with configurable progress logging, row validation, rejection counters, and simple supporting loaders for garages/BES CSVs.

## Parsing assumptions
- Required traffic columns are `link_id`, `speed`, `travel_time`, and `data_as_of`.
- Accepted timestamp formats are `YYYY-MM-DDTHH:MM:SS[.sss][Z]` and `YYYY-MM-DD HH:MM:SS`.
- Rows with non-numeric `link_id`, `speed`, or `travel_time` are rejected as malformed.
- Rows with missing or unparseable timestamps are rejected and counted separately.

## Validation and quality flags
- `speed_mph < 0` or `speed_mph > 120` increments `suspicious_speed`.
- `travel_time_seconds < 0` increments `suspicious_travel_time` and the row is rejected by core-field validation.

## Open issues
- Query APIs and benchmark harness integration are deferred to Deliverables 4+.
