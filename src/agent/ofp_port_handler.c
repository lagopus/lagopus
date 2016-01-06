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

static void
s_ofp_port_stats_list_elem_free(struct port_stats_list *port_stats_list) {
  struct port_stats *port_stats;
  while ((port_stats = TAILQ_FIRST(port_stats_list)) != NULL) {
    TAILQ_REMOVE(port_stats_list, port_stats, entry);
    free(port_stats);
  }
}

static lagopus_result_t
s_port_stats_list_encode(struct pbuf_list *pbuf_list,
                         struct pbuf **pbuf,
                         struct port_stats_list *port_stats_list) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct port_stats *port_stats = NULL;

  if (TAILQ_EMPTY(port_stats_list) != true) {
    /* encode port_stats list */
    TAILQ_FOREACH(port_stats, port_stats_list, entry) {
      /* encode port_stats */
      res = ofp_port_stats_encode_list(pbuf_list, pbuf, &(port_stats->ofp));
      if (res != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(res));
        break;
      }
    }
  } else {
    res = LAGOPUS_RESULT_OK;
  }
  return res;
}

/* createport_stats reply. */
STATIC lagopus_result_t
ofp_port_stats_reply_create(struct channel *channel,
                            struct pbuf_list **pbuf_list,
                            struct port_stats_list *port_stats_list,
                            struct ofp_header *xid_header) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t length = 0;
  struct pbuf *pbuf = NULL;
  struct ofp_multipart_reply reply;

  /* check params */
  if (channel != NULL &&
      pbuf_list != NULL &&
      port_stats_list != NULL &&
      xid_header != NULL) {
    /* alloc */
    *pbuf_list = NULL;
    *pbuf_list = pbuf_list_alloc();
    if (*pbuf_list != NULL) {
      /* alloc&get tail of pbuf_list */
      pbuf = pbuf_list_last_get(*pbuf_list);
      if (pbuf != NULL) {
        /* set data. */
        memset(&reply, 0, sizeof(reply));
        ofp_header_set(&reply.header,
                       channel_version_get(channel),
                       OFPT_MULTIPART_REPLY,
                       0, /* length set in ofp_header_length_set()  */
                       xid_header->xid);
        reply.type = OFPMP_PORT_STATS;

        /* encode header, multipart reply */
        pbuf_plen_set(pbuf, pbuf_size_get(pbuf));
        res = ofp_multipart_reply_encode_list(*pbuf_list, &pbuf, &reply);
        if (res == LAGOPUS_RESULT_OK) {
          res = s_port_stats_list_encode(*pbuf_list, &pbuf, port_stats_list);
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

/* port Request packet receive. */
lagopus_result_t
ofp_port_stats_request_handle(struct channel *channel,
                              struct pbuf *pbuf,
                              struct ofp_header *xid_header,
                              struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;
  struct pbuf_list *send_pbuf_list = NULL;
  struct ofp_port_stats_request request;
  struct port_stats_list port_stats_list;

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    /* parse port_stats_request */
    res = ofp_port_stats_request_decode(pbuf, &request);
    if (res == LAGOPUS_RESULT_OK) {
      /* init. */
      TAILQ_INIT(&port_stats_list);
      dpid = channel_dpid_get(channel);
      res = ofp_port_stats_request_get(dpid,
                                       &request, &port_stats_list, error);
      if (res == LAGOPUS_RESULT_OK) { /* decode success */
        /* create port_stats_reply. */
        res = ofp_port_stats_reply_create(channel,
                                          &send_pbuf_list,
                                          &port_stats_list,
                                          xid_header);
        if (res == LAGOPUS_RESULT_OK) {
          /* send port_stats reply */
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
      } else {
        lagopus_msg_warning("ofp_port_stats_request_get error (%s)\n",
                            lagopus_error_get_string(res));
      }

      s_ofp_port_stats_list_elem_free(&port_stats_list);
    } else {
      lagopus_msg_warning("port_stats_request decode error (%s)\n",
                          lagopus_error_get_string(res));
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      res = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return res;
}
