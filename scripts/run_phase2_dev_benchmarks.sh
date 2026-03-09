#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
SERIAL_BINARY="${BUILD_DIR}/run_serial"
PARALLEL_BINARY="${BUILD_DIR}/run_parallel"
SCENARIO_FILE="${ROOT_DIR}/configs/phase2_dev_scenarios.conf"

SUBSETS_DIR="${ROOT_DIR}/data/subsets"
OUT_DIR="${ROOT_DIR}/results/raw/phase2_dev"
LOG_DIR="${ROOT_DIR}/results/raw/logs"
RUNS=3
THREAD_LIST="1,2,4,8,16"
ALLOW_UNDER_MIN_RUNS=false
TIMEOUT_SECONDS=0
DATASET_PATH=""
DATASET_LABEL=""

SMALL_PATH=""
MEDIUM_PATH=""
LARGE_PATH=""

usage() {
  cat <<'USAGE'
Usage: scripts/run_phase2_dev_benchmarks.sh [options]

Run repeatable Phase 2 serial-vs-parallel development benchmarks on subset datasets.

Options:
  --subsets-dir <path>    Directory containing subset CSV files (default: data/subsets)
  --small <path>          Small subset CSV path (10k)
  --medium <path>         Medium subset CSV path (100k)
  --large <path>          Large-dev subset CSV path (1M)
  --runs <n>              Benchmark repetitions per scenario (default: 3)
  --allow-under-min-runs  Allow runs < 3 (smoke/debug only)
  --threads <csv>         Parallel thread list (default: 1,2,4,8,16)
  --timeout-seconds <n>   Per scenario timeout (0 disables; default: 0)
  --dataset-path <path>   Run only one dataset path (overrides --small/--medium/--large)
  --dataset-label <name>  Label to use with --dataset-path (default: custom)
  --out-dir <path>        Raw benchmark output dir (default: results/raw/phase2_dev)
  --log-dir <path>        Log output dir (default: results/raw/logs)
  --help                  Show this help
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --subsets-dir)
      SUBSETS_DIR="$2"
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
    --runs)
      RUNS="$2"
      shift 2
      ;;
    --allow-under-min-runs)
      ALLOW_UNDER_MIN_RUNS=true
      shift
      ;;
    --threads)
      THREAD_LIST="$2"
      shift 2
      ;;
    --timeout-seconds)
      TIMEOUT_SECONDS="$2"
      shift 2
      ;;
    --dataset-path)
      DATASET_PATH="$2"
      shift 2
      ;;
    --dataset-label)
      DATASET_LABEL="$2"
      shift 2
      ;;
    --out-dir)
      OUT_DIR="$2"
      shift 2
      ;;
    --log-dir)
      LOG_DIR="$2"
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

if ! [[ "${RUNS}" =~ ^[0-9]+$ ]]; then
  echo "[phase2-dev] --runs must be a positive integer." >&2
  exit 2
fi
if [[ "${RUNS}" -lt 1 ]]; then
  echo "[phase2-dev] --runs must be >= 1." >&2
  exit 2
fi
if [[ "${RUNS}" -lt 3 && "${ALLOW_UNDER_MIN_RUNS}" != "true" ]]; then
  echo "[phase2-dev] --runs must be >= 3 for deliverable-grade dev benchmarking." >&2
  echo "[phase2-dev] use --allow-under-min-runs only for smoke/debug runs." >&2
  exit 2
fi
if ! [[ "${TIMEOUT_SECONDS}" =~ ^[0-9]+$ ]]; then
  echo "[phase2-dev] --timeout-seconds must be a non-negative integer." >&2
  exit 2
fi

if [[ ! -f "${SCENARIO_FILE}" ]]; then
  echo "[phase2-dev] missing scenario file: ${SCENARIO_FILE}" >&2
  exit 1
fi

if [[ ! -x "${SERIAL_BINARY}" || ! -x "${PARALLEL_BINARY}" ]]; then
  echo "[phase2-dev] building run_serial/run_parallel..."
  "${ROOT_DIR}/scripts/configure_openmp_build.sh" --build-dir "${BUILD_DIR}" --build-type Release
fi

mkdir -p "${OUT_DIR}" "${LOG_DIR}"

find_single_match() {
  local pattern="$1"
  find "${SUBSETS_DIR}" -maxdepth 1 -type f -name "${pattern}" | sort | head -n 1
}

