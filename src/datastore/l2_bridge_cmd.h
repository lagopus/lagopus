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
 * @file	l2_bridge_cmd.h
 */

#ifndef __L2_BRIDGE_CMD_H__
#define __L2_BRIDGE_CMD_H__

/**
 * Initialize l2_bridge cmd.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
l2_bridge_cmd_initialize(void);

/**
 * Finalize l2_bridge cmd.
 *
 *     @retval	void
 */
void
l2_bridge_cmd_finalize(void);

/**
 * Enable l2_bridge.
 *
 *     @param[in]	iptr	An interpreter.
 *     @paran[in]	state	A status of the interpreter.
 *     @paran[in]	name	A l2_bridge name.
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
l2_bridge_cmd_enable_propagation(datastore_interp_t *iptr,
                                 datastore_interp_state_t state,
                                 char *name,
                                 lagopus_dstring_t *result);

/**
 * Disable l2_bridge.
 *
 *     @param[in]	iptr	An interpreter.
 *     @paran[in]	state	A status of the interpreter.
 *     @paran[in]	name	A l2_bridge name.
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
l2_bridge_cmd_disable_propagation(datastore_interp_t *iptr,
                                  datastore_interp_state_t state,
                                  char *name,
                                  lagopus_dstring_t *result);

/**
 * Call update func in l2_bridge cmd.
 *
 *     @param[in]	iptr	An interpreter.
 *     @paran[in]	state	A status of the interpreter.
 *     @paran[in]	name	A l2_bridge name.
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_NOT_FOUND	Failed, not found.
 */
lagopus_result_t
l2_bridge_cmd_update_propagation(datastore_interp_t *iptr,
                                 datastore_interp_state_t state,
                                 char *name,
                                 lagopus_dstring_t *result);

/**
 * Set bridge name.
 *
 *     @paran[in]	name	A l2_bridge name.
 *     @paran[in]	bridge_name	A bridge name.
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_NOT_FOUND	Failed, not found.
 */
lagopus_result_t
l2_bridge_cmd_set_bridge(const char *name,
                         const char *bridge_name,
                         lagopus_dstring_t *result);

#endif /* __L2_BRIDGE_CMD_H__ */
