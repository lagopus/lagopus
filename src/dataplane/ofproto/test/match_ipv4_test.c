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

#include <netinet/ip_icmp.h>

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
test_match_basic_IPV4_IP_DSCP(void) {
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

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);
  refresh_match(flow);
  m->data[12] = 0x08;
  m->data[13] = 0x00;
  m->data[14] = 0x45;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_DSCP << 1,
            0x3f);
  m->data[15] = 0x3f;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IP_DSCP mismatch error.");
  m->data[15] = 0xfc;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IP_DSCP match error.");
}

void
test_match_basic_IPV4_IP_ECN(void) {
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

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);
  refresh_match(flow);
  m->data[12] = 0x08;
  m->data[13] = 0x00;
  m->data[14] = 0x45;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_ECN << 1,
            0x3);
  m->data[15] = 0xcc;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IP_ECN mismatch error.");
  m->data[15] = 0x03;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IP_ECN match error.");
}

void
test_match_basic_IPV4_PROTO(void) {
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

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);

  /* IP proto */
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_TCP);

  refresh_match(flow);
  m->data[12] = 0x08;
  m->data[13] = 0x00;
  m->data[14] = 0x45;

  m->data[23] = IPPROTO_UDP;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IP_PROTO mismatch error.");
  m->data[23] = IPPROTO_TCP;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IP_PROTO match error.");
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;

  m->data[27] = IPPROTO_UDP;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IP_PROTO mismatch(vlan) error.");
  m->data[27] = IPPROTO_TCP;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IP_PROTO match(vlan) error.");
  free(m);
}

void
test_match_basic_IPV4_SRC(void) {
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

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  /* IP src */
  add_match(&flow->match_list, 4, OFPXMT_OFB_IPV4_SRC << 1,
            192, 168, 0, 1);
  m->data[30] = 192;
  m->data[31] = 167;
  m->data[32] = 0;
  m->data[33] = 1;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV4_SRC mismatch(1) error.");
  add_match(&flow->match_list, 4, OFPXMT_OFB_IPV4_SRC << 1,
            192, 168, 0, 1);
  refresh_match(flow);
  m->data[30] = 1;
  m->data[31] = 0;
  m->data[32] = 168;
  m->data[33] = 192;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV4_SRC mismatch(2) error.");
  m->data[30] = 192;
  m->data[31] = 168;
  m->data[32] = 0;
  m->data[33] = 1;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV4_SRC match error.");
  free(m);
}

void
test_match_basic_IPV4_SRC_W(void) {
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

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  /* IP src */
  add_match(&flow->match_list, 8, (OFPXMT_OFB_IPV4_SRC << 1) + 1,
            172, 21, 1, 0, 255, 255, 255, 0);

  m->data[30] = 192;
  m->data[31] = 167;
  m->data[32] = 0;
  m->data[33] = 1;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV4_SRC_W mismatch(1) error.");
  m->data[30] = 1;
  m->data[31] = 1;
  m->data[32] = 21;
  m->data[33] = 172;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV4_SRC_W mismatch(2) error.");
  m->data[30] = 172;
  m->data[31] = 21;
  m->data[32] = 1;
  m->data[33] = 2;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV4_SRC_W match error.");
  free(m);
}

void
test_match_basic_IPV4_DST(void) {
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

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  /* IP dst */
  add_match(&flow->match_list, 4, (OFPXMT_OFB_IPV4_DST << 1),
            192, 168, 0, 2);
  m->data[34] = 192;
  m->data[35] = 168;
  m->data[36] = 0;
  m->data[37] = 1;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV4_DST mismatch(1) error.");
  m->data[34] = 2;
  m->data[35] = 0;
  m->data[36] = 168;
  m->data[37] = 192;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV4_DST mismatch(2) error.");
  m->data[34] = 192;
  m->data[35] = 168;
  m->data[36] = 0;
  m->data[37] = 2;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV4_DST match error.");
}

void
test_match_basic_IPV4_DST_W(void) {
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

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  /* IP dst */
  add_match(&flow->match_list, 8, (OFPXMT_OFB_IPV4_DST << 1) + 1,
            192, 168, 0, 0, 255, 255, 255, 0);
  m->data[34] = 192;
  m->data[35] = 168;
  m->data[36] = 1;
  m->data[37] = 2;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV4_DST_W mismatch(1) error.");
  m->data[34] = 2;
  m->data[35] = 0;
  m->data[36] = 168;
  m->data[37] = 192;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV4_DST_W mismatch(2) error.");
  m->data[34] = 192;
  m->data[35] = 168;
  m->data[36] = 0;
  m->data[37] = 2;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV4_DST_W match error.");
}

