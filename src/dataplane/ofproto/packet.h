/*
 * Copyright 2014-2016 Nippon Telegraph and Telephone Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 *      @file   packet.h
 *      @brief  Packet structure definition
 */

#ifndef SRC_DATAPLANE_OFPROTO_PACKET_H_
#define SRC_DATAPLANE_OFPROTO_PACKET_H_

#ifdef HAVE_DPDK
#include "rte_config.h"
#include "rte_ether.h"
#define ether_header ether_hdr
#define ether_dhost  d_addr.addr_bytes
#define ether_shost  s_addr.addr_bytes
#ifndef __NET_ETHERNET_H
#define __NET_ETHERNET_H /* hmm, conflict ethernet.h with rte_ether.h */
#endif
#ifndef _NET_ETHERNET_H_
#define _NET_ETHERNET_H_ /* hmm, conflict ethernet.h with rte_ether.h */
#endif
#else
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#else
#include <net/if.h>
#include <net/if_ether.h>
#endif /* HAVE_NET_ETHERNET_H */
#endif /* HAVE_DPDK */

#if defined HYBRID && defined PIPELINER
#include "lagopus/pipeline.h"
#endif /* HYBRID && PIPELINER */

#define __FAVOR_BSD
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

#include "lagopus/flowinfo.h"
#include "mbtree.h"

/**
 * Ethernet type for VLAN
 */
#define ETHER_TYPE_VLAN         0x8100

/**
 * VLAN header
 */
struct vlanhdr {
  uint16_t vlan_tci;
  uint16_t vlan_eth;
} __attribute__((__packed__));

/**
 * Ethernet type for MPLS
 */
#define ETHER_TYPE_MPLS         0x8847
#define ETHER_TYPE_MPLS_MCAST   0x8848

/**
 * MPLS shim header.
 */
struct mpls_hdr {
  uint32_t mpls_lse;
} __attribute__((__packed__));

/**
 * label stack entry manipulate macros.  see RFC3032.
 */
#define MPLS_LBL_MASK 0xfffff000U
#define MPLS_EXP_MASK 0x00000e00U
#define MPLS_BOS_MASK 0x00000100U
#define MPLS_TTL_MASK 0x000000ffU

#define MPLS_LBL_SHIFT 12
#define MPLS_EXP_SHIFT 9
#define MPLS_BOS_SHIFT 8
#define MPLS_TTL_SHIFT 0

/* host byte order */
#define MPLS_LBL(lse) ((OS_NTOHL(lse)&MPLS_LBL_MASK) >> MPLS_LBL_SHIFT)
#define MPLS_EXP(lse) ((OS_NTOHL(lse)&MPLS_EXP_MASK) >> MPLS_EXP_SHIFT)
#define MPLS_BOS(lse) ((OS_NTOHL(lse)&MPLS_BOS_MASK) >> MPLS_BOS_SHIFT)
#define MPLS_TTL(lse) ((uint8_t)(OS_NTOHL(lse)&MPLS_TTL_MASK))

/* label is host byte order.  size of exp, bos and ttl are 1byte. */
#define SET_MPLS_LSE(lse, label, exp, bos, ttl) \
  do {                                                                  \
    (lse) = OS_HTONL(((label) << 12) | ((exp) << 9) | (bos) << 8 | ttl);  \
  } while (0)

/* parameter: host byte order */
#define SET_MPLS_LBL(lse, val)                          \
  do {                                                  \
    (lse) &= OS_HTONL(~MPLS_LBL_MASK);                  \
    (lse) |= OS_HTONL(OS_NTOHL(val) << MPLS_LBL_SHIFT); \
  } while (0)

#define SET_MPLS_EXP(lse, val)                               \
  do {                                                       \
    (lse) &= OS_HTONL(~MPLS_EXP_MASK);                       \
    (lse) |= OS_HTONL(((uint32_t)(val)) << MPLS_EXP_SHIFT);  \
  } while (0)

#define SET_MPLS_BOS(lse, val)                  \
  do {                                          \
    if (val == 0) {                             \
      (lse) &= OS_HTONL(~MPLS_BOS_MASK);        \
    } else {                                    \
      (lse) |= OS_HTONL(1 << MPLS_BOS_SHIFT);   \
    }                                           \
  } while (0)

#define SET_MPLS_TTL(lse, val)                  \
  do {                                          \
    (lse) &= OS_HTONL(~MPLS_TTL_MASK);          \
    (lse) |= OS_HTONL(val);                     \
  } while (0)

/**
 * Ethertype for 802.1ah Provider Backbone Bridging I-TAG frame
 */
#define ETHER_TYPE_PBB 0x88e7

