#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PY_SCRIPT="${ROOT_DIR}/scripts/make_subsets.py"

usage() {
  cat <<'EOF'
Usage: scripts/make_subsets.sh --input <traffic.csv> [--output-dir <dir>] [--sizes <n1> <n2> ...]

Create deterministic subset CSV files for Phase 1 development benchmarking.

Options:
  --input <path>        Source traffic CSV path (required)
  --output-dir <path>   Output directory (default: data/subsets)
  --sizes <list...>     Subset sizes (default: 10000 100000 1000000)
  --help                Show this help message

Example:
  scripts/make_subsets.sh \
    --input /data/raw/i4gi-tjb9.csv \
    --output-dir data/subsets \
    --sizes 10000 100000 1000000
EOF
}

if [[ ! -f "${PY_SCRIPT}" ]]; then
  echo "[make_subsets.sh] missing script: ${PY_SCRIPT}" >&2
  exit 1
fi

if [[ $# -eq 0 ]]; then
  usage
  exit 2
fi

# Pass all arguments through to the Python implementation.
if [[ "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

exec python3 "${PY_SCRIPT}" "$@"
