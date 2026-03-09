# Phase 2 Detailed Review

Date: 2026-03-09 (UTC)
Scope: Phase 2 deliverables (`docs/TODO_PHASE2.md`) reviewed end-to-end, with emphasis on redundancy, memory leaks, and operational faults.

## Deliverable Coverage

All Phase 2 deliverables (D1-D15) were reviewed against:
- code paths (`run_parallel`, parallel query/aggregation implementations, benchmark harness)
- tests (`ctest`, phase2 correctness suites)
- artifacts/scripts (`run_phase2_dev_benchmarks.sh`, determinism scripts, summary/plot scripts)

Verification baseline:
- Full test suite passes (`16/16`).
- Phase 2 determinism check passes on subset runs.
- Deliverable/token audits are passing in review docs.

## Findings (ordered by severity)

1. Medium: `run_parallel` default behavior is operationally inconsistent.
   - `query_type` defaults to `speed_below`, but threshold is not default-enabled.
   - Running only `--traffic <path>` exits with `speed_below requires --threshold`.
   - This is not a crash, but it is confusing/default-hostile.
   - Evidence:
     - `apps/run_parallel.cpp:87`
     - `apps/run_parallel.cpp:101`
     - `apps/run_parallel.cpp:173`
     - `apps/run_parallel.cpp:174`

2. Medium: `--thread-list` accepts `0` values and runs them as implicit "auto/max threads".
   - Parse logic rejects `< 0` but allows `0`.
   - Output reports `threads=0`, which is ambiguous operationally.
   - Evidence:
     - `apps/run_parallel.cpp:69`
     - `apps/run_parallel.cpp:72`
   - Repro:
     - `./build/run_parallel --traffic ... --query summary --thread-list 0,1 --benchmark-runs 1`

3. Low/Medium: CLI numeric parsing logic is redundant across binaries.
   - Similar `ParseInt64/ParseDouble/ParseSizeT` logic appears independently in:
     - `apps/run_serial.cpp`
     - `apps/run_parallel.cpp`
     - `apps/run_optimized.cpp`
   - Risk: future fixes diverge between binaries.

## Memory/Leak Review

- No direct memory-leak patterns found in Phase 2 query/CLI paths.
- Containers use RAII (`std::vector`, `std::unordered_map`) and no unmanaged ownership patterns.
- Main Phase 2 risk is operational consistency, not leak behavior.

## Recommended Next Fixes

1. Decide a consistent default query policy for `run_parallel`:
   - either default query to `summary`, or
   - keep `speed_below` default but set/announce a default threshold.
2. Validate `--thread-list` entries as strictly `> 0` and normalize list (dedupe/sort optional).
3. Move shared numeric/CLI parse helpers into a reusable utility used by all app runners.
