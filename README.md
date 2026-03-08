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

These assignment requirements come directly from the mini guidelines.
