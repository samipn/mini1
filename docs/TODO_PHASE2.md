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
- [x] Keep `run_serial` unchanged except for bug fixes
- [x] Tag or branch the repository at the Phase 1 completion point
- [x] Confirm Phase 1 benchmark scenarios still run
- [x] Save a stable copy of Phase 1 benchmark outputs for comparison
- [x] Define identical datasets and query scenarios for Phase 2

#### Deliverable
- A known-good serial baseline exists for direct comparison

---

### 2. Add Parallel Build Support
#### Tasks
- [x] Update `CMakeLists.txt` to support parallel builds
- [x] If using OpenMP:
  - [x] detect OpenMP in CMake
  - [x] link required OpenMP flags/libraries
- [ ] If using `std::thread`:
  - [ ] ensure thread support is available and linked as needed
- [x] Build a new executable:
  - [x] `run_parallel`
- [x] Keep compile warnings enabled
- [x] Ensure Release mode is easy to build and run

#### Deliverable
- `run_parallel` builds successfully with the chosen parallel model

---

### 3. Define Parallelization Targets
#### Tasks
- [x] Identify the most expensive read-only workloads from Phase 1
- [x] Choose at least 2–3 query paths to parallelize
- [x] Prioritize:
  - [x] speed-threshold scans
  - [ ] time-window scans
  - [x] borough + threshold scans
  - [x] aggregations over the full record set
- [x] Document why these paths were chosen

#### Good candidates
- [x] scanning all traffic records for a predicate match
- [x] computing average speed over subsets
- [x] counting low-speed records by condition
- [x] grouping partial results by thread and reducing at the end

#### Lower-priority candidates
- [ ] ingestion parsing
- [ ] string-heavy preprocessing
- [ ] anything requiring a lot of lock contention

#### Deliverable
- A documented list of parallelized workloads

---

### 4. Implement Parallel Query Infrastructure
#### Tasks
- [x] Add parallel-capable query implementations
- [x] Keep serial implementations available for comparison
- [x] Create separate classes or methods as needed:
  - [x] `ParallelCongestionQuery`
  - [x] `ParallelTrafficAggregator`
- [x] Avoid shared mutable output structures without a reduction strategy
- [x] Use thread-local partial results where appropriate
- [x] Merge partial results after parallel work completes

#### Deliverable
- Parallel query code exists without breaking the serial code path

---

### 5. Parallelize Speed-Threshold Query
#### Tasks
- [x] Implement parallel scan over all loaded traffic records
- [x] Evaluate predicate such as:
  - [x] speed below threshold
- [x] Store results safely:
  - [ ] thread-local vectors then merge, or
  - [x] counts only if full record output is not needed
- [x] Compare returned result count to serial version
- [x] Add benchmark scenario using the same threshold as Phase 1

#### Deliverable
- Parallel low-speed scan returns the same result as the serial baseline

---

### 6. Parallelize Time-Window Query
#### Tasks
- [ ] Implement parallel scan for records in a selected time window
- [x] Implement parallel scan for records in a selected time window
- [x] Ensure timestamp comparisons are deterministic
- [ ] Compare:
  - [x] record count
  - [ ] aggregate metrics
  - [ ] optional sample records
- [x] Benchmark against the same time windows used in Phase 1

#### Deliverable
- Parallel time-range query matches serial behavior

---

### 7. Parallelize Aggregation Queries
#### Tasks
- [ ] Add parallel aggregation for:
  - [x] average speed
  - [x] average travel time
  - [x] counts by condition
- [x] Use thread-local accumulators
- [x] Reduce safely after the parallel region
- [x] Verify numerical behavior is acceptable
- [x] Note if floating-point reduction order changes tiny decimal results

#### Deliverable
- Parallel aggregations produce acceptable and explainable results

---

### 8. Evaluate Top-N or Ranking Queries Carefully
#### Tasks
- [x] Decide whether top-N slowest query should be parallelized in Phase 2
- [x] If yes:
  - [x] compute thread-local candidates
  - [x] merge and rank after parallel scan