declare -a LABELS=()
declare -a PATHS=()
if [[ -n "${DATASET_PATH}" ]]; then
  if [[ ! -f "${DATASET_PATH}" ]]; then
    echo "[phase2-dev] dataset not found: ${DATASET_PATH}" >&2
    exit 1
  fi
  LABELS+=("${DATASET_LABEL:-custom}")
  PATHS+=("${DATASET_PATH}")
else
  if [[ -z "${SMALL_PATH}" ]]; then
    SMALL_PATH="$(find_single_match "*_subset_10000.csv")"
  fi
  if [[ -z "${MEDIUM_PATH}" ]]; then
    MEDIUM_PATH="$(find_single_match "*_subset_100000.csv")"
  fi
  if [[ -z "${LARGE_PATH}" ]]; then
    LARGE_PATH="$(find_single_match "*_subset_1000000.csv")"
  fi

  for required in "${SMALL_PATH}" "${MEDIUM_PATH}" "${LARGE_PATH}"; do
    if [[ -z "${required}" || ! -f "${required}" ]]; then
      echo "[phase2-dev] missing subset file. Provide --small/--medium/--large or check --subsets-dir." >&2
      exit 1
    fi
  done

  LABELS=("small" "medium" "large_dev")
  PATHS=("${SMALL_PATH}" "${MEDIUM_PATH}" "${LARGE_PATH}")
fi

TS="$(date -u +%Y%m%dT%H%M%SZ)"
SAFE_BRANCH="$(git -C "${ROOT_DIR}" branch --show-current | tr '/ ' '__')"
GIT_BRANCH="$(git -C "${ROOT_DIR}" branch --show-current 2>/dev/null || echo unknown)"
GIT_COMMIT="$(git -C "${ROOT_DIR}" rev-parse --short HEAD 2>/dev/null || echo unknown)"

SUBSET_MANIFEST="${OUT_DIR}/subset_manifest_${TS}.csv"
BATCH_MANIFEST="${OUT_DIR}/batch_${TS}_manifest.csv"
RPS_CSV="${OUT_DIR}/batch_${TS}_records_per_second.csv"
ENV_CSV="${OUT_DIR}/batch_${TS}_environment.csv"

cat > "${SUBSET_MANIFEST}" <<CSV
batch_utc,subset_label,dataset_path,expected_data_rows,actual_data_rows,serial_access_ok,parallel_access_ok
CSV

cat > "${BATCH_MANIFEST}" <<CSV
batch_utc,git_branch,git_commit,subset_label,scenario_name,mode,thread_list,dataset_path,benchmark_runs,output_csv,log_file
CSV

cat > "${RPS_CSV}" <<CSV
batch_utc,git_branch,git_commit,subset_label,scenario_name,mode,thread_count,run_number,total_ms,rows_accepted,records_per_second
CSV

csv_escape() {
  local value="$1"
  value="${value//\"/\"\"}"
  printf '"%s"' "${value}"
}

cpu_model="$(lscpu 2>/dev/null | awk -F: '/Model name/ {gsub(/^[ \t]+/, "", $2); print $2; exit}' || true)"
if [[ -z "${cpu_model}" && -f /proc/cpuinfo ]]; then
  cpu_model="$(awk -F: '/model name/ {gsub(/^[ \t]+/, "", $2); print $2; exit}' /proc/cpuinfo || true)"
fi
logical_cores="$(nproc 2>/dev/null || echo unknown)"
mem_total_kb="$(awk '/MemTotal/ {print $2; exit}' /proc/meminfo 2>/dev/null || echo unknown)"
compiler_bin="${CXX:-c++}"
compiler_version="$(${compiler_bin} --version 2>/dev/null | head -n 1 || echo unknown)"

cat > "${ENV_CSV}" <<CSV
batch_utc,host,os_kernel,cpu_model,logical_cores,mem_total_kb,compiler,compiler_version,git_branch,git_commit,thread_list,benchmark_runs
CSV

printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" \
  "${TS}" \
  "$(csv_escape "$(hostname 2>/dev/null || echo unknown)")" \
  "$(csv_escape "$(uname -sr 2>/dev/null || echo unknown)")" \
  "$(csv_escape "${cpu_model}")" \
  "${logical_cores}" \
  "${mem_total_kb}" \
  "$(csv_escape "${compiler_bin}")" \
  "$(csv_escape "${compiler_version}")" \
  "$(csv_escape "${GIT_BRANCH}")" \
  "$(csv_escape "${GIT_COMMIT}")" \
  "$(csv_escape "${THREAD_LIST}")" \
  "${RUNS}" >> "${ENV_CSV}"

calc_rows() {
  local file="$1"
  local lines
  lines=$(wc -l < "${file}")
  if [[ "${lines}" -le 1 ]]; then
    echo 0
  else
    echo $((lines - 1))
  fi
}

run_access_probe() {
  local bin="$1"
  local path="$2"
  if [[ "${bin}" == "${SERIAL_BINARY}" ]]; then
    "${bin}" --traffic "${path}" --query summary --benchmark-runs 1 >/dev/null 2>&1
  else
    "${bin}" --traffic "${path}" --query summary --threads 1 --benchmark-runs 1 --validate-serial >/dev/null 2>&1
  fi
}

run_with_optional_timeout() {
  if [[ "${TIMEOUT_SECONDS}" -gt 0 ]]; then
    timeout --signal=TERM --kill-after=20 "${TIMEOUT_SECONDS}" "$@"
  else
    "$@"
  fi
}

emit_rps_rows() {
  local csv_file="$1"
  local subset_label="$2"
  local scenario_name="$3"
  local mode="$4"

  awk -F, -v ts="${TS}" -v branch="${GIT_BRANCH}" -v commit="${GIT_COMMIT}" \
      -v subset="${subset_label}" -v scenario="${scenario_name}" -v mode="${mode}" '
    NR == 1 {
      for (i = 1; i <= NF; ++i) {
        col[$i] = i;
      }
      if (!("thread_count" in col) || !("run_number" in col) || !("total_ms" in col) || !("rows_accepted" in col)) {
        print "[phase2-dev] benchmark CSV missing required columns in " FILENAME > "/dev/stderr";
        exit 2;
      }
      next;
    }
    NR > 1 {
      total_ms = $(col["total_ms"]) + 0.0;
      rows_accepted = $(col["rows_accepted"]) + 0.0;
      rps = (total_ms > 0.0) ? (rows_accepted / (total_ms / 1000.0)) : 0.0;
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%.6f,%s,%.6f\n",
             ts, branch, commit, subset, scenario, mode, $(col["thread_count"]), $(col["run_number"]), total_ms, $(col["rows_accepted"]), rps;
    }
  ' "${csv_file}" >> "${RPS_CSV}"
}

run_serial_scenario() {
  local subset_label="$1"
  local dataset_path="$2"
  local scenario_name="$3"
  local query_type="$4"
  local threshold="$5"
  local start_epoch="$6"
  local end_epoch="$7"
  local borough="$8"

  local out_csv="${OUT_DIR}/${subset_label}_${scenario_name}_serial_${SAFE_BRANCH}_${GIT_COMMIT}_${TS}.csv"
  local log_file="${LOG_DIR}/phase2_dev_${subset_label}_${scenario_name}_serial_${TS}.log"

  local cmd=(
    "${SERIAL_BINARY}"
    --traffic "${dataset_path}"
    --benchmark-runs "${RUNS}"
    --dataset-label "${subset_label}"
    --benchmark-out "${out_csv}"
  )

  if [[ "${query_type}" != "ingest_only" ]]; then
    cmd+=(--query "${query_type}")
  fi
  if [[ -n "${threshold}" ]]; then
    cmd+=(--threshold "${threshold}")
  fi
  if [[ -n "${start_epoch}" ]]; then
    cmd+=(--start-epoch "${start_epoch}")
  fi
  if [[ -n "${end_epoch}" ]]; then
    cmd+=(--end-epoch "${end_epoch}")
  fi
  if [[ -n "${borough}" ]]; then
    cmd+=(--borough "${borough}")
  fi

  echo "[phase2-dev] serial subset=${subset_label} scenario=${scenario_name}"
  run_with_optional_timeout "${cmd[@]}" >"${log_file}" 2>&1

  printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" \
    "${TS}" "${GIT_BRANCH}" "${GIT_COMMIT}" "${subset_label}" "${scenario_name}" \
    "serial" "1" "${dataset_path}" "${RUNS}" "${out_csv}" "${log_file}" >> "${BATCH_MANIFEST}"

  emit_rps_rows "${out_csv}" "${subset_label}" "${scenario_name}" "serial"
}

