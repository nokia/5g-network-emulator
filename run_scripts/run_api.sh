#!/usr/bin/env bash
# Arranca la API REST/WebSocket del plano de control.
#
# El venv de la API es independiente del .venv del repo, que es de los analizadores:
# la API se despliega con el emulador y no debe arrastrar matplotlib ni numpy.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
API_DIR="${ROOT_DIR}/api"
VENV_DIR="${API_DIR}/.venv"

HOST="${FIKORE_API_HOST:-127.0.0.1}"
PORT="${FIKORE_API_PORT:-8100}"

if [ ! -x "${VENV_DIR}/bin/python" ]; then
  echo "[run_api] creando ${VENV_DIR}"
  python3 -m venv "${VENV_DIR}"
  "${VENV_DIR}/bin/pip" install --quiet --upgrade pip
  "${VENV_DIR}/bin/pip" install --quiet -r "${API_DIR}/requirements.txt"
fi

cd "${API_DIR}"
exec "${VENV_DIR}/bin/python" -m uvicorn fikore_api.main:app --host "${HOST}" --port "${PORT}" "$@"
