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
#include "lagopus/datastore/bridge.h"
#include "pktbuf.h"
#include "packet.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"


void
setUp(void) {
  TEST_ASSERT_EQUAL(dp_api_init(), LAGOPUS_RESULT_OK);
}

void
tearDown(void) {
  dp_api_fini();
}

void
test_set_field_IN_PORT(void) {
  datastore_bridge_info_t info;
  struct action_list action_list;
  struct bridge *bridge;
  struct port *port;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct port nport;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  /* setup bridge and port */
  memset(&info, 0, sizeof(info));
  info.fail_mode = DATASTORE_BRIDGE_FAIL_MODE_SECURE;
  TEST_ASSERT_EQUAL(dp_bridge_create("br0", &info), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_create("port0"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_create("port1"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_bridge_port_set("br0", "port0", 1), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_bridge_port_set("br0", "port1", 2), LAGOPUS_RESULT_OK);

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  bridge = dp_bridge_lookup("br0");
  TEST_ASSERT_NOT_NULL(bridge);
  pkt->in_port = port_lookup(&bridge->ports, 1);
  TEST_ASSERT_NOT_NULL(pkt->in_port);
  lagopus_packet_init(pkt, m, pkt->in_port);
  set_match(action_set->field, 4, OFPXMT_OFB_IN_PORT << 1,
            0x00, 0x00, 0x00, 0x02);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->in_port->ofp_port.port_no, 2,
                            "SET_FIELD IN_PORT error.");
}

void
test_set_field_METADATA(void) {
  struct port port;
  static const uint8_t metadata[] =
  { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 };
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

  pkt->oob_data.metadata = 0;
  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 8, OFPXMT_OFB_METADATA << 1,
            0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&pkt->oob_data.metadata, metadata, 8,
                                        "SET_FIELD METADATA error.");
}
