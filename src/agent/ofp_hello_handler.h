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
 * @file	ofp_hello_handler.h
 */

#ifndef __OFP_HELLO_HANDLER_H__
#define __OFP_HELLO_HANDLER_H__

#include "ofp_common.h"
#include "channel.h"

/**
 * ofp_hello handler.
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
ofp_hello_handle(struct channel *channel, struct pbuf *pbuf,
                 struct ofp_error *error);

/**
 * Send ofp_hello.
 *
 *     @param[in]	channel	A pointer to \e channel structure.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_hello_send(struct channel *channel);

#ifdef __UNIT_TESTING__
/**
 * Create ofp_hello.
 *
 *     @param[in]	channel	A pointer to \e channel structure.
 *     @param[out]	pbuf	A pointer to \e pbuf structure.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_hello_create(struct channel *channel, struct pbuf **pbuf);
#endif /* __UNIT_TESTING__ */

#endif /* __OFP_HELLO_HANDLER_H__ */
