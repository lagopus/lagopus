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


#include "unity.h"

#include "lagopus/dpmgr.h"
#include "lagopus/flowdb.h"
#include "lagopus/ethertype.h"
#include "lagopus/port.h"
#include "lagopus/flowinfo.h"
#include "lagopus/dataplane.h"
#include "match.h"
#include "pktbuf.h"
#include "packet.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"


void
setUp(void) {
}

void
tearDown(void) {
}

void
test_lagopus_set_in_port(void) {
  struct lagopus_packet pkt;
  struct port port;

  lagopus_set_in_port(&pkt, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.in_port, &port,
                            "port error.");
}

void
test_lagopus_receive_packet(void) {
  struct lagopus_packet pkt;
  struct dpmgr *my_dpmgr;
  struct port nport, *port;
  OS_MBUF *m;

  my_dpmgr = dpmgr_alloc();
  dpmgr_bridge_add(my_dpmgr, "br0", 0);
  nport.type = LAGOPUS_PORT_TYPE_PHYSICAL;
  nport.ofp_port.port_no = 1;
  nport.ifindex = 0;
  dpmgr_port_add(my_dpmgr, &nport);
  nport.ofp_port.port_no = 2;
  nport.ifindex = 1;
  dpmgr_port_add(my_dpmgr, &nport);
  dpmgr_bridge_port_add(my_dpmgr, "br0", 0, 1);
  dpmgr_bridge_port_add(my_dpmgr, "br0", 1, 2);

  /* prepare packet */
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "m: calloc error.");
  m->data = &m->dat[128];

  m->data[12] = 0x08;
  m->data[13] = 0x00;
  m->data[14] = 0x45;
  m->data[23] = IPPROTO_TCP;
  m->refcnt = 2;

  pkt.mbuf = (OS_MBUF *)m;
  pkt.in_port = NULL;
  pkt.cache = NULL;
  pkt.table_id = 0;
  pkt.flags = 0;
  port = port_lookup(my_dpmgr->ports, 0);
  lagopus_receive_packet(port, &pkt);
  TEST_ASSERT_NOT_EQUAL_MESSAGE(pkt.in_port, NULL,
                                "port error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "m->refcnt error.");
}

void
test_execute_instruction_GOTO_TABLE(void) {
  struct instruction *insns[INSTRUCTION_INDEX_MAX];
  struct instruction *insn;
  struct lagopus_packet pkt;
  struct ofp_instruction_goto_table *ofp_insn;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "m: calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

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

  lagopus_packet_init(&pkt, m);
  execute_instruction(&pkt, insns);
}

void
test_execute_instruction_WRITE_METADATA(void) {
  struct instruction *insns[INSTRUCTION_INDEX_MAX];
  struct instruction *insn;
  struct lagopus_packet pkt;
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
  OS_MEMCPY(&pkt.oob_data.metadata, md_i, sizeof(uint64_t));
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

  execute_instruction(&pkt, insns);
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&pkt.oob_data.metadata, md_r, 8,
                                        "WRITE_METADATA error.");
}

void
test_execute_instruction_WRITE_ACTIONS(void) {
  struct instruction *insns[INSTRUCTION_INDEX_MAX];
  struct instruction insn;
  struct lagopus_packet pkt;
  struct ofp_instruction *ofp_insn;
  struct action *action;
  struct ofp_action_push *action_push;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "m: calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

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

  lagopus_packet_init(&pkt, m);
  execute_instruction(&pkt, insns);
  TEST_ASSERT_NOT_NULL_MESSAGE(TAILQ_FIRST(&pkt.actions[2]),
                               "WRITE_ACTIONS(push_mpls) error.");
}

