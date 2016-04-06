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

/*
 * The common flow match APIs for the unittest.
 *
 * The declarations and definitions should be sorted by the layer in
 * the bottom-first order, and then lexicographically within each
 * layer.  NB the basic fields are the lowest.
 */

#include <sys/types.h>

/*
 * XXX a hack for the datapath-dependent types.
 */
typedef void ETHER_HDR;
typedef void IPV4_HDR;
typedef void IPV6_HDR;
typedef void OS_MBUF;
typedef void SCTP_HDR;
typedef void TCP_HDR;
typedef void UDP_HDR;
typedef void VLAN_HDR;

#include "lagopus/flowdb.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"
#include "datapath_test_match.h"
#include "datapath_test_match_macros.h"
#include "packet.h"


/*
 * Helper function prototypes.
 */
static void _add_port_match(struct match_list *match_list, uint8_t field,
                            uint32_t port);
static void _add_eth_addr_match(struct match_list *restrict match_list,
                                uint8_t field,
                                const uint8_t *restrict addr);
static void _add_ip_addr_match(struct match_list *restrict match_list,
                               uint8_t field,
                               const struct sockaddr *restrict addr);
static void _add_ip_addr_w_match(struct match_list *restrict match_list,
                                 uint8_t field,
                                 const struct sockaddr *restrict addr,
                                 const struct sockaddr *restrict mask);
static void _add_arp_ipv4_match(struct match_list *restrict match_list,
                                uint8_t field,
                                const struct in_addr *restrict addr);
static void _add_arp_ipv4_w_match(struct match_list *restrict match_list,
                                  uint8_t field,
                                  const struct in_addr *restrict addr,
                                  const struct in_addr *restrict mask);
static void _add_l4_port_match(struct match_list *match_list, uint8_t field,
                               uint16_t port);
static void _add_icmp_type_match(struct match_list *match_list, uint8_t field,
                                 uint8_t type);
static void _add_icmp_code_match(struct match_list *match_list, uint8_t field,
                                 uint8_t code);


/*
 * Basic.
 */

static void
_add_port_match(struct match_list *match_list, uint8_t field, uint32_t port) {
  add_match(match_list, sizeof(port), (uint8_t)(field << 1),
            NBO_BYTE(port, 0), NBO_BYTE(port, 1), NBO_BYTE(port, 2), NBO_BYTE(port, 3));
}

void
add_port_match(struct match_list *match_list, uint32_t port) {
  _add_port_match(match_list, OFPXMT_OFB_IN_PORT, port);
}

void
add_phyport_match(struct match_list *match_list, uint32_t port) {
  _add_port_match(match_list, OFPXMT_OFB_IN_PHY_PORT, port);
}

void
add_metadata_match(struct match_list *match_list, uint64_t md) {
  add_match(match_list, sizeof(md), OFPXMT_OFB_METADATA << 1,
            NBO_BYTE(md, 0), NBO_BYTE(md, 1), NBO_BYTE(md, 2), NBO_BYTE(md, 3),
            NBO_BYTE(md, 4), NBO_BYTE(md, 5), NBO_BYTE(md, 6), NBO_BYTE(md, 7));
}

void
add_metadata_w_match(struct match_list *match_list, uint64_t md,
                     uint64_t mask) {
  add_match(match_list, 2 * sizeof(md), (OFPXMT_OFB_METADATA << 1) | 1,
            NBO_BYTE(md, 0), NBO_BYTE(md, 1), NBO_BYTE(md, 2), NBO_BYTE(md, 3),
            NBO_BYTE(md, 4), NBO_BYTE(md, 5), NBO_BYTE(md, 6), NBO_BYTE(md, 7),
            NBO_BYTE(mask, 0), NBO_BYTE(mask, 1), NBO_BYTE(mask, 2), NBO_BYTE(mask, 3),
            NBO_BYTE(mask, 4), NBO_BYTE(mask, 5), NBO_BYTE(mask, 6), NBO_BYTE(mask, 7));
}

void
add_tunnelid_match(struct match_list *match_list, uint64_t id) {
  add_match(match_list, sizeof(id), OFPXMT_OFB_TUNNEL_ID << 1,
            NBO_BYTE(id, 0), NBO_BYTE(id, 1), NBO_BYTE(id, 2), NBO_BYTE(id, 3),
            NBO_BYTE(id, 4), NBO_BYTE(id, 5), NBO_BYTE(id, 6), NBO_BYTE(id, 7));
}

