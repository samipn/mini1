# Phase 2 Detailed Review (D1-D5)

Date: 2026-03-09 (UTC)
Scope: Phase 2 Deliverables 1-5 only.

Reviewed deliverables:
- D1 Preserve/lock Phase 1 baseline
- D2 Parallel build support
- D3 Parallelization target definition
- D4 Parallel query infrastructure
- D5 Parallel speed-threshold query

## Findings (ordered by severity)

1. Medium: strict warning policy is not uniformly applied beyond core library target.
   - Behavior: warning flags are attached to `congestion_core`, but not explicitly to app/test targets (`run_parallel`, test binaries).
   - Impact: D2 "keep compile warnings enabled" is only partially enforced in practice.
   - Evidence:
     - `CMakeLists.txt:59`
     - `CMakeLists.txt:67`
     - `CMakeLists.txt:85`

2. Medium: OpenMP thread selection uses process-global `omp_set_num_threads` in query paths.
   - Behavior: parallel query methods call `omp_set_num_threads(...)` on each request.
   - Impact: operational coupling risk if multiple benchmark/query paths run concurrently in the same process.
   - Evidence:
     - `src/query/ParallelCongestionQuery.cpp:48`
     - `src/query/ParallelTrafficAggregator.cpp:41`

3. Low: D5 implementation intentionally returns counts-only for speed-threshold path.
   - Behavior: parallel speed query returns count (no materialized records), while serial API has both vector and count semantics.
   - Impact: acceptable for Phase 2 scope, but should remain explicit in documentation to avoid API expectation drift.
   - Evidence:
     - `include/query/ParallelCongestionQuery.hpp:14`
     - `src/query/ParallelCongestionQuery.cpp:37`

## Memory / Leak Notes (D1-D5 scope)

- No direct memory leaks found in D1-D5 paths.
- Containers and loader/query objects are RAII-managed.

## Recommended Remediation (D1-D5)

1. Apply warning-policy flags via an interface target and link all apps/tests to it.
2. Replace global `omp_set_num_threads` use with scoped `num_threads(...)` clauses where practical.
3. Keep count-only parallel query semantics explicitly documented in Phase 2 notes/tests.
