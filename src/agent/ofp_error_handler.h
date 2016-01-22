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
 * @file	ofp_error_handler.h
 */

#ifndef __OFP_ERROR_HANDLER_H__
#define __OFP_ERROR_HANDLER_H__

#include "ofp_common.h"
#include "channel.h"

/**
 * ofp_error_msg handler.
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
ofp_error_msg_handle(struct channel *channel, struct pbuf *pbuf,
                     struct ofp_header *xid_header,
                     struct ofp_error *error);

/**
 * Send ofp_error_msg for ofp_error structure.
 *
 *     @param[in]	channel	A pointer to \e channel structure.
 *     @param[in]	xid_header	A pointer to \e ofp_header structure in request.
 *     @param[in]	error	A pointer to \e ofp_error structure.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_error_msg_send(struct channel *channel,
                   struct ofp_header *xid_header,
                   struct ofp_error *error);

/**
 * Unsupported packet handler.
 *
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OFP_ERROR Failed, ofp_error.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_unsupported_handle(struct ofp_error *error);

/**
 * Bad experimenter packet handler.
 *
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OFP_ERROR Failed, ofp_error.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_bad_experimenter_handle(struct ofp_error *error);

/**
 * ofp_error_msg handler (for dp).
 *
 *     @param[in]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *     @param[in]	dpid	dpid.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_error_msg_from_queue_handle(struct error *error,
                                uint64_t dpid);

/**
 * Free ofp_error_msg.
 *
 *     @param[in]	data	A pointer to \e eventq_data structure.
 *
 *     @retval	void
 */
void
ofp_error_msg_free(struct eventq_data *data);

/**
 * Get type in ofp_error_msg. (String)
 *
 *     @param[in]	type	Type of ofp_error.
 *
 *     @retval	string
 */
const char *
ofp_error_type_str_get(uint16_t type);

/**
 * Get code in ofp_error_msg. (String)
 *
 *     @param[in]	type	Type of ofp_error.
 *     @param[in]	code	Code of ofp_error.
 *
 *     @retval	string
 */
const char *
ofp_error_code_str_get(uint16_t type, uint16_t code);

#ifdef __UNIT_TESTING__
/**
 * Create ofp_error_msg.
 *
 *     @param[in]	channel	A pointer to \e channel structure.
 *     @param[in]	error	A pointer to \e ofp_error structure.
 *     @param[in]	xid_header	A pointer to \e ofp_header structure.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_error_msg_create(struct channel *channel, struct ofp_error *error,
                     struct ofp_header *xid_header, struct pbuf **pbuf);
#endif /* __UNIT_TESTING__ */

#endif /* __OFP_ERROR_HANDLER_H__ */
