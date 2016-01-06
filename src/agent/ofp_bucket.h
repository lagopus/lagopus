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
 * @file	ofp_bucket.h
 */

#ifndef __BUCKET_H__
#define __BUCKET_H__

#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_dp_apis.h"

/**
 * Alloc bucket.
 *
 *     @retval	*bucket	Succeeded, A pointer to \e bucket structure.
 *     @retval	NULL	Failed.
 */
struct bucket *
bucket_alloc(void);

/**
 * Trace bucket_list.
 *
 *     @param[in]	flag	Trace flags. Or'd value of TRACE_OFPT_*.
 *     @param[in]	bucket_list	A pointer to list of \e bucket structures.
 *
 *     @retval	void
 */
void
ofp_bucket_list_trace(uint32_t flags,
                      struct bucket_list *bucket_list);

/**
 * Free meter_band_stats_list elements.
 *
 *     @param[in]	band_stats_list	A pointer to list of
 *     \e meter_band_stats structures.
 *
 *     @retval	void
 */
void
ofp_bucket_list_free(struct bucket_list *bucket_list);

/**
 * Parse bucket.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[out]	bucket_list	A pointer to list of \e bucket structures.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OFP_ERROR Failed, ofp_error.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_bucket_parse(struct pbuf *pbuf,
                 struct bucket_list *bucket_list,
                 struct ofp_error *error);

/**
 * Encode bucket_list.
 *
 *     @param[out]	pbuf_list	A pointer to list of \e pbuf structures.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	bucket_list	A pointer to list of \e bucket structures.
 *     @param[out]	total_length	A pointer to \e size of packet.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE Failed, out of range.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_bucket_list_encode(struct pbuf_list *pbuf_list,
                       struct pbuf **pbuf,
                       struct bucket_list *bucket_list,
                       uint16_t *total_length);

#endif /* __BUCKET_H__ */
