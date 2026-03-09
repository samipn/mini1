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
- [x] Keep `run_serial` and `run_parallel` buildable
- [x] Save known-good benchmark outputs from Phase 1 and Phase 2
- [x] Define the exact dataset snapshot(s) and query scenarios for final comparison
- [x] Ensure the same hardware/compiler settings are used where possible
- [ ] Create a new executable:
  - [x] `run_optimized`

#### Deliverable
- Stable baselines exist for side-by-side comparison with the optimized version

---

### 2. Profile or Identify Hot Paths
#### Tasks
- [x] Identify the slowest or most frequently used query paths from earlier phases
- [x] Prioritize optimization targets such as:
  - [x] full-dataset speed-threshold scans
  - [x] time-window scans
  - [x] borough + threshold filters
  - [x] aggregations over large record ranges
- [x] Record why these paths were chosen
- [x] If available, use profiling tools or timing breakdowns to confirm hotspots

#### Deliverable
- A short written list of the hot paths chosen for optimization

---

### 3. Design the Object-of-Arrays Traffic Layout
#### Tasks
- [x] Create an optimized data container, for example:
  - [x] `TrafficColumns`
  - [x] `TrafficDatasetOptimized`
- [x] Convert row-oriented storage such as:
  - [x] `std::vector<TrafficRecord>`
- [x] into columnar/vectorized storage such as:
  - [x] `std::vector<int> link_ids`
  - [x] `std::vector<float> speeds`
  - [x] `std::vector<float> travel_times`
  - [x] `std::vector<int> borough_codes`
  - [x] `std::vector<int64_t> timestamps`
- [x] Replace repeated string fields with compact encoded forms where practical
- [x] Define a mapping strategy for categorical fields:
  - [x] borough string to integer code
  - [x] link descriptors optionally stored out-of-band

#### Deliverable
- An optimized traffic dataset structure exists and compiles cleanly

---

### 4. Build Conversion or Direct-Load Path
#### Tasks
- [x] Choose one of the following approaches:
  - [x] load into row objects first, then convert
  - [x] parse directly into columnar arrays
- [x] Prefer the simplest approach first for correctness
- [x] Track ingest time for both approaches if both are attempted
- [x] Ensure all column vectors remain aligned by index
- [x] Validate row counts after conversion or direct load

#### Deliverable
- Real traffic data can be loaded into the optimized layout correctly

---

### 5. Implement Optimized Query APIs
#### Tasks
- [x] Add optimized query classes or functions, such as:
  - [x] `OptimizedCongestionQuery`
  - [x] `OptimizedTrafficAggregator`
- [x] Port the hottest queries first:
  - [x] speed-threshold scan
  - [x] time-window scan
  - [x] borough + threshold filter
  - [x] average speed / travel time aggregation
- [x] Keep outputs comparable to earlier implementations
- [x] Minimize heap allocations in hot paths
- [x] Avoid reconstructing row objects unless required for output

#### Deliverable
- Optimized query paths exist and return correct results

---

### 6. Reduce Hot-Loop Overhead
#### Tasks
- [x] Remove unnecessary string comparisons inside tight loops
- [x] Use encoded/category values instead of repeated text checks
- [x] Precompute threshold or range parameters where appropriate
- [x] Minimize branching in scan-heavy code
- [x] Avoid repeated virtual dispatch in critical paths if it shows up as overhead
- [x] Reuse buffers where practical

#### Deliverable
- Hot loops are cleaner, simpler, and more cache-friendly

---

### 7. Evaluate Memory Usage Changes
#### Tasks
- [x] Measure memory usage of:
  - [x] row-oriented Phase 1/2 layout
  - [x] optimized Phase 3 layout
- [x] Record any savings from:
  - [x] compact categorical encoding
  - [x] removing duplicated strings
  - [x] reduced per-record object overhead
- [x] Note cases where memory usage increases for speed

#### Deliverable
- Memory tradeoffs are documented, not just runtime changes

---

