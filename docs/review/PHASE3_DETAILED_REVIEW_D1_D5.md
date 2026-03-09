# Phase 3 Detailed Review (D1-D5)

Date: 2026-03-09 (UTC)
Scope: Phase 3 Deliverables 1-5 only.

Reviewed deliverables:
- D1 Freeze comparison baselines
- D2 Identify/profile hot paths
- D3 Object-of-Arrays layout design
- D4 Conversion/direct-load path
- D5 Optimized query APIs

## Findings (ordered by severity)

1. High: `run_optimized` row-convert mode can silently succeed on unknown query types.
   - Behavior: `--load-mode row_convert` bypasses benchmark harness query validation and falls through with `result_count=0`, returning exit code `0`.
   - Repro:
     - `./build/run_optimized --traffic <csv> --query does_not_exist --load-mode row_convert`
   - Evidence:
     - `apps/run_optimized.cpp:280`
     - `apps/run_optimized.cpp:295`
     - `apps/run_optimized.cpp:337`
     - `apps/run_optimized.cpp:346`

2. High: `TrafficColumns::AddRecord` lacks strong exception safety and can desynchronize SoA columns on allocation failure.
   - Behavior: pushes to 5 column vectors before `InternLinkName`; if interning throws (`bad_alloc`), `link_name_ids_` is not updated, breaking index alignment guarantees.
   - Evidence:
     - `src/data_model/TrafficColumns.cpp:39`
     - `src/data_model/TrafficColumns.cpp:44`
     - `src/data_model/TrafficColumns.cpp:75`
   - Risk category: operational fault under memory pressure; latent data-structure corruption.

3. Medium: D2 profiling claim is weakly evidenced.
   - Observation: hotspot choices are documented, but there is no direct profiler artifact (perf/callgrind output) in repository evidence chain for D2.
   - Evidence:
     - `docs/TODO_PHASE3.md:79` (claims profiling/timing confirmation)
     - no profiler artifact references under `results/raw/phase3_*` or `report/`.

4. Medium: Conversion-path benchmarking parity is incomplete in CLI behavior.
   - Behavior: `row_convert` path in `run_optimized` does not honor benchmark/validation output controls (`--benchmark-out`, repeated runs, serial validation mode) the same way direct mode does.
   - Evidence:
     - direct mode uses `BenchmarkHarness::RunOptimized`: `apps/run_optimized.cpp:246`
     - row_convert path executes one-off query and returns without benchmark rows: `apps/run_optimized.cpp:280`

## Memory / Leak Notes (D1-D5 scope)

- No direct memory leaks found in D1-D5 code paths (RAII containers, no manual heap ownership).
- Main memory-related D1-D5 risk is exception-safety integrity in SoA insertion, not leak.

## Recommended Remediation (D1-D5)

1. Enforce unknown-query rejection in `run_optimized` row-convert mode (exit code `2` + usage/error message).
2. Refactor `TrafficColumns::AddRecord` for atomic consistency under exceptions (compute link-name id first and only then append all columns, or rollback on failure).
3. Add a profiler evidence task or explicit “timing-breakdown-only” wording adjustment for D2.
4. Decide whether row-convert mode should support full benchmark harness semantics; if not, document this as explicit non-goal.
