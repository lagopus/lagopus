/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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
#include "lagopus/flowinfo.h"
#include "lagopus/group.h"
#include "lagopus/vector.h"
#include "openflow13.h"
#include "ofp_action.h"
#include "ofp_match.h"
#include "ofp_instruction.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"

/* Create packet data. */
#define BUF_SIZE 65535

static int
is_hex(char c) {
  if ((c >= '0' && c <= '9') ||
      (c >= 'a' && c <= 'f') ||
      (c >= 'A' && c <= 'F')) {
    return 1;
  }
  return 0;
}

static void
create_packet(char in_data[], struct pbuf **pbuf) {
  int i;
  int j;
  int len = 0;
  char num[3] = {0};

  *pbuf = pbuf_alloc(BUF_SIZE);

  for (i = 0; i < (int) strlen(in_data) - 1; ) {
    j = 0;
    while (j < 2 && i < (int) strlen(in_data)) {
      if (is_hex(in_data[i])) {
        num[j] = in_data[i];
        j++;
      }
      i++;
    }
    *((*pbuf)->putp) = (uint8_t) strtol(num, NULL, 16);
    (*pbuf)->putp++;
    (*pbuf)->plen--;
    len++;
  }
  (*pbuf)->plen = (size_t) len;
}
/* END : Create packet data. */


/*
 * Build a metadata in the host byte order.
 * XXX should be made public.
 */
static uint64_t
build_metadata(const uint8_t *b) {
  return (uint64_t)b[0] << 56 | (uint64_t)b[1] << 48
         | (uint64_t)b[2] << 40 | (uint64_t)b[3] << 32
         | (uint64_t)b[4] << 24 | (uint64_t)b[5] << 16
         | (uint64_t)b[6] << 8 | (uint64_t)b[7];
}


/*
 * Common setup and teardown.
 */
static struct bridge *bridge;
static struct flowdb *flowdb;
static struct vector *ports;
static struct group_table *group_table;
static const char bridge_name[] = "br0";
static const uint64_t dpid = 12345678;

/* XXX */
struct vector *ports_alloc(void);
void ports_free(struct vector *v);


void
setUp(void) {
  datastore_bridge_info_t info;

  TEST_ASSERT_NULL(bridge);
  TEST_ASSERT_NULL(flowdb);
  TEST_ASSERT_NULL(ports);
  TEST_ASSERT_NULL(group_table);

  TEST_ASSERT_EQUAL(dp_api_init(), LAGOPUS_RESULT_OK);
  memset(&info, 0, sizeof(info));
  info.dpid = dpid;
  info.fail_mode = DATASTORE_BRIDGE_FAIL_MODE_SECURE;
  TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == dp_bridge_create(bridge_name, &info));
  TEST_ASSERT_NOT_NULL(bridge = dp_bridge_lookup(bridge_name));
  TEST_ASSERT_NOT_NULL(flowdb = bridge->flowdb);
  TEST_ASSERT_NOT_NULL(ports = ports_alloc());
  TEST_ASSERT_NOT_NULL(group_table = group_table_alloc(bridge));
}

void
tearDown(void) {
  TEST_ASSERT_NOT_NULL(bridge);
  TEST_ASSERT_NOT_NULL(flowdb);
  TEST_ASSERT_NOT_NULL(ports);
  TEST_ASSERT_NOT_NULL(group_table);

  TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == dp_bridge_destroy(bridge_name));
  ports_free(ports);
  group_table_free(group_table);

  ports = NULL;
  group_table = NULL;
  flowdb = NULL;
  bridge = NULL;
}

void
test_ofp_match_list_elem_free(void) {
  struct match_list match_list;
  struct match *match;
  int i;
  int max_cnt = 4;

  TAILQ_INIT(&match_list);

  /* data */
  for (i = 0; i < max_cnt; i++) {
    match = (struct match *)
            malloc(sizeof(struct match));
    TAILQ_INSERT_TAIL(&match_list, match, entry);
  }

  TEST_ASSERT_EQUAL_MESSAGE(TAILQ_EMPTY(&match_list),
                            false, "not match list error.");

  /* call func. */
  ofp_match_list_elem_free(&match_list);

  TEST_ASSERT_EQUAL_MESSAGE(TAILQ_EMPTY(&match_list),
                            true, "match list error.");
}


