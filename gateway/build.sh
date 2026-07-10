#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if ! command -v idf.py >/dev/null 2>&1; then
  echo "error: idf.py not found. Source your ESP-IDF environment first." >&2
  echo "example: . /path/to/esp-idf/export.sh" >&2
  exit 127
fi

TARGET="${IDF_TARGET:-esp32}"
BUILD_DIR="${BUILD_DIR:-build-wt32}"

idf.py -B "$BUILD_DIR" set-target "$TARGET"
idf.py -B "$BUILD_DIR" build "$@"
