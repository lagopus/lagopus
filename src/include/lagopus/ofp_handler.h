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
 * @file	ofp_handler.h
 * @brief	OpenFlow Packet handler
 * @details	Handling OpenFlow Packet
 */
#ifndef __LAGOPUS_OFP_HANDLER_H__
#define __LAGOPUS_OFP_HANDLER_H__

#include "lagopus/ofp_bridge.h"
#include "lagopus_gstate.h"
#include "lagopus/pbuf.h"

struct ofp_handler_record;
struct channel;

/**
 * Create ofp_handler
 */
lagopus_result_t
ofp_handler_initialize(void *arg,
                       lagopus_thread_t **retptr);

/**
 * Start ofp_handler
 */
lagopus_result_t
ofp_handler_start(void);

/**
 * Shutdown ofp_handler (graceful)
 */
lagopus_result_t
ofp_handler_shutdown(shutdown_grace_level_t level);

/**
 * Shutdown ofp_handler (force)
 */
lagopus_result_t
ofp_handler_stop(void);

/**
 * Destroy ofp_handler
 */
void
ofp_handler_finalize(void);

/**
 * get channelq from ofp-handler
 */
lagopus_result_t
ofp_handler_get_channelq(lagopus_bbq_t **retptr);

/**
 * put eventq_data for event_dataq
 */
lagopus_result_t
ofp_handler_event_dataq_put(uint64_t dpid,
                            struct eventq_data *ptr);

/**
 * register channel.
 */
lagopus_result_t
ofp_handler_register_cahnnel(uint64_t dpid, struct channel *channel);

/**
 * Put data to dataq.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	dataq	A pointer to \e eventq_data structure.
 *     @param[in]	A wait time (in nsec).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.

 */
lagopus_result_t
ofp_handler_dataq_data_put(uint64_t dpid,
                           struct eventq_data **data,
                           lagopus_chrono_t timeout);

/**
 * Put data to eventq.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	dataq	A pointer to \e eventq_data structure.
 *     @param[in]	A wait time (in nsec).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_NOT_FOUND	Not found dpid.
 *     @retval	LAGOPUS_RESULT_TIMEDOUT	Timedout.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_handler_eventq_data_put(uint64_t dpid,
                            struct eventq_data **data,
                            lagopus_chrono_t timeout);

/**
 * Get data event_dataq.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	dataq	A pointer to \e eventq_data structure.
 *     @param[in]	A wait time (in nsec).
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_NOT_FOUND	Not found dpid.
 *     @retval	LAGOPUS_RESULT_TIMEDOUT	Timedout.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_handler_event_dataq_data_get(uint64_t dpid,
                                 struct eventq_data **data,
                                 lagopus_chrono_t timeout);
/**
 * Returns \b true if the bounded blocking queue is operational.
 *
 *     @param[in]	bbq	A pointer to \e BBQ.
 *
 *     @retval	true	Operational.
 *     @retval	false	Not operational.
 */
bool
ofp_handler_validate_bbq(lagopus_bbq_t *bbq);

/**
 * Set channelq_size.
 *
 *     @param[in]	val	val
 *
 *     @retval	void
 */
void
ofp_handler_channelq_size_set(uint16_t val);

/**
 * Set channelq_max_batches.
 *
 *     @param[in]	val	val
 *
 *     @retval	void
 */
void
ofp_handler_channelq_max_batches_set(uint16_t val);

/**
 * Get channelq_size.
 *
 *     @retval	channelq_size
 */
uint16_t
ofp_handler_channelq_size_get(void);

/**
 * Get channelq_max_batches.
 *
 *     @retval	channelq_max_batches
 */
uint16_t
ofp_handler_channelq_max_batches_get(void);

/**
 * Get channelq stats.
 *
 *     @param[out]	val	val
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_OBJECT	Failed, invalid object
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_handler_channelq_stats_get(uint16_t *val);

#endif /* __LAGOPUS_OFP_HANDLER_H__ */
