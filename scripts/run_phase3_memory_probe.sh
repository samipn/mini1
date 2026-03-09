#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
SERIAL_BIN="${BUILD_DIR}/run_serial"
PAR_BIN="${BUILD_DIR}/run_parallel"
OPT_BIN="${BUILD_DIR}/run_optimized"

DATASET_PATH="${ROOT_DIR}/data/subsets/i4gi-tjb9_subset_100000.csv"
OUT_DIR="${ROOT_DIR}/results/raw/phase3_dev/memory"
THREADS="4"
OPTIMIZATION_STEP="soa_encoded_hotloop"
BATCH_UTC=""

usage() {
  cat <<'USAGE'
Usage: scripts/run_phase3_memory_probe.sh [options] [dataset_path [out_dir [threads [optimization_step]]]]

Run a lightweight memory probe across serial/parallel/optimized modes.

Options:
  --dataset <path>            Dataset CSV path (default: data/subsets/i4gi-tjb9_subset_100000.csv)
  --out-dir <path>            Output directory (default: results/raw/phase3_dev/memory)
  --threads <n>               Thread count for parallel modes (default: 4)
  --optimization-step <name>  Optimization step label (default: soa_encoded_hotloop)
  --batch-utc <ts>            Batch UTC key to align with benchmark manifest batch_utc
  --help                      Show this help

Positional compatibility (legacy):
  dataset_path out_dir threads optimization_step
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --dataset)
      DATASET_PATH="$2"
      shift 2
      ;;
    --out-dir)
      OUT_DIR="$2"
      shift 2
      ;;
    --threads)
      THREADS="$2"
      shift 2
      ;;
    --optimization-step)
      OPTIMIZATION_STEP="$2"
      shift 2
      ;;
    --batch-utc)
      BATCH_UTC="$2"
      shift 2
      ;;
    --help)
      usage
      exit 0
      ;;
    *)
      # Legacy positional arguments for backward compatibility.
      DATASET_PATH="$1"
      shift 1
      if [[ $# -gt 0 ]]; then
        OUT_DIR="$1"
        shift 1
      fi
      if [[ $# -gt 0 ]]; then
        THREADS="$1"
        shift 1
      fi
      if [[ $# -gt 0 ]]; then
        OPTIMIZATION_STEP="$1"
        shift 1
      fi
      if [[ $# -gt 0 ]]; then
        echo "Unknown extra arguments: $*" >&2
        usage
        exit 2
      fi
      ;;
  esac
done

mkdir -p "${OUT_DIR}"

if [[ ! -f "${DATASET_PATH}" ]]; then
  echo "[phase3-memory] dataset not found: ${DATASET_PATH}" >&2
  exit 1
fi

if [[ ! -x "${SERIAL_BIN}" || ! -x "${PAR_BIN}" || ! -x "${OPT_BIN}" ]]; then
  "${ROOT_DIR}/scripts/configure_openmp_build.sh" --build-dir "${BUILD_DIR}" --build-type Release
fi

TS="$(date -u +%Y%m%dT%H%M%SZ)"
if [[ -n "${BATCH_UTC}" ]]; then
  TS="${BATCH_UTC}"
fi
OUT_CSV="${OUT_DIR}/memory_probe_${TS}.csv"
GIT_BRANCH="$(git -C "${ROOT_DIR}" branch --show-current 2>/dev/null || echo unknown)"
GIT_COMMIT="$(git -C "${ROOT_DIR}" rev-parse --short HEAD 2>/dev/null || echo unknown)"

echo "batch_utc,git_branch,git_commit,optimization_step,mode,thread_count,query_type,max_rss_kb,elapsed_wall_clock,exit_code,status,log_file,command" > "${OUT_CSV}"

run_probe() {
  local mode="$1"
  local thread_count="$2"
  local query="$3"
  shift 3
  local cmd=("$@")
  local log_file="${OUT_DIR}/memory_${mode}_${query}_${TS}.log"

  local exit_code=0
  if ! /usr/bin/time -v "${cmd[@]}" > "${log_file}" 2>&1; then
    exit_code=$?
  fi

  local rss
  local elapsed
  local status="ok"
  rss="$(grep -E "Maximum resident set size" "${log_file}" | awk -F: '{print $2}' | xargs || echo "")"
  elapsed="$(grep -E "Elapsed \(wall clock\) time" "${log_file}" | awk -F': ' '{print $2}' | xargs || echo "")"
  if [[ -z "${rss}" ]]; then
    rss="0"
  fi
  if [[ "${exit_code}" -ne 0 ]]; then
    status="failed"
  fi

  printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%q,%q\n" \
    "${TS}" "${GIT_BRANCH}" "${GIT_COMMIT}" "${OPTIMIZATION_STEP}" "${mode}" "${thread_count}" "${query}" "${rss}" "${elapsed}" \
    "${exit_code}" "${status}" "${log_file}" "${cmd[*]}" >> "${OUT_CSV}"

  if [[ "${status}" == "failed" ]]; then
    echo "[phase3-memory] probe failed mode=${mode} query=${query} exit_code=${exit_code} log=${log_file}" >&2
  fi
}

run_probe "serial" "1" "summary" "${SERIAL_BIN}" --traffic "${DATASET_PATH}" --query summary --benchmark-runs 1
run_probe "parallel" "${THREADS}" "summary" "${PAR_BIN}" --traffic "${DATASET_PATH}" --query summary --threads "${THREADS}" --benchmark-runs 1 --validate-serial
run_probe "optimized_serial" "1" "summary" "${OPT_BIN}" --traffic "${DATASET_PATH}" --query summary --execution serial --benchmark-runs 1 --validate-serial
run_probe "optimized_parallel" "${THREADS}" "summary" "${OPT_BIN}" --traffic "${DATASET_PATH}" --query summary --execution parallel --threads "${THREADS}" --benchmark-runs 1 --validate-serial

echo "[phase3-memory] wrote ${OUT_CSV}"
