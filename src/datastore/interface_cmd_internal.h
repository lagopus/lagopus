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
 * @file	interface_cmd_internal.h
 */

#ifndef __INTERFACE_CMD_INTERNAL_H__
#define __INTERFACE_CMD_INTERNAL_H__

#include "cmd_common.h"

/**
 * Parse command for interface.
 *
 *     @param[in]	iptr	An interpreter.
 *     @paran[in]	state	A status of the interpreter.
 *     @param[in]	argc	A # of tokens in the \b argv.
 *     @param[in]	argv	An array of tokens.
 *     @param[in]	hptr	A hashmap which has related objects.
 *     @param[in]	u_proc	An update proc.
 *     @param[in]	e_proc	An enable proc.
 *     @param[in]	s_proc	A serialize proc.
 *     @param[in]	d_proc	A destroy proc.
 *     @param[out]	result	A result/output string.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
STATIC lagopus_result_t
interface_cmd_parse(datastore_interp_t *iptr,
                    datastore_interp_state_t state,
                    size_t argc, const char *const argv[],
                    lagopus_hashmap_t *hptr,
                    datastore_update_proc_t u_proc,
                    datastore_enable_proc_t e_proc,
                    datastore_serialize_proc_t s_proc,
                    datastore_destroy_proc_t d_proc,
                    lagopus_dstring_t *result);

/**
 * Update interface obj.
 *
 *     @param[in]	iptr	An interpreter.
 *     @paran[in]	state	A status of the interpreter.
 *     @param[in]	objc	A pointer to a interface obj.
 *     @param[out]	result	A result/output string.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_DATASTORE_INTERP_ERROR	Failed, interp error.
 */
STATIC lagopus_result_t
interface_cmd_update(datastore_interp_t *iptr,
                     datastore_interp_state_t state,
                     void *obj,
                     lagopus_dstring_t *result);

/**
 * Serialize interface obj.
 *
 *     @param[in]	iptr	An interpreter.
 *     @paran[in]	state	A status of the interpreter.
 *     @param[in]	objc	A pointer to a interface obj.
 *     @param[out]	result	A result/output string.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_DATASTORE_INTERP_ERROR	Failed, interp error.
 */
STATIC lagopus_result_t
interface_cmd_serialize(datastore_interp_t *iptr,
                        datastore_interp_state_t state,
                        const void *obj,
                        lagopus_dstring_t *result);

#endif /* __INTERFACE_CMD_INTERNAL_H__ */
