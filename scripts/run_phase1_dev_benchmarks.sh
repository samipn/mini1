#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BINARY="${BUILD_DIR}/run_serial"
SCENARIO_FILE="${ROOT_DIR}/configs/phase1_dev_scenarios.csv"

SUBSETS_DIR="${ROOT_DIR}/data/subsets"
OUT_DIR="${ROOT_DIR}/results/raw/phase1_dev"
LOG_DIR="${ROOT_DIR}/results/raw/logs"
RUNS=3
DATASET_PATH=""
DATASET_LABEL=""

SMALL_PATH=""
MEDIUM_PATH=""
LARGE_PATH=""

usage() {
  cat <<'EOF'
Usage: scripts/run_phase1_dev_benchmarks.sh [options]

Run stable Phase 1 development benchmark scenarios on subset datasets.

Options:
  --dataset-path <path>   Run only one dataset path (overrides subset auto-detect)
  --dataset-label <name>  Label to use with --dataset-path (default: custom)
  --subsets-dir <path>    Directory containing subset CSV files (default: data/subsets)
  --small <path>          Small subset CSV path (10k)
  --medium <path>         Medium subset CSV path (100k)
  --large <path>          Large-dev subset CSV path (1M)
  --runs <n>              Benchmark repetitions per scenario (default: 3)
  --out-dir <path>        Raw benchmark CSV output dir (default: results/raw/phase1_dev)
  --log-dir <path>        Log output dir (default: results/raw/logs)
  --help                  Show this help

Scenario definitions:
  configs/phase1_dev_scenarios.csv
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --dataset-path)
      DATASET_PATH="$2"
      shift 2
      ;;
    --dataset-label)
      DATASET_LABEL="$2"
      shift 2
      ;;
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

if [[ ! -f "${SCENARIO_FILE}" ]]; then
  echo "[phase1-dev] missing scenario definition file: ${SCENARIO_FILE}" >&2
  exit 1
fi

if [[ ! -x "${BINARY}" ]]; then
  echo "[phase1-dev] building run_serial..."
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
  cmake --build "${BUILD_DIR}" -j
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
    echo "[phase1-dev] dataset not found: ${DATASET_PATH}" >&2
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
      echo "[phase1-dev] missing subset file. Provide --small/--medium/--large or check --subsets-dir." >&2
      exit 1
    fi
  done

  LABELS+=("small" "medium" "large_dev")
  PATHS+=("${SMALL_PATH}" "${MEDIUM_PATH}" "${LARGE_PATH}")
fi

TS="$(date -u +%Y%m%dT%H%M%SZ)"
GIT_BRANCH="$(git -C "${ROOT_DIR}" branch --show-current 2>/dev/null || echo "unknown")"
GIT_COMMIT="$(git -C "${ROOT_DIR}" rev-parse --short HEAD 2>/dev/null || echo "unknown")"
MANIFEST_CSV="${OUT_DIR}/batch_${TS}_manifest.csv"
RPS_CSV="${OUT_DIR}/batch_${TS}_records_per_second.csv"

cat > "${MANIFEST_CSV}" <<EOF
batch_utc,git_branch,git_commit,subset_label,scenario_name,dataset_path,benchmark_runs,output_csv,log_file
EOF

cat > "${RPS_CSV}" <<EOF
batch_utc,git_branch,git_commit,dataset_label,query_type,run_number,total_ms,rows_accepted,records_per_second
EOF

echo "[phase1-dev] start ts=${TS} runs=${RUNS}"
echo "[phase1-dev] scenarios=${SCENARIO_FILE}"
echo "[phase1-dev] git_branch=${GIT_BRANCH} git_commit=${GIT_COMMIT}"

