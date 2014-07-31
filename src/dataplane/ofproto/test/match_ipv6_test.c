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

#include <netinet/icmp6.h>

#include "lagopus/dpmgr.h"
#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "match.h"
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
test_match_basic_IPV6_IP_DSCP(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_DSCP << 1,
            0x3f);
  m->data[14] = 0x00;
  m->data[15] = 0x00;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_IP_DSCP mismatch error.");
  m->data[14] = 0x0f;
  m->data[15] = 0xc0;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_IP_DSCP match error.");
}

void
test_match_basic_IPV6_IP_ECN(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_ECN << 1,
            0x03);
  m->data[15] = 0x00;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_IP_ECN mismatch error.");
  m->data[15] = 0x30;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_IP_ECN match error.");
}

void
test_match_basic_IPV6_PROTO(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);

  /* IP proto */
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_TCP);
  refresh_match(flow);

  m->data[12] = 0x86;
  m->data[13] = 0xdd;

  m->data[20] = IPPROTO_UDP;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_IP_PROTO mismatch error.");
  m->data[20] = IPPROTO_TCP;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_IP_PROTO match error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_UDP;
  m->data[55] = 0;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_IP_PROTO mismatch(next hdr) error.");
  m->data[54] = IPPROTO_TCP;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_IP_PROTO match(next hdr) error.");
  free(m);
}

void
test_match_basic_IPV6_SRC(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;
  int i;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  i = 22;
  m->data[i++] = 0x20;
  m->data[i++] = 0x01;
  m->data[i++] = 0x20;
  m->data[i++] = 0xff;
  m->data[i++] = 0x00;
  m->data[i++] = 0x00;
  m->data[i++] = 0x10;
  m->data[i++] = 0x20;
  m->data[i++] = 0x11;
  m->data[i++] = 0xe1;
  m->data[i++] = 0x50;
  m->data[i++] = 0x89;
  m->data[i++] = 0xbf;
  m->data[i++] = 0xc6;
  m->data[i++] = 0x43;
  m->data[i++] = 0x11;
  lagopus_packet_init(&pkt, m);
  add_match(&flow->match_list, 16, OFPXMT_OFB_IPV6_SRC << 1,
            0x20, 0x01, 0x00, 0x00, 0xe0, 0x45, 0x22, 0xeb,
            0x09, 0x00, 0x00, 0x08, 0xdc, 0x18, 0x94, 0xad);
  refresh_match(flow);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_SRC mismatch error.");
  i = 22;
  m->data[i++] = 0x20;
  m->data[i++] = 0x01;
  m->data[i++] = 0x00;
  m->data[i++] = 0x00;
  m->data[i++] = 0xe0;
  m->data[i++] = 0x45;
  m->data[i++] = 0x22;
  m->data[i++] = 0xeb;
  m->data[i++] = 0x09;
  m->data[i++] = 0x00;
  m->data[i++] = 0x00;
  m->data[i++] = 0x08;
  m->data[i++] = 0xdc;
  m->data[i++] = 0x18;
  m->data[i++] = 0x94;
  m->data[i++] = 0xad;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_SRC match error.");
}

void
test_match_basic_IPV6_SRC_W(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;
  int i;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  i = 22;
  m->data[i++] = 0x20;
  m->data[i++] = 0x01;
  m->data[i++] = 0x20;
  m->data[i++] = 0xff;
  m->data[i++] = 0x00;
  m->data[i++] = 0x00;
  m->data[i++] = 0x10;
  m->data[i++] = 0x20;
  m->data[i++] = 0x11;
  m->data[i++] = 0xe1;
  m->data[i++] = 0x50;
  m->data[i++] = 0x89;
  m->data[i++] = 0xbf;
  m->data[i++] = 0xc6;
  m->data[i++] = 0x43;
  m->data[i++] = 0x11;
  lagopus_packet_init(&pkt, m);
  add_match(&flow->match_list, 32, (OFPXMT_OFB_IPV6_SRC << 1) + 1,
            0x20, 0x01, 0x00, 0x00, 0xe0, 0x45, 0x22, 0xeb,
            0x09, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00);
  refresh_match(flow);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_SRC_W mismatch error.");
  i = 22;
  m->data[i++] = 0x20;
  m->data[i++] = 0x01;
  m->data[i++] = 0x00;
  m->data[i++] = 0x00;
  m->data[i++] = 0xe0;
  m->data[i++] = 0x45;
  m->data[i++] = 0x22;
  m->data[i++] = 0xeb;
  m->data[i++] = 0x09;
  m->data[i++] = 0x00;
  m->data[i++] = 0x00;
  m->data[i++] = 0x08;
  m->data[i++] = 0xdc;
  m->data[i++] = 0x18;
  m->data[i++] = 0x94;
  m->data[i++] = 0xad;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_SRC_W match error.");
}

