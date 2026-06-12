#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GENERATED_DIR="${ROOT_DIR}/run_scripts/.generated"
STATE_FILE="${GENERATED_DIR}/fikore_nfqueue_lab.env"

ACTION="${1:-run}"
BACKEND="${BACKEND:-}"
CFG="${CFG:-}"
CONFIG_FILE="${CONFIG_FILE:-${CFG}}"

CONTAINER_NAME="${CONTAINER_NAME:-fikore-emu}"
CONTAINER_WORKDIR="${CONTAINER_WORKDIR:-/usr/src/5g-network-emulator}"

UE_COUNT="${UE_COUNT:-}"
QUEUE_BASE="${QUEUE_BASE:-100}"
QUEUE_STEP="${QUEUE_STEP:-10}"

UE_PREFIX="${UE_PREFIX:-10.201}"
DN_EMU_IP="${DN_EMU_IP:-10.255.0.1}"
DN_REAL_IP="${DN_REAL_IP:-10.255.0.2}"
DN_CIDR="${DN_CIDR:-24}"

MGMT_HOST_IP="${MGMT_HOST_IP:-172.30.0.1}"
MGMT_EMU_IP="${MGMT_EMU_IP:-172.30.0.2}"
MGMT_CIDR="${MGMT_CIDR:-30}"

CHAIN_NAME="${CHAIN_NAME:-FIKORE_EMU}"
CAPTURE_DIR="${CAPTURE_DIR:-${ROOT_DIR}/run_scripts/.captures}"
CAPTURE_ID="${CAPTURE_ID:-default}"
CAPTURE_UDP_PORT="${CAPTURE_UDP_PORT:-}"
CAPTURE_DN_NAMESPACE="${CAPTURE_DN_NAMESPACE:-1}"
CAPTURE_ROTATE_SECONDS="${CAPTURE_ROTATE_SECONDS:-0}"

CALLER_PWD="${PWD}"
UE_ACTIVE_SET="${UE_ACTIVE_SET:-}"

log() {
    echo "[INFO] $*"
}

warn() {
    echo "[WARN] $*" >&2
}

die() {
    echo "[ERROR] $*" >&2
    exit 1
}

apply_default_settings() {
    BACKEND="${BACKEND:-host}"
    CONFIG_FILE="${CONFIG_FILE:-${ROOT_DIR}/config/emulated_rural_n78_single_with_background.ini}"
    UE_COUNT="${UE_COUNT:-2}"
    if [[ -z "${UE_ACTIVE_SET}" ]]; then
        UE_ACTIVE_SET="$(default_active_ue_set "${UE_COUNT}")"
    fi
}

load_saved_lab_state() {
    if [[ -f "${STATE_FILE}" ]]; then
        # shellcheck disable=SC1090
        . "${STATE_FILE}"
    fi
    apply_default_settings
}

load_lab_state_for_run() {
    local explicit_config
    explicit_config="${CONFIG_FILE}"

    load_saved_lab_state
    [[ -n "${explicit_config}" ]] && CONFIG_FILE="${explicit_config}"
}

save_lab_state() {
    mkdir -p "${GENERATED_DIR}"
    cat > "${STATE_FILE}" <<EOF
BACKEND='${BACKEND}'
CONFIG_FILE='${CONFIG_FILE}'
CONTAINER_NAME='${CONTAINER_NAME}'
CONTAINER_WORKDIR='${CONTAINER_WORKDIR}'
UE_COUNT='${UE_COUNT}'
QUEUE_BASE='${QUEUE_BASE}'
QUEUE_STEP='${QUEUE_STEP}'
UE_PREFIX='${UE_PREFIX}'
DN_EMU_IP='${DN_EMU_IP}'
DN_REAL_IP='${DN_REAL_IP}'
DN_CIDR='${DN_CIDR}'
UE_ACTIVE_SET='${UE_ACTIVE_SET}'
MGMT_HOST_IP='${MGMT_HOST_IP}'
MGMT_EMU_IP='${MGMT_EMU_IP}'
MGMT_CIDR='${MGMT_CIDR}'
CHAIN_NAME='${CHAIN_NAME}'
CAPTURE_DIR='${CAPTURE_DIR}'
EOF
}

remove_lab_state() {
    rm -f "${STATE_FILE}"
}

default_active_ue_set() {
    local count="${1:-0}"
    local items=()
    local i
    for ((i = 1; i <= count; i++)); do
        items+=("${i}")
    done
    (IFS=,; printf "%s" "${items[*]}")
}

active_ue_indices() {
    tr ',' '\n' <<< "${UE_ACTIVE_SET}" | sed '/^$/d'
}

active_ue_count() {
    local count=0
    local _
    while read -r _; do
        count=$((count + 1))
    done < <(active_ue_indices)
    printf "%d" "${count}"
}

ue_set_contains() {
    local wanted="$1"
    local idx
    while read -r idx; do
        [[ "${idx}" == "${wanted}" ]] && return 0
    done < <(active_ue_indices)
    return 1
}

