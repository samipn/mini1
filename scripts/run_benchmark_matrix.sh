#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_CSV="/datasets/i4gi-tjb9.csv"
SUBSETS_DIR="${ROOT_DIR}/data/subsets"
SIZES="10000,50000,100000,250000,500000,1000000,2000000,4000000"
PHASES="1,2,3"
RUNS=3
THREADS="1,2,4,8,16"
TIMEOUT_SECONDS=0
CLEAN_FIRST=0

usage() {
  cat <<'USAGE'
Usage: scripts/run_benchmark_matrix.sh [options]

Consolidated benchmark runner for Phase 1/2/3 with configurable subset sizes.
Generates subsets, runs per-size phase benchmarks, merges manifests per phase,
and regenerates summary/graph artifacts.

Options:
  --source-csv <path>       Source dataset CSV (default: /datasets/i4gi-tjb9.csv)
  --subsets-dir <path>      Subset directory (default: data/subsets)
  --sizes <csv>             Subset sizes (default: 10000,50000,100000,250000,500000,1000000,2000000,4000000)
  --phases <csv>            Phases to run (subset of 1,2,3; default: 1,2,3)
  --runs <n>                Benchmark repeats (default: 3)
  --threads <csv>           Thread list for phase2/3 (default: 1,2,4,8,16)
  --timeout-seconds <n>     Per-scenario timeout for phase2/3 (default: 0; disabled)
  --clean-first             Remove old benchmark outputs before running
  --help                    Show help
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --source-csv) SOURCE_CSV="$2"; shift 2 ;;
    --subsets-dir) SUBSETS_DIR="$2"; shift 2 ;;
    --sizes) SIZES="$2"; shift 2 ;;
    --phases) PHASES="$2"; shift 2 ;;
    --runs) RUNS="$2"; shift 2 ;;
    --threads) THREADS="$2"; shift 2 ;;
    --timeout-seconds) TIMEOUT_SECONDS="$2"; shift 2 ;;
    --clean-first) CLEAN_FIRST=1; shift ;;
    --help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage; exit 2 ;;
  esac
done

if [[ ! -f "${SOURCE_CSV}" ]]; then
  echo "[benchmark-matrix] source CSV not found: ${SOURCE_CSV}" >&2
  exit 1
fi
if ! [[ "${RUNS}" =~ ^[0-9]+$ ]] || [[ "${RUNS}" -lt 1 ]]; then
  echo "[benchmark-matrix] --runs must be >= 1" >&2
  exit 2
fi
if ! [[ "${TIMEOUT_SECONDS}" =~ ^[0-9]+$ ]]; then
  echo "[benchmark-matrix] --timeout-seconds must be non-negative integer" >&2
  exit 2
fi

IFS=',' read -r -a SIZE_ARR <<< "${SIZES}"
for s in "${SIZE_ARR[@]}"; do
  if ! [[ "${s}" =~ ^[0-9]+$ ]] || [[ "${s}" -le 0 ]]; then
    echo "[benchmark-matrix] invalid size in --sizes: ${s}" >&2
    exit 2
  fi
done

if [[ "${CLEAN_FIRST}" == "1" ]]; then
  "${ROOT_DIR}/scripts/clean_benchmark_outputs.sh"
fi

"${ROOT_DIR}/scripts/configure_openmp_build.sh" --build-dir "${ROOT_DIR}/build" --build-type Release --compiler g++

mkdir -p "${SUBSETS_DIR}"
python3 "${ROOT_DIR}/scripts/make_subsets.py" --input "${SOURCE_CSV}" --output-dir "${SUBSETS_DIR}" --sizes "${SIZE_ARR[@]}"

run_phase_enabled() {
  local phase="$1"
  [[ ",${PHASES}," == *",${phase},"* ]]
}

collect_new_manifest() {
  local out_dir="$1"
  local before_list="$2"
  local latest
  latest="$(find "${out_dir}" -maxdepth 1 -type f -name 'batch_*_manifest.csv' | sort)"
  comm -13 <(printf "%s\n" "${before_list}" | sed '/^$/d') <(printf "%s\n" "${latest}" | sed '/^$/d') | tail -n 1
}

merge_manifests() {
  local output_path="$1"
  shift
  local first=1
  : > "${output_path}"
  for m in "$@"; do
    [[ -z "${m}" ]] && continue
    if [[ "${first}" -eq 1 ]]; then
      cat "${m}" > "${output_path}"
      first=0
    else
      tail -n +2 "${m}" >> "${output_path}"
    fi
  done
}

TS="$(date -u +%Y%m%dT%H%M%SZ)"
PH1_MANIFESTS=()
PH2_MANIFESTS=()
PH3_MANIFESTS=()

