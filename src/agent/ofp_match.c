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
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/ofp_pdump.h"
#include "lagopus_apis.h"
#include "openflow.h"
#include "openflow13packet.h"
#include "ofp_match.h"
#include "ofp_tlv.h"
#include "ofp_header_handler.h"
#include "ofp_padding.h"
#include "ofp_oxm.h"

struct match *
match_alloc(uint8_t size) {
  struct match *match;
  match = (struct match *)calloc(1, sizeof(struct match) + size);
  return match;
}

lagopus_result_t
match_trace(const void *pac, size_t pac_size,
            char *str, size_t max_len) {
  int n, i;
  struct match *packet = (struct match *) pac;

  if (pac_size == sizeof(struct match)) {
    /* Trace packet. */
    TRACE_STR_ADD(n, str, max_len, "match [");
    if (packet != NULL) {
      TRACE_STR_ADD(n, str, max_len, "MATCH: %s(%d) mask:%d len:%d val:",
                    oxm_ofb_match_fields_str(OXM_FIELD_TYPE(packet->oxm_field)),
                    OXM_FIELD_TYPE(packet->oxm_field),
                    packet->oxm_field & 1,
                    packet->oxm_length);

      for (i = 0; i < packet->oxm_length; i++) {
        TRACE_STR_ADD(n, str, max_len,
                      "%02x ", *(packet->oxm_value + i));
      }
    } else {
      TRACE_STR_ADD(n, str, max_len, "NULL");
    }
    TRACE_STR_ADD(n, str, max_len, "]");
  } else {
    return LAGOPUS_RESULT_OUT_OF_RANGE;
  }

  return LAGOPUS_RESULT_OK;
}

void
ofp_match_list_trace(uint32_t flags,
                     struct match_list *match_list) {
  struct match *match;

  TAILQ_FOREACH(match, match_list, entry) {
    lagopus_msg_pdump(flags, PDUMP_OFP_MATCH,
                      *match, "");
  }
}

static lagopus_result_t
ofp_oxm_check(uint16_t oxm_class,
              uint8_t oxm_field,
              struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* check class. */
  if (oxm_class == OFPXMC_OPENFLOW_BASIC) {
    /* check field */
    ret = ofp_oxm_field_check(oxm_field);
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
      ofp_error_set(error, OFPET_BAD_MATCH, OFPBMC_BAD_FIELD);
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    lagopus_msg_warning("OFPXMC_OPENFLOW_BASIC is not supported.\n");
    ofp_error_set(error, OFPET_BAD_MATCH, OFPBMC_BAD_TYPE);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  }

  return ret;
}

/* ofp_match parser.
   It is variable size depends on TLV length with 8
   octet alignment. */
