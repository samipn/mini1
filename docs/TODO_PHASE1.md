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