void
test_ofp_instruction_list_elem_free(void) {
  lagopus_result_t ret;
  struct pbuf *test_pbuf;
  struct instruction_list instruction_list;
  struct ofp_error error;

  /* instruction packet. */
  char nomal_data[] = "00 03 00 18 00 00 00 00"
                      "00 00 00 10 00 00 01 00 00 30 00 00 00 00 00 00";

  /* data */
  TAILQ_INIT(&instruction_list);
  create_packet(nomal_data, &test_pbuf);
  ret = ofp_instruction_parse(test_pbuf, &instruction_list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "ofp_instruction_parse(nomal) error.");

  TEST_ASSERT_EQUAL_MESSAGE(TAILQ_EMPTY(&instruction_list),
                            false, "not instruction list error.");

  /* call func. */
  ofp_instruction_list_elem_free(&instruction_list);

  TEST_ASSERT_EQUAL_MESSAGE(TAILQ_EMPTY(&instruction_list),
                            true, "instruction list error.");
}


void
test_ofp_action_list_elem_free(void) {
  struct action_list action_list;
  struct action *action;
  int i;
  int max_cnt = 4;

  TAILQ_INIT(&action_list);

  /* data */
  for (i = 0; i < max_cnt; i++) {
    action = (struct action *)
             malloc(sizeof(struct action));
    TAILQ_INSERT_TAIL(&action_list, action, entry);
  }

  TEST_ASSERT_EQUAL_MESSAGE(TAILQ_EMPTY(&action_list),
                            false, "not action list error.");

  /* call func. */
  ofp_action_list_elem_free(&action_list);

  TEST_ASSERT_EQUAL_MESSAGE(TAILQ_EMPTY(&action_list),
                            true, "action list error.");
}

static lagopus_result_t
stub_write_metadata(void) {
  fprintf(stdout, "%s.\n", __func__);
  return LAGOPUS_RESULT_OK;
}

/* XXX for the verification in test_flowdb_flow_modify(). */
static const uint8_t md_n[][8] = {
  { 0x00, 0x00, 0x00, 0x55, 0xaa, 0x00, 0x00, 0x00},
  { 0x00, 0x00, 0x00, 0xaa, 0x55, 0x00, 0x00, 0x00},
  { 0x00, 0x00, 0x00, 0x5a, 0x5a, 0x00, 0x00, 0x00},
};
static const uint8_t md_m[] =
{ 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00};
static void
add_write_metadata_instruction(struct instruction_list *instruction_list,
                               int mdset) {
  /*
   * XXX stolen from test_execute_instruction_WRITE_METADATA(); should
   * be made public.
   */
  struct instruction *insn;
  struct ofp_instruction_write_metadata *ofp_insn;

  insn = calloc(1, sizeof(struct instruction) +
                sizeof(struct ofp_instruction_write_metadata) -
                sizeof(struct ofp_instruction));
  TEST_ASSERT_NOT_NULL_MESSAGE(insn, "insn: calloc error.");
  ofp_insn = (struct ofp_instruction_write_metadata *)&insn->ofpit;
  ofp_insn->type = OFPIT_WRITE_METADATA;
  /* lagopus_set_instruction_function(insn); */
  insn->exec = (lagopus_result_t ( *)(struct lagopus_packet *,
                                      const struct instruction *))stub_write_metadata;
  /* OS_MEMCPY(&pkt.of_metadata, md_i, sizeof(uint64_t)); */
  ofp_insn->metadata = build_metadata(md_n[mdset]);
  ofp_insn->metadata_mask =  build_metadata(md_m);
  /* TAILQ_INIT(&insns_list); */
  /* TAILQ_INSERT_TAIL(&insns_list, insn, entry); */
  TAILQ_INSERT_TAIL(instruction_list, insn, entry);

  /* execute_instruction(&pkt, &insns_list); */
  /* TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&pkt.of_metadata, md_r, 8,
     "WRITE_METADATA error."); */
}

void
test_flowdb_flow_add(void) {
  struct table **tablep;
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct ofp_error error;

  TAILQ_INIT(&match_list);
  TAILQ_INIT(&instruction_list);

  flowinfo_init();

  flow_mod.table_id = 0;
  flow_mod.priority = 1;
  flow_mod.flags = 0;
  flow_mod.cookie = 0;
  flow_mod.out_port = OFPP_ANY;
  flow_mod.out_group = OFPG_ANY;

  TABLE_GET(tablep, flowdb, flow_mod.table_id);

  /* Add. */
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 1);

  /* Add (overriden) */
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 1);

  /* Add (overriden, CHECK_OVERLAP) */
  flow_mod.flags = OFPFF_CHECK_OVERLAP;
  TEST_ASSERT_FLOW_ADD_NG(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 1);

  /* Add (different priority, not overlap) */
  flow_mod.priority = 2;
  flow_mod.flags = 0;
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 2);

  /* Add with match */
  add_port_match(&match_list, 1);
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 3);

  /* Add with match (overriden) */
  add_port_match(&match_list, 1);
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 3);

  /* Add with match (overriden, CHECK_OVERLAP) */
  add_port_match(&match_list, 1);
  flow_mod.flags = OFPFF_CHECK_OVERLAP;
  TEST_ASSERT_FLOW_ADD_NG(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 3);

