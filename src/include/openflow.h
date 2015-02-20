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


/**
 * @file        openflow.h
 * @brief       Openflow common header.
 */

#ifndef __OPENFLOW_H__
#define __OPENFLOW_H__

/* OpenFlow Wire Protocol version. */
#define OPENFLOW_VERSION_0_0         0x00
#define OPENFLOW_VERSION_1_0         0x01
#define OPENFLOW_VERSION_1_1         0x02
#define OPENFLOW_VERSION_1_2         0x03
#define OPENFLOW_VERSION_1_3         0x04
#define OPENFLOW_VERSION_1_4         0x05

/* Constants. */
#define OFP_BUFSIZ                   4096
#define OFP_ETH_ALEN                    6
#define OFP_MAX_PORT_NAME_LEN          16
#define OFPMT_STANDARD_LENGTH          88
#define DESC_STR_LEN                  256
#define SERIAL_NUM_LEN                 32
#define OFP_MAX_TABLE_NAME_LEN         32

#define OFP_HEADER_LEN                  8

#define OFP_PACKET_MAX_SIZE (64*1024) - 1 /* 64KB */
#define OFP_ERROR_MAX_SIZE             64 /* 64B */

#define OXM_HEADER(class, field, length) \
  (uint32_t)(((class) << 16) | ((field) << 9) | (length))
#define OXM_HEADER_W(class, field, length) \
  (uint32_t)(((class) << 16) | ((field) << 9) | (1 << 8) | ((length) * 2))

