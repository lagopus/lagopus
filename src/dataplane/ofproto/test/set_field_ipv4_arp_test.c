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

#include "unity.h"

#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
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
test_set_field_ARP_OP(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x08;
  m->data[13] = 0x06;
  m->data[20] = 0x00;
  m->data[21] = ARPOP_REQUEST;

  lagopus_packet_init(&pkt, m, &port);
  set_match(action_set->field, 2, OFPXMT_OFB_ARP_OP << 1,
            0x00, ARPOP_REPLY);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[20], 0x00,
                            "SET_FIELD ARP_OP[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[21], ARPOP_REPLY,
                            "SET_FIELD ARP_OP[1] error.");
}

void
test_set_field_ARP_SPA(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x08;
  m->data[13] = 0x06;
  m->data[20] = 0x00;
  m->data[21] = ARPOP_REQUEST;
  m->data[28] = 172;
  m->data[29] = 21;
  m->data[30] = 0;
  m->data[31] = 1;

  lagopus_packet_init(&pkt, m, &port);
  set_match(action_set->field, 4, OFPXMT_OFB_ARP_SPA << 1,
            192, 168, 1, 2);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[28], 192,
                            "SET_FIELD ARP_SPA[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[29], 168,
                            "SET_FIELD ARP_SPA[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[30], 1,
                            "SET_FIELD ARP_SPA[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[31], 2,
                            "SET_FIELD ARP_SPA[3] error.");
}

void
test_set_field_ARP_TPA(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x08;
  m->data[13] = 0x06;
  m->data[20] = 0x00;
  m->data[21] = ARPOP_REQUEST;
  m->data[38] = 172;
  m->data[39] = 21;
  m->data[40] = 0;
  m->data[31] = 1;

  lagopus_packet_init(&pkt, m, &port);
  set_match(action_set->field, 4, OFPXMT_OFB_ARP_TPA << 1,
            192, 168, 1, 2);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[38], 192,
                            "SET_FIELD ARP_TPA[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[39], 168,
                            "SET_FIELD ARP_TPA[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[40], 1,
                            "SET_FIELD ARP_TPA[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[41], 2,
                            "SET_FIELD ARP_TPA[3] error.");
}

void
test_set_field_ARP_SHA(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x08;
  m->data[13] = 0x06;
  m->data[20] = 0x00;
  m->data[21] = ARPOP_REQUEST;
  m->data[22] = 0xaa;
  m->data[23] = 0x55;
  m->data[24] = 0xaa;
  m->data[25] = 0x55;
  m->data[26] = 0xaa;
  m->data[27] = 0x55;

  lagopus_packet_init(&pkt, m, &port);
  set_match(action_set->field, 6, OFPXMT_OFB_ARP_SHA << 1,
            0xe0, 0x4d, 0x01, 0x34, 0x56, 0x78);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[22], 0xe0,
                            "SET_FIELD ARP_SHA[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[23], 0x4d,
                            "SET_FIELD ARP_SHA[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[24], 0x01,
                            "SET_FIELD ARP_SHA[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[25], 0x34,
                            "SET_FIELD ARP_SHA[3] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[26], 0x56,
                            "SET_FIELD ARP_SHA[4] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[27], 0x78,
                            "SET_FIELD ARP_SHA[5] error.");
}

void
test_set_field_ARP_THA(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x08;
  m->data[13] = 0x06;
  m->data[20] = 0x00;
  m->data[21] = ARPOP_REQUEST;
  m->data[32] = 0xaa;
  m->data[33] = 0x55;
  m->data[34] = 0xaa;
  m->data[35] = 0x55;
  m->data[36] = 0xaa;
  m->data[37] = 0x55;

  lagopus_packet_init(&pkt, m, &port);
  set_match(action_set->field, 6, OFPXMT_OFB_ARP_THA << 1,
            0xe0, 0x4d, 0x01, 0x34, 0x56, 0x78);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[32], 0xe0,
                            "SET_FIELD ARP_THA[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[33], 0x4d,
                            "SET_FIELD ARP_THA[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[34], 0x01,
                            "SET_FIELD ARP_THA[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[35], 0x34,
                            "SET_FIELD ARP_THA[3] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[36], 0x56,
                            "SET_FIELD ARP_THA[4] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[37], 0x78,
                            "SET_FIELD ARP_THA[5] error.");
}