for s in "${SIZE_ARR[@]}"; do
  dataset="${SUBSETS_DIR}/$(basename "${SOURCE_CSV%.*}")_subset_${s}.csv"
  if [[ ! -f "${dataset}" ]]; then
    echo "[benchmark-matrix] missing subset after generation: ${dataset}" >&2
    exit 1
  fi
  label="n${s}"

  if run_phase_enabled 1; then
    before="$(find "${ROOT_DIR}/results/raw/phase1_dev" -maxdepth 1 -type f -name 'batch_*_manifest.csv' 2>/dev/null | sort || true)"
    "${ROOT_DIR}/scripts/run_phase1_dev_benchmarks.sh" --dataset-path "${dataset}" --dataset-label "${label}" --runs "${RUNS}"
    m="$(collect_new_manifest "${ROOT_DIR}/results/raw/phase1_dev" "${before}")"
    PH1_MANIFESTS+=("${m}")
  fi

  if run_phase_enabled 2; then
    before="$(find "${ROOT_DIR}/results/raw/phase2_dev" -maxdepth 1 -type f -name 'batch_*_manifest.csv' 2>/dev/null | sort || true)"
    "${ROOT_DIR}/scripts/run_phase2_dev_benchmarks.sh" \
      --dataset-path "${dataset}" \
      --dataset-label "${label}" \
      --runs "${RUNS}" \
      --threads "${THREADS}" \
      --timeout-seconds "${TIMEOUT_SECONDS}"
    m="$(collect_new_manifest "${ROOT_DIR}/results/raw/phase2_dev" "${before}")"
    PH2_MANIFESTS+=("${m}")
  fi

  if run_phase_enabled 3; then
    before="$(find "${ROOT_DIR}/results/raw/phase3_dev" -maxdepth 1 -type f -name 'batch_*_manifest.csv' 2>/dev/null | sort || true)"
    "${ROOT_DIR}/scripts/run_phase3_dev_benchmarks.sh" \
      --dataset-path "${dataset}" \
      --dataset-label "${label}" \
      --runs "${RUNS}" \
      --threads "${THREADS}" \
      --timeout-seconds "${TIMEOUT_SECONDS}" \
      --no-validate-serial
    m="$(collect_new_manifest "${ROOT_DIR}/results/raw/phase3_dev" "${before}")"
    PH3_MANIFESTS+=("${m}")
  fi

done

if run_phase_enabled 1; then
  python3 "${ROOT_DIR}/scripts/summarize_phase1_dev.py" \
    --input-dir "${ROOT_DIR}/results/raw/phase1_dev" \
    --output-dir "${ROOT_DIR}/results/tables/phase1_dev" \
    --output-file "phase1_dev_summary_matrix_${TS}.csv" \
    --include-all-labels
  python3 "${ROOT_DIR}/scripts/plot_phase1_dev.py" \
    --summary-csv "${ROOT_DIR}/results/tables/phase1_dev/phase1_dev_summary_matrix_${TS}.csv" \
    --output-dir "${ROOT_DIR}/results/graphs/phase1_dev"
fi

if run_phase_enabled 2; then
  merged2="${ROOT_DIR}/results/raw/phase2_dev/batch_${TS}_manifest_merged.csv"
  merge_manifests "${merged2}" "${PH2_MANIFESTS[@]}"
  python3 "${ROOT_DIR}/scripts/summarize_phase2_dev.py" \
    --manifest "${merged2}" \
    --output-dir "${ROOT_DIR}/results/tables/phase2_dev" \
    --output-file "phase2_dev_summary_matrix_${TS}.csv"
  python3 "${ROOT_DIR}/scripts/plot_phase2_dev.py" \
    --summary-csv "${ROOT_DIR}/results/tables/phase2_dev/phase2_dev_summary_matrix_${TS}.csv" \
    --output-dir "${ROOT_DIR}/results/graphs/phase2_dev" \
    --parallel-thread 4 \
    --parallel-threads "${THREADS}" \
    --allow-partial
fi

if run_phase_enabled 3; then
  merged3="${ROOT_DIR}/results/raw/phase3_dev/batch_${TS}_manifest_merged.csv"
  merge_manifests "${merged3}" "${PH3_MANIFESTS[@]}"
  python3 "${ROOT_DIR}/scripts/summarize_phase3_dev.py" \
    --manifest "${merged3}" \
    --allow-stale-manifest \
    --output-dir "${ROOT_DIR}/results/tables/phase3_dev" \
    --output-file "phase3_dev_summary_matrix_${TS}.csv"
  python3 "${ROOT_DIR}/scripts/plot_phase3_dev.py" \
    --summary-csv "${ROOT_DIR}/results/tables/phase3_dev/phase3_dev_summary_matrix_${TS}.csv" \
    --output-dir "${ROOT_DIR}/results/graphs/phase3_dev" \
    --parallel-thread 4 \
    --optimized-thread 4 \
    --allow-partial
fi

# Threading verification snapshot (phase2 and phase3 binaries)
VERIFY_DIR="${ROOT_DIR}/results/raw/verification"
mkdir -p "${VERIFY_DIR}"
verify_target_size="${SIZE_ARR[0]}"
if [[ " ${SIZES} " == *"1000000"* ]]; then
  verify_target_size="1000000"
fi
verify_dataset="${SUBSETS_DIR}/$(basename "${SOURCE_CSV%.*}")_subset_${verify_target_size}.csv"
verify_csv="${VERIFY_DIR}/thread_verification_${TS}.csv"

./build/run_parallel --traffic "${verify_dataset}" --query summary --thread-list "${THREADS}" --benchmark-runs 1 --dataset-label verify --benchmark-out "${verify_csv}" >/dev/null
./build/run_optimized --traffic "${verify_dataset}" --query summary --execution parallel --threads 1 --benchmark-runs 1 --dataset-label verify --benchmark-out "${verify_csv}" --benchmark-append >/dev/null

echo "[benchmark-matrix] complete"
echo "[benchmark-matrix] phase1_manifests=${#PH1_MANIFESTS[@]} phase2_manifests=${#PH2_MANIFESTS[@]} phase3_manifests=${#PH3_MANIFESTS[@]}"
echo "[benchmark-matrix] thread_verification=${verify_csv}"
