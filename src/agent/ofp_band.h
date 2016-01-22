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
 * @file	ofp_band.h
 */

#ifndef __BAND_H__
#define __BAND_H__

#include "lagopus/meter.h"

/**
 * Trace band list.
 *
 *     @param[in]	flags	Trace flags. Or'd value of TRACE_OFPT_*.
 *     @param[in]	band_list	A pointer to list of \e meter_band structures.
 *
 *     @retval	void
 */
void
ofp_meter_band_list_trace(uint32_t flags,
                          struct meter_band_list *band_list);

/**
 * Parse drop.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	band_list	A pointer to list of \e meter_band structures.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OFP_ERROR Failed, ofp_error.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_meter_band_drop_parse(struct pbuf *pbuf,
                          struct meter_band_list *band_list,
                          struct ofp_error *error);

/**
 * Parse DSCP remark.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	band_list	A pointer to list of \e meter_band structures.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OFP_ERROR Failed, ofp_error.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_meter_band_dscp_remark_parse(struct pbuf *pbuf,
                                 struct meter_band_list *band_list,
                                 struct ofp_error *error);

/**
 * Parse Experimenter.
 *
 *     @param[in]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	band_list	A pointer to list of \e meter_band structures.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OFP_ERROR Failed, ofp_error.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_meter_band_experimenter_parse(struct pbuf *pbuf,
                                  struct meter_band_list *band_list,
                                  struct ofp_error *error);

/**
 * Encode band list.
 *
 *     @param[out]	pbuf_list	A pointer to list of \e pbuf structures.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	band_list	A pointer to list of \e meter_band structures.
 *     @param[out]	total_length	Size of packet.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_band_list_encode(struct pbuf_list *pbuf_list,
                     struct pbuf **pbuf,
                     struct meter_band_list *band_list,
                     uint16_t *total_length);

#endif /* __BAND_H__ */
