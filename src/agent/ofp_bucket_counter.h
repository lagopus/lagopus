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
 * @file	ofp_bucket_counter.h
 */

#ifndef __BUCKET_COUNTER_H__
#define __BUCKET_COUNTER_H__

#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_dp_apis.h"

/**
 * Alloc bucket counter.
 *
 *     @retval	*bucket_counter	Succeeded,
 *     A pointer to \e bucket_counter structure.
 *     @retval	NULL	Failed.
 */
struct bucket_counter *
bucket_counter_alloc(void);

/**
 * Free bucket_counter_list elements.
 *
 *     @param[in]	bucket_counter_list	A pointer to list of
 *     \e bucket_counter structures.
 *
 *     @retval	void
 */
void
bucket_counter_list_elem_free(struct bucket_counter_list
                              *bucket_counter_list);

/**
 * Encode bucket_counter_list.
 *
 *     @param[out]	pbuf_list	A pointer to list of \e pbuf structures.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	bucket_counter_list	A pointer to list of
 *     \e bucket_counter structures.
 *     @param[out]	total_length	A pointer to \e size of packet.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE Failed, out of range.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_bucket_counter_list_encode(struct pbuf_list *pbuf_list,
                               struct pbuf **pbuf,
                               struct bucket_counter_list *bucket_counter_list,
                               uint16_t *total_length);

#endif /* __BUCKET_COUNTER_H__ */
