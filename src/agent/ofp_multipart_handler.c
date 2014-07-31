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


#include <stdbool.h>
#include <stdint.h>
#include "lagopus_apis.h"
#include "openflow.h"
#include "openflow13packet.h"
#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/ofp_pdump.h"
#include "ofp_apis.h"

static void
multipart_request_trace(struct ofp_multipart_request *multipart) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_MULTIPART_REQUEST)) {
    lagopus_msg_pdump(TRACE_OFPT_MULTIPART_REQUEST, PDUMP_OFP_HEADER,
                      multipart->header, "");
    lagopus_msg_pdump(TRACE_OFPT_MULTIPART_REQUEST, PDUMP_OFP_MULTIPART_REQUEST,
                      *multipart, "");
  }
}

/* multipart_request received. */
lagopus_result_t
ofp_multipart_request_handle(struct channel *channel, struct pbuf *pbuf,
                             struct ofp_header *xid_header,
                             struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_multipart_request multipart;
  struct pbuf *all = NULL;
  uint16_t code;

  /* check params */
  if (channel == NULL || pbuf == NULL ||
      xid_header == NULL || error == NULL) {
    res = LAGOPUS_RESULT_INVALID_ARGS;
    goto done;
  }

  /* Parse multipart header. */
  res = ofp_multipart_request_decode(pbuf, &(multipart));
  if (res != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("multipart_request decode error (%s)\n",
                        lagopus_error_get_string(res));
    ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
    res = LAGOPUS_RESULT_OFP_ERROR;
    goto done;
  }

  /* dump trace. */
  multipart_request_trace(&multipart);

  /* check role. */
  if (ofp_role_mp_check(channel, pbuf, &multipart) == false) {
    lagopus_msg_warning("role is slave (multipart, %s).\n",
                        ofp_multipart_type_str(multipart.type));
    res = LAGOPUS_RESULT_OFP_ERROR;
    ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_IS_SLAVE);
    goto done;
  }

  res = channel_multipart_put(channel, pbuf, xid_header, multipart.type);
  if (res != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("multipart put error (%s).\n",
                        lagopus_error_get_string(res));
    if (res == LAGOPUS_RESULT_NO_MEMORY) {
      code = OFPBRC_MULTIPART_BUFFER_OVERFLOW;
    } else {
      code = OFPBRC_BAD_MULTIPART;
    }
    res = LAGOPUS_RESULT_OFP_ERROR;
    ofp_error_set(error, OFPET_BAD_REQUEST, code);
    goto done;
  }

  if (multipart.flags == 0) {
    /* multipart is last one. */
    res = channel_multipart_get(channel, &all, xid_header, multipart.type);
    if (res != LAGOPUS_RESULT_OK) {
      if (res == LAGOPUS_RESULT_NO_MEMORY) {
        code = OFPBRC_MULTIPART_BUFFER_OVERFLOW;
      } else {
        code = OFPBRC_BAD_MULTIPART;
      }
      res = LAGOPUS_RESULT_OFP_ERROR;
      ofp_error_set(error, OFPET_BAD_REQUEST, code);
      goto done;
    }
  } else {
    /* more multipart data on the fly. */
    goto done;
  }

  switch (multipart.type) {
    case OFPMP_DESC:
      res = ofp_desc_request_handle(channel, all, xid_header, error);
      break;
    case OFPMP_FLOW:
      res = ofp_flow_stats_request_handle(channel, all, xid_header, error);
      break;
    case OFPMP_AGGREGATE:
      res = ofp_aggregate_stats_request_handle(channel, all, xid_header, error);
      break;
    case OFPMP_TABLE:
      res = ofp_table_stats_request_handle(channel, all, xid_header, error);
      break;
    case OFPMP_PORT_STATS:
      res = ofp_port_stats_request_handle(channel, all, xid_header, error);
      break;
    case OFPMP_QUEUE:
      res = ofp_queue_stats_request_handle(channel, all, xid_header, error);
      break;
    case OFPMP_GROUP:
      res = ofp_group_stats_request_handle(channel, all, xid_header, error);
      break;
    case OFPMP_GROUP_DESC:
      res = ofp_group_desc_request_handle(channel, all, xid_header, error);
      break;
    case OFPMP_GROUP_FEATURES:
      res = ofp_group_features_request_handle(channel, all, xid_header, error);
      break;
    case OFPMP_METER:
      res = ofp_meter_stats_request_handle(channel, all, xid_header, error);
      break;
    case OFPMP_METER_CONFIG:
      res = ofp_meter_config_request_handle(channel, all, xid_header, error);
      break;
    case OFPMP_METER_FEATURES:
      res = ofp_meter_features_request_handle(channel, all, xid_header, error);
      break;
    case OFPMP_TABLE_FEATURES:
      res = ofp_table_features_request_handle(channel, all, xid_header, error);
      break;
    case OFPMP_PORT_DESC:
      res = ofp_port_desc_request_handle(channel, all, xid_header, error);
      break;
    case OFPMP_EXPERIMENTER:
      /* unsupport. */
      /* res = ofp_experimenter_mp_request_handle(channel, all, xid_header); */
      res = ofp_bad_experimenter_handle(error);
      break;
    default:
      lagopus_msg_warning("unknown multipart type.\n");
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_MULTIPART);
      res = LAGOPUS_RESULT_OFP_ERROR;
      break;
  }

  if (res == LAGOPUS_RESULT_OK && pbuf_plen_get(all) != 0) {
    lagopus_msg_warning("Decode failed : Over packet length(" PFSZ(
                          u) "), result : %s\n",
                        pbuf_plen_get(all), lagopus_error_get_string(res));
    switch (multipart.type) {
      case OFPMP_TABLE_FEATURES:
        ofp_error_set(error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);
        break;
      default:
        ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
        break;
    }
    res = LAGOPUS_RESULT_OFP_ERROR;
    goto done;
  }

done:
  if (all != NULL) {
    pbuf_free(all);
  }
  return res;
}

lagopus_result_t
ofp_multipart_length_set(uint8_t *multipart_head,
                         uint16_t length) {
  if (multipart_head != NULL) {
    *(multipart_head) = (uint8_t) ((length >> 8) & 0xFF);
    *(multipart_head + 1) = (uint8_t) (length & 0xFF);
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}
