#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DRY_RUN=0

usage() {
  cat <<'USAGE'
Usage: scripts/clean_benchmark_outputs.sh [--dry-run]

Remove generated benchmark artifacts for phase1/phase2/phase3 dev outputs while preserving placeholders (.gitkeep).

Targets:
  results/raw/phase1_dev
  results/raw/phase2_dev
  results/raw/phase3_dev
  results/raw/logs
  results/tables/phase1_dev
  results/tables/phase2_dev
  results/tables/phase3_dev
  results/graphs/phase1_dev
  results/graphs/phase2_dev
  results/graphs/phase3_dev
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --dry-run)
      DRY_RUN=1
      shift
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

clean_dir() {
  local d="$1"
  if [[ ! -d "${d}" ]]; then
    return 0
  fi
  if [[ "${DRY_RUN}" == "1" ]]; then
    find "${d}" -mindepth 1 \( -name .gitkeep -o -name .gitignore \) -prune -o -print
    return 0
  fi
  find "${d}" -mindepth 1 \( -name .gitkeep -o -name .gitignore \) -prune -o -exec rm -rf {} +
}

echo "[clean-benchmarks] start dry_run=${DRY_RUN}"
clean_dir "${ROOT_DIR}/results/raw/phase1_dev"
clean_dir "${ROOT_DIR}/results/raw/phase2_dev"
clean_dir "${ROOT_DIR}/results/raw/phase3_dev"
clean_dir "${ROOT_DIR}/results/raw/logs"
clean_dir "${ROOT_DIR}/results/tables/phase1_dev"
clean_dir "${ROOT_DIR}/results/tables/phase2_dev"
clean_dir "${ROOT_DIR}/results/tables/phase3_dev"
clean_dir "${ROOT_DIR}/results/graphs/phase1_dev"
clean_dir "${ROOT_DIR}/results/graphs/phase2_dev"
clean_dir "${ROOT_DIR}/results/graphs/phase3_dev"
echo "[clean-benchmarks] complete"
