#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_CONFIG_FILE="${ROOT_DIR}/config/emulated_rural_n78_single_with_background.ini"
CONFIG_FILE="${CONFIG_FILE:-${CFG:-}}"
LAB_SCRIPT="${ROOT_DIR}/run_scripts/run_fikore_nfqueue_lab.sh"

usage() {
    cat <<EOF
Usage: $(basename "$0") [config.ini]

Environment:
  CONFIG_FILE=/path/to/config.ini
  CFG=/path/to/config.ini          # accepted as an alias
  BACKEND=host|docker-host|docker-none
  UE_COUNT=2

Examples:
  $(basename "$0")
  $(basename "$0") config/emulated_uma_n78_compare_with_background.ini
  CONFIG_FILE=$ROOT_DIR/config/emulated_rural_n78_single_with_background.ini $(basename "$0")
  CFG=$ROOT_DIR/config/emulated_uma_n78_compare_with_background.ini $(basename "$0")
EOF
}

ACTION="${1:-}"

case "${ACTION}" in
    ""|run)
        if [[ -z "${CONFIG_FILE}" ]]; then
            CONFIG_FILE="${DEFAULT_CONFIG_FILE}"
        fi
        ;;
    -h|--help|help)
        usage
        exit 0
        ;;
    *)
        if [[ -z "${CONFIG_FILE}" ]]; then
            CONFIG_FILE="${ACTION}"
            ACTION="run"
        else
            usage
            exit 1
        fi
        ;;
esac

[[ -f "${CONFIG_FILE}" ]] || {
    echo "[ERROR] Config file does not exist: ${CONFIG_FILE}" >&2
    exit 1
}

cd "${ROOT_DIR}"
echo "[RUN] Running emulated setup with $(basename "${CONFIG_FILE}")"
exec sudo CFG="${CONFIG_FILE}" CONFIG_FILE="${CONFIG_FILE}" "${LAB_SCRIPT}" run