### 8. Add Optional Query-Support Optimizations
#### Possible tasks
- [x] Add precomputed index ranges if useful
- [x] Add sorted or grouped storage by frequently filtered key if justified
- [x] Add light auxiliary lookup structures for:
  - [x] link ID grouping
  - [x] borough grouping
  - [x] coarse time partitioning
- [x] Benchmark each addition independently
- [x] Reject any structure that adds complexity without measurable benefit

#### Deliverable
- Any extra indexing is justified by benchmark evidence

---

### 9. Integrate Parallelism with the Optimized Layout
#### Tasks
- [x] Reuse the best Phase 2 parallel strategy on top of the optimized layout
- [x] Measure:
  - [x] optimized serial query performance
  - [x] optimized parallel query performance
- [x] Compare whether the new layout improves:
  - [x] single-thread performance
  - [x] multi-thread scaling
- [x] Watch for memory-bandwidth limitations

#### Deliverable
- Optimized layout is benchmarked in both serial and parallel execution modes where relevant

---

### 10. Implement the Optimized CLI Runner
#### Tasks
- [x] Implement `apps/run_optimized.cpp`
- [x] Accept command-line arguments for:
  - [x] dataset path
  - [x] query type
  - [x] thresholds
  - [x] time ranges
  - [x] execution mode if needed
  - [x] thread count if parallel optimized mode is supported
- [x] Print useful execution logs:
  - [x] optimized layout load started/completed
  - [x] query started/completed
  - [x] elapsed time
  - [x] result size or aggregate summary
- [x] Return nonzero exit code on failure

#### Deliverable
- Optimized executable runs benchmarkable workloads cleanly

---

### 11. Extend Benchmark Harness for Final Comparisons
#### Tasks
- [x] Update `BenchmarkHarness` to support:
  - [x] implementation mode: serial / parallel / optimized
  - [x] thread count where applicable
  - [x] repeated runs
- [x] Record:
  - [x] ingest time
  - [x] query time
  - [x] total runtime
  - [x] memory usage if available
  - [x] implementation mode
  - [x] query type
  - [x] run number
- [x] Save outputs to:
  - [x] `results/raw/`
- [x] Keep formats easy to graph and compare later

#### Deliverable
- Final benchmark outputs can compare all three phases directly

---

### 12. Benchmark Each Major Optimization Separately
#### Minimum comparison points
- [x] Phase 1 row-oriented serial baseline
- [x] Phase 2 row-oriented parallel baseline
- [x] Phase 3 optimized serial
- [x] Phase 3 optimized parallel if implemented

#### Suggested measurement steps
- [x] baseline row-oriented query
- [x] columnar layout only
- [x] columnar layout + encoded categorical fields
- [x] columnar layout + hot-loop cleanup
- [x] columnar layout + parallel query

#### Benchmark requirements
- [x] at least 10 runs per scenario
- [x] same dataset
- [x] same query parameters
- [x] same machine conditions as much as possible

#### Deliverable
- You can attribute performance changes to specific design choices

---

### 13. Validate Correctness Against Earlier Phases
#### Tasks
- [x] Compare optimized outputs against serial baseline outputs
- [x] Validate:
  - [x] record counts
  - [x] aggregate metrics
  - [x] sample outputs where useful
- [x] Add a validation mode if helpful
- [x] Log mismatches clearly
- [x] Resolve or explain acceptable differences such as tiny floating-point deviations

#### Deliverable
- Optimized implementation is correct enough to defend in the report

---

### 14. Summarize Performance Tradeoffs
#### Tasks
- [x] Identify which optimizations produced the biggest gains
- [x] Identify which optimizations produced little or no benefit
- [x] Identify any regressions
- [x] Note tradeoffs such as:
  - [x] better scan speed but more complex code
  - [x] lower memory footprint but costlier ingest
  - [x] better single-thread speed but limited scaling
- [x] Record the likely causes behind final performance behavior

#### Deliverable
- Phase 3 includes a clear story, not just raw numbers

