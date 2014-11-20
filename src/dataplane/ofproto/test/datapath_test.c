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
#include "lagopus/ethertype.h"
#include "lagopus/port.h"
#include "lagopus/flowinfo.h"
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
test_lagopus_set_in_port(void) {
  struct lagopus_packet pkt;
  struct port port;

  lagopus_set_in_port(&pkt, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.in_port, &port,
                            "port error.");
}

void
test_lagopus_receive_packet(void) {
  struct lagopus_packet pkt;
  struct dpmgr *my_dpmgr;
  struct port nport, *port;
  struct bridge *bridge;
  OS_MBUF *m;
  lagopus_result_t rv;

  my_dpmgr = dpmgr_alloc();
  TEST_ASSERT_NOT_NULL_MESSAGE(my_dpmgr, "alloc error");
  rv = dpmgr_bridge_add(my_dpmgr, "br0", 0);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK, "bridge add error\n");

  nport.type = LAGOPUS_PORT_TYPE_NULL; /* for test */
  nport.ifindex = 0;
  nport.ofp_port.hw_addr[0] = 1;
  dpmgr_port_add(my_dpmgr, &nport);
  nport.ifindex = 1;
  dpmgr_port_add(my_dpmgr, &nport);

  rv = dpmgr_bridge_port_add(my_dpmgr, "br0", 0, 1);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dpmgr_bridge_port_add(my_dpmgr, "br0", 1, 2);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  bridge = dpmgr_bridge_lookup(my_dpmgr, "br0");
  TEST_ASSERT_NOT_NULL(bridge);

  /* prepare packet */
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "m: calloc error.");
  m->data = &m->dat[128];

  m->data[12] = 0x08;
  m->data[13] = 0x00;
  m->data[14] = 0x45;
  m->data[23] = IPPROTO_TCP;
  m->refcnt = 2;

  pkt.cache = NULL;
  pkt.table_id = 0;
  pkt.flags = 0;

  port = port_lookup(bridge->ports, 1);
  TEST_ASSERT_NOT_NULL_MESSAGE(port, "port lookup error.");
  lagopus_set_in_port(&pkt, port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.in_port, port, "port error.");
  TEST_ASSERT_EQUAL(pkt.in_port->bridge, bridge);

  lagopus_packet_init(&pkt, m);
  lagopus_receive_packet(port, &pkt);
  TEST_ASSERT_NOT_EQUAL_MESSAGE(pkt.in_port, NULL,
                                "port error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "m->refcnt error.");
  dpmgr_free(my_dpmgr);
}

void
test_action_OUTPUT(void) {
  struct action_list action_list;
  struct dpmgr *my_dpmgr;
  struct bridge *bridge;
  struct port *port;
  struct action *action;
  struct ofp_action_output *action_set;
  struct port nport;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  /* setup bridge and port */
  my_dpmgr = dpmgr_alloc();
  dpmgr_bridge_add(my_dpmgr, "br0", 0);
  nport.type = LAGOPUS_PORT_TYPE_NULL; /* for test */
  nport.ofp_port.port_no = 1;
  nport.ifindex = 0;
  nport.ofp_port.hw_addr[0] = 1;
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
  action_set = (struct ofp_action_output *)&action->ofpat;
  action_set->type = OFPAT_OUTPUT;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  memset(&pkt, 0, sizeof(pkt));

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  bridge = dpmgr_bridge_lookup(my_dpmgr, "br0");
  TEST_ASSERT_NOT_NULL(bridge);
  lagopus_set_in_port(&pkt, port_lookup(bridge->ports, 1));
  TEST_ASSERT_NOT_NULL(pkt.in_port);
  lagopus_packet_init(&pkt, m);

  /* output action always decrement reference count. */
  m->refcnt = 2;
  action_set->port = 1;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = 2;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = OFPP_ALL;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = OFPP_NORMAL;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = OFPP_IN_PORT;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = OFPP_CONTROLLER;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = OFPP_FLOOD;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = OFPP_LOCAL;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");

  m->refcnt = 2;
  action_set->port = 0;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "OUTPUT refcnt error.");
  free(m);
  dpmgr_free(my_dpmgr);
}

void
test_lagopus_match_and_action(void) {
  struct action_list action_list;
  struct dpmgr *my_dpmgr;
  struct bridge *bridge;
  struct table *table;
  struct action *action;
  struct ofp_action_output *action_set;
  struct port nport, *port;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  /* setup bridge and port */
  my_dpmgr = dpmgr_alloc();
  dpmgr_bridge_add(my_dpmgr, "br0", 0);

  nport.type = LAGOPUS_PORT_TYPE_NULL; /* for test */
  nport.ifindex = 0;
  nport.ofp_port.hw_addr[0] = 1;
  dpmgr_port_add(my_dpmgr, &nport);
  nport.ifindex = 1;
  dpmgr_port_add(my_dpmgr, &nport);

  dpmgr_bridge_port_add(my_dpmgr, "br0", 0, 1);
  dpmgr_bridge_port_add(my_dpmgr, "br0", 1, 2);

  bridge = dpmgr_bridge_lookup(my_dpmgr, "br0");
  TEST_ASSERT_NOT_NULL(bridge);

  flowdb_switch_mode_set(bridge->flowdb, SWITCH_MODE_OPENFLOW);
  table = table_get(bridge->flowdb, 0);
  table->userdata = new_flowinfo_eth_type();

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_output *)&action->ofpat;
  action_set->type = OFPAT_OUTPUT;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  m->refcnt = 2;

  port = port_lookup(bridge->ports, 1);
  TEST_ASSERT_NOT_NULL(port);
  lagopus_set_in_port(&pkt, port);
  TEST_ASSERT_EQUAL(pkt.in_port, port);
  TEST_ASSERT_EQUAL(pkt.in_port->bridge, bridge);
  pkt.table_id = 0;
  lagopus_packet_init(&pkt, m);
  lagopus_match_and_action(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(m->refcnt, 1,
                            "match_and_action refcnt error.");
  free(m);
  dpmgr_free(my_dpmgr);
}
