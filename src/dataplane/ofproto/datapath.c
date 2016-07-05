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
 *      @file   datapath.c
 *      @brief  Dataplane APIs except for match
 */

#include "lagopus_config.h"

#include <inttypes.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

#include <openflow.h>

#include "lagopus/ofp_handler.h"
#include "lagopus/ethertype.h"
#include "lagopus/flowdb.h"
#include "lagopus/bridge.h"
#include "lagopus/port.h"
#include "lagopus/interface.h"
#include "lagopus/group.h"
#include "lagopus/meter.h"
#include "lagopus/dataplane.h"
#include "lagopus/flowinfo.h"
#include "lagopus/ofcache.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/dp_apis.h"
#include "../agent/ofp_match.h"
#include "callback.h"
#include "pktbuf.h"
#include "packet.h"
#include "csum.h"
#include "pcap.h"
#include "City.h"
#include "murmurhash3.h"
#include "mbtree.h"

#ifdef HAVE_DPDK
#ifdef __SSE4_2__
#include "rte_hash_crc.h"
#include "dpdk/rte_hash_crc64.h"
#endif /* __SSE4_2__ */
#include "dpdk/dpdk.h"
#endif /* HAVE_DPDK */

#undef DEBUG
#ifdef DEBUG
#include <stdio.h>

#define DPRINT(...) printf(__VA_ARGS__)
#define DPRINT_FLOW(flow) flow_dump(flow, stdout)
#else
#define DPRINT(...)
#define DPRINT_FLOW(flow)
#endif

#if 1
#define STATIC
#else
#define STATIC static
#endif

#define OFPIEH_AFTER_AH (OFPIEH_ESP|OFPIEH_DEST|OFPIEH_UNREP)
#define OFPIEH_AFTER_ESP (OFPIEH_DEST||OFPIEH_UNREP)

#define GET_OXM_FIELD(ofpat) \
  ((((const struct ofp_action_set_field *)(ofpat))->field[2] >> 1) & 0x7f)

#define NEED_COPY_ETH_ADDR(flags)                     \
  ((flags & (SET_FIELD_ETH_DST|SET_FIELD_ETH_SRC)) != \
   (SET_FIELD_ETH_DST|SET_FIELD_ETH_SRC))

#define PUT_TIMEOUT 1LL * 1000LL
#define FIELD(n) ((n) << 1)

/**
 * action property for each type.  index is OFPAT_*.
 */
static struct action_prop {
  /* see 5.10 Action Set */
  int priority;
} action_prop[] = {
  { 11 },       /* 0: OUTPUT */
  { 0 },        /* 1: reserved */
  { 0 },        /* 2: reserved */
  { 0 },        /* 3: reserved */
  { 0 },        /* 4: reserved */
  { 0 },        /* 5: reserved */
  { 0 },        /* 6: reserved */
  { 0 },        /* 7: reserved */
  { 0 },        /* 8: reserved */
  { 0 },        /* 9: reserved */
  { 0 },        /* 10: reserved */
  { 6 },        /* 11: COPY_TTL_OUT */
  { 1 },        /* 12: COOY_TTL_IN */
  { 0 },        /* 13: reserved */
  { 0 },        /* 14: reserved */
  { 8 },        /* 15: SET_MPLS_TTL */
  { 7 },        /* 16: DEC_MPLS_TTL */
  { 5 },        /* 17: PUSH_VLAN */
  { 2 },        /* 18: POP_VLAN */
  { 3 },        /* 19: PUSH_MPLS */
  { 2 },        /* 20: POP_MPLS */
  { 9 },        /* 21: SET_QUEUE */
  { 10 },       /* 22: GROUP */
  { 1 },        /* 23: SET_NW_TTL */
  { 1 },        /* 24: DEC_NW_TTL */
  { 8 },        /* 25: SET_FIELD */
  { 4 },        /* 26: PUSH_PBB */
  { 2 },        /* 27: POP_PBB */
};

/* Prototypes. */
STATIC void classify_ether_packet(struct lagopus_packet *);

STATIC int write_metadata(struct lagopus_packet *, uint64_t, uint64_t);

STATIC void clear_action_set(struct lagopus_packet *);

lagopus_result_t execute_group_action(struct lagopus_packet *, uint32_t);

STATIC int
apply_meter(struct lagopus_packet *, struct meter_table *, uint32_t);


#ifdef HAVE_DPDK
#ifdef __SSE4_2__
static uint64_t
calc_murmur_hash(const char *buf, size_t len, uint64_t seed) {
  union {
    uint64_t hash64;
    struct {
      uint32_t hash32_h;
      uint32_t hash32_l;
    };
  } val;
  MurmurHash3_x86_32(buf, (int)len, (uint32_t)seed, &val.hash32_h);
  val.hash32_l = rte_hash_crc(buf, (uint32_t)len, (uint32_t)seed);
  return val.hash64;
}
#endif /* __SSE4_2__ */

static inline uint64_t
calc_hash(const uint8_t *buf, size_t len, uint64_t seed) {
  static uint64_t (*hash_func)(const char *buf, size_t len, uint64_t seed)
    = NULL;

  if (hash_func == NULL) {
    switch (app.hashtype) {
#ifndef __SSE4_2__
      default:
#endif /* !__SSE4_2__ */
      case HASH_TYPE_CITY64:
        hash_func = CityHash64WithSeed;
        break;
#ifdef __SSE4_2__
      case HASH_TYPE_MURMUR3:
        hash_func = calc_murmur_hash;
        break;

      case HASH_TYPE_INTEL64:
      default:
        hash_func = lagopus_hash_crc64;
        break;
#endif /* __SSE4_2__ */
    }
  }
  return hash_func((const char *)buf, len, seed);
}
#else
#define calc_hash CityHash64WithSeed
#endif /* HAVE_DPDK */

static inline uint64_t
calc_l4_hash(const struct lagopus_packet *pkt, uint64_t hash64) {
  switch (*pkt->proto) {
    case IPPROTO_ICMP:
      hash64 = calc_hash(pkt->l4_hdr,
                         sizeof(uint8_t) * 2,
                         hash64);
      break;
    case IPPROTO_TCP:
    case IPPROTO_UDP:
    case IPPROTO_SCTP:
      hash64 = calc_hash(pkt->l4_hdr,
                         sizeof(uint16_t) * 2,
                         hash64);
      break;
    case IPPROTO_ICMPV6:
      hash64 = calc_hash(pkt->l4_hdr,
                         sizeof(uint8_t) * 2,
                         hash64);
      if (pkt->nd_sll != NULL) {
        hash64 = calc_hash(&pkt->nd_sll[2], ETHER_ADDR_LEN, hash64);
      } else if (pkt->nd_tll != NULL) {
        hash64 = calc_hash(&pkt->nd_tll[2], ETHER_ADDR_LEN, hash64);
      }
      break;
    default:
      break;
  }
  return hash64;
}

static inline uint64_t
calc_ipv4_hash(const struct lagopus_packet *pkt, uint64_t hash64) {
  struct ip *ipv4_hdr = pkt->ipv4;

  hash64 = calc_hash(&ipv4_hdr->ip_tos,
                     sizeof(ipv4_hdr->ip_tos),
                     hash64);
  hash64 = calc_hash((const uint8_t *)&ipv4_hdr->ip_src,
                     sizeof(ipv4_hdr->ip_src) << 1,
                     hash64);
  hash64 = calc_hash(&ipv4_hdr->ip_p,
                     sizeof(ipv4_hdr->ip_p),
                     hash64);
  return hash64;
}

static inline uint64_t
calc_ipv6_hash(const struct lagopus_packet *pkt, uint64_t hash64) {
  hash64 = calc_hash((const uint8_t *)pkt->ipv6,
                     4,
                     hash64);
  hash64 = calc_hash(pkt->proto,
                     1,
                     hash64);
  hash64 = calc_hash((const uint8_t *)&pkt->ipv6->ip6_src,
                     sizeof(struct in6_addr) << 1,
                     hash64);
  return hash64;
}

static inline uint64_t
calc_arp_hash(const struct lagopus_packet *pkt, uint64_t hash64) {
  return calc_hash(pkt->arp->arp_sha,
                   ETHER_ADDR_LEN * 2 + 4 + 4,
                   hash64);
}

static inline uint64_t
calc_l2_hash(const struct lagopus_packet *pkt, uint64_t seed) {
  return calc_hash(pkt->l2_hdr, (size_t)(pkt->l3_hdr - pkt->l2_hdr), seed);
}

static inline void
classify_packet_l4(struct lagopus_packet *pkt) {
  switch (*pkt->proto) {
    case IPPROTO_ICMPV6:
      /* pointer is shared with nd_ns */
      pkt->nd_sll = NULL;
      pkt->nd_tll = NULL;

      if (pkt->icmp6->icmp6_type == 135) {          /* NDP NS */
        uint8_t *nd_ptr, *end_ptr;

        end_ptr = ((uint8_t *)pkt->nd_ns) + IPV6_PLEN(pkt->ipv6);
        nd_ptr = (uint8_t *)&pkt->nd_ns[1];
        while (nd_ptr < end_ptr && nd_ptr[1] != 0) {
          if (nd_ptr[0] == 1) {
            pkt->nd_sll = nd_ptr;
            break;
          }
          nd_ptr += nd_ptr[1] << 3;
        }
      } else if (pkt->icmp6->icmp6_type == 136) {   /* NDP NA */
        uint8_t *nd_ptr, *end_ptr;

        end_ptr = ((uint8_t *)pkt->nd_ns) + IPV6_PLEN(pkt->ipv6);
        nd_ptr = (uint8_t *)&pkt->nd_ns[1];
        while (nd_ptr < end_ptr && nd_ptr[1] != 0) {
          if (nd_ptr[0] == 2) {
            pkt->nd_tll = nd_ptr;
            break;
          }
          nd_ptr += nd_ptr[1] << 3;
        }
      }
      break;

    case IPPROTO_UDP:
      pkt->l4_payload = pkt->l4_hdr + sizeof(UDP_HDR);
      break;

    case IPPROTO_TCP:
      pkt->l4_payload = pkt->l4_hdr + sizeof(TCP_HDR);
      break;

    case IPPROTO_SCTP:
      pkt->l4_payload = pkt->l4_hdr + sizeof(SCTP_HDR);
      break;

    default:
      break;
  }
}

static inline void
classify_packet_ipv4(struct lagopus_packet *pkt) {
  IPV4_HDR *ipv4_hdr;

  /* scan packet */
  DP_PRINT("IPv4 packet\n");
  ipv4_hdr = pkt->ipv4;
  pkt->proto = &IPV4_PROTO(ipv4_hdr);
  pkt->l4_hdr_l = pkt->l3_hdr_l + IPV4_HLEN(ipv4_hdr);
}

static void
classify_packet_ipv6(struct lagopus_packet *pkt) {
  IPV6_HDR *ipv6_hdr;
  ssize_t pktlen;
  uint8_t *proto;
  uint8_t *next_hdr;

  /* scan packet */
  DP_PRINT("IPv6 packet\n");
  ipv6_hdr = pkt->ipv6;
  pkt->oob2_data.ipv6_exthdr = 0;
  /*
   * finding protocol, parse IPv6 extension header.
   * 0: Hop-by-Hop Options Header
   * 43: Routing Header
   * 44: Fragmentation Header
   * 50: Encapsulating Security Payload Header (RFC2406)
   * 51: Authentication Header (RFC2402)
   * 59: No Next Header
   * 60: Destination Options Header
   * and prepare for ipv6 extention header pseudo match.
   */
  proto = &IPV6_PROTO(ipv6_hdr);
  pkt->v6src = IPV6_SRC(ipv6_hdr);
  pkt->v6dst = IPV6_DST(ipv6_hdr);
  next_hdr = pkt->l3_hdr + sizeof(IPV6_HDR);
  pktlen = OS_M_PKTLEN(PKT2MBUF(pkt)) - (uint32_t)(pkt->l3_hdr - pkt->l2_hdr);
  for (;;) {
    if (pktlen <= 2) {
      /* valid protocol is not found */
      break;
    }
    switch (*proto) {
      case IPPROTO_HOPOPTS:
        if (pkt->oob2_data.ipv6_exthdr != 0) {
          pkt->oob2_data.ipv6_exthdr |= OFPIEH_UNSEQ;
        }
        if ((pkt->oob2_data.ipv6_exthdr & OFPIEH_HOP) != 0) {
          pkt->oob2_data.ipv6_exthdr |= OFPIEH_UNREP;
        }
        pkt->oob2_data.ipv6_exthdr |= OFPIEH_HOP;
        proto = next_hdr;
        pktlen -= (1 + next_hdr[1]) << 3;
        next_hdr += (1 + next_hdr[1]) << 3;
        break;

      case IPPROTO_ROUTING:
        if ((pkt->oob2_data.ipv6_exthdr &
             (OFPIEH_HOP|OFPIEH_DEST|OFPIEH_UNREP)) != 0) {
          pkt->oob2_data.ipv6_exthdr |= OFPIEH_UNSEQ;
        }
        if ((pkt->oob2_data.ipv6_exthdr & OFPIEH_ROUTER) != 0) {
          pkt->oob2_data.ipv6_exthdr |= OFPIEH_UNREP;
        }
        pkt->oob2_data.ipv6_exthdr |= OFPIEH_ROUTER;
        proto = next_hdr;
        pktlen -= (1 + next_hdr[1]) << 3;
        next_hdr += (1 + next_hdr[1]) << 3;
        break;

      case IPPROTO_FRAGMENT:
        if ((pkt->oob2_data.ipv6_exthdr &
             (OFPIEH_HOP|OFPIEH_DEST|OFPIEH_ROUTER|OFPIEH_UNREP)) != 0) {
          pkt->oob2_data.ipv6_exthdr |= OFPIEH_UNSEQ;
        }
        if ((pkt->oob2_data.ipv6_exthdr & OFPIEH_FRAG) != 0) {
          pkt->oob2_data.ipv6_exthdr |= OFPIEH_UNREP;
        }
        pkt->oob2_data.ipv6_exthdr |= OFPIEH_FRAG;
        proto = next_hdr;
        pktlen -= 8;
        next_hdr += 8;
        break;

      case IPPROTO_AH:
        if ((pkt->oob2_data.ipv6_exthdr & OFPIEH_AFTER_AH) != 0) {
          pkt->oob2_data.ipv6_exthdr |= OFPIEH_UNSEQ;
        }
        if ((pkt->oob2_data.ipv6_exthdr & OFPIEH_AUTH) != 0) {
          pkt->oob2_data.ipv6_exthdr |= OFPIEH_UNREP;
        }
        pkt->oob2_data.ipv6_exthdr |= OFPIEH_AUTH;
        proto = next_hdr;
        pktlen -= (2 + next_hdr[1]) << 2;
        next_hdr += (2 + next_hdr[1]) << 2;
        break;

      case IPPROTO_ESP:
        if ((pkt->oob2_data.ipv6_exthdr & OFPIEH_AFTER_ESP) != 0) {
          pkt->oob2_data.ipv6_exthdr |= OFPIEH_UNSEQ;
        }
        if ((pkt->oob2_data.ipv6_exthdr & OFPIEH_ESP) != 0) {
          pkt->oob2_data.ipv6_exthdr |= OFPIEH_UNREP;
        }
        pkt->oob2_data.ipv6_exthdr |= OFPIEH_ESP;
        proto = next_hdr;
        pktlen -= (1 + next_hdr[1]) << 3;
        next_hdr += (1 + next_hdr[1]) << 3;
        break;

      case IPPROTO_DSTOPTS:
        /* first destination and final desitination */
        if ((pkt->oob2_data.ipv6_exthdr & OFPIEH_DEST) != 0) {
          pkt->oob2_data.ipv6_exthdr |= OFPIEH_UNREP;
        }
        pkt->oob2_data.ipv6_exthdr |= OFPIEH_DEST;
        proto = next_hdr;
        pktlen -= (1 + next_hdr[1]) << 3;
        next_hdr += (1 + next_hdr[1]) << 3;
        break;

      case IPPROTO_NONE:
        /* exit loop */
        if ((pkt->oob2_data.ipv6_exthdr & OFPIEH_NONEXT) != 0) {
          pkt->oob2_data.ipv6_exthdr |= OFPIEH_UNREP;
        }
        pkt->oob2_data.ipv6_exthdr |= OFPIEH_NONEXT;
        proto = next_hdr;
        next_hdr += 2;
        goto next;

      default:
        /* exit loop */
        goto next;
    }
  }
next:
  pkt->proto = proto;
  pkt->l4_hdr = next_hdr;
  pkt->oob2_data.ipv6_exthdr = htons(pkt->oob2_data.ipv6_exthdr);
}

