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
 *      @file   dp_apis.c
 *      @brief  Dataplane public APIs.
 */

#include "openflow.h"
#include "lagopus/bridge.h"
#include "lagopus/port.h"
#include "lagopus/interface.h"
#include "lagopus/vector.h"

#include "lagopus_apis.h"
#include "lagopus/dp_apis.h"
#include "lagopus/dataplane.h"

struct dp_bridge_iter {
  struct flowdb *flowdb;
  int table_id;
  int flow_idx;
};

static lagopus_hashmap_t interface_hashmap;
static lagopus_hashmap_t port_hashmap;
static lagopus_hashmap_t bridge_hashmap;
static lagopus_hashmap_t dpid_hashmap;
static lagopus_hashmap_t queue_hashmap;
static lagopus_hashmap_t queueid_hashmap;

/**
 * physical port id --> struct port table
 */
static struct vector *port_vector;

static void dp_port_interface_unset_internal(struct port *port);
static void dp_queue_free(void *queue);

lagopus_result_t
dp_api_init(void) {
  lagopus_result_t rv;

  port_vector = ports_alloc();
  if (port_vector == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  rv = lagopus_hashmap_create(&interface_hashmap,
                              LAGOPUS_HASHMAP_TYPE_STRING,
                              dp_interface_free);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = lagopus_hashmap_create(&port_hashmap,
                                LAGOPUS_HASHMAP_TYPE_STRING,
                                port_free);
  }
  if (rv == LAGOPUS_RESULT_OK) {
    rv = lagopus_hashmap_create(&bridge_hashmap,
                                LAGOPUS_HASHMAP_TYPE_STRING,
                                bridge_free);
  }
  if (rv == LAGOPUS_RESULT_OK) {
    /* dpid_hashmap is alternatively bridge reference. */
    rv = lagopus_hashmap_create(&dpid_hashmap,
                                LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                                NULL);
  }
  if (rv == LAGOPUS_RESULT_OK) {
    rv = lagopus_hashmap_create(&queue_hashmap,
                                LAGOPUS_HASHMAP_TYPE_STRING,
                                dp_queue_free);
  }
  if (rv == LAGOPUS_RESULT_OK) {
    rv = lagopus_hashmap_create(&queueid_hashmap,
                                LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                                NULL);
  }
  /* Initialize read write lock. */
  flowdb_lock_init(NULL);
  init_dp_timer();

  return rv;
}

void
dp_api_fini(void) {
  lagopus_hashmap_destroy(&interface_hashmap, true);
  lagopus_hashmap_destroy(&port_hashmap, true);
  lagopus_hashmap_destroy(&bridge_hashmap, true);
  lagopus_hashmap_destroy(&dpid_hashmap, false);
  lagopus_hashmap_destroy(&queue_hashmap, true);
  lagopus_hashmap_destroy(&queueid_hashmap, true);
  ports_free(port_vector);
}

/*
 * interface API
 */

lagopus_result_t
dp_interface_create(const char *name) {
  struct interface *ifp;
  lagopus_result_t rv;

  if (name == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&interface_hashmap, (void *)name, (void **)&ifp);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = LAGOPUS_RESULT_ALREADY_EXISTS;
    goto out;
  }
  ifp = dp_interface_alloc();
  if (ifp == NULL) {
    rv = LAGOPUS_RESULT_NO_MEMORY;
    goto out;
  }
  ifp->name = strdup(name);
  rv = lagopus_hashmap_add(&interface_hashmap,
                           (void *)name, (void **)&ifp, false);
  if (rv != LAGOPUS_RESULT_OK) {
    dp_interface_free(ifp);
    goto out;
  }
out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_interface_destroy(const char *name) {
  struct interface *ifp;
  lagopus_result_t rv;

  rv = dp_interface_stop(name);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  rv = dp_interface_info_set(name, NULL);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&interface_hashmap, (void *)name, (void **)&ifp);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }
  if (ifp->port != NULL) {
    dp_port_interface_unset_internal(ifp->port);
  }
  if (ifp->name != NULL) {
    free(ifp->name);
  }
  lagopus_hashmap_delete(&interface_hashmap, (void *)name, NULL, true);
