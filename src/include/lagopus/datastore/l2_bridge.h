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

#ifndef __LAGOPUS_DATASTORE_L2_BRIDGE_H__
#define __LAGOPUS_DATASTORE_L2_BRIDGE_H__

/**
 * @brief	datastore_l2_bridge_stats_t
 */
typedef struct datastore_l2_bridge_stats {
  uint64_t entries;
} datastore_l2_bridge_stats_t;

/**
 * Get the value to attribute 'enabled' of the l2_bridge table record'
 *
 *  @param[in] name
 *  @param[out] enabled the value of attribute 'enabled'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'enabled' getted sucessfully.
 */
lagopus_result_t
datastore_l2_bridge_is_enabled(const char *name, bool *enabled);

/**
 * Get the value to attribute 'used' of the l2_bridge table record'
 *
 *  @param[in] name
 *  @param[out] used the value of attribute 'used'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'used' getted sucessfully.
 */
lagopus_result_t
datastore_l2_bridge_is_used(const char *name, bool *used);

/**
 * Get the value to attribute 'expire' of the l2_bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] expire the value of attribute 'expire'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'expire' getted sucessfully.
 */
lagopus_result_t
datastore_l2_bridge_get_expire(const char *name, bool current,
                               uint64_t *expire);

/**
 * Get the value to attribute 'max_entries' of the l2_bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] max_entries the value of attribute 'max_entries'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'max_entries' getted sucessfully.
 */
lagopus_result_t
datastore_l2_bridge_get_max_entries(const char *name, bool current,
                                    uint64_t *max_entries);

/**
 * Get the value to attribute 'bridge_name' of the l2_bridge table record'
 *
 *  @param[in] name
 *  @param[out] bridge_name the value of attribute 'bridge_name'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'bridge_name' getted sucessfully.
 */
lagopus_result_t
datastore_l2_bridge_get_bridge_name(const char *name,
                                    char **bridge_name);

/**
 * Get the value to attribute 'tmp_dir' of the l2_bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] tmp_dir the value of attribute 'tmp_dir'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'tmp_dir' getted sucessfully.
 */
lagopus_result_t
datastore_l2_bridge_get_tmp_dir(const char *name, bool current,
                                char **tmp_dir);

#endif /* ! __LAGOPUS_DATASTORE_L2_BRIDGE_H__ */
