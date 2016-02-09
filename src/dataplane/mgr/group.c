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
 *      @file   group.c
 *      @brief  OpenFlow Group management.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "openflow.h"
#include "lagopus_apis.h"
#include "lagopus/flowdb.h"
#include "lagopus/ofp_handler.h"
#include "lagopus/port.h"
#include "lagopus/group.h"
#include "lagopus/dataplane.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/dp_apis.h"
#include "lagopus_types.h"
#include "lagopus_error.h"

#include "lock.h"

#define GROUP_ID_KEY_LEN   32

static struct bucket *
bucket_alloc(void) {
  struct bucket *bucket;

  bucket = (struct bucket *)
           calloc(1, sizeof(struct bucket));
  if (bucket != NULL) {
    TAILQ_INIT(&bucket->action_list);
  }

  return bucket;
}

static void
bucket_list_free(struct bucket_list *bucket_list) {
  struct bucket *bucket;

  while (TAILQ_EMPTY(bucket_list) == false) {
    bucket = TAILQ_FIRST(bucket_list);
    if (TAILQ_EMPTY(&bucket->action_list) == false) {
      ofp_action_list_elem_free(&bucket->action_list);
    }
    TAILQ_REMOVE(bucket_list, bucket, entry);
    free(bucket);
  }
}

struct ref_flow {
  struct flow *flow;
  struct bridge *bridge;
};

struct group_table {
  lagopus_hashmap_t hashmap;
  struct bridge *bridge;
};

static inline void
group_table_rdlock(struct group_table *group_table) {
  (void) group_table;

  FLOWDB_RWLOCK_RDLOCK();
}

static inline void
group_table_rdunlock(struct group_table *group_table) {
  (void) group_table;

  FLOWDB_RWLOCK_RDUNLOCK();
}

static inline void
group_table_wrlock(struct group_table *group_table) {
  (void) group_table;

  FLOWDB_UPDATE_BEGIN();
}

static inline void
group_table_wrunlock(struct group_table *group_table) {
  (void) group_table;

  FLOWDB_UPDATE_END();
}

struct group_table *
group_table_alloc(struct bridge *parent) {
  struct group_table *group_table;

  group_table = (struct group_table *)calloc(1, sizeof(struct group_table));
  if (group_table == NULL) {
    return NULL;
  }

  /* Group id 32 bit key. */
  if (lagopus_hashmap_create(&group_table->hashmap,
                             LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                             group_free) != LAGOPUS_RESULT_OK) {
    free(group_table);
    return NULL;
  }

  /* Reference parent bridge. */
  group_table->bridge = parent;

  return group_table;
}

void
group_table_free(struct group_table *group_table) {
  lagopus_hashmap_destroy(&group_table->hashmap, true);
  free(group_table);
}

static bool
group_loop_detect(struct group_table *group_table,
                  struct group *group,
                  uint32_t id) {
  struct bucket *bucket;
  struct action *action;
  struct group *a_group;
  uint32_t group_id;

  TAILQ_FOREACH(bucket, &group->bucket_list, entry) {
    TAILQ_FOREACH(action, &bucket->action_list, entry) {
      if (action->ofpat.type == OFPAT_GROUP) {
        group_id = ((struct ofp_action_group *)&action->ofpat)->group_id;
        if (group_id == id) {
          return true;
        }
        a_group = group_table_lookup(group_table, group_id);
        if (a_group == NULL) {
          continue;
        }
        if (group_loop_detect(group_table, a_group, id) == true) {
          return true;
        }
      }
    }
    if (bucket->ofp.watch_group == id) {
      true;
    }
  }
  return false;
}

