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
#include "ofp_apis.h"
#include "lagopus/ofp_handler.h"

/* Header only packet receive. */
lagopus_result_t
ofp_header_handle(struct channel *channel, struct pbuf *pbuf,
                  struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_header msg;

  if (channel != NULL && pbuf != NULL &&
      error != NULL) {
    /* Parse ofp_header. */
    ret = ofp_header_decode(pbuf, &msg);
    if (ret == LAGOPUS_RESULT_OK) {
      /* Skip payload if it exists. */
      if (pbuf_plen_get(pbuf) > 0) {
        ret = pbuf_forward(pbuf, pbuf_plen_get(pbuf));
        if (ret != LAGOPUS_RESULT_OK) {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
          if (ret == LAGOPUS_RESULT_OUT_OF_RANGE) {
            ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
            ret = LAGOPUS_RESULT_OFP_ERROR;
          }
        }
      }

      if (ret == LAGOPUS_RESULT_OK &&
          channel_version_get(channel) != msg.version) {
        lagopus_msg_warning("Unsupported vetsion.\n");
        ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_VERSION);
        ret = LAGOPUS_RESULT_OFP_ERROR;
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      return LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* Utility function to set ofp_header. */
void
ofp_header_set(struct ofp_header *header, uint8_t version,
               uint8_t type, uint16_t length, uint32_t xid) {
  header->version = version;
  header->type = type;
  header->length = length;
  header->xid = xid;
}

/* header only packet. */
lagopus_result_t
ofp_header_create(struct channel *channel, uint8_t type,
                  struct ofp_header *xid_header,
                  struct ofp_header *header,
                  struct pbuf *pbuf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint32_t xid;

  if (channel != NULL && header != NULL &&
      pbuf != NULL) {
    if (xid_header != NULL) {
      xid = xid_header->xid;
    } else {
      xid = channel_xid_get(channel);
    }

    ofp_header_set(header, channel_version_get(channel), type,
                   (uint16_t) pbuf_plen_get(pbuf), xid);

    ret = ofp_header_encode(pbuf, header);
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_header_packet_set(struct channel *channel,
                      struct pbuf *pbuf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t cur_length;
  pbuf_info_t cur_pbuf_info;
  pbuf_info_t update_pbuf_info;
  struct ofp_header header;

  if (channel != NULL && pbuf != NULL) {
    /* Store current pbuf info. */
    pbuf_info_store(pbuf, &cur_pbuf_info);
    ret = pbuf_length_get(pbuf, &cur_length);

    if (ret == LAGOPUS_RESULT_OK) {
      /* Update pbuf info for ofp_header_decode_sneak. */
      pbuf_getp_set(&update_pbuf_info, pbuf_data_get(pbuf));
      pbuf_putp_set(&update_pbuf_info,
                    pbuf_data_get(pbuf) + sizeof(struct ofp_header));
      pbuf_plen_set(&update_pbuf_info, sizeof(struct ofp_header));
      pbuf_info_load(pbuf, &update_pbuf_info);

      ret = ofp_header_decode_sneak(pbuf, &header);
      if (ret == LAGOPUS_RESULT_OK) {
        /* Update pbuf info for ofp_header_create. */
        pbuf_reset(pbuf);
        pbuf_plen_set(pbuf, (size_t) cur_length);

        ret = ofp_header_create(channel, header.type, NULL,
                                &header, pbuf);
        if (ret == LAGOPUS_RESULT_OK) {
          /* Load pbuf info. */
          pbuf_info_load(pbuf, &cur_pbuf_info);
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
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_header_mp_copy(struct pbuf *dst_pbuf,
                   struct pbuf *src_pbuf) {
  struct ofp_multipart_reply mp_reply;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  pbuf_info_t cur_src_pbuf_info;
  pbuf_info_t cur_dst_pbuf_info;
  pbuf_info_t update_pbuf_info;
  uint16_t cur_src_length = 0;

  if (dst_pbuf != NULL && src_pbuf != NULL) {
    /* Store current dst/src_pbuf info. */
    pbuf_info_store(dst_pbuf, &cur_dst_pbuf_info);
    pbuf_info_store(src_pbuf, &cur_src_pbuf_info);
    ret = pbuf_length_get(src_pbuf, &cur_src_length);

    if (ret == LAGOPUS_RESULT_OK) {
      /* Update src_pbuf info for ofp_multipart_reply_decode. */
      pbuf_getp_set(&update_pbuf_info, pbuf_data_get(src_pbuf));
      pbuf_putp_set(&update_pbuf_info,
                    pbuf_data_get(src_pbuf) + sizeof(struct ofp_multipart_reply));
      pbuf_plen_set(&update_pbuf_info, sizeof(struct ofp_multipart_reply));
      pbuf_info_load(src_pbuf, &update_pbuf_info);

      ret = ofp_multipart_reply_decode(src_pbuf, &mp_reply);

      if (ret == LAGOPUS_RESULT_OK) {
        /* Set length/flag in src_pbuf. */
        mp_reply.header.length = cur_src_length;
        mp_reply.flags = OFPMPF_REPLY_MORE;
        /* Update src_pbuf info for ofp_multipart_reply_encode. */
        pbuf_reset(src_pbuf);
        pbuf_plen_set(src_pbuf, sizeof(struct ofp_multipart_reply));

        ret = ofp_multipart_reply_encode(src_pbuf, &mp_reply);

        if (ret == LAGOPUS_RESULT_OK) {
          /* Set length/flag in dst_pbuf. */
          mp_reply.header.length = sizeof(struct ofp_multipart_reply);
          mp_reply.flags = 0;
          /* Update dst_pbuf info for ofp_multipart_reply_encode. */
          pbuf_reset(dst_pbuf);
          pbuf_plen_set(dst_pbuf, pbuf_plen_get(&cur_dst_pbuf_info));

          ret = ofp_multipart_reply_encode(dst_pbuf, &mp_reply);
          if (ret == LAGOPUS_RESULT_OK) {
            /* Load pbuf info. */
            pbuf_info_load(src_pbuf, &cur_src_pbuf_info);
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
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_header_length_set(struct pbuf *pbuf,
                      uint16_t length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  pbuf_info_t cur_pbuf_info;
  pbuf_info_t update_pbuf_info;
  struct ofp_header header;

  if (pbuf != NULL) {
    /* Store current pbuf info. */
    pbuf_info_store(pbuf, &cur_pbuf_info);

    /* Update pbuf info for ofp_header_decode_sneak. */
    pbuf_getp_set(&update_pbuf_info, pbuf_data_get(pbuf));
    pbuf_putp_set(&update_pbuf_info,
                  pbuf_data_get(pbuf) + sizeof(struct ofp_header));
    pbuf_plen_set(&update_pbuf_info, sizeof(struct ofp_header));
    pbuf_info_load(pbuf, &update_pbuf_info);

    ret = ofp_header_decode_sneak(pbuf, &header);
    if (ret == LAGOPUS_RESULT_OK) {
      /* Set length.*/
      header.length = length;
      /* Update pbuf info for ofp_header_encode. */
      pbuf_reset(pbuf);
      pbuf_plen_set(pbuf, sizeof(struct ofp_header));

      ret = ofp_header_encode(pbuf, &header);
      if (ret == LAGOPUS_RESULT_OK) {
        /* Load pbuf info. */
        pbuf_info_load(pbuf, &cur_pbuf_info);
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

bool
ofp_header_version_check(struct channel *channel,
                         struct ofp_header *header) {
  bool ret = false;
  uint8_t bridge_ofp_version;
  uint64_t dpid;

  if (channel != NULL && header != NULL) {
    dpid = channel_dpid_get(channel);
    ret = ofp_switch_version_get(dpid, &bridge_ofp_version);

    if (ret == LAGOPUS_RESULT_OK) {
      if (bridge_ofp_version == header->version) {
        ret = true;
      } else {
        lagopus_msg_warning("Unsupported ofp version : %"PRIu8".\n",
                            header->version);
        ret = false;
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
      ret = false;
    }
  } else {
    lagopus_msg_warning("Arg is NULL.\n");
    ret = false;
  }

  return ret;
}