add_ue_to_active_set() {
    local wanted="$1"
    local merged
    merged="$(
        {
            active_ue_indices
            printf "%s\n" "${wanted}"
        } | awk 'NF { print $1 }' | sort -n | uniq | paste -sd,
    )"
    UE_ACTIVE_SET="${merged}"
    UE_COUNT="$(active_ue_count)"
}

remove_ue_from_active_set() {
    local wanted="$1"
    local merged
    merged="$(
        active_ue_indices | awk -v wanted="${wanted}" '$1 != wanted' | paste -sd,
    )"
    UE_ACTIVE_SET="${merged}"
    UE_COUNT="$(active_ue_count)"
}

capture_session_dir() {
    printf "%s/%s" "${CAPTURE_DIR}" "${CAPTURE_ID}"
}

capture_meta_file() {
    printf "%s/capture.env" "$(capture_session_dir)"
}

capture_pid_file() {
    printf "%s/%s.pid" "$(capture_session_dir)" "$1"
}

capture_pcap_file() {
    printf "%s/%s.pcap" "$(capture_session_dir)" "$1"
}

capture_log_file() {
    printf "%s/%s.log" "$(capture_session_dir)" "$1"
}

require_root() {
    if [[ "${EUID}" -ne 0 ]]; then
        die "This script must be run as root."
    fi
}

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "Missing required command: $1"
}

require_namespace_exec_cmds() {
    require_cmd ip
    require_cmd setpriv
    require_cmd getent
}

validate_backend() {
    case "${BACKEND}" in
        host|docker-host|docker-none) ;;
        *) die "Invalid BACKEND: ${BACKEND}. Use host, docker-host, or docker-none." ;;
    esac
}

emu_node_is_host() {
    [[ "${BACKEND}" == "host" || "${BACKEND}" == "docker-host" ]]
}

container_required() {
    [[ "${BACKEND}" == "docker-host" || "${BACKEND}" == "docker-none" ]]
}

ue_ns() {
    printf "fkue%d" "$1"
}

ue_host_if() {
    printf "fkhu%d" "$1"
}

ue_emu_if() {
    printf "fkeu%d" "$1"
}

dn_ns() {
    printf "fkdn"
}

dn_host_if() {
    printf "fkhdn"
}

dn_emu_if() {
    printf "fkedn"
}

mgmt_host_if() {
    printf "fkmgh"
}

mgmt_emu_if() {
    printf "fkmge"
}

ue_real_ip() {
    printf "%s.%d.2" "${UE_PREFIX}" "$1"
}

ue_emu_ip() {
    printf "%s.%d.1" "${UE_PREFIX}" "$1"
}

queue_ul() {
    local ue_idx="$1"
    printf "%d" $((QUEUE_BASE + (ue_idx - 1) * QUEUE_STEP))
}

queue_dl() {
    local ue_idx="$1"
    printf "%d" $((QUEUE_BASE + (ue_idx - 1) * QUEUE_STEP + 1))
}

docker_pid() {
    docker inspect -f '{{.State.Pid}}' "${CONTAINER_NAME}" 2>/dev/null
}

namespace_exists() {
    [[ -e "/var/run/netns/$1" ]]
}

resolve_target_identity() {
    TARGET_RUN_UID="${TARGET_UID:-${SUDO_UID:-}}"
    TARGET_RUN_GID="${TARGET_GID:-${SUDO_GID:-}}"
    TARGET_RUN_USER="${TARGET_USER:-${SUDO_USER:-}}"
    TARGET_RUN_HOME="${TARGET_HOME:-}"

    [[ -n "${TARGET_RUN_UID}" ]] || die "Missing target user identity. Run this command via sudo from a regular user, or set TARGET_UID/TARGET_GID/TARGET_USER/TARGET_HOME."
    [[ -n "${TARGET_RUN_GID}" ]] || die "Missing target group identity. Run this command via sudo from a regular user, or set TARGET_UID/TARGET_GID/TARGET_USER/TARGET_HOME."

    local passwd_entry
    passwd_entry="$(getent passwd "${TARGET_RUN_UID}" || true)"
    [[ -n "${passwd_entry}" ]] || die "Could not resolve passwd entry for UID ${TARGET_RUN_UID}."

    if [[ -z "${TARGET_RUN_USER}" ]]; then
        TARGET_RUN_USER="$(printf "%s" "${passwd_entry}" | cut -d: -f1)"
    fi
    if [[ -z "${TARGET_RUN_HOME}" ]]; then
        TARGET_RUN_HOME="$(printf "%s" "${passwd_entry}" | cut -d: -f6)"
    fi
    TARGET_RUN_SHELL="$(printf "%s" "${passwd_entry}" | cut -d: -f7)"

    [[ -n "${TARGET_RUN_HOME}" ]] || die "Could not resolve home directory for UID ${TARGET_RUN_UID}."
    [[ -n "${TARGET_RUN_SHELL}" ]] || TARGET_RUN_SHELL="/bin/bash"
}