#define OXM_OF_IN_PORT     OXM_HEADER  (0x8000, OFPXMT_OFB_IN_PORT, 4)
#define OXM_OF_IN_PHY_PORT OXM_HEADER  (0x8000, OFPXMT_OFB_IN_PHY_PORT, 4)
#define OXM_OF_METADATA    OXM_HEADER  (0x8000, OFPXMT_OFB_METADATA, 8)
#define OXM_OF_METADATA_W  OXM_HEADER_W(0x8000, OFPXMT_OFB_METADATA, 8)
#define OXM_OF_ETH_DST     OXM_HEADER  (0x8000, OFPXMT_OFB_ETH_DST, 6)
#define OXM_OF_ETH_DST_W   OXM_HEADER_W(0x8000, OFPXMT_OFB_ETH_DST, 6)
#define OXM_OF_ETH_SRC     OXM_HEADER  (0x8000, OFPXMT_OFB_ETH_SRC, 6)
#define OXM_OF_ETH_SRC_W   OXM_HEADER_W(0x8000, OFPXMT_OFB_ETH_SRC, 6)
#define OXM_OF_ETH_TYPE    OXM_HEADER  (0x8000, OFPXMT_OFB_ETH_TYPE, 2)
#define OXM_OF_VLAN_VID    OXM_HEADER  (0x8000, OFPXMT_OFB_VLAN_VID, 2)
#define OXM_OF_VLAN_VID_W  OXM_HEADER_W(0x8000, OFPXMT_OFB_VLAN_VID, 2)
#define OXM_OF_VLAN_PCP    OXM_HEADER  (0x8000, OFPXMT_OFB_VLAN_PCP, 1)
#define OXM_OF_IP_DSCP     OXM_HEADER  (0x8000, OFPXMT_OFB_IP_DSCP, 1)
#define OXM_OF_IP_ECN      OXM_HEADER  (0x8000, OFPXMT_OFB_IP_ECN, 1)
#define OXM_OF_IP_PROTO    OXM_HEADER  (0x8000, OFPXMT_OFB_IP_PROTO, 1)
#define OXM_OF_IPV4_SRC    OXM_HEADER  (0x8000, OFPXMT_OFB_IPV4_SRC, 4)
#define OXM_OF_IPV4_SRC_W  OXM_HEADER_W(0x8000, OFPXMT_OFB_IPV4_SRC, 4)
#define OXM_OF_IPV4_DST    OXM_HEADER  (0x8000, OFPXMT_OFB_IPV4_DST, 4)
#define OXM_OF_IPV4_DST_W  OXM_HEADER_W(0x8000, OFPXMT_OFB_IPV4_DST, 4)
#define OXM_OF_TCP_SRC     OXM_HEADER  (0x8000, OFPXMT_OFB_TCP_SRC, 2)
#define OXM_OF_TCP_DST     OXM_HEADER  (0x8000, OFPXMT_OFB_TCP_DST, 2)
#define OXM_OF_UDP_SRC     OXM_HEADER  (0x8000, OFPXMT_OFB_UDP_SRC, 2)
#define OXM_OF_UDP_DST     OXM_HEADER  (0x8000, OFPXMT_OFB_UDP_DST, 2)
#define OXM_OF_SCTP_SRC    OXM_HEADER  (0x8000, OFPXMT_OFB_SCTP_SRC, 2)
#define OXM_OF_SCTP_DST    OXM_HEADER  (0x8000, OFPXMT_OFB_SCTP_DST, 2)
#define OXM_OF_ICMPV4_TYPE OXM_HEADER  (0x8000, OFPXMT_OFB_ICMPV4_TYPE, 1)
#define OXM_OF_ICMPV4_CODE OXM_HEADER  (0x8000, OFPXMT_OFB_ICMPV4_CODE, 2)
#define OXM_OF_ARP_OP      OXM_HEADER  (0x8000, OFPXMT_OFB_ARP_OP, 2)
#define OXM_OF_ARP_SPA     OXM_HEADER  (0x8000, OFPXMT_OFB_ARP_SPA, 4)
#define OXM_OF_ARP_SPA_W   OXM_HEADER_W(0x8000, OFPXMT_OFB_ARP_SPA, 4)
#define OXM_OF_ARP_TPA     OXM_HEADER  (0x8000, OFPXMT_OFB_ARP_TPA, 4)
#define OXM_OF_ARP_TPA_W   OXM_HEADER_W(0x8000, OFPXMT_OFB_ARP_TPA, 4)
#define OXM_OF_ARP_SHA     OXM_HEADER  (0x8000, OFPXMT_OFB_ARP_SHA, 6)
#define OXM_OF_ARP_THA     OXM_HEADER  (0x8000, OFPXMT_OFB_ARP_THA, 6)
#define OXM_OF_IPV6_SRC    OXM_HEADER  (0x8000, OFPXMT_OFB_IPV6_SRC, 16)
#define OXM_OF_IPV6_SRC_W  OXM_HEADER_W(0x8000, OFPXMT_OFB_IPV6_SRC, 16)
#define OXM_OF_IPV6_DST    OXM_HEADER  (0x8000, OFPXMT_OFB_IPV6_DST, 16)
#define OXM_OF_IPV6_DST_W  OXM_HEADER_W(0x8000, OFPXMT_OFB_IPV6_DST, 16)
#define OXM_OF_IPV6_FLABEL OXM_HEADER  (0x8000, OFPXMT_OFB_IPV6_FLABEL, 4)
#define OXM_OF_ICMPV6_TYPE OXM_HEADER  (0x8000, OFPXMT_OFB_ICMPV6_TYPE, 1)
#define OXM_OF_ICMPV6_CODE OXM_HEADER  (0x8000, OFPXMT_OFB_ICMPV6_CODE, 1)
#define OXM_OF_IPV6_ND_TARGET OXM_HEADER(0x8000, OFPXMT_OFB_IPV6_ND_TARGET, 16)
#define OXM_OF_IPV6_ND_SLL OXM_HEADER  (0x8000, OFPXMT_OFB_IPV6_ND_SLL, 6)
#define OXM_OF_IPV6_ND_TLL OXM_HEADER  (0x8000, OFPXMT_OFB_IPV6_ND_TLL, 6)
#define OXM_OF_MPLS_LABEL  OXM_HEADER  (0x8000, OFPXMT_OFB_MPLS_LABEL, 4)
#define OXM_OF_MPLS_TC     OXM_HEADER  (0x8000, OFPXMT_OFB_MPLS_TC, 1)
#define OXM_OF_MPLS_BOS    OXM_HEADER  (0x8000, OFPXMT_OFB_MPLS_BOS, 1)
#define OXM_OF_PBB_ISID    OXM_HEADER  (0x8000, OFPXMT_OFB_PBB_ISID, 3)
#define OXM_OF_PBB_ISID_W  OXM_HEADER_W(0x8000, OFPXMT_OFB_PBB_ISID, 3)
#define OXM_OF_TUNNEL_ID   OXM_HEADER  (0x8000, OFPXMT_OFB_TUNNEL_ID, 8)
#define OXM_OF_TUNNEL_ID_W OXM_HEADER_W(0x8000, OFPXMT_OFB_TUNNEL_ID, 8)
#define OXM_OF_IPV6_EXTHDR OXM_HEADER  (0x8000, OFPXMT_OFB_IPV6_EXTHDR, 2)
#define OXM_OF_IPV6_EXTHDR_W OXM_HEADER_W(0x8000, OFPXMT_OFB_IPV6_EXTHDR, 2)

/* Include each version header. */
#include "openflow13.h"

struct ofp_error {
  uint16_t type;
  uint16_t code;
  union {
    const char *str;
    struct pbuf *req;
  };
};

#endif /* __OPENFLOW_H__ */