void
test_match_basic_IPV6_DST(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;
  int i;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  i = 38;
  m->data[i++] = 0x20;
  m->data[i++] = 0x01;
  m->data[i++] = 0x20;
  m->data[i++] = 0xff;
  m->data[i++] = 0x00;
  m->data[i++] = 0x00;
  m->data[i++] = 0x10;
  m->data[i++] = 0x20;
  m->data[i++] = 0x11;
  m->data[i++] = 0xe1;
  m->data[i++] = 0x50;
  m->data[i++] = 0x89;
  m->data[i++] = 0xbf;
  m->data[i++] = 0xc6;
  m->data[i++] = 0x43;
  m->data[i++] = 0x11;
  lagopus_packet_init(&pkt, m);
  add_match(&flow->match_list, 16, OFPXMT_OFB_IPV6_DST << 1,
            0x20, 0x01, 0x00, 0x00, 0xe0, 0x45, 0x22, 0xeb,
            0x09, 0x00, 0x00, 0x08, 0xdc, 0x18, 0x94, 0xad);
  refresh_match(flow);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_DST mismatch error.");
  i = 38;
  m->data[i++] = 0x20;
  m->data[i++] = 0x01;
  m->data[i++] = 0x00;
  m->data[i++] = 0x00;
  m->data[i++] = 0xe0;
  m->data[i++] = 0x45;
  m->data[i++] = 0x22;
  m->data[i++] = 0xeb;
  m->data[i++] = 0x09;
  m->data[i++] = 0x00;
  m->data[i++] = 0x00;
  m->data[i++] = 0x08;
  m->data[i++] = 0xdc;
  m->data[i++] = 0x18;
  m->data[i++] = 0x94;
  m->data[i++] = 0xad;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_DST match error.");
}

void
test_match_basic_IPV6_DST_W(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;
  int i;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  i = 38;
  m->data[i++] = 0x20;
  m->data[i++] = 0x01;
  m->data[i++] = 0x20;
  m->data[i++] = 0xff;
  m->data[i++] = 0x00;
  m->data[i++] = 0x00;
  m->data[i++] = 0x10;
  m->data[i++] = 0x20;
  m->data[i++] = 0x11;
  m->data[i++] = 0xe1;
  m->data[i++] = 0x50;
  m->data[i++] = 0x89;
  m->data[i++] = 0xbf;
  m->data[i++] = 0xc6;
  m->data[i++] = 0x43;
  m->data[i++] = 0x11;
  lagopus_packet_init(&pkt, m);
  add_match(&flow->match_list, 32, (OFPXMT_OFB_IPV6_DST << 1) + 1,
            0x20, 0x01, 0x00, 0x00, 0xe0, 0x45, 0x22, 0xeb,
            0x09, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00);
  refresh_match(flow);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_DST_W mismatch error.");
  i = 38;
  m->data[i++] = 0x20;
  m->data[i++] = 0x01;
  m->data[i++] = 0x00;
  m->data[i++] = 0x00;
  m->data[i++] = 0xe0;
  m->data[i++] = 0x45;
  m->data[i++] = 0x22;
  m->data[i++] = 0xeb;
  m->data[i++] = 0x09;
  m->data[i++] = 0x00;
  m->data[i++] = 0x00;
  m->data[i++] = 0x08;
  m->data[i++] = 0xdc;
  m->data[i++] = 0x18;
  m->data[i++] = 0x94;
  m->data[i++] = 0xad;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_DST_W match error.");
}

void
test_match_basic_IPV6_FLABEL(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[15] = 0x00;
  m->data[16] = 0x12;
  m->data[17] = 0x34;
  add_match(&flow->match_list, 4, OFPXMT_OFB_IPV6_FLABEL << 1,
            0x00, 0x01, 0x23, 0x45);
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_FLABEL mismatch error.");
  m->data[15] = 0x01;
  m->data[16] = 0x23;
  m->data[17] = 0x45;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_FLABEL match error.");
}

