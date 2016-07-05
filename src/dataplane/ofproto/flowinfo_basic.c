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
 *      @file   flowinfo_basic.c
 *      @brief  Optimized flow database for dataplane, linear search
 */

#include "lagopus_config.h"

#include <inttypes.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/queue.h>

#ifndef HAVE_DPDK
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#else
#include <net/if.h>
#include <net/if_ether.h>
#endif /* HAVE_NET_ETHERNET_H */
#endif /* HAVE_DPDK */

#define __FAVOR_BSD
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

#include <openflow.h>

#include "lagopus/ethertype.h"
#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
#include "pktbuf.h"
#include "packet.h"

#include "lagopus/flowinfo.h"

#if 1
#define STATIC
#else
#define STATIC static
#endif

#undef DEBUG
#ifdef DEBUG
#define DPRINT(...) printf(__VA_ARGS__)
#else
#define DPRINT(...)
#endif

STATIC bool match_basic(const struct lagopus_packet *, struct flow *);

static lagopus_result_t
add_flow_basic(struct flowinfo *, struct flow *);
static lagopus_result_t
del_flow_basic(struct flowinfo *, struct flow *);
static struct flow *
match_flow_basic(struct flowinfo *, struct lagopus_packet *, int32_t *);
static struct flow *
find_flow_basic(struct flowinfo *, struct flow *);
static void
destroy_flowinfo_basic(struct flowinfo *);

/**
 * make oxm_field + mask 8bit value from OpenFlow OFPXMT_OFB_* macros.
 */
#define FIELD(n) ((n) << 1)
#define FIELD_WITH_MASK(n) (((n) << 1) + 1)

STATIC bool
match_basic(const struct lagopus_packet *pkt, struct flow *flow) {
  struct byteoff_match *match;
  uint8_t *base;
  uint32_t bits;
  int off, i, max;

  if (unlikely(pkt->ether_type == ETHERTYPE_IPV6)) {
    max = MAX_BASE;
  } else {
    max = OOB2_BASE + 1;
  }
  for (i = 0; i < max; i++) {
    match = &flow->byteoff_match[i];
    if (match->bits == 0) {
      continue;
    }
    base = pkt->base[i];
    if (base == NULL) {
      DPRINT("byteoff not matched (index=%d, base is NULL)\n", i);
      return false;
    }
    off = 0;
    bits = match->bits;
    do {
      if ((bits & 0x0f) != 0) {
        uint32_t b, m, c;

        memcpy(&b, &base[off], sizeof(uint32_t));
        memcpy(&m, &match->masks[off], sizeof(uint32_t));
        memcpy(&c, &match->bytes[off], sizeof(uint32_t));
        if ((b & m) != c) {
          DPRINT("pkt 0x%04x, mask 0x%04x, flow 0x%04x\n", b, m, c);
          DPRINT("byteoff not matched (index=%d, off=%d)\n", i, off);
          return false;
        }
      }
      off += 4;
      bits >>= 4;
    } while (bits != 0);
  }
  DPRINT("byteoff matched\n");

  if ((flow->flags & OFPFF_NO_PKT_COUNTS) == 0) {
    flow->packet_count++;
  }
  if ((flow->flags & OFPFF_NO_BYT_COUNTS) == 0) {
    flow->byte_count += OS_M_PKTLEN(PKT2MBUF(pkt));
  }

  return true;
}


struct flowinfo *
new_flowinfo_basic(void) {
  struct flowinfo *self;

  self = calloc(1, sizeof(struct flowinfo));
  if (self != NULL) {
    self->flows = malloc(1);
    self->add_func = add_flow_basic;
    self->del_func = del_flow_basic;
    self->match_func = match_flow_basic;
    self->find_func = find_flow_basic;
    self->destroy_func = destroy_flowinfo_basic;
  }
  return self;
}

static void
destroy_flowinfo_basic(struct flowinfo *self) {
  free(self->flows);
  free(self);
}

#define BYTEBITS(bytes, off) ((uint64_t)((1 << (bytes)) - 1) << (off))
#define BYTEPTR(base, off) (&byteoff[base].bytes[off])
#define MASKPTR(base, off) (&byteoff[base].masks[off])

#define MAKE_BYTEOFF(field, off, base)                                \
  case FIELD(field):                                                  \
  byteoff[base].bits |= (uint32_t)BYTEBITS(match->oxm_length, off);           \
  memcpy(BYTEPTR(base, off), match->oxm_value, match->oxm_length);  \
  memset(MASKPTR(base, off), 0xff, match->oxm_length);              \
  break;

