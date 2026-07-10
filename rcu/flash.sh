#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"

MPLABX_DIR="${MPLABX_DIR:-/opt/microchip/mplabx/v6.00}"
JAVA_BIN="${JAVA_BIN:-$MPLABX_DIR/sys/java/zulu8.54.0.21-ca-fx-jre8.0.292-linux_x64/bin/java}"
IPECMD_JAR="${IPECMD_JAR:-$MPLABX_DIR/mplab_platform/mplab_ipe/ipecmd.jar}"
TOOL="${TOOL:-AUTO}"
ACTION="${ACTION:-M}"
PROGRAM_REGION="${PROGRAM_REGION:-}"
LOG_LEVEL="${LOG_LEVEL:-OL}"
VERIFY="${VERIFY:-1}"
VERIFY_REGION="${VERIFY_REGION:-}"
PRESERVE_EEPROM="${PRESERVE_EEPROM:-1}"
JUST_CONNECT="${JUST_CONNECT:-0}"
POWER_TARGET="${POWER_TARGET:-}"
VDD_FIRST="${VDD_FIRST:-}"
DIAG_LOG_LEVEL="${DIAG_LOG_LEVEL:-0}"
DIAG_LOG_PATH="${DIAG_LOG_PATH:-$PROJECT_DIR/flash-ipecmd.log}"
EXTRA_ARGS="${EXTRA_ARGS:-}"

CONFIG_XML="$PROJECT_DIR/nbproject/configurations.xml"
artifact_path="$(sed -n 's/^CND_ARTIFACT_PATH_default=//p' "$PROJECT_DIR/nbproject/Makefile-variables.mk" | head -n 1)"
target_device="$(sed -n 's:.*<targetDevice>\(.*\)</targetDevice>.*:\1:p' "$CONFIG_XML" | head -n 1)"
project_platform_tool="$(sed -n 's:.*<platformTool>\(.*\)</platformTool>.*:\1:p' "$CONFIG_XML" | head -n 1)"

tool_to_platform_tag() {
  case "$1" in
    PK3) printf 'PICkit3PlatformTool\n' ;;
    ICD3) printf 'ICD3PlatformTool\n' ;;
    RI) printf 'RealICEPlatformTool\n' ;;
    PK4) printf 'PICkit4PlatformTool\n' ;;
    ICD4) printf 'ICD4PlatformTool\n' ;;
    *) return 1 ;;
  esac
}

platform_tag_to_tool() {
  case "$1" in
    PICkit3PlatformTool) printf 'PK3\n' ;;
    ICD3PlatformTool) printf 'ICD3\n' ;;
    RealICEPlatformTool) printf 'RI\n' ;;
    PICkit4PlatformTool) printf 'PK4\n' ;;
    ICD4PlatformTool) printf 'ICD4\n' ;;
    *) return 1 ;;
  esac
}

extract_tool_property() {
  local platform_tag="$1"
  local property_key="$2"

  sed -n "/<$platform_tag>/,/<\\/$platform_tag>/ s:.*<property key=\"$property_key\" value=\"\\([^\"]*\\)\".*:\\1:p" "$CONFIG_XML" | head -n 1
}

