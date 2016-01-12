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
 * @file	policer_cmd.h
 */

#ifndef __POLICER_CMD_H__
#define __POLICER_CMD_H__

/**
 * Initialize policer cmd.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
policer_cmd_initialize(void);

/**
 * Finalize policer cmd.
 *
 *     @retval	void
 */
void
policer_cmd_finalize(void);

/**
 * Enable policer.
 *
 *     @param[in]	iptr	An interpreter.
 *     @paran[in]	state	A status of the interpreter.
 *     @paran[in]	name	A policer name.
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
policer_cmd_enable_propagation(datastore_interp_t *iptr,
                               datastore_interp_state_t state,
                               char *name,
                               lagopus_dstring_t *result);

/**
 * Disable policer.
 *
 *     @param[in]	iptr	An interpreter.
 *     @paran[in]	state	A status of the interpreter.
 *     @paran[in]	name	A policer name.
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
policer_cmd_disable_propagation(datastore_interp_t *iptr,
                                datastore_interp_state_t state,
                                char *name,
                                lagopus_dstring_t *result);

/**
 * Call update func in policer cmd.
 *
 *     @param[in]	iptr	An interpreter.
 *     @paran[in]	state	A status of the interpreter.
 *     @paran[in]	name	A policer name.
 *     @paran[in]	is_modified	Is modified.
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_NOT_FOUND	Failed, not found.
 */
lagopus_result_t
policer_cmd_update_propagation(datastore_interp_t *iptr,
                               datastore_interp_state_t state,
                               char *name,
                               lagopus_dstring_t *result);

#endif /* __POLICER_CMD_H__ */
