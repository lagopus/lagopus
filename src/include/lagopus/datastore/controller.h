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

#ifndef __LAGOPUS_DATASTORE_CONTROLLER_H__
#define __LAGOPUS_DATASTORE_CONTROLLER_H__

typedef enum datastore_controller_role {
  DATASTORE_CONTROLLER_ROLE_UNKNOWN = 0,
  DATASTORE_CONTROLLER_ROLE_MASTER,
  DATASTORE_CONTROLLER_ROLE_SLAVE,
  DATASTORE_CONTROLLER_ROLE_EQUAL,
  DATASTORE_CONTROLLER_ROLE_MIN = DATASTORE_CONTROLLER_ROLE_UNKNOWN,
  DATASTORE_CONTROLLER_ROLE_MAX = DATASTORE_CONTROLLER_ROLE_EQUAL,
} datastore_controller_role_t;

typedef enum datastore_controller_connection_type {
  DATASTORE_CONTROLLER_CONNECTION_TYPE_UNKNOWN = 0,
  DATASTORE_CONTROLLER_CONNECTION_TYPE_MAIN,
  DATASTORE_CONTROLLER_CONNECTION_TYPE_AUXILIARY,
  DATASTORE_CONTROLLER_CONNECTION_TYPE_MIN =
    DATASTORE_CONTROLLER_CONNECTION_TYPE_UNKNOWN,
  DATASTORE_CONTROLLER_CONNECTION_TYPE_MAX =
    DATASTORE_CONTROLLER_CONNECTION_TYPE_AUXILIARY,
} datastore_controller_connection_type_t;

/**
 * @brief   datastore_controller_stats_t
 */
typedef struct datastore_controller_stats {
  bool is_connected;
  uint8_t version;
  datastore_controller_role_t role;
} datastore_controller_stats_t;

/**
 * Get the value to attribute 'enabled' of the controller table record'
 *
 *  @param[in] name
 *  @param[out] enabled the value of attribute 'enabled'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'enabled' getted sucessfully.
 */
lagopus_result_t
datastore_controller_is_enabled(const char *name, bool *enabled);

/**
 * Get the value to attribute 'used' of the controller table record'
 *
 *  @param[in] name
 *  @param[out] used the value of attribute 'used'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'used' getted sucessfully.
 */
lagopus_result_t
datastore_controller_is_used(const char *name, bool *used);

/**
 * Get the value to attribute 'channel_name' of the controller table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] channel_name the value of attribute 'channel_name'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'channel_name' getted sucessfully.
 */
lagopus_result_t
datastore_controller_get_channel_name(const char *name, bool current,
                                      char **channel_name);

/**
 * Get the value to attribute 'role' of the controller table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] role the value of attribute 'role'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'role' getted sucessfully.
 */
lagopus_result_t
datastore_controller_get_role(const char *name, bool current,
                              datastore_controller_role_t *role);

/**
 * Get the value to attribute 'type' of the controller table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] type the value of attribute 'type'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'type' getted sucessfully.
 */
lagopus_result_t
datastore_controller_get_connection_type(const char *name, bool current,
    datastore_controller_connection_type_t *type);

#endif /* ! __LAGOPUS_DATASTORE_CONTROLLER_H__ */

