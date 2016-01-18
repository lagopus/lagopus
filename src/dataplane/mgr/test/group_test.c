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

#include <sys/queue.h>
#include "unity.h"
#include "lagopus_apis.h"
#include "lagopus/pbuf.h"
#include "lagopus/flowdb.h"
#include "lagopus/group.h"
#include "lagopus/bridge.h"
#include "lagopus/datastore/bridge.h"
#include "lagopus/dp_apis.h"
#include "openflow13.h"
#include "ofp_action.h"
#include "ofp_match.h"
#include "ofp_instruction.h"

void
setUp(void) {
  datastore_bridge_info_t info;

  TEST_ASSERT_EQUAL(dp_api_init(), LAGOPUS_RESULT_OK);
  memset(&info, 0, sizeof(info));
  info.fail_mode = DATASTORE_BRIDGE_FAIL_MODE_SECURE;
  TEST_ASSERT_EQUAL(dp_bridge_create("br0", &info), LAGOPUS_RESULT_OK);
}

void
tearDown(void) {
  dp_bridge_destroy("br0");
  dp_api_fini();
}

void
test_group_table_alloc(void) {
  struct bridge *bridge;
  struct group_table *group_table;
  lagopus_result_t rv;

  bridge = dp_bridge_lookup("br0");
  TEST_ASSERT_NOT_NULL_MESSAGE(bridge, "bridge alloc error.");
  group_table = group_table_alloc(bridge);
  TEST_ASSERT_NOT_NULL_MESSAGE(group_table, "alloc error");
}

void
test_group_table_add(void) {
  struct bridge *bridge;
  struct group_table *group_table;
  struct group *group;
  struct ofp_group_mod group_mod;
  struct bucket_list bucket_list;
  struct bucket *bucket;
  struct action *action;
  struct ofp_error error;
  lagopus_result_t ret;

  bridge = dp_bridge_lookup("br0");
  group_table = group_table_alloc(bridge);
  TEST_ASSERT_NOT_NULL_MESSAGE(bridge, "group_table alloc error.");
  bridge->group_table = group_table;
  group_mod.group_id = 1;
  group_mod.type = OFPGT_ALL;
  TAILQ_INIT(&bucket_list);
  group = group_alloc(&group_mod, &bucket_list);
  TEST_ASSERT_NOT_NULL_MESSAGE(group, "group_alloc error");
  ret = group_table_add(group_table, group, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "group_table_add error");

  group_mod.group_id = 2;
  group_mod.type = OFPGT_ALL;
  TAILQ_INIT(&bucket_list);

  action = action_alloc(sizeof(uint32_t));
  TEST_ASSERT_NOT_NULL_MESSAGE(action, "action alloc error");
  action->ofpat.type = OFPAT_GROUP;
  bucket = calloc(1, sizeof(struct bucket));
  TEST_ASSERT_NOT_NULL_MESSAGE(bucket, "bucket alloc error");
  TAILQ_INIT(&bucket->action_list);
  ((struct ofp_action_group *)&action->ofpat)->group_id = 3;
  ((struct ofp_action_group *)&action->ofpat)->len =
    sizeof(struct ofp_action_header) + sizeof(uint32_t);
  TAILQ_INSERT_TAIL(&bucket->action_list, action, entry);
  TAILQ_INSERT_TAIL(&bucket_list, bucket, entry);

  action = action_alloc(sizeof(uint32_t));
  TEST_ASSERT_NOT_NULL_MESSAGE(action, "action alloc error");
  action->ofpat.type = OFPAT_OUTPUT;
  bucket = calloc(1, sizeof(struct bucket));
  TEST_ASSERT_NOT_NULL_MESSAGE(bucket, "bucket alloc error");
  TAILQ_INIT(&bucket->action_list);
  ((struct ofp_action_output *)&action->ofpat)->port = 3;
  ((struct ofp_action_output *)&action->ofpat)->len =
    sizeof(struct ofp_action_header) + sizeof(uint32_t);
  TAILQ_INSERT_TAIL(&bucket->action_list, action, entry);
  TAILQ_INSERT_TAIL(&bucket_list, bucket, entry);

  group = group_alloc(&group_mod, &bucket_list);
  ret = group_table_add(group_table, group, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "group_table_add error");
  bucket = TAILQ_FIRST(&group->bucket_list);
  TEST_ASSERT_NOT_NULL_MESSAGE(bucket, "bucket copy error");
  action = TAILQ_FIRST(&bucket->action_list);
  TEST_ASSERT_NOT_NULL_MESSAGE(action, "action copy error");
  TEST_ASSERT_EQUAL_MESSAGE(action->ofpat.type, OFPAT_GROUP,
                            "action copy error");
  bucket = TAILQ_NEXT(bucket, entry);
  TEST_ASSERT_NOT_NULL_MESSAGE(bucket, "bucket copy error");
  action = TAILQ_FIRST(&bucket->action_list);
  TEST_ASSERT_NOT_NULL_MESSAGE(action, "action copy error");
  TEST_ASSERT_EQUAL_MESSAGE(action->ofpat.type, OFPAT_OUTPUT,
                            "action copy error");
  TEST_ASSERT_EQUAL_MESSAGE(
    ((struct ofp_action_output *)&action->ofpat)->port, 3,
    "action copy error");

  group_mod.group_id = 3;
  group_mod.type = OFPGT_ALL;
  action = action_alloc(sizeof(uint32_t));
  TEST_ASSERT_NOT_NULL_MESSAGE(action, "action alloc error");
  action->ofpat.type = OFPAT_GROUP;
  ((struct ofp_action_group *)&action->ofpat)->group_id = 2;
  ((struct ofp_action_group *)&action->ofpat)->len =
    sizeof(struct ofp_action_header) + sizeof(uint32_t);
  bucket = calloc(1, sizeof(struct bucket));
  TAILQ_INIT(&bucket->action_list);
  TAILQ_INSERT_TAIL(&bucket->action_list, action, entry);
  TAILQ_INIT(&bucket_list);
  TAILQ_INSERT_TAIL(&bucket_list, bucket, entry);
  group = group_alloc(&group_mod, &bucket_list);
  ret = group_table_add(group_table, group, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OFP_ERROR,
                            "group_table_add loop error");
  TEST_ASSERT_EQUAL_MESSAGE(error.code, OFPGMFC_LOOP,
                            "loop error detection failed");
}

