#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <pthread.h>

#define _BSD_SOURCE             /* See feature_test_macros(7) */
#include <endian.h>

#include <libmnl/libmnl.h>
#include <linux/netfilter.h>
#include <linux/netfilter/nfnetlink.h>

#include <linux/types.h>
#include <linux/netfilter/nfnetlink_queue.h>

#include <libnetfilter_queue/libnetfilter_queue.h>

// We are using linux/... header files for homogeneity (instead of e.g. <netinet/ip.h>)
#include <linux/if_ether.h> // ETH_P_IP
#include <linux/ip.h>



#include <netfilter/netfilter_interface.h>

/*

Our user-space application communicates with the kernel module (for nefilter_queue) using Netlink sockets.

We need to understand all the levels of the protocol:

1. NETLINK

libmnl is the library handling the netlink communication. All mnl_* functions refer to this library.
See <libmnl/libmnl.h> for details.

At NETLINK protocol level, messages have the following format:

        |<----------------- 4 bytes ------------------->|
        |<----- 2 bytes ------>|<------- 2 bytes ------>|
        |-----------------------------------------------|
        |      Message length (including header)        |
        |-----------------------------------------------|
        |     Message type     |     Message flags      |
        |-----------------------------------------------|
        |           Message sequence number             |
        |-----------------------------------------------|
        |                 Netlink PortID                |
        |-----------------------------------------------|
        |                                               |
        .                   Payload                     .
        |_______________________________________________|


Payload is a TLV structure handled by libmnl.

The message itself (struct nlmsghdr) is defined in <netlink.h>

struct nlmsghdr
{
        __u32                nlmsg_len;        // Length of message including header 
        __u16                nlmsg_type;       // Message content 
        __u16                nlmsg_flags;      // Additional flags
        __u32                nlmsg_seq;        // Sequence number
        __u32                nlmsg_pid;        // Sending process port ID
};

We are using always REQUEST messagses (NLM_F_REQUEST, as defined in <netlink.h>)


2. NETFILTER

Within these messages, we communicate specifically with NETFILTER -- see <linux/netfilter/nfnetlink.h>.

NETLINK NETFILTER  message types are split in two pieces: 8 bit subsystem, 8bit operation

	- Message type: nlmsg_type = (NFNL_SUBSYS_QUEUE << 8) | type 

In our case, the subsystem will be NFNL_SUBSYS_QUEUE. The operation ("type") depends on the subsystem.

All netlink-netfilter messages have this header at the beginning of the payload:

struct nfgenmsg {
	__u8  nfgen_family;		// AF_xxx
	__u8  version;		    // nfnetlink version --> it is always NFNETLINK_V0 = 0
	__be16    res_id;		  // resource id
};

Therefore if you do

struct nlmsghdr *nlh = your parsed netlink message from libmnl
struct nfgenmsg *nfg = mnl_nlmsg_get_payload(nlh);

then you get a pointer to the _beginning_ of the payload of the netlink message, which is this netlink-netfilter header.

(This is implemented in "nlmsg.c").


3. NETFILTER_QUEUE

And now we arrive to our specific netlink messages which communicate with netfilter-queue module.
 
This is at <linux/netfilter/nfnetlink_queue.h> and it includes NFQNL prefix

We have:

a) At netlink level <netlink.h>:
		- type = (NFNL_SUBSYS_QUEUE << 8) | NFQNL_MSG_XXXXX
		- flags = NLM_F_REQUEST

b) At netlink-netfilter level <linux/netfilter/nfnetlink.h>:
		- nfgen_family = AF_UNSPEC
		- version = 0
		- res_id = queue_number --> The NFQUEUE number that we configure with iptables (0 by default)

c) The specific data depending on the  NFQNL_MSG_XXXXX type

We have these type of messages:

	NFQNL_MSG_PACKET,		// packet from kernel to userspace 
	NFQNL_MSG_VERDICT,	// verdict from userspace to kernel
	NFQNL_MSG_CONFIG,		// connect to a particular queue
	NFQNL_MSG_VERDICT_BATCH,	// batchv from userspace to kernel 


3.1. NFQNL_MSG_PACKET messages

These are the ones we receive with the packets from the kernel.

The packet message is filled by function nfqnl_build_packet_message() at "nfnetlink_queue.c". 
Go there for details (there is no other documentation).

NFQNL_MSG_PACKET messages have the following header:

struct nfqnl_msg_packet_hdr {
	__be32		packet_id;	  // unique ID of packet in queue 
	__be16		hw_protocol;	// hw protocol (network order) -> EtherType from ethernet header
	__u8	hook;		          // netfilter hook --> (enum nf_inet_hooks) from netfilter.h
} __attribute__ ((packed));

We expect the following values for hw_protocol (see https://en.wikipedia.org/wiki/EtherType)
	- 0x0800 -> IPV4
	- 0x86DD -> IPV6

We expect the following values for hook:
	NF_INET_PRE_ROUTING = 0
	NF_INET_LOCAL_IN = 1
	NF_INET_FORWARD = 2
	NF_INET_LOCAL_OUT = 3
	NF_INET_POST_ROUTING = 4

The packet contains several attributes within its TLV structures:

enum nfqnl_attr_type {
	NFQA_UNSPEC,
	NFQA_PACKET_HDR,
	NFQA_VERDICT_HDR,		// nfqnl_msg_verdict_hrd 
	NFQA_MARK,			// __u32 nfmark 
	NFQA_TIMESTAMP,			// nfqnl_msg_packet_timestamp 
	NFQA_IFINDEX_INDEV,		// __u32 ifindex 
	NFQA_IFINDEX_OUTDEV,		// __u32 ifindex 
	NFQA_IFINDEX_PHYSINDEV,		// __u32 ifindex 
	NFQA_IFINDEX_PHYSOUTDEV,	// __u32 ifindex 
	NFQA_HWADDR,			// nfqnl_msg_packet_hw 
	NFQA_PAYLOAD,			// opaque data payload 
	NFQA_CT,			// nf_conntrack_netlink.h 
	NFQA_CT_INFO,			// enum ip_conntrack_info 
	NFQA_CAP_LEN,			// __u32 length of captured packet 
	NFQA_SKB_INFO,			// __u32 skb meta information 
	NFQA_EXP,			// nf_conntrack_netlink.h 
	NFQA_UID,			// __u32 sk uid 
	NFQA_GID,			// __u32 sk gid 
	NFQA_SECCTX,			// security context string 
	NFQA_VLAN,			// nested attribute: packet vlan info 
	NFQA_L2HDR,			// full L2 header 

	__NFQA_MAX
};

But some of them, including the payload *and the length fo the captured packet (NFQA_CAP_LEN)* are only available
if we have configured the netfilter queue to copy the whole message (NFQNL_COPY_PACKET).

So the only possible mode for our purposes is NFQNL_COPY_PACKET.

HOWEVER, we can set the copy_range to a small value (but higher than zero!) if we want to copy only some bytes of the
packet for inspection (and it may hopefully reduce data transfer -- although it's suppose to happen on zerocopy mode, 
so let's see)


3.2. NFQNL_MSG_CONFIG messages

These are the configuration messages. So far we can just follow the existing recipe from the example code.

Except that when we configure:

nfq_nlmsg_cfg_put_params(nlh, NFQNL_COPY_PACKET, 0xffff);

We can replace 0xffff for our desired copy range

Check <linux/netfilter/nfnetlink_queue.h> to see all the parameters that can be configured. There are not so many.



3.3. NFQNL_MSG_VERDICT


Verdict header is the simplest:

struct nfqnl_msg_verdict_hdr {
	__be32 verdict; // Verdict from <netfilter.h>
	__be32 id; // This is the packet ID
};


Verdicts are:

#define NF_DROP 0
#define NF_ACCEPT 1
#define NF_STOLEN 2
#define NF_QUEUE 3
#define NF_REPEAT 4
#define NF_STOP 5

In practice, we shoud either NF_ACCEPT or NF_DROP

*/

