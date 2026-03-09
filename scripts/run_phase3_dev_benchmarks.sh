#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
SERIAL_BINARY="${BUILD_DIR}/run_serial"
PARALLEL_BINARY="${BUILD_DIR}/run_parallel"
OPTIMIZED_BINARY="${BUILD_DIR}/run_optimized"
SCENARIO_FILE="${ROOT_DIR}/configs/phase3_dev_scenarios.conf"

SUBSETS_DIR="${ROOT_DIR}/data/subsets"
OUT_DIR="${ROOT_DIR}/results/raw/phase3_dev"
LOG_DIR="${ROOT_DIR}/results/raw/logs"
RUNS=3
THREAD_LIST="1,2,4,8,16"
OPTIMIZATION_STEP="soa_encoded_hotloop"
VALIDATE_SERIAL=0
ALLOW_LOW_RUNS=0
EVIDENCE_TIER="deliverable"
TIMEOUT_SECONDS=0
DATASET_PATH=""
DATASET_LABEL=""

SMALL_PATH=""
MEDIUM_PATH=""
LARGE_PATH=""

usage() {
  cat <<'USAGE'
Usage: scripts/run_phase3_dev_benchmarks.sh [options]

Run repeatable Phase 3 development benchmarks across serial, parallel, optimized serial, and optimized parallel.

Options:
  --subsets-dir <path>    Directory containing subset CSV files (default: data/subsets)
  --small <path>          Small subset CSV path (10k)
  --medium <path>         Medium subset CSV path (100k)
  --large <path>          Large-dev subset CSV path (1M)
  --runs <n>              Benchmark repetitions per scenario (default: 3)
  --threads <csv>         Parallel thread list (default: 1,2,4,8,16)
  --timeout-seconds <n>   Per scenario timeout (0 disables; default: 0)
  --dataset-path <path>   Run only one dataset path (overrides --small/--medium/--large)
  --dataset-label <name>  Label to use with --dataset-path (default: custom)
  --out-dir <path>        Raw benchmark output dir (default: results/raw/phase3_dev)
  --log-dir <path>        Log output dir (default: results/raw/logs)
  --optimization-step <label>  Label for optimization attribution (default: soa_encoded_hotloop)
  --allow-low-runs       Permit --runs < 3 and mark outputs as exploratory evidence
  --validate-serial       Enable serial parity validation during benchmark runs (default: off)
  --no-validate-serial    Disable serial parity validation during benchmark runs
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
    --optimization-step)
      OPTIMIZATION_STEP="$2"
      shift 2
      ;;
    --allow-low-runs)
      ALLOW_LOW_RUNS=1
      shift 1
      ;;
    --validate-serial)
      VALIDATE_SERIAL=1
      shift 1
      ;;
    --no-validate-serial)
      VALIDATE_SERIAL=0
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

if ! [[ "${RUNS}" =~ ^[0-9]+$ ]] || [[ "${RUNS}" -eq 0 ]]; then
  echo "[phase3-dev] --runs must be a positive integer" >&2
  exit 2
fi
if [[ "${RUNS}" -lt 3 ]]; then
  if [[ "${ALLOW_LOW_RUNS}" -ne 1 ]]; then
    echo "[phase3-dev] --runs < 3 is not deliverable-grade. Re-run with --allow-low-runs for exploratory batches." >&2
    exit 2
  fi
  EVIDENCE_TIER="exploratory"
fi
if ! [[ "${TIMEOUT_SECONDS}" =~ ^[0-9]+$ ]]; then
  echo "[phase3-dev] --timeout-seconds must be a non-negative integer" >&2
  exit 2
fi

if [[ ! -f "${SCENARIO_FILE}" ]]; then
  echo "[phase3-dev] missing scenario file: ${SCENARIO_FILE}" >&2
  exit 1
fi

if [[ ! -x "${SERIAL_BINARY}" || ! -x "${PARALLEL_BINARY}" || ! -x "${OPTIMIZED_BINARY}" ]]; then
  echo "[phase3-dev] building binaries..."
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
    echo "[phase3-dev] dataset not found: ${DATASET_PATH}" >&2
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
      echo "[phase3-dev] missing subset file. Provide --small/--medium/--large or check --subsets-dir." >&2
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
batch_utc,subset_label,dataset_path,expected_data_rows,actual_data_rows,serial_access_ok,parallel_access_ok,optimized_access_ok
CSV

cat > "${BATCH_MANIFEST}" <<CSV
batch_utc,git_branch,git_commit,optimization_step,evidence_tier,subset_label,scenario_name,mode,thread_list,dataset_path,benchmark_runs,validation_enabled,output_csv,log_file
CSV

cat > "${RPS_CSV}" <<CSV
batch_utc,git_branch,git_commit,optimization_step,subset_label,scenario_name,mode,thread_count,run_number,total_ms,rows_accepted,records_per_second
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
batch_utc,host,os_kernel,cpu_model,logical_cores,mem_total_kb,compiler,compiler_version,git_branch,git_commit,thread_list,benchmark_runs,validation_enabled,optimization_step,evidence_tier
CSV

printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" \
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
  "${RUNS}" \
  "${VALIDATE_SERIAL}" \
  "$(csv_escape "${OPTIMIZATION_STEP}")" \
  "${EVIDENCE_TIER}" >> "${ENV_CSV}"

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

append_scenario_args() {
  local -n out_ref=$1
  local threshold="$2"
  local start_epoch="$3"
  local end_epoch="$4"
  local borough="$5"
  local top_n="$6"
  local min_link_samples="$7"

  if [[ -n "${threshold}" ]]; then
    out_ref+=(--threshold "${threshold}")
  fi
  if [[ -n "${start_epoch}" ]]; then
    out_ref+=(--start-epoch "${start_epoch}")
  fi
  if [[ -n "${end_epoch}" ]]; then
    out_ref+=(--end-epoch "${end_epoch}")
  fi
  if [[ -n "${borough}" ]]; then
    out_ref+=(--borough "${borough}")
  fi
  if [[ -n "${top_n}" ]]; then
    out_ref+=(--top-n "${top_n}")
  fi
  if [[ -n "${min_link_samples}" ]]; then
    out_ref+=(--min-link-samples "${min_link_samples}")
  fi
}

run_access_probe() {
  local mode="$1"
  local path="$2"
  local tmp_csv
  tmp_csv="$(mktemp)"

  if [[ "${mode}" == "serial" ]]; then
    "${SERIAL_BINARY}" --traffic "${path}" --query summary --benchmark-runs 1 --benchmark-out "${tmp_csv}" >/dev/null 2>&1
  elif [[ "${mode}" == "parallel" ]]; then
    "${PARALLEL_BINARY}" --traffic "${path}" --query summary --threads 1 --benchmark-runs 1 --benchmark-out "${tmp_csv}" --validate-serial >/dev/null 2>&1
  else
    "${OPTIMIZED_BINARY}" --traffic "${path}" --query summary --execution serial --benchmark-runs 1 --benchmark-out "${tmp_csv}" --validate-serial >/dev/null 2>&1
  fi

  local rc=$?
  rm -f "${tmp_csv}"
  return ${rc}
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
      -v step="${OPTIMIZATION_STEP}" -v subset="${subset_label}" -v scenario="${scenario_name}" -v mode="${mode}" '
    NR == 1 {
      for (i = 1; i <= NF; ++i) {
        col[$i] = i;
      }
      if (!("thread_count" in col) || !("run_number" in col) || !("total_ms" in col) || !("rows_accepted" in col)) {
        print "[phase3-dev] benchmark CSV missing required columns in " FILENAME > "/dev/stderr";
        exit 2;
      }
      next;
    }
    NR > 1 {
      total_ms = $(col["total_ms"]) + 0.0;
      rows_accepted = $(col["rows_accepted"]) + 0.0;
      rps = (total_ms > 0.0) ? (rows_accepted / (total_ms / 1000.0)) : 0.0;
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%.6f,%s,%.6f\n",
             ts, branch, commit, step, subset, scenario, mode, $(col["thread_count"]), $(col["run_number"]), total_ms, $(col["rows_accepted"]), rps;
    }
  ' "${csv_file}" >> "${RPS_CSV}"
}

