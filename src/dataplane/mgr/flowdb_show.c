/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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


#include <stdio.h>
#include <stdint.h>

#include "confsys.h"
#include "lagopus/ethertype.h"
#include "lagopus/dpmgr.h"
#include "lagopus/ptree.h"

/* We must have better way to access to dpmgr information. */
extern struct dpmgr *dpmgr;

static void
show_flow_list(struct confsys *, struct flow_list *);

static void
show_match(struct confsys *confsys, struct match *match) {
  uint16_t val16;
  uint32_t val32;
  uint64_t val64;
  char ip_str[INET6_ADDRSTRLEN];
  int field_type = match->oxm_field >> 1;
  int hasmask = match->oxm_field & 1;

  switch (field_type) {
    case OFPXMT_OFB_IN_PORT:
      val32 = *((uint32_t *)match->oxm_value);
      show(confsys, ",in_port=%d", ntohl(val32));
      break;
    case OFPXMT_OFB_IN_PHY_PORT:
      val32 = *((uint32_t *)match->oxm_value);
      show(confsys, ",in_phy_port=%d", ntohl(val32));
      break;
    case OFPXMT_OFB_METADATA:
      val64 = *(uint32_t *)match->oxm_value;
      val32 = *(uint32_t *)&match->oxm_value[4];;
      val64 = (ntohl(val64) << 32) | ntohl(val32);
      show(confsys, ",metadata=%d", val64);
      if (hasmask) {
        show(confsys, "/:0x%02x%02x%02x%02x%02x%02x%02x%02x",
             match->oxm_value[8], match->oxm_value[9],
             match->oxm_value[10], match->oxm_value[11],
             match->oxm_value[12], match->oxm_value[13],
             match->oxm_value[14], match->oxm_value[15]);
      }
      break;
    case OFPXMT_OFB_ETH_DST:
      show(confsys, ",eth_dst=%02x:%02x:%02x:%02x:%02x:%02x",
           match->oxm_value[0], match->oxm_value[1],
           match->oxm_value[2], match->oxm_value[3],
           match->oxm_value[4], match->oxm_value[5]);
      if (hasmask) {
        show(confsys, "/0x%02x%02x%02x%02x%02x%02x",
             match->oxm_value[6], match->oxm_value[7],
             match->oxm_value[8], match->oxm_value[9],
             match->oxm_value[10], match->oxm_value[11]);
      }
      break;
    case OFPXMT_OFB_ETH_SRC:
      show(confsys, ",eth_src=%02x:%02x:%02x:%02x:%02x:%02x",
           match->oxm_value[0], match->oxm_value[1],
           match->oxm_value[2], match->oxm_value[3],
           match->oxm_value[4], match->oxm_value[5]);
      if (hasmask) {
        show(confsys, "/0x%02x%02x%02x%02x%02x%02x",
             match->oxm_value[6], match->oxm_value[7],
             match->oxm_value[8], match->oxm_value[9],
             match->oxm_value[10], match->oxm_value[11]);
      }
      break;
    case OFPXMT_OFB_ETH_TYPE:
      switch ((match->oxm_value[0] << 8) |  match->oxm_value[1]) {
        case ETHERTYPE_ARP:
          show(confsys, ",arp");
          break;
        case ETHERTYPE_IP:
          show(confsys, ",ip");
          break;
        case ETHERTYPE_IPV6:
          show(confsys, ",ipv6");
          break;
        case ETHERTYPE_MPLS:
          show(confsys, ",mpls");
          break;
        case ETHERTYPE_MPLS_MCAST:
          show(confsys, ",mplsmc");
          break;
        case ETHERTYPE_PBB:
          show(confsys, ",pbb");
          break;
        default:
          show(confsys, ",eth_type=0x%02x%02x",
               match->oxm_value[0], match->oxm_value[1]);
          break;
      }
      break;
    case OFPXMT_OFB_VLAN_VID:
      val16 = *((uint16_t *)match->oxm_value);
      show(confsys, ",vlan_vid=%d", ntohs(val16));
      if (hasmask) {
        val16 = *((uint16_t *)&match->oxm_value[2]);
        show(confsys, "/0x%02x", ntohs(val16));
      }
      break;
    case OFPXMT_OFB_VLAN_PCP:
      show(confsys, ",vlan_pcp=%d", *match->oxm_value);
      break;
    case OFPXMT_OFB_IP_DSCP:
      show(confsys, ",ip_dscp=%d", *match->oxm_value);
      break;
    case OFPXMT_OFB_IP_ECN:
      show(confsys, ",ip_ecn=%d", *match->oxm_value);
      break;
    case OFPXMT_OFB_IP_PROTO:
      show(confsys, ",ip_proto=%d", *match->oxm_value);
      break;
    case OFPXMT_OFB_IPV4_SRC:
      show(confsys, ",ipv4_src=%s",
           inet_ntop(AF_INET, match->oxm_value, ip_str, sizeof(ip_str)));
      if (hasmask) {
        val32 = *((uint32_t *)&match->oxm_value[4]);
        show(confsys, "/0x%04x", ntohl(val32));
      }
      break;
    case OFPXMT_OFB_IPV4_DST:
      show(confsys, ",ipv4_dst=%s",
           inet_ntop(AF_INET, match->oxm_value, ip_str, sizeof(ip_str)));
      if (hasmask) {
        val32 = *((uint32_t *)&match->oxm_value[4]);
        show(confsys, "/0x%04x", ntohl(val32));
      }
      break;
    case OFPXMT_OFB_TCP_SRC:
      val16 = *((uint16_t *)match->oxm_value);
      show(confsys, ",tcp_src=%d", ntohs(val16));
      break;
    case OFPXMT_OFB_TCP_DST:
      val16 = *((uint16_t *)match->oxm_value);
      show(confsys, ",tcp_dst=%d", ntohs(val16));
      break;
    case OFPXMT_OFB_UDP_SRC:
      val16 = *((uint16_t *)match->oxm_value);
      show(confsys, ",udp_src=%d", ntohs(val16));
      break;
    case OFPXMT_OFB_UDP_DST:
      val16 = *((uint16_t *)match->oxm_value);
      show(confsys, ",udp_dst=%d", ntohs(val16));
      break;
    case OFPXMT_OFB_SCTP_SRC:
      val16 = *((uint16_t *)match->oxm_value);
      show(confsys, ",sctp_src=%d", ntohs(val16));
      break;
    case OFPXMT_OFB_SCTP_DST:
      val16 = *((uint16_t *)match->oxm_value);
      show(confsys, ",sctp_dst=%d", ntohs(val16));
      break;
    case OFPXMT_OFB_ICMPV4_TYPE:
      show(confsys, ",icmpv4_type=%d", *match->oxm_value);
      break;
    case OFPXMT_OFB_ICMPV4_CODE:
      show(confsys, ",icmpv4_code=%d", *match->oxm_value);
      break;
    case OFPXMT_OFB_ARP_OP:
      val16 = *((uint16_t *)match->oxm_value);
      show(confsys, ",arp_op=%d", ntohs(val16));
      break;
    case OFPXMT_OFB_ARP_SPA:
      show(confsys, ",arp_spa=%s",
           inet_ntop(AF_INET, match->oxm_value, ip_str, sizeof(ip_str)));
      if (hasmask) {
        val32 = *((uint32_t *)&match->oxm_value[4]);
        show(confsys, "/0x%04x", ntohl(val32));
      }
      break;
    case OFPXMT_OFB_ARP_TPA:
      show(confsys, ",arp_tpa=%s",
           inet_ntop(AF_INET, match->oxm_value, ip_str, sizeof(ip_str)));
      if (hasmask) {
        val32 = *((uint32_t *)&match->oxm_value[4]);
        show(confsys, "/0x%04x", ntohl(val32));
      }
      break;
    case OFPXMT_OFB_ARP_SHA:
      show(confsys, ",arp_sha=%02x:%02x:%02x:%02x:%02x:%02x",
           match->oxm_value[0], match->oxm_value[1],
           match->oxm_value[2], match->oxm_value[3],
           match->oxm_value[4], match->oxm_value[5]);
      if (hasmask) {
        show(confsys, "/0x%02x%02x%02x%02x%02x%02x",
             match->oxm_value[6], match->oxm_value[7],
             match->oxm_value[8], match->oxm_value[9],
             match->oxm_value[10], match->oxm_value[11]);
      }
      break;
    case OFPXMT_OFB_ARP_THA:
      show(confsys, ",arp_tha=%02x:%02x:%02x:%02x:%02x:%02x",
           match->oxm_value[0], match->oxm_value[1],
           match->oxm_value[2], match->oxm_value[3],
           match->oxm_value[4], match->oxm_value[5]);
      if (hasmask) {
        show(confsys, "/0x%02x%02x%02x%02x%02x%02x",
             match->oxm_value[6], match->oxm_value[7],
             match->oxm_value[8], match->oxm_value[9],
             match->oxm_value[10], match->oxm_value[11]);
      }
      break;
    case OFPXMT_OFB_IPV6_SRC:
      show(confsys, ",ipv6_src=%s",
           inet_ntop(AF_INET6, match->oxm_value, ip_str, sizeof(ip_str)));
      if (hasmask) {
        val32 = *(uint32_t *)&match->oxm_value[16];
        show(confsys, "/0x%04x", ntohl(val32));
        val32 = *(uint32_t *)&match->oxm_value[20];
        show(confsys, "0x%04x", ntohl(val32));
        val32 = *(uint32_t *)&match->oxm_value[24];
        show(confsys, "0x%04x", ntohl(val32));
        val32 = *(uint32_t *)&match->oxm_value[28];
        show(confsys, "0x%04x", ntohl(val32));
      }
      break;
    case OFPXMT_OFB_IPV6_DST:
      show(confsys, ",ipv6_dst=%s",
           inet_ntop(AF_INET6, match->oxm_value, ip_str, sizeof(ip_str)));
      if (hasmask) {
        val32 = *(uint32_t *)&match->oxm_value[16];
        show(confsys, "/0x%04x", ntohl(val32));
        val32 = *(uint32_t *)&match->oxm_value[20];
        show(confsys, "0x%04x", ntohl(val32));
        val32 = *(uint32_t *)&match->oxm_value[24];
        show(confsys, "0x%04x", ntohl(val32));
        val32 = *(uint32_t *)&match->oxm_value[28];
        show(confsys, "0x%04x", ntohl(val32));
      }
      break;
    case OFPXMT_OFB_IPV6_FLABEL:
      val32 = *((uint32_t *)match->oxm_value);
      show(confsys, ",ipv6_flabel=%d", ntohl(val32));
      break;
    case OFPXMT_OFB_ICMPV6_TYPE:
      show(confsys, ",icmpv6_type=%d", *match->oxm_value);
      break;
    case OFPXMT_OFB_ICMPV6_CODE:
      show(confsys, ",icmpv6_code=%d", *match->oxm_value);
      break;
    case OFPXMT_OFB_IPV6_ND_TARGET:
      show(confsys, ",ipv6_nd_target=%s",
           inet_ntop(AF_INET6, match->oxm_value, ip_str, sizeof(ip_str)));
      break;
    case OFPXMT_OFB_IPV6_ND_SLL:
      show(confsys, ",ipv6_nd_sll=%02x:%02x:%02x:%02x:%02x:%02x",
           match->oxm_value[0], match->oxm_value[1],
           match->oxm_value[2], match->oxm_value[3],
           match->oxm_value[4], match->oxm_value[5]);
      break;
    case OFPXMT_OFB_IPV6_ND_TLL:
      show(confsys, ",ipv6_nd_tll=%02x:%02x:%02x:%02x:%02x:%02x",
           match->oxm_value[0], match->oxm_value[1],
           match->oxm_value[2], match->oxm_value[3],
           match->oxm_value[4], match->oxm_value[5]);
      break;
    case OFPXMT_OFB_MPLS_LABEL:
      val32 = match_mpls_label_host_get(match);
      show(confsys, ",mpls_label=%d", val32);
      break;
    case OFPXMT_OFB_MPLS_TC:
      show(confsys, ",mpls_tc=%d", *match->oxm_value);
      break;
    case OFPXMT_OFB_MPLS_BOS:
      show(confsys, ",mpls_bos=%d", *match->oxm_value);
      break;
    case OFPXMT_OFB_PBB_ISID:
      val32 = match->oxm_value[0];
      val32 = (val32 << 8) | match->oxm_value[1];
      val32 = (val32 << 8) | match->oxm_value[2];
      show(confsys, ",pbb_isid=%d", val32);
      if (hasmask) {
        show(confsys, "/0x%02x%02x%02x",
             match->oxm_value[3],
             match->oxm_value[4],
             match->oxm_value[5]);
      }
      break;

    case OFPXMT_OFB_TUNNEL_ID:
      val64 = *(uint32_t *)match->oxm_value;
      val32 = *(uint32_t *)&match->oxm_value[4];;
      val64 = (ntohl(val64) << 32) | ntohl(val32);
      show(confsys, ",tunnel_id=%d", val64);
      if (hasmask) {
        val32 = *(uint32_t *)&match->oxm_value[8];
        show(confsys, "/0x%04x", ntohl(val32));
        val32 = *(uint32_t *)&match->oxm_value[8];
        show(confsys, "0x%04x", ntohl(val32));
      }
      break;
    case OFPXMT_OFB_IPV6_EXTHDR:
      val16 = *((uint16_t *)match->oxm_value);
      show(confsys, ",ipv6_exthdr=%d", ntohs(val16));
      if (hasmask) {
        val16 = *((uint16_t *)match->oxm_value);
        show(confsys, "/0x%02x", ntohs(val16));
      }
      break;
    default:
      show(confsys, ",unknown");
      break;
  }
}

