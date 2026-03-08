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
- [ ] Confirm the repository builds with `cmake` and `clang++` or `g++`
- [ ] Create a root `CMakeLists.txt` that builds:
  - [ ] `run_serial`
  - [ ] placeholder targets for later phases if needed
- [ ] Enable warnings:
  - [ ] `-Wall`
  - [ ] `-Wextra`
  - [ ] `-pedantic`
- [ ] Add a debug build option
- [ ] Add a release build option
- [ ] Verify clean build from scratch

#### Deliverable
- Build succeeds with:
  - `cmake -S . -B build`
  - `cmake --build build`

---

### 2. Define Core Data Model
#### Tasks
- [ ] Create `TrafficRecord`
- [ ] Create `TrafficDataset`
- [ ] Create `GarageRecord`
- [ ] Create `BuildingRecord` or minimal BES placeholder model
- [ ] Create `GeoPoint` if spatial support is needed in Phase 1
- [ ] Use primitive field types wherever possible:
  - [ ] integers for IDs when valid
  - [ ] `float` or `double` for speed/travel time
  - [ ] compact date/time representation where practical
  - [ ] strings only where unavoidable
- [ ] Document any field-normalization assumptions

#### Minimum fields for `TrafficRecord`
- [ ] link ID
- [ ] speed
- [ ] travel time
- [ ] timestamp or parsed time representation
- [ ] borough if available
- [ ] optional link name or location descriptor

#### Deliverable
- Header and source files compile cleanly
- Data model is documented in comments or notes

---

### 3. Implement Serial File Ingestion
#### Tasks
- [ ] Create `CSVReader` or equivalent parser wrapper
- [ ] Implement serial parsing for DOT traffic speed data
- [ ] Load records into `TrafficDataset`
- [ ] Add input validation for malformed rows
- [ ] Count:
  - [ ] rows read
  - [ ] rows accepted
  - [ ] rows rejected
- [ ] Add progress logging for long dataset loads
- [ ] Support configurable input path through CLI arguments

#### Supporting dataset ingestion
- [ ] Add basic ingestion for garages dataset
- [ ] Add basic ingestion or stub loader for BES dataset
- [ ] Keep these loaders simple in Phase 1
- [ ] Focus effort on traffic dataset first

#### Deliverable
- `run_serial` can ingest at least one real traffic dataset file end-to-end
- Loader prints summary statistics after ingest

---

### 4. Implement Dataset Container APIs
#### Tasks
- [ ] Add methods to `TrafficDataset` for:
  - [ ] record count
  - [ ] empty check
  - [ ] direct record access if needed
  - [ ] iteration support
- [ ] Separate ingestion from query logic
- [ ] Keep storage simple for baseline:
  - [ ] likely `std::vector<TrafficRecord>`

#### Deliverable
- Dataset can be loaded once and reused across multiple query runs

---

### 5. Implement Phase 1 Query APIs
#### Required baseline query capabilities
- [ ] Time-range filtering
- [ ] Speed-threshold filtering
- [ ] Borough filtering if supported by dataset
- [ ] Link-ID lookup or link-range query
- [ ] Top-N slowest links or records
- [ ] Basic aggregation:
  - [ ] average speed
  - [ ] average travel time
  - [ ] record count by condition

#### Suggested query classes
- [ ] `TimeWindowQuery`
- [ ] `CongestionQuery`
- [ ] `TrafficAggregator`

#### Example baseline queries to support
- [ ] all records with speed below threshold
- [ ] all records for a selected link ID
- [ ] all records within a time window
- [ ] all records in a borough with speed below threshold
- [ ] top N slowest recurring links

#### Deliverable
- Query methods callable from `run_serial`
- Query outputs are deterministic and easy to benchmark

---

### 6. Build a Serial CLI Runner
#### Tasks
- [ ] Implement `apps/run_serial.cpp`
- [ ] Accept command-line arguments for:
  - [ ] traffic dataset path
  - [ ] garages dataset path optional
  - [ ] BES dataset path optional
  - [ ] query type
  - [ ] threshold values
  - [ ] time range values
- [ ] Print useful execution logs:
  - [ ] dataset load started
  - [ ] dataset load completed
  - [ ] query started
  - [ ] query completed
  - [ ] elapsed time
- [ ] Return nonzero exit code on failure

#### Deliverable
- One executable that can load data and run at least 3 benchmarkable query scenarios

---

### 7. Add Timing and Benchmark Harness
#### Tasks
- [ ] Create `BenchmarkHarness`
- [ ] Measure:
  - [ ] ingest time
  - [ ] query time
  - [ ] total runtime
