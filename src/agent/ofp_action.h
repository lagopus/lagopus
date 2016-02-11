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
 * @file	ofp_action.h
 */

#ifndef __ACTION_H__
#define __ACTION_H__

/**
 * Alloc Encap/Decap property.
 *
 *     @retval	*ed_prop	Succeeded, A pointer to \e ed_prop structure.
 *     @retval	NULL	Failed.
 */
struct ed_prop *
ed_prop_alloc(void);

/**
 * Free Encap/Decap properties.
 *
 *     @param[in]	ed_prop_list	List of Encap/Decap property.
 *
 *     @retval	void
 */
void
ed_prop_list_free(struct ed_prop_list *ed_prop_list);

/**
 * Alloc action.
 *
 *     @param[in]	size	Size of body in action.
 *
 *     @retval	*action	Succeeded, A pointer to \e action structure.
 *     @retval	NULL	Failed.
 */
struct action *
action_alloc(uint16_t size);

/**
 * Free action.
 *
 *     @param[in]	action	A pointer to \e action structure.
 *
 *     @retval	void
 */
void
action_free(struct action *action);

/**
 * Trace action list.
 *
 *     @param[in]	flag	Trace flags. Or'd value of TRACE_OFPT_*.
 *     @param[in]	action_list	A pointer to list of \e action structures.
 *
 *     @retval	void
 */
void
ofp_action_list_trace(uint32_t flags,
                      struct action_list *action_list);

/**
 * Parse action.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	action_length	Size of action.
 *     @param[out]	action_list	A pointer to list of \e action structures.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OFP_ERROR Failed, ofp_error.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_action_parse(struct pbuf *pbuf, size_t action_length,
                 struct action_list *action_list,
                 struct ofp_error *error);

/**
 * Encode action list.
 *
 *     @param[out]	pbuf_list	A pointer to list of \e pbuf structures.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	action_list	A pointer to list of \e action structures.
 *     @param[out]	total_length	A pointer to \e size of packet.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE Failed, out of range.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_action_list_encode(struct pbuf_list *pbuf_list,
                       struct pbuf **pbuf,
                       struct action_list *action_list,
                       uint16_t *total_length);

/**
 * Parse action header for ofp_table_feature.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure
 *     @param[in]	action_length	Size of action.
 *     @param[out]	action_list	A pointer to list of \e action structures.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OFP_ERROR Failed, ofp_error.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_action_header_parse(struct pbuf *pbuf, size_t action_length,
                        struct action_list *action_list,
                        struct ofp_error *error);

/**
 * Encode action header for ofp_table_feature.
 *
 *     @param[out]	pbuf_list	A pointer to list of \e pbuf structures.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	action_list	A pointer to list of \e action structures.
 *     @param[out]	total_length	A pointer to \e size of packet.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE Failed, out of range.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_action_header_list_encode(struct pbuf_list *pbuf_list,
                              struct pbuf **pbuf,
                              struct action_list *action_list,
                              uint16_t *total_length);

#endif /* __ACTION_H__ */
