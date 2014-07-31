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
 *	@file	dpmgr.c
 *	@brief	Datapath manager.
 */

#include "lagopus_apis.h"
#include "lagopus/dpmgr.h"
#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "lagopus/bridge.h"

static struct dpmgr *dpmgr_singleton;

/* Generate random datapath id. */
uint64_t
dpmgr_dpid_generate(void) {
  uint64_t dpid;

  dpid = (uint64_t)random();
  dpid <<= 32;
  dpid |= (uint64_t)random();

  return dpid;
}

/* Allocate datapath manager. */
struct dpmgr *
dpmgr_alloc(void) {
  struct dpmgr *dpmgr;

  dpmgr = (struct dpmgr *)calloc(1, sizeof(struct dpmgr));
  if (dpmgr == NULL) {
    return NULL;
  }

  dpmgr->ports = ports_alloc();
  if (dpmgr->ports == NULL) {
    return NULL;
  }

  TAILQ_INIT(&dpmgr->bridge_list);

#if defined(HAVE_DPDK)
  rte_rwlock_init(&dpmgr_lock);
#endif /* HAVE_DPDK */

  init_flow_timer();

  dpmgr_singleton = dpmgr;

  return dpmgr;
}

/* Free datapath manager. */
void
dpmgr_free(struct dpmgr *dpmgr) {
  free(dpmgr);
  dpmgr_singleton = NULL;
}

struct dpmgr *
dpmgr_get_instance(void) {
  return dpmgr_singleton;
}

/* Add physical interface. */
lagopus_result_t
dpmgr_port_add(struct dpmgr *dpmgr, const struct port *port) {
  lagopus_result_t ret;

  ret = port_add(dpmgr->ports, port);

  return ret;
}

lagopus_result_t
dpmgr_port_delete(struct dpmgr *dpmgr, uint32_t port_index) {
  lagopus_result_t ret;

  ret = port_delete(dpmgr->ports, port_index);

  return ret;
}

/* Add bridge to datapath manager. */
lagopus_result_t
dpmgr_bridge_add(struct dpmgr *dpmgr, const char *bridge_name,
                 uint64_t dpid) {
  lagopus_result_t ret;

  ret = bridge_add(&dpmgr->bridge_list, bridge_name, dpid);

  return ret;
}

/* Delete bridge from datapaht manager. */
lagopus_result_t
dpmgr_bridge_delete(struct dpmgr *dpmgr, const char *bridge_name) {
  lagopus_result_t ret;

  ret = bridge_delete(&dpmgr->bridge_list, bridge_name);

  return ret;
}

/* Lookup bridge. */
struct bridge *
dpmgr_bridge_lookup(struct dpmgr *dpmgr, const char *bridge_name) {
  return bridge_lookup(&dpmgr->bridge_list, bridge_name);
}

/* Lookup bridge dpid. */
lagopus_result_t
dpmgr_bridge_dpid_lookup(struct dpmgr *dpmgr,
                         const char *bridge_name,
                         uint64_t *dpidp) {
  struct bridge *bridge;
  lagopus_result_t rv;

  FLOWDB_RWLOCK_RDLOCK(NULL);
  bridge = bridge_lookup(&dpmgr->bridge_list, bridge_name);
  if (bridge != NULL) {
    *dpidp = bridge->dpid;
    rv = LAGOPUS_RESULT_OK;
  } else {
    rv = LAGOPUS_RESULT_NOT_FOUND;
  }
  FLOWDB_RWLOCK_RDUNLOCK(NULL);
  return rv;
}

/* Lookup bridge. */
struct bridge *
dpmgr_bridge_lookup_by_controller_address(struct dpmgr *dpmgr,
    const char *controller_address) {
  return bridge_lookup_by_controller_address(&dpmgr->bridge_list,
         controller_address);
}

