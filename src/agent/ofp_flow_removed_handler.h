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
 * @file	ofp_flow_removed_handler.h
 */

#ifndef __OFP_FLOW_REMOVED_HANDLER_H__
#define __OFP_FLOW_REMOVED_HANDLER_H__

#include "ofp_common.h"
#include "ofp_role.h"

/**
 * ofp_flow_removed handler.
 *
 *     @param[in]	channel	A pointer to \e flow_removed structure.
 *     @param[in]	dpid	dpid.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_flow_removed_handle(struct flow_removed *flow_removed,
                        uint64_t dpid);

#ifdef __UNIT_TESTING__
/**
 * Create ofp_flow_removed.
 *
 *     @param[in]	reply	A pointer to \e flow_removed structure.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_flow_removed_create(struct flow_removed *flow_removed,
                        struct pbuf **pbuf);
#endif /* __UNIT_TESTING__ */

#endif /* __OFP_FLOW_REMOVED_HANDLER_H__ */