/**
 * packet classification for L2.
 *
 * @param[in]   pkt     packet structure with data.
 *
 * analyze packet data, and set up internal metadata of the packet.
 */
static inline void
classify_packet_l2(struct lagopus_packet *pkt) {
  uint16_t ether_type;
  ssize_t pktlen;
  OS_MBUF *m;

  /* reset */
  pkt->oob_data.vlan_tci = 0;
  pkt->vlan = NULL;
  pkt->pbb = NULL;
  pkt->mpls = NULL;
  pkt->proto = NULL;
  pkt->base[OOB_BASE] = (void *)&pkt->oob_data;
  pkt->base[OOB2_BASE] = (void *)&pkt->oob2_data;

  m = PKT2MBUF(pkt);
  pktlen = (ssize_t)OS_M_PKTLEN(m);

  pkt->eth = OS_MTOD(m, ETHER_HDR *);
  ether_type = OS_NTOHS(ETHER_TYPE(pkt->eth));

  pkt->l3_hdr = pkt->base[ETH_BASE] + sizeof(ETHER_HDR);
  switch (ether_type) {
    case ETHERTYPE_PBB:
      DP_PRINT("PBB packet\n");
      pkt->pbb = (struct pbb_hdr *)pkt->l3_hdr_l;
      ether_type = OS_NTOHS(pkt->pbb->c_ethtype);
      pkt->l3_hdr += sizeof(struct pbb_hdr *);
      break;

    case ETHERTYPE_VLAN:
    case 0x88a8: /* double tagging */
      pkt->vlan = (VLAN_HDR *)pkt->l3_hdr_w;
      pkt->oob_data.vlan_tci = VLAN_TCI(pkt->vlan) | htons(OFPVID_PRESENT);
      ether_type = OS_NTOHS(VLAN_ETH(pkt->vlan));
      pkt->l3_hdr += sizeof(VLAN_HDR);
      break;

    default:
      break;
  }
  /* strip innner vlan/pbb header */
  while (ether_type == ETHERTYPE_VLAN ||
         ether_type == ETHERTYPE_PBB ||
         ether_type == 0x88a8) {
    if (ether_type == ETHERTYPE_PBB) {
      struct pbb_hdr *pbb;

      pbb = (struct pbb_hdr *)pkt->l3_hdr_l;
      ether_type = OS_NTOHS(pbb->c_ethtype);
      if (pkt->pbb == NULL) {
        pkt->pbb = pbb;
      }
      pkt->l3_hdr += sizeof(struct pbb_hdr);
    } else {
      if ((pktlen -= (ssize_t)sizeof(VLAN_HDR)) < 0) {
        return;
      }
      if (pkt->vlan == NULL) {
        pkt->vlan = (VLAN_HDR *)pkt->l3_hdr_w;
        pkt->oob_data.vlan_tci = VLAN_TCI(pkt->vlan) | htons(OFPVID_PRESENT);
      }
      ether_type =  OS_NTOHS(VLAN_ETH((VLAN_HDR *)pkt->l3_hdr_w));
      pkt->l3_hdr += sizeof(VLAN_HDR);
    }
  }

  pkt->ether_type = ether_type;
  pkt->oob_data.ether_type = htons(ether_type);

  switch (ether_type) {
    case ETHERTYPE_MPLS:
    case ETHERTYPE_MPLS_MCAST:
      DP_PRINT("MPLS packet\n");
      if ((pktlen -= (ssize_t)sizeof(struct mpls_hdr)) < 0) {
        return;
      }
      pkt->mpls = (struct mpls_hdr *)pkt->l3_hdr_l;
      pkt->l3_hdr +=  sizeof(struct mpls_hdr);
      if (pkt->l3_hdr[0] >= 0x45 && pkt->l3_hdr[0] <= 0x4f) {
        pkt->ether_type = ETHERTYPE_IP;
      } else if ((pkt->l3_hdr[0] & 0xf0) == 0x60) {
        pkt->ether_type = ETHERTYPE_IPV6;
      }
      break;

    case ETHERTYPE_PBB:
      DP_PRINT("PBB packet\n");
      if ((pktlen -= (ssize_t)sizeof(struct pbb_hdr)) < 0) {
        return;
      }
      pkt->pbb = (struct pbb_hdr *)pkt->l3_hdr_l;
      pkt->l3_hdr += sizeof(struct pbb_hdr);
      pkt->ether_type = OS_NTOHS(pkt->pbb->c_ethtype);
      break;

    default:
      break;
  }
}

/**
 * Like classify_ether_packet, but don't re-calculation pkt->hash64.
 *
 * @param[in]   pkt     Packet.
 */
static inline void
re_classify_packet(struct lagopus_packet *pkt) {
  classify_packet_l2(pkt);
  switch (pkt->ether_type) {
    case ETHERTYPE_IP:
      classify_packet_ipv4(pkt);
      classify_packet_l4(pkt);
      break;
    case ETHERTYPE_IPV6:
      classify_packet_ipv6(pkt);
      classify_packet_l4(pkt);
      break;
    default:
      break;
  }
}

/* Setup packet information. */
void
lagopus_packet_init(struct lagopus_packet *pkt, void *m, struct port *port) {

  /* initialize OpenFlow related members */
  pkt->table_id = 0;
  pkt->oob_data.metadata = 0;
  pkt->oob2_data.tunnel_id = 0;

  pkt->flags = 0;
  pkt->nmatched = 0;
  /* set raw packet data and port */
  pkt->in_port = port;
  pkt->bridge = port->bridge;
  pkt->oob_data.in_port = htonl(port->ofp_port.port_no);
  pkt->oob_data.in_phy_port= htonl(port->ofp_port.port_no);
  /* pre match */
  classify_ether_packet(pkt);
}

/**
 * Classify ethernet packet.
 */
STATIC void
classify_ether_packet(struct lagopus_packet *pkt) {
  classify_packet_l2(pkt);
  pkt->oob_data.packet_type = ((OFPHTN_ONF << 16) | OFPHTO_ETHERNET);
  switch (pkt->ether_type) {
    case ETHERTYPE_IP:
      classify_packet_ipv4(pkt);
      classify_packet_l4(pkt);
      break;

    case ETHERTYPE_IPV6:
      classify_packet_ipv6(pkt);
      classify_packet_l4(pkt);
      break;

    default:
      break;
  }
}

static inline void
calc_packet_hash(struct lagopus_packet *pkt) {
  uint64_t hash64;

  hash64 = calc_l2_hash(pkt, pkt->in_port->ifindex);
  switch (pkt->ether_type) {
    case ETHERTYPE_IP:
      hash64 = calc_ipv4_hash(pkt, hash64);
      hash64 = calc_l4_hash(pkt, hash64);
      break;

    case ETHERTYPE_IPV6:
      hash64 = calc_ipv6_hash(pkt, hash64);
      hash64 = calc_l4_hash(pkt, hash64);
      break;

    case ETHERTYPE_ARP:
      hash64 = calc_arp_hash(pkt, hash64);
      break;

    default:
      break;
  }
  pkt->hash64 = hash64;
}

/**
 * Copy packet for output.
 */
struct lagopus_packet *
copy_packet(struct lagopus_packet *src_pkt) {
  OS_MBUF *mbuf;
  struct lagopus_packet *pkt;
  uint8_t *srcm, *dstm;
  size_t pktlen;

  pkt = alloc_lagopus_packet();
  if (pkt == NULL) {
    lagopus_msg_error("alloc_lagopus_packet failed\n");
    return NULL;
  }
  mbuf = PKT2MBUF(pkt);
  pktlen = OS_M_PKTLEN(PKT2MBUF(src_pkt));
  OS_M_APPEND(mbuf, pktlen);
  srcm = OS_MTOD(PKT2MBUF(src_pkt), uint8_t *);
  dstm = OS_MTOD(PKT2MBUF(pkt), uint8_t *);
  memcpy(dstm, srcm, pktlen);
  pkt->in_port = src_pkt->in_port;
  pkt->bridge = src_pkt->bridge;
  pkt->ether_type = src_pkt->ether_type;
  pkt->l3_hdr = src_pkt->l3_hdr + (dstm - srcm);
  pkt->l4_hdr = src_pkt->l4_hdr + (dstm - srcm);
  pkt->flags = src_pkt->flags | PKT_FLAG_CACHED_FLOW;
  /* other pkt members are not used in physical output. */
  return pkt;
}

/*
 * instructions.
 */

/**
 * Do set-field instruction; modify specified field value in the packet.
 *
 * @param[in]   pkt     target packet.
 * @param[in]   oxm     pointer to OXM (OpenFlow Extensible Match) TLV.
 *
 * @retval      LAGOPUS_RESULT_OK       success.
 *
 * So far, re-calculate ip/tc/udp checksum at set-field.
 * It should be executed when output packet.
 * XXX: output action may not send packet immediately,
 * queued instead.  In apply set-field after output case,
 * the packet should be duplicated instead of referenced.
 *
 * NOTE: all OXM values are in network byte order.
 */
