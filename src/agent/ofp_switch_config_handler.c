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
switch_get_config_request_trace(struct ofp_header *header) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_GET_CONFIG_REQUEST)) {
    lagopus_msg_pdump(TRACE_OFPT_GET_CONFIG_REQUEST, PDUMP_OFP_HEADER,
                      *header, "");
  }
}

static void
switch_set_config_trace(struct ofp_switch_config *switch_config) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_SET_CONFIG)) {
    lagopus_msg_pdump(TRACE_OFPT_SET_CONFIG, PDUMP_OFP_HEADER,
                      switch_config->header, "");
    lagopus_msg_pdump(TRACE_OFPT_SET_CONFIG, PDUMP_OFP_SET_CONFIG,
                      *switch_config, "");
  }
}

/* Set Config packet receive. */
static lagopus_result_t
ofp_switch_config_flag_validate(uint16_t flags) {
  return ((flags > OFPC_INVALID_TTL_TO_CONTROLLER) ? LAGOPUS_RESULT_INVALID_ARGS
          : LAGOPUS_RESULT_OK);
}

lagopus_result_t
ofp_set_config_handle(struct channel *channel,
                      struct pbuf *pbuf,
                      struct ofp_header *xid_header,
                      struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;
  struct ofp_switch_config switch_config;

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    /* check length. */
    if (pbuf_plen_equal_check(pbuf, (size_t) xid_header->length) ==
        LAGOPUS_RESULT_OK) {
      /* Parse packet. */
      res = ofp_switch_config_decode(pbuf, &switch_config);
      if (res != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("FAILED : ofp_switch_config_decode.\n");
        ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
        res = LAGOPUS_RESULT_OFP_ERROR;
      } else if (pbuf_plen_get(pbuf) != 0) {
        lagopus_msg_warning("packet decode failed. (size over).\n");
        ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
        res = LAGOPUS_RESULT_OFP_ERROR;
      } else {                    /* decode success */
        /* trace */
        switch_set_config_trace(&switch_config);

        /* valid parameters */
        if (ofp_switch_config_flag_validate(switch_config.flags) !=
            LAGOPUS_RESULT_OK) {
          res = LAGOPUS_RESULT_OFP_ERROR;
          ofp_error_set(error, OFPET_SWITCH_CONFIG_FAILED, OFPSCFC_BAD_FLAGS);
        } else if (switch_config.miss_send_len > OFPCML_MAX &&
                   switch_config.miss_send_len != OFPCML_NO_BUFFER) {
          res = LAGOPUS_RESULT_OFP_ERROR;
          ofp_error_set(error, OFPET_SWITCH_CONFIG_FAILED, OFPSCFC_BAD_LEN);
        } else {
          dpid = channel_dpid_get(channel);
          /* set ofp_switch_config. */
          if ((res = ofp_switch_config_set(dpid, &switch_config, error))
              != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("FAILED : %s.\n",
                                lagopus_error_get_string(res));
          }
        }
      }
    } else {
      lagopus_msg_warning("but packet length.\n");
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      res = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return res;
}

/* Create switch get_config reply. */
STATIC lagopus_result_t
ofp_get_config_reply_create(struct channel *channel, struct pbuf **pbuf,
                            struct ofp_switch_config *switch_config,
                            struct ofp_header *xid_header) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  const size_t packet_size = sizeof(struct ofp_switch_config);

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      switch_config != NULL && xid_header != NULL) {
    /* alloc pbuf */
    *pbuf = channel_pbuf_list_get(channel, packet_size);
    if (*pbuf != NULL) {
      /* set header. */
      ofp_header_set(&(switch_config->header),
                     channel_version_get(channel),
                     OFPT_GET_CONFIG_REPLY,
                     (uint16_t)packet_size,
                     xid_header->xid);

      /* Encode message. */
      pbuf_plen_set(*pbuf, packet_size);
      res = ofp_switch_config_encode(*pbuf, switch_config);
      if (res != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("FAILED : ofp_switch_config_encode.\n");
        channel_pbuf_list_unget(channel, *pbuf);
        *pbuf = NULL;
      } else {
        res = LAGOPUS_RESULT_OK;
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

/* RECV */
/* Get_Config Request packet receive. */
lagopus_result_t
ofp_get_config_request_handle(struct channel *channel, struct pbuf *pbuf,
                              struct ofp_header *xid_header,
                              struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;
  struct pbuf *send_pbuf = NULL;
  struct ofp_header header;
  struct ofp_switch_config switch_config;
  const size_t packet_size = sizeof(struct ofp_switch_config);

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    /* Parse packet. */
    res = ofp_header_decode(pbuf, &header);
    if (res != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("packet decode failed.\n");
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      res = LAGOPUS_RESULT_OFP_ERROR;
    } else if (pbuf_plen_get(pbuf) != 0) {
      lagopus_msg_warning("packet decode failed. (size over).\n");
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      res = LAGOPUS_RESULT_OFP_ERROR;
    } else {                    /* decode success.  */
      /* dump trace. */
      switch_get_config_request_trace(&header);

      dpid = channel_dpid_get(channel);
      /* get config */
      memset(&switch_config, 0, packet_size);
      if ((res = ofp_switch_config_get(dpid, &switch_config, error)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("FAILED : ofp_switch_config_flag_get (%s).\n",
                            lagopus_error_get_string(res));
      } else {                  /* get config success. */
        /* create get_config reply. */
        res = ofp_get_config_reply_create(channel, &send_pbuf, &switch_config,
                                          xid_header);
        if (res == LAGOPUS_RESULT_OK && send_pbuf != NULL) {
          /* send get_config reply */
          channel_send_packet(channel, send_pbuf);
        } else {
          lagopus_msg_warning("FAILED : %s.\n",
                              lagopus_error_get_string(res));
        }
      }
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return res;
}
