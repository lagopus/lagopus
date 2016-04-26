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
#include "lagopus/flowinfo.h"
#include "lagopus/group.h"
#include "lagopus/ethertype.h"
#include "lagopus/dp_apis.h"
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
static lagopus_hashmap_t ports;
static struct group_table *group_table;
static const char bridge_name[] = "br0";
static const uint64_t dpid = 12345678;


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
  TEST_ASSERT_EQUAL(lagopus_hashmap_create(&ports,
                                           LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                                           NULL), LAGOPUS_RESULT_OK);
  TEST_ASSERT_NOT_NULL(group_table = group_table_alloc(bridge));
}

void
tearDown(void) {
  TEST_ASSERT_NOT_NULL(bridge);
  TEST_ASSERT_NOT_NULL(flowdb);
  TEST_ASSERT_NOT_NULL(ports);
  TEST_ASSERT_NOT_NULL(group_table);

  TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == dp_bridge_destroy(bridge_name));
  lagopus_hashmap_destroy(&ports, false);
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

static void
make_match(struct match_list *match_list, int n, ...) {
  va_list ap;

  int match_type;
  uint32_t match_arg, match_mask;
  int nbytes, i;

  va_start(ap, n);
  for (i = 0; i < n; i++) {
    nbytes = va_arg(ap, int);
    match_type = va_arg(ap, int);
    match_arg = va_arg(ap, uint32_t);
    if (match_type & 1) {
      match_mask = va_arg(ap, uint32_t);
      switch (nbytes) {
        case 8:
          add_match(match_list, nbytes, match_type,
                    (match_arg >> 24) & 0xff,
                    (match_arg >> 16) & 0xff,
                    (match_arg >>  8) & 0xff,
                    match_arg & 0xff,
                    (match_mask >> 24) & 0xff,
                    (match_mask >> 16) & 0xff,
                    (match_mask >>  8) & 0xff,
                    match_mask & 0xff);
          break;

        case 4:
          add_match(match_list, nbytes, match_type,
                    (match_arg >>  8) & 0xff,
                    match_arg & 0xff,
                    (match_mask >>  8) & 0xff,
                    match_mask & 0xff);
          break;

        case 2:
          add_match(match_list, nbytes, match_type,
                    match_arg & 0xff,
                    match_mask & 0xff);
          break;
      }
    } else {
      switch (nbytes) {
        case 4:
          add_match(match_list, nbytes, match_type,
                    (match_arg >> 24) & 0xff,
                    (match_arg >> 16) & 0xff,
                    (match_arg >>  8) & 0xff,
                    match_arg & 0xff);
          break;

        case 2:
          add_match(match_list, nbytes, match_type,
                    (match_arg >>  8) & 0xff,
                    match_arg & 0xff);
          break;

        case 1:
          add_match(match_list, nbytes, match_type,
                    match_arg & 0xff);
          break;
      }
    }
  }
  va_end(ap);
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

static struct instruction *
add_instruction(struct instruction_list *instruction_list, uint16_t type) {
  struct instruction *insn;
  struct ofp_instruction *ofp_insn;

  insn = calloc(1, sizeof(struct instruction));
  TEST_ASSERT_NOT_NULL(insn);
  if (insn != NULL) {
    ofp_insn = &insn->ofpit;
    ofp_insn->type = type;
    lagopus_set_instruction_function(insn);

    /* TAILQ_INSERT_TAIL(&insns_list, insn, entry); */
    TAILQ_INSERT_TAIL(instruction_list, insn, entry);
  }
  return insn;
}

static void
add_action_output_instruction(struct instruction_list *instruction_list,
                              uint32_t port_number) {
  struct instruction *insn;
  struct action *action;
  struct ofp_action_output *action_output;

  insn = add_instruction(instruction_list, OFPIT_APPLY_ACTIONS);
  if (insn != NULL) {
    action = calloc(1, sizeof(*action) +
                    sizeof(*action_output) - sizeof(struct ofp_action_header));
    TEST_ASSERT_NOT_NULL_MESSAGE(action, "action: calloc error.");
    action_output = (struct ofp_action_output *)&action->ofpat;
    action_output->type = OFPAT_OUTPUT;
    action_output->port = port_number;
    lagopus_set_action_function(action);
    TAILQ_INIT(&insn->action_list);
    TAILQ_INSERT_TAIL(&insn->action_list, action, entry);
  }
}

static void
add_write_action_output_instruction(struct instruction_list *instruction_list,
                                    uint32_t port_number) {
  struct instruction *insn;
  struct action *action;
  struct ofp_action_output *action_output;

  insn = add_instruction(instruction_list, OFPIT_WRITE_ACTIONS);
  if (insn != NULL) {
    action = calloc(1, sizeof(*action) +
                    sizeof(*action_output) - sizeof(struct ofp_action_header));
    TEST_ASSERT_NOT_NULL_MESSAGE(action, "action: calloc error.");
    action_output = (struct ofp_action_output *)&action->ofpat;
    action_output->type = OFPAT_OUTPUT;
    action_output->port = port_number;
    lagopus_set_action_function(action);
    TAILQ_INIT(&insn->action_list);
    TAILQ_INSERT_TAIL(&insn->action_list, action, entry);
  }
}

void
test_flowdb_flow_add(void) {
  struct table *table;
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

  table = flowdb_get_table(flowdb, flow_mod.table_id);

  /* Add. */
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 1);

  /* Add (overriden) */
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 1);

  /* Add (overriden, CHECK_OVERLAP) */
  flow_mod.flags = OFPFF_CHECK_OVERLAP;
  TEST_ASSERT_FLOW_ADD_NG(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 1);

  /* Add (different priority, not overlap) */
  flow_mod.priority = 2;
  flow_mod.flags = 0;
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 2);

  /* Add with match */
  add_port_match(&match_list, 1);
  make_match(&match_list,
             4,
             2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
             1, OFPXMT_OFB_IP_PROTO << 1, 0,
             4, OFPXMT_OFB_IPV4_SRC << 1, 0xffffffff,
             4, OFPXMT_OFB_IPV4_DST << 1, 0xffffffff);
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 3);

  /* Add with match (overriden) */
  add_port_match(&match_list, 1);
  make_match(&match_list,
             4,
             2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
             1, OFPXMT_OFB_IP_PROTO << 1, 0,
             4, OFPXMT_OFB_IPV4_SRC << 1, 0xffffffff,
             4, OFPXMT_OFB_IPV4_DST << 1, 0xffffffff);
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 3);

  /* Add with match (overriden, CHECK_OVERLAP) */
  add_port_match(&match_list, 1);
  make_match(&match_list,
             4,
             2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
             1, OFPXMT_OFB_IP_PROTO << 1, 0,
             4, OFPXMT_OFB_IPV4_SRC << 1, 0xffffffff,
             4, OFPXMT_OFB_IPV4_DST << 1, 0xffffffff);
  flow_mod.flags = OFPFF_CHECK_OVERLAP;
  TEST_ASSERT_FLOW_ADD_NG(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 3);

  /* bad instruction */
  add_instruction(&instruction_list, 0xff);
  TEST_ASSERT_FLOW_ADD_NG(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 3);
  /* duplicate instruction */
  add_instruction(&instruction_list, OFPIT_CLEAR_ACTIONS);
  add_instruction(&instruction_list, OFPIT_CLEAR_ACTIONS);
  TEST_ASSERT_FLOW_ADD_NG(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 3);