static void
show_set_field(struct confsys *confsys, uint8_t *oxm) {
  uint8_t *oxm_value;
  uint16_t val16;
  uint32_t val32;
  uint64_t val64;
  char ip_str[INET6_ADDRSTRLEN];
  int field_type = oxm[2] >> 1;

  oxm_value = &oxm[4];

  switch (field_type) {
    case OFPXMT_OFB_IN_PORT:
      val32 = *((uint32_t *)oxm_value);
      show(confsys, "%d->in_port", ntohl(val32));
      break;
    case OFPXMT_OFB_IN_PHY_PORT:
      val32 = *((uint32_t *)oxm_value);
      show(confsys, "%d->in_phy_port", ntohl(val32));
      break;
    case OFPXMT_OFB_METADATA:
      val64 = *(uint32_t *)oxm_value;
      val32 = *(uint32_t *)&oxm_value[4];;
      val64 = (ntohl(val64) << 32) | ntohl(val32);
      show(confsys, "%d->metadata", val64);
      break;
    case OFPXMT_OFB_ETH_DST:
      show(confsys, "%02x:%02x:%02x:%02x:%02x:%02x->eth_dst",
           oxm_value[0], oxm_value[1],
           oxm_value[2], oxm_value[3],
           oxm_value[4], oxm_value[5]);
      break;
    case OFPXMT_OFB_ETH_SRC:
      show(confsys, "%02x:%02x:%02x:%02x:%02x:%02x->eth_src",
           oxm_value[0], oxm_value[1],
           oxm_value[2], oxm_value[3],
           oxm_value[4], oxm_value[5]);
      break;
    case OFPXMT_OFB_ETH_TYPE:
      show(confsys, "0x%02x%02x->eth_type",
           oxm_value[0], oxm_value[1]);
      break;
    case OFPXMT_OFB_VLAN_VID:
      val16 = *((uint16_t *)oxm_value);
      show(confsys, "%d->vlan_vid", ntohs(val16));
      break;
    case OFPXMT_OFB_VLAN_PCP:
      show(confsys, "%d->vlan_pcp", *oxm_value);
      break;
    case OFPXMT_OFB_IP_DSCP:
      show(confsys, "%d->ip_dscp", *oxm_value);
      break;
    case OFPXMT_OFB_IP_ECN:
      show(confsys, "%d->ip_ecn", *oxm_value);
      break;
    case OFPXMT_OFB_IP_PROTO:
      show(confsys, "%d->ip_proto", *oxm_value);
      break;
    case OFPXMT_OFB_IPV4_SRC:
      show(confsys, "%s->ipv4_src",
           inet_ntop(AF_INET, oxm_value, ip_str, sizeof(ip_str)));
      break;
    case OFPXMT_OFB_IPV4_DST:
      show(confsys, "%s->ipv4_dst",
           inet_ntop(AF_INET, oxm_value, ip_str, sizeof(ip_str)));
      break;
    case OFPXMT_OFB_TCP_SRC:
      val16 = *((uint16_t *)oxm_value);
      show(confsys, "%d->tcp_src", ntohs(val16));
      break;
    case OFPXMT_OFB_TCP_DST:
      val16 = *((uint16_t *)oxm_value);
      show(confsys, "%d->tcp_dst", ntohs(val16));
      break;
    case OFPXMT_OFB_UDP_SRC:
      val16 = *((uint16_t *)oxm_value);
      show(confsys, "%d->udp_src", ntohs(val16));
      break;
    case OFPXMT_OFB_UDP_DST:
      val16 = *((uint16_t *)oxm_value);
      show(confsys, "%d->udp_dst", ntohs(val16));
      break;
    case OFPXMT_OFB_SCTP_SRC:
      val16 = *((uint16_t *)oxm_value);
      show(confsys, "%d->sctp_src", ntohs(val16));
      break;
    case OFPXMT_OFB_SCTP_DST:
      val16 = *((uint16_t *)oxm_value);
      show(confsys, "%d->sctp_dst", ntohs(val16));
      break;
    case OFPXMT_OFB_ICMPV4_TYPE:
      show(confsys, "%d->icmpv4_type", *oxm_value);
      break;
    case OFPXMT_OFB_ICMPV4_CODE:
      show(confsys, "%d->icmpv4_code", *oxm_value);
      break;
    case OFPXMT_OFB_ARP_OP:
      val16 = *((uint16_t *)oxm_value);
      show(confsys, "%d->arp_op", ntohs(val16));
      break;
    case OFPXMT_OFB_ARP_SPA:
      show(confsys, "%s->arp_spa",
           inet_ntop(AF_INET, oxm_value, ip_str, sizeof(ip_str)));
      break;
    case OFPXMT_OFB_ARP_TPA:
      show(confsys, "%s->arp_tpa",
           inet_ntop(AF_INET, oxm_value, ip_str, sizeof(ip_str)));
      break;
    case OFPXMT_OFB_ARP_SHA:
      show(confsys, "%02x:%02x:%02x:%02x:%02x:%02x->arp_sha",
           oxm_value[0], oxm_value[1],
           oxm_value[2], oxm_value[3],
           oxm_value[4], oxm_value[5]);
      break;
    case OFPXMT_OFB_ARP_THA:
      show(confsys, "%02x:%02x:%02x:%02x:%02x:%02x->arp_tha",
           oxm_value[0], oxm_value[1],
           oxm_value[2], oxm_value[3],
           oxm_value[4], oxm_value[5]);
      break;
    case OFPXMT_OFB_IPV6_SRC:
      show(confsys, "%s->ipv6_src",
           inet_ntop(AF_INET6, oxm_value, ip_str, sizeof(ip_str)));
      break;
    case OFPXMT_OFB_IPV6_DST:
      show(confsys, "%s->ipv6_dst",
           inet_ntop(AF_INET6, oxm_value, ip_str, sizeof(ip_str)));
      break;
    case OFPXMT_OFB_IPV6_FLABEL:
      val32 = *((uint32_t *)oxm_value);
      show(confsys, "%d->ipv6_flabel", ntohl(val32));
      break;
    case OFPXMT_OFB_ICMPV6_TYPE:
      show(confsys, "%d->icmpv6_type", *oxm_value);
      break;
    case OFPXMT_OFB_ICMPV6_CODE:
      show(confsys, "%d->icmpv6_code", *oxm_value);
      break;
    case OFPXMT_OFB_IPV6_ND_TARGET:
      show(confsys, "%s->ipv6_nd_target",
           inet_ntop(AF_INET6, oxm_value, ip_str, sizeof(ip_str)));
      break;
    case OFPXMT_OFB_IPV6_ND_SLL:
      show(confsys, "%02x:%02x:%02x:%02x:%02x:%02x->ipv6_nd_sll",
           oxm_value[0], oxm_value[1],
           oxm_value[2], oxm_value[3],
           oxm_value[4], oxm_value[5]);
      break;
    case OFPXMT_OFB_IPV6_ND_TLL:
      show(confsys, "%02x:%02x:%02x:%02x:%02x:%02x->ipv6_nd_tll",
           oxm_value[0], oxm_value[1],
           oxm_value[2], oxm_value[3],
           oxm_value[4], oxm_value[5]);
      break;
    case OFPXMT_OFB_MPLS_LABEL:
      val32 = *((uint32_t *)oxm_value);
      show(confsys, "%d->mpls_label", ntohl(val32));
      break;
    case OFPXMT_OFB_MPLS_TC:
      show(confsys, "%d->mpls_tc", *oxm_value);
      break;
    case OFPXMT_OFB_MPLS_BOS:
      show(confsys, "%d->mpls_bos", *oxm_value);
      break;
    case OFPXMT_OFB_PBB_ISID:
      val32 = oxm_value[0];
      val32 = (val32 << 8) | oxm_value[1];
      val32 = (val32 << 8) | oxm_value[2];
      show(confsys, "%d->pbb_isid", val32);
      break;

    case OFPXMT_OFB_TUNNEL_ID:
      val64 = *(uint32_t *)oxm_value;
      val32 = *(uint32_t *)&oxm_value[4];;
      val64 = (ntohl(val64) << 32) | ntohl(val32);
      show(confsys, "%d->tunnel_id", val64);
      break;
    case OFPXMT_OFB_IPV6_EXTHDR:
      val16 = *((uint16_t *)oxm_value);
      show(confsys, "%d->ipv6_exthdr", ntohs(val16));
      break;
    default:
      show(confsys, "->unknown");
      break;
  }
}

