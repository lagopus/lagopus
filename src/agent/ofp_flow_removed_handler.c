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
#include <sys/queue.h>
#include "lagopus_apis.h"
#include "openflow.h"
#include "openflow13packet.h"
#include "ofp_apis.h"
#include "channel.h"
#include "ofp_match.h"
#include "lagopus/flowdb.h"
#include "lagopus/eventq_data.h"

/* Free FlowRemoved. */
void
ofp_flow_removed_free(struct eventq_data *data) {
  if (data != NULL) {
    ofp_match_list_elem_free(&data->flow_removed.match_list);
    free(data);
  }
}

/* SEND */
STATIC lagopus_result_t
ofp_flow_removed_create(struct flow_removed *flow_removed,
                        struct pbuf **pbuf) {
  lagopus_result_t ret;
  uint8_t tmp_version = 0x00;
  uint32_t tmp_xid = 0x00;
  uint16_t tmp_length = 0;
  uint16_t match_total_len = 0;

  if (flow_removed != NULL && pbuf != NULL) {
    /* alloc */
    *pbuf = pbuf_alloc(OFP_PACKET_MAX_SIZE);

    if (*pbuf != NULL) {
      pbuf_plen_set(*pbuf, OFP_PACKET_MAX_SIZE);

      /* Fill in header. */
      /* tmp_* is replaced later. */
      ofp_header_set(&flow_removed->ofp_flow_removed.header, tmp_version,
                     OFPT_FLOW_REMOVED, tmp_length, tmp_xid);

      ret = ofp_flow_removed_encode(*pbuf, &flow_removed->ofp_flow_removed);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = ofp_match_list_encode(NULL, pbuf, &flow_removed->match_list,
                                    &match_total_len);

        if (ret == LAGOPUS_RESULT_OK) {
          /* set plen. */
          pbuf_plen_reset(*pbuf);

          ret = LAGOPUS_RESULT_OK;
        } else {
          lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("Can't allocate pbuf.\n");
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }

    /* free. */
    if (ret != LAGOPUS_RESULT_OK && *pbuf == NULL) {
      pbuf_free(*pbuf);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* Send FlowRemoved packet. */
lagopus_result_t
ofp_flow_removed_handle(struct flow_removed *flow_removed,
                        uint64_t dpid) {
  lagopus_result_t ret;
  struct pbuf *send_pbuf = NULL;

  if (flow_removed != NULL) {
    /* FlowRemoved. */
    ret = ofp_flow_removed_create(flow_removed, &send_pbuf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = ofp_role_channel_write(send_pbuf, dpid,
                                   OFPT_FLOW_REMOVED,
                                   flow_removed->ofp_flow_removed.reason);
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("Socket write error (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
    }
  } else {
    lagopus_msg_warning("Arg is NULL.\n");
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (send_pbuf != NULL) {
    pbuf_free(send_pbuf);
  }

  return ret;
}
