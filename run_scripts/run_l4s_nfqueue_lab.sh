#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GENERATED_DIR="${ROOT_DIR}/run_scripts/.generated"

ACTION="${1:-run}"
BACKEND="${BACKEND:-host}"
CONFIG_FILE="${CONFIG_FILE:-${ROOT_DIR}/config/live_dashboard_multi_user.ini}"

CONTAINER_NAME="${CONTAINER_NAME:-fikore-emu}"
CONTAINER_WORKDIR="${CONTAINER_WORKDIR:-/usr/src/5g-network-emulator}"

UE_COUNT="${UE_COUNT:-2}"
QUEUE_BASE="${QUEUE_BASE:-100}"
QUEUE_STEP="${QUEUE_STEP:-10}"

UE_PREFIX="${UE_PREFIX:-10.201}"
DN_EMU_IP="${DN_EMU_IP:-10.255.0.1}"
DN_REAL_IP="${DN_REAL_IP:-10.255.0.2}"
DN_CIDR="${DN_CIDR:-24}"

MGMT_HOST_IP="${MGMT_HOST_IP:-172.30.0.1}"
MGMT_EMU_IP="${MGMT_EMU_IP:-172.30.0.2}"
MGMT_CIDR="${MGMT_CIDR:-30}"

MONITOR_OUTPUT_NAME="${MONITOR_OUTPUT_NAME:-dashboard_udp}"
MONITOR_PORT="${MONITOR_PORT:-8096}"
MONITOR_ADDRESS_OVERRIDE="${MONITOR_ADDRESS_OVERRIDE:-}"

CHAIN_NAME="${CHAIN_NAME:-FIKORE_EMU}"
CAPTURE_DIR="${CAPTURE_DIR:-${ROOT_DIR}/run_scripts/.captures}"
CAPTURE_ID="${CAPTURE_ID:-default}"
CAPTURE_UDP_PORT="${CAPTURE_UDP_PORT:-}"
CAPTURE_DN_NAMESPACE="${CAPTURE_DN_NAMESPACE:-1}"
CAPTURE_ROTATE_SECONDS="${CAPTURE_ROTATE_SECONDS:-0}"

RUNTIME_CONFIG="${GENERATED_DIR}/$(basename "${CONFIG_FILE%.ini}")_${BACKEND}.ini"

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
        die "Este script debe ejecutarse como root."
    fi
}

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "Falta el comando requerido: $1"
}

validate_backend() {
    case "${BACKEND}" in
        host|docker-host|docker-none) ;;
        *) die "BACKEND inválido: ${BACKEND}. Usa host, docker-host o docker-none." ;;
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

ensure_container_running() {
    container_required || return 0
    docker inspect "${CONTAINER_NAME}" >/dev/null 2>&1 || die "No existe el contenedor ${CONTAINER_NAME}. Lánzalo con run_docker.sh."
    local running
    running="$(docker inspect -f '{{.State.Running}}' "${CONTAINER_NAME}")"
    [[ "${running}" == "true" ]] || die "El contenedor ${CONTAINER_NAME} no está en ejecución."
}

node_exec() {
    if emu_node_is_host; then
        "$@"
    else
        local pid
        pid="$(docker_pid)"
        [[ -n "${pid}" && "${pid}" != "0" ]] || die "No se pudo obtener el PID de ${CONTAINER_NAME}"
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
        [[ -n "${pid}" && "${pid}" != "0" ]] || die "No se pudo obtener el PID de ${CONTAINER_NAME}"
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

setup_ue_namespaces() {
    local i
    for ((i = 1; i <= UE_COUNT; i++)); do
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

    for ((i = 1; i <= UE_COUNT; i++)); do
        local ue_if ue_ip q_ul q_dl
        ue_if="$(ue_emu_if "${i}")"
        ue_ip="$(ue_real_ip "${i}")"
        q_ul="$(queue_ul "${i}")"
        q_dl="$(queue_dl "${i}")"

        node_iptables -A "${CHAIN_NAME}" -i "${ue_if}" -o "${dn_if}" -s "${ue_ip}" -d "${DN_REAL_IP}" -j NFQUEUE --queue-num "${q_ul}"
        node_iptables -A "${CHAIN_NAME}" -i "${ue_if}" -o "${dn_if}" -s "${ue_ip}" -d "${DN_REAL_IP}" -j ACCEPT
        node_iptables -A "${CHAIN_NAME}" -i "${dn_if}" -o "${ue_if}" -s "${DN_REAL_IP}" -d "${ue_ip}" -j NFQUEUE --queue-num "${q_dl}"
        node_iptables -A "${CHAIN_NAME}" -i "${dn_if}" -o "${ue_if}" -s "${DN_REAL_IP}" -d "${ue_ip}" -j ACCEPT
    done
}

teardown_nfqueue_rules() {
    node_iptables -D FORWARD -j "${CHAIN_NAME}" >/dev/null 2>&1 || true
    node_iptables -F "${CHAIN_NAME}" >/dev/null 2>&1 || true
    node_iptables -X "${CHAIN_NAME}" >/dev/null 2>&1 || true
}

cleanup_namespaces() {
    local i
    for ((i = 1; i <= UE_COUNT; i++)); do
        ip netns del "$(ue_ns "${i}")" >/dev/null 2>&1 || true
        ip link del "$(ue_emu_if "${i}")" >/dev/null 2>&1 || true
        ip link del "$(ue_host_if "${i}")" >/dev/null 2>&1 || true
    done

    ip netns del "$(dn_ns)" >/dev/null 2>&1 || true
    ip link del "$(dn_emu_if)" >/dev/null 2>&1 || true
    ip link del "$(dn_host_if)" >/dev/null 2>&1 || true

    if [[ "${BACKEND}" == "docker-none" ]]; then
        ip link del "$(mgmt_host_if)" >/dev/null 2>&1 || true
    fi
}

monitor_address_for_backend() {
    if [[ -n "${MONITOR_ADDRESS_OVERRIDE}" ]]; then
        printf "%s" "${MONITOR_ADDRESS_OVERRIDE}"
        return
    fi

    case "${BACKEND}" in
        docker-none) printf "%s" "${MGMT_HOST_IP}" ;;
        *) printf "127.0.0.1" ;;
    esac
}

