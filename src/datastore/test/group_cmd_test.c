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

#include "unity.h"
#include "lagopus_apis.h"
#include "cmd_test_utils.h"
#include "../datastore_apis.h"
#include "../agent/ofp_bucket.h"
#include "../agent/ofp_action.h"
#include "../group_cmd_internal.h"
#include "../channel_cmd_internal.h"
#include "../controller_cmd_internal.h"
#include "../interface_cmd_internal.h"
#include "../port_cmd_internal.h"
#include "../bridge_cmd_internal.h"
#include "../datastore_internal.h"

static lagopus_dstring_t ds = NULL;
static lagopus_hashmap_t tbl = NULL;
static datastore_interp_t interp = NULL;
static bool destroy = false;

void
setUp(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create interp. */
  INTERP_CREATE(ret, NULL, interp, tbl, ds);
}

void
tearDown(void) {
  /* destroy interp. */
  INTERP_DESTROY(NULL, interp, tbl, ds, destroy)
}

void
group_create(uint64_t dpid, uint32_t group_id,
             size_t bucket_num, size_t action_num) {
  struct ofp_group_mod mod;
  struct bucket_list bucket_list;
  struct ofp_error error;
  struct bucket *bucket;
  struct action *action;
  struct ofp_action_output *action_output;
  size_t i, j;

  TAILQ_INIT(&bucket_list);

  mod.command = OFPGC_ADD;
  mod.type = OFPGT_ALL;
  mod.group_id = group_id;

  for (i = 0; i < bucket_num; i++) {
    bucket = bucket_alloc();
    TAILQ_INSERT_TAIL(&bucket_list, bucket, entry);
    bucket->ofp.len = 0x30;
    bucket->ofp.weight = (uint16_t) (0x02 + i);
    bucket->ofp.watch_port = (uint32_t) (0x03 +i);
    bucket->ofp.watch_group = (uint32_t) (0x4 + i);

    TAILQ_INIT(&bucket->action_list);
    for (j = 0; j < action_num; j++) {
      action = action_alloc(0x10);
      TAILQ_INSERT_TAIL(&bucket->action_list, action, entry);
      action->ofpat.type = OFPAT_OUTPUT;
      action->ofpat.len = 0x10;
      action_output = (struct ofp_action_output *) &action->ofpat;
      action_output->port = (uint32_t) (0x05 + j);
      action_output->max_len = (uint16_t) (0x06 + j);
    }
  }

  ofp_group_mod_add(dpid, &mod, &bucket_list, &error);
}

void
group_destroy(uint64_t dpid, uint32_t group_id) {
  struct ofp_group_mod mod;
  struct ofp_error error;

  mod.command = OFPGC_DELETE;
  mod.type = OFPGT_ALL;
  mod.group_id = group_id;

  ofp_group_mod_delete(dpid, &mod, &error);
}

void
test_group_cmd_parse_dump_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"group", "b1",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
    "\"groups\":[]}]}";

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE(ret, &interp, state, &tbl, &ds, str,
                     "b1", "1", "cha1", "c1", "i1", "p1", "1");

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, group_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY(ret, &interp, state, &tbl, &ds, str,
                      "b1", "cha1", "c1", "i1", "p1");
}

void
test_group_cmd_parse_stats_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"group", "b2", "stats",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b2\",\n"
    "\"groups\":[]}]}";

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE(ret, &interp, state, &tbl, &ds, str,
                     "b2", "2", "cha2", "c2", "i2", "p2", "2");

  /* stats cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, group_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY(ret, &interp, state, &tbl, &ds, str,
                      "b2", "cha2", "c2", "i2", "p2");
}

void
test_group_cmd_parse_bat_opt_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"group", "hoge",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"hoge\"}";

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 group_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_group_cmd_parse_bat_opt_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"group", "br3", "hoge",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"sub_cmd = hoge.\"}";

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE(ret, &interp, state, &tbl, &ds, str,
                     "b3", "3", "cha3", "c3", "i3", "p3", "3");

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 group_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY(ret, &interp, state, &tbl, &ds, str,
                      "b3", "cha3", "c3", "i3", "p3");
}

void
test_group_cmd_parse_stats_bat_opt_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"group", "br4", "stats",
                         "hoge",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"br4\"}";

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE(ret, &interp, state, &tbl, &ds, str,
                     "b4", "4", "cha4", "c4", "i4", "p4", "4");

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 group_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY(ret, &interp, state, &tbl, &ds, str,
                      "b4", "cha4", "c4", "i4", "p4");
}

void
test_group_cmd_parse_dump_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  uint64_t dpid = 5;
  uint32_t group_id = 5;
  size_t bucket_num = 2;
  size_t action_num = 2;
  char *str = NULL;
  const char *argv1[] = {"group", "b5",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b5\",\n"
    "\"groups\":[{\"group-id\":5,\n"
    "\"type\":\"all\",\n"
    "\"buckets\":[{\"bucket-id\":0,\n"
    "\"weight\":2,\n"
    "\"watch-port\":3,\n"
    "\"watch-group\":4,\n"
    "\"actions\":[{\"output\":5},\n"
    "\{\"output\":6}]},\n"
    "\{\"bucket-id\":1,\n"
    "\"weight\":3,\n"
    "\"watch-port\":4,\n"
    "\"watch-group\":5,\n"
    "\"actions\":[{\"output\":5},\n"
    "\{\"output\":6}]}]}]}]}";

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE(ret, &interp, state, &tbl, &ds, str,
                     "b5", "5", "cha5", "c5", "i5", "p5", "5");

  group_create(dpid, group_id, bucket_num, action_num);

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, group_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  group_destroy(dpid, group_id);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY(ret, &interp, state, &tbl, &ds, str,
                      "b5", "cha5", "c5", "i5", "p5");
}

void
test_group_cmd_parse_stats_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  uint64_t dpid = 6;
  uint32_t group_id = 6;
  size_t bucket_num = 2;
  size_t action_num = 2;
  char *str = NULL;
  const char *argv1[] = {"group", "b6", "stats",
                         NULL
                        };

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE(ret, &interp, state, &tbl, &ds, str,
                     "b6", "6", "cha6", "c6", "i6", "p6", "6");

  group_create(dpid, group_id, bucket_num, action_num);

  /* stats cmd. */
  /* duration_sec/duration_nsec is undefined value. */
  TEST_CMD_PARSE_WITHOUT_STRCMP(ret, LAGOPUS_RESULT_OK,
                                group_cmd_parse, &interp, state,
                                ARGV_SIZE(argv1), argv1, &tbl, NULL,
                                &ds, str);

  group_destroy(dpid, group_id);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY(ret, &interp, state, &tbl, &ds, str,
                      "b6", "cha6", "c6", "i6", "p6");
}

void
test_destroy(void) {
  destroy = true;
}
