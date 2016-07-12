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

#include "lagopus/dp_apis.h"
#include "lagopus/flowdb.h"
#include "lagopus/ethertype.h"
#include "lagopus/port.h"
#include "lagopus/flowinfo.h"
#include "lagopus/dataplane.h"
#include "pktbuf.h"
#include "packet.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"

static struct port port;
static struct lagopus_packet *pkt;

void
setUp(void) {
  TEST_ASSERT_EQUAL(dp_api_init(), LAGOPUS_RESULT_OK);
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
}

void
tearDown(void) {
  lagopus_packet_free(pkt);
  dp_api_fini();
}

void
test_execute_instruction_GOTO_TABLE(void) {
  struct instruction *insns[INSTRUCTION_INDEX_MAX];
  struct instruction *insn;
  struct ofp_instruction_goto_table *ofp_insn;
  OS_MBUF *m;

  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  insn = calloc(1, sizeof(struct instruction) +
                sizeof(struct ofp_instruction_goto_table) -
                sizeof(struct ofp_instruction));
  TEST_ASSERT_NOT_NULL_MESSAGE(insn, "insn: calloc error.");

  ofp_insn = (struct ofp_instruction_goto_table *)&insn->ofpit;
  ofp_insn->type = OFPIT_GOTO_TABLE;
  lagopus_set_instruction_function(insn);
  ofp_insn->table_id = 0;
  memset(insns, 0, sizeof(insns));
  insns[INSTRUCTION_INDEX_GOTO_TABLE] = insn;

  lagopus_packet_init(pkt, m, &port);
  execute_instruction(pkt, (const struct instruction **)insns);
}

void
test_execute_instruction_WRITE_METADATA(void) {
  struct instruction *insns[INSTRUCTION_INDEX_MAX];
  struct instruction *insn;
  struct ofp_instruction_write_metadata *ofp_insn;

  static const uint8_t md_i[] =
  { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0};
  static const uint8_t md_n[] =
  { 0x00, 0x00, 0x00, 0x55, 0xaa, 0x00, 0x00, 0x00};
  static const uint8_t md_m[] =
  { 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00};
  static const uint8_t md_r[] =
  { 0x12, 0x34, 0x00, 0x55, 0xaa, 0x00, 0xde, 0xf0};

  insn = calloc(1, sizeof(struct instruction) +
                sizeof(struct ofp_instruction_write_metadata) -
                sizeof(struct ofp_instruction));
  TEST_ASSERT_NOT_NULL_MESSAGE(insn, "insn: calloc error.");
  ofp_insn = (struct ofp_instruction_write_metadata *)&insn->ofpit;
  ofp_insn->type = OFPIT_WRITE_METADATA;
  lagopus_set_instruction_function(insn);
  OS_MEMCPY(&pkt->oob_data.metadata, md_i, sizeof(uint64_t));
  ofp_insn->metadata =  /* host byte order */
    (uint64_t)md_n[0] << 56 | (uint64_t)md_n[1] << 48
    | (uint64_t)md_n[2] << 40 | (uint64_t)md_n[3] << 32
    | (uint64_t)md_n[4] << 24 | (uint64_t)md_n[5] << 16
    | (uint64_t)md_n[6] << 8 | (uint64_t)md_n[7];
  ofp_insn->metadata_mask =  /* host byte order */
    (uint64_t)md_m[0] << 56 | (uint64_t)md_m[1] << 48
    | (uint64_t)md_m[2] << 40 | (uint64_t)md_m[3] << 32
    | (uint64_t)md_m[4] << 24 | (uint64_t)md_m[5] << 16
    | (uint64_t)md_m[6] << 8 | (uint64_t)md_m[7];
  memset(insns, 0, sizeof(insns));
  insns[INSTRUCTION_INDEX_WRITE_METADATA] = insn;

  execute_instruction(pkt, (const struct instruction **)insns);
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&pkt->oob_data.metadata, md_r, 8,
                                        "WRITE_METADATA error.");
}

void
test_execute_instruction_WRITE_ACTIONS(void) {
  struct instruction *insns[INSTRUCTION_INDEX_MAX];
  struct instruction insn;
  struct ofp_instruction *ofp_insn;
  struct action *action;
  struct ofp_action_push *action_push;
  OS_MBUF *m;

  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  ofp_insn = &insn.ofpit;
  ofp_insn->type = OFPIT_WRITE_ACTIONS;
  lagopus_set_instruction_function(&insn);
  action = calloc(1, sizeof(*action) +
                  sizeof(*action_push) - sizeof(struct ofp_action_header));
  TEST_ASSERT_NOT_NULL_MESSAGE(action, "action: calloc error.");
  action_push = (struct ofp_action_push *)&action->ofpat;
  action_push->type = OFPAT_PUSH_MPLS;
  action_push->ethertype = 0x8847;
  lagopus_set_action_function(action);
  TAILQ_INIT(&insn.action_list);
  TAILQ_INSERT_TAIL(&insn.action_list, action, entry);
  memset(insns, 0, sizeof(insns));
  insns[INSTRUCTION_INDEX_WRITE_ACTIONS] = &insn;

  lagopus_packet_init(pkt, m, &port);
  execute_instruction(pkt, (const struct instruction **)insns);
  TEST_ASSERT_NOT_NULL_MESSAGE(TAILQ_FIRST(&pkt->actions[2]),
                               "WRITE_ACTIONS(push_mpls) error.");
}

