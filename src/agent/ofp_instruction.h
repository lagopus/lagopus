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
 * @file	ofp_instruction.h
 */

#ifndef __INSTRUCTION_H__
#define __INSTRUCTION_H__

#include "lagopus/flowdb.h"
#include "lagopus/pbuf.h"

/**
 * Alloc instruction.
 *
 *     @retval	*instruction	Succeeded,
 *     A pointer to \e instruction structure.
 *     @retval	NULL	Failed.
 */
struct instruction *
instruction_alloc(void);

/**
 * Trace instruction_list.
 *
 *     @param[in]	flag	Trace flags. Or'd value of TRACE_OFPT_*.
 *     @param[in]	instruction_list	A pointer to list of
 *     \e instruction structures.
 *
 *     @retval	void
 */
void
ofp_instruction_list_trace(uint32_t flags,
                           struct instruction_list *instruction_list);

/**
 * Parse instruction.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[out]	instruction_list	A pointer to list of
 *     \e instruction structures.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OFP_ERROR Failed, ofp_error.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_instruction_parse(struct pbuf *pbuf,
                      struct instruction_list *instruction_list,
                      struct ofp_error *error);

/**
 * Encode instruction_list.
 *
 *     @param[out]	pbuf_list	A pointer to list of \e pbuf structures.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	instruction_list	A pointer to list of
 *     \e instruction structures.
 *     @param[out]	total_length	A pointer to \e size of packet.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE Failed, out of range.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_instruction_list_encode(struct pbuf_list *pbuf_list,
                            struct pbuf **pbuf,
                            struct instruction_list *instruction_list,
                            uint16_t *total_length);

/**
 * Parse instruction header for ofp_table_feature.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure
 *     @param[out]	instruction_list	A pointer to list of
 *     \e instruction structures.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OFP_ERROR Failed, ofp_error.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_instruction_header_parse(struct pbuf *pbuf,
                             struct instruction_list *instruction_list,
                             struct ofp_error *error);

/**
 * Encode instruction_list (header) for ofp_table_feature.
 *
 *     @param[out]	pbuf_list	A pointer to list of \e pbuf structures.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	instruction_list	A pointer to list of
 *     \e instruction structures.
 *     @param[out]	total_length	A pointer to \e size of packet.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE Failed, out of range.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_instruction_header_list_encode(struct pbuf_list *pbuf_list,
                                   struct pbuf **pbuf,
                                   struct instruction_list *instruction_list,
                                   uint16_t *total_length);

#endif /* __INSTRUCTION_H__ */
