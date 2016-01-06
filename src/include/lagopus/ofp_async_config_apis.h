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
 * @file	ofp_async_config_apis.h
 * @brief	Agent/Data-Plane APIs for ofp_async_config
 * @details	Describe APIs between Agent and Data-Plane for ofp_async_config.
 */
#ifndef __LAGOPUS_OFP_ASYNC_CONFIG_APIS_H__
#define __LAGOPUS_OFP_ASYNC_CONFIG_APIS_H__

#include "lagopus_apis.h"
#include "openflow.h"
#include "lagopus/bridge.h"


/* Async conf */
/**
 * Set async configuration for \b OFPT_SET_ASYNC.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	async_config	A pointer to ofp_async_config structure.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Data-Plane side.
 */
lagopus_result_t
ofp_async_config_set(uint64_t dpid,
                     struct ofp_async_config *async_config,
                     struct ofp_error *error);

/**
 * Get async configuration for \b OFPT_GET_ASYNC_REQUEST/REPLY.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[out]	async_config	A pointer to ofp_async_config structure.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Agent side.
 */
lagopus_result_t
ofp_async_config_get(uint64_t dpid,
                     struct ofp_async_config *async_config,
                     struct ofp_error *error);
/* Async conf END */

#endif /* __LAGOPUS_OFP_ASYNC_CONFIG_APIS_H__ */