---

### 15. Add Final Research Notes
#### Tasks
- [x] Update `report/notes.md` with:
  - [x] chosen optimized layout
  - [x] field encoding choices
  - [x] conversion vs direct-load decision
  - [x] which hot paths improved most
  - [x] which ideas failed
  - [x] final recommended design
- [x] Record benchmark caveats
- [x] Record open questions that were not fully resolved
- [x] Capture possible content for the final one-slide presentation

#### Deliverable
- Repo contains enough material to support final report writing and presentation selection

---

## File-Level TODO Suggestions

### `include/data_model/`
- [x] `TrafficColumns.hpp`
- [x] `TrafficDatasetOptimized.hpp`

### `include/query/`
- [x] `OptimizedCongestionQuery.hpp`
- [x] `OptimizedTrafficAggregator.hpp`

### `include/benchmark/`
- [x] update `BenchmarkHarness.hpp` for final comparison modes

### `src/data_model/`
- [x] `TrafficColumns.cpp`
- [x] `TrafficDatasetOptimized.cpp`

### `src/query/`
- [x] `OptimizedCongestionQuery.cpp`
- [x] `OptimizedTrafficAggregator.cpp`

### `src/benchmark/`
- [x] update `BenchmarkHarness.cpp`

### `apps/`
- [x] `run_optimized.cpp`

### `scripts/`
- [x] update `benchmark.sh` for full phase comparison runs
- [x] update `plot_results.py` to graph serial vs parallel vs optimized
- [x] optional helper script for final experiment bundles

---

## Suggested Phase 3 Milestones

### Milestone A — Optimized Layout Exists
- [x] columnar traffic storage compiles
- [x] data loads correctly
- [x] counts match baseline

### Milestone B — First Optimized Query
- [x] speed-threshold query runs on optimized layout
- [x] correctness verified against baseline

### Milestone C — Core Query Port Complete
- [x] time-window query ported
- [x] borough + threshold query ported
- [x] aggregation ported

### Milestone D — Final Benchmarking
- [x] optimized serial runs complete
- [x] optimized parallel runs complete if supported
- [x] all raw outputs saved

### Milestone E — Final Analysis Notes
- [x] strongest optimization documented
- [x] regressions documented
- [x] slide-worthy finding identified

---

## Definition of Done for Phase 3
Phase 3 is complete when all of the following are true:

- [x] The project builds an optimized executable cleanly
- [x] Key traffic data is stored in a vectorized/Object-of-Arrays layout
- [x] At least the main query workloads run on the optimized layout
- [x] Benchmarks compare Phase 1, Phase 2, and Phase 3 directly
- [x] Results are validated against the earlier baseline
- [x] Raw outputs and notes are saved to disk
- [x] The team can clearly explain which optimization mattered most and why

---

## Explicit Non-Goals for Phase 3
Do not do these unless they are clearly justified and benchmarked:
- [ ] no unmeasured “cleanup” changes mixed into performance claims
- [ ] no replacing every part of the codebase if only a few hot paths matter
- [ ] no hiding regressions
- [ ] no report claims without benchmark evidence

---

## Deferred From Phase 1 (Additive)
These items were identified during late Phase 1 review and are appended for Phase 3 optimization work.

### High-cardinality encoding and consolidation
- [x] Implement dictionary/interning strategy for high-cardinality text-heavy traffic fields:
  - [x] `link_name`
  - [x] any remaining repeated descriptor strings in hot paths
- [x] Replace repeated string comparisons in hot loops with integer-coded category checks where possible.
- [x] Quantify memory and runtime impact of dictionary encoding vs raw string storage.

### Cross-dataset optimized relationships
- [x] Finalize cross-dataset indexed/materialized join structures after Phase 2 prototypes:
  - [x] garage `bbl` <-> BES `bbl` optimized lookup path
  - [x] borough-coded relationship views for repeated analytical scans
- [x] Benchmark and report tradeoffs of pre-materialized joins vs on-demand joins.

