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
 * @file	ofp_band_stats.h
 */

#ifndef __BAND_STATS_H__
#define __BAND_STATS_H__

#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_dp_apis.h"

/**
 * Alloc band stats.
 *
 *     @retval	*meter_band_stats	Succeeded, A pointer to
 *     \e meter_band_stats structure.
 *     @retval	NULL	Failed.
 */
struct meter_band_stats *
meter_band_stats_alloc(void);

/**
 * Free meter_band_stats_list elements.
 *
 *     @param[in]	band_stats_list	A pointer to list of
 *     \e meter_band_stats structures.
 *
 *     @retval	void
 */
void
meter_band_stats_list_elem_free(struct meter_band_stats_list
                                *band_stats_list);

/**
 * Encode ofp_meter_band_stats_list.
 *
 *     @param[out]	pbuf_list	A pointer to list of \e pbuf structures.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	band_stats_list	A pointer to list of
 *     \e meter_band_stats structures.
 *     @param[out]	total_length	A pointer to \e size of actions.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE Failed, out of range.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_meter_band_stats_list_encode(struct pbuf_list *pbuf_list,
                                 struct pbuf **pbuf,
                                 struct meter_band_stats_list *band_stats_list,
                                 uint16_t *total_length);

#endif /* __BAND_STATS_H__ */
