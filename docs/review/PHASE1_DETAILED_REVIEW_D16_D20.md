# Phase 1 Detailed Review (D16-D20)

Date: 2026-03-09 (UTC)
Scope: Phase 1 Deliverables 16-20 only.

Reviewed deliverables:
- D16 Development summary tables
- D17 Development graphs
- D18 Phase 1 development benchmark log
- D19 Correctness cross-check across subsets
- D20 Pre-baseline-only labeling/separation

## Findings (ordered by severity)

1. High: D16 summary script defaults do not work against current Phase 1 dev artifacts.
   - Behavior: `summarize_phase1_dev.py` defaults to labels `small`, `medium`, `large_dev`, but current raw artifacts use `small_smoke`; default invocation exits with "No matching rows found".
   - Impact: default D16 workflow is fragile and can fail immediately on the current snapshot.
   - Evidence:
     - `scripts/summarize_phase1_dev.py:40`
     - `results/raw/phase1_dev/small_smoke_ingest_only.csv`
     - smoke check: `python3 scripts/summarize_phase1_dev.py --input-dir results/raw/phase1_dev ...` -> `No matching rows found after applying label filters.`

2. High: D17 plotting workflow is brittle relative to current labels/scenarios and environment.
   - Behavior: plotting script expects `dataset_label` values `small/medium/large_dev` and a fixed scenario set; with current snapshot labels it fails unless summary inputs are manually normalized. Also graph generation fails without local `matplotlib` install.
   - Impact: D17 "single-command" reproducibility is not currently robust.
   - Evidence:
     - `scripts/plot_phase1_dev.py:54`
     - `scripts/plot_phase1_dev.py:59`
     - `scripts/plot_phase1_dev.py:79`
     - smoke check: `python3 scripts/plot_phase1_dev.py ...` -> `matplotlib is required to generate graphs.`

3. Medium: D18 benchmark log is stale relative to latest Phase 1 dev artifacts/scenario set.
   - Behavior: `report/phase1_dev_benchmark_log.md` records older `8mph` / `top10` scenario names and does not include the latest local batch (`batch_20260309T061031Z_manifest.csv`).
   - Impact: traceability is incomplete for current artifact state.
   - Evidence:
     - `report/phase1_dev_benchmark_log.md:16`
     - `report/phase1_dev_benchmark_log.md:42`
     - `results/raw/phase1_dev/batch_20260309T061031Z_manifest.csv`
     - `configs/phase1_dev_scenarios.csv:3`

4. Medium: D19 determinism checker validates counts but not top-N payload/order stability.
   - Behavior: cross-check currently asserts result-count/ingest aggregate consistency; it does not compare top-N result members/order across runs.
   - Impact: nondeterministic ordering/content regressions in top-N outputs can evade D19 checks.
   - Evidence:
     - `scripts/check_phase1_dev_determinism.py:154`
     - `scripts/check_phase1_dev_determinism.py:173`

5. Low: D20 pre-baseline separation is documented, but not strongly enforced by script defaults.
   - Behavior: `report/notes.md` clearly marks `phase1_dev` as pre-baseline and reserves `phase1_baseline` paths, but broad benchmark entrypoint defaults to `results/raw` and mixes serial/parallel/optimized outputs.
   - Impact: operator error risk remains for output-path hygiene.
   - Evidence:
     - `report/notes.md:304`
     - `report/notes.md:316`
     - `scripts/benchmark.sh:12`
     - `scripts/benchmark.sh:47`

## Memory / Leak Notes (D16-D20 scope)

- No memory-leak patterns found in D16-D20 scope (scripts/docs/artifact processing).
- Main risks are workflow robustness, stale evidence, and reproducibility drift.

## Recommended Remediation (D16-D20)

1. Make D16 summary tooling robust to observed dataset label variants (or enforce canonical labels in benchmark runner output).
2. Make D17 plotting consume discovered labels/scenarios from summary CSV instead of hardcoded values, and document/install plotting dependency path.
3. Append fresh D18 benchmark-log entries for the latest local batch and scenario config.
4. Extend D19 determinism checks to validate top-N membership/order (or explicitly define/tolerate ordering policy).
5. Add stricter path/mode guards so D20 pre-baseline separation is operationally hard to violate.
