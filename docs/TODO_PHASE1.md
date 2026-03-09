# UrbanDrop Mini 1A — Phase 1 TODO
## System 1: Congestion Intelligence Engine
## Phase 1 Goal: Serial Baseline

This document defines the Phase 1 implementation plan for System 1. The goal is to build a **serial C/C++ baseline** for ingesting large traffic datasets, exposing query APIs, and benchmarking performance before any parallelization or memory-layout optimization.

## Phase 1 Constraints
- No threads
- No OpenMP
- No databases
- Focus on correctness, clean structure, and measurable baseline performance
- Use primitive data types where appropriate
- Keep design object-oriented and reusable

---

## Phase 1 Success Criteria
By the end of Phase 1, the repository should contain:

1. A buildable C++ project using CMake
2. A serial ingestion pipeline for traffic speed data
3. Basic supporting ingestion for shared infrastructure layers
4. Query APIs for range and threshold-based traffic analysis
5. A serial benchmark runner
6. Raw benchmark outputs saved to disk
7. Notes on assumptions, failed attempts, and open issues

---

## Implementation Order

### 1. Repository and Build Validation
#### Tasks
- [x] Confirm the repository builds with `cmake` and `clang++` or `g++`
- [x] Create a root `CMakeLists.txt` that builds:
  - [x] `run_serial`
  - [x] placeholder targets for later phases if needed
- [x] Enable warnings:
  - [x] `-Wall`
  - [x] `-Wextra`
  - [x] `-pedantic`
- [x] Add a debug build option
- [x] Add a release build option
- [x] Verify clean build from scratch

#### Deliverable
- Build succeeds with:
  - `cmake -S . -B build`
  - `cmake --build build`

---

### 2. Define Core Data Model
#### Tasks
- [x] Create `TrafficRecord`
- [x] Create `TrafficDataset`
- [x] Create `GarageRecord`
- [x] Create `BuildingRecord` or minimal BES placeholder model
- [x] Create `GeoPoint` if spatial support is needed in Phase 1
- [x] Use primitive field types wherever possible:
  - [x] integers for IDs when valid
  - [x] `float` or `double` for speed/travel time
  - [x] compact date/time representation where practical
  - [x] strings only where unavoidable
- [x] Document any field-normalization assumptions

#### Minimum fields for `TrafficRecord`
- [x] link ID
- [x] speed
- [x] travel time
- [x] timestamp or parsed time representation
- [x] borough if available
- [x] optional link name or location descriptor

#### Deliverable
- Header and source files compile cleanly
- Data model is documented in comments or notes

---

### 3. Implement Serial File Ingestion
#### Tasks
- [x] Create `CSVReader` or equivalent parser wrapper
- [x] Implement serial parsing for DOT traffic speed data
- [x] Load records into `TrafficDataset`
- [x] Add input validation for malformed rows
- [x] Count:
  - [x] rows read
  - [x] rows accepted
  - [x] rows rejected
- [x] Add progress logging for long dataset loads
- [x] Support configurable input path through CLI arguments

#### Supporting dataset ingestion
- [x] Add basic ingestion for garages dataset
- [x] Add basic ingestion or stub loader for BES dataset
- [x] Keep these loaders simple in Phase 1
- [x] Focus effort on traffic dataset first

#### Deliverable
- `run_serial` can ingest at least one real traffic dataset file end-to-end
- Loader prints summary statistics after ingest

---

### 4. Implement Dataset Container APIs
#### Tasks
- [x] Add methods to `TrafficDataset` for:
  - [x] record count
  - [x] empty check
  - [x] direct record access if needed
  - [x] iteration support
- [x] Separate ingestion from query logic
- [x] Keep storage simple for baseline:
  - [x] likely `std::vector<TrafficRecord>`

#### Deliverable
- Dataset can be loaded once and reused across multiple query runs

---

### 5. Implement Phase 1 Query APIs
#### Required baseline query capabilities
- [x] Time-range filtering
- [x] Speed-threshold filtering
- [x] Borough filtering if supported by dataset
- [x] Link-ID lookup or link-range query
- [x] Top-N slowest links or records
- [x] Basic aggregation:
  - [x] average speed
  - [x] average travel time
  - [x] record count by condition

#### Suggested query classes
- [x] `TimeWindowQuery`
- [x] `CongestionQuery`
- [x] `TrafficAggregator`

#### Example baseline queries to support
- [x] all records with speed below threshold
- [x] all records for a selected link ID
- [x] all records within a time window
- [x] all records in a borough with speed below threshold
- [x] top N slowest recurring links

#### Deliverable
- Query methods callable from `run_serial`
- Query outputs are deterministic and easy to benchmark