out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_interface_info_set(const char *name,
                      datastore_interface_info_t *interface_info) {
  struct interface *ifp;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&interface_hashmap, (void *)name, (void **)&ifp);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  if (interface_info == NULL) {
    rv = dp_interface_unconfigure_internal(ifp);
    if (rv != LAGOPUS_RESULT_OK) {
      return rv;
    }
    /* Before clear ifp->info, free some pointers */
    switch (ifp->info.type) {
      case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
        if (ifp->info.eth_dpdk_phy.device) {
          free(ifp->info.eth_dpdk_phy.device);
        }
        if (ifp->info.eth_dpdk_phy.ip_addr) {
          free(ifp->info.eth_dpdk_phy.ip_addr);
        }
        break;
      case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
        if (ifp->info.eth_rawsock.device) {
          free(ifp->info.eth_rawsock.device);
        }
        if (ifp->info.eth_rawsock.ip_addr) {
          free(ifp->info.eth_rawsock.ip_addr);
        }
        break;
      case DATASTORE_INTERFACE_TYPE_VXLAN:
        if (ifp->info.vxlan.dst_addr) {
          free(ifp->info.vxlan.dst_addr);
        }
        if (ifp->info.vxlan.mcast_group) {
          free(ifp->info.vxlan.mcast_group);
        }
        if (ifp->info.vxlan.src_addr) {
          free(ifp->info.vxlan.src_addr);
        }
        if (ifp->info.vxlan.ip_addr) {
          free(ifp->info.vxlan.ip_addr);
        }
        break;
      case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      default:
        break;
    }
    memset(&ifp->info, 0, sizeof(ifp->info));
    return rv;
  } else {
    switch (ifp->info.type) {
      case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
      case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      case DATASTORE_INTERFACE_TYPE_VXLAN:
      case DATASTORE_INTERFACE_TYPE_UNKNOWN:
        break;
      default:
        return LAGOPUS_RESULT_INVALID_ARGS;
    }
    ifp->info = *interface_info;
    return dp_interface_configure_internal(ifp);
  }
}

lagopus_result_t
dp_interface_start(const char *name) {
  struct interface *ifp;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&interface_hashmap, (void *)name, (void **)&ifp);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  return dp_interface_start_internal(ifp);
}

lagopus_result_t
dp_interface_stop(const char *name) {
  struct interface *ifp;
  lagopus_result_t rv;

  if (name == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  rv = lagopus_hashmap_find(&interface_hashmap, (void *)name, (void **)&ifp);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  return dp_interface_stop_internal(ifp);
}

lagopus_result_t
dp_interface_hw_addr_get(const char *name, uint8_t *hw_addr) {
  struct interface *ifp;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&interface_hashmap, (void *)name, (void **)&ifp);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  return dp_interface_hw_addr_get_internal(ifp, hw_addr);
}


lagopus_result_t
dp_interface_stats_get(const char *name,
                       datastore_interface_stats_t *stats) {
  struct interface *ifp;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&interface_hashmap, (void *)name, (void **)&ifp);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  return dp_interface_stats_get_internal(ifp, stats);
}

lagopus_result_t
dp_interface_stats_clear(const char *name) {
  struct interface *ifp;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&interface_hashmap, (void *)name, (void **)&ifp);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  return dp_interface_stats_clear_internal(ifp);
}

/*
 * port API
 */

lagopus_result_t
dp_port_create(const char *name) {
  struct port *port;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&port_hashmap, (void *)name, (void **)&port);
  if (rv == LAGOPUS_RESULT_OK) {
    return LAGOPUS_RESULT_ALREADY_EXISTS;
  }
  port = port_alloc();
  if (port == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  strncpy(port->ofp_port.name, name, sizeof(port->ofp_port.name));
  port->ofp_port.config |= OFPPC_PORT_DOWN;
  rv = lagopus_hashmap_add(&port_hashmap, (void *)name, (void **)&port, false);
  if (rv != LAGOPUS_RESULT_OK) {
    port_free(port);
  }
  return rv;
}

lagopus_result_t
dp_port_destroy(const char *name) {

  /* XXX relation? */
  dp_port_stop(name);
  dp_port_interface_unset(name);
  lagopus_hashmap_delete(&port_hashmap, (void *)name, NULL, true);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_port_start(const char *name) {
  struct port *port;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&port_hashmap, (void *)name, (void **)&port);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  if (port->interface == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  port->ofp_port.config &= ~OFPPC_PORT_DOWN;

  return rv;
}

lagopus_result_t
dp_port_stop(const char *name) {
  struct port *port;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&port_hashmap, (void *)name, (void **)&port);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  port->ofp_port.config |= OFPPC_PORT_DOWN;

  return rv;
}