#if 0
  flow_mod.table_id = 1;
  TEST_ASSERT_FLOW_ADD_NG(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 1);
#endif /* 0 */

  FLOWDB_DUMP(flowdb, "After addition", stdout);

  /* Cleanup. */
  TEST_ASSERT_FLOW_DELETE_OK(bridge, &flow_mod, &match_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 0);

  FLOWDB_DUMP(flowdb, "After cleanup", stdout);
}

/*
 * XXX these macros depend on build_metadata() and md_*.
 */

/* Assert if a flow has a Metadata Write instruction. */
#define TEST_ASSERT_HAS_METADATA_WRITE(_fl, _mdset)			\
  do {									\
    struct instruction *_insn;						\
    TEST_ASSERT_NOT_NULL(_fl);						\
    TEST_ASSERT_INSNL_HAS_INSN(_insn, OFPIT_WRITE_METADATA, &(_fl)->instruction_list, entry); \
    TEST_ASSERT_TRUE(build_metadata(md_n[(_mdset)]) == _insn->ofpit_write_metadata.metadata); \
    TEST_ASSERT_TRUE(build_metadata(md_m) == _insn->ofpit_write_metadata.metadata_mask); \
  } while (0)

/* Assert if a flow has no Metadata Write instruction. */
#define TEST_ASSERT_NO_METADATA_WRITE(_fl)				\
  do {									\
    TEST_ASSERT_NOT_NULL(_fl);						\
    TEST_ASSERT_INSNL_NO_INSN(OFPIT_WRITE_METADATA, &(_fl)->instruction_list, entry); \
  } while (0)
void
test_flowdb_flow_del_strict(void) {
  struct table **tablep;
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct ofp_error error;
  int count;

  TAILQ_INIT(&match_list);
  TAILQ_INIT(&instruction_list);

  flow_mod.table_id = 0;
  flow_mod.priority = 1;
  flow_mod.flags = 0;
  flow_mod.cookie = 0;

  TABLE_GET(tablep, flowdb, flow_mod.table_id);

  add_port_match(&match_list, 1);

  /* Add a flow with a match. */
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 1);

  TAILQ_COUNT(count, struct match, &match_list, entry);
  TEST_ASSERT_EQUAL(0, count);

  FLOWDB_DUMP(flowdb, "After addition", stdout);

  /* Cleanup with a strict delete. */
  flow_mod.command = OFPFC_DELETE_STRICT;
  TEST_ASSERT_FLOW_DELETE_OK(bridge, &flow_mod, &match_list, &error);
  /* The entry is alive because the matches are not equal. */
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 1);

  /* The new flow has taken match_list; make it again. */
  add_port_match(&match_list, 1);

  TEST_ASSERT_FLOW_DELETE_OK(bridge, &flow_mod, &match_list, &error);
  /* The entry is gone now. */
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 0);

  TAILQ_COUNT(count, struct match, &match_list, entry);
  TEST_ASSERT_EQUAL(0, count);

  FLOWDB_DUMP(flowdb, "After cleanup", stdout);

  match_list_entry_free(&match_list);
}

/*
 * XXX these macros depend on build_metadata() and md_*.
 */

/* Assert if a flow has a Metadata Write instruction. */
#define TEST_ASSERT_HAS_METADATA_WRITE(_fl, _mdset)			\
  do {									\
    struct instruction *_insn;						\
    TEST_ASSERT_NOT_NULL(_fl);						\
    TEST_ASSERT_INSNL_HAS_INSN(_insn, OFPIT_WRITE_METADATA, &(_fl)->instruction_list, entry); \
    TEST_ASSERT_TRUE(build_metadata(md_n[(_mdset)]) == _insn->ofpit_write_metadata.metadata); \
    TEST_ASSERT_TRUE(build_metadata(md_m) == _insn->ofpit_write_metadata.metadata_mask); \
  } while (0)

/* Assert if a flow has no Metadata Write instruction. */
#define TEST_ASSERT_NO_METADATA_WRITE(_fl)				\
  do {									\
    TEST_ASSERT_NOT_NULL(_fl);						\
    TEST_ASSERT_INSNL_NO_INSN(OFPIT_WRITE_METADATA, &(_fl)->instruction_list, entry); \
  } while (0)
