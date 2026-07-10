#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if ! command -v idf.py >/dev/null 2>&1; then
  for export_sh in \
    "${IDF_PATH:-}/export.sh" \
    "$HOME/esp/esp-idf/export.sh" \
    "/home/kaina/esp/esp-idf/export.sh"; do
    if [[ -n "$export_sh" && -f "$export_sh" ]]; then
      # shellcheck disable=SC1090
      source "$export_sh" >/dev/null
      break
    fi
  done
fi

if ! command -v idf.py >/dev/null 2>&1; then
  echo "error: idf.py not found. Source your ESP-IDF environment first." >&2
  echo "example: source /home/kaina/esp/esp-idf/export.sh" >&2
  exit 127
fi

TARGET="${IDF_TARGET:-esp32}"
BUILD_DIR="${BUILD_DIR:-build-wt32}"
ESPTOOL_BAUD="${ESPTOOL_BAUD:-460800}"

if [[ $# -gt 0 && "$1" == /dev/* ]]; then
  PORT="$1"
  shift
fi

if [[ -z "${PORT:-}" ]]; then
  ports=()
  for port in /dev/ttyUSB* /dev/ttyACM*; do
    [[ -e "$port" ]] && ports+=("$port")
  done

  if [[ ${#ports[@]} -eq 1 ]]; then
    PORT="${ports[0]}"
    echo "Auto-detected serial port: $PORT"
  elif [[ ${#ports[@]} -gt 1 ]]; then
    echo "error: multiple serial ports found:" >&2
    printf '  %s\n' "${ports[@]}" >&2
    echo "Set one explicitly, e.g. PORT=${ports[0]} ./flash.sh" >&2
    exit 2
  fi
fi

if [[ -z "${PORT:-}" ]]; then
  echo "error: no serial port found. Connect a device or run: PORT=/dev/ttyUSB0 ./flash.sh" >&2
  exit 2
fi

BOOTLOADER_BIN="$BUILD_DIR/bootloader/bootloader.bin"
PARTITION_BIN="$BUILD_DIR/partition_table/partition-table.bin"
OTA_DATA_BIN="$BUILD_DIR/ota_data_initial.bin"
APP_BIN="$BUILD_DIR/gateway.bin"

missing=()
for image in "$BOOTLOADER_BIN" "$PARTITION_BIN" "$OTA_DATA_BIN" "$APP_BIN"; do
  [[ -f "$image" ]] || missing+=("$image")
done

if [[ ${#missing[@]} -gt 0 ]]; then
  echo "error: flash image(s) not found:" >&2
  printf '  %s\n' "${missing[@]}" >&2
  echo "Build once before flashing. This script will not build automatically." >&2
  echo "example: idf.py -B $BUILD_DIR build" >&2
  exit 2
fi

python -m esptool \
  --chip "$TARGET" \
  -p "$PORT" \
  -b "$ESPTOOL_BAUD" \
  --before default-reset \
  --after hard-reset \
  write-flash \
  --flash-mode dio \
  --flash-freq 40m \
  --flash-size 8MB \
  0x1000 "$BOOTLOADER_BIN" \
  0x8000 "$PARTITION_BIN" \
  0xF000 "$OTA_DATA_BIN" \
  0x20000 "$APP_BIN" \
  "$@"