lagopus_result_t
dp_port_config_get(const char *name, uint32_t *config) {
  struct port *port;
  lagopus_result_t rv;

  flowdb_rdlock(NULL);
  rv = lagopus_hashmap_find(&port_hashmap, (void *)name, (void **)&port);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }
  *config = port->ofp_port.config;
out:
  flowdb_rdunlock(NULL);
  return rv;
}

lagopus_result_t
dp_port_state_get(const char *name, uint32_t *state) {
  struct port *port;
  lagopus_result_t rv;

  flowdb_rdlock(NULL);
  rv = lagopus_hashmap_find(&port_hashmap, (void *)name, (void **)&port);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }
  if (port->interface != NULL) {
    port->interface->stats(port);
  }
  *state = port->ofp_port.state;
out:
  flowdb_rdunlock(NULL);
  return rv;
}

lagopus_result_t
dp_port_interface_set(const char *name, const char *ifname) {
  struct port *port;
  struct interface *ifp;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&port_hashmap, (void *)name, (void **)&port);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }
  rv = lagopus_hashmap_find(&interface_hashmap, (void *)ifname, (void **)&ifp);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }
  port->interface = ifp;
  port->ifindex = ifp->info.eth.port_number;
  ifp->port = port;
  ifp->stats = port->stats;
  vector_set_index(port_vector,
                   ifp->info.eth.port_number,
                   port);
  if (port->interface != NULL) {
    rv = dp_interface_queue_configure(port->interface);
  }
out:
  flowdb_wrunlock(NULL);
  return rv;
}

static void
dp_port_interface_unset_internal(struct port *port) {
  if (port->interface != NULL) {
#ifdef HAVE_DPDK
    dpdk_queue_unconfigure(port->interface);
#endif /* HAVE_DPDK */
    vector_set_index(port_vector, port->ifindex, NULL);
    port->interface->port = NULL;
    port->interface->stats = NULL;
    port->interface = NULL;
  }
}

struct port *
dp_port_lookup(uint32_t portid) {
  return port_lookup(port_vector, portid);
}

uint32_t
dp_port_count(void) {
  uint32_t count, idx;

  for (count = 0, idx = 0; idx <= vector_max(port_vector); idx++) {
    if (vector_slot(port_vector, idx) != NULL) {
      count++;
    }
  }
  return count;
}

lagopus_result_t
dp_port_interface_unset(const char *name) {
  struct port *port;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&port_hashmap, (void *)name, (void **)&port);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }
  dp_port_interface_unset_internal(port);
out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_port_stats_get(const char *name,
                  datastore_port_stats_t *stats) {
  struct port *port;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&port_hashmap, (void *)name, (void **)&port);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }
  stats->config = port->ofp_port.config;
  stats->state =  port->ofp_port.state;
  stats->curr_features = port->ofp_port.curr;
  stats->supported_features = port->ofp_port.supported;

out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_port_queue_add(const char *name,
                  const char *queue_name) {
  struct port *port;
  datastore_queue_info_t *queue;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&port_hashmap, (void *)name, (void **)&port);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  rv = lagopus_hashmap_find(&queue_hashmap, (void *)queue_name,
                            (void **)&queue);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  if (port->interface != NULL) {
    rv = dp_interface_queue_add(port->interface, queue);
  }
  return rv;
}

lagopus_result_t
dp_port_queue_delete(const char *name,
                     const char *queue_name) {
  struct port *port;
  datastore_queue_info_t *queue;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&port_hashmap, (void *)name, (void **)&port);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  rv = lagopus_hashmap_find(&queue_hashmap, (void *)queue_name,
                            (void **)&queue);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  if (port->interface != NULL) {
    rv = dp_interface_queue_delete(port->interface, queue->id);
  }

  return rv;
}

