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
test_push_MPLS_and_set_field_ETH_DST_SRC(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_push *action_push;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);

  action = calloc(1, sizeof(*action) +
                  sizeof(*action_push) - sizeof(struct ofp_action_header));
  action_push = (struct ofp_action_push *)&action->ofpat;
  action_push->type = OFPAT_PUSH_MPLS;
  action_push->ethertype = 0x8847;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  set_match(action_set->field, 6, OFPXMT_OFB_ETH_DST << 1,
            0x23, 0x45, 0x67, 0x89, 0xab, 0xcd);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  set_match(action_set->field, 6, OFPXMT_OFB_ETH_SRC << 1,
            0x22, 0x44, 0x66, 0x88, 0xaa, 0xcc);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x08;
  m->data[13] = 0x00;
  m->data[14] = 0x45;
  m->data[22] = 240;

  lagopus_set_in_port(&pkt, &port);
  lagopus_packet_init(&pkt, m);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_M_PKTLEN(m), 64 + 4,
                            "PUSH_MPLS length error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[12], 0x88,
                            "PUSH_MPLS ethertype[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[13], 0x47,
                            "PUSH_MPLS ethertype[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[16], 1,
                            "PUSH_MPLS BOS error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[17], 240,
                            "PUSH_MPLS TTL error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[18], 0x45,
                            "PUSH_MPLS payload error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[0], 0x23,
                            "SET_FIELD ETH_DST[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[1], 0x45,
                            "SET_FIELD ETH_DST[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[2], 0x67,
                            "SET_FIELD ETH_DST[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[3], 0x89,
                            "SET_FIELD ETH_DST[3] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[4], 0xab,
                            "SET_FIELD ETH_DST[4] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[5], 0xcd,
                            "SET_FIELD ETH_DST[5] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[6], 0x22,
                            "SET_FIELD ETH_SRC[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[7], 0x44,
                            "SET_FIELD ETH_SRC[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[8], 0x66,
                            "SET_FIELD ETH_SRC[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[9], 0x88,
                            "SET_FIELD ETH_SRC[3] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[10], 0xaa,
                            "SET_FIELD ETH_SRC[4] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[11], 0xcc,
                            "SET_FIELD ETH_SRC[5] error.");
}
