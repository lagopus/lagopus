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
 *      @file   lagopus/bridge.h
 *      @brief  "Bridge" as OpenFlow logical switch APIs.
 */

#ifndef SRC_INCLUDE_LAGOPUS_BRIDGE_H_
#define SRC_INCLUDE_LAGOPUS_BRIDGE_H_

#include "lagopus_apis.h"
#include "openflow.h"
#include "lagopus/flowdb.h"
#ifdef HYBRID
#include "lagopus/mactable.h"
#include "lagopus/rib.h"
#endif /* HYBRID */

struct port;

/* Tepmorary inherit OFP_MAX_PORT_NAME_LEN */
#define BRIDGE_MAX_NAME_LEN                16

/* Max number of buffers. */
#define BRIDGE_N_BUFFERS_MAX            65535

/* Max number of tables. */
#define BRIDGE_N_TABLES_MAX               255

/* Set default values to max. */
#define BRIDGE_N_BUFFERS_DEFAULT BRIDGE_N_BUFFERS_MAX
#define BRIDGE_N_TABLES_DEFAULT BRIDGE_N_TABLES_MAX

/**
 * @brief Fail mode of the openflow bridge.
 */
enum fail_mode {
  FAIL_SECURE_MODE = 0,
  FAIL_STANDALONE_MODE = 1
};

struct dp_bridge_iter;
typedef struct dp_bridge_iter *dp_bridge_iter_t;

/**
 * @brief Bridge internal object.
 */
struct bridge {
  TAILQ_ENTRY(bridge) entry;            /** Linked list. */
  char name[BRIDGE_MAX_NAME_LEN];       /** Bridge name. */
  uint64_t dpid;                        /** Datapath ID. */
  enum fail_mode fail_mode;             /** Lost connection behavior. */
  bool running;                         /** Running status. */
  uint8_t version;                      /** Wire protocol version. */
  uint32_t version_bitmap;              /** Wire protocol version bitmap. */
  struct ofp_switch_features features;  /** OpenFlow features. */
  lagopus_hashmap_t ports;              /** Ports. */
  struct flowdb *flowdb;                /** Flow database. */
  struct group_table *group_table;      /** Group table. */
  struct meter_table *meter_table;      /** Meter table. */
#ifdef HYBRID
  struct bridge **updater_timer;         /**< Timer for updater. */
  struct mactable mactable;             /** Mac learning table. */
  struct rib rib;
#endif /* HYBRID */
  struct ofp_port controller_port;      /** Controller port config. */
  struct ofp_switch_config switch_config;  /** Switch config. */
  bool l2_bridge;                       /** L2 bridge enable */
};

#ifdef HYBRID
struct mactable_args {
  struct macentry *entries;
  unsigned int num;
  unsigned int no;
};
#endif /* HYBRID */

/**
 * Allocate bridge object.
 *
 * @param[in]  bridge   Name of bridge.
 *
 * @retval      !=NULL  Bridge object.
 * @retval      ==NULL  Memory exhausted.
 */
struct bridge *
bridge_alloc(const char *name);

/**
 * Free bridge object.
 *
 * @param[in]   bridge  Bridge.
 */
void
bridge_free(struct bridge *bridge);

/**
 * Count number of ports assigned for the bridge.
 *
 * @param[in]   bridge  Bridge.
 *
 * @retval      Number of ports.
 */
uint32_t dp_bridge_port_count(const char *name);

#ifdef HYBRID
/* mactable */
/**
 * Set ageing time of MAC address table.
 * @param[in]  bridge       Bridge.
 * @param[in]  ageing_time  Ageing time of MAC address.
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
bridge_mactable_ageing_time_set(struct bridge *bridge, uint32_t ageing_time);

/**
 * Set number of max entries of MAC address table.
 * @param[in]  bridge       Bridge.
 * @param[in]  max_entries  Number of max entries of MAC address.
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
bridge_mactable_max_entries_set(struct bridge *bridge, uint16_t max_entries);

/**
 * Set the MAC address entry to MAC address table.
 * @param[in]  bridge   Bridge.
 * @param[in]  ethaddr  MAC address.
 * @param[in]  portid   Port number that corresponding to MAC address.
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_INVALID_ARGS     Arguments are invalid.
 * @retval      LAGOPUS_RESULT_NO_MEMORY        Memory exhausted.
 */
lagopus_result_t
bridge_mactable_entry_set(struct bridge *bridge, const uint8_t ethaddr[], uint32_t portid);

/**
 * Update the MAC address entry to MAC address table.
 * @param[in]  bridge   Bridge.
 * @param[in]  ethaddr  MAC address.
 * @param[in]  portid   Port number that corresponding to MAC address.
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_INVALID_ARGS     Arguments are invalid.
 * @retval      LAGOPUS_RESULT_NO_MEMORY        Memory exhausted.
 */
lagopus_result_t
bridge_mactable_entry_modify(struct bridge *bridge, const uint8_t ethaddr[], uint32_t portid);

/**
 * Delete the MAC address entry from MAC address table.
 * @param[in]  bridge   Bridge.
 * @param[in]  ethaddr  MAC address.
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
bridge_mactable_entry_delete(struct bridge *bridge, const uint8_t ethaddr[]);

/**
 * Clear all MAC address entries from MAC address table.
 * @param[in]  bridge   Bridge.
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
bridge_mactable_entry_clear(struct bridge *bridge);

/**
 * Get ageing time of MAC address table.
 * @param[in]  bridge   Bridge.
 * @param[out] ageing_time Ageing time of MAC address table.
 * @retval     LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
bridge_mactable_ageing_time_get(struct bridge *bridge, uint32_t *ageing_time);

/**
 * Get max entries in MAC address table.
 * @param[in]  bridge   Bridge.
 * @param[out] max_entries max entries list.
 * @retval     LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
bridge_mactable_max_entries_get(struct bridge *bridge, uint32_t *max_entries);

/**
 * Get number of MAC addres table entries.
 * @param[in]  bridge   Bridge.
 * @retval     number    Number of entries.
 * @retval     LAGOPUS_RESULT_NOT_OPERATIONAL Failed, not operational.
 * @retval     LAGOPUS_RESULT_INVALID_ARGS  fail_mode value is wrong.
 */
unsigned int
bridge_mactable_num_entries_get(struct bridge *bridge);

/**
 * Get entries.
 * @param[in]  bridge   Bridge.
 * @param[in]  entries  Entries list.
 * @param[in]  num      Number of entries.
 * @retval     LAGOPUS_RESULT_OK               Succeeded.
 * @retval     LAGOPUS_RESULT_NOT_FOUND Bridge is not exist.
 */
lagopus_result_t
bridge_mactable_entries_get(struct bridge *bridge, struct macentry *entries, unsigned int num);
#endif /* HYBRID */

lagopus_result_t
dp_bridge_table_id_iter_create(const char *name, dp_bridge_iter_t *iterp);
lagopus_result_t
dp_bridge_table_id_iter_get(dp_bridge_iter_t iter, uint8_t *idp);
void
dp_bridge_table_id_iter_destroy(dp_bridge_iter_t iter);

lagopus_result_t
dp_bridge_flow_iter_create(const char *name, uint8_t table_id,
                           dp_bridge_iter_t *iterp);
lagopus_result_t
dp_bridge_flow_iter_get(dp_bridge_iter_t iter, struct flow **flowp);
void
dp_bridge_flow_iter_destroy(dp_bridge_iter_t iter);

#endif /* SRC_INCLUDE_LAGOPUS_BRIDGE_H_ */
