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
 * @file	ofp_bridge.h
 * @brief	Agent/Data-Plane connection
 * @details	Describe APIs between Agent and Data-Plane.
 */
#ifndef __LAGOPUS_OFP_BRIDGE_H__
#define __LAGOPUS_OFP_BRIDGE_H__

#include "lagopus/datastore.h"
#include "lagopus/eventq_data.h"

#ifdef OFPH_POLL_WRITING
/* decl edq_buffer */
STAILQ_HEAD(edq_buffer, edq_buffer_entry);
struct edq_buffer_entry {
  STAILQ_ENTRY(edq_buffer_entry) qentry;
  struct eventq_data *qdata;
};
#endif  /* OFPH_POLL_WRITING */

struct ofp_bridge {
  uint64_t dpid;
  const char *name;
  datastore_bridge_info_t info;
  volatile uint16_t dataq_max_batches;
  volatile uint16_t eventq_max_batches;
  volatile uint16_t event_dataq_max_batches;

  /* Data Queue */
  eventq_t dataq;
  /* Event Queue */
  eventq_t eventq;
  /* Event/Data Queue */
  eventq_t event_dataq;

#ifdef OFPH_POLL_WRITING
  struct edq_buffer edq_buffer;
#endif  /* OFPH_POLL_WRITING */
};

/**
 * create ofp_bridge
 */
lagopus_result_t
ofp_bridge_create(struct ofp_bridge **retptr,
                  uint64_t dpid,
                  const char *name,
                  datastore_bridge_info_t *info,
                  datastore_bridge_queue_info_t *q_info);

/**
 * shutdown ofp_bridge
 */
void
ofp_bridge_shutdown(struct ofp_bridge *ptr, bool is_canceled);

/**
 * destroy ofp_bridge
 */
void
ofp_bridge_destroy(struct ofp_bridge *ptr);

/**
 * Set dataq_max_batches.
 *
 *     @param[in]	ptr	A pointer to \e ofp_bridge.
 *     @param[in]	val	val.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridge_dataq_max_batches_set(struct ofp_bridge *ptr,
                                 uint16_t val);

/**
 * Set eventq_max_batches.
 *
 *     @param[in]	ptr	A pointer to \e ofp_bridge.
 *     @param[in]	val	val.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridge_eventq_max_batches_set(struct ofp_bridge *ptr,
                                  uint16_t val);

/**
 * Set event_dataq_max_batches.
 *
 *     @param[in]	ptr	A pointer to \e ofp_bridge.
 *     @param[in]	val	val.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridge_event_dataq_max_batches_set(struct ofp_bridge *ptr,
                                       uint16_t val);

/**
 * Get dataq_max_batches.
 *
 *     @param[in]	ptr	A pointer to \e ofp_bridge.
 *     @param[out]	val	A pointer to val.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridge_dataq_max_batches_get(struct ofp_bridge *ptr,
                                 uint16_t *val);

/**
 * Get eventq_max_batches;
 *
 *     @param[in]	ptr	A pointer to \e ofp_bridge.
 *     @param[out]	val	A pointer to val.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridge_eventq_max_batches_get(struct ofp_bridge *ptr,
                                  uint16_t *val);

/**
 * Get event_dataq_max_batches.
 *
 *     @param[in]	ptr	A pointer to \e ofp_bridge.
 *     @param[out]	val	A pointer to val.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridge_event_dataq_max_batches_get(struct ofp_bridge *ptr,
                                       uint16_t *val);

/**
 * Get dataq stats.
 *
 *     @param[in]	ptr	A pointer to \e ofp_bridge.
 *     @param[out]	val	A pointer to val.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridge_dataq_stats_get(struct ofp_bridge *ptr,
                           uint16_t *val);

/**
 * Get eventq stats.
 *
 *     @param[in]	ptr	A pointer to \e ofp_bridge.
 *     @param[out]	val	A pointer to val.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridge_eventq_stats_get(struct ofp_bridge *ptr,
                            uint16_t *val);

/**
 * Get event dataq stats.
 *
 *     @param[in]	ptr	A pointer to \e ofp_bridge.
 *     @param[out]	val	A pointer to val.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridge_event_dataq_stats_get(struct ofp_bridge *ptr,
                                 uint16_t *val);

/**
 * Clear dataq.
 *
 *     @param[in]	ptr	A pointer to \e ofp_bridge.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridge_dataq_clear(struct ofp_bridge *ptr);

/**
 * Clear eventq.
 *
 *     @param[in]	ptr	A pointer to \e ofp_bridge.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridge_eventq_clear(struct ofp_bridge *ptr);

/**
 * Clear event dataq.
 *
 *     @param[in]	ptr	A pointer to \e ofp_bridge.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridge_event_dataq_clear(struct ofp_bridge *ptr);

/**
 * Get bridge name.
 *
 *     @param[in]	ptr	A pointer to \e ofp_bridge.
 *     @param[out]	name	A pointer to name.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridge_name_get(struct ofp_bridge *ptr,
                    char **name);

/**
 * Get bridge info.
 *
 *     @param[in]	ptr	A pointer to \e ofp_bridge.
 *     @param[out]	info	A pointer to \e datastore_bridge_info_t.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 */
lagopus_result_t
ofp_bridge_info_get(struct ofp_bridge *ptr,
                    datastore_bridge_info_t *info);

#endif /* __LAGOPUS_OFP_BRIDGE_H__ */