#if 1 // Required to compile with libnetfilter_queue versions < 1.0.4
static struct nlmsghdr *
nfq_nlmsg_put(char *buf, int type, uint32_t queue_num)
{
	struct nlmsghdr *nlh = (nlmsghdr*)mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type	= (NFNL_SUBSYS_QUEUE << 8) | type;
	nlh->nlmsg_flags = NLM_F_REQUEST;

	struct nfgenmsg *nfg = (nfgenmsg*)mnl_nlmsg_put_extra_header(nlh, sizeof(*nfg));
	nfg->nfgen_family = AF_UNSPEC;
	nfg->version = NFNETLINK_V0;
	nfg->res_id = htons(queue_num);

	return nlh;
}
#endif


#ifdef DEBUG_NETFILTER_INTERFACE
#define PRINTF(...)  printf(__VA_ARGS__)
#define PERROR(...) perror(__VA_ARGS__)
#else
#define PRINTF(...)  
#define PERROR(...) 
#endif


static int queue_cb(const struct nlmsghdr *nlh, void *data)
{

	uint32_t id = 0;
	struct nfqnl_msg_packet_hdr *ph = NULL;
	struct nfqnl_msg_packet_timestamp *pts = NULL;
	struct nlattr *attr[NFQA_MAX+1] = {};
	uint32_t orig_len = 0;
	uint64_t sec, usec;

	netfilter_interface_t *nfiface = (netfilter_interface_t *)data;

	if (nfq_nlmsg_parse(nlh, attr) < 0) {
		PERROR("problems parsing");
		return MNL_CB_ERROR;
	}

	if (attr[NFQA_PACKET_HDR] == NULL) {
		PRINTF("metaheader not set\n");
		return MNL_CB_ERROR;
	}
	ph = (nfqnl_msg_packet_hdr*)mnl_attr_get_payload(attr[NFQA_PACKET_HDR]);
	id = ntohl(ph->packet_id);

	if (attr[NFQA_CAP_LEN]) {
		orig_len = ntohl(mnl_attr_get_u32(attr[NFQA_CAP_LEN]));
	}
	else{
		orig_len = nlh->nlmsg_len;
	}

	if(attr[NFQA_TIMESTAMP]) {
		pts = (nfqnl_msg_packet_timestamp*)mnl_attr_get_payload(attr[NFQA_TIMESTAMP]);
		sec = be64toh(pts->sec);
		usec = be64toh(pts->usec);
	}
	else {
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		sec = ts.tv_sec;
		usec = ts.tv_nsec / 1000;
	}
	nfiface->callback(nfiface->callback_data, nfiface, sec, usec, orig_len, id);

#ifdef true

	
	uint32_t skbinfo;
	struct nfgenmsg *nfg;
	uint16_t plen;
	

	nfg = mnl_nlmsg_get_payload(nlh);


	plen = mnl_attr_get_payload_len(attr[NFQA_PAYLOAD]);
	if (orig_len != plen)
			PRINTF("truncated ");
	void *payload = mnl_attr_get_payload(attr[NFQA_PAYLOAD]);

	skbinfo = attr[NFQA_SKB_INFO] ? ntohl(mnl_attr_get_u32(attr[NFQA_SKB_INFO])) : 0;


	if (skbinfo & NFQA_SKB_GSO)
		PRINTF("GSO ");

	PRINTF("packet received (id=%u hw=0x%04x hook=%u, payload len %u",
		id, ntohs(ph->hw_protocol), ph->hook, plen);

	// We have only called the bind to AF_INET protocol, so we shouldn't get anything else than ipv4...
	if(ntohs(ph->hw_protocol) == ETH_P_IP) {
		struct iphdr *ip = (struct iphdr*) payload;
		if(plen < sizeof(*ip)) {
			PRINTF("Not enough size to parse IP packet!!\n"); // FIXME return not ok or something
		}
		else {
			uint8_t *c = (uint8_t *) &(ip->saddr);
			PRINTF(", src=%u.%u.%u.%u", c[0], c[1], c[2], c[3]);
			c = (uint8_t *)  &(ip->daddr);
			PRINTF(", dst=%u.%u.%u.%u", c[0], c[1], c[2], c[3]);
		}
	}



	/*
	 * ip/tcp checksums are not yet valid, e.g. due to GRO/GSO.
	 * The application should behave as if the checksums are correct.
	 *
	 * If these packets are later forwarded/sent out, the checksums will
	 * be corrected by kernel/hardware.
	 */
	if (skbinfo & NFQA_SKB_CSUMNOTREADY)
		PRINTF(", checksum not ready");
	//PRINTF(")\n");

	//nfq_send_verdict(ntohs(nfg->res_id), id);
#endif

	return MNL_CB_OK;
}


