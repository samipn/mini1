# Phase 3 Baseline Plan

Purpose: keep final, report-grade Phase 3 benchmark artifacts separate from development trial runs.

## Directory Layout

- Raw benchmark CSV/log outputs:
  - `results/raw/phase3_baseline/`
- Summary tables:
  - `results/tables/phase3_baseline/`
- Graph outputs:
  - `results/graphs/phase3_baseline/`

`phase3_dev` remains for incremental tuning; `phase3_baseline` is for final comparable runs.

## Execution Flow

1. Run baseline batch (performance mode):

```bash
scripts/run_phase3_baseline_benchmarks.sh --no-validate-serial
```

2. Run validation batch (correctness mode, separate):

```bash
scripts/run_phase3_baseline_benchmarks.sh --validate-serial --out-dir results/raw/phase3_baseline_validation
```

3. Summarize:

```bash
python3 scripts/summarize_phase3_dev.py \
  --manifest results/raw/phase3_baseline/batch_<ts>_manifest.csv \
  --output-dir results/tables/phase3_baseline \
  --output-file phase3_baseline_summary_<ts>.csv
```

4. Plot:

```bash
python3 scripts/plot_phase3_dev.py \
  --summary-csv results/tables/phase3_baseline/phase3_baseline_summary_<ts>.csv \
  --output-dir results/graphs/phase3_baseline
```

5. Validate completeness:

```bash
python3 scripts/check_phase3_dev_completeness.py \
  --manifest results/raw/phase3_baseline/batch_<ts>_manifest.csv
```

