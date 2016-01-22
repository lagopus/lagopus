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
role_request_trace(struct ofp_role_request *role_request) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_ROLE_REQUEST)) {
    lagopus_msg_pdump(TRACE_OFPT_ROLE_REQUEST, PDUMP_OFP_HEADER,
                      role_request->header, "");
    lagopus_msg_pdump(TRACE_OFPT_ROLE_REQUEST, PDUMP_OFP_ROLE_REQUEST,
                      *role_request, "");
  }
}

static lagopus_result_t
role_check(uint32_t role, struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_OFP_ERROR;

  switch (role) {
    case OFPCR_ROLE_NOCHANGE:
    case OFPCR_ROLE_EQUAL:
    case OFPCR_ROLE_MASTER:
    case OFPCR_ROLE_SLAVE:
      ret = LAGOPUS_RESULT_OK;
      break;
    default:
      lagopus_msg_warning("unknown role.\n");
      ofp_error_set(error, OFPET_ROLE_REQUEST_FAILED, OFPRRFC_BAD_ROLE);
      ret = LAGOPUS_RESULT_OFP_ERROR;
      break;
  }

  return ret;
}

STATIC lagopus_result_t
ofp_role_reply_create(struct channel *channel,
                      struct pbuf **pbuf,
                      struct ofp_header *xid_header,
                      struct ofp_role_request *role_request) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  /* reply is ofp_role_request structure. */
  struct ofp_role_request role_reply;

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && role_request != NULL) {
    /* alloc */
    *pbuf = channel_pbuf_list_get(channel,
                                  sizeof(struct ofp_role_request));
    if (*pbuf != NULL) {
      pbuf_plen_set(*pbuf, sizeof(struct ofp_role_request));

      /* Fill in header. */
      ofp_header_set(&role_reply.header,
                     channel_version_get(channel),
                     OFPT_ROLE_REPLY,
                     (uint16_t) pbuf_plen_get(*pbuf),
                     xid_header->xid);

      role_reply.role = role_request->role;
      memset(role_reply.pad, 0, sizeof(role_request->pad));
      role_reply.generation_id = role_request->generation_id;

      /* Encode message. */
      ret = ofp_role_request_encode(*pbuf, &role_reply);
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
      }

      if (ret != LAGOPUS_RESULT_OK && *pbuf != NULL) {
        channel_pbuf_list_unget(channel, *pbuf);
        *pbuf = NULL;
      }
    } else {
      lagopus_msg_warning("Can't allocate pbuf.\n");
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_role_request_handle(struct channel *channel,
                        struct pbuf *pbuf,
                        struct ofp_header *xid_header,
                        struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_role_request role_request;
  struct ofp_role_request modified_role_request;
  struct pbuf *send_pbuf = NULL;
  uint64_t dpid;

  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    /* Parse packet. */
    ret = ofp_role_request_decode(pbuf, &role_request);

    if (ret == LAGOPUS_RESULT_OK) {
      /* dump trace. */
      role_request_trace(&role_request);

      ret = role_check(role_request.role, error);

      if (ret == LAGOPUS_RESULT_OK) {
        dpid = channel_dpid_get(channel);
        if (ofp_role_generation_id_check(dpid, role_request.role,
                                         role_request.generation_id) == true) {
          /* copy role request. */
          modified_role_request = role_request;
          ret = ofp_role_channel_update(channel,
                                        &modified_role_request,
                                        dpid);
          if (ret == LAGOPUS_RESULT_OK) {
            /* Reply send.*/
            ret = ofp_role_reply_create(channel, &send_pbuf,
                                        xid_header, &modified_role_request);
            if (ret == LAGOPUS_RESULT_OK) {
              channel_send_packet(channel, send_pbuf);
            } else {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(ret));
            }
          } else {
            lagopus_msg_warning("FAILED (%s).\n",
                                lagopus_error_get_string(ret));
          }
        } else {
          lagopus_msg_warning("bad generation_id.\n");
          ofp_error_set(error, OFPET_ROLE_REQUEST_FAILED, OFPRRFC_STALE);
          ret = LAGOPUS_RESULT_OFP_ERROR;
        }

        if (ret != LAGOPUS_RESULT_OK && send_pbuf != NULL) {
          channel_pbuf_list_unget(channel, send_pbuf);
        }
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
