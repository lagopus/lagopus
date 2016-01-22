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
#include "ofp_apis.h"

/* send */
/* Send experimenter reply. */
STATIC lagopus_result_t
ofp_experimenter_reply_create(struct channel *channel,
                              struct pbuf **pbuf,
                              struct ofp_header *xid_header,
                              struct ofp_experimenter_header *exper_req) {
  struct ofp_experimenter_header exper_reply;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && exper_req != NULL) {
    *pbuf = NULL;
    /* alloc */
    *pbuf = channel_pbuf_list_get(channel,
                                  sizeof(struct ofp_experimenter_header));
    if (*pbuf != NULL) {
      pbuf_plen_set(*pbuf, sizeof(struct ofp_experimenter_header));

      exper_reply.experimenter = exper_req->experimenter;
      exper_reply.exp_type = exper_req->exp_type;

      /* Fill in header. */
      ofp_header_set(&exper_reply.header, channel_version_get(channel),
                     OFPT_EXPERIMENTER, (uint16_t) pbuf_plen_get(*pbuf),
                     xid_header->xid);

      /* Encode message. */
      ret = ofp_experimenter_header_encode(*pbuf, &exper_reply);
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("Can't allocate pbuf.\n");
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }

    if (ret != LAGOPUS_RESULT_OK && *pbuf != NULL) {
      channel_pbuf_list_unget(channel, *pbuf);
      *pbuf = NULL;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* RECV */
/* Experimenter packet receive. */
lagopus_result_t
ofp_experimenter_request_handle(struct channel *channel, struct pbuf *pbuf,
                                struct ofp_header *xid_header) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *send_pbuf = NULL;
  struct ofp_experimenter_header exper_req;

  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL) {
    /* Parse packet. */
    ret = ofp_experimenter_header_decode(pbuf, &exper_req);
    if (ret == LAGOPUS_RESULT_OK) {
      /* Experimenter request reply. */
      ret = ofp_experimenter_reply_create(channel, &send_pbuf,
                                          xid_header, &exper_req);
      if (ret == LAGOPUS_RESULT_OK) {
        channel_send_packet(channel, send_pbuf);
        ret = LAGOPUS_RESULT_OK;
      } else {
        lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }

    if (ret != LAGOPUS_RESULT_OK && send_pbuf != NULL) {
      channel_pbuf_list_unget(channel, send_pbuf);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
