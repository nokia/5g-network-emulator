#!/bin/bash

# Base directories
ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
CONFIG_DIR="${ROOT_DIR}/config"
LOGS_DIR="${ROOT_DIR}/logs"
ANALYZER_DIR="${ROOT_DIR}/py_analizers"

# Input config
CONFIG_FILE="${CONFIG_DIR}/Linear_walking_UMi.ini"
CONFIG_NAME=$(basename "$CONFIG_FILE")


# Python scripts
DRAW_MAC="${ANALYZER_DIR}/draw_mac.py"
DRAW_UE="${ANALYZER_DIR}/draw_ue.py"

# Run emulator
cd "$ROOT_DIR"
echo "[RUN] Running emulator with $CONFIG_NAME"
./bin/fikore "$CONFIG_FILE"
if [ $? -ne 0 ]; then
    echo "[ERROR] Emulator failed."
    exit 1
fi

# Find latest log directory
LATEST_LOG_DIR=$(ls -td "${LOGS_DIR}"/*/ | head -n 1)
if [ -z "$LATEST_LOG_DIR" ]; then
    echo "[ERROR] No log directory found."
    exit 1
fi
echo "[INFO] Latest log directory: $LATEST_LOG_DIR"

# Run analyzers if corresponding subfolders exist
if [ -d "${LATEST_LOG_DIR}/mac" ]; then
    echo "[PLOT] Running draw_mac.py"
    python3 "$DRAW_MAC" "$OUTPUT_DIR"
fi

if [ -d "${LATEST_LOG_DIR}/ue" ]; then
    echo "[PLOT] Running draw_ue.py"
    python3 "$DRAW_UE" "$OUTPUT_DIR"
fi

echo "[DONE] All available plots saved to $OUTPUT_DIR"