struct pbb_hdr {
  uint8_t i_pcp_dei; /* |3:I-PCP|1:I-DEI|1:UCA|3:RES| */
  uint8_t i_sid[3];
  uint8_t c_dhost[6];
  uint8_t c_shost[6];
  uint16_t c_ethtype;
} __attribute__((__packed__));

/**
 * GRE
 */
struct gre_hdr {
  uint16_t flags; /* 13bit:flags, 3bit:ver */
  uint16_t ptype;
  uint32_t key;
#if 0
  uint32_t seq_num;
  uint32_t ack_num;
#endif
} __attribute__((__packed__));

/**
 * VXLAN
 */
struct vxlanhdr {
  uint8_t flags;
  uint8_t pad[3];
  uint32_t vni; /* 24bit:vni, 8bit:reserved */
} __attribute__((__packed__));

#define VXLAN_PORT 4789

/**
 * GTP-U
 */
struct gtpu_hdr {
  uint8_t verflags;
  uint8_t msgtype;
  uint16_t length;
  uint32_t te_id;
} __attribute__((__packed__));

#define GTPU_PORT 2152

/**
 * packet flags
 */
enum {
  PKT_FLAG_HAS_ACTION =          1 << 0,
  PKT_FLAG_CACHED_FLOW =         1 << 1,
  PKT_FLAG_RECALC_IPV4_CKSUM =   1 << 2,
  PKT_FLAG_RECALC_TCP_CKSUM =    1 << 3,
  PKT_FLAG_RECALC_UDP_CKSUM =    1 << 4,
  PKT_FLAG_RECALC_SCTP_CKSUM =   1 << 5,
  PKT_FLAG_RECALC_ICMP_CKSUM =   1 << 6,
  PKT_FLAG_RECALC_IPV6_CKSUM =   1 << 7,
  PKT_FLAG_RECALC_ICMPV6_CKSUM = 1 << 8,
};

#define PKT_FLAG_RECALC_L4_CKSUM (                                     \
    PKT_FLAG_RECALC_TCP_CKSUM |                                        \
    PKT_FLAG_RECALC_UDP_CKSUM |                                        \
    PKT_FLAG_RECALC_SCTP_CKSUM |                                       \
    PKT_FLAG_RECALC_ICMP_CKSUM |                                       \
    PKT_FLAG_RECALC_ICMPV6_CKSUM )

#define PKT_FLAG_RECALC_CKSUM_MASK (  \
    PKT_FLAG_RECALC_IPV4_CKSUM |      \
    PKT_FLAG_RECALC_IPV6_CKSUM |      \
    PKT_FLAG_RECALC_L4_CKSUM )

/**
 * max number of pipelines.
 */
#define LAGOPUS_DP_PIPELINE_MAX 254

#define MBUF2PKT(m) ((struct lagopus_packet *)&(m)[1])
#define PKT2MBUF(p) (&((OS_MBUF *)(p))[-1])

/**
 * lagopus packet structure
 */
struct lagopus_packet {
  union {
    uint64_t hash64;
    struct {
      uint32_t hash32_h;
      uint32_t hash32_l;
    };
  };

  /*
   * flowcache information.
   */
  void *cache;
  unsigned nmatched;
  const struct flow *matched_flow[LAGOPUS_DP_PIPELINE_MAX];

  /*
   * flow information.
   */
  uint8_t table_id;
  struct flow *flow;

  /* related port and bridge. */
  struct port *in_port;
  struct bridge *bridge;

  uint16_t ether_type;
  /*
   * access pointer placeholder, like skb
   */
  struct oob_data oob_data;
  struct oob2_data oob2_data;
  struct vlanhdr *vlan;
  union {
    uint8_t *base[MAX_BASE];
    struct {
      struct oob_data *oob;
      union {
        struct ether_header *eth;
        uint8_t *l2_hdr;
        uint16_t *l2_hdr_w;
        uint32_t *l2_hdr_l;
      };
      struct pbb_hdr *pbb;
      struct mpls_hdr *mpls;
      union {
        uint8_t *l3_hdr;
        uint16_t *l3_hdr_w;
        uint32_t *l3_hdr_l;
        struct ether_arp *arp;
        struct ip *ipv4;
        struct ip6_hdr *ipv6;
      };
      uint8_t *proto;
      union {
        uint8_t *l4_hdr;
        uint16_t *l4_hdr_w;
        uint32_t *l4_hdr_l;
        struct icmp *icmp;
        uint16_t *tcp;
        uint16_t *udp;
        uint16_t *sctp;
        struct icmp6_hdr *icmp6;
        struct nd_neighbor_solicit *nd_ns;
        struct gre_hdr *gre;
      };
      union {
        uint8_t *l4_payload;
        uint16_t *l4_payload_w;
        uint32_t *l4_payload_l;
        struct vxlanhdr *vxlan;
        struct gtpu_hdr *gtpu;
      };
      struct oob2_data *oob2;
      struct in6_addr *v6src;
      struct in6_addr *v6dst;
      uint8_t *nd_sll;
      uint8_t *nd_tll;
    };
  };
  struct action_list actions[LAGOPUS_ACTION_SET_ORDER_MAX];

