# UrbanDrop Mini 1A — Congestion Intelligence Engine

## Project Metadata

| Field | Value |
|---|---|
| **Assignment** | UrbanDrop Mini 1A |
| **Course context** | Memory Overload (v0.1a) |
| **System** | System 1 — Congestion Intelligence Engine |
| **Language** | C/C++ (gcc/g++ v13+ or clang/clang++ v16+) |
| **Build system** | CMake |
| **Primary dataset** | Transportation DOT Traffic Speeds NBE (~40 GB) |
| **Supporting datasets** | Active DCA-Licensed Garages and Parking Lots; Building Elevation and Subgrade (BES) |
| **Min. combined size** | > 12 GB |
| **Min. combined records** | > 2 million |
| **Data source** | NYC OpenData |
| **Time windows** | 2020–2022, 2022–2024, 2024–2026 |
| **Primary research area** | Temporal congestion analysis and traffic hotspot detection |
| **Secondary research area** | Memory-efficient query processing over large urban mobility datasets |
| **Parallelization** | OpenMP / threading (Phase 2) |
| **Memory optimization** | Object-of-Arrays / vectorized layout (Phase 3) |
| **Benchmark runs** | ≥ 10 per experiment |
| **Deliverables** | Code, report, one-slide presentation, `.tar.gz` submission |


## Project Context
This repository contains **System 1** of our two-part UrbanDrop research effort for **Memory Overload (v0.1a)**.

UrbanDrop is a research concept centered on whether autonomous delivery workflows could reduce curbside delivery pressure by shifting portions of last-mile exchange activity away from congested streets and toward designated pickup or transfer points. In this repository, we focus specifically on the **traffic and congestion side** of that question.

This system is designed as a **high-performance C/C++ data-processing and query engine** for large-scale urban traffic analysis. The project is not an application prototype; it is a performance-oriented research system built to study how efficiently large NYC mobility datasets can be ingested, queried, parallelized, and optimized in memory, in line with the mini assignment guidelines.

## Assignment Focus
- Assignment name/code: `UrbanDrop Mini 1A`
- Primary research area: `Temporal congestion analysis and traffic hotspot detection`
- Secondary research area: `Memory-efficient query processing over large urban mobility datasets`
- Hypothesis to test: `Persistent congestion patterns on major NYC road links create identifiable delivery-friction zones, and a high-performance C/C++ query system can efficiently expose those patterns across multiple operational eras.`

## Research Question
**Which streets, corridors, and local areas in New York City show persistent or recurring congestion patterns severe enough to suggest that traditional curbside last-mile delivery is inefficient, and how do those patterns differ across the 2020–2022, 2022–2024, and 2024–2026 eras?**

This repository studies the **problem side** of UrbanDrop:

1. Where is congestion happening?
2. When is it happening?
3. How stable are those congestion patterns across different city operating conditions?
4. Which road segments may be poor candidates for repeated curbside delivery activity?

## System Scope
System 1 is the **Congestion Intelligence Engine**.

Its purpose is to:
- ingest large NYC traffic-speed datasets,
- parse and store records using appropriate primitive data types,
- support API-level temporal and range-based queries,
- benchmark serial, parallel, and optimized implementations,
- produce congestion-oriented outputs that can later inform UrbanDrop feasibility analysis.

This system is primarily built around NYC traffic-speed data. The uploaded traffic speed detector metadata describes the dataset as a real-time feed from NYCDOT sensors, mostly on major arterials and highways within city limits, including link IDs, average speed, travel time, timestamps, and borough/location link information.

## Datasets

### Primary Dataset
- **Transportation DOT Traffic Speeds NBE** (`~40 GB`)
- Role in this system:
  - primary traffic-performance dataset
  - source for congestion detection
  - source for temporal comparisons across operating eras
  - source for road-link range queries and benchmark workloads

The NYCDOT traffic speed detector data includes average speed, travel time, link identifiers, timestamps, and borough/link descriptions, making it well suited for large-scale traffic analysis.  

