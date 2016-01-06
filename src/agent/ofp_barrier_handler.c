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
#include "ofp_action.h"
#include "channel_mgr.h"
#include "lagopus/flowdb.h"
#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_pdump.h"

/* Freeup Barrier. */
void
ofp_barrier_free(struct eventq_data *data) {
  if (data != NULL) {
    if (data->barrier.req != NULL) {
      pbuf_free(data->barrier.req);
    }
    free(data);
  }
}

#define TRACE_OFPT_BARRIER (TRACE_OFPT_BARRIER_REQUEST | \
                            TRACE_OFPT_BARRIER_REPLY)

static void
barrier_trace(struct ofp_header *header) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_BARRIER)) {
    lagopus_msg_pdump(TRACE_OFPT_BARRIER, PDUMP_OFP_HEADER,
                      *header, "");
  }
}

/* RECV */
/* BarrierRequest packet receive. */
lagopus_result_t
ofp_barrier_request_handle(struct channel *channel, struct pbuf *pbuf,
                           struct ofp_header *xid_header,
                           struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  struct pbuf *req_pbuf = NULL;
  uint64_t dpid;
  (void) pbuf;

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    /* check length. */
    if (pbuf_plen_equal_check(pbuf, sizeof(struct ofp_header)) ==
        LAGOPUS_RESULT_OK) {
      /* dump trace. */
      barrier_trace(xid_header);

      dpid = channel_dpid_get(channel);

      /* create Barrier data. */
      eventq_data = malloc(sizeof(*eventq_data));

      if (eventq_data != NULL) {
        memset(eventq_data, 0, sizeof(*eventq_data));

        /* set eventq_data members */
        eventq_data->type = LAGOPUS_EVENTQ_BARRIER_REQUEST;
        eventq_data->free = ofp_barrier_free;
        eventq_data->barrier.type = xid_header->type;
        eventq_data->barrier.xid = xid_header->xid;
        eventq_data->barrier.channel_id = channel_id_get(channel);
        eventq_data->barrier.req = NULL;

        /* set ret val. */
        ret = LAGOPUS_RESULT_OK;
        /* Copy request for ofp_error. */
        if (error->req != NULL) {
          req_pbuf = pbuf_alloc(OFP_ERROR_MAX_SIZE);
          if (req_pbuf != NULL) {
            ret = pbuf_copy(req_pbuf, error->req);

            if (ret == LAGOPUS_RESULT_OK) {
              eventq_data->barrier.req = req_pbuf;
            } else {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(ret));
            }
          } else {
            lagopus_msg_warning("Can't allocate data_pbuf.\n");
            ret = LAGOPUS_RESULT_NO_MEMORY;
          }
        }

        if (ret == LAGOPUS_RESULT_OK) {
          /* send to DataPlane */
          ret = ofp_handler_event_dataq_put(dpid, eventq_data);
          if (ret != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("FAILED (%s).\n",
                                lagopus_error_get_string(ret));
          }
        }
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      lagopus_msg_warning("Not match header length.\n");
      ret = LAGOPUS_RESULT_OFP_ERROR;
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
    }

    /* free. */
    if (ret != LAGOPUS_RESULT_OK) {
      ofp_barrier_free(eventq_data);
    }
  } else {
    lagopus_msg_warning("Arg is NULL.\n");
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* SEND */
STATIC lagopus_result_t
ofp_barrier_reply_create(struct channel *channel,
                         struct barrier *barrier,
                         struct pbuf **pbuf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t length;
  struct ofp_header header;
  struct ofp_header xid_header;

  if (channel != NULL && barrier != NULL &&
      pbuf != NULL) {

    /* alloc */
    length = sizeof(struct ofp_header);
    *pbuf = NULL;
    *pbuf = channel_pbuf_list_get(channel, length);

    if (*pbuf != NULL) {
      pbuf_plen_set(*pbuf, length);

      /* Fill in header. */
      /* tmp_* is replaced later. */
      xid_header.xid = barrier->xid;
      ret = ofp_header_create(channel, OFPT_BARRIER_REPLY,
                              &xid_header,
                              &header, *pbuf);
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("Can't allocate pbuf.\n");
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }

    if (ret != LAGOPUS_RESULT_OK && *pbuf != NULL) {
      channel_pbuf_list_unget(channel, *pbuf);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* Send BarrierReply packet. */

lagopus_result_t
ofp_barrier_reply_send(struct channel *channel,
                       struct barrier *barrier,
                       uint64_t dpid) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *send_pbuf = NULL;
  (void) dpid;

  if (channel != NULL && barrier != NULL) {
    /* BarrierReply. */
    ret = ofp_barrier_reply_create(channel, barrier, &send_pbuf);

    if (ret == LAGOPUS_RESULT_OK) {
      channel_send_packet(channel, send_pbuf);
    } else {
      lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
    }
  } else {
    lagopus_msg_warning("Arg is NULL.\n");
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (ret != LAGOPUS_RESULT_OK && send_pbuf != NULL) {
    channel_pbuf_list_unget(channel, send_pbuf);
  }

  return ret;
}

lagopus_result_t
ofp_barrier_reply_handle(struct barrier *barrier,
                         uint64_t dpid) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = NULL;

  if (barrier != NULL) {
    channel_mgr_channel_lookup_by_channel_id(dpid,
        barrier->channel_id, &channel);
    if (channel != NULL) {
      ret = ofp_barrier_reply_send(channel, barrier, dpid);
    } else {
      lagopus_msg_warning("Not found channel.\n");
      ret = LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    lagopus_msg_warning("Arg is NULL.\n");
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
