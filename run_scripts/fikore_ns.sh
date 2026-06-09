#!/usr/bin/env bash

FIKORE_NS_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fikorens() {
    if [[ $# -lt 1 ]]; then
        echo "Usage: fikorens <ueN|dn|emu>" >&2
        return 1
    fi

    exec sudo -E \
        "${FIKORE_NS_ROOT}/run_scripts/run_fikore_nfqueue_lab.sh" shell "$1"
}

fikorens_open() {
    if [[ $# -lt 1 ]]; then
        echo "Usage: fikorens_open <ueN|dn|emu>" >&2
        return 1
    fi

    sudo -E \
        "${FIKORE_NS_ROOT}/run_scripts/run_fikore_nfqueue_lab.sh" shell "$1"
}

fikorens_exec() {
    if [[ $# -lt 2 ]]; then
        echo "Usage: fikorens_exec <ueN|dn|emu> <command> [args...]" >&2
        return 1
    fi

    local target="$1"
    shift
    sudo -E \
        "${FIKORE_NS_ROOT}/run_scripts/run_fikore_nfqueue_lab.sh" exec "${target}" -- "$@"
}
