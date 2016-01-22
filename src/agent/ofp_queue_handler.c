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
#include "lagopus/ofp_dp_apis.h"

void
queue_stats_list_elem_free(struct queue_stats_list *queue_stats_list) {
  struct queue_stats *queue_stats;

  while ((queue_stats = TAILQ_FIRST(queue_stats_list)) != NULL) {
    TAILQ_REMOVE(queue_stats_list, queue_stats, entry);
    free(queue_stats);
  }
}

/* SEND */
STATIC lagopus_result_t
ofp_queue_stats_reply_create(struct channel *channel,
                             struct pbuf_list **pbuf_list,
                             struct queue_stats_list *queue_stats_list,
                             struct ofp_header *xid_header) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t tmp_length = 0;
  uint16_t length = 0;
  struct pbuf *pbuf = NULL;
  struct ofp_multipart_reply mp_reply;
  struct queue_stats *queue_stats = NULL;

  if (channel != NULL && pbuf_list != NULL &&
      queue_stats_list != NULL && xid_header != NULL) {

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

        mp_reply.type = OFPMP_QUEUE;
        mp_reply.flags = 0;

        /* Encode multipart reply. */
        ret = ofp_multipart_reply_encode(pbuf, &mp_reply);

        if (ret == LAGOPUS_RESULT_OK) {
          if (TAILQ_EMPTY(queue_stats_list) == false) {
            TAILQ_FOREACH(queue_stats, queue_stats_list, entry) {
              ret = ofp_queue_stats_encode_list(*pbuf_list, &pbuf,
                                                &queue_stats->ofp);
              if (ret != LAGOPUS_RESULT_OK) {
                lagopus_msg_warning("FAILED (%s).\n",
                                    lagopus_error_get_string(ret));
                break;
              }
            }
          } else {
            /* queue_stats_list is empty. */
            ret = LAGOPUS_RESULT_OK;
          }

          if (ret == LAGOPUS_RESULT_OK) {
            /* set length for last pbuf. */
            ret = pbuf_length_get(pbuf, &length);
            if (ret == LAGOPUS_RESULT_OK) {
              ret = ofp_header_length_set(pbuf, length);
              if (ret == LAGOPUS_RESULT_OK) {
                pbuf_plen_reset(pbuf);
              } else {
                lagopus_msg_warning("FAILED (%s).\n",
                                    lagopus_error_get_string(ret));
              }
            } else {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(ret));
            }
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

    /* free. */
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
/* Queue Stats Request packet receive. */
lagopus_result_t
ofp_queue_stats_request_handle(struct channel *channel, struct pbuf *pbuf,
                               struct ofp_header *xid_header,
                               struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;
  struct ofp_queue_stats_request ofp_queue_stats;
  struct pbuf_list *pbuf_list = NULL;
  struct queue_stats_list queue_stats_list;

  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    /* init list. */
    TAILQ_INIT(&queue_stats_list);

    ret = ofp_queue_stats_request_decode(pbuf, &ofp_queue_stats);
    if (ret == LAGOPUS_RESULT_OK) {
      dpid = channel_dpid_get(channel);

      ret = ofp_queue_stats_request_get(dpid,
                                        &ofp_queue_stats,
                                        &queue_stats_list,
                                        error);
      if (ret == LAGOPUS_RESULT_OK) {
        ret = ofp_queue_stats_reply_create(channel, &pbuf_list,
                                           &queue_stats_list,
                                           xid_header);
        if (ret == LAGOPUS_RESULT_OK) {
          /* write packets. */
          ret = channel_send_packet_list(channel, pbuf_list);
          if (ret != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("Can't write.\n");
          }
        } else {
          lagopus_msg_warning("FAILED (%s)\n",
                              lagopus_error_get_string(ret));
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("FAILED (%s)\n", lagopus_error_get_string(ret));
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }

    /* free. */
    queue_stats_list_elem_free(&queue_stats_list);
    if (pbuf_list != NULL) {
      pbuf_list_free(pbuf_list);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