void
test_execute_instruction_CLEAR_ACTIONS(void) {
  struct instruction *insns[INSTRUCTION_INDEX_MAX];
  struct instruction insn;
  struct lagopus_packet pkt;
  struct ofp_instruction *ofp_insn;
  struct action *action;
  struct ofp_action_push *action_push;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "m: calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  ofp_insn = &insn.ofpit;
  ofp_insn->type = OFPIT_CLEAR_ACTIONS;
  lagopus_set_instruction_function(&insn);
  memset(insns, 0, sizeof(insns));
  insns[INSTRUCTION_INDEX_CLEAR_ACTIONS] = &insn;

  lagopus_packet_init(&pkt, m);
  action = calloc(1, sizeof(*action) +
                  sizeof(*action_push) - sizeof(struct ofp_action_header));
  TEST_ASSERT_NOT_NULL_MESSAGE(action, "action: calloc error.");
  action_push = (struct ofp_action_push *)&action->ofpat;
  action_push->type = OFPAT_PUSH_MPLS;
  action_push->ethertype = 0x8847;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&pkt.actions[2], action, entry);

  execute_instruction(&pkt, insns);
  TEST_ASSERT_NULL_MESSAGE(TAILQ_FIRST(&pkt.actions[2]),
                           "CLEAR_ACTIONS(push_mpls) error.");
}

void
test_execute_instruction_APPLY_ACTIONS(void) {
  struct instruction *insns[INSTRUCTION_INDEX_MAX];
  struct instruction insn;
  struct lagopus_packet pkt;
  struct ofp_instruction *ofp_insn;
  struct action *action;
  struct ofp_action_push *action_push;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "m: calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x08;
  m->data[13] = 0x00;
  m->data[14] = 0x45;
  m->data[22] = 240;

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

  lagopus_packet_init(&pkt, m);
  execute_instruction(&pkt, insns);
  TEST_ASSERT_EQUAL_MESSAGE(OS_M_PKTLEN(m), 64 + 4,
                            "APPLY_ACTIONS length error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[12], 0x88,
                            "APPLY_ACTIONS ethertype[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[13], 0x47,
                            "APPLY_ACTIONS ethertype[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[16], 1,
                            "APPLY_ACTIONS BOS error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[17], 240,
                            "APPLY_ACTIONS TTL error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[18], 0x45,
                            "APPLY_ACTIONS payload error.");
}

void
test_execute_instruction_EXPERIMENTER(void) {
  struct instruction *insns[INSTRUCTION_INDEX_MAX];
  struct instruction *insn;
  struct lagopus_packet pkt;
  struct ofp_instruction_experimenter *ofp_insn;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "m: calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

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

  lagopus_packet_init(&pkt, m);
  execute_instruction(&pkt, insns);
}

void
test_execute_instruction_METER(void) {
  struct instruction *insns[INSTRUCTION_INDEX_MAX];
  struct instruction *insn;
  struct lagopus_packet pkt;
  struct ofp_instruction_meter *ofp_insn;
  struct dpmgr *my_dpmgr;
  struct bridge *bridge;
  struct port *port;
  struct port nport;
  OS_MBUF *m;

  /* setup bridge and port */
  my_dpmgr = dpmgr_alloc();
  dpmgr_bridge_add(my_dpmgr, "br0", 0);
  nport.type = LAGOPUS_PORT_TYPE_PHYSICAL;
  nport.ofp_port.port_no = 1;
  nport.ifindex = 0;
  dpmgr_port_add(my_dpmgr, &nport);
  port = port_lookup(my_dpmgr->ports, 0);
  TEST_ASSERT_NOT_NULL(port);
  port->ofp_port.hw_addr[0] = 0xff;
  nport.ofp_port.port_no = 2;
  nport.ifindex = 1;
  dpmgr_port_add(my_dpmgr, &nport);
  dpmgr_port_add(my_dpmgr, &nport);
  port = port_lookup(my_dpmgr->ports, 1);
  TEST_ASSERT_NOT_NULL(port);
  port->ofp_port.hw_addr[0] = 0xff;
  dpmgr_bridge_port_add(my_dpmgr, "br0", 0, 1);
  dpmgr_bridge_port_add(my_dpmgr, "br0", 1, 2);

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "m: calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;
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

  bridge = dpmgr_bridge_lookup(my_dpmgr, "br0");
  TEST_ASSERT_NOT_NULL(bridge);
  pkt.in_port = port_lookup(bridge->ports, 1);
  TEST_ASSERT_NOT_NULL(pkt.in_port);
  lagopus_packet_init(&pkt, m);
  execute_instruction(&pkt, insns);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "METER refcnt error.");
}