### Supporting Datasets
- **Active DCA-Licensed Garages and Parking Lots**
- **Building Elevation and Subgrade (BES)**

The garages dataset contains licensed garage and parking-lot records with address, borough, detail fields, and latitude/longitude coordinates, which makes it useful as a supporting infrastructure layer for later comparison against congestion hotspots.
The Building Elevation and Subgrade dataset is also included by dataset title as a shared supporting layer for the UrbanDrop project. 

### Official Links
- Active DCA-Licensed Garages and Parking Lots: <https://data.cityofnewyork.us/Business/Active-DCA-Licensed-Garages-and-Parking-Lots/a7m8-iids/about_data>
- Building Elevation and Subgrade (BES): <https://data.cityofnewyork.us/City-Government/Building-Elevation-and-Subgrade-BES-/bsin-59hv/about_data>

## Why This System Exists
UrbanDrop is motivated by the idea that some parts of the city may be especially poor fits for repeated curbside delivery access because traffic already moves slowly and unpredictably there. Before evaluating infrastructure solutions, we first need a defensible answer to a simpler question:

**Is there a measurable, recurring congestion problem, and where is it strongest?**

This repository is therefore focused on **problem detection**, not solution ranking.

## Time Windows Under Study
We organize analysis around three operating eras:

- **2020–2022** — COVID-era delivery surge and atypical mobility patterns
- **2022–2024** — partial return to normality
- **2024–2026** — more normal operating baseline

These windows are used to compare how congestion intensity, persistence, and time-of-day behavior changed across distinct urban conditions.

## Core Requirements from Guidelines

### Dataset
- Source: NYC OpenData / approved public dataset sources used by the team
- Combined dataset size must be **> 12 GB**
- Combined records must be **> 2 million**
- This repository exceeds the size threshold through the selected traffic-speed dataset and supporting layers
- Do not use:
  - 2018 Squirrel Census
  - Moving collisions/violations dataset

These assignment requirements come directly from the mini guidelines.# UrbanDrop Mini 1A — Congestion Intelligence Engine

## Project Context
This repository contains **System 1** of our two-part UrbanDrop research effort for **Memory Overload (v0.1a)**.

UrbanDrop is a research concept centered on whether autonomous delivery workflows could reduce curbside delivery pressure by shifting portions of last-mile exchange activity away from congested streets and toward designated pickup or transfer points. In this repository, we focus specifically on the **traffic and congestion side** of that question.

This system is designed as a **high-performance C/C++ data-processing and query engine** for large-scale urban traffic analysis. The project is not an application prototype; it is a performance-oriented research system built to study how efficiently large NYC mobility datasets can be ingested, queried, parallelized, and optimized in memory, in line with the mini assignment guidelines.

## Assignment Focus
- Assignment name/code: `UrbanDrop Mini 1A`
- Primary research area: `Temporal congestion analysis and traffic hotspot detection`
- Secondary research area: `Memory-efficient query processing over large urban mobility datasets`
- Hypothesis to test: `Persistent congestion patterns on major NYC road links create identifiable delivery-friction zones, and a high-performance C/C++ query system can efficiently expose those patterns across multiple operational eras.`

## Research Question
**Which streets, corridors, and local areas in New York City show persistent or recurring congestion patterns severe enough to suggest that traditional curbside last-mile delivery is inefficient, and how do those patterns differ across the 2020–2022, 2022–2024, and 2024–2026 eras?**

This repository studies the **problem side** of UrbanDrop:

1. Where is congestion happening?
2. When is it happening?
3. How stable are those congestion patterns across different city operating conditions?
4. Which road segments may be poor candidates for repeated curbside delivery activity?

## System Scope
System 1 is the **Congestion Intelligence Engine**.

Its purpose is to:
- ingest large NYC traffic-speed datasets,
- parse and store records using appropriate primitive data types,
- support API-level temporal and range-based queries,
- benchmark serial, parallel, and optimized implementations,
- produce congestion-oriented outputs that can later inform UrbanDrop feasibility analysis.