run_one_scenario() {
  local subset_label="$1"
  local dataset_path="$2"
  local scenario_name="$3"
  local query_type="$4"
  local threshold="$5"
  local start_epoch="$6"
  local end_epoch="$7"
  local borough="$8"
  local top_n="$9"
  local min_link_samples="${10}"

  local out_csv="${OUT_DIR}/${subset_label}_${scenario_name}.csv"
  local log_file="${LOG_DIR}/phase1_dev_${subset_label}_${scenario_name}_${TS}.log"

  local cmd=(
    "${BINARY}"
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
  if [[ -n "${top_n}" ]]; then
    cmd+=(--top-n "${top_n}")
  fi
  if [[ -n "${min_link_samples}" ]]; then
    cmd+=(--min-link-samples "${min_link_samples}")
  fi

  echo "[phase1-dev] run subset=${subset_label} scenario=${scenario_name}"
  "${cmd[@]}" > "${log_file}" 2>&1

  printf "%s,%s,%s,%s,%s,%s,%s,%s,%s\n" \
    "${TS}" \
    "${GIT_BRANCH}" \
    "${GIT_COMMIT}" \
    "${subset_label}" \
    "${scenario_name}" \
    "${dataset_path}" \
    "${RUNS}" \
    "${out_csv}" \
    "${log_file}" >> "${MANIFEST_CSV}"

  awk -F, -v ts="${TS}" -v branch="${GIT_BRANCH}" -v commit="${GIT_COMMIT}" '
    NR == 1 {
      for (i = 1; i <= NF; ++i) {
        col[$i] = i;
      }
      if (!("dataset_label" in col) || !("query_type" in col) || !("run_number" in col) ||
          !("total_ms" in col) || !("rows_accepted" in col)) {
        print "[phase1-dev] benchmark CSV missing required columns in " FILENAME > "/dev/stderr";
        exit 2;
      }
      next;
    }
    NR > 1 {
      total_ms = $(col["total_ms"]) + 0.0;
      rows_accepted = $(col["rows_accepted"]) + 0.0;
      rps = (total_ms > 0.0) ? (rows_accepted / (total_ms / 1000.0)) : 0.0;
      printf "%s,%s,%s,%s,%s,%s,%.6f,%s,%.6f\n",
             ts, branch, commit, $(col["dataset_label"]), $(col["query_type"]), $(col["run_number"]), total_ms, $(col["rows_accepted"]), rps;
    }
  ' "${out_csv}" >> "${RPS_CSV}"

  local avg_ingest
  local avg_query
  local avg_total
  avg_ingest="$(grep -E '^  avg_ingest_ms=' "${log_file}" | tail -n 1 | cut -d= -f2- || true)"
  avg_query="$(grep -E '^  avg_query_ms=' "${log_file}" | tail -n 1 | cut -d= -f2- || true)"
  avg_total="$(grep -E '^  avg_total_ms=' "${log_file}" | tail -n 1 | cut -d= -f2- || true)"
  echo "  avg_ingest_ms=${avg_ingest:-n/a} avg_query_ms=${avg_query:-n/a} avg_total_ms=${avg_total:-n/a}"
  echo "  csv=${out_csv}"
  echo "  log=${log_file}"
}

for i in "${!LABELS[@]}"; do
  subset_label="${LABELS[$i]}"
  dataset_path="${PATHS[$i]}"

  while IFS=',' read -r scenario_name query_type threshold start_epoch end_epoch borough top_n min_link_samples; do
    if [[ "${scenario_name}" == "scenario_name" ]]; then
      continue
    fi
    run_one_scenario \
      "${subset_label}" \
      "${dataset_path}" \
      "${scenario_name}" \
      "${query_type}" \
      "${threshold}" \
      "${start_epoch}" \
      "${end_epoch}" \
      "${borough}" \
      "${top_n}" \
      "${min_link_samples}"
  done < "${SCENARIO_FILE}"
done

echo "[phase1-dev] complete."
echo "[phase1-dev] raw_csv_dir=${OUT_DIR}"
echo "[phase1-dev] log_dir=${LOG_DIR}"
echo "[phase1-dev] batch_manifest=${MANIFEST_CSV}"
echo "[phase1-dev] batch_rps=${RPS_CSV}"
