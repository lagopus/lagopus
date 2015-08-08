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

/**
 *      @file   bridge.c
 *      @brief  Bridge (a.k.a. OpenFlow Switch) management.
 */

#include "openflow.h"
#include "lagopus/bridge.h"
#include "lagopus/port.h"
#include "lagopus/flowdb.h"
#include "lagopus/group.h"
#include "lagopus/meter.h"
#include "lagopus/vector.h"

#include "lagopus/dp_apis.h"

#define SET32_FLAG(V, F)        (V) = (V) | (uint32_t)(F)
#define UNSET32_FLAG(V, F)      (V) = (V) & (uint32_t)~(F)

/* Allocate a new bridge. */
struct bridge *
bridge_alloc(const char *name) {
  struct bridge *bridge;

  /* Allocate memory. */
  bridge = (struct bridge *)calloc(1, sizeof(struct bridge));
  if (bridge == NULL) {
    return NULL;
  }

  /* Set bridge name. */
  strncpy(bridge->name, name, BRIDGE_MAX_NAME_LEN);

  /* Set default wire protocol version to OpenFlow 1.3. */
  bridge_ofp_version_set(bridge, OPENFLOW_VERSION_1_3);
  bridge_ofp_version_bitmap_set(bridge, OPENFLOW_VERSION_1_3);

  /* Set default fail mode. */
  bridge_fail_mode_set(bridge, FAIL_STANDALONE_MODE);

  /* Set default features. */
  bridge->features.n_buffers = BRIDGE_N_BUFFERS_DEFAULT;
  bridge->features.n_tables = BRIDGE_N_TABLES_DEFAULT;

  /* Auxiliary connection id.  This value is not used.  For
   * auxiliary_id, please use channel_auxiliary_id_get(). */
  bridge->features.auxiliary_id = 0;

  /* Capabilities. */
  SET32_FLAG(bridge->features.capabilities, OFPC_FLOW_STATS);
  SET32_FLAG(bridge->features.capabilities, OFPC_TABLE_STATS);
  SET32_FLAG(bridge->features.capabilities, OFPC_PORT_STATS);
  SET32_FLAG(bridge->features.capabilities, OFPC_GROUP_STATS);
  UNSET32_FLAG(bridge->features.capabilities, OFPC_IP_REASM);
  SET32_FLAG(bridge->features.capabilities, OFPC_QUEUE_STATS);
  UNSET32_FLAG(bridge->features.capabilities, OFPC_PORT_BLOCKED);

  bridge->switch_config.flags = OFPC_FRAG_NORMAL;
  bridge->switch_config.miss_send_len = 128;

  /* Prepare bridge's port vector. */
  bridge->ports = vector_alloc();
  if (bridge->ports == NULL) {
    goto out;
  }

  /* Allocate flowdb with table 0. */
  bridge->flowdb = flowdb_alloc(1);
  if (bridge->flowdb == NULL) {
    goto out;
  }

  /* Allocate group table. */
  bridge->group_table = group_table_alloc(bridge);
  if (bridge->group_table == NULL) {
    goto out;
  }

  /* Allocate meter table. */
  bridge->meter_table = meter_table_alloc();
  if (bridge->meter_table == NULL) {
    goto out;
  }

  /* Return bridge. */
  return bridge;

out:
  bridge_free(bridge);
  return NULL;
}

/* Bridge free. */
void
bridge_free(struct bridge *bridge) {
  if (bridge->ports != NULL) {
    struct port *port;
    vindex_t i, max;

    max = bridge->ports->allocated;
    for (i = 0; i < max; i++) {
      port = vector_slot(bridge->ports, i);
      if (port == NULL) {
        continue;
      }
      port->bridge = NULL;
    }
    vector_free(bridge->ports);
  }
  if (bridge->group_table != NULL) {
    group_table_free(bridge->group_table);
  }
  if (bridge->flowdb != NULL) {
#ifdef HAVE_DPDK
    clear_worker_flowcache(true);
#endif /* HAVE_DPDK */
    flowdb_free(bridge->flowdb);
  }
  if (bridge->meter_table != NULL) {
    meter_table_free(bridge->meter_table);
  }
  free(bridge);
}

lagopus_result_t
bridge_fail_mode_get(struct bridge *bridge,
                     enum fail_mode *fail_mode) {
  *fail_mode = bridge->fail_mode;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
bridge_fail_mode_set(struct bridge *bridge,
                     enum fail_mode fail_mode) {
  if (fail_mode != FAIL_SECURE_MODE && fail_mode != FAIL_STANDALONE_MODE) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  bridge->fail_mode = fail_mode;

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
bridge_ofp_version_get(struct bridge *bridge, uint8_t *version) {
  *version = bridge->version;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
bridge_ofp_version_set(struct bridge *bridge, uint8_t version) {
  if (version != OPENFLOW_VERSION_1_3 && version != OPENFLOW_VERSION_1_4) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (bridge->version != version) {
    /* We need to reset all of connections and flows. */
    bridge->version = version;
  }
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
bridge_ofp_version_bitmap_get(struct bridge *bridge,
                              uint32_t *version_bitmap) {
  *version_bitmap = bridge->version_bitmap;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
bridge_ofp_version_bitmap_set(struct bridge *bridge, uint8_t version) {
  if (version != OPENFLOW_VERSION_1_3 && version != OPENFLOW_VERSION_1_4) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  SET32_FLAG(bridge->version_bitmap, (1 << version));
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
bridge_ofp_version_bitmap_unset(struct bridge *bridge, uint8_t version) {
  if (version != OPENFLOW_VERSION_1_3 && version != OPENFLOW_VERSION_1_4) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  UNSET32_FLAG(bridge->version_bitmap, (1 << version));
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
bridge_ofp_features_get(struct bridge *bridge,
                        struct ofp_switch_features *features) {
  *features = bridge->features;
  return LAGOPUS_RESULT_OK;
}