#define MAKE_BYTEOFF_W(field, off, base)                        \
  case FIELD_WITH_MASK(field):                                  \
  {                                                           \
    size_t len = match->oxm_length >> 1;                      \
    byteoff[base].bits |= (uint32_t)BYTEBITS(len, off);               \
    memcpy(BYTEPTR(base, off), match->oxm_value, len);        \
    memcpy(MASKPTR(base, off), match->oxm_value + len, len);  \
  }                                                           \
  break;

#define MAKE_BYTE(field, type, member, base)                    \
  MAKE_BYTEOFF(field, offsetof(struct type, member), base)

#define MAKE_BYTE_W(field, type, member, base)                  \
  MAKE_BYTEOFF_W(field, offsetof(struct type, member), base)

/* analyze flow entry and make byte offset match. */
void
flow_make_match(struct flow *flow) {
  struct byteoff_match *byteoff;
  struct match *match;
  uint16_t l3_ether_type = 0;

  byteoff = flow->byteoff_match;

  TAILQ_FOREACH(match, &flow->match_list, entry) {
    switch (match->oxm_field) {
        /* VLAN_VID, VLAN_VID_W, VLAN_PCP and ETH_TYPE as metadata */
      case FIELD(OFPXMT_OFB_ETH_TYPE):
        memcpy(&l3_ether_type, match->oxm_value, match->oxm_length);
        l3_ether_type = ntohs(l3_ether_type);
        break;

      case FIELD(OFPXMT_OFB_VLAN_VID): {
        uint8_t val8[2];
        const int off = offsetof(struct oob_data, vlan_tci);

        byteoff[OOB_BASE].bits |= (uint32_t)BYTEBITS(2, off);
        memcpy(val8, match->oxm_value, 2);
        BYTEPTR(OOB_BASE, off)[0] &= 0xe0;
        BYTEPTR(OOB_BASE, off)[0] |= val8[0] | 0x10;
        BYTEPTR(OOB_BASE, off)[1] = val8[1];
        MASKPTR(OOB_BASE, off)[0] |= 0x1f;
        MASKPTR(OOB_BASE, off)[1] = 0xff;
      }
      break;

      case FIELD_WITH_MASK(OFPXMT_OFB_VLAN_VID): {
        uint8_t val8[4];
        const int off = offsetof(struct oob_data, vlan_tci);

        byteoff[OOB_BASE].bits |= (uint32_t)BYTEBITS(2, off);
        memcpy(val8, match->oxm_value, 4);
        BYTEPTR(OOB_BASE, off)[0] &= 0xe0;
        BYTEPTR(OOB_BASE, off)[0] |= val8[0] | (0x10 & val8[2]);
        BYTEPTR(OOB_BASE, off)[1] = val8[1];
        MASKPTR(OOB_BASE, off)[0] &= 0xe0;
        MASKPTR(OOB_BASE, off)[0] |= val8[2];
        MASKPTR(OOB_BASE, off)[1] = val8[3];
      }
      break;

      case FIELD(OFPXMT_OFB_VLAN_PCP): {
        const int off = offsetof(struct oob_data, vlan_tci);

        byteoff[OOB_BASE].bits |= (uint32_t)BYTEBITS(1, off);
        BYTEPTR(OOB_BASE, off)[0] &= 0x1f;
        BYTEPTR(OOB_BASE, off)[0] |= (uint8_t)(match->oxm_value[0] << 5);
        MASKPTR(OOB_BASE, off)[0] |= 0xe0;
      }
      break;

      /* IN_PORT, IN_PHY_PORT, METADATA and METADATA_W as metadata */
      MAKE_BYTE(OFPXMT_OFB_IN_PORT, oob_data, in_port, OOB_BASE);
      MAKE_BYTE(OFPXMT_OFB_IN_PHY_PORT, oob_data, in_phy_port, OOB_BASE);
      MAKE_BYTE(OFPXMT_OFB_METADATA, oob_data, metadata, OOB_BASE);
      MAKE_BYTE_W(OFPXMT_OFB_METADATA, oob_data, metadata, OOB_BASE);

      MAKE_BYTE(OFPXMT_OFB_ETH_DST, ether_header, ether_dhost, ETH_BASE);
      MAKE_BYTE_W(OFPXMT_OFB_ETH_DST, ether_header, ether_dhost, ETH_BASE);
      MAKE_BYTE(OFPXMT_OFB_ETH_SRC, ether_header, ether_shost, ETH_BASE);
      MAKE_BYTE_W(OFPXMT_OFB_ETH_SRC, ether_header, ether_shost, ETH_BASE);

      MAKE_BYTEOFF(OFPXMT_OFB_IP_PROTO, 0, IPPROTO_BASE);

      MAKE_BYTE(OFPXMT_OFB_IPV4_SRC, ip, ip_src, L3_BASE);
      MAKE_BYTE_W(OFPXMT_OFB_IPV4_SRC, ip, ip_src, L3_BASE);
      MAKE_BYTE(OFPXMT_OFB_IPV4_DST, ip, ip_dst, L3_BASE);
      MAKE_BYTE_W(OFPXMT_OFB_IPV4_DST, ip, ip_dst, L3_BASE);

      MAKE_BYTE(OFPXMT_OFB_ARP_OP, ether_arp, arp_op, L3_BASE);
      MAKE_BYTE(OFPXMT_OFB_ARP_SHA, ether_arp, arp_sha, L3_BASE);
      MAKE_BYTE_W(OFPXMT_OFB_ARP_SHA, ether_arp, arp_sha, L3_BASE);
      MAKE_BYTE(OFPXMT_OFB_ARP_SPA, ether_arp, arp_spa, L3_BASE);
      MAKE_BYTE_W(OFPXMT_OFB_ARP_SPA, ether_arp, arp_spa, L3_BASE);
      MAKE_BYTE(OFPXMT_OFB_ARP_THA, ether_arp, arp_tha, L3_BASE);
      MAKE_BYTE_W(OFPXMT_OFB_ARP_THA, ether_arp, arp_tha, L3_BASE);
      MAKE_BYTE(OFPXMT_OFB_ARP_TPA, ether_arp, arp_tpa, L3_BASE);
      MAKE_BYTE_W(OFPXMT_OFB_ARP_TPA, ether_arp, arp_tpa, L3_BASE);

      MAKE_BYTEOFF(OFPXMT_OFB_IPV6_SRC, 0, V6SRC_BASE);
      MAKE_BYTEOFF_W(OFPXMT_OFB_IPV6_SRC, 0, V6SRC_BASE);
      MAKE_BYTEOFF(OFPXMT_OFB_IPV6_DST, 0, V6DST_BASE);
      MAKE_BYTEOFF_W(OFPXMT_OFB_IPV6_DST, 0, V6DST_BASE);

      MAKE_BYTE(OFPXMT_OFB_TCP_SRC, tcphdr, th_sport, L4_BASE);
      MAKE_BYTE(OFPXMT_OFB_TCP_DST, tcphdr, th_dport, L4_BASE);

      MAKE_BYTE(OFPXMT_OFB_UDP_SRC, udphdr, uh_sport, L4_BASE);
      MAKE_BYTE(OFPXMT_OFB_UDP_DST, udphdr, uh_dport, L4_BASE);

#if 1
      MAKE_BYTE(OFPXMT_OFB_SCTP_SRC, tcphdr, th_sport, L4_BASE);
      MAKE_BYTE(OFPXMT_OFB_SCTP_DST, tcphdr, th_dport, L4_BASE);
#else
      MAKE_BYTE(OFPXMT_OFB_SCTP_SRC, sctp, sctp_sport, L4_BASE);
      MAKE_BYTE(OFPXMT_OFB_SCTP_DST, sctp, sctp_dport, L4_BASE);
#endif

      MAKE_BYTE(OFPXMT_OFB_ICMPV4_TYPE, icmp, icmp_type, L4_BASE);
      MAKE_BYTE(OFPXMT_OFB_ICMPV4_CODE, icmp, icmp_code, L4_BASE);

      MAKE_BYTE(OFPXMT_OFB_ICMPV6_TYPE, icmp6_hdr, icmp6_type, L4_BASE);
      MAKE_BYTE(OFPXMT_OFB_ICMPV6_CODE, icmp6_hdr, icmp6_code, L4_BASE);

      MAKE_BYTE(OFPXMT_OFB_IPV6_ND_TARGET,
                nd_neighbor_solicit, nd_ns_target, L4_BASE);
      MAKE_BYTEOFF(OFPXMT_OFB_IPV6_ND_SLL, 2, NDSLL_BASE);
      MAKE_BYTEOFF(OFPXMT_OFB_IPV6_ND_TLL, 2, NDTLL_BASE);

      MAKE_BYTE(OFPXMT_OFB_PBB_ISID, pbb_hdr, i_sid, PBB_BASE);
      MAKE_BYTE_W(OFPXMT_OFB_PBB_ISID, pbb_hdr, i_sid, PBB_BASE);

      /* TUNNEL_ID and IPV6_EXTHDR as metadata */
      MAKE_BYTE(OFPXMT_OFB_TUNNEL_ID, oob2_data, tunnel_id, OOB2_BASE);
      MAKE_BYTE_W(OFPXMT_OFB_TUNNEL_ID, oob2_data, tunnel_id, OOB2_BASE);
      MAKE_BYTE(OFPXMT_OFB_IPV6_EXTHDR, oob2_data, ipv6_exthdr, OOB2_BASE);
      MAKE_BYTE_W(OFPXMT_OFB_IPV6_EXTHDR, oob2_data, ipv6_exthdr, OOB2_BASE);

      case FIELD(OFPXMT_OFB_IP_DSCP):
        /* 6bit */
        if (l3_ether_type == ETHERTYPE_IPV6) {
          uint8_t val;
          const int off = 0;

          byteoff[L3_BASE].bits |= (uint32_t)BYTEBITS(2, off);
          memcpy(&val, match->oxm_value, 1);
          BYTEPTR(L3_BASE, off)[0] &= 0xf0;
          BYTEPTR(L3_BASE, off)[0] |= val >> 2;
          BYTEPTR(L3_BASE, off)[1] &= 0x3f;
          BYTEPTR(L3_BASE, off)[1] |= (uint8_t)(val << 6);
          MASKPTR(L3_BASE, off)[0] |= 0x0f;
          MASKPTR(L3_BASE, off)[1] |= 0xc0;
        } else {
          uint8_t val;
          const int off = offsetof(struct ip, ip_tos);

          byteoff[L3_BASE].bits |= (uint32_t)BYTEBITS(1, off);
          memcpy(&val, match->oxm_value, 1);
          BYTEPTR(L3_BASE, off)[0] &= 0x03;
          BYTEPTR(L3_BASE, off)[0] |= (uint8_t)(val << 2);
          MASKPTR(L3_BASE, off)[0] |= 0xfc;
        }
        break;
      case FIELD(OFPXMT_OFB_IP_ECN):
        /* 2bit */
        if (l3_ether_type == ETHERTYPE_IPV6) {
          uint8_t val;
          const int off = 1;

          byteoff[L3_BASE].bits |= (uint32_t)BYTEBITS(1, off);
          memcpy(&val, match->oxm_value, 1);
          BYTEPTR(L3_BASE, off)[0] &= 0xcf;
          BYTEPTR(L3_BASE, off)[0] |= (uint8_t)(val << 4);
          MASKPTR(L3_BASE, off)[0] |= 0x30;
        } else {
          uint8_t val;
          const int off = offsetof(struct ip, ip_tos);

          byteoff[L3_BASE].bits |= (uint32_t)BYTEBITS(1, off);
          memcpy(&val, match->oxm_value, 1);
          BYTEPTR(L3_BASE, off)[0] &= 0xfc;
          BYTEPTR(L3_BASE, off)[0] |= val;
          MASKPTR(L3_BASE, off)[0] |= 0x03;
        }
        break;

      case FIELD(OFPXMT_OFB_IPV6_FLABEL):
        /* 20bit */
      {
        uint8_t val8[4];
        const int off = 1;

        byteoff[L3_BASE].bits |= (uint32_t)BYTEBITS(3, off);
        memcpy(val8, match->oxm_value, match->oxm_length);
        BYTEPTR(L3_BASE, off)[0] &= 0xf0;
        BYTEPTR(L3_BASE, off)[0] |= val8[1];
        BYTEPTR(L3_BASE, off)[1] = val8[2];
        BYTEPTR(L3_BASE, off)[2] = val8[3];
        MASKPTR(L3_BASE, off)[0] = 0x0f;
        MASKPTR(L3_BASE, off)[1] = 0xff;
        MASKPTR(L3_BASE, off)[2] = 0xff;
      }
      break;

      case FIELD_WITH_MASK(OFPXMT_OFB_IPV6_FLABEL):
        /* 20bit */
      {
        uint8_t val8[4];
        const int off = 1;

        byteoff[L3_BASE].bits |= (uint32_t)BYTEBITS(3, off);
        memcpy(val8, match->oxm_value, 4);
        BYTEPTR(L3_BASE, off)[0] &= 0xf0;
        BYTEPTR(L3_BASE, off)[0] |= val8[1];
        BYTEPTR(L3_BASE, off)[1] = val8[2];
        BYTEPTR(L3_BASE, off)[2] = val8[3];
        memcpy(val8, match->oxm_value + 4, 4);
        MASKPTR(L3_BASE, off)[0] &= 0xf0;
        MASKPTR(L3_BASE, off)[0] |= val8[1];
        MASKPTR(L3_BASE, off)[1] = val8[2];
        MASKPTR(L3_BASE, off)[2] = val8[3];
      }
      break;

      case FIELD(OFPXMT_OFB_MPLS_LABEL):
        /* 20bit */
      {
        union {
          uint32_t val32;
          uint8_t val8[4];
        } val;
        const int off = 0;

        byteoff[MPLS_BASE].bits |= (uint32_t)BYTEBITS(3, off);
        memcpy(&val, match->oxm_value, 4);
        val.val32 = ntohl(val.val32) << 12;
        val.val32 = htonl(val.val32);
        BYTEPTR(MPLS_BASE, off)[0] = val.val8[0];
        BYTEPTR(MPLS_BASE, off)[1] = val.val8[1];
        BYTEPTR(MPLS_BASE, off)[2] &= 0x0f;
        BYTEPTR(MPLS_BASE, off)[2] |= val.val8[2];
        MASKPTR(MPLS_BASE, off)[0] = 0xff;
        MASKPTR(MPLS_BASE, off)[1] = 0xff;
        MASKPTR(MPLS_BASE, off)[2] = 0xf0;
      }
      break;

      case FIELD(OFPXMT_OFB_MPLS_TC):
        /* 3bit */
      {
        uint8_t val;
        const int off = 2;

        byteoff[MPLS_BASE].bits |= (uint32_t)BYTEBITS(1, off);
        memcpy(&val, match->oxm_value, 1);
        BYTEPTR(MPLS_BASE, off)[0] &= 0xf1;
        BYTEPTR(MPLS_BASE, off)[0] |= (uint8_t)(val << 1);
        MASKPTR(MPLS_BASE, off)[0] |= 0x0e;
      }
      break;

      case FIELD(OFPXMT_OFB_MPLS_BOS):
        /* 1bit */
      {
        uint8_t val;
        const int off = 2;

        byteoff[MPLS_BASE].bits |= (uint32_t)BYTEBITS(1, off);
        memcpy(&val, match->oxm_value, 1);
        BYTEPTR(MPLS_BASE, off)[0] &= 0xfe;
        BYTEPTR(MPLS_BASE, off)[0] |= val;
        MASKPTR(MPLS_BASE, off)[0] |= 0x01;
      }
      break;

#ifdef PBB_UCA_SUPPORT
      case FIELD(OFPXMT_OFB_PBB_UCA):
        /* 1bit */
      {
        uint8_t val;
        const int off = offsetof(struct pbb_hdr, i_pcp_dei);

        byteoff[PBB_BASE].bits |= (uint32_t)BYTEBITS(1, off);
        memcpy(&val, match->oxm_value, 1);
        BYTEPTR(PBB_BASE, off)[0] &= 0xf7;
        BYTEPTR(PBB_BASE, off)[0] |= val << 3;
        MASKPTR(PBB_BASE, off)[0] |= 0x08;
      }
      break;
#endif /* PBB_UCA_SUPPORT */
#ifdef GENERAL_TUNNEL_SUPPORT
      MAKE_BYTE(OFPXMT_OFB_PACKET_TYPE, oob_data, packet_type, OOB_BASE);
      MAKE_BYTE(OFPXMT_OFB_VXLAN_FLAGS, vxlanhdr, flags, L4P_BASE);
      MAKE_BYTE_W(OFPXMT_OFB_VXLAN_FLAGS, vxlanhdr, flags, L4P_BASE);

      case FIELD(OFPXMT_OFB_VXLAN_VNI):
        /* 24bit */
      {
        const int off = offsetof(struct vxlanhdr, vni);

        byteoff[L4P_BASE].bits |= (uint32_t)BYTEBITS(3, off);
        memcpy(BYTEPTR(MPLS_BASE, off), match->oxm_value, 3);
        MASKPTR(L4P_BASE, off)[0] = 0xff;
        MASKPTR(L4P_BASE, off)[1] = 0xff;
        MASKPTR(L4P_BASE, off)[2] = 0xff;
      }
      break;

      case FIELD(OFPXMT_OFB_GRE_FLAGS):
        /* 13bit */
      {
        uint16_t val;
        const int off = offsetof(struct gre_hdr, flags);

        byteoff[L4_BASE].bits |= (uint32_t)BYTEBITS(2, off);
        memcpy(&val, match->oxm_value, 2);
        BYTEPTR(L4_BASE, off)[0] = val >> 5;
        MASKPTR(L4_BASE, off)[0] = 0xff;
        BYTEPTR(L4_BASE, off)[1] &= 0x07;
        BYTEPTR(L4_BASE, off)[1] |= (val << 3) & 0xff;
        MASKPTR(L4_BASE, off)[1] |= 0xf8;
      }
        break;

      case FIELD_WITH_MASK(OFPXMT_OFB_GRE_FLAGS):
        /* 13bit */
      {
        uint16_t val;
        uint16_t mask;
        const int off = offsetof(struct gre_hdr, flags);

        memcpy(&val, match->oxm_value, 2);
        memcpy(&mask, match->oxm_value + 2, 2);
        byteoff[L4_BASE].bits |= (uint32_t)BYTEBITS(2, off);
        BYTEPTR(L4_BASE, off)[0] |= val >> 5;
        BYTEPTR(L4_BASE, off)[1] |= (val << 3) & 0xff;
        MASKPTR(L4_BASE, off)[0] |= mask >> 5;
        MASKPTR(L4_BASE, off)[1] |= (mask << 3) & 0xff;
      }
        break;

      case FIELD(OFPXMT_OFB_GRE_VER):
        /* 3bit, lower of flags */
      {
        uint8_t val;
        const int off = offsetof(struct gre_hdr, flags);

        byteoff[L4_BASE].bits |= (uint32_t)BYTEBITS(2, off);
        memcpy(&val, match->oxm_value, 1);
        BYTEPTR(L4_BASE, off)[1] &= 0xf8;
        BYTEPTR(L4_BASE, off)[1] |= val;
        MASKPTR(L4_BASE, off)[1] |= 0x07;
      }
        break;

      MAKE_BYTE(OFPXMT_OFB_GRE_PROTOCOL, gre_hdr, ptype, L4_BASE);
      MAKE_BYTE(OFPXMT_OFB_GRE_KEY, gre_hdr, key, L4_BASE);
      MAKE_BYTE_W(OFPXMT_OFB_GRE_KEY, gre_hdr, key, L4_BASE);
#if 0
      MAKE_BYTE(OFPXMT_OFB_GRE_SEQNUM, gre_hdr, seq_num, L4_BASE);
#endif

      case FIELD(OFPXMT_OFB_LISP_FLAGS):
        break;

      case FIELD(OFPXMT_OFB_LISP_NONCE):
        break;

      case FIELD(OFPXMT_OFB_LISP_ID):
        break;

      case FIELD(OFPXMT_OFB_MPLS_DATA_FIRST_NIBBLE):
        break;

      case FIELD(OFPXMT_OFB_MPLS_ACH_VERSION):
        break;

      case FIELD(OFPXMT_OFB_MPLS_ACH_CHANNEL):
        break;

      case FIELD(OFPXMT_OFB_MPLS_PW_METADATA):
        break;

      case FIELD(OFPXMT_OFB_MPLS_CW_FLAGS):
        break;

      case FIELD(OFPXMT_OFB_MPLS_CW_FRAG):
        break;

      case FIELD(OFPXMT_OFB_MPLS_CW_LEN):
        break;

      case FIELD(OFPXMT_OFB_MPLS_CW_SEQ_NUM):
        break;

      case FIELD(OFPXMT_OFB_GTPU_FLAGS):
        /* 5bit, lower of verflags */
        {
          uint8_t val;
          const int off = offsetof(struct gtpu_hdr, verflags);

          byteoff[L4P_BASE].bits |= (uint32_t)BYTEBITS(1, off);
          memcpy(&val, match->oxm_value, 1);
          BYTEPTR(L4P_BASE, off)[1] &= 0xe0;
          BYTEPTR(L4P_BASE, off)[1] |= val;
          MASKPTR(L4P_BASE, off)[1] |= 0x1f;
        }
        break;

      case FIELD(OFPXMT_OFB_GTPU_VER):
        /* 3bit, highter of verflags */
        {
          uint8_t val;
          const int off = offsetof(struct gtpu_hdr, verflags);

          byteoff[L4P_BASE].bits |= (uint32_t)BYTEBITS(1, off);
          memcpy(&val, match->oxm_value, 1);
          BYTEPTR(L4P_BASE, off)[1] &= 0x1f;
          BYTEPTR(L4P_BASE, off)[1] |= val << 5;
          MASKPTR(L4P_BASE, off)[1] |= 0xe0;
        }
        break;

        MAKE_BYTE(OFPXMT_OFB_GTPU_MSGTYPE, gtpu_hdr, msgtype, L4P_BASE);
        MAKE_BYTE(OFPXMT_OFB_GTPU_TEID, gtpu_hdr, te_id, L4P_BASE);
        /* EXTN_HDR, EXTN_HDP_PORT, EXTN_SCI is not supported yet */
#endif /* GENERAL_TUNNEL_SUPPORT */

      default:
        break;
    }
  }
}

