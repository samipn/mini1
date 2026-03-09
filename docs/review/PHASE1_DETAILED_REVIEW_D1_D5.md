# Phase 1 Detailed Review (D1-D5)

Date: 2026-03-09 (UTC)
Scope: Phase 1 Deliverables 1-5 only.

Reviewed deliverables:
- D1 Repository and build validation
- D2 Core data model
- D3 Serial file ingestion (traffic + supporting loaders)
- D4 Dataset container APIs
- D5 Phase 1 query APIs

## Findings (ordered by severity)

1. High: supporting loaders append into caller vectors without clearing prior contents.
   - Behavior: `GarageLoader::LoadCSV` and `BuildingLoader::LoadCSV` reset `LoaderStats` but never clear `out_records`, so repeated loads into the same vector silently duplicate accepted rows.
   - Impact: stale-state accumulation and false record counts across repeated ingest runs.
   - Evidence:
     - `src/io/GarageLoader.cpp:173`
     - `src/io/GarageLoader.cpp:310`
     - `src/io/BuildingLoader.cpp:137`
     - `src/io/BuildingLoader.cpp:183`

2. Medium: warning flags are only applied to `congestion_core`, not app/test targets.
   - Behavior: strict warnings (`-Wall -Wextra -Wpedantic`) are configured on library target only; `run_serial` and test binaries do not inherit these options by target configuration.
   - Impact: D1 warning policy is partially enforced, leaving CLI/test compilation paths less strict.
   - Evidence:
     - `CMakeLists.txt:63`
     - `CMakeLists.txt:69`
     - `CMakeLists.txt:86`

3. Medium: borough dual-representation can produce query false negatives when records are inserted directly.
   - Behavior: borough filter prefers `record.borough_code` for recognized borough names; `TrafficDataset::AddRecord` does not normalize `borough` -> `borough_code`. Directly added records with only `borough` set can be missed by `FilterBoroughAndSpeedBelow`.
   - Impact: API-level correctness drift outside CSV-ingest path.
   - Evidence:
     - `include/data_model/TrafficRecord.hpp:13`
     - `include/data_model/TrafficRecord.hpp:14`
     - `src/data_model/TrafficDataset.cpp:5`
     - `src/query/CongestionQuery.cpp:26`
     - `src/query/CongestionQuery.cpp:29`

4. Medium: CSV parsing/timestamp helper logic remains duplicated across loaders.
   - Behavior: `Trim`, `ToLower`, `ParseCsvLine`, and parse helpers are independently implemented in traffic, garage, and building loaders.
   - Impact: higher maintenance cost and elevated risk of behavior drift across ingest paths.
   - Evidence:
     - `src/io/CSVReader.cpp:22`
     - `src/io/GarageLoader.cpp:20`
     - `src/io/BuildingLoader.cpp:15`

## Memory / Leak Notes (D1-D5 scope)

- No direct memory leaks found in D1-D5 code paths (no unmanaged owning heap usage).
- Main risks are operational correctness and maintainability, not leak behavior.

## Recommended Remediation (D1-D5)

1. Clear `out_records` at loader entry (or explicitly document append semantics and rename APIs to reflect append mode).
2. Apply warning flags consistently to app/test targets (or define a shared warning interface target linked by all).
3. Normalize borough fields on ingest/insert boundary (or enforce invariant checks in `AddRecord` and tests).
4. Consolidate shared CSV/parse helpers into a common utility used by all loaders.
