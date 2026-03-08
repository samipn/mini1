# UrbanDrop Mini 1A — Phase 3 TODO
## System 1: Congestion Intelligence Engine
## Phase 3 Goal: Memory / Performance Optimization

This document defines the Phase 3 implementation plan for System 1. The goal is to optimize performance by rewriting key traffic data structures toward a **vectorized / Object-of-Arrays style layout**, improving memory locality, reducing avoidable overhead, and benchmarking the impact of each optimization against both the Phase 1 serial baseline and the Phase 2 parallel implementation.

Phase 3 is the performance-focused stage. Correctness still matters, but the main question now is:

**How much performance can be gained by changing memory layout and optimizing hot paths in a large traffic-query system?**

## Phase 3 Constraints
- Keep Phase 1 and Phase 2 implementations available for comparison
- Do not erase the baseline implementations
- Make changes incrementally and benchmark each one
- Preserve reproducibility of experiments
- Prioritize measurable performance improvements over elegance
- Avoid mixing unrelated changes into one benchmark step
- Document regressions as well as wins

---

## Phase 3 Success Criteria
By the end of Phase 3, the repository should contain:

1. A buildable optimized version of System 1
2. A vectorized / Object-of-Arrays traffic data layout
3. Updated query paths operating on the optimized layout
4. Benchmark runs comparing:
   - Phase 1 serial baseline
   - Phase 2 parallel baseline
   - Phase 3 optimized implementation
5. Measurements showing the effect of each major optimization
6. Notes explaining tradeoffs, regressions, and design decisions
7. Enough evidence to support the final report and one-slide key finding

---

## Phase 3 Strategy
Optimize in layers, not all at once.

Recommended order:

1. identify the hottest Phase 1/2 query paths
2. redesign core traffic storage into an Object-of-Arrays layout
3. port the most important queries to the optimized layout
4. reduce string-heavy and branch-heavy work in hot loops
5. benchmark after each major change
6. only then consider secondary optimizations

The goal is to isolate which changes actually matter.

---

## Implementation Order

### 1. Freeze the Comparison Baselines
#### Tasks
- [ ] Keep `run_serial` and `run_parallel` buildable
- [ ] Save known-good benchmark outputs from Phase 1 and Phase 2
- [ ] Define the exact dataset snapshot(s) and query scenarios for final comparison
- [ ] Ensure the same hardware/compiler settings are used where possible
- [ ] Create a new executable:
  - [ ] `run_optimized`

#### Deliverable
- Stable baselines exist for side-by-side comparison with the optimized version

---

### 2. Profile or Identify Hot Paths
#### Tasks
- [ ] Identify the slowest or most frequently used query paths from earlier phases
- [ ] Prioritize optimization targets such as:
  - [ ] full-dataset speed-threshold scans
  - [ ] time-window scans
  - [ ] borough + threshold filters
  - [ ] aggregations over large record ranges
- [ ] Record why these paths were chosen
- [ ] If available, use profiling tools or timing breakdowns to confirm hotspots

#### Deliverable
- A short written list of the hot paths chosen for optimization

---

### 3. Design the Object-of-Arrays Traffic Layout
#### Tasks
- [ ] Create an optimized data container, for example:
  - [ ] `TrafficColumns`
  - [ ] `TrafficDatasetOptimized`
- [ ] Convert row-oriented storage such as:
  - [ ] `std::vector<TrafficRecord>`
- [ ] into columnar/vectorized storage such as:
  - [ ] `std::vector<int> link_ids`
  - [ ] `std::vector<float> speeds`
  - [ ] `std::vector<float> travel_times`
  - [ ] `std::vector<int> borough_codes`
  - [ ] `std::vector<int64_t> timestamps`
- [ ] Replace repeated string fields with compact encoded forms where practical
- [ ] Define a mapping strategy for categorical fields:
  - [ ] borough string to integer code
  - [ ] link descriptors optionally stored out-of-band

#### Deliverable
- An optimized traffic dataset structure exists and compiles cleanly

---

### 4. Build Conversion or Direct-Load Path
#### Tasks
- [ ] Choose one of the following approaches:
  - [ ] load into row objects first, then convert
  - [ ] parse directly into columnar arrays
- [ ] Prefer the simplest approach first for correctness
- [ ] Track ingest time for both approaches if both are attempted
- [ ] Ensure all column vectors remain aligned by index
- [ ] Validate row counts after conversion or direct load

#### Deliverable
- Real traffic data can be loaded into the optimized layout correctly

---

