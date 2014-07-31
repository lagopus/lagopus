/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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
 * @file	meter.h
 * @brief	OpenFlow meter.
 */

#ifndef __LAGOPUS_METER_H__
#define __LAGOPUS_METER_H__

#include "lagopus_config.h"
#include "lagopus_apis.h"
#include "lagopus/ptree.h"
#include "openflow.h"

#ifdef HAVE_BSD_SYS_QUEUE_H
#include <bsd/sys/queue.h>
#elif HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

struct meter_stats_list;
struct meter_config_list;

#define LAGOPUS_METER_MAX_BANDS	16

/* Meter bands. */
struct meter_band {
  TAILQ_ENTRY(meter_band) entry;

  /* One of the OFPMBT_*: drop packet, remark DSCP or experimental. */
  uint16_t type;

  /* Length in bytes of this band. */
  uint16_t len;

  /* Rate for this band. */
  uint32_t rate;

  /* Size of bursts. */
  uint32_t burst_size;

  /* Number of drop precedence level to add.  Only used by
   * OFPMBT_DSCP_REMARK. */
  uint8_t prec_level;

  /* Experimenter. */
  uint32_t experimenter;

  /* Counters. */
  struct ofp_meter_band_stats stats;
};

/* Meter band list. */
TAILQ_HEAD(meter_band_list, meter_band);

/* Meter. */
struct meter {
  /* Meter id. */
  uint32_t meter_id;

  /* Flags defined by ofp_meter_flags. */
  uint16_t flags;

  /* Unordered list of meter band. */
  struct meter_band_list band_list;

  /* Counters. */
  uint32_t flow_count;
  uint64_t input_packet_count;
  uint64_t input_byte_count;
  uint32_t duration_sec;
  uint32_t duration_nanosec;

  /* Creation time. */
  struct timespec create_time;

  /* Driver specific data */
  void *driverdata;
};

/* Meter table. */
struct meter_table {
  /* Read-write lock. */
  pthread_rwlock_t rwlock;

  /* Meter id tree. */
  struct ptree *ptree;
};

/**
 * Allocate meter table.
 *
 * @retval	!=NULL		Meter table.
 * @retval	==NULL		Memory exhausted.
 */
struct meter_table *
meter_table_alloc(void);

/**
 * Free meter table.
 *
 * @param[in]	meter_table	Metar table.
 */
void
meter_table_free(struct meter_table *meter_table);

/**
 * Read lock meter table.
 *
 * @param[in]	meter_table	Metar table.
 */
int
meter_table_rdlock(struct meter_table *meter_table);

/**
 * Write lock meter table.
 *
 * @param[in]	meter_table	Metar table.
 */
int
meter_table_wrlock(struct meter_table *meter_table);

/**
 * Unlock meter table.
 *
 * @param[in]	meter_table	Metar table.
 */
int
meter_table_unlock(struct meter_table *meter_table);

/**
 * Add meter.
 *
 * @param[in]	meter_table	Metar table.
 * @param[in]	mod		Meter modificaiton message.
 * @param[in]	band_list	Meter band.
 * @param[out]	error		Error information.
 *
 * @retval	LAGOPUS_RESULT_OK	Success.
 */
lagopus_result_t
meter_table_meter_add(struct meter_table *meter_table,
                      struct ofp_meter_mod *mod,
                      struct meter_band_list *band_list,
                      struct ofp_error *error);

/**
 * Modify meter.
 *
 * @param[in]	meter_table	Metar table.
 * @param[in]	mod		Meter modificaiton message.
 * @param[in]	band_list	Meter band.
 * @param[out]	error		Error information.
 *
 * @retval	LAGOPUS_RESULT_OK	Success.
 */
lagopus_result_t
meter_table_meter_modify(struct meter_table *meter_table,
                         struct ofp_meter_mod *mod,
                         struct meter_band_list *band_list,
                         struct ofp_error *error);

/**
 * Delete meter.
 */
lagopus_result_t
meter_table_meter_delete(struct meter_table *meter_table,
                         struct ofp_meter_mod *mod,
                         struct ofp_error *error);

/**
 * Lookup meter.
 */
struct meter *
meter_table_lookup(struct meter_table *meter_table, uint32_t meter_id);

/**
 * Allocate meter band.
 */
struct meter_band *
meter_band_alloc(struct ofp_meter_band_header *band);

/**
 * Free meter band.
 */
void
meter_band_free(struct meter_band *meter_band);

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
 * @param[in]	band_list	List of \e meter_band structures.
 *
 * @retval	void
 */
void
ofp_meter_band_list_elem_free(struct meter_band_list *band_list);

/**
 * Get meter statistics.
 */
lagopus_result_t
get_meter_stats(struct meter_table *meter_table,
                struct ofp_meter_multipart_request *request,
                struct meter_stats_list *list,
                struct ofp_error *error);

/**
 * Get meter config.
 */
lagopus_result_t
get_meter_config(struct meter_table *meter_table,
                 struct ofp_meter_multipart_request *request,
                 struct meter_config_list *list,
                 struct ofp_error *error);

/**
 * Get meter features.
 */
lagopus_result_t
get_meter_features(struct meter_table *meter_table,
                   struct ofp_meter_features *features,
                   struct ofp_error *error);

#endif /* __LAGOPUS_METER_H__ */