resolve_namespace_target() {
    local target="$1"
    TARGET_LABEL="${target}"

    case "${target}" in
        dn)
            TARGET_KIND="netns"
            TARGET_NAMESPACE="$(dn_ns)"
            ;;
        ue[0-9]*)
            [[ "${target}" =~ ^ue([0-9]+)$ ]] || die "Invalid UE target: ${target}"
            TARGET_KIND="netns"
            TARGET_NAMESPACE="$(ue_ns "${BASH_REMATCH[1]}")"
            ;;
        emu)
            if [[ "${BACKEND}" == "docker-none" ]]; then
                TARGET_KIND="emu-docker-none"
                TARGET_NAMESPACE="emu"
            else
                die "Target 'emu' is only available with BACKEND=docker-none. Use ueN or dn for host-backed topologies."
            fi
            ;;
        *)
            die "Unknown target namespace: ${target}. Use ueN, dn, or emu."
            ;;
    esac
}

ensure_namespace_target_ready() {
    local target="$1"

    validate_backend
    require_root
    require_namespace_exec_cmds
    resolve_target_identity
    resolve_namespace_target "${target}"

    case "${TARGET_KIND}" in
        netns)
            namespace_exists "${TARGET_NAMESPACE}" || die "Namespace ${TARGET_NAMESPACE} does not exist. Run 'up' or 'run' first."
            ;;
        emu-docker-none)
            require_cmd docker
            ensure_container_running
            ;;
    esac
}

run_in_target_namespace() {
    case "${TARGET_KIND}" in
        netns)
            ip netns exec "${TARGET_NAMESPACE}" \
                setpriv \
                    --reuid "${TARGET_RUN_UID}" \
                    --regid "${TARGET_RUN_GID}" \
                    --init-groups \
                    env \
                        HOME="${TARGET_RUN_HOME}" \
                        USER="${TARGET_RUN_USER}" \
                        LOGNAME="${TARGET_RUN_USER}" \
                        SHELL="${TARGET_RUN_SHELL}" \
                        PATH="${PATH}" \
                        TERM="${TERM:-xterm-256color}" \
                        FIKORE_NS_NAME="${TARGET_NAMESPACE}" \
                        FIKORE_NS_LABEL="${TARGET_LABEL}" \
                        FIKORE_NS_PWD="${CALLER_PWD}" \
                        "$@"
            ;;
        emu-docker-none)
            local pid
            pid="$(docker_pid)"
            [[ -n "${pid}" && "${pid}" != "0" ]] || die "Could not get PID for ${CONTAINER_NAME}"
            nsenter -t "${pid}" -n \
                setpriv \
                    --reuid "${TARGET_RUN_UID}" \
                    --regid "${TARGET_RUN_GID}" \
                    --init-groups \
                    env \
                        HOME="${TARGET_RUN_HOME}" \
                        USER="${TARGET_RUN_USER}" \
                        LOGNAME="${TARGET_RUN_USER}" \
                        SHELL="${TARGET_RUN_SHELL}" \
                        PATH="${PATH}" \
                        TERM="${TERM:-xterm-256color}" \
                        FIKORE_NS_NAME="emu" \
                        FIKORE_NS_LABEL="${TARGET_LABEL}" \
                        FIKORE_NS_PWD="${CALLER_PWD}" \
                        "$@"
            ;;
    esac
}

namespace_shell() {
    local target="${1:-}"
    [[ -n "${target}" ]] || die "Missing target namespace. Use ueN, dn, or emu."

    ensure_namespace_target_ready "${target}"
    log "Opening interactive shell in ${TARGET_NAMESPACE} as ${TARGET_RUN_USER}"
    run_in_target_namespace /bin/bash -lc '
        cd "$FIKORE_NS_PWD" 2>/dev/null || cd "$HOME"
        prefix="(${FIKORE_NS_LABEL}) "
        if [[ "$(basename "$SHELL")" == "bash" ]]; then
            rcfile="$(mktemp)"
            cat >"$rcfile" <<EOF
if [[ -f ~/.bashrc ]]; then
    . ~/.bashrc
fi
prefix="${prefix}"
if [[ "\${PS1:-}" != "\${prefix}"* ]]; then
    PS1="\${prefix}\${PS1:-}"
fi
rm -f -- "\${BASH_SOURCE[0]}"
unset prefix
EOF
            exec "$SHELL" --noprofile --rcfile "$rcfile" -i
        fi

        if [[ "${PS1:-}" != "${prefix}"* ]]; then
            export PS1="${prefix}${PS1:-}"
        fi
        exec "$SHELL" -i
    '
}

namespace_exec() {
    local target="${1:-}"
    shift || true
    [[ -n "${target}" ]] || die "Missing target namespace. Use ueN, dn, or emu."
    [[ "${1:-}" == "--" ]] || die "Usage: $(basename "$0") exec <ueN|dn|emu> -- <command> [args...]"
    shift
    [[ "$#" -gt 0 ]] || die "Missing command for exec action."

    ensure_namespace_target_ready "${target}"
    log "Running command in ${TARGET_NAMESPACE} as ${TARGET_RUN_USER}: $*"
    run_in_target_namespace "$@"
}