run_parallel_scenario() {
  local subset_label="$1"
  local dataset_path="$2"
  local scenario_name="$3"
  local query_type="$4"
  local threshold="$5"
  local start_epoch="$6"
  local end_epoch="$7"
  local borough="$8"

  local out_csv="${OUT_DIR}/${subset_label}_${scenario_name}_parallel_${SAFE_BRANCH}_${GIT_COMMIT}_${TS}.csv"
  local log_file="${LOG_DIR}/phase2_dev_${subset_label}_${scenario_name}_parallel_${TS}.log"
  local thread_field="${THREAD_LIST//,/;}"

  local cmd=(
    "${PARALLEL_BINARY}"
    --traffic "${dataset_path}"
    --benchmark-runs "${RUNS}"
    --dataset-label "${subset_label}"
    --thread-list "${THREAD_LIST}"
    --benchmark-out "${out_csv}"
    --validate-serial
  )

  if [[ "${query_type}" != "ingest_only" ]]; then
    cmd+=(--query "${query_type}")
  fi
  if [[ -n "${threshold}" ]]; then
    cmd+=(--threshold "${threshold}")
  fi
  if [[ -n "${start_epoch}" ]]; then
    cmd+=(--start-epoch "${start_epoch}")
  fi
  if [[ -n "${end_epoch}" ]]; then
    cmd+=(--end-epoch "${end_epoch}")
  fi
  if [[ -n "${borough}" ]]; then
    cmd+=(--borough "${borough}")
  fi

  echo "[phase2-dev] parallel subset=${subset_label} scenario=${scenario_name} threads=${THREAD_LIST}"
  run_with_optional_timeout "${cmd[@]}" >"${log_file}" 2>&1

  printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" \
    "${TS}" "${GIT_BRANCH}" "${GIT_COMMIT}" "${subset_label}" "${scenario_name}" \
    "parallel" "${thread_field}" "${dataset_path}" "${RUNS}" "${out_csv}" "${log_file}" >> "${BATCH_MANIFEST}"

  emit_rps_rows "${out_csv}" "${subset_label}" "${scenario_name}" "parallel"
}

for i in "${!LABELS[@]}"; do
  subset_label="${LABELS[$i]}"
  dataset_path="${PATHS[$i]}"
  expected_rows="$(calc_rows "${dataset_path}")"
  actual_rows="$(calc_rows "${dataset_path}")"

  serial_access_ok="false"
  parallel_access_ok="false"
  if run_access_probe "${SERIAL_BINARY}" "${dataset_path}"; then
    serial_access_ok="true"
  fi
  if run_access_probe "${PARALLEL_BINARY}" "${dataset_path}"; then
    parallel_access_ok="true"
  fi

  printf "%s,%s,%s,%s,%s,%s,%s\n" \
    "${TS}" "${subset_label}" "${dataset_path}" "${expected_rows}" "${actual_rows}" \
    "${serial_access_ok}" "${parallel_access_ok}" >> "${SUBSET_MANIFEST}"

done

for i in "${!LABELS[@]}"; do
  subset_label="${LABELS[$i]}"
  dataset_path="${PATHS[$i]}"

  while IFS=',' read -r scenario_name query_type threshold start_epoch end_epoch borough; do
    if [[ "${scenario_name}" == "scenario_name" ]]; then
      continue
    fi
    run_serial_scenario "${subset_label}" "${dataset_path}" "${scenario_name}" "${query_type}" \
      "${threshold}" "${start_epoch}" "${end_epoch}" "${borough}"
    run_parallel_scenario "${subset_label}" "${dataset_path}" "${scenario_name}" "${query_type}" \
      "${threshold}" "${start_epoch}" "${end_epoch}" "${borough}"
  done < "${SCENARIO_FILE}"
done

echo "[phase2-dev] complete"
echo "[phase2-dev] subset_manifest=${SUBSET_MANIFEST}"
echo "[phase2-dev] batch_manifest=${BATCH_MANIFEST}"
echo "[phase2-dev] rps=${RPS_CSV}"
echo "[phase2-dev] env=${ENV_CSV}"