- [x] Compare ranking output to serial result
- [ ] If too complex or too slow, document why and defer deeper optimization to Phase 3

#### Deliverable
- Either a working parallel top-N flow or a documented reason for deferral

---

### 9. Extend the CLI Runner for Parallel Execution
#### Tasks
- [x] Implement `apps/run_parallel.cpp`
- [ ] Accept command-line arguments for:
  - [x] traffic dataset path
  - [x] query type
  - [x] thresholds
  - [x] time ranges
  - [x] number of threads
  - [x] repetitions
- [ ] Print logs showing:
  - [x] thread count
  - [x] query type
  - [x] dataset size
  - [x] query start/end
  - [x] elapsed time
- [x] Return nonzero exit code on failure

#### Deliverable
- Parallel executable runs benchmarkable workloads with configurable thread counts

---

### 10. Add Thread Count Controls
#### Tasks
- [ ] Support explicit thread counts such as:
  - [x] 1
  - [x] 2
  - [x] 4
  - [x] 8
  - [x] maximum available reasonable count
- [x] Ensure `1 thread` mode can be compared against serial behavior
- [x] Record thread count in benchmark outputs
- [x] Avoid auto-tuning that changes behavior between runs

#### Deliverable
- Benchmark runs can compare scaling across thread counts

---

### 11. Add Correctness Validation for Parallel Results
#### Tasks
- [x] Compare serial vs parallel outputs for all supported query types
- [x] Validate:
  - [x] result counts
  - [x] aggregate values
  - [x] sampled outputs where needed
- [x] Add a validation mode that runs both versions back-to-back
- [x] Log mismatches clearly
- [x] Do not benchmark a query path until correctness is confirmed

#### Deliverable
- Parallel results are verified against serial output

---

### 12. Extend Benchmark Harness for Parallel Experiments
#### Tasks
- [x] Update `BenchmarkHarness` to support:
  - [x] execution mode: serial or parallel
  - [x] thread count
  - [x] repeated runs
- [x] Record:
  - [x] ingest time
  - [x] query time
  - [x] total runtime
  - [x] thread count
  - [x] query type
  - [x] run number
- [x] Save outputs to:
  - [x] `results/raw/`
- [x] Separate serial and parallel result files clearly
- [x] Ensure output format stays graph-friendly

#### Deliverable
- Raw benchmark results exist for serial and parallel runs in comparable format

---

### 13. Benchmark Parallel Scaling
#### Minimum benchmark scenarios
- [x] serial ingest + serial speed-threshold query
- [x] serial ingest + parallel speed-threshold query
- [x] serial ingest + serial time-window query
- [x] serial ingest + parallel time-window query
- [x] serial ingest + serial aggregation
- [x] serial ingest + parallel aggregation

#### Thread-count experiments
- [x] 1 thread
- [x] 2 threads
- [x] 4 threads
- [x] 8 threads or appropriate upper bound

#### Benchmark requirements
- [x] at least 10 runs per scenario
- [x] same dataset
- [x] same query parameters
- [x] same machine conditions as much as possible

#### Deliverable
- Scaling data exists and is usable for Phase 2 analysis

---

### 14. Measure Speedup and Parallel Overhead
#### Tasks
- [x] Compute speedup relative to serial baseline
- [x] Identify workloads with:
  - [x] strong speedup
  - [x] weak speedup
  - [x] no improvement
  - [x] regressions
- [x] Note causes such as:
  - [x] thread startup overhead
  - [x] memory bandwidth limits
  - [x] reduction overhead
  - [x] uneven work distribution
  - [x] false sharing or contention
- [x] Record findings in notes for the report

#### Deliverable
- Phase 2 includes both positive and negative findings, not just best cases

---

### 15. Add Logging and Research Notes
#### Tasks
- [x] Update `report/notes.md` with:
  - [x] which query paths were parallelized
  - [x] why those paths were chosen
  - [x] thread model used
  - [x] known bottlenecks
  - [x] workloads that scaled poorly
  - [x] failed approaches
- [x] Record whether parallelizing ingestion was attempted
- [x] Record any numerical differences in floating-point reductions
- [x] Record any memory-related observations