lagopus_result_t
dp_port_policer_set(const char *name,
                    const char *policer_name) {
  (void) name;
  (void) policer_name;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_port_policer_unset(const char *name) {
  (void) name;
  return LAGOPUS_RESULT_OK;
}

/*
 * bridge API
 */
lagopus_result_t
dp_bridge_create(const char *name,
                 datastore_bridge_info_t *info) {
  struct bridge *bridge;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = LAGOPUS_RESULT_ALREADY_EXISTS;
    goto out;
  }
  bridge = bridge_alloc(name);
  if (bridge == NULL) {
    rv = LAGOPUS_RESULT_NO_MEMORY;
    goto out;
  }
  flowdb_wrlock(NULL);
  bridge->dpid = info->dpid;
  switch (info->fail_mode) {
    case DATASTORE_BRIDGE_FAIL_MODE_SECURE:
      bridge->fail_mode = FAIL_SECURE_MODE;
      break;
    case DATASTORE_BRIDGE_FAIL_MODE_STANDALONE:
      bridge->fail_mode = FAIL_STANDALONE_MODE;
      break;
    default:
      rv = LAGOPUS_RESULT_INVALID_ARGS;
      goto uout;
  }
  /* other parameter is just ignored. */
  rv = lagopus_hashmap_add(&bridge_hashmap, name, (void **)&bridge, false);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
    if (rv == LAGOPUS_RESULT_OK) {
      rv = lagopus_hashmap_add(&dpid_hashmap,
                               (void *)info->dpid, (void **)&bridge, false);
    }
  }
uout:
  flowdb_wrunlock(NULL);
out:
  return rv;
}

lagopus_result_t
dp_bridge_destroy(const char *name) {
  struct bridge *bridge;
  lagopus_result_t rv;

  dp_bridge_stop(name);
  flowdb_wrlock(NULL);
  /* Lookup bridge by name. */
  bridge = dp_bridge_lookup(name);
  if (bridge != NULL) {
    lagopus_hashmap_delete(&dpid_hashmap, (void *)bridge->dpid, NULL, true);
    lagopus_hashmap_delete(&bridge_hashmap, name, NULL, true);
    rv = LAGOPUS_RESULT_OK;
  } else {
    rv = LAGOPUS_RESULT_NOT_FOUND;
  }
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_bridge_start(const char *name) {
  struct bridge *bridge;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }

  bridge->running = true;

out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_bridge_stop(const char *name) {
  struct bridge *bridge;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }

  bridge->running = false;

out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_bridge_port_set(const char *name,
                   const char *port_name,
                   uint32_t port_num) {
  struct bridge *bridge;
  struct port *port;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);

  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }

  rv = lagopus_hashmap_find(&port_hashmap, (void *)port_name, (void **)&port);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }
  if (port->bridge != NULL) {
    rv = LAGOPUS_RESULT_ALREADY_EXISTS;
    goto out;
  }
  port->ofp_port.port_no = port_num;
  vector_set_index(bridge->ports, port->ofp_port.port_no, port);
  port->bridge = bridge;
  send_port_status(port, OFPPR_ADD);

out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_bridge_port_unset(const char *name, const char *port_name) {
  struct bridge *bridge;
  struct port *port;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);

  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }

  rv = lagopus_hashmap_find(&port_hashmap, (void *)port_name, (void **)&port);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }

  vector_set_index(bridge->ports, port->ofp_port.port_no, NULL);
  send_port_status(port, OFPPR_DELETE);
  port->bridge = NULL;

out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_bridge_port_unset_num(const char *name, uint32_t port_no) {
  struct bridge *bridge;
  struct port *port;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);

  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }

  port = port_lookup(bridge->ports, port_no);
  if (port == NULL) {
    rv = LAGOPUS_RESULT_NOT_FOUND;
    goto out;
  }

  vector_set_index(bridge->ports, port_no, NULL);
  send_port_status(port, OFPPR_DELETE);
  port->bridge = NULL;

out:
  flowdb_wrunlock(NULL);
  return rv;
}

struct bridge *
dp_bridge_lookup(const char *name) {
  struct bridge *bridge;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    bridge = NULL;
  }

  return bridge;
}

struct bridge *
dp_bridge_lookup_by_dpid(uint64_t dpid) {
  struct bridge *bridge;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&dpid_hashmap, (void *)dpid, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    bridge = NULL;
  }

  return bridge;
}

