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
 * @file	ofp_match.h
 */

#ifndef __MATCH_H__
#define __MATCH_H__

#include "channel.h"
#include "lagopus/flowdb.h"

/**
 * Alloc match.
 *
 *     @retval	*match	Succeeded, A pointer to \e match structure.
 *     @retval	NULL	Failed.
 */
struct match *
match_alloc(uint8_t size);

/**
 * Create trace string for \e match.
 *
 *     @param[in]	pac	A pointer to \e match structure.
 *     @param[in]	pac_size	Size of \e match structure.
 *     @param[out]	str	A pointer to \e trace string.
 *     @param[in]	max_len	Max length.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE	Over max length.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
match_trace(const void *pac, size_t pac_size,
            char *str, size_t max_len);

/**
 * Trace match_list.
 *
 *     @param[in]	flag	Trace flags. Or'd value of TRACE_OFPT_*.
 *     @param[in]	match_list	A pointer to list of \e match structures.
 *
 *     @retval	void
 */
void
ofp_match_list_trace(uint32_t flags,
                     struct match_list *match_list);

/**
 * Parse match.
 *
 *     @param[in]	channel	A pointer to \e channel structure.
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[out]	match_list	A pointer to list of \e match structures.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OFP_ERROR Failed, ofp_error.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_match_parse(struct channel *channel, struct pbuf *pbuf,
                struct match_list *match_list,
                struct ofp_error *error);

/**
 * Encode match_list.
 *
 *     @param[out]	pbuf_list	A pointer to list of \e pbuf structures.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	match_list	A pointer to list of \e match structures.
 *     @param[out]	total_length	A pointer to \e size of packet.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE Failed, out of range.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_match_list_encode(struct pbuf_list *pbuf_list,
                      struct pbuf **pbuf,
                      struct match_list *match_list,
                      uint16_t *total_length);

#endif /* __MATCH_H__ */