void
test_execute_instruction_CLEAR_ACTIONS(void) {
  struct instruction *insns[INSTRUCTION_INDEX_MAX];
  struct instruction insn;
  struct ofp_instruction *ofp_insn;
  struct action *action;
  struct ofp_action_push *action_push;
  OS_MBUF *m;

  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  ofp_insn = &insn.ofpit;
  ofp_insn->type = OFPIT_CLEAR_ACTIONS;
  lagopus_set_instruction_function(&insn);
  memset(insns, 0, sizeof(insns));
  insns[INSTRUCTION_INDEX_CLEAR_ACTIONS] = &insn;

  lagopus_packet_init(pkt, m, &port);
  TAILQ_INIT(&pkt->actions[2]);
  action = calloc(1, sizeof(*action) +
                  sizeof(*action_push) - sizeof(struct ofp_action_header));
  TEST_ASSERT_NOT_NULL_MESSAGE(action, "action: calloc error.");
  action_push = (struct ofp_action_push *)&action->ofpat;
  action_push->type = OFPAT_PUSH_MPLS;
  action_push->ethertype = 0x8847;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&pkt->actions[2], action, entry);
  pkt->flags |= PKT_FLAG_HAS_ACTION;

  execute_instruction(pkt, (const struct instruction **)insns);
  TEST_ASSERT_NULL_MESSAGE(TAILQ_FIRST(&pkt->actions[2]),
                           "CLEAR_ACTIONS(push_mpls) error.");
}

void
test_execute_instruction_APPLY_ACTIONS(void) {
  struct instruction *insns[INSTRUCTION_INDEX_MAX];
  struct instruction insn;
  struct ofp_instruction *ofp_insn;
  struct action *action;
  struct ofp_action_push *action_push;
  OS_MBUF *m;

  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;
  OS_MTOD(m, uint8_t *)[22] = 240;

  ofp_insn = &insn.ofpit;
  ofp_insn->type = OFPIT_APPLY_ACTIONS;
  lagopus_set_instruction_function(&insn);
  action = calloc(1, sizeof(*action) +
                  sizeof(*action_push) - sizeof(struct ofp_action_header));
  TEST_ASSERT_NOT_NULL_MESSAGE(action, "action: calloc error.");
  action_push = (struct ofp_action_push *)&action->ofpat;
  action_push->type = OFPAT_PUSH_MPLS;
  action_push->ethertype = 0x8847;
  lagopus_set_action_function(action);
  TAILQ_INIT(&insn.action_list);
  TAILQ_INSERT_TAIL(&insn.action_list, action, entry);
  memset(insns, 0, sizeof(insns));
  insns[INSTRUCTION_INDEX_APPLY_ACTIONS] = &insn;

  lagopus_packet_init(pkt, m, &port);
  execute_instruction(pkt, (const struct instruction **)insns);
  TEST_ASSERT_EQUAL_MESSAGE(OS_M_PKTLEN(m), 64 + 4,
                            "APPLY_ACTIONS length error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[12], 0x88,
                            "APPLY_ACTIONS ethertype[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[13], 0x47,
                            "APPLY_ACTIONS ethertype[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[16], 1,
                            "APPLY_ACTIONS BOS error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[17], 240,
                            "APPLY_ACTIONS TTL error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[18], 0x45,
                            "APPLY_ACTIONS payload error.");
}

void
test_execute_instruction_EXPERIMENTER(void) {
  struct instruction *insns[INSTRUCTION_INDEX_MAX];
  struct instruction *insn;
  struct ofp_instruction_experimenter *ofp_insn;
  OS_MBUF *m;

  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  insn = calloc(1, sizeof(struct instruction) +
                sizeof(struct ofp_instruction_experimenter) -
                sizeof(struct ofp_instruction));
  TEST_ASSERT_NOT_NULL_MESSAGE(insn, "insn: calloc error.");

  ofp_insn = (struct ofp_instruction_experimenter *)&insn->ofpit;
  ofp_insn->type = OFPIT_EXPERIMENTER;
  lagopus_set_instruction_function(insn);
  ofp_insn->experimenter = 0;
  memset(insns, 0, sizeof(insns));
  /* insns[INSTRUCTION_INDEX_EXPERIMENTER] = insn; */

  lagopus_packet_init(pkt, m, &port);
  execute_instruction(pkt, (const struct instruction **)insns);
}

void
test_execute_instruction_METER(void) {
  struct instruction *insns[INSTRUCTION_INDEX_MAX];
  struct instruction *insn;
  struct ofp_instruction_meter *ofp_insn;
  OS_MBUF *m;

  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);
  m->refcnt = 1;

  insn = calloc(1, sizeof(struct instruction) +
                sizeof(struct ofp_instruction_meter) -
                sizeof(struct ofp_instruction));
  TEST_ASSERT_NOT_NULL_MESSAGE(insn, "insn: calloc error.");

  ofp_insn = (struct ofp_instruction_meter *)&insn->ofpit;
  ofp_insn->type = OFPIT_METER;
  lagopus_set_instruction_function(insn);
  ofp_insn->meter_id = 1;
  memset(insns, 0, sizeof(insns));
  insns[INSTRUCTION_INDEX_METER] = insn;

  lagopus_packet_init(pkt, m, &port);
  execute_instruction(pkt, (const struct instruction **)insns);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "METER refcnt error.");
}