void
test_group_table_lookup(void) {
  struct bridge *bridge;
  struct group_table *group_table;
  struct group *group, *gl;
  struct ofp_group_mod group_mod;
  struct bucket_list bucket_list;
  struct ofp_error error;
  lagopus_result_t ret;

  bridge = dp_bridge_lookup("br0");
  TEST_ASSERT_NOT_NULL_MESSAGE(bridge, "bridge alloc error.");
  TAILQ_INIT(&bucket_list);
  group_table = group_table_alloc(bridge);
  TEST_ASSERT_NOT_NULL_MESSAGE(bridge, "group_table alloc error.");
  bridge->group_table = group_table;
  group_mod.group_id = 1;
  group_mod.type = OFPGT_ALL;
  group = group_alloc(&group_mod, &bucket_list);
  ret = group_table_add(group_table, group, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "group_table_add error");
  gl = group_table_lookup(group_table, 0);
  TEST_ASSERT_NULL_MESSAGE(gl, "invalid group found error");
  gl = group_table_lookup(group_table, 1);
  TEST_ASSERT_EQUAL_MESSAGE(gl, group,
                            "invalid group found error");
}

void
test_group_desc(void) {
  struct bridge *bridge;
  struct group_table *group_table;
  struct group *group;
  struct ofp_group_mod group_mod;
  struct bucket_list bucket_list;
  struct group_desc_list group_desc_list;
  struct group_desc *desc;
  struct ofp_error error;
  lagopus_result_t ret;

  bridge = dp_bridge_lookup("br0");
  TEST_ASSERT_NOT_NULL_MESSAGE(bridge, "bridge alloc error.");
  TAILQ_INIT(&bucket_list);
  group_table = group_table_alloc(bridge);
  TEST_ASSERT_NOT_NULL_MESSAGE(bridge, "group_table alloc error.");
  bridge->group_table = group_table;

  group_mod.group_id = 1;
  group_mod.type = OFPGT_ALL;
  group = group_alloc(&group_mod, &bucket_list);
  ret = group_table_add(group_table, group, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "group_table_add error");

  group_mod.group_id = 1000000;
  group_mod.type = OFPGT_SELECT;
  group = group_alloc(&group_mod, &bucket_list);
  ret = group_table_add(group_table, group, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "group_table_add error");

  TAILQ_INIT(&group_desc_list);
  ret = group_descs(group_table, &group_desc_list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "get desc error");
  desc = TAILQ_FIRST(&group_desc_list);
  TEST_ASSERT_NOT_NULL_MESSAGE(desc, "group not found");
  if (desc->ofp.group_id == 1) {
    TEST_ASSERT_EQUAL_MESSAGE(desc->ofp.group_id, 1,
                              "group id error");
    TEST_ASSERT_EQUAL_MESSAGE(desc->ofp.type, OFPGT_ALL,
                              "group type error");
    desc = TAILQ_NEXT(desc, entry);
    TEST_ASSERT_NOT_NULL_MESSAGE(desc, "number of groups error");
    TEST_ASSERT_EQUAL_MESSAGE(desc->ofp.group_id, 1000000,
                              "group id error");
    TEST_ASSERT_EQUAL_MESSAGE(desc->ofp.type, OFPGT_SELECT,
                              "group type error");
  } else {
    TEST_ASSERT_EQUAL_MESSAGE(desc->ofp.group_id, 1000000,
                              "group id error");
    TEST_ASSERT_EQUAL_MESSAGE(desc->ofp.type, OFPGT_SELECT,
                              "group type error");
    desc = TAILQ_NEXT(desc, entry);
    TEST_ASSERT_NOT_NULL_MESSAGE(desc, "number of groups error");
    TEST_ASSERT_EQUAL_MESSAGE(desc->ofp.group_id, 1,
                              "group id error");
    TEST_ASSERT_EQUAL_MESSAGE(desc->ofp.type, OFPGT_ALL,
                              "group type error");
  }
  desc = TAILQ_NEXT(desc, entry);
  TEST_ASSERT_NULL_MESSAGE(desc, "number of groups error");
}

