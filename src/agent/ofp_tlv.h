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
 * @file	ofp_tlv.h
 */

#ifndef __OFP_TLV_H__
#define __OFP_TLV_H__

/**
 * Set length in TLV.
 *
 *     @param[out]	tlv_head	A pointer to head of TLV.
 *     @param[in]	length	Size of packet.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_tlv_length_set(uint8_t *tlv_head,
                   uint16_t length);

/**
 * Sum length and check overflow(uint16_t).
 *
 *     @param[out]	total_length	A pointer to sum length.
 *     @param[in]	length	length.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE Failed, overflow.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_tlv_length_sum(uint16_t *total_length,
                   uint16_t length);

#endif /* __OFP_TLV__H__ */
