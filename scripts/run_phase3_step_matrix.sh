#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_ROOT="${ROOT_DIR}/results/raw/phase3_step_matrix"
LOG_DIR="${ROOT_DIR}/results/raw/logs"
RUNS=3
THREAD_LIST="1,2,4,8,16"
VALIDATE_SERIAL=0
DRY_RUN=0

usage() {
  cat <<'USAGE'
Usage: scripts/run_phase3_step_matrix.sh [options]

Run an automated Phase 3 step-matrix benchmark campaign with separate batch artifacts per optimization step.

Options:
  --out-root <path>       Root output dir for step batches (default: results/raw/phase3_step_matrix)
  --log-dir <path>        Log output dir (default: results/raw/logs)
  --runs <n>              Benchmark repetitions per step batch (default: 3)
  --threads <csv>         Thread list (default: 1,2,4,8,16)
  --validate-serial       Enable serial parity validation during benchmark runs
  --no-validate-serial    Disable serial parity validation (default)
  --dry-run               Print planned commands only
  --help                  Show this help
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-root)
      OUT_ROOT="$2"
      shift 2
      ;;
    --log-dir)
      LOG_DIR="$2"
      shift 2
      ;;
    --runs)
      RUNS="$2"
      shift 2
      ;;
    --threads)
      THREAD_LIST="$2"
      shift 2
      ;;
    --validate-serial)
      VALIDATE_SERIAL=1
      shift 1
      ;;
    --no-validate-serial)
      VALIDATE_SERIAL=0
      shift 1
      ;;
    --dry-run)
      DRY_RUN=1
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

if ! [[ "${RUNS}" =~ ^[0-9]+$ ]] || [[ "${RUNS}" -lt 1 ]]; then
  echo "[phase3-step-matrix] --runs must be a positive integer" >&2
  exit 2
fi

mkdir -p "${OUT_ROOT}" "${LOG_DIR}"
TS="$(date -u +%Y%m%dT%H%M%SZ)"
CAMPAIGN_CSV="${OUT_ROOT}/campaign_${TS}.csv"

cat > "${CAMPAIGN_CSV}" <<CSV
campaign_utc,step_name,runner,batch_manifest,environment_csv,records_per_second_csv
CSV

run_step() {
  local step_name="$1"
  local runner="$2"
  local step_out_dir="${OUT_ROOT}/${step_name}"
  mkdir -p "${step_out_dir}"

  local cmd=(
    "${runner}"
    --runs "${RUNS}"
    --threads "${THREAD_LIST}"
    --out-dir "${step_out_dir}"
    --log-dir "${LOG_DIR}"
    --optimization-step "${step_name}"
  )

  if [[ "${VALIDATE_SERIAL}" == "1" ]]; then
    cmd+=(--validate-serial)
  else
    cmd+=(--no-validate-serial)
  fi

  if [[ "${DRY_RUN}" == "1" ]]; then
    echo "[phase3-step-matrix] dry-run step=${step_name}: ${cmd[*]}"
    printf "%s,%s,%s,%s,%s,%s\n" \
      "${TS}" "${step_name}" "${runner}" "(dry-run)" "(dry-run)" "(dry-run)" >> "${CAMPAIGN_CSV}"
    return 0
  fi

  echo "[phase3-step-matrix] running step=${step_name}"
  "${cmd[@]}"

  local manifest
  manifest="$(find "${step_out_dir}" -maxdepth 1 -type f -name 'batch_*_manifest.csv' | sort | tail -n 1)"
  if [[ -z "${manifest}" || ! -f "${manifest}" ]]; then
    echo "[phase3-step-matrix] missing manifest for step=${step_name}" >&2
    exit 1
  fi

  local env_csv="${manifest/_manifest.csv/_environment.csv}"
  local rps_csv="${manifest/_manifest.csv/_records_per_second.csv}"
  printf "%s,%s,%s,%s,%s,%s\n" \
    "${TS}" "${step_name}" "${runner}" "${manifest}" "${env_csv}" "${rps_csv}" >> "${CAMPAIGN_CSV}"
}

# Step matrix. baseline step uses baseline wrapper; later steps use dev wrapper with explicit labels.
run_step "row_baseline" "${ROOT_DIR}/scripts/run_phase3_baseline_benchmarks.sh"
run_step "columnar_only" "${ROOT_DIR}/scripts/run_phase3_dev_benchmarks.sh"
run_step "columnar_plus_encoding" "${ROOT_DIR}/scripts/run_phase3_dev_benchmarks.sh"
run_step "columnar_plus_hotloop" "${ROOT_DIR}/scripts/run_phase3_dev_benchmarks.sh"
run_step "columnar_plus_hotloop_parallel" "${ROOT_DIR}/scripts/run_phase3_dev_benchmarks.sh"

echo "[phase3-step-matrix] complete"
echo "[phase3-step-matrix] campaign_csv=${CAMPAIGN_CSV}"
