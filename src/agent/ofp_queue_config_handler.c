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
#include "channel.h"
#include "ofp_apis.h"
#include "lagopus/ofp_pdump.h"

static void
queue_get_config_request_trace(struct ofp_queue_get_config_request *req) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_QUEUE_GET_CONFIG_REQUEST)) {
    lagopus_msg_pdump(TRACE_OFPT_QUEUE_GET_CONFIG_REQUEST, PDUMP_OFP_HEADER,
                      req->header, "");
    lagopus_msg_pdump(TRACE_OFPT_QUEUE_GET_CONFIG_REQUEST,
                      PDUMP_OFP_QUEUE_GET_CONFIG_REQUEST,
                      *req, "");
  }
}

static void
s_queue_property_list_elem_free(struct queue_property_list
                                *queue_property_list) {
  struct queue_property *queue_property;

  while ((queue_property = TAILQ_FIRST(queue_property_list)) != NULL) {
    TAILQ_REMOVE(queue_property_list, queue_property, entry);
    pbuf_free(queue_property->data);
    free(queue_property);
  }
}

static void
s_packet_queue_list_elem_free(struct packet_queue_list *packet_queue_list) {
  struct packet_queue *packet_queue;

  while ((packet_queue = TAILQ_FIRST(packet_queue_list)) != NULL) {
    TAILQ_REMOVE(packet_queue_list, packet_queue, entry);
    s_queue_property_list_elem_free(&packet_queue->queue_property_list);
    free(packet_queue);
  }
}

static lagopus_result_t
s_ofp_port_no_check(uint32_t port, struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_OFP_ERROR;

  if (port != OFPP_ANY && port > OFPP_MAX) {
    lagopus_msg_warning("over OFPP_MAX.\n");
    ofp_error_set(error, OFPET_QUEUE_OP_FAILED, OFPQOFC_BAD_PORT);
    res = LAGOPUS_RESULT_OFP_ERROR;
  } else {
    res = LAGOPUS_RESULT_OK;
  }

  return res;
}


static lagopus_result_t
s_queue_prop_encode(struct pbuf *pbuf,
                    struct queue_property *queue_property) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;

  switch (queue_property->ofp.property) {
    case OFPQT_MIN_RATE:
      res = ofp_queue_prop_min_rate_encode(
              pbuf,
              (const struct ofp_queue_prop_min_rate *)&(queue_property->ofp));
      if (res != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning(
          "FAILED : ofp_queue_prop_min_rate_encode.\n");
      }
      break;

    case OFPQT_MAX_RATE:
      res = ofp_queue_prop_max_rate_encode(
              pbuf,
              (const struct ofp_queue_prop_max_rate *)&(queue_property->ofp));
      if (res != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning(
          "FAILED : ofp_queue_prop_max_rate_encode.\n");
      }
      break;

    case OFPQT_EXPERIMENTER:
      res = ofp_queue_prop_experimenter_encode(
              pbuf,
              (const struct ofp_queue_prop_experimenter *)&(queue_property->ofp));
      if (res != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning(
          "FAILED : ofp_queue_prop_experimenter_encode.\n");
      }
      break;

    default:
      lagopus_msg_warning("FAILED : unknown queue_property (%d).\n",
                          queue_property->ofp.property);
      res = LAGOPUS_RESULT_OUT_OF_RANGE;
      break;
  }
  return res;
}

static lagopus_result_t
s_packet_queue_encode(struct pbuf *pbuf,
                      struct packet_queue *packet_queue) {
  struct queue_property *queue_property = NULL;
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;

  /* encode queue. */
  res = ofp_packet_queue_encode(pbuf, &(packet_queue->ofp));
  if (res == LAGOPUS_RESULT_OK) {
    /* encode property_list. */
    if (TAILQ_EMPTY(&(packet_queue->queue_property_list)) == false) {
      TAILQ_FOREACH(queue_property, &(packet_queue->queue_property_list), entry) {
        res = s_queue_prop_encode(pbuf, queue_property);
        if (res != LAGOPUS_RESULT_OK) {
          break;
        }
      }
    } else {
      /* queue_property_list is empty */
      res = LAGOPUS_RESULT_OK;
    }
  } else {
    lagopus_msg_warning("FAILED : ofp_packet_queue_encode (%s).\n",
                        lagopus_error_get_string(res));
  }

  return res;
}