generate_runtime_config() {
    mkdir -p "${GENERATED_DIR}"
    [[ -f "${CONFIG_FILE}" ]] || die "No existe el fichero de config: ${CONFIG_FILE}"

    local monitor_addr monitor_key_addr monitor_key_port
    monitor_addr="$(monitor_address_for_backend)"
    monitor_key_addr="output_${MONITOR_OUTPUT_NAME}_address:"
    monitor_key_port="output_${MONITOR_OUTPUT_NAME}_port:"

    awk -v addr_key="${monitor_key_addr}" \
        -v port_key="${monitor_key_port}" \
        -v addr_val="${monitor_addr}" \
        -v port_val="${MONITOR_PORT}" '
        BEGIN {
            addr_done = 0;
            port_done = 0;
        }
        $1 == addr_key {
            print addr_key " " addr_val;
            addr_done = 1;
            next;
        }
        $1 == port_key {
            print port_key " " port_val;
            port_done = 1;
            next;
        }
        {
            print;
        }
        END {
            if (!addr_done) print addr_key " " addr_val;
            if (!port_done) print port_key " " port_val;
        }
    ' "${CONFIG_FILE}" > "${RUNTIME_CONFIG}"

    log "Config runtime generada en ${RUNTIME_CONFIG}"
}

start_topology() {
    validate_backend
    require_root
    require_cmd ip
    require_cmd iptables
    require_cmd awk
    if container_required; then
        require_cmd docker
        ensure_container_running
    fi

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
    generate_runtime_config
}

run_fikore() {
    [[ -f "${ROOT_DIR}/bin/fikore" ]] || die "No existe ${ROOT_DIR}/bin/fikore. Compila antes con make."

    case "${BACKEND}" in
        host)
            log "Ejecutando FikoRE en el host"
            exec "${ROOT_DIR}/bin/fikore" "${RUNTIME_CONFIG}"
            ;;
        docker-host|docker-none)
            log "Ejecutando FikoRE en el contenedor ${CONTAINER_NAME}"
            exec docker exec -i "${CONTAINER_NAME}" bash -lc \
                "cd '${CONTAINER_WORKDIR}' && ./bin/fikore '${CONTAINER_WORKDIR}/run_scripts/.generated/$(basename "${RUNTIME_CONFIG}")'"
            ;;
    esac
}

status_topology() {
    local i

    echo "BACKEND=${BACKEND}"
    echo "CONFIG_FILE=${CONFIG_FILE}"
    echo "RUNTIME_CONFIG=${RUNTIME_CONFIG}"
    echo
    echo "Namespaces UE/DN:"
    for ((i = 1; i <= UE_COUNT; i++)); do
        printf "  %s ue=%s emu=%s q_ul=%s q_dl=%s\n" \
            "$(ue_ns "${i}")" \
            "$(ue_real_ip "${i}")" \
            "$(ue_emu_ip "${i}")" \
            "$(queue_ul "${i}")" \
            "$(queue_dl "${i}")"
    done
    printf "  %s ue=%s emu=%s\n" "$(dn_ns)" "${DN_REAL_IP}" "${DN_EMU_IP}"
    if [[ "${BACKEND}" == "docker-none" ]]; then
        printf "  mgmt host=%s emu=%s\n" "${MGMT_HOST_IP}" "${MGMT_EMU_IP}"
    fi
    echo
    echo "Reglas NFQUEUE:"
    if emu_node_is_host; then
        iptables -S "${CHAIN_NAME}" 2>/dev/null || true
    else
        ensure_container_running
        docker exec "${CONTAINER_NAME}" iptables -S "${CHAIN_NAME}" 2>/dev/null || true
    fi
}

