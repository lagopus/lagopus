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
 * @file	datastore_internal.h
 */

#ifndef __LAGOPUS_DATASTORE_INTERNAL_H__
#define __LAGOPUS_DATASTORE_INTERNAL_H__

#include "lagopus/datastore/common.h"
#include "datastore_apis.h"

#define DATASTORE_TMP_DIR "/tmp"

enum ns_types {
  NS_FULLNAME = 1,
  NS_DEFAULT_NAMESPACE,
  NS_NAMESPACE,
  NS_NAME,
};

typedef uint8_t mac_address_t[6];

bool
bridge_exists(const char *name);

bool
channel_exists(const char *name);

bool
controller_exists(const char *name);

bool
interface_exists(const char *name);

bool
port_exists(const char *name);

bool
queue_exists(const char *name);

bool
policer_exists(const char *name);

bool
policer_action_exists(const char *name);

bool
namespace_exists(const char *namespace);

lagopus_result_t
bridge_set_used(const char *name, bool is_used);

lagopus_result_t
channel_set_used(const char *name, bool is_used);

lagopus_result_t
controller_set_used(const char *name, bool is_used);

lagopus_result_t
interface_set_used(const char *name, bool is_used);

lagopus_result_t
port_set_used(const char *name, bool is_used);

lagopus_result_t
queue_set_used(const char *name, bool is_used);

lagopus_result_t
policer_set_used(const char *name, bool is_used);

lagopus_result_t
policer_action_set_used(const char *name, bool is_used);

lagopus_result_t
bridge_set_enabled(const char *name, bool is_enabled);

lagopus_result_t
channel_set_enabled(const char *name, bool is_enabled);

lagopus_result_t
controller_set_enabled(const char *name, bool is_enabled);

lagopus_result_t
interface_set_enabled(const char *name, bool is_enabled);

lagopus_result_t
port_set_enabled(const char *name, bool is_enabled);

lagopus_result_t
queue_set_enabled(const char *name, bool is_enabled);

lagopus_result_t
policer_set_enabled(const char *name, bool is_enabled);

lagopus_result_t
policer_action_set_enabled(const char *name, bool is_enabled);

lagopus_result_t
log_cmd_serialize(lagopus_dstring_t *result);

lagopus_result_t
datastore_cmd_serialize(lagopus_dstring_t *result);

lagopus_result_t
tls_cmd_serialize(lagopus_dstring_t *result);

lagopus_result_t
snmp_cmd_serialize(lagopus_dstring_t *result);

lagopus_result_t
namespace_cmd_serialize(lagopus_dstring_t *result);

lagopus_result_t
agent_cmd_serialize(lagopus_dstring_t *result);

lagopus_result_t
copy_mac_address(const mac_address_t src, mac_address_t dst);

bool
equals_mac_address(const mac_address_t mac1, const mac_address_t mac2);


/**
 *  Get the current namespace.
 *
 *     @param[out]	namespace	A pointer to a current namespace.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
namespace_get_current_namespace(char **namespace);

/**
 *  Get the saved namespace.
 *
 *     @param[out]	namespace	A pointer to a saved namespace.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
namespace_get_saved_namespace(char **namespace);

/**
 *  Get the namespace of the object.
 *
 *     @param[in]	fullname	A pointer to a full name.
 *     @param[out]	namespace	A pointer to a namespace.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 *
 *     @details The full name of the format is "<current namespace>:<object name>"
 */
lagopus_result_t
namespace_get_namespace(const char *fullname, char **namespace);

/**
 *  Get the full name of the object.
 *
 *     @param[in]	name	A pointer to a object name.
 *     @param[out]	fullname	A pointer to a full name.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 *
 *     @details The full name of the format is "<current namespace>:<object name>"
 */
lagopus_result_t
namespace_get_fullname(const char *name, char **fullname);

/**
 *  Split the full name to namespace and name.
 *
 *     @param[in]	fullname	A pointer to a full name.
 *     @param[out]	target		A pointer to a target.
 *
 *     @retval	>0	Succeeded, search type returned.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
namespace_get_search_target(const char *fullname, char **target);

lagopus_result_t
dryrun_begin(void);

lagopus_result_t
dryrun_end(void);

lagopus_result_t
datastore_all_commands_initialize(void);

void
datastore_all_commands_finalize(void);

lagopus_result_t
controller_get_name_by_channel(const char *channel_name,
                               bool is_current,
                               char **name);

#endif /* ! __LAGOPUS_DATASTORE_INTERNAL_H__ */
