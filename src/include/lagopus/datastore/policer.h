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

#ifndef __LAGOPUS_DATASTORE_POLICER_H__
#define __LAGOPUS_DATASTORE_POLICER_H__

typedef struct datastore_policer_info {
  uint64_t bandwidth_limit;
  uint64_t burst_size_limit;
  uint8_t bandwidth_percent;
} datastore_policer_info_t;

/**
 * Get the value to attribute 'enabled' of the policer table record'
 *
 *  @param[in] name
 *  @param[out] enabled the value of attribute 'enabled'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'enabled' getted sucessfully.
 */
lagopus_result_t
datastore_policer_is_enabled(const char *name, bool *enabled);

/**
 * Get the value to attribute 'used' of the policer table record'
 *
 *  @param[in] name
 *  @param[out] used the value of attribute 'used'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'used' getted sucessfully.
 */
lagopus_result_t
datastore_policer_is_used(const char *name, bool *used);

/**
 * Get the value to attribute 'bandwidth_limit' of the policer table record'
 *
 *  @param[in] name
 *  @param[out] bandwidth_limit the value of attribute 'bandwidth_limit'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'bandwidth_limit' getted sucessfully.
 */
lagopus_result_t
datastore_policer_get_bandwidth_limit(const char *name, bool current,
                                      uint64_t *bandwidth_limit);

/**
 * Get the value to attribute 'burst_size_limit' of the policer table record'
 *
 *  @param[in] name
 *  @param[out] burst_size_limit the value of attribute 'burst_size_limit'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'burst_size_limit' getted sucessfully.
 */
lagopus_result_t
datastore_policer_get_burst_size_limit(const char *name, bool current,
                                       uint64_t *burst_size_limit);

/**
 * Get the value to attribute 'bandwidth_percent' of the policer table record'
 *
 *  @param[in] name
 *  @param[out] bandwidth_percent the value of attribute 'bandwidth_percent'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'bandwidth_percent' getted sucessfully.
 */
lagopus_result_t
datastore_policer_get_bandwidth_percent(const char *name, bool current,
                                        uint8_t *bandwidth_percent);

/**
 * Get the value to attribute 'action_names' of the policer table record'
 *
 *  @param[in] name
 *  @param[in] current	current or modified.
 *  @param[out] action_names the value of attribute 'action_names'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'action_names' getted sucessfully.
 */
lagopus_result_t
datastore_policer_get_action_names(const char *name,
                                   bool current,
                                   datastore_name_info_t **action_names);

#endif /* ! __LAGOPUS_DATASTORE_POLICER_H__ */