ensure_container_running() {
    container_required || return 0
    docker inspect "${CONTAINER_NAME}" >/dev/null 2>&1 || die "Container ${CONTAINER_NAME} does not exist. Launch it with run_docker.sh."
    local running
    running="$(docker inspect -f '{{.State.Running}}' "${CONTAINER_NAME}")"
    [[ "${running}" == "true" ]] || die "Container ${CONTAINER_NAME} is not running."
}

node_exec() {
    if emu_node_is_host; then
        "$@"
    else
        local pid
        pid="$(docker_pid)"
        [[ -n "${pid}" && "${pid}" != "0" ]] || die "Could not get PID for ${CONTAINER_NAME}"
        nsenter -t "${pid}" -n "$@"
    fi
}

node_iptables() {
    node_exec iptables "$@"
}

node_tcpdump_to_file() {
    local outfile="$1"
    local logfile="$2"
    shift
    shift
    if emu_node_is_host; then
        tcpdump "$@" -w "${outfile}" >"${logfile}" 2>&1 &
        echo $!
    else
        local pid
        pid="$(docker_pid)"
        [[ -n "${pid}" && "${pid}" != "0" ]] || die "Could not get PID for ${CONTAINER_NAME}"
        nsenter -t "${pid}" -n tcpdump "$@" -w - > "${outfile}" 2>"${logfile}" &
        echo $!
    fi
}

create_veth_pair() {
    local host_if="$1"
    local peer_if="$2"
    if ip link show "${host_if}" >/dev/null 2>&1; then
        ip link del "${host_if}"
    fi
    ip link add "${host_if}" type veth peer name "${peer_if}"
}

setup_single_ue_namespace() {
    local i="$1"
    local ns host_if emu_if ue_ip emu_ip
    ns="$(ue_ns "${i}")"
    host_if="$(ue_host_if "${i}")"
    emu_if="$(ue_emu_if "${i}")"
    ue_ip="$(ue_real_ip "${i}")"
    emu_ip="$(ue_emu_ip "${i}")"

    ip netns del "${ns}" >/dev/null 2>&1 || true
    ip netns add "${ns}"

    create_veth_pair "${host_if}" "${emu_if}"
    ip link set "${host_if}" netns "${ns}"

    if emu_node_is_host; then
        ip addr add "${emu_ip}/30" dev "${emu_if}"
        ip link set "${emu_if}" up
    else
        local pid
        pid="$(docker_pid)"
        ip link set "${emu_if}" netns "${pid}"
        node_exec ip addr add "${emu_ip}/30" dev "${emu_if}"
        node_exec ip link set "${emu_if}" up
    fi

    ip -n "${ns}" addr add "${ue_ip}/30" dev "${host_if}"
    ip -n "${ns}" link set lo up
    ip -n "${ns}" link set "${host_if}" up
    ip -n "${ns}" route add default via "${emu_ip}"
}

setup_ue_namespaces() {
    local i
    while read -r i; do
        [[ -n "${i}" ]] || continue
        setup_single_ue_namespace "${i}"
    done < <(active_ue_indices)
}

delete_single_ue_namespace() {
    local i="$1"
    ip netns del "$(ue_ns "${i}")" >/dev/null 2>&1 || true
    ip link del "$(ue_emu_if "${i}")" >/dev/null 2>&1 || true
    ip link del "$(ue_host_if "${i}")" >/dev/null 2>&1 || true
}

cleanup_known_ue_namespaces() {
    local ns idx
    while read -r ns _; do
        [[ "${ns}" =~ ^fkue([0-9]+)$ ]] || continue
        idx="${BASH_REMATCH[1]}"
        delete_single_ue_namespace "${idx}"
    done < <(ip netns list)
}

cleanup_known_links() {
    local link
    for link in "$(dn_emu_if)" "$(dn_host_if)" "$(mgmt_host_if)"; do
        ip link del "${link}" >/dev/null 2>&1 || true
    done
}

setup_dn_namespace() {
    local ns host_if emu_if
    ns="$(dn_ns)"
    host_if="$(dn_host_if)"
    emu_if="$(dn_emu_if)"

    ip netns del "${ns}" >/dev/null 2>&1 || true
    ip netns add "${ns}"

    create_veth_pair "${host_if}" "${emu_if}"
    ip link set "${host_if}" netns "${ns}"

    if emu_node_is_host; then
        ip addr add "${DN_EMU_IP}/${DN_CIDR}" dev "${emu_if}"
        ip link set "${emu_if}" up
    else
        local pid
        pid="$(docker_pid)"
        ip link set "${emu_if}" netns "${pid}"
        node_exec ip addr add "${DN_EMU_IP}/${DN_CIDR}" dev "${emu_if}"
        node_exec ip link set "${emu_if}" up
    fi

    ip -n "${ns}" addr add "${DN_REAL_IP}/${DN_CIDR}" dev "${host_if}"
    ip -n "${ns}" link set lo up
    ip -n "${ns}" link set "${host_if}" up
    ip -n "${ns}" route add "${UE_PREFIX}.0.0/16" via "${DN_EMU_IP}"
}

