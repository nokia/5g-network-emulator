#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG_FILE="${CONFIG_FILE:-${ROOT_DIR}/config/Linear_walking_UMi.ini}"
DRAW_MAC="${ROOT_DIR}/py_analizers/draw_mac.py"
DRAW_UE="${ROOT_DIR}/py_analizers/draw_ue.py"

usage() {
    cat <<EOF
Usage: $(basename "$0") [plots]

Environment:
  CONFIG_FILE=/path/to/config.ini

Examples:
  $(basename "$0")
  CONFIG_FILE=$ROOT_DIR/config/simu_rural.ini $(basename "$0")
  $(basename "$0") plots
EOF
}

generate_plots() {
    echo "[INFO] Generating offline plots from the latest log directory..."
    python3 "${DRAW_MAC}"
    python3 "${DRAW_UE}"
    echo "[DONE] Offline plots generated under results/<latest-log>/."
}

ACTION="${1:-run}"

case "${ACTION}" in
    run)
        [[ -f "${CONFIG_FILE}" ]] || {
            echo "[ERROR] Config file does not exist: ${CONFIG_FILE}" >&2
            exit 1
        }
        cd "${ROOT_DIR}"
        echo "[RUN] Running offline simulation with $(basename "${CONFIG_FILE}")"
        ./bin/fikore "${CONFIG_FILE}"
        generate_plots
        ;;
    plots)
        generate_plots
        ;;
    -h|--help|help)
        usage
        ;;
    *)
        usage
        exit 1
        ;;
esac