void
test_match_basic_IPV6_TCP_SRC(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_TCP);
  m->data[20] = IPPROTO_TCP;
  /* TCP_SRC */
  add_match(&flow->match_list, 2, OFPXMT_OFB_TCP_SRC << 1,
            0xf0, 0x00);
  m->data[54] = 0;
  m->data[55] = 0xf0;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_TCP_SRC mismatch error.");
  m->data[54] = 0xf0;
  m->data[55] = 0x00;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_TCP_SRC match error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_TCP;
  m->data[55] = 0;
  m->data[62] = 0x00;
  m->data[63] = 0xf0;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_TCP_SRC mismatch(next hdr) error.");
  m->data[62] = 0xf0;
  m->data[63] = 0x00;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_TCP_SRC match(next hdr) error.");
  free(m);
}

void
test_match_basic_IPV6_TCP_DST(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_TCP);
  m->data[20] = IPPROTO_TCP;
  /* TCP_DST */
  add_match(&flow->match_list, 2, OFPXMT_OFB_TCP_DST << 1,
            0xf0, 0x00);
  m->data[56] = 0;
  m->data[57] = 0xf0;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_TCP_DST mismatch error.");
  m->data[56] = 0xf0;
  m->data[57] = 0x00;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_TCP_DST match error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_TCP;
  m->data[55] = 0;
  m->data[64] = 0x00;
  m->data[65] = 0xf0;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_TCP_DST mismatch(next hdr) error.");
  m->data[64] = 0xf0;
  m->data[65] = 0x00;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_TCP_DST match(next hdr) error.");
  free(m);
}

void
test_match_basic_IPV6_UDP_SRC(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_UDP);
  m->data[20] = IPPROTO_UDP;
  /* UDP_SRC */
  add_match(&flow->match_list, 2, OFPXMT_OFB_UDP_SRC << 1,
            0xf0, 0x00);
  m->data[54] = 0;
  m->data[55] = 0xf0;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_UDP_SRC mismatch error.");
  m->data[54] = 0xf0;
  m->data[55] = 0x00;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_UDP_SRC match error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_UDP;
  m->data[55] = 0;
  m->data[62] = 0x00;
  m->data[63] = 0xf0;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_UDP_SRC mismatch(next hdr) error.");
  m->data[62] = 0xf0;
  m->data[63] = 0x00;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_UDP_SRC match(next hdr) error.");
  free(m);
}

void
test_match_basic_IPV6_UDP_DST(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_UDP);
  m->data[20] = IPPROTO_UDP;
  /* UDP_DST */
  add_match(&flow->match_list, 2, OFPXMT_OFB_UDP_DST << 1,
            0xf0, 0x00);
  m->data[56] = 0;
  m->data[57] = 0xf0;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_UDP_DST mismatch error.");
  m->data[56] = 0xf0;
  m->data[57] = 0x00;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_UDP_DST match error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_UDP;
  m->data[55] = 0;
  m->data[64] = 0x00;
  m->data[65] = 0xf0;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_UDP_DST mismatch(next hdr) error.");
  m->data[64] = 0xf0;
  m->data[65] = 0x00;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_UDP_DST match(next hdr) error.");
  free(m);
}

void
test_match_basic_IPV6_SCTP_SRC(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_SCTP);
  m->data[20] = IPPROTO_SCTP;
  /* SCTP_SRC */
  add_match(&flow->match_list, 2, OFPXMT_OFB_SCTP_SRC << 1,
            0xf0, 0x00);
  m->data[54] = 0;
  m->data[55] = 0xf0;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_SCTP_SRC mismatch error.");
  m->data[54] = 0xf0;
  m->data[55] = 0x00;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_SCTP_SRC match error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_SCTP;
  m->data[55] = 0;
  m->data[62] = 0x00;
  m->data[63] = 0xf0;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_SCTP_SRC mismatch(next hdr) error.");
  m->data[62] = 0xf0;
  m->data[63] = 0x00;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_SCTP_SRC match(next hdr) error.");
  free(m);
}

void
test_match_basic_IPV6_SCTP_DST(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_SCTP);
  m->data[20] = IPPROTO_SCTP;
  /* SCTP_DST */
  add_match(&flow->match_list, 2, OFPXMT_OFB_SCTP_DST << 1,
            0xf0, 0x00);
  m->data[56] = 0;
  m->data[57] = 0xf0;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_SCTP_DST mismatch error.");
  m->data[56] = 0xf0;
  m->data[57] = 0x00;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_SCTP_DST match error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_SCTP;
  m->data[55] = 0;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  m->data[64] = 0x00;
  m->data[65] = 0xf0;
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_SCTP_DST mismatch(next hdr) error.");
  m->data[64] = 0xf0;
  m->data[65] = 0x00;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_SCTP_DST match(next hdr) error.");
  free(m);
}