#### Deliverable
- Repo contains enough notes to support Phase 2 report writing

---

## File-Level TODO Suggestions

### `include/query/`
- [x] `ParallelCongestionQuery.hpp`
- [x] `ParallelTrafficAggregator.hpp`

### `include/benchmark/`
- [x] update `BenchmarkHarness.hpp` for thread-aware benchmarking

### `src/query/`
- [x] `ParallelCongestionQuery.cpp`
- [x] `ParallelTrafficAggregator.cpp`

### `src/benchmark/`
- [x] update `BenchmarkHarness.cpp`

### `apps/`
- [x] `run_parallel.cpp`

### `scripts/`
- [x] update `benchmark.sh` to run thread-count sweeps
- [x] optional helper script for serial-vs-parallel comparison

---

## Suggested Phase 2 Milestones

### Milestone A — Parallel Build Path
- [x] `run_parallel` builds
- [x] OpenMP or thread setup works
- [x] executable launches successfully

### Milestone B — First Parallel Query
- [x] speed-threshold query parallelized
- [x] correctness verified against serial

### Milestone C — Multiple Parallel Workloads
- [x] time-window query parallelized
- [x] aggregation parallelized
- [x] results verified

### Milestone D — Scaling Benchmarks
- [x] multiple thread counts tested
- [x] 10+ runs per scenario completed
- [x] raw outputs saved

### Milestone E — Research Notes
- [x] speedups documented
- [x] regressions documented
- [x] failed attempts documented

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

## Deferred From Phase 1 (Additive)
These items were identified during late Phase 1 review and are appended here for Phase 2/3 planning.

### Phase 2 tasks
- [ ] Add optional indexed lookup structures to improve cross-dataset joins without changing baseline correctness paths:
  - [ ] BBL index for garage-to-BES relationship checks
  - [ ] optional link-id index for faster repeated traffic lookups
- [ ] Prototype a materialized relationship pass for shared keys where beneficial:
  - [ ] garage `bbl` <-> BES `bbl`
  - [ ] borough-level cross-dataset rollups
- [ ] Benchmark indexed/materialized lookup overhead vs. baseline scans before enabling by default.
- [ ] Evaluate selective Boost usage for indexing/prototyping ergonomics in Phase 2:
  - [ ] `boost::container::flat_map` for cache-friendly sorted lookup experiments
  - [ ] `boost::unordered_flat_map` (if available) for hash lookup comparison against STL maps
  - [ ] `boost::dynamic_bitset` for optional membership/filter masks in validation passes
- [ ] Document which cross-dataset indexing choices are kept for Phase 2 and which are deferred to Phase 3 layout optimization.

### Handoff to Phase 3
- [ ] If index/materialization complexity grows or memory overhead is high, defer final design to Phase 3 and track benchmarks proving why.

---

---

## Phase 2 Baseline Comparative Testing and Early Data Collection
## Purpose
These tasks are for **development-stage parallel benchmarking on subset datasets**, not the final full-dataset benchmark campaign.

The goal is to:
- verify that parallel results match serial results,
- establish repeatable serial-vs-parallel benchmark workflows,
- collect preliminary scaling data,
- and ensure the benchmark harness is stable before running the full dataset later.

These tasks should be completed by the coding agent while implementing Phase 2.

### Rules for This Section
- Use **subset datasets only**
- Use the **same subset files** already created for Phase 1 development benchmarking
- Keep serial and parallel query definitions identical
- Keep benchmark commands and output formats stable across runs
- Save all raw outputs for later comparison
- Do **not** treat these results as final project conclusions

---

### 17. Reuse or Validate Subset Datasets for Phase 2
#### Tasks
- [x] Reuse the reproducible subset datasets created during Phase 1
- [x] Confirm subset row counts still match expected sizes
- [x] Confirm subset files are accessible to both serial and parallel runners
- [x] Record which subset files are used in Phase 2 development tests

#### Agent instructions
- Do not regenerate subsets unless the source data changed or earlier subset files are invalid
- Save a short manifest of subset file names and row counts under:
  - [x] `results/raw/phase2_dev/`