static lagopus_result_t
execute_action_set_field(struct lagopus_packet *pkt,
                         struct action *action) {
  uint8_t *oxm;
  uint32_t oxm_header;
  bool oxm_hasmask;
#ifdef DIAGNOSTIC
  uint16_t oxm_class;
#endif
  uint8_t oxm_field;
  uint8_t *oxm_value;
  uint16_t val16;
  uint32_t val32;

  DP_PRINT("action set_field\n");
  /* extract variables */
  oxm = ((struct ofp_action_set_field *)&action->ofpat)->field;
  OS_MEMCPY(&val32, oxm, sizeof(uint32_t));
  oxm_header = OS_NTOHL(val32);
  oxm_hasmask = ((oxm_header & 0x00000100) != 0);
#ifdef DIAGNOSTIC
  oxm_class = (uint16_t)((oxm_header >> 16) & 0xffff);
  if (oxm_class != OFPXMC_OPENFLOW_BASIC) {
    /* XXX experimenter class support */
    DP_PRINT("not supported yet\n");
  }
#endif
  oxm_field = oxm[2] >> 1;
  oxm_value = &oxm[4];

  switch (oxm_field) {
    case OFPXMT_OFB_IN_PORT:
      OS_MEMCPY(&val32, oxm_value, sizeof(uint32_t));
      DP_PRINT("set_field in_port: %d\n", OS_NTOHL(val32));
      if (pkt->bridge != NULL) {

        pkt->in_port = port_lookup(&pkt->bridge->ports,
                                   OS_NTOHL(val32));
        pkt->oob_data.in_port = val32;
      }
      break;

#if 0 /* cannot modify, the data is not in packet */
    case OFPXMT_OFB_IN_PHY_PORT:
      break;
#endif

    case OFPXMT_OFB_METADATA:
      DP_PRINT("set_field metadata: 0x%016" PRIx64 "\n",
               OS_NTOHLL(*((uint64_t *)oxm_value)));
      OS_MEMCPY(&pkt->oob_data.metadata, oxm_value, sizeof(uint64_t));
      break;

    case OFPXMT_OFB_ETH_DST:
      DP_PRINT("set_field eth_dst: %02x:%02x:%02x:%02x:%02x:%02x\n",
               oxm_value[0], oxm_value[1], oxm_value[2],
               oxm_value[3], oxm_value[4], oxm_value[5]);
      OS_MEMCPY(ETHER_DST(pkt->eth), oxm_value, ETHER_ADDR_LEN);
      break;

    case OFPXMT_OFB_ETH_SRC:
      DP_PRINT("set_field eth_src: %02x:%02x:%02x:%02x:%02x:%02x\n",
               oxm_value[0], oxm_value[1], oxm_value[2],
               oxm_value[3], oxm_value[4], oxm_value[5]);
      OS_MEMCPY(ETHER_SRC(pkt->eth), oxm_value, ETHER_ADDR_LEN);
      break;

    case OFPXMT_OFB_ETH_TYPE:
      OS_MEMCPY(&val16, oxm_value, sizeof(uint16_t));
      DP_PRINT("set_field eth_type: 0x%04x\n", OS_NTOHS(val16));
      if (pkt->vlan != NULL) {
        VLAN_ETH(pkt->vlan) = val16;
      } else {
        ETHER_TYPE(pkt->eth) = val16;
      }
      /* re-classify packet if needed. */
      re_classify_packet(pkt);
      break;

    case OFPXMT_OFB_VLAN_VID:
      /* 16bits:VLAN TCI --> |3bits:PCP|1bit:CFI|12bits:VID| */
      DP_PRINT("set_field vlan_vid: %d\n", oxm_value[0] << 8 | oxm_value[1]);
      if (pkt->vlan != NULL) {
        OS_MEMCPY(&val16, oxm_value, sizeof(uint16_t));
        VLAN_TCI(pkt->vlan) &= OS_HTONS(0xf000);
        VLAN_TCI(pkt->vlan) |= val16 & OS_HTONS(0x0fff);
        pkt->oob_data.vlan_tci = VLAN_TCI(pkt->vlan);
      }
      break;

    case OFPXMT_OFB_VLAN_PCP:
      /* 16bits:VLAN TCI --> |3bits:PCP|1bit:CFI|12bits:VID| */
      DP_PRINT("set_field vlan_pcp: %d\n", *oxm_value);
      if (pkt->vlan != NULL) {
        VLAN_TCI(pkt->vlan) &= OS_HTONS(0x01fff);
        VLAN_TCI(pkt->vlan) |= OS_HTONS((uint16_t)(*oxm_value << 13));
        pkt->oob_data.vlan_tci = VLAN_TCI(pkt->vlan);
      }
      break;

    case OFPXMT_OFB_IP_DSCP:
      DP_PRINT("set_field ip_dscp: 0x%x\n", *oxm_value);
      if (pkt->ether_type == ETHERTYPE_IP) {
        /* IPv4, 6bit of ToS field */
        IPV4_TOS(pkt->ipv4) &= 0x03;
        IPV4_TOS(pkt->ipv4) |= (uint8_t)((*oxm_value) << 2);
        pkt->flags |= PKT_FLAG_RECALC_IPV4_CKSUM;
      } else if (pkt->ether_type == ETHERTYPE_IPV6) {
        /* IPv6, 6bit of Traffic Class field */
        IPV6_VTCF(pkt->ipv6) &= OS_HTONL(0xf03fffffU);
        IPV6_VTCF(pkt->ipv6) |= OS_HTONL((uint32_t)(*oxm_value << 22));
      }
      break;

    case OFPXMT_OFB_IP_ECN:
      DP_PRINT("set_field ip_ecn: %d\n", *oxm_value);
      if (pkt->ether_type == ETHERTYPE_IP) {
        /* IPv4, lower 2bit2 of ToS field */
        IPV4_TOS(pkt->ipv4) &= 0xfc;
        IPV4_TOS(pkt->ipv4) |= *oxm_value;
        pkt->flags |= PKT_FLAG_RECALC_IPV4_CKSUM;
      } else if (pkt->ether_type == ETHERTYPE_IPV6) {
        /* IPv6, 2bit of Traffic Class field */
        IPV6_VTCF(pkt->ipv6) &= OS_HTONL(0xffcfffffU);
        IPV6_VTCF(pkt->ipv6) |= OS_HTONL((uint32_t)(*oxm_value << 20));
      }
      break;

    case OFPXMT_OFB_IP_PROTO:
      DP_PRINT("set_field ip_proto: %d\n", *oxm_value);
      *pkt->proto = *oxm_value;
      if (pkt->ether_type == ETHERTYPE_IP) {
        pkt->flags |= PKT_FLAG_RECALC_IPV4_CKSUM;
        classify_packet_ipv4(pkt);
        classify_packet_l4(pkt);
      } else {
        classify_packet_ipv6(pkt);
        classify_packet_l4(pkt);
      }
      break;

    case OFPXMT_OFB_IPV4_SRC:
      DP_PRINT("set_field ipv4_src: %d.%d.%d.%d\n",
               oxm_value[0], oxm_value[1], oxm_value[2], oxm_value[3]);
      OS_MEMCPY(&val32, oxm_value, sizeof(uint32_t));
      IPV4_SRC(pkt->ipv4) = val32;
      pkt->flags |= PKT_FLAG_RECALC_IPV4_CKSUM | PKT_FLAG_RECALC_L4_CKSUM;
      break;

    case OFPXMT_OFB_IPV4_DST:
      DP_PRINT("set_field ipv4_src: %d.%d.%d.%d\n",
               oxm_value[0], oxm_value[1], oxm_value[2], oxm_value[3]);
      OS_MEMCPY(&val32, oxm_value, sizeof(uint32_t));
      IPV4_DST(pkt->ipv4) = val32;
      pkt->flags |= PKT_FLAG_RECALC_IPV4_CKSUM | PKT_FLAG_RECALC_L4_CKSUM;
      break;

    case OFPXMT_OFB_TCP_SRC:
      OS_MEMCPY(&val16, oxm_value, sizeof(uint16_t));
      DP_PRINT("set_field tcp_src: %d\n", OS_HTONS(val16));
      TCP_SPORT(pkt->tcp) = val16;
      pkt->flags |= PKT_FLAG_RECALC_TCP_CKSUM;
      break;

    case OFPXMT_OFB_TCP_DST:
      OS_MEMCPY(&val16, oxm_value, sizeof(uint16_t));
      DP_PRINT("set_field tcp_dst: %d\n", OS_HTONS(val16));
      TCP_DPORT(pkt->tcp) = val16;
      pkt->flags |= PKT_FLAG_RECALC_TCP_CKSUM;
      break;

    case OFPXMT_OFB_UDP_SRC:
      OS_MEMCPY(&val16, oxm_value, sizeof(uint16_t));
      DP_PRINT("set_field udp_src: %d\n", OS_HTONS(val16));
      UDP_SPORT(pkt->udp) = val16;
      pkt->flags |= PKT_FLAG_RECALC_UDP_CKSUM;
      break;

    case OFPXMT_OFB_UDP_DST:
      OS_MEMCPY(&val16, oxm_value, sizeof(uint16_t));
      DP_PRINT("set_field udp_dst: %d\n", OS_HTONS(val16));
      UDP_DPORT(pkt->udp) = val16;
      pkt->flags |= PKT_FLAG_RECALC_UDP_CKSUM;
      break;

    case OFPXMT_OFB_SCTP_SRC:
      OS_MEMCPY(&val16, oxm_value, sizeof(uint16_t));
      DP_PRINT("set_field sctp_src: %d\n", OS_HTONS(val16));
      SCTP_SPORT(pkt->sctp) = val16;
      pkt->flags |= PKT_FLAG_RECALC_SCTP_CKSUM;
      break;

    case OFPXMT_OFB_SCTP_DST:
      OS_MEMCPY(&val16, oxm_value, sizeof(uint16_t));
      DP_PRINT("set_field sctp_dst: %d\n", OS_HTONS(val16));
      SCTP_DPORT(pkt->sctp) = val16;
      pkt->flags |= PKT_FLAG_RECALC_SCTP_CKSUM;
      break;

    case OFPXMT_OFB_ICMPV4_TYPE:
      DP_PRINT("set_field icmpv4_type: %d\n", *oxm_value);
      pkt->icmp->icmp_type = *oxm_value;
      pkt->flags |= PKT_FLAG_RECALC_ICMP_CKSUM;
      break;

    case OFPXMT_OFB_ICMPV4_CODE:
      DP_PRINT("set_field icmpv4_code: %d\n", *oxm_value);
      pkt->icmp->icmp_code = *oxm_value;
      pkt->flags |= PKT_FLAG_RECALC_ICMP_CKSUM;
      break;

    case OFPXMT_OFB_ARP_OP:
      OS_MEMCPY(&pkt->arp->arp_op, oxm_value, sizeof(pkt->arp->arp_op));
      DP_PRINT("set_field arp_op: %d\n", OS_NTOHS(pkt->arp->arp_op));
      break;

    case OFPXMT_OFB_ARP_SPA:
      DP_PRINT("set_field arp_spa: %d.%d.%d.%d\n",
               oxm_value[0], oxm_value[1], oxm_value[2], oxm_value[3]);
      OS_MEMCPY(pkt->arp->arp_spa, oxm_value, 4);
      break;

    case OFPXMT_OFB_ARP_TPA:
      DP_PRINT("set_field arp_tpa: %d.%d.%d.%d\n",
               oxm_value[0], oxm_value[1], oxm_value[2], oxm_value[3]);
      OS_MEMCPY(pkt->arp->arp_tpa, oxm_value, 4);
      break;

    case OFPXMT_OFB_ARP_SHA:
      DP_PRINT("set_field arp_sha: %02x:%02x:%02x:%02x:%02x:%02x\n",
               oxm_value[0], oxm_value[1], oxm_value[2],
               oxm_value[3], oxm_value[4], oxm_value[5]);
      OS_MEMCPY(pkt->arp->arp_sha, oxm_value, 6);
      break;

    case OFPXMT_OFB_ARP_THA:
      DP_PRINT("set_field arp_tha: %02x:%02x:%02x:%02x:%02x:%02x\n",
               oxm_value[0], oxm_value[1], oxm_value[2],
               oxm_value[3], oxm_value[4], oxm_value[5]);
      OS_MEMCPY(pkt->arp->arp_tha, oxm_value, 6);
      break;

    case OFPXMT_OFB_IPV6_SRC:
      DP_PRINT("set_field ipv6_src:");
      DP_PRINT_HEXDUMP(oxm_value, 16);
      DP_PRINT("\n");
      OS_MEMCPY(IPV6_SRC(pkt->ipv6), oxm_value, 16);
      pkt->flags |= PKT_FLAG_RECALC_IPV6_CKSUM | PKT_FLAG_RECALC_L4_CKSUM;
      break;

    case OFPXMT_OFB_IPV6_DST:
      DP_PRINT("set_field ipv6_dst:");
      DP_PRINT_HEXDUMP(oxm_value, 16);
      DP_PRINT("\n");
      OS_MEMCPY(IPV6_DST(pkt->ipv6), oxm_value, 16);
      pkt->flags |= PKT_FLAG_RECALC_IPV6_CKSUM | PKT_FLAG_RECALC_L4_CKSUM;
      break;

    case OFPXMT_OFB_IPV6_FLABEL:
      OS_MEMCPY(&val32, oxm_value, sizeof(uint32_t));
      DP_PRINT("set_field ipv6_flabel: 0x%08x\n", OS_NTOHL(val32));
      IPV6_VTCF(pkt->ipv6) &= OS_HTONL(~0x000fffffU);
      IPV6_VTCF(pkt->ipv6) |= val32;
      break;

    case OFPXMT_OFB_ICMPV6_TYPE:
      DP_PRINT("set_field icmpv6_type: %d\n", *oxm_value);
      pkt->icmp6->icmp6_type = *oxm_value;
      pkt->flags |= PKT_FLAG_RECALC_ICMPV6_CKSUM;
      break;

    case OFPXMT_OFB_ICMPV6_CODE:
      DP_PRINT("set_field icmpv6_code: %d\n", *oxm_value);
      pkt->icmp6->icmp6_code = *oxm_value;
      pkt->flags |= PKT_FLAG_RECALC_ICMPV6_CKSUM;
      break;

    case OFPXMT_OFB_IPV6_ND_TARGET:
      DP_PRINT("set_field ipv6_nd_target:");
      DP_PRINT_HEXDUMP(oxm_value, 16);
      DP_PRINT("\n");
      OS_MEMCPY(&pkt->nd_ns->nd_ns_target, oxm_value, 16);
      pkt->flags |= PKT_FLAG_RECALC_ICMPV6_CKSUM;
      break;

    case OFPXMT_OFB_IPV6_ND_SLL:
      DP_PRINT("set_field ipv6_nd_sll: %02x:%02x:%02x:%02x:%02x:%02x\n",
               oxm_value[0], oxm_value[1], oxm_value[2],
               oxm_value[3], oxm_value[4], oxm_value[5]);
      if (pkt->nd_sll != NULL) {
        OS_MEMCPY(&pkt->nd_sll[2], oxm_value, ETHER_ADDR_LEN);
      pkt->flags |= PKT_FLAG_RECALC_ICMPV6_CKSUM;
      }
      break;

    case OFPXMT_OFB_IPV6_ND_TLL:
      DP_PRINT("set_field ipv6_nd_dll: %02x:%02x:%02x:%02x:%02x:%02x\n",
               oxm_value[0], oxm_value[1], oxm_value[2],
               oxm_value[3], oxm_value[4], oxm_value[5]);
      if (pkt->nd_tll != NULL) {
        OS_MEMCPY(&pkt->nd_tll[2], oxm_value, ETHER_ADDR_LEN);
      pkt->flags |= PKT_FLAG_RECALC_ICMPV6_CKSUM;
      }
      break;

    case OFPXMT_OFB_MPLS_LABEL:
      /* 20bit */
      OS_MEMCPY(&val32, oxm_value, sizeof(uint32_t));
      DP_PRINT("set_field mpls_label: %d\n", OS_NTOHL(val32));
      SET_MPLS_LBL(pkt->mpls->mpls_lse, val32);
      break;

    case OFPXMT_OFB_MPLS_TC:
      /* 3bit */
      DP_PRINT("set_field mpls_tc: %d\n", *oxm_value);
      SET_MPLS_EXP(pkt->mpls->mpls_lse, *oxm_value);
      break;

    case OFPXMT_OFB_MPLS_BOS:
      DP_PRINT("set_field mpls_bos: %d\n", *oxm_value);
      SET_MPLS_BOS(pkt->mpls->mpls_lse, *oxm_value);
      break;

    case OFPXMT_OFB_PBB_ISID:
      DP_PRINT("set_field pbb_isid: %d\n",
               oxm_value[0] << 16 | oxm_value[1] << 8 | oxm_value[0]);
      OS_MEMCPY(pkt->pbb->i_sid, oxm_value, 3);
      break;

    case OFPXMT_OFB_TUNNEL_ID:
      /* network byte order */
      OS_MEMCPY(&pkt->oob2_data.tunnel_id, oxm_value, sizeof(uint64_t));
      break;

#if 0
    case OFPXMT_OFB_IPV6_EXTHDR:
      /* cannot modify pseudo header flags */
      break;
#endif

#ifdef PBB_UCA_SUPPORT
    case OFPXMT_OFB_PBB_UCA:
      break;
#endif /* PBB_UCA_SUPPORT */

#ifdef GENERAL_TUNNEL_SUPPORT
    case OFPXMT_OFB_PACKET_TYPE:
      OS_MEMCPY(&val32, oxm_value, sizeof(uint32_t));
      pkt->oob_data.packet_type = OS_NTOHL(val32);
      break;

    case OFPXMT_OFB_GRE_FLAGS:
      {
        uint16_t flags;

        pkt->gre->flags &= OS_HTONS(7);
        OS_MEMCPY(&flags, oxm_value, sizeof(uint16_t));
        pkt->gre->flags |= OS_HTONS(OS_NTOHS(flags) << 3);
      }
      break;

    case OFPXMT_OFB_GRE_VER:
        pkt->gre->flags &= OS_HTONS(0xfff8);
        pkt->gre->flags |= OS_HTONS(*oxm_value);
      break;

    case OFPXMT_OFB_GRE_PROTOCOL:
      OS_MEMCPY(&pkt->gre->ptype, oxm_value, sizeof(uint16_t));
      break;

    case OFPXMT_OFB_GRE_KEY:
      if (oxm_hasmask) {
        uint32_t key, new_key, mask;

        OS_MEMCPY(&key, &pkt->gre->key, sizeof(uint32_t));
        OS_MEMCPY(&new_key, oxm_value, sizeof(uint32_t));
        OS_MEMCPY(&mask, &oxm_value[4], sizeof(uint32_t));
        key &= ~mask;
        key |= new_key;
        OS_MEMCPY(&pkt->gre->key, &key, sizeof(uint32_t));
      } else {
        OS_MEMCPY(&pkt->gre->key, oxm_value, sizeof(uint32_t));
      }
      break;

#if 0
    case OFPXMT_OFB_GRE_SEQNUM:
      OS_MEMCPY(&pkt->gre->seq_num, oxm_value, sizeof(uint32_t));
      break;
#endif

    case OFPXMT_OFB_LISP_FLAGS:
      break;

    case OFPXMT_OFB_LISP_NONCE:
      break;

    case OFPXMT_OFB_LISP_ID:
      break;

    case OFPXMT_OFB_VXLAN_FLAGS:
      pkt->vxlan->flags = *oxm_value;
      break;

    case OFPXMT_OFB_VXLAN_VNI:
      OS_MEMCPY(&pkt->vxlan->vni, oxm_value, 3);
      break;

    case OFPXMT_OFB_MPLS_DATA_FIRST_NIBBLE:
      break;

    case OFPXMT_OFB_MPLS_ACH_VERSION:
      break;

    case OFPXMT_OFB_MPLS_ACH_CHANNEL:
      break;

    case OFPXMT_OFB_MPLS_PW_METADATA:
      break;

    case OFPXMT_OFB_MPLS_CW_FLAGS:
      break;

    case OFPXMT_OFB_MPLS_CW_FRAG:
      break;

    case OFPXMT_OFB_MPLS_CW_LEN:
      break;

    case OFPXMT_OFB_MPLS_CW_SEQ_NUM:
      break;

    case OFPXMT_OFB_GTPU_FLAGS:
      pkt->gtpu->verflags &= 0xe0;
      pkt->gtpu->verflags |= *oxm_value;
      break;

    case OFPXMT_OFB_GTPU_VER:
      pkt->gtpu->verflags &= 0x1f;
      pkt->gtpu->verflags |= (*oxm_value) << 5;
      break;

    case OFPXMT_OFB_GTPU_MSGTYPE:
      pkt->gtpu->msgtype = *oxm_value;
      break;

    case OFPXMT_OFB_GTPU_TEID:
      OS_MEMCPY(&pkt->gtpu->te_id, oxm_value, 4);
      break;

    case OFPXMT_OFB_GTPU_EXTN_HDR:
      break;

    case OFPXMT_OFB_GTPU_EXTN_UDP_PORT:
      break;

    case OFPXMT_OFB_GTPU_EXTN_SCI:
      break;
#endif /* GENERAL_TUNNEL_SUPPORT */

    default:
      break;
  }
  return 0;
}

