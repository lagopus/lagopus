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

#ifndef __LAGOPUS_DATASTORE_POLICER_ACTION_H__
#define __LAGOPUS_DATASTORE_POLICER_ACTION_H__

typedef enum datastore_policer_action_type {
  DATASTORE_POLICER_ACTION_TYPE_UNKNOWN = 0,
  DATASTORE_POLICER_ACTION_TYPE_DISCARD,
  DATASTORE_POLICER_ACTION_TYPE_MIN = DATASTORE_POLICER_ACTION_TYPE_UNKNOWN,
  DATASTORE_POLICER_ACTION_TYPE_MAX = DATASTORE_POLICER_ACTION_TYPE_DISCARD,
} datastore_policer_action_type_t;

typedef struct datastore_policer_action_info {
  datastore_policer_action_type_t type;
} datastore_policer_action_info_t;

/**
 * Get the value to attribute 'enabled' of the policer_action table record'
 *
 *  @param[in] name
 *  @param[out] enabled the value of attribute 'enabled'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'enabled' getted sucessfully.
 */
lagopus_result_t
datastore_policer_action_is_enabled(const char *name, bool *enabled);

/**
 * Get the value to attribute 'used' of the policer_action table record'
 *
 *  @param[in] name
 *  @param[out] used the value of attribute 'used'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'used' getted sucessfully.
 */
lagopus_result_t
datastore_policer_action_is_used(const char *name, bool *used);

/**
 * Get the value to attribute 'type' of the policer_action table record'
 *
 *  @param[in] name
 *  @param[out] type the value of attribute 'type'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'type' getted sucessfully.
 */
lagopus_result_t
datastore_policer_action_get_type(const char *name, bool current,
                                  datastore_policer_action_type_t *type);

#endif /* ! __LAGOPUS_DATASTORE_POLICER_ACTION_H__ */
