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
 * @file	ofp_packet_in_handler.h
 */

#ifndef __OFP_PACKET_IN_HANDLER_H__
#define __OFP_PACKET_IN_HANDLER_H__

#include "ofp_common.h"
#include "ofp_role.h"

/**
 * ofp_packet_in handler.
 *
 *     @param[in]	packet_in	A pointer to \e packet_in structure.
 *     @param[in]	dpid	dpid.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_packet_in_handle(struct packet_in *packet_in,
                     uint64_t dpid);

#ifdef __UNIT_TESTING__
/**
 * Create ofp_packet_in.
 *
 *     @param[in]	packet_in	A pointer to \e packet_in structure.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_packet_in_create(struct packet_in *packet_in,
                     struct pbuf **pbuf);
#endif /* __UNIT_TESTING__ */

#endif /* __OFP_PACKET_IN_HANDLER_H__ */
