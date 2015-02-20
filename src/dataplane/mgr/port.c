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
 *      @file   port.c
 *      @brief  OpenFlow Port management.
 */

#include "lagopus_apis.h"
#include <openflow.h>

#include "lagopus/dpmgr.h"
#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
#include "lagopus/vector.h"
#include "lagopus/ofp_dp_apis.h"

struct vector *
ports_alloc(void) {
  struct vector *v;

  v = vector_alloc();
  if (v == NULL) {
    return NULL;
  }

  return v;
}

void
ports_free(struct vector *v) {
  vector_free(v);
}

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

static struct port *
port_alloc(void) {
  struct port *port;

  port = calloc(1, sizeof(struct port));
  if (port == NULL) {
    return NULL;
  }
  port->stats = port_stats;
  clock_gettime(CLOCK_MONOTONIC, &port->create_time);

  return port;
}

static void
port_free(struct port *port) {
  free(port);
}

/**
 * lookup port structure from specified vector
 *
 * @param[in] v       vector associated with bridge or dpmgr
 * @param[in] id      OpenFlow Port number or Physical Index number
 * @retval    ==NULL  port is not configured
 * @retval    !=NULL  port structure
 */
struct port *
port_lookup(struct vector *v, uint32_t id) {
  if (id >= v->allocated) {
    return NULL;
  }
  return (struct port *)v->index[id];
}

/**
 * Add port to dpmgr vector.
 *
 * @param[in]   v               Vector owned by dpmgr.
 * @param[in]   port_param      Port description.
 */
