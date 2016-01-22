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
#include "lagopus/ofp_pdump.h"
#include "openflow.h"
#include "openflow13packet.h"
#include "ofp_apis.h"

#define TRACE_OFPT_ECHO (TRACE_OFPT_ECHO_REQUEST | \
                         TRACE_OFPT_ECHO_REPLY)

static void
echo_trace(struct ofp_header *header) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_ECHO)) {
    lagopus_msg_pdump(TRACE_OFPT_ECHO, PDUMP_OFP_HEADER,
                      *header, "");
  }
}

/* SEND */
/* Send switch features reply. */
STATIC lagopus_result_t
ofp_echo_reply_create(struct channel *channel,
                      struct pbuf **pbuf,
                      struct pbuf *req_pbuf,
                      struct ofp_header *xid_header) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t pbuf_length = 0;
  size_t length;
  struct ofp_header header;

  /* create reply. */
  if (channel != NULL && pbuf != NULL &&
      req_pbuf != NULL && xid_header != NULL) {
    if (pbuf_plen_get(req_pbuf) == 0) {
      length = sizeof(struct ofp_header);
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = pbuf_length_get(req_pbuf, &pbuf_length);
      if (ret == LAGOPUS_RESULT_OK) {
        length = sizeof(struct ofp_header) +
                 (size_t) pbuf_length;
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
      }
    }

    if (ret == LAGOPUS_RESULT_OK) {
      *pbuf = channel_pbuf_list_get(channel, length);
      if (*pbuf != NULL) {
        /* reply length equal request length. */
        pbuf_plen_set(*pbuf, (size_t) length);

        ret = ofp_header_create(channel, OFPT_ECHO_REPLY, xid_header,
                                &header, *pbuf);
        if (ret == LAGOPUS_RESULT_OK) {
          if (pbuf_plen_get(req_pbuf) != 0) {
            ret = pbuf_copy(*pbuf, req_pbuf);

            if (ret == LAGOPUS_RESULT_OK) {
              pbuf_plen_reset(req_pbuf);
              pbuf_plen_reset(*pbuf);
            } else {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(ret));
            }
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
        }
      } else {
        lagopus_msg_warning("Can't allocate pbuf.\n");
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }

      /* free. */
      if (ret != LAGOPUS_RESULT_OK && *pbuf != NULL) {
        channel_pbuf_list_unget(channel, *pbuf);
        *pbuf = NULL;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

STATIC lagopus_result_t
ofp_echo_request_create(struct channel *channel,
                        struct pbuf **pbuf,
                        struct ofp_header *xid_header) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_header header;

  if (channel != NULL && pbuf != NULL) {
    /* create reply. */
    *pbuf = channel_pbuf_list_get(channel, sizeof(struct ofp_header));

    if (*pbuf != NULL) {
      pbuf_plen_set(*pbuf, sizeof(struct ofp_header));

      ret = ofp_header_create(channel, OFPT_ECHO_REQUEST, xid_header,
                              &header, *pbuf);
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("Can't allocate pbuf.\n");
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }

    /* free. */
    if (ret != LAGOPUS_RESULT_OK && *pbuf != NULL) {
      channel_pbuf_list_unget(channel, *pbuf);
      *pbuf = NULL;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_echo_request_send(struct channel *channel) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *send_pbuf = NULL;

  if (channel != NULL) {
    /* Echo reply send. */
    ret = ofp_echo_request_create(channel, &send_pbuf, NULL);
    if (ret == LAGOPUS_RESULT_OK) {
      channel_send_packet_by_event(channel, send_pbuf);
      ret = LAGOPUS_RESULT_OK;
    } else {
      lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  /* free. */
  if (ret != LAGOPUS_RESULT_OK && send_pbuf != NULL) {
    channel_pbuf_list_unget(channel, send_pbuf);
  }

  return ret;
}

/* RECV */
/* Echo Request packet receive. */
lagopus_result_t
ofp_echo_request_handle(struct channel *channel, struct pbuf *pbuf,
                        struct ofp_header *xid_header, struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_header header;
  struct pbuf *send_pbuf = NULL;

  if (channel != NULL && pbuf != NULL && xid_header != NULL) {
    /* Parse packet. */
    ret = ofp_header_decode(pbuf, &header);
    if (ret == LAGOPUS_RESULT_OK) {
      /* dump trace. */
      echo_trace(&header);

      /* Echo reply send. */
      ret = ofp_echo_reply_create(channel, &send_pbuf, pbuf, xid_header);
      if (ret == LAGOPUS_RESULT_OK) {
        channel_send_packet(channel, send_pbuf);
        ret = LAGOPUS_RESULT_OK;
      } else {
        lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      return LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  /* free. */
  if (ret != LAGOPUS_RESULT_OK && send_pbuf != NULL) {
    channel_pbuf_list_unget(channel, send_pbuf);
  }

  return ret;
}

lagopus_result_t
ofp_echo_reply_handle(struct channel *channel,
                      struct pbuf *pbuf,
                      struct ofp_header *xid_header,
                      struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (channel != NULL && pbuf != NULL &&
      error != NULL) {
    /* Parse packet. */
    ret = ofp_header_handle(channel, pbuf, error);

    if (ret == LAGOPUS_RESULT_OK) {
      /* dump trace. */
      echo_trace(xid_header);
    } else {
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
