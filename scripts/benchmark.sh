#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
SERIAL_BINARY="${BUILD_DIR}/run_serial"
PARALLEL_BINARY="${BUILD_DIR}/run_parallel"

DATASET_PATH="${1:-/datasets/i4gi-tjb9.csv}"
RUNS="${2:-10}"
OUT_DIR="${3:-${ROOT_DIR}/results/raw}"
THREAD_LIST="${4:-1,2,4,8}"

mkdir -p "${OUT_DIR}"

if [[ ! -x "${SERIAL_BINARY}" || ! -x "${PARALLEL_BINARY}" ]]; then
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
  cmake --build "${BUILD_DIR}" -j
fi

COMMON=(--traffic "${DATASET_PATH}" --benchmark-runs "${RUNS}" --dataset-label "traffic_$(basename "${DATASET_PATH}")")

"${SERIAL_BINARY}" "${COMMON[@]}" \
  --benchmark-out "${OUT_DIR}/serial_ingest_only.csv"

"${SERIAL_BINARY}" "${COMMON[@]}" \
  --query speed_below --threshold 15 \
  --benchmark-out "${OUT_DIR}/serial_speed_below.csv"

"${SERIAL_BINARY}" "${COMMON[@]}" \
  --query time_window --start-epoch 0 --end-epoch 4102444800 \
  --benchmark-out "${OUT_DIR}/serial_time_window.csv"

"${SERIAL_BINARY}" "${COMMON[@]}" \
  --query borough_speed_below --borough Manhattan --threshold 15 \
  --benchmark-out "${OUT_DIR}/serial_borough_speed_below.csv"

"${SERIAL_BINARY}" "${COMMON[@]}" \
  --query summary \
  --benchmark-out "${OUT_DIR}/serial_summary.csv"

"${SERIAL_BINARY}" "${COMMON[@]}" \
  --query top_n_slowest --top-n 20 --min-link-samples 1 \
  --benchmark-out "${OUT_DIR}/serial_top_n_slowest.csv"

"${PARALLEL_BINARY}" "${COMMON[@]}" \
  --query speed_below --threshold 15 --thread-list "${THREAD_LIST}" --validate-serial \
  --benchmark-out "${OUT_DIR}/parallel_speed_below.csv"

"${PARALLEL_BINARY}" "${COMMON[@]}" \
  --query time_window --start-epoch 0 --end-epoch 4102444800 --thread-list "${THREAD_LIST}" --validate-serial \
  --benchmark-out "${OUT_DIR}/parallel_time_window.csv"

"${PARALLEL_BINARY}" "${COMMON[@]}" \
  --query borough_speed_below --borough Manhattan --threshold 15 --thread-list "${THREAD_LIST}" --validate-serial \
  --benchmark-out "${OUT_DIR}/parallel_borough_speed_below.csv"

"${PARALLEL_BINARY}" "${COMMON[@]}" \
  --query summary --thread-list "${THREAD_LIST}" --validate-serial \
  --benchmark-out "${OUT_DIR}/parallel_summary.csv"

printf 'Serial/parallel benchmark outputs written to %s\n' "${OUT_DIR}"
