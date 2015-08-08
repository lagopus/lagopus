/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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

#include "lagopus_apis.h"
#include "datastore_apis.h"
#include "datastore_internal.h"
#include "cmd_common.h"
#include "cmd_dump.h"
#include "flow_cmd.h"
#include "flow_cmd_internal.h"
#include "conv_json.h"
#include "lagopus/flowdb.h"
#include "lagopus/ethertype.h"
#include "lagopus/dp_apis.h"

#define TMP_FILE "/.lagopus_flow_XXXXXX"
#define PORT_STR_BUF_SIZE 32

static lagopus_hashmap_t thread_table = NULL;
static lagopus_mutex_t lock = NULL;
static char dump_tmp_dir[PATH_MAX] = DATASTORE_TMP_DIR;

typedef struct args {
  flow_conf_t *conf;
  datastore_interp_t *iptr;
  datastore_config_type_t ftype;
  void *stream_out;
  datastore_printf_proc_t printf_proc;
  char file_name[PATH_MAX];
} args_t;

static inline const char *
port_string(uint32_t val, char *buf) {
  int size;

  switch (val) {
    case OFPP_IN_PORT:
      return "\"in_port\"";
    case OFPP_TABLE:
      return "\"table\"";
    case OFPP_NORMAL:
      return "\"normal\"";
    case OFPP_FLOOD:
      return "\"flood\"";
    case OFPP_ALL:
      return "\"all\"";
    case OFPP_CONTROLLER:
      return "\"controller\"";
    case OFPP_LOCAL:
      return "\"local\"";
    default:
      size = snprintf(buf, PORT_STR_BUF_SIZE, "%u", val);
      if (size < PORT_STR_BUF_SIZE) {
        return buf;
      }
      return "";
  }
}