/* Assign port to the bridge. */
lagopus_result_t
dpmgr_bridge_port_add(struct dpmgr *dpmgr, const char *bridge_name,
                      uint32_t portid, uint32_t port_no) {
  lagopus_result_t ret;
  struct port *port;

  /* Lookup port from the dpmgr. */
  port = port_lookup(dpmgr->ports, portid);
  if (port == NULL) {
    lagopus_msg_error("Physical port %d is not registered\n", portid);
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  /* Is already part of bridge? */
  if (port->bridge != NULL) {
    lagopus_msg_error("Physical port %d is already assigned\n", portid);
    return LAGOPUS_RESULT_ALREADY_EXISTS;
  }

  port->ofp_port.port_no = port_no;

  /* Add the port to the bridge. */
  ret = bridge_port_add(&dpmgr->bridge_list, bridge_name, port);

  return ret;
}

/* Assign port to the bridge. */
lagopus_result_t
dpmgr_bridge_port_delete(struct dpmgr *dpmgr, const char *bridge_name,
                         uint32_t portid) {
  lagopus_result_t ret;

  /* Delete the port to the bridge. */
  ret = bridge_port_delete(&dpmgr->bridge_list, bridge_name, portid);

  return ret;
}

/* Set bridge dpid. */
lagopus_result_t
dpmgr_bridge_dpid_set(struct dpmgr *dpmgr,
                      const char *bridge_name,
                      uint64_t dpid) {
  struct bridge *bridge;
  lagopus_result_t rv;

  FLOWDB_RWLOCK_WRLOCK(NULL);
  bridge = bridge_lookup(&dpmgr->bridge_list, bridge_name);
  if (bridge != NULL) {
    bridge->dpid = dpid;
    rv = LAGOPUS_RESULT_OK;
  } else {
    rv = LAGOPUS_RESULT_NOT_FOUND;
  }
  FLOWDB_RWLOCK_WRUNLOCK(NULL);
  return rv;
}

/* Set controller. */
lagopus_result_t
dpmgr_controller_add(struct dpmgr *dpmgr, const char *bridge_name,
                     const char *controller_str) {
  struct bridge *bridge;

  bridge = dpmgr_bridge_lookup(dpmgr, bridge_name);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }
  return bridge_controller_add(bridge, controller_str);
}

/* Unset controller. */
lagopus_result_t
dpmgr_controller_delete(struct dpmgr *dpmgr, const char *bridge_name,
                        const char *controller_str) {
  struct bridge *bridge;

  bridge = dpmgr_bridge_lookup(dpmgr, bridge_name);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }
  return bridge_controller_delete(bridge, controller_str);
}

