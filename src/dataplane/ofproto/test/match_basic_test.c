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
#include "pktbuf.h"
#include "packet.h"
#include "lagopus/dataplane.h"
#include "lagopus/flowinfo.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"
#include "datapath_test_match.h"
#include "datapath_test_match_macros.h"


static struct flow *test_flow[3];


void
setUp(void) {
}

void
tearDown(void) {
}

void
test_match_flow_basic(void) {
  struct lagopus_packet *pkt;
  struct flowinfo *flowinfo;
  struct flow *flow;
  struct port port;
  OS_MBUF *m;
  int32_t prio;
  int i, nflow;

  /* prepare flow table */
  test_flow[0] = allocate_test_flow(10 * sizeof(struct match));
  test_flow[0]->priority = 3;
  FLOW_ADD_PORT_MATCH(test_flow[0], 1);

  test_flow[1] = allocate_test_flow(10 * sizeof(struct match));
  test_flow[1]->priority = 2;
  FLOW_ADD_PORT_MATCH(test_flow[1], 2);

  test_flow[2] = allocate_test_flow(10 * sizeof(struct match));
  test_flow[2]->priority = 1;
  FLOW_ADD_PORT_MATCH(test_flow[2], 3);

  /* create flowinfo */
  flowinfo = new_flowinfo_basic();
  nflow = sizeof(test_flow) / sizeof(test_flow[0]);
  for (i = 0; i < nflow; i++) {
    flowinfo->add_func(flowinfo, test_flow[i]);
  }

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "alloc_lagopus_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);
  lagopus_packet_init(pkt, m, &port);

  /* test */
  prio = 0;
  pkt->oob_data.in_port = htonl(1);
  flow = flowinfo->match_func(flowinfo, pkt, &prio);
  TEST_ASSERT_EQUAL_MESSAGE(flow, test_flow[0],
                            "Port 1 match (prio 0) flow error.");
  TEST_ASSERT_EQUAL_MESSAGE(prio, 3,
                            "Port 1 match (prio 0) prio error.");
  prio = 1;
  flow = flowinfo->match_func(flowinfo, pkt, &prio);
  TEST_ASSERT_EQUAL_MESSAGE(flow, test_flow[0],
                            "Port 1 match (prio 1) flow error.");
  TEST_ASSERT_EQUAL_MESSAGE(prio, 3,
                            "Port 1 match (prio 1) prio error.");
  prio = 2;
  flow = flowinfo->match_func(flowinfo, pkt, &prio);
  TEST_ASSERT_EQUAL_MESSAGE(flow, test_flow[0],
                            "Port 1 match (prio 2) flow error.");
  TEST_ASSERT_EQUAL_MESSAGE(prio, 3,
                            "Port 1 match (prio 2) prio error.");
  flow = flowinfo->match_func(flowinfo, pkt, &prio);
  TEST_ASSERT_EQUAL_MESSAGE(flow, NULL,
                            "Port 1 match (prio 3) flow error.");
  prio = 0;
  pkt->oob_data.in_port = htonl(2);
  flow = flowinfo->match_func(flowinfo, pkt, &prio);
  TEST_ASSERT_EQUAL_MESSAGE(flow, test_flow[1],
                            "Port 2 match (prio 0) flow error.");
  TEST_ASSERT_EQUAL_MESSAGE(prio, 2,
                            "Port 2 match (prio 0) prio error.");
  prio = 1;
  flow = flowinfo->match_func(flowinfo, pkt, &prio);
  TEST_ASSERT_EQUAL_MESSAGE(flow, test_flow[1],
                            "Port 2 match (prio 1) flow error.");
  TEST_ASSERT_EQUAL_MESSAGE(prio, 2,
                            "Port 2 match (prio 1) prio error.");
  flow = flowinfo->match_func(flowinfo, pkt, &prio);
  TEST_ASSERT_EQUAL_MESSAGE(flow, NULL,
                            "Port 2 match (prio 2) flow error.");
  TEST_ASSERT_EQUAL_MESSAGE(prio, 2,
                            "Port 2 match (prio 2) prio error.");
  prio = 3;
  flow = flowinfo->match_func(flowinfo, pkt, &prio);
  TEST_ASSERT_EQUAL_MESSAGE(flow, NULL,
                            "Port 2 match (prio 3) flow error.");

  prio = 0;
  pkt->oob_data.in_port = htonl(3);
  flow = flowinfo->match_func(flowinfo, pkt, &prio);
  TEST_ASSERT_EQUAL_MESSAGE(flow, test_flow[2],
                            "Port 3 match (prio 0) flow error.");
  TEST_ASSERT_EQUAL_MESSAGE(prio, 1,
                            "Port 3 match (prio 0) prio error.");
  flow = flowinfo->match_func(flowinfo, pkt, &prio);
  TEST_ASSERT_EQUAL_MESSAGE(flow, NULL,
                            "Port 3 match (prio 1) error.");
  prio = 0;
  pkt->oob_data.in_port = htonl(4);
  flow = flowinfo->match_func(flowinfo, pkt, &prio);
  TEST_ASSERT_NULL_MESSAGE(flow, "mismatch error.");
}

void
test_match_basic_IN_PORT(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "alloc_lagopus_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* prepare flow */
  flow = allocate_test_flow(10 * sizeof(struct match));
  refresh_match(flow);

  /* Port */
  pkt->oob_data.in_port = htonl(2);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IN_PORT wildcard match error.");
  FLOW_ADD_PORT_MATCH(flow, 1);
  refresh_match(flow);
  pkt->oob_data.in_port = htonl(2);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IN_PORT mismatch error.");
  pkt->oob_data.in_port = htonl(1);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IN_PORT match error.");
  free(m);
}

void
test_match_basic_PHY_PORT(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  memset(&port, 0, sizeof(port));
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "alloc_lagopus_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* prepare flow */
  flow = allocate_test_flow(10 * sizeof(struct match));

  /* Port */
  FLOW_ADD_PORT_MATCH(flow, 4);
  add_match(&flow->match_list, 4, OFPXMT_OFB_IN_PHY_PORT << 1,
            0x00, 0x00, 0x00, 0x04);
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  pkt->oob_data.in_port = htonl(4);
  pkt->oob_data.in_phy_port = htonl(1);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IN_PHY_PORT mismatch error.");
  pkt->oob_data.in_phy_port = htonl(4);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IN_PHY_PORT match error.");
}

void
test_match_basic_METADATA(void) {
  static const uint8_t metadata[] =
  { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 };
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "alloc_lagopus_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  /* metadata */
  pkt->oob_data.metadata = 0;
  add_match(&flow->match_list, 8, OFPXMT_OFB_METADATA << 1,
            0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0);
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "METAADTA mismatch error.");
  memcpy(&pkt->oob_data.metadata, metadata, sizeof(pkt->oob_data.metadata));
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "METADATA match error.");
}

void
test_match_basic_METADATA_W(void) {
  static const uint8_t metadata[] =
  { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 };
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "alloc_lagopus_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  /* metadata */
  pkt->oob_data.metadata = 0;
  add_match(&flow->match_list, 16, (OFPXMT_OFB_METADATA << 1) + 1,
            0x12, 0x34, 0x56, 0x78, 0x00, 0x00, 0x00, 0x00,
            0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00);
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "METAADTA_W mismatch error.");
  memcpy(&pkt->oob_data.metadata, metadata, sizeof(pkt->oob_data.metadata));
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "METADATA_W match error.");
}

void
xtest_match_basic_TUNNEL_ID(void) {
}

void
xtest_match_basic_TUNNEL_ID_W(void) {
}