lagopus_result_t
port_add(struct vector *v, const struct port *port_param) {
  int i;
  struct port *find;
  struct port *port;
  lagopus_result_t rv;

  find = port_lookup(v, port_param->ifindex);
  if (find != NULL) {
    return LAGOPUS_RESULT_ALREADY_EXISTS;
  }

  port = port_alloc();
  if (port == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  port->type = port_param->type;
  port->ofp_port = port_param->ofp_port;
  port->ifindex = port_param->ifindex;

  rv = lagopus_configure_physical_port(port);
  if (rv != LAGOPUS_RESULT_OK) {
    port_free(port);
    return rv;
  }

  printf("Adding Physical Port %u", port->ifindex);
  for (i = 0; i < 6; i++) {
    printf(":%02x", port->ofp_port.hw_addr[i]);
  }
  printf("\n");
  vector_set_index(v, port->ifindex, port);

  return LAGOPUS_RESULT_OK;
}

/**
 * Delete port to dpmgr vector.
 *
 * @param[in]   v               Vector owned by dpmgr.
 * @param[in]   port_index      Physical port ifindex.
 */
lagopus_result_t
port_delete(struct vector *v, uint32_t ifindex) {
  struct port *port;

  port = port_lookup(v, ifindex);
  if (port == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }
  if (port->bridge != NULL) {
    /* assigned port to bridge, don't delete. */
    return LAGOPUS_RESULT_ANY_FAILURES;
  }
  vector_set_index(v, ifindex, NULL);
  lagopus_unconfigure_physical_port(port);
  port_free(port);

  return LAGOPUS_RESULT_OK;
}

/**
 * get port structure by physical ifindex
 *
 * @param[in]   vector owned by bridge
 * @param[in]   ifindex interface index
 * @retval      !=NULL  lagopus port object
 *              ==NULL  lagopus port is not configured
 */
struct port *
ifindex2port(struct vector *v, uint32_t ifindex) {
  unsigned int id;

  for (id = 0; id < v->allocated; id++) {
    struct port *port;

    port = v->index[id];
    if (port != NULL && port->ifindex == ifindex) {
      return port;
    }
  }
  return NULL;
}

/**
 * get port structure by OpenFlow flow number.
 *
 * @param[in]   vector owned by dpmgr
 * @param[in]   port_no OpenFlow Port number.
 * @retval      !=NULL  lagopus port object
 *              ==NULL  lagopus port is not configured
 */
struct port *
port_lookup_number(struct vector *v, uint32_t port_no) {
  unsigned int id;

  for (id = 0; id < v->allocated; id++) {
    struct port *port;

    port = v->index[id];
    if (port != NULL && port->ofp_port.port_no == port_no) {
      return port;
    }
  }
  return NULL;
}

unsigned int
num_ports(struct vector *v) {
  unsigned int id, count;

  count = 0;
  for (id = 0; id < v->allocated; id++) {
    struct port *port;

    port = v->index[id];
    if (port != NULL) {
      count++;
    }
  }
  return count;
}

lagopus_result_t
lagopus_get_port_statistics(struct vector *ports,
                            struct ofp_port_stats_request *request,
                            struct port_stats_list *list,
                            struct ofp_error *error) {
  struct port *port;
  struct port_stats *stats;
  vindex_t i, max;

  max = ports->allocated;
  if (request->port_no == OFPP_ANY) {
    for (i = 0; i < max; i++) {
      port = vector_slot(ports, i);
      if (port == NULL) {
        continue;
      }
      /* XXX read lock */
      stats = port->stats(port);
      /* XXX unlock */
      if (stats == NULL) {
        continue;
      }
      TAILQ_INSERT_TAIL(list, stats, entry);
    }
  } else {
    for (i = 0; i < max; i++) {
      port = vector_slot(ports, i);
      if (port == NULL) {
        continue;
      }
      if (port->ofp_port.port_no == request->port_no) {
        break;
      }
    }
    if (i == max) {
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_PORT);
      return LAGOPUS_RESULT_OFP_ERROR;
    }
    /* XXX read lock */
    stats = port->stats(port);
    /* XXX unlock */
    if (stats == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    TAILQ_INSERT_TAIL(list, stats, entry);
  }

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
    port = port_lookup(bridge->ports, port_mod->port_no);
    if (port == NULL) {
      ofp_error_set(error, OFPET_PORT_MOD_FAILED, OFPPMFC_BAD_PORT);
      return LAGOPUS_RESULT_OFP_ERROR;
    }
    ofp_port = &port->ofp_port;
    if (memcmp(ofp_port->hw_addr, port_mod->hw_addr, OFP_ETH_ALEN) != 0) {
      ofp_error_set(error, OFPET_PORT_MOD_FAILED, OFPPMFC_BAD_HW_ADDR);
      return LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    port = NULL; /* XXX shut out warning */
    ofp_port = &bridge->controller_port;
  }
  /* XXX write lock for thread safe */
  if ((port_mod->config & (uint32_t)~port_mod->mask) != 0) {
    ofp_error_set(error, OFPET_PORT_MOD_FAILED, OFPPMFC_BAD_CONFIG);
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

lagopus_result_t
get_port_desc(struct vector *ports,
              struct port_desc_list *list,
              struct ofp_error *error) {
  struct port *port;
  struct port_desc *desc;
  vindex_t i, max;

  (void) error;

  max = ports->allocated;
  for (i = 0; i < max; i++) {
    port = vector_slot(ports, i);
    if (port == NULL) {
      continue;
    }
    /**/
    desc = calloc(1, sizeof(struct port_desc));
    if (desc == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    memcpy(&desc->ofp, &port->ofp_port, sizeof(struct ofp_port));
    TAILQ_INSERT_TAIL(list, desc, entry);
  }

  return LAGOPUS_RESULT_OK;
}

bool
port_liveness(struct bridge *bridge, uint32_t port_no) {
  struct port *port;

  if (port_no == OFPP_ANY) {
    return false;
  }
  port = port_lookup(bridge->ports, port_no);
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
  struct dpmgr *dpmgr;
  struct bridge *bridge;

  dpmgr = dpmgr_get_instance();
  bridge = bridge_lookup_by_dpid(&dpmgr->bridge_list, dpid);
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
  struct dpmgr *dpmgr;
  struct bridge *bridge;

  dpmgr = dpmgr_get_instance();
  bridge = bridge_lookup_by_dpid(&dpmgr->bridge_list, dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return lagopus_get_port_statistics(bridge->ports,
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
  struct dpmgr *dpmgr;
  struct bridge *bridge;

  dpmgr = dpmgr_get_instance();
  bridge = bridge_lookup_by_dpid(&dpmgr->bridge_list, dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return get_port_desc(bridge->ports, port_desc_list, error);
}