void
test_flowdb_flow_modify(void) {
  struct table **tablep;
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct ofp_error error;

  TAILQ_INIT(&match_list);
  TAILQ_INIT(&instruction_list);

  flow_mod.table_id = 0;
  flow_mod.priority = 1;
  flow_mod.flags = 0;
  flow_mod.cookie = 0;
  flow_mod.out_port = OFPP_ANY;
  flow_mod.out_group = OFPG_ANY;

  TABLE_GET(tablep, flowdb, flow_mod.table_id);

  /* Add two flows. */
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 1);

  add_port_match(&match_list, 1);
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 2);

  FLOWDB_DUMP(flowdb, "After addition", stdout);

  /* Strictly modify the instruction of the flow without the match only. */
  flow_mod.command = OFPFC_MODIFY_STRICT;
  add_write_metadata_instruction(&instruction_list, 0);
  TAILQ_INIT(&match_list);

  TEST_ASSERT_FLOW_MODIFY_OK(bridge, &flow_mod, &match_list,
                             &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 2);

  TEST_ASSERT_HAS_METADATA_WRITE((*tablep)->flow_list.flows[0], 0);
  TEST_ASSERT_NO_METADATA_WRITE((*tablep)->flow_list.flows[1]);

  FLOWDB_DUMP(flowdb, "After modification (1)", stdout);

  /* Strictly modify the instruction of the flow with the match only. */
  flow_mod.command = OFPFC_MODIFY_STRICT;
  add_write_metadata_instruction(&instruction_list, 1);
  add_port_match(&match_list, 1);

  TEST_ASSERT_FLOW_MODIFY_OK(bridge, &flow_mod, &match_list,
                             &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 2);

  TEST_ASSERT_HAS_METADATA_WRITE((*tablep)->flow_list.flows[0], 0);
  TEST_ASSERT_HAS_METADATA_WRITE((*tablep)->flow_list.flows[1], 1);

  FLOWDB_DUMP(flowdb, "After modification (2)", stdout);

  /* Non-strictly modify the instruction of all the flow. */
  flow_mod.command = OFPFC_MODIFY;
  add_write_metadata_instruction(&instruction_list, 2);
  TAILQ_INIT(&match_list);

  TEST_ASSERT_FLOW_MODIFY_OK(bridge, &flow_mod, &match_list,
                             &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 2);

  TEST_ASSERT_HAS_METADATA_WRITE((*tablep)->flow_list.flows[0], 2);
  TEST_ASSERT_HAS_METADATA_WRITE((*tablep)->flow_list.flows[1], 2);

  FLOWDB_DUMP(flowdb, "After modification (3)", stdout);

  /* Cleanup. */
  flow_mod.command = OFPFC_DELETE;
  TEST_ASSERT_FLOW_DELETE_OK(bridge, &flow_mod, &match_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 0);

  FLOWDB_DUMP(flowdb, "After cleanup", stdout);
}

/* XXX already covered by test_flowdb_flow_add(), et. al. */
void
test_flowdb_flow_del(void) {
  struct table **tablep;
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct ofp_error error;

  TAILQ_INIT(&match_list);
  TAILQ_INIT(&instruction_list);

  flow_mod.table_id = 0;
  flow_mod.priority = 1;
  flow_mod.flags = 0;
  flow_mod.cookie = 0;
  flow_mod.out_port = OFPP_ANY;
  flow_mod.out_group = OFPG_ANY;

  TABLE_GET(tablep, flowdb, flow_mod.table_id);

  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 1);

  FLOWDB_DUMP(flowdb, "After addition", stdout);

  TEST_ASSERT_FLOW_DELETE_OK(bridge, &flow_mod, &match_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 0);

  FLOWDB_DUMP(flowdb, "After deletion", stdout);

  /* Cleaned up already. */
}

void
test_flowdb_flow_del_with_cookie(void) {
  struct table **tablep;
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct ofp_error error;

  TAILQ_INIT(&match_list);
  TAILQ_INIT(&instruction_list);

  flow_mod.table_id = 0;
  flow_mod.priority = 1;
  flow_mod.flags = 0;
  flow_mod.cookie = 12345;
  flow_mod.cookie_mask = 0;
  flow_mod.out_port = OFPP_ANY;
  flow_mod.out_group = OFPG_ANY;

  TABLE_GET(tablep, flowdb, flow_mod.table_id);

  flow_mod.command = OFPFC_ADD;
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 1);

  FLOWDB_DUMP(flowdb, "After addition", stdout);

  flow_mod.command = OFPFC_DELETE;
  TEST_ASSERT_FLOW_DELETE_OK(bridge, &flow_mod, &match_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 0);

  FLOWDB_DUMP(flowdb, "After deletion", stdout);

  /* Cleaned up already. */
}

