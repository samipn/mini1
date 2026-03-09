#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BUILD_TYPE="Release"
COMPILER="${CXX:-g++}"

usage() {
  cat <<'USAGE'
Usage: scripts/configure_openmp_build.sh [options]

Configure and build with GCC+OpenMP guarantees.
If an existing build cache uses a different compiler, cache files are reset.

Options:
  --build-dir <path>      Build directory (default: ./build)
  --build-type <type>     CMake build type (default: Release)
  --compiler <path/name>  C++ compiler (default: $CXX or g++)
  --help                  Show this help
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir)
      BUILD_DIR="$2"
      shift 2
      ;;
    --build-type)
      BUILD_TYPE="$2"
      shift 2
      ;;
    --compiler)
      COMPILER="$2"
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

if ! command -v "${COMPILER}" >/dev/null 2>&1; then
  echo "[configure-openmp] compiler not found: ${COMPILER}" >&2
  exit 1
fi

mkdir -p "${BUILD_DIR}"
CACHE_FILE="${BUILD_DIR}/CMakeCache.txt"
if [[ -f "${CACHE_FILE}" ]]; then
  cached_compiler="$(awk -F= '/^CMAKE_CXX_COMPILER:FILEPATH=/{print $2; exit}' "${CACHE_FILE}" || true)"
  resolved_compiler="$(command -v "${COMPILER}")"
  if [[ -n "${cached_compiler}" && "${cached_compiler}" != "${resolved_compiler}" ]]; then
    echo "[configure-openmp] compiler changed (${cached_compiler} -> ${resolved_compiler}), resetting CMake cache"
    rm -rf "${BUILD_DIR}/CMakeCache.txt" "${BUILD_DIR}/CMakeFiles"
  fi
fi

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_CXX_COMPILER="${COMPILER}" \
  -DENABLE_OPENMP=ON \
  -DREQUIRE_OPENMP=ON

jobs="$(nproc 2>/dev/null || echo 4)"
cmake --build "${BUILD_DIR}" -j "${jobs}"
