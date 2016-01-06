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
#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/ofp_pdump.h"
#include "ofp_apis.h"
#include "ofp_element.h"
#include "ofp_role.h"

static void
set_async_trace(struct ofp_async_config *async_config) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_SET_ASYNC)) {
    lagopus_msg_pdump(TRACE_OFPT_SET_ASYNC, PDUMP_OFP_HEADER,
                      async_config->header, "");
    lagopus_msg_pdump(TRACE_OFPT_SET_ASYNC, PDUMP_OFP_SET_ASYNC,
                      *async_config, "");
  }
}

lagopus_result_t
ofp_set_async_handle(struct channel *channel, struct pbuf *pbuf,
                     struct ofp_header *xid_header,
                     struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_async_config async_config;

  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {

    ret = pbuf_plen_check(pbuf, xid_header->length);

    if (ret == LAGOPUS_RESULT_OK) {
      /* Parse packet. */
      ret = ofp_async_config_decode(pbuf, &async_config);
      if (ret == LAGOPUS_RESULT_OK) {
        /* dump trace. */
        set_async_trace(&async_config);

        /* Copy packet_in_mask, port_status_mask, flow_removed_mask. */
        channel_role_mask_set(channel, &async_config);
      } else {
        lagopus_msg_warning("FAILED : ofp_config_decode (%s).\n",
                            lagopus_error_get_string(ret));
        ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
        ret = LAGOPUS_RESULT_OFP_ERROR;
      }
    } else {
      lagopus_msg_warning("bad length.\n");
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return ret;
}
