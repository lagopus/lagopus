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
#include "lagopus/flowdb.h"
#include "lagopus/dp_apis.h"
#include "../datastore_apis.h"
#include "../datastore_internal.h"
#include "../flow_cmd.h"
#include "../flow_cmd_internal.h"
#include "../agent/ofp_match.h"
#include "../agent/ofp_instruction.h"
#include "../agent/ofp_action.h"
#include "../channel_cmd_internal.h"
#include "../controller_cmd_internal.h"
#include "../interface_cmd_internal.h"
#include "../port_cmd_internal.h"
#include "../bridge_cmd_internal.h"

#define OUT_LEN 0x10
#define GROUP_LEN 0x08
#define SET_FIELD_LEN 0x10
#define ENCAP_LEN 0x20
#define DECAP_LEN 0x28
#define ED_PROP_LEN 0x18
#define WRITE_ACT_LEN OUT_LEN + GROUP_LEN + SET_FIELD_LEN + \
  ENCAP_LEN + DECAP_LEN + 0x08

static lagopus_dstring_t ds = NULL;
static lagopus_hashmap_t tbl = NULL;
static datastore_interp_t interp = NULL;
static bool destroy = false;
static const uint64_t dpid = 1;
static const char *bridge_name = DATASTORE_NAMESPACE_DELIMITER"test_bridge01";

static void
create_instructions(struct instruction_list *instruction_list) {
  struct instruction *instruction;
  struct action *action;
  struct ofp_action_output *act_output;
  struct ofp_action_group *act_group;
  struct ofp_action_set_field *act_set_field;
  struct ofp_action_encap *act_encap;
  struct ofp_action_decap *act_decap;
  struct ed_prop *ed_prop;
  uint8_t field[4] = {0x80, 0x00, 0x08, 0x06};
  uint8_t val[6] = {0x00, 0x0c, 0x29, 0x7a, 0x90, 0xb3};

  TAILQ_INIT(instruction_list);

  instruction = instruction_alloc();
  TAILQ_INSERT_TAIL(instruction_list, instruction, entry);
  instruction->ofpit_actions.type = OFPIT_WRITE_ACTIONS;
  instruction->ofpit_actions.len = WRITE_ACT_LEN;

  /* OUTPUT */
  action = action_alloc(OUT_LEN);
  TAILQ_INSERT_TAIL(&instruction->action_list, action, entry);
  act_output = (struct ofp_action_output *)&action->ofpat;
  act_output->type = OFPAT_OUTPUT;
  act_output->len = OUT_LEN;
  act_output->port = 0x1;
  act_output->max_len = 0x20;

  /* GROUP */
  action = action_alloc(GROUP_LEN);
  TAILQ_INSERT_TAIL(&instruction->action_list, action, entry);
  act_group = (struct ofp_action_group *)&action->ofpat;
  act_group->type = OFPAT_GROUP;
  act_group->len = GROUP_LEN;
  act_group->group_id = 0x03;

  /* SET_FIELD */
  action = action_alloc(SET_FIELD_LEN);
  TAILQ_INSERT_TAIL(&instruction->action_list, action, entry);
  act_set_field = (struct ofp_action_set_field *)&action->ofpat;
  act_set_field->type = OFPAT_SET_FIELD;
  act_set_field->len = SET_FIELD_LEN;
  memcpy(act_set_field->field, field, 4);
  memcpy(act_set_field->field + 4, val, 6);

  /* SET_FIELD / encap */
  action = action_alloc(ENCAP_LEN);
  TAILQ_INSERT_TAIL(&instruction->action_list, action, entry);
  act_encap = (struct ofp_action_encap *)&action->ofpat;
  act_encap->type = OFPAT_ENCAP;
  act_encap->len = ENCAP_LEN;
  act_encap->packet_type = 0x04;

  ed_prop = ed_prop_alloc();
  TAILQ_INSERT_TAIL(&action->ed_prop_list, ed_prop, entry);
  ed_prop->ofp_ed_prop_portname.type = OFPPPT_PROP_PORT_NAME;
  ed_prop->ofp_ed_prop_portname.len = ED_PROP_LEN;
  ed_prop->ofp_ed_prop_portname.port_flags = 0x05;
  strcpy((char *) &ed_prop->ofp_ed_prop_portname.name, "hoge1");

  /* SET_FIELD / decap */
  action = action_alloc(DECAP_LEN);
  TAILQ_INSERT_TAIL(&instruction->action_list, action, entry);
  act_decap = (struct ofp_action_decap *)&action->ofpat;
  act_decap->type = OFPAT_DECAP;
  act_decap->len = DECAP_LEN;
  act_decap->cur_pkt_type = 0x06;
  act_decap->new_pkt_type = 0x07;

  ed_prop = ed_prop_alloc();
  TAILQ_INSERT_TAIL(&action->ed_prop_list, ed_prop, entry);
  ed_prop->ofp_ed_prop_portname.type = OFPPPT_PROP_PORT_NAME;
  ed_prop->ofp_ed_prop_portname.len = ED_PROP_LEN;
  ed_prop->ofp_ed_prop_portname.port_flags = 0x08;
  strcpy((char *) &ed_prop->ofp_ed_prop_portname.name, "hoge2");
}