- Use stable file naming across repeated benchmark batches

#### Deliverable
- Phase 2 development benchmarks use consistent subset inputs

---

### 18. Add Serial-vs-Parallel Validation Runs on Subsets
#### Tasks
- [x] Run the serial query implementation on the small subset
- [x] Run the parallel query implementation on the small subset
- [ ] Repeat the same comparison for:
  - [x] medium subset
  - [x] large-dev subset
- [ ] Record for each scenario:
  - [x] result count
  - [x] aggregate values if applicable
  - [x] mismatches if found

#### Agent instructions
- Add a validation mode or helper script that runs serial and parallel back-to-back on the same subset and query scenario
- Save comparison outputs to:
  - [x] `results/raw/phase2_dev/validation/`
- If mismatches occur, save both raw outputs and log the issue clearly

#### Deliverable
- Parallel subset results are checked against serial baseline behavior

---

### 19. Add Early Parallel Benchmark Scenarios for Subsets
#### Tasks
- [x] Define a repeatable Phase 2 subset benchmark set using the same query parameters as Phase 1 where possible
- [ ] Minimum scenarios:
  - [x] serial ingest + serial low-speed threshold query
  - [x] serial ingest + parallel low-speed threshold query
  - [x] serial ingest + serial time-window query
  - [x] serial ingest + parallel time-window query
  - [x] serial ingest + serial aggregation query
  - [x] serial ingest + parallel aggregation query
- [ ] If borough filtering exists, include:
  - [x] serial ingest + serial borough + threshold query
  - [x] serial ingest + parallel borough + threshold query

#### Agent instructions
- Keep scenario names stable and machine-readable
- Use the same thresholds and time windows previously defined unless a new experiment is explicitly documented
- Record thread count in every parallel scenario name or output row

#### Deliverable
- A stable early benchmark set exists for subset-based serial-vs-parallel testing

---

### 20. Create a Repeatable Phase 2 Development Benchmark Runner
#### Tasks
- [x] Add or extend a script that runs the Phase 2 subset benchmark suite automatically
- [ ] Support:
  - [x] small subset
  - [x] medium subset
  - [x] large-dev subset
- [x] Support repeated runs per scenario
- [x] Support configurable thread counts
- [ ] Save raw outputs to:
  - [x] `results/raw/phase2_dev/`
- [ ] Save logs to:
  - [x] `results/raw/logs/`

#### Agent instructions
- Implement or extend a script such as:
  - [x] `scripts/run_phase2_dev_benchmarks.sh`
- The script should:
  - [x] accept a dataset path
  - [x] accept a repetition count
  - [x] accept one or more thread counts
  - [x] run all selected serial and parallel benchmark scenarios
  - [x] save each run result in a machine-readable format
  - [x] print a concise summary to console

#### Deliverable
- A single command can run the subset-based Phase 2 development benchmark suite

---

### 21. Collect Preliminary Serial-vs-Parallel Timing Data
#### Tasks
- [ ] Run each selected benchmark scenario at least:
  - [x] `3` times on the small subset
  - [x] `3` times on the medium subset
  - [x] `3` times on the large-dev subset
- [ ] For parallel runs, test at least:
  - [x] `1` thread
  - [x] `2` threads
  - [x] `4` threads
  - [x] `8` threads if reasonable for the VM
- [ ] Record:
  - [x] ingest time
  - [x] query time
  - [x] total runtime
  - [x] thread count
  - [x] records processed per second if available

#### Agent instructions
- The coding agent should execute the benchmark runner after changes that affect parallel query logic or reduction strategy
- Save each batch with labels including:
  - [x] git branch
  - [x] commit hash if possible
  - [x] subset size
  - [x] query scenario
  - [x] thread count
- Prefer CSV output for later summarization

#### Deliverable
- Preliminary timing and scaling data exists for subset-based Phase 2 scenarios

---

### 22. Generate Development Summary Tables for Phase 2
#### Tasks
- [ ] Add or extend a script to summarize raw Phase 2 development benchmark results
- [ ] Compute:
  - [ ] mean runtime
  - [ ] median runtime
  - [ ] minimum runtime
  - [ ] maximum runtime
  - [ ] standard deviation if enough runs exist
  - [ ] speedup relative to serial for the same subset/scenario