  uint32_t queue_id;
  uint32_t flags;

#ifdef HYBRID
  uint32_t output_port;
  bool send_kernel;
  struct interface *ifp;
#ifdef PIPELINER
  struct pipeline_context pipeline_context;
#endif /* PIPELINER */
#endif /* HYBRID */
};

#define ETHER_HDR     struct ether_header
#define ETHER_TYPE(h) ((h)->ether_type)
#define ETHER_SRC(h)  ((h)->ether_shost)
#define ETHER_DST(h)  ((h)->ether_dhost)
#define VLAN_HDR      struct vlanhdr
#define VLAN_ETH(h)   ((h)->vlan_eth)
#define VLAN_TCI(h)   ((h)->vlan_tci)
#define VLAN_VID(h)   (OS_NTOHS(VLAN_TCI(h)) & 0x0fff)
#define VLAN_PCP(h)   ((OS_NTOHS(VLAN_TCI(h)) & 0xe000) >> 13)
#define MPLS_HDR      struct mpls_hdr
#define IPV4_HDR      struct ip
#define IPV4_VER(h)   ((h)->ip_v)
#define IPV4_HLEN(h)  ((h)->ip_hl)
#define IPV4_SRC(h)   ((h)->ip_src.s_addr)
#define IPV4_DST(h)   ((h)->ip_dst.s_addr)
#define IPV4_PROTO(h) ((h)->ip_p)
#define IPV4_TOS(h)   ((h)->ip_tos)
#define IPV4_TTL(h)   ((h)->ip_ttl)
#define IPV4_CSUM(h)  ((h)->ip_sum)
#define IPV4_TLEN(h)  OS_NTOHS((h)->ip_len)
#define IPV6_HDR      struct ip6_hdr
#define IPV6_SRC(h)   (&(h)->ip6_src)
#define IPV6_DST(h)   (&(h)->ip6_dst)
#define IPV6_VTCF(h)  ((h)->ip6_flow)
#define IPV6_VER(h)   ((OS_NTOHL(IPV6_VTCF(h))&0xf0000000)>>28)
#define IPV6_FLOW(h)  (OS_NTOHL(IPV6_VTCF(h))&0x000fffff)
#define IPV6_TC(h)    ((OS_NTOHL(IPV6_VTCF(h))&0x0ff00000)>>20)
#define IPV6_PLEN(h)  OS_NTOHS((h)->ip6_plen)
#define IP46_DSCP(n)  (((n)&0xfc)>>2)
#define IPV6_PROTO(h) ((h)->ip6_nxt)
#define IPV6_HLIM(h)  ((h)->ip6_hlim)
#define TCP_HDR       uint16_t
#define TCP_SPORT(h)  ((h)[0])
#define TCP_DPORT(h)  ((h)[1])
#define TCP_CKSUM(h)  ((h)[8])
#define UDP_HDR       uint16_t
#define UDP_SPORT(h)  ((h)[0])
#define UDP_DPORT(h)  ((h)[1])
#define UDP_LEN(h)    ((h)[2])
#define UDP_CKSUM(h)  ((h)[3])
#define SCTP_HDR      uint16_t
#define SCTP_SPORT(h) ((h)[0])
#define SCTP_DPORT(h) ((h)[1])
#define SCTP_CKSUM(h) (*(uint32_t *)&((h)[4]))
#define ICMP_CKSUM(h) ((h)->icmp_cksum)
#define GRE_HDR       struct gre_hdr
#define VXLAN_HDR       struct vxlanhdr
#define MTOD_OFS(m, offset, type)                       \
  (type)(OS_MTOD((m), unsigned char *) + (offset))

/**
 * find flow entry matching packet from the table.
 *
 * @param[in]   pkt     packet.
 * @param[in]   table   flow table related with the bridge.
 *
 * @retval      NULL    flow is not found.
 * @retval      !=NULL  matched flow.
 */
static inline struct flow *
lagopus_find_flow(struct lagopus_packet *pkt, struct table *table) {
#ifdef USE_MBTREE
  return find_mbtree(pkt, table->flow_list);
#else
  struct flowinfo *flowinfo;
  struct flow *flow;
  int32_t prio;

  prio = -1;

  flowinfo = table->userdata;
  if (flowinfo != NULL) {
    flow = flowinfo->match_func(flowinfo, pkt, &prio);
  } else {
    flow = NULL;
  }
  return flow;
#endif /* USE_MBTREE */
}

#endif /* SRC_DATAPLANE_OFPROTO_PACKET_H_ */