This system is primarily built around NYC traffic-speed data. The uploaded traffic speed detector metadata describes the dataset as a real-time feed from NYCDOT sensors, mostly on major arterials and highways within city limits, including link IDs, average speed, travel time, timestamps, and borough/location link information.

## Datasets

### Primary Dataset
- **Transportation DOT Traffic Speeds NBE** (`~40 GB`)
- Role in this system:
  - primary traffic-performance dataset
  - source for congestion detection
  - source for temporal comparisons across operating eras
  - source for road-link range queries and benchmark workloads

The NYCDOT traffic speed detector data includes average speed, travel time, link identifiers, timestamps, and borough/link descriptions, making it well suited for large-scale traffic analysis.

### Supporting Datasets
- **Active DCA-Licensed Garages and Parking Lots**
- **Building Elevation and Subgrade (BES)**

The garages dataset contains licensed garage and parking-lot records with address, borough, detail fields, and latitude/longitude coordinates, which makes it useful as a supporting infrastructure layer for later comparison against congestion hotspots.
The Building Elevation and Subgrade dataset is also included by dataset title as a shared supporting layer for the UrbanDrop project.

### Official Links
- Active DCA-Licensed Garages and Parking Lots: <https://data.cityofnewyork.us/Business/Active-DCA-Licensed-Garages-and-Parking-Lots/a7m8-iids/about_data>
- Building Elevation and Subgrade (BES): <https://data.cityofnewyork.us/City-Government/Building-Elevation-and-Subgrade-BES-/bsin-59hv/about_data>

## Why This System Exists
UrbanDrop is motivated by the idea that some parts of the city may be especially poor fits for repeated curbside delivery access because traffic already moves slowly and unpredictably there. Before evaluating infrastructure solutions, we first need a defensible answer to a simpler question:

**Is there a measurable, recurring congestion problem, and where is it strongest?**

This repository is therefore focused on **problem detection**, not solution ranking.

## Time Windows Under Study
We organize analysis around three operating eras:

- **2020–2022** — COVID-era delivery surge and atypical mobility patterns
- **2022–2024** — partial return to normality
- **2024–2026** — more normal operating baseline

These windows are used to compare how congestion intensity, persistence, and time-of-day behavior changed across distinct urban conditions.

## Core Requirements from Guidelines

### Dataset
- Source: NYC OpenData / approved public dataset sources used by the team
- Combined dataset size must be **> 12 GB**
- Combined records must be **> 2 million**
- This repository exceeds the size threshold through the selected traffic-speed dataset and supporting layers
- Do not use:
  - 2018 Squirrel Census
  - Moving collisions/violations dataset

These assignment requirements come directly from the mini guidelines.

### Phase 1: Serial Baseline (No Threads)
1. Implement C/C++ ingestion and query APIs for the selected datasets.
2. Use object-oriented design with abstractions intended for long-term reuse.
3. Represent fields using appropriate primitive types.
4. Provide API-level capabilities for:
   - data reading/parsing,
   - time-range filtering,
   - road-link range searching,
   - congestion-threshold filtering.
5. Benchmark serial performance with **10+ runs** and record averages.

The mini explicitly requires serial processing for Phase 1 and treats this baseline as the foundation for later comparison.

### Phase 2: Parallelization
1. Parallelize relevant processing paths using OpenMP or threading.
2. Re-run the benchmark harness on the same workloads.
3. Compare speedup and tradeoffs against the serial baseline.

This follows the mini’s Phase 2 requirement to parallelize and compare results against the original benchmark.

### Phase 3: Memory / Performance Optimization
1. Rewrite data layout toward **Object-of-Arrays / vectorized organization**.
2. Optimize identified hotspots beyond layout changes where justified.
3. Re-benchmark and isolate the impact of each modification.
4. Prioritize performance and explain design tradeoffs.

The guidelines specifically call for rewriting data classes into a vectorized organization and treating performance as the top goal in Phase 3.

## Research Deliverables for This Repository
This system is expected to produce:

- recurring congestion hotspots by link, borough, corridor, and time window,
- average-speed and travel-time summaries by era,
- threshold-based hotspot rankings,
- comparison tables for the three operating periods,
- benchmark data showing the effect of:
  - serial parsing,
  - parallel query execution,
  - memory-layout optimization.

