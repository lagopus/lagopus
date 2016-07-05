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

#include <netinet/icmp6.h>

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
test_match_basic_IPV6_IP_DSCP(void) {
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
            0x86, 0xdd);
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_DSCP << 1,
            0x3f);
  OS_MTOD(m, uint8_t *)[14] = 0x00;
  OS_MTOD(m, uint8_t *)[15] = 0x00;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_IP_DSCP mismatch error.");
  OS_MTOD(m, uint8_t *)[14] = 0x0f;
  OS_MTOD(m, uint8_t *)[15] = 0xc0;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_IP_DSCP match error.");
}

void
test_match_basic_IPV6_IP_ECN(void) {
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
            0x86, 0xdd);
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_ECN << 1,
            0x03);
  OS_MTOD(m, uint8_t *)[15] = 0x00;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_IP_ECN mismatch error.");
  OS_MTOD(m, uint8_t *)[15] = 0x30;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_IP_ECN match error.");
}

void
test_match_basic_IPV6_PROTO(void) {
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
            0x86, 0xdd);

  /* IP proto */
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_TCP);
  refresh_match(flow);

  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;

  OS_MTOD(m, uint8_t *)[20] = IPPROTO_UDP;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_IP_PROTO mismatch error.");
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_TCP;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_IP_PROTO match error.");
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_UDP;
  OS_MTOD(m, uint8_t *)[55] = 0;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_IP_PROTO mismatch(next hdr) error.");
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_TCP;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_IP_PROTO match(next hdr) error.");
  free(m);
}

void
test_match_basic_IPV6_SRC(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;
  int i;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  i = 22;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0x01;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0xff;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x10;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0x11;
  OS_MTOD(m, uint8_t *)[i++] = 0xe1;
  OS_MTOD(m, uint8_t *)[i++] = 0x50;
  OS_MTOD(m, uint8_t *)[i++] = 0x89;
  OS_MTOD(m, uint8_t *)[i++] = 0xbf;
  OS_MTOD(m, uint8_t *)[i++] = 0xc6;
  OS_MTOD(m, uint8_t *)[i++] = 0x43;
  OS_MTOD(m, uint8_t *)[i++] = 0x11;
  lagopus_packet_init(pkt, m, &port);
  add_match(&flow->match_list, 16, OFPXMT_OFB_IPV6_SRC << 1,
            0x20, 0x01, 0x00, 0x00, 0xe0, 0x45, 0x22, 0xeb,
            0x09, 0x00, 0x00, 0x08, 0xdc, 0x18, 0x94, 0xad);
  refresh_match(flow);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_SRC mismatch error.");
  i = 22;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0x01;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0xe0;
  OS_MTOD(m, uint8_t *)[i++] = 0x45;
  OS_MTOD(m, uint8_t *)[i++] = 0x22;
  OS_MTOD(m, uint8_t *)[i++] = 0xeb;
  OS_MTOD(m, uint8_t *)[i++] = 0x09;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x08;
  OS_MTOD(m, uint8_t *)[i++] = 0xdc;
  OS_MTOD(m, uint8_t *)[i++] = 0x18;
  OS_MTOD(m, uint8_t *)[i++] = 0x94;
  OS_MTOD(m, uint8_t *)[i++] = 0xad;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_SRC match error.");
}

void
test_match_basic_IPV6_SRC_W(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;
  int i;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  i = 22;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0x01;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0xff;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x10;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0x11;
  OS_MTOD(m, uint8_t *)[i++] = 0xe1;
  OS_MTOD(m, uint8_t *)[i++] = 0x50;
  OS_MTOD(m, uint8_t *)[i++] = 0x89;
  OS_MTOD(m, uint8_t *)[i++] = 0xbf;
  OS_MTOD(m, uint8_t *)[i++] = 0xc6;
  OS_MTOD(m, uint8_t *)[i++] = 0x43;
  OS_MTOD(m, uint8_t *)[i++] = 0x11;
  lagopus_packet_init(pkt, m, &port);
  add_match(&flow->match_list, 32, (OFPXMT_OFB_IPV6_SRC << 1) + 1,
            0x20, 0x01, 0x00, 0x00, 0xe0, 0x45, 0x22, 0xeb,
            0x09, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00);
  refresh_match(flow);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_SRC_W mismatch error.");
  i = 22;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0x01;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0xe0;
  OS_MTOD(m, uint8_t *)[i++] = 0x45;
  OS_MTOD(m, uint8_t *)[i++] = 0x22;
  OS_MTOD(m, uint8_t *)[i++] = 0xeb;
  OS_MTOD(m, uint8_t *)[i++] = 0x09;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x08;
  OS_MTOD(m, uint8_t *)[i++] = 0xdc;
  OS_MTOD(m, uint8_t *)[i++] = 0x18;
  OS_MTOD(m, uint8_t *)[i++] = 0x94;
  OS_MTOD(m, uint8_t *)[i++] = 0xad;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_SRC_W match error.");
}