- [ ] Use consistent timing approach with `std::chrono`
- [ ] Run each benchmark scenario at least 10 times
- [ ] Save benchmark outputs to:
  - [ ] `results/raw/`
- [ ] Produce machine-readable output:
  - [ ] CSV preferred
  - [ ] JSON acceptable if easier
- [ ] Include metadata in benchmark output:
  - [ ] date/time
  - [ ] binary name
  - [ ] compiler if known
  - [ ] dataset path or dataset label
  - [ ] query type
  - [ ] run number

#### Deliverable
- Repeated serial benchmark results stored on disk
- Results format is easy to graph later

---

### 8. Add Validation and Sanity Checks
#### Tasks
- [ ] Validate parsed speed ranges
- [ ] Validate travel time is nonnegative where expected
- [ ] Detect missing or malformed timestamps
- [ ] Add sample record print mode for debugging
- [ ] Add counts for suspicious rows
- [ ] Compare accepted row counts against expectations

#### Deliverable
- Loader can identify dirty data instead of silently failing

---

### 9. Add Logging and Notes for Research Use
#### Tasks
- [ ] Create `report/notes.md` entries for:
  - [ ] parsing assumptions
  - [ ] rejected-row logic
  - [ ] timestamp handling
  - [ ] fields ignored in baseline
- [ ] Record failed attempts
- [ ] Record performance surprises
- [ ] Record dataset-specific issues

#### Deliverable
- Repo contains enough notes to support report writing later

---

### 10. Produce Baseline Benchmark Scenarios
#### Minimum scenarios
- [ ] Full dataset ingest only
- [ ] Full dataset ingest + speed threshold query
- [ ] Full dataset ingest + time window query
- [ ] Full dataset ingest + borough + threshold query
- [ ] Full dataset ingest + top-N slowest query

#### Benchmark requirements
- [ ] 10 or more runs per scenario
- [ ] store raw outputs
- [ ] compute average later or in helper script

#### Deliverable
- A reproducible serial baseline for later comparison

---

## File-Level TODO Suggestions

### `include/data_model/`
- [ ] `TrafficRecord.hpp`
- [ ] `TrafficDataset.hpp`
- [ ] `GarageRecord.hpp`
- [ ] `BuildingRecord.hpp`
- [ ] `GeoPoint.hpp`

### `include/io/`
- [ ] `CSVReader.hpp`
- [ ] `TrafficLoader.hpp`
- [ ] `GarageLoader.hpp`
- [ ] `BuildingLoader.hpp`

### `include/query/`
- [ ] `TimeWindowQuery.hpp`
- [ ] `CongestionQuery.hpp`
- [ ] `TrafficAggregator.hpp`

### `include/benchmark/`
- [ ] `BenchmarkHarness.hpp`

### `src/...`
- [ ] implement matching `.cpp` files for all of the above

### `apps/`
- [ ] `run_serial.cpp`

### `scripts/`
- [ ] `benchmark.sh`
- [ ] optional helper for repeated serial runs

---

## Suggested Phase 1 Milestones

### Milestone A — Build and Models
- [ ] CMake works
- [ ] core classes compile
- [ ] placeholder executable runs

### Milestone B — Ingestion
- [ ] traffic dataset loads
- [ ] row counts and validation work
- [ ] summary logs print correctly

### Milestone C — Query Layer
- [ ] time and threshold queries work
- [ ] outputs look correct on sample data

### Milestone D — Benchmark Baseline
- [ ] serial timing is implemented
- [ ] repeated runs complete
- [ ] outputs saved in `results/raw`

### Milestone E — Research Notes
- [ ] assumptions documented
- [ ] known issues documented
- [ ] failed attempts recorded

---

## Definition of Done for Phase 1
Phase 1 is complete when all of the following are true:

- [ ] The project builds cleanly
- [ ] Traffic data can be ingested serially
- [ ] At least 3 useful query types are implemented
- [ ] Serial benchmark runs execute repeatedly and save results
- [ ] No threading or OpenMP is used
- [ ] Notes exist to support the report
- [ ] The codebase is clean enough to branch into Phase 2 parallelization

---

## Explicit Non-Goals for Phase 1
Do not do these yet:
- [ ] no multithreading
- [ ] no OpenMP
- [ ] no object-of-arrays rewrite
- [ ] no aggressive micro-optimizations
- [ ] no premature spatial indexing unless required for correctness
- [ ] no Phase 2 or Phase 3 benchmarking claims

---