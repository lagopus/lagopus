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

#ifndef __LAGOPUS_DATASTORE_QUEUE_H__
#define __LAGOPUS_DATASTORE_QUEUE_H__

#include "openflow.h"

typedef enum datastore_queue_type {
  DATASTORE_QUEUE_TYPE_UNKNOWN = 0,
  DATASTORE_QUEUE_TYPE_SINGLE_RATE,
  DATASTORE_QUEUE_TYPE_TWO_RATE,
  DATASTORE_QUEUE_TYPE_MIN = DATASTORE_QUEUE_TYPE_UNKNOWN,
  DATASTORE_QUEUE_TYPE_MAX = DATASTORE_QUEUE_TYPE_TWO_RATE,
} datastore_queue_type_t;

typedef enum datastore_queue_color {
  DATASTORE_QUEUE_COLOR_UNKNOWN = 0,
  DATASTORE_QUEUE_COLOR_AWARE,
  DATASTORE_QUEUE_COLOR_BLIND,
  DATASTORE_QUEUE_COLOR_MIN = DATASTORE_QUEUE_COLOR_UNKNOWN,
  DATASTORE_QUEUE_COLOR_MAX = DATASTORE_QUEUE_COLOR_BLIND,
} datastore_queue_color_t;

typedef struct datastore_queue_info {
  uint32_t id; /* TODO: delete. */
  uint16_t priority;
  datastore_queue_type_t type;
  datastore_queue_color_t color;
  uint64_t committed_burst_size;
  uint64_t committed_information_rate;
  uint64_t excess_burst_size;
  uint64_t peak_burst_size;
  uint64_t peak_information_rate;
} datastore_queue_info_t;

/**
 * @brief	datastore_queue_stats_list_t
 */
typedef struct ofp_queue_stats datastore_queue_stats_t;

/**
 * Get the value to attribute 'enabled' of the queue table record'
 *
 *  @param[in] name
 *  @param[out] enabled the value of attribute 'enabled'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'enabled' getted sucessfully.
 */
lagopus_result_t
datastore_queue_is_enabled(const char *name, bool *enabled);

/**
 * Get the value to attribute 'used' of the queue table record'
 *
 *  @param[in] name
 *  @param[out] used the value of attribute 'used'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'used' getted sucessfully.
 */
lagopus_result_t
datastore_queue_is_used(const char *name, bool *used);

/**
 * Get the value to attribute 'id' of the queue table record'
 *
 *  @param[in] name
 *  @param[out] id the value of attribute 'id'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'id' getted sucessfully.
 */
lagopus_result_t
datastore_queue_get_id(const char *name, bool current,
                       uint32_t *id);

/**
 * Get the value to attribute 'priority' of the queue table record'
 *
 *  @param[in] name
 *  @param[out] priority the value of attribute 'priority'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'priority' getted sucessfully.
 */
lagopus_result_t
datastore_queue_get_priority(const char *name, bool current,
                             uint16_t *priority);

/**
 * Get the value to attribute 'type' of the queue table record'
 *
 *  @param[in] name
 *  @param[out] type the value of attribute 'type'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'type' getted sucessfully.
 */
lagopus_result_t
datastore_queue_get_type(const char *name, bool current,
                         datastore_queue_type_t *type);

/**
 * Get the value to attribute 'color' of the queue table record'
 *
 *  @param[in] name
 *  @param[out] color the value of attribute 'color'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'color' getted sucessfully.
 */
lagopus_result_t
datastore_queue_get_color(const char *name, bool current,
                          datastore_queue_color_t *color);

/**
 * Get the value to attribute 'committed_burst_size' of the queue table record'
 *
 *  @param[in] name
 *  @param[out] val the value of attribute 'committed_burst_size'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'committed_burst_size' getted sucessfully.
 */
lagopus_result_t
datastore_queue_get_committed_burst_size(const char *name,
                                         bool current,
                                         uint64_t *val);

/**
 * Get the value to attribute 'committed_information_rate' of the queue table record'
 *
 *  @param[in] name
 *  @param[out] val the value of attribute 'committed_information_rate'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'committed_information_rate' getted sucessfully.
 */
lagopus_result_t
datastore_queue_get_committed_information_rate(const char *name,
                                               bool current,
                                               uint64_t *val);

/**
 * Get the value to attribute 'excess_burst_size' of the queue table record'
 *
 *  @param[in] name
 *  @param[out] val the value of attribute 'excess_burst_size'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'excess_burst_size' getted sucessfully.
 */
lagopus_result_t
datastore_queue_get_excess_burst_size(const char *name,
                                      bool current,
                                      uint64_t *val);

/**
 * Get the value to attribute 'peak_burst_size' of the queue table record'
 *
 *  @param[in] name
 *  @param[out] val the value of attribute 'peak_burst_size'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'peak_burst_size' getted sucessfully.
 */
lagopus_result_t
datastore_queue_get_peak_burst_size(const char *name,
                                    bool current,
                                    uint64_t *val);

/**
 * Get the value to attribute 'peak_information_rate' of the queue table record'
 *
 *  @param[in] name
 *  @param[out] val the value of attribute 'peak_information_rate'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'peak_information_rate' getted sucessfully.
 */
lagopus_result_t
datastore_queue_get_peak_information_rate(const char *name,
                                          bool current,
                                          uint64_t *val);

#endif /* ! __LAGOPUS_DATASTORE_QUEUE_H__ */