lagopus_result_t
dpmgr_controller_dpid_find(struct dpmgr *dpmgr,
                           const char *controller_str, uint64_t *dpidp) {
  struct bridge *bridge;

  if (dpidp == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  bridge = dpmgr_bridge_lookup_by_controller_address(dpmgr, controller_str);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }
  *dpidp = bridge_dpid(bridge);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dpmgr_switch_mode_get(uint64_t dpid, enum switch_mode *switch_mode) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct dpmgr *dpmgr = NULL;
  struct bridge *bridge = NULL;

  dpmgr = dpmgr_get_instance();
  if (dpmgr != NULL) {
    bridge = bridge_lookup_by_dpid(&dpmgr->bridge_list, dpid);
    if (bridge != NULL) {
      ret = flowdb_switch_mode_get(bridge->flowdb, switch_mode);
    } else {
      lagopus_msg_warning("Not found bridge.\n");
      ret = LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    lagopus_msg_warning("Not found dpmgr.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }

  return ret;
}

lagopus_result_t
dpmgr_switch_mode_set(uint64_t dpid, enum switch_mode switch_mode) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct dpmgr *dpmgr = NULL;
  struct bridge *bridge = NULL;

  dpmgr = dpmgr_get_instance();
  if (dpmgr != NULL) {
    bridge = bridge_lookup_by_dpid(&dpmgr->bridge_list, dpid);
    if (bridge != NULL) {
      ret = flowdb_switch_mode_set(bridge->flowdb, switch_mode);
    } else {
      lagopus_msg_warning("Not found bridge.\n");
      ret = LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    lagopus_msg_warning("Not found dpmgr.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }

  return ret;
}

lagopus_result_t
dpmgr_bridge_fail_mode_get(uint64_t dpid,
                           enum fail_mode *fail_mode) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct dpmgr *dpmgr = NULL;
  struct bridge *bridge = NULL;

  dpmgr = dpmgr_get_instance();
  if (dpmgr != NULL) {
    bridge = bridge_lookup_by_dpid(&dpmgr->bridge_list, dpid);
    if (bridge != NULL) {
      ret = bridge_fail_mode_get(bridge, fail_mode);
    } else {
      lagopus_msg_warning("Not found bridge.\n");
      ret = LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    lagopus_msg_warning("Not found dpmgr.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }

  return ret;
}

lagopus_result_t
dpmgr_bridge_ofp_version_get(uint64_t dpid, uint8_t *version) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct dpmgr *dpmgr = NULL;
  struct bridge *bridge = NULL;

  dpmgr = dpmgr_get_instance();
  if (dpmgr != NULL) {
    bridge = bridge_lookup_by_dpid(&dpmgr->bridge_list, dpid);
    if (bridge != NULL) {
      ret = bridge_ofp_version_get(bridge, version);
    } else {
      lagopus_msg_warning("Not found bridge.\n");
      ret = LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    lagopus_msg_warning("Not found dpmgr.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }

  return ret;
}

lagopus_result_t
dpmgr_bridge_ofp_version_set(uint64_t dpid, uint8_t version) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct dpmgr *dpmgr = NULL;
  struct bridge *bridge = NULL;

  dpmgr = dpmgr_get_instance();
  if (dpmgr != NULL) {
    bridge = bridge_lookup_by_dpid(&dpmgr->bridge_list, dpid);
    if (bridge != NULL) {
      ret = bridge_ofp_version_set(bridge, version);
    } else {
      lagopus_msg_warning("Not found bridge.\n");
      ret = LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    lagopus_msg_warning("Not found dpmgr.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }

  return ret;
}

lagopus_result_t
dpmgr_bridge_ofp_version_bitmap_get(uint64_t dpid,
                                    uint32_t *version_bitmap) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct dpmgr *dpmgr = NULL;
  struct bridge *bridge = NULL;

  dpmgr = dpmgr_get_instance();
  if (dpmgr != NULL) {
    bridge = bridge_lookup_by_dpid(&dpmgr->bridge_list, dpid);
    if (bridge != NULL) {
      ret = bridge_ofp_version_bitmap_get(bridge, version_bitmap);
    } else {
      lagopus_msg_warning("Not found bridge.\n");
      ret = LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    lagopus_msg_warning("Not found dpmgr.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }

  return ret;
}

lagopus_result_t
dpmgr_bridge_ofp_version_bitmap_set(uint64_t dpid, uint8_t version) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct dpmgr *dpmgr = NULL;
  struct bridge *bridge = NULL;

  dpmgr = dpmgr_get_instance();
  if (dpmgr != NULL) {
    bridge = bridge_lookup_by_dpid(&dpmgr->bridge_list, dpid);
    if (bridge != NULL) {
      ret = bridge_ofp_version_bitmap_set(bridge, version);
    } else {
      lagopus_msg_warning("Not found bridge.\n");
      ret = LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    lagopus_msg_warning("Not found dpmgr.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }

  return ret;
}

lagopus_result_t
dpmgr_bridge_ofp_features_get(uint64_t dpid,
                              struct ofp_switch_features *features) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct dpmgr *dpmgr = NULL;
  struct bridge *bridge = NULL;

  dpmgr = dpmgr_get_instance();
  if (dpmgr != NULL) {
    bridge = bridge_lookup_by_dpid(&dpmgr->bridge_list, dpid);
    if (bridge != NULL) {
      ret = bridge_ofp_features_get(bridge, features);
    } else {
      lagopus_msg_warning("Not found bridge.\n");
      ret = LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    lagopus_msg_warning("Not found dpmgr.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }

  return ret;
}
