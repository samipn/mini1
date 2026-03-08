# UrbanDrop Mini 1A — Phase 2 TODO
## System 1: Congestion Intelligence Engine
## Phase 2 Goal: Parallelization

This document defines the Phase 2 implementation plan for System 1. The goal is to introduce **parallel processing** into the serial baseline built in Phase 1, measure the performance impact, and compare all results directly against the established baseline.

Phase 2 should improve throughput where parallel work is appropriate, while preserving correctness and keeping benchmark conditions comparable to Phase 1.

## Phase 2 Constraints
- Keep the Phase 1 serial implementation intact for direct comparison
- Do not replace the baseline; extend it
- Parallelize only after correctness is verified
- No database usage
- Avoid changing too many variables at once
- Do not begin full Object-of-Arrays redesign yet; that belongs to Phase 3
- Measure all changes against the same workloads used in Phase 1

---

## Phase 2 Success Criteria
By the end of Phase 2, the repository should contain:

1. A buildable parallel version of System 1
2. One or more parallel query paths using OpenMP or threads
3. Benchmark runs comparing serial and parallel execution
4. Thread-count experiments
5. Correctness checks showing parallel results match serial results
6. Raw benchmark outputs saved to disk
7. Notes explaining speedups, slowdowns, overheads, and failed attempts

---

## Phase 2 Strategy
Do **not** parallelize everything at once.

Parallelize in this order:

1. read-only query scans over loaded traffic data
2. filtered aggregations
3. top-N style analysis if safe and measurable
4. optional ingestion-adjacent work only if it does not distort the experiment

The safest and most defensible Phase 2 result is usually:
- serial ingestion
- parallel query execution

That keeps the experiment focused and easier to explain.

---

## Implementation Order

### 1. Preserve and Lock the Phase 1 Baseline
#### Tasks
- [ ] Keep `run_serial` unchanged except for bug fixes
- [ ] Tag or branch the repository at the Phase 1 completion point
- [ ] Confirm Phase 1 benchmark scenarios still run
- [ ] Save a stable copy of Phase 1 benchmark outputs for comparison
- [ ] Define identical datasets and query scenarios for Phase 2

#### Deliverable
- A known-good serial baseline exists for direct comparison

---

### 2. Add Parallel Build Support
#### Tasks
- [ ] Update `CMakeLists.txt` to support parallel builds
- [ ] If using OpenMP:
  - [ ] detect OpenMP in CMake
  - [ ] link required OpenMP flags/libraries
- [ ] If using `std::thread`:
  - [ ] ensure thread support is available and linked as needed
- [ ] Build a new executable:
  - [ ] `run_parallel`
- [ ] Keep compile warnings enabled
- [ ] Ensure Release mode is easy to build and run

#### Deliverable
- `run_parallel` builds successfully with the chosen parallel model

---

### 3. Define Parallelization Targets
#### Tasks
- [ ] Identify the most expensive read-only workloads from Phase 1
- [ ] Choose at least 2–3 query paths to parallelize
- [ ] Prioritize:
  - [ ] speed-threshold scans
  - [ ] time-window scans
  - [ ] borough + threshold scans
  - [ ] aggregations over the full record set
- [ ] Document why these paths were chosen

#### Good candidates
- [ ] scanning all traffic records for a predicate match
- [ ] computing average speed over subsets
- [ ] counting low-speed records by condition
- [ ] grouping partial results by thread and reducing at the end

#### Lower-priority candidates
- [ ] ingestion parsing
- [ ] string-heavy preprocessing
- [ ] anything requiring a lot of lock contention

#### Deliverable
- A documented list of parallelized workloads

---

### 4. Implement Parallel Query Infrastructure
#### Tasks
- [ ] Add parallel-capable query implementations
- [ ] Keep serial implementations available for comparison
- [ ] Create separate classes or methods as needed:
  - [ ] `ParallelCongestionQuery`
  - [ ] `ParallelTrafficAggregator`
- [ ] Avoid shared mutable output structures without a reduction strategy
- [ ] Use thread-local partial results where appropriate
- [ ] Merge partial results after parallel work completes

#### Deliverable
- Parallel query code exists without breaking the serial code path

---

