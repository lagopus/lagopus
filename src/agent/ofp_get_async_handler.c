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
#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/ofp_pdump.h"
#include "ofp_apis.h"
#include "ofp_element.h"
#include "ofp_role.h"

static void
get_async_request_trace(struct ofp_header *header) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_GET_ASYNC_REQUEST)) {
    lagopus_msg_pdump(TRACE_OFPT_GET_ASYNC_REQUEST, PDUMP_OFP_HEADER,
                      *header, "");
  }
}

STATIC lagopus_result_t
ofp_get_async_reply_create(struct channel *channel,
                           struct pbuf **pbuf,
                           struct ofp_header *xid_header) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_async_config async_config;

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL) {
    /* alloc */
    *pbuf = channel_pbuf_list_get(channel,
                                  sizeof(struct ofp_async_config));
    if (*pbuf != NULL) {
      pbuf_plen_set(*pbuf, sizeof(struct ofp_async_config));

      /* Copy packet_in_mask, port_status_mask, flow_removed_mask. */
      channel_role_mask_get(channel, &async_config);

      /* Fill in header. */
      ofp_header_set(&async_config.header,
                     channel_version_get(channel),
                     OFPT_GET_ASYNC_REPLY,
                     (uint16_t) pbuf_plen_get(*pbuf),
                     xid_header->xid);

      /* Encode message. */
      ret = ofp_async_config_encode(*pbuf, &async_config);

      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("FAILED : ofp_async_config_encode (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("Can't allocate pbuf.\n");
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }

    if (ret != LAGOPUS_RESULT_OK &&  *pbuf != NULL) {
      channel_pbuf_list_unget(channel, *pbuf);
      *pbuf = NULL;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_get_async_request_handle(struct channel *channel,
                             struct pbuf *pbuf,
                             struct ofp_header *xid_header,
                             struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *send_pbuf = NULL;

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {

    ret = pbuf_plen_check(pbuf, xid_header->length);

    if (ret == LAGOPUS_RESULT_OK) {
      /* Parse packet. */
      ret = ofp_header_handle(channel, pbuf, error);

      if (ret == LAGOPUS_RESULT_OK) {
        /* dump trace. */
        get_async_request_trace(xid_header);

        /* Reply send.*/
        ret = ofp_get_async_reply_create(channel, &send_pbuf,
                                         xid_header);

        if (ret == LAGOPUS_RESULT_OK) {
          channel_send_packet(channel, send_pbuf);
        } else {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
        ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
        ret = LAGOPUS_RESULT_OFP_ERROR;
      }

      if (ret != LAGOPUS_RESULT_OK && send_pbuf != NULL) {
        channel_pbuf_list_unget(channel, send_pbuf);
      }
    } else {
      lagopus_msg_warning("bad length.\n");
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
