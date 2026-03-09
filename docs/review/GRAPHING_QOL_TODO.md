# Graphing QoL TODO (Phase 1 / 2 / 3)

Purpose: improve graph completeness, visual quality, and interpretability across all plotting scripts.

## 1) Cross-Phase Graphing Standards
- [x] Define one shared chart style module (fonts, palette, line widths, grid style, DPI, figure sizes).
- [ ] Use consistent color semantics across phases:
  - serial = one fixed color
  - parallel = one fixed color family
  - optimized = one fixed color family
  - baseline/dev variants = consistent shades
- [x] Add chart subtitle template to every graph:
  - dataset scope
  - scenario/query
  - run count
  - thread policy
  - artifact timestamp
- [ ] Require explicit axis labels with units on every chart.
- [x] Add a short “What this graph tests” annotation block on every chart.
- [ ] Add legend titles where multiple modes/threads are shown.
- [ ] Add error bars or variability bands (stddev/CI) where repeated runs exist.
- [ ] Add automatic warnings on charts when required comparison groups are missing.
- [x] Add metadata footer text on each chart:
  - summary source CSV path
  - script name/version
  - generated UTC timestamp

## 2) Data Completeness Rules
- [ ] Validate summary inputs before plotting:
  - required columns present
  - expected scenario names present
  - expected subset labels present
  - expected mode/thread combinations present
- [ ] Fail fast with actionable error messages when required data is missing.
- [ ] Add a `--strict` mode that refuses partial chart generation.
- [ ] Add a `--allow-partial` mode that generates partial charts with visible “partial data” watermark.
- [ ] Add consistency checks to prevent plotting mixed dev/baseline batches unless explicitly requested.

## 3) Phase 1 Plot Improvements (`scripts/plot_phase1_dev.py`)
- [ ] Add per-scenario runtime chart with variability (error bars from summary stddev columns).
- [ ] Add throughput chart (records/sec) by subset and scenario.
- [ ] Add ingest vs query stacked view to explain total runtime composition.
- [ ] Add chart notes clarifying that ingest-only and query scenarios answer different questions.
- [x] Improve output naming convention with batch timestamp and summary source token.

### Incremental Progress (2026-03-09)
- Added shared plotting helper module: `scripts/plot_common.py`.
- Refactored Phase 1 plotting to include:
  - subtitle context
  - “What this tests” annotation block
  - metadata footer text (source + generated UTC)
  - dual-output export (`.png` + `.svg`)
  - chart manifest output (`chart_manifest.csv`)
  - strict vs partial data handling (`--allow-partial`)
- Refactored Phase 2/3 plotting to include:
  - subtitle context
  - “What this tests” annotation block
  - metadata footer text (source + generated UTC)
  - strict vs partial data handling (`--allow-partial`)
  - chart manifest output (`chart_manifest.csv`)

## 4) Phase 2 Plot Improvements (`scripts/plot_phase2_dev.py`)
- [x] Add default all-thread comparison charts for every scenario and subset.
- [x] Add dedicated speedup charts that clearly define baseline:
  - speedup = serial query mean / candidate query mean
- [x] Add scalability chart (runtime vs thread count) with monotonicity indicators.
- [ ] Add parity/validation summary plot:
  - pass/fail counts by scenario/thread
- [ ] Add overhead plot comparing:
  - 1-thread parallel vs serial
  - N-thread parallel vs serial

## 5) Phase 3 Plot Improvements (`scripts/plot_phase3_dev.py`)
- [ ] Add stage-attribution chart showing incremental gains:
  - serial
  - parallel
  - optimized serial
  - optimized parallel
- [ ] Add memory vs runtime tradeoff chart using memory probe outputs.
- [x] Add per-thread optimized speedup charts with clear baseline definition.
- [ ] Add large-subset focused plots and all-subset overview plots.
- [ ] Add “validation enabled vs disabled” timing comparison where data is available.

## 6) Visual Design and Readability
- [ ] Improve typography and spacing for report-ready legibility.
- [ ] Standardize tick formatting for time units (ms/sec) and large counts.
- [ ] Prevent label overlap (rotation/wrapping/truncation policy).
- [ ] Add optional dark-on-light and print-friendly themes.
- [ ] Ensure colorblind-safe palette and distinguishability in grayscale exports.

## 7) Output Formats and Report Integration
- [ ] Emit both SVG and PNG for each chart by default.
- [x] Add a machine-readable chart manifest:
  - chart file
  - source summary file
  - scenario/mode/thread filters
  - generation timestamp
- [ ] Add an auto-generated markdown index linking all produced charts with one-line descriptions.
- [ ] Add script option to export only selected scenario/subset/thread slices.

## 8) Testing and CI Guardrails
- [ ] Add unit tests for plotting data-validation helpers.
- [ ] Add smoke test that runs each plot script on a tiny fixture summary.
- [ ] Add regression test to ensure required chart files are produced per phase.
- [ ] Add CI check that fails if chart generation silently skips expected scenarios.
- [ ] Add CI check to verify axis labels and titles are non-empty in generated SVG.

## 9) Rollout Plan
- [x] Step 1: extract shared chart utilities and style config.
- [x] Step 2: refactor Phase 1 plot script to new utilities.
- [x] Step 3: refactor Phase 2 plot script, then validate against latest manifests.
- [x] Step 4: refactor Phase 3 plot script and memory/tradeoff visuals.
- [ ] Step 5: add chart manifest/index generation and CI checks.

## 10) Acceptance Criteria
- [ ] Every chart states what is being tested.
- [ ] Every chart has explicit x-axis and y-axis labels with units.
- [ ] Every repeated-run chart shows variability.
- [ ] Every phase has complete chart coverage for declared scenarios and thread policies.
- [ ] Graph outputs are report-ready without manual annotation.
