#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RUNS=3
THREADS="1,2,4,8,16"
MEMORY_THREADS=16
CLEAN_FIRST=0

usage() {
  cat <<'USAGE'
Usage: scripts/run_all_phase_dev_benchmarks.sh [options]

Unified dev benchmark pipeline for Phase 1/2/3.

Options:
  --runs <n>          Benchmark repetitions per scenario (default: 3)
  --threads <csv>     Thread list for phase2/phase3 (default: 1,2,4,8,16)
  --memory-threads <n> Thread count for phase3 memory probe (default: 16)
  --clean-first       Run scripts/clean_benchmark_outputs.sh before benchmarking
  --help              Show this help
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs)
      RUNS="$2"
      shift 2
      ;;
    --threads)
      THREADS="$2"
      shift 2
      ;;
    --memory-threads)
      MEMORY_THREADS="$2"
      shift 2
      ;;
    --clean-first)
      CLEAN_FIRST=1
      shift
      ;;
    --help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 2
      ;;
  esac
done

if ! python3 -c "import matplotlib" >/dev/null 2>&1; then
  echo "[all-phases] missing required Python package: matplotlib" >&2
  echo "Install with: python3 -m pip install matplotlib" >&2
  exit 2
fi

if [[ "${CLEAN_FIRST}" == "1" ]]; then
  "${ROOT_DIR}/scripts/clean_benchmark_outputs.sh"
fi

echo "[all-phases] Phase 1 benchmarks"
"${ROOT_DIR}/scripts/run_phase1_dev_benchmarks.sh" --runs "${RUNS}"
python3 "${ROOT_DIR}/scripts/summarize_phase1_dev.py" \
  --input-dir "${ROOT_DIR}/results/raw/phase1_dev" \
  --output-dir "${ROOT_DIR}/results/tables/phase1_dev" \
  --output-file "phase1_dev_summary_latest.csv"
python3 "${ROOT_DIR}/scripts/plot_phase1_dev.py" \
  --summary-csv "${ROOT_DIR}/results/tables/phase1_dev/phase1_dev_summary_latest.csv" \
  --output-dir "${ROOT_DIR}/results/graphs/phase1_dev"

echo "[all-phases] Phase 2 benchmarks"
"${ROOT_DIR}/scripts/run_phase2_dev_benchmarks.sh" --runs "${RUNS}" --threads "${THREADS}"
PH2_MANIFEST="$(find "${ROOT_DIR}/results/raw/phase2_dev" -maxdepth 1 -type f -name 'batch_*_manifest.csv' | sort | tail -n 1)"
if [[ -z "${PH2_MANIFEST}" ]]; then
  echo "[all-phases] failed to locate latest phase2 manifest" >&2
  exit 1
fi
python3 "${ROOT_DIR}/scripts/summarize_phase2_dev.py" \
  --manifest "${PH2_MANIFEST}" \
  --output-dir "${ROOT_DIR}/results/tables/phase2_dev" \
  --output-file "phase2_dev_summary_$(basename "${PH2_MANIFEST}")"
python3 "${ROOT_DIR}/scripts/plot_phase2_dev.py" \
  --summary-csv "${ROOT_DIR}/results/tables/phase2_dev/phase2_dev_summary_$(basename "${PH2_MANIFEST}")" \
  --output-dir "${ROOT_DIR}/results/graphs/phase2_dev" \
  --parallel-thread 4 \
  --parallel-threads "${THREADS}"

echo "[all-phases] Phase 3 benchmarks"
"${ROOT_DIR}/scripts/run_phase3_dev_benchmarks.sh" --runs "${RUNS}" --threads "${THREADS}" --no-validate-serial
PH3_MANIFEST="$(find "${ROOT_DIR}/results/raw/phase3_dev" -maxdepth 1 -type f -name 'batch_*_manifest.csv' | sort | tail -n 1)"
if [[ -z "${PH3_MANIFEST}" ]]; then
  echo "[all-phases] failed to locate latest phase3 manifest" >&2
  exit 1
fi
BATCH_UTC="$(basename "${PH3_MANIFEST}" | sed -E 's/^batch_([0-9TZ]+)_manifest\.csv$/\1/')"
"${ROOT_DIR}/scripts/run_phase3_memory_probe.sh" \
  --threads "${MEMORY_THREADS}" \
  --batch-utc "${BATCH_UTC}" \
  --optimization-step soa_encoded_hotloop
PH3_MEMORY="${ROOT_DIR}/results/raw/phase3_dev/memory/memory_probe_${BATCH_UTC}.csv"

"${ROOT_DIR}/scripts/run_phase3_dev_reporting.sh" \
  --manifest "${PH3_MANIFEST}" \
  --memory-csv "${PH3_MEMORY}" \
  --parallel-thread 4 \
  --optimized-thread 4

echo "[all-phases] complete"
echo "[all-phases] phase2_manifest=${PH2_MANIFEST}"
echo "[all-phases] phase3_manifest=${PH3_MANIFEST}"
echo "[all-phases] phase3_memory=${PH3_MEMORY}"