#if 0
  flow_mod.table_id = 1;
  TEST_ASSERT_FLOW_ADD_NG(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 1);
#endif /* 0 */

  FLOWDB_DUMP(flowdb, "After addition", stdout);

  /* Cleanup. */
  TEST_ASSERT_FLOW_DELETE_OK(bridge, &flow_mod, &match_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 0);

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
  struct table *table;
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct ofp_error error;
  int count;

  TAILQ_INIT(&match_list);
  TAILQ_INIT(&instruction_list);

  flow_mod.table_id = 0;
  flow_mod.priority = 1;
  flow_mod.flags = OFPFF_SEND_FLOW_REM;
  flow_mod.cookie = 0;

  table = flowdb_get_table(flowdb, flow_mod.table_id);

  add_port_match(&match_list, 1);

  /* Add a flow with a match. */
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 1);

  TAILQ_COUNT(count, struct match, &match_list, entry);
  TEST_ASSERT_EQUAL(0, count);

  FLOWDB_DUMP(flowdb, "After addition", stdout);

  /* Cleanup with a strict delete. */
  flow_mod.command = OFPFC_DELETE_STRICT;
  TEST_ASSERT_FLOW_DELETE_OK(bridge, &flow_mod, &match_list, &error);
  /* The entry is alive because the matches are not equal. */
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 1);

  /* The new flow has taken match_list; make it again. */
  add_port_match(&match_list, 1);

  TEST_ASSERT_FLOW_DELETE_OK(bridge, &flow_mod, &match_list, &error);
  /* The entry is gone now. */
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 0);

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
  struct table *table;
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

  table = flowdb_get_table(flowdb, flow_mod.table_id);

  /* Add two flows. */
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 1);

  add_port_match(&match_list, 1);
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 2);

  FLOWDB_DUMP(flowdb, "After addition", stdout);

  /* Strictly modify the instruction of the flow without the match only. */
  flow_mod.command = OFPFC_MODIFY_STRICT;
  add_write_metadata_instruction(&instruction_list, 0);
  TAILQ_INIT(&match_list);

  TEST_ASSERT_FLOW_MODIFY_OK(bridge, &flow_mod, &match_list,
                             &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 2);

  TEST_ASSERT_HAS_METADATA_WRITE(table->flow_list->flows[0], 0);
  TEST_ASSERT_NO_METADATA_WRITE(table->flow_list->flows[1]);

  FLOWDB_DUMP(flowdb, "After modification (1)", stdout);

  /* Strictly modify the instruction of the flow with the match only. */
  flow_mod.command = OFPFC_MODIFY_STRICT;
  add_write_metadata_instruction(&instruction_list, 1);
  add_port_match(&match_list, 1);

  TEST_ASSERT_FLOW_MODIFY_OK(bridge, &flow_mod, &match_list,
                             &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 2);

  TEST_ASSERT_HAS_METADATA_WRITE(table->flow_list->flows[0], 0);
  TEST_ASSERT_HAS_METADATA_WRITE(table->flow_list->flows[1], 1);

  FLOWDB_DUMP(flowdb, "After modification (2)", stdout);

  /* Non-strictly modify the instruction of all the flow. */
  flow_mod.command = OFPFC_MODIFY;
  add_write_metadata_instruction(&instruction_list, 2);
  TAILQ_INIT(&match_list);

  TEST_ASSERT_FLOW_MODIFY_OK(bridge, &flow_mod, &match_list,
                             &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 2);

  TEST_ASSERT_HAS_METADATA_WRITE(table->flow_list->flows[0], 2);
  TEST_ASSERT_HAS_METADATA_WRITE(table->flow_list->flows[1], 2);

  FLOWDB_DUMP(flowdb, "After modification (3)", stdout);

  /* Non-strictly modify the instruction of all the flow. */
  flow_mod.command = OFPFC_MODIFY;
  add_write_action_output_instruction(&instruction_list, 1);
  TAILQ_INIT(&match_list);
  TEST_ASSERT_FLOW_MODIFY_OK(bridge, &flow_mod, &match_list,
                             &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 2);
  TEST_ASSERT_NO_METADATA_WRITE(table->flow_list->flows[0]);
  TEST_ASSERT_HAS_METADATA_WRITE(table->flow_list->flows[1], 2);

  /* Cleanup. */
  flow_mod.command = OFPFC_DELETE;
  TEST_ASSERT_FLOW_DELETE_OK(bridge, &flow_mod, &match_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 0);

  FLOWDB_DUMP(flowdb, "After cleanup", stdout);
}

