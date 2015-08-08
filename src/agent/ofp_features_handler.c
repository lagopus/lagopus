/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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
#include "lagopus/datastore.h"
#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_pdump.h"
#include "lagopus/ofp_bridgeq_mgr.h"
#include "ofp_apis.h"

typedef struct capability_proc_info {
  uint32_t features_capability;
  uint64_t ds_capability;
} capability_proc_info_t;

static const capability_proc_info_t capabilities[] = {
  {OFPC_FLOW_STATS, DATASTORE_BRIDGE_BIT_CAPABILITY_FLOW_STATS},
  {OFPC_TABLE_STATS, DATASTORE_BRIDGE_BIT_CAPABILITY_TABLE_STATS},
  {OFPC_PORT_STATS, DATASTORE_BRIDGE_BIT_CAPABILITY_PORT_STATS},
  {OFPC_GROUP_STATS, DATASTORE_BRIDGE_BIT_CAPABILITY_GROUP_STATS},
  {OFPC_IP_REASM, DATASTORE_BRIDGE_BIT_CAPABILITY_REASSEMBLE_IP_FRGS},
  {OFPC_QUEUE_STATS, DATASTORE_BRIDGE_BIT_CAPABILITY_QUEUE_STATS},
  {OFPC_PORT_BLOCKED, DATASTORE_BRIDGE_BIT_CAPABILITY_BLOCK_LOOPING_PORTS}
};

static const size_t capabilities_size =
    sizeof(capabilities) / sizeof(capability_proc_info_t);

typedef lagopus_result_t
(*capability_set_proc_t)(const char *name, bool current, bool *b);

static void
features_request_trace(struct ofp_header *header) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_FEATURES_REQUEST)) {
    lagopus_msg_pdump(TRACE_OFPT_FEATURES_REQUEST, PDUMP_OFP_HEADER,
                      *header, "");
  }
}

static lagopus_result_t
features_reply_features_capability_set(uint64_t ds_capability,
                                       size_t i,
                                       uint32_t *flags) {
  if ((ds_capability & capabilities[i].ds_capability) == 0) {
    *flags &= ~capabilities[i].features_capability;
  } else {
    *flags |= capabilities[i].features_capability;
  }

  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
features_reply_features_get(struct channel *channel,
                            struct ofp_switch_features *features) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_bridge_info_t info;
  size_t i;

  /* set ids/reserved. */
  features->datapath_id = channel_dpid_get(channel);
  features->auxiliary_id = channel_auxiliary_id_get(channel);
  features->reserved = 0;

  if ((ret = ofp_bridgeq_mgr_info_get(features->datapath_id,
                                      &info)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("FAILED (%s).\n",
                        lagopus_error_get_string(ret));
    goto done;
  }

  /* set max. */
  features->n_buffers = info.max_buffered_packets;
  features->n_tables = info.max_tables;

  /* set capabilities. */
  features->capabilities = 0;
  for (i = 0; i < capabilities_size; i++) {
    if ((ret = features_reply_features_capability_set(
            info.capabilities,
            i,
            &features->capabilities)) != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
      goto done;
    }
  }

done:
  return ret;
}

/* send */
/* Send switch features reply. */
STATIC lagopus_result_t
ofp_features_reply_create(struct channel *channel,
                          struct pbuf **pbuf,
                          struct ofp_header *xid_header) {
  struct ofp_switch_features features;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (channel != NULL && pbuf != NULL && xid_header != NULL) {
    /* alloc */
    *pbuf = channel_pbuf_list_get(channel,
                                  sizeof(struct ofp_switch_features));
    if (*pbuf != NULL) {
      pbuf_plen_set(*pbuf, sizeof(struct ofp_switch_features));

      ret = features_reply_features_get(channel, &features);
      if (ret == LAGOPUS_RESULT_OK) {
        /* Fill in header. */
        ofp_header_set(&features.header, channel_version_get(channel),
                       OFPT_FEATURES_REPLY, (uint16_t) pbuf_plen_get(*pbuf),
                       xid_header->xid);

        /* Encode message. */
        ret = ofp_switch_features_encode(*pbuf, &features);
        if (ret != LAGOPUS_RESULT_OK) {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("Can't allocate pbuf.\n");
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }

    if (ret != LAGOPUS_RESULT_OK && *pbuf != NULL) {
      channel_pbuf_list_unget(channel, *pbuf);
      *pbuf = NULL;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* RECV */
/* Features Request packet receive. */
lagopus_result_t
ofp_features_request_handle(struct channel *channel, struct pbuf *pbuf,
                            struct ofp_header *xid_header,
                            struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *send_pbuf = NULL;

  /* Parse packet. */
  ret = ofp_header_handle(channel, pbuf, error);
  if (ret == LAGOPUS_RESULT_OK) {
    /* Features request reply. */
    ret = ofp_features_reply_create(channel, &send_pbuf,
                                    xid_header);
    if (ret == LAGOPUS_RESULT_OK) {
      features_request_trace(xid_header);

      channel_send_packet(channel, send_pbuf);
      ret = LAGOPUS_RESULT_OK;
    } else {
      lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
    }
  }

  if (ret != LAGOPUS_RESULT_OK && send_pbuf != NULL) {
    channel_pbuf_list_unget(channel, send_pbuf);
  }

  return ret;
}
