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
#include "ofp_apis.h"
#include "ofp_bucket.h"
#include "lagopus/group.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/ofp_pdump.h"

static void
group_mod_trace(struct ofp_group_mod *group_mod,
                struct bucket_list *bucket_list) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_GROUP_MOD)) {
    lagopus_msg_pdump(TRACE_OFPT_GROUP_MOD, PDUMP_OFP_HEADER,
                      group_mod->header, "");
    lagopus_msg_pdump(TRACE_OFPT_GROUP_MOD, PDUMP_OFP_GROUP_MOD,
                      *group_mod, "");
    if (bucket_list != NULL) {
      ofp_bucket_list_trace(TRACE_OFPT_GROUP_MOD, bucket_list);
    }
  }
}

static lagopus_result_t
s_parse_bucket_list(struct pbuf *pbuf,
                    struct bucket_list *bucket_list,
                    struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_OK;
  if (TAILQ_EMPTY(bucket_list) == true && pbuf_plen_get(pbuf) == 0) {
    res = LAGOPUS_RESULT_OK;    /* bucket_list is empty */
  } else {
    /* decode buckets. */
    while (pbuf_plen_get(pbuf) > 0) {
      res = ofp_bucket_parse(pbuf, bucket_list, error);
      if (res != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("FAILED : ofp_bucket_parse (%s).\n",
                            lagopus_error_get_string(res));
        break;
      }
    }
    /* check plen. */
    if (res == LAGOPUS_RESULT_OK && pbuf_plen_get(pbuf) > 0) {
      lagopus_msg_warning("packet decode failed. (size over).\n");
      ofp_error_set(error, OFPET_GROUP_MOD_FAILED,
                    OFPGMFC_BAD_BUCKET);
      res = LAGOPUS_RESULT_OFP_ERROR;
    }
  }
  return res;
}

static lagopus_result_t
s_ofpgc_add(struct pbuf *pbuf,
            uint64_t dpid,
            struct ofp_group_mod *group_mod,
            struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct bucket_list bucket_list;
  TAILQ_INIT(&bucket_list);

  /* decode buckets. */
  res = s_parse_bucket_list(pbuf, &bucket_list, error);
  if (res == LAGOPUS_RESULT_OK) {
    res = ofp_group_mod_add(dpid, group_mod, &bucket_list, error);
  }

  /* dump trace. */
  group_mod_trace(group_mod, &bucket_list);

  ofp_bucket_list_free(&bucket_list);
  return res;
}

static lagopus_result_t
s_ofpgc_modify(struct pbuf *pbuf,
               uint64_t dpid,
               struct ofp_group_mod *group_mod,
               struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct bucket_list bucket_list;
  TAILQ_INIT(&bucket_list);

  /* decode buckets. */
  res = s_parse_bucket_list(pbuf, &bucket_list, error);
  if (res == LAGOPUS_RESULT_OK) {
    res = ofp_group_mod_modify(dpid, group_mod, &bucket_list, error);
  }

  /* dump trace. */
  group_mod_trace(group_mod, &bucket_list);

  ofp_bucket_list_free(&bucket_list);
  return res;
}

static lagopus_result_t
s_ofpgc_delete(struct pbuf *pbuf,
               uint64_t dpid,
               struct ofp_group_mod *group_mod,
               struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;

  /* dump trace. */
  group_mod_trace(group_mod, NULL);

  /* check plen. */
  if (pbuf_plen_get(pbuf) == 0) {
    res = ofp_group_mod_delete(dpid, group_mod, error);
  } else {
    lagopus_msg_warning("packet decode failed. (size over).\n");
    ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
    res = LAGOPUS_RESULT_OFP_ERROR;
  }
  return res;
}

static lagopus_result_t
s_group_type_check(uint8_t type, struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_OFP_ERROR;

  switch (type) {
    case OFPGT_ALL:
    case OFPGT_SELECT:
    case OFPGT_INDIRECT:
    case OFPGT_FF:
      res = LAGOPUS_RESULT_OK;
      break;
    default:
      lagopus_msg_warning("unknown group_mod type.\n");
      ofp_error_set(error, OFPET_GROUP_MOD_FAILED, OFPGMFC_BAD_TYPE);
      res = LAGOPUS_RESULT_OFP_ERROR;
      break;
  }

  return res;
}

/* Group mod received. */
lagopus_result_t
ofp_group_mod_handle(struct channel *channel, struct pbuf *pbuf,
                     struct ofp_header *xid_header,
                     struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;
  struct ofp_group_mod group_mod;

  /* check params */
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    /* Parse group_mod header. */
    res = ofp_group_mod_decode(pbuf, &(group_mod));
    if (res == LAGOPUS_RESULT_OK) {
      /* check type */
      res = s_group_type_check(group_mod.type, error);
      if (res == LAGOPUS_RESULT_OK) {
        /* exec group_mod command. */
        dpid = channel_dpid_get(channel);
        switch (group_mod.command) {
          case OFPGC_ADD:
            res = s_ofpgc_add(pbuf, dpid,
                              &group_mod, error);
            break;
          case OFPGC_MODIFY:
            res = s_ofpgc_modify(pbuf, dpid,
                                 &group_mod, error);
            break;
          case OFPGC_DELETE:
            res = s_ofpgc_delete(pbuf, dpid,
                                 &group_mod, error);
            break;
          default:
            lagopus_msg_warning("unknown group_mod command.\n");
            ofp_error_set(error, OFPET_GROUP_MOD_FAILED, OFPGMFC_BAD_COMMAND);
            res = LAGOPUS_RESULT_OFP_ERROR;
            break;
        }
      }

      if (res != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(res));
      }
    } else {
      lagopus_msg_warning("group_mod decode error (%s)\n",
                          lagopus_error_get_string(res));
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      res = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    /* params are NULL */
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return res;
}
