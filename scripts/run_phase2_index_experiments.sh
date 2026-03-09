#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BINARY="${BUILD_DIR}/run_index_experiments"

TRAFFIC_PATH=""
GARAGES_PATH=""
BES_PATH=""
REPEATS=3
OUTPUT_CSV="${ROOT_DIR}/results/raw/phase2/index_lookup_benchmark.csv"

usage() {
  cat <<'USAGE'
Usage: scripts/run_phase2_index_experiments.sh [options]

Options:
  --traffic <path>   Traffic CSV path (required)
  --garages <path>   Garages CSV path (optional, synthetic fixture used if omitted)
  --bes <path>       BES/buildings CSV path (optional, synthetic fixture used if omitted)
  --repeats <n>      Repeat count for scan/lookup loops (default: 3)
  --output <path>    Output CSV path (default: results/raw/phase2/index_lookup_benchmark.csv)
  --help             Show help
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --traffic)
      TRAFFIC_PATH="$2"
      shift 2
      ;;
    --garages)
      GARAGES_PATH="$2"
      shift 2
      ;;
    --bes)
      BES_PATH="$2"
      shift 2
      ;;
    --repeats)
      REPEATS="$2"
      shift 2
      ;;
    --output)
      OUTPUT_CSV="$2"
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

if [[ -z "${TRAFFIC_PATH}" || ! -f "${TRAFFIC_PATH}" ]]; then
  echo "[phase2-index] --traffic is required and must exist" >&2
  exit 1
fi

if [[ ! -x "${BINARY}" ]]; then
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
  cmake --build "${BUILD_DIR}" -j
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "${TMP_DIR}"' EXIT

if [[ -z "${GARAGES_PATH}" ]]; then
  GARAGES_PATH="${TMP_DIR}/garages_fixture.csv"
  cat > "${GARAGES_PATH}" <<'CSV'
license_nbr,bbl,address_borough
LIC-1,1-00001-0001,Manhattan
LIC-2,1-00001-0001,Manhattan
LIC-3,2-00002-0002,Brooklyn
CSV
fi

if [[ -z "${BES_PATH}" ]]; then
  BES_PATH="${TMP_DIR}/bes_fixture.csv"
  cat > "${BES_PATH}" <<'CSV'
bbl,subgrade_level,elevation_feet,latitude,longitude
1-00001-0001,1,12.5,40.0,-73.0
3-00003-0003,2,13.1,40.1,-73.1
CSV
fi

mkdir -p "$(dirname "${OUTPUT_CSV}")"

"${BINARY}" \
  --traffic "${TRAFFIC_PATH}" \
  --garages "${GARAGES_PATH}" \
  --bes "${BES_PATH}" \
  --repeats "${REPEATS}" \
  --output "${OUTPUT_CSV}"

echo "[phase2-index] wrote ${OUTPUT_CSV}"
