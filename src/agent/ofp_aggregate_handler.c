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
#include "ofp_match.h"
#include "ofp_instruction.h"

/* create aggregate_stats reply. */
STATIC lagopus_result_t
ofp_aggregate_stats_reply_create(struct channel *channel,
                                 struct pbuf_list **pbuf_list,
                                 struct ofp_aggregate_stats_reply *aggre_reply,
                                 struct ofp_header *xid_header) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t length = 0;
  struct pbuf *pbuf = NULL;
  struct ofp_multipart_reply mp_reply;

  /* check params */
  if (channel != NULL && pbuf_list != NULL &&
      aggre_reply != NULL && xid_header != NULL) {
    /* alloc */
    *pbuf_list = pbuf_list_alloc();
    if (*pbuf_list != NULL) {
      pbuf = pbuf_list_last_get(*pbuf_list);
      if (pbuf != NULL) {
        /* set data. */
        memset(&mp_reply, 0, sizeof(mp_reply));
        ofp_header_set(&mp_reply.header,
                       channel_version_get(channel),
                       OFPT_MULTIPART_REPLY,
                       0, /* length set in ofp_header_length_set()  */
                       xid_header->xid);
        mp_reply.type = OFPMP_AGGREGATE;

        /* encode header, multipart reply */
        pbuf_plen_set(pbuf, pbuf_size_get(pbuf));
        res = ofp_multipart_reply_encode_list(*pbuf_list, &pbuf, &mp_reply);
        if (res == LAGOPUS_RESULT_OK) {
          res = ofp_aggregate_stats_reply_encode_list(*pbuf_list, &pbuf, aggre_reply);
          if (res == LAGOPUS_RESULT_OK) {
            /* set packet length */
            res = pbuf_length_get(pbuf, &length);
            if (res == LAGOPUS_RESULT_OK) {
              res = ofp_header_length_set(pbuf, length);
              if (res == LAGOPUS_RESULT_OK) {
                pbuf_plen_reset(pbuf);
              } else {
                lagopus_msg_warning("FAILED : ofp_header_length_set (%s).\n",
                                    lagopus_error_get_string(res));
              }
            }
          } else {
            lagopus_msg_warning("FAILED (%s).\n",
                                lagopus_error_get_string(res));
          }
        } else {
          lagopus_msg_warning("FAILED : ofp_multipart_reply_encode (%s).\n",
                              lagopus_error_get_string(res));
        }
      } else {
        /* pbuf_list_last_get returns NULL */
        res = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      /* pbuf_list_alloc returns NULL */
      res = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    /* params are NULL */
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return res;
}

/* Aggregate Request packet receive. */
lagopus_result_t
ofp_aggregate_stats_request_handle(struct channel *channel, struct pbuf *pbuf,
                                   struct ofp_header *xid_header,
                                   struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf_list *send_pbuf_list = NULL;
  struct ofp_aggregate_stats_request request;
  struct match_list match_list;
  struct ofp_aggregate_stats_reply aggre_reply;

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    /* parse aggregate_stats_request */
    res = ofp_aggregate_stats_request_decode(pbuf, &request);
    if (res == LAGOPUS_RESULT_OK) {
      /* init. */
      TAILQ_INIT(&match_list);
      memset(&aggre_reply, 0, sizeof(aggre_reply));

      /* decode */
      if ((res = ofp_match_parse(channel, pbuf, &match_list, error))
          != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("match decode error (%s)\n",
                            lagopus_error_get_string(res));
      } else if ((res = ofp_aggregate_stats_reply_get(
                          channel_dpid_get(channel),
                          &request, &match_list, &aggre_reply, error))
                 != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("aggregate_stats decode error (%s)\n",
                            lagopus_error_get_string(res));
      } else {                  /* decode success */
        /* create ofp_aggregate_stats_reply. */
        res = ofp_aggregate_stats_reply_create(channel, &send_pbuf_list,
                                               &aggre_reply, xid_header);
        if (res == LAGOPUS_RESULT_OK) {
          /* send aggregate_stats reply */
          res = channel_send_packet_list(channel, send_pbuf_list);
          if (res != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("Socket write error (%s).\n",
                                lagopus_error_get_string(res));
          }
        } else {
          lagopus_msg_warning("reply creation failed, (%s).\n",
                              lagopus_error_get_string(res));
        }
        pbuf_list_free(send_pbuf_list);
      }

      ofp_match_list_elem_free(&match_list);
    } else {
      lagopus_msg_warning("aggregate_stats_request decode error (%s)\n",
                          lagopus_error_get_string(res));
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      res = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return res;
}
