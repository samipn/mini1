#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
SERIAL_BIN="${BUILD_DIR}/run_serial"
PAR_BIN="${BUILD_DIR}/run_parallel"
OPT_BIN="${BUILD_DIR}/run_optimized"

DATASET_PATH="${1:-${ROOT_DIR}/data/subsets/i4gi-tjb9_subset_100000.csv}"
OUT_DIR="${2:-${ROOT_DIR}/results/raw/phase3_dev/memory}"
THREADS="${3:-4}"
OPTIMIZATION_STEP="${4:-soa_encoded_hotloop}"

mkdir -p "${OUT_DIR}"

if [[ ! -x "${SERIAL_BIN}" || ! -x "${PAR_BIN}" || ! -x "${OPT_BIN}" ]]; then
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
  cmake --build "${BUILD_DIR}" -j
fi

TS="$(date -u +%Y%m%dT%H%M%SZ)"
OUT_CSV="${OUT_DIR}/memory_probe_${TS}.csv"
GIT_BRANCH="$(git -C "${ROOT_DIR}" branch --show-current 2>/dev/null || echo unknown)"
GIT_COMMIT="$(git -C "${ROOT_DIR}" rev-parse --short HEAD 2>/dev/null || echo unknown)"

echo "batch_utc,git_branch,git_commit,optimization_step,mode,query_type,max_rss_kb,elapsed_wall_clock,command" > "${OUT_CSV}"

run_probe() {
  local mode="$1"
  local query="$2"
  shift 2
  local cmd=("$@")
  local log_file="${OUT_DIR}/memory_${mode}_${query}_${TS}.log"

  /usr/bin/time -v "${cmd[@]}" > "${log_file}" 2>&1 || true

  local rss
  local elapsed
  rss="$(grep -E "Maximum resident set size" "${log_file}" | awk -F: '{print $2}' | xargs || echo "")"
  elapsed="$(grep -E "Elapsed \(wall clock\) time" "${log_file}" | awk -F': ' '{print $2}' | xargs || echo "")"
  if [[ -z "${rss}" ]]; then
    rss="0"
  fi

  printf "%s,%s,%s,%s,%s,%s,%s,%s,%q\n" \
    "${TS}" "${GIT_BRANCH}" "${GIT_COMMIT}" "${OPTIMIZATION_STEP}" "${mode}" "${query}" "${rss}" "${elapsed}" "${cmd[*]}" >> "${OUT_CSV}"
}

run_probe "serial" "summary" "${SERIAL_BIN}" --traffic "${DATASET_PATH}" --query summary --benchmark-runs 1
run_probe "parallel" "summary" "${PAR_BIN}" --traffic "${DATASET_PATH}" --query summary --threads "${THREADS}" --benchmark-runs 1 --validate-serial
run_probe "optimized_serial" "summary" "${OPT_BIN}" --traffic "${DATASET_PATH}" --query summary --execution serial --benchmark-runs 1 --validate-serial
run_probe "optimized_parallel" "summary" "${OPT_BIN}" --traffic "${DATASET_PATH}" --query summary --execution parallel --threads "${THREADS}" --benchmark-runs 1 --validate-serial

echo "[phase3-memory] wrote ${OUT_CSV}"
