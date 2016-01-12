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
#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/ofp_pdump.h"
#include "ofp_apis.h"
#include "ofp_bucket.h"

#define OFPPC_FULL_MASK                          \
  (OFPPC_PORT_DOWN | OFPPC_NO_RECV |             \
   OFPPC_NO_FWD | OFPPC_NO_PACKET_IN)

#define OFPPF_FULL_MASK                         \
  (OFPPF_10MB_HD | OFPPF_10MB_FD |              \
   OFPPF_100MB_HD | OFPPF_100MB_FD |            \
   OFPPF_1GB_HD | OFPPF_1GB_FD |                \
   OFPPF_10GB_FD | OFPPF_40GB_FD |              \
   OFPPF_100GB_FD | OFPPF_1TB_FD |              \
   OFPPF_OTHER | OFPPF_COPPER |                 \
   OFPPF_FIBER | OFPPF_AUTONEG |                \
   OFPPF_PAUSE | OFPPF_PAUSE_ASYM)

static void
port_mod_trace(struct ofp_port_mod *port_mod) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_PORT_MOD)) {
    lagopus_msg_pdump(TRACE_OFPT_PORT_MOD, PDUMP_OFP_HEADER,
                      port_mod->header, "");
    lagopus_msg_pdump(TRACE_OFPT_PORT_MOD, PDUMP_OFP_PORT_MOD,
                      *port_mod, "");
  }
}

static lagopus_result_t
ofp_port_config_check(uint32_t config,
                      struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_OFP_ERROR;

  if ((config & (uint32_t) ~OFPPC_FULL_MASK) == 0) {
    res = LAGOPUS_RESULT_OK;
  } else {
    lagopus_msg_warning("unknown port_mod config.\n");
    ofp_error_set(error, OFPET_PORT_MOD_FAILED, OFPPMFC_BAD_CONFIG);
    res = LAGOPUS_RESULT_OFP_ERROR;
  }

  return res;
}

static lagopus_result_t
ofp_port_features_check(uint32_t advertise,
                        struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_OFP_ERROR;

  if ((advertise & (uint32_t) ~OFPPF_FULL_MASK) == 0) {
    res = LAGOPUS_RESULT_OK;
  } else {
    lagopus_msg_warning("unknown port_mod advertise.\n");
    ofp_error_set(error, OFPET_PORT_MOD_FAILED, OFPPMFC_BAD_ADVERTISE);
    res = LAGOPUS_RESULT_OFP_ERROR;
  }

  return res;
}

/* Port mod received. */
lagopus_result_t
ofp_port_mod_handle(struct channel *channel, struct pbuf *pbuf,
                    struct ofp_header *xid_header,
                    struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;
  struct ofp_port_mod port_mod;

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    /* Parse port_mod header. */
    res = ofp_port_mod_decode(pbuf, &(port_mod));
    if (res != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("port_mod decode error.\n");
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      res = LAGOPUS_RESULT_OFP_ERROR;
    } else if (pbuf_plen_get(pbuf) > 0) {
      lagopus_msg_warning("packet decode failed. (size over).\n");
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      res = LAGOPUS_RESULT_OFP_ERROR;
    } else {                    /* decode success */
      /* dump trace. */
      port_mod_trace(&port_mod);

      /* check config. */
      res = ofp_port_config_check(port_mod.config, error);
      if (res == LAGOPUS_RESULT_OK) {
        /* check mask. */
        res = ofp_port_config_check(port_mod.mask, error);
        if (res == LAGOPUS_RESULT_OK) {
          /* check advertise. */
          res = ofp_port_features_check(port_mod.advertise, error);
          if (res == LAGOPUS_RESULT_OK) {
            dpid = channel_dpid_get(channel);
            /* call API */
            res = ofp_port_mod_modify(dpid, &port_mod, error);
            if (res != LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(res));
            }
          }
        }
      }
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return res;
}