## Planned Query Workloads
Representative query workloads for this repository include:

- all traffic links in a selected borough with average speed below a threshold,
- all records for a selected road link across a date range,
- all links with recurring low-speed conditions during peak hours,
- comparison of congestion metrics for the same corridor across the three operating eras,
- top-N slowest recurring road links by borough or time bucket.

These workloads are intentionally chosen to match the mini’s emphasis on **data reading and range searching**.

## Planned Data Model
Initial object-oriented layout may include components such as:

- `TrafficRecord`
- `TrafficLink`
- `TrafficDataset`
- `TimeWindow`
- `CongestionAnalyzer`
- `BenchmarkHarness`

Supporting infrastructure layers may include:

- `GarageRecord`
- `BuildingRecord`
- `GeoPoint`

In Phase 3, these classes may be partially or fully transformed into a more vectorized representation for improved cache behavior and throughput.

## Benchmarking Plan

### Hardware and OS details
- `TBD`

### Compiler and flags
- `TBD`

### Dataset version/date
- DOT Traffic Speeds NBE snapshot or export date: `TBD`
- Supporting dataset snapshot dates: `TBD`

### Workload / query scenarios
- serial ingest of full traffic dataset
- serial congestion-threshold scans
- serial road-link range queries
- parallelized scans and filtered aggregations
- optimized object-of-arrays scanning and comparison

### Metrics captured
- runtime:
  - mean
  - median
  - standard deviation
- memory usage:
  - peak RSS
- throughput:
  - records/sec
- optional:
  - speedup vs. baseline
  - scaling efficiency by thread count

### Runs per experiment
- `>= 10`

The assignment expects repeated runs and average-based benchmarking rather than single measurements.

## Tooling and Language Constraints

### Required
- C/C++
- Compiler:
  - `gcc/g++` v13+ or
  - `clang/clang++` v16+ (non-Apple Xcode toolchain)
- `cmake`

### Allowed (as needed)
- `boost`
- Python 3 for graph generation
- CSV parsing tools
- graphing / plotting tools

### Not Allowed
- third-party libraries or services beyond listed allowances,
- databases,
- workarounds that bypass the learning intent.

These limits are stated in the mini handout.

## Deliverables
1. Code
2. Report containing:
   - results,
   - supporting data,
   - conclusions,
   - citations/references,
   - failed attempts,
   - individual contributions
3. Exactly one-slide presentation focused on a single important finding
4. Submission package as `.tar.gz` with no test data included

The assignment requires all three components and emphasizes that the one-slide presentation must communicate a single meaningful finding rather than act as a project summary.

## Repository Starter Structure

