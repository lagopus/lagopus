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
#include "openflow13.h"
#include "openflow13packet.h"
#include "channel.h"
#include "ofp_apis.h"
#include "ofp_instruction.h"

STATIC lagopus_result_t
ofp_meter_features_reply_create(struct channel *channel,
                                struct pbuf_list **pbuf_list,
                                struct ofp_meter_features *ofp_meter_features,
                                struct ofp_header *xid_header) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t length = 0;
  struct pbuf *pbuf = NULL;
  struct ofp_multipart_reply ofpmp_reply;

  /* check params */
  if (channel              != NULL &&
      pbuf_list            != NULL &&
      ofp_meter_features   != NULL &&
      xid_header           != NULL) {
    /* alloc */
    *pbuf_list = NULL;
    *pbuf_list = pbuf_list_alloc();
    if (*pbuf_list != NULL) {
      /* alloc&get tail of pbuf_list */
      pbuf = pbuf_list_last_get(*pbuf_list);
      if (pbuf != NULL) {
        /* set data. */
        memset(&ofpmp_reply, 0, sizeof(ofpmp_reply));
        ofp_header_set(&ofpmp_reply.header,
                       channel_version_get(channel),
                       OFPT_MULTIPART_REPLY,
                       0, /* length set in ofp_header_length_set()  */
                       xid_header->xid);
        ofpmp_reply.type = OFPMP_METER_FEATURES;

        /* encode message. */
        pbuf_plen_set(pbuf, pbuf_size_get(pbuf));
        res = ofp_multipart_reply_encode_list(*pbuf_list, &pbuf, &ofpmp_reply);
        if (res == LAGOPUS_RESULT_OK) {
          res = ofp_meter_features_encode_list(*pbuf_list, &pbuf, ofp_meter_features);
          if (res == LAGOPUS_RESULT_OK) {
            /* set packet length */
            res = pbuf_length_get(pbuf, &length);
            if (res == LAGOPUS_RESULT_OK) {
              res = ofp_header_length_set(pbuf, length);
              if (res == LAGOPUS_RESULT_OK) {
                pbuf_plen_reset(pbuf);
              } else {
                lagopus_msg_warning("FAILED (%s).\n",
                                    lagopus_error_get_string(res));
              }
            } else {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(res));
            }
          } else {
            lagopus_msg_warning("FAILED (%s).\n",
                                lagopus_error_get_string(res));
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(res));
        }
      } else {
        res = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      res = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return res;
}

lagopus_result_t
ofp_meter_features_request_handle(struct channel *channel,
                                  struct pbuf *pbuf,
                                  struct ofp_header *xid_header,
                                  struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;
  struct ofp_meter_features ofp_meter_features;
  struct pbuf_list *send_pbuf_list = NULL;

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    if (pbuf_plen_equal_check(pbuf, 0) == LAGOPUS_RESULT_OK) {
      /* get datas */
      memset(&ofp_meter_features, 0, sizeof(ofp_meter_features));
      dpid = channel_dpid_get(channel);
      res = ofp_meter_features_get(dpid, &ofp_meter_features, error);
      if (res == LAGOPUS_RESULT_OK) {
        /* create desc reply. */
        res = ofp_meter_features_reply_create(channel,
                                              &send_pbuf_list,
                                              &ofp_meter_features,
                                              xid_header);
        if (res == LAGOPUS_RESULT_OK) {
          /* send desc reply */
          res = channel_send_packet_list(channel, send_pbuf_list);
          if (res != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("Socket write error (%s).\n",
                                lagopus_error_get_string(res));
          }
        } else {
          lagopus_msg_warning("reply creation failed, (%s).\n",
                              lagopus_error_get_string(res));
        }
        /* free. */
        if (send_pbuf_list != NULL) {
          pbuf_list_free(send_pbuf_list);
        }
      }
    } else {
      lagopus_msg_warning("over packet length.\n");
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      res = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return res;
}
