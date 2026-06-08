#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

namespace
{
void die(const char* msg)
{
    perror(msg);
    std::exit(EXIT_FAILURE);
}

int parse_int(const char* s, const char* name)
{
    char* end = nullptr;
    long value = std::strtol(s, &end, 10);
    if(end == s || *end != '\0' || value <= 0 || value > 65535)
    {
        std::fprintf(stderr, "Valor inválido para %s: %s\n", name, s);
        std::exit(EXIT_FAILURE);
    }
    return static_cast<int>(value);
}
}

int main(int argc, char** argv)
{
    if(argc < 2 || argc > 3)
    {
        std::fprintf(stderr, "Usage: %s <port> [report_seconds]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const int port = parse_int(argv[1], "port");
    const int report_seconds = (argc == 3) ? parse_int(argv[2], "report_seconds") : 1;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) die("socket");

    int on = 1;
    if(setsockopt(sockfd, IPPROTO_IP, IP_RECVTOS, &on, sizeof(on)) < 0) die("setsockopt(IP_RECVTOS)");

    int rcvbuf = 4 * 1024 * 1024;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)) < 0) die("setsockopt(SO_RCVBUF)");

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) die("bind");

    std::printf("udp_tos_probe listening on UDP port %d, report interval %d s\n", port, report_seconds);

    unsigned long long total_packets = 0;
    unsigned long long ecn_counts[4] = {0, 0, 0, 0};
    unsigned long long no_tos_cmsg = 0;
    unsigned long long ctrunc = 0;

    unsigned long long interval_packets = 0;
    unsigned long long interval_ecn[4] = {0, 0, 0, 0};
    unsigned long long interval_no_tos_cmsg = 0;
    unsigned long long interval_ctrunc = 0;

    time_t interval_start = time(nullptr);

    for(;;)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        timeval timeout {};
        timeout.tv_sec = 0;
        timeout.tv_usec = 200000;

        int ready = select(sockfd + 1, &readfds, nullptr, nullptr, &timeout);
        if(ready < 0)
        {
            if(errno == EINTR) continue;
            die("select");
        }

        if(ready > 0 && FD_ISSET(sockfd, &readfds))
        {
            uint8_t buf[2048];
            uint8_t control[256];
            iovec iov {};
            iov.iov_base = buf;
            iov.iov_len = sizeof(buf);

            sockaddr_in src {};
            msghdr msg {};
            msg.msg_name = &src;
            msg.msg_namelen = sizeof(src);
            msg.msg_iov = &iov;
            msg.msg_iovlen = 1;
            msg.msg_control = control;
            msg.msg_controllen = sizeof(control);

            ssize_t n = recvmsg(sockfd, &msg, 0);
            if(n < 0)
            {
                if(errno == EINTR) continue;
                die("recvmsg");
            }

            bool found_tos = false;
            int ecn = -1;
            for(cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr; cmsg = CMSG_NXTHDR(&msg, cmsg))
            {
                if(cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TOS)
                {
                    uint8_t tos = *reinterpret_cast<uint8_t*>(CMSG_DATA(cmsg));
                    ecn = tos & 0x03;
                    found_tos = true;
                    break;
                }
            }

            total_packets++;
            interval_packets++;

            if(msg.msg_flags & MSG_CTRUNC)
            {
                ctrunc++;
                interval_ctrunc++;
            }

            if(found_tos && ecn >= 0 && ecn <= 3)
            {
                ecn_counts[ecn]++;
                interval_ecn[ecn]++;
            }
            else
            {
                no_tos_cmsg++;
                interval_no_tos_cmsg++;
            }
        }

        time_t now = time(nullptr);
        if(now - interval_start >= report_seconds)
        {
            const unsigned long long interval_ecn13 = interval_ecn[1] + interval_ecn[3];
            const unsigned long long total_ecn13 = ecn_counts[1] + ecn_counts[3];
            const double interval_ecn1_ratio = interval_ecn13 > 0
                ? static_cast<double>(interval_ecn[1]) / static_cast<double>(interval_ecn13)
                : 0.0;
            const double total_ecn1_ratio = total_ecn13 > 0
                ? static_cast<double>(ecn_counts[1]) / static_cast<double>(total_ecn13)
                : 0.0;
            std::printf(
                "interval pkts=%llu ecn0=%llu ecn1=%llu ecn2=%llu ecn3=%llu ecn1_ratio=%.6f no_tos_cmsg=%llu ctrunc=%llu | total pkts=%llu ecn0=%llu ecn1=%llu ecn2=%llu ecn3=%llu ecn1_ratio=%.6f no_tos_cmsg=%llu ctrunc=%llu\n",
                interval_packets,
                interval_ecn[0], interval_ecn[1], interval_ecn[2], interval_ecn[3],
                interval_ecn1_ratio,
                interval_no_tos_cmsg, interval_ctrunc,
                total_packets,
                ecn_counts[0], ecn_counts[1], ecn_counts[2], ecn_counts[3],
                total_ecn1_ratio,
                no_tos_cmsg, ctrunc);
            std::fflush(stdout);

            interval_packets = 0;
            interval_ecn[0] = interval_ecn[1] = interval_ecn[2] = interval_ecn[3] = 0;
            interval_no_tos_cmsg = 0;
            interval_ctrunc = 0;
            interval_start = now;
        }
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