### 5. Implement Optimized Query APIs
#### Tasks
- [ ] Add optimized query classes or functions, such as:
  - [ ] `OptimizedCongestionQuery`
  - [ ] `OptimizedTrafficAggregator`
- [ ] Port the hottest queries first:
  - [ ] speed-threshold scan
  - [ ] time-window scan
  - [ ] borough + threshold filter
  - [ ] average speed / travel time aggregation
- [ ] Keep outputs comparable to earlier implementations
- [ ] Minimize heap allocations in hot paths
- [ ] Avoid reconstructing row objects unless required for output

#### Deliverable
- Optimized query paths exist and return correct results

---

### 6. Reduce Hot-Loop Overhead
#### Tasks
- [ ] Remove unnecessary string comparisons inside tight loops
- [ ] Use encoded/category values instead of repeated text checks
- [ ] Precompute threshold or range parameters where appropriate
- [ ] Minimize branching in scan-heavy code
- [ ] Avoid repeated virtual dispatch in critical paths if it shows up as overhead
- [ ] Reuse buffers where practical

#### Deliverable
- Hot loops are cleaner, simpler, and more cache-friendly

---

### 7. Evaluate Memory Usage Changes
#### Tasks
- [ ] Measure memory usage of:
  - [ ] row-oriented Phase 1/2 layout
  - [ ] optimized Phase 3 layout
- [ ] Record any savings from:
  - [ ] compact categorical encoding
  - [ ] removing duplicated strings
  - [ ] reduced per-record object overhead
- [ ] Note cases where memory usage increases for speed

#### Deliverable
- Memory tradeoffs are documented, not just runtime changes

---

### 8. Add Optional Query-Support Optimizations
#### Possible tasks
- [ ] Add precomputed index ranges if useful
- [ ] Add sorted or grouped storage by frequently filtered key if justified
- [ ] Add light auxiliary lookup structures for:
  - [ ] link ID grouping
  - [ ] borough grouping
  - [ ] coarse time partitioning
- [ ] Benchmark each addition independently
- [ ] Reject any structure that adds complexity without measurable benefit

#### Deliverable
- Any extra indexing is justified by benchmark evidence

---

### 9. Integrate Parallelism with the Optimized Layout
#### Tasks
- [ ] Reuse the best Phase 2 parallel strategy on top of the optimized layout
- [ ] Measure:
  - [ ] optimized serial query performance
  - [ ] optimized parallel query performance
- [ ] Compare whether the new layout improves:
  - [ ] single-thread performance
  - [ ] multi-thread scaling
- [ ] Watch for memory-bandwidth limitations

#### Deliverable
- Optimized layout is benchmarked in both serial and parallel execution modes where relevant

---

### 10. Implement the Optimized CLI Runner
#### Tasks
- [ ] Implement `apps/run_optimized.cpp`
- [ ] Accept command-line arguments for:
  - [ ] dataset path
  - [ ] query type
  - [ ] thresholds
  - [ ] time ranges
  - [ ] execution mode if needed
  - [ ] thread count if parallel optimized mode is supported
- [ ] Print useful execution logs:
  - [ ] optimized layout load started/completed
  - [ ] query started/completed
  - [ ] elapsed time
  - [ ] result size or aggregate summary
- [ ] Return nonzero exit code on failure

#### Deliverable
- Optimized executable runs benchmarkable workloads cleanly

---

### 11. Extend Benchmark Harness for Final Comparisons
#### Tasks
- [ ] Update `BenchmarkHarness` to support:
  - [ ] implementation mode: serial / parallel / optimized
  - [ ] thread count where applicable
  - [ ] repeated runs
- [ ] Record:
  - [ ] ingest time
  - [ ] query time
  - [ ] total runtime
  - [ ] memory usage if available
  - [ ] implementation mode
  - [ ] query type
  - [ ] run number
- [ ] Save outputs to:
  - [ ] `results/raw/`
- [ ] Keep formats easy to graph and compare later

#### Deliverable
- Final benchmark outputs can compare all three phases directly

---

### 12. Benchmark Each Major Optimization Separately
#### Minimum comparison points
- [ ] Phase 1 row-oriented serial baseline
- [ ] Phase 2 row-oriented parallel baseline
- [ ] Phase 3 optimized serial
- [ ] Phase 3 optimized parallel if implemented

#### Suggested measurement steps
- [ ] baseline row-oriented query
- [ ] columnar layout only
- [ ] columnar layout + encoded categorical fields
- [ ] columnar layout + hot-loop cleanup
- [ ] columnar layout + parallel query