run_scenario() {
  local mode="$1"
  local subset_label="$2"
  local dataset_path="$3"
  local scenario_name="$4"
  local query_type="$5"
  local threshold="$6"
  local start_epoch="$7"
  local end_epoch="$8"
  local borough="$9"
  local top_n="${10}"
  local min_link_samples="${11}"

  local out_csv="${OUT_DIR}/${subset_label}_${scenario_name}_${mode}_${SAFE_BRANCH}_${GIT_COMMIT}_${TS}.csv"
  local log_file="${LOG_DIR}/phase3_dev_${subset_label}_${scenario_name}_${mode}_${TS}.log"

  local cmd=()
  local thread_field="1"

  if [[ "${mode}" == "serial" ]]; then
    cmd=(
      "${SERIAL_BINARY}"
      --traffic "${dataset_path}"
      --benchmark-runs "${RUNS}"
      --dataset-label "${subset_label}"
      --benchmark-out "${out_csv}"
    )
  elif [[ "${mode}" == "parallel" ]]; then
    thread_field="${THREAD_LIST//,/;}"
    cmd=(
      "${PARALLEL_BINARY}"
      --traffic "${dataset_path}"
      --thread-list "${THREAD_LIST}"
      --benchmark-runs "${RUNS}"
      --dataset-label "${subset_label}"
      --benchmark-out "${out_csv}"
    )
  elif [[ "${mode}" == "optimized_serial" ]]; then
    cmd=(
      "${OPTIMIZED_BINARY}"
      --traffic "${dataset_path}"
      --execution serial
      --benchmark-runs "${RUNS}"
      --dataset-label "${subset_label}"
      --benchmark-out "${out_csv}"
    )
  else
    thread_field="${THREAD_LIST//,/;}"
    cmd=(
      "${OPTIMIZED_BINARY}"
      --traffic "${dataset_path}"
      --execution parallel
      --threads 1
      --benchmark-runs "${RUNS}"
      --dataset-label "${subset_label}"
      --benchmark-out "${out_csv}"
    )
  fi

  if [[ "${query_type}" != "ingest_only" ]]; then
    cmd+=(--query "${query_type}")
  fi
  if [[ "${mode}" != "serial" && "${VALIDATE_SERIAL}" == "1" ]]; then
    cmd+=(--validate-serial)
  fi
  append_scenario_args cmd "${threshold}" "${start_epoch}" "${end_epoch}" "${borough}" "${top_n}" "${min_link_samples}"

  echo "[phase3-dev] mode=${mode} subset=${subset_label} scenario=${scenario_name}"

  if [[ "${mode}" == "optimized_parallel" ]]; then
    : > "${log_file}"
    IFS=',' read -r -a thread_array <<< "${THREAD_LIST}"
    rm -f "${out_csv}"
    for t in "${thread_array[@]}"; do
      local tcmd=("${cmd[@]}")
      tcmd+=(--threads "${t}")
      if [[ -f "${out_csv}" ]]; then
        tcmd+=(--benchmark-append)
      fi
      run_with_optional_timeout "${tcmd[@]}" >>"${log_file}" 2>&1
    done
  else
    run_with_optional_timeout "${cmd[@]}" >"${log_file}" 2>&1
  fi

  printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" \
    "${TS}" "${GIT_BRANCH}" "${GIT_COMMIT}" "${OPTIMIZATION_STEP}" "${EVIDENCE_TIER}" "${subset_label}" "${scenario_name}" \
    "${mode}" "${thread_field}" "${dataset_path}" "${RUNS}" "${VALIDATE_SERIAL}" "${out_csv}" "${log_file}" >> "${BATCH_MANIFEST}"

  emit_rps_rows "${out_csv}" "${subset_label}" "${scenario_name}" "${mode}"
}

for i in "${!LABELS[@]}"; do
  subset_label="${LABELS[$i]}"
  dataset_path="${PATHS[$i]}"
  expected_rows="$(calc_rows "${dataset_path}")"
  actual_rows="$(calc_rows "${dataset_path}")"

  serial_access_ok="false"
  parallel_access_ok="false"
  optimized_access_ok="false"

  if run_access_probe "serial" "${dataset_path}"; then
    serial_access_ok="true"
  fi
  if run_access_probe "parallel" "${dataset_path}"; then
    parallel_access_ok="true"
  fi
  if run_access_probe "optimized" "${dataset_path}"; then
    optimized_access_ok="true"
  fi

  printf "%s,%s,%s,%s,%s,%s,%s,%s\n" \
    "${TS}" "${subset_label}" "${dataset_path}" "${expected_rows}" "${actual_rows}" \
    "${serial_access_ok}" "${parallel_access_ok}" "${optimized_access_ok}" >> "${SUBSET_MANIFEST}"
done

for i in "${!LABELS[@]}"; do
  subset_label="${LABELS[$i]}"
  dataset_path="${PATHS[$i]}"

  while IFS=',' read -r scenario_name query_type threshold start_epoch end_epoch borough top_n min_link_samples; do
    if [[ "${scenario_name}" == "scenario_name" ]]; then
      continue
    fi
    run_scenario "serial" "${subset_label}" "${dataset_path}" "${scenario_name}" "${query_type}" "${threshold}" "${start_epoch}" "${end_epoch}" "${borough}" "${top_n}" "${min_link_samples}"
    run_scenario "parallel" "${subset_label}" "${dataset_path}" "${scenario_name}" "${query_type}" "${threshold}" "${start_epoch}" "${end_epoch}" "${borough}" "${top_n}" "${min_link_samples}"
    run_scenario "optimized_serial" "${subset_label}" "${dataset_path}" "${scenario_name}" "${query_type}" "${threshold}" "${start_epoch}" "${end_epoch}" "${borough}" "${top_n}" "${min_link_samples}"
    run_scenario "optimized_parallel" "${subset_label}" "${dataset_path}" "${scenario_name}" "${query_type}" "${threshold}" "${start_epoch}" "${end_epoch}" "${borough}" "${top_n}" "${min_link_samples}"
  done < "${SCENARIO_FILE}"
done

echo "[phase3-dev] complete"
echo "[phase3-dev] subset_manifest=${SUBSET_MANIFEST}"
echo "[phase3-dev] batch_manifest=${BATCH_MANIFEST}"
echo "[phase3-dev] rps=${RPS_CSV}"
echo "[phase3-dev] env=${ENV_CSV}"
