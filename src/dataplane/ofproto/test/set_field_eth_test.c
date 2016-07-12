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
test_set_field_ETH_DST(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 6, OFPXMT_OFB_ETH_DST << 1,
            0x23, 0x45, 0x67, 0x89, 0xab, 0xcd);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[0], 0x23,
                            "SET_FIELD ETH_DST[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[1], 0x45,
                            "SET_FIELD ETH_DST[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[2], 0x67,
                            "SET_FIELD ETH_DST[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[3], 0x89,
                            "SET_FIELD ETH_DST[3] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[4], 0xab,
                            "SET_FIELD ETH_DST[4] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[5], 0xcd,
                            "SET_FIELD ETH_DST[5] error.");
}

void
test_set_field_ETH_SRC(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 6, OFPXMT_OFB_ETH_SRC << 1,
            0x22, 0x44, 0x66, 0x88, 0xaa, 0xcc);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[6], 0x22,
                            "SET_FIELD ETH_SRC[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[7], 0x44,
                            "SET_FIELD ETH_SRC[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[8], 0x66,
                            "SET_FIELD ETH_SRC[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[9], 0x88,
                            "SET_FIELD ETH_SRC[3] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[10], 0xaa,
                            "SET_FIELD ETH_SRC[4] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[11], 0xcc,
                            "SET_FIELD ETH_SRC[5] error.");
}

void
test_set_field_ETH_TYPE(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x20, 0xf1);
  execute_action(pkt, &action_list);

  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[12], 0x20,
                            "SET_FIELD ETH_TYPE[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[13], 0xf1,
                            "SET_FIELD ETH_TYPE[1] error.");
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;

  lagopus_packet_init(pkt, m, &port);
  execute_action(pkt, &action_list);

  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[16], 0x20,
                            "SET_FIELD(vlan) ETH_TYPE[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[17], 0xf1,
                            "SET_FIELD(vlan) ETH_TYPE[1] error.");
}