static void *run(void *arg) {
	netfilter_interface_t *nfiface = (netfilter_interface_t *)arg;
	
	/* ENOBUFS is signalled to userspace when packets were lost
	 * on kernel side.  In most cases, userspace isn't interested
	 * in this information, so turn it off.
	 */
	int ret = 1;
	//mnl_socket_setsockopt(nl, NETLINK_NO_ENOBUFS, &ret, sizeof(int));

	for (;;) {
		ret = mnl_socket_recvfrom(nfiface->nl, nfiface->buf, nfiface->sizeof_buf);
		if (ret == -1) {
			PERROR("mnl_socket_recvfrom");
			if(errno == ENOBUFS)
				continue; // Do not exit in this scenario	
			//exit(EXIT_FAILURE);
		}
		ret = mnl_cb_run(nfiface->buf, ret, 0, nfiface->portid, queue_cb, nfiface);
		if (ret < 0){
			PERROR("mnl_cb_run");
			//exit(EXIT_FAILURE);
		}
	}

}


static void netfilter_interface_free(netfilter_interface_t *iface) {
	if(iface) {
		mnl_socket_close(iface->nl);
		if(iface->buf)
			delete(iface->buf);
		delete(iface);
	}
}

void cb(void* handler, netfilter_interface_t *nfiface, uint64_t timestamp_sec, uint64_t timestamp_usec, 
               uint64_t bytes, uint32_t pkt_id)
               {   
                   netfilter_interface_release_pkt(nfiface, pkt_id, 1);
               }

