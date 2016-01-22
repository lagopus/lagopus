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
 * @file        meter.h
 * @brief       OpenFlow meter.
 */

#ifndef SRC_INCLUDE_LAGOPUS_METER_H_
#define SRC_INCLUDE_LAGOPUS_METER_H_

#include "lagopus_config.h"
#include "lagopus_apis.h"
#include "openflow.h"

#ifdef HAVE_BSD_SYS_QUEUE_H
#include <bsd/sys/queue.h>
#elif HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

struct meter_stats_list;
struct meter_config_list;

#define LAGOPUS_METER_MAX_BANDS 16

/**
 * @brief Meter bands.
 */
struct meter_band {
  TAILQ_ENTRY(meter_band) entry;        /** Link for next band */
  uint16_t type;                        /** Band type. */
  uint16_t len;                         /** Length in bytes of this band. */
  uint32_t rate;                        /** Rate for this band. */
  uint32_t burst_size;                  /** Burst size. */
  uint8_t prec_level;                   /** Number of drop precedence level
                                         ** to add.  Only used by
                                         ** OFPMBT_DSCP_REMARK. */
  uint32_t experimenter;                /** Experimenter. */
  struct ofp_meter_band_stats stats;    /** Counters. */
};

TAILQ_HEAD(meter_band_list, meter_band);        /** Meter band list. */

/**
 * @brief Meter structure.
 */
struct meter {
  uint32_t meter_id;                    /** OpenFlow meter id. */
  uint16_t flags;                       /** ofp_meter_flags. */
  struct meter_band_list band_list;     /** Unordered list of meter band. */
  uint32_t flow_count;                  /** Flow count. */
  uint64_t input_packet_count;          /** Input packet count. */
  uint64_t input_byte_count;            /** Input byte count. */
  uint32_t duration_sec;                /** Duration (sec part) */
  uint32_t duration_nanosec;            /** Duration (nanosec part) */
  struct timespec create_time;          /** Creation time. */
  void *driverdata;                     /** Driver specific data */
};

struct meter_table;

/**
 * Allocate meter table.
 *
 * @retval      !=NULL          Meter table.
 * @retval      ==NULL          Memory exhausted.
 */
struct meter_table *
meter_table_alloc(void);

/**
 * Free meter table.
 *
 * @param[in]   meter_table     Metar table.
 */
void
meter_table_free(struct meter_table *meter_table);

/**
 * Add meter.
 *
 * @param[in]   meter_table     Metar table.
 * @param[in]   mod             Meter modificaiton message.
 * @param[in]   band_list       Meter band.
 * @param[out]  error           Error information.
 *
 * @retval      LAGOPUS_RESULT_OK       Success.
 */
lagopus_result_t
meter_table_meter_add(struct meter_table *meter_table,
                      struct ofp_meter_mod *mod,
                      struct meter_band_list *band_list,
                      struct ofp_error *error);

/**
 * Modify meter.
 *
 * @param[in]   meter_table     Metar table.
 * @param[in]   mod             Meter modificaiton message.
 * @param[in]   band_list       Meter band.
 * @param[out]  error           Error information.
 *
 * @retval      LAGOPUS_RESULT_OK       Success.
 */
lagopus_result_t
meter_table_meter_modify(struct meter_table *meter_table,
                         struct ofp_meter_mod *mod,
                         struct meter_band_list *band_list,
                         struct ofp_error *error);

/**
 * Delete meter.
 *
 * @param[in]   meter_table     Metar table.
 * @param[in]   mod             Meter modificaiton message.
 * @param[out]  error           Error information.
 *
 * @retval      LAGOPUS_RESULT_OK       Success.
 */
lagopus_result_t
meter_table_meter_delete(struct meter_table *meter_table,
                         struct ofp_meter_mod *mod,
                         struct ofp_error *error);

/**
 * Lookup meter.
 *
 * @param[in]   meter_table     Metar table.
 * @param[in]   meter_id        Meter ID.
 *
 * @retval      !=NULL          Meter structure.
 * @retval      ==NULL          Meter is not found.
 */
struct meter *
meter_table_lookup(struct meter_table *meter_table, uint32_t meter_id);

/**
 * Allocate meter band.
 *
 * @param[in]   band    Meter band data.
 *
 * @retval      !=NULL  Meter band structure.
 * @retval      ==NULL  Memory exhausted.
 */
struct meter_band *
meter_band_alloc(struct ofp_meter_band_header *band);

/**
 * Free meter band.
 *
 * @param[in]   meter_band      Meter band structure.
 */
void
meter_band_free(struct meter_band *meter_band);

/**
 * Dump meter table (for debug)
 *
 * @param[in]   meter_table     Metar table.
 */
void
meter_table_dump(struct meter_table *meter_table);

/**
 * hook for registering meter.
 */
void (*lagopus_register_meter)(struct meter *);

/**
 * hook for unregistering meter.
 */
void (*lagopus_unregister_meter)(struct meter *);

/**
 * Free band list elements.
 *
 * @param[in]   band_list       List of \e meter_band structures.
 */
void
ofp_meter_band_list_elem_free(struct meter_band_list *band_list);

/**
 * Get meter statistics.
 *
 * @param[in]   meter_table     Metar table.
 * @param[in]   request         Request includes meter id.
 * @param[out]  list            Statistics for each meter.
 * @param[out]  error           Error indicated if invalid request.
 *
 * @retval      LAGOPUS_RESULT_OK       Success.
 */
lagopus_result_t
get_meter_stats(struct meter_table *meter_table,
                struct ofp_meter_multipart_request *request,
                struct meter_stats_list *list,
                struct ofp_error *error);

/**
 * Get meter config.
 *
 * @param[in]   meter_table     Metar table.
 * @param[in]   request         Request includes meter id.
 * @param[out]  list            Configuration for each meter.
 * @param[out]  error           Error indicated if invalid request.
 *
 * @retval      LAGOPUS_RESULT_OK       Success.
 */
lagopus_result_t
get_meter_config(struct meter_table *meter_table,
                 struct ofp_meter_multipart_request *request,
                 struct meter_config_list *list,
                 struct ofp_error *error);

/**
 * Get meter features.
 *
 * @param[in]   meter_table     Metar table.
 * @param[out]  features        Features.
 * @param[out]  error           Error indicated if invalid request.
 *
 * @retval      LAGOPUS_RESULT_OK       Success.
 */
lagopus_result_t
get_meter_features(struct meter_table *meter_table,
                   struct ofp_meter_features *features,
                   struct ofp_error *error);

#endif /* SRC_INCLUDE_LAGOPUS_METER_H_ */