setup_mgmt_plane() {
    [[ "${BACKEND}" == "docker-none" ]] || return 0

    local host_if emu_if pid
    host_if="$(mgmt_host_if)"
    emu_if="$(mgmt_emu_if)"
    pid="$(docker_pid)"

    create_veth_pair "${host_if}" "${emu_if}"
    ip addr add "${MGMT_HOST_IP}/${MGMT_CIDR}" dev "${host_if}"
    ip link set "${host_if}" up

    ip link set "${emu_if}" netns "${pid}"
    node_exec ip addr add "${MGMT_EMU_IP}/${MGMT_CIDR}" dev "${emu_if}"
    node_exec ip link set "${emu_if}" up
}

configure_emu_node() {
    node_exec ip link set lo up
    node_exec sysctl -q -w net.ipv4.ip_forward=1
    node_exec sysctl -q -w net.ipv4.conf.all.rp_filter=0
}

setup_iptables_chain() {
    node_iptables -N "${CHAIN_NAME}" 2>/dev/null || true
    node_iptables -F "${CHAIN_NAME}"
    node_iptables -C FORWARD -j "${CHAIN_NAME}" >/dev/null 2>&1 || node_iptables -I FORWARD 1 -j "${CHAIN_NAME}"
}

setup_nfqueue_rules() {
    setup_iptables_chain

    local dn_if i
    dn_if="$(dn_emu_if)"

    while read -r i; do
        [[ -n "${i}" ]] || continue
        local ue_if ue_ip q_ul q_dl
        ue_if="$(ue_emu_if "${i}")"
        ue_ip="$(ue_real_ip "${i}")"
        q_ul="$(queue_ul "${i}")"
        q_dl="$(queue_dl "${i}")"

        node_iptables -A "${CHAIN_NAME}" -i "${ue_if}" -o "${dn_if}" -s "${ue_ip}" -d "${DN_REAL_IP}" -j NFQUEUE --queue-num "${q_ul}"
        node_iptables -A "${CHAIN_NAME}" -i "${ue_if}" -o "${dn_if}" -s "${ue_ip}" -d "${DN_REAL_IP}" -j ACCEPT
        node_iptables -A "${CHAIN_NAME}" -i "${dn_if}" -o "${ue_if}" -s "${DN_REAL_IP}" -d "${ue_ip}" -j NFQUEUE --queue-num "${q_dl}"
        node_iptables -A "${CHAIN_NAME}" -i "${dn_if}" -o "${ue_if}" -s "${DN_REAL_IP}" -d "${ue_ip}" -j ACCEPT
    done < <(active_ue_indices)
}

teardown_nfqueue_rules() {
    node_iptables -D FORWARD -j "${CHAIN_NAME}" >/dev/null 2>&1 || true
    node_iptables -F "${CHAIN_NAME}" >/dev/null 2>&1 || true
    node_iptables -X "${CHAIN_NAME}" >/dev/null 2>&1 || true
}

cleanup_namespaces() {
    cleanup_known_ue_namespaces
    ip netns del "$(dn_ns)" >/dev/null 2>&1 || true
    cleanup_known_links
}