```text
.
├── CMakeLists.txt
├── include/
│   ├── data_model/
│   ├── io/
│   ├── query/
│   └── benchmark/
├── src/
│   ├── data_model/
│   ├── io/
│   ├── query/
│   └── benchmark/
├── apps/
│   ├── run_serial.cpp
│   ├── run_parallel.cpp
│   └── run_optimized.cpp
├── scripts/
│   ├── benchmark.sh
│   └── plot_results.py
├── results/
│   ├── raw/
│   ├── tables/
│   └── graphs/
└── report/
    ├── notes.md
    └── citations.md

### Phase 1: Serial Baseline (No Threads)
1. Implement C/C++ ingestion and query APIs for the selected datasets.
2. Use object-oriented design with abstractions intended for long-term reuse.
3. Represent fields using appropriate primitive types.
4. Provide API-level capabilities for:
   - data reading/parsing,
   - time-range filtering,
   - road-link range searching,
   - congestion-threshold filtering.
5. Benchmark serial performance with **10+ runs** and record averages.

The mini explicitly requires serial processing for Phase 1 and treats this baseline as the foundation for later comparison.

### Phase 2: Parallelization
1. Parallelize relevant processing paths using OpenMP or threading.
2. Re-run the benchmark harness on the same workloads.
3. Compare speedup and tradeoffs against the serial baseline.

This follows the mini’s Phase 2 requirement to parallelize and compare results against the original benchmark.

### Phase 3: Memory / Performance Optimization
1. Rewrite data layout toward **Object-of-Arrays / vectorized organization**.
2. Optimize identified hotspots beyond layout changes where justified.
3. Re-benchmark and isolate the impact of each modification.
4. Prioritize performance and explain design tradeoffs.

The guidelines specifically call for rewriting data classes into a vectorized organization and treating performance as the top goal in Phase 3.

## Research Deliverables for This Repository
This system is expected to produce:

- recurring congestion hotspots by link, borough, corridor, and time window,
- average-speed and travel-time summaries by era,
- threshold-based hotspot rankings,
- comparison tables for the three operating periods,
- benchmark data showing the effect of:
  - serial parsing,
  - parallel query execution,
  - memory-layout optimization.

## Planned Query Workloads
Representative query workloads for this repository include:

- all traffic links in a selected borough with average speed below a threshold,
- all records for a selected road link across a date range,
- all links with recurring low-speed conditions during peak hours,
- comparison of congestion metrics for the same corridor across the three operating eras,
- top-N slowest recurring road links by borough or time bucket.

These workloads are intentionally chosen to match the mini’s emphasis on **data reading and range searching**.

## Planned Data Model
Initial object-oriented layout may include components such as:

- `TrafficRecord`
- `TrafficLink`
- `TrafficDataset`
- `TimeWindow`
- `CongestionAnalyzer`
- `BenchmarkHarness`

Supporting infrastructure layers may include:

- `GarageRecord`
- `BuildingRecord`
- `GeoPoint`

In Phase 3, these classes may be partially or fully transformed into a more vectorized representation for improved cache behavior and throughput.

## Benchmarking Plan

### Hardware and OS details
- `TBD`

### Compiler and flags
- `TBD`

### Dataset version/date
- DOT Traffic Speeds NBE snapshot or export date: `TBD`
- Supporting dataset snapshot dates: `TBD`

### Workload / query scenarios
- serial ingest of full traffic dataset
- serial congestion-threshold scans
- serial road-link range queries
- parallelized scans and filtered aggregations
- optimized object-of-arrays scanning and comparison

### Metrics captured
- runtime:
  - mean
  - median
  - standard deviation
- memory usage:
  - peak RSS
- throughput:
  - records/sec
- optional:
  - speedup vs. baseline
  - scaling efficiency by thread count

### Runs per experiment
- `>= 10`

The assignment expects repeated runs and average-based benchmarking rather than single measurements.

## Tooling and Language Constraints

### Required
- C/C++
- Compiler:
  - `gcc/g++` v13+ or
  - `clang/clang++` v16+ (non-Apple Xcode toolchain)
- `cmake`

### Allowed (as needed)
- `boost`
- Python 3 for graph generation
- CSV parsing tools
- graphing / plotting tools

### Not Allowed
- third-party libraries or services beyond listed allowances,
- databases,
- workarounds that bypass the learning intent.


## Deliverables
1. Code
2. Report containing:
   - results,
   - supporting data,
   - conclusions,
   - citations/references,
   - failed attempts,
   - individual contributions
3. Exactly one-slide presentation focused on a single important finding
4. Submission package as `.tar.gz` with no test data included

The assignment requires all three components and emphasizes that the one-slide presentation must communicate a single meaningful finding rather than act as a project summary.

## Repository Starter Structure

```text
.
├── CMakeLists.txt
├── include/
│   ├── data_model/
│   ├── io/
│   ├── query/
│   └── benchmark/
├── src/
│   ├── data_model/
│   ├── io/
│   ├── query/
│   └── benchmark/
├── apps/
│   ├── run_serial.cpp
│   ├── run_parallel.cpp
│   └── run_optimized.cpp
├── scripts/
│   ├── benchmark.sh
│   └── plot_results.py
├── results/
│   ├── raw/
│   ├── tables/
│   └── graphs/
└── report/
    ├── notes.md
    └── citations.md