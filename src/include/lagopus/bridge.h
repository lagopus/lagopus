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
 * @file        bridge.h
 * @brief       "Bridge" as OpenFlow logical switch APIs.
 */

#ifndef SRC_INCLUDE_LAGOPUS_BRIDGE_H_
#define SRC_INCLUDE_LAGOPUS_BRIDGE_H_

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
  struct vector *ports;                 /** Ports. */
  struct flowdb *flowdb;                /** Flow database. */
  struct group_table *group_table;      /** Group table. */
  struct meter_table *meter_table;      /** Meter table. */
  struct ofp_port controller_port;      /** Controller port config. */
  struct ofp_switch_config switch_config;  /** Switch config. */
};

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


/**
 * Get OpenFlow switch fail mode.
 *
 * @param[in]   bridge  Bridge.
 * @param[out]  fail_mode       Bridge fail mode.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 */
lagopus_result_t
bridge_fail_mode_get(struct bridge *bridge, enum fail_mode *fail_mode) __attribute__ ((deprecated));

/**
 * Get OpenFlow switch fail mode.
 *
 * @param[in]   bridge  Bridge.
 * @param[in]   fail_mode       Bridge fail mode.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS  fail_mode value is wrong.
 */
lagopus_result_t
bridge_fail_mode_set(struct bridge *bridge, enum fail_mode fail_mode) __attribute__ ((deprecated));

/**
 * Get OpenFlow switch features.
 *
 * @param[in]   bridge  Bridge.
 * @param[out]  features        ofp_switch_features structure.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 */
lagopus_result_t
bridge_ofp_features_get(struct bridge *bridge,
                        struct ofp_switch_features *features) __attribute__ ((deprecated));

/**
 * Get primary OpenFlow version.
 *
 * @param[in]   bridge  Bridge.
 * @param[out]  version Wire protocol version.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 */
lagopus_result_t
bridge_ofp_version_get(struct bridge *bridge, uint8_t *version) __attribute__ ((deprecated));

/**
 * Set primary OpenFlow version.
 *
 * @param[in]   bridge  Bridge.
 * @param[in]   version Wire protocol version.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS  Version number is wrong.
 */
lagopus_result_t
bridge_ofp_version_set(struct bridge *bridge, uint8_t version) __attribute__ ((deprecated));

/**
 * Get supported OpenFlow version bitmap.
 *
 * @param[in]   bridge  Bridge.
 * @param[out]  version_bitmap  Support version bitmap.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 */
lagopus_result_t
bridge_ofp_version_bitmap_get(struct bridge *bridge, uint32_t *version_bitmap) __attribute__ ((deprecated));

/**
 * Set supported OpenFlow version bitmap.
 *
 * @param[in]   bridge  Bridge.
 * @param[in]   version Wire protocol version to be set to version bitmap.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS  Version number is wrong.
 */
lagopus_result_t
bridge_ofp_version_bitmap_set(struct bridge *bridge, uint8_t version) __attribute__ ((deprecated));

/**
 * Get supported OpenFlow version bitmap.
 *
 * @param[in]   bridge  Bridge.
 * @param[in]   version Wire protocol version to be unset from version bitmap.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 * @retval LAGOPUS_RESULT_INVALID_ARGS  Version number is wrong.
 */
lagopus_result_t
bridge_ofp_version_bitmap_unset(struct bridge *bridge, uint8_t version) __attribute__ ((deprecated));

/**
 * Get switch features.
 *
 * @param[in]   bridge                  Bridge.
 * @param[out]  features                Pointer of features.
 *
 * @retval LAGOPUS_RESULT_OK            Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND Bridge is not exist.
 */
lagopus_result_t
bridge_ofp_features_get(struct bridge *bridge,
                        struct ofp_switch_features *features) __attribute__ ((deprecated));

#endif /* SRC_INCLUDE_LAGOPUS_BRIDGE_H_ */
