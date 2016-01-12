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
 * @file	ofp_role.h
 */

#ifndef __OFP_ROLE_H__
#define __OFP_ROLE_H__

#define DEFAULT_REASON_MASK 0xffffffff

/**
 * Update channel's role.
 *
 *     @param[in]	channel	A pointer to \e channel structure.
 *     @param[in]	role_request	A pointer to \e ofp_role_request structure.
 *     @param[in]	dpid	dpid.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_role_channel_update(struct channel *channel,
                        struct ofp_role_request *role_request,
                        uint64_t dpid);

/**
 * Check role for packet_in, flow_removed, port_status.
 *
 *     @param[in]	role_mask	A pointer to \e ofp_async_config structure(role mask).
 *     @param[in]	type	Type (OFPT_{PACKET_IN,FLOW_REMOVED,PORT_STATUS}).
 *     @param[in]	reason	Reason.
 *     @param[in]	role	Type of role.
 *
 *     @retval	true	Succeeded.
 *     @retval	false	Failed.
 */
bool
ofp_role_channel_check_mask(struct ofp_async_config *role_mask, uint8_t type,
                            uint8_t reason, uint32_t role);

/**
 * Write packet_in, flow_removed, port_status packet.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	dpid	dpid.
 *     @param[in]	type	Type (OFPT_{PACKET_IN,FLOW_REMOVED,PORT_STATUS}).
 *     @param[in]	reason	Reason.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_role_channel_write(struct pbuf *pbuf, uint64_t dpid,
                       uint8_t type, uint8_t reason);

/**
 * Check role for request.
 *
 *     @param[in]	channel	A pointer to \e channel structure.
 *     @param[in]	header	A pointer to \e ofp_header structure.
 *
 *     @retval	true	Succeeded.
 *     @retval	false	Failed.
 */
bool
ofp_role_check(struct channel *channel,
               struct ofp_header *header);

/**
 * Check role for multipart request.
 *
 *     @param[in]	channel	A pointer to \e channel structure.
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	multipart	A pointer to \e ofp_multipart_request structure.
 *
 *     @retval	true	Succeeded.
 *     @retval	false	Failed.
 */
bool
ofp_role_mp_check(struct channel *channel,
                  struct pbuf *pbuf,
                  struct ofp_multipart_request *multipart);

/**
 * Check generation_id.
 *
 *     @param[in]	dpid	dpid.
 *     @param[in]	role	Type of role.
 *     @param[in]	generation_id	generation_id in request.
 *
 *     @retval	true	Succeeded.
 *     @retval	false	Failed.
 */
bool
ofp_role_generation_id_check(uint64_t dpid,
                             uint32_t role,
                             uint64_t generation_id);

/**
 * Set default role mask.
 *
 *     @param[out]	role_mask	role_mask.
 *
 *     @retval	void
 */
void
ofp_role_channel_mask_init(struct ofp_async_config *role_mask);

#endif /* __OFP_ROLE_H__ */
