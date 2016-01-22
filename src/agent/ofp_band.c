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
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/ofp_pdump.h"
#include "openflow.h"
#include "openflow13packet.h"
#include "ofp_band.h"
#include "ofp_tlv.h"

void
ofp_meter_band_list_trace(uint32_t flags,
                          struct meter_band_list *band_list) {
  struct meter_band *meter_band;
  struct ofp_meter_band_drop band_drop;
  struct ofp_meter_band_dscp_remark band_remark;
  struct ofp_meter_band_experimenter band_exp;

  TAILQ_FOREACH(meter_band, band_list, entry) {
    switch (meter_band->type) {
      case OFPMBT_DROP:
        band_drop.type = meter_band->type;
        band_drop.len = meter_band->len;
        band_drop.rate = meter_band->rate;
        band_drop.burst_size = meter_band->burst_size;
        memset(band_drop.pad, 0, sizeof(band_drop.pad));

        lagopus_msg_pdump(flags, PDUMP_OFP_METER_BAND_DROP,
                          band_drop, "");
        break;
      case OFPMBT_DSCP_REMARK:
        band_remark.type = meter_band->type;
        band_remark.len = meter_band->len;
        band_remark.rate = meter_band->rate;
        band_remark.burst_size = meter_band->burst_size;
        band_remark.prec_level = meter_band->prec_level;
        memset(band_remark.pad, 0, sizeof(band_remark.pad));

        lagopus_msg_pdump(flags, PDUMP_OFP_METER_BAND_DSCP_REMARK,
                          band_remark, "");
        break;
      case OFPMBT_EXPERIMENTER:
        band_exp.type = meter_band->type;
        band_exp.len = meter_band->len;
        band_exp.rate = meter_band->rate;
        band_exp.burst_size = meter_band->burst_size;
        band_exp.experimenter = meter_band->experimenter;

        lagopus_msg_pdump(flags, PDUMP_OFP_METER_BAND_EXPERIMENTER,
                          band_exp, "");
        break;
    }
  }
}

/* DROP. */
lagopus_result_t
ofp_meter_band_drop_parse(struct pbuf *pbuf,
                          struct meter_band_list *band_list,
                          struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_meter_band_drop band;
  struct meter_band *meter_band;

  if (pbuf != NULL && band_list != NULL) {
    ret = ofp_meter_band_drop_decode(pbuf, &band);

    if (ret == LAGOPUS_RESULT_OK) {
      meter_band = meter_band_alloc((struct ofp_meter_band_header *)&band);

      if (meter_band != NULL) {
        TAILQ_INSERT_TAIL(band_list, meter_band, entry);
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* DSCP remark. */
lagopus_result_t
ofp_meter_band_dscp_remark_parse(struct pbuf *pbuf,
                                 struct meter_band_list *band_list,
                                 struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_meter_band_dscp_remark band;
  struct meter_band *meter_band;

  if (pbuf != NULL && band_list != NULL) {
    ret = ofp_meter_band_dscp_remark_decode(pbuf, &band);

    if (ret == LAGOPUS_RESULT_OK) {
      meter_band = meter_band_alloc((struct ofp_meter_band_header *)&band);

      if (meter_band != NULL) {
        TAILQ_INSERT_TAIL(band_list, meter_band, entry);

        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* Experimenter. */
lagopus_result_t
ofp_meter_band_experimenter_parse(struct pbuf *pbuf,
                                  struct meter_band_list *band_list,
                                  struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_meter_band_experimenter band;
  struct meter_band *meter_band;

  if (pbuf != NULL && band_list != NULL) {
    ret = ofp_meter_band_experimenter_decode(pbuf, &band);

    if (ret == LAGOPUS_RESULT_OK) {
      meter_band = meter_band_alloc((struct ofp_meter_band_header *) &band);

      if (meter_band != NULL) {
        TAILQ_INSERT_TAIL(band_list, meter_band, entry);

        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
ofp_drop_encode(struct pbuf_list *pbuf_list,
                struct pbuf **pbuf,
                struct meter_band *meter_band) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_meter_band_drop band;

  /* set length. */
  meter_band->len = (uint16_t) sizeof(struct ofp_meter_band_drop);

  band.type = meter_band->type;
  band.len = meter_band->len;
  band.rate = meter_band->rate;
  band.burst_size = meter_band->burst_size;
  memset(band.pad, 0, sizeof(band.pad));

  ret = ofp_meter_band_drop_encode_list(pbuf_list, pbuf, &band);

  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("FAILED (%s)\n", lagopus_error_get_string(ret));
  }

  return ret;
}

static lagopus_result_t
ofp_dscp_remark_encode(struct pbuf_list *pbuf_list,
                       struct pbuf **pbuf,
                       struct meter_band *meter_band) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_meter_band_dscp_remark band;

  /* set length. */
  meter_band->len = (uint16_t) sizeof(struct ofp_meter_band_dscp_remark);

  band.type = meter_band->type;
  band.len = meter_band->len;
  band.rate = meter_band->rate;
  band.burst_size = meter_band->burst_size;
  band.prec_level = meter_band->prec_level;
  memset(band.pad, 0, sizeof(band.pad));

  ret = ofp_meter_band_dscp_remark_encode_list(pbuf_list, pbuf, &band);

  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("FAILED (%s)\n", lagopus_error_get_string(ret));
  }

  return ret;
}

static lagopus_result_t
ofp_experimenter_encode(struct pbuf_list *pbuf_list,
                        struct pbuf **pbuf,
                        struct meter_band *meter_band) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_meter_band_experimenter band;

  /* set length. */
  meter_band->len = (uint16_t) sizeof(struct ofp_meter_band_experimenter);

  band.type = meter_band->type;
  band.len = meter_band->len;
  band.rate = meter_band->rate;
  band.burst_size = meter_band->burst_size;
  band.experimenter = meter_band->experimenter;

  ret = ofp_meter_band_experimenter_encode_list(pbuf_list, pbuf, &band);

  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("FAILED (%s)\n", lagopus_error_get_string(ret));
  }

  return ret;
}

lagopus_result_t
ofp_band_list_encode(struct pbuf_list *pbuf_list,
                     struct pbuf **pbuf,
                     struct meter_band_list *band_list,
                     uint16_t *total_length) {
  struct meter_band *meter_band;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (pbuf != NULL && band_list != NULL &&
      total_length != NULL) {
    *total_length = 0;
    if (TAILQ_EMPTY(band_list) == false) {
      TAILQ_FOREACH(meter_band, band_list, entry) {
        switch (meter_band->type) {
          case OFPMBT_DROP:
            ret = ofp_drop_encode(pbuf_list, pbuf, meter_band);
            break;
          case OFPMBT_DSCP_REMARK:
            ret = ofp_dscp_remark_encode(pbuf_list, pbuf, meter_band);
            break;
          case OFPMBT_EXPERIMENTER:
            ret = ofp_experimenter_encode(pbuf_list, pbuf, meter_band);
            break;
          default:
            /* error*/
            lagopus_msg_warning("Bad type(%d).\n",
                                meter_band->type);
            ret = LAGOPUS_RESULT_OUT_OF_RANGE;
            break;
        }

        if (ret == LAGOPUS_RESULT_OK) {
          /* Sum length. And check overflow. */
          ret = ofp_tlv_length_sum(total_length, meter_band->len);
          if (ret != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("over meter_band length (%s).\n",
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
      /* band_list is empty. */
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