/**
 * Do write metadata.
 *
 * @param[in]   pkt             target packet.
 * @param[in]   metadata        writing metadata.
 * @param[in]   metadata_mask   mask of the metadata.
 */
STATIC int
write_metadata(struct lagopus_packet *pkt,
               uint64_t metadata, uint64_t metadata_mask) {
  pkt->oob_data.metadata &= ~OS_HTONLL(metadata_mask);
  pkt->oob_data.metadata |= OS_HTONLL(metadata & metadata_mask);

  return 0;
}

lagopus_result_t
merge_action_set(struct action_list *actions,
                 const struct action_list *action_list) {
  const struct action *action;

  TAILQ_FOREACH(action, action_list, entry) {
    const struct ofp_action_header *ofp_action;
    struct action *new_action, *ac;
    size_t size;
    int priority;

    ofp_action = &action->ofpat;
    if (ofp_action->type > sizeof(action_prop) / sizeof(action_prop[0])) {
      /* reserved action type */
      continue;
    }
    priority = action_prop[ofp_action->type].priority;
    if (unlikely(priority == 0)) {
      /* reserved action type */
      continue;
    }
    size = sizeof(struct action) + ofp_action->len - sizeof(*ofp_action);
    new_action = (struct action *)calloc(1, size);
    OS_MEMCPY(new_action, action, size);
    /*
     * 5.10 says, an action set contains a maximum of one action of each type.
     * if same action type is exist in order list, overwrite it.
     * if action type is OFPAT_SET_FIELD, overwrite action has same field type.
     */
    TAILQ_FOREACH(ac, &actions[priority - 1], entry) {
      if (ac->ofpat.type == ofp_action->type) {
        if (ac->ofpat.type == OFPAT_SET_FIELD) {
          if (GET_OXM_FIELD(&ac->ofpat) == GET_OXM_FIELD(ofp_action)) {
            TAILQ_REMOVE(&actions[priority - 1], ac, entry);
            free(ac);
            break;
          }
        } else {
          TAILQ_REMOVE(&actions[priority - 1], ac, entry);
          free(ac);
          break;
        }
      }
    }
    TAILQ_INSERT_TAIL(&actions[priority - 1], new_action, entry);
  }
  return LAGOPUS_RESULT_OK;
}

/**
 * Remove all (wrote) action set.
 *
 * @param[in]   pkt     target packet.
 */
STATIC void
clear_action_set(struct lagopus_packet *pkt) {
  int i;

  if ((pkt->flags & PKT_FLAG_HAS_ACTION) != 0) {
    for (i = 0; i < LAGOPUS_ACTION_SET_ORDER_MAX; i++) {
      struct action_list *action_list;
      struct action *action;

      action_list = &pkt->actions[i];
      while ((action = TAILQ_FIRST(action_list)) != NULL) {
        TAILQ_REMOVE(action_list, action, entry);
        free(action);
      }
    }
  }
}

static inline lagopus_result_t
execute_action_set(struct lagopus_packet *pkt, struct action_list *actions) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  int i;

  for (i = 0; i < LAGOPUS_ACTION_SET_ORDER_MAX; i++) {
    rv = execute_action(pkt, &actions[i]);
    if (rv != LAGOPUS_RESULT_OK) {
      break;
    }
  }
  return rv;
}

struct bucket *
group_select_bucket(struct lagopus_packet *pkt, struct bucket_list *list) {
  struct bucket *bucket;
  uint64_t sel, weight, total_weight;

  total_weight = 0;
  TAILQ_FOREACH(bucket, list, entry) {
    total_weight += bucket->ofp.weight;
  }
  if (total_weight == 0) {
    TAILQ_FOREACH(bucket, list, entry) {
      total_weight++;
    }
    if (total_weight == 0) {
      return NULL;
    }
    if (pkt->hash64 == 0) {
      calc_packet_hash(pkt);
    }
    weight = 0;
    sel = (pkt->hash64 % total_weight) + 1;
    TAILQ_FOREACH(bucket, list, entry) {
      weight++;
      if (sel <= weight) {
        break;
      }
    }
  } else {
    if (pkt->hash64 == 0) {
      calc_packet_hash(pkt);
    }
    sel = (pkt->hash64 % total_weight) + 1;
    weight = 0;
    TAILQ_FOREACH(bucket, list, entry) {
      weight += bucket->ofp.weight;
      if (sel <= weight) {
        break;
      }
    }
  }
  return bucket;
}

/**
 * Execute action bucket referenced by group id.
 *
 * @param[in]   pkt             target packet.
 * @param[in]   group_id        OpenFlow Group ID.
 */
lagopus_result_t
execute_group_action(struct lagopus_packet *pkt, uint32_t group_id) {
  /*
   * 5.6.1 Group Types says,
   *  requiled: all
   *  optional: select
   *  requiled: indirect
   *  optional fast failover
   */
  struct group *group;
  struct bucket *bucket;
  lagopus_result_t rv;

  if (pkt->bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }
  group = group_table_lookup(pkt->bridge->group_table, group_id);

  if (unlikely(group == NULL)) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }
  group->packet_count++;
  group->byte_count += OS_M_PKTLEN(PKT2MBUF(pkt));
  rv = LAGOPUS_RESULT_OK;

  switch (group->type) {
    case OFPGT_ALL:
      TAILQ_FOREACH(bucket, &group->bucket_list, entry) {
        struct lagopus_packet *cpkt;

        bucket->counter.packet_count++;
        bucket->counter.byte_count += OS_M_PKTLEN(PKT2MBUF(pkt));
        cpkt = copy_packet(pkt);
        if (cpkt != NULL) {
          re_classify_packet(cpkt);
          rv = execute_action_set(cpkt, bucket->actions);
          if (rv != LAGOPUS_RESULT_NO_MORE_ACTION) {
            lagopus_packet_free(cpkt);
          }
        }
      }
      if ((pkt->flags & PKT_FLAG_CACHED_FLOW) == 0 && pkt->cache != NULL &&
          pkt->hash64 != 0) {
        /* register crc and flows to cache. */
        register_cache(pkt->cache, pkt->hash64,
                       pkt->nmatched, pkt->matched_flow);
      }
      /* to free original packet */
      lagopus_packet_free(pkt);
      rv = LAGOPUS_RESULT_NO_MORE_ACTION;
      break;

    case OFPGT_SELECT:
      /*
       * select one bucket.
       * selection algorithm is depend on the switch.
       */
      bucket = group_select_bucket(pkt, &group->bucket_list);
      if (bucket != NULL) {
        bucket->counter.packet_count++;
        bucket->counter.byte_count += OS_M_PKTLEN(PKT2MBUF(pkt));
        rv = execute_action_set(pkt, bucket->actions);
      }
      break;

    case OFPGT_INDIRECT:
      /* execute only one bucket */
      bucket = TAILQ_FIRST(&group->bucket_list);
      if (bucket != NULL) {
        bucket->counter.packet_count++;
        bucket->counter.byte_count += OS_M_PKTLEN(PKT2MBUF(pkt));
        rv = execute_action_set(pkt, bucket->actions);
      }
      break;

    case OFPGT_FF:
      /* execute only one live bucket */
      bucket = group_live_bucket(pkt->bridge, group);
      if (bucket != NULL) {
        bucket->counter.packet_count++;
        bucket->counter.byte_count += OS_M_PKTLEN(PKT2MBUF(pkt));
        rv = execute_action_set(pkt, bucket->actions);
      }
      break;

    default:
      break;
  }
  return rv;
}

/**
 * Metering packet internal API.
 *
 * @param[in]   pkt             target packet.
 * @param[in]   meter_table     meter database.
 * @param[in]   meter_id        OpenFlow Meter ID.
 *
 * @retval      0       continue to process packet.
 * @retval      -1      packet is dropped.
 *
 * rate counter and timer is depended on lower driver.
 * lower driver is coloring packet and returns result.
 */
