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
 * @file	bridge.h
 * @brief	"Bridge" as OpenFlow logical switch APIs.
 */

#ifndef __LAGOPUS_BRIDGE_H__
#define __LAGOPUS_BRIDGE_H__

#include "lagopus_apis.h"
#include "openflow.h"
#include "lagopus/flowdb.h"

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

/* Fail mode of the openflow bridge. */
enum fail_mode {
  FAIL_SECURE_MODE = 0,
  FAIL_STANDALONE_MODE = 1
};

/* Bridge list. */
TAILQ_HEAD(bridge_list, bridge);

struct bridge {
  /* Linked list. */
  TAILQ_ENTRY(bridge) entry;

  /* Bridge name. */
  char name[BRIDGE_MAX_NAME_LEN];

  /* Datapath ID. */
  uint64_t dpid;

  /* Lost connection behavior. */
  enum fail_mode fail_mode;

  /* Wire protocol version. */
  uint8_t version;

  /* Wire protocol version bitmap. */
  uint32_t version_bitmap;

  /* OpenFlow features. */
  struct ofp_switch_features features;

  /* Ports. */
  struct vector *ports;

  /* FlowDB. */
  struct flowdb *flowdb;

  /* Group table. */
  struct group_table *group_table;

  /* Meter table. */
  struct meter_table *meter_table;

  /* Controller port config. */
  struct ofp_port controller_port;

  /* Switch config. */
  struct ofp_switch_config switch_config;

  /* Controller address. */
  const char *controller_address;
};

/**
 * Create and add bridge.
 *
 * @param[in]	bridge_list	Bridge list.
 * @param[in]	name		Bridge name.
 * @param[in]	dpid		Datapath id associated with the bridge.
 *
 * @retval	LAGOPUS_RESULT_OK		Succeeded.
 * @retval	LAGOPUS_RESULT_ALREADY_EXISTS	Already exist same name bridge.
 * @retval	LAGOPUS_RESULT_NO_MEMORY	Memory exhausted.
 */
lagopus_result_t
bridge_add(struct bridge_list *bridge_list, const char *name, uint64_t dpid);

/**
 * Delete bridge.
 *
 * @param[in]	bridge_list	Bridge list.
 * @param[in]	name		Bridge name.
 *
 * @retval	LAGOPUS_RESULT_OK		Succeeded.
 * @retval	LAGOPUS_RESULT_NOT_FOUND	Bridge is not exist.
 */
lagopus_result_t
bridge_delete(struct bridge_list *bridge_list, const char *name);

/**
 * Free bridge object.
 *
 * @param[in]	bridge	Bridge.
 */
void
bridge_free(struct bridge *bridge);

/**
 * Lookup bridge.
 *
 * @param[in]	bridge_list	Bridge list.
 * @param[in]	name		Bridge name.
 *
 * @retval	!=NULL		Bridge object.
 * @retval	==NULL		Bridge is not exist.
 */
struct bridge *
bridge_lookup(struct bridge_list *bridge_list, const char *name);

/**
 * Lookup bridge by datapath id.
 *
 * @param[in]	bridge_list	Bridge list.
 * @param[in]	dpid		Datapath id.
 *
 * @retval	!=NULL		Bridge object.
 * @retval	==NULL		Bridge is not exist.
 */
struct bridge *
bridge_lookup_by_dpid(struct bridge_list *bridge_list, uint64_t dpid);

/**
 * Add port to the bridge.
 *
 * @param[in]	bridge_list	Bridge list.
 * @param[in]	name		Bridge name.
 * @param[in]	port		Port.
 *
 * @retval	LAGOPUS_RESULT_OK		Succeeded.
 * @retval	LAGOPUS_RESULT_NOT_FOUND	Bridge is not exist.
 */
lagopus_result_t
bridge_port_add(struct bridge_list *bridge_list, const char *name,
                struct port *port);

/**
 * Delete port to the bridge.
 *
 * @param[in]	bridge_list	Bridge list.
 * @param[in]	name		Bridge name.
 * @param[in]	port_no		OpenFlow Port number.
 *
 * @retval	LAGOPUS_RESULT_OK		Succeeded.
 * @retval	LAGOPUS_RESULT_NOT_FOUND	Bridge is not exist.
 */
lagopus_result_t
bridge_port_delete(struct bridge_list *bridge_list, const char *name,
                   uint32_t port_no);

/**
 * Get OpenFlow switch fail mode.
 *
 * @param[in]	bridge	Bridge.
 * @param[out]  fail_mode	Bridge fail mode.
 *
 * @retval LAGOPUS_RESULT_OK		Succeeded.
 */
