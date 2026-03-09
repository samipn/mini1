# Phase 2 Detailed Review (D6-D10)

Date: 2026-03-09 (UTC)
Scope: Phase 2 Deliverables 6-10 only.

Reviewed deliverables:
- D6 Parallel time-window query
- D7 Parallel aggregation queries
- D8 Top-N/ranking parallelization
- D9 Parallel CLI runner
- D10 Thread-count controls

## Findings (ordered by severity)

1. High: `run_parallel` default query behavior is operationally inconsistent.
   - Behavior: default `query_type` is `speed_below`, but threshold is required and not defaulted.
   - Impact: `run_parallel --traffic <path>` fails immediately, reducing D9 CLI usability.
   - Evidence:
     - `apps/run_parallel.cpp:107`
     - `apps/run_parallel.cpp:205`

2. Medium: explicit `--thread-list` currently allows `0`, producing ambiguous benchmark rows.
   - Behavior: parser rejects negative thread values but allows `0`, which is later emitted as `threads=0` in output rows.
   - Impact: D10 thread-count semantics become unclear in artifacts.
   - Evidence:
     - `apps/run_parallel.cpp:89`
     - `apps/run_parallel.cpp:92`
     - `apps/run_parallel.cpp:278`

3. Medium: Top-N parallel path can incur high transient memory overhead at larger thread counts.
   - Behavior: one `unordered_map` is allocated per thread, merged after scan.
   - Impact: no correctness failure, but can degrade scalability for high-cardinality link sets.
   - Evidence:
     - `src/query/ParallelCongestionQuery.cpp:146`
     - `src/query/ParallelCongestionQuery.cpp:167`

## Memory / Leak Notes (D6-D10 scope)

- No leak patterns found.
- Main risk is transient memory overhead in top-N thread-local map strategy, not ownership leakage.

## Recommended Remediation (D6-D10)

1. Align default CLI query policy (`summary` default or default threshold policy).
2. Reject `0` in `--thread-list` to keep artifact semantics explicit.
3. Add optional capacity heuristics/reserve strategy for thread-local top-N maps.