void
test_group_stats(void) {
  struct bridge *bridge;
  struct group_table *group_table;
  struct group *group;
  struct ofp_group_mod group_mod;
  struct bucket_list bucket_list;
  struct ofp_group_stats_request request;
  struct group_stats_list group_stats_list;
  struct group_stats *stats;
  struct ofp_error error;
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct instruction *instruction;
  struct action *action;
  lagopus_result_t ret;

  bridge = dp_bridge_lookup("br0");
  TEST_ASSERT_NOT_NULL_MESSAGE(bridge, "bridge alloc error.");
  TAILQ_INIT(&bucket_list);
  group_table = group_table_alloc(bridge);
  TEST_ASSERT_NOT_NULL_MESSAGE(bridge, "group_table alloc error.");
  bridge->group_table = group_table;

  group_mod.group_id = 1;
  group_mod.type = OFPGT_ALL;
  group = group_alloc(&group_mod, &bucket_list);
  ret = group_table_add(group_table, group, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "group_table_add error");

  group_mod.group_id = 1000000;
  group_mod.type = OFPGT_ALL;
  group = group_alloc(&group_mod, &bucket_list);
  ret = group_table_add(group_table, group, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "group_table_add error");

  request.group_id = 1;
  TAILQ_INIT(&group_stats_list);
  ret = group_stats(group_table, &request, &group_stats_list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "get group stats(single) error");
  stats = TAILQ_FIRST(&group_stats_list);
  TEST_ASSERT_NOT_NULL_MESSAGE(stats, "group not found");
  TEST_ASSERT_EQUAL_MESSAGE(stats->ofp.group_id, 1,
                            "group id error");
  TEST_ASSERT_EQUAL_MESSAGE(stats->ofp.duration_sec, 0,
                            "duration sec error");
  TEST_ASSERT_EQUAL_MESSAGE(stats->ofp.ref_count, 0,
                            "ref_count error");
  stats = TAILQ_NEXT(stats, entry);
  TEST_ASSERT_NULL_MESSAGE(stats, "number of groups error");

  TAILQ_INIT(&match_list);
  TAILQ_INIT(&instruction_list);
  instruction = instruction_alloc();
  TEST_ASSERT_NOT_NULL_MESSAGE(instruction, "instruction alloc error.");
  instruction->ofpit.type = OFPIT_APPLY_ACTIONS;
  TAILQ_INIT(&instruction->action_list);
  action = action_alloc(sizeof(struct ofp_action_group));
  TEST_ASSERT_NOT_NULL_MESSAGE(action, "action alloc error.");
  action->ofpat.type = OFPAT_GROUP;
  ((struct ofp_action_group *)&action->ofpat)->group_id = 5;
  TAILQ_INSERT_TAIL(&instruction->action_list, action, entry);
  TAILQ_INSERT_TAIL(&instruction_list, instruction, entry);
  flow_mod.table_id = 0;
  flow_mod.priority = 1;
  flow_mod.flags = 0;
  flow_mod.cookie = 0;
  ret = flowdb_flow_add(bridge, &flow_mod, &match_list,
                        &instruction_list, &error);
  TEST_ASSERT_NOT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                                "flowdb_flow_add: group check error");
  TAILQ_INIT(&match_list);
  TAILQ_INIT(&instruction_list);
  instruction = instruction_alloc();
  TEST_ASSERT_NOT_NULL_MESSAGE(instruction, "instruction alloc error.");
  instruction->ofpit.type = OFPIT_APPLY_ACTIONS;
  TAILQ_INIT(&instruction->action_list);
  action = action_alloc(sizeof(struct ofp_action_group));
  TEST_ASSERT_NOT_NULL_MESSAGE(action, "action alloc error.");
  action->ofpat.type = OFPAT_GROUP;
  ((struct ofp_action_group *)&action->ofpat)->group_id = 1;
  TAILQ_INSERT_TAIL(&instruction->action_list, action, entry);
  TAILQ_INSERT_TAIL(&instruction_list, instruction, entry);
  ret = flowdb_flow_add(bridge, &flow_mod, &match_list,
                        &instruction_list, &error);
  printf("%d.%d\n", error.type, error.code);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "flowdb_flow_add: add to empty table error");

  request.group_id = 1;
  TAILQ_INIT(&group_stats_list);
  ret = group_stats(group_table, &request, &group_stats_list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "get stats(single) error");
  stats = TAILQ_FIRST(&group_stats_list);
  TEST_ASSERT_NOT_NULL_MESSAGE(stats, "group not found");
  TEST_ASSERT_EQUAL_MESSAGE(stats->ofp.group_id, 1,
                            "group id error");
  TEST_ASSERT_EQUAL_MESSAGE(stats->ofp.duration_sec, 0,
                            "duration sec error");
  TEST_ASSERT_EQUAL_MESSAGE(stats->ofp.ref_count, 1,
                            "ref_count error");
  stats = TAILQ_NEXT(stats, entry);
  TEST_ASSERT_NULL_MESSAGE(stats, "number of groups error");

  request.group_id = OFPG_ALL;
  TAILQ_INIT(&group_stats_list);
  ret = group_stats(group_table, &request, &group_stats_list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "get stats error");
  stats = TAILQ_FIRST(&group_stats_list);
  TEST_ASSERT_NOT_NULL_MESSAGE(stats, "group not found");
  if (stats->ofp.group_id == 1) {
    TEST_ASSERT_EQUAL_MESSAGE(stats->ofp.group_id, 1,
                              "group id error");
    TEST_ASSERT_EQUAL_MESSAGE(stats->ofp.duration_sec, 0,
                              "duration sec error");
    stats = TAILQ_NEXT(stats, entry);
    TEST_ASSERT_NOT_NULL_MESSAGE(stats, "group not found");
    TEST_ASSERT_EQUAL_MESSAGE(stats->ofp.group_id, 1000000,
                              "group id error");
  } else {
    TEST_ASSERT_EQUAL_MESSAGE(stats->ofp.group_id, 1000000,
                              "group id error");
    stats = TAILQ_NEXT(stats, entry);
    TEST_ASSERT_NOT_NULL_MESSAGE(stats, "group not found");
    TEST_ASSERT_EQUAL_MESSAGE(stats->ofp.group_id, 1,
                              "group id error");
    TEST_ASSERT_EQUAL_MESSAGE(stats->ofp.duration_sec, 0,
                              "duration sec error");
  }
  stats = TAILQ_NEXT(stats, entry);
  TEST_ASSERT_NULL_MESSAGE(stats, "number of groups error");

  sleep(1);
  TAILQ_INIT(&group_stats_list);
  ret = group_stats(group_table, &request, &group_stats_list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "get stats error");
  stats = TAILQ_FIRST(&group_stats_list);
  TEST_ASSERT_NOT_NULL_MESSAGE(stats, "group not found");
  TEST_ASSERT_EQUAL_MESSAGE(stats->ofp.duration_sec, 1,
                            "duration sec error");

  request.group_id = 20;
  TAILQ_INIT(&group_stats_list);
  ret = group_stats(group_table, &request, &group_stats_list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "get stats error");
  stats = TAILQ_FIRST(&group_stats_list);
  TEST_ASSERT_NULL_MESSAGE(stats, "stats is not empry");
}

