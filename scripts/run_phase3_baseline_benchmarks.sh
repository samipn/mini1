#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${ROOT_DIR}/results/raw/phase3_baseline"
LOG_DIR="${ROOT_DIR}/results/raw/logs"
RUNS=10
THREAD_LIST="1,2,4,8,16"
VALIDATE_SERIAL=0

usage() {
  cat <<'USAGE'
Usage: scripts/run_phase3_baseline_benchmarks.sh [options]

Run a Phase 3 baseline benchmark batch in a directory separate from phase3_dev artifacts.
This is a thin wrapper over scripts/run_phase3_dev_benchmarks.sh with baseline defaults.

Options:
  --runs <n>                 Benchmark repetitions per scenario (default: 10)
  --threads <csv>            Thread list (default: 1,2,4,8,16)
  --out-dir <path>           Output dir (default: results/raw/phase3_baseline)
  --log-dir <path>           Log dir (default: results/raw/logs)
  --validate-serial          Enable serial parity validation during benchmark runs
  --no-validate-serial       Disable serial parity validation (default)
  --small <path>             Override small subset CSV path
  --medium <path>            Override medium subset CSV path
  --large <path>             Override large subset CSV path
  --subsets-dir <path>       Override subset directory
  --optimization-step <label> Override optimization step label
  --help                     Show this help
USAGE
}

forward_args=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs)
      RUNS="$2"
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
    --log-dir)
      LOG_DIR="$2"
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
    --small|--medium|--large|--subsets-dir|--optimization-step)
      forward_args+=("$1" "$2")
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

mkdir -p "${OUT_DIR}" "${LOG_DIR}"

cmd=(
  "${ROOT_DIR}/scripts/run_phase3_dev_benchmarks.sh"
  --runs "${RUNS}"
  --threads "${THREAD_LIST}"
  --out-dir "${OUT_DIR}"
  --log-dir "${LOG_DIR}"
  --optimization-step "phase3_baseline"
)

if [[ "${VALIDATE_SERIAL}" == "1" ]]; then
  cmd+=(--validate-serial)
else
  cmd+=(--no-validate-serial)
fi

if [[ "${#forward_args[@]}" -gt 0 ]]; then
  cmd+=("${forward_args[@]}")
fi

echo "[phase3-baseline] invoking: ${cmd[*]}"
"${cmd[@]}"

