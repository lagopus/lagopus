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
 * @file	ofp_padding.h
 */

#ifndef __OFP_PADDIN_H__
#define __OFP_PADDIN_H__

#include "lagopus/pbuf.h"

// For 64 bit padding.
// See also "OpenFlow Switch Specification Version 1.3.2 (Wire Protocol 0x04)" p.81/82
#define GET_64BIT_PADDING_LENGTH(length) (uint16_t)((((length + 7) / 8 ) * 8 - length))

/**
 * Encode padding.
 *
 *     @param[out]	pbuf_list	A pointer to list of \e pbuf structures.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	length	Size of packet.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE Failed, out of range.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_padding_encode(struct pbuf_list *pbuf_list,
                   struct pbuf **pbuf, uint16_t *length);

/**
 * Add padding.
 *
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *     @param[in]	length	Size of padding.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_OUT_OF_RANGE Failed, out of range.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_padding_add(struct pbuf *pbuf,
                uint16_t pad_length);

#endif /* __OFP_PADDIN_H__ */
