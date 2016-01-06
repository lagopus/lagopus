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
 * @file	datapath_test_match_macros.h
 */

#ifndef __DATAPLANE_TEST_MATCH_MACROS_H__
#define __DATAPLANE_TEST_MATCH_MACROS_H__

/*
 * The common flow match APIs for the unittest.
 *
 * The declarations and definitions should be sorted by the layer in
 * the bottom-first order, and then lexicographically within each
 * layer.  NB the basic fields are the lowest.
 *
 * Functions should be placed in datapath_test_match.h so that we
 * prevent namespace pollution.
 */


/*
 * Tools.
 */

/* Add an ARP operation match to a flow. */
#define FLOW_ADD_ARP_OP_MATCH(_fl, _op)			\
  do {							\
    add_arp_op_match(&(_fl)->match_list, (_op));	\
  } while (0)

/* Add an ARP source IPv4 address match to a flow. */
#define FLOW_ADD_ARP_SPA_MATCH(_fl, _addr)		\
  do {							\
    add_arp_spa_match(&(_fl)->match_list, (_addr));	\
  } while (0)

/* Add an ARP source IPv4 address match with a mask to a flow. */
#define FLOW_ADD_ARP_SPA_W_MATCH(_fl, _addr, _mask)		\
  do {								\
    add_arp_spa_w_match(&(_fl)->match_list, (_addr), (_mask));	\
  } while (0)

/* Add an ARP target IPv4 address match to a flow. */
#define FLOW_ADD_ARP_TPA_MATCH(_fl, _addr)		\
  do {							\
    add_arp_tpa_match(&(_fl)->match_list, (_addr));	\
  } while (0)

/* Add an ARP target IPv4 address match with a mask to a flow. */
#define FLOW_ADD_ARP_TPA_W_MATCH(_fl, _addr, _mask)		\
  do {								\
    add_arp_tpa_w_match(&(_fl)->match_list, (_addr), (_mask));	\
  } while (0)

/* Add an ARP source Ethernet address match to a flow. */
#define FLOW_ADD_ARP_SHA_MATCH(_fl, _addr)		\
  do {							\
    add_arp_sha_match(&(_fl)->match_list, (_addr));	\
  } while (0)

/* Add an ARP source Ethernet address match with a mask to a flow. */
#define FLOW_ADD_ARP_SHA_W_MATCH(_fl, _addr, _mask)		\
  do {								\
    add_arp_sha_w_match(&(_fl)->match_list, (_addr), (_mask));	\
  } while (0)

/* Add an ARP target Ethernet address match to a flow. */
#define FLOW_ADD_ARP_THA_MATCH(_fl, _addr)		\
  do {							\
    add_arp_tha_match(&(_fl)->match_list, (_addr));	\
  } while (0)

/* Add an ARP target Ethernet address match with a mask to a flow. */
#define FLOW_ADD_ARP_THA_W_MATCH(_fl, _addr, _mask)		\
  do {								\
    add_arp_tha_w_match(&(_fl)->match_list, (_addr), (_mask));	\
  } while (0)

/* Add an Ethernet type match to a flow. */
#define FLOW_ADD_ETH_TYPE_MATCH(_fl, _type)		\
  do {							\
    add_eth_type_match(&(_fl)->match_list, (_type));	\
  } while (0)

/* Add an Ethernet destination match to a flow. */
#define FLOW_ADD_ETH_DST_MATCH(_fl, _addr)		\
  do {							\
    add_eth_dst_match(&(_fl)->match_list, (_addr));	\
  } while (0)

/* Add an Ethernet destination match with a mask to a flow. */
#define FLOW_ADD_ETH_DST_W_MATCH(_fl, _addr, _mask)		\
  do {								\
    add_eth_dst_w_match(&(_fl)->match_list, (_addr), (_mask));	\
  } while (0)

/* Add an Ethernet source match to a flow. */
#define FLOW_ADD_ETH_SRC_MATCH(_fl, _addr)		\
  do {							\
    add_eth_src_match(&(_fl)->match_list, (_addr));	\
  } while (0)

/* Add an Ethernet source match with a mask to a flow. */
#define FLOW_ADD_ETH_SRC_W_MATCH(_fl, _addr, _mask)		\
  do {								\
    add_eth_src_w_match(&(_fl)->match_list, (_addr), (_mask));	\
  } while (0)

/* Add an IP source address match to a flow. */
#define FLOW_ADD_IP_SRC_MATCH(_fl, _src)		\
  do {							\
    add_ip_src_match(&(_fl)->match_list, (_src));	\
  } while (0)

/* Add an IP source address match with a mask to a flow. */
#define FLOW_ADD_IP_SRC_W_MATCH(_fl, _src, _mask)		\
  do {								\
    add_ip_src_w_match(&(_fl)->match_list, (_src), (_mask));	\
  } while (0)

