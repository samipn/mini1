# Phase Deliverable Verification

Date: 2026-03-09 (UTC)
Branch: `P3-R1-INGEST-TEST-GRAPH`

## Scope

This verification pass focuses on objective evidence for Phase 1/2/3 TODO claims:
- source artifacts exist (apps/scripts)
- expected result artifact paths exist
- checked TODO path tokens map to real files/directories

Evidence sources:
- `docs/review/PHASE_DELIVERABLE_AUDIT.md`
- `docs/review/TODO_EVIDENCE_AUDIT.md`

## Current Status

- Phase 1: pass (`6/6` deliverable checks, `16/16` TODO path tokens)
- Phase 2: pass (`7/7` deliverable checks, `13/13` TODO path tokens)
- Phase 3: pass (`10/10` deliverable checks, `14/14` TODO path tokens)
- Per-deliverable review backlog status:
  - Phase 1 D1-D10 reviewed (with open operational hardening gaps)
  - Phase 2 D1-D15 reviewed (with open operational consistency gaps)
  - Phase 3 reviewed in chunks:
    - D1-D5 reviewed (open correctness/robustness gaps identified)
    - D6-D10 reviewed (open performance/measurement hardening gaps identified)
    - D11-D15 reviewed (open attribution/evidence-integrity gaps identified)
    - D18-D24 reviewed (open validation robustness and artifact-freshness gaps identified)
    - D25-D29 reviewed (open graph evidence/log freshness/attribution rigor gaps identified)

## Fixes Applied During Verification

1. Restored missing Phase 1 scenario config:
   - `configs/phase1_dev_scenarios.csv`
2. Fixed Phase 1 benchmark script reliability:
   - `scripts/run_phase1_dev_benchmarks.sh`
   - RPS extraction now uses CSV headers (not hardcoded column numbers).
3. Fixed `.gitignore` conflict that blocked config CSV tracking:
   - Added `!configs/*.csv`.
4. Added proper CLI help and option parsing for memory probe script:
   - `scripts/run_phase3_memory_probe.sh`
5. Added reusable audit tooling:
   - `scripts/audit_phase_deliverables.py`
   - `scripts/audit_todo_evidence.py`
6. Fixed D3 ingestion correctness hardening issues:
   - `CSVReader` now resets target datasets per load (row + optimized).
   - `BuildingLoader` now resets `LoaderStats` per load.
   - Added regression coverage for repeated-load semantics.

## Above-and-Beyond Items in This Pass

- Added cross-phase, repeatable evidence audit automation rather than manual checklist review.
- Added phase baseline separation support (`phase3_baseline`) and documented execution plan.
- Added environment snapshot capture in benchmark runners for reproducibility.
- Hardened benchmark scripts against schema drift with header-based CSV parsing.

## Caveat

Some evidence artifacts are generated under ignored directories (for example `results/raw/...`).
They exist locally for verification but are not committed to git by design.

## Open Hardening Gaps Discovered After Verification

1. Shared CSV/parse helper duplication across `CSVReader`, `GarageLoader`, `BuildingLoader` (maintainability risk).
2. `run_parallel` default query behavior is operationally inconsistent (`speed_below` default but no default threshold).
3. `run_parallel` allows `0` in explicit `--thread-list` values, causing ambiguous `threads=0` reporting.
4. `run_optimized` row-convert path can silently succeed on unknown query types.
5. `TrafficColumns::AddRecord` exception-safety risk can desynchronize SoA columns under allocation failure.
6. `OptimizedTrafficAggregator` hot-loop path uses `std::function` predicate dispatch in scan loops.
7. `run_optimized_support_experiments` aborts on malformed numeric `--repeats` input.
8. Phase 3 memory probe currently swallows command failures and can emit ambiguous rows.
9. Phase 3 optimization-attribution workflow is label-based and does not enforce full step-isolated campaign automation.
10. Phase 3 dev benchmark runner does not enforce deliverable-grade run count (`>=10`), allowing underpowered batches.
11. Benchmark harness comparison CSVs do not currently carry memory metrics directly.
12. Phase 3 benchmark notes/log metadata can drift from current thread policy and newest artifacts.
13. Phase 3 subset-validation script uses fixed benchmark CSV column positions, creating schema-fragile checks.
14. Phase 3 scenario set currently omits top-N validation even though top-N query mode exists.
15. Phase 3 summary/graph artifacts can lag behind newest raw batch manifests without detection.
16. Phase 3 memory probe can emit invalid-run rows that appear usable (for example malformed command path cases).
17. Phase 3 graph artifact presence is currently inconsistent with TODO completion claims.

Previously identified CLI numeric-parse abort issues and Phase 1 benchmark output overwrite risk have been fixed on this branch. Remaining gaps are tracked in `docs/review/THOROUGH_CODE_REVIEW_TODO.md`.