/* XXX already covered by test_flowdb_flow_add(), et. al. */
void
test_flowdb_flow_del(void) {
  struct table *table;
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct ofp_error error;

  TAILQ_INIT(&match_list);
  TAILQ_INIT(&instruction_list);

  flow_mod.table_id = 0;
  flow_mod.priority = 1;
  flow_mod.flags = OFPFF_SEND_FLOW_REM;
  flow_mod.cookie = 0;
  flow_mod.out_port = OFPP_ANY;
  flow_mod.out_group = OFPG_ANY;

  table = flowdb_get_table(flowdb, flow_mod.table_id);

  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 1);

  add_port_match(&match_list, 1);
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 2);

  FLOWDB_DUMP(flowdb, "After addition", stdout);

  add_port_match(&match_list, 1);
  TEST_ASSERT_FLOW_DELETE_OK(bridge, &flow_mod, &match_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 1);
  TAILQ_INIT(&match_list);
  TEST_ASSERT_FLOW_DELETE_OK(bridge, &flow_mod, &match_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 0);

  FLOWDB_DUMP(flowdb, "After deletion", stdout);

  /* Cleaned up already. */
}

void
test_flowdb_flow_del_with_cookie(void) {
  struct table *table;
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct ofp_error error;

  TAILQ_INIT(&match_list);
  TAILQ_INIT(&instruction_list);

  flow_mod.table_id = 0;
  flow_mod.priority = 1;
  flow_mod.flags = OFPFF_SEND_FLOW_REM;
  flow_mod.cookie = 12345;
  flow_mod.cookie_mask = 0;
  flow_mod.out_port = OFPP_ANY;
  flow_mod.out_group = OFPG_ANY;

  table = flowdb_get_table(flowdb, flow_mod.table_id);

  flow_mod.command = OFPFC_ADD;
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 1);

  FLOWDB_DUMP(flowdb, "After addition", stdout);

  flow_mod.command = OFPFC_DELETE;
  TEST_ASSERT_FLOW_DELETE_OK(bridge, &flow_mod, &match_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 0);

  FLOWDB_DUMP(flowdb, "After deletion", stdout);

  /* Cleaned up already. */
}

