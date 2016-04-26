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
#include "lagopus/ofp_dp_apis.h"
#include "lagopus_apis.h"
#include "openflow.h"
#include "openflow13.h"
#include "openflow13packet.h"
#include "channel.h"
#include "ofp_apis.h"
#include "ofp_match.h"
#include "ofp_instruction.h"
#include "ofp_tlv.h"

static void
s_ofp_flow_stats_list_elem_free(struct flow_stats_list *flow_stats_list) {
  struct flow_stats *flow_stats;

  while ((flow_stats = TAILQ_FIRST(flow_stats_list)) != NULL) {
    TAILQ_REMOVE(flow_stats_list, flow_stats, entry);
    ofp_match_list_elem_free(&flow_stats->match_list);
    ofp_instruction_list_elem_free(&flow_stats->instruction_list);
    free(flow_stats);
  }
}

static lagopus_result_t
s_flow_stats_list_encode(struct pbuf_list *pbuf_list,
                         struct pbuf **pbuf,
                         struct flow_stats_list *flow_stats_list) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t match_total_len = 0;
  uint16_t instruction_total_len = 0;
  uint16_t flow_stats_len;
  uint8_t *flow_stats_head = NULL;
  struct flow_stats *flow_stats = NULL;

  if (TAILQ_EMPTY(flow_stats_list) == false) {
    /* encode flow_stats list */
    TAILQ_FOREACH(flow_stats, flow_stats_list, entry) {
      /* encode flow_stats */
      res = ofp_flow_stats_encode_list(pbuf_list, pbuf, &(flow_stats->ofp));
      if (res == LAGOPUS_RESULT_OK) {

        /* flow_stats head pointer. */
        flow_stats_head = pbuf_putp_get(*pbuf) - sizeof(struct ofp_flow_stats);

        /* encode match */
        res = ofp_match_list_encode(pbuf_list, pbuf, &(flow_stats->match_list),
                                    &match_total_len);
        if (res == LAGOPUS_RESULT_OK) {
          /* encode instruction */
          res = ofp_instruction_list_encode(pbuf_list, pbuf,
                                            &(flow_stats->instruction_list),
                                            &instruction_total_len);
          if (res == LAGOPUS_RESULT_OK) {
            /* Set flow_stats length (match total length +       */
            /*                        instruction total length + */
            /*                        size of ofp_flow_stats).   */
            /* And check overflow.                               */
            flow_stats_len = match_total_len;
            res = ofp_tlv_length_sum(&flow_stats_len, instruction_total_len);
            if (res == LAGOPUS_RESULT_OK) {
              res = ofp_tlv_length_sum(&flow_stats_len,
                                       sizeof(struct ofp_flow_stats));
              if (res == LAGOPUS_RESULT_OK) {
                res = ofp_multipart_length_set(flow_stats_head,
                                               flow_stats_len);
                if (res != LAGOPUS_RESULT_OK) {
                  lagopus_msg_warning("FAILED (%s).\n",
                                      lagopus_error_get_string(res));
                }
              } else {
                lagopus_msg_warning("over flow_stats length.\n");
                break;
              }
            } else {
              lagopus_msg_warning("over flow_stats length.\n");
              break;
            }
          } else {
            lagopus_msg_warning("FAILED : ofp_instruction_list_encode (%s).\n",
                                lagopus_error_get_string(res));
            break;
          }
        } else {
          lagopus_msg_warning("FAILED : ofp_match_list_encode (%s).\n",
                              lagopus_error_get_string(res));
          break;
        }
      } else {
        lagopus_msg_warning("FAILED : ofp_flow_stats_encode (%s).\n",
                            lagopus_error_get_string(res));
        break;
      }
    }
  } else {
    /* flow_stats_list is empty. */
    res = LAGOPUS_RESULT_OK;
  }
  return res;
}

/* createflow_stats reply. */
STATIC lagopus_result_t
ofp_flow_stats_reply_create(struct channel *channel,
                            struct pbuf_list **pbuf_list,
                            struct flow_stats_list *flow_stats_list,
                            struct ofp_header *xid_header) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t length = 0;
  struct pbuf *pbuf = NULL;
  struct ofp_multipart_reply reply;

  /* check params */
  if (channel != NULL && pbuf_list != NULL &&
      flow_stats_list != NULL && xid_header != NULL) {
    /* alloc */
    *pbuf_list = pbuf_list_alloc();
    if (*pbuf_list != NULL) {
      pbuf = pbuf_list_last_get(*pbuf_list);
      if (pbuf != NULL) {
        /* set data. */
        memset(&reply, 0, sizeof(reply));
        ofp_header_set(&reply.header,
                       channel_version_get(channel),
                       OFPT_MULTIPART_REPLY,
                       0, /* length set in ofp_header_length_set()  */
                       xid_header->xid);
        reply.type = OFPMP_FLOW;

        /* encode header, multipart reply */
        pbuf_plen_set(pbuf, pbuf_size_get(pbuf));
        res = ofp_multipart_reply_encode_list(*pbuf_list, &pbuf, &reply);
        if (res == LAGOPUS_RESULT_OK) {
          res = s_flow_stats_list_encode(*pbuf_list, &pbuf, flow_stats_list);
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
            } else {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(res));
            }
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

/* Flow Request packet receive. */
lagopus_result_t
ofp_flow_stats_request_handle(struct channel *channel, struct pbuf *pbuf,
                              struct ofp_header *xid_header,
                              struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf_list *send_pbuf_list = NULL;
  struct ofp_flow_stats_request request;
  struct match_list match_list;
  struct flow_stats_list flow_stats_list;

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    /* parse flow_stats_request */
    res = ofp_flow_stats_request_decode(pbuf, &request);
    if (res == LAGOPUS_RESULT_OK) {
      /* init. */
      TAILQ_INIT(&match_list);
      TAILQ_INIT(&flow_stats_list);

      /* decode */
      if ((res = ofp_match_parse(channel, pbuf, &match_list, error))
          != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("match decode error (%s)\n",
                            lagopus_error_get_string(res));
      } else if ((res = ofp_flow_stats_get(
                          channel_dpid_get(channel),
                          &request, &match_list, &flow_stats_list, error))
                 != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("flow_stats decode error (%s)\n",
                            lagopus_error_get_string(res));
      } else {                  /* decode success */
        /* create flow_stats_reply. */
        res = ofp_flow_stats_reply_create(channel, &send_pbuf_list,
                                          &flow_stats_list,
                                          xid_header);
        if (res == LAGOPUS_RESULT_OK) {
          /* send flow_stats reply */
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
      s_ofp_flow_stats_list_elem_free(&flow_stats_list);
    } else {
      lagopus_msg_warning("flow_stats_request decode error (%s)\n",
                          lagopus_error_get_string(res));
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      res = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    /* params are NULL */
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return res;
}