static void
create_matchs(struct match_list *match_list) {
  struct match *match;
  uint8_t value[4] = {0x00, 0x00, 0x00, 0x10};

  TAILQ_INIT(match_list);

  match = match_alloc(0x04);
  TAILQ_INSERT_TAIL(match_list, match, entry);
  match->oxm_class = OFPXMC_OPENFLOW_BASIC;
  match->oxm_field = OFPXMT_OFB_IN_PORT << 1;
  match->oxm_length = 0x04;
  memcpy(match->oxm_value, value, match->oxm_length);
}

static void
create_buckets(struct bucket_list *bucket_list) {
  TAILQ_INIT(bucket_list);
}

void
setUp(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;

  /* create interp. */
  INTERP_CREATE(ret, NULL, interp, tbl, ds);

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE(ret, &interp, state, &tbl, &ds, str,
                     "test_bridge01", "1", "cha1", "c1",
                     "test_if01", "test_port01", "1");
}

void
tearDown(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY(ret, &interp, state, &tbl, &ds, str,
                      "test_bridge01", "cha1", "c1",
                      "test_if01", "test_port01");

  /* destroy interp. */
  INTERP_DESTROY(NULL, interp, tbl, ds, destroy);
}

void
test_flow_cmd_dump_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct instruction_list instruction_list1;
  struct instruction_list instruction_list2;
  struct match_list match_list;
  struct ofp_flow_mod flow_mod;
  struct bucket_list bucket_list;
  struct ofp_group_mod group_mod;
  struct ofp_error error;
  char *str = NULL;
  const char test_str1[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_bridge01\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":1,\n"
      "\"idle_timeout\":4,\n"
      "\"hard_timeout\":5,\n"
      "\"check_overlap\":null,\n"
      "\"cookie\":3,\n"
      "\"in_port\":16,\n"
      "\"actions\":[{\"write_actions\":\n"
      "[{\"output\":1},\n"
      "{\"group\":3},\n"
      "{\"dl_src\":\"00:0c:29:7a:90:b3\"},\n"
      "{\"encap\":{\"packet_type\":4,\n"
      "\"properties\":[{\"port_name\":{\"port_flags\":5,\n"
      "\"name\":\"hoge1\"}}]}},\n"
      "{\"decap\":{\"cur_packet_type\":6,\n"
      "\"new_packet_type\":7,\n"
      "\"properties\":[{\"port_name\":{\"port_flags\":8,\n"
      "\"name\":\"hoge2\"}}]}}]}]},\n"
      "{\"priority\":1,\n"
      "\"idle_timeout\":4,\n"
      "\"hard_timeout\":5,\n"
      "\"check_overlap\":null,\n"
      "\"cookie\":3,\n"
      "\"actions\":[{\"write_actions\":\n"
      "[{\"output\":1},\n"
      "{\"group\":3},\n"
      "{\"dl_src\":\"00:0c:29:7a:90:b3\"},\n"
      "{\"encap\":{\"packet_type\":4,\n"
      "\"properties\":[{\"port_name\":{\"port_flags\":5,\n"
      "\"name\":\"hoge1\"}}]}},\n"
      "{\"decap\":{\"cur_packet_type\":6,\n"
      "\"new_packet_type\":7,\n"
      "\"properties\":[{\"port_name\":{\"port_flags\":8,\n"
      "\"name\":\"hoge2\"}}]}}]}]}]}]}";

  /* group_mod */
  create_buckets(&bucket_list);
  memset(&group_mod, 0, sizeof(group_mod));
  group_mod.group_id = 3;
  group_mod.type = OFPGT_ALL;

  TEST_ASSERT_GROUP_ADD(ret, dpid, &group_mod,
                        &bucket_list, &error);

  /* flow_mod */
  create_matchs(&match_list);
  create_instructions(&instruction_list1);
  create_instructions(&instruction_list2);
  memset(&flow_mod, 0, sizeof(flow_mod));
  flow_mod.table_id = 0;
  flow_mod.priority = 1;
  flow_mod.flags = 2;
  flow_mod.cookie = 3;
  flow_mod.idle_timeout = 4;
  flow_mod.hard_timeout = 5;
  flow_mod.out_port = OFPP_ANY;
  flow_mod.out_group = OFPG_ANY;

  TEST_ASSERT_FLOW_ADD(ret, dpid, &flow_mod, &match_list,
                       &instruction_list1, &error);

  TEST_ASSERT_FLOW_ADD(ret, dpid, &flow_mod, &match_list,
                       &instruction_list2, &error);

  (void) lagopus_dstring_clear(&ds);
  ret = dump_bridge_domains_flow(bridge_name, OFPTT_ALL,
                                 false, true, &ds);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "flow_cmd_dump error.");
  TEST_DSTRING(ret, &ds, str, test_str1, true);

  TEST_ASSERT_FLOW_DEL(ret, dpid, &flow_mod, &match_list,
                       &error);

  TEST_ASSERT_GROUP_DEL(ret, dpid, &group_mod, &error);
}

