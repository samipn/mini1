# Phase 3 Detailed Review (D6-D10)

Date: 2026-03-09 (UTC)
Scope: Phase 3 Deliverables 6-10 only.

Reviewed deliverables:
- D6 Reduce hot-loop overhead
- D7 Evaluate memory usage changes
- D8 Optional query-support optimizations
- D9 Integrate parallelism with optimized layout
- D10 Implement optimized CLI runner

## Findings (ordered by severity)

1. High: D6 hot-loop path still uses `std::function` predicate dispatch per row.
   - Behavior: `OptimizedTrafficAggregator::SummarizeConditional` accepts `std::function<bool(std::size_t)>` and invokes it inside scan loops; this adds type-erased call overhead in critical loops and conflicts with the D6 objective to minimize hot-loop overhead and dispatch cost.
   - Evidence:
     - `src/query/OptimizedTrafficAggregator.cpp:13`
     - `src/query/OptimizedTrafficAggregator.cpp:39`
     - `src/query/OptimizedTrafficAggregator.cpp:50`

2. High: D8 support experiment CLI aborts on malformed numeric input.
   - Behavior: `run_optimized_support_experiments` parses `--repeats` with raw `std::stoull` and no error handling; malformed input terminates with `SIGABRT` instead of controlled nonzero usage failure.
   - Repro:
     - `./build/run_optimized_support_experiments --traffic /tmp/urbandrop_review_fixture.csv --repeats nope`
     - observed `rc=134` and `std::invalid_argument: stoull`
   - Evidence:
     - `apps/run_optimized_support_experiments.cpp:43`

3. Medium: D7 memory probe can record rows after failed commands without failure metadata.
   - Behavior: memory probe uses `/usr/bin/time ... || true`, then always emits a CSV row. Failed runs can be logged as if successful, with no status field for downstream filtering.
   - Evidence:
     - `scripts/run_phase3_memory_probe.sh:106`
     - `scripts/run_phase3_memory_probe.sh:116`

4. Medium: D10 row-convert mode still diverges from benchmark semantics.
   - Behavior: direct mode uses `BenchmarkHarness::RunOptimized`; row-convert mode runs one-off query execution and does not emit benchmark rows, repeated-run metrics, or validation output parity.
   - Evidence:
     - `apps/run_optimized.cpp:246`
     - `apps/run_optimized.cpp:280`
     - `apps/run_optimized.cpp:339`

5. Medium: D9 parallel optimized path lacks explicit thread-value guard in query kernels.
   - Behavior: optimized query kernels pass `num_threads` directly into OpenMP pragmas. CLI-level sanitization currently keeps values safe in normal use, but kernels themselves do not defend against `0` when called by non-CLI callers.
   - Evidence:
     - `src/query/OptimizedCongestionQuery.cpp:33`
     - `src/query/OptimizedCongestionQuery.cpp:74`
     - `src/query/OptimizedCongestionQuery.cpp:112`
     - `src/query/OptimizedCongestionQuery.cpp:184`

## Memory / Leak Notes (D6-D10 scope)

- No direct memory leaks found in D6-D10 code paths (RAII containers, no raw owning pointers in reviewed files).
- Primary operational memory risk in this scope is measurement integrity (probe failure masking), not leak.

## Recommended Remediation (D6-D10)

1. Replace `std::function`-based aggregation predicate path with templated/inlined predicates (or dedicated specialized loops) for hot query variants.
2. Harden `run_optimized_support_experiments` numeric parsing (shared safe parse helper, return code `2` on invalid args) and add a negative CLI test.
3. Add `status`/`exit_code` fields to memory probe CSV and remove failure swallowing (`|| true`) or mark failed rows explicitly.
4. Either route row-convert through benchmark harness semantics or clearly document/test row-convert as intentionally non-benchmark mode.
5. Add defensive guard inside optimized parallel kernels for `num_threads == 0` (normalize to `1` or OpenMP max) to improve API robustness.
