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

#ifndef __DATAPLANE_TEST_MATCH_H__
#define __DATAPLANE_TEST_MATCH_H__

/*
 * The common flow match APIs for the unittest.
 *
 * The declarations and definitions should be sorted by the layer in
 * the bottom-first order, and then lexicographically within each
 * layer.  NB the basic fields are the lowest.
 *
 * Macros should be placed in datapath_test_match_macros.h so that we
 * prevent namespace pollution.
 */

/* Forward declarations. */
struct sockaddr;
struct flow;
struct lagopus_packet;
struct match_list;

/* Function prototypes. */

/*
 * Basic.
 */
void add_port_match(struct match_list *match_list, uint32_t port);
void add_phyport_match(struct match_list *match_list, uint32_t port);
void add_metadata_match(struct match_list *match_list, uint64_t md);
void add_metadata_w_match(struct match_list *match_list, uint64_t md,
                          uint64_t mask);
void add_tunnelid_match(struct match_list *match_list, uint64_t id);
void add_tunnelid_w_match(struct match_list *match_list, uint64_t id,
                          uint64_t mask);

/*
 * VLAN.
 */
void add_vlan_vid_match(struct match_list *match_list, uint16_t vid);
void add_vlan_vid_w_match(struct match_list *match_list, uint16_t vid,
                          uint16_t mask);
void add_vlan_pcp_match(struct match_list *match_list, uint8_t pcp);

/*
 * Ethernet.
 */
void add_eth_dst_match(struct match_list *restrict match_list,
                       const uint8_t *restrict addr);
void add_eth_dst_w_match(struct match_list *restrict match_list,
                         const uint8_t  *restrict addr,
                         const uint8_t *restrict mask);
void add_eth_src_match(struct match_list *restrict match_list,
                       const uint8_t *restrict addr);
void add_eth_src_w_match(struct match_list *restrict match_list,
                         const uint8_t  *restrict addr,
                         const uint8_t *restrict mask);
void add_eth_type_match(struct match_list *match_list, uint16_t type);

/*
 * IPv4/IPv6.
 */
void add_ip_src_match(struct match_list *restrict match_list,
                      const struct sockaddr *restrict addr);
void add_ip_src_w_match(struct match_list *restrict match_list,
                        const struct sockaddr *restrict addr,
                        const struct sockaddr *restrict mask);
void add_ip_dst_match(struct match_list *restrict match_list,
                      const struct sockaddr *restrict addr);
void add_ip_dst_w_match(struct match_list *restrict match_list,
                        const struct sockaddr *restrict addr,
                        const struct sockaddr *restrict mask);
void add_ip_dscp_match(struct match_list *match_list, uint8_t dscp);
void add_ip_ecn_match(struct match_list *match_list, uint8_t ecn);
void add_ip_proto_match(struct match_list *match_list, uint8_t proto);
void add_ipv6_flowlabel_match(struct match_list *match_list, uint32_t label);
void add_ipv6_exthdr_match(struct match_list *match_list, uint16_t xh);
void add_ipv6_exthdr_w_match(struct match_list *match_list, uint16_t xh,
                             uint16_t mask);


/*
 * ARP. (NB IPv6 ND is L4)
 */
void add_arp_op_match(struct match_list *match_list,
                      uint16_t op);
void add_arp_spa_match(struct match_list *restrict match_list,
                       const struct in_addr *restrict addr);
void add_arp_spa_w_match(struct match_list *restrict match_list,
                         const struct in_addr *restrict addr,
                         const struct in_addr *restrict mask);
void add_arp_tpa_match(struct match_list *restrict match_list,
                       const struct in_addr *restrict addr);
void add_arp_tpa_w_match(struct match_list *restrict match_list,
                         const struct in_addr *restrict addr,
                         const struct in_addr *restrict mask);
void add_arp_sha_match(struct match_list *restrict match_list,
                       const uint8_t *restrict addr);
void add_arp_sha_w_match(struct match_list *restrict match_list,
                         const uint8_t *restrict addr,
                         const uint8_t *restrict mask);
void add_arp_tha_match(struct match_list *restrict match_list,
                       const uint8_t *restrict addr);
void add_arp_tha_w_match(struct match_list *restrict match_list,
                         const uint8_t *restrict addr,
                         const uint8_t *restrict mask);


/*
 * MPLS.
 */
void add_mpls_label_match(struct match_list *match_list, uint32_t label);
void add_mpls_tc_match(struct match_list *match_list, uint8_t tc);
void add_mpls_bos_match(struct match_list *match_list, uint8_t bos);


/*
 * PBB.
 */
void add_pbb_isid_match(struct match_list *match_list, uint32_t isid);
void add_pbb_isid_w_match(struct match_list *match_list, uint32_t isid,
                          uint32_t mask);
#ifdef notyet
void add_pbb_uca_match(struct match_list *match_list, uint8_t uca);
#endif /* notyet */

/*
 * L4.
 */
void add_tcp_src_match(struct match_list *match_list, uint16_t port);
void add_tcp_dst_match(struct match_list *match_list, uint16_t port);
void add_udp_src_match(struct match_list *match_list, uint16_t port);
void add_udp_dst_match(struct match_list *match_list, uint16_t port);
void add_sctp_src_match(struct match_list *match_list, uint16_t port);
void add_sctp_dst_match(struct match_list *match_list, uint16_t port);
void add_icmp_type_match(struct match_list *match_list, uint8_t type);
void add_icmp_code_match(struct match_list *match_list, uint8_t code);
void add_icmpv6_type_match(struct match_list *match_list, uint8_t type);
void add_icmpv6_code_match(struct match_list *match_list, uint8_t code);

/*
 * IPv6 ND.
 */
void add_ipv6_nd_target_match(struct match_list *restrict match_list,
                              const struct in6_addr *restrict addr);
void add_ipv6_nd_sll_match(struct match_list *restrict match_list,
                           const uint8_t *restrict addr);
void add_ipv6_nd_tll_match(struct match_list *restrict match_list,
                           const uint8_t *restrict addr);

#endif /* __DATAPLANE_TEST_MATCH_H__ */
