#!/bin/bash

# Relative paths
ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
CONFIG_FILE="${ROOT_DIR}/config/real_mimosa.ini"
PY_ANALYZERS="${ROOT_DIR}/py_analizers"

DRAW_MAC="${PY_ANALYZERS}/draw_mac.py"
DRAW_UE="${PY_ANALYZERS}/draw_ue.py"

# Function to generate plots
generatePlots() {
    echo "[INFO] Running analysis scripts..."

    echo "[PLOT] draw_mac.py"
    python3 "$DRAW_MAC" || { echo "[ERROR] draw_mac.py failed."; exit 1; }

    echo "[PLOT] draw_ue.py"
    python3 "$DRAW_UE" || { echo "[ERROR] draw_ue.py failed."; exit 1; }

    echo "[DONE] Plots generated successfully."
}

# Only plot if called with argument 'graficas'
if [ "$1" == "graficas" ]; then
    generatePlots
    exit 0
fi

# Gracefully handle Ctrl+C
trap 'echo "[INTERRUPT] Stopping client and generating plots..."; kill ${CLIENT_PID} 2>/dev/null; wait; generatePlots; exit 0' SIGINT

# Cleanup: iptables rules and previous processes
echo "[CLEANUP] Flushing iptables rules..."
sudo iptables -F
sudo iptables -X

echo "[CLEANUP] Killing processes on UDP port 5202..."
sudo fuser -k 5202/udp
sleep 1

# Start iperf3 UDP server
echo "[START] Starting iperf3 UDP server on port 5202..."
iperf3 -s -p 5202 &
SERVER_PID=$!
sleep 2

# Start iperf3 UDP client
echo "[START] Starting iperf3 UDP client..."
iperf3 -c 127.0.0.1 -u -t 1000 -p 5202 -b 200M &
CLIENT_PID=$!
sleep 5

# iptables rules for UDP traffic (UL)
sudo iptables -I INPUT -p udp --sport 5202 -j NFQUEUE --queue-num 1
sudo iptables -I OUTPUT -p udp --dport 5202 -j NFQUEUE --queue-num 0

# Run emulator
echo "[EMULATOR] Running emulator with ${CONFIG_FILE}..."
pushd "$ROOT_DIR" > /dev/null
sudo ./bin/main "$CONFIG_FILE"
RESULT_CODE=$?
popd > /dev/null

# Wait for client to finish if still running
wait ${CLIENT_PID}

# Check emulator result
if [ $RESULT_CODE -ne 0 ]; then
    echo "[ERROR] Emulator execution failed. Aborting."
    exit 1
fi

echo "[INFO] Emulator finished successfully."

# Generate plots after execution
generatePlots
