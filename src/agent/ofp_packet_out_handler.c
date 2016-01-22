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
#include "lagopus/flowdb.h"
#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/ofp_pdump.h"

static void
packet_out_trace(struct ofp_packet_out *packet_out,
                 struct action_list *action_list) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_PACKET_OUT)) {
    lagopus_msg_pdump(TRACE_OFPT_PACKET_OUT, PDUMP_OFP_HEADER,
                      packet_out->header, "");
    lagopus_msg_pdump(TRACE_OFPT_PACKET_OUT, PDUMP_OFP_PACKET_OUT,
                      *packet_out, "");
    ofp_action_list_trace(TRACE_OFPT_PACKET_OUT, action_list);
  }
}

/* Freeup PacketOut. */
void
ofp_packet_out_free(struct eventq_data *data) {
  if (data != NULL) {
    ofp_action_list_elem_free(&(data->packet_out.action_list));
    if (data->packet_out.data != NULL) {
      pbuf_free(data->packet_out.data);
    }
    if (data->packet_out.req != NULL) {
      pbuf_free(data->packet_out.req);
    }
    free(data);
  }
}

/* RECV */
/* Packetout packet receive. */
lagopus_result_t
ofp_packet_out_handle(struct channel *channel, struct pbuf *pbuf,
                      struct ofp_header *xid_header,
                      struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  struct pbuf *data_pbuf = NULL;
  struct pbuf *req_pbuf = NULL;
  uint64_t dpid;
  uint16_t data_len = 0;

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    dpid = channel_dpid_get(channel);

    /* create packet_out */
    eventq_data = malloc(sizeof(*eventq_data));
    if (eventq_data != NULL) {
      memset(eventq_data, 0, sizeof(*eventq_data));

      /* Init action-list. */
      TAILQ_INIT(&eventq_data->packet_out.action_list);
      eventq_data->packet_out.data = NULL;
      eventq_data->packet_out.req = NULL;

      /* decode. */
      if ((res = ofp_packet_out_decode(
                   pbuf, &(eventq_data->packet_out.ofp_packet_out))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("packet_out decode error.\n");
        ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
        res = LAGOPUS_RESULT_OFP_ERROR;
      } else if ((res = ofp_action_parse(
                          pbuf,
                          eventq_data->packet_out.ofp_packet_out.actions_len,
                          &(eventq_data->packet_out.action_list), error)) !=
                 LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("action_list decode error.\n");
      } else {                  /* decode success */
        /* set eventq_data members */
        eventq_data->type = LAGOPUS_EVENTQ_PACKET_OUT;
        eventq_data->free = ofp_packet_out_free;
        eventq_data->packet_out.channel_id = channel_id_get(channel);

        /* copy packet_out.data if needed */
        res = pbuf_length_get(pbuf, &data_len);
        if (res == LAGOPUS_RESULT_OK) {
          if (data_len != 0) {
            if (eventq_data->packet_out.ofp_packet_out.buffer_id ==
                OFP_NO_BUFFER) {
              /* alloc packet_out.data */
              data_pbuf = pbuf_alloc(data_len);
              if (data_pbuf != NULL) {
                res = pbuf_copy_with_length(data_pbuf, pbuf, data_len);

                if (res == LAGOPUS_RESULT_OK) {
                  eventq_data->packet_out.data = data_pbuf;
                } else {
                  lagopus_msg_warning("FAILED (%s).\n",
                                      lagopus_error_get_string(res));
                }
              } else {
                lagopus_msg_warning("Can't allocate data_pbuf.\n");
                res = LAGOPUS_RESULT_NO_MEMORY;
              }
            } else {
              lagopus_msg_warning("Not empty data filed in request(buffer_id = %x).\n",
                                  eventq_data->packet_out.ofp_packet_out.buffer_id);
              ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BUFFER_UNKNOWN);
              res = LAGOPUS_RESULT_OFP_ERROR;
            }
          } else {
            res = LAGOPUS_RESULT_OK;
          }

          /* Copy request for ofp_error. */
          if (res == LAGOPUS_RESULT_OK && error->req != NULL) {
            req_pbuf = pbuf_alloc(OFP_ERROR_MAX_SIZE);
            if (req_pbuf != NULL) {
              res = pbuf_copy(req_pbuf, error->req);

              if (res == LAGOPUS_RESULT_OK) {
                eventq_data->packet_out.req = req_pbuf;
              } else {
                lagopus_msg_warning("FAILED (%s).\n",
                                    lagopus_error_get_string(res));
              }
            } else {
              lagopus_msg_warning("Can't allocate data_pbuf.\n");
              res = LAGOPUS_RESULT_NO_MEMORY;
            }
          }

          if (res == LAGOPUS_RESULT_OK) {
            /* dump trace.*/
            packet_out_trace(&eventq_data->packet_out.ofp_packet_out,
                             &eventq_data->packet_out.action_list);

            /* send to DataPlane */
            res = ofp_handler_event_dataq_put(dpid, eventq_data);
            if (res != LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(res));
            }
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(res));
        }
      }

      if (res != LAGOPUS_RESULT_OK && eventq_data != NULL) {
        ofp_packet_out_free(eventq_data);
      }
    } else {
      /* channel_pbuf_list_get returns NULL */
      res = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    /* params are NULL */
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return res;
}