config_file_for_execution() {
    [[ -f "${CONFIG_FILE}" ]] || die "Config file does not exist: ${CONFIG_FILE}"

    case "${BACKEND}" in
        host)
            printf "%s" "${CONFIG_FILE}"
            ;;
        docker-host|docker-none)
            case "${CONFIG_FILE}" in
                "${ROOT_DIR}"/*)
                    printf "%s%s" "${CONTAINER_WORKDIR}" "${CONFIG_FILE#${ROOT_DIR}}"
                    ;;
                *)
                    die "CONFIG_FILE must be inside ${ROOT_DIR} for Docker backends so the container can read the same file."
                    ;;
            esac
            ;;
    esac
}

lab_exists() {
    [[ -f "${STATE_FILE}" ]]
}

lab_is_usable() {
    LAB_STATE_ERROR=""

    lab_exists || {
        LAB_STATE_ERROR="No saved lab state."
        return 1
    }

    validate_backend

    if container_required; then
        require_cmd docker
        if ! docker inspect "${CONTAINER_NAME}" >/dev/null 2>&1; then
            LAB_STATE_ERROR="Container ${CONTAINER_NAME} does not exist."
            return 1
        fi
        if [[ "$(docker inspect -f '{{.State.Running}}' "${CONTAINER_NAME}" 2>/dev/null)" != "true" ]]; then
            LAB_STATE_ERROR="Container ${CONTAINER_NAME} is not running."
            return 1
        fi
    fi

    local i
    while read -r i; do
        [[ -n "${i}" ]] || continue
        namespace_exists "$(ue_ns "${i}")" || {
            LAB_STATE_ERROR="Namespace $(ue_ns "${i}") is missing."
            return 1
        }
    done < <(active_ue_indices)

    namespace_exists "$(dn_ns)" || {
        LAB_STATE_ERROR="Namespace $(dn_ns) is missing."
        return 1
    }

    if [[ "${BACKEND}" == "docker-none" ]] && ! ip link show "$(mgmt_host_if)" >/dev/null 2>&1; then
        LAB_STATE_ERROR="Management interface $(mgmt_host_if) is missing."
        return 1
    fi

    return 0
}

start_topology() {
    apply_default_settings
    validate_backend
    require_root
    require_cmd ip
    require_cmd iptables
    require_cmd awk
    if container_required; then
        require_cmd docker
        ensure_container_running
    fi

    UE_ACTIVE_SET="$(default_active_ue_set "${UE_COUNT}")"
    cleanup_namespaces
    if emu_node_is_host; then
        teardown_nfqueue_rules || true
    elif [[ "${BACKEND}" == "docker-none" ]]; then
        teardown_nfqueue_rules || true
    fi

    configure_emu_node
    setup_ue_namespaces
    setup_dn_namespace
    setup_mgmt_plane
    setup_nfqueue_rules
    save_lab_state
}

run_fikore() {
    [[ -f "${ROOT_DIR}/bin/fikore" ]] || die "${ROOT_DIR}/bin/fikore does not exist. Build it first with make."
    local effective_config
    effective_config="$(config_file_for_execution)"

    case "${BACKEND}" in
        host)
            log "Running FikoRE on the host with ${CONFIG_FILE}"
            exec "${ROOT_DIR}/bin/fikore" "${effective_config}"
            ;;
        docker-host|docker-none)
            log "Running FikoRE in container ${CONTAINER_NAME} with ${CONFIG_FILE}"
            exec docker exec -i "${CONTAINER_NAME}" bash -lc \
                "cd '${CONTAINER_WORKDIR}' && ./bin/fikore '${effective_config}'"
            ;;
    esac
}

status_topology() {
    local i

    if ! lab_exists; then
        echo "LAB_STATE=missing"
        return 0
    fi

    if lab_is_usable; then
        echo "LAB_STATE=active"
    else
        echo "LAB_STATE=broken"
        echo "LAB_ERROR=${LAB_STATE_ERROR}"
    fi

    echo "BACKEND=${BACKEND}"
    echo "CONFIG_FILE=${CONFIG_FILE}"
    echo "UE_COUNT=${UE_COUNT}"
    echo "UE_ACTIVE_SET=${UE_ACTIVE_SET}"
    echo
    echo "Namespaces UE/DN:"
    while read -r i; do
        [[ -n "${i}" ]] || continue
        printf "  %s ue=%s emu=%s q_ul=%s q_dl=%s\n" \
            "$(ue_ns "${i}")" \
            "$(ue_real_ip "${i}")" \
            "$(ue_emu_ip "${i}")" \
            "$(queue_ul "${i}")" \
            "$(queue_dl "${i}")"
    done < <(active_ue_indices)
    printf "  %s ue=%s emu=%s\n" "$(dn_ns)" "${DN_REAL_IP}" "${DN_EMU_IP}"
    if [[ "${BACKEND}" == "docker-none" ]]; then
        printf "  mgmt host=%s emu=%s\n" "${MGMT_HOST_IP}" "${MGMT_EMU_IP}"
    fi
    echo
    echo "NFQUEUE rules:"
    if ! lab_is_usable; then
        echo "  unavailable (${LAB_STATE_ERROR})"
    elif emu_node_is_host; then
        iptables -S "${CHAIN_NAME}" 2>/dev/null || true
    else
        ensure_container_running
        docker exec "${CONTAINER_NAME}" iptables -S "${CHAIN_NAME}" 2>/dev/null || true
    fi
}

require_active_lab() {
    load_saved_lab_state
    if ! lab_exists; then
        die "No saved lab state. Run '$(basename "$0") up' first."
    fi
    if ! lab_is_usable; then
        die "Saved lab state is broken: ${LAB_STATE_ERROR} Run '$(basename "$0") up' to recreate the topology."
    fi
}

ensure_lab_for_run() {
    apply_default_settings

    if lab_exists; then
        if ! lab_is_usable; then
            die "Saved lab state is broken: ${LAB_STATE_ERROR} Run '$(basename "$0") up' to recreate the topology."
        fi
        save_lab_state
        return 0
    fi

    log "No active lab found. Creating topology first."
    start_topology
}

cleanup_all() {
    load_saved_lab_state
    require_root
    validate_backend

    if container_required; then
        require_cmd docker
    fi

    if emu_node_is_host; then
        teardown_nfqueue_rules || true
    elif [[ "${BACKEND}" == "docker-none" ]]; then
        if docker inspect "${CONTAINER_NAME}" >/dev/null 2>&1; then
            teardown_nfqueue_rules || true
        fi
    fi

    cleanup_namespaces
    remove_lab_state
}

capture_filter() {
    local filter="udp and host ${DN_REAL_IP}"
    if [[ -n "${CAPTURE_UDP_PORT}" ]]; then
        filter="${filter} and port ${CAPTURE_UDP_PORT}"
    fi
    printf "%s" "${filter}"
}

ensure_capture_requirements() {
    load_saved_lab_state
    require_root
    validate_backend
    require_cmd tcpdump
    require_cmd awk
    if container_required; then
        require_cmd docker
        ensure_container_running
    fi
    [[ -e "/var/run/netns/$(dn_ns)" ]] || die "Namespace $(dn_ns) does not exist. Run 'up' or 'run' first."
    if emu_node_is_host; then
        ip link show "$(dn_emu_if)" >/dev/null 2>&1 || die "Interface $(dn_emu_if) does not exist. Bring up the topology first."
    else
        node_exec ip link show "$(dn_emu_if)" >/dev/null 2>&1 || die "Interface $(dn_emu_if) does not exist on the EMU node."
    fi
}

capture_start() {
    ensure_capture_requirements

    local session_dir meta_file filter emu_pid dn_pid rotate_args
    session_dir="$(capture_session_dir)"
    meta_file="$(capture_meta_file)"
    filter="$(capture_filter)"

    if [[ -d "${session_dir}" ]]; then
        die "Capture session ${CAPTURE_ID} already exists in ${session_dir}. Use another CAPTURE_ID or remove the session."
    fi

    mkdir -p "${session_dir}"

    rotate_args=()
    if [[ "${CAPTURE_ROTATE_SECONDS}" -gt 0 ]]; then
        rotate_args=(-G "${CAPTURE_ROTATE_SECONDS}" -W 1)
    fi

    log "Starting EMU capture on $(dn_emu_if) with filter: ${filter}"
    emu_pid="$(node_tcpdump_to_file "$(capture_pcap_file emu)" "$(capture_log_file emu)" -U -n -s 0 -i "$(dn_emu_if)" "${rotate_args[@]}" "${filter}")"
    printf "%s\n" "${emu_pid}" > "$(capture_pid_file emu)"
    ensure_capture_started emu "${emu_pid}"

    if [[ "${CAPTURE_DN_NAMESPACE}" == "1" ]]; then
        log "Starting DN capture in $(dn_ns):$(dn_host_if) with filter: ${filter}"
        ip netns exec "$(dn_ns)" tcpdump -U -n -s 0 -i "$(dn_host_if)" "${rotate_args[@]}" "${filter}" -w "$(capture_pcap_file dn)" >"$(capture_log_file dn)" 2>&1 &
        dn_pid=$!
        printf "%s\n" "${dn_pid}" > "$(capture_pid_file dn)"
        ensure_capture_started dn "${dn_pid}"
    fi

    cat > "${meta_file}" <<EOF
CAPTURE_ID='${CAPTURE_ID}'
BACKEND='${BACKEND}'
FILTER='${filter}'
EMU_IF='$(dn_emu_if)'
DN_NS='$(dn_ns)'
DN_IF='$(dn_host_if)'
CAPTURE_DN_NAMESPACE='${CAPTURE_DN_NAMESPACE}'
CAPTURE_UDP_PORT='${CAPTURE_UDP_PORT}'
EOF

    log "Captures active in $(capture_session_dir)"
}

stop_capture_pid() {
    local name="$1"
    local pid_file pid
    pid_file="$(capture_pid_file "${name}")"
    [[ -f "${pid_file}" ]] || return 0
    pid="$(cat "${pid_file}")"
    if [[ -n "${pid}" ]] && kill -0 "${pid}" >/dev/null 2>&1; then
        kill "${pid}" >/dev/null 2>&1 || true
        wait "${pid}" 2>/dev/null || true
    fi
    rm -f "${pid_file}"
}

ensure_capture_started() {
    local name="$1"
    local pid="$2"
    local logfile
    logfile="$(capture_log_file "${name}")"
    sleep 1
    if [[ -z "${pid}" ]] || ! kill -0 "${pid}" >/dev/null 2>&1; then
        echo "[ERROR] Capture ${name} did not start correctly." >&2
        if [[ -f "${logfile}" ]]; then
            sed -n '1,40p' "${logfile}" >&2
        fi
        exit 1
    fi
}

capture_stop() {
    ensure_capture_requirements
    [[ -d "$(capture_session_dir)" ]] || die "Capture session does not exist: $(capture_session_dir)"
    stop_capture_pid emu
    stop_capture_pid dn
    log "Captures stopped in $(capture_session_dir)"
}

summarize_pcap() {
    local pcap="$1"
    local label="$2"
    [[ -f "${pcap}" ]] || return 0

    echo "${label}: ${pcap}"
    tcpdump -n -vvv -r "${pcap}" 2>/dev/null | awk '
        /tos 0x/ {
            total++
            if ($0 ~ /CE/) ce++
            else if ($0 ~ /ECT\(1\)/) ect1++
            else if ($0 ~ /ECT\(0\)/) ect0++
            else if ($0 ~ /Not-ECT/) notect++
            if ($0 ~ /\[bad .* cksum/) bad_cksum++
            samples[count++] = $0
        }
        END {
            printf("  total_ip=%d ce=%d ect1=%d ect0=%d notect=%d bad_cksum=%d\n",
                   total + 0, ce + 0, ect1 + 0, ect0 + 0, notect + 0, bad_cksum + 0)
            limit = count < 5 ? count : 5
            for (i = 0; i < limit; i++) {
                printf("  sample[%d] %s\n", i + 1, samples[i])
            }
        }
    '
}

capture_summary() {
    ensure_capture_requirements
    [[ -d "$(capture_session_dir)" ]] || die "Capture session does not exist: $(capture_session_dir)"
    [[ -f "$(capture_meta_file)" ]] && . "$(capture_meta_file)"

    echo "CAPTURE_ID=${CAPTURE_ID}"
    echo "FILTER=${FILTER:-$(capture_filter)}"
    summarize_pcap "$(capture_pcap_file emu)" "EMU"
    summarize_pcap "$(capture_pcap_file dn)" "DN"
}

ue_add() {
    local ue_idx="${1:-}"
    [[ "${ue_idx}" =~ ^[1-9][0-9]*$ ]] || die "Usage: $(basename "$0") ue-add <index>"

    require_active_lab
    require_root
    require_cmd ip
    require_cmd iptables

    if ue_set_contains "${ue_idx}"; then
        die "UE${ue_idx} is already present in the lab."
    fi

    add_ue_to_active_set "${ue_idx}"
    setup_single_ue_namespace "${ue_idx}"
    teardown_nfqueue_rules
    setup_nfqueue_rules
    save_lab_state
    log "Added UE${ue_idx} to the lab topology."
}

ue_del() {
    local ue_idx="${1:-}"
    [[ "${ue_idx}" =~ ^[1-9][0-9]*$ ]] || die "Usage: $(basename "$0") ue-del <index>"

    require_active_lab
    require_root
    require_cmd ip
    require_cmd iptables

    ue_set_contains "${ue_idx}" || die "UE${ue_idx} is not part of the current lab."

    delete_single_ue_namespace "${ue_idx}"
    remove_ue_from_active_set "${ue_idx}"
    teardown_nfqueue_rules
    setup_nfqueue_rules
    save_lab_state
    log "Removed UE${ue_idx} from the lab topology."
}

usage() {
    cat <<EOF
Usage: $(basename "$0") [run|up|down|status|capture-start|capture-stop|capture-summary|shell|exec|ue-add|ue-del]

Variables:
  BACKEND=host|docker-host|docker-none
  CONFIG_FILE=/path/to/config.ini
  UE_COUNT=2
  CONTAINER_NAME=fikore-emu
  CAPTURE_ID=default
  CAPTURE_UDP_PORT=9999
  CAPTURE_DN_NAMESPACE=1
  CAPTURE_DIR=${ROOT_DIR}/run_scripts/.captures
  TARGET_UID=<uid>
  TARGET_GID=<gid>
  TARGET_USER=<user>
  TARGET_HOME=/home/<user>

Examples:
  sudo BACKEND=host UE_COUNT=2 $(basename "$0") up
  sudo CONFIG_FILE=$PWD/config/emulated_rural_n78_single_with_background.ini $(basename "$0") run
  sudo BACKEND=docker-host $(basename "$0") run
  sudo $(basename "$0") status
  sudo $(basename "$0") shell ue1
  sudo $(basename "$0") exec dn -- iperf3 -s -p 5202
  sudo $(basename "$0") ue-add 3
  sudo $(basename "$0") ue-del 3
  sudo CAPTURE_ID=ce-check CAPTURE_UDP_PORT=9999 $(basename "$0") capture-start
  sudo CAPTURE_ID=ce-check $(basename "$0") capture-stop
  sudo CAPTURE_ID=ce-check $(basename "$0") capture-summary
EOF
}

case "${ACTION}" in
    run)
        load_lab_state_for_run
        ensure_lab_for_run
        run_fikore
        ;;
    up)
        start_topology
        status_topology
        ;;
    down)
        cleanup_all
        ;;
    status)
        load_saved_lab_state
        status_topology
        ;;
    capture-start)
        capture_start
        ;;
    capture-stop)
        capture_stop
        ;;
    capture-summary)
        capture_summary
        ;;
    shell)
        shift
        require_active_lab
        namespace_shell "${1:-}"
        ;;
    exec)
        shift
        require_active_lab
        namespace_exec "$@"
        ;;
    ue-add)
        shift
        ue_add "${1:-}"
        ;;
    ue-del)
        shift
        ue_del "${1:-}"
        ;;
    *)
        usage
        exit 1
        ;;
esac
