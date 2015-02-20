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
#include "pktbuf.h"
#include "packet.h"
#include "lagopus/dataplane.h"
#include "lagopus/flowinfo.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"


void
setUp(void) {
}

void
tearDown(void) {
}

void
test_lagopus_find_flow(void) {
  struct dpmgr *my_dpmgr;
  struct bridge *bridge;
  struct port *port;
  struct port nport;
  struct lagopus_packet pkt;
  struct table *table;
  struct flow *flow;
  OS_MBUF *m;

  /* setup bridge and port */
  my_dpmgr = dpmgr_alloc();
  dpmgr_bridge_add(my_dpmgr, "br0", 0);
  nport.type = LAGOPUS_PORT_TYPE_NULL; /* for test */
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

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;
  m->refcnt = 2;

  bridge = dpmgr_bridge_lookup(my_dpmgr, "br0");
  TEST_ASSERT_NOT_NULL(bridge);
  lagopus_set_in_port(&pkt, port_lookup(bridge->ports, 1));
  table = table_get(pkt.in_port->bridge->flowdb, 0);
  table->userdata = new_flowinfo_eth_type();
  lagopus_packet_init(&pkt, m);
  flow = lagopus_find_flow(&pkt, table);
  TEST_ASSERT_EQUAL_MESSAGE(table->lookup_count, 1,
                            "lookup_count(misc) error.");
  TEST_ASSERT_NULL_MESSAGE(flow,
                           "flow(misc) error.");
  m->data[14] = 0x08;
  m->data[15] = 0x06;
  lagopus_packet_init(&pkt, m);
  flow = lagopus_find_flow(&pkt, table);
  TEST_ASSERT_EQUAL_MESSAGE(table->lookup_count, 2,
                            "lookup_count(arp) error.");
  TEST_ASSERT_NULL_MESSAGE(flow,
                           "flow(arp) error.");
  m->data[14] = 0x08;
  m->data[15] = 0x00;
  lagopus_packet_init(&pkt, m);
  flow = lagopus_find_flow(&pkt, table);
  TEST_ASSERT_EQUAL_MESSAGE(table->lookup_count, 3,
                            "lookup_count(ipv4) error.");
  TEST_ASSERT_NULL_MESSAGE(flow,
                           "flow(ipv4) error.");
  m->data[14] = 0x86;
  m->data[15] = 0xdd;
  m->data[20] = IPPROTO_TCP;
  lagopus_packet_init(&pkt, m);
  flow = lagopus_find_flow(&pkt, table);
  TEST_ASSERT_EQUAL_MESSAGE(table->lookup_count, 4,
                            "lookup_count(ipv6) error.");
  TEST_ASSERT_NULL_MESSAGE(flow,
                           "flow(ipv6) error.");
  m->data[14] = 0x88;
  m->data[15] = 0x47;
  lagopus_packet_init(&pkt, m);
  flow = lagopus_find_flow(&pkt, table);
  TEST_ASSERT_EQUAL_MESSAGE(table->lookup_count, 5,
                            "lookup_count(mpls) error.");
  TEST_ASSERT_NULL_MESSAGE(flow,
                           "flow(mpls) error.");
  m->data[14] = 0x88;
  m->data[15] = 0x48;
  lagopus_packet_init(&pkt, m);
  flow = lagopus_find_flow(&pkt, table);
  TEST_ASSERT_EQUAL_MESSAGE(table->lookup_count, 6,
                            "lookup_count(mpls-mc) error.");
  TEST_ASSERT_NULL_MESSAGE(flow,
                           "flow(mpls-mc) error.");
  m->data[14] = 0x88;
  m->data[15] = 0xe7;
  lagopus_packet_init(&pkt, m);
  flow = lagopus_find_flow(&pkt, table);
  TEST_ASSERT_EQUAL_MESSAGE(table->lookup_count, 7,
                            "lookup_count(pbb) error.");
  TEST_ASSERT_NULL_MESSAGE(flow,
                           "flow(pbb) error.");
}