lagopus_result_t
bridge_fail_mode_get(struct bridge *bridge, enum fail_mode *fail_mode);

/**
 * Get OpenFlow switch fail mode.
 *
 * @param[in]	bridge	Bridge.
 * @param[in]	fail_mode	Bridge fail mode.
 *
 * @retval LAGOPUS_RESULT_OK		Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS	fail_mode value is wrong.
 */
lagopus_result_t
bridge_fail_mode_set(struct bridge *bridge, enum fail_mode fail_mode);

/**
 * Get OpenFlow switch features.
 *
 * @param[in]	bridge	Bridge.
 * @param[out]  features	ofp_switch_features structure.
 *
 * @retval LAGOPUS_RESULT_OK		Succeeded.
 */
lagopus_result_t
bridge_ofp_features_get(struct bridge *bridge,
                        struct ofp_switch_features *features);

/**
 * Get primary OpenFlow version.
 *
 * @param[in]	bridge	Bridge.
 * @param[out]  version	Wire protocol version.
 *
 * @retval LAGOPUS_RESULT_OK		Succeeded.
 */
lagopus_result_t
bridge_ofp_version_get(struct bridge *bridge, uint8_t *version);

/**
 * Set primary OpenFlow version.
 *
 * @param[in]	bridge	Bridge.
 * @param[in]	version	Wire protocol version.
 *
 * @retval LAGOPUS_RESULT_OK		Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS	Version number is wrong.
 */
lagopus_result_t
bridge_ofp_version_set(struct bridge *bridge, uint8_t version);

/**
 * Get supported OpenFlow version bitmap.
 *
 * @param[in]	bridge	Bridge.
 * @param[out]  version_bitmap	Support version bitmap.
 *
 * @retval LAGOPUS_RESULT_OK		Succeeded.
 */
lagopus_result_t
bridge_ofp_version_bitmap_get(struct bridge *bridge,
                              uint32_t *version_bitmap);

/**
 * Set supported OpenFlow version bitmap.
 *
 * @param[in]	bridge	Bridge.
 * @param[in]	version	Wire protocol version to be set to version bitmap.
 *
 * @retval LAGOPUS_RESULT_OK		Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS	Version number is wrong.
 */
lagopus_result_t
bridge_ofp_version_bitmap_set(struct bridge *bridge, uint8_t version);

/**
 * Get supported OpenFlow version bitmap.
 *
 * @param[in]	bridge	Bridge.
 * @param[in]	version	Wire protocol version to be unset from version bitmap.
 *
 * @retval LAGOPUS_RESULT_OK		Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS	Version number is wrong.
 */
lagopus_result_t
bridge_ofp_version_bitmap_unset(struct bridge *bridge, uint8_t version);

/**
 * Get switch features.
 *
 * @param[in]	bridge			Bridge.
 * @param[out]	features		Pointer of features.
 *
 * @retval LAGOPUS_RESULT_OK		Succeeded.
 * @retval	LAGOPUS_RESULT_NOT_FOUND Bridge is not exist.
 */
lagopus_result_t
bridge_ofp_features_get(struct bridge *bridge,
                        struct ofp_switch_features *features);

/**
 * Get bridge name.
 *
 * @param[in]	bridge			Bridge.
 *
 * @retval	Bridge name.
 */
const char *
bridge_name_get(struct bridge *bridge);

/**
 * Add controller address to the bridge.
 *
 * @param[in]	bridge			Bridge.
 * @param[in]	controller_addresss	Controller address string.
 *
 * @retval	LAGOPUS_RESULT_OK	Success.
 * @retval	LAGOPUS_RESULT_NOT_FOUND Bridge is not exist.
 */
lagopus_result_t
bridge_controller_add(struct bridge *bridge,
                      const char *controller_str);

/**
 * Delete controller address to the bridge.
 *
 * @param[in]	bridge			Bridge.
 * @param[in]	controller_addresss	Controller address string.
 *
 * @retval	LAGOPUS_RESULT_OK	Success.
 * @retval	LAGOPUS_RESULT_NOT_FOUND Bridge is not exist.
 */
lagopus_result_t
bridge_controller_delete(struct bridge *bridge,
                         const char *controller_str);

/**
 * Get datapath id owned by bridge.
 *
 * @param[in]	bridge			Bridge.
 *
 * @retval	Datapath id.
 */
uint64_t
bridge_dpid(struct bridge *bridge);

#endif /* __LAGOPUS_BRIDGE_H__ */
