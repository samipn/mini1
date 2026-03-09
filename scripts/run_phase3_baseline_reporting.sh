#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ALLOW_NONBASELINE=0
MANIFEST=""
TABLES_DIR="${ROOT_DIR}/results/tables/phase3_baseline"
GRAPHS_DIR="${ROOT_DIR}/results/graphs/phase3_baseline"

usage() {
  cat <<'USAGE'
Usage: scripts/run_phase3_baseline_reporting.sh --manifest <path> [options]

Baseline-specific wrapper around run_phase3_dev_reporting.sh with baseline output defaults.

Options:
  --manifest <path>           Baseline batch manifest (expected under results/raw/phase3_baseline)
  --tables-dir <path>         Baseline summary output dir (default: results/tables/phase3_baseline)
  --graphs-dir <path>         Baseline graph output dir (default: results/graphs/phase3_baseline)
  --allow-nonbaseline         Disable manifest-path baseline guard
  --help                      Show this help
USAGE
}

extra_args=()
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
    --allow-nonbaseline)
      ALLOW_NONBASELINE=1
      shift 1
      ;;
    --help)
      usage
      exit 0
      ;;
    *)
      extra_args+=("$1")
      shift 1
      if [[ $# -gt 0 && "$1" != --* ]]; then
        extra_args+=("$1")
        shift 1
      fi
      ;;
  esac
done

if [[ -z "${MANIFEST}" ]]; then
  echo "[phase3-baseline-report] --manifest is required" >&2
  exit 2
fi

if [[ "${ALLOW_NONBASELINE}" != "1" ]]; then
  case "${MANIFEST}" in
    *"/phase3_baseline/"*|*"phase3_baseline"*) ;;
    *)
      echo "[phase3-baseline-report] manifest does not look like phase3_baseline path: ${MANIFEST}" >&2
      echo "Use --allow-nonbaseline to override." >&2
      exit 2
      ;;
  esac
fi

"${ROOT_DIR}/scripts/run_phase3_dev_reporting.sh" \
  --manifest "${MANIFEST}" \
  --tables-dir "${TABLES_DIR}" \
  --graphs-dir "${GRAPHS_DIR}" \
  "${extra_args[@]}"
