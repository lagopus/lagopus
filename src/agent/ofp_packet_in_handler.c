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
#include "channel.h"
#include "ofp_match.h"
#include "ofp_padding.h"
#include "lagopus/flowdb.h"
#include "lagopus/eventq_data.h"

/* Free PacketIn. */
void
ofp_packet_in_free(struct eventq_data *data) {
  if (data != NULL) {
    ofp_match_list_elem_free(&data->packet_in.match_list);
    if (data->packet_in.data != NULL) {
      pbuf_free(data->packet_in.data);
    }
    free(data);
  }
}

#define OFP_PACKET_IN_PAD 2

/* SEND */
STATIC lagopus_result_t
ofp_packet_in_create(struct packet_in *packet_in,
                     struct pbuf **pbuf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t miss_send_len;
  uint16_t length = 0;
  uint16_t remain_length = 0;
  uint16_t match_total_len = 0;
  uint8_t tmp_version = 0x00;
  uint32_t tmp_xid = 0x00;
  uint16_t tmp_length = 0;

  if (packet_in != NULL && packet_in->data != NULL &&
      pbuf != NULL) {

    /* alloc */
    *pbuf = pbuf_alloc(OFP_PACKET_MAX_SIZE);

    if (*pbuf != NULL) {
      pbuf_plen_set(*pbuf, OFP_PACKET_MAX_SIZE);

      /* set total_len. */
      ret = pbuf_length_get(packet_in->data, &length);
      if (ret == LAGOPUS_RESULT_OK) {
        packet_in->ofp_packet_in.total_len = length;

        /* Fill in header. */
        /* tmp_* is replaced later. */
        ofp_header_set(&packet_in->ofp_packet_in.header, tmp_version,
                       OFPT_PACKET_IN, tmp_length, tmp_xid);

        ret = ofp_packet_in_encode(*pbuf, &packet_in->ofp_packet_in);

        if (ret == LAGOPUS_RESULT_OK) {
          ret = ofp_match_list_encode(NULL, pbuf, &packet_in->match_list,
                                      &match_total_len);

          if (ret == LAGOPUS_RESULT_OK) {
            /* Cut packet. */
            remain_length = (uint16_t) pbuf_plen_get(*pbuf);
            if (packet_in->ofp_packet_in.buffer_id == OFP_NO_BUFFER) {
              miss_send_len = remain_length;
            } else {
              miss_send_len = packet_in->miss_send_len;
            }
            if (length < miss_send_len) {
              miss_send_len = length;
            }

            /* exist data */
            if (miss_send_len != 0) {
              /* add padding */
              ret = ofp_padding_add(*pbuf, OFP_PACKET_IN_PAD);

              if (ret == LAGOPUS_RESULT_OK) {
                /* Cut packet. (remain_length) */
                if (miss_send_len + OFP_PACKET_IN_PAD > remain_length) {
                  miss_send_len = remain_length;
                }

                if (pbuf_plen_check(*pbuf, miss_send_len) !=
                    LAGOPUS_RESULT_OK) {
                  lagopus_msg_warning("FAILED : over data length.\n");
                  ret = LAGOPUS_RESULT_OUT_OF_RANGE;
                } else {
                  /* copy data. */
                  ret = pbuf_copy_with_length(*pbuf, packet_in->data,
                                              miss_send_len);
                  if (ret == LAGOPUS_RESULT_OK) {
                    pbuf_plen_reset(*pbuf);
                  } else {
                    lagopus_msg_warning("FAILED (%s).\n",
                                        lagopus_error_get_string(ret));
                  }
                }
              } else {
                lagopus_msg_warning("FAILED : over padding length.\n");
              }
            } else {
              ret = LAGOPUS_RESULT_OK;
            }
          } else {
            lagopus_msg_warning("FAILED (%s).\n",
                                lagopus_error_get_string(ret));
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("Can't allocate pbuf.\n");
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }

    /* free. */
    if (ret != LAGOPUS_RESULT_OK && *pbuf != NULL) {
      pbuf_free(*pbuf);
      *pbuf = NULL;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* Send PacketIn packet. */
lagopus_result_t
ofp_packet_in_handle(struct packet_in *packet_in,
                     uint64_t dpid) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *send_pbuf = NULL;

  if (packet_in != NULL && packet_in->data != NULL) {
    /* PacketIn. */
    ret = ofp_packet_in_create(packet_in, &send_pbuf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = ofp_role_channel_write(send_pbuf, dpid,
                                   OFPT_PACKET_IN,
                                   packet_in->ofp_packet_in.reason);
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("Socket write error (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
    }
  } else {
    lagopus_msg_warning("Arg is NULL.\n");
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  /* free. */
  if (send_pbuf != NULL) {
    pbuf_free(send_pbuf);
  }

  return ret;
}
