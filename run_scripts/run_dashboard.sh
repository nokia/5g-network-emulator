#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/run_scripts/python_env.sh"

DASHBOARD_SCRIPT="${ROOT_DIR}/py_analyzers/live_dashboard.py"

activate_fikore_python

exec "$(fikore_python_bin)" "${DASHBOARD_SCRIPT}" "$@"
