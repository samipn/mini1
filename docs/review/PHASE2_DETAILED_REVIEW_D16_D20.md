# Phase 2 Detailed Review (D16-D20)

Date: 2026-03-09 (UTC)
Scope: Phase 2 Deliverables 16-20 only.

Reviewed deliverables:
- D16 Deferred-from-Phase-1 additive index/materialization + Boost evaluation tasks
- D17 Reuse/validate subset datasets
- D18 Serial-vs-parallel validation runs on subsets
- D19 Early parallel benchmark scenarios on subsets
- D20 Repeatable Phase 2 dev benchmark runner

## Findings (ordered by severity)

1. High: subset validation script is schema-fragile due fixed-column extraction.
   - Behavior: reads specific CSV fields by positional index (`$19,$21,$22,$6`) instead of header names.
   - Impact: D18 validation can silently break when benchmark CSV schema changes.
   - Evidence:
     - `scripts/run_phase2_subset_validation.sh:188`
     - `scripts/run_phase2_subset_validation.sh:208`

2. Medium: `run_index_experiments` uses exception-throwing numeric parse for `--repeats`.
   - Behavior: `std::stoull` is used without guard; malformed input can abort process.
   - Impact: D16 experimental CLI lacks hardened failure semantics.
   - Evidence:
     - `apps/run_index_experiments.cpp:107`

3. Medium: D16 additive outputs default to non-dev path naming (`results/raw/phase2`) while dev runner uses `phase2_dev`.
   - Behavior: index experiment default output differs from dev/pre-baseline naming convention.
   - Impact: artifact organization is less consistent for audit workflows.
   - Evidence:
     - `apps/run_index_experiments.cpp:96`
     - `scripts/run_phase2_index_experiments.sh:12`

## Memory / Leak Notes (D16-D20 scope)

- No leaks detected in index/materialization paths.
- Main risk is operational robustness and artifact consistency.

## Recommended Remediation (D16-D20)

1. Convert D18 extraction logic to header-based CSV parsing.
2. Harden `run_index_experiments` numeric parsing (`--repeats`) to return usage errors.
3. Align index experiment default output path with explicit dev/baseline separation policy.
