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
 * @file        group.h
 * @brief       OpenFlow Group.
 */

#ifndef SRC_INCLUDE_LAGOPUS_GROUP_H_
#define SRC_INCLUDE_LAGOPUS_GROUP_H_

#include "ofp_dp_apis.h"

struct group_stats_list;
struct group_desc_list;

/**
 * @brief Group structure.
 */
struct group {                          /** Internal group structure. */
  uint32_t id;                          /** OpenFlow group id. */
  enum ofp_group_type type;             /** Group type. */
  struct bucket_list bucket_list;       /** List of goup bucket */
  int select;                           /** Round-robin index
                                         ** for OFPGT_SELECT */
  uint64_t packet_count;                /** Packet count. */
  uint64_t byte_count;                  /** Byte count. */
  uint32_t duration_sec;                /** Duration (sec part) */
  uint32_t duration_nsec;               /** Duration (nano sec part */
  struct timespec create_time;          /** Creation time. */
  lagopus_hashmap_t flows;              /** Relationship with flow entries */
  struct group_table *group_table;      /** Relationship with group taable. */
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
 *
 * @param[in]   group_mod       OpenFlow group_mod structure.
 * @param[in]   bucket_list     List of bucket for allocated group.
 *
 * @retval      !=NULL          New group structure.
 * @retval      ==NULL          Memory exhausted.
 *
 * group_alloc is for internal use.
 */
struct group *
group_alloc(struct ofp_group_mod *group_mod, struct bucket_list *bucket_list);

/**
 * Free group.
 *
 * @param[in]   group   Internal group structure.
 *
 * group_free is for internal use.
 */
void
group_free(struct group *group);

/**
 * Modify group.
 *
 * @param[in]   group           Group structure.
 * @param[in]   group_mod       OpenFlow group_mod structure.
 * @param[in]   bucket_list     List of backet for modified group.
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
 *
 * @param[in]   group_table     Group table that inclues all groups.
 * @param[in]   request         Group statistics request includes group id.
 * @param[out]  list            Group statistics for each group.
 * @param[out]  error           Error indicated if request is invalid.
 *
 * @retval      LAGOPUS_RESULT_OK       Success.
 */
lagopus_result_t
group_stats(struct group_table *group_table,
            struct ofp_group_stats_request *request,
            struct group_stats_list *list, struct ofp_error *error);

/**
 * Get group description.
 *
 * @param[in]   group_table     Group table that inclues all groups.
 * @param[out]  group_desc_list Group description for each group.
 * @param[out]  error           Error indicated if request is invalid.
 *
 * @retval      LAGOPUS_RESULT_OK       Success.
 */
lagopus_result_t
group_descs(struct group_table *group_table,
            struct group_desc_list *group_desc_list,
            struct ofp_error *error);

/**
 * Get group features.
 *
 * @param[in]   group_table     Group table that inclues all groups.
 * @param[out]  features        Group features.
 * @param[out]  error           Error indicated if request is invalid.
 *
 * @retval      LAGOPUS_RESULT_OK       Success.
 */
lagopus_result_t
get_group_features(struct group_table *group_table,
                   struct ofp_group_features *features,
                   struct ofp_error *error);

/**
 * Get live bucket.
 *
 * @param[in]   bridge  Bridge.
 * @param[in]   group   Group.
 *
 * @retval      !=NULL  Selected live bucket in the group.
 * @retval      ==NULL  Live bufket is not found.
 */
struct bucket *
group_live_bucket(struct bridge *bridge,
                  struct group *group);

#endif /* SRC_INCLUDE_LAGOPUS_GROUP_H_ */
