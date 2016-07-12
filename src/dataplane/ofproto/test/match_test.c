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
#include "lagopus/datastore/bridge.h"
#include "pktbuf.h"
#include "packet.h"
#include "lagopus/dataplane.h"
#include "lagopus/flowinfo.h"
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
test_lagopus_find_flow(void) {
  datastore_bridge_info_t info;
  struct bridge *bridge;
  struct port *port;
  struct port nport;
  struct lagopus_packet *pkt;
  struct table *table;
  struct flow *flow;
  OS_MBUF *m;

  /* setup bridge and port */
  memset(&info, 0, sizeof(info));
  info.fail_mode = DATASTORE_BRIDGE_FAIL_MODE_SECURE;
  TEST_ASSERT_EQUAL(dp_bridge_create("br0", &info), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_create("port0"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_create("port1"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_bridge_port_set("br0", "port0", 1), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_bridge_port_set("br0", "port1", 2), LAGOPUS_RESULT_OK);

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);
  m->refcnt = 2;

  bridge = dp_bridge_lookup("br0");
  TEST_ASSERT_NOT_NULL(bridge);
  lagopus_packet_init(pkt, m, port_lookup(&bridge->ports, 1));
  table = flowdb_get_table(pkt->in_port->bridge->flowdb, 0);
  table->userdata = new_flowinfo_eth_type();
  flow = lagopus_find_flow(pkt, table);
  TEST_ASSERT_EQUAL_MESSAGE(table->lookup_count, 0,
                            "lookup_count(misc) error.");
  TEST_ASSERT_NULL_MESSAGE(flow,
                           "flow(misc) error.");
  OS_MTOD(m, uint8_t *)[14] = 0x08;
  OS_MTOD(m, uint8_t *)[15] = 0x06;
  lagopus_packet_init(pkt, m, &port);
  flow = lagopus_find_flow(pkt, table);
  TEST_ASSERT_EQUAL_MESSAGE(table->lookup_count, 0,
                            "lookup_count(arp) error.");
  TEST_ASSERT_NULL_MESSAGE(flow,
                           "flow(arp) error.");
  OS_MTOD(m, uint8_t *)[14] = 0x08;
  OS_MTOD(m, uint8_t *)[15] = 0x00;
  lagopus_packet_init(pkt, m, port_lookup(&bridge->ports, 1));
  flow = lagopus_find_flow(pkt, table);
  TEST_ASSERT_EQUAL_MESSAGE(table->lookup_count, 0,
                            "lookup_count(ipv4) error.");
  TEST_ASSERT_NULL_MESSAGE(flow,
                           "flow(ipv4) error.");
  OS_MTOD(m, uint8_t *)[14] = 0x86;
  OS_MTOD(m, uint8_t *)[15] = 0xdd;
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_TCP;
  lagopus_packet_init(pkt, m, port_lookup(&bridge->ports, 1));
  flow = lagopus_find_flow(pkt, table);
  TEST_ASSERT_EQUAL_MESSAGE(table->lookup_count, 0,
                            "lookup_count(ipv6) error.");
  TEST_ASSERT_NULL_MESSAGE(flow,
                           "flow(ipv6) error.");
  OS_MTOD(m, uint8_t *)[14] = 0x88;
  OS_MTOD(m, uint8_t *)[15] = 0x47;
  lagopus_packet_init(pkt, m, port_lookup(&bridge->ports, 1));
  flow = lagopus_find_flow(pkt, table);
  TEST_ASSERT_EQUAL_MESSAGE(table->lookup_count, 0,
                            "lookup_count(mpls) error.");
  TEST_ASSERT_NULL_MESSAGE(flow,
                           "flow(mpls) error.");
  OS_MTOD(m, uint8_t *)[14] = 0x88;
  OS_MTOD(m, uint8_t *)[15] = 0x48;
  lagopus_packet_init(pkt, m, port_lookup(&bridge->ports, 1));
  flow = lagopus_find_flow(pkt, table);
  TEST_ASSERT_EQUAL_MESSAGE(table->lookup_count, 0,
                            "lookup_count(mpls-mc) error.");
  TEST_ASSERT_NULL_MESSAGE(flow,
                           "flow(mpls-mc) error.");
  OS_MTOD(m, uint8_t *)[14] = 0x88;
  OS_MTOD(m, uint8_t *)[15] = 0xe7;
  lagopus_packet_init(pkt, m, port_lookup(&bridge->ports, 1));
  flow = lagopus_find_flow(pkt, table);
  TEST_ASSERT_EQUAL_MESSAGE(table->lookup_count, 0,
                            "lookup_count(pbb) error.");
  TEST_ASSERT_NULL_MESSAGE(flow,
                           "flow(pbb) error.");
}