lagopus_result_t
dp_bridge_table_id_iter_create(const char *name, dp_bridge_iter_t *iterp) {
  dp_bridge_iter_t iter;
  struct bridge *bridge;

  bridge = dp_bridge_lookup(name);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }
  iter = calloc(1, sizeof(struct dp_bridge_iter));
  if (iter == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  iter->flowdb = bridge->flowdb;
  iter->table_id = 0;

  *iterp = iter;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_bridge_table_id_iter_get(dp_bridge_iter_t iter, uint8_t *idp) {
  int i;

  for (i = iter->table_id; i < OFPTT_ALL; i++) {
    if (iter->flowdb->tables[i] != NULL) {
      iter->table_id = i + 1;
      *idp = i;
      return LAGOPUS_RESULT_OK;
    }
  }
  return LAGOPUS_RESULT_EOF;
}

void
dp_bridge_table_id_iter_destroy(dp_bridge_iter_t iter) {
  free(iter);
}

lagopus_result_t
dp_bridge_flow_iter_create(const char *name, uint8_t table_id,
                           dp_bridge_iter_t *iterp) {
  dp_bridge_iter_t iter;
  struct bridge *bridge;

  bridge = dp_bridge_lookup(name);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }
  if (bridge->flowdb->tables[table_id] == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }
  iter = calloc(1, sizeof(struct dp_bridge_iter));
  if (iter == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  iter->flowdb = bridge->flowdb;
  iter->table_id = table_id;
  iter->flow_idx = 0;

  *iterp = iter;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_bridge_flow_iter_get(dp_bridge_iter_t iter, struct flow **flowp) {
  struct table *table;
  struct flow_list *flow_list;

  table = iter->flowdb->tables[iter->table_id];
  if (table != NULL) {
    flow_list = table->flow_list;
    if (iter->flow_idx < flow_list->nflow) {
      *flowp = flow_list->flows[iter->flow_idx++];
      return LAGOPUS_RESULT_OK;
    }
  }
  return LAGOPUS_RESULT_EOF;
}

void
dp_bridge_flow_iter_destroy(dp_bridge_iter_t iter) {
  free(iter);
}

lagopus_result_t
dp_bridge_stats_get(const char *name,
                    datastore_bridge_stats_t *stats) {
  struct table_stats_list list;
  struct ofp_error error;
  struct ofcachestat cache_stats;
  struct bridge *bridge;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }

  stats->flowcache_entries = 0;
  stats->flowcache_hit = 0;
  stats->flowcache_miss = 0;
  stats->flow_entries = 0;
  stats->flow_lookup_count = 0;
  stats->flow_matched_count = 0;
  TAILQ_INIT(&list);
  rv = flowdb_table_stats(bridge->flowdb, &list, &error);
  if (rv == LAGOPUS_RESULT_OK) {
    struct table_stats *table_stats;

    TAILQ_FOREACH(table_stats, &list, entry) {
      stats->flow_entries += table_stats->ofp.active_count;
      stats->flow_lookup_count += table_stats->ofp.lookup_count;
      stats->flow_matched_count += table_stats->ofp.matched_count;
    }
    while ((table_stats = TAILQ_FIRST(&list)) != NULL) {
      TAILQ_REMOVE(&list, table_stats, entry);
      free(table_stats);
    }
  }
  dp_get_flowcache_statistics(bridge, &cache_stats);
  stats->flowcache_entries = cache_stats.nentries;
  stats->flowcache_hit = cache_stats.hit;
  stats->flowcache_miss = cache_stats.miss;

out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_bridge_l2_clear(const char *name) {
  struct bridge *bridge;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }
  /* not implemented yet */

out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_bridge_l2_expire_set(const char *name, uint64_t expire) {
  struct bridge *bridge;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }
  /* not implemented yet */

out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_bridge_l2_max_entries_set(const char *name,
                             uint64_t max_entries) {
  struct bridge *bridge;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }
  /* not implemented yet */

out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_bridge_l2_entries_get(const char *name, uint64_t *entries) {
  struct bridge *bridge;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }
  /* not implemented yet */

out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_bridge_l2_info_get(const char *name,
                      datastore_l2_dump_proc_t proc,
                      FILE *fp,
                      lagopus_dstring_t *result) {
  struct bridge *bridge;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }
  /* not implemented yet */

out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_bridge_meter_list_get(const char *name,
                         datastore_bridge_meter_info_list_t *list) {
  struct ofp_meter_multipart_request req;
  struct ofp_error error;
  struct bridge *bridge;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }

  req.meter_id = OFPM_ALL;
  rv = get_meter_config(bridge->meter_table, &req, list, &error);
out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_bridge_meter_stats_list_get(const char *name,
                               datastore_bridge_meter_stats_list_t *list) {
  struct ofp_meter_multipart_request req;
  struct ofp_error error;
  struct bridge *bridge;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }
  req.meter_id = OFPM_ALL;
  rv = get_meter_stats(bridge->meter_table, &req, list, &error);
out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_bridge_group_list_get(const char *name,
                         datastore_bridge_group_info_list_t *list) {
  struct ofp_error error;
  struct bridge *bridge;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }

  rv = group_descs(bridge->group_table, list, &error);

out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_bridge_group_stats_list_get(const char *name,
                               datastore_bridge_group_stats_list_t *list) {
  struct ofp_group_stats_request req;
  struct ofp_error error;
  struct bridge *bridge;
  lagopus_result_t rv;

  flowdb_wrlock(NULL);
  rv = lagopus_hashmap_find(&bridge_hashmap, (void *)name, (void **)&bridge);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }

  req.group_id = OFPG_ALL;
  rv = group_stats(bridge->group_table, &req, list, &error);
out:
  flowdb_wrunlock(NULL);
  return rv;
}

lagopus_result_t
dp_affinition_list_get(datastore_affinition_info_list_t *list) {
  return LAGOPUS_RESULT_ANY_FAILURES;
}

lagopus_result_t
dp_queue_create(const char *name,
                datastore_queue_info_t *queue_info) {
  datastore_queue_info_t *queue;
  uint64_t id;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&queue_hashmap, (void *)name, (void **)&queue);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = LAGOPUS_RESULT_ALREADY_EXISTS;
    goto out;
  }
  id = queue_info->id;
  rv = lagopus_hashmap_find(&queueid_hashmap, (void *)id, (void **)&queue);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = LAGOPUS_RESULT_ALREADY_EXISTS;
    goto out;
  }
  queue = calloc(1, sizeof(datastore_queue_info_t));
  if (queue == NULL) {
    rv = LAGOPUS_RESULT_NO_MEMORY;
    goto out;
  }
  memcpy(queue, queue_info, sizeof(datastore_queue_info_t));
  rv = lagopus_hashmap_add(&queue_hashmap, (void *)name, (void **)&queue, false);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = lagopus_hashmap_find(&queue_hashmap, (void *)name, (void **)&queue);
    if (rv != LAGOPUS_RESULT_OK) {
      goto out;
    }
    id = queue_info->id;
    rv = lagopus_hashmap_add(&queueid_hashmap, (void *)id,
                             (void **)&queue, false);
  }