void
test_match_basic_IPV6_DST(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;
  int i;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  i = 38;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0x01;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0xff;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x10;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0x11;
  OS_MTOD(m, uint8_t *)[i++] = 0xe1;
  OS_MTOD(m, uint8_t *)[i++] = 0x50;
  OS_MTOD(m, uint8_t *)[i++] = 0x89;
  OS_MTOD(m, uint8_t *)[i++] = 0xbf;
  OS_MTOD(m, uint8_t *)[i++] = 0xc6;
  OS_MTOD(m, uint8_t *)[i++] = 0x43;
  OS_MTOD(m, uint8_t *)[i++] = 0x11;
  lagopus_packet_init(pkt, m, &port);
  add_match(&flow->match_list, 16, OFPXMT_OFB_IPV6_DST << 1,
            0x20, 0x01, 0x00, 0x00, 0xe0, 0x45, 0x22, 0xeb,
            0x09, 0x00, 0x00, 0x08, 0xdc, 0x18, 0x94, 0xad);
  refresh_match(flow);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_DST mismatch error.");
  i = 38;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0x01;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0xe0;
  OS_MTOD(m, uint8_t *)[i++] = 0x45;
  OS_MTOD(m, uint8_t *)[i++] = 0x22;
  OS_MTOD(m, uint8_t *)[i++] = 0xeb;
  OS_MTOD(m, uint8_t *)[i++] = 0x09;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x08;
  OS_MTOD(m, uint8_t *)[i++] = 0xdc;
  OS_MTOD(m, uint8_t *)[i++] = 0x18;
  OS_MTOD(m, uint8_t *)[i++] = 0x94;
  OS_MTOD(m, uint8_t *)[i++] = 0xad;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_DST match error.");
}

void
test_match_basic_IPV6_DST_W(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;
  int i;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  i = 38;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0x01;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0xff;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x10;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0x11;
  OS_MTOD(m, uint8_t *)[i++] = 0xe1;
  OS_MTOD(m, uint8_t *)[i++] = 0x50;
  OS_MTOD(m, uint8_t *)[i++] = 0x89;
  OS_MTOD(m, uint8_t *)[i++] = 0xbf;
  OS_MTOD(m, uint8_t *)[i++] = 0xc6;
  OS_MTOD(m, uint8_t *)[i++] = 0x43;
  OS_MTOD(m, uint8_t *)[i++] = 0x11;
  lagopus_packet_init(pkt, m, &port);
  add_match(&flow->match_list, 32, (OFPXMT_OFB_IPV6_DST << 1) + 1,
            0x20, 0x01, 0x00, 0x00, 0xe0, 0x45, 0x22, 0xeb,
            0x09, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00);
  refresh_match(flow);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_DST_W mismatch error.");
  i = 38;
  OS_MTOD(m, uint8_t *)[i++] = 0x20;
  OS_MTOD(m, uint8_t *)[i++] = 0x01;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0xe0;
  OS_MTOD(m, uint8_t *)[i++] = 0x45;
  OS_MTOD(m, uint8_t *)[i++] = 0x22;
  OS_MTOD(m, uint8_t *)[i++] = 0xeb;
  OS_MTOD(m, uint8_t *)[i++] = 0x09;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x00;
  OS_MTOD(m, uint8_t *)[i++] = 0x08;
  OS_MTOD(m, uint8_t *)[i++] = 0xdc;
  OS_MTOD(m, uint8_t *)[i++] = 0x18;
  OS_MTOD(m, uint8_t *)[i++] = 0x94;
  OS_MTOD(m, uint8_t *)[i++] = 0xad;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_DST_W match error.");
}