void
test_group_live_bucket(void) {
  struct bridge *bridge;
  struct group_table *group_table;
  struct group *group;
  struct ofp_group_mod group_mod;
  struct bucket_list bucket_list;
  struct bucket *bucket;
  struct action *action;
  struct ofp_error error;
  lagopus_result_t rv;

  bridge = dp_bridge_lookup("br0");
  group_table = bridge->group_table;
  group_mod.group_id = 1;
  group_mod.type = OFPGT_ALL;
  TAILQ_INIT(&bucket_list);
  action = action_alloc(sizeof(uint32_t));
  TEST_ASSERT_NOT_NULL(action);
  action->ofpat.type = OFPAT_OUTPUT;
  bucket = calloc(1, sizeof(struct bucket));
  TEST_ASSERT_NOT_NULL(bucket);
  bucket->ofp.watch_port = 1;
  TAILQ_INIT(&bucket->action_list);
  ((struct ofp_action_output *)&action->ofpat)->port = 1;
  ((struct ofp_action_group *)&action->ofpat)->len =
    sizeof(struct ofp_action_header) + sizeof(uint32_t);
  TAILQ_INSERT_TAIL(&bucket->action_list, action, entry);
  TAILQ_INSERT_TAIL(&bucket_list, bucket, entry);
  action = action_alloc(sizeof(uint32_t));
  TEST_ASSERT_NOT_NULL(action);
  action->ofpat.type = OFPAT_OUTPUT;
  bucket = calloc(1, sizeof(struct bucket));
  TEST_ASSERT_NOT_NULL(bucket);
  TAILQ_INIT(&bucket->action_list);
  ((struct ofp_action_output *)&action->ofpat)->port = 3;
  ((struct ofp_action_output *)&action->ofpat)->len =
    sizeof(struct ofp_action_header) + sizeof(uint32_t);
  TAILQ_INSERT_TAIL(&bucket->action_list, action, entry);
  TAILQ_INSERT_TAIL(&bucket_list, bucket, entry);
  group = group_alloc(&group_mod, &bucket_list);
  rv = group_table_add(group_table, group, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_NULL(group_live_bucket(bridge, group));
}
