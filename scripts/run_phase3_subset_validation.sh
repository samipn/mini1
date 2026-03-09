#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
SERIAL_BINARY="${BUILD_DIR}/run_serial"
PARALLEL_BINARY="${BUILD_DIR}/run_parallel"
OPTIMIZED_BINARY="${BUILD_DIR}/run_optimized"
SCENARIO_FILE="${ROOT_DIR}/configs/phase3_dev_scenarios.conf"

SUBSETS_DIR="${ROOT_DIR}/data/subsets"
OUT_DIR="${ROOT_DIR}/results/raw/phase3_dev/validation"
THREAD_LIST="1,2,4,8,16"

SMALL_PATH=""
MEDIUM_PATH=""
LARGE_PATH=""

usage() {
  cat <<'USAGE'
Usage: scripts/run_phase3_subset_validation.sh [options]

Run serial, parallel, optimized-serial, and optimized-parallel validation on subset datasets.

Options:
  --subsets-dir <path>    Directory containing subset CSV files (default: data/subsets)
  --small <path>          Small subset CSV path (10k)
  --medium <path>         Medium subset CSV path (100k)
  --large <path>          Large-dev subset CSV path (1M)
  --threads <csv>         Thread list for parallel modes (default: 1,2,4,8,16)
  --out-dir <path>        Validation output dir (default: results/raw/phase3_dev/validation)
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
    --threads)
      THREAD_LIST="$2"
      shift 2
      ;;
    --out-dir)
      OUT_DIR="$2"
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

if [[ ! -f "${SCENARIO_FILE}" ]]; then
  echo "[phase3-validate] missing scenario file: ${SCENARIO_FILE}" >&2
  exit 1
fi

if [[ ! -x "${SERIAL_BINARY}" || ! -x "${PARALLEL_BINARY}" || ! -x "${OPTIMIZED_BINARY}" ]]; then
  echo "[phase3-validate] building binaries..."
  "${ROOT_DIR}/scripts/configure_openmp_build.sh" --build-dir "${BUILD_DIR}" --build-type Release
fi

mkdir -p "${OUT_DIR}"

