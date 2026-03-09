#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RUNNER="${ROOT_DIR}/scripts/run_phase2_dev_benchmarks.sh"

SUBSETS_DIR="${ROOT_DIR}/data/subsets"
OUT_DIR="${ROOT_DIR}/results/raw/phase2_baseline"
LOG_DIR="${ROOT_DIR}/results/raw/logs"
RUNS=10
THREAD_LIST="1,2,4,8,16"
ALLOW_UNDER_MIN_RUNS=false

SMALL_PATH=""
MEDIUM_PATH=""
LARGE_PATH=""

usage() {
  cat <<'USAGE'
Usage: scripts/run_phase2_baseline_benchmarks.sh [options]

Run serial+parallel Phase 2 baseline benchmarks with baseline-grade repeat policy.

Options:
  --subsets-dir <path>      Directory containing subset CSV files (default: data/subsets)
  --small <path>            Small subset CSV path (10k)
  --medium <path>           Medium subset CSV path (100k)
  --large <path>            Large-dev subset CSV path (1M)
  --threads <csv>           Parallel thread list (default: 1,2,4,8,16)
  --runs <n>                Benchmark repetitions per scenario (default: 10)
  --allow-under-min-runs    Allow runs < 10 (smoke/debug only)
  --out-dir <path>          Raw output dir (default: results/raw/phase2_baseline)
  --log-dir <path>          Log output dir (default: results/raw/logs)
  --help                    Show this help
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
    --runs)
      RUNS="$2"
      shift 2
      ;;
    --allow-under-min-runs)
      ALLOW_UNDER_MIN_RUNS=true
      shift
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
  echo "[phase2-baseline] --runs must be a positive integer." >&2
  exit 2
fi
if [[ "${RUNS}" -lt 1 ]]; then
  echo "[phase2-baseline] --runs must be >= 1." >&2
  exit 2
fi
if [[ "${RUNS}" -lt 10 && "${ALLOW_UNDER_MIN_RUNS}" != "true" ]]; then
  echo "[phase2-baseline] --runs must be >= 10 for baseline-grade evidence." >&2
  echo "[phase2-baseline] use --allow-under-min-runs only for smoke/debug runs." >&2
  exit 2
fi

if [[ ! -x "${RUNNER}" ]]; then
  echo "[phase2-baseline] missing runner script: ${RUNNER}" >&2
  exit 1
fi

cmd=(
  "${RUNNER}"
  --subsets-dir "${SUBSETS_DIR}"
  --threads "${THREAD_LIST}"
  --runs "${RUNS}"
  --out-dir "${OUT_DIR}"
  --log-dir "${LOG_DIR}"
)

if [[ -n "${SMALL_PATH}" ]]; then
  cmd+=(--small "${SMALL_PATH}")
fi
if [[ -n "${MEDIUM_PATH}" ]]; then
  cmd+=(--medium "${MEDIUM_PATH}")
fi
if [[ -n "${LARGE_PATH}" ]]; then
  cmd+=(--large "${LARGE_PATH}")
fi

echo "[phase2-baseline] invoking phase2 benchmark runner"
echo "[phase2-baseline] out_dir=${OUT_DIR} runs=${RUNS} threads=${THREAD_LIST}"
"${cmd[@]}"
