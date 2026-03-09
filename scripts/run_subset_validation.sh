#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BINARY="${BUILD_DIR}/run_serial"
SUBSETS_DIR="${ROOT_DIR}/data/subsets"
OUT_DIR="${ROOT_DIR}/results/raw/validation"
PROGRESS_EVERY=500000

SMALL_PATH=""
MEDIUM_PATH=""
LARGE_PATH=""

usage() {
  cat <<'EOF'
Usage: scripts/run_subset_validation.sh [options]

Run loader validation on small/medium/large subset CSV files and save
timestamped logs under results/raw/validation/.

Options:
  --subsets-dir <path>     Directory containing subset CSV files
                           (default: data/subsets)
  --out-dir <path>         Output directory for validation logs
                           (default: results/raw/validation)
  --small <path>           Path for small subset CSV (10,000 rows)
  --medium <path>          Path for medium subset CSV (100,000 rows)
  --large <path>           Path for large-dev subset CSV (1,000,000 rows)
  --progress-every <rows>  Loader progress interval
                           (default: 500000)
  --help                   Show this help message

Notes:
  - If --small/--medium/--large are omitted, the script auto-detects files in
    --subsets-dir using patterns: *_subset_10000.csv, *_subset_100000.csv,
    *_subset_1000000.csv.
  - Logs are never overwritten; each run uses a UTC timestamp.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --subsets-dir)
      SUBSETS_DIR="$2"
      shift 2
      ;;
    --out-dir)
      OUT_DIR="$2"
      shift 2
      ;;
    --small)
      SMALL_PATH="$2"
      shift 2
      ;;
    --medium)
      MEDIUM_PATH="$2"
      shift 2
      ;;
    --large)
      LARGE_PATH="$2"
      shift 2
      ;;
    --progress-every)
      PROGRESS_EVERY="$2"
      shift 2
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

find_single_match() {
  local pattern="$1"
  local matches=()
  while IFS= read -r line; do
    matches+=("$line")
  done < <(find "${SUBSETS_DIR}" -maxdepth 1 -type f -name "${pattern}" | sort)

  if [[ ${#matches[@]} -eq 0 ]]; then
    echo ""
    return 0
  fi
  echo "${matches[0]}"
}

if [[ -z "${SMALL_PATH}" ]]; then
  SMALL_PATH="$(find_single_match "*_subset_10000.csv")"
fi
if [[ -z "${MEDIUM_PATH}" ]]; then
  MEDIUM_PATH="$(find_single_match "*_subset_100000.csv")"
fi
if [[ -z "${LARGE_PATH}" ]]; then
  LARGE_PATH="$(find_single_match "*_subset_1000000.csv")"
fi

if [[ ! -x "${BINARY}" ]]; then
  echo "[validation] build binary not found, building run_serial..."
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
  cmake --build "${BUILD_DIR}" -j
fi

for required in "${SMALL_PATH}" "${MEDIUM_PATH}" "${LARGE_PATH}"; do
  if [[ -z "${required}" ]]; then
    echo "[validation] missing subset file. Provide --small/--medium/--large or check --subsets-dir." >&2
    exit 1
  fi
  if [[ ! -f "${required}" ]]; then
    echo "[validation] subset file not found: ${required}" >&2
    exit 1
  fi
done

mkdir -p "${OUT_DIR}"
TS="$(date -u +%Y%m%dT%H%M%SZ)"

run_one() {
  local label="$1"
  local csv_path="$2"
  local log_path="${OUT_DIR}/validation_${label}_${TS}.log"

  echo "[validation] running label=${label} csv=${csv_path}"
  "${BINARY}" \
    --traffic "${csv_path}" \
    --progress-every "${PROGRESS_EVERY}" \
    > "${log_path}" 2>&1

  echo "[validation] log=${log_path}"
  echo "[validation] summary ${label}:"
  grep -E "rows_read=|rows_accepted=|rows_rejected=|malformed_rows=|missing_timestamp=|suspicious_speed=|suspicious_travel_time=" "${log_path}" || true
  echo
}

run_one "small" "${SMALL_PATH}"
run_one "medium" "${MEDIUM_PATH}"
run_one "large_dev" "${LARGE_PATH}"

echo "[validation] complete. Logs saved to ${OUT_DIR}"