void
add_tunnelid_w_match(struct match_list *match_list, uint64_t id,
                     uint64_t mask) {
  add_match(match_list, sizeof(id), (OFPXMT_OFB_TUNNEL_ID << 1) | 1,
            NBO_BYTE(id, 0), NBO_BYTE(id, 1), NBO_BYTE(id, 2), NBO_BYTE(id, 3),
            NBO_BYTE(id, 4), NBO_BYTE(id, 5), NBO_BYTE(id, 6), NBO_BYTE(id, 7),
            NBO_BYTE(mask, 0), NBO_BYTE(mask, 1), NBO_BYTE(mask, 2), NBO_BYTE(mask, 3),
            NBO_BYTE(mask, 4), NBO_BYTE(mask, 5), NBO_BYTE(mask, 6), NBO_BYTE(mask, 7));
}


/*
 * VLAN.
 */

void
add_vlan_vid_match(struct match_list *match_list, uint16_t vid) {
  add_match(match_list, sizeof(vid), OFPXMT_OFB_VLAN_VID << 1,
            NBO_BYTE(vid, 0), NBO_BYTE(vid, 1));
}

void
add_vlan_vid_w_match(struct match_list *match_list, uint16_t vid,
                     uint16_t mask) {
  add_match(match_list, sizeof(vid), (OFPXMT_OFB_VLAN_VID << 1) | 1,
            NBO_BYTE(vid, 0), NBO_BYTE(vid, 1),
            NBO_BYTE(mask, 0), NBO_BYTE(mask, 1));
}

void
add_vlan_pcp_match(struct match_list *match_list, uint8_t pcp) {
  add_match(match_list, sizeof(pcp), OFPXMT_OFB_VLAN_PCP << 1,
            pcp);
}


/*
 * Ethernet.
 */