netfilter_interface_t *netfilter_interface_open(int queue_num, add_pkt_callback_t callback, void *handler)
{
	struct nlmsghdr *nlh;
	int ret;

	netfilter_interface_t *nfiface = new netfilter_interface_t; 
	printf("Queue_num: %d", queue_num);
	nfiface->queue_num = queue_num;
	nfiface->callback = callback;
	nfiface->callback_data = handler;
	nfiface->buf = NULL;
	/* largest possible packet payload, plus netlink data overhead: */
	nfiface->sizeof_buf = 0xffff + (MNL_SOCKET_BUFFER_SIZE/2);;

	

	nfiface->nl = mnl_socket_open(NETLINK_NETFILTER);
	if (nfiface->nl == NULL) {
		PERROR("mnl_socket_open");
		netfilter_interface_free(nfiface);
		return NULL;
	}

	if (mnl_socket_bind(nfiface->nl, 0, MNL_SOCKET_AUTOPID) < 0) {
		PERROR("mnl_socket_bind");
		netfilter_interface_free(nfiface);
		return NULL;
	}

	nfiface->portid = mnl_socket_get_portid(nfiface->nl);

	nfiface->buf = (char*)malloc(nfiface->sizeof_buf);
	if (!nfiface->buf) {
		PERROR("allocate receive buffer");
		netfilter_interface_free(nfiface);
		return NULL;
	}

	nlh = nfq_nlmsg_put(nfiface->buf, NFQNL_MSG_CONFIG, nfiface->queue_num);
	nfq_nlmsg_cfg_put_cmd(nlh, AF_INET, NFQNL_CFG_CMD_BIND);

	if (mnl_socket_sendto(nfiface->nl, nlh, nlh->nlmsg_len) < 0) {
		PERROR("mnl_socket_send");
		netfilter_interface_free(nfiface);
		return NULL;
	}	
    
	nlh = nfq_nlmsg_put(nfiface->buf, NFQNL_MSG_CONFIG, nfiface->queue_num);
	nfq_nlmsg_cfg_put_params(nlh, NFQNL_COPY_PACKET, 0xffff);

	mnl_attr_put_u32(nlh, NFQA_CFG_FLAGS, htonl(NFQA_CFG_F_GSO));
	mnl_attr_put_u32(nlh, NFQA_CFG_MASK, htonl(NFQA_CFG_F_GSO));

	if (mnl_socket_sendto(nfiface->nl, nlh, nlh->nlmsg_len) < 0) {
		PERROR("mnl_socket_send");
		netfilter_interface_free(nfiface);
		return NULL;
	}
	
	ret = pthread_create(&nfiface->tid, NULL, &run, nfiface);

	if(ret) {
		PERROR("pthread_create");
		netfilter_interface_free(nfiface);
		return NULL;
	}
	return nfiface;
}


int netfilter_interface_release_pkt(netfilter_interface_t *nfiface, uint32_t pkt_id, int accept) {
	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh;
	nlh = nfq_nlmsg_put(buf, NFQNL_MSG_VERDICT, nfiface->queue_num);
	nfq_nlmsg_verdict_put(nlh, pkt_id, accept ? NF_ACCEPT : NF_DROP);

	if (mnl_socket_sendto(nfiface->nl, nlh, nlh->nlmsg_len) < 0) {
		PERROR("mnl_socket_send");
		return -1;
	}

	return 0;
}

void netfilter_interface_close(netfilter_interface_t *nfiface) {

	pthread_cancel(nfiface->tid);
	netfilter_interface_free(nfiface);

}