void
test_match_basic_IPV6_FLABEL(void) {
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
            0x86, 0xdd);
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  OS_MTOD(m, uint8_t *)[15] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x12;
  OS_MTOD(m, uint8_t *)[17] = 0x34;
  add_match(&flow->match_list, 4, OFPXMT_OFB_IPV6_FLABEL << 1,
            0x00, 0x01, 0x23, 0x45);
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_FLABEL mismatch error.");
  OS_MTOD(m, uint8_t *)[15] = 0x01;
  OS_MTOD(m, uint8_t *)[16] = 0x23;
  OS_MTOD(m, uint8_t *)[17] = 0x45;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_FLABEL match error.");
}

void
test_match_basic_IPV6_TCP_SRC(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_TCP);
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_TCP;
  /* TCP_SRC */
  add_match(&flow->match_list, 2, OFPXMT_OFB_TCP_SRC << 1,
            0xf0, 0x00);
  OS_MTOD(m, uint8_t *)[54] = 0;
  OS_MTOD(m, uint8_t *)[55] = 0xf0;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_TCP_SRC mismatch error.");
  OS_MTOD(m, uint8_t *)[54] = 0xf0;
  OS_MTOD(m, uint8_t *)[55] = 0x00;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_TCP_SRC match error.");
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_TCP;
  OS_MTOD(m, uint8_t *)[55] = 0;
  OS_MTOD(m, uint8_t *)[62] = 0x00;
  OS_MTOD(m, uint8_t *)[63] = 0xf0;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_TCP_SRC mismatch(next hdr) error.");
  OS_MTOD(m, uint8_t *)[62] = 0xf0;
  OS_MTOD(m, uint8_t *)[63] = 0x00;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_TCP_SRC match(next hdr) error.");
  free(m);
}

void
test_match_basic_IPV6_TCP_DST(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_TCP);
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_TCP;
  /* TCP_DST */
  add_match(&flow->match_list, 2, OFPXMT_OFB_TCP_DST << 1,
            0xf0, 0x00);
  OS_MTOD(m, uint8_t *)[56] = 0;
  OS_MTOD(m, uint8_t *)[57] = 0xf0;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_TCP_DST mismatch error.");
  OS_MTOD(m, uint8_t *)[56] = 0xf0;
  OS_MTOD(m, uint8_t *)[57] = 0x00;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_TCP_DST match error.");
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_TCP;
  OS_MTOD(m, uint8_t *)[55] = 0;
  OS_MTOD(m, uint8_t *)[64] = 0x00;
  OS_MTOD(m, uint8_t *)[65] = 0xf0;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_TCP_DST mismatch(next hdr) error.");
  OS_MTOD(m, uint8_t *)[64] = 0xf0;
  OS_MTOD(m, uint8_t *)[65] = 0x00;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_TCP_DST match(next hdr) error.");
  free(m);
}

void
test_match_basic_IPV6_UDP_SRC(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_UDP);
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_UDP;
  /* UDP_SRC */
  add_match(&flow->match_list, 2, OFPXMT_OFB_UDP_SRC << 1,
            0xf0, 0x00);
  OS_MTOD(m, uint8_t *)[54] = 0;
  OS_MTOD(m, uint8_t *)[55] = 0xf0;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_UDP_SRC mismatch error.");
  OS_MTOD(m, uint8_t *)[54] = 0xf0;
  OS_MTOD(m, uint8_t *)[55] = 0x00;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_UDP_SRC match error.");
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_UDP;
  OS_MTOD(m, uint8_t *)[55] = 0;
  OS_MTOD(m, uint8_t *)[62] = 0x00;
  OS_MTOD(m, uint8_t *)[63] = 0xf0;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_UDP_SRC mismatch(next hdr) error.");
  OS_MTOD(m, uint8_t *)[62] = 0xf0;
  OS_MTOD(m, uint8_t *)[63] = 0x00;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_UDP_SRC match(next hdr) error.");
  free(m);
}

void
test_match_basic_IPV6_UDP_DST(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_UDP);
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_UDP;
  /* UDP_DST */
  add_match(&flow->match_list, 2, OFPXMT_OFB_UDP_DST << 1,
            0xf0, 0x00);
  OS_MTOD(m, uint8_t *)[56] = 0;
  OS_MTOD(m, uint8_t *)[57] = 0xf0;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_UDP_DST mismatch error.");
  OS_MTOD(m, uint8_t *)[56] = 0xf0;
  OS_MTOD(m, uint8_t *)[57] = 0x00;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_UDP_DST match error.");
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_UDP;
  OS_MTOD(m, uint8_t *)[55] = 0;
  OS_MTOD(m, uint8_t *)[64] = 0x00;
  OS_MTOD(m, uint8_t *)[65] = 0xf0;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_UDP_DST mismatch(next hdr) error.");
  OS_MTOD(m, uint8_t *)[64] = 0xf0;
  OS_MTOD(m, uint8_t *)[65] = 0x00;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_UDP_DST match(next hdr) error.");
  free(m);
}