cleanup_all() {
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
}

capture_filter() {
    local filter="udp and host ${DN_REAL_IP}"
    if [[ -n "${CAPTURE_UDP_PORT}" ]]; then
        filter="${filter} and port ${CAPTURE_UDP_PORT}"
    fi
    printf "%s" "${filter}"
}

ensure_capture_requirements() {
    require_root
    validate_backend
    require_cmd tcpdump
    require_cmd awk
    if container_required; then
        require_cmd docker
        ensure_container_running
    fi
    [[ -e "/var/run/netns/$(dn_ns)" ]] || die "El namespace $(dn_ns) no existe. Ejecuta primero 'up' o 'run'."
    if emu_node_is_host; then
        ip link show "$(dn_emu_if)" >/dev/null 2>&1 || die "No existe la interfaz $(dn_emu_if). Levanta primero la topología."
    else
        node_exec ip link show "$(dn_emu_if)" >/dev/null 2>&1 || die "No existe la interfaz $(dn_emu_if) en el nodo EMU."
    fi
}

capture_start() {
    ensure_capture_requirements

    local session_dir meta_file filter emu_pid dn_pid rotate_args
    session_dir="$(capture_session_dir)"
    meta_file="$(capture_meta_file)"
    filter="$(capture_filter)"

    if [[ -d "${session_dir}" ]]; then
        die "La sesión de captura ${CAPTURE_ID} ya existe en ${session_dir}. Usa otro CAPTURE_ID o elimina la sesión."
    fi

    mkdir -p "${session_dir}"

    rotate_args=()
    if [[ "${CAPTURE_ROTATE_SECONDS}" -gt 0 ]]; then
        rotate_args=(-G "${CAPTURE_ROTATE_SECONDS}" -W 1)
    fi

    log "Iniciando captura EMU en $(dn_emu_if) con filtro: ${filter}"
    emu_pid="$(node_tcpdump_to_file "$(capture_pcap_file emu)" "$(capture_log_file emu)" -U -n -s 0 -i "$(dn_emu_if)" "${rotate_args[@]}" "${filter}")"
    printf "%s\n" "${emu_pid}" > "$(capture_pid_file emu)"
    ensure_capture_started emu "${emu_pid}"

    if [[ "${CAPTURE_DN_NAMESPACE}" == "1" ]]; then
        log "Iniciando captura DN en $(dn_ns):$(dn_host_if) con filtro: ${filter}"
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

    log "Capturas activas en $(capture_session_dir)"
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
        echo "[ERROR] La captura ${name} no arrancó correctamente." >&2
        if [[ -f "${logfile}" ]]; then
            sed -n '1,40p' "${logfile}" >&2
        fi
        exit 1
    fi
}

capture_stop() {
    ensure_capture_requirements
    [[ -d "$(capture_session_dir)" ]] || die "No existe la sesión de captura $(capture_session_dir)"
    stop_capture_pid emu
    stop_capture_pid dn
    log "Capturas detenidas en $(capture_session_dir)"
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
    [[ -d "$(capture_session_dir)" ]] || die "No existe la sesión de captura $(capture_session_dir)"
    [[ -f "$(capture_meta_file)" ]] && . "$(capture_meta_file)"

    echo "CAPTURE_ID=${CAPTURE_ID}"
    echo "FILTER=${FILTER:-$(capture_filter)}"
    summarize_pcap "$(capture_pcap_file emu)" "EMU"
    summarize_pcap "$(capture_pcap_file dn)" "DN"
}

usage() {
    cat <<EOF
Usage: $(basename "$0") [run|up|down|status|capture-start|capture-stop|capture-summary]

Variables:
  BACKEND=host|docker-host|docker-none
  CONFIG_FILE=/ruta/a/config.ini
  UE_COUNT=2
  CONTAINER_NAME=fikore-emu
  MONITOR_OUTPUT_NAME=dashboard_udp
  MONITOR_PORT=8096
  MONITOR_ADDRESS_OVERRIDE=...
  CAPTURE_ID=default
  CAPTURE_UDP_PORT=9999
  CAPTURE_DN_NAMESPACE=1
  CAPTURE_DIR=${ROOT_DIR}/run_scripts/.captures

Examples:
  sudo BACKEND=host $(basename "$0") run
  sudo BACKEND=docker-host $(basename "$0") run
  sudo BACKEND=docker-none $(basename "$0") up
  sudo BACKEND=host CAPTURE_ID=ce-check CAPTURE_UDP_PORT=9999 $(basename "$0") capture-start
  sudo BACKEND=host CAPTURE_ID=ce-check $(basename "$0") capture-stop
  sudo BACKEND=host CAPTURE_ID=ce-check $(basename "$0") capture-summary
EOF
}

case "${ACTION}" in
    run)
        start_topology
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
    *)
        usage
        exit 1
        ;;
esac
