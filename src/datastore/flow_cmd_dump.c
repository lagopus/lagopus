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
  bool is_with_stats;
} args_t;

static inline const char *
dump_port_string(uint32_t val, char *buf) {
  int size;
  enum flow_port port = FLOW_PORT_MAX;

  switch (val) {
    case OFPP_IN_PORT:
      port = FLOW_PORT_IN_PORT;
      break;
    case OFPP_TABLE:
      port = FLOW_PORT_TABLE;
      break;
    case OFPP_NORMAL:
      port = FLOW_PORT_NORMAL;
      break;
    case OFPP_FLOOD:
      port = FLOW_PORT_FLOOD;
      break;
    case OFPP_ALL:
      port = FLOW_PORT_ALL;
      break;
    case OFPP_CONTROLLER:
      port = FLOW_PORT_CONTROLLER;
      break;
    case OFPP_LOCAL:
      port = FLOW_PORT_LOCAL;
      break;
    case OFPP_ANY:
      port = FLOW_PORT_ANY;
      break;
    default:
      break;
  }
  if (port == FLOW_PORT_MAX) {
    size = snprintf(buf, PORT_STR_BUF_SIZE, "%u", val);
  } else {
    size = snprintf(buf, PORT_STR_BUF_SIZE, "\"%s\"",
                    flow_port_strs[port]);
  }
  if (size < PORT_STR_BUF_SIZE) {
    return buf;
  }

  return "";
}

