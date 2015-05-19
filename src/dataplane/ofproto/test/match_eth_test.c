/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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


static struct flow *test_flow[3];


void
setUp(void) {
}

void
tearDown(void) {
}


void
test_match_flow_eth_type(void) {
  struct lagopus_packet pkt;
  struct flowinfo *flowinfo;
  struct flow *flow;
  struct port port;
  OS_MBUF *m;
  int32_t prio;
  int i, nflow;

  /* prepare packet */
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  /* prepare flow table */
  test_flow[0] = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  test_flow[0]->priority = 3;
  TAILQ_INIT(&test_flow[0]->match_list);
  add_match(&test_flow[0]->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x12, 0x34);

  test_flow[1] = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  test_flow[1]->priority = 2;
  TAILQ_INIT(&test_flow[1]->match_list);
  add_match(&test_flow[1]->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);

  test_flow[2] = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  test_flow[2]->priority = 1;
  TAILQ_INIT(&test_flow[2]->match_list);
  add_match(&test_flow[2]->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x88, 0x47);

  /* create flowinfo */
  flowinfo = new_flowinfo_eth_type();
  nflow = sizeof(test_flow) / sizeof(test_flow[0]);
  for (i = 0; i < nflow; i++) {
    flowinfo->add_func(flowinfo, test_flow[i]);
  }
  /* test */
  m->data[12] = 0x00;
  m->data[13] = 0x00;

  prio = 0;
  lagopus_set_in_port(&pkt, &port);
  lagopus_packet_init(&pkt, m);
  flow = flowinfo->match_func(flowinfo, &pkt, &prio);
  TEST_ASSERT_NULL_MESSAGE(flow, "match_flow_eth_type mismatch error");

  prio = 0;
  m->data[12] = 0x12;
  m->data[13] = 0x34;
  lagopus_packet_init(&pkt, m);
  flow = flowinfo->match_func(flowinfo, &pkt, &prio);
  TEST_ASSERT_EQUAL_MESSAGE(flow, test_flow[0],
                            "match_flow_eth_type[0] match flow error.");
  TEST_ASSERT_EQUAL_MESSAGE(prio, 3,
                            "match_flow_eth_type[0] match prio error.");
  prio = 0;
  m->data[12] = 0x08;
  m->data[13] = 0x00;
  lagopus_packet_init(&pkt, m);
  flow = flowinfo->match_func(flowinfo, &pkt, &prio);
  TEST_ASSERT_EQUAL_MESSAGE(flow, test_flow[1],
                            "match_flow_eth_type[1] match flow error.");
  TEST_ASSERT_EQUAL_MESSAGE(prio, 2,
                            "match_flow_eth_type[1] match prio error.");
  prio = 0;
  m->data[12] = 0x88;
  m->data[13] = 0x47;
  lagopus_packet_init(&pkt, m);
  flow = flowinfo->match_func(flowinfo, &pkt, &prio);
  TEST_ASSERT_EQUAL_MESSAGE(flow, test_flow[2],
                            "match_flow_eth_type[2] match flow error.");
  TEST_ASSERT_EQUAL_MESSAGE(prio, 1,
                            "match_flow_eth_type[2] match prio error.");
}

void
test_match_basic_ETH_DST(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;
  OS_M_PKTLEN(m) = 64;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);
  refresh_match(flow);

  /* Ether dest */
  m->data[0] = 0x11;
  m->data[1] = 0x22;
  m->data[2] = 0x33;
  m->data[3] = 0x44;
  m->data[4] = 0x55;
  m->data[5] = 0x60;
  lagopus_set_in_port(&pkt, &port);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ETH_DST wildcard match error.");
  add_match(&flow->match_list, 6, OFPXMT_OFB_ETH_DST << 1,
            0x11, 0x22, 0x33, 0x44, 0x55, 0x66);
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ETH_DST mismatch error.");
  m->data[0] = 0x11;
  m->data[1] = 0x22;
  m->data[2] = 0x33;
  m->data[3] = 0x44;
  m->data[4] = 0x55;
  m->data[5] = 0x66;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ETH_DST match error.");
  free(m);
}

void
test_match_basic_ETH_DST_W(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  /* Ether dest */
  m->data[0] = 0x33;
  m->data[1] = 0x22;
  m->data[2] = 0x11;
  m->data[3] = 0x44;
  m->data[4] = 0x55;
  m->data[5] = 0x66;
  add_match(&flow->match_list, 12, (OFPXMT_OFB_ETH_DST << 1) + 1,
            0x11, 0x22, 0x33, 0x00, 0x00, 0x00,
            0xff, 0xff, 0xff, 0x00, 0x00, 0x00);
  refresh_match(flow);
  lagopus_set_in_port(&pkt, &port);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ETH_DST_W mismatch error.");
  m->data[0] = 0x11;
  m->data[1] = 0x22;
  m->data[2] = 0x33;
  m->data[3] = 0x44;
  m->data[4] = 0x55;
  m->data[5] = 0x66;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ETH_DST_W match error.");
  free(m);
}

void
test_match_basic_ETH_SRC(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);
  refresh_match(flow);

  /* Ether src */
  m->data[6] = 0xa0;
  m->data[7] = 0xa0;
  m->data[8] = 0xa0;
  m->data[9] = 0xa0;
  m->data[10] = 0xa0;
  m->data[11] = 0xa0;
  lagopus_set_in_port(&pkt, &port);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ETH_SRC wildcard match error.");
  add_match(&flow->match_list, 6, OFPXMT_OFB_ETH_SRC << 1,
            0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff);
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ETH_SRC mismatch error.");
  m->data[6] = 0xaa;
  m->data[7] = 0xbb;
  m->data[8] = 0xcc;
  m->data[9] = 0xdd;
  m->data[10] = 0xee;
  m->data[11] = 0xff;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ETH_SRC match error.");
  free(m);
}

void
test_match_basic_ETH_SRC_W(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  /* Ether src */
  m->data[6] = 0xa0;
  m->data[7] = 0xa0;
  m->data[8] = 0xa0;
  m->data[9] = 0xa0;
  m->data[10] = 0xa0;
  m->data[11] = 0xa0;
  add_match(&flow->match_list, 12, (OFPXMT_OFB_ETH_SRC << 1) + 1,
            0xaa, 0xbb, 0xcc, 0x00, 0x00, 0x00,
            0xff, 0xff, 0xff, 0x00, 0x00, 0x00);
  refresh_match(flow);
  lagopus_set_in_port(&pkt, &port);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ETH_SRC_W mismatch error.");
  m->data[6] = 0xaa;
  m->data[7] = 0xbb;
  m->data[8] = 0xcc;
  m->data[9] = 0xdd;
  m->data[10] = 0xee;
  m->data[11] = 0xff;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ETH_SRC_W match error.");
  free(m);

}
