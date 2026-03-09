# Build Matrix Notes

Purpose: make benchmark runs reproducible across compiler/OpenMP configurations without changing the main project README.

## Recommended Matrix

| Compiler | Configure Flags | Expected OpenMP State | Notes |
|---|---|---|---|
| GCC (`g++`) | `-DENABLE_OPENMP=ON -DREQUIRE_OPENMP=ON` | Enabled | Preferred for OpenMP scaling runs. |
| Clang (`clang++`) | `-DENABLE_OPENMP=ON -DREQUIRE_OPENMP=OFF` | Optional | Builds without OpenMP when runtime/toolchain support is missing. |
| Any | `-DENABLE_OPENMP=OFF` | Disabled | Serial-only/reference behavior checks. |

## Canonical Commands

```bash
# GCC + strict OpenMP requirement
cmake -S . -B build-gcc -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++ -DENABLE_OPENMP=ON -DREQUIRE_OPENMP=ON
cmake --build build-gcc -j

# Clang + optional OpenMP
cmake -S . -B build-clang -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ -DENABLE_OPENMP=ON -DREQUIRE_OPENMP=OFF
cmake --build build-clang -j

# Serial-only reference build
cmake -S . -B build-serial -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENMP=OFF
cmake --build build-serial -j
```

## Run Metadata Capture

- `scripts/run_phase2_dev_benchmarks.sh` now writes `batch_<ts>_environment.csv`.
- `scripts/run_phase3_dev_benchmarks.sh` now writes `batch_<ts>_environment.csv`.
- Environment snapshot fields include host/kernel, CPU model, logical cores, memory, compiler/version, branch/commit, thread list, and benchmark knobs.

This file should be kept with raw benchmark CSVs when producing final graphs/tables.