static void
show_action(struct confsys *confsys, struct action *action) {
  switch (action->ofpat.type) {
    case OFPAT_OUTPUT:
      show(confsys, "output:%d",
           ((struct ofp_action_output *)&action->ofpat)->port);
      break;
    case OFPAT_COPY_TTL_OUT:
      show(confsys, "copy_ttl_out");
      break;
    case OFPAT_COPY_TTL_IN:
      show(confsys, "copy_ttl_in");
      break;
    case OFPAT_SET_MPLS_TTL:
      show(confsys, "set_mpls_ttl:%d",
           ((struct ofp_action_mpls_ttl *)&action->ofpat)->mpls_ttl);
      break;
    case OFPAT_DEC_MPLS_TTL:
      show(confsys, "dec_mpls_ttl");
      break;
    case OFPAT_PUSH_VLAN:
      show(confsys, "push_vlan:0x%04x",
           ((struct ofp_action_push *)&action->ofpat)->ethertype);
      break;
    case OFPAT_POP_VLAN:
      show(confsys, "pop_vlan");
      break;
    case OFPAT_PUSH_MPLS:
      show(confsys, "push_mpls:0x%04x",
           ((struct ofp_action_push *)&action->ofpat)->ethertype);
      break;
    case OFPAT_POP_MPLS:
      show(confsys, "pop_mpls:0x%04x",
           ((struct ofp_action_push *)&action->ofpat)->ethertype);
      break;
    case OFPAT_SET_QUEUE:
      show(confsys, "set_queue");
      break;
    case OFPAT_GROUP:
      show(confsys, "group:%d",
           ((struct ofp_action_group *)&action->ofpat)->group_id);
      break;
    case OFPAT_SET_NW_TTL:
      show(confsys, "set_nw_ttl:%d",
           ((struct ofp_action_nw_ttl *)&action->ofpat)->nw_ttl);
      break;
    case OFPAT_DEC_NW_TTL:
      show(confsys, "dec_nw_ttl");
      break;
    case OFPAT_SET_FIELD:
      show(confsys, "set_field:");
      show_set_field(confsys,
                     ((struct ofp_action_set_field *)&action->ofpat)->field);
      break;
    case OFPAT_PUSH_PBB:
      show(confsys, "push_pbb:0x%04x",
           ((struct ofp_action_push *)&action->ofpat)->ethertype);
      break;
    case OFPAT_POP_PBB:
      show(confsys, "pop_pbb");
      break;
    default:
      show(confsys, "unknown");
      break;
  }
}