---

### 6. Build a Serial CLI Runner
#### Tasks
- [x] Implement `apps/run_serial.cpp`
- [x] Accept command-line arguments for:
  - [x] traffic dataset path
  - [x] garages dataset path optional
  - [x] BES dataset path optional
  - [x] query type
  - [x] threshold values
  - [x] time range values
- [x] Print useful execution logs:
  - [x] dataset load started
  - [x] dataset load completed
  - [x] query started
  - [x] query completed
  - [x] elapsed time
- [x] Return nonzero exit code on failure

#### Deliverable
- One executable that can load data and run at least 3 benchmarkable query scenarios

---

### 7. Add Timing and Benchmark Harness
#### Tasks
- [x] Create `BenchmarkHarness`
- [x] Measure:
  - [x] ingest time
  - [x] query time
  - [x] total runtime
- [x] Use consistent timing approach with `std::chrono`
- [x] Run each benchmark scenario at least 10 times
- [x] Save benchmark outputs to:
  - [x] `results/raw/`
- [x] Produce machine-readable output:
  - [x] CSV preferred
  - [ ] JSON acceptable if easier
- [x] Include metadata in benchmark output:
  - [x] date/time
  - [x] binary name
  - [x] compiler if known
  - [x] dataset path or dataset label
  - [x] query type
  - [x] run number

#### Deliverable
- Repeated serial benchmark results stored on disk
- Results format is easy to graph later

---

### 8. Add Validation and Sanity Checks
#### Tasks
- [x] Validate parsed speed ranges
- [x] Validate travel time is nonnegative where expected
- [x] Detect missing or malformed timestamps
- [x] Add sample record print mode for debugging
- [x] Add counts for suspicious rows
- [x] Compare accepted row counts against expectations

#### Deliverable
- Loader can identify dirty data instead of silently failing

---

### 9. Add Logging and Notes for Research Use
#### Tasks
- [x] Create `report/notes.md` entries for:
  - [x] parsing assumptions
  - [x] rejected-row logic
  - [x] timestamp handling
  - [x] fields ignored in baseline
- [x] Record failed attempts
- [x] Record performance surprises
- [x] Record dataset-specific issues

#### Deliverable
- Repo contains enough notes to support report writing later

---

### 10. Produce Baseline Benchmark Scenarios
#### Minimum scenarios
- [x] Full dataset ingest only
- [x] Full dataset ingest + speed threshold query
- [x] Full dataset ingest + time window query
- [x] Full dataset ingest + borough + threshold query
- [x] Full dataset ingest + top-N slowest query

#### Benchmark requirements
- [x] 10 or more runs per scenario
- [x] store raw outputs
- [x] compute average later or in helper script

#### Deliverable
- A reproducible serial baseline for later comparison

---

## File-Level TODO Suggestions

### `include/data_model/`
- [x] `TrafficRecord.hpp`
- [x] `TrafficDataset.hpp`
- [x] `GarageRecord.hpp`
- [x] `BuildingRecord.hpp`
- [x] `GeoPoint.hpp`

### `include/io/`
- [x] `CSVReader.hpp`
- [ ] `TrafficLoader.hpp`
- [x] `GarageLoader.hpp`
- [x] `BuildingLoader.hpp`

### `include/query/`
- [x] `TimeWindowQuery.hpp`
- [x] `CongestionQuery.hpp`
- [x] `TrafficAggregator.hpp`

### `include/benchmark/`
- [x] `BenchmarkHarness.hpp`

### `src/...`
- [x] implement matching `.cpp` files for all of the above

### `apps/`
- [x] `run_serial.cpp`

### `scripts/`
- [x] `benchmark.sh`
- [x] optional helper for repeated serial runs

---

## Suggested Phase 1 Milestones

### Milestone A — Build and Models
- [x] CMake works
- [x] core classes compile
- [x] placeholder executable runs

### Milestone B — Ingestion
- [x] traffic dataset loads
- [x] row counts and validation work
- [x] summary logs print correctly

### Milestone C — Query Layer
- [x] time and threshold queries work
- [x] outputs look correct on sample data

### Milestone D — Benchmark Baseline
- [x] serial timing is implemented
- [x] repeated runs complete
- [x] outputs saved in `results/raw`

### Milestone E — Research Notes
- [x] assumptions documented
- [x] known issues documented
- [x] failed attempts recorded

---

## Definition of Done for Phase 1
Phase 1 is complete when all of the following are true:

- [x] The project builds cleanly
- [x] Traffic data can be ingested serially
- [x] At least 3 useful query types are implemented
- [x] Serial benchmark runs execute repeatedly and save results
- [x] No threading or OpenMP is used
- [x] Notes exist to support the report
- [x] The codebase is clean enough to branch into Phase 2 parallelization

