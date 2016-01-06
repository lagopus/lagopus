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

#ifndef __LAGOPUS_DATASTORE_PORT_H__
#define __LAGOPUS_DATASTORE_PORT_H__

/**
 * @brief	datastore_port_stats_t
 */
typedef struct datastore_port_stats {
  uint32_t config; /* OFPPC_* (TODO) */
  uint32_t state; /* OFPPS_* (TODO) */
  uint32_t curr_features; /* OFPPF_* (TODO) */
  uint32_t supported_features; /* OFPPF_* (TODO) */
} datastore_port_stats_t;

/**
 * Get the value to attribute 'enabled' of the port table record'
 *
 *  @param[in] name
 *  @param[out] enabled the value of attribute 'enabled'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'enabled' getted sucessfully.
 */
lagopus_result_t
datastore_port_is_enabled(const char *name, bool *enabled);

/**
 * Get the value to attribute 'used' of the port table record'
 *
 *  @param[in] name
 *  @param[out] used the value of attribute 'used'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'used' getted sucessfully.
 */
lagopus_result_t
datastore_port_is_used(const char *name, bool *used);

/**
 * Get the value to attribute 'port_number' of the port table record'
 *
 *  @param[in] name
 *  @param[out] port_number the value of attribute 'port_number'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'port_number' getted sucessfully.
 */
lagopus_result_t
datastore_port_get_port_number(const char *name, bool current,
                               uint32_t *port_number);

/**
 * Get the value to attribute 'interface_name' of the port table record'
 *
 *  @param[in] name
 *  @param[out] interface_name the value of attribute 'interface_name'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'interface_name' getted sucessfully.
 */
lagopus_result_t
datastore_port_get_interface_name(const char *name, bool current,
                                  char **interface_name);

/**
 * Get the value to attribute 'policer_name' of the port table record'
 *
 *  @param[in] name
 *  @param[in] current	current or modified.
 *  @param[out] policer_name the value of attribute 'policer_name'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'policer_name' getted sucessfully.
 */
lagopus_result_t
datastore_port_get_policer_name(const char *name, bool current,
                                char **policer_name) ;

/**
 * Get the value to attribute 'queue_names' of the port table record'
 *
 *  @param[in] name
 *  @param[in] current	current or modified.
 *  @param[out] queue_names the value of attribute 'queue_names'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'queue_names' getted sucessfully.
 */
lagopus_result_t
datastore_port_get_queue_names(const char *name,
                               bool current,
                               datastore_name_info_t **queue_names);

#endif /* ! __LAGOPUS_DATASTORE_PORT_H__ */

