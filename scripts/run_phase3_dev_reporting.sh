#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MANIFEST=""
TABLES_DIR="${ROOT_DIR}/results/tables/phase3_dev"
GRAPHS_DIR="${ROOT_DIR}/results/graphs/phase3_dev"
PARALLEL_THREAD="4"
OPTIMIZED_THREAD="4"
MEMORY_CSV=""
ALLOW_STALE=0

usage() {
  cat <<'USAGE'
Usage: scripts/run_phase3_dev_reporting.sh --manifest <path> [options]

Run Phase 3 reporting pipeline: completeness check -> summary generation -> plotting -> artifact presence checks.

Options:
  --manifest <path>           Batch manifest CSV from run_phase3_dev_benchmarks.sh (required)
  --tables-dir <path>         Summary output dir (default: results/tables/phase3_dev)
  --graphs-dir <path>         Graph output dir (default: results/graphs/phase3_dev)
  --parallel-thread <n>       Parallel thread for subset comparison (default: 4)
  --optimized-thread <n>      Optimized-parallel thread for subset comparison (default: 4)
  --memory-csv <path>         Optional memory probe CSV to include memory chart
  --allow-stale-manifest      Allow stale manifest in summarization
  --help                      Show this help
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --manifest)
      MANIFEST="$2"
      shift 2
      ;;
    --tables-dir)
      TABLES_DIR="$2"
      shift 2
      ;;
    --graphs-dir)
      GRAPHS_DIR="$2"
      shift 2
      ;;
    --parallel-thread)
      PARALLEL_THREAD="$2"
      shift 2
      ;;
    --optimized-thread)
      OPTIMIZED_THREAD="$2"
      shift 2
      ;;
    --memory-csv)
      MEMORY_CSV="$2"
      shift 2
      ;;
    --allow-stale-manifest)
      ALLOW_STALE=1
      shift 1
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

if [[ -z "${MANIFEST}" ]]; then
  echo "[phase3-report] --manifest is required" >&2
  exit 2
fi
if [[ ! -f "${MANIFEST}" ]]; then
  echo "[phase3-report] manifest not found: ${MANIFEST}" >&2
  exit 1
fi

mkdir -p "${TABLES_DIR}" "${GRAPHS_DIR}"

batch_utc="$(awk -F, 'NR==2 {print $1; exit}' "${MANIFEST}")"
manifest_base="$(basename "${MANIFEST}")"
summary_file="phase3_dev_summary_${manifest_base}"
summary_path="${TABLES_DIR}/${summary_file}"

python3 "${ROOT_DIR}/scripts/check_phase3_dev_completeness.py" --manifest "${MANIFEST}"

summarize_cmd=(
  python3 "${ROOT_DIR}/scripts/summarize_phase3_dev.py"
  --manifest "${MANIFEST}"
  --output-dir "${TABLES_DIR}"
  --output-file "${summary_file}"
)
if [[ "${ALLOW_STALE}" == "1" ]]; then
  summarize_cmd+=(--allow-stale-manifest)
fi
if [[ -n "${MEMORY_CSV}" ]]; then
  summarize_cmd+=(--memory-csv "${MEMORY_CSV}")
fi
"${summarize_cmd[@]}"

plot_cmd=(
  python3 "${ROOT_DIR}/scripts/plot_phase3_dev.py"
  --summary-csv "${summary_path}"
  --output-dir "${GRAPHS_DIR}"
  --parallel-thread "${PARALLEL_THREAD}"
  --optimized-thread "${OPTIMIZED_THREAD}"
)
if [[ -n "${MEMORY_CSV}" ]]; then
  plot_cmd+=(--memory-csv "${MEMORY_CSV}" --memory-batch-utc "${batch_utc}")
fi
"${plot_cmd[@]}"

if [[ ! -f "${GRAPHS_DIR}/chart_manifest.csv" ]]; then
  echo "[phase3-report] missing chart manifest: ${GRAPHS_DIR}/chart_manifest.csv" >&2
  exit 1
fi
if [[ "$(wc -l < "${GRAPHS_DIR}/chart_manifest.csv")" -le 1 ]]; then
  echo "[phase3-report] chart manifest has no chart rows" >&2
  exit 1
fi
if [[ -z "$(find "${GRAPHS_DIR}" -maxdepth 1 -type f -name '*.svg' | head -n 1)" ]]; then
  echo "[phase3-report] no svg charts generated in ${GRAPHS_DIR}" >&2
  exit 1
fi

echo "[phase3-report] complete"
echo "[phase3-report] summary=${summary_path}"
echo "[phase3-report] graphs=${GRAPHS_DIR}"