void
test_flowdb_flow_stats(void) {
  struct table *table;
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

  table = flowdb_get_table(flowdb, flow_mod.table_id);

  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 1);

  FLOWDB_DUMP(flowdb, "After addition", stdout);

  request.table_id = 5;
  request.cookie = 0;

  TEST_ASSERT_FLOW_STATS_OK(flowdb, &request, &match_list,
                            &flow_stats_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 1);
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
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 0);

  FLOWDB_DUMP(flowdb, "After cleanup", stdout);
}

void
test_flowdb_flow_aggregate_stats(void) {
  struct table *table;
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct ofp_aggregate_stats_request request;
  struct ofp_aggregate_stats_reply reply;
  struct ofp_error error;
  lagopus_result_t rv;

  TAILQ_INIT(&match_list);
  TAILQ_INIT(&instruction_list);

  /* Add flow entry. */
  flow_mod.table_id = 5;
  flow_mod.priority = 1;
  flow_mod.flags = 0;
  flow_mod.cookie = 0;
  flow_mod.out_port = OFPP_ANY;
  flow_mod.out_group = OFPG_ANY;

  table = flowdb_get_table(flowdb, flow_mod.table_id);

  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 1);

  FLOWDB_DUMP(flowdb, "After addition", stdout);

  request.table_id = 5;
  request.cookie = 0;

  rv = flowdb_aggregate_stats(flowdb, &request, &match_list, &reply, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(reply.flow_count, 1);

  request.table_id = 0;

  rv = flowdb_aggregate_stats(flowdb, &request, &match_list, &reply, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(reply.flow_count, 0);

  request.table_id = 5;
  request.cookie = 1;
  request.cookie_mask = 1;

  rv = flowdb_aggregate_stats(flowdb, &request, &match_list, &reply, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(reply.flow_count, 0);

  request.table_id = OFPTT_ALL;
  request.cookie = 0;
  request.cookie_mask = 0;

  rv = flowdb_aggregate_stats(flowdb, &request, &match_list, &reply, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(reply.flow_count, 1);

  /* Cleanup. */
  TEST_ASSERT_FLOW_DELETE_OK(bridge, &flow_mod, &match_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 0);

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

void
test_ofp_table_stats(void) {
  lagopus_result_t rv;
  struct table_stats_list list;
  struct ofp_error error;

  TAILQ_INIT(&list);
  rv = ofp_table_stats_get(0xff00ff00, &list, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = ofp_table_stats_get(dpid, &list, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_ofp_table_features(void) {
  lagopus_result_t rv;
  struct table_features_list list;
  struct ofp_error error;

  TAILQ_INIT(&list);
  rv = ofp_table_features_get(0xff00ff00, &list, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = ofp_table_features_get(dpid, &list, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_flow_dump(void) {
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct ofp_error error;
  struct table *table;
  FILE *fp;

  TAILQ_INIT(&match_list);
  TAILQ_INIT(&instruction_list);

  flowinfo_init();

  flow_mod.table_id = 0;
  flow_mod.priority = 1;
  flow_mod.flags = 0;
  flow_mod.cookie = 0;
  flow_mod.out_port = OFPP_ANY;

  table = flowdb_get_table(flowdb, flow_mod.table_id);
  TEST_ASSERT_NOT_NULL(table);

  make_match(&match_list,
             6,
             4, OFPXMT_OFB_IN_PORT << 1, 1,
             4, OFPXMT_OFB_IN_PHY_PORT << 1, 1,
             2, OFPXMT_OFB_VLAN_VID << 1, 0,
             2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_MPLS,
             4, OFPXMT_OFB_MPLS_LABEL << 1, 1,
             1, OFPXMT_OFB_MPLS_BOS << 1, 1);
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  make_match(&match_list,
             7,
             2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
             4, OFPXMT_OFB_IPV4_SRC << 1, 0xffffffff,
             4, OFPXMT_OFB_IPV4_DST << 1, 0xffffffff,
             1, OFPXMT_OFB_IP_PROTO << 1, IPPROTO_ICMP,
             1, OFPXMT_OFB_ICMPV4_TYPE << 1, 0,
             1, OFPXMT_OFB_ICMPV4_CODE << 1, 0);
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  make_match(&match_list,
             4,
             2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IPV6,
             1, OFPXMT_OFB_IP_PROTO << 1, IPPROTO_ICMPV6,
             1, OFPXMT_OFB_ICMPV6_TYPE << 1, 0,
             1, OFPXMT_OFB_ICMPV6_CODE << 1, 0);
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  make_match(&match_list,
             4,
             2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
             1, OFPXMT_OFB_IP_PROTO << 1, IPPROTO_TCP,
             2, OFPXMT_OFB_TCP_SRC << 1, 0,
             2, OFPXMT_OFB_TCP_DST << 1, 0);
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  make_match(&match_list,
             4,
             2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
             1, OFPXMT_OFB_IP_PROTO << 1, IPPROTO_UDP,
             2, OFPXMT_OFB_UDP_SRC << 1, 0,
             2, OFPXMT_OFB_UDP_DST << 1, 0);
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  make_match(&match_list,
             4,
             2, OFPXMT_OFB_ETH_TYPE << 1, ETHERTYPE_IP,
             1, OFPXMT_OFB_IP_PROTO << 1, IPPROTO_SCTP,
             2, OFPXMT_OFB_SCTP_SRC << 1, 0,
             2, OFPXMT_OFB_SCTP_DST << 1, 0);
  add_write_metadata_instruction(&instruction_list, 0);
  add_action_output_instruction(&instruction_list, OFPP_ALL);

  /* Add. */
  TEST_ASSERT_FLOW_ADD_OK(bridge, &flow_mod, &match_list,
                          &instruction_list, &error);
  TEST_ASSERT_TABLE_NFLOW(&table, MISC_FLOWS, 6);
  TEST_ASSERT_NOT_NULL(table->flow_list);
  TEST_ASSERT_NOT_NULL(table->flow_list->flows);
  TEST_ASSERT_NOT_NULL(table->flow_list->flows[0]);

  fp = fopen("/dev/null", "rw");
  TEST_ASSERT_NOT_NULL(fp);
  flow_dump(table->flow_list->flows[0], fp);
  fclose(fp);
}