/* Add an IP destination address match to a flow. */
#define FLOW_ADD_IP_DST_MATCH(_fl, _dst)		\
  do {							\
    add_ip_dst_match(&(_fl)->match_list, (_dst));	\
  } while (0)

/* Add an IP destination address match with a mask to a flow. */
#define FLOW_ADD_IP_DST_W_MATCH(_fl, _dst, _mask)		\
  do {								\
    add_ip_dst_w_match(&(_fl)->match_list, (_dst), (_mask));	\
  } while (0)

/* Add an IP DSCP match to a flow. */
#define FLOW_ADD_IP_DSCP_MATCH(_fl, _dscp)		\
  do {							\
    add_ip_dscp_match(&(_fl)->match_list, (_dscp));	\
  } while (0)

/* Add an IP ECN match to a flow. */
#define FLOW_ADD_IP_ECN_MATCH(_fl, _ecn)		\
  do {							\
    add_ip_ecn_match(&(_fl)->match_list, (_ecn));	\
  } while (0)

/* Add an IP protocol match to a flow. */
#define FLOW_ADD_IP_PROTO_MATCH(_fl, _proto)		\
  do {							\
    add_ip_proto_match(&(_fl)->match_list, (_proto));	\
  } while (0)

/* Add an IPv6 flow label match to a flow. */
#define FLOW_ADD_IPV6_FLOWLABEL_MATCH(_fl, _label)		\
  do {								\
    add_ipv6_flowlabel_match(&(_fl)->match_list, (_label));	\
  } while (0)

/* Add an IPv6 ND target address match to a flow. */
#define FLOW_ADD_IPV6_ND_TARGET_MATCH(_fl, _addr)		\
  do {								\
    add_ipv6_nd_target_match(&(_fl)->match_list, (_addr));	\
  } while (0)

/* Add an IPv6 ND source link layer address match to a flow. */
#define FLOW_ADD_IPV6_ND_SLL_MATCH(_fl, _addr)		\
  do {							\
    add_ipv6_nd_sll_match(&(_fl)->match_list, (_addr));	\
  } while (0)

/* Add an IPv6 ND target link layer address match to a flow. */
#define FLOW_ADD_IPV6_ND_TLL_MATCH(_fl, _addr)		\
  do {							\
    add_ipv6_nd_tll_match(&(_fl)->match_list, (_addr));	\
  } while (0)

/* Add an IPv6 extension header match to a flow. */
#define FLOW_ADD_IPV6_EXTHDR_MATCH(_fl, _xh)		\
  do {							\
    add_ipv6_exthdr_match(&(_fl)->match_list, (_xh));	\
  } while (0)

/* Add an IPv6 extension header match with a mask to a flow. */
#define FLOW_ADD_IPV6_EXTHDR_W_MATCH(_fl, _xh, _mask)			\
  do {									\
    add_ipv6_exthdr_w_match(&(_fl)->match_list, (_xh), (_mask));	\
  } while (0)

/* Add a port match to a flow. */
#define FLOW_ADD_PORT_MATCH(_fl, _pt)		\
  do {						\
    add_port_match(&(_fl)->match_list, (_pt));	\
  } while (0)

/* Add a physical port match to a flow. */
#define FLOW_ADD_PHYPORT_MATCH(_fl, _pt)		\
  do {							\
    add_phyport_match(&(_fl)->match_list, (_pt));	\
  } while (0)

/* Add a PBB I-SID match to a flow. */
#define FLOW_ADD_PBB_ISID_MATCH(_fl, _isid)		\
  do {							\
    add_pbb_isid_match(&(_fl)->match_list, (_isid));	\
  } while (0)

/* Add a PBB I-SID match with a match to a flow. */
#define FLOW_ADD_PBB_ISID_W_MATCH(_fl, _isid, _mask)		\
  do {								\
    add_pbb_isid_w_match(&(_fl)->match_list, (_isid), (_mask));	\
  } while (0)

#ifdef notyet
/* Add a PBB UCA match to a flow. */
#define FLOW_ADD_PBB_UCA_MATCH(_fl, _uca)		\
  do {							\
    add_pbb_uca_match(&(_fl)->match_list, (_uca));	\
  } while (0)
#endif /* notyet */

/* Add a metadata match to a flow. */
#define FLOW_ADD_METADATA_MATCH(_fl, _md)		\
  do {							\
    add_metadata_match(&(_fl)->match_list, (_md));	\
  } while (0)

/* Add a metadata match with a mask to a flow. */
#define FLOW_ADD_METADATA_W_MATCH(_fl, _md, _mask)		\
  do {								\
    add_metadata_w_match(&(_fl)->match_list, (_md), (_mask));	\
  } while (0)

