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
#include "ofp_padding.h"
#include "ofp_apis.h"
#include "openflow.h"
#include "openflow13packet.h"
#include "ofp_tlv.h"

static lagopus_result_t
padding_encode(struct pbuf *pbuf, uint16_t length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t pad = 0;

  /* Size check. */
  ret = pbuf_plen_check(pbuf, length);
  if (ret == LAGOPUS_RESULT_OK) {
    /* Encode packet. */
    ENCODE_PUT(&pad, length);
  }

  return ret;
}

static lagopus_result_t
padding_encode_list(struct pbuf_list *pbuf_list, struct pbuf **pbuf,
                    uint16_t length) {
  struct pbuf *before_pbuf;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (pbuf_list == NULL) {
    return padding_encode(*pbuf, length);
  }

  *pbuf = pbuf_list_last_get(pbuf_list);
  if (*pbuf == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  ret = padding_encode(*pbuf, length);
  if (ret == LAGOPUS_RESULT_OUT_OF_RANGE) {
    before_pbuf = *pbuf;
    *pbuf = pbuf_alloc(OFP_PACKET_MAX_SIZE);
    if (*pbuf == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    pbuf_plen_set(*pbuf, OFP_PACKET_MAX_SIZE);
    ret = ofp_header_mp_copy(*pbuf, before_pbuf);
    if (ret != LAGOPUS_RESULT_OK) {
      if (*pbuf != NULL) {
        pbuf_free(*pbuf);
      }
      return ret;
    }
    pbuf_list_add(pbuf_list, *pbuf);
    ret = padding_encode(*pbuf, length);
  }

  return ret;
}

static lagopus_result_t
ofp_padding_encode_internal(struct pbuf_list *pbuf_list,
                            struct pbuf **pbuf, uint16_t length,
                            uint16_t *pad_length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (pbuf != NULL && pad_length != NULL) {
    *pad_length = GET_64BIT_PADDING_LENGTH(length);
    ret = padding_encode_list(pbuf_list, pbuf, *pad_length);

    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("FAILED (%s)\n",
                          lagopus_error_get_string(ret));
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;

}

lagopus_result_t
ofp_padding_encode(struct pbuf_list *pbuf_list,
                   struct pbuf **pbuf, uint16_t *length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t pad_length;

  if (pbuf != NULL && length != NULL) {
    ret = ofp_padding_encode_internal(pbuf_list, pbuf, *length,
                                      &pad_length);
    if (ret == LAGOPUS_RESULT_OK) {
      /* Sum length. And check overflow. */
      ret = ofp_tlv_length_sum(length, pad_length);
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("over padding length (%s).\n",
                            lagopus_error_get_string(ret));

      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_padding_add(struct pbuf *pbuf,
                uint16_t pad_length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (pbuf != NULL) {
    ret = padding_encode(pbuf, pad_length);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