static lagopus_result_t
s_packet_queue_list_encode(struct pbuf *pbuf,
                           struct packet_queue_list *packet_queue_list) {
  struct packet_queue *packet_queue = NULL;
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;

  if (TAILQ_EMPTY(packet_queue_list) == true) {
    res = LAGOPUS_RESULT_OK;
  } else {
    TAILQ_FOREACH(packet_queue, packet_queue_list, entry) {
      res = s_packet_queue_encode(pbuf, packet_queue);
      if (res != LAGOPUS_RESULT_OK) {
        break;
      }
    }
  }

  return res;
}

/* Create queue get_config reply. */
STATIC lagopus_result_t
ofp_queue_get_config_reply_create(struct channel *channel,
                                  struct pbuf **pbuf,
                                  uint32_t port,
                                  struct packet_queue_list *packet_queue_list,
                                  struct ofp_header *xid_header) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_queue_get_config_reply reply;
  const size_t packet_size = sizeof(struct ofp_queue_get_config_reply);

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      packet_queue_list != NULL && xid_header != NULL) {
    /* alloc pbuf */
    *pbuf = channel_pbuf_list_get(channel, packet_size);
    if (*pbuf != NULL) {
      /* set data. */
      memset(&reply, 0, packet_size);
      ofp_header_set(&reply.header,
                     channel_version_get(channel),
                     OFPT_QUEUE_GET_CONFIG_REPLY,
                     (uint16_t)packet_size,
                     xid_header->xid);
      reply.port = port;

      /* Encode message. */
      pbuf_plen_set(*pbuf, packet_size);
      res = ofp_queue_get_config_reply_encode(*pbuf, &reply);
      if (res >= 0) {
        res = s_packet_queue_list_encode(*pbuf, packet_queue_list);
      } else {
        lagopus_msg_warning("FAILED : ofp_queue_get_config_reply_encode (%s).\n",
                            lagopus_error_get_string(res));
        channel_pbuf_list_unget(channel, *pbuf);
        *pbuf = NULL;
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

/* Get_Config Request packet receive. */
lagopus_result_t
ofp_queue_get_config_request_handle(struct channel *channel, struct pbuf *pbuf,
                                    struct ofp_header *xid_header,
                                    struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;
  struct pbuf *send_pbuf = NULL;
  struct ofp_queue_get_config_request request;
  struct packet_queue_list packet_queue_list;

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    /* Parse packet. */
    res = ofp_queue_get_config_request_decode(pbuf, &request);
    if (res != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("packet decode failed. (%s).\n",
                          lagopus_error_get_string(res));
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      res = LAGOPUS_RESULT_OFP_ERROR;
    } else if (pbuf_plen_get(pbuf) > 0) {
      lagopus_msg_warning("packet decode failed. (size over).\n");
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      res = LAGOPUS_RESULT_OFP_ERROR;
    } else {                    /* decode success */
      /* dump trace. */
      queue_get_config_request_trace(&request);

      res = s_ofp_port_no_check(request.port, error);
      if (res == LAGOPUS_RESULT_OK) {
        TAILQ_INIT(&packet_queue_list);

        dpid = channel_dpid_get(channel);
        /* get data */
        res = ofp_packet_queue_get(dpid,
                                   &request,
                                   &packet_queue_list,
                                   error);
        if (res == LAGOPUS_RESULT_OK) {
          /* create get_config reply. */
          res = ofp_queue_get_config_reply_create(channel, &send_pbuf,
                                                  request.port,
                                                  &packet_queue_list,
                                                  xid_header);
          if (res == LAGOPUS_RESULT_OK && send_pbuf != NULL) {
            /* send queue_config_get reply */
            channel_send_packet(channel, send_pbuf);
          } else {
            lagopus_msg_warning(
              "FAILED : ofp_queue_get_config_reply_create (%s).\n",
              lagopus_error_get_string(res));
          }
        }
        s_packet_queue_list_elem_free(&packet_queue_list);
      }
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return res;
}