void
test_flow_cmd_dump_with_stats(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct instruction_list instruction_list1;
  struct instruction_list instruction_list2;
  struct match_list match_list;
  struct ofp_flow_mod flow_mod;
  struct bucket_list bucket_list;
  struct ofp_group_mod group_mod;
  struct ofp_error error;
  char *str = NULL;
  const char test_str1[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_bridge01\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":1,\n"
      "\"idle_timeout\":4,\n"
      "\"hard_timeout\":5,\n"
      "\"check_overlap\":null,\n"
      "\"cookie\":3,\n"
      "\"in_port\":16,\n"
      "\"actions\":[{\"write_actions\":\n"
      "[{\"output\":1},\n"
      "{\"group\":3},\n"
      "{\"dl_src\":\"00:0c:29:7a:90:b3\"},\n"
      "{\"encap\":{\"packet_type\":4,\n"
      "\"properties\":[{\"port_name\":{\"port_flags\":5,\n"
      "\"name\":\"hoge1\"}}]}},\n"
      "{\"decap\":{\"cur_packet_type\":6,\n"
      "\"new_packet_type\":7,\n"
      "\"properties\":[{\"port_name\":{\"port_flags\":8,\n"
      "\"name\":\"hoge2\"}}]}}]}],\n"
      "\"stats\":{\"packet_count\":0,\n"
      "\"byte_count\":0}},\n"
      "{\"priority\":1,\n"
      "\"idle_timeout\":4,\n"
      "\"hard_timeout\":5,\n"
      "\"check_overlap\":null,\n"
      "\"cookie\":3,\n"
      "\"actions\":[{\"write_actions\":\n"
      "[{\"output\":1},\n"
      "{\"group\":3},\n"
      "{\"dl_src\":\"00:0c:29:7a:90:b3\"},\n"
      "{\"encap\":{\"packet_type\":4,\n"
      "\"properties\":[{\"port_name\":{\"port_flags\":5,\n"
      "\"name\":\"hoge1\"}}]}},\n"
      "{\"decap\":{\"cur_packet_type\":6,\n"
      "\"new_packet_type\":7,\n"
      "\"properties\":[{\"port_name\":{\"port_flags\":8,\n"
      "\"name\":\"hoge2\"}}]}}]}],\n"
      "\"stats\":{\"packet_count\":0,\n"
      "\"byte_count\":0}}]}]}";

  /* group_mod */
  create_buckets(&bucket_list);
  memset(&group_mod, 0, sizeof(group_mod));
  group_mod.group_id = 3;
  group_mod.type = OFPGT_ALL;

  TEST_ASSERT_GROUP_ADD(ret, dpid, &group_mod,
                        &bucket_list, &error);

  /* flow_mod */
  create_matchs(&match_list);
  create_instructions(&instruction_list1);
  create_instructions(&instruction_list2);
  memset(&flow_mod, 0, sizeof(flow_mod));
  flow_mod.table_id = 0;
  flow_mod.priority = 1;
  flow_mod.flags = 2;
  flow_mod.cookie = 3;
  flow_mod.idle_timeout = 4;
  flow_mod.hard_timeout = 5;
  flow_mod.out_port = OFPP_ANY;
  flow_mod.out_group = OFPG_ANY;

  TEST_ASSERT_FLOW_ADD(ret, dpid, &flow_mod, &match_list,
                       &instruction_list1, &error);

  TEST_ASSERT_FLOW_ADD(ret, dpid, &flow_mod, &match_list,
                       &instruction_list2, &error);

  (void) lagopus_dstring_clear(&ds);
  ret = dump_bridge_domains_flow(bridge_name, OFPTT_ALL,
                                 true, true, &ds);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "flow_cmd_dump error.");
  TEST_DSTRING(ret, &ds, str, test_str1, true);

  TEST_ASSERT_FLOW_DEL(ret, dpid, &flow_mod, &match_list,
                       &error);

  TEST_ASSERT_GROUP_DEL(ret, dpid, &group_mod, &error);
}

void
test_destroy(void) {
  destroy = true;
}
