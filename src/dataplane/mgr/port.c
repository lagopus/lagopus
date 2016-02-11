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

/**
 *      @file   port.c
 *      @brief  OpenFlow Port management.
 */

#include "lagopus_apis.h"
#include <openflow.h>

#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
#include "lagopus/ofp_dp_apis.h"

#include "lagopus/dp_apis.h"
#include "lagopus/interface.h"

/**
 * no driver version of port_stats().
 */
static struct port_stats *
port_stats(struct port *port) {
  struct timespec ts;
  struct port_stats *stats;

  stats = calloc(1, sizeof(struct port_stats));
  if (stats == NULL) {
    return NULL;
  }
  /* all statistics are not supported except duration. */
  stats->ofp.port_no = port->ofp_port.port_no;
  stats->ofp.rx_packets = UINT64_MAX;
  stats->ofp.tx_packets = UINT64_MAX;
  stats->ofp.rx_bytes = UINT64_MAX;
  stats->ofp.tx_bytes = UINT64_MAX;
  stats->ofp.rx_dropped = UINT64_MAX;
  stats->ofp.tx_dropped = UINT64_MAX;
  stats->ofp.rx_errors = UINT64_MAX;
  stats->ofp.tx_errors = UINT64_MAX;
  stats->ofp.rx_frame_err = UINT64_MAX;
  stats->ofp.rx_over_err = UINT64_MAX;
  stats->ofp.rx_crc_err = UINT64_MAX;
  stats->ofp.collisions = UINT64_MAX;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  stats->ofp.duration_sec = (uint32_t)(ts.tv_sec - port->create_time.tv_sec);
  if (ts.tv_nsec < port->create_time.tv_nsec) {
    stats->ofp.duration_sec--;
    stats->ofp.duration_nsec = 1 * 1000 * 1000 * 1000;
  } else {
    stats->ofp.duration_nsec = 0;
  }
  stats->ofp.duration_nsec += (uint32_t)ts.tv_nsec;
  stats->ofp.duration_nsec -= (uint32_t)port->create_time.tv_nsec;

  return stats;
}

struct port *
port_alloc(void) {
  struct port *port;

  port = calloc(1, sizeof(struct port));
  if (port == NULL) {
    return NULL;
  }
  clock_gettime(CLOCK_MONOTONIC, &port->create_time);

  return port;
}

void
port_free(struct port *port) {
  free(port);
}

/**
 * lookup port structure from specified table
 *
 * @param[in] hm      hashmap table associated with bridge
 * @param[in] id      OpenFlow Port number
 * @retval    ==NULL  port is not configured
 * @retval    !=NULL  port structure
 */
struct port *
port_lookup(lagopus_hashmap_t *hm, uint32_t id) {
  struct port *port;

  lagopus_hashmap_find_no_lock(hm, id, &port);
  return port;
}

static void
port_add_stats(struct port *port, struct port_stats_list *list) {
  struct port_stats *stats;

  if (port->interface != NULL && port->interface->stats != NULL) {
    stats = port->interface->stats(port);
    if (stats != NULL) {
      TAILQ_INSERT_TAIL(list, stats, entry);
    }
  }
}

static bool
port_do_stats_iterate(void *key, void *val,
                      lagopus_hashentry_t he, void *arg) {
  struct port *port;
  struct port_stats_list *list;

  (void) key;
  (void) he;

  port = val;
  list = arg;

  port_add_stats(port, list);
  return true;
}

lagopus_result_t
lagopus_get_port_statistics(lagopus_hashmap_t *hm,
                            struct ofp_port_stats_request *request,
                            struct port_stats_list *list,
                            struct ofp_error *error) {
  struct port *port;

  if (request->port_no == OFPP_ANY) {
    lagopus_hashmap_iterate(hm, port_do_stats_iterate, list);
  } else {
    if (lagopus_hashmap_find(hm, (void *)request->port_no, &port)
        != LAGOPUS_RESULT_OK) {
      error->type = OFPET_BAD_REQUEST;
      error->code = OFPBRC_BAD_PORT;
      lagopus_msg_info("port stats: %d: no such port (%d:%d)",
                       request->port_no, error->type, error->code);
      return LAGOPUS_RESULT_OFP_ERROR;
    }
    port_add_stats(port, list);
  }

  return LAGOPUS_RESULT_OK;
}

void
dp_port_update_link_status(struct port *port) {
  struct interface *ifp;

  ifp = port->interface;
  if (ifp == NULL) {
    return;
  }
  switch (ifp->info.type) {
#ifdef HAVE_DPDK
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
      dpdk_update_port_link_status(port);
      break;
#endif /* HAVE_DPDK */
    default:
      break;
  }
}

/**
 * Change physical port state.
 *
 * @param[in]   port    physical port.
 *
 * @retval      LAGOPUS_RESULT_OK       success.
 */