static void
_add_eth_addr_match(struct match_list *restrict match_list, uint8_t field,
                    const uint8_t *restrict addr) {
  /* Assume a MAC address in the network byte order. */
  add_match(match_list, OFP_ETH_ALEN, ((uint8_t)(field << 1)) & 0xff,
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}

static void
_add_eth_addr_w_match(struct match_list *restrict match_list, uint8_t field,
                      const uint8_t *restrict addr, const uint8_t *restrict mask) {
  /* Assume a MAC address in the network byte order. */
  add_match(match_list, OFP_ETH_ALEN, ((uint8_t)((field << 1) | 1)) & 0xff,
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
            mask[0], mask[1], mask[2], mask[3], mask[4], mask[5]);
}

void
add_eth_dst_match(struct match_list *restrict match_list,
                  const uint8_t *restrict addr) {
  _add_eth_addr_match(match_list, OFPXMT_OFB_ETH_DST, addr);
}

void
add_eth_dst_w_match(struct match_list *restrict match_list,
                    const uint8_t *restrict addr,
                    const uint8_t *restrict mask) {
  _add_eth_addr_w_match(match_list, OFPXMT_OFB_ETH_DST, addr, mask);
}

void
add_eth_src_match(struct match_list *restrict match_list,
                  const uint8_t *restrict addr) {
  _add_eth_addr_match(match_list, OFPXMT_OFB_ETH_SRC, addr);
}

void
add_eth_src_w_match(struct match_list *restrict match_list,
                    const uint8_t *restrict addr,
                    const uint8_t *restrict mask) {
  _add_eth_addr_w_match(match_list, OFPXMT_OFB_ETH_SRC, addr, mask);
}

void
add_eth_type_match(struct match_list *match_list, uint16_t type) {
  add_match(match_list, sizeof(type), OFPXMT_OFB_ETH_TYPE << 1,
            NBO_BYTE(type, 0), NBO_BYTE(type, 1));
}


/*
 * IPv4/IPv6.
 */

static void
_add_ip_addr_match(struct match_list *restrict match_list, uint8_t field,
                   const struct sockaddr *restrict addr) {
  const char *p;

  /*
   * XXX inline expanded because the lib functions do not take the
   * pointers to const!
   */
  switch (addr->sa_family) {
    case AF_INET:
      /* Assume the IPv4 address in the network byte order. */
      p = (const char *)&((struct sockaddr_in *)addr)->sin_addr.s_addr;
      add_match(match_list, sizeof(((struct sockaddr_in *)addr)->sin_addr.s_addr), (uint8_t)(field << 1),
                p[0], p[1], p[2], p[3]);
      break;

    case AF_INET6:
      /* An IPv6 address is always in the network byte order. */
      p = (const char *)&((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr;
      add_match(match_list, sizeof(((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr), (uint8_t)(field << 1),
                p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
                p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
      break;
  }
}

static void
_add_ip_addr_w_match(struct match_list *restrict match_list, uint8_t field,
                     const struct sockaddr *restrict addr,
                     const struct sockaddr *restrict mask) {
  const char *p, *q;
  /*
   * XXX inline expanded because the lib functions do not take the
   * pointers to const!
   */
  switch (addr->sa_family) {
    case AF_INET:
      /* Assume the IPv4 address in the network byte order. */
      p = (const char *)&((struct sockaddr_in *)addr)->sin_addr.s_addr;
      q = (const char *)&((struct sockaddr_in *)mask)->sin_addr.s_addr;
      add_match(match_list, sizeof(((struct sockaddr_in *)addr)->sin_addr.s_addr), (uint8_t)((field << 1) | 1),
                p[0], p[1], p[2], p[3],
                q[0], q[1], q[2], q[3]);
      break;

    case AF_INET6:
      /* An IPv6 address is always in the network byte order. */
      p = (const char *)&((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr;
      q = (const char *)&((struct sockaddr_in6 *)mask)->sin6_addr.s6_addr;
      add_match(match_list, sizeof(((struct sockaddr_in6 *)mask)->sin6_addr.s6_addr), (uint8_t)((field << 1) | 1),
                p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
                p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15],
                q[0], q[1], q[2], q[3], q[4], q[5], q[6], q[7],
                q[8], q[9], q[10], q[11], q[12], q[13], q[14], q[15]);
      break;
  }
}

void
add_ip_src_match(struct match_list *restrict match_list,
                 const struct sockaddr *restrict addr) {
  switch (addr->sa_family) {
    case AF_INET:
      _add_ip_addr_match(match_list, OFPXMT_OFB_IPV4_SRC, addr);
      break;

    case AF_INET6:
      _add_ip_addr_match(match_list, OFPXMT_OFB_IPV6_SRC, addr);
      break;
  }
}

void
add_ip_src_w_match(struct match_list *restrict match_list,
                   const struct sockaddr *restrict addr,
                   const struct sockaddr *restrict mask) {
  switch (addr->sa_family) {
    case AF_INET:
      _add_ip_addr_w_match(match_list, OFPXMT_OFB_IPV4_SRC, addr, mask);
      break;

    case AF_INET6:
      _add_ip_addr_w_match(match_list, OFPXMT_OFB_IPV6_SRC, addr, mask);
      break;
  }
}

void
add_ip_dst_match(struct match_list *restrict match_list,
                 const struct sockaddr *restrict addr) {
  switch (addr->sa_family) {
    case AF_INET:
      _add_ip_addr_match(match_list, OFPXMT_OFB_IPV4_DST, addr);
      break;

    case AF_INET6:
      _add_ip_addr_match(match_list, OFPXMT_OFB_IPV6_DST, addr);
      break;
  }
}

void
add_ip_dst_w_match(struct match_list *restrict match_list,
                   const struct sockaddr *restrict addr,
                   const struct sockaddr *restrict mask) {
  switch (addr->sa_family) {
    case AF_INET:
      _add_ip_addr_w_match(match_list, OFPXMT_OFB_IPV4_DST, addr, mask);
      break;

    case AF_INET6:
      _add_ip_addr_w_match(match_list, OFPXMT_OFB_IPV6_DST, addr, mask);
      break;
  }
}

void
add_ip_dscp_match(struct match_list *match_list, uint8_t dscp) {
  add_match(match_list, sizeof(dscp), OFPXMT_OFB_IP_DSCP << 1,
            dscp);
}

void
add_ip_ecn_match(struct match_list *match_list, uint8_t ecn) {
  add_match(match_list, sizeof(ecn), OFPXMT_OFB_IP_ECN << 1,
            ecn);
}

void
add_ip_proto_match(struct match_list *match_list, uint8_t proto) {
  add_match(match_list, sizeof(proto), OFPXMT_OFB_IP_PROTO << 1,
            proto);
}

void
add_ipv6_flowlabel_match(struct match_list *match_list, uint32_t label) {
  add_match(match_list, sizeof(label), OFPXMT_OFB_IPV6_FLABEL << 1,
            NBO_BYTE(label, 0), NBO_BYTE(label, 1), NBO_BYTE(label, 2), NBO_BYTE(label,
                3));
}

void
add_ipv6_exthdr_match(struct match_list *match_list, uint16_t xh) {
  add_match(match_list, sizeof(xh), OFPXMT_OFB_IPV6_EXTHDR << 1,
            NBO_BYTE(xh, 0), NBO_BYTE(xh, 1));
}

void
add_ipv6_exthdr_w_match(struct match_list *match_list, uint16_t xh,
                        uint16_t mask) {
  add_match(match_list, sizeof(xh), (OFPXMT_OFB_IPV6_EXTHDR << 1) | 1,
            NBO_BYTE(xh, 0), NBO_BYTE(xh, 1),
            NBO_BYTE(mask, 0), NBO_BYTE(mask, 1));
}


/*
 * ARP. (NB IPv6 ND is L4)
 */

void
add_arp_op_match(struct match_list *match_list, uint16_t op) {
  add_match(match_list, sizeof(op), OFPXMT_OFB_ARP_OP << 1,
            NBO_BYTE(op, 0), NBO_BYTE(op, 1));
}

static void
_add_arp_ipv4_match(struct match_list *restrict match_list, uint8_t field,
                    const struct in_addr *restrict addr) {
  const char *p;

  p = (const char *)&addr->s_addr;
  add_match(match_list, sizeof(struct in_addr), ((uint8_t)(field << 1)),
            p[0], p[1], p[2], p[3]);
}

static void
_add_arp_ipv4_w_match(struct match_list *restrict match_list, uint8_t field,
                      const struct in_addr *restrict addr,
                      const struct in_addr *restrict mask) {
  const char *p, *q;

  p = (const char *)&addr->s_addr;
  q = (const char *)&mask->s_addr;
  add_match(match_list, sizeof(struct in_addr), (uint8_t)((field << 1) | 1),
            p[0], p[1], p[2], p[3],
            q[0], q[1], q[2], q[3]);
}

void
add_arp_spa_match(struct match_list *restrict match_list,
                  const struct in_addr *restrict addr) {
  _add_arp_ipv4_match(match_list, OFPXMT_OFB_ARP_SPA, addr);
}

void
add_arp_spa_w_match(struct match_list *restrict match_list,
                    const struct in_addr *restrict addr,
                    const struct in_addr *restrict mask) {
  _add_arp_ipv4_w_match(match_list, OFPXMT_OFB_ARP_SPA, addr, mask);
}

void
add_arp_tpa_match(struct match_list *restrict match_list,
                  const struct in_addr *restrict addr) {
  _add_arp_ipv4_match(match_list, OFPXMT_OFB_ARP_TPA, addr);
}

void
add_arp_tpa_w_match(struct match_list *restrict match_list,
                    const struct in_addr *restrict addr,
                    const struct in_addr *restrict mask) {
  _add_arp_ipv4_w_match(match_list, OFPXMT_OFB_ARP_TPA, addr, mask);
}

static void
_add_arp_eth_match(struct match_list *restrict match_list, uint8_t field,
                   const uint8_t *restrict addr) {
  add_match(match_list, OFP_ETH_ALEN, ((uint8_t)(field << 1)),
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}

static void
_add_arp_eth_w_match(struct match_list *restrict match_list, uint8_t field,
                     const uint8_t *restrict addr,
                     const uint8_t *restrict mask) {
  add_match(match_list, OFP_ETH_ALEN, (uint8_t)((field << 1) | 1),
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
            mask[0], mask[1], mask[2], mask[3], mask[4], mask[5]);
}

void
add_arp_sha_match(struct match_list *restrict match_list,
                  const uint8_t *restrict addr) {
  _add_arp_eth_match(match_list, OFPXMT_OFB_ARP_SHA, addr);
}

void
add_arp_sha_w_match(struct match_list *restrict match_list,
                    const uint8_t *restrict addr,
                    const uint8_t *restrict mask) {
  _add_arp_eth_w_match(match_list, OFPXMT_OFB_ARP_SHA, addr, mask);
}

void
add_arp_tha_match(struct match_list *restrict match_list,
                  const uint8_t *restrict addr) {
  _add_arp_eth_match(match_list, OFPXMT_OFB_ARP_THA, addr);
}

void
add_arp_tha_w_match(struct match_list *restrict match_list,
                    const uint8_t *restrict addr,
                    const uint8_t *restrict mask) {
  _add_arp_eth_w_match(match_list, OFPXMT_OFB_ARP_THA, addr, mask);
}


/*
 * MPLS.
 */

void
add_mpls_label_match(struct match_list *match_list, uint32_t label) {
  add_match(match_list, sizeof(label), OFPXMT_OFB_MPLS_LABEL << 1,
            NBO_BYTE(label, 0), NBO_BYTE(label, 1), NBO_BYTE(label, 2), NBO_BYTE(label,
                3));
}

void
add_mpls_tc_match(struct match_list *match_list, uint8_t tc) {
  add_match(match_list, sizeof(tc), OFPXMT_OFB_MPLS_TC << 1,
            tc);
}

void
add_mpls_bos_match(struct match_list *match_list, uint8_t bos) {
  add_match(match_list, sizeof(bos), OFPXMT_OFB_MPLS_BOS << 1,
            bos);
}


/*
 * PBB.
 */

void
add_pbb_isid_match(struct match_list *match_list, uint32_t isid) {
  add_match(match_list, sizeof(isid) - 1, OFPXMT_OFB_PBB_ISID << 1,
            NBO_BYTE(isid, 1), NBO_BYTE(isid, 2), NBO_BYTE(isid, 3));
}

void
add_pbb_isid_w_match(struct match_list *match_list, uint32_t isid,
                     uint32_t mask) {
  add_match(match_list, sizeof(isid) - 1, (OFPXMT_OFB_PBB_ISID << 1) | 1,
            NBO_BYTE(isid, 1), NBO_BYTE(isid, 2), NBO_BYTE(isid, 3),
            NBO_BYTE(mask, 1), NBO_BYTE(mask, 2), NBO_BYTE(mask, 3));
}

#ifdef notyet
void
add_pbb_uca_match(struct match_list *match_list, uint8_t uca) {
  add_match(match_list, sizeof(uca), OFPXMT_OFB_PBB_UCA << 1,
            uca);
}
#endif /* notyet */


/*
 * L4.
 */

static void
_add_l4_port_match(struct match_list *match_list, uint8_t field,
                   uint16_t port) {
  add_match(match_list, sizeof(port), (uint8_t)(field << 1),
            NBO_BYTE(port, 0), NBO_BYTE(port, 1));
}

void
add_tcp_src_match(struct match_list *match_list, uint16_t port) {
  _add_l4_port_match(match_list, OFPXMT_OFB_TCP_SRC, port);
}

void
add_tcp_dst_match(struct match_list *match_list, uint16_t port) {
  _add_l4_port_match(match_list, OFPXMT_OFB_TCP_DST, port);
}

void
add_udp_src_match(struct match_list *match_list, uint16_t port) {
  _add_l4_port_match(match_list, OFPXMT_OFB_UDP_SRC, port);
}

void
add_udp_dst_match(struct match_list *match_list, uint16_t port) {
  _add_l4_port_match(match_list, OFPXMT_OFB_UDP_DST, port);
}

void
add_sctp_src_match(struct match_list *match_list, uint16_t port) {
  _add_l4_port_match(match_list, OFPXMT_OFB_SCTP_SRC, port);
}

void
add_sctp_dst_match(struct match_list *match_list, uint16_t port) {
  _add_l4_port_match(match_list, OFPXMT_OFB_SCTP_DST, port);
}

static void
_add_icmp_type_match(struct match_list *match_list, uint8_t field,
                     uint8_t type) {
  add_match(match_list, sizeof(type), (uint8_t)(field << 1),
            type);
}

static void
_add_icmp_code_match(struct match_list *match_list, uint8_t field,
                     uint8_t code) {
  add_match(match_list, sizeof(code), (uint8_t)(field << 1),
            code);
}

void
add_icmp_type_match(struct match_list *match_list, uint8_t type) {
  _add_icmp_type_match(match_list, OFPXMT_OFB_ICMPV4_TYPE, type);
}

void
add_icmp_code_match(struct match_list *match_list, uint8_t code) {
  _add_icmp_code_match(match_list, OFPXMT_OFB_ICMPV4_CODE, code);
}

void
add_icmpv6_type_match(struct match_list *match_list, uint8_t type) {
  _add_icmp_type_match(match_list, OFPXMT_OFB_ICMPV6_TYPE, type);
}

void
add_icmpv6_code_match(struct match_list *match_list, uint8_t code) {
  _add_icmp_code_match(match_list, OFPXMT_OFB_ICMPV6_CODE, code);
}


/*
 * IPv6 ND.
 */

void
add_ipv6_nd_target_match(struct match_list *restrict match_list,
                         const struct in6_addr *restrict addr) {
  const char *p;

  p = (const char *)&addr->s6_addr;
  add_match(match_list, sizeof(*addr), OFPXMT_OFB_IPV6_ND_TARGET << 1,
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
            p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
}

void
add_ipv6_nd_sll_match(struct match_list *restrict match_list,
                      const uint8_t *restrict addr) {
  _add_arp_eth_match(match_list, OFPXMT_OFB_IPV6_ND_SLL, addr);
}

void
add_ipv6_nd_tll_match(struct match_list *restrict match_list,
                      const uint8_t *restrict addr) {
  _add_arp_eth_match(match_list, OFPXMT_OFB_IPV6_ND_TLL, addr);
}
