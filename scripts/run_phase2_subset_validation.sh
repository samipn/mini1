#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
SERIAL_BINARY="${BUILD_DIR}/run_serial"
PARALLEL_BINARY="${BUILD_DIR}/run_parallel"
SCENARIO_FILE="${ROOT_DIR}/configs/phase2_dev_scenarios.conf"

SUBSETS_DIR="${ROOT_DIR}/data/subsets"
OUT_DIR="${ROOT_DIR}/results/raw/phase2_dev/validation"
THREAD_LIST="1,2,4,8,16"

SMALL_PATH=""
MEDIUM_PATH=""
LARGE_PATH=""

usage() {
  cat <<'USAGE'
Usage: scripts/run_phase2_subset_validation.sh [options]

Run serial and parallel validation back-to-back on subset datasets.

Options:
  --subsets-dir <path>    Directory containing subset CSV files (default: data/subsets)
  --small <path>          Small subset CSV path (10k)
  --medium <path>         Medium subset CSV path (100k)
  --large <path>          Large-dev subset CSV path (1M)
  --threads <csv>         Thread list for parallel validation (default: 1,2,4,8,16)
  --out-dir <path>        Validation output dir (default: results/raw/phase2_dev/validation)
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
  echo "[phase2-validate] missing scenario file: ${SCENARIO_FILE}" >&2
  exit 1
fi

if [[ ! -x "${SERIAL_BINARY}" || ! -x "${PARALLEL_BINARY}" ]]; then
  echo "[phase2-validate] building run_serial/run_parallel..."
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
    echo "[phase2-validate] missing subset file. Provide --small/--medium/--large or check --subsets-dir." >&2
    exit 1
  fi
done

TS="$(date -u +%Y%m%dT%H%M%SZ)"
RESULT_CSV="${OUT_DIR}/validation_${TS}.csv"

cat > "${RESULT_CSV}" <<CSV
batch_utc,subset_label,scenario_name,thread_count,serial_result_count,parallel_result_count,serial_avg_speed_mph,serial_avg_travel_time_seconds,parallel_avg_speed_mph,parallel_avg_travel_time_seconds,serial_match,status,serial_csv,parallel_csv,note
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

emit_parallel_rows() {
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

declare -a LABELS=("small" "medium" "large_dev")
declare -a PATHS=("${SMALL_PATH}" "${MEDIUM_PATH}" "${LARGE_PATH}")

for i in "${!LABELS[@]}"; do
  subset_label="${LABELS[$i]}"
  dataset_path="${PATHS[$i]}"

  while IFS=',' read -r scenario_name query_type threshold start_epoch end_epoch borough; do
    if [[ "${scenario_name}" == "scenario_name" ]]; then
      continue
    fi

    serial_csv="${OUT_DIR}/${subset_label}_${scenario_name}_serial_${TS}.csv"
    parallel_csv="${OUT_DIR}/${subset_label}_${scenario_name}_parallel_${TS}.csv"

    serial_cmd=(
      "${SERIAL_BINARY}"
      --traffic "${dataset_path}"
      --query "${query_type}"
      --benchmark-runs 1
      --dataset-label "${subset_label}"
      --benchmark-out "${serial_csv}"
    )
    parallel_cmd=(
      "${PARALLEL_BINARY}"
      --traffic "${dataset_path}"
      --query "${query_type}"
      --benchmark-runs 1
      --dataset-label "${subset_label}"
      --thread-list "${THREAD_LIST}"
      --benchmark-out "${parallel_csv}"
      --validate-serial
    )

    if [[ -n "${threshold}" ]]; then
      serial_cmd+=(--threshold "${threshold}")
      parallel_cmd+=(--threshold "${threshold}")
    fi
    if [[ -n "${start_epoch}" ]]; then
      serial_cmd+=(--start-epoch "${start_epoch}")
      parallel_cmd+=(--start-epoch "${start_epoch}")
    fi
    if [[ -n "${end_epoch}" ]]; then
      serial_cmd+=(--end-epoch "${end_epoch}")
      parallel_cmd+=(--end-epoch "${end_epoch}")
    fi
    if [[ -n "${borough}" ]]; then
      serial_cmd+=(--borough "${borough}")
      parallel_cmd+=(--borough "${borough}")
    fi

    echo "[phase2-validate] subset=${subset_label} scenario=${scenario_name}"

    if ! "${serial_cmd[@]}" >/dev/null 2>&1; then
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" \
        "${TS}" "${subset_label}" "${scenario_name}" "n/a" "n/a" "n/a" "n/a" "n/a" "n/a" "n/a" \
        "false" "failed" "${serial_csv}" "${parallel_csv}" "serial command failed" >> "${RESULT_CSV}"
      continue
    fi

    if ! "${parallel_cmd[@]}" >/dev/null 2>&1; then
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" \
        "${TS}" "${subset_label}" "${scenario_name}" "n/a" "n/a" "n/a" "n/a" "n/a" "n/a" "n/a" \
        "false" "failed" "${serial_csv}" "${parallel_csv}" "parallel command failed or serial parity mismatch" >> "${RESULT_CSV}"
      continue
    fi

    serial_count="$(read_csv_field "${serial_csv}" "result_count")"
    serial_avg_speed="$(read_csv_field "${serial_csv}" "average_speed_mph")"
    serial_avg_travel="$(read_csv_field "${serial_csv}" "average_travel_time_seconds")"

    while IFS=',' read -r thread_count parallel_count parallel_avg_speed parallel_avg_travel serial_match; do
      status="ok"
      note=""
      if [[ "${serial_match}" != "true" ]]; then
        status="failed"
        note="serial_match=false"
      fi
      if [[ "${parallel_count}" != "${serial_count}" ]]; then
        status="failed"
        note="result_count mismatch"
      fi

      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" \
        "${TS}" "${subset_label}" "${scenario_name}" "${thread_count}" "${serial_count}" "${parallel_count}" \
        "${serial_avg_speed}" "${serial_avg_travel}" "${parallel_avg_speed}" "${parallel_avg_travel}" \
        "${serial_match}" "${status}" "${serial_csv}" "${parallel_csv}" "${note}" >> "${RESULT_CSV}"
    done < <(emit_parallel_rows "${parallel_csv}")

  done < "${SCENARIO_FILE}"
done

echo "[phase2-validate] complete"
echo "[phase2-validate] result_csv=${RESULT_CSV}"