### Boost evaluation in optimized phase
- [x] Evaluate Boost-based optimization candidates against STL baselines:
  - [x] `boost::container::small_vector` for hot-path temporary buffers
  - [x] `boost::container::flat_map` or similar sorted containers for compressed lookup tables
  - [x] `boost::dynamic_bitset` for vectorized filtering masks
- [x] Keep Boost usage only where measured wins are reproducible and clearly justified in benchmark notes.

---

---

## Phase 3 Baseline Comparative Testing and Early Data Collection
## Purpose
These tasks are for **development-stage optimized-layout benchmarking on subset datasets**, not the final full-dataset benchmark campaign.

The goal is to:
- verify that optimized-layout results match earlier correct behavior,
- establish repeatable serial-vs-parallel-vs-optimized benchmark workflows,
- collect preliminary evidence about memory-layout tradeoffs,
- and ensure the final benchmark harness is stable before running the full dataset later.

These tasks should be completed by the coding agent while implementing Phase 3.

### Rules for This Section
- Use **subset datasets only**
- Use the **same subset files** already used in Phase 1 and Phase 2 development benchmarking
- Keep query definitions identical across serial, parallel, and optimized modes where possible
- Save all raw outputs for later comparison
- Do **not** treat these results as final project conclusions
- Keep optimized experiments attributable to specific changes whenever possible

---

### 18. Reuse or Validate Subset Datasets for Phase 3
#### Tasks
- [x] Reuse the reproducible subset datasets created earlier
- [x] Confirm subset row counts still match expected sizes
- [x] Confirm subset files are accessible to serial, parallel, and optimized runners
- [x] Record which subset files are used in Phase 3 development tests

#### Agent instructions
- Do not regenerate subsets unless necessary
- Save a short manifest of subset file names and row counts under:
  - [x] `results/raw/phase3_dev/`
- Keep naming stable across repeated benchmark batches

#### Deliverable
- Phase 3 development benchmarks use consistent subset inputs

---

### 19. Add Cross-Mode Validation Runs on Subsets
#### Tasks
- [x] Run the serial implementation on each selected subset
- [x] Run the parallel implementation on each selected subset
- [x] Run the optimized implementation on each selected subset
- [x] Compare for each scenario:
  - [x] result count
  - [x] aggregate values
  - [x] top-N outputs if applicable
  - [x] mismatch reports if found

#### Agent instructions
- Add a validation mode or helper script that runs serial, parallel, and optimized versions back-to-back on the same subset and scenario
- Save comparison outputs to:
  - [x] `results/raw/phase3_dev/validation/`
- If mismatches occur, save all raw outputs and log the issue clearly

#### Deliverable
- Optimized subset results are checked against earlier correct behavior

---

### 20. Add Early Optimized Benchmark Scenarios for Subsets
#### Tasks
- [x] Define a repeatable Phase 3 subset benchmark set using the same query parameters as earlier phases
- [x] Minimum scenarios:
  - [x] serial ingest + serial low-speed threshold query
  - [x] parallel query version of the same workload
  - [x] optimized serial query version of the same workload
  - [x] optimized parallel query version if supported
  - [x] serial ingest + serial time-window query
  - [x] optimized serial time-window query
  - [x] serial ingest + serial aggregation query
  - [x] optimized serial aggregation query
- [x] If borough filtering exists, include optimized comparison variants there too

#### Agent instructions
- Keep scenario names stable and machine-readable
- Use the same thresholds and time windows unless a new experiment is explicitly documented
- Tag every result row with:
  - [x] implementation mode
  - [x] subset size
  - [x] query scenario
  - [x] thread count if applicable

#### Deliverable
- A stable early benchmark set exists for subset-based optimized testing

---

### 21. Create a Repeatable Phase 3 Development Benchmark Runner
#### Tasks
- [x] Add or extend a script that runs the Phase 3 subset benchmark suite automatically
- [x] Support:
  - [x] small subset
  - [x] medium subset
  - [x] large-dev subset
