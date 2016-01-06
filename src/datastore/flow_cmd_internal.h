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
 * @file	flow_cmd_internal.h
 */

#ifndef __FLOW_CMD_INTERNAL_H__
#define __FLOW_CMD_INTERNAL_H__

#include "cmd_common.h"

/**
 * Parse command for flow.
 *
 *     @param[in]	iptr	An interpreter.
 *     @paran[in]	state	A status of the interpreter.
 *     @param[in]	argc	A # of tokens in the \b argv.
 *     @param[in]	argv	An array of tokens.
 *     @param[in]	hptr	A hashmap which has related objects.
 *     @param[in]	proc	An update proc.
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
STATIC lagopus_result_t
flow_cmd_parse(datastore_interp_t *iptr,
               datastore_interp_state_t state,
               size_t argc, const char *const argv[],
               lagopus_hashmap_t *hptr,
               datastore_update_proc_t u_proc,
               datastore_enable_proc_t e_proc,
               datastore_serialize_proc_t s_proc,
               datastore_destroy_proc_t d_proc,
               lagopus_dstring_t *result);

/**
 * Create flow dump json string.
 *
 *     @param[in]	name	A bridge name.
 *     @paran[in]	table_id	Table id.
 *     @param[in]	is_with_stats	Dump with stats.
 *     @param[in]	is_bridge_first	A first element flag for bridges.
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
STATIC lagopus_result_t
dump_bridge_domains_flow(const char *name,
                         uint8_t table_id,
                         bool is_with_stats,
                         bool is_bridge_first,
                         lagopus_dstring_t *result);

#endif /* __FLOW_CMD_INTERNAL_H__ */