void
test_flowdb_flow_stats(void) {
  struct table **tablep;
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct ofp_flow_stats_request request;
  struct flow_stats_list flow_stats_list;
  struct flow_stats *flow_stats;
  struct ofp_error error;

  TAILQ_INIT(&match_list);
  TAILQ_INIT(&instruction_list);
  TAILQ_INIT(&flow_stats_list);

  /* Add flow entry. */
  flow_mod.table_id = 5;
  flow_mod.priority = 1;
  flow_mod.flags = 0;
  flow_mod.cookie = 0;
  flow_mod.out_port = OFPP_ANY;
  flow_mod.out_group = OFPG_ANY;

  TABLE_GET(tablep, flowdb, flow_mod.table_id);

  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 1);

  FLOWDB_DUMP(flowdb, "After addition", stdout);

  request.table_id = 5;
  request.cookie = 0;

  TEST_ASSERT_FLOW_STATS_OK(flowdb, &request, &match_list,
                            &flow_stats_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 1);
  flow_stats = TAILQ_FIRST(&flow_stats_list);
  TEST_ASSERT_NOT_NULL_MESSAGE(flow_stats,
                               "empty stats error");
  TEST_ASSERT_EQUAL_MESSAGE(flow_stats->ofp.table_id, 5,
                            "table id error");
  TEST_ASSERT_EQUAL_MESSAGE(flow_stats->ofp.duration_sec, 0,
                            "duration sec error");

  sleep(1);

  TAILQ_INIT(&flow_stats_list);
  TEST_ASSERT_FLOW_STATS_OK(flowdb, &request, &match_list,
                            &flow_stats_list, &error);
  flow_stats = TAILQ_FIRST(&flow_stats_list);
  TEST_ASSERT_NOT_NULL_MESSAGE(flow_stats,
                               "empty stats error");
  TEST_ASSERT_EQUAL_MESSAGE(flow_stats->ofp.table_id, 5,
                            "table id error");
  TEST_ASSERT_EQUAL_MESSAGE(flow_stats->ofp.duration_sec, 1,
                            "duration sec error");

  TAILQ_INIT(&flow_stats_list);
  request.table_id = 0;

  TEST_ASSERT_FLOW_STATS_OK(flowdb, &request, &match_list,
                            &flow_stats_list, &error);
  flow_stats = TAILQ_FIRST(&flow_stats_list);
  TEST_ASSERT_NULL_MESSAGE(flow_stats,
                           "empty stats error");

  TAILQ_INIT(&flow_stats_list);
  request.table_id = 5;
  request.cookie = 1;
  request.cookie_mask = 1;

  TEST_ASSERT_FLOW_STATS_OK(flowdb, &request, &match_list,
                            &flow_stats_list, &error);
  flow_stats = TAILQ_FIRST(&flow_stats_list);
  TEST_ASSERT_NULL_MESSAGE(flow_stats,
                           "empty stats error");

  TAILQ_INIT(&flow_stats_list);
  request.table_id = OFPTT_ALL;
  request.cookie = 0;
  request.cookie_mask = 0;

  TEST_ASSERT_FLOW_STATS_OK(flowdb, &request, &match_list,
                            &flow_stats_list, &error);
  flow_stats = TAILQ_FIRST(&flow_stats_list);
  TEST_ASSERT_NOT_NULL_MESSAGE(flow_stats,
                               "empty stats error");
  TEST_ASSERT_EQUAL_MESSAGE(flow_stats->ofp.table_id, 5,
                            "table id error");

  /* Cleanup. */
  TEST_ASSERT_FLOW_DELETE_OK(bridge, &flow_mod, &match_list, &error);
  TEST_ASSERT_TABLE_NFLOW(tablep, MISC_FLOWS, 0);

  FLOWDB_DUMP(flowdb, "After cleanup", stdout);
}

void
test_flowdb_switch_mode(void) {
  size_t i;
  enum switch_mode mode, origmode;
  static const enum switch_mode modes[] = {
                                            SWITCH_MODE_OPENFLOW,
                                            SWITCH_MODE_SECURE,
                                            SWITCH_MODE_STANDALONE,
                                          };

  TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == flowdb_switch_mode_get(flowdb,
                   &origmode));

  for (i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
    mode = modes[i];
    TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == flowdb_switch_mode_set(flowdb, mode));
    mode = 0;
    TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == flowdb_switch_mode_get(flowdb, &mode));
    TEST_ASSERT_TRUE(modes[i] == mode);
  }

  TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == flowdb_switch_mode_set(flowdb,
                   origmode));
}
