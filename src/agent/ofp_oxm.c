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

#include <stdbool.h>
#include <stdint.h>
#include "lagopus_apis.h"
#include "openflow.h"
#include "openflow13packet.h"
#include "ofp_oxm.h"

lagopus_result_t
ofp_oxm_field_check(uint8_t oxm_field) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  switch (OXM_FIELD_TYPE(oxm_field)) {
    case OFPXMT_OFB_IN_PORT:
    case OFPXMT_OFB_IN_PHY_PORT:
    case OFPXMT_OFB_METADATA:
    case OFPXMT_OFB_ETH_DST:
    case OFPXMT_OFB_ETH_SRC:
    case OFPXMT_OFB_ETH_TYPE:
    case OFPXMT_OFB_VLAN_VID:
    case OFPXMT_OFB_VLAN_PCP:
    case OFPXMT_OFB_IP_DSCP:
    case OFPXMT_OFB_IP_ECN:
    case OFPXMT_OFB_IP_PROTO:
    case OFPXMT_OFB_IPV4_SRC:
    case OFPXMT_OFB_IPV4_DST:
    case OFPXMT_OFB_TCP_SRC:
    case OFPXMT_OFB_TCP_DST:
    case OFPXMT_OFB_UDP_SRC:
    case OFPXMT_OFB_UDP_DST:
    case OFPXMT_OFB_SCTP_SRC:
    case OFPXMT_OFB_SCTP_DST:
    case OFPXMT_OFB_ICMPV4_TYPE:
    case OFPXMT_OFB_ICMPV4_CODE:
    case OFPXMT_OFB_ARP_OP:
    case OFPXMT_OFB_ARP_SPA:
    case OFPXMT_OFB_ARP_TPA:
    case OFPXMT_OFB_ARP_SHA:
    case OFPXMT_OFB_ARP_THA:
    case OFPXMT_OFB_IPV6_SRC:
    case OFPXMT_OFB_IPV6_DST:
    case OFPXMT_OFB_IPV6_FLABEL:
    case OFPXMT_OFB_ICMPV6_TYPE:
    case OFPXMT_OFB_ICMPV6_CODE:
    case OFPXMT_OFB_IPV6_ND_TARGET:
    case OFPXMT_OFB_IPV6_ND_SLL:
    case OFPXMT_OFB_IPV6_ND_TLL:
    case OFPXMT_OFB_MPLS_LABEL:
    case OFPXMT_OFB_MPLS_TC:
    case OFPXMT_OFB_MPLS_BOS:
    case OFPXMT_OFB_PBB_ISID:
    case OFPXMT_OFB_TUNNEL_ID:
    case OFPXMT_OFB_IPV6_EXTHDR:
    case OFPXMT_OFB_PBB_UCA:
    case OFPXMT_OFB_PACKET_TYPE:
    case OFPXMT_OFB_GRE_FLAGS:
    case OFPXMT_OFB_GRE_VER:
    case OFPXMT_OFB_GRE_PROTOCOL:
    case OFPXMT_OFB_GRE_KEY:
    case OFPXMT_OFB_GRE_SEQNUM:
    case OFPXMT_OFB_LISP_FLAGS:
    case OFPXMT_OFB_LISP_NONCE:
    case OFPXMT_OFB_LISP_ID:
    case OFPXMT_OFB_VXLAN_FLAGS:
    case OFPXMT_OFB_VXLAN_VNI:
    case OFPXMT_OFB_MPLS_DATA_FIRST_NIBBLE:
    case OFPXMT_OFB_MPLS_ACH_VERSION:
    case OFPXMT_OFB_MPLS_ACH_CHANNEL:
    case OFPXMT_OFB_MPLS_PW_METADATA:
    case OFPXMT_OFB_MPLS_CW_FLAGS:
    case OFPXMT_OFB_MPLS_CW_FRAG:
    case OFPXMT_OFB_MPLS_CW_LEN:
    case OFPXMT_OFB_MPLS_CW_SEQ_NUM:
    case OFPXMT_OFB_GTPU_FLAGS:
    case OFPXMT_OFB_GTPU_VER:
    case OFPXMT_OFB_GTPU_MSGTYPE:
    case OFPXMT_OFB_GTPU_TEID:
    case OFPXMT_OFB_GTPU_EXTN_HDR:
    case OFPXMT_OFB_GTPU_EXTN_UDP_PORT:
    case OFPXMT_OFB_GTPU_EXTN_SCI:
      ret = LAGOPUS_RESULT_OK;
      break;
    default:
      lagopus_msg_warning("bad field (%"PRIu8").\n", oxm_field);
      ret = LAGOPUS_RESULT_OXM_ERROR;
      break;
  }

  return ret;
}
