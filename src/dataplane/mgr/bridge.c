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
 *      @file   bridge.c
 *      @brief  Bridge (a.k.a. OpenFlow Switch) management.
 */

#include "lagopus_config.h"

#include "openflow.h"
#include "lagopus/bridge.h"
#include "lagopus/port.h"
#include "lagopus/flowdb.h"
#include "lagopus/group.h"
#include "lagopus/meter.h"
#ifdef HYBRID
#include "lagopus/mactable.h"
#include "lagopus/rib.h"
#endif /* HYBRID */

#include "lagopus/dp_apis.h"

#define SET32_FLAG(V, F)        (V) = (V) | (uint32_t)(F)
#define UNSET32_FLAG(V, F)      (V) = (V) & (uint32_t)~(F)

/**
 * Get OpenFlow switch fail mode.
 *
 * @param[in]   bridge  Bridge.
 * @param[out]  fail_mode       Bridge fail mode.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 */
static lagopus_result_t
bridge_fail_mode_get(struct bridge *bridge,
                     enum fail_mode *fail_mode) {
  *fail_mode = bridge->fail_mode;
  return LAGOPUS_RESULT_OK;
}

/**
 * Set OpenFlow switch fail mode.
 *
 * @param[in]   bridge  Bridge.
 * @param[in]   fail_mode       Bridge fail mode.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS  fail_mode value is wrong.
 */
static lagopus_result_t
bridge_fail_mode_set(struct bridge *bridge,
                     enum fail_mode fail_mode) {
  if (fail_mode != FAIL_SECURE_MODE && fail_mode != FAIL_STANDALONE_MODE) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  bridge->fail_mode = fail_mode;

  return LAGOPUS_RESULT_OK;
}

/**
 * Get primary OpenFlow version.
 *
 * @param[in]   bridge  Bridge.
 * @param[out]  version Wire protocol version.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 */
static lagopus_result_t
bridge_ofp_version_get(struct bridge *bridge, uint8_t *version) {
  *version = bridge->version;
  return LAGOPUS_RESULT_OK;
}

/**
 * Set primary OpenFlow version.
 *
 * @param[in]   bridge  Bridge.
 * @param[in]   version Wire protocol version.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS  Version number is wrong.
 */
static lagopus_result_t
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

/**
 * Get supported OpenFlow version bitmap.
 *
 * @param[in]   bridge  Bridge.
 * @param[out]  version_bitmap  Support version bitmap.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 */
static lagopus_result_t
bridge_ofp_version_bitmap_get(struct bridge *bridge,
                              uint32_t *version_bitmap) {
  *version_bitmap = bridge->version_bitmap;
  return LAGOPUS_RESULT_OK;
}

/**
 * Set supported OpenFlow version bitmap.
 *
 * @param[in]   bridge  Bridge.
 * @param[in]   version Wire protocol version to be set to version bitmap.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS  Version number is wrong.
 */
static lagopus_result_t
bridge_ofp_version_bitmap_set(struct bridge *bridge, uint8_t version) {
  if (version != OPENFLOW_VERSION_1_3 && version != OPENFLOW_VERSION_1_4) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  SET32_FLAG(bridge->version_bitmap, (1 << version));
  return LAGOPUS_RESULT_OK;
}

/**
 * Unset supported OpenFlow version bitmap.
 *
 * @param[in]   bridge  Bridge.
 * @param[in]   version Wire protocol version to be unset from version bitmap.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS  Version number is wrong.
 */
static lagopus_result_t
bridge_ofp_version_bitmap_unset(struct bridge *bridge, uint8_t version) {
  if (version != OPENFLOW_VERSION_1_3 && version != OPENFLOW_VERSION_1_4) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  UNSET32_FLAG(bridge->version_bitmap, (1 << version));
  return LAGOPUS_RESULT_OK;
}

/**
 * Get OpenFlow switch features.
 *
 * @param[in]   bridge                  Bridge.
 * @param[out]  features                Pointer of features.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 */
static lagopus_result_t
bridge_ofp_features_get(struct bridge *bridge,
                        struct ofp_switch_features *features) {
  *features = bridge->features;
  return LAGOPUS_RESULT_OK;
}

/**
 * Allocate a new bridge.
 */
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

  /* Prepare bridge's port hashmap. */
  lagopus_hashmap_create(&bridge->ports, LAGOPUS_HASHMAP_TYPE_ONE_WORD, NULL);
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


#ifdef HYBRID
  if (mactable_init(&bridge->mactable) != LAGOPUS_RESULT_OK) {
    goto out;
  }
  if (rib_init(&bridge->rib) != LAGOPUS_RESULT_OK) {
    goto out;
  }

  add_updater_timer(bridge, UPDATER_TABLE_UPDATE_TIME);