out:
  return rv;
}

static void
dp_queue_free(void *queue) {
  free((datastore_queue_info_t *)queue);
}

void
dp_queue_destroy(const char *name) {
  datastore_queue_info_t *queue;
  lagopus_result_t rv;
  uint64_t id;

  dp_queue_stop(name);
  rv = lagopus_hashmap_find(&queue_hashmap, (void *)name, (void **)&queue);
  if (rv == LAGOPUS_RESULT_OK) {
    id = queue->id;
    lagopus_hashmap_delete(&queueid_hashmap, (void *)id, NULL, false);
    lagopus_hashmap_delete(&queue_hashmap, (void *)name, NULL, true);
  }
}

lagopus_result_t
dp_queue_start(const char *name) {
  (void) name;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_queue_stop(const char *name) {
  (void) name;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_queue_stats_get(const char *name,
                   datastore_queue_stats_t *stats) {
  (void) name;
  (void) stats;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_policer_create(const char *name,
                  datastore_policer_info_t *policer_info) {
  (void) name;
  (void) policer_info;
  return LAGOPUS_RESULT_OK;
}

void
dp_policer_destroy(const char *name) {
  (void) name;
}

lagopus_result_t
dp_policer_start(const char *name) {
  (void) name;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_policer_stop(const char *name) {
  (void) name;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_policer_action_add(const char *name,
                      char *action_name) {
  (void) name;
  (void) action_name;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_policer_action_delete(const char *name,
                         char *action_name) {
  (void) name;
  (void) action_name;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_policer_action_create(const char *name,
                         datastore_policer_action_info_t *
                         policer_action_info) {
  (void) name;
  (void) policer_action_info;
  return LAGOPUS_RESULT_OK;
}

void
dp_policer_action_destroy(const char *name) {
  (void) name;
}

lagopus_result_t
dp_policer_action_start(const char *name) {
  (void) name;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_policer_action_stop(const char *name) {
  (void) name;
  return LAGOPUS_RESULT_OK;
}