- [x] Support repeated runs per scenario
- [x] Support implementation modes:
  - [x] serial
  - [x] parallel
  - [x] optimized
  - [x] optimized parallel if supported
- [x] Save raw outputs to:
  - [x] `results/raw/phase3_dev/`
- [x] Save logs to:
  - [x] `results/raw/logs/`

#### Agent instructions
- Implement or extend a script such as:
  - [x] `scripts/run_phase3_dev_benchmarks.sh`
- The script should:
  - [x] accept a dataset path
  - [x] accept a repetition count
  - [x] accept thread counts if needed
  - [x] run all selected comparison scenarios
  - [x] save each run result in a machine-readable format
  - [x] print a concise summary to console

#### Deliverable
- A single command can run the subset-based Phase 3 development benchmark suite

---

### 22. Collect Preliminary Optimized Timing and Comparison Data
#### Tasks
- [x] Run each selected benchmark scenario at least:
  - [x] `3` times on the small subset
  - [x] `3` times on the medium subset
  - [x] `3` times on the large-dev subset
- [x] For optimized parallel runs, test at least:
  - [x] `1` thread
  - [x] `2` threads
  - [x] `4` threads
  - [x] `8` threads if reasonable
- [x] Record:
  - [x] ingest time
  - [x] query time
  - [x] total runtime
  - [x] implementation mode
  - [x] thread count
  - [x] records processed per second if available
  - [x] memory usage if available in your harness

#### Agent instructions
- The coding agent should execute the benchmark runner after changes affecting:
  - [x] data layout
  - [x] encoded field handling
  - [x] hot-loop logic
  - [x] optimized query logic
- Save each batch with labels including:
  - [x] git branch
  - [x] commit hash if possible
  - [x] subset size
  - [x] scenario
  - [x] implementation mode
  - [x] thread count if applicable
- Prefer CSV output for later summarization

#### Deliverable
- Preliminary timing and comparison data exists for subset-based Phase 3 scenarios

---

### 23. Collect Preliminary Memory-Tradeoff Data
#### Tasks
- [x] Measure memory usage on subset workloads for:
  - [x] row-oriented serial implementation
  - [x] row-oriented parallel implementation if useful
  - [x] optimized implementation
- [x] Record memory-related metrics such as:
  - [x] peak RSS
  - [x] approximate bytes per record if estimated
  - [x] effect of encoded categorical fields if measured
- [x] Save raw memory-related outputs to:
  - [x] `results/raw/phase3_dev/memory/`

#### Agent instructions
- Extend the benchmark harness or helper scripts to collect memory data if practical
- If exact memory instrumentation is difficult, record at least one consistent proxy or observation method and document it clearly
- Keep measurement method consistent across comparison runs

#### Deliverable
- Preliminary memory-tradeoff evidence exists for Phase 3 development

---

### 24. Generate Development Summary Tables for Phase 3
#### Tasks
- [x] Add or extend a script to summarize raw Phase 3 development benchmark results
- [x] Compute:
  - [x] mean runtime
  - [x] median runtime
  - [x] minimum runtime
  - [x] maximum runtime
  - [x] standard deviation if enough runs exist
  - [x] speedup relative to serial baseline for the same subset/scenario
  - [x] speedup relative to Phase 2 parallel where applicable
- [x] Save summary tables to:
  - [x] `results/tables/phase3_dev/`

#### Agent instructions
- Implement or extend a helper such as:
  - [x] `scripts/summarize_phase3_dev.py`
- Read raw CSV benchmark outputs
- Produce grouped tables by:
  - [x] subset size
  - [x] scenario
  - [x] implementation mode
  - [x] thread count
- Do not delete raw files after summarization

#### Deliverable
- Development-stage optimized summary tables exist and are easy to inspect

---

### 25. Generate Simple Development Graphs for Phase 3
#### Tasks
- [x] Add or extend plotting scripts for optimized benchmark results
- [x] Plot at least:
  - [x] serial vs parallel vs optimized runtime by subset size
  - [x] speedup by implementation mode
  - [x] optimized runtime vs thread count if optimized parallel is supported
  - [x] memory usage comparison by implementation mode if available