static inline lagopus_result_t
dump_match(struct match *match,
           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t val16;
  uint32_t val32;
  uint64_t val64;
  char ip_str[INET6_ADDRSTRLEN];
  int field_type = match->oxm_field >> 1;
  int hasmask = match->oxm_field & 1;
  char buf[PORT_STR_BUF_SIZE];
  unsigned int i;

  switch (field_type) {
    case OFPXMT_OFB_IN_PORT:
      memcpy(&val32, match->oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%s"),
                   "in_port", port_string(ntohl(val32), buf))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IN_PHY_PORT:
      memcpy(&val32, match->oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%s"),
                   "in_phy_port", port_string(ntohl(val32), buf))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_METADATA:
      val64 = 0;
      for (i = 0; i < sizeof(uint64_t); i++) {
        val64 <<= 8;
        val64 |= match->oxm_value[i];
      }
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu64),
                   "metadata", val64)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/:0x%02x%02x%02x%02x%02x%02x%02x%02x",
                     match->oxm_value[8], match->oxm_value[9],
                     match->oxm_value[10], match->oxm_value[11],
                     match->oxm_value[12], match->oxm_value[13],
                     match->oxm_value[14], match->oxm_value[15])) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "\"")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ETH_DST:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x"),
                   "eth_dst",
                   match->oxm_value[0], match->oxm_value[1],
                   match->oxm_value[2], match->oxm_value[3],
                   match->oxm_value[4], match->oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/0x%02x%02x%02x%02x%02x%02x",
                     match->oxm_value[6], match->oxm_value[7],
                     match->oxm_value[8], match->oxm_value[9],
                     match->oxm_value[10], match->oxm_value[11])) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "\"")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ETH_SRC:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x"),
                   "eth_src",
                   match->oxm_value[0], match->oxm_value[1],
                   match->oxm_value[2], match->oxm_value[3],
                   match->oxm_value[4], match->oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/0x%02x%02x%02x%02x%02x%02x",
                     match->oxm_value[6], match->oxm_value[7],
                     match->oxm_value[8], match->oxm_value[9],
                     match->oxm_value[10], match->oxm_value[11])) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "\"")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ETH_TYPE:
      switch ((match->oxm_value[0] << 8) | match->oxm_value[1]) {
        case ETHERTYPE_ARP:
          if ((ret = lagopus_dstring_appendf(
                       result, DELIMITER_INSTERN(KEY_FMT "\"arp\""),
                       "eth_type")) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_IP:
          if ((ret = lagopus_dstring_appendf(
                       result, DELIMITER_INSTERN(KEY_FMT "\"ip\""),
                       "eth_type")) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_IPV6:
          if ((ret = lagopus_dstring_appendf(
                       result, DELIMITER_INSTERN(KEY_FMT "\"ipv6\""),
                       "eth_type")) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_MPLS:
          if ((ret = lagopus_dstring_appendf(
                       result, DELIMITER_INSTERN(KEY_FMT "\"mpls\""),
                       "eth_type")) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_MPLS_MCAST:
          if ((ret = lagopus_dstring_appendf(
                       result, DELIMITER_INSTERN(KEY_FMT "\"mplsmc\""),
                       "eth_type")) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_PBB:
          if ((ret = lagopus_dstring_appendf(
                       result, DELIMITER_INSTERN(KEY_FMT "\"pbb\""),
                       "eth_type")) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        default:
          if ((ret = lagopus_dstring_appendf(
                       result, DELIMITER_INSTERN(KEY_FMT "\"0x%02x%02x\""),
                       "eth_type",
                       match->oxm_value[0], match->oxm_value[1])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
      }
      break;
    case OFPXMT_OFB_VLAN_VID:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%d"),
                   "lan_vid",
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        memcpy(&val16, &match->oxm_value[2], sizeof(uint16_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/0x%02x",
                     ntohs(val16))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "\"")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_VLAN_PCP:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "vlan_pcp", *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IP_DSCP:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "ip_dscp", *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IP_ECN:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "ip_ecn", *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IP_PROTO:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "ip_proto", *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV4_SRC:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%s"),
                   "ipv4_src",
                   inet_ntop(AF_INET, match->oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        memcpy(&val32, &match->oxm_value[4], sizeof(uint32_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/0x%04x",
                     ntohl(val32))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "\"")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV4_DST:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%s"),
                   "ipv4_dst",
                   inet_ntop(AF_INET, match->oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        memcpy(&val32, &match->oxm_value[4], sizeof(uint32_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/0x%04x",
                     ntohl(val32))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "\"")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_TCP_SRC:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "tcp_src",
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_TCP_DST:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "tcp_dst",
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_UDP_SRC:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "udp_src",
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_UDP_DST:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "udp_dst",
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_SCTP_SRC:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "sctp_src",
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_SCTP_DST:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "sctp_dst",
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ICMPV4_TYPE:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "icmpv4_type",
                   *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ICMPV4_CODE:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "icmpv4_code",
                   *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_OP:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "arp_op",
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_SPA:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%s"),
                   "arp_spa",
                   inet_ntop(AF_INET, match->oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        memcpy(&val32, &match->oxm_value[4], sizeof(uint32_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/0x%04x",
                     ntohl(val32))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "\"")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_TPA:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%s"),
                   "arp_tpa",
                   inet_ntop(AF_INET, match->oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        memcpy(&val32, &match->oxm_value[4], sizeof(uint32_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/0x%04x",
                     ntohl(val32))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "\"")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_SHA:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x"),
                   "arp_sha",
                   match->oxm_value[0], match->oxm_value[1],
                   match->oxm_value[2], match->oxm_value[3],
                   match->oxm_value[4], match->oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/0x%02x%02x%02x%02x%02x%02x",
                     match->oxm_value[6], match->oxm_value[7],
                     match->oxm_value[8], match->oxm_value[9],
                     match->oxm_value[10], match->oxm_value[11])) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "\"")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_THA:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x"),
                   "arp_tha",
                   match->oxm_value[0], match->oxm_value[1],
                   match->oxm_value[2], match->oxm_value[3],
                   match->oxm_value[4], match->oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/0x%02x%02x%02x%02x%02x%02x",
                     match->oxm_value[6], match->oxm_value[7],
                     match->oxm_value[8], match->oxm_value[9],
                     match->oxm_value[10], match->oxm_value[11])) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "\"")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_SRC:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%s"),
                   "ipv6_src",
                   inet_ntop(AF_INET6, match->oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        memcpy(&val32, &match->oxm_value[16], sizeof(uint32_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/0x%04x", ntohl(val32))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
        memcpy(&val32, &match->oxm_value[20], sizeof(uint32_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "%04x", ntohl(val32))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
        memcpy(&val32, &match->oxm_value[24], sizeof(uint32_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "%04x", ntohl(val32))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
        memcpy(&val32, &match->oxm_value[28], sizeof(uint32_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "%04x", ntohl(val32))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "\"")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_DST:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%s"),
                   "ipv6_dst",
                   inet_ntop(AF_INET6, match->oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        memcpy(&val32, &match->oxm_value[16], sizeof(uint32_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/0x%04x", ntohl(val32))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
        memcpy(&val32, &match->oxm_value[20], sizeof(uint32_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "%04x", ntohl(val32))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
        memcpy(&val32, &match->oxm_value[24], sizeof(uint32_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "%04x", ntohl(val32))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
        memcpy(&val32, &match->oxm_value[28], sizeof(uint32_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "%04x", ntohl(val32))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "\"")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_FLABEL:
      memcpy(&val32, match->oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "ipv6_flabel",
                   ntohl(val32))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ICMPV6_TYPE:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "icmpv6_type",
                   *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ICMPV6_CODE:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "icmpv6_code",
                   *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_ND_TARGET:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%s\""),
                   "ipv6_nd_target",
                   inet_ntop(AF_INET6, match->oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_ND_SLL:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x\""),
                   "ipv6_nd_sll",
                   match->oxm_value[0], match->oxm_value[1],
                   match->oxm_value[2], match->oxm_value[3],
                   match->oxm_value[4], match->oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_ND_TLL:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x\""),
                   "ipv6_nd_tll",
                   match->oxm_value[0], match->oxm_value[1],
                   match->oxm_value[2], match->oxm_value[3],
                   match->oxm_value[4], match->oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_LABEL:
      val32 = match_mpls_label_host_get(match);
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "mpls_label",
                   val32)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_TC:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "mpls_tc",
                   *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_BOS:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%d"),
                   "mpls_bos",
                   *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_PBB_ISID:
      val32 = match->oxm_value[0];
      val32 = (val32 << 8) | match->oxm_value[1];
      val32 = (val32 << 8) | match->oxm_value[2];
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%d"),
                   "pbb_isid=",
                   val32)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/0x%02x%02x%02x",
                     match->oxm_value[3],
                     match->oxm_value[4],
                     match->oxm_value[5])) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "\"")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_TUNNEL_ID:
      val64 = 0;
      for (i = 0; i < sizeof(uint64_t); i++) {
        val64 <<= 8;
        val64 |= match->oxm_value[i];
      }
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu64),
                   "tunnel_id",
                   val64)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        memcpy(&val32, &match->oxm_value[8], sizeof(uint32_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/0x%04x",
                     ntohl(val32))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
        memcpy(&val32, &match->oxm_value[12], sizeof(uint32_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "%04x",
                     ntohl(val32))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "\"")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_EXTHDR:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%d"),
                   "ipv6_exthdr",
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        memcpy(&val16, &match->oxm_value[2], sizeof(uint16_t));
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/0x%02x",
                     ntohs(val16))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "\"")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    default:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "null"),
                   "unknown")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
  }

done:
  return ret;
}

static inline lagopus_result_t
dump_set_field(uint8_t *oxm,
               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t *oxm_value;
  uint16_t val16;
  uint32_t val32;
  uint64_t val64;
  char ip_str[INET6_ADDRSTRLEN];
  char buf[PORT_STR_BUF_SIZE];
  int field_type = oxm[2] >> 1;
  unsigned int i;

  oxm_value = &oxm[4];

  if ((ret = lagopus_dstring_appendf(
               result, "{")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  switch (field_type) {
    case OFPXMT_OFB_IN_PORT:
      memcpy(&val32, oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%s",
                   "in_port", port_string(ntohl(val32), buf))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IN_PHY_PORT:
      memcpy(&val32, oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%s",
                   "in_phy_port", port_string(ntohl(val32), buf))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_METADATA:
      val64 = 0;
      for (i = 0; i < sizeof(uint64_t); i++) {
        val64 <<= 8;
        val64 |= oxm_value[i];
      }
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu64,
                   "metadata", val64)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ETH_DST:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x\"",
                   "eth_dst",
                   oxm_value[0], oxm_value[1],
                   oxm_value[2], oxm_value[3],
                   oxm_value[4], oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ETH_SRC:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x\"",
                   "eth_src",
                   oxm_value[0], oxm_value[1],
                   oxm_value[2], oxm_value[3],
                   oxm_value[4], oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ETH_TYPE:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"0x%02x%02x\"",
                   "eth_type",
                   oxm_value[0], oxm_value[1])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_VLAN_VID:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "vlan_vid", ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_VLAN_PCP:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "vlan_pcp", *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IP_DSCP:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "ip_dscp", *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IP_ECN:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "ip_ecn", *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IP_PROTO:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "ip_proto", *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV4_SRC:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%s\"",
                   "ipv4_src",
                   inet_ntop(AF_INET, oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV4_DST:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%s\"",
                   "ipv4_dst",
                   inet_ntop(AF_INET, oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_TCP_SRC:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "tcp_src", ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_TCP_DST:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "tcp_dst", ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_UDP_SRC:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "udp_src", ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_UDP_DST:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "udp_dst", ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_SCTP_SRC:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "sctp_src", ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_SCTP_DST:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "sctp_dst", ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ICMPV4_TYPE:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "icmpv4_type", *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ICMPV4_CODE:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "icmpv4_code", *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_OP:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "arp_op", ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_SPA:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%s\"",
                   "arp_spa",
                   inet_ntop(AF_INET, oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_TPA:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%s\"",
                   "arp_tpa",
                   inet_ntop(AF_INET, oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_SHA:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x\"",
                   "arp_sha",
                   oxm_value[0], oxm_value[1],
                   oxm_value[2], oxm_value[3],
                   oxm_value[4], oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_THA:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x\"",
                   "arp_tha",
                   oxm_value[0], oxm_value[1],
                   oxm_value[2], oxm_value[3],
                   oxm_value[4], oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_SRC:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%s\"",
                   "ipv6_src",
                   inet_ntop(AF_INET6, oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_DST:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%s\"",
                   "ipv6_dst",
                   inet_ntop(AF_INET6, oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_FLABEL:
      memcpy(&val32, oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "ipv6_dst", ntohl(val32))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ICMPV6_TYPE:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "icmpv6_type", *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ICMPV6_CODE:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "icmpv6_code", *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_ND_TARGET:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%s\"",
                   "ipv6_nd_target",
                   inet_ntop(AF_INET6, oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_ND_SLL:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x\"",
                   "ipv6_nd_sll",
                   oxm_value[0], oxm_value[1],
                   oxm_value[2], oxm_value[3],
                   oxm_value[4], oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_ND_TLL:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x\"",
                   "ipv6_nd_tll",
                   oxm_value[0], oxm_value[1],
                   oxm_value[2], oxm_value[3],
                   oxm_value[4], oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_LABEL:
      memcpy(&val32, oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "mpls_label", ntohl(val32))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_TC:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "mpls_tc", *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_BOS:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "mpls_bos", *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_PBB_ISID:
      val32 = oxm_value[0];
      val32 = (val32 << 8) | oxm_value[1];
      val32 = (val32 << 8) | oxm_value[2];
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "pbb_isid", val32)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_TUNNEL_ID:
      val64 = 0;
      for (i = 0; i < sizeof(uint64_t); i++) {
        val64 <<= 8;
        val64 |= oxm_value[i];
      }
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu64,
                   "tunnel_id", val64)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_EXTHDR:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%d",
                   "ipv6_exthdr", ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    default:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "null",
                   "unknown")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
  }

  if ((ret = lagopus_dstring_appendf(
               result, "}")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

done:
  return ret;
}

lagopus_result_t
flow_cmd_dump_action(struct action *action,
                     bool is_action_first,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char buf[PORT_STR_BUF_SIZE];

  if ((ret = lagopus_dstring_appendf(
               result, DS_JSON_DELIMITER(is_action_first, ""))) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  switch (action->ofpat.type) {
    case OFPAT_OUTPUT:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "%s}",
                   "output",
                   port_string(((struct ofp_action_output *)
                                &action->ofpat)->port, buf))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_COPY_TTL_OUT:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "null}",
                   "copy_ttl_out")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_COPY_TTL_IN:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "null}",
                   "copy_ttl_in")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_SET_MPLS_TTL:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "%d}",
                   "set_mpls_ttl",
                   ((struct ofp_action_mpls_ttl *)
                    &action->ofpat)->mpls_ttl)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_DEC_MPLS_TTL:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "null}",
                   "dec_mpls_ttl")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_PUSH_VLAN:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "\"0x%04x\"}",
                   "push_vlan",
                   ((struct ofp_action_push *)
                    &action->ofpat)->ethertype)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_POP_VLAN:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "null}",
                   "pop_vlan")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_PUSH_MPLS:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "\"0x%04x\"}",
                   "push_mpls",
                   ((struct ofp_action_push *)
                    &action->ofpat)->ethertype)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_POP_MPLS:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "\"0x%04x\"}",
                   "pop_mpls",
                   ((struct ofp_action_push *)
                    &action->ofpat)->ethertype)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_SET_QUEUE:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "null}",
                   "set_queue")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_GROUP:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "%d}",
                   "group",
                   ((struct ofp_action_group *)
                    &action->ofpat)->group_id)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_SET_NW_TTL:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "%d}",
                   "set_nw_ttl",
                   ((struct ofp_action_nw_ttl *)
                    &action->ofpat)->nw_ttl)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_DEC_NW_TTL:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "null}",
                   "dec_nw_ttl")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_SET_FIELD:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT,
                   "set_field")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if ((ret = dump_set_field(
                   ((struct ofp_action_set_field *)
                    &action->ofpat)->field,
                   result)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "}")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_PUSH_PBB:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "\"0x%04x\"}",
                   "push_pbb",
                   ((struct ofp_action_push *)&action->ofpat)->ethertype)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_POP_PBB:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "null}",
                   "pop_pbb")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    default:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "null}",
                   "unknown")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
  }