void
test_match_basic_IPV6_SCTP_SRC(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_SCTP);
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_SCTP;
  /* SCTP_SRC */
  add_match(&flow->match_list, 2, OFPXMT_OFB_SCTP_SRC << 1,
            0xf0, 0x00);
  OS_MTOD(m, uint8_t *)[54] = 0;
  OS_MTOD(m, uint8_t *)[55] = 0xf0;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_SCTP_SRC mismatch error.");
  OS_MTOD(m, uint8_t *)[54] = 0xf0;
  OS_MTOD(m, uint8_t *)[55] = 0x00;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_SCTP_SRC match error.");
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_SCTP;
  OS_MTOD(m, uint8_t *)[55] = 0;
  OS_MTOD(m, uint8_t *)[62] = 0x00;
  OS_MTOD(m, uint8_t *)[63] = 0xf0;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_SCTP_SRC mismatch(next hdr) error.");
  OS_MTOD(m, uint8_t *)[62] = 0xf0;
  OS_MTOD(m, uint8_t *)[63] = 0x00;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_SCTP_SRC match(next hdr) error.");
  free(m);
}

void
test_match_basic_IPV6_SCTP_DST(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_SCTP);
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_SCTP;
  /* SCTP_DST */
  add_match(&flow->match_list, 2, OFPXMT_OFB_SCTP_DST << 1,
            0xf0, 0x00);
  OS_MTOD(m, uint8_t *)[56] = 0;
  OS_MTOD(m, uint8_t *)[57] = 0xf0;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_SCTP_DST mismatch error.");
  OS_MTOD(m, uint8_t *)[56] = 0xf0;
  OS_MTOD(m, uint8_t *)[57] = 0x00;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_SCTP_DST match error.");
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_SCTP;
  OS_MTOD(m, uint8_t *)[55] = 0;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  OS_MTOD(m, uint8_t *)[64] = 0x00;
  OS_MTOD(m, uint8_t *)[65] = 0xf0;
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_SCTP_DST mismatch(next hdr) error.");
  OS_MTOD(m, uint8_t *)[64] = 0xf0;
  OS_MTOD(m, uint8_t *)[65] = 0x00;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_SCTP_DST match(next hdr) error.");
  free(m);
}

void
test_match_basic_IPV6_ICMPV6_TYPE(void) {
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
            0x86, 0xdd);

  /* IP proto */
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_ICMPV6);

  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_ICMPV6;

  add_match(&flow->match_list, 1, OFPXMT_OFB_ICMPV6_TYPE << 1,
            ICMP6_ECHO_REPLY);
  OS_MTOD(m, uint8_t *)[54] = ICMP6_ECHO_REQUEST;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ICMPV6_TYPE mismatch error.");
  OS_MTOD(m, uint8_t *)[54] = ICMP6_ECHO_REPLY;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ICMPV6_TYPE match error.");
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_ICMPV6;
  OS_MTOD(m, uint8_t *)[55] = 0;
  OS_MTOD(m, uint8_t *)[62] = ICMP6_ECHO_REQUEST;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ICMPV6_TYPE mismatch(next hdr) error.");
  OS_MTOD(m, uint8_t *)[62] = ICMP6_ECHO_REPLY;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ICMPV6_TYPE match(next hdr) error.");
}

void
test_match_basic_IPV6_ICMPV6_CODE(void) {
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
            0x86, 0xdd);

  /* IP proto */
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_ICMPV6);

  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_ICMPV6;

  add_match(&flow->match_list, 1, OFPXMT_OFB_ICMPV6_CODE << 1,
            ICMP6_DST_UNREACH_ADDR);
  OS_MTOD(m, uint8_t *)[55] = ICMP6_DST_UNREACH_ADMIN;
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ICMPV6_CODE mismatch error.");
  OS_MTOD(m, uint8_t *)[55] = ICMP6_DST_UNREACH_ADDR;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ICMPV6_CODE match error.");
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_ICMPV6;
  OS_MTOD(m, uint8_t *)[55] = 0;
  OS_MTOD(m, uint8_t *)[63] = ICMP6_DST_UNREACH_ADMIN;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ICMPV6_CODE mismatch(next hdr) error.");
  OS_MTOD(m, uint8_t *)[63] = ICMP6_DST_UNREACH_ADDR;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ICMPV6_CODE match(next hdr) error.");
}

