# Phase 1 Detailed Review

Date: 2026-03-09 (UTC)
Scope: Phase 1 deliverables (`docs/TODO_PHASE1.md`) with emphasis on redundancy, memory leaks, and operational faults.

## Summary

- Memory leak review: no direct heap leak patterns found (no unmanaged `new`/`delete` ownership in Phase 1 hot paths).
- Operational fault review: two concrete reliability faults found.
- Redundancy review: one high-maintenance duplication hotspot found.

## Findings (ordered by severity)

1. High: `run_serial` can abort on invalid numeric CLI input (uncaught exceptions).
   - Behavior: args parsed with `std::stoull` throw `std::invalid_argument`/`std::out_of_range`, causing process abort instead of user-facing error.
   - Repro:
     - `./build/run_serial --traffic data/subsets/i4gi-tjb9_subset_10000.csv --progress-every notnum`
     - exits with abort (`what(): stoull`) instead of clean exit code 2.
   - Evidence:
     - `apps/run_serial.cpp:108`
     - `apps/run_serial.cpp:110`
     - `apps/run_serial.cpp:128`
     - `apps/run_serial.cpp:130`
     - `apps/run_serial.cpp:132`
     - `apps/run_serial.cpp:140`

2. High: Phase 1 benchmark script overwrites scenario CSVs across batches.
   - Behavior: output file naming omits timestamp/commit; repeated runs overwrite prior files while old manifests still reference mutable paths.
   - Operational risk: benchmark lineage/reproducibility drift and accidental data loss.
   - Evidence:
     - `scripts/run_phase1_dev_benchmarks.sh:173`
     - filename pattern currently: `results/raw/phase1_dev/<subset>_<scenario>.csv`

3. Medium: CSV parsing/timestamp helper logic duplicated across loaders.
   - Behavior: `Trim`, `ToLower`, `ParseCsvLine`, parsing helpers repeated in:
     - `src/io/CSVReader.cpp`
     - `src/io/GarageLoader.cpp`
     - `src/io/BuildingLoader.cpp`
   - Risk: bug fixes can diverge; behavior inconsistencies are likely over time.

4. Medium: CLI test coverage does not currently assert graceful handling for malformed numeric arguments.
   - Current tests validate success path, but not invalid-number robustness.
   - Evidence:
     - `tests/test_run_serial_benchmark_cli.cpp`

## Memory Notes

- No obvious memory leaks identified in Phase 1 codepaths.
- Dataset/vector growth is bounded by ingest size and released by RAII at scope end.
- Main memory risk in Phase 1 is operational (artifact overwrite), not leak.

## Recommended Next Fixes

1. Introduce safe integer parsing for all `stoull` CLI fields in `run_serial` and return exit code 2 on invalid input.
2. Update `run_phase1_dev_benchmarks.sh` output naming to include branch/commit/timestamp (same style as phase2/phase3 scripts).
3. Extract shared CSV/parse helpers into a common utility module used by all loaders.
4. Add CLI negative tests for malformed numeric arguments.

## Resolution Status (Current Branch)

- Completed:
1. Safe numeric parsing hardening in `run_serial` (invalid numeric args now return code 2).
2. Phase 1 benchmark output naming now includes branch/commit/timestamp (no overwrite-prone fixed names).
3. Negative CLI regression test added for malformed numeric arguments.
- Open:
1. Shared loader parsing helper consolidation remains pending (maintainability task).