static lagopus_result_t
add_flow_basic(struct flowinfo *self, struct flow *flow) {
  int i, st, ed, off;

  flow_make_match(flow);

  self->flows = realloc(self->flows, (size_t)(self->nflow + 1) * sizeof(flow));
  if (self->flows == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  st = 0;
  ed = self->nflow;
  while (st < ed) {
    off = st + (ed - st) / 2;
    if (self->flows[off]->priority >= flow->priority) {
      st = off + 1;
    } else {
      ed = off;
    }
  }
  i = ed;
  if (i < self->nflow) {
    memmove(&self->flows[i + 1], &self->flows[i],
            sizeof(struct flow *) * (size_t)(self->nflow - i));
  }
  self->flows[i] = flow;
  self->nflow++;
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
del_flow_basic(struct flowinfo *self, struct flow *flow) {
  int i;

  for (i = 0; i < self->nflow; i++) {
    if (self->flows[i] == flow) {
      memmove(&self->flows[i], &self->flows[i + 1],
              sizeof(struct flow *) * (size_t)(self->nflow - i));
      self->nflow--;
      return LAGOPUS_RESULT_OK;
    }
  }
  return LAGOPUS_RESULT_NOT_FOUND;
}

static struct flow *
match_flow_basic(struct flowinfo *self, struct lagopus_packet *pkt,
                 int32_t *pri) {
  struct flow *matched;
  int32_t prio;
  int i;

  prio = *pri;
  matched = NULL;

  for (i = 0; i < self->nflow; i++) {
    struct flow *flow = self->flows[i];

    if (prio >= flow->priority) {
      break;
    }
    if (match_basic(pkt, flow) == true) {
      matched = flow;
      *pri = flow->priority;
      break;
    }
  }
  return matched;
}

static bool
flow_compare(struct flow *f1, struct flow *f2) {
  struct match *m1;
  struct match *m2;

  /* Priority compare. */
  if (f1->priority != f2->priority) {
    return false;
  }

  /* Field bits compare. */
  if (f1->field_bits != f2->field_bits) {
    return false;
  }

  m1 = TAILQ_FIRST(&f1->match_list);
  m2 = TAILQ_FIRST(&f2->match_list);

  while (m1 && m2) {
    if (m1->oxm_class != m2->oxm_class ||
        m1->oxm_field != m2->oxm_field ||
        m1->oxm_length != m2->oxm_length ||
        memcmp(m1->oxm_value, m2->oxm_value, m1->oxm_length) != 0) {
      return false;
    }

    m1 = TAILQ_NEXT(m1, entry);
    m2 = TAILQ_NEXT(m2, entry);
  }

  if (m1 != m2) {
    return false;
  }

  return true;
}

static struct flow *
find_flow_basic(struct flowinfo *self, struct flow *flow) {
  int i;

  for (i = 0; i < self->nflow; i++) {
    if (flow_compare(flow, self->flows[i]) == true) {
      return self->flows[i];
    }
  }
  return NULL;
}
