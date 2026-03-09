# Phase 1 Detailed Review (D11-D15)

Date: 2026-03-09 (UTC)
Scope: Phase 1 Deliverables 11-15 only.

Reviewed deliverables:
- D11 Reproducible subset datasets
- D12 Sample validation runs on subsets
- D13 Early benchmark scenarios for subsets
- D14 Repeatable benchmark runner for subsets
- D15 Preliminary timing data collection

## Findings (ordered by severity)

1. High: D13 scenario definitions have drifted between benchmark and determinism flows.
   - Behavior: `run_phase1_dev_benchmarks.sh` reads scenarios from `configs/phase1_dev_scenarios.csv`, but `check_phase1_dev_determinism.py` hardcodes a different/older scenario set (`speed_below_8mph`, `top10_slowest`, different borough/time-window settings).
   - Impact: subset correctness/determinism checks are no longer validating the same benchmark set that D13 declares as stable.
   - Evidence:
     - `configs/phase1_dev_scenarios.csv:1`
     - `scripts/run_phase1_dev_benchmarks.sh:7`
     - `scripts/check_phase1_dev_determinism.py:20`

2. High: latest Phase 1 subset benchmark batch in snapshot does not satisfy D15 minimum repeats.
   - Behavior: latest manifest and summary rows show smoke-only runs (`benchmark_runs=1`, `runs=1`) instead of at least 3 runs per scenario/subset.
   - Impact: D15 checkbox is stronger than currently available local evidence.
   - Evidence:
     - `docs/TODO_PHASE1.md:476`
     - `results/raw/phase1_dev/batch_20260309T061031Z_manifest.csv`
     - `results/tables/phase1_dev/phase1_dev_summary.csv`

3. Medium: D12 validation artifact chain is currently missing in this workspace snapshot.
   - Behavior: validation runner exists and writes to `results/raw/validation/`, but there are no validation logs present in that directory right now.
   - Impact: ingestion-validation claims are not currently auditable from local artifacts without rerunning the script.
   - Evidence:
     - `scripts/run_subset_validation.sh:8`
     - `docs/TODO_PHASE1.md:402`
     - `results/raw/validation/` (empty in current snapshot)

4. Medium: D14 runner accepts arbitrary `--runs` values without an explicit minimum policy gate.
   - Behavior: script default is `RUNS=3`, but the value is not validated/enforced at script level to remain `>= 3` for deliverable-grade collection.
   - Impact: underpowered batches can be produced accidentally and later mixed into summary artifacts.
   - Evidence:
     - `scripts/run_phase1_dev_benchmarks.sh:12`
     - `scripts/run_phase1_dev_benchmarks.sh:69`
     - `scripts/run_phase1_dev_benchmarks.sh:180`

## Memory / Leak Notes (D11-D15 scope)

- No memory-leak ownership issues found in D11-D15 scope (script-layer workflow only).
- Primary risks are scenario consistency, evidence freshness, and benchmark-policy enforcement.

## Recommended Remediation (D11-D15)

1. Make determinism checker scenario-driven from `configs/phase1_dev_scenarios.csv` (single source of truth).
2. Enforce a minimum repeat-count gate for D15-grade batches (for example `--runs >= 3`) and label smoke batches explicitly.
3. Add a quick artifact-presence guard for D12 (`results/raw/validation/*.log`) in verification/audit scripts.
4. Regenerate and log one fresh D11-D15 evidence batch after policy alignment so checklist status matches repository evidence.
