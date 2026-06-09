#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

ACTION="${1:-run}"
MODE="${MODE:-host}"
IMAGE_NAME="${IMAGE_NAME:-fikore:latest}"
CONTAINER_NAME="${CONTAINER_NAME:-fikore-emu}"
CONTAINER_WORKDIR="${CONTAINER_WORKDIR:-/usr/src/5g-network-emulator}"

log() {
    echo "[INFO] $*"
}

die() {
    echo "[ERROR] $*" >&2
    exit 1
}

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "Missing required command: $1"
}

validate_mode() {
    case "${MODE}" in
        host|none) ;;
        *) die "Invalid MODE: ${MODE}. Use host or none." ;;
    esac
}

docker_run_args() {
    local args=(
        --name "${CONTAINER_NAME}"
        --cap-add NET_ADMIN
        --cap-add NET_RAW
        -v "${ROOT_DIR}:${CONTAINER_WORKDIR}"
        -w "${CONTAINER_WORKDIR}"
    )

    if [[ "${MODE}" == "host" ]]; then
        args+=(--network host)
    else
        args+=(--network none)
    fi

    printf '%s\n' "${args[@]}"
}

run_container() {
    validate_mode
    require_cmd docker

    docker rm -f "${CONTAINER_NAME}" >/dev/null 2>&1 || true

    mapfile -t args < <(docker_run_args)
    log "Launching container ${CONTAINER_NAME} with MODE=${MODE}"
    docker run -d "${args[@]}" "${IMAGE_NAME}" sleep infinity >/dev/null
    docker exec "${CONTAINER_NAME}" mkdir -p "${CONTAINER_WORKDIR}/run_scripts/.generated"
}

build_image() {
    require_cmd docker
    log "Building image ${IMAGE_NAME}"
    docker build -t "${IMAGE_NAME}" "${ROOT_DIR}"
}

stop_container() {
    require_cmd docker
    docker rm -f "${CONTAINER_NAME}" >/dev/null 2>&1 || true
}

container_status() {
    require_cmd docker
    docker ps -a --filter "name=^/${CONTAINER_NAME}$"
}

container_shell() {
    require_cmd docker
    exec docker exec -it "${CONTAINER_NAME}" bash
}

usage() {
    cat <<EOF
Usage: $(basename "$0") [build|run|stop|status|shell]

Variables:
  MODE=host|none
  IMAGE_NAME=fikore:latest
  CONTAINER_NAME=fikore-emu

Examples:
  ./$(basename "$0") build
  MODE=host ./$(basename "$0") run
  MODE=none ./$(basename "$0") run
EOF
}

case "${ACTION}" in
    build)
        build_image
        ;;
    run)
        run_container
        ;;
    stop)
        stop_container
        ;;
    status)
        container_status
        ;;
    shell)
        container_shell
        ;;
    *)
        usage
        exit 1
        ;;
esac
