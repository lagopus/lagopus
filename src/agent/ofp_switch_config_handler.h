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
 * @file	ofp_switch_config_handler.h
 */

#ifndef __OFP_SWITCH_CONFIG_HANDLER_H__
#define __OFP_SWITCH_CONFIG_HANDLER_H__

#include "ofp_common.h"
#include "channel.h"
#include "openflow13.h"

/**
 * ofp_set_config handler.
 *
 *     @param[in]	channel	A pointer to \e channel structure.
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	xid_header	A pointer to \e ofp_header structure in request.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OFP_ERROR Failed, ofp_error.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_set_config_handle(struct channel *channel,
                      struct pbuf *pbuf,
                      struct ofp_header *xid_header,
                      struct ofp_error *error);

/**
 * ofp_get_config_request handler.
 *
 *     @param[in]	channel	A pointer to \e channel structure.
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	xid_header	A pointer to \e ofp_header structure in request.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OFP_ERROR Failed, ofp_error.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_get_config_request_handle(struct channel *channel, struct pbuf *pbuf,
                              struct ofp_header *xid_header,
                              struct ofp_error *error);

#ifdef __UNIT_TESTING__
/**
 * Create ofp_get_config_reply.
 *
 *     @param[in]	channel	A pointer to \e channel structure.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	switch_config	A pointer to \e ofp_switch_config structure.
 *     @param[in]	xid_header	A pointer to \e ofp_header structure.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_get_config_reply_create(struct channel *channel, struct pbuf **pbuf,
                            struct ofp_switch_config *switch_config,
                            struct ofp_header *xid_header);
#endif /* __UNIT_TESTING__ */

#endif /* __OFP_SWITCH_CONFIG_HANDLER_H__ */
