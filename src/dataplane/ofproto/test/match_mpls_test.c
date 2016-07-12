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
test_match_flow_mpls(void) {
  struct lagopus_packet *pkt;
  struct flowinfo *flowinfo;
  struct flow *flow;
  struct port port;
  OS_MBUF *m;
  int32_t prio;
  int i, nflow;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* prepare flow table */
  test_flow[0] = allocate_test_flow(10 * sizeof(struct match));
  test_flow[0]->priority = 3;
  add_match(&test_flow[0]->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x88, 0x47);
  add_match(&test_flow[0]->match_list, 4, OFPXMT_OFB_MPLS_LABEL << 1,
            0x00, 0x00, 0x00, 0x01);

  test_flow[1] = allocate_test_flow(10 * sizeof(struct match));
  test_flow[1]->priority = 2;
  FLOW_ADD_PORT_MATCH(test_flow[1], 2);

  test_flow[2] = allocate_test_flow(10 * sizeof(struct match));
  test_flow[2]->priority = 1;
  FLOW_ADD_PORT_MATCH(test_flow[2], 3);

  /* create flowinfo */
  flowinfo = new_flowinfo_eth_type();
  nflow = sizeof(test_flow) / sizeof(test_flow[0]);
  for (i = 0; i < nflow; i++) {
    flowinfo->add_func(flowinfo, test_flow[i]);
  }

  /* test */
  prio = 0;
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x47;

  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0xff;
  OS_MTOD(m, uint8_t *)[16] = 0xff;
  OS_MTOD(m, uint8_t *)[17] = 0xff;
  lagopus_packet_init(pkt, m, &port);
  flow = flowinfo->match_func(flowinfo, pkt, &prio);
  TEST_ASSERT_NULL_MESSAGE(flow, "match_flow_mpls mismatch error");
  OS_MTOD(m, uint8_t *)[14] = 0x00;
  OS_MTOD(m, uint8_t *)[15] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x1f;
  OS_MTOD(m, uint8_t *)[17] = 0xff;
  flow = flowinfo->match_func(flowinfo, pkt, &prio);
  TEST_ASSERT_EQUAL_MESSAGE(flow, test_flow[0],
                            "match_flow_mpls match flow error.");
  TEST_ASSERT_EQUAL_MESSAGE(prio, 3,
                            "match_flow_mpls match prio error.");
}

void
test_match_basic_MPLS_LABEL(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x88, 0x47);
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x47;

  /* MPLS_LABEL */
  add_match(&flow->match_list, 4, OFPXMT_OFB_MPLS_LABEL << 1,
            0x00, 0x00, 0x00, 0x01);
  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0xff;
  OS_MTOD(m, uint8_t *)[16] = 0xff;
  OS_MTOD(m, uint8_t *)[17] = 0xff;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "MPLS_LABEL mismatch(1) error.");
  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0x1f;
  OS_MTOD(m, uint8_t *)[16] = 0x00;
  OS_MTOD(m, uint8_t *)[17] = 0x00;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "MPLS_LABEL mismatch(2) error.");
  OS_MTOD(m, uint8_t *)[14] = 0x00;
  OS_MTOD(m, uint8_t *)[15] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x1f;
  OS_MTOD(m, uint8_t *)[17] = 0xff;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "MPLS_LABEL match error.");
}

void
test_match_basic_MPLS_TC(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x88, 0x47);
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x47;

  /* MPLS_TC */
  add_match(&flow->match_list, 1, OFPXMT_OFB_MPLS_TC << 1,
            2);
  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0xff;
  OS_MTOD(m, uint8_t *)[16] = 0xff;
  OS_MTOD(m, uint8_t *)[17] = 0xff;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "MPLS_TC mismatch(1) error.");
  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0xf4;
  OS_MTOD(m, uint8_t *)[16] = 0xff;
  OS_MTOD(m, uint8_t *)[17] = 0xff;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "MPLS_TC mismatch(2) error.");
  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0xff;
  OS_MTOD(m, uint8_t *)[16] = 0xf4;
  OS_MTOD(m, uint8_t *)[17] = 0xff;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "MPLS_TC match error.");
}