static void
show_instruction(struct confsys *confsys, struct instruction *instruction) {
  struct action *action;
  uint32_t val32;
  uint64_t val64;
  int first;

  switch (instruction->ofpit.type) {
    case OFPIT_GOTO_TABLE:
      show(confsys, "goto_table:%d", instruction->ofpit_goto_table.table_id);
      break;
    case OFPIT_WRITE_METADATA:
      val32 = *(uint32_t *)&instruction->ofpit_write_metadata.metadata;
      val64 = ((uint32_t *)&instruction->ofpit_write_metadata.metadata)[1];
      val64 = (ntohl(val64) << 32) | ntohl(val32);
      show(confsys, "write_metadata:0x%x", val64);
      break;
    case OFPIT_WRITE_ACTIONS:
      first = 1;
      TAILQ_FOREACH(action, &instruction->action_list, entry) {
        if (first) {
          first = 0;
        } else {
          show(confsys, ",");
        }
        show_action(confsys, action);
      }
      break;
    case OFPIT_APPLY_ACTIONS:
      first = 1;
      TAILQ_FOREACH(action, &instruction->action_list, entry) {
        if (first) {
          first = 0;
        } else {
          show(confsys, ",");
        }
        show_action(confsys, action);
      }
      break;
    case OFPIT_CLEAR_ACTIONS:
      show(confsys, "clear_actions");
      break;
    case OFPIT_METER:
      show(confsys, "meter:%d", instruction->ofpit_meter.meter_id);
      break;
    case OFPIT_EXPERIMENTER:
    default:
      show(confsys, "unknown");
      break;
  }
}

