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
 * @file	ofp_header_handler.h
 */

#ifndef __OFP_HEADER_HANDLER_H__
#define __OFP_HEADER_HANDLER_H__

#include "channel.h"

/**
 * ofp_header handler.
 *
 *     @param[in]	channel	A pointer to \e channel structure.
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OFP_ERROR Failed, ofp_error.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_header_handle(struct channel *channel, struct pbuf *pbuf,
                  struct ofp_error *error);

/**
 * Set ofp_header.
 *
 *     @param[out]	header	A pointer to \e ofp_header structure.
 *     @param[in]	version	OFP version.
 *     @param[in]	type	Type of OFP.
 *     @param[in]	length	Size of structure.
 *     @param[in]	xid	xid.
 *
 *     @retval	void
 */
void
ofp_header_set(struct ofp_header *header, uint8_t version,
               uint8_t type, uint16_t length, uint32_t xid);

/**
 * Create ofp_header.
 *
 *     @param[in]	channel	A pointer to \e channel structure.
 *     @param[in]	type	Type of OFP.
 *     @param[in]	xid_header	A pointer to \e ofp_header structure in request.
 *     @param[in]	header	A pointer to \e ofp_header structure.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_header_create(struct channel *channel, uint8_t type,
                  struct ofp_header *xid_header,
                  struct ofp_header *header,
                  struct pbuf *pbuf);

/**
 * Set ofp verion, length, type in packet.
 *
 *     @param[in]	channel	A pointer to \e channel structure.
 *     @param[in,out]	pbuf	A pointer to \e pbuf structure.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_header_packet_set(struct channel *channel,
                      struct pbuf *pbuf);

/**
 * Set length in packet.
 *
 *     @param[in,out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	length	Size of packet.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_header_length_set(struct pbuf *pbuf,
                      uint16_t length);

/**
 * Copy ofp_header and multipart header.
 *
 *     @param[in,out]	dst_pbuf	A pointer to \e pbuf structure.
 *     @param[in]	src_pbuf	A pointer to \e pbuf structure.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_header_mp_copy(struct pbuf *dst_pbuf,
                   struct pbuf *src_pbuf);

/**
 * Check ofp_version in ofp_header.
 *
 *     @param[in]	channel	A pointer to \e channel structure.
 *     @param[in]	header	A pointer to \e ofp_header structure in request.
 *
 *     @retval	ture	Supported ofp version.
 *     @retval	false	Unsupported ofp version.
 */
bool
ofp_header_version_check(struct channel *channel,
                         struct ofp_header *header);

#endif /* __OFP_HEADER_HANDLER_H__ */