done:
  return ret;
}

static inline lagopus_result_t
dump_instruction(struct instruction *instruction,
                 bool is_instruction_first,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct action *action;
  uint64_t val64;
  bool is_action_first = true;
  unsigned int i;
  uint8_t *p;

  if ((ret = lagopus_dstring_appendf(
               result, DS_JSON_DELIMITER(is_instruction_first, ""))) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  switch (instruction->ofpit.type) {
    case OFPIT_GOTO_TABLE:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "%d}",
                   "goto_table",
                   instruction->ofpit_goto_table.table_id)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPIT_WRITE_METADATA:
      val64 = 0;
      p = (uint8_t *)&instruction->ofpit_write_metadata.metadata;
      for (i = 0; i < sizeof(uint64_t); i++) {
        val64 <<= 8;
        val64 |= p[i];
      }
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "\"0x%"PRIx64"\"}",
                   "write_metadata",
                   val64)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPIT_WRITE_ACTIONS:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT,
                   "write_action")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if ((ret = lagopus_dstring_appendf(
                   result, "[")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if (TAILQ_EMPTY(&instruction->action_list) == false) {
        is_action_first = true;
        TAILQ_FOREACH(action, &instruction->action_list, entry) {
          if ((ret = flow_cmd_dump_action(action, is_action_first, result)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          if (is_action_first == true) {
            is_action_first = false;
          }
        }
      }

      if ((ret = lagopus_dstring_appendf(
                   result, "]}")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPIT_APPLY_ACTIONS:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT,
                   "apply_action")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if ((ret = lagopus_dstring_appendf(
                   result, "[")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if (TAILQ_EMPTY(&instruction->action_list) == false) {
        is_action_first = true;
        TAILQ_FOREACH(action, &instruction->action_list, entry) {
          if ((ret = flow_cmd_dump_action(action, is_action_first, result)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          if (is_action_first == true) {
            is_action_first = false;
          }
        }
      }

      if ((ret = lagopus_dstring_appendf(
                   result, "]}")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPIT_CLEAR_ACTIONS:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "null}",
                   "clear_actions")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPIT_METER:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "%d}",
                   "meter",
                   instruction->ofpit_meter.meter_id)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPIT_EXPERIMENTER:
    default:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "null}",
                   "unknown")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
  }

done:
  return ret;
}

static inline lagopus_result_t
dump_flow_stats(struct flow *flow,
                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((ret = lagopus_dstring_appendf(
               result, DELIMITER_INSTERN(KEY_FMT "%u"),
               "idle_timeout",
               flow->idle_timeout)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
               result, DELIMITER_INSTERN(KEY_FMT "%u"),
               "hard_timeout",
               flow->hard_timeout)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
               result, DELIMITER_INSTERN(KEY_FMT "%u"),
               "flags",
               flow->flags)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
               result, DELIMITER_INSTERN(KEY_FMT "%"PRIu64),
               "cookie",
               flow->cookie)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
               result, DELIMITER_INSTERN(KEY_FMT "%"PRIu64),
               "packet_count",
               flow->packet_count)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
               result, DELIMITER_INSTERN(KEY_FMT "%"PRIu64),
               "byte_count",
               flow->byte_count)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

done:
  return ret;
}

static inline lagopus_result_t
dump_flow(struct flow *flow,
          bool *is_flow_first,
          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct match *match;
  struct instruction *instruction;
  bool is_instruction_first = true;

  if ((ret = lagopus_dstring_appendf(
               result, DS_JSON_DELIMITER(*is_flow_first, "{"))) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
               result, KEY_FMT "%d",
               "priority",
               flow->priority)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = dump_flow_stats(flow, result)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if (TAILQ_EMPTY(&flow->match_list) == false) {
    TAILQ_FOREACH(match, &flow->match_list, entry) {
      if ((ret = dump_match(match, result)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
    }
  }

  if ((ret = lagopus_dstring_appendf(
               result, DELIMITER_INSTERN(KEY_FMT),
               "actions")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
               result, "[")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if (TAILQ_EMPTY(&flow->instruction_list) == true) {
    if ((ret = lagopus_dstring_appendf(
                 result, "{"KEY_FMT "null}",
                 "drop")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  } else {
    TAILQ_FOREACH(instruction, &flow->instruction_list, entry) {
      if ((ret = dump_instruction(instruction,
                                  is_instruction_first,
                                  result)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (is_instruction_first == true) {
        is_instruction_first = false;
      }
    }
  }

  if ((ret = lagopus_dstring_appendf(
               result, "]")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
               result, "}")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

done:
  return ret;
}

static inline lagopus_result_t
dump_flow_list(const char *name,
               uint8_t table_id,
               bool *is_flow_first,
               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  dp_bridge_iter_t iter = NULL;
  struct flow *flow = NULL;;

  if ((ret = dp_bridge_flow_iter_create(name,
                                        table_id,
                                        &iter)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  for (;;) {
    if ((ret = dp_bridge_flow_iter_get(iter, &flow)) !=
        LAGOPUS_RESULT_OK) {
      if (ret == LAGOPUS_RESULT_EOF) {
        /* ignore eof. */
        ret = LAGOPUS_RESULT_OK;
        break;
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    if ((ret = dump_flow(flow, is_flow_first, result)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
    if (*is_flow_first == true) {
      *is_flow_first = false;
    }
    flow = NULL;
  }

done:
  if (iter != NULL) {
    dp_bridge_flow_iter_destroy(iter);
  }

  return ret;
}

static inline lagopus_result_t
dump_table_flows(const char *name,
                 uint8_t table_id,
                 bool is_table_first,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_flow_first = true;

  if ((ret = lagopus_dstring_appendf(
               result, DS_JSON_DELIMITER(is_table_first, "{"))) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
               result, KEY_FMT "%d",
               "table_id",
               table_id)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
               result, DELIMITER_INSTERN(KEY_FMT "["),
               "flows")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = dump_flow_list(name, table_id, &is_flow_first, result)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
               result, "]")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
               result, "}")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

done:
  return ret;
}

STATIC lagopus_result_t
dump_bridge_domains_flow(const char *name,
                         uint8_t table_id,
                         bool is_bridge_first,
                         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t tid;
  bool is_table_first = true;
  dp_bridge_iter_t iter = NULL;

  if ((ret = lagopus_dstring_appendf(
               result, "{")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
          result, DS_JSON_DELIMITER(is_bridge_first, KEY_FMT "\"%s\""),
          "name",
          name)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
          result, DELIMITER_INSTERN(KEY_FMT "["),
          "tables")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if (table_id == OFPTT_ALL) {
    /* dump all tables. */
    if ((ret = dp_bridge_table_id_iter_create(name, &iter)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    for (;;) {
      if ((ret = dp_bridge_table_id_iter_get(iter, &tid)) !=
          LAGOPUS_RESULT_OK) {
        if (ret == LAGOPUS_RESULT_EOF) {
          /* ignore eof. */
          ret = LAGOPUS_RESULT_OK;
          break;
        } else {
          lagopus_perror(ret);
          goto done;
        }
      }

      if ((ret = dump_table_flows(name, tid, is_table_first, result)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (is_table_first == true) {
        is_table_first = false;
      }
    }
  } else {
    /* dump a table. */
    if ((ret = dump_table_flows(name, table_id,
                                is_table_first, result)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  }

  if (ret == LAGOPUS_RESULT_OK) {
    if ((ret = lagopus_dstring_appendf(
            result, "]")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    if ((ret = lagopus_dstring_appendf(
            result, "}")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  }

done:
  if (iter != NULL) {
    dp_bridge_table_id_iter_destroy(iter);
  }

  return ret;
}

static lagopus_result_t
flow_cmd_dump(datastore_interp_t *iptr,
              void *c,
              FILE *fp,
              void *stream_out,
              datastore_printf_proc_t printf_proc,
              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_bridge_first = true;
  datastore_name_info_t *names = NULL;
  struct datastore_name_entry *name = NULL;
  flow_conf_t *conf = NULL;

  if (iptr != NULL && c != NULL && fp != NULL &&
      stream_out !=NULL && printf_proc != NULL &&
      result != NULL) {
    conf = (flow_conf_t *) c;

    if ((ret = datastore_bridge_get_names(conf->name,
                                          &names)) !=
        LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(
          result, ret,
          "Can't get bridge names.");
      goto done;
    }

    if (fputs("[", fp) == EOF) {
      ret = datastore_json_result_string_setf(
          result, LAGOPUS_RESULT_OUTPUT_FAILURE,
          "Can't write file.");
      goto done;
    }

    /* dump bridge(s). */
    if (TAILQ_EMPTY(&names->head) == false) {
      TAILQ_FOREACH(name, &names->head, name_entries) {
        if ((ret = dump_bridge_domains_flow(name->str,
                                            conf->table_id,
                                            is_bridge_first,
                                            result)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result, ret,
              "Can't get flow stats.");
          goto done;
        }
        if (is_bridge_first == true) {
          is_bridge_first = false;
        }

        ret = cmd_dump_file_write(fp, result);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result, ret,
              "Can't write file.");
          goto done;
        }
      }
    }

    if (fputs("]", fp) != EOF) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_string_setf(
          result, LAGOPUS_RESULT_OUTPUT_FAILURE,
          "Can't write file.");
      goto done;
    }

 done:
    if (ret == LAGOPUS_RESULT_OK) {
      ret = cmd_dump_file_send(iptr, fp, stream_out,
                               printf_proc, result);
    } else {
      ret = cmd_dump_error_send(stream_out, printf_proc,
                                result);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  return ret;
}

static inline void
flow_conf_destroy(flow_conf_t **conf) {
  if (*conf != NULL) {
    free((*conf)->name);
    (*conf)->name = NULL;
    free(*conf);
    *conf = NULL;
  }
}

static inline lagopus_result_t
flow_conf_copy(flow_conf_t **dst, flow_conf_t *src) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *name = NULL;

  if (dst != NULL && *dst == NULL && src != NULL) {
    *dst = (flow_conf_t *) calloc(1, sizeof(flow_conf_t));
    if (*dst != NULL) {
      if (src->name != NULL) {
        name = strdup(src->name);
        if (name == NULL) {
          (*dst)->name = NULL;
          ret = LAGOPUS_RESULT_NO_MEMORY;
          lagopus_perror(ret);
          goto done;
        }
      }
      (*dst)->name = name;
      (*dst)->table_id = src->table_id;
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      lagopus_perror(ret);
    }
  done:
    if (ret != LAGOPUS_RESULT_OK) {
      flow_conf_destroy(dst);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  return ret;
}

static inline void
args_destroy(args_t **args) {
  if (args != NULL && *args != NULL) {
    if ((*args)->conf != NULL) {
      flow_conf_destroy(&((*args)->conf));
    }
    free(*args);
    *args = NULL;
  }
}

static inline lagopus_result_t
args_create(args_t **args,
            datastore_interp_t *iptr,
            flow_conf_t *conf,
            datastore_config_type_t ftype,
            void *stream_out,
            datastore_printf_proc_t printf_proc) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int pre_size, size;

  if (args != NULL && *args == NULL &&
      iptr != NULL && conf != NULL &&
      stream_out != NULL && printf_proc != NULL) {
    *args = (args_t *) calloc(1, sizeof(args_t));
    if (*args != NULL) {
      (*args)->iptr = iptr;

      /* copy file name. */
      lagopus_mutex_lock(&lock);
      if (strlen(dump_tmp_dir) + strlen(TMP_FILE) < PATH_MAX) {
        pre_size = sizeof((*args)->file_name);
        size = snprintf((*args)->file_name,
                        (size_t) pre_size,
                        "%s%s", dump_tmp_dir, TMP_FILE);
        if (size < pre_size) {
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = LAGOPUS_RESULT_OUT_OF_RANGE;
          lagopus_perror(ret);
        }
      } else {
        ret = LAGOPUS_RESULT_OUT_OF_RANGE;
        lagopus_perror(ret);
      }
      lagopus_mutex_unlock(&lock);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = flow_conf_copy(&((*args)->conf), conf);
        if (ret == LAGOPUS_RESULT_OK) {
          (*args)->ftype = ftype;
          (*args)->stream_out = stream_out;
          (*args)->printf_proc = printf_proc;
        } else {
          (*args)->conf = NULL;
          (*args)->stream_out = NULL;
          (*args)->printf_proc = NULL;
        }
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      lagopus_perror(ret);
    }

    if (ret != LAGOPUS_RESULT_OK) {
      args_destroy(args);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  return ret;
}

static lagopus_result_t
thread_main(const lagopus_thread_t *t, void *a) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  args_t *args = NULL;

  if (t != NULL && a != NULL) {
    args = (args_t *) a;

    if ((ret = cmd_dump_main((lagopus_thread_t *) t,
                             args->iptr,
                             (void *) args->conf,
                             args->stream_out,
                             args->printf_proc,
                             args->ftype,
                             args->file_name,
                             flow_cmd_dump)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  if (args != NULL) {
    args_destroy(&args);
  }

  return ret;
}

static inline lagopus_result_t
flow_cmd_dump_initialize(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create lock. */
  if ((ret = lagopus_mutex_create(&lock)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

done:
  return ret;
}

static inline void
flow_cmd_dump_finalize(void) {
  lagopus_hashmap_destroy(&thread_table, true);
  thread_table = NULL;
  lagopus_mutex_destroy(&lock);
}

static inline lagopus_result_t
flow_cmd_dump_thread_start(flow_conf_t *conf,
                           datastore_interp_t *iptr,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_config_type_t ftype = DATASTORE_CONFIG_TYPE_UNKNOWN;
  args_t *args = NULL;
  void *stream_out = NULL;
  datastore_printf_proc_t printf_proc = NULL;
  lagopus_thread_t th = NULL;

  lagopus_msg_info("start.\n");
  if (conf != NULL && iptr != NULL && result != NULL) {
    if ((ret = datastore_interp_get_current_file_context(iptr,
               NULL, NULL,
               NULL, &stream_out,
               NULL, &printf_proc,
               &ftype)) ==
        LAGOPUS_RESULT_OK) {
      if (stream_out != NULL && printf_proc != NULL &&
          (ftype == DATASTORE_CONFIG_TYPE_STREAM_SESSION ||
           ftype == DATASTORE_CONFIG_TYPE_STREAM_FD)) {
        if ((ret = args_create(&args, iptr, conf, ftype,
                               stream_out, printf_proc)) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_thread_create(&th,
                                           thread_main,
                                           NULL,
                                           NULL,
                                           "flow_cmd_dump",
                                           (void *) args)) ==
              LAGOPUS_RESULT_OK) {
            ret = lagopus_thread_start(&th, true);
            if (ret != LAGOPUS_RESULT_OK) {
              lagopus_perror(ret);
            }
          } else {
            lagopus_perror(ret);
          }
        } else {
          lagopus_perror(ret);
        }

        if (ret != LAGOPUS_RESULT_OK && args != NULL) {
          args_destroy(&args);
        }
      } else {
        /* ignore other datastore_config_type_t. */
        ret = LAGOPUS_RESULT_OK;
      }
    } else {
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
flow_cmd_dump_tmp_dir_set(const char *path) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct stat st;

  if (path != NULL) {
    if (strlen(path) < PATH_MAX) {
      if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode) == true) {
          lagopus_mutex_lock(&lock);
          strncpy(dump_tmp_dir, path, PATH_MAX - 1);
          lagopus_mutex_unlock(&lock);
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = LAGOPUS_RESULT_INVALID_ARGS;
        }
      } else {
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = LAGOPUS_RESULT_OUT_OF_RANGE;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
flow_cmd_dump_tmp_dir_get(char **path) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (path != NULL) {
    lagopus_mutex_lock(&lock);
    *path = strdup(dump_tmp_dir);
    if (*path == NULL) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
    lagopus_mutex_unlock(&lock);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
