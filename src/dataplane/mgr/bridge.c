/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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
 *	@file	bridge.c
 *	@brief	Bridge (a.k.a. OpenFlow Switch) management.
 */

#include "openflow.h"
#include "lagopus/bridge.h"
#include "lagopus/port.h"
#include "lagopus/flowdb.h"
#include "lagopus/group.h"
#include "lagopus/meter.h"
#include "lagopus/vector.h"

#define SET32_FLAG(V, F)        (V) = (V) | (uint32_t)(F)
#define UNSET32_FLAG(V, F)      (V) = (V) & (uint32_t)~(F)

/* Allocate a new bridge. */
static struct bridge *
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
  if (bridge->flowdb != NULL) {
#ifdef HAVE_DPDK
    clear_worker_flowcache(true);
#endif /* HAVE_DPDK */
    flowdb_free(bridge->flowdb);
  }
  if (bridge->group_table != NULL) {
    group_table_free(bridge->group_table);
  }
  if (bridge->meter_table != NULL) {
    meter_table_free(bridge->meter_table);
  }
  if (bridge->controller_address != NULL) {
    free(bridge->controller_address);
  }
  free(bridge);
}

/* Lookup bridge. */
struct bridge *
bridge_lookup(struct bridge_list *bridge_list, const char *name) {
  struct bridge *bridge;

  TAILQ_FOREACH(bridge, bridge_list, entry) {
    if (strncmp(bridge->name, name, BRIDGE_MAX_NAME_LEN) == 0) {
      return bridge;
    }
  }
  return NULL;
}


struct bridge *
bridge_lookup_by_dpid(struct bridge_list *bridge_list, uint64_t dpid) {
  struct bridge *bridge;

  TAILQ_FOREACH(bridge, bridge_list, entry) {
    if (bridge->dpid == dpid) {
      return bridge;
    }
  }
  return NULL;
}

/* Lookup bridge by controller_address. */
struct bridge *
bridge_lookup_by_controller_address(struct bridge_list *bridge_list,
                                    const char *name) {
  struct bridge *bridge;

  TAILQ_FOREACH(bridge, bridge_list, entry) {
    if (bridge->controller_address != NULL &&
        strcmp(bridge->controller_address, name) == 0) {
      return bridge;
    }
  }
  return NULL;
}

/* Generate random datapath id. */
static uint64_t
bridge_dpid_generate(void) {
  uint64_t dpid;

  dpid = (uint64_t)random();
  dpid <<= 32;
  dpid |= (uint64_t)random();

  return dpid;
}

/* Add a new bridge. */
lagopus_result_t
bridge_add(struct bridge_list *bridge_list, const char *name, uint64_t dpid) {
  struct bridge *bridge;

  /* Find bridge. */
  bridge = bridge_lookup(bridge_list, name);
  if (bridge != NULL) {
    return LAGOPUS_RESULT_ALREADY_EXISTS;
  }

  /* Allocate a new bridge. */
  bridge = bridge_alloc(name);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  /* If given dpid is zero, generate random dpid value. */
  if (dpid == 0) {
    dpid = bridge_dpid_generate();
  }
  bridge->dpid = dpid;

  flowdb_wrlock(NULL);
  TAILQ_INSERT_TAIL(bridge_list, bridge, entry);
  flowdb_wrunlock(NULL);

  lagopus_msg_info("Bridge %s dpid:%0" PRIx64 " is added\n", name, dpid);
  return LAGOPUS_RESULT_OK;
}

/* Delete bridge. */
lagopus_result_t
bridge_delete(struct bridge_list *bridge_list, const char *name) {
  struct bridge *bridge;
  lagopus_result_t ret;

  flowdb_wrlock(NULL);
  /* Lookup bridge by name. */
  bridge = bridge_lookup(bridge_list, name);
  if (bridge != NULL) {
    TAILQ_REMOVE(bridge_list, bridge, entry);
    bridge_free(bridge);
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  flowdb_wrunlock(NULL);
  /* not implemented yet. */
  return ret;
}

/* Add port to the bridge. */
lagopus_result_t
bridge_port_add(struct bridge_list *bridge_list, const char *name,
                struct port *port) {
  struct bridge *bridge;
  lagopus_result_t ret;

  flowdb_wrlock(NULL);
  /* Lookup bridge by name. */
  bridge = bridge_lookup(bridge_list, name);
  if (bridge != NULL) {
    static const uint8_t zeromac[] = "\0\0\0\0\0\0";
    if (memcmp(port->ofp_port.hw_addr, zeromac, 6) != 0) {
      /* Set port to the bridge's port vector. */
      printf("Assigning port id %u to bridge %s\n",
             port->ifindex, bridge->name);
      vector_set_index(bridge->ports, port->ofp_port.port_no, port);
      port->bridge = bridge;
      send_port_status(port, OFPPR_ADD);
    }
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }

  flowdb_wrunlock(NULL);
  return ret;
}

/* Delete port to the bridge. */
lagopus_result_t
bridge_port_delete(struct bridge_list *bridge_list, const char *name,
                   uint32_t portid) {
  struct bridge *bridge;
  struct port *port;
  lagopus_result_t ret;

  flowdb_wrlock(NULL);
  /* Lookup bridge by name. */
  bridge = bridge_lookup(bridge_list, name);
  if (bridge != NULL) {
    port = ifindex2port(bridge->ports, portid);
    if (port != NULL && port->bridge == bridge) {
      printf("Release port id %u from bridge %s\n",
             port->ifindex, bridge->name);
      vector_set_index(bridge->ports, port->ofp_port.port_no, NULL);
      send_port_status(port, OFPPR_DELETE);
      port->bridge = NULL;
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }

  flowdb_wrunlock(NULL);
  return ret;
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

const char *
bridge_name_get(struct bridge *bridge) {
  return (const char *) bridge->name;
}

lagopus_result_t
bridge_controller_add(struct bridge *bridge,
                      const char *controller_address) {
  if (bridge->controller_address != NULL) {
    free(bridge->controller_address);
  }
  bridge->controller_address = strdup(controller_address);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
bridge_controller_delete(struct bridge *bridge,
                         const char *controller_address) {
  if (bridge->controller_address != NULL &&
      !strcmp(bridge->controller_address, controller_address)) {
    free(bridge->controller_address);
    bridge->controller_address = NULL;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_NOT_FOUND;
}

uint64_t
bridge_dpid(struct bridge *bridge) {
  return bridge->dpid;
}
