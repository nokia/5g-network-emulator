#!/usr/bin/env bash

if [[ -n "${FIKORE_PYTHON_ENV_SH_LOADED:-}" ]]; then
    return 0
fi
FIKORE_PYTHON_ENV_SH_LOADED=1

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENV_DIR="${ROOT_DIR}/.venv"
MPLCONFIGDIR_DEFAULT="${ROOT_DIR}/tmp/matplotlib"

fikore_python_bin() {
    if [[ -x "${VENV_DIR}/bin/python" ]]; then
        printf "%s\n" "${VENV_DIR}/bin/python"
    else
        command -v python3
    fi
}

activate_fikore_python() {
    mkdir -p "${MPLCONFIGDIR:-${MPLCONFIGDIR_DEFAULT}}"
    export MPLCONFIGDIR="${MPLCONFIGDIR:-${MPLCONFIGDIR_DEFAULT}}"

    if [[ -x "${VENV_DIR}/bin/python" ]]; then
        export VIRTUAL_ENV="${VENV_DIR}"
        export PATH="${VENV_DIR}/bin:${PATH}"
        return 0
    fi

    echo "[WARN] Python virtual environment not found at ${VENV_DIR}. Falling back to system python3." >&2
}

fikore_python() {
    local pybin
    pybin="$(fikore_python_bin)"
    "${pybin}" "$@"
}
