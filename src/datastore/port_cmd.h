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
 * @file	port_cmd.h
 */

#ifndef __PORT_CMD_H__
#define __PORT_CMD_H__

/**
 * Initialize port cmd.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
port_cmd_initialize(void);

/**
 * Finalize port cmd.
 *
 *     @retval	void
 */
void
port_cmd_finalize(void);

/**
 * Enable port.
 *
 *     @param[in]	iptr	An interpreter.
 *     @paran[in]	state	A status of the interpreter.
 *     @paran[in]	name	A port name.
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
port_cmd_enable_propagation(datastore_interp_t *iptr,
                            datastore_interp_state_t state,
                            char *name,
                            lagopus_dstring_t *result);

/**
 * Disable port.
 *
 *     @param[in]	iptr	An interpreter.
 *     @paran[in]	state	A status of the interpreter.
 *     @paran[in]	name	A port name.
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
port_cmd_disable_propagation(datastore_interp_t *iptr,
                             datastore_interp_state_t state,
                             char *name,
                             lagopus_dstring_t *result);

/**
 * Call update func in port cmd.
 *
 *     @param[in]	iptr	An interpreter.
 *     @paran[in]	state	A status of the interpreter.
 *     @paran[in]	name	A port name.
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_NOT_FOUND	Failed, not found.
 */
lagopus_result_t
port_cmd_update_propagation(datastore_interp_t *iptr,
                            datastore_interp_state_t state,
                            char *name,
                            lagopus_dstring_t *result);

/**
 * Set port number.
 *
 *     @paran[in]	name	A port name.
 *     @paran[in]	port_number	A port number.
 *     @param[out]	result	A result/output string (NULL allowed).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
port_cmd_set_port_number(char *name,
                         const uint32_t port_number,
                         lagopus_dstring_t *result);

/**
 * Is exists port no.
 *
 *     @param[in]	name	Port name.
 *     @paran[in]	port_number	Port no.
 *     @param[out]	b	Is exists port no.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
lagopus_result_t
port_cmd_port_number_is_exists(char *name,
                               const uint32_t port_number,
                               bool *b);

#endif /* __PORT_CMD_H__ */
