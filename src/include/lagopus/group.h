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
 * @file        group.h
 * @brief       OpenFlow Group.
 */

#ifndef SRC_INCLUDE_LAGOPUS_GROUP_H_
#define SRC_INCLUDE_LAGOPUS_GROUP_H_

#include "ofp_dp_apis.h"

struct group {
  /* Group id. */
  uint32_t id;

  /* Group type. */
  enum ofp_group_type type;

  /* Bucket. */
  struct bucket_list bucket_list;

  /* Round-robin index for OFPGT_SELECT */
  int select;

  /* Statistics. */
  uint64_t packet_count;
  uint64_t byte_count;
  uint32_t duration_sec;
  uint32_t duration_nsec;

  /* Creation time. */
  struct timespec create_time;

  /* Relationship with flow entries */
  struct vector *flows;

  /* Relationship with group taable. */
  struct group_table *group_table;
};

/**
 * Allocate a new group table.
 *
 * @param[in]   bridge  Parent bridge.
 *
 * @retval      !=NULL  Allocated group table.
 * @retval      ==NULL  Memory exhausted.
 */
struct group_table *
group_table_alloc(struct bridge *);

/**
 * Free group table.
 *
 * @param[in]   group_table     Group table to be freed.
 */
void
group_table_free(struct group_table *group_table);

/**
 * Lookup group.
 *
 * @param[in]   group_table     Group table.
 * @param[in]   id              Group ID.
 *
 * @retval      !=NULL  Group.
 * @retval      ==NULL  Group is not found.
 */
struct group *
group_table_lookup(struct group_table *group_table, uint32_t id);

/**
 * Add group.
 *
 * @param[in]   group_table     Group table.
 * @param[in]   group           Group.
 * @param[out]  error           Error information.
 *
 * @retval      LAGOPUS_RESULT_OK               Success.
 * @retval      LAGOPUS_RESULT_OFP_ERROR        Detected loop.
 * @retval      LAGOPUS_RESULT_NO_MEMORY        Memory exhausted.
 */
lagopus_result_t
group_table_add(struct group_table *group_table,
                struct group *group,
                struct ofp_error *error);

/**
 * Delete group.
 *
 * @param[in]   group_table     Group table.
 * @param[in]   id              Group ID.
 *
 * @retval      LAGOPUS_RESULT_OK               Success.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Group is not exist.
 */
lagopus_result_t
group_table_delete(struct group_table *group_table, uint32_t group_id);

/**
 * Alloc group.
 */
struct group *
group_alloc(struct ofp_group_mod *group_mod, struct bucket_list *bucket_list);

/**
 * Free group.
 */
void
group_free(struct group *group);

/**
 * Modify group.
 */
void
group_modify(struct group *group, struct ofp_group_mod *group_mod,
             struct bucket_list *bucket_list);

/**
 */
void group_add_ref_flow(struct group *group, struct flow *flow);

/**
 */
void group_remove_ref_flow(struct group *group, struct flow *flow);

/**
 * Get group statistics.
 */
lagopus_result_t
group_stats(struct group_table *group_table,
            struct ofp_group_stats_request *request,
            struct group_stats_list *list, struct ofp_error *error);

/**
 * Get group description.
 */
lagopus_result_t
group_descs(struct group_table *group_table,
            struct group_desc_list *group_desc_list,
            struct ofp_error *error);

/**
 * Get group features.
 */
lagopus_result_t
get_group_features(struct group_table *group_table,
                   struct ofp_group_features *features,
                   struct ofp_error *error);

/**
 * Get live bucket.
 */
struct bucket *
group_live_bucket(struct bridge *bridge,
                  struct group *group);

#endif /* SRC_INCLUDE_LAGOPUS_GROUP_H_ */