void
test_action_OUTPUT(void) {
  struct action_list action_list;
  struct dpmgr *my_dpmgr;
  struct bridge *bridge;
  struct port *port;
  struct action *action;
  struct ofp_action_output *action_set;
  struct port nport;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  /* setup bridge and port */
  my_dpmgr = dpmgr_alloc();
  dpmgr_bridge_add(my_dpmgr, "br0", 0);
  nport.type = LAGOPUS_PORT_TYPE_PHYSICAL;
  nport.ofp_port.port_no = 1;
  nport.ifindex = 0;
  dpmgr_port_add(my_dpmgr, &nport);
  port = port_lookup(my_dpmgr->ports, 0);
  TEST_ASSERT_NOT_NULL(port);
  port->ofp_port.hw_addr[0] = 0xff;
  nport.ofp_port.port_no = 2;
  nport.ifindex = 1;
  dpmgr_port_add(my_dpmgr, &nport);
  port = port_lookup(my_dpmgr->ports, 1);
  TEST_ASSERT_NOT_NULL(port);
  port->ofp_port.hw_addr[0] = 0xff;
  dpmgr_bridge_port_add(my_dpmgr, "br0", 0, 1);
  dpmgr_bridge_port_add(my_dpmgr, "br0", 1, 2);

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_output *)&action->ofpat;
  action_set->type = OFPAT_OUTPUT;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  memset(&pkt, 0, sizeof(pkt));

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  m->refcnt = 1;

  bridge = dpmgr_bridge_lookup(my_dpmgr, "br0");
  TEST_ASSERT_NOT_NULL(bridge);
  pkt.in_port = port_lookup(bridge->ports, 1);
  TEST_ASSERT_NOT_NULL(pkt.in_port);
  lagopus_packet_init(&pkt, m);
  action_set->port = 1;
  execute_action(&pkt, &action_list);
  action_set->port = 2;
  execute_action(&pkt, &action_list);
  action_set->port = OFPP_ALL;
  execute_action(&pkt, &action_list);
  action_set->port = OFPP_NORMAL;
  execute_action(&pkt, &action_list);
  action_set->port = OFPP_IN_PORT;
  execute_action(&pkt, &action_list);
  action_set->port = OFPP_CONTROLLER;
  execute_action(&pkt, &action_list);
  action_set->port = OFPP_FLOOD;
  execute_action(&pkt, &action_list);
  action_set->port = OFPP_LOCAL;
  execute_action(&pkt, &action_list);
  action_set->port = 0;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");
}

void
test_lagopus_match_and_action(void) {
  struct action_list action_list;
  struct dpmgr *my_dpmgr;
  struct bridge *bridge;
  struct table *table;
  struct action *action;
  struct ofp_action_output *action_set;
  struct port nport;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  /* setup bridge and port */
  my_dpmgr = dpmgr_alloc();
  dpmgr_bridge_add(my_dpmgr, "br0", 0);
  nport.type = LAGOPUS_PORT_TYPE_PHYSICAL;
  nport.ofp_port.port_no = 1;
  nport.ifindex = 0;
  dpmgr_port_add(my_dpmgr, &nport);
  nport.ofp_port.port_no = 2;
  nport.ifindex = 1;
  dpmgr_port_add(my_dpmgr, &nport);
  dpmgr_bridge_port_add(my_dpmgr, "br0", 0, 1);
  dpmgr_bridge_port_add(my_dpmgr, "br0", 1, 2);
  bridge = dpmgr_bridge_lookup(my_dpmgr, "br0");
  TEST_ASSERT_NOT_NULL(bridge);
  flowdb_switch_mode_set(bridge->flowdb, SWITCH_MODE_OPENFLOW);
  table = table_get(bridge->flowdb, 0);
  table->userdata = new_flowinfo_eth_type();

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_output *)&action->ofpat;
  action_set->type = OFPAT_OUTPUT;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  m->refcnt = 2;

  pkt.in_port = port_lookup(bridge->ports, 1);
  TEST_ASSERT_NOT_NULL(pkt.in_port);
  lagopus_packet_init(&pkt, m);
  pkt.table_id = 0;
  lagopus_match_and_action(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "match_and_action refcnt error.");
}