### 5. Parallelize Speed-Threshold Query
#### Tasks
- [ ] Implement parallel scan over all loaded traffic records
- [ ] Evaluate predicate such as:
  - [ ] speed below threshold
- [ ] Store results safely:
  - [ ] thread-local vectors then merge, or
  - [ ] counts only if full record output is not needed
- [ ] Compare returned result count to serial version
- [ ] Add benchmark scenario using the same threshold as Phase 1

#### Deliverable
- Parallel low-speed scan returns the same result as the serial baseline

---

### 6. Parallelize Time-Window Query
#### Tasks
- [ ] Implement parallel scan for records in a selected time window
- [ ] Ensure timestamp comparisons are deterministic
- [ ] Compare:
  - [ ] record count
  - [ ] aggregate metrics
  - [ ] optional sample records
- [ ] Benchmark against the same time windows used in Phase 1

#### Deliverable
- Parallel time-range query matches serial behavior

---

### 7. Parallelize Aggregation Queries
#### Tasks
- [ ] Add parallel aggregation for:
  - [ ] average speed
  - [ ] average travel time
  - [ ] counts by condition
- [ ] Use thread-local accumulators
- [ ] Reduce safely after the parallel region
- [ ] Verify numerical behavior is acceptable
- [ ] Note if floating-point reduction order changes tiny decimal results

#### Deliverable
- Parallel aggregations produce acceptable and explainable results

---

### 8. Evaluate Top-N or Ranking Queries Carefully
#### Tasks
- [ ] Decide whether top-N slowest query should be parallelized in Phase 2
- [ ] If yes:
  - [ ] compute thread-local candidates
  - [ ] merge and rank after parallel scan
- [ ] Compare ranking output to serial result
- [ ] If too complex or too slow, document why and defer deeper optimization to Phase 3

#### Deliverable
- Either a working parallel top-N flow or a documented reason for deferral

---

### 9. Extend the CLI Runner for Parallel Execution
#### Tasks
- [ ] Implement `apps/run_parallel.cpp`
- [ ] Accept command-line arguments for:
  - [ ] traffic dataset path
  - [ ] query type
  - [ ] thresholds
  - [ ] time ranges
  - [ ] number of threads
  - [ ] repetitions
- [ ] Print logs showing:
  - [ ] thread count
  - [ ] query type
  - [ ] dataset size
  - [ ] query start/end
  - [ ] elapsed time
- [ ] Return nonzero exit code on failure

#### Deliverable
- Parallel executable runs benchmarkable workloads with configurable thread counts

---

### 10. Add Thread Count Controls
#### Tasks
- [ ] Support explicit thread counts such as:
  - [ ] 1
  - [ ] 2
  - [ ] 4
  - [ ] 8
  - [ ] maximum available reasonable count
- [ ] Ensure `1 thread` mode can be compared against serial behavior
- [ ] Record thread count in benchmark outputs
- [ ] Avoid auto-tuning that changes behavior between runs

#### Deliverable
- Benchmark runs can compare scaling across thread counts

---

### 11. Add Correctness Validation for Parallel Results
#### Tasks
- [ ] Compare serial vs parallel outputs for all supported query types
- [ ] Validate:
  - [ ] result counts
  - [ ] aggregate values
  - [ ] sampled outputs where needed
- [ ] Add a validation mode that runs both versions back-to-back
- [ ] Log mismatches clearly
- [ ] Do not benchmark a query path until correctness is confirmed

#### Deliverable
- Parallel results are verified against serial output

---

### 12. Extend Benchmark Harness for Parallel Experiments
#### Tasks
- [ ] Update `BenchmarkHarness` to support:
  - [ ] execution mode: serial or parallel
  - [ ] thread count
  - [ ] repeated runs
- [ ] Record:
  - [ ] ingest time
  - [ ] query time
  - [ ] total runtime
  - [ ] thread count
  - [ ] query type
  - [ ] run number
- [ ] Save outputs to:
  - [ ] `results/raw/`
- [ ] Separate serial and parallel result files clearly
- [ ] Ensure output format stays graph-friendly

#### Deliverable
- Raw benchmark results exist for serial and parallel runs in comparable format

---

