#!/usr/bin/env bash
# Arranca el emulador y, por defecto, la API de control junto a el.
#
#   docker run ... <config.ini>          emulador + API
#   docker run ... --no-api <config.ini> solo el emulador
#   docker run ... --api-only            solo la API (emulador en otro contenedor)
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WITH_API=1
API_ONLY=0
ARGS=()

for arg in "$@"; do
  case "$arg" in
    --no-api)   WITH_API=0 ;;
    --api-only) API_ONLY=1 ;;
    *)          ARGS+=("$arg") ;;
  esac
done

API_PID=""
cleanup() {
  [ -n "${API_PID}" ] && kill "${API_PID}" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

if [ "${WITH_API}" -eq 1 ]; then
  FIKORE_API_HOST="${FIKORE_API_HOST:-0.0.0.0}" "${ROOT_DIR}/run_scripts/run_api.sh" &
  API_PID=$!
  echo "[entrypoint] API en ${FIKORE_API_HOST:-0.0.0.0}:${FIKORE_API_PORT:-8100} (pid ${API_PID})"
fi

if [ "${API_ONLY}" -eq 1 ]; then
  wait "${API_PID}"
  exit $?
fi

exec "${ROOT_DIR}/bin/fikore" "${ARGS[@]}"