static inline lagopus_result_t
dump_match(struct match *match,
           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t val8;
  uint16_t val16;
  uint32_t val32;
  uint64_t val64;
  char ip_str[INET6_ADDRSTRLEN];
  int field_type = match->oxm_field >> 1;
  int hasmask = match->oxm_field & 1;
  char buf[PORT_STR_BUF_SIZE];

  switch (field_type) {
    case OFPXMT_OFB_IN_PORT:
      memcpy(&val32, match->oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%s"),
                   flow_match_field_strs[FLOW_MATCH_FIELD_IN_PORT],
                   dump_port_string(ntohl(val32), buf))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IN_PHY_PORT:
      memcpy(&val32, match->oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%s"),
                   flow_match_field_strs[FLOW_MATCH_FIELD_IN_PHY_PORT],
                   dump_port_string(ntohl(val32), buf))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_METADATA:
      memcpy(&val64, match->oxm_value, sizeof(uint64_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu64),
                   flow_match_field_strs[FLOW_MATCH_FIELD_METADATA],
                   ntohll(val64))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/0x%02x%02x%02x%02x%02x%02x%02x%02x",
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
                   flow_match_field_strs[FLOW_MATCH_FIELD_DL_DST],
                   match->oxm_value[0], match->oxm_value[1],
                   match->oxm_value[2], match->oxm_value[3],
                   match->oxm_value[4], match->oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/%02x:%02x:%02x:%02x:%02x:%02x",
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
                   flow_match_field_strs[FLOW_MATCH_FIELD_DL_SRC],
                   match->oxm_value[0], match->oxm_value[1],
                   match->oxm_value[2], match->oxm_value[3],
                   match->oxm_value[4], match->oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/%02x:%02x:%02x:%02x:%02x:%02x",
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
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      switch (ntohs(val16)) {
        case ETHERTYPE_ARP:
          if ((ret = lagopus_dstring_appendf(
                       result, DELIMITER_INSTERN(KEY_FMT "\"%s\""),
                       flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                       flow_dl_type_strs[FLOW_DL_TYPE_ARP])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_VLAN:
          if ((ret = lagopus_dstring_appendf(
                       result, DELIMITER_INSTERN(KEY_FMT "\"%s\""),
                       flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                       flow_dl_type_strs[FLOW_DL_TYPE_VLAN])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_IP:
          if ((ret = lagopus_dstring_appendf(
                       result, DELIMITER_INSTERN(KEY_FMT "\"%s\""),
                       flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                       flow_dl_type_strs[FLOW_DL_TYPE_IP])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_IPV6:
          if ((ret = lagopus_dstring_appendf(
                       result, DELIMITER_INSTERN(KEY_FMT "\"%s\""),
                       flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                       flow_dl_type_strs[FLOW_DL_TYPE_IPV6 ])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_MPLS:
          if ((ret = lagopus_dstring_appendf(
                       result, DELIMITER_INSTERN(KEY_FMT "\"%s\""),
                       flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                       flow_dl_type_strs[FLOW_DL_TYPE_MPLS])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_MPLS_MCAST:
          if ((ret = lagopus_dstring_appendf(
                       result, DELIMITER_INSTERN(KEY_FMT "\"%s\""),
                       flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                       flow_dl_type_strs[FLOW_DL_TYPE_MPLS_MCAST])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_PBB:
          if ((ret = lagopus_dstring_appendf(
                       result, DELIMITER_INSTERN(KEY_FMT "\"%s\""),
                       flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                       flow_dl_type_strs[FLOW_DL_TYPE_PBB])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        default:
          if ((ret = lagopus_dstring_appendf(
                       result, DELIMITER_INSTERN(KEY_FMT "%"PRIu16),
                       flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                       ntohs(val16))) !=
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
                   result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu16),
                   flow_match_field_strs[FLOW_MATCH_FIELD_VLAN_VID],
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
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu8),
                   flow_match_field_strs[FLOW_MATCH_FIELD_VLAN_PCP],
                   *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IP_DSCP:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu8),
                   flow_match_field_strs[FLOW_MATCH_FIELD_IP_DSCP],
                   *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IP_ECN:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu8),
                   flow_match_field_strs[FLOW_MATCH_FIELD_NW_ECN],
                   *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IP_PROTO:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu8),
                   flow_match_field_strs[FLOW_MATCH_FIELD_NW_PROTO],
                   *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV4_SRC:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%s"),
                   flow_match_field_strs[FLOW_MATCH_FIELD_NW_SRC],
                   inet_ntop(AF_INET, match->oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/%s",
                     inet_ntop(AF_INET, &match->oxm_value[4],
                               ip_str, sizeof(ip_str)))) !=
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
                   flow_match_field_strs[FLOW_MATCH_FIELD_NW_DST],
                   inet_ntop(AF_INET, match->oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                result, "\\/%s",
                inet_ntop(AF_INET, &match->oxm_value[4],
                          ip_str, sizeof(ip_str)))) !=
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
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu16),
                   flow_match_field_strs[FLOW_MATCH_FIELD_TCP_SRC],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_TCP_DST:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu16),
                   flow_match_field_strs[FLOW_MATCH_FIELD_TCP_DST],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_UDP_SRC:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu16),
                   flow_match_field_strs[FLOW_MATCH_FIELD_UDP_SRC],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_UDP_DST:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu16),
                   flow_match_field_strs[FLOW_MATCH_FIELD_UDP_DST],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_SCTP_SRC:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu16),
                   flow_match_field_strs[FLOW_MATCH_FIELD_SCTP_SRC],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_SCTP_DST:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu16),
                   flow_match_field_strs[FLOW_MATCH_FIELD_SCTP_DST],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ICMPV4_TYPE:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu16),
                   flow_match_field_strs[FLOW_MATCH_FIELD_ICMP_TYPE],
                   *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ICMPV4_CODE:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu8),
                   flow_match_field_strs[FLOW_MATCH_FIELD_ICMP_CODE],
                   *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_OP:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu16),
                   flow_match_field_strs[FLOW_MATCH_FIELD_ARP_OP],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_SPA:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%s"),
                   flow_match_field_strs[FLOW_MATCH_FIELD_ARP_SPA],
                   inet_ntop(AF_INET, match->oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                result, "\\/%s",
                inet_ntop(AF_INET, &match->oxm_value[4],
                          ip_str, sizeof(ip_str)))) !=
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
                   flow_match_field_strs[FLOW_MATCH_FIELD_ARP_TPA],
                   inet_ntop(AF_INET, match->oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                result, "\\/%s",
                inet_ntop(AF_INET, &match->oxm_value[4],
                          ip_str, sizeof(ip_str)))) !=
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
                   flow_match_field_strs[FLOW_MATCH_FIELD_ARP_SHA],
                   match->oxm_value[0], match->oxm_value[1],
                   match->oxm_value[2], match->oxm_value[3],
                   match->oxm_value[4], match->oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/%02x:%02x:%02x:%02x:%02x:%02x",
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
                   flow_match_field_strs[FLOW_MATCH_FIELD_ARP_THA],
                   match->oxm_value[0], match->oxm_value[1],
                   match->oxm_value[2], match->oxm_value[3],
                   match->oxm_value[4], match->oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/%02x:%02x:%02x:%02x:%02x:%02x",
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
                   flow_match_field_strs[FLOW_MATCH_FIELD_IPV6_SRC],
                   inet_ntop(AF_INET6, match->oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/%s",
                     inet_ntop(AF_INET6, &match->oxm_value[16], ip_str, sizeof(ip_str)))) !=
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
                   flow_match_field_strs[FLOW_MATCH_FIELD_IPV6_DST],
                   inet_ntop(AF_INET6, match->oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                     result, "\\/%s",
                     inet_ntop(AF_INET6, &match->oxm_value[16], ip_str, sizeof(ip_str)))) !=
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
                   result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu32),
                   flow_match_field_strs[FLOW_MATCH_FIELD_IPV6_LABEL],
                   ntohl(val32))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                result, "\\/0x%02x%02x%02x%02x",
                match->oxm_value[4],
                match->oxm_value[5],
                match->oxm_value[6],
                match->oxm_value[7])) !=
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
    case OFPXMT_OFB_ICMPV6_TYPE:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu8),
                   flow_match_field_strs[FLOW_MATCH_FIELD_ICMPV6_TYPE],
                   *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ICMPV6_CODE:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu8),
                   flow_match_field_strs[FLOW_MATCH_FIELD_ICMPV6_CODE],
                   *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_ND_TARGET:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%s\""),
                   flow_match_field_strs[FLOW_MATCH_FIELD_ND_TARGET],
                   inet_ntop(AF_INET6, match->oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_ND_SLL:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x\""),
                   flow_match_field_strs[FLOW_MATCH_FIELD_ND_SLL],
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
                   flow_match_field_strs[FLOW_MATCH_FIELD_ND_TLL],
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
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu32),
                   flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_LABEL],
                   val32)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_TC:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu8),
                   flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_TC],
                   *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_BOS:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu8),
                   flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_BOS],
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
                   result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu32),
                   flow_match_field_strs[FLOW_MATCH_FIELD_PBB_ISID],
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
      memcpy(&val64, match->oxm_value, sizeof(uint64_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu64),
                   flow_match_field_strs[FLOW_MATCH_FIELD_TUNNEL_ID],
                   ntohll(val64))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                result, "\\/0x%02x%02x%02x%02x%02x%02x%02x%02x",
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
    case OFPXMT_OFB_IPV6_EXTHDR:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu16),
                   flow_match_field_strs[FLOW_MATCH_FIELD_IPV6_EXTHDR],
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
    case OFPXMT_OFB_PBB_UCA:
      if ((ret = lagopus_dstring_appendf(
              result, DELIMITER_INSTERN(KEY_FMT "%"PRIu8),
              flow_match_field_strs[FLOW_MATCH_FIELD_PBB_UCA],
              *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_PACKET_TYPE:
      memcpy(&val32, match->oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu32),
                   flow_match_field_strs[FLOW_MATCH_FIELD_PACKET_TYPE],
                   ntohl(val32))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_GRE_FLAGS:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu16),
                   flow_match_field_strs[FLOW_MATCH_FIELD_GRE_FLAGS],
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
    case OFPXMT_OFB_GRE_VER:
      if ((ret = lagopus_dstring_appendf(
              result, DELIMITER_INSTERN(KEY_FMT "%"PRIu8),
              flow_match_field_strs[FLOW_MATCH_FIELD_GRE_VER],
              *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_GRE_PROTOCOL:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu16),
                   flow_match_field_strs[FLOW_MATCH_FIELD_GRE_PROTOCOL],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_GRE_KEY:
      memcpy(&val32, match->oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu32),
                   flow_match_field_strs[FLOW_MATCH_FIELD_GRE_KEY],
                   ntohl(val32))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        if ((ret = lagopus_dstring_appendf(
                result, "\\/0x%02x%02x%02x%02x",
                match->oxm_value[4],
                match->oxm_value[5],
                match->oxm_value[6],
                match->oxm_value[7])) !=
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
    case OFPXMT_OFB_GRE_SEQNUM:
      memcpy(&val32, match->oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu32),
                   flow_match_field_strs[FLOW_MATCH_FIELD_GRE_SEQNUM],
                   ntohl(val32))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_LISP_FLAGS:
      memcpy(&val8, match->oxm_value, sizeof(uint8_t));
      if ((ret = lagopus_dstring_appendf(
              result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu8),
              flow_match_field_strs[FLOW_MATCH_FIELD_LISP_FLAGS],
              val8)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        memcpy(&val8, &match->oxm_value[1], sizeof(uint8_t));
        if ((ret = lagopus_dstring_appendf(
                result, "\\/0x%02x",
                val8)) !=
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
    case OFPXMT_OFB_LISP_NONCE:
      val32 = match->oxm_value[0];
      val32 = (val32 << 8) | match->oxm_value[1];
      val32 = (val32 << 8) | match->oxm_value[2];
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu32),
                   flow_match_field_strs[FLOW_MATCH_FIELD_LISP_NONCE],
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
    case OFPXMT_OFB_LISP_ID:
      memcpy(&val32, match->oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu32),
                   flow_match_field_strs[FLOW_MATCH_FIELD_LISP_ID],
                   ntohl(val32))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_VXLAN_FLAGS:
      memcpy(&val8, match->oxm_value, sizeof(uint8_t));
      if ((ret = lagopus_dstring_appendf(
              result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu8),
              flow_match_field_strs[FLOW_MATCH_FIELD_VXLAN_FLAGS],
              val8)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        memcpy(&val8, &match->oxm_value[1], sizeof(uint8_t));
        if ((ret = lagopus_dstring_appendf(
                result, "\\/0x%02x",
                val8)) !=
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
    case OFPXMT_OFB_VXLAN_VNI:
      val32 = match->oxm_value[0];
      val32 = (val32 << 8) | match->oxm_value[1];
      val32 = (val32 << 8) | match->oxm_value[2];
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu32),
                   flow_match_field_strs[FLOW_MATCH_FIELD_VXLAN_VNI],
                   val32)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_DATA_FIRST_NIBBLE:
      memcpy(&val8, match->oxm_value, sizeof(uint8_t));
      if ((ret = lagopus_dstring_appendf(
              result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu8),
              flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_DATA_FIRST_NIBBLE],
              val8)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        memcpy(&val8, &match->oxm_value[1], sizeof(uint8_t));
        if ((ret = lagopus_dstring_appendf(
                result, "\\/0x%02x",
                val8)) !=
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
    case OFPXMT_OFB_MPLS_ACH_VERSION:
      if ((ret = lagopus_dstring_appendf(
              result, DELIMITER_INSTERN(KEY_FMT "%"PRIu8),
              flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_ACH_VERSION],
              *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_ACH_CHANNEL:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu16),
                   flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_ACH_CHANNEL],
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
    case OFPXMT_OFB_MPLS_PW_METADATA:
      memcpy(&val8, match->oxm_value, sizeof(uint8_t));
      if ((ret = lagopus_dstring_appendf(
              result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu8),
              flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_PW_METADATA],
              val8)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        memcpy(&val8, &match->oxm_value[1], sizeof(uint8_t));
        if ((ret = lagopus_dstring_appendf(
                result, "\\/0x%02x",
                val8)) !=
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
    case OFPXMT_OFB_MPLS_CW_FLAGS:
      memcpy(&val8, match->oxm_value, sizeof(uint8_t));
      if ((ret = lagopus_dstring_appendf(
              result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu8),
              flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_CW_FLAGS],
              val8)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        memcpy(&val8, &match->oxm_value[1], sizeof(uint8_t));
        if ((ret = lagopus_dstring_appendf(
                result, "\\/0x%02x",
                val8)) !=
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
    case OFPXMT_OFB_MPLS_CW_FRAG:
      memcpy(&val8, match->oxm_value, sizeof(uint8_t));
      if ((ret = lagopus_dstring_appendf(
              result, DELIMITER_INSTERN(KEY_FMT "\"%"PRIu8),
              flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_CW_FRAG],
              val8)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (hasmask) {
        memcpy(&val8, &match->oxm_value[1], sizeof(uint8_t));
        if ((ret = lagopus_dstring_appendf(
                result, "\\/0x%02x",
                val8)) !=
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
    case OFPXMT_OFB_MPLS_CW_LEN:
      if ((ret = lagopus_dstring_appendf(
              result, DELIMITER_INSTERN(KEY_FMT "%"PRIu8),
              flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_CW_LEN],
              *match->oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_CW_SEQ_NUM:
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "%"PRIu16),
                   flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_CW_SEQ_NUM],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    default:
      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "null"),
                   FLOW_UNKNOWN)) !=
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
                   flow_match_field_strs[FLOW_MATCH_FIELD_IN_PORT],
                   dump_port_string(ntohl(val32), buf))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IN_PHY_PORT:
      memcpy(&val32, oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%s",
                   flow_match_field_strs[FLOW_MATCH_FIELD_IN_PHY_PORT],
                   dump_port_string(ntohl(val32), buf))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_METADATA:
      memcpy(&val64, oxm_value, sizeof(uint64_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu64,
                   flow_match_field_strs[FLOW_MATCH_FIELD_METADATA],
                   ntohll(val64))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ETH_DST:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x\"",
                   flow_match_field_strs[FLOW_MATCH_FIELD_DL_DST],
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
                   flow_match_field_strs[FLOW_MATCH_FIELD_DL_SRC],
                   oxm_value[0], oxm_value[1],
                   oxm_value[2], oxm_value[3],
                   oxm_value[4], oxm_value[5])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ETH_TYPE:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      switch (ntohs(val16)) {
        case ETHERTYPE_ARP:
          if ((ret = lagopus_dstring_appendf(
                       result, KEY_FMT "\"%s\"",
                       flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                       flow_dl_type_strs[FLOW_DL_TYPE_ARP])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_VLAN:
          if ((ret = lagopus_dstring_appendf(
                       result, KEY_FMT "\"%s\"",
                       flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                       flow_dl_type_strs[FLOW_DL_TYPE_VLAN])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_IP:
          if ((ret = lagopus_dstring_appendf(
                       result, KEY_FMT "\"%s\"",
                       flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                       flow_dl_type_strs[FLOW_DL_TYPE_IP])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_IPV6:
          if ((ret = lagopus_dstring_appendf(
                       result, KEY_FMT "\"%s\"",
                       flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                       flow_dl_type_strs[FLOW_DL_TYPE_IPV6 ])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_MPLS:
          if ((ret = lagopus_dstring_appendf(
                       result, KEY_FMT "\"%s\"",
                       flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                       flow_dl_type_strs[FLOW_DL_TYPE_MPLS])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_MPLS_MCAST:
          if ((ret = lagopus_dstring_appendf(
                       result, KEY_FMT "\"%s\"",
                       flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                       flow_dl_type_strs[FLOW_DL_TYPE_MPLS_MCAST])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case ETHERTYPE_PBB:
          if ((ret = lagopus_dstring_appendf(
                       result, KEY_FMT "\"%s\"",
                       flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                       flow_dl_type_strs[FLOW_DL_TYPE_PBB])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        default:
          if ((ret = lagopus_dstring_appendf(
                  result, KEY_FMT "%"PRIu16,
                  flow_match_field_strs[FLOW_MATCH_FIELD_DL_TYPE],
                  ntohs(val16))) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
      }
      break;
    case OFPXMT_OFB_VLAN_VID:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu16,
                   flow_match_field_strs[FLOW_MATCH_FIELD_VLAN_VID],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_VLAN_PCP:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu8,
                   flow_match_field_strs[FLOW_MATCH_FIELD_VLAN_PCP],
                   *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IP_DSCP:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu8,
                   flow_match_field_strs[FLOW_MATCH_FIELD_IP_DSCP],
                   *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IP_ECN:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu8,
                   flow_match_field_strs[FLOW_MATCH_FIELD_NW_ECN],
                   *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IP_PROTO:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu8,
                   flow_match_field_strs[FLOW_MATCH_FIELD_NW_PROTO],
                   *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV4_SRC:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%s\"",
                   flow_match_field_strs[FLOW_MATCH_FIELD_NW_SRC],
                   inet_ntop(AF_INET, oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV4_DST:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%s\"",
                   flow_match_field_strs[FLOW_MATCH_FIELD_NW_DST],
                   inet_ntop(AF_INET, oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_TCP_SRC:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu16,
                   flow_match_field_strs[FLOW_MATCH_FIELD_TCP_SRC],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_TCP_DST:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu16,
                   flow_match_field_strs[FLOW_MATCH_FIELD_TCP_DST],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_UDP_SRC:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu16,
                   flow_match_field_strs[FLOW_MATCH_FIELD_UDP_SRC],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_UDP_DST:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu16,
                   flow_match_field_strs[FLOW_MATCH_FIELD_UDP_DST],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_SCTP_SRC:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu16,
                   flow_match_field_strs[FLOW_MATCH_FIELD_SCTP_SRC],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_SCTP_DST:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu16,
                   flow_match_field_strs[FLOW_MATCH_FIELD_SCTP_DST],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ICMPV4_TYPE:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu8,
                   flow_match_field_strs[FLOW_MATCH_FIELD_ICMP_TYPE],
                   *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ICMPV4_CODE:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu8,
                   flow_match_field_strs[FLOW_MATCH_FIELD_ICMP_CODE],
                   *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_OP:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu16,
                   flow_match_field_strs[FLOW_MATCH_FIELD_ARP_OP],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_SPA:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%s\"",
                   flow_match_field_strs[FLOW_MATCH_FIELD_ARP_SPA],
                   inet_ntop(AF_INET, oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_TPA:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%s\"",
                   flow_match_field_strs[FLOW_MATCH_FIELD_ARP_TPA],
                   inet_ntop(AF_INET, oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ARP_SHA:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x\"",
                   flow_match_field_strs[FLOW_MATCH_FIELD_ARP_SHA],
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
                   flow_match_field_strs[FLOW_MATCH_FIELD_ARP_THA],
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
                   flow_match_field_strs[FLOW_MATCH_FIELD_IPV6_SRC],
                   inet_ntop(AF_INET6, oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_DST:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%s\"",
                   flow_match_field_strs[FLOW_MATCH_FIELD_IPV6_DST],
                   inet_ntop(AF_INET6, oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_FLABEL:
      memcpy(&val32, oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu32,
                   flow_match_field_strs[FLOW_MATCH_FIELD_IPV6_LABEL],
                   ntohl(val32))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ICMPV6_TYPE:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu8,
                   flow_match_field_strs[FLOW_MATCH_FIELD_ICMPV6_TYPE],
                   *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_ICMPV6_CODE:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu8,
                   flow_match_field_strs[FLOW_MATCH_FIELD_ICMPV6_CODE],
                   *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_ND_TARGET:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%s\"",
                   flow_match_field_strs[FLOW_MATCH_FIELD_ND_TARGET],
                   inet_ntop(AF_INET6, oxm_value, ip_str, sizeof(ip_str)))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_ND_SLL:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x\"",
                   flow_match_field_strs[FLOW_MATCH_FIELD_ND_SLL ],
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
                   flow_match_field_strs[FLOW_MATCH_FIELD_ND_TLL],
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
                   result, KEY_FMT "%"PRIu32,
                   flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_LABEL],
                   ntohl(val32))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_TC:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu8,
                   flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_TC],
                   *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_BOS:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu8,
                   flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_BOS],
                   *oxm_value)) !=
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
                   result, KEY_FMT "%"PRIu32,
                   flow_match_field_strs[FLOW_MATCH_FIELD_PBB_ISID],
                   val32)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_TUNNEL_ID:
      memcpy(&val64, oxm_value, sizeof(uint64_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu64,
                   flow_match_field_strs[FLOW_MATCH_FIELD_TUNNEL_ID],
                   ntohll(val64))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_IPV6_EXTHDR:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu16,
                   flow_match_field_strs[FLOW_MATCH_FIELD_IPV6_EXTHDR],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_PBB_UCA:
      if ((ret = lagopus_dstring_appendf(
              result, KEY_FMT "%"PRIu8,
              flow_match_field_strs[FLOW_MATCH_FIELD_PBB_UCA],
              *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_PACKET_TYPE:
      memcpy(&val32, oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu32,
                   flow_match_field_strs[FLOW_MATCH_FIELD_PACKET_TYPE],
                   ntohl(val32))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_GRE_FLAGS:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu16,
                   flow_match_field_strs[FLOW_MATCH_FIELD_GRE_FLAGS],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_GRE_VER:
      if ((ret = lagopus_dstring_appendf(
              result, KEY_FMT "%"PRIu8,
              flow_match_field_strs[FLOW_MATCH_FIELD_GRE_VER],
              *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_GRE_PROTOCOL:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu16,
                   flow_match_field_strs[FLOW_MATCH_FIELD_GRE_PROTOCOL],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_GRE_KEY:
      memcpy(&val32, oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu32,
                   flow_match_field_strs[FLOW_MATCH_FIELD_GRE_KEY],
                   ntohl(val32))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_GRE_SEQNUM:
      memcpy(&val32, oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu32,
                   flow_match_field_strs[FLOW_MATCH_FIELD_GRE_SEQNUM],
                   ntohl(val32))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_LISP_FLAGS:
      if ((ret = lagopus_dstring_appendf(
              result, KEY_FMT "%"PRIu8,
              flow_match_field_strs[FLOW_MATCH_FIELD_LISP_FLAGS],
              *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_LISP_NONCE:
      val32 = oxm_value[0];
      val32 = (val32 << 8) | oxm_value[1];
      val32 = (val32 << 8) | oxm_value[2];
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu32,
                   flow_match_field_strs[FLOW_MATCH_FIELD_LISP_NONCE],
                   val32)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_LISP_ID:
      memcpy(&val32, oxm_value, sizeof(uint32_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu32,
                   flow_match_field_strs[FLOW_MATCH_FIELD_LISP_ID],
                   ntohl(val32))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_VXLAN_FLAGS:
      if ((ret = lagopus_dstring_appendf(
              result, KEY_FMT "%"PRIu8,
              flow_match_field_strs[FLOW_MATCH_FIELD_VXLAN_FLAGS],
              *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_VXLAN_VNI:
      val32 = oxm_value[0];
      val32 = (val32 << 8) | oxm_value[1];
      val32 = (val32 << 8) | oxm_value[2];
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu32,
                   flow_match_field_strs[FLOW_MATCH_FIELD_VXLAN_VNI],
                   val32)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_DATA_FIRST_NIBBLE:
      if ((ret = lagopus_dstring_appendf(
              result, KEY_FMT "%"PRIu8,
              flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_DATA_FIRST_NIBBLE],
              *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_ACH_VERSION:
      if ((ret = lagopus_dstring_appendf(
              result, KEY_FMT "%"PRIu8,
              flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_ACH_VERSION],
              *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_ACH_CHANNEL:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu16,
                   flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_ACH_CHANNEL],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_PW_METADATA:
      if ((ret = lagopus_dstring_appendf(
              result, KEY_FMT "%"PRIu8,
              flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_PW_METADATA],
              *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_CW_FLAGS:
      if ((ret = lagopus_dstring_appendf(
              result, KEY_FMT "%"PRIu8,
              flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_CW_FLAGS],
              *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_CW_FRAG:
      if ((ret = lagopus_dstring_appendf(
              result, KEY_FMT "%"PRIu8,
              flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_CW_FRAG],
              *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_CW_LEN:
      if ((ret = lagopus_dstring_appendf(
              result, KEY_FMT "%"PRIu8,
              flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_CW_LEN],
              *oxm_value)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPXMT_OFB_MPLS_CW_SEQ_NUM:
      memcpy(&val16, oxm_value, sizeof(uint16_t));
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "%"PRIu16,
                   flow_match_field_strs[FLOW_MATCH_FIELD_MPLS_CW_SEQ_NUM],
                   ntohs(val16))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    default:
      if ((ret = lagopus_dstring_appendf(
                   result, KEY_FMT "null",
                   FLOW_UNKNOWN)) !=
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

static inline lagopus_result_t
dump_ed_props(struct ed_prop *ed_prop,
              bool is_ed_prop_first,
              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char *escape_str = NULL;

  if ((ret = lagopus_dstring_appendf(
          result, DS_JSON_DELIMITER(is_ed_prop_first, ""))) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  switch (ed_prop->ofp_ed_prop.type) {
    case OFPPPT_PROP_PORT_NAME:
      if ((ret = lagopus_dstring_appendf(
              result, "{" KEY_FMT,
              ed_prop_strs[FLOW_ACTION_ED_PROP_PORT_NAME])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if ((ret = lagopus_dstring_appendf(
              result, "{" KEY_FMT "%"PRIu16,
              ed_prop_portname_strs[FLOW_ACTION_ED_PROP_PORT_NAME_PORT_FLAGS],
              ed_prop->ofp_ed_prop_portname.port_flags)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      /* escape for json. */
      str = (char *) ed_prop->ofp_ed_prop_portname.name;
      if ((ret = datastore_json_string_escape(str, &escape_str)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if ((ret = lagopus_dstring_appendf(
              result, DELIMITER_INSTERN(KEY_FMT "\"%s\""),
              ed_prop_portname_strs[FLOW_ACTION_ED_PROP_PORT_NAME_NAME],
              escape_str)) !=
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

      if ((ret = lagopus_dstring_appendf(
              result, "}")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      break;
    case OFPPPT_PROP_NONE:
    case OFPPPT_PROP_EXPERIMENTER:
    default:
      if ((ret = lagopus_dstring_appendf(
              result, "{" KEY_FMT "null}",
              FLOW_UNKNOWN)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
  }

done:
  free(escape_str);

  return ret;
}

lagopus_result_t
flow_cmd_dump_action(struct action *action,
                     bool is_action_first,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char buf[PORT_STR_BUF_SIZE];
  struct ed_prop *ed_prop;
  bool is_ed_prop_first = true;

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
                   flow_action_strs[FLOW_ACTION_OUTPUT],
                   dump_port_string(((struct ofp_action_output *)
                                     &action->ofpat)->port, buf))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_COPY_TTL_OUT:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "null}",
                   flow_action_strs[FLOW_ACTION_COPY_TTL_OUT])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_COPY_TTL_IN:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "null}",
                   flow_action_strs[FLOW_ACTION_COPY_TTL_IN])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_SET_MPLS_TTL:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "%"PRIu8"}",
                   flow_action_strs[FLOW_ACTION_SET_MPLS_TTL],
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
                   flow_action_strs[FLOW_ACTION_DEC_MPLS_TTL])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_PUSH_VLAN:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "%"PRIu16"}",
                   flow_action_strs[FLOW_ACTION_PUSH_VLAN],
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
                   flow_action_strs[FLOW_ACTION_STRIP_VLAN])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_PUSH_MPLS:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "%"PRIu16"}",
                   flow_action_strs[FLOW_ACTION_PUSH_MPLS],
                   ((struct ofp_action_push *)
                    &action->ofpat)->ethertype)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_POP_MPLS:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "%"PRIu16"}",
                   flow_action_strs[FLOW_ACTION_POP_MPLS],
                   ((struct ofp_action_push *)
                    &action->ofpat)->ethertype)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_SET_QUEUE:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "%"PRIu16"}",
                   flow_action_strs[FLOW_ACTION_SET_QUEUE],
                   ((struct ofp_action_set_queue *)
                    &action->ofpat)->queue_id)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_GROUP:
      switch (((struct ofp_action_group *) &action->ofpat)->group_id) {
        case OFPG_ALL:
          if ((ret = lagopus_dstring_appendf(
                  result, "{" KEY_FMT "\"%s\"}",
                  flow_action_strs[FLOW_ACTION_GROUP],
                  flow_group_strs[FLOW_GROUP_ALL])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        case OFPG_ANY:
          if ((ret = lagopus_dstring_appendf(
                  result, "{" KEY_FMT "\"%s\"}",
                  flow_action_strs[FLOW_ACTION_GROUP],
                  flow_group_strs[FLOW_GROUP_ANY])) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
        default:
          if ((ret = lagopus_dstring_appendf(
                  result, "{" KEY_FMT "%"PRIu32"}",
                  flow_action_strs[FLOW_ACTION_GROUP],
                  ((struct ofp_action_group *)
                   &action->ofpat)->group_id)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          break;
      }
      break;
    case OFPAT_SET_NW_TTL:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "%"PRIu8"}",
                   flow_action_strs[FLOW_ACTION_SET_NW_TTL],
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
                   flow_action_strs[FLOW_ACTION_DEC_NW_TTL])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_SET_FIELD:
      if ((ret = dump_set_field(
                   ((struct ofp_action_set_field *)
                    &action->ofpat)->field,
                   result)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_PUSH_PBB:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "%"PRIu16"}",
                   flow_action_strs[FLOW_ACTION_PUSH_PBB],
                   ((struct ofp_action_push *)&action->ofpat)->ethertype)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_POP_PBB:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "null}",
                   flow_action_strs[FLOW_ACTION_POP_PBB])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_ENCAP:
      if ((ret = lagopus_dstring_appendf(
              result, "{" KEY_FMT,
              flow_action_strs[FLOW_ACTION_ENCAP])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if ((ret = lagopus_dstring_appendf(
              result, "{" KEY_FMT "%"PRIu16,
              action_encap_strs[FLOW_ACTION_ENCAP_PACKET_TYPE],
              ((struct ofp_action_encap *)&action->ofpat)->packet_type)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "["),
                   action_encap_strs[FLOW_ACTION_ENCAP_PROPS])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if (TAILQ_EMPTY(&action->ed_prop_list) == true) {
        /* nothing ed_props. */
      } else {
        TAILQ_FOREACH(ed_prop, &action->ed_prop_list, entry) {
          if ((ret = dump_ed_props(ed_prop,
                                   is_ed_prop_first,
                                   result)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          if (is_ed_prop_first == true) {
            is_ed_prop_first = false;
          }
        }
      }

      if ((ret = lagopus_dstring_appendf(
              result, "]}}")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_DECAP:
      if ((ret = lagopus_dstring_appendf(
              result, "{" KEY_FMT,
              flow_action_strs[FLOW_ACTION_DECAP])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if ((ret = lagopus_dstring_appendf(
              result, "{" KEY_FMT "%"PRIu16,
              action_decap_strs[FLOW_ACTION_DECAP_CUR_PKT_TYPE],
              ((struct ofp_action_decap *)&action->ofpat)->cur_pkt_type)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if ((ret = lagopus_dstring_appendf(
              result,  DELIMITER_INSTERN(KEY_FMT "%"PRIu16),
              action_decap_strs[FLOW_ACTION_DECAP_NEW_PKT_TYPE],
              ((struct ofp_action_decap *)&action->ofpat)->new_pkt_type)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if ((ret = lagopus_dstring_appendf(
                   result, DELIMITER_INSTERN(KEY_FMT "["),
                   action_decap_strs[FLOW_ACTION_DECAP_PROPS])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if (TAILQ_EMPTY(&action->ed_prop_list) == true) {
        /* nothing ed_props. */
      } else {
        TAILQ_FOREACH(ed_prop, &action->ed_prop_list, entry) {
          if ((ret = dump_ed_props(ed_prop,
                                   is_ed_prop_first,
                                   result)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
          if (is_ed_prop_first == true) {
            is_ed_prop_first = false;
          }
        }
      }

      if ((ret = lagopus_dstring_appendf(
              result, "]}}")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPAT_EXPERIMENTER:
    default:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "null}",
                   FLOW_UNKNOWN)) !=
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
  bool is_action_first = true;

  if ((ret = lagopus_dstring_appendf(
               result, DS_JSON_DELIMITER(is_instruction_first, ""))) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  switch (instruction->ofpit.type) {
    case OFPIT_GOTO_TABLE:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "%"PRIu8"}",
                   flow_instruction_strs[FLOW_INSTRUCTION_GOTO_TABLE],
                   instruction->ofpit_goto_table.table_id)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPIT_WRITE_METADATA:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "\"%"PRIu64,
                   flow_instruction_strs[FLOW_INSTRUCTION_WRITE_METADATA],
                   instruction->ofpit_write_metadata.metadata)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if (instruction->ofpit_write_metadata.metadata_mask != 0) {
        if ((ret = lagopus_dstring_appendf(
                result, "\\/0x%016"PRIx64,
                instruction->ofpit_write_metadata.metadata_mask)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
        }
      }
      if ((ret = lagopus_dstring_appendf(
              result, "\"}")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPIT_WRITE_ACTIONS:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "\n",
                   flow_instruction_strs[FLOW_INSTRUCTION_WRITE_ACTIONS])) !=
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
                   result, "{" KEY_FMT "\n",
                   flow_instruction_strs[FLOW_INSTRUCTION_APPLY_ACTIONS])) !=
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
                   flow_instruction_strs[FLOW_INSTRUCTION_CLEAR_ACTIONS])) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      break;
    case OFPIT_METER:
      if ((ret = lagopus_dstring_appendf(
                   result, "{" KEY_FMT "%"PRIu32"}",
                   flow_instruction_strs[FLOW_INSTRUCTION_METER],
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
                   FLOW_UNKNOWN)) !=
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
dump_flow_field(struct flow *flow,
                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((ret = lagopus_dstring_appendf(
               result, DELIMITER_INSTERN(KEY_FMT "%u"),
               flow_field_strs[FLOW_FIELD_IDLE_TIMEOUT],
               flow->idle_timeout)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
               result, DELIMITER_INSTERN(KEY_FMT "%u"),
               flow_field_strs[FLOW_FIELD_HARD_TIMEOUT],
               flow->hard_timeout)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((flow->flags & OFPFF_SEND_FLOW_REM) != 0) {
    if ((ret = lagopus_dstring_appendf(
            result, DELIMITER_INSTERN(KEY_FMT "null"),
            flow_field_strs[FLOW_FIELD_SEND_FLOW_REM])) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  }

  if ((flow->flags & OFPFF_CHECK_OVERLAP) != 0) {
    if ((ret = lagopus_dstring_appendf(
            result, DELIMITER_INSTERN(KEY_FMT "null"),
            flow_field_strs[FLOW_FIELD_CHECK_OVERLAP])) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  }

  if ((ret = lagopus_dstring_appendf(
               result, DELIMITER_INSTERN(KEY_FMT "%"PRIu64),
               flow_field_strs[FLOW_FIELD_COOKIE],
               flow->cookie)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

done:
  return ret;
}

static inline lagopus_result_t
dump_flow_stat(struct flow *flow,
               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((ret = lagopus_dstring_appendf(
          result, DELIMITER_INSTERN(KEY_FMT "{"),
          FLOW_STATS)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
          result, KEY_FMT "%"PRIu64,
          flow_stat_strs[FLOW_STAT_PACKET_COUNT],
          flow->packet_count)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
          result, DELIMITER_INSTERN(KEY_FMT "%"PRIu64),
          flow_stat_strs[FLOW_STAT_BYTE_COUNT],
          flow->byte_count)) !=
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
dump_flow(struct flow *flow,
          bool is_with_stats,
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
               result, KEY_FMT "%"PRIu16,
               flow_field_strs[FLOW_FIELD_PRIORITY],
               flow->priority)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = dump_flow_field(flow, result)) !=
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
               FLOW_ACTIONS)) !=
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
    /* nothing instructions. */
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

  if (is_with_stats == true) {
    if ((ret = dump_flow_stat(flow, result)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
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
               bool is_with_stats,
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

    if ((ret = dump_flow(flow, is_with_stats,
                         is_flow_first, result)) !=
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
                 bool is_with_stats,
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
               result, KEY_FMT "%"PRIu8,
               flow_field_strs[FLOW_FIELD_TABLE],
               table_id)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
               result, DELIMITER_INSTERN(KEY_FMT "["),
               FLOW_FLOWS)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = dump_flow_list(name, table_id,
                            is_with_stats, &is_flow_first, result)) !=
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
                         bool is_with_stats,
                         bool is_bridge_first,
                         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t tid;
  bool is_table_first = true;
  dp_bridge_iter_t iter = NULL;

  if ((ret = lagopus_dstring_appendf(
          result, DS_JSON_DELIMITER(is_bridge_first, "{"))) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
          result, KEY_FMT "\"%s\"",
          FLOW_NAME,
          name)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(
          result, DELIMITER_INSTERN(KEY_FMT "["),
          FLOW_TABLES)) !=
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

      if ((ret = dump_table_flows(name, tid, is_with_stats,
                                  is_table_first, result)) !=
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
    if ((ret = dump_table_flows(name, table_id, is_with_stats,
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
              bool is_with_stats,
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
                                            is_with_stats,
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
        (void) lagopus_dstring_clear(result);
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
            configs_t *configs,
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
      (*args)->is_with_stats = configs->is_with_stats;

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
                             args->is_with_stats,
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
                           configs_t *configs,
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
        if ((ret = args_create(&args, iptr, conf, configs, ftype,
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