STATIC int
apply_meter(struct lagopus_packet *pkt, struct meter_table *meter_table,
            uint32_t meter_id) {
  /*
   * 5.7.1 Meter Bands says,
   * meter band contains
   * - band type
   * - rate
   * - counters
   * - type specific arguments
   * there is no band type "required".
   * optional band types are
   * - drop
   * - dscp remark
   */
  struct meter *meter;
  uint8_t prec_level;

  meter = meter_table_lookup(meter_table, meter_id);
  if (meter != NULL) {
    int dscp;

    switch (lagopus_meter_packet(pkt, meter, &prec_level)) {
      case OFPMBT_DROP:
        return -1;

      case OFPMBT_DSCP_REMARK:
        if (pkt->ether_type == ETHERTYPE_IP) {
          dscp = IPV4_TOS(pkt->ipv4) >> 2;
        } else if (pkt->ether_type == ETHERTYPE_IPV6) {
          dscp = (OS_NTOHL(IPV6_VTCF(pkt->ipv6)) >> 22) & 0x3f;
        } else {
          /* DSCP remarking for non-IP packet, no effect. */
          return 0;
        }
        switch (dscp & 0x07) {
          case 0:
            /* CSx */
            if (dscp >= (prec_level << 3)) {
              dscp -= (uint8_t)(prec_level << 3);
            }
            break;
          case 2:
          case 4:
            /* AFxy */
            if ((dscp & 0x07) + (prec_level << 1) <= 7) {
              dscp  += (uint8_t)(prec_level << 1);
            }
            break;
          default:
            /*
             * experimental or already high drop precedence,
             * nothing to do.
             */
            break;
        }
        if (pkt->ether_type == ETHERTYPE_IP) {
          IPV4_TOS(pkt->ipv4) &= 0x03;
          IPV4_TOS(pkt->ipv4) |= (uint8_t)(dscp << 2);
          pkt->flags |= PKT_FLAG_RECALC_IPV4_CKSUM;
        } else if (pkt->ether_type == ETHERTYPE_IPV6) {
          IPV6_VTCF(pkt->ipv6) &= OS_HTONL(0xf03fffffU);
          IPV6_VTCF(pkt->ipv6) |= OS_HTONL((uint32_t)(dscp << 22));
        }
        break;

      default:
        break;
    }
  }
  return 0;
}

