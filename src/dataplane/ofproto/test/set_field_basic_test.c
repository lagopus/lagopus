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
test_set_field_IN_PORT(void) {
  struct action_list action_list;
  struct dpmgr *my_dpmgr;
  struct bridge *bridge;
  struct port *port;
  struct action *action;
  struct ofp_action_set_field *action_set;
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
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  bridge = dpmgr_bridge_lookup(my_dpmgr, "br0");
  TEST_ASSERT_NOT_NULL(bridge);
  pkt.in_port = port_lookup(bridge->ports, 1);
  TEST_ASSERT_NOT_NULL(pkt.in_port);
  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 4, OFPXMT_OFB_IN_PORT << 1,
            0x00, 0x00, 0x00, 0x02);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.in_port->ofp_port.port_no, 2,
                            "SET_FIELD IN_PORT error.");
}

void
test_set_field_METADATA(void) {
  static const uint8_t metadata[] =
  { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 };
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

  pkt.oob_data.metadata = 0;
  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 8, OFPXMT_OFB_METADATA << 1,
            0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&pkt.oob_data.metadata, metadata, 8,
                                        "SET_FIELD METADATA error.");
}
