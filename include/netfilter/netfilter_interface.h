#ifndef NETFILTER_INTERFACE_H
#define NETFILTER_INTERFACE_H

//#define DEBUG_NETFILTER_INTERFACE // Uncomment/Comment to enable/disable debug
#include <stdint.h>
#include <functional>

extern "C" {

typedef struct netfilter_interface netfilter_interface_t;

typedef std::function<void(void*, netfilter_interface_t*, uint64_t, uint64_t, uint64_t, uint32_t)> add_pkt_callback_t;

struct netfilter_interface {
	int queue_num;
	unsigned int portid;
	struct mnl_socket *nl;
	add_pkt_callback_t callback;
	void *callback_data;
	char *buf;
	size_t sizeof_buf;
	pthread_t tid;
};

netfilter_interface_t *netfilter_interface_open(int queue_num, add_pkt_callback_t callback, void *handler);

int netfilter_interface_release_pkt(netfilter_interface_t *nfiface, uint32_t pkt_id, int accept);

void netfilter_interface_close(netfilter_interface_t *nfiface);

}

#endif
