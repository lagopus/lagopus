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
test_set_field_MPLS_LABEL(void) {
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
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x47;
  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0xff;
  OS_MTOD(m, uint8_t *)[16] = 0xff;
  OS_MTOD(m, uint8_t *)[17] = 0xff;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 4, OFPXMT_OFB_MPLS_LABEL << 1,
            0x00, 0x00, 0x01, 0x00);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[14], 0x00,
                            "SET_FIELD MPLS_LABEL[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[15], 0x10,
                            "SET_FIELD MPLS_LABEL[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[16], 0x0f,
                            "SET_FIELD MPLS_LABEL[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[17], 0xff,
                            "SET_FIELD MPLS_LABEL[3] error.");

  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x48;
  OS_MTOD(m, uint8_t *)[14] = 0x00;
  OS_MTOD(m, uint8_t *)[15] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x00;
  OS_MTOD(m, uint8_t *)[17] = 0x00;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 4, OFPXMT_OFB_MPLS_LABEL << 1,
            0x00, 0x0c, 0xa4, 0x21);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[14], 0xca,
                            "SET_FIELD MPLS_LABEL(2)[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[15], 0x42,
                            "SET_FIELD MPLS_LABEL(2)[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[16], 0x10,
                            "SET_FIELD MPLS_LABEL(2)[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[17], 0x00,
                            "SET_FIELD MPLS_LABEL(2)[3] error.");
}

void
test_set_field_MPLS_TC(void) {
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
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x47;
  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0xff;
  OS_MTOD(m, uint8_t *)[16] = 0xff;
  OS_MTOD(m, uint8_t *)[17] = 0xff;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 1, OFPXMT_OFB_MPLS_TC << 1,
            0x00);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[14], 0xff,
                            "SET_FIELD MPLS_TC[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[15], 0xff,
                            "SET_FIELD MPLS_TC[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[16], 0xf1,
                            "SET_FIELD MPLS_TC[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[17], 0xff,
                            "SET_FIELD MPLS_TC[3] error.");

  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x48;
  OS_MTOD(m, uint8_t *)[14] = 0x00;
  OS_MTOD(m, uint8_t *)[15] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x00;
  OS_MTOD(m, uint8_t *)[17] = 0x00;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 1, OFPXMT_OFB_MPLS_TC << 1,
            0x07);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[14], 0x00,
                            "SET_FIELD MPLS_TC(2)[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[15], 0x00,
                            "SET_FIELD MPLS_TC(2)[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[16], 0x0e,
                            "SET_FIELD MPLS_TC(2)[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[17], 0x00,
                            "SET_FIELD MPLS_TC(2)[3] error.");
}

void
test_set_field_MPLS_BOS(void) {
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
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x47;
  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0xff;
  OS_MTOD(m, uint8_t *)[16] = 0xff;
  OS_MTOD(m, uint8_t *)[17] = 0xff;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 1, OFPXMT_OFB_MPLS_BOS << 1,
            0x00);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[14], 0xff,
                            "SET_FIELD MPLS_BOS[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[15], 0xff,
                            "SET_FIELD MPLS_BOS[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[16], 0xfe,
                            "SET_FIELD MPLS_BOS[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[17], 0xff,
                            "SET_FIELD MPLS_BOS[3] error.");

  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x48;
  OS_MTOD(m, uint8_t *)[14] = 0x00;
  OS_MTOD(m, uint8_t *)[15] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x00;
  OS_MTOD(m, uint8_t *)[17] = 0x00;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 1, OFPXMT_OFB_MPLS_BOS << 1,
            0x01);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[14], 0x00,
                            "SET_FIELD MPLS_BOS(2)[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[15], 0x00,
                            "SET_FIELD MPLS_BOS(2)[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[16], 0x01,
                            "SET_FIELD MPLS_BOS(2)[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[17], 0x00,
                            "SET_FIELD MPLS_BOS(2)[3] error.");
}