### 13. Benchmark Parallel Scaling
#### Minimum benchmark scenarios
- [ ] serial ingest + serial speed-threshold query
- [ ] serial ingest + parallel speed-threshold query
- [ ] serial ingest + serial time-window query
- [ ] serial ingest + parallel time-window query
- [ ] serial ingest + serial aggregation
- [ ] serial ingest + parallel aggregation

#### Thread-count experiments
- [ ] 1 thread
- [ ] 2 threads
- [ ] 4 threads
- [ ] 8 threads or appropriate upper bound

#### Benchmark requirements
- [ ] at least 10 runs per scenario
- [ ] same dataset
- [ ] same query parameters
- [ ] same machine conditions as much as possible

#### Deliverable
- Scaling data exists and is usable for Phase 2 analysis

---

### 14. Measure Speedup and Parallel Overhead
#### Tasks
- [ ] Compute speedup relative to serial baseline
- [ ] Identify workloads with:
  - [ ] strong speedup
  - [ ] weak speedup
  - [ ] no improvement
  - [ ] regressions
- [ ] Note causes such as:
  - [ ] thread startup overhead
  - [ ] memory bandwidth limits
  - [ ] reduction overhead
  - [ ] uneven work distribution
  - [ ] false sharing or contention
- [ ] Record findings in notes for the report

#### Deliverable
- Phase 2 includes both positive and negative findings, not just best cases

---

### 15. Add Logging and Research Notes
#### Tasks
- [ ] Update `report/notes.md` with:
  - [ ] which query paths were parallelized
  - [ ] why those paths were chosen
  - [ ] thread model used
  - [ ] known bottlenecks
  - [ ] workloads that scaled poorly
  - [ ] failed approaches
- [ ] Record whether parallelizing ingestion was attempted
- [ ] Record any numerical differences in floating-point reductions
- [ ] Record any memory-related observations

#### Deliverable
- Repo contains enough notes to support Phase 2 report writing

---

## File-Level TODO Suggestions

### `include/query/`
- [ ] `ParallelCongestionQuery.hpp`
- [ ] `ParallelTrafficAggregator.hpp`

### `include/benchmark/`
- [ ] update `BenchmarkHarness.hpp` for thread-aware benchmarking

### `src/query/`
- [ ] `ParallelCongestionQuery.cpp`
- [ ] `ParallelTrafficAggregator.cpp`

### `src/benchmark/`
- [ ] update `BenchmarkHarness.cpp`

### `apps/`
- [ ] `run_parallel.cpp`

### `scripts/`
- [ ] update `benchmark.sh` to run thread-count sweeps
- [ ] optional helper script for serial-vs-parallel comparison

---

## Suggested Phase 2 Milestones

### Milestone A — Parallel Build Path
- [ ] `run_parallel` builds
- [ ] OpenMP or thread setup works
- [ ] executable launches successfully

### Milestone B — First Parallel Query
- [ ] speed-threshold query parallelized
- [ ] correctness verified against serial

### Milestone C — Multiple Parallel Workloads
- [ ] time-window query parallelized
- [ ] aggregation parallelized
- [ ] results verified

### Milestone D — Scaling Benchmarks
- [ ] multiple thread counts tested
- [ ] 10+ runs per scenario completed
- [ ] raw outputs saved

### Milestone E — Research Notes
- [ ] speedups documented
- [ ] regressions documented
- [ ] failed attempts documented

---

## Definition of Done for Phase 2
Phase 2 is complete when all of the following are true:

- [ ] The project builds a parallel executable cleanly
- [ ] At least 2–3 query paths are parallelized
- [ ] Parallel results are validated against serial output
- [ ] Benchmarks compare serial and parallel execution directly
- [ ] Thread-count scaling data is collected
- [ ] Results are saved to disk
- [ ] Notes exist explaining both improvements and limitations
- [ ] The codebase is ready for Phase 3 memory-layout optimization

---

## Explicit Non-Goals for Phase 2
Do not do these yet:
- [ ] no full Object-of-Arrays rewrite
- [ ] no large-scale data model redesign
- [ ] no mixing too many optimizations into benchmark comparisons
- [ ] no claiming Phase 3 performance wins yet
- [ ] no hiding regressions or failed attempts

---