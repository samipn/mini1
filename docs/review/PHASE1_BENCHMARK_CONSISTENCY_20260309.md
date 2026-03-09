# Phase 1 Benchmark Consistency Analysis (2026-03-09)

## Inputs
- Dev manifest: `/home/var/mini1/results/raw/phase1_dev/batch_20260309T080611Z_manifest.csv`
- Baseline manifest: `/home/var/mini1/results/raw/phase1_baseline/batch_20260309T080652Z_manifest.csv`
- Determinism report: `/home/var/mini1/results/raw/correctness/determinism_report_20260309T080652Z.md`

## Determinism
- Status: `PASS` across all subset/scenario combinations (`runs=5`).
- Result counts and ingest counters were stable.
- `top_n_slowest` payload/order signatures were stable on small/medium/large subsets.

## Timing Consistency Summary

### Dev batch (`runs=3`)
- Groups analyzed: `18`
- Median total-time CV: `3.501%`
- Median ingest-time CV: `3.472%`
- Groups with total-time CV <= 5%: `14/18`
- Groups with total-time CV <= 10%: `15/18`
- Worst total-time CV: `12.455%` at `large_dev/summary`
- Worst ingest-time CV: `12.459%` at `large_dev/summary`

### Baseline batch (`runs=10`)
- Groups analyzed: `18`
- Median total-time CV: `3.443%`
- Median ingest-time CV: `3.430%`
- Groups with total-time CV <= 5%: `12/18`
- Groups with total-time CV <= 10%: `17/18`
- Worst total-time CV: `10.094%` at `large_dev/ingest_only`
- Worst ingest-time CV: `10.094%` at `large_dev/ingest_only`

## Interpretation
- Functional consistency is strong (`determinism=PASS`).
- Runtime variance is moderate on most groups (median total CV ~3-4%), with a few high-jitter outliers on large subset ingest-heavy runs.
- Very high query CV outliers are on near-zero query-duration scenarios (for example `ingest_only`), where CV is not meaningful due to tiny denominator.

## Recommendation for stricter stability
- Pin CPU governor/performance mode and avoid concurrent background load during runs.
- Keep `runs>=10` for baseline claims and optionally use median-of-means reporting for large subset ingest-heavy scenarios.