- [x] Save graphs to:
  - [x] `results/graphs/phase3_dev/`

#### Agent instructions
- Use Python for graph generation
- Keep graphs simple and readable
- Label axes clearly
- Include subset size, implementation mode, scenario, and thread count where relevant
- Save plots to files rather than only displaying interactively

#### Deliverable
- Early Phase 3 graphs exist to show optimization trends before full-dataset testing

---

### 26. Add a Phase 3 Development Benchmark Log
#### Tasks
- [x] Record each benchmark batch in `report/notes.md` or a dedicated Phase 3 dev benchmark log
- [x] For each batch, record:
  - [x] date/time
  - [x] git branch
  - [x] commit hash if available
  - [x] subset size
  - [x] benchmark scenarios run
  - [x] implementation modes tested
  - [x] thread counts tested if applicable
  - [x] notable observations
  - [x] failures or anomalies
- [x] Record whether the benchmark batch followed a layout or optimization change

#### Agent instructions
- The coding agent should append a short notes entry after completing a benchmark batch
- Include enough detail to trace performance changes back to specific optimization steps

#### Deliverable
- Development benchmark history is documented and traceable for Phase 3

---

### 27. Add Stability Checks for Optimized Runs
#### Tasks
- [x] Rerun identical optimized scenarios multiple times on the same subset
- [x] Confirm consistency of:
  - [x] result count
  - [x] aggregate values
  - [x] timing distribution within reason
- [x] Check for obvious optimized-path corruption or mismatch
- [x] Record any acceptable numerical variation

#### Agent instructions
- Add a helper mode or script that reruns the same optimized scenario multiple times and compares outputs
- Save mismatches and timing anomalies to:
  - [x] `results/raw/phase3_dev/stability/`

#### Deliverable
- Optimized subset benchmarks are shown to be stable enough for development use

---

### 28. Attribute Performance Changes to Specific Optimization Steps
#### Tasks
- [x] Label benchmark batches by optimization step where practical, such as:
  - [x] columnar layout only
  - [x] encoded categorical fields
  - [x] reduced string comparisons
  - [x] optimized query rewrite
  - [x] optimized parallel query
- [x] Keep benchmark outputs attributable to one major change whenever possible
- [x] Record when multiple changes were introduced together

#### Agent instructions
- The coding agent should include an `optimization_step` field in benchmark outputs or summary tables where possible
- Use stable labels so later report writing is easier

#### Deliverable
- Phase 3 development evidence can be tied to specific design decisions

---

### 29. Mark These Results as Pre-Baseline Only
#### Tasks
- [x] Add a note in the README or report notes that Phase 3 subset benchmarks are:
  - [x] development-stage measurements
  - [x] not the final official comparison data
- [x] Distinguish clearly between:
  - [x] `phase3_dev`
  - [x] final `phase3_baseline`
- [x] Reserve final full-dataset comparison runs for the official benchmark campaign later

#### Agent instructions
- Ensure output directories and filenames make this distinction obvious
- Do not mix subset-development outputs with final benchmark outputs

#### Deliverable
- Early Phase 3 data collection is organized without being confused with final results

---

## Extra Definition of Done for Development-Stage Optimized Benchmarking
This subsection is complete when all of the following are true:

- [x] Reproducible subset datasets are reused successfully
- [x] Serial, parallel, and optimized programs all run successfully on selected subset sizes
- [x] At least 3 benchmark scenarios run repeatedly across implementation modes
- [x] Multiple thread counts are tested where applicable
- [x] Raw results are saved to disk
- [x] Summary tables are generated
- [x] Simple graphs are generated
- [x] Development benchmark notes are recorded
- [x] Outputs are clearly labeled as pre-baseline and not final full-dataset benchmarks
- [x] At least some benchmark results are attributable to specific optimization steps
