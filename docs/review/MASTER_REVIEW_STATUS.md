# Master Review Status

Date: 2026-03-09 (UTC)
Source of truth: `docs/review/THOROUGH_CODE_REVIEW_TODO.md`

## Snapshot
- Total tracked review items: `121`
- Completed: `104`
- Remaining: `17`

## Completed
- Phase 1 deep review and multi-chunk fixes completed.
- Phase 2 deep review and multi-chunk fixes completed.
- Phase 3 deep review and chunked fixes completed through D29-related gaps.
- Review evidence chain strengthened with:
  - deterministic CSV/header parsing guards
  - benchmark run-policy gates
  - reporting freshness and artifact presence checks
  - baseline/dev workflow guardrails
  - memory-to-summary linkage and memory plotting keying

## Remaining (from THOROUGH review TODO)
- Keep heavy benchmark execution deferred until review-pass instrumentation fixes are complete.
- Re-run Phase 3 dev benchmarks without `--validate-serial` and regenerate summary/graphs.
- Keep a separate validation batch for parity checks only.
- Re-check optimized query scaling (1/2/4/8/16 threads) after timing-scope fix.
- Add thread-scaling plots for serial/parallel/optimized in one chart family per scenario.
- Add test case that fails if optimized `query_ms` unexpectedly includes serial validation ingest overhead.
- Align `docs/TODO_PHASE3.md` checkboxes with measured evidence only.
- Separate “implemented” vs “fully benchmark-validated” in Phase 3 TODO wording.
- Confirm each claimed deliverable has a corresponding source file and artifact.
- Reconcile checked path tokens in phase TODO docs with actual repo snapshot evidence.
- Add per-query profiler integration option (e.g., perf/callgrind wrapper script).
- Add confidence intervals / variability bands in summary tables.
- Add regression guardrails in CI for benchmark schema and artifact completeness.
- P0: timing-scope fix in `BenchmarkHarness` + benchmark rerun + graph regeneration.
- P1: TODO/docs claim alignment and explicit benchmark methodology split.
- P2: memory graph support + richer statistical summaries.
- P2 maintainability: deduplicate CLI numeric parsing helpers.

## Recommended Next Ordering
1. Close the timing-scope measurement integrity item (`P0`) and rerun fresh benchmark batches.
2. Then update TODO/docs claim alignment (`P1`) to prevent evidence drift.
3. Finally address statistics/profiler/CI hardening (`P2`) as follow-on quality improvements.
