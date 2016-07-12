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
test_set_field_VLAN_VID(void) {
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
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x20;
  OS_MTOD(m, uint8_t *)[15] = 0x01;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 2, OFPXMT_OFB_VLAN_VID << 1,
            0x0f, 0xfe);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[14], 0x2f,
                            "SET_FIELD VLAN_VID[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[15], 0xfe,
                            "SET_FIELD VLAN_VID[1] error.");
}

void
test_set_field_VLAN_PCP(void) {
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
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x0f;
  OS_MTOD(m, uint8_t *)[15] = 0xfe;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 1, OFPXMT_OFB_VLAN_PCP << 1,
            0x2);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[14], 0x4f,
                            "SET_FIELD VLAN_PCP error.");
}
