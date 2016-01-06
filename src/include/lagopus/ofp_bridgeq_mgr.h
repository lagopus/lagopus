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
 * @file	ofp_bridgeq_mgr.h
 */

#ifndef __LAGOPUS_OFP_BRIDGEQ_MGR_H__
#define __LAGOPUS_OFP_BRIDGEQ_MGR_H__

#include "lagopus/datastore.h"

#define MAX_BRIDGES 512

/**
 * @brief	ofp_bridgeq_polls_type
 * @details	Type of polls.
 */
enum ofp_bridgeq_polls_type {
  AGENT_POLLS,
  DP_POLLS,
};

/**
 * @brief	ofp_bridgeq_agent_poll_type
 * @details	Type of polls in ofp_bridgeq for agent.
 */
enum ofp_bridgeq_agent_poll_type {
  EVENTQ_POLL,
  DATAQ_POLL,
#ifdef OFPH_POLL_WRITING
  EVENT_DATAQ_POLL,
#endif  /* OFPH_POLL_WRITING */

  MAX_BRIDGE_POLLS
};

/**
 * @brief	ofp_bridgeq_dp_poll_type
 * @details	Type of polls in ofp_bridgeq for DataPlane.
 */
enum ofp_bridgeq_dp_poll_type {
  EVENT_DATAQ_DP_POLL,

  MAX_BRIDGE_DP_POLLS
};

#define MAX_POLLS (MAX_BRIDGES * MAX_BRIDGE_POLLS)
#define MAX_DP_POLLS (MAX_BRIDGES * MAX_BRIDGE_DP_POLLS)

struct ofp_bridgeq;

/**
 * Init bridgeq_mgr.
 *
 *     @param[in]	arg	arg
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
void
ofp_bridgeq_mgr_initialize(void *arg);

/**
 * Clear hash table in bridgeq_mgr.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_clear(void);

/**
 * Destroy bridgeq_mgr.
 *
 *     @retval	void
 */
void
ofp_bridgeq_mgr_destroy(void);

/**
 * Register bridge.
 *
 *     @param[in]	dpid	dpid
 *     @param[in]	name	A bridge name.
 *     @param[in]	info	A pointer to \e datastore_bridge_info_t.
 *     @param[in]	q_info	A pointer to \e datastore_bridge_queue_info_t.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ALREADY_EXISTS	Failed, already exists.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_bridge_register(uint64_t dpid,
                                const char *name,
                                datastore_bridge_info_t *info,
                                datastore_bridge_queue_info_t *q_info);

/**
 * Unregister bridge.
 *
 *     @param[in]	dpid	dpid
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_bridge_unregister(uint64_t dpid);

/**
 * Get bridgeq.
 *
 *     @param[in]	dpid	dpid
 *     @param[out]	bridgeq	A pointer to \e ofp_bridgeq structure.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_bridge_lookup(uint64_t dpid,
                              struct ofp_bridgeq **bridgeq);

/**
 * Get array of bridgeqs.
 *
 *     @param[out]	brqs	A pointer to array of
 *     \e ofp_bridgeq structures.
 *     @param[in,out]	count	A pointer to counter.
 *     @param[in]	max_size	Max size of brqs.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_bridgeqs_to_array(struct ofp_bridgeq *brqs[],
                                  uint64_t *count,
                                  const size_t max_size);

/**
 * Get array of polls.
 *
 *     @param[out]	polls	A pointer to array of
 *     \e lagopus_qmuxer_poll_t structures.
 *     @param[in]	brqs A pointer to array of
 *     \e ofp_bridgeq structures.
 *     @param[in,out]	idx	A pointer to index for polls.
 *     @param[in]	bridgeqs_size	Size of brqs.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_polls_get(lagopus_qmuxer_poll_t *polls,
                          struct ofp_bridgeq *brqs[],
                          uint64_t *idx,
                          const uint64_t bridgeqs_size);

/**
 * Get array of polls for DataPlane.
 *
 *     @param[out]	polls	A pointer to array of
 *     \e lagopus_qmuxer_poll_t structures.
 *     @param[in]	brqs A pointer to array of
 *     \e ofp_bridgeq structures.
 *     @param[in,out]	idx	A pointer to index for polls.
 *     @param[in]	bridgeqs_size	Size of brqs.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_dp_polls_get(lagopus_qmuxer_poll_t *polls,
                             struct ofp_bridgeq *brqs[],
                             uint64_t *idx,
                             const uint64_t bridgeqs_size);

/**
 * Free bridgeq.
 *
 *     @param[in]	brqs	A pointer to  \e ofp_bridgeq structure.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
void
ofp_bridgeq_mgr_bridgeq_free(struct ofp_bridgeq *brqs);

/**
 * Free bridgeqs.
 *
 *     @param[in]	brqs	A pointer to array of
 *     \e ofp_bridgeq structures.
 *     @param[in]	bridgeqs_size	Size of brqs.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
void
ofp_bridgeq_mgr_bridgeqs_free(struct ofp_bridgeq *brqs[],
                              const uint64_t num);

/**
 * Inclement reference counter in bridgeq.
 *
 *     @param[in]	bridgeq	A pointer to \e ofp_bridgeq structure.
 *
 *     @retval	void
 */
