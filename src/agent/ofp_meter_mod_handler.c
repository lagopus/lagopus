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
#include "ofp_tlv.h"
#include "ofp_band.h"
#include "ofp_meter.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/meter.h"
#include "lagopus/ofp_pdump.h"

#define OFPMF_FULL_MASK                         \
  (OFPMF_KBPS | OFPMF_PKTPS |                   \
   OFPMF_BURST | OFPMF_STATS)

static void
meter_mod_trace(struct ofp_meter_mod *meter_mod,
                struct meter_band_list *band_list) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_METER_MOD)) {
    lagopus_msg_pdump(TRACE_OFPT_METER_MOD, PDUMP_OFP_HEADER,
                      meter_mod->header, "");
    lagopus_msg_pdump(TRACE_OFPT_METER_MOD, PDUMP_OFP_METER_MOD,
                      *meter_mod, "");
    ofp_meter_band_list_trace(TRACE_OFPT_METER_MOD, band_list);
  }
}

/* RECV */
/* MeterMod(Add). */
static lagopus_result_t
meter_mod_add(uint64_t dpid,
              struct ofp_meter_mod *meter_mod,
              struct meter_band_list *band_list,
              struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (meter_mod->meter_id == OFPM_CONTROLLER) {
    lagopus_msg_warning("bad meter id (OFPM_CONTROLLER).\n");
    ofp_error_set(error, OFPET_METER_MOD_FAILED, OFPMMFC_UNKNOWN_METER);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  } else {
    ret = ofp_meter_mod_add(dpid, meter_mod,
                            band_list, error);
  }
  return ret;
}

/* MeterMod(Modify). */
static lagopus_result_t
meter_mod_modify(uint64_t dpid,
                 struct ofp_meter_mod *meter_mod,
                 struct meter_band_list *band_list,
                 struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (meter_mod->meter_id == OFPM_CONTROLLER) {
    lagopus_msg_warning("bad meter id (OFPM_CONTROLLER).\n");
    ofp_error_set(error, OFPET_METER_MOD_FAILED, OFPMMFC_UNKNOWN_METER);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  } else {
    ret = ofp_meter_mod_modify(dpid, meter_mod,
                               band_list, error);
  }

  return ret;
}

/* MeterMod(Delete). */
static lagopus_result_t
meter_mod_delete(uint64_t dpid,
                 struct ofp_meter_mod *meter_mod,
                 struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (meter_mod->meter_id == OFPM_CONTROLLER) {
    lagopus_msg_warning("bad meter id (OFPM_CONTROLLER).\n");
    ofp_error_set(error, OFPET_METER_MOD_FAILED, OFPMMFC_UNKNOWN_METER);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  } else {
    ret = ofp_meter_mod_delete(dpid, meter_mod,
                               error);
  }

  return ret;
}

static lagopus_result_t
ofp_meter_flags_check(uint16_t flags, struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_OFP_ERROR;

  if ((flags & ~OFPMF_FULL_MASK) == 0) {
    ret = LAGOPUS_RESULT_OK;
  } else {
    lagopus_msg_warning("bad flags.\n");
    ofp_error_set(error, OFPET_METER_MOD_FAILED, OFPMMFC_BAD_FLAGS);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  }

  return ret;
}