static void
show_flow_stats(struct confsys *confsys, struct flow *flow) {
  show(confsys, ",idle_timeout=%u", flow->idle_timeout);
  show(confsys, ",hard_timeout=%u", flow->hard_timeout);
  show(confsys, ",flags=%u", flow->flags);
  show(confsys, ",cookie=%" PRIu64, flow->cookie);
  show(confsys, ",packet_count=%" PRIu64, flow->packet_count);
  show(confsys, ",byte_count=%" PRIu64, flow->byte_count);
}

static void
show_flow(struct confsys *confsys, struct flow *flow) {
  struct match *match;
  struct instruction *instruction;
  int first;

  show(confsys, "  priority=%d", flow->priority);
  show_flow_stats(confsys, flow);

  TAILQ_FOREACH(match, &flow->match_list, entry) {
    show_match(confsys, match);
  }
  show(confsys, " actions=");
  if (TAILQ_EMPTY(&flow->instruction_list)) {
    show(confsys, "drop");
  } else {
    first = 1;
    TAILQ_FOREACH(instruction, &flow->instruction_list, entry) {
      if (first) {
        first = 0;
      } else {
        show(confsys, ",");
      }
      show_instruction(confsys, instruction);
    }
  }
  show(confsys, "\n");
}

static void
show_ptree_flows(struct confsys *confsys, struct ptree *ptree) {
  struct ptree_node *node;

  for (node = ptree_top(ptree); node != NULL; node = ptree_next(node)) {
    struct flow_list *flow_list;
    if ((flow_list = node->info) != NULL) {
      show_flow_list(confsys, flow_list);
    }
  }
}