void
ofp_bridgeq_refs_get(struct ofp_bridgeq *bridgeq);

/**
 * Decrement reference counter in bridgeq.
 *
 *     @param[in]	bridgeq	A pointer to \e ofp_bridgeq structure.
 *
 *     @retval	void
 */
void
ofp_bridgeq_refs_put(struct ofp_bridgeq *bridgeq);

/**
 * Get ofp_bridge in bridgeq.
 *
 *     @param[in]	bridgeq	A pointer to \e ofp_bridgeq structure.
 *
 *     @retval	ofp_bridge
 */
struct ofp_bridge *
ofp_bridgeq_mgr_bridge_get(struct ofp_bridgeq *bridgeq);

/**
 * Free ofp_bridgeq.
 *
 *     @param[in]	bridgeq	A pointer to \e ofp_bridgeq structure.
 *     @param[in]	force	true/false.
 *
 *     @retval	ofp_bridge
 */
void
ofp_bridgeq_free(struct ofp_bridgeq *bridgeq,
                 bool force);

/**
 * Get eventq_poll in bridgeq.
 *
 *     @param[in]	bridgeq	A pointer to \e ofp_bridgeq structure.
 *
 *     @retval	ofp_bridge
 */
lagopus_qmuxer_poll_t
ofp_bridgeq_mgr_eventq_poll_get(struct ofp_bridgeq *bridgeq);

/**
 * Get dataq_poll in bridgeq.
 *
 *     @param[in]	bridgeq	A pointer to \e ofp_bridgeq structure.
 *
 *     @retval	ofp_bridge
 */
lagopus_qmuxer_poll_t
ofp_bridgeq_mgr_dataq_poll_get(struct ofp_bridgeq *bridgeq);

#ifdef OFPH_POLL_WRITING
/**
 * Get event_dataq_poll in bridgeq.
 *
 *     @param[in]	bridgeq	A pointer to \e ofp_bridgeq structure.
 *
 *     @retval	ofp_bridge
 */
lagopus_qmuxer_poll_t
ofp_bridgeq_mgr_event_dataq_poll_get(struct ofp_bridgeq *bridgeq);
#endif

/**
 * Get event_dataq_poll in bridgeq for DataPlane.
 *
 *     @param[in]	bridgeq	A pointer to \e ofp_bridgeq structure.
 *
 *     @retval	ofp_bridge
 */
lagopus_qmuxer_poll_t
ofp_bridgeq_mgr_event_dataq_dp_poll_get(struct ofp_bridgeq *bridgeq);

/**
 * Reset list of poll.
 *
 *     @param[in]	polls	A pointer to polls.
 *     @param[in]	size	Size of polls.
 *
 *     @retval	ofp_bridge
 */
void
ofp_bridgeq_mgr_poll_reset(lagopus_qmuxer_poll_t *polls,
                           const size_t size);

/**
 * Set dataq_max_batches.
 *
 *     @param[in]	dpid	dpid
 *     @param[in]	val	val.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_NOT_FOUND	Failed, Not found.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_dataq_max_batches_set(uint64_t dpid,
                                      uint16_t val);

/**
 * Set eventq_max_batches.
 *
 *     @param[in]	dpid	dpid
 *     @param[in]	val	val.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_NOT_FOUND	Failed, Not found.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_eventq_max_batches_set(uint64_t dpid,
                                       uint16_t val);

/**
 * Set event_dataq_max_batches.
 *
 *     @param[in]	dpid	dpid
 *     @param[in]	val	val.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_NOT_FOUND	Failed, Not found.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_event_dataq_max_batches_set(uint64_t dpid,
                                            uint16_t val);

/**
 * Get stats.
 *
 *     @param[in]	dpid	dpid
 *     @param[in]	q_info	A pointer to \e datastore_bridge_stats_t.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_stats_get(uint64_t dpid,
                          datastore_bridge_stats_t *q_stats);

/**
 * Clear dataq.
 *
 *     @param[in]	dpid	dpid
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_NOT_FOUND	Failed, Not found.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_dataq_clear(uint64_t dpid);

/**
 * Clear eventq.
 *
 *     @param[in]	dpid	dpid
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_NOT_FOUND	Failed, Not found.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_eventq_clear(uint64_t dpid);

/**
 * Clear event_dataq.
 *
 *     @param[in]	dpid	dpid
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_NOT_FOUND	Failed, Not found.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_event_dataq_clear(uint64_t dpid);

/**
 * Get bridge name.
 *
 *     @param[in]	dpid	dpid
 *     @param[in]	name	A pointer to bridge name.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_name_get(uint64_t dpid,
                         char **name);

/**
 * Get bridge name.
 *
 *     @param[in]	dpid	dpid
 *     @param[in]	name	A pointer to bridge name.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES Failed.
 */
lagopus_result_t
ofp_bridgeq_mgr_info_get(uint64_t dpid,
                         datastore_bridge_info_t *info);

#endif /* __LAGOPUS_OFP_BRIDGEQ_MGR_H__ */