/* Add an MPLS label match to a flow. */
#define FLOW_ADD_MPLS_LABEL_MATCH(_fl, _label)		\
  do {							\
    add_mpls_label_match(&(_fl)->match_list, (_label));	\
  } while (0)

/* Add an MPLS TC match to a flow. */
#define FLOW_ADD_MPLS_TC_MATCH(_fl, _tc)		\
  do {							\
    add_mpls_tc_match(&(_fl)->match_list, (_tc));	\
  } while (0)

/* Add an MPLS BOS match to a flow. */
#define FLOW_ADD_MPLS_BOS_MATCH(_fl, _bos)		\
  do {							\
    add_mpls_bos_match(&(_fl)->match_list, (_bos));	\
  } while (0)

/* Add a TCP source port match to a flow. */
#define FLOW_ADD_TCP_SRC_MATCH(_fl, _port)		\
  do {							\
    add_tcp_src_match(&(_fl)->match_list, (_port));	\
  } while (0)

/* Add a TCP destination port match to a flow. */
#define FLOW_ADD_TCP_DST_MATCH(_fl, _port)		\
  do {							\
    add_tcp_dst_match(&(_fl)->match_list, (_port));	\
  } while (0)

/* Add a UDP source port match to a flow. */
#define FLOW_ADD_UDP_SRC_MATCH(_fl, _port)		\
  do {							\
    add_udp_src_match(&(_fl)->match_list, (_port));	\
  } while (0)

/* Add a UDP destination port match to a flow. */
#define FLOW_ADD_UDP_DST_MATCH(_fl, _port)		\
  do {							\
    add_udp_dst_match(&(_fl)->match_list, (_port));	\
  } while (0)

/* Add an SCTP source port match to a flow. */
#define FLOW_ADD_SCTP_SRC_MATCH(_fl, _port)		\
  do {							\
    add_sctp_src_match(&(_fl)->match_list, (_port));	\
  } while (0)

/* Add an SCTP destination port match to a flow. */
#define FLOW_ADD_SCTP_DST_MATCH(_fl, _port)		\
  do {							\
    add_sctp_dst_match(&(_fl)->match_list, (_port));	\
  } while (0)

/* Add an ICMP type match to a flow. */
#define FLOW_ADD_ICMP_TYPE_MATCH(_fl, _type)		\
  do {							\
    add_icmp_type_match(&(_fl)->match_list, (_type));	\
  } while (0)

/* Add an ICMP code match to a flow. */
#define FLOW_ADD_ICMP_CODE_MATCH(_fl, _code)		\
  do {							\
    add_icmp_code_match(&(_fl)->match_list, (_code));	\
  } while (0)

/* Add an ICMPv6 type match to a flow. */
#define FLOW_ADD_ICMPV6_TYPE_MATCH(_fl, _type)		\
  do {							\
    add_icmpv6_type_match(&(_fl)->match_list, (_type));	\
  } while (0)

/* Add an ICMPv6 code match to a flow. */
#define FLOW_ADD_ICMPV6_CODE_MATCH(_fl, _code)		\
  do {							\
    add_icmpv6_code_match(&(_fl)->match_list, (_code));	\
  } while (0)

/* Add a tunnel ID match to a flow. */
#define FLOW_ADD_TUNNELID_MATCH(_fl, _id)		\
  do {							\
    add_tunnelid_match(&(_fl)->match_list, (_id));	\
  } while (0)

/* Add a tunnel ID match with a mask to a flow. */
#define FLOW_ADD_TUNNELID_W_MATCH(_fl, _id, _mask)		\
  do {								\
    add_tunnelid_w_match(&(_fl)->match_list, (_id), (_mask));	\
  } while (0)

/* Add a VLAN VID match to a flow. */
#define FLOW_ADD_VLAN_VID_MATCH(_fl, _vid)		\
  do {							\
    TEST_ASSERT_VLAN_VID(_vid);				\
    add_vlan_vid_match(&(_fl)->match_list, (_vid));	\
  } while (0)

/* Add a VLAN VID match with a mask to a flow. */
#define FLOW_ADD_VLAN_VID_W_MATCH(_fl, _vid, _mask)		\
  do {								\
    TEST_ASSERT_VLAN_VID(_vid);					\
    TEST_ASSERT_VLAN_VID(_mask);				\
    add_vlan_vid_w_match(&(_fl)->match_list, (_vid), (_mask));	\
  } while (0)

/* Add a VLAN PCP match to a flow. */
#define FLOW_ADD_VLAN_PCP_MATCH(_fl, _pcp)		\
  do {							\
    add_vlan_pcp_match(&(_fl)->match_list, (_pcp));	\
  } while (0)


#endif /* __DATAPLANE_TEST_MISC_MACROS_H__ */