detect_usb_programmers() {
  local usb_listing
  local -a detected_tools=()

  if ! command -v lsusb >/dev/null 2>&1; then
    return 0
  fi

  usb_listing="$(lsusb 2>/dev/null || true)"
  if [[ -z "$usb_listing" ]]; then
    return 0
  fi

  if grep -qiE '04d8:9004|REAL ICE' <<<"$usb_listing"; then
    detected_tools+=(RI)
  fi
  if grep -qiE '04d8:9009|(^|[^[:alnum:]])ICD[[:space:]]*3([^[:alnum:]]|$)' <<<"$usb_listing"; then
    detected_tools+=(ICD3)
  fi
  if grep -qiE '04d8:900a|PICkit[[:space:]]*3' <<<"$usb_listing"; then
    detected_tools+=(PK3)
  fi
  if grep -qiE '04d8:9012|PICkit[[:space:]]*4' <<<"$usb_listing"; then
    detected_tools+=(PK4)
  fi
  if grep -qiE '04d8:9015|ICD[[:space:]]*4' <<<"$usb_listing"; then
    detected_tools+=(ICD4)
  fi

  if [[ ${#detected_tools[@]} -gt 0 ]]; then
    printf '%s\n' "${detected_tools[@]}"
  fi
}

join_by_comma() {
  local first=1
  local item

  for item in "$@"; do
    if [[ $first -eq 1 ]]; then
      printf '%s' "$item"
      first=0
    else
      printf ', %s' "$item"
    fi
  done
}

select_tool() {
  local requested_tool="$1"
  local preferred_tool=""
  local detected_output=""
  local candidate

  DETECTED_USB_TOOLS=()
  TOOL_SOURCE="manual"

  if [[ "$requested_tool" != "AUTO" ]]; then
    TOOL="$requested_tool"
    TOOL_SOURCE="manual"
    return
  fi

  if preferred_tool="$(platform_tag_to_tool "$project_platform_tool" 2>/dev/null)"; then
    :
  else
    preferred_tool=""
  fi

  detected_output="$(detect_usb_programmers)"
  if [[ -n "$detected_output" ]]; then
    mapfile -t DETECTED_USB_TOOLS <<<"$detected_output"
  fi

  if [[ ${#DETECTED_USB_TOOLS[@]} -gt 0 ]]; then
    if [[ -n "$preferred_tool" ]]; then
      for candidate in "${DETECTED_USB_TOOLS[@]}"; do
        if [[ "$candidate" == "$preferred_tool" ]]; then
          TOOL="$candidate"
          TOOL_SOURCE="usb+project"
          return
        fi
      done
    fi

    TOOL="${DETECTED_USB_TOOLS[0]}"
    if [[ ${#DETECTED_USB_TOOLS[@]} -gt 1 ]]; then
      TOOL_SOURCE="usb+fallback"
    else
      TOOL_SOURCE="usb"
    fi
    return
  fi

  if [[ -n "$preferred_tool" ]]; then
    TOOL="$preferred_tool"
    TOOL_SOURCE="project"
    return
  fi

  TOOL="ICD3"
  TOOL_SOURCE="legacy-default"
}

apply_tool_defaults() {
  local platform_tag
  local entry_method
  local power_enable

  if ! platform_tag="$(tool_to_platform_tag "$TOOL" 2>/dev/null)"; then
    if [[ -z "$POWER_TARGET" ]]; then
      POWER_TARGET="0"
    fi
    if [[ -z "$VDD_FIRST" ]]; then
      VDD_FIRST="0"
    fi
    return
  fi

  entry_method="$(extract_tool_property "$platform_tag" "programoptions.testmodeentrymethod")"
  power_enable="$(extract_tool_property "$platform_tag" "poweroptions.powerenable")"

  if [[ -z "$POWER_TARGET" ]]; then
    if [[ "$power_enable" == "true" ]]; then
      POWER_TARGET="1"
    else
      POWER_TARGET="0"
    fi
  fi

  if [[ -z "$VDD_FIRST" ]]; then
    if [[ "$entry_method" == "VDDFirst" ]]; then
      VDD_FIRST="1"
    else
      VDD_FIRST="0"
    fi
  fi
}

if [[ -z "$artifact_path" ]]; then
  echo "Cannot determine artifact path from nbproject/Makefile-variables.mk" >&2
  exit 1
fi

if [[ -z "$target_device" ]]; then
  echo "Cannot determine target device from nbproject/configurations.xml" >&2
  exit 1
fi

hex_path="$PROJECT_DIR/$artifact_path"
ipe_device="${target_device#PIC}"

if [[ ! -f "$JAVA_BIN" ]]; then
  echo "Java binary not found: $JAVA_BIN" >&2
  exit 1
fi

if [[ ! -f "$IPECMD_JAR" ]]; then
  echo "ipecmd.jar not found: $IPECMD_JAR" >&2
  exit 1
fi

if [[ ! -f "$hex_path" ]]; then
  echo "HEX file not found: $hex_path" >&2
  echo "Build first, then rerun this script." >&2
  exit 1
fi

select_tool "$TOOL"
apply_tool_defaults

if [[ -z "$VERIFY_REGION" && "$VERIFY" == "1" && "$PRESERVE_EEPROM" == "1" ]]; then
  VERIFY_REGION="PIC"
fi

if [[ -z "$PROGRAM_REGION" && "$ACTION" == "M" && "$PRESERVE_EEPROM" == "1" ]]; then
  PROGRAM_REGION="PIC"
fi

cmd=(
  "$JAVA_BIN"
  -jar "$IPECMD_JAR"
  "-TP$TOOL"
  "-P$ipe_device"
  "-F$hex_path"
  "-$LOG_LEVEL"
)

if [[ -n "$PROGRAM_REGION" ]]; then
  cmd+=("-${ACTION}${PROGRAM_REGION}")
else
  cmd+=("-$ACTION")
fi

if [[ "$VERIFY" == "1" ]]; then
  if [[ -n "$VERIFY_REGION" ]]; then
    cmd+=("-Y$VERIFY_REGION")
  else
    cmd+=("-Y")
  fi
fi

if [[ "$JUST_CONNECT" == "1" ]]; then
  cmd+=("-OK")
fi

if [[ "$POWER_TARGET" == "1" ]]; then
  cmd+=("-W")
fi

if [[ "$VDD_FIRST" == "1" ]]; then
  cmd+=("-OD")
fi

if [[ "$DIAG_LOG_LEVEL" != "0" ]]; then
  cmd+=("-OSL${DIAG_LOG_LEVEL}${DIAG_LOG_PATH}")
fi

if [[ -n "$EXTRA_ARGS" ]]; then
  # Intentional word splitting so callers can pass raw ipecmd switches.
  # shellcheck disable=SC2206
  extra_args=( $EXTRA_ARGS )
  cmd+=("${extra_args[@]}")
fi

if [[ "${DRY_RUN:-0}" == "1" ]]; then
  printf 'Using HEX: %s\n' "$hex_path"
  printf 'Target MCU: %s\n' "$target_device"
  printf 'Programmer: %s\n' "$TOOL"
  printf 'Programmer source: %s\n' "$TOOL_SOURCE"
  if [[ ${#DETECTED_USB_TOOLS[@]} -gt 0 ]]; then
    printf 'Detected USB programmers: %s\n' "$(join_by_comma "${DETECTED_USB_TOOLS[@]}")"
  fi
  printf 'Action: %s\n' "$ACTION"
  printf 'Program region: %s\n' "${PROGRAM_REGION:-all}"
  printf 'Verify after program: %s\n' "$VERIFY"
  printf 'Verify region: %s\n' "${VERIFY_REGION:-all}"
  printf 'Preserve EEPROM: %s\n' "$PRESERVE_EEPROM"
  printf 'Just connect only: %s\n' "$JUST_CONNECT"
  printf 'Power target from tool: %s\n' "$POWER_TARGET"
  printf 'Use VDD first: %s\n' "$VDD_FIRST"
  if [[ "$DIAG_LOG_LEVEL" != "0" ]]; then
    printf 'Diagnostics log: level %s -> %s\n' "$DIAG_LOG_LEVEL" "$DIAG_LOG_PATH"
  fi
  printf 'Command:'
  printf ' %q' "${cmd[@]}"
  printf '\n'
  exit 0
fi

printf 'Flashing %s to %s using %s\n' "$hex_path" "$target_device" "$TOOL"
case "$TOOL_SOURCE" in
  usb|usb+project|usb+fallback)
    printf 'Programmer source: auto-detected from USB'
    if [[ "$TOOL_SOURCE" == "usb+project" ]]; then
      printf ' (matched project preference)'
    elif [[ "$TOOL_SOURCE" == "usb+fallback" ]]; then
      printf ' (multiple found, selected first match)'
    fi
    printf '\n'
    ;;
  project)
    printf 'Programmer source: project default from nbproject/configurations.xml\n'
    ;;
  legacy-default)
    printf 'Programmer source: fallback default (ICD3)\n'
    ;;
esac
if [[ ${#DETECTED_USB_TOOLS[@]} -gt 0 ]]; then
  printf 'Detected USB programmers: %s\n' "$(join_by_comma "${DETECTED_USB_TOOLS[@]}")"
fi
if [[ "$POWER_TARGET" == "1" ]]; then
  printf 'Target power: programmer supplies VDD\n'
fi
if [[ "$VDD_FIRST" == "1" ]]; then
  printf 'Programming order: VDD first\n'
fi
if [[ "$VERIFY" == "1" ]]; then
  printf 'Post-program verify: enabled\n'
  printf 'Verify region: %s\n' "${VERIFY_REGION:-all}"
fi
if [[ "$PRESERVE_EEPROM" == "1" ]]; then
  printf 'EEPROM: preserved by skipping EEPROM programming\n'
fi
if [[ "$DIAG_LOG_LEVEL" != "0" ]]; then
  printf 'Diagnostics log: %s\n' "$DIAG_LOG_PATH"
fi
"${cmd[@]}"