static lagopus_result_t
lagopus_change_physical_port(struct port *port) {
  if (port->interface != NULL) {
    dp_interface_change_config(port->interface,
                               port->ofp_port.advertised,
                               port->ofp_port.config);
  }
  send_port_status(port, OFPPR_MODIFY);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
port_config(struct bridge *bridge,
            struct ofp_port_mod *port_mod,
            struct ofp_error *error) {
  struct port *port;
  struct ofp_port *ofp_port;
  uint32_t oldconfig, newconfig;

  if (port_mod->port_no != OFPP_CONTROLLER) {
    port = port_lookup(&bridge->ports, port_mod->port_no);
    if (port == NULL) {
      error->type = OFPET_PORT_MOD_FAILED;
      error->code = OFPPMFC_BAD_PORT;
      lagopus_msg_info("port config: %d: no such port (%d:%d)",
                       port_mod->port_no, error->type, error->code);
      return LAGOPUS_RESULT_OFP_ERROR;
    }
    ofp_port = &port->ofp_port;
    if (port->interface == NULL) {
      error->type = OFPET_PORT_MOD_FAILED;
      error->code = OFPPMFC_BAD_HW_ADDR;
      lagopus_msg_info("port config: %d: do not assigned interface (%d:%d)",
                       port_mod->port_no, error->type, error->code);
      return LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    port = NULL; /* XXX shut out warning */
    ofp_port = &bridge->controller_port;
  }
  /* XXX write lock for thread safe */
  if ((port_mod->config & (uint32_t)~port_mod->mask) != 0) {
    error->type = OFPET_PORT_MOD_FAILED;
    error->code = OFPPMFC_BAD_CONFIG;
    lagopus_msg_info("port config: "
                     "config(0x%x) and mask(0x%x) inconsistency (%d:%d)",
                     port_mod->config, port_mod->mask,
                     error->type, error->code);
    return LAGOPUS_RESULT_OFP_ERROR;
  }
  oldconfig = ofp_port->config;
  newconfig = oldconfig;
  newconfig &= (uint32_t)~port_mod->mask;
  newconfig |= port_mod->config;
  ofp_port->config = newconfig;
  /* advertise, depend on lower driver */
  if ((oldconfig != newconfig ||
       ofp_port->advertised != port_mod->advertise) &&
      port_mod->port_no != OFPP_CONTROLLER) {
    ofp_port->advertised = port_mod->advertise;
    lagopus_change_physical_port(port);
  }
  /* XXX unlock */

  return LAGOPUS_RESULT_OK;
}

static bool
port_do_desc_iterate(void *key, void *val,
                     lagopus_hashentry_t he, void *arg) {
  struct port_desc *desc;
  struct port *port;
  struct port_stats_list *list;

  (void) key;
  (void) he;

  port = val;
  list = arg;

  desc = calloc(1, sizeof(struct port_desc));
  if (desc == NULL) {
    return true;
  }
  memcpy(&desc->ofp, &port->ofp_port, sizeof(struct ofp_port));
  TAILQ_INSERT_TAIL(list, desc, entry);
  return true;
}

lagopus_result_t
get_port_desc(lagopus_hashmap_t *hm,
              struct port_desc_list *list,
              struct ofp_error *error) {
  (void) error;

  lagopus_hashmap_iterate(hm, port_do_desc_iterate, list);
  return LAGOPUS_RESULT_OK;
}

bool
port_liveness(struct bridge *bridge, uint32_t port_no) {
  struct port *port;

  if (port_no == OFPP_ANY) {
    return false;
  }
  port = port_lookup(&bridge->ports, port_no);
  if (port == NULL) {
    return false;
  }
  if ((port->ofp_port.state & OFPPS_LINK_DOWN) != 0 ||
      (port->ofp_port.config & OFPPC_PORT_DOWN) != 0) {
    return false;
  }
  return true;
}

/*
 * port_mod (Agent/DP API)
 */
lagopus_result_t
ofp_port_mod_modify(uint64_t dpid,
                    struct ofp_port_mod *port_mod,
                    struct ofp_error *error) {
  struct bridge *bridge;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return port_config(bridge, port_mod, error);
}

/*
 * port_stats (Agent/DP API)
 */
lagopus_result_t
ofp_port_stats_request_get(uint64_t dpid,
                           struct ofp_port_stats_request *port_stats_request,
                           struct port_stats_list *port_stats_list,
                           struct ofp_error *error) {
  struct bridge *bridge;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return lagopus_get_port_statistics(&bridge->ports,
                                     port_stats_request,
                                     port_stats_list,
                                     error);
}

/*
 * port description (Agent/DP API)
 */
lagopus_result_t
ofp_port_get(uint64_t dpid,
             struct port_desc_list *port_desc_list,
             struct ofp_error *error) {
  struct bridge *bridge;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return get_port_desc(&bridge->ports, port_desc_list, error);
}
