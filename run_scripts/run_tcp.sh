#!/bin/bash

# Relative base directories
ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
CONFIG_FILE="${ROOT_DIR}/config/real_rural_shadowing.ini"
PY_ANALYZERS="${ROOT_DIR}/py_analizers"

DRAW_MAC="${PY_ANALYZERS}/draw_mac.py"
DRAW_UE="${PY_ANALYZERS}/draw_ue.py"

# Function to generate plots
generatePlots() {
    echo "[INFO] Searching and running analysis scripts..."

    echo "[PLOT] Running draw_mac.py"
    python3 "$DRAW_MAC" || { echo "[ERROR] draw_mac.py failed."; exit 1; }

    echo "[PLOT] Running draw_ue.py"
    python3 "$DRAW_UE" || { echo "[ERROR] draw_ue.py failed."; exit 1; }

    echo "[DONE] Plots generated successfully."
}

# If called with argument 'graficas', only plot
if [ "$1" == "graficas" ]; then
    generatePlots
    exit 0
fi

# Trap Ctrl+C to stop client and generate plots
trap 'echo "[INTERRUPT] Stopping client and generating plots..."; kill ${CLIENT_PID}; wait; generatePlots; exit 0' SIGINT

# Kill previous processes on port 5202 (DL)
sudo fuser -k 5202/tcp
sleep 1

# Flush previous iptables rules
sudo iptables -F
sudo iptables -X

# Set new iptables rules
sudo iptables -I INPUT -p tcp --dport 5202 -j NFQUEUE --queue-num 0
sudo iptables -I INPUT -p tcp --sport 5202 -j NFQUEUE --queue-num 1

# Start iperf3 server
iperf3 -s -p 5202 &
sleep 2

# Start iperf3 client in reverse mode
iperf3 -c 127.0.0.1 -t 3000 -p 5202 &
client_pid=$!
CLIENT_PID=$client_pid

# Run emulator
pushd "$ROOT_DIR" > /dev/null
echo "[RUN] Running bin/fikore with ${CONFIG_FILE}"
sudo ./bin/fikore "$CONFIG_FILE"
RESULT_CODE=$?
popd > /dev/null

# Wait for client to finish
wait $client_pid

# Check emulator result
if [ $RESULT_CODE -ne 0 ]; then
    echo "[ERROR] Emulator execution failed. Aborting."
    exit 1
fi

echo "[INFO] Emulator finished successfully."

# Generate plots
generatePlots