static lagopus_result_t
ofp_band_parse(struct ofp_meter_mod *meter_mod,
               struct meter_band_list *band_list,
               struct pbuf *pbuf,
               struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t *band_end = NULL;
  struct ofp_tlv tlv;

  if (meter_mod->command == OFPMC_ADD ||
      meter_mod->command == OFPMC_MODIFY) {

    while (pbuf_plen_get(pbuf) > 0) {
      /* Parse meter band. */
      /* Decode TLV. */
      ret = ofp_tlv_decode_sneak(pbuf, &tlv);

      if (ret == LAGOPUS_RESULT_OK) {
        lagopus_msg_debug(1, "RECV: %s(%d)\n",
                          ofp_meter_band_type_str(tlv.type), tlv.type);
        band_end = pbuf_getp_get(pbuf) + tlv.length;

        switch (tlv.type) {
          case OFPMBT_DROP:
            ret = ofp_meter_band_drop_parse(pbuf, band_list, error);
            break;
          case OFPMBT_DSCP_REMARK:
            ret = ofp_meter_band_dscp_remark_parse(pbuf, band_list,
                                                   error);
            break;
          case OFPMBT_EXPERIMENTER:
            ret = ofp_meter_band_experimenter_parse(pbuf, band_list,
                                                    error);
            break;
          default:
            /* error*/
            lagopus_msg_warning("Bad TLV type(%d).\n",
                                tlv.type);
            ofp_error_set(error, OFPET_METER_MOD_FAILED, OFPMMFC_BAD_BAND);
            ret = LAGOPUS_RESULT_OFP_ERROR;
        }

        if (ret == LAGOPUS_RESULT_OK &&
            (band_end == NULL || band_end != pbuf_getp_get(pbuf))) {
          lagopus_msg_warning("bad band length.\n");
          ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
          ret = LAGOPUS_RESULT_OFP_ERROR;
          break;
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
        ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
        ret = LAGOPUS_RESULT_OFP_ERROR;
      }

      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("FAILED : parse meter band (%s).\n",
                            lagopus_error_get_string(ret));
        break;
      }
    }
  } else {
    /* OFPMC_DELETE */
    /* skip pbuf. */
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

  return ret;
}

/* Meter mod received. */
lagopus_result_t
ofp_meter_mod_handle(struct channel *channel, struct pbuf *pbuf,
                     struct ofp_header *xid_header,
                     struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;
  struct ofp_meter_mod meter_mod;
  struct meter_band_list band_list;

  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {

    ret = pbuf_plen_check(pbuf, (size_t) xid_header->length);

    if (ret == LAGOPUS_RESULT_OK) {
      /* Initialize band list. */
      TAILQ_INIT(&band_list);

      /* Parse meter mod header. */
      ret = ofp_meter_mod_decode(pbuf, &meter_mod);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = ofp_meter_id_check(meter_mod.meter_id, error);

        if (ret == LAGOPUS_RESULT_OK) {
          ret = ofp_meter_flags_check(meter_mod.flags, error);

          if (ret == LAGOPUS_RESULT_OK) {
            /* parse band. */
            ret = ofp_band_parse(&meter_mod, &band_list, pbuf, error);

            if (ret == LAGOPUS_RESULT_OK) {
              /* dump trace. */
              meter_mod_trace(&meter_mod, &band_list);

              /* Add meter to meter_table. */
              dpid = channel_dpid_get(channel);
              switch (meter_mod.command) {
                case OFPMC_ADD:
                  ret = meter_mod_add(dpid, &meter_mod,
                                      &band_list, error);
                  break;
                case OFPMC_MODIFY:
                  ret = meter_mod_modify(dpid, &meter_mod,
                                         &band_list, error);
                  break;
                case OFPMC_DELETE:
                  ret = meter_mod_delete(dpid, &meter_mod,
                                         error);
                  break;
                default:
                  /* error*/
                  lagopus_msg_warning("Bad command(%d).\n",
                                      meter_mod.command);
                  ofp_error_set(error, OFPET_METER_MOD_FAILED, OFPMMFC_BAD_COMMAND);
                  ret = LAGOPUS_RESULT_OFP_ERROR;
              }

              if (ret != LAGOPUS_RESULT_OK) {
                lagopus_msg_warning("FAILED (%s).\n",
                                    lagopus_error_get_string(ret));
              }
            } else {
              lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
            }
          }
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
        ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
        ret = LAGOPUS_RESULT_OFP_ERROR;
      }

      /* free. */
      if (ret != LAGOPUS_RESULT_OK) {
        ofp_meter_band_list_elem_free(&band_list);
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