lagopus_result_t
ofp_match_parse(struct channel *channel, struct pbuf *pbuf,
                struct match_list *match_list,
                struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_tlv tlv;
  size_t total_length;
  uint8_t *oxm = NULL;
  uint8_t *match_end = NULL;
  uint16_t oxm_class;
  uint8_t oxm_field;
  uint8_t oxm_length;
  struct match *match = NULL;
  struct match *mp =NULL;
  struct ofp_match *ofp_match = NULL;

  if (channel != NULL && pbuf != NULL &&
      match_list != NULL && error != NULL) {

    /* Sneak preview of TLV. */
    ret = ofp_tlv_decode_sneak(pbuf, &tlv);

    if (ret == LAGOPUS_RESULT_OK) {
      /* When version is 0x04, type must be OFPMT_OXM. */
      if (channel_version_get(channel) >= OPENFLOW_VERSION_1_3 &&
          tlv.type != OFPMT_OXM) {
        lagopus_msg_warning("Match type is not OFPMT_OXM.\n");
        ret = LAGOPUS_RESULT_OFP_ERROR;
        ofp_error_set(error, OFPET_BAD_MATCH, OFPBMC_BAD_TYPE);
      } else {
        /* otal length including padding. */
        total_length = (size_t)(((tlv.length + 7) / 8) * 8);

        /* Size must has enough for total_length. */
        ret = pbuf_plen_check(pbuf, total_length);
        if (ret != LAGOPUS_RESULT_OK) {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
          if (ret == LAGOPUS_RESULT_OUT_OF_RANGE) {
            ret = LAGOPUS_RESULT_OFP_ERROR;
            ofp_error_set(error, OFPET_BAD_MATCH, OFPBMC_BAD_LEN);
          }
        } else {
          /* Any match. */
          if (tlv.length == 4) {
            ret = pbuf_forward(pbuf, total_length);
            if (ret != LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(ret));
              if (ret == LAGOPUS_RESULT_OUT_OF_RANGE) {
                ret = LAGOPUS_RESULT_OFP_ERROR;
                ofp_error_set(error, OFPET_BAD_MATCH, OFPBMC_BAD_LEN);
              }
            }
          } else {
            /* ofp_match pointer. */
            ofp_match = (struct ofp_match *) pbuf_getp_get(pbuf);
            match_end = ((uint8_t *)ofp_match) + tlv.length;

            /* OXM parse. */
            oxm = ofp_match->oxm_fields;

            while (oxm < match_end) {
              oxm_class = (uint16_t)(((*oxm) << 8) + *(oxm + 1));
              oxm += 2;
              oxm_field = (*oxm);
              oxm++;
              oxm_length = (*oxm);
              oxm++;

              /* check oxm. */
              ret = ofp_oxm_check(oxm_class, oxm_field,
                                  error);
              if (ret == LAGOPUS_RESULT_OK) {
                /* Alloc match. */
                match = match_alloc(oxm_length);

                if (match != NULL) {
                  /* Copy field. */
                  match->oxm_class = oxm_class;
                  match->oxm_field = oxm_field;
                  match->oxm_length = oxm_length;
                  memcpy(match->oxm_value, oxm, oxm_length);

                  /* Add to match list.  We sort match entry from the smallest
                   * to the largest. */
                  for (mp = TAILQ_FIRST(match_list); mp != NULL;
                       mp = TAILQ_NEXT(mp, entry)) {
                    if (OXM_FIELD_TYPE(match->oxm_field) <
                        OXM_FIELD_TYPE(mp->oxm_field)) {
                      break;
                    }
                  }
                  if (mp != NULL) {
                    TAILQ_INSERT_BEFORE(mp, match, entry);
                  } else {
                    TAILQ_INSERT_TAIL(match_list, match, entry);
                  }

                  /* Foward pointer. */
                  oxm += oxm_length;
                  ret = LAGOPUS_RESULT_OK;
                } else {
                  lagopus_msg_warning("Can't allocate match.\n");
                  ret = LAGOPUS_RESULT_NO_MEMORY;
                  break;
                }
              } else {
                lagopus_msg_warning("FAILED (%s).\n",
                                    lagopus_error_get_string(ret));
                break;
              }
            }

            if (ret == LAGOPUS_RESULT_OK) {
              /* check length. */
              if (match_end == oxm) {
                /* Forward the packet. */
                ret = pbuf_forward(pbuf, total_length);
                if (ret != LAGOPUS_RESULT_OK) {
                  lagopus_msg_warning("FAILED (%s).\n",
                                      lagopus_error_get_string(ret));
                  if (ret == LAGOPUS_RESULT_OUT_OF_RANGE) {
                    ofp_error_set(error, OFPET_BAD_MATCH, OFPBMC_BAD_LEN);
                    ret = LAGOPUS_RESULT_OFP_ERROR;
                  }
                }
              } else {
                lagopus_msg_warning("Match length is invalid value.\n");
                ofp_error_set(error, OFPET_BAD_MATCH, OFPBMC_BAD_LEN);
                ret = LAGOPUS_RESULT_OFP_ERROR;
              }
            }
          }
        }
      }
    } else {
      lagopus_msg_warning("Match length is shorter than TLV's one.\n");
      ret = LAGOPUS_RESULT_OFP_ERROR;
      ofp_error_set(error, OFPET_BAD_MATCH, OFPBMC_BAD_LEN);
    }

    /* free. */
    if (ret != LAGOPUS_RESULT_OK) {
      ofp_match_list_elem_free(match_list);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_match_list_encode(struct pbuf_list *pbuf_list,
                      struct pbuf **pbuf,
                      struct match_list *match_list,
                      uint16_t *total_length) {
  struct pbuf *before_pbuf = NULL;
  struct match *match = NULL;
  struct ofp_oxm ofp_oxm;
  struct ofp_tlv ofp_tlv;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t *match_head = NULL;

  if (pbuf != NULL && match_list != NULL &&
      total_length != NULL) {
    /* OMX only. */
    ofp_tlv.type = OFPMT_OXM;
    /* length is replaced later. */
    ofp_tlv.length = 0;

    ret = ofp_tlv_encode_list(pbuf_list, pbuf, &ofp_tlv);
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("FAILED : ofp_tlv_encode_list.\n");
      goto done;
    }

    match_head = pbuf_putp_get(*pbuf) - sizeof(struct ofp_tlv);
    *total_length = (uint16_t) sizeof(struct ofp_tlv);

    if (TAILQ_EMPTY(match_list) == false) {
      TAILQ_FOREACH(match, match_list, entry) {
        ofp_oxm.oxm_class = match->oxm_class;
        ofp_oxm.oxm_field = match->oxm_field;
        ofp_oxm.oxm_length = match->oxm_length;

        ret = ofp_oxm_encode_list(pbuf_list, pbuf, &ofp_oxm);

        if (ret == LAGOPUS_RESULT_OK) {
          /* check oxm_length. */
          ret = pbuf_plen_check(*pbuf, match->oxm_length);
          if (ret == LAGOPUS_RESULT_OUT_OF_RANGE) {
            if (pbuf_list != NULL) {
              before_pbuf = *pbuf;
              *pbuf = pbuf_alloc(OFP_PACKET_MAX_SIZE);
              if (*pbuf == NULL) {
                ret = LAGOPUS_RESULT_NO_MEMORY;
                break;
              }
              pbuf_list_add(pbuf_list, *pbuf);
              pbuf_plen_set(*pbuf, OFP_PACKET_MAX_SIZE);
              ret = ofp_header_mp_copy(*pbuf, before_pbuf);
              if (ret != LAGOPUS_RESULT_OK) {
                lagopus_msg_warning("FAILED : ofp_header_mp_copy.\n");
                break;
              }
            } else {
              lagopus_msg_warning("FAILED : over packet length.\n");
              ret = LAGOPUS_RESULT_OUT_OF_RANGE;
              break;
            }
          } else if (ret != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("FAILED (%s).\n",
                                lagopus_error_get_string(ret));
            break;
          }

          /* copy oxm_value. */
          ret = pbuf_encode(*pbuf, match->oxm_value,
                            match->oxm_length);
          if (ret == LAGOPUS_RESULT_OK) {
            /* Sum length (size of ofp_oxm[header] + ofp_oxm.oxm_length). */
            /* And check overflow.                                        */
            ret = ofp_tlv_length_sum(total_length, sizeof(struct ofp_oxm));
            if (ret == LAGOPUS_RESULT_OK) {
              ret = ofp_tlv_length_sum(total_length, ofp_oxm.oxm_length);
              if (ret != LAGOPUS_RESULT_OK) {
                lagopus_msg_warning("over match length (%s).\n",
                                    lagopus_error_get_string(ret));
                break;
              }
            } else {
              lagopus_msg_warning("over match length (%s).\n",
                                  lagopus_error_get_string(ret));
              break;
            }
          } else {
            lagopus_msg_warning("FAILED (%s).\n",
                                lagopus_error_get_string(ret));
            break;
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
          break;
        }
      }
    } else {
      /* match_list is empty. */
      ret = LAGOPUS_RESULT_OK;
    }

    if (ret == LAGOPUS_RESULT_OK) {
      /* replace length (excluding padding).*/
      ret = ofp_tlv_length_set(match_head, *total_length);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = ofp_padding_encode(pbuf_list, pbuf,
                                 total_length);

        if (ret != LAGOPUS_RESULT_OK) {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}