void
test_match_basic_IPV6_ICMPV6_TYPE(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);

  /* IP proto */
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_ICMPV6);

  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_ICMPV6;

  add_match(&flow->match_list, 1, OFPXMT_OFB_ICMPV6_TYPE << 1,
            ICMP6_ECHO_REPLY);
  m->data[54] = ICMP6_ECHO_REQUEST;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ICMPV6_TYPE mismatch error.");
  m->data[54] = ICMP6_ECHO_REPLY;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ICMPV6_TYPE match error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_ICMPV6;
  m->data[55] = 0;
  m->data[62] = ICMP6_ECHO_REQUEST;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ICMPV6_TYPE mismatch(next hdr) error.");
  m->data[62] = ICMP6_ECHO_REPLY;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ICMPV6_TYPE match(next hdr) error.");
}

void
test_match_basic_IPV6_ICMPV6_CODE(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);

  /* IP proto */
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_ICMPV6);

  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_ICMPV6;

  add_match(&flow->match_list, 1, OFPXMT_OFB_ICMPV6_CODE << 1,
            ICMP6_DST_UNREACH_ADDR);
  m->data[55] = ICMP6_DST_UNREACH_ADMIN;
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ICMPV6_CODE mismatch error.");
  m->data[55] = ICMP6_DST_UNREACH_ADDR;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ICMPV6_CODE match error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_ICMPV6;
  m->data[55] = 0;
  m->data[63] = ICMP6_DST_UNREACH_ADMIN;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ICMPV6_CODE mismatch(next hdr) error.");
  m->data[63] = ICMP6_DST_UNREACH_ADDR;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ICMPV6_CODE match(next hdr) error.");
}

void
test_match_basic_IPV6_EXTHDR(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 256;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_TCP);
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_TCP;
  m->data[55] = 4;
  add_match(&flow->match_list, 2, OFPXMT_OFB_IPV6_EXTHDR << 1,
            OFPIEH_UNSEQ >> 8,
            OFPIEH_NONEXT|OFPIEH_ESP|OFPIEH_AUTH|OFPIEH_DEST|OFPIEH_FRAG|
            OFPIEH_ROUTER|OFPIEH_HOP|OFPIEH_UNREP);
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_EXTHDR mismatch error.");
  m->data[54] = IPPROTO_HOPOPTS;
  m->data[55] = 0;
  m->data[62] = IPPROTO_ESP;
  m->data[63] = 0;
  m->data[70] = IPPROTO_AH;
  m->data[72] = 0;
  m->data[78] = IPPROTO_ROUTING;
  m->data[79] = 0;
  m->data[86] = IPPROTO_FRAGMENT;
  m->data[94] = IPPROTO_AH;
  m->data[95] = 0;
  m->data[102] = IPPROTO_ROUTING;
  m->data[103] = 0;
  m->data[110] = IPPROTO_HOPOPTS;
  m->data[111] = 0;
  m->data[118] = IPPROTO_FRAGMENT;
  m->data[126] = IPPROTO_ESP;
  m->data[127] = 0;
  m->data[134] = IPPROTO_DSTOPTS;
  m->data[135] = 0;
  m->data[142] = IPPROTO_NONE;
  m->data[143] = 0;
  m->data[150] = IPPROTO_TCP;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_EXTHDR match error.");
}

void
test_match_basic_IPV6_EXTHDR_W(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt.in_port = &port;
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_TCP);
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_TCP;
  m->data[55] = 4;
  add_match(&flow->match_list, 4, (OFPXMT_OFB_IPV6_EXTHDR << 1) + 1,
            OFPIEH_UNSEQ >> 8, 0,
            OFPIEH_UNSEQ >> 8, OFPIEH_UNREP);
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_EXTHDR_W mismatch error.");
  m->data[54] = IPPROTO_HOPOPTS;
  m->data[55] = 0;
  m->data[62] = IPPROTO_ESP;
  m->data[63] = 0;
  m->data[70] = IPPROTO_AH;
  m->data[72] = 0;
  m->data[78] = IPPROTO_ROUTING;
  m->data[79] = 0;
  m->data[86] = IPPROTO_FRAGMENT;
  m->data[94] = IPPROTO_NONE;
  m->data[95] = 0;
  m->data[102] = IPPROTO_TCP;
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_EXTHDR_W match error.");
}