struct bucket *
group_live_bucket(struct bridge *bridge,
                  struct group *group) {
  struct group_table *group_table;
  struct bucket *bucket, *rv;
  struct group *a_group;

  group_table = bridge->group_table;

  TAILQ_FOREACH(bucket, &group->bucket_list, entry) {
    if (port_liveness(bridge, bucket->ofp.watch_port) == true) {
      return bucket;
    }
    if (bucket->ofp.watch_group == OFPG_ANY) {
      continue;
    }
    a_group = group_table_lookup(group_table, bucket->ofp.watch_group);
    if (a_group == NULL) {
      continue;
    }
    rv = group_live_bucket(bridge, a_group);
    if (rv != NULL) {
      return bucket;
    }
  }
  return NULL;
}

lagopus_result_t
group_table_add(struct group_table *group_table,
                struct group *group,
                struct ofp_error *error) {
  uint32_t key;
  lagopus_result_t rv;
  void *val;

  if (group_loop_detect(group_table, group, group->id) == true) {
    error->type = OFPET_GROUP_MOD_FAILED;
    error->code = OFPGMFC_LOOP;
    lagopus_msg_info("group add: %d: loop detected (%d:%d)",
                     group->id, error->type, error->code);
    return LAGOPUS_RESULT_OFP_ERROR;
  }
  key = htonl(group->id);
  val = group;
  rv = lagopus_hashmap_add_no_lock(&group_table->hashmap,
                                   (void *)key, &val, false);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  /* Reference table. */
  group->group_table = group_table;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
group_table_delete(struct group_table *group_table, uint32_t group_id) {
  uint32_t key;

  if (group_id == OFPG_ALL) {
    lagopus_hashmap_clear_no_lock(&group_table->hashmap, true);
  } else {
    key = htonl(group_id);
    lagopus_hashmap_delete_no_lock(&group_table->hashmap,
                                   (void *)key, NULL, true);
  }

  return LAGOPUS_RESULT_OK;
}

/*
 * note: no need lock.
 */
struct group *
group_table_lookup(struct group_table *group_table, uint32_t id) {
  uint32_t key;
  struct group *group = NULL;

  key = htonl(id);
  lagopus_hashmap_find_no_lock(&group_table->hashmap, (uint8_t *)key, &group);
  return group;
}

static lagopus_result_t
copy_bucket_list(struct bucket_list *dst, const struct bucket_list *src) {
  struct bucket *dst_bucket;
  struct bucket *src_bucket;

  TAILQ_FOREACH(src_bucket, src, entry) {
    dst_bucket = bucket_alloc();
    if (dst_bucket == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    TAILQ_INIT(&dst_bucket->action_list);
    dst_bucket->ofp = src_bucket->ofp;
    copy_action_list(&dst_bucket->action_list, &src_bucket->action_list);
    TAILQ_INSERT_TAIL(dst, dst_bucket, entry);
  }
  return LAGOPUS_RESULT_OK;
}

struct group *
group_alloc(struct ofp_group_mod *group_mod,
            struct bucket_list *bucket_list) {
  struct group *group;
  struct bucket *bucket;
  struct action *action;

  if (group_mod == NULL || bucket_list == NULL) {
    return NULL;
  }
  group = (struct group *)calloc(1, sizeof(struct group));
  if (group == NULL) {
    return NULL;
  }

  group->id = group_mod->group_id;
  group->type = group_mod->type;
  TAILQ_INIT(&group->bucket_list);
  copy_bucket_list(&group->bucket_list, bucket_list);
  if (lagopus_register_action_hook != NULL) {
    TAILQ_FOREACH(bucket, &group->bucket_list, entry) {
      int i;

      TAILQ_FOREACH(action, &bucket->action_list, entry) {
        lagopus_register_action_hook(action);
      }
      for (i = 0; i < LAGOPUS_ACTION_SET_ORDER_MAX; i++) {
        TAILQ_INIT(&bucket->actions[i]);
      }
      merge_action_set(bucket->actions, &bucket->action_list);
    }
  }
  lagopus_hashmap_create(&group->flows, LAGOPUS_HASHMAP_TYPE_ONE_WORD, NULL);
  clock_gettime(CLOCK_MONOTONIC, &group->create_time);

  return group;
}

static bool
group_do_flow_iterate(void *key, void *val,
                      lagopus_hashentry_t he, void *arg) {
  struct bridge *bridge;
  struct flow *flow;
  struct ofp_error error;

  (void) key;
  (void) he;

  flow = val;
  bridge = arg;
  flow_remove_with_reason_nolock(flow,
                                 bridge,
                                 OFPRR_GROUP_DELETE,
                                 &error);
  return true;
}

void
group_free(struct group *group) {
  bucket_list_free(&group->bucket_list);
  /* remove group action from each flows. */
  lagopus_hashmap_iterate_no_lock(&group->flows,
                                  group_do_flow_iterate,
                                  group->group_table->bridge);
  lagopus_hashmap_destroy(&group->flows, false);
  free(group);
}

void
group_modify(struct group *group, struct ofp_group_mod *group_mod,
             struct bucket_list *bucket_list) {
  bucket_list_free(&group->bucket_list);
  group->type = group_mod->type;
  TAILQ_INIT(&group->bucket_list);
  copy_bucket_list(&group->bucket_list, bucket_list);

  /* refresh action hook */
  if (lagopus_register_action_hook != NULL) {
    struct bucket *bucket;
    struct action *action;

    TAILQ_FOREACH(bucket, &group->bucket_list, entry) {
      int i;

      TAILQ_FOREACH(action, &bucket->action_list, entry) {
        lagopus_register_action_hook(action);
      }
      for (i = 0; i < LAGOPUS_ACTION_SET_ORDER_MAX; i++) {
        TAILQ_INIT(&bucket->actions[i]);
      }
      merge_action_set(bucket->actions, &bucket->action_list);
    }
  }
}

void
group_add_ref_flow(struct group *group,
                   struct flow *flow) {
  if (group != NULL) {
    lagopus_hashmap_add_no_lock(&group->flows,
                                (void *)flow, (void *)&flow, true);
  }
}

void
group_remove_ref_flow(struct group *group,
                      struct flow *flow) {
  if (group != NULL) {
    lagopus_hashmap_delete_no_lock(&group->flows, flow, NULL, false);
  }
}

static lagopus_result_t
set_group_stats(struct group_stats *stats, const struct group *group) {
  struct timespec ts;
  struct bucket *bucket;

  stats->ofp.group_id = group->id;
  stats->ofp.ref_count = (uint32_t)lagopus_hashmap_size(&group->flows);
  stats->ofp.packet_count = group->packet_count;
  stats->ofp.byte_count = group->byte_count;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  stats->ofp.duration_sec = (uint32_t)(ts.tv_sec - group->create_time.tv_sec);
  if (ts.tv_nsec < group->create_time.tv_nsec) {
    stats->ofp.duration_sec--;
    stats->ofp.duration_nsec = 1 * 1000 * 1000 * 1000;
  } else {
    stats->ofp.duration_nsec = 0;
  }
  stats->ofp.duration_nsec += (uint32_t)ts.tv_nsec;
  stats->ofp.duration_nsec -= (uint32_t)group->create_time.tv_nsec;

  /* bucket stats */
  TAILQ_INIT(&stats->bucket_counter_list);
  TAILQ_FOREACH(bucket, &group->bucket_list, entry) {
    struct bucket_counter *bucket_counter;

    bucket_counter = calloc(1, sizeof(struct bucket_counter));
    if (bucket_counter == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    bucket_counter->ofp = bucket->counter;
    TAILQ_INSERT_TAIL(&stats->bucket_counter_list, bucket_counter, entry);
  }
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
group_add_stats(struct group *group, struct group_stats_list *list) {
  struct group_stats *stats;

  if (group != NULL) {
    stats = calloc(1, sizeof(struct group_stats));
    if (stats == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    set_group_stats(stats, group);
    TAILQ_INSERT_TAIL(list, stats, entry);
  }
  return LAGOPUS_RESULT_OK;
}

static bool
group_do_stats_iterate(void *key, void *val,
                       lagopus_hashentry_t he, void *arg) {
  struct group_stats_list *list;

  (void) key;
  (void) val;
  (void) he;

  list = arg;
  if (group_add_stats(val, list) == LAGOPUS_RESULT_OK) {
    return true;
  } else {
    return false;
  }
}

lagopus_result_t
group_stats(struct group_table *group_table,
            struct ofp_group_stats_request *request,
            struct group_stats_list *list,
            struct ofp_error *error) {
  struct group *group;

  (void)error;

  if (request->group_id == OFPG_ALL) {
    lagopus_hashmap_iterate(&group_table->hashmap,
                            group_do_stats_iterate,
                            list);
  } else {
    group = group_table_lookup(group_table, request->group_id);
    return group_add_stats(group, list);
  }

  return LAGOPUS_RESULT_OK;
}

static bool
group_do_descs_iterate(void *key, void *val,
                       lagopus_hashentry_t he, void *arg) {
  struct group *group;
  struct group_desc_list *list;

  (void) key;
  (void) he;

  group = val;
  list = arg;
  if (group != NULL) {
    struct group_desc *desc;

    desc = calloc(1, sizeof(*desc));
    if (desc == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    desc->ofp.type = group->type;
    desc->ofp.group_id = group->id;
    TAILQ_INIT(&desc->bucket_list);
    copy_bucket_list(&desc->bucket_list, &group->bucket_list);
    TAILQ_INSERT_TAIL(list, desc, entry);
  }
  return true;
}


lagopus_result_t
group_descs(struct group_table *group_table,
            struct group_desc_list *group_desc_list,
            struct ofp_error *error) {
  (void)error;
  lagopus_hashmap_iterate(&group_table->hashmap,
                          group_do_descs_iterate,
                          group_desc_list);
  return LAGOPUS_RESULT_OK;
}

/**
 * group features are depend on dataplane implementation.
 */
lagopus_result_t
get_group_features(struct group_table *group_table,
                   struct ofp_group_features *features,
                   struct ofp_error *error) {
  (void)group_table;
  (void)error;

  features->types = (1 << OFPGT_ALL) |
                    (1 << OFPGT_SELECT) |
                    (1 << OFPGT_INDIRECT) |
                    (0 << OFPGT_FF);
  features->capabilities = OFPGFC_CHAINING | OFPGFC_CHAINING_CHECKS;
  features->max_groups[OFPGT_ALL] = OFPG_MAX;
  features->max_groups[OFPGT_SELECT] = OFPG_MAX;
  features->max_groups[OFPGT_INDIRECT] = OFPG_MAX;
  features->max_groups[OFPGT_FF] = 0;
  features->actions[OFPGT_ALL] = (1 << OFPAT_OUTPUT) |
                                 (1 << OFPAT_COPY_TTL_OUT) |
                                 (1 << OFPAT_COPY_TTL_IN) |
                                 (1 << OFPAT_SET_MPLS_TTL) |
                                 (1 << OFPAT_DEC_MPLS_TTL) |
                                 (1 << OFPAT_PUSH_VLAN) |
                                 (1 << OFPAT_POP_VLAN) |
                                 (1 << OFPAT_PUSH_MPLS) |
                                 (1 << OFPAT_POP_MPLS) |
                                 (0 << OFPAT_SET_QUEUE) |
                                 (1 << OFPAT_GROUP) |
                                 (1 << OFPAT_SET_NW_TTL) |
                                 (1 << OFPAT_DEC_NW_TTL) |
                                 (1 << OFPAT_SET_FIELD) |
                                 (1 << OFPAT_PUSH_PBB) |
                                 (1 << OFPAT_POP_PBB);
  features->actions[OFPGT_SELECT] = features->actions[OFPGT_ALL];
  features->actions[OFPGT_INDIRECT] = features->actions[OFPGT_ALL];
  features->actions[OFPGT_FF] = 0;

  return LAGOPUS_RESULT_OK;
}

/*
 * group_mod (Agent/DP API)
 */
lagopus_result_t
ofp_group_mod_add(uint64_t dpid,
                  struct ofp_group_mod *group_mod,
                  struct bucket_list *bucket_list,
                  struct ofp_error *error) {
  struct bridge *bridge;
  struct group *group;
  lagopus_result_t rv;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  group_table_wrlock(bridge->group_table);
  /* Look up existing group. */
  group = group_table_lookup(bridge->group_table, group_mod->group_id);
  if (group != NULL) {
    /* Group exists, send error. */
    error->type = OFPET_GROUP_MOD_FAILED;
    error->code = OFPGMFC_GROUP_EXISTS;
    lagopus_msg_info("group add: %d: group exists (%d:%d)",
                     group_mod->group_id, error->type, error->code);
    rv = LAGOPUS_RESULT_OFP_ERROR;
  } else {
    /* Allocate a new group. */
    group = group_alloc(group_mod, bucket_list);
    if (group == NULL) {
      bucket_list_free(bucket_list);
      rv = LAGOPUS_RESULT_NO_MEMORY;
    } else {
      /* Add a group. */
      rv = group_table_add(bridge->group_table, group, error);
    }
  }
  group_table_wrunlock(bridge->group_table);

  return rv;
}

lagopus_result_t
ofp_group_mod_modify(uint64_t dpid,
                     struct ofp_group_mod *group_mod,
                     struct bucket_list *bucket_list,
                     struct ofp_error *error) {
  struct bridge *bridge;
  struct group *group;
  lagopus_result_t rv;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  group_table_wrlock(bridge->group_table);
  /* Look up existing group. */
  group = group_table_lookup(bridge->group_table, group_mod->group_id);
  if (group == NULL) {
    /* Group does not exist, send error. */
    error->type = OFPET_GROUP_MOD_FAILED;
    error->code = OFPGMFC_UNKNOWN_GROUP;
    lagopus_msg_info("group modify: %d: group is not exist (%d:%d)",
                     group_mod->group_id, error->type, error->code);
    rv = LAGOPUS_RESULT_OFP_ERROR;
  } else {
    /* Modify group contents. */
    group_modify(group, group_mod, bucket_list);
    rv = LAGOPUS_RESULT_OK;
  }
  group_table_wrunlock(bridge->group_table);

  return rv;
}

lagopus_result_t
ofp_group_mod_delete(uint64_t dpid,
                     struct ofp_group_mod *group_mod,
                     struct ofp_error *error) {
  struct bridge *bridge;

  (void) error;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  /* Delete group with group_id. */
  group_table_wrlock(bridge->group_table);
  group_table_delete(bridge->group_table, group_mod->group_id);
  group_table_wrunlock(bridge->group_table);

  return LAGOPUS_RESULT_OK;
}

/*
 * group_stats (Agent/DP API)
 */
lagopus_result_t
ofp_group_stats_request_get(uint64_t dpid,
                            struct ofp_group_stats_request *group_stats_request,
                            struct group_stats_list *group_stats_list,
                            struct ofp_error *error) {
  struct bridge *bridge;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return group_stats(bridge->group_table,
                     group_stats_request,
                     group_stats_list,
                     error);
}

/*
 * group_desc (Agent/DP API)
 */
lagopus_result_t
ofp_group_desc_get(uint64_t dpid,
                   struct group_desc_list *group_desc_list,
                   struct ofp_error *error) {
  struct bridge *bridge;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return group_descs(bridge->group_table,
                     group_desc_list,
                     error);
}

/*
 * group_features (Agent/DP API)
 */
lagopus_result_t
ofp_group_features_get(uint64_t dpid,
                       struct ofp_group_features *group_features,
                       struct ofp_error *error) {
  struct bridge *bridge;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return get_group_features(bridge->group_table,
                            group_features,
                            error);
}