static void
show_flow_list(struct confsys *confsys, struct flow_list *flow_list) {
  int i;

  for (i = 0; i < flow_list->nflow; i++) {
    show_flow(confsys, flow_list->flows[i]);
  }

  if (flow_list->mpls_tree != NULL) {
    show_ptree_flows(confsys, flow_list->mpls_tree);
  }
}

static void
show_table_flows(struct confsys *confsys, struct table *table,
                 uint8_t table_id) {
  int i;
  struct flow_list *flow_list;

  show(confsys, " Table id: %d\n", table_id);

  for (i = 0; i < MAX_FLOWS; i++) {
    if ((flow_list = &table->flows[i]) != NULL) {
      show_flow_list(confsys, flow_list);
    }
  }
}

static void
show_bridge_domains_flow(struct confsys *confsys, struct bridge *bridge) {
  uint8_t i;
  struct flowdb *flowdb;
  struct table *table;

  flowdb = bridge->flowdb;
  flowdb_rdlock(flowdb);
  show(confsys, "Bridge: %s\n", bridge->name);

  for (i = 0; i < flowdb->table_size; i++) {
    if ((table = flowdb->tables[i]) != NULL) {
      show_table_flows(confsys, table, i);
    }
  }
  flowdb_rdunlock(flowdb);
}

CALLBACK(show_flow_func) {
  struct bridge *bridge;
  ARG_USED();

  TAILQ_FOREACH(bridge, &dpmgr->bridge_list, entry) {
    show_bridge_domains_flow(confsys, bridge);
  }
  return 0;
}