#### Benchmark requirements
- [ ] at least 10 runs per scenario
- [ ] same dataset
- [ ] same query parameters
- [ ] same machine conditions as much as possible

#### Deliverable
- You can attribute performance changes to specific design choices

---

### 13. Validate Correctness Against Earlier Phases
#### Tasks
- [ ] Compare optimized outputs against serial baseline outputs
- [ ] Validate:
  - [ ] record counts
  - [ ] aggregate metrics
  - [ ] sample outputs where useful
- [ ] Add a validation mode if helpful
- [ ] Log mismatches clearly
- [ ] Resolve or explain acceptable differences such as tiny floating-point deviations

#### Deliverable
- Optimized implementation is correct enough to defend in the report

---

### 14. Summarize Performance Tradeoffs
#### Tasks
- [ ] Identify which optimizations produced the biggest gains
- [ ] Identify which optimizations produced little or no benefit
- [ ] Identify any regressions
- [ ] Note tradeoffs such as:
  - [ ] better scan speed but more complex code
  - [ ] lower memory footprint but costlier ingest
  - [ ] better single-thread speed but limited scaling
- [ ] Record the likely causes behind final performance behavior

#### Deliverable
- Phase 3 includes a clear story, not just raw numbers

---

### 15. Add Final Research Notes
#### Tasks
- [ ] Update `report/notes.md` with:
  - [ ] chosen optimized layout
  - [ ] field encoding choices
  - [ ] conversion vs direct-load decision
  - [ ] which hot paths improved most
  - [ ] which ideas failed
  - [ ] final recommended design
- [ ] Record benchmark caveats
- [ ] Record open questions that were not fully resolved
- [ ] Capture possible content for the final one-slide presentation

#### Deliverable
- Repo contains enough material to support final report writing and presentation selection

---

## File-Level TODO Suggestions

### `include/data_model/`
- [ ] `TrafficColumns.hpp`
- [ ] `TrafficDatasetOptimized.hpp`

### `include/query/`
- [ ] `OptimizedCongestionQuery.hpp`
- [ ] `OptimizedTrafficAggregator.hpp`

### `include/benchmark/`
- [ ] update `BenchmarkHarness.hpp` for final comparison modes

### `src/data_model/`
- [ ] `TrafficColumns.cpp`
- [ ] `TrafficDatasetOptimized.cpp`

### `src/query/`
- [ ] `OptimizedCongestionQuery.cpp`
- [ ] `OptimizedTrafficAggregator.cpp`

### `src/benchmark/`
- [ ] update `BenchmarkHarness.cpp`

### `apps/`
- [ ] `run_optimized.cpp`

### `scripts/`
- [ ] update `benchmark.sh` for full phase comparison runs
- [ ] update `plot_results.py` to graph serial vs parallel vs optimized
- [ ] optional helper script for final experiment bundles

---

## Suggested Phase 3 Milestones

### Milestone A — Optimized Layout Exists
- [ ] columnar traffic storage compiles
- [ ] data loads correctly
- [ ] counts match baseline

### Milestone B — First Optimized Query
- [ ] speed-threshold query runs on optimized layout
- [ ] correctness verified against baseline

### Milestone C — Core Query Port Complete
- [ ] time-window query ported
- [ ] borough + threshold query ported
- [ ] aggregation ported

### Milestone D — Final Benchmarking
- [ ] optimized serial runs complete
- [ ] optimized parallel runs complete if supported
- [ ] all raw outputs saved

### Milestone E — Final Analysis Notes
- [ ] strongest optimization documented
- [ ] regressions documented
- [ ] slide-worthy finding identified

---

## Definition of Done for Phase 3
Phase 3 is complete when all of the following are true:

- [ ] The project builds an optimized executable cleanly
- [ ] Key traffic data is stored in a vectorized/Object-of-Arrays layout
- [ ] At least the main query workloads run on the optimized layout
- [ ] Benchmarks compare Phase 1, Phase 2, and Phase 3 directly
- [ ] Results are validated against the earlier baseline
- [ ] Raw outputs and notes are saved to disk
- [ ] The team can clearly explain which optimization mattered most and why

---

## Explicit Non-Goals for Phase 3
Do not do these unless they are clearly justified and benchmarked:
- [ ] no unmeasured “cleanup” changes mixed into performance claims
- [ ] no replacing every part of the codebase if only a few hot paths matter
- [ ] no hiding regressions
- [ ] no report claims without benchmark evidence

---