- [ ] Save summary tables to:
  - [ ] `results/tables/phase2_dev/`

#### Agent instructions
- Implement or extend a helper such as:
  - [ ] `scripts/summarize_phase2_dev.py`
- Read raw CSV benchmark outputs
- Produce grouped tables by:
  - [ ] subset size
  - [ ] query scenario
  - [ ] thread count
- Do not delete raw files after summarization

#### Deliverable
- Development-stage serial-vs-parallel summary tables exist and are easy to inspect

---

### 23. Generate Simple Development Graphs for Phase 2
#### Tasks
- [ ] Add or extend plotting scripts for subset-based parallel benchmark results
- [ ] Plot at least:
  - [ ] query runtime vs thread count
  - [ ] speedup vs thread count
  - [ ] serial vs parallel runtime by subset size
  - [ ] runtime by scenario
- [ ] Save graphs to:
  - [ ] `results/graphs/phase2_dev/`

#### Agent instructions
- Use Python for graph generation
- Keep graphs simple and readable
- Label axes clearly
- Include subset size, thread count, and scenario labels
- Save plots to files rather than only showing interactively

#### Deliverable
- Early Phase 2 graphs exist to show scaling trends before full-dataset testing

---

### 24. Add a Phase 2 Development Benchmark Log
#### Tasks
- [ ] Record each benchmark batch in `report/notes.md` or a dedicated Phase 2 dev benchmark log
- [ ] For each batch, record:
  - [ ] date/time
  - [ ] git branch
  - [ ] commit hash if available
  - [ ] subset size
  - [ ] benchmark scenarios run
  - [ ] thread counts tested
  - [ ] notable observations
  - [ ] failures or anomalies
- [ ] Record whether the benchmark batch followed a code change

#### Agent instructions
- The coding agent should append a short notes entry after completing a benchmark batch
- Include enough detail to trace performance changes back to code changes or thread-model changes

#### Deliverable
- Development benchmark history is documented and traceable for Phase 2

---

### 25. Add Stability and Determinism Checks for Parallel Runs
#### Tasks
- [ ] Rerun identical parallel scenarios multiple times on the same subset
- [ ] Confirm consistency of:
  - [ ] result count
  - [ ] aggregate values
  - [ ] timing distribution within reason
- [ ] Check for obvious nondeterministic result corruption
- [ ] Record any floating-point variation if present

#### Agent instructions
- Add a helper mode or script that reruns the same scenario multiple times and compares outputs
- Save mismatches and timing anomalies to:
  - [ ] `results/raw/phase2_dev/stability/`

#### Deliverable
- Parallel subset benchmarks are shown to be stable enough for development use

---

### 26. Mark These Results as Pre-Baseline Only
#### Tasks
- [ ] Add a note in the README or report notes that Phase 2 subset benchmarks are:
  - [ ] development-stage measurements
  - [ ] not the final official comparison data
- [ ] Distinguish clearly between:
  - [ ] `phase2_dev`
  - [ ] final `phase2_baseline`
- [ ] Reserve final full-dataset serial-vs-parallel benchmarking for the official comparison campaign later

#### Agent instructions
- Ensure output directories and filenames make this distinction obvious
- Do not mix subset-development outputs with final benchmark outputs

#### Deliverable
- Early Phase 2 data collection is organized without being confused with final results

---

## Extra Definition of Done for Development-Stage Parallel Benchmarking
This subsection is complete when all of the following are true:

- [ ] Reproducible subset datasets are reused successfully
- [ ] Serial and parallel programs both run successfully on all selected subset sizes
- [ ] At least 3 benchmark scenarios run repeatedly in serial and parallel modes
- [ ] Multiple thread counts are tested
- [ ] Raw results are saved to disk
- [ ] Summary tables are generated
- [ ] Simple graphs are generated
- [ ] Development benchmark notes are recorded
- [ ] Outputs are clearly labeled as pre-baseline and not final full-dataset benchmarks
