# Memory Overload Mini Project Template

## Project Context
This repository is one of two related mini assignments for **Memory Overload (v0.1a)**.

- Both assignments use the **same NYC OpenData dataset selection**.
- Each assignment targets a **different research area**.
- The exact research focus for this repository is still being finalized.

## Assignment Focus (Fill In)
- Assignment name/code: `TBD`
- Primary research area: `TBD`
- Secondary research area (optional): `TBD`
- Hypothesis to test: `TBD`

## Core Requirements from Guidelines

### Dataset
- Source: NYC OpenData (<https://opendata.cityofnewyork.us>)
- Combined dataset size must be **> 12 GB**
- Combined records must be **> 2 million**
- You may combine multiple datasets to meet requirements
- Do not use:
  - 2018 Squirrel Census (insufficient update/continuity for requirement intent)
  - Moving collisions/violations dataset (explicitly disallowed)

### Phase 1: Serial Baseline (No Threads)
1. Implement C/C++ ingestion and query APIs for selected datasets.
2. Use object-oriented design with abstractions for long-term reuse.
3. Represent fields using appropriate primitive types.
4. Provide API-level capabilities for:
   - Data reading/parsing
   - Basic range searching
5. Benchmark serial performance with **10+ runs** and record averages.

### Phase 2: Parallelization
1. Parallelize relevant parts (OpenMP or threading).
2. Re-run benchmark harness.
3. Compare against Phase 1 baseline.

### Phase 3: Memory/Performance Optimization
1. Rewrite data layout toward **Object-of-Arrays (vectorized organization)**.
2. Optimize hotspots beyond data layout where justified.
3. Re-benchmark and measure impact of each change.
4. Prioritize performance and explain tradeoffs.

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
- CSV parsing tools and graphing/plotting tools

### Not Allowed
- Third-party libraries/services beyond listed allowances
- Databases
- Workarounds that bypass the learning intent

## Deliverables
1. Code
2. Report (results, data, conclusions, citations/references, failed attempts, team contributions)
3. Exactly one-slide presentation (single key finding, not a project summary)
4. Submission package as `.tar.gz` (exclude test data)

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
```

## Benchmarking Plan Template
- Hardware and OS details: `TBD`
- Compiler and flags: `TBD`
- Dataset version/date: `TBD`
- Workload/query scenarios: `TBD`
- Metrics captured:
  - runtime (mean, median, stddev)
  - memory usage (peak RSS)
  - throughput (records/sec)
- Runs per experiment: `>= 10`

## Research Log (Keep Failed Attempts)
| Date | Phase | Change | Expected Outcome | Actual Outcome | Notes |
|---|---|---|---|---|---|
| TBD | TBD | TBD | TBD | TBD | TBD |

## Report Checklist
- [ ] Problem framing and research objective
- [ ] Dataset rationale and constraints compliance
- [ ] Phase-by-phase design and implementation details
- [ ] Benchmark tables and plots
- [ ] Comparison and analysis across phases
- [ ] Failed attempts and what they revealed
- [ ] Conclusions and recommendations
- [ ] Citations and references
- [ ] Individual contributions

## Open Decisions
- Final dataset combination and acquisition process
- Query set used to evaluate designs
- Distinct research focus split between Assignment 1 and Assignment 2
- Build/benchmark automation level