find_single_match() {
  local pattern="$1"
  find "${SUBSETS_DIR}" -maxdepth 1 -type f -name "${pattern}" | sort | head -n 1
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

for required in "${SMALL_PATH}" "${MEDIUM_PATH}" "${LARGE_PATH}"; do
  if [[ -z "${required}" || ! -f "${required}" ]]; then
    echo "[phase3-validate] missing subset file. Provide --small/--medium/--large or check --subsets-dir." >&2
    exit 1
  fi
done

TS="$(date -u +%Y%m%dT%H%M%SZ)"
RESULT_CSV="${OUT_DIR}/validation_${TS}.csv"

cat > "${RESULT_CSV}" <<CSV
batch_utc,subset_label,scenario_name,mode,thread_count,serial_result_count,candidate_result_count,serial_avg_speed_mph,serial_avg_travel_time_seconds,candidate_avg_speed_mph,candidate_avg_travel_time_seconds,serial_match,status,candidate_csv,note
CSV

read_csv_field() {
  local csv_file="$1"
  local field_name="$2"
  awk -F, -v field="${field_name}" '
    NR==1 {
      for (i=1; i<=NF; ++i) {
        col[$i]=i;
      }
      next;
    }
    NR==2 {
      if (!(field in col)) {
        exit 2;
      }
      print $(col[field]);
      exit 0;
    }
  ' "${csv_file}"
}

emit_candidate_rows() {
  local csv_file="$1"
  awk -F, '
    NR==1 {
      for (i=1; i<=NF; ++i) {
        col[$i]=i;
      }
      req1="thread_count"; req2="result_count"; req3="average_speed_mph"; req4="average_travel_time_seconds"; req5="serial_match";
      if (!(req1 in col) || !(req2 in col) || !(req3 in col) || !(req4 in col) || !(req5 in col)) {
        exit 2;
      }
      next;
    }
    NR>1 {
      print $(col["thread_count"])","$(col["result_count"])","$(col["average_speed_mph"])","$(col["average_travel_time_seconds"])","$(col["serial_match"]);
    }
  ' "${csv_file}"
}

read_csv_thread_field() {
  local csv_file="$1"
  local thread_count="$2"
  local field_name="$3"
  awk -F, -v t="${thread_count}" -v field="${field_name}" '
    NR==1 {
      for (i=1; i<=NF; ++i) {
        col[$i]=i;
      }
      if (!("thread_count" in col) || !(field in col)) {
        exit 2;
      }
      next;
    }
    NR>1 && $(col["thread_count"]) == t {
      val = $(col[field]);
    }
    END {
      print val;
    }
  ' "${csv_file}"
}

write_row() {
  printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" "$@" >> "${RESULT_CSV}"
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

declare -a LABELS=("small" "medium" "large_dev")
declare -a PATHS=("${SMALL_PATH}" "${MEDIUM_PATH}" "${LARGE_PATH}")

for i in "${!LABELS[@]}"; do
  subset_label="${LABELS[$i]}"
  dataset_path="${PATHS[$i]}"

  while IFS=',' read -r scenario_name query_type threshold start_epoch end_epoch borough top_n min_link_samples; do
    if [[ "${scenario_name}" == "scenario_name" ]]; then
      continue
    fi

    serial_csv="${OUT_DIR}/${subset_label}_${scenario_name}_serial_${TS}.csv"
    serial_cmd=(
      "${SERIAL_BINARY}"
      --traffic "${dataset_path}"
      --query "${query_type}"
      --benchmark-runs 1
      --dataset-label "${subset_label}"
      --benchmark-out "${serial_csv}"
    )
    append_scenario_args serial_cmd "${threshold}" "${start_epoch}" "${end_epoch}" "${borough}" "${top_n}" "${min_link_samples}"

    if ! "${serial_cmd[@]}" >/dev/null 2>&1; then
      write_row "${TS}" "${subset_label}" "${scenario_name}" "serial" "1" "n/a" "n/a" "n/a" "n/a" "n/a" "n/a" "false" "failed" "${serial_csv}" "serial command failed"
      continue
    fi

    serial_count="$(read_csv_field "${serial_csv}" "result_count")"
    serial_avg_speed="$(read_csv_field "${serial_csv}" "average_speed_mph")"
    serial_avg_travel="$(read_csv_field "${serial_csv}" "average_travel_time_seconds")"

    parallel_csv="${OUT_DIR}/${subset_label}_${scenario_name}_parallel_${TS}.csv"
    parallel_cmd=(
      "${PARALLEL_BINARY}"
      --traffic "${dataset_path}"
      --query "${query_type}"
      --thread-list "${THREAD_LIST}"
      --benchmark-runs 1
      --dataset-label "${subset_label}"
      --benchmark-out "${parallel_csv}"
      --validate-serial
    )
    append_scenario_args parallel_cmd "${threshold}" "${start_epoch}" "${end_epoch}" "${borough}" "${top_n}" "${min_link_samples}"

    if "${parallel_cmd[@]}" >/dev/null 2>&1; then
      while IFS=',' read -r thread_count candidate_count candidate_avg_speed candidate_avg_travel serial_match; do
        status="ok"
        note=""
        if [[ "${serial_match}" != "true" || "${candidate_count}" != "${serial_count}" ]]; then
          status="failed"
          note="parallel mismatch"
        fi
        write_row "${TS}" "${subset_label}" "${scenario_name}" "parallel" "${thread_count}" "${serial_count}" "${candidate_count}" "${serial_avg_speed}" "${serial_avg_travel}" "${candidate_avg_speed}" "${candidate_avg_travel}" "${serial_match}" "${status}" "${parallel_csv}" "${note}"
      done < <(emit_candidate_rows "${parallel_csv}")
    else
      write_row "${TS}" "${subset_label}" "${scenario_name}" "parallel" "n/a" "${serial_count}" "n/a" "${serial_avg_speed}" "${serial_avg_travel}" "n/a" "n/a" "false" "failed" "${parallel_csv}" "parallel command failed"
    fi

    optimized_serial_csv="${OUT_DIR}/${subset_label}_${scenario_name}_optimized_serial_${TS}.csv"
    optimized_serial_cmd=(
      "${OPTIMIZED_BINARY}"
      --traffic "${dataset_path}"
      --query "${query_type}"
      --execution serial
      --benchmark-runs 1
      --dataset-label "${subset_label}"
      --benchmark-out "${optimized_serial_csv}"
      --validate-serial
    )
    append_scenario_args optimized_serial_cmd "${threshold}" "${start_epoch}" "${end_epoch}" "${borough}" "${top_n}" "${min_link_samples}"

    if "${optimized_serial_cmd[@]}" >/dev/null 2>&1; then
      while IFS=',' read -r thread_count candidate_count candidate_avg_speed candidate_avg_travel serial_match; do
        status="ok"
        note=""
        if [[ "${serial_match}" != "true" || "${candidate_count}" != "${serial_count}" ]]; then
          status="failed"
          note="optimized serial mismatch"
        fi
        write_row "${TS}" "${subset_label}" "${scenario_name}" "optimized_serial" "${thread_count}" "${serial_count}" "${candidate_count}" "${serial_avg_speed}" "${serial_avg_travel}" "${candidate_avg_speed}" "${candidate_avg_travel}" "${serial_match}" "${status}" "${optimized_serial_csv}" "${note}"
      done < <(emit_candidate_rows "${optimized_serial_csv}")
    else
      write_row "${TS}" "${subset_label}" "${scenario_name}" "optimized_serial" "1" "${serial_count}" "n/a" "${serial_avg_speed}" "${serial_avg_travel}" "n/a" "n/a" "false" "failed" "${optimized_serial_csv}" "optimized serial command failed"
    fi

    optimized_parallel_csv="${OUT_DIR}/${subset_label}_${scenario_name}_optimized_parallel_${TS}.csv"
    IFS=',' read -r -a thread_array <<< "${THREAD_LIST}"
    for thread_count in "${thread_array[@]}"; do
      optimized_parallel_cmd=(
        "${OPTIMIZED_BINARY}"
        --traffic "${dataset_path}"
        --query "${query_type}"
        --execution parallel
        --threads "${thread_count}"
        --benchmark-runs 1
        --dataset-label "${subset_label}"
        --benchmark-out "${optimized_parallel_csv}"
        --benchmark-append
        --validate-serial
      )
      append_scenario_args optimized_parallel_cmd "${threshold}" "${start_epoch}" "${end_epoch}" "${borough}" "${top_n}" "${min_link_samples}"

      if "${optimized_parallel_cmd[@]}" >/dev/null 2>&1; then
        candidate_count="$(read_csv_thread_field "${optimized_parallel_csv}" "${thread_count}" "result_count")"
        candidate_avg_speed="$(read_csv_thread_field "${optimized_parallel_csv}" "${thread_count}" "average_speed_mph")"
        candidate_avg_travel="$(read_csv_thread_field "${optimized_parallel_csv}" "${thread_count}" "average_travel_time_seconds")"
        serial_match="$(read_csv_thread_field "${optimized_parallel_csv}" "${thread_count}" "serial_match")"
        status="ok"
        note=""
        if [[ "${serial_match}" != "true" || "${candidate_count}" != "${serial_count}" ]]; then
          status="failed"
          note="optimized parallel mismatch"
        fi
        write_row "${TS}" "${subset_label}" "${scenario_name}" "optimized_parallel" "${thread_count}" "${serial_count}" "${candidate_count}" "${serial_avg_speed}" "${serial_avg_travel}" "${candidate_avg_speed}" "${candidate_avg_travel}" "${serial_match}" "${status}" "${optimized_parallel_csv}" "${note}"
      else
        write_row "${TS}" "${subset_label}" "${scenario_name}" "optimized_parallel" "${thread_count}" "${serial_count}" "n/a" "${serial_avg_speed}" "${serial_avg_travel}" "n/a" "n/a" "false" "failed" "${optimized_parallel_csv}" "optimized parallel command failed"
      fi
    done

  done < "${SCENARIO_FILE}"
done

echo "[phase3-validate] complete"
echo "[phase3-validate] result_csv=${RESULT_CSV}"