void
test_match_basic_MPLS_BOS(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x88, 0x47);
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x47;

  /* MPLS_BOS */
  add_match(&flow->match_list, 1, OFPXMT_OFB_MPLS_BOS << 1,
            1);
  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0xff;
  OS_MTOD(m, uint8_t *)[16] = 0xfe;
  OS_MTOD(m, uint8_t *)[17] = 0xff;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "MPLS_BOS mismatch(1) error.");
  OS_MTOD(m, uint8_t *)[14] = 0x00;
  OS_MTOD(m, uint8_t *)[15] = 0x01;
  OS_MTOD(m, uint8_t *)[16] = 0x00;
  OS_MTOD(m, uint8_t *)[17] = 0x00;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "MPLS_BOS mismatch(2) error.");
  OS_MTOD(m, uint8_t *)[14] = 0x00;
  OS_MTOD(m, uint8_t *)[15] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x01;
  OS_MTOD(m, uint8_t *)[17] = 0x00;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "MPLS_BOS match error.");
}

void
test_match_basic_MPLSMC_LABEL(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x88, 0x48);
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x48;

  /* MPLSMC_LABEL */
  add_match(&flow->match_list, 4, OFPXMT_OFB_MPLS_LABEL << 1,
            0x00, 0x00, 0x00, 0x01);
  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0xff;
  OS_MTOD(m, uint8_t *)[16] = 0xff;
  OS_MTOD(m, uint8_t *)[17] = 0xff;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "MPLSMC_LABEL mismatch(1) error.");
  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0x1f;
  OS_MTOD(m, uint8_t *)[16] = 0x00;
  OS_MTOD(m, uint8_t *)[17] = 0x00;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "MPLSMC_LABEL mismatch(2) error.");
  OS_MTOD(m, uint8_t *)[14] = 0x00;
  OS_MTOD(m, uint8_t *)[15] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x1f;
  OS_MTOD(m, uint8_t *)[17] = 0xff;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "MPLSMC_LABEL match error.");
}

void
test_match_basic_MPLSMC_TC(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x88, 0x48);
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x48;

  /* MPLSMC_TC */
  add_match(&flow->match_list, 1, OFPXMT_OFB_MPLS_TC << 1,
            2);
  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0xff;
  OS_MTOD(m, uint8_t *)[16] = 0xff;
  OS_MTOD(m, uint8_t *)[17] = 0xff;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "MPLSMC_TC mismatch(1) error.");
  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0xf4;
  OS_MTOD(m, uint8_t *)[16] = 0xff;
  OS_MTOD(m, uint8_t *)[17] = 0xff;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "MPLSMC_TC mismatch(2) error.");
  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0xff;
  OS_MTOD(m, uint8_t *)[16] = 0xf4;
  OS_MTOD(m, uint8_t *)[17] = 0xff;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "MPLSMC_TC match error.");
}

void
test_match_basic_MPLSMC_BOS(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x88, 0x48);
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x48;

  /* MPLSMC_BOS */
  add_match(&flow->match_list, 1, OFPXMT_OFB_MPLS_BOS << 1,
            1);
  OS_MTOD(m, uint8_t *)[14] = 0xff;
  OS_MTOD(m, uint8_t *)[15] = 0xff;
  OS_MTOD(m, uint8_t *)[16] = 0xfe;
  OS_MTOD(m, uint8_t *)[17] = 0xff;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "MPLSMC_BOS mismatch(1) error.");
  OS_MTOD(m, uint8_t *)[14] = 0x00;
  OS_MTOD(m, uint8_t *)[15] = 0x01;
  OS_MTOD(m, uint8_t *)[16] = 0x00;
  OS_MTOD(m, uint8_t *)[17] = 0x00;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "MPLSMC_BOS mismatch(2) error.");
  OS_MTOD(m, uint8_t *)[14] = 0x00;
  OS_MTOD(m, uint8_t *)[15] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x01;
  OS_MTOD(m, uint8_t *)[17] = 0x00;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "MPLSMC_BOS match error.");
}
