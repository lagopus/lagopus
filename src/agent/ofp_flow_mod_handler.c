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
#include "ofp_instruction.h"
#include "ofp_match.h"
#include "lagopus/flowdb.h"
#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_pdump.h"

#define OFPFF_OFPMF_FULL_MASK                   \
  (OFPFF_SEND_FLOW_REM | OFPFF_CHECK_OVERLAP |  \
   OFPFF_RESET_COUNTS | OFPFF_NO_PKT_COUNTS |   \
   OFPFF_NO_BYT_COUNTS )

static lagopus_result_t
flow_mod_flags_check(uint16_t flags, struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_OFP_ERROR;

  if ((flags & ~OFPFF_OFPMF_FULL_MASK) == 0) {
    ret = LAGOPUS_RESULT_OK;
  } else {
    lagopus_msg_warning("bad flags.\n");
    ofp_error_set(error, OFPET_FLOW_MOD_FAILED, OFPFMFC_BAD_FLAGS);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  }

  return ret;
}

static void
flow_mod_trace(struct ofp_flow_mod *flow_mod,
               struct match_list *match_list,
               struct instruction_list *instruction_list) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_FLOW_MOD)) {
    lagopus_msg_pdump(TRACE_OFPT_FLOW_MOD, PDUMP_OFP_HEADER,
                      flow_mod->header, "");
    lagopus_msg_pdump(TRACE_OFPT_FLOW_MOD, PDUMP_OFP_FLOW_MOD,
                      *flow_mod, "");
    ofp_match_list_trace(TRACE_OFPT_FLOW_MOD, match_list);
    ofp_instruction_list_trace(TRACE_OFPT_FLOW_MOD, instruction_list);
  }
}

/* RECV */
/* FlowMod packet receive. */
lagopus_result_t
ofp_flow_mod_handle(struct channel *channel, struct pbuf *pbuf,
                    struct ofp_header *xid_header,
                    struct ofp_error *error) {
  lagopus_result_t ret;
  uint64_t dpid;
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;

  if (channel != NULL && pbuf != NULL && xid_header != NULL) {
    /* Init lists. */
    TAILQ_INIT(&match_list);
    TAILQ_INIT(&instruction_list);

    /* Parse flow mod header. */
    ret = ofp_flow_mod_decode(pbuf, &flow_mod);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = flow_mod_flags_check(flow_mod.flags, error);

      if (ret == LAGOPUS_RESULT_OK) {
        /* Parse matches. */
        ret = ofp_match_parse(channel, pbuf, &match_list, error);

        if (ret == LAGOPUS_RESULT_OK) {
          /* Parse instructions. */
          if (flow_mod.command == OFPFC_DELETE ||
              flow_mod.command == OFPFC_DELETE_STRICT) {
            /* skip pbuf. */
            ret = pbuf_forward(pbuf, pbuf_plen_get(pbuf));
            if (ret != LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(ret));
            }
          } else {
            while (pbuf_plen_get(pbuf) > 0) {
              ret = ofp_instruction_parse(pbuf, &instruction_list, error);
              if (ret != LAGOPUS_RESULT_OK) {
                lagopus_msg_warning("FAILED (%s).\n",
                                    lagopus_error_get_string(ret));
                break;
              }
            }
          }

          if (ret == LAGOPUS_RESULT_OK) {
            /* trace. */
            flow_mod_trace(&flow_mod, &match_list, &instruction_list);

            /* Flow add, modify, delete. */
            dpid = channel_dpid_get(channel);
            switch (flow_mod.command) {
              case OFPFC_ADD:
                ret = ofp_flow_mod_check_add(dpid, &flow_mod,
                                             &match_list, &instruction_list,
                                             error);
                break;
              case OFPFC_MODIFY:
              case OFPFC_MODIFY_STRICT:
                ret = ofp_flow_mod_modify(dpid, &flow_mod,
                                          &match_list, &instruction_list,
                                          error);
                break;
              case OFPFC_DELETE:
              case OFPFC_DELETE_STRICT:
                ret = ofp_flow_mod_delete(dpid,
                                          &flow_mod, &match_list,
                                          error);
                break;
              default:
                ofp_error_set(error, OFPET_FLOW_MOD_FAILED, OFPFMFC_BAD_COMMAND);
                ret = LAGOPUS_RESULT_OFP_ERROR;
                break;
            }

            if (ret == LAGOPUS_RESULT_OFP_ERROR) {
              lagopus_msg_warning("OFP ERROR (%s).\n",
                                  lagopus_error_get_string(ret));
            }
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
      ret = LAGOPUS_RESULT_OFP_ERROR;
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
    }

    /* free. */
    if (ret != LAGOPUS_RESULT_OK) {
      ofp_instruction_list_elem_free(&instruction_list);
      ofp_match_list_elem_free(&match_list);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