#endif /* HYBRID */

  /* Return bridge. */
  return bridge;

out:
  bridge_free(bridge);
  return NULL;
}

static bool
bridge_do_ports_free_iterate(void *key, void *val,
                             lagopus_hashentry_t he, void *arg) {
  struct port *port;

  (void) key;
  (void) val;
  (void) he;
  (void) arg;

  port = val;
  port->bridge = NULL;
  return true;
}

/**
 *Bridge free.
 */
void
bridge_free(struct bridge *bridge) {
  if (bridge->ports != NULL) {

    lagopus_hashmap_iterate(&bridge->ports,
                            bridge_do_ports_free_iterate,
                            NULL);
    lagopus_hashmap_destroy(&bridge->ports, false);
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
#ifdef HYBRID
  /* stop timer. */
  if (bridge->updater_timer != NULL) {
    *bridge->updater_timer = NULL;
  }
  mactable_fini(&bridge->mactable);
  rib_fini(&bridge->rib);
#endif /* HYBRID */
  free(bridge);
}

#ifdef HYBRID
/**
 * Set ageing time of the mac table.
 */
lagopus_result_t
bridge_mactable_ageing_time_set(struct bridge *bridge, uint32_t ageing_time) {
  mactable_ageing_time_set(&bridge->mactable, ageing_time);

  return LAGOPUS_RESULT_OK;
}

/**
 * Set max entries of the mac table.
 */
lagopus_result_t
bridge_mactable_max_entries_set(struct bridge *bridge, uint16_t max_entries) {
  mactable_max_entries_set(&bridge->mactable, max_entries);

  return LAGOPUS_RESULT_OK;
}

/**
 * Update entry in the mac table.
 */
lagopus_result_t
bridge_mactable_entry_set(struct bridge *bridge,
                          const uint8_t ethaddr[],
                          uint32_t portid) {
  return mactable_entry_update(&bridge->mactable, ethaddr, portid);
}

/**
 * Modify entry in the mac table.
 */
lagopus_result_t
bridge_mactable_entry_modify(struct bridge *bridge,
                             const uint8_t ethaddr[],
                             uint32_t portid) {
  return mactable_entry_update(&bridge->mactable, ethaddr, portid);
}

/**
 * Delete entry in the mac table.
 */
lagopus_result_t
bridge_mactable_entry_delete(struct bridge *bridge, const uint8_t ethaddr[]) {
  return mactable_entry_delete(&bridge->mactable, ethaddr);
}

/**
 * Clear all entries in the mac table.
 */
lagopus_result_t
bridge_mactable_entry_clear(struct bridge *bridge) {
  return mactable_entry_clear(&bridge->mactable);
}

/* for debug */
static lagopus_result_t
mac_addr_to_str(uint64_t mac, char *macstr) {
  int i, j;
  uint8_t str[6];
  for (i = 6 - 1, j = 0; i >= 0; i--, j++)
    str[j] = 0xff & mac >> (8 * i);
  sprintf(macstr, "%02x:%02x:%02x:%02x:%02x:%02x",
          str[5], str[4], str[3], str[2], str[1], str[0]);
}

static bool
get_macentry(void *key, void *val, lagopus_hashentry_t he, void *arg) {
  bool result = false;
  char macstr[18];
  struct mactable_args *ma = (struct mactable_args *)arg;
  struct macentry *entries = (struct macentry *)ma->entries;
  struct macentry *entry = (struct macentry *)val;
  (void) he;
  (void) key;

  if (entry != NULL && (val != NULL || ma->no < ma->num)) {
    entries[ma->no].inteth = entry->inteth;
    entries[ma->no].portid = entry->portid;
    entries[ma->no].update_time = entry->update_time;
    entries[ma->no].address_type = entry->address_type;
    ma->no++;
    result = true;
  } else {
    result = false;
  }

  return result;
}

lagopus_result_t
bridge_mactable_entries_get(struct bridge *bridge,
                            struct macentry *entries,
                            unsigned int num) {
  int cnt; // debug
  char macstr[18]; //debug

  lagopus_result_t result;
  struct mactable_args ma;
  struct mactable *mactable = &bridge->mactable;
  lagopus_hashmap_t *hashmap;
  hashmap = &mactable->hashmap[__sync_add_and_fetch(&mactable->read_table, 0)];

  ma.entries = entries;
  ma.num = num;
  ma.no = 0;
  result = lagopus_hashmap_iterate(hashmap, get_macentry, &ma);

  if (result == LAGOPUS_RESULT_OK) {
  } else {
    lagopus_perror(result);
  }

  for (cnt = 0; cnt < num; cnt++) {
    mac_addr_to_str(entries[cnt].inteth, macstr);
    lagopus_msg_info("inteth: %s\n", macstr);
    lagopus_msg_info("port: %"PRIu32"\n", entries[cnt].portid);
    lagopus_msg_info("address type: %s\n",
                     (entries[cnt].address_type == MACTABLE_SETTYPE_STATIC ?
                      "static" : "dynamic"));
    lagopus_msg_info("update time: %"PRIu32"\n",
                     (uint32_t)(entries[cnt].update_time.tv_sec));
  }

  return result;
}

unsigned int
bridge_mactable_num_entries_get(struct bridge *bridge) {
  struct mactable *mactable = &bridge->mactable;
  lagopus_hashmap_t *hashmap;
  hashmap = &mactable->hashmap[__sync_add_and_fetch(&mactable->read_table, 0)];

  return (unsigned int)lagopus_hashmap_size(hashmap);
}

lagopus_result_t
bridge_mactable_ageing_time_get(struct bridge *bridge, uint32_t *ageing_time) {
  *ageing_time = mactable_ageing_time_get(&bridge->mactable);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
bridge_mactable_max_entries_get(struct bridge *bridge, uint32_t *max_entries) {
  *max_entries = mactable_max_entries_get(&bridge->mactable);
  return LAGOPUS_RESULT_OK;
}
#endif /* HYBRID */

/**
 * Set switch configuration.
 */
lagopus_result_t
ofp_switch_config_set(uint64_t dpid,
                      struct ofp_switch_config *switch_config,
                      struct ofp_error *error) {
  struct bridge *bridge;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }
  /* Reassemble fragmented packet is not supported */
  if ((switch_config->flags & OFPC_FRAG_REASM) != 0) {
    error->type = OFPET_SWITCH_CONFIG_FAILED;
    error->code = OFPSCFC_BAD_FLAGS;
    lagopus_msg_info("switch config: reassemble is not supported (%d:%d)",
                     error->type, error->code);
    return LAGOPUS_RESULT_OFP_ERROR;
  }
  if (switch_config->miss_send_len < 64) {
    error->type = OFPET_SWITCH_CONFIG_FAILED;
    error->code = OFPSCFC_BAD_LEN;
    lagopus_msg_info("switch config: %d: too short length (%d:%d)",
                     switch_config->miss_send_len, error->type, error->code);
    return LAGOPUS_RESULT_OFP_ERROR;
  }
  bridge->switch_config = *switch_config;

  return LAGOPUS_RESULT_OK;
}

/*
 * Get switch configuration. (Agent/DP API)
 */
lagopus_result_t
ofp_switch_config_get(uint64_t dpid,
                      struct ofp_switch_config *switch_config,
                      struct ofp_error *error) {
  struct bridge *bridge;

  (void) error;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  if (bridge == NULL || switch_config == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  *switch_config = bridge->switch_config;

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
ofp_switch_mode_get(uint64_t dpid, enum switch_mode *switch_mode) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = flowdb_switch_mode_get(bridge->flowdb, switch_mode);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}

lagopus_result_t
ofp_switch_mode_set(uint64_t dpid, enum switch_mode switch_mode) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = flowdb_switch_mode_set(bridge->flowdb, switch_mode);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}

lagopus_result_t
ofp_switch_fail_mode_get(uint64_t dpid,
                         enum fail_mode *fail_mode) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = bridge_fail_mode_get(bridge, fail_mode);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}

lagopus_result_t
ofp_switch_version_get(uint64_t dpid, uint8_t *version) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = bridge_ofp_version_get(bridge, version);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}

lagopus_result_t
ofp_switch_version_set(uint64_t dpid, uint8_t version) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = bridge_ofp_version_set(bridge, version);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}

lagopus_result_t
ofp_switch_version_bitmap_get(uint64_t dpid,
                              uint32_t *version_bitmap) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = bridge_ofp_version_bitmap_get(bridge, version_bitmap);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}

lagopus_result_t
ofp_switch_version_bitmap_set(uint64_t dpid, uint8_t version) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = bridge_ofp_version_bitmap_set(bridge, version);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}

lagopus_result_t
ofp_switch_features_get(uint64_t dpid,
                        struct ofp_switch_features *features) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge *bridge = NULL;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge != NULL) {
    ret = bridge_ofp_features_get(bridge, features);
  } else {
    lagopus_msg_warning("Not found bridge.\n");
    ret = LAGOPUS_RESULT_NOT_FOUND;
  }
  return ret;
}