void
test_match_basic_IPV6_EXTHDR(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 256;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_TCP);
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_TCP;
  OS_MTOD(m, uint8_t *)[55] = 4;
  add_match(&flow->match_list, 2, OFPXMT_OFB_IPV6_EXTHDR << 1,
            OFPIEH_UNSEQ >> 8,
            OFPIEH_NONEXT|OFPIEH_ESP|OFPIEH_AUTH|OFPIEH_DEST|OFPIEH_FRAG|
            OFPIEH_ROUTER|OFPIEH_HOP|OFPIEH_UNREP);
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_EXTHDR mismatch error.");
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_HOPOPTS;
  OS_MTOD(m, uint8_t *)[55] = 0;
  OS_MTOD(m, uint8_t *)[62] = IPPROTO_ESP;
  OS_MTOD(m, uint8_t *)[63] = 0;
  OS_MTOD(m, uint8_t *)[70] = IPPROTO_AH;
  OS_MTOD(m, uint8_t *)[72] = 0;
  OS_MTOD(m, uint8_t *)[78] = IPPROTO_ROUTING;
  OS_MTOD(m, uint8_t *)[79] = 0;
  OS_MTOD(m, uint8_t *)[86] = IPPROTO_FRAGMENT;
  OS_MTOD(m, uint8_t *)[94] = IPPROTO_AH;
  OS_MTOD(m, uint8_t *)[95] = 0;
  OS_MTOD(m, uint8_t *)[102] = IPPROTO_ROUTING;
  OS_MTOD(m, uint8_t *)[103] = 0;
  OS_MTOD(m, uint8_t *)[110] = IPPROTO_HOPOPTS;
  OS_MTOD(m, uint8_t *)[111] = 0;
  OS_MTOD(m, uint8_t *)[118] = IPPROTO_FRAGMENT;
  OS_MTOD(m, uint8_t *)[126] = IPPROTO_ESP;
  OS_MTOD(m, uint8_t *)[127] = 0;
  OS_MTOD(m, uint8_t *)[134] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[135] = 0;
  OS_MTOD(m, uint8_t *)[142] = IPPROTO_NONE;
  OS_MTOD(m, uint8_t *)[143] = 0;
  OS_MTOD(m, uint8_t *)[150] = IPPROTO_TCP;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_EXTHDR match error.");
}

void
test_match_basic_IPV6_EXTHDR_W(void) {
  struct lagopus_packet *pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

  /* prepare flow */
  flow = calloc(1, sizeof(struct flow) + 10 * sizeof(struct match));
  TAILQ_INIT(&flow->match_list);

  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x86, 0xdd);
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  add_match(&flow->match_list, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_TCP);
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_TCP;
  OS_MTOD(m, uint8_t *)[55] = 4;
  add_match(&flow->match_list, 4, (OFPXMT_OFB_IPV6_EXTHDR << 1) + 1,
            OFPIEH_UNSEQ >> 8, 0,
            OFPIEH_UNSEQ >> 8, OFPIEH_UNREP);
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_EXTHDR_W mismatch error.");
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_HOPOPTS;
  OS_MTOD(m, uint8_t *)[55] = 0;
  OS_MTOD(m, uint8_t *)[62] = IPPROTO_ESP;
  OS_MTOD(m, uint8_t *)[63] = 0;
  OS_MTOD(m, uint8_t *)[70] = IPPROTO_AH;
  OS_MTOD(m, uint8_t *)[72] = 0;
  OS_MTOD(m, uint8_t *)[78] = IPPROTO_ROUTING;
  OS_MTOD(m, uint8_t *)[79] = 0;
  OS_MTOD(m, uint8_t *)[86] = IPPROTO_FRAGMENT;
  OS_MTOD(m, uint8_t *)[94] = IPPROTO_NONE;
  OS_MTOD(m, uint8_t *)[95] = 0;
  OS_MTOD(m, uint8_t *)[102] = IPPROTO_TCP;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_EXTHDR_W match error.");
}
