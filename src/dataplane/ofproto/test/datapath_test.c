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
#include "lagopus/ethertype.h"
#include "lagopus/flowinfo.h"
#include "lagopus/dataplane.h"
#include "lagopus/dp_apis.h"
#include "lagopus/datastore/bridge.h"
#include "pktbuf.h"
#include "packet.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"

void
setUp(void) {
  datastore_bridge_info_t info;
  datastore_interface_info_t ifinfo;
  TEST_ASSERT_EQUAL(dp_api_init(), LAGOPUS_RESULT_OK);

  /* setup bridge and port */
  memset(&info, 0, sizeof(info));
  info.fail_mode = DATASTORE_BRIDGE_FAIL_MODE_SECURE;
  TEST_ASSERT_EQUAL(dp_bridge_create("br0", &info), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_create("port0"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_create("port1"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_interface_create("if0"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_interface_create("if1"), LAGOPUS_RESULT_OK);
  memset(&ifinfo, 0, sizeof(ifinfo));
  ifinfo.eth.port_number = 0;
  TEST_ASSERT_EQUAL(dp_interface_info_set("if0", &ifinfo), LAGOPUS_RESULT_OK);
  ifinfo.eth.port_number = 1;
  TEST_ASSERT_EQUAL(dp_interface_info_set("if1", &ifinfo), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_interface_set("port0", "if0"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_interface_set("port1", "if1"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_bridge_port_set("br0", "port0", 1), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_bridge_port_set("br0", "port1", 2), LAGOPUS_RESULT_OK);
}

void
tearDown(void) {
  TEST_ASSERT_EQUAL(dp_port_interface_unset("port0"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_interface_unset("port1"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_bridge_port_unset("br0", "port0"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_bridge_port_unset("br0", "port1"), LAGOPUS_RESULT_OK);
  dp_interface_destroy("if0");
  dp_interface_destroy("if1");
  dp_port_destroy("port0");
  dp_port_destroy("port1");
  dp_bridge_destroy("br0");
  dp_api_fini();
}

void
test_action_OUTPUT(void) {
  struct action_list action_list;
  struct bridge *bridge;
  struct port *port;
  struct action *action;
  struct ofp_action_output *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  bridge = dp_bridge_lookup("br0");
  TEST_ASSERT_NOT_NULL(bridge);

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_output *)&action->ofpat;
  action_set->type = OFPAT_OUTPUT;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  lagopus_packet_init(pkt, m, port_lookup(&bridge->ports, 1));
  TEST_ASSERT_NOT_NULL(pkt->in_port);

  /* output action always decrement reference count. */
  m->refcnt = 2;
  action_set->port = 1;
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = 2;
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = OFPP_ALL;
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = OFPP_NORMAL;
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = OFPP_IN_PORT;
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = OFPP_CONTROLLER;
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = OFPP_FLOOD;
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = OFPP_LOCAL;
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = 0;
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");
  free(m);
}

void
test_lagopus_match_and_action(void) {
  struct action_list action_list;
  struct bridge *bridge;
  struct table *table;
  struct action *action;
  struct ofp_action_output *action_set;
  struct port *port;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  bridge = dp_bridge_lookup("br0");
  TEST_ASSERT_NOT_NULL(bridge);

  flowdb_switch_mode_set(bridge->flowdb, SWITCH_MODE_OPENFLOW);
  table = flowdb_get_table(bridge->flowdb, 0);
  table->userdata = new_flowinfo_eth_type();

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_output *)&action->ofpat;
  action_set->type = OFPAT_OUTPUT;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  m->refcnt = 2;

  port = port_lookup(&bridge->ports, 1);
  TEST_ASSERT_NOT_NULL(port);
  pkt->table_id = 0;
  pkt->cache = NULL;
  lagopus_packet_init(pkt, m, port);
  TEST_ASSERT_EQUAL(pkt->in_port, port);
  TEST_ASSERT_EQUAL(pkt->in_port->bridge, bridge);
  lagopus_match_and_action(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "match_and_action refcnt error.");
  free(m);
}
