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
 * @file	flow_cmd.h
 */

#ifndef __FLOW_CMD_H__
#define __FLOW_CMD_H__

#include "cmd_common.h"
#include "lagopus/flowdb.h"

/**
 * Initialize flow cmd.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
flow_cmd_initialize(void);

/**
 * Finalize flow cmd.
 *
 *     @retval	void
 */
void
flow_cmd_finalize(void);

/**
 * Dump action.
 *
 *     @param[in]	action	A action structure.
 *     @param[in]	is_action_first	A delimiter flag.
 *     @param[out]	result	A result.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
lagopus_result_t
flow_cmd_dump_action(struct action *action,
                     bool is_action_first,
                     lagopus_dstring_t *result);

#endif /* __FLOW_CMD_H__ */
