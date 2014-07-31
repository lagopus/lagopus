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
test_set_field_PBB_ISID(void) {
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
  m->data[12] = 0x88;
  m->data[13] = 0xe7;
  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 3, OFPXMT_OFB_PBB_ISID << 1,
            0xa5, 0x5a, 0xc3);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[15], 0xa5,
                            "SET_FIELD PBB_ISID[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[16], 0x5a,
                            "SET_FIELD PBB_ISID[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[17], 0xc3,
                            "SET_FIELD PBB_ISID[2] error.");
}