---

## Explicit Non-Goals for Phase 1
Do not do these yet:
- [x] no multithreading
- [x] no OpenMP
- [x] no object-of-arrays rewrite
- [x] no aggressive micro-optimizations
- [x] no premature spatial indexing unless required for correctness
- [x] no Phase 2 or Phase 3 benchmarking claims

---

## Phase 1 Baseline Subset Testing and Early Data Collection
## Purpose
These tasks are for **development-stage benchmarking on smaller subsets**, not the final full-dataset benchmark campaign.

The goal is to:
- verify correctness early,
- establish repeatable benchmark workflows,
- collect preliminary performance data,
- and ensure the benchmark harness is stable before running the full dataset later.

These tasks should be completed by the coding agent while implementing Phase 1.

### Rules for This Section
- Use **subset datasets only**
- Do **not** treat these results as final project conclusions
- Keep dataset slices reproducible
- Keep benchmark commands and output formats stable across runs
- Save all raw outputs for later comparison

---

### 11. Create Reproducible Subset Datasets
#### Tasks
- [x] Add a script that creates reproducible subset files from the main traffic dataset
- [x] Support at least these subset sizes:
  - [x] small subset
  - [x] medium subset
  - [x] large-dev subset
- [x] Suggested targets:
  - [x] `10,000` rows
  - [x] `100,000` rows
  - [x] `1,000,000` rows
- [x] Preserve the original header row
- [x] Keep the subset generation deterministic
- [x] Save subset files under a predictable directory such as:
  - [x] `data/dev/`
  - [x] or `data/subsets/`
- [x] Log the source file and row counts used to create each subset

#### Agent instructions
- Implement a script such as:
  - [x] `scripts/make_subsets.sh`
  - [x] or `scripts/make_subsets.py`
- The script should accept:
  - [x] input dataset path
  - [x] output directory
  - [x] requested subset sizes
- [x] The script should print progress and final file paths

#### Deliverable
- Reproducible subset datasets exist for early benchmark work

---

### 12. Add Sample Validation Runs on Subsets
#### Tasks
- [x] Run the serial loader on the small subset
- [x] Run the serial loader on the medium subset
- [x] Run the serial loader on the large-dev subset
- [x] Record:
  - [x] rows read
  - [x] rows accepted
  - [x] rows rejected
  - [x] parse failures by type if available
- [x] Save validation summaries to:
  - [x] `results/raw/validation/`

#### Agent instructions
- Add a mode in `run_serial` or a helper script that prints a loader summary cleanly
- Save the output to timestamped files
- Do not overwrite earlier runs unless explicitly requested

#### Deliverable
- Baseline ingestion correctness is checked on multiple subset sizes

---

### 13. Add Early Benchmark Scenarios for Subset Datasets
#### Tasks
- [x] Define a small set of repeatable Phase 1 subset benchmark scenarios
- [x] Use the same query definitions each time
- [x] Minimum subset scenarios:
  - [x] ingest only
  - [x] ingest + low-speed threshold query
  - [x] ingest + time-window query
  - [x] ingest + borough + threshold query if borough data is available
  - [x] ingest + top-N slowest query

#### Suggested default development parameters
- [x] low-speed threshold:
  - [x] `8 mph` or another documented threshold
- [x] a fixed time window for repeatability
- [x] a fixed borough if used
- [x] a fixed top-N size such as:
  - [x] `10`
  - [x] or `100`

#### Agent instructions
- Create a benchmark scenario definition file if useful
- Keep the scenario names stable
- Use the same parameter values across subset benchmark runs unless a new experiment explicitly requires change

#### Deliverable
- A stable early benchmark set exists for subset-based development testing

---

### 14. Create a Repeatable Benchmark Runner for Subsets
#### Tasks
- [x] Add a script that runs the Phase 1 subset benchmarks automatically
- [x] Support running the benchmark suite against:
  - [x] small subset
  - [x] medium subset
  - [x] large-dev subset
- [x] Support repeated runs per scenario
- [x] Save raw outputs to:
  - [x] `results/raw/phase1_dev/`
- [x] Save logs to:
  - [x] `results/raw/logs/`

#### Agent instructions
- Implement a script such as:
  - [x] `scripts/run_phase1_dev_benchmarks.sh`
- The script should:
  - [x] accept a dataset path
  - [x] accept a repetition count
  - [x] run all selected benchmark scenarios
  - [x] save each run result in a machine-readable format
  - [x] print a concise summary to console
- Keep this runner compatible with later reuse in other branches if possible