void
test_match_basic_IPV4_TCP_SRC(void) {
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

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_TCP);
  m->data[27] = IPPROTO_TCP;
  /* TCP_SRC */
  add_match(&flow->match_list, 2, OFPXMT_OFB_TCP_SRC << 1,
            0xf0, 0x00);
  m->data[38] = 0;
  m->data[39] = 0xf0;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "TCP_SRC mismatch error.");
  m->data[38] = 0xf0;
  m->data[39] = 0x00;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "TCP_SRC match error.");
  free(m);
}

void
test_match_basic_IPV4_TCP_DST(void) {
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

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_TCP);
  m->data[27] = IPPROTO_TCP;

  /* TCP_DST */
  add_match(&flow->match_list, 2, OFPXMT_OFB_TCP_DST << 1,
            0, 80);
  m->data[40] = 80;
  m->data[41] = 0;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "TCP_DST mismatch error.");
  m->data[40] = 0;
  m->data[41] = 80;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "TCP_DST match error.");
  free(m);
}

void
test_match_basic_IPV4_UDP_SRC(void) {
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

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_UDP);
  m->data[27] = IPPROTO_UDP;
  /* UDP_SRC */
  add_match(&flow->match_list, 2, OFPXMT_OFB_UDP_SRC << 1,
            0xf0, 0x00);
  m->data[38] = 0;
  m->data[39] = 0xf0;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "UDP_SRC mismatch error.");
  m->data[38] = 0xf0;
  m->data[39] = 0x00;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "UDP_SRC match error.");
  free(m);
}

void
test_match_basic_IPV4_UDP_DST(void) {
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

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_UDP);
  m->data[27] = IPPROTO_UDP;

  /* UDP_DST */
  add_match(&flow->match_list, 2, OFPXMT_OFB_UDP_DST << 1,
            0, 80);
  m->data[40] = 80;
  m->data[41] = 0;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "UDP_DST mismatch error.");
  m->data[40] = 0;
  m->data[41] = 80;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "UDP_DST match error.");
  free(m);
}

void
test_match_basic_IPV4_SCTP_SRC(void) {
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

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_SCTP);
  m->data[27] = IPPROTO_SCTP;
  /* SCTP_SRC */
  add_match(&flow->match_list, 2, OFPXMT_OFB_SCTP_SRC << 1,
            0xf0, 0x00);
  m->data[38] = 0;
  m->data[39] = 0xf0;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "SCTP_SRC mismatch error.");
  m->data[38] = 0xf0;
  m->data[39] = 0x00;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "SCTP_SRC match error.");
  free(m);
}

void
test_match_basic_IPV4_SCTP_DST(void) {
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

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_SCTP);
  m->data[27] = IPPROTO_SCTP;

  /* SCTP_DST */
  add_match(&flow->match_list, 2, OFPXMT_OFB_SCTP_DST << 1,
            0, 80);
  m->data[40] = 80;
  m->data[41] = 0;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "SCTP_DST mismatch error.");
  m->data[40] = 0;
  m->data[41] = 80;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "SCTP_DST match error.");
  free(m);
}

void
test_match_basic_IPV4_ICMP_TYPE(void) {
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

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_ICMP);
  m->data[27] = IPPROTO_ICMP;

  /* ICMPV4_TYPE */
  add_match(&flow->match_list, 1, OFPXMT_OFB_ICMPV4_TYPE << 1,
            ICMP_ECHO);
  m->data[38] = ICMP_ECHOREPLY;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ICMPV4_TYPE mismatch error.");
  m->data[38] = ICMP_ECHO;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ICMPV4_TYPE match error.");
}

void
test_match_basic_IPV4_ICMP_CODE(void) {
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

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x08, 0x00);
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_ICMP);
  m->data[27] = IPPROTO_ICMP;

  /* ICMPV4_CODE */
  add_match(&flow->match_list, 1, OFPXMT_OFB_ICMPV4_CODE << 1,
            ICMP_HOST_UNREACH);
  m->data[39] = ICMP_PORT_UNREACH;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ICMPV4_CODE mismatch error.");
  m->data[39] = ICMP_HOST_UNREACH;
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ICMPV4_CODE match error.");
}
