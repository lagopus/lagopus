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
test_set_field_ARP_OP(void) {
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
  OS_MTOD(m, uint8_t *)[13] = 0x06;
  OS_MTOD(m, uint8_t *)[20] = 0x00;
  OS_MTOD(m, uint8_t *)[21] = ARPOP_REQUEST;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 2, OFPXMT_OFB_ARP_OP << 1,
            0x00, ARPOP_REPLY);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[20], 0x00,
                            "SET_FIELD ARP_OP[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[21], ARPOP_REPLY,
                            "SET_FIELD ARP_OP[1] error.");
}

void
test_set_field_ARP_SPA(void) {
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
  OS_MTOD(m, uint8_t *)[13] = 0x06;
  OS_MTOD(m, uint8_t *)[20] = 0x00;
  OS_MTOD(m, uint8_t *)[21] = ARPOP_REQUEST;
  OS_MTOD(m, uint8_t *)[28] = 172;
  OS_MTOD(m, uint8_t *)[29] = 21;
  OS_MTOD(m, uint8_t *)[30] = 0;
  OS_MTOD(m, uint8_t *)[31] = 1;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 4, OFPXMT_OFB_ARP_SPA << 1,
            192, 168, 1, 2);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[28], 192,
                            "SET_FIELD ARP_SPA[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[29], 168,
                            "SET_FIELD ARP_SPA[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[30], 1,
                            "SET_FIELD ARP_SPA[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[31], 2,
                            "SET_FIELD ARP_SPA[3] error.");
}

void
test_set_field_ARP_TPA(void) {
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
  OS_MTOD(m, uint8_t *)[13] = 0x06;
  OS_MTOD(m, uint8_t *)[20] = 0x00;
  OS_MTOD(m, uint8_t *)[21] = ARPOP_REQUEST;
  OS_MTOD(m, uint8_t *)[38] = 172;
  OS_MTOD(m, uint8_t *)[39] = 21;
  OS_MTOD(m, uint8_t *)[40] = 0;
  OS_MTOD(m, uint8_t *)[31] = 1;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 4, OFPXMT_OFB_ARP_TPA << 1,
            192, 168, 1, 2);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[38], 192,
                            "SET_FIELD ARP_TPA[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[39], 168,
                            "SET_FIELD ARP_TPA[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[40], 1,
                            "SET_FIELD ARP_TPA[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[41], 2,
                            "SET_FIELD ARP_TPA[3] error.");
}

void
test_set_field_ARP_SHA(void) {
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
  OS_MTOD(m, uint8_t *)[13] = 0x06;
  OS_MTOD(m, uint8_t *)[20] = 0x00;
  OS_MTOD(m, uint8_t *)[21] = ARPOP_REQUEST;
  OS_MTOD(m, uint8_t *)[22] = 0xaa;
  OS_MTOD(m, uint8_t *)[23] = 0x55;
  OS_MTOD(m, uint8_t *)[24] = 0xaa;
  OS_MTOD(m, uint8_t *)[25] = 0x55;
  OS_MTOD(m, uint8_t *)[26] = 0xaa;
  OS_MTOD(m, uint8_t *)[27] = 0x55;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 6, OFPXMT_OFB_ARP_SHA << 1,
            0xe0, 0x4d, 0x01, 0x34, 0x56, 0x78);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[22], 0xe0,
                            "SET_FIELD ARP_SHA[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[23], 0x4d,
                            "SET_FIELD ARP_SHA[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[24], 0x01,
                            "SET_FIELD ARP_SHA[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[25], 0x34,
                            "SET_FIELD ARP_SHA[3] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[26], 0x56,
                            "SET_FIELD ARP_SHA[4] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[27], 0x78,
                            "SET_FIELD ARP_SHA[5] error.");
}

void
test_set_field_ARP_THA(void) {
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
  OS_MTOD(m, uint8_t *)[13] = 0x06;
  OS_MTOD(m, uint8_t *)[20] = 0x00;
  OS_MTOD(m, uint8_t *)[21] = ARPOP_REQUEST;
  OS_MTOD(m, uint8_t *)[32] = 0xaa;
  OS_MTOD(m, uint8_t *)[33] = 0x55;
  OS_MTOD(m, uint8_t *)[34] = 0xaa;
  OS_MTOD(m, uint8_t *)[35] = 0x55;
  OS_MTOD(m, uint8_t *)[36] = 0xaa;
  OS_MTOD(m, uint8_t *)[37] = 0x55;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 6, OFPXMT_OFB_ARP_THA << 1,
            0xe0, 0x4d, 0x01, 0x34, 0x56, 0x78);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[32], 0xe0,
                            "SET_FIELD ARP_THA[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[33], 0x4d,
                            "SET_FIELD ARP_THA[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[34], 0x01,
                            "SET_FIELD ARP_THA[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[35], 0x34,
                            "SET_FIELD ARP_THA[3] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[36], 0x56,
                            "SET_FIELD ARP_THA[4] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[37], 0x78,
                            "SET_FIELD ARP_THA[5] error.");
}
