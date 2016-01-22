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

/* SEND */
/* Send experimenter multipart reply. */
STATIC lagopus_result_t
ofp_experimenter_mp_reply_create(
  struct channel *channel,
  struct pbuf_list **pbuf_list,
  struct ofp_header *xid_header,
  struct ofp_experimenter_multipart_header *exper_req) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t tmp_length = 0;
  uint16_t length = 0;
  struct ofp_multipart_reply mp_reply;
  struct pbuf *pbuf = NULL;
  struct ofp_experimenter_multipart_header exper_reply;

  if (channel != NULL && pbuf_list != NULL &&
      xid_header != NULL && exper_req != NULL) {

    /* alloc */
    *pbuf_list = pbuf_list_alloc();

    if (*pbuf_list != NULL) {
      pbuf = pbuf_list_last_get(*pbuf_list);

      if (pbuf != NULL) {
        pbuf_plen_set(pbuf, pbuf_size_get(pbuf));

        /* Fill in header. */
        memset(&mp_reply, 0, sizeof(mp_reply));
        ofp_header_set(&mp_reply.header, channel_version_get(channel),
                       OFPT_MULTIPART_REPLY, tmp_length, xid_header->xid);

        mp_reply.type = OFPMP_EXPERIMENTER;
        mp_reply.flags = 0;

        /* Encode multipart reply. */
        ret = ofp_multipart_reply_encode(pbuf, &mp_reply);

        if (ret == LAGOPUS_RESULT_OK) {
          exper_reply.experimenter = exper_req->experimenter;
          exper_reply.exp_type = exper_req->exp_type;

          /* Encode message. */
          ret = ofp_experimenter_multipart_header_encode_list(*pbuf_list,
                &pbuf,
                &exper_reply);

          if (ret == LAGOPUS_RESULT_OK) {
            /* set length for last pbuf. */
            ret = pbuf_length_get(pbuf, &length);
            if (ret == LAGOPUS_RESULT_OK) {
              ret = ofp_header_length_set(pbuf, length);
              if (ret == LAGOPUS_RESULT_OK) {
                pbuf_plen_reset(pbuf);
                ret = LAGOPUS_RESULT_OK;
              } else {
                lagopus_msg_warning("FAILED (%s).\n",
                                    lagopus_error_get_string(ret));
              }
            } else {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(ret));
            }
          } else {
            lagopus_msg_warning("FAILED (%s).\n",
                                lagopus_error_get_string(ret));
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
        }
      } else {
        lagopus_msg_warning("Can't allocate pbuf.\n");
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      lagopus_msg_warning("Can't allocate pbuf_list.\n");
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }

    if (ret != LAGOPUS_RESULT_OK && *pbuf_list != NULL) {
      pbuf_list_free(*pbuf_list);
      *pbuf_list = NULL;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* RECV */
/* Experimenter multipart packet receive. */
lagopus_result_t
ofp_experimenter_mp_request_handle(struct channel *channel, struct pbuf *pbuf,
                                   struct ofp_header *xid_header) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf_list *pbuf_list = NULL;
  struct ofp_experimenter_multipart_header exper_req;

  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL) {
    /* Parse packet. */
    ret = ofp_experimenter_multipart_header_decode(pbuf, &exper_req);
    if (ret == LAGOPUS_RESULT_OK) {
      /* Experimenter request reply. */
      ret = ofp_experimenter_mp_reply_create(channel, &pbuf_list,
                                             xid_header, &exper_req);
      if (ret == LAGOPUS_RESULT_OK) {
        ret = channel_send_packet_list(channel, pbuf_list);
        if (ret != LAGOPUS_RESULT_OK) {
          lagopus_msg_warning("Can't write.\n");
          ret = LAGOPUS_RESULT_OFP_ERROR;
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }

    /* free. */
    if (pbuf_list != NULL) {
      pbuf_list_free(pbuf_list);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