#### Deliverable
- A single command can run the subset benchmark suite and save outputs

---

### 15. Collect Preliminary Timing Data
#### Tasks
- [x] Run each subset benchmark scenario at least:
  - [x] `3` times on the small subset
  - [x] `3` times on the medium subset
  - [x] `3` times on the large-dev subset
- [x] Record:
  - [x] ingest time
  - [x] query time
  - [x] total runtime
  - [x] records processed per second if available
- [x] Store each run separately before computing summaries

#### Agent instructions
- The coding agent should execute the benchmark runner after implementation changes that affect loading or query behavior
- Save each batch with a clear label indicating:
  - [x] git branch
  - [x] commit hash if possible
  - [x] dataset subset size
  - [x] query scenario
- Prefer CSV output for later aggregation

#### Deliverable
- Preliminary timing data exists for multiple subset sizes and benchmark scenarios

---

### 16. Generate Development Summary Tables
#### Tasks
- [x] Add a small script to summarize raw subset benchmark results
- [x] Compute:
  - [x] mean runtime
  - [x] median runtime
  - [x] minimum runtime
  - [x] maximum runtime
  - [x] standard deviation if enough runs exist
- [x] Save summary tables to:
  - [x] `results/tables/phase1_dev/`

#### Agent instructions
- Implement a helper such as:
  - [x] `scripts/summarize_phase1_dev.py`
- Read raw CSV benchmark outputs
- Produce one summary table per dataset subset or one combined table with clear labels
- Do not delete raw files after summarization

#### Deliverable
- Development-stage benchmark summaries exist and are easy to inspect

---

### 17. Generate Simple Development Graphs
#### Tasks
- [x] Add a plotting script for subset benchmark results
- [x] Plot at least:
  - [x] ingest time vs subset size
  - [x] total runtime vs subset size
  - [x] query runtime by scenario
- [x] Save graphs to:
  - [x] `results/graphs/phase1_dev/`

#### Agent instructions
- Use Python for graph generation
- Keep graphs simple and readable
- Label all axes clearly
- Include subset size and scenario labels
- Save plots to file rather than only displaying them interactively

#### Deliverable
- Early graphs exist to show scaling trends before full-dataset testing

---

### 18. Add a Phase 1 Development Benchmark Log
#### Tasks
- [x] Record each benchmark batch in `report/notes.md` or a dedicated dev benchmark log
- [x] For each batch, record:
  - [x] date/time
  - [x] git branch
  - [x] commit hash if available
  - [x] subset size
  - [x] benchmark scenarios run
  - [x] notable observations
  - [x] failures or anomalies
- [x] Record whether the benchmark run followed a code change

#### Agent instructions
- The coding agent should append a short notes entry after completing a benchmark batch
- Include enough detail that the team can later trace performance changes back to code changes

#### Deliverable
- Development benchmark history is documented and traceable

---

### 19. Add a Correctness Cross-Check Between Subsets
#### Tasks
- [x] Compare query result counts across repeated runs on the same subset
- [x] Confirm deterministic results for:
  - [x] low-speed threshold query
  - [x] time-window query
  - [x] borough + threshold query if applicable
  - [x] top-N query if deterministic ordering rules are defined
- [x] Record any nondeterministic behavior

#### Agent instructions
- Add a validation mode or helper script that reruns the same subset query and checks:
  - [x] result count consistency
  - [x] aggregate consistency
- If mismatches occur, save both outputs and log the issue

#### Deliverable
- Subset benchmark runs are shown to be stable enough for baseline development use

---

### 20. Mark These Results as Pre-Baseline Only
#### Tasks
- [x] Add a note in the README or report notes that subset benchmarks are:
  - [x] development-stage measurements
  - [x] not the final official comparison data
- [x] Distinguish clearly between:
  - [x] `phase1_dev`
  - [x] final `phase1_baseline`
- [x] Reserve final full-dataset benchmarking for the official comparison campaign later

#### Agent instructions
- Ensure output directories and filenames make this distinction obvious
- Do not mix subset-development outputs with final benchmark outputs

#### Deliverable
- Early data collection is useful and organized without being confused with final results

---

## Extra Definition of Done for Development-Stage Benchmarking
This subsection is complete when all of the following are true:

- [x] Reproducible subset datasets exist
- [x] The serial program runs successfully on all selected subset sizes
- [x] At least 3 benchmark scenarios run repeatedly on subsets
- [x] Raw results are saved to disk
- [x] Summary tables are generated
- [x] Simple graphs are generated
- [x] Development benchmark notes are recorded
- [x] Outputs are clearly labeled as pre-baseline and not final full-dataset benchmarks