lagopus_result_t
send_port_status(struct port *port, uint8_t reason) {
  struct eventq_data *entry;
  lagopus_result_t rv;

  if (port == NULL || port->bridge == NULL) {
    return LAGOPUS_RESULT_INVALID_OBJECT;
  }
  entry = malloc(sizeof(*entry));
  if (entry == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  entry->type = LAGOPUS_EVENTQ_PORT_STATUS;
  entry->free = (void *)free;
  entry->port_status.ofp_port_status.reason = reason;
  entry->port_status.ofp_port_status.desc = port->ofp_port;

  rv = dp_eventq_data_put(port->bridge->dpid,
                          &entry, PUT_TIMEOUT);
  if (rv != LAGOPUS_RESULT_OK) {
    lagopus_perror(rv);
  }
  return rv;
}

static void
packet_in_free(struct eventq_data *data) {
  if (data != NULL) {
    ofp_match_list_elem_free(&data->packet_in.match_list);
    if (data->packet_in.data != NULL) {
      pbuf_free(data->packet_in.data);
    }
    free(data);
  }
}

int
lagopus_send_packet_physical(struct lagopus_packet *pkt,
                             struct interface *ifp) {
  if (ifp == NULL) {
    return LAGOPUS_RESULT_OK;
  }
  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV:
#ifdef HAVE_DPDK
      return dpdk_send_packet_physical(pkt, ifp);
#else
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      return rawsock_send_packet_physical(pkt,
                                          ifp->info.eth_rawsock.port_number);

    case DATASTORE_INTERFACE_TYPE_GRE:
    case DATASTORE_INTERFACE_TYPE_NVGRE:
    case DATASTORE_INTERFACE_TYPE_VXLAN:
    case DATASTORE_INTERFACE_TYPE_VHOST_USER:
    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      /* TODO */
      lagopus_packet_free(pkt);
      return LAGOPUS_RESULT_OK;
    default:
      break;
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

static lagopus_result_t
send_packet_in(struct lagopus_packet *pkt,
               size_t size,
               uint8_t reason,
               uint16_t miss_send_len,
               uint64_t cookie) {
  struct eventq_data *data;
  struct match *port_match, *metadata_match;
  struct pbuf *pbuf;
  uint32_t port_no;
  lagopus_result_t rv;

  DP_PRINT("%s\n", __func__);
  if (pkt->bridge == NULL) {
    return LAGOPUS_RESULT_INVALID_OBJECT;
  }
  if ((pkt->flags & PKT_FLAG_RECALC_CKSUM_MASK) != 0) {
    if (pkt->ether_type == ETHERTYPE_IP) {
      lagopus_update_ipv4_checksum(pkt);
    } else if (pkt->ether_type == ETHERTYPE_IPV6) {
      lagopus_update_ipv6_checksum(pkt);
    }
  }
  data = malloc(sizeof(*data));
  if (data == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  pbuf = pbuf_alloc(size);
  if (pbuf == NULL) {
    free(data);
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  port_match = calloc(1, sizeof(struct match) + sizeof(port_no));
  if (port_match == NULL) {
    pbuf_free(pbuf);
    free(data);
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  if (pkt->oob_data.metadata != 0) {
    metadata_match = calloc(1, sizeof(struct match) +
                            sizeof(pkt->oob_data.metadata));
    if (metadata_match == NULL) {
      pbuf_free(pbuf);
      free(data);
      free(port_match);
      return LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    metadata_match = NULL;
  }
  data->type = LAGOPUS_EVENTQ_PACKET_IN;
  data->free = packet_in_free;
  data->packet_in.ofp_packet_in.buffer_id = OFP_NO_BUFFER;
  data->packet_in.ofp_packet_in.reason = reason;
  data->packet_in.ofp_packet_in.table_id = pkt->table_id;
  data->packet_in.ofp_packet_in.cookie = cookie;
  ENCODE_PUT(OS_MTOD(PKT2MBUF(pkt), void *), size);
  data->packet_in.data = pbuf;
  data->packet_in.miss_send_len = miss_send_len;

  TAILQ_INIT(&data->packet_in.match_list);
  /*
   * make context as match_list.
   * standard contexts are IN_PORT, IN_PHY_PORT, METADATA and TUNNEL_ID.
   */
  /* IN_PORT */
  port_match->oxm_field = FIELD(OFPXMT_OFB_IN_PORT);
  port_match->oxm_length = sizeof(port_no);
  port_no = OS_HTONL(pkt->in_port->ofp_port.port_no);
  OS_MEMCPY(port_match->oxm_value, &port_no, sizeof(port_no));
  port_match->oxm_class = OFPXMC_OPENFLOW_BASIC;
  TAILQ_INSERT_TAIL(&data->packet_in.match_list, port_match, entry);

  /* IN_PHY_PORT for physical port is omitted. */

  /* METADATA */
  if (pkt->oob_data.metadata != 0) {
    metadata_match->oxm_field = FIELD(OFPXMT_OFB_METADATA);
    metadata_match->oxm_length = sizeof(pkt->oob_data.metadata);
    OS_MEMCPY(metadata_match->oxm_value,
              &pkt->oob_data.metadata,
              sizeof(pkt->oob_data.metadata));
    metadata_match->oxm_class = OFPXMC_OPENFLOW_BASIC;
    TAILQ_INSERT_TAIL(&data->packet_in.match_list, metadata_match, entry);
  } else {
    free(metadata_match);
  }

  /* TUNNEL_ID for physical port is omitted. */

  DP_PRINT("%s: put packet to dataq\n", __func__);
  rv = dp_dataq_data_put(pkt->bridge->dpid,
                         &data, PUT_TIMEOUT);
  if (rv != LAGOPUS_RESULT_OK) {
    DP_PRINT("%s: %s\n", __func__, lagopus_error_get_string(rv));
    data->free(data);
  }
  return rv;
}

static bool
lagopus_do_send_iterate(void *key, void *val,
                        lagopus_hashentry_t he, void *arg) {
  struct port *port;
  struct lagopus_packet *pkt;

  (void) key;
  (void) he;

  port = val;
  pkt = arg;
  if ((port->ofp_port.config & OFPPC_PORT_DOWN) == 0 &&
      port->interface != NULL &&
      port != pkt->in_port &&
      (port->ofp_port.config & OFPPC_NO_FWD) == 0) {
    /* send packet */
    OS_M_ADDREF(PKT2MBUF(pkt));
    lagopus_send_packet_physical(pkt, port->interface);
  }
  return true;
}


static void
dp_interface_tx_packet(struct lagopus_packet *pkt,
                       uint32_t out_port,
                       uint64_t cookie) {
  struct port *port;
  uint32_t in_port;

  in_port = pkt->in_port->ofp_port.port_no;

  switch (out_port) {
    case OFPP_TABLE:
      /*
       * required: used only valid in an output action
       * in the action list of a packet-out message
       * output action to process the packet through the OpenFlow pipeline,
       * starting at the first flow table.
       * see 4.5 Reserved Ports and 7.3.7 Packet-Out Message.
       *
       * do loop infinity if OFPP_TABLE in flow entry.
       * - check flow entry by OpenFlow Controller?
       * - check flow entry at registering flow?
       * - and/or check loop execution runtime?
       */
      DP_PRINT("OFPP_TABLE\n");
      pkt->table_id = 0;
      lagopus_match_and_action(pkt);
      return;

    case OFPP_NORMAL:
      /* optional */
      DP_PRINT("OFPP_NORMAL\n");
      dp_interface_send_packet_normal(pkt, pkt->in_port->interface, pkt->bridge->l2_bridge);
      break;

    case OFPP_FLOOD:
      /* optional */
      /* XXX not implemented yet */
      /* if no mac address learning, OFPP_FLOOD is identical to OFPP_ALL. */
      DP_PRINT("OFPP_FLOOD as ");
      /*FALLTHROUGH*/

    case OFPP_ALL:
      /* required: send packet to all physical ports except in_port. */
      /* XXX destination mac address learning should be needed for flooding. */
      DP_PRINT("OFPP_ALL\n");
      lagopus_hashmap_iterate(&pkt->bridge->ports,
                              lagopus_do_send_iterate,
                              pkt);
      lagopus_packet_free(pkt);
      break;

    case OFPP_CONTROLLER:
      /* required: send packet-in message with OFPR_ACTION to controller */
      /* XXX max_len from config */
      DP_PRINT("OFPP_CONTROLLER\n");
      if ((pkt->bridge->controller_port.config & OFPPC_NO_PACKET_IN) == 0) {
        uint8_t reason;

        if (pkt->flow != NULL && pkt->flow->priority == 0) {
          reason = OFPR_NO_MATCH;
        } else {
          reason = OFPR_ACTION;
        }
        send_packet_in(pkt, OS_M_PKTLEN(PKT2MBUF(pkt)), reason,
                       OFPCML_NO_BUFFER, cookie);
      }
      lagopus_packet_free(pkt);
      break;

    case OFPP_LOCAL:
      /* optional */
      /* XXX not implemented yet */
      DP_PRINT("OFPP_LOCAL\n");
      lagopus_packet_free(pkt);
      break;

    case OFPP_IN_PORT:
      /* required: send packet to ingress port. */
      DP_PRINT("OFPP_IN_PORT as ");
      out_port = in_port;
      /*FALLTHROUGH*/
    default:
      port = port_lookup(&pkt->bridge->ports, out_port);
      if (port != NULL && (port->ofp_port.config & OFPPC_NO_FWD) == 0) {
        /* so far, we support only physical port. */
        DP_PRINT("Forwarding packet to port %d\n", port->ifindex);
        lagopus_send_packet_physical(pkt, port->interface);
      } else {
        lagopus_packet_free(pkt);
      }
      break;
  }
}

#ifdef HYBRID
void
lagopus_forward_packet_to_port_hybrid(struct lagopus_packet *pkt) {
  dp_interface_tx_packet(pkt, pkt->output_port, 0);
}
#endif /* HYBRID */

void
lagopus_forward_packet_to_port(struct lagopus_packet *pkt,
                               uint32_t out_port) {
  dp_interface_tx_packet(pkt, out_port, 0);
}

/**
 * Output action.
 * If last action is this, free packet.
 * freeing packet implicitly by send packet function, or
 * explicitly by this function.
 */
static lagopus_result_t
execute_action_output(struct lagopus_packet *pkt,
                      struct action *action) {
  lagopus_result_t rv;
  uint32_t port;

  /* required action */
  port = ((struct ofp_action_output *)&action->ofpat)->port;
  DP_PRINT("action output: %d\n", port);
  if (unlikely(action->flags == OUTPUT_COPIED_PACKET)) {
    /* send copied packet */
    dp_interface_tx_packet(copy_packet(pkt), port, action->cookie);
    rv = LAGOPUS_RESULT_OK;
  } else {
    if ((pkt->flags & PKT_FLAG_CACHED_FLOW) == 0 && pkt->cache != NULL &&
        pkt->hash64 != 0) {
      /* register crc and flows to cache. */
      register_cache(pkt->cache, pkt->hash64,
                     pkt->nmatched, pkt->matched_flow);
    }
    dp_interface_tx_packet(pkt, port, action->cookie);
    rv = LAGOPUS_RESULT_NO_MORE_ACTION;
  }
  return rv;
}

static lagopus_result_t
execute_action_copy_ttl_out(struct lagopus_packet *pkt,
                            __UNUSED struct action *action) {
  IPV4_HDR *ipv4_hdr;
  IPV6_HDR *ipv6_hdr;
  struct mpls_hdr *mpls_hdr;
  OS_MBUF *m;

  DP_PRINT("action copy_ttl_out\n");

  m = PKT2MBUF(pkt);

  /* optional */
  if (pkt->mpls != NULL) {
    if (MPLS_BOS(pkt->mpls->mpls_lse) != 0) {
      /* outer MPLS is Bottom of Stack */
      ipv4_hdr = pkt->ipv4;
      ipv6_hdr = pkt->ipv6;
      /*
       * hmm, MPLS overrides ether type. how to detect L3 protocol?
       * draft-hsmit-isis-aal5mux-00.txt says
       * 0x45-0x4f is IPv4
       * 0x60-0x6f is IPv6
       * 0x81-0x83 is OSI (CLNP,ES-IS,IS-IS)
       */
      if (IPV4_VER(ipv4_hdr) == 4) {
        SET_MPLS_TTL(pkt->mpls->mpls_lse, IPV4_TTL(ipv4_hdr));
      } else if (IPV6_VER(ipv6_hdr) == 6) {
        SET_MPLS_TTL(pkt->mpls->mpls_lse, IPV6_HLIM(ipv6_hdr));
      }
    } else {
      /* MPLS-MPLS */
      mpls_hdr = MTOD_OFS(m, sizeof(ETHER_HDR) + sizeof(struct mpls_hdr),
                          struct mpls_hdr *);
      SET_MPLS_TTL(pkt->mpls->mpls_lse, MPLS_TTL(mpls_hdr->mpls_lse));
    }
  } else {
    /*
     * XXX - The following cases are not implemented yet.
     *   inner-IP(v4/v6) to outer-IP(v4/v6) copy
     */
  }
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_copy_ttl_in(struct lagopus_packet *pkt,
                           __UNUSED struct action *action) {
  IPV4_HDR *ipv4_hdr;
  IPV6_HDR *ipv6_hdr;
  struct mpls_hdr *mpls_hdr;
  OS_MBUF *m;

  DP_PRINT("action copy_ttl_in\n");

  m = PKT2MBUF(pkt);

  /* optional */
  if (pkt->mpls != NULL) {
    if (MPLS_BOS(pkt->mpls->mpls_lse) != 0) {
      ipv4_hdr = pkt->ipv4;
      ipv6_hdr = pkt->ipv6;
      /*
       * hmm, MPLS overrides ether type. how to detect L3 protocol?
       * draft-hsmit-isis-aal5mux-00.txt says
       * 0x45-0x4f is IPv4
       * 0x60-0x6f is IPv6
       * 0x81-0x83 is OSI (CLNP,ES-IS,IS-IS)
       */
      if (IPV4_VER(ipv4_hdr) == 4) {
        IPV4_TTL(ipv4_hdr) = MPLS_TTL(pkt->mpls->mpls_lse);
        pkt->flags |= PKT_FLAG_RECALC_IPV4_CKSUM;
      } else if (IPV6_VER(ipv6_hdr) == 6) {
        IPV6_HLIM(ipv6_hdr) = MPLS_TTL(pkt->mpls->mpls_lse);
      }
    } else {
      /* MPLS-MPLS */
      mpls_hdr = MTOD_OFS(m, sizeof(ETHER_HDR) + sizeof(struct mpls_hdr),
                          struct mpls_hdr *);
      SET_MPLS_TTL(mpls_hdr->mpls_lse, MPLS_TTL(pkt->mpls->mpls_lse));
    }
  } else {
    /*
     * XXX - The following cases are not implemented yet.
     *   outer-IP(v4/v6) to inner-IP(v4/v6) copy
     */
  }
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_set_mpls_ttl(struct lagopus_packet *pkt,
                            struct action *action) {
  DP_PRINT("action set_mpls_ttl: %d\n",
           ((struct ofp_action_mpls_ttl *)&action->ofpat)->mpls_ttl);

  /* optional */
  SET_MPLS_TTL(pkt->mpls->mpls_lse,
               ((struct ofp_action_mpls_ttl *)&action->ofpat)->mpls_ttl);
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_dec_mpls_ttl(struct lagopus_packet *pkt,
                            __UNUSED struct action *action) {
  uint32_t ttl;
  uint16_t miss_send_len;

  DP_PRINT("action dec_mpls_ttl\n");

  /* optional */
  ttl = MPLS_TTL(pkt->mpls->mpls_lse);
  if (likely(ttl > 0)) {
    SET_MPLS_TTL(pkt->mpls->mpls_lse, ttl - 1);
  }

  /* if invalid.  send packet_in with OFPR_INVALID_TTL to controller. */
  if (unlikely(MPLS_TTL(pkt->mpls->mpls_lse) == 0)) {
    if (pkt->bridge != NULL) {
      miss_send_len = pkt->bridge->switch_config.miss_send_len;
    } else {
      miss_send_len = 128;
    }
    send_packet_in(pkt,
                   OS_M_PKTLEN(PKT2MBUF(pkt)),
                   OFPR_INVALID_TTL,
                   miss_send_len,
                   action->cookie);
    return LAGOPUS_RESULT_STOP;
  }
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_decap(struct lagopus_packet *pkt,
                     struct action *action) {
  struct ofp_action_decap *decap = &action->ofpat;
  OS_MBUF *m;
  void *new_p;
  uint32_t new_type;

  m = PKT2MBUF(pkt);
  switch (decap->cur_pkt_type) {
    case (OFPHTN_ONF << 16) | OFPHTO_ETHERNET:
      new_type = (OFPHTN_ETHERTYPE << 16) | ETHER_TYPE(pkt->eth);
      new_p = OS_M_ADJ(m, sizeof(ETHER_HDR));
      pkt->eth = NULL;
      break;

    case (OFPHTN_ETHERTYPE << 16) | ETHERTYPE_MPLS:
    case (OFPHTN_ETHERTYPE << 16) | ETHERTYPE_MPLS_MCAST:
      if (MPLS_BOS(pkt->mpls->mpls_lse)) {
        new_type = ((OFPHTN_ONF << 16) | OFPHTO_ETHERNET);
      } else {
        new_type = decap->cur_pkt_type;
      }
      new_p = OS_M_ADJ(m, sizeof(MPLS_HDR));
      pkt->ipv4 = NULL;
      break;

    case (OFPHTN_ETHERTYPE << 16) | ETHERTYPE_IP:
      new_type = (OFPHTN_IP_PROTO << 16) | *pkt->proto;
      new_p = OS_M_ADJ(m, sizeof(IPV4_HDR));
      pkt->ipv4 = NULL;
      pkt->proto = NULL;
      break;

    case (OFPHTN_IP_PROTO << 16) | IPPROTO_UDP:
      new_type = (OFPHTN_UDP_TCP_PORT << 16) | UDP_DPORT(pkt->udp);
      new_p = OS_M_ADJ(m, sizeof(uint16_t) * 4);
      pkt->udp = NULL;
      break;

    case (OFPHTN_IP_PROTO << 16) | IPPROTO_GRE:
      switch (OS_NTOHS(pkt->gre->ptype)) {
        case 0x880b:
          new_type = ((OFPHTN_ONF << 16) | OFPHTO_ETHERNET);
          break;
        default:
          new_type = ((OFPHTN_ETHERTYPE << 16) | OS_NTOHS(pkt->gre->ptype));
          break;
      }
      new_p = OS_M_ADJ(m, sizeof(GRE_HDR));
      pkt->gre = NULL;
      break;

    case (OFPHTN_UDP_TCP_PORT << 16) | VXLAN_PORT:
      new_type = ((OFPHTN_ONF << 16) | OFPHTO_ETHERNET);
      new_p = OS_M_ADJ(m, sizeof(VXLAN_HDR));
      pkt->vxlan = NULL;
      break;

    case (OFPHTN_UDP_TCP_PORT << 16) | GTPU_PORT:
      new_p = OS_M_ADJ(m, sizeof(struct gtpu_hdr));
      if (IPV4_VER((IPV4_HDR *)new_p) == 4) {
        new_type = (OFPHTN_ETHERTYPE << 16) | ETHERTYPE_IP;
      } else if (IPV6_VER((IPV6_HDR *)new_p) == 6) {
        new_type = (OFPHTN_ETHERTYPE << 16) | ETHERTYPE_IPV6;
      } else {
        /* XXX? */
        new_type = decap->new_pkt_type;
      }
      pkt->gtpu = NULL;
      break;

    default:
      return OFPBAC_BAD_HEADER_TYPE;
  }
  if (decap->new_pkt_type == ((OFPHTN_ONF << 16) | OFPHTO_USE_NEXT_PROTO)) {
    pkt->oob_data.packet_type = new_type;
    if (new_type == ((OFPHTN_ONF << 16) | OFPHTO_ETHERNET)) {
      /* don't recalc hash value of original packet */
      re_classify_packet(pkt);
    } else if ((new_type >> 16) == OFPHTN_ETHERTYPE) {
      pkt->l3_hdr = new_p;
      switch (new_type & 0xffff) {
        case ETHERTYPE_IP:
          classify_packet_ipv4(pkt);
          classify_packet_l4(pkt);
          break;
        case ETHERTYPE_IPV6:
          classify_packet_ipv6(pkt);
          classify_packet_l4(pkt);
          break;
        default:
          break;
      }
    } else if ((new_type >> 16) == OFPHTN_IP_PROTO) {
      pkt->l4_hdr = new_p;
      /* XXX pkt->proto */
      classify_packet_l4(pkt);
    }
  } else {
    pkt->oob_data.packet_type = decap->new_pkt_type;
  }

  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_encap(struct lagopus_packet *pkt,
                     struct action *action) {
  struct ofp_action_encap *encap = &action->ofpat;
  OS_MBUF *m;

  m = PKT2MBUF(pkt);
  switch (encap->packet_type) {
    case (OFPHTN_ETHERTYPE << 16) | ETHERTYPE_MPLS:
    case (OFPHTN_ETHERTYPE << 16) | ETHERTYPE_MPLS_MCAST: {
      MPLS_HDR *new_mpls;

      new_mpls = (MPLS_HDR *)OS_M_PREPEND(m, sizeof(MPLS_HDR));
      pkt->mpls = new_mpls;
    }
      break;

    case ((OFPHTN_IP_PROTO << 16) | IPPROTO_GRE): {
      GRE_HDR *new_gre;

      new_gre = (GRE_HDR *)OS_M_PREPEND(m, sizeof(GRE_HDR));
      new_gre->flags = OS_HTONS(0x2000);
      if (pkt->oob_data.packet_type == ((OFPHTN_ONF << 16) | OFPHTO_ETHERNET)) {
        new_gre->ptype = OS_HTONS(0x6558);
      } else if ((pkt->oob_data.packet_type >> 16) == OFPHTN_ETHERTYPE) {
        new_gre->ptype = OS_HTONS(pkt->oob_data.packet_type & 0xffff);
      }
      new_gre->key = OS_HTONL((OS_M_PKTLEN(m) - sizeof(GRE_HDR)) << 16);
      pkt->gre = new_gre;
    }
      break;

    case ((OFPHTN_UDP_TCP_PORT << 16) | VXLAN_PORT): {
      VXLAN_HDR *new_vxlan;

      new_vxlan = (VXLAN_HDR *)OS_M_PREPEND(m, sizeof(VXLAN_HDR));
      new_vxlan->flags = 0x08;
      new_vxlan->pad[0] = 0;
      new_vxlan->pad[1] = 0;
      new_vxlan->pad[2] = 0;
      new_vxlan->vni = 0;
      pkt->vxlan = new_vxlan;
    }
      break;

    case ((OFPHTN_UDP_TCP_PORT << 16) | GTPU_PORT): {
      struct gtpu_hdr *new_gtpu;

      new_gtpu = (struct gtpu_hdr *)OS_M_PREPEND(m, sizeof(struct gtpu_hdr));
      new_gtpu->verflags = 0x20; /* ver = 1 */
      new_gtpu->msgtype = 255; /* G-PDU */
      /* note: extension header size is included in length */
      new_gtpu->length = OS_HTONS(OS_M_PKTLEN(m) - sizeof(struct gtpu_hdr));
      new_gtpu->te_id = 0;
      pkt->gtpu = new_gtpu;
    }
      break;

    case ((OFPHTN_IP_PROTO << 16) | IPPROTO_UDP): {
      UDP_HDR *new_udp;
      uint16_t sport;

      new_udp = (UDP_HDR *)OS_M_PREPEND(m, sizeof(uint16_t) * 4);
      if ((pkt->oob_data.packet_type >> 16) ==OFPHTN_UDP_TCP_PORT) {
        sport = CityHash64WithSeed(pkt->l4_payload, sizeof(ETHER_HDR), 0);
        UDP_SPORT(new_udp) = OS_HTONS(sport | 0xc000);
        UDP_DPORT(new_udp) = OS_HTONS(pkt->oob_data.packet_type & 0xffff);
      }
      UDP_LEN(new_udp) = OS_HTONS(OS_M_PKTLEN(m));
      UDP_CKSUM(new_udp) = 0;
      pkt->flags |= PKT_FLAG_RECALC_L4_CKSUM;
      pkt->udp = new_udp;
    }
      break;

    case (OFPHTN_ETHERTYPE << 16) | ETHERTYPE_IP: {
      IPV4_HDR *new_ip;

      new_ip = (IPV4_HDR *)OS_M_PREPEND(m, sizeof(IPV4_HDR));
      IPV4_VER(new_ip) = 4;
      IPV4_HLEN(new_ip) = sizeof(IPV4_HDR) >> 2;
      if ((pkt->oob_data.packet_type >> 16) == OFPHTN_IP_PROTO) {
        IPV4_PROTO(new_ip) = pkt->oob_data.packet_type & 0xff;
      }
      IPV4_TTL(new_ip) = 64;
      new_ip->ip_len = OS_HTONS(OS_M_PKTLEN(m));
      pkt->flags |= PKT_FLAG_RECALC_IPV4_CKSUM;
      pkt->ipv4 = new_ip;
      pkt->proto = &pkt->ipv4->ip_p;
      pkt->ether_type = ETHERTYPE_IP;
    }
      break;

    case (OFPHTN_ONF << 16) | OFPHTO_ETHERNET: {
      ETHER_HDR *new_eth;

      new_eth = (ETHER_HDR *)OS_M_PREPEND(m, sizeof(ETHER_HDR));
      ETHER_TYPE(new_eth) = OS_HTONS(pkt->oob_data.packet_type & 0xffff);
      pkt->eth = new_eth;
    }
      break;

    default:
      DPRINT("encap->packet_type 0x%08x\n", encap->packet_type);
      return LAGOPUS_RESULT_INVALID_ARGS;
  }
  pkt->oob_data.packet_type = encap->packet_type;
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_push_vlan(struct lagopus_packet *pkt,
                         struct action *action) {
  ETHER_HDR *new_hdr;
  VLAN_HDR *vlan_hdr;
  OS_MBUF *m;

  DP_PRINT("action push_vlan: 0x%04x\n",
           ((struct ofp_action_push *)&action->ofpat)->ethertype);

  m = PKT2MBUF(pkt);

  /* optional */
  new_hdr = (ETHER_HDR *)OS_M_PREPEND(m, sizeof(VLAN_HDR));
  if (NEED_COPY_ETH_ADDR(action->flags)) {
    memmove(new_hdr, pkt->eth, ETHER_ADDR_LEN * 2);
  }
  ETHER_TYPE(new_hdr) =
    OS_HTONS(((struct ofp_action_push *)&action->ofpat)->ethertype);
  vlan_hdr = (VLAN_HDR *)&new_hdr[1];
  if (pkt->vlan != NULL) {
    /* priority and VID */
    VLAN_TCI(vlan_hdr) = VLAN_TCI(pkt->vlan);
  } else {
    VLAN_TCI(vlan_hdr) = 0;
  }
  /* re-classify packet. */
  pkt->eth = new_hdr;
  pkt->vlan = vlan_hdr;
  pkt->oob_data.vlan_tci = VLAN_TCI(vlan_hdr) | htons(OFPVID_PRESENT);
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_pop_vlan(struct lagopus_packet *pkt,
                        struct action *action) {
  ETHER_HDR *new_hdr;
  OS_MBUF *m;

  DP_PRINT("action pop_vlan\n");

  m = PKT2MBUF(pkt);

  /* optional */
  new_hdr = MTOD_OFS(m, sizeof(VLAN_HDR), ETHER_HDR *);
  if (NEED_COPY_ETH_ADDR(action->flags)) {
    memmove(new_hdr, pkt->eth, ETHER_ADDR_LEN * 2);
  }
  OS_M_ADJ(m, sizeof(VLAN_HDR));
  /* re-classify packet. */
  pkt->eth = new_hdr;
  pkt->vlan = NULL;
  pkt->oob_data.vlan_tci = 0;
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_push_mpls(struct lagopus_packet *pkt,
                         struct action *action) {
  ETHER_HDR *new_hdr;
  struct mpls_hdr *mpls_hdr;
  uint32_t label;
  uint32_t ttl;
  uint32_t tc;
  uint32_t bos;
  size_t copy_size;
  OS_MBUF *m;

  DP_PRINT("action push_mpls: 0x%04x\n",
           ((struct ofp_action_push *)&action->ofpat)->ethertype);

  /* optional */
  /* inherit value from existing fields */
  if (pkt->mpls != NULL) {
    /* multiple stack */
    mpls_hdr = pkt->mpls;
    label = MPLS_LBL(mpls_hdr->mpls_lse);
    ttl = MPLS_TTL(mpls_hdr->mpls_lse);
    tc = MPLS_EXP(mpls_hdr->mpls_lse);
    bos = 0;
    copy_size = (size_t)(pkt->base[MPLS_BASE] - pkt->base[ETH_BASE]);
  } else {
    label = 0;
    tc = 0;
    bos = 1;
    if (pkt->ether_type == ETHERTYPE_IP) {
      ttl = IPV4_TTL(pkt->ipv4);
    } else if (pkt->ether_type == ETHERTYPE_IPV6) {
      ttl = IPV6_HLIM(pkt->ipv6);
    } else {
      ttl = 0;
    }
    copy_size = (size_t)(pkt->base[L3_BASE] - pkt->base[ETH_BASE]);
  }

  m = PKT2MBUF(pkt);
  new_hdr = (ETHER_HDR *)OS_M_PREPEND(m, sizeof(struct mpls_hdr));
  if (NEED_COPY_ETH_ADDR(action->flags)) {
    memmove(new_hdr, pkt->eth, copy_size);
  } else if (copy_size > sizeof(ETHER_HDR)) {
    memmove(&ETHER_TYPE(new_hdr), &ETHER_TYPE(pkt->eth),
            copy_size - sizeof(ETHER_HDR));
  }
  if (pkt->mpls != NULL) {
    mpls_hdr = pkt->mpls - 1;
  } else {
    mpls_hdr = (struct mpls_hdr *)(pkt->l3_hdr_l - 1);
  }
  *(((uint16_t *)mpls_hdr) - 1) =
    OS_HTONS(((struct ofp_action_push *)&action->ofpat)->ethertype);
  SET_MPLS_LSE(mpls_hdr->mpls_lse, label, tc, bos, ttl);
  /* re-classify packet. */
  pkt->eth = new_hdr;
  pkt->mpls = mpls_hdr;
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_pop_mpls(struct lagopus_packet *pkt,
                        struct action *action) {
  ETHER_HDR *new_hdr;
  OS_MBUF *m;

  DP_PRINT("action pop_mpls: 0x%04x\n",
           ((struct ofp_action_push *)&action->ofpat)->ethertype);

  m = PKT2MBUF(pkt);
  /* optional */
  /*
   * XXX
   * return error if BoS == 0 and the wrong ether type in the action.
   * see 7.2.5 Action Structures (page 61).
   */
  new_hdr = MTOD_OFS(m, sizeof(struct mpls_hdr), ETHER_HDR *);
  memmove(new_hdr, pkt->eth, pkt->base[MPLS_BASE] - pkt->base[ETH_BASE] - 2);
  pkt->ether_type = ((struct ofp_action_pop_mpls *)&action->ofpat)->ethertype;
  *(((uint16_t *)&pkt->mpls[1]) - 1) = OS_HTONS(pkt->ether_type);
  OS_M_ADJ(m, sizeof(struct mpls_hdr));
  pkt->mpls = NULL;
  if (pkt->ether_type != ETHERTYPE_IP &&
      pkt->ether_type != ETHERTYPE_IPV6) {
    /* re-classify packet if needed. */
    re_classify_packet(pkt);
  } else {
    pkt->eth = new_hdr;
  }
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_set_queue(struct lagopus_packet *pkt,
                         struct action *action) {
  struct ofp_action_set_queue *actq;
  DP_PRINT("action set_queue\n");

  /* optional */
  /* send packets to given queue on port. */
  /* XXX not implemented yet */
  actq = (struct ofp_action_set_queue *)&action->ofpat;
  pkt->queue_id = actq->queue_id;
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_group(struct lagopus_packet *pkt,
                     struct action *action) {
  uint32_t group_id;

  DP_PRINT("action group: %d\n",
           ((struct ofp_action_group *)&action->ofpat)->group_id);

  group_id = ((struct ofp_action_group *)&action->ofpat)->group_id;
  return execute_group_action(pkt, group_id);
}

static lagopus_result_t
execute_action_set_nw_ttl(struct lagopus_packet *pkt,
                          struct action *action) {
  DP_PRINT("action set_nw_ttl: %d\n",
           ((struct ofp_action_nw_ttl *)&action->ofpat)->nw_ttl);

  /* optional */
  if (pkt->ether_type == ETHERTYPE_IP) {
    IPV4_TTL(pkt->ipv4) =
      ((struct ofp_action_nw_ttl *)&action->ofpat)->nw_ttl;
    pkt->flags |= PKT_FLAG_RECALC_IPV4_CKSUM;
  } else if (pkt->ether_type == ETHERTYPE_IPV6) {
    IPV6_HLIM(pkt->ipv6) =
      ((struct ofp_action_nw_ttl *)&action->ofpat)->nw_ttl;
  }
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_dec_nw_ttl(struct lagopus_packet *pkt,
                          __UNUSED struct action *action) {
  uint16_t miss_send_len;

  DP_PRINT("action dec_nw_ttl\n");

  /* optional */
  if (pkt->ether_type == ETHERTYPE_IP) {
    if (likely(IPV4_TTL(pkt->ipv4) > 0)) {
      IPV4_TTL(pkt->ipv4)--;
      if (unlikely(IPV4_CSUM(pkt->ipv4) == 0xffff)) {
        pkt->flags |= PKT_FLAG_RECALC_IPV4_CKSUM;
      } else {
        IPV4_CSUM(pkt->ipv4)++;
      }
    }

    /* if invalid.  send packet_in with OFPR_INVALID_TTL to controller. */
    if (likely(IPV4_TTL(pkt->ipv4) == 0)) {
      if (pkt->bridge != NULL) {
        miss_send_len = pkt->bridge->switch_config.miss_send_len;
      } else {
        miss_send_len = 128;
      }
      send_packet_in(pkt,
                     OS_M_PKTLEN(PKT2MBUF(pkt)),
                     OFPR_INVALID_TTL,
                     miss_send_len,
                     action->cookie);
      return LAGOPUS_RESULT_STOP;
    }
  } else if (pkt->ether_type == ETHERTYPE_IPV6) {
    if (IPV6_HLIM(pkt->ipv6) > 0) {
      IPV6_HLIM(pkt->ipv6)--;
    }

    /* if invalid.  send packet_in with OFPR_INVALID_TTL to controller. */
    if (IPV6_HLIM(pkt->ipv6) == 0) {
      if (pkt->bridge != NULL) {
        miss_send_len = pkt->bridge->switch_config.miss_send_len;
      } else {
        miss_send_len = 128;
      }
      send_packet_in(pkt,
                     OS_M_PKTLEN(PKT2MBUF(pkt)),
                     OFPR_INVALID_TTL,
                     miss_send_len,
                     action->cookie);
      return LAGOPUS_RESULT_STOP;
    }
  }
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_push_pbb(struct lagopus_packet *pkt,
                        struct action *action) {
  struct pbb_hdr *pbb_hdr;
  VLAN_HDR *vlan_hdr;
  ETHER_HDR *new_hdr;
  OS_MBUF *m;

  DP_PRINT("action push_pbb: 0x%04x\n",
           ((struct ofp_action_push *)&action->ofpat)->ethertype);

  m = PKT2MBUF(pkt);

  /* optional */
  /* inherit field value of vlan, ether, or PBB header. */
  new_hdr = (ETHER_HDR *)OS_M_PREPEND(m, sizeof(struct pbb_hdr));
  if (NEED_COPY_ETH_ADDR(action->flags)) {
    memmove(new_hdr, pkt->eth, ETHER_ADDR_LEN * 2);
  }
  ETHER_TYPE(new_hdr) =
    OS_HTONS(((struct ofp_action_push *)&action->ofpat)->ethertype);
  pbb_hdr = MTOD_OFS(m, sizeof(ETHER_HDR), struct pbb_hdr *);
  if (OS_NTOHS(pbb_hdr->c_ethtype) == ETHERTYPE_VLAN) {
    vlan_hdr = MTOD_OFS(m, sizeof(ETHER_HDR) + sizeof(struct pbb_hdr),
                        VLAN_HDR *);
    pbb_hdr->i_pcp_dei = (uint8_t)(VLAN_PCP(vlan_hdr) << 5);
    memset(pbb_hdr->i_sid, 0, sizeof(pbb_hdr->i_sid));
  } else if (OS_NTOHS(pbb_hdr->c_ethtype) == ETHERTYPE_PBB) {
    struct pbb_hdr *inner;

    inner = &pbb_hdr[1];
    pbb_hdr->i_pcp_dei = 0;
    OS_MEMCPY(pbb_hdr->i_sid, inner->i_sid, sizeof(pbb_hdr->i_sid));
  } else {
    pbb_hdr->i_pcp_dei = 0;
    memset(pbb_hdr->i_sid, 0, sizeof(pbb_hdr->i_sid));
  }
  /* re-classify packet if needed. */
  re_classify_packet(pkt);
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_pop_pbb(struct lagopus_packet *pkt,
                       __UNUSED struct action *action) {
  OS_MBUF *m;
  uint8_t *src, *dst;
  size_t size;

  DP_PRINT("action pop_pbb\n");

  m = PKT2MBUF(pkt);
  /* optional */
  OS_MEMCPY(ETHER_DST(pkt->eth), pkt->pbb->c_dhost, ETHER_ADDR_LEN);
  OS_MEMCPY(ETHER_SRC(pkt->eth), pkt->pbb->c_shost, ETHER_ADDR_LEN);
  src = pkt->base[ETH_BASE];
  dst = src + sizeof(struct pbb_hdr);
  size = (size_t)((uint8_t *)pkt->pbb - src) - sizeof(uint16_t);
  memmove(dst, src, size);
  OS_M_ADJ(m, sizeof(struct pbb_hdr));
  /* re-classify packet if needed. */
  re_classify_packet(pkt);
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_experimenter(__UNUSED struct lagopus_packet *pkt,
                            __UNUSED struct action *action) {
  DP_PRINT("action experimenter\n");
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
execute_action_none(__UNUSED struct lagopus_packet *pkt,
                    __UNUSED struct action *action) {
  DP_PRINT("action none\n");
  return LAGOPUS_RESULT_OK;
}

void
lagopus_set_action_function(struct action *action) {
  switch (action->ofpat.type) {
    case OFPAT_OUTPUT:
      action->exec = execute_action_output;
      break;
    case OFPAT_COPY_TTL_OUT:
      action->exec = execute_action_copy_ttl_out;
      break;
    case OFPAT_COPY_TTL_IN:
      action->exec = execute_action_copy_ttl_in;
      break;
    case OFPAT_SET_MPLS_TTL:
      action->exec = execute_action_set_mpls_ttl;
      break;
    case OFPAT_DEC_MPLS_TTL:
      action->exec = execute_action_dec_mpls_ttl;
      break;
    case OFPAT_PUSH_VLAN:
      action->exec = execute_action_push_vlan;
      break;
    case OFPAT_POP_VLAN:
      action->exec = execute_action_pop_vlan;
      break;
    case OFPAT_PUSH_MPLS:
      action->exec = execute_action_push_mpls;
      break;
    case OFPAT_POP_MPLS:
      action->exec = execute_action_pop_mpls;
      break;
    case OFPAT_SET_QUEUE:
      action->exec = execute_action_set_queue;
      break;
    case OFPAT_GROUP:
      action->exec = execute_action_group;
      break;
    case OFPAT_SET_NW_TTL:
      action->exec = execute_action_set_nw_ttl;
      break;
    case OFPAT_DEC_NW_TTL:
      action->exec = execute_action_dec_nw_ttl;
      break;
    case OFPAT_SET_FIELD:
      action->exec = execute_action_set_field;
      break;
    case OFPAT_PUSH_PBB:
      action->exec = execute_action_push_pbb;
      break;
    case OFPAT_POP_PBB:
      action->exec = execute_action_pop_pbb;
      break;
#ifdef GENERAL_TUNNEL_SUPPORT
    case OFPAT_ENCAP:
      action->exec = execute_action_encap;
      break;
    case OFPAT_DECAP:
      action->exec = execute_action_decap;
      break;
#endif /* GENERAL_TUNNEL_SUPPORT */
    case OFPAT_EXPERIMENTER:
      action->exec = execute_action_experimenter;
      break;
    default:
      action->exec = execute_action_none;
      break;
  }
}

#define LAGOPUS_RESULT_CONTINUE 1

lagopus_result_t
execute_instruction_goto_table(struct lagopus_packet *pkt,
                               const struct instruction *instruction) {
  const struct ofp_instruction_goto_table *goto_table;

  if (pkt->cache != NULL && (pkt->flags & PKT_FLAG_CACHED_FLOW)) {
    /* cached flow, table pipelining is cached.  do nothing. */
    return LAGOPUS_RESULT_OK;
  }
  goto_table = &instruction->ofpit_goto_table;
  DP_PRINT("instruction goto_table: %d\n", goto_table->table_id);

  pkt->table_id = goto_table->table_id;
  return LAGOPUS_RESULT_CONTINUE;
}

lagopus_result_t
execute_instruction_write_metadata(struct lagopus_packet *pkt,
                                   const struct instruction *instruction) {
  const struct ofp_instruction_write_metadata *insn;

  insn = &instruction->ofpit_write_metadata;
  DP_PRINT("instruction write_metadata: 0x%016" PRIx64 "/0x%016" PRIx64 "\n",
           insn->metadata, insn->metadata_mask);
  write_metadata(pkt, insn->metadata, insn->metadata_mask);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
execute_instruction_write_actions(struct lagopus_packet *pkt,
                                  const struct instruction *instruction) {
  /* required instruction */
  DP_PRINT("instruction write_actions\n");
  if ((pkt->flags & PKT_FLAG_HAS_ACTION) == 0) {
    int i;

    for (i = 0; i < LAGOPUS_ACTION_SET_ORDER_MAX; i++) {
      TAILQ_INIT(&pkt->actions[i]);
    }
  }
  merge_action_set(pkt->actions, &instruction->action_list);
  pkt->flags |= PKT_FLAG_HAS_ACTION;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
execute_instruction_apply_actions(struct lagopus_packet *pkt,
                                  const struct instruction *instruction) {
  /* optional */
  DP_PRINT("instruction apply_actions\n");
  return execute_action(pkt, &instruction->action_list);
}

lagopus_result_t
execute_instruction_clear_actions(struct lagopus_packet *pkt,
                                  __UNUSED const struct instruction *insn) {
  /* optional */
  DP_PRINT("instruction clear_actions\n");
  clear_action_set(pkt);
  pkt->flags &= (uint32_t)~PKT_FLAG_HAS_ACTION;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
execute_instruction_meter(struct lagopus_packet *pkt,
                          const struct instruction *instruction) {
  /* optional */
  DP_PRINT("instruction meter: %d\n", instruction->ofpit_meter.meter_id);
  if (pkt->bridge != NULL) {
    struct meter_table *meter_table;
    uint32_t meter_id;

    meter_id = instruction->ofpit_meter.meter_id;
    meter_table = pkt->bridge->meter_table;
    if (apply_meter(pkt, meter_table, meter_id) != 0) {
      /* dropped.  exit loop immediately */
      clear_action_set(pkt);
      return LAGOPUS_RESULT_STOP;
    }
  }
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
execute_instruction_experimenter(struct lagopus_packet *pkt,
                                 const struct instruction *instruction) {
  uint32_t exp_id;

  DP_PRINT("instruction experimenter\n");
  exp_id = instruction->ofpit_experimenter.experimenter;
  lagopus_instruction_experimenter(pkt, exp_id);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
execute_instruction_none(__UNUSED struct lagopus_packet *pkt,
                         __UNUSED const struct instruction *instruction) {
  DP_PRINT("instruction none\n");
  return LAGOPUS_RESULT_OK;
}

void
lagopus_set_instruction_function(struct instruction *instruction) {
  switch (instruction->ofpit.type) {
    case OFPIT_GOTO_TABLE:
      instruction->exec = execute_instruction_goto_table;
      break;
    case OFPIT_WRITE_METADATA:
      instruction->exec = execute_instruction_write_metadata;
      break;
    case OFPIT_WRITE_ACTIONS:
      instruction->exec = execute_instruction_write_actions;
      break;
    case OFPIT_APPLY_ACTIONS:
      instruction->exec = execute_instruction_apply_actions;
      break;
    case OFPIT_CLEAR_ACTIONS:
      instruction->exec = execute_instruction_clear_actions;
      break;
    case OFPIT_METER:
      instruction->exec = execute_instruction_meter;
      break;
    case OFPIT_EXPERIMENTER:
      instruction->exec = execute_instruction_experimenter;
      break;
    default:
      instruction->exec = execute_instruction_none;
      break;
  }
}

static inline lagopus_result_t
dp_openflow_do_cached_action(struct lagopus_packet *pkt) {
  struct flowdb *flowdb;
  struct flow *flow;
  struct flow **flowp;
  struct table *table;
  const struct cache_entry *cache_entry;
  lagopus_result_t rv;
  unsigned i;

  flowdb = pkt->bridge->flowdb;

  calc_packet_hash(pkt);
  cache_entry = cache_lookup(pkt->cache, pkt);
  if (likely(cache_entry != NULL)) {
    DP_PRINT("MATCHED (cache)\n");
    pkt->flags |= PKT_FLAG_CACHED_FLOW;
    flowp = cache_entry->flow;

    rv = LAGOPUS_RESULT_OK;
    for (i = 0; i < cache_entry->nmatched; i++) {
      flow = *flowp++;
      flow->packet_count++;
      flow->byte_count += OS_M_PKTLEN(PKT2MBUF(pkt));
      if (flow->idle_timeout != 0 || flow->hard_timeout != 0) {
        flow->update_time = get_current_time();
      }
      pkt->flow = flow;
      pkt->table_id = flow->table_id;
      table = table_lookup(flowdb, pkt->table_id);
#ifdef DIAGNOSTIC
      if (table == NULL) {
        printf("cache_entry->flow[%u].table_id = %d, invalid\n",
               i, flow->table_id);
      }
#endif /* DIAGNOSTIC */
      table->lookup_count++;
      if (likely(flow->priority > 0)) {
        table->matched_count++;
      }
      rv = execute_instruction(pkt,
                               (const struct instruction **)flow->instruction);
      if (rv != LAGOPUS_RESULT_OK) {
        break;
      }
    }
  } else {
    rv = LAGOPUS_RESULT_NOT_FOUND;
  }
  return rv;
}

/**
 * match packet (no cache)
 */
static inline lagopus_result_t
dp_openflow_match(struct lagopus_packet *pkt) {
  struct flowdb *flowdb;
  struct flow *flow;
  struct table *table;
  lagopus_result_t rv;

  flowdb = pkt->bridge->flowdb;

  /* Get table from tabile_id. */
  table = table_lookup(flowdb, pkt->table_id);
  if (table == NULL) {
    /* table not found.  finish action. */
    return LAGOPUS_RESULT_STOP;
  }

  table->lookup_count++;
#ifdef USE_MBTREE
  flow = find_mbtree(pkt, table->flow_list);
#else
  flow = lagopus_find_flow(pkt, table);
#endif
  if (likely(flow != NULL && pkt->nmatched < LAGOPUS_DP_PIPELINE_MAX)) {
    DP_PRINT("MATCHED\n");
    /* execute_instruction is able to call this function recursively. */
    pkt->flow = flow;
    pkt->matched_flow[pkt->nmatched++] = flow;
    rv = LAGOPUS_RESULT_OK;
  } else {
    DP_PRINT("NOT MATCHED\n");
    /*
     * the behavior on a table miss depends on the table configuration.
     * 5.4 Table-miss says,
     * by default packets unmached are dropped (discarded).
     * A switch configuration, for example using the OpenFlow Configuration
     * Protocol, may override this default and specify another behaviour.
     */
    rv = LAGOPUS_RESULT_STOP;
  }
  return rv;
}

static inline lagopus_result_t
dp_openflow_do_action(struct lagopus_packet *pkt) {
  struct flow *flow;
  lagopus_result_t rv;

  flow = pkt->flow;
  rv = execute_instruction(pkt,
                           (const struct instruction **)flow->instruction);

  return rv;
}

static inline lagopus_result_t
dp_openflow_do_action_set(struct lagopus_packet *pkt) {
  lagopus_result_t rv;

  rv = LAGOPUS_RESULT_OK;
  /*
   * required: execute stored action-set.
   * execution order:
   * 1. copy TTL inwards
   * 2. pop
   * 3. push-MPLS
   * 4. push-PBB
   * 5. push-VLAN
   * 6. copy TTL
   * 7. decrement TTL
   * 8. set
   * 9. qos
   * 10. group
   * 11. output
   */
  if ((pkt->flags & PKT_FLAG_HAS_ACTION) != 0) {
    rv = execute_action_set(pkt, pkt->actions);
    pkt->flags &= (uint32_t)~PKT_FLAG_HAS_ACTION;
  }
  return rv;
}

/*
 * process received packet.
 */
lagopus_result_t
lagopus_match_and_action(struct lagopus_packet *pkt) {
  lagopus_result_t rv;

  rv = dp_openflow_do_cached_action(pkt);
  if (unlikely(rv == LAGOPUS_RESULT_NOT_FOUND)) {
    for (;;) {
      rv = dp_openflow_match(pkt);
      if (rv != LAGOPUS_RESULT_OK) {
        break;
      }
      rv = dp_openflow_do_action(pkt);
      if (rv <= LAGOPUS_RESULT_OK) {
        break;
      }
    }
  }
  if (rv == LAGOPUS_RESULT_OK) {
    rv = dp_openflow_do_action_set(pkt);
  }
  /* required: if no output action, drop packet. */
  if (rv != LAGOPUS_RESULT_NO_MORE_ACTION) {
    lagopus_packet_free(pkt);
  }
  return rv;
}

#ifdef HYBRID
/* for L3 routing */
/*
 * Decriment ttl.
 */
static lagopus_result_t
dec_ttl(struct lagopus_packet *pkt) {
  uint16_t miss_send_len;

  /* optional */
  if (pkt->ether_type == ETHERTYPE_IP) {
    if (likely(IPV4_TTL(pkt->ipv4) > 0)) {
      IPV4_TTL(pkt->ipv4)--;
      if (unlikely(IPV4_CSUM(pkt->ipv4) == 0xffff)) {
        pkt->flags |= PKT_FLAG_RECALC_IPV4_CKSUM;
      } else {
        IPV4_CSUM(pkt->ipv4)++;
      }
    }

    /* if invalid.  send packet_in with OFPR_INVALID_TTL to controller. */
    if (likely(IPV4_TTL(pkt->ipv4) == 0)) {
      if (pkt->in_port != NULL && pkt->in_port->bridge != NULL) {
        miss_send_len = pkt->in_port->bridge->switch_config.miss_send_len;
      } else {
        miss_send_len = 128;
      }
      return LAGOPUS_RESULT_STOP;
    }
  } else if (pkt->ether_type == ETHERTYPE_IPV6) {
    if (IPV6_HLIM(pkt->ipv6) > 0) {
      IPV6_HLIM(pkt->ipv6)--;
    }

    /* if invalid.  send packet_in with OFPR_INVALID_TTL to controller. */
    if (IPV6_HLIM(pkt->ipv6) == 0) {
      if (pkt->in_port != NULL && pkt->in_port->bridge != NULL) {
        miss_send_len = pkt->in_port->bridge->switch_config.miss_send_len;
      } else {
        miss_send_len = 128;
      }
      return LAGOPUS_RESULT_STOP;
    }
  }
  return LAGOPUS_RESULT_OK;
}

/*
 * Rewrite packet header to routing.
 */
lagopus_result_t
lagopus_rewrite_pkt_header(struct lagopus_packet *pkt,
                           uint8_t *src, uint8_t *dst) {
  lagopus_result_t rv;

  /* rewrite ether header */
  OS_MEMCPY(ETHER_SRC(pkt->eth), src, ETHER_ADDR_LEN);
  OS_MEMCPY(ETHER_DST(pkt->eth), dst, ETHER_ADDR_LEN);

  /* dec ttl */
  rv = dec_ttl(pkt);

  return rv;
}

uint32_t
lagopus_get_ethertype(struct lagopus_packet *pkt) {
  return pkt->ether_type;
}

/*
 * Get string of ip address.
 */
lagopus_result_t
lagopus_get_ip(struct lagopus_packet *pkt, void *dst, const int family) {
  if (family == AF_INET) {
    if (pkt && (pkt->ipv4)) {
      *((struct in_addr*)dst) = pkt->ipv4->ip_dst;
      return LAGOPUS_RESULT_OK;
    }
  } else if (family == AF_INET6) {
    /* TODO: */
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}
#endif /* HYBRID */

