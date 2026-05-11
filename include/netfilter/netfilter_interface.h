#ifndef NETFILTER_INTERFACE_H
#define NETFILTER_INTERFACE_H

//#define DEBUG_NETFILTER_INTERFACE // Uncomment/Comment to enable/disable debug
#include <stdint.h>
#include <functional>
#include <vector>

extern "C" {

typedef struct netfilter_interface netfilter_interface_t;

struct nfq_packet_metadata
{
	uint64_t timestamp_sec = 0;
	uint64_t timestamp_usec = 0;
	uint64_t bytes = 0;
	uint32_t pkt_id = 0;
	uint8_t ecn = 0;
	std::vector<uint8_t> payload;
};

typedef std::function<void(void*, netfilter_interface_t*, const nfq_packet_metadata&)> add_pkt_callback_t;

struct netfilter_interface {
	int queue_num;
	unsigned int portid;
	struct mnl_socket *nl;
	add_pkt_callback_t callback;
	void *callback_data;
	char *buf;
	size_t sizeof_buf;
	pthread_t tid;
	uint32_t last_recv_id; // Last received packed it (for debug)
	uint32_t total_recv; // Total received packets (for debug)
	uint32_t bytes_recv;
	uint32_t last_rlsd_id; // Last released packed it (for debug)
	uint32_t total_rlsd; // Total released packets (for debug)
	uint32_t recv_fails;
	uint32_t rlsd_fails;
};

netfilter_interface_t *netfilter_interface_open(int queue_num, add_pkt_callback_t callback, void *handler);

int netfilter_interface_release_pkt(netfilter_interface_t *nfiface, uint32_t pkt_id, int accept);
int netfilter_interface_release_pkt_payload(netfilter_interface_t *nfiface, uint32_t pkt_id, int accept, const uint8_t *payload, uint32_t payload_len);

void netfilter_interface_close(netfilter_interface_t *nfiface);

}

#endif
