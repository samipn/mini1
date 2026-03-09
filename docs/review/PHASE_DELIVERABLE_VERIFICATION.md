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
  - Phase 3 D1-D15 reviewed

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

Previously identified CLI numeric-parse abort issues and Phase 1 benchmark output overwrite risk have been fixed on this branch. Remaining gaps are tracked in `docs/review/THOROUGH_CODE_REVIEW_TODO.md`.
