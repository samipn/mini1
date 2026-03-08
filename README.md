# UrbanDrop Mini 1A — Congestion Intelligence Engine

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

| Field | Value |
|---|---|
| **Name** | Transportation DOT Traffic Speeds NBE |
| **Source** | NYC OpenData / NYCDOT |
| **Approximate size** | ~40 GB |
| **Key fields** | Link ID, average speed, travel time, timestamp, borough, link description |
| **Coverage** | Major arterials and highways within NYC limits |
| **Role** | Primary traffic-performance dataset; source for congestion detection, temporal comparisons, and range-query benchmarks |

### Supporting Datasets

| Field | Active DCA-Licensed Garages and Parking Lots | Building Elevation and Subgrade (BES) |
|---|---|---|
| **Source** | NYC OpenData | NYC OpenData |
| **Key fields** | Address, borough, license details, latitude/longitude | Building elevation, subgrade classification |
| **Role** | Infrastructure layer for comparison against congestion hotspots | Shared supporting layer for UrbanDrop project |
| **URL** | [about_data](https://data.cityofnewyork.us/Business/Active-DCA-Licensed-Garages-and-Parking-Lots/a7m8-iids/about_data) | [about_data](https://data.cityofnewyork.us/City-Government/Building-Elevation-and-Subgrade-BES-/bsin-59hv/about_data) |

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
