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
#include "ofp_bucket.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/ofp_pdump.h"

static void
table_mod_trace(struct ofp_table_mod *table_mod) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_TABLE_MOD)) {
    lagopus_msg_pdump(TRACE_OFPT_TABLE_MOD, PDUMP_OFP_HEADER,
                      table_mod->header, "");
    lagopus_msg_pdump(TRACE_OFPT_TABLE_MOD, PDUMP_OFP_TABLE_MOD,
                      *table_mod, "");
  }
}

static lagopus_result_t
ofp_table_mod_config_check(uint32_t config, struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_OFP_ERROR;

  switch (config) {
    case OFPTC_DEPRECATED_MASK:
      res = LAGOPUS_RESULT_OK;
      break;
    default:
      lagopus_msg_warning("bad config.\n");
      ofp_error_set(error, OFPET_TABLE_MOD_FAILED, OFPTMFC_BAD_CONFIG);
      res = LAGOPUS_RESULT_OFP_ERROR;
      break;
  }

  return res;
}

/* Table mod received. */
lagopus_result_t
ofp_table_mod_handle(struct channel *channel, struct pbuf *pbuf,
                     struct ofp_header *xid_header,
                     struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;
  struct ofp_table_mod table_mod;

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    /* Parse table_mod header. */
    res = ofp_table_mod_decode(pbuf, &(table_mod));
    if (res != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("table_mod decode error.\n");
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      res = LAGOPUS_RESULT_OFP_ERROR;
    } else if (pbuf_plen_get(pbuf) > 0) {
      lagopus_msg_warning("packet decode failed. (size over).\n");
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      res = LAGOPUS_RESULT_OFP_ERROR;
    } else {                    /* decode success */
      /* dump trace. */
      table_mod_trace(&table_mod);

      /* check flag. */
      res = ofp_table_mod_config_check(table_mod.config, error);

      if (res == LAGOPUS_RESULT_OK) {
        dpid = channel_dpid_get(channel);
        /* set table_mod */
        res = ofp_table_mod_set(dpid, &table_mod, error);
        if (res != LAGOPUS_RESULT_OK) {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(res));
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(res));
      }
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return res;
}
