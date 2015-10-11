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


void
setUp(void) {
}

void
tearDown(void) {
}


void
test_match_basic_ARP_OP(void) {
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
            0x08, 0x06);
  m->data[12] = 0x08;
  m->data[13] = 0x06;
  m->data[20] = 0x00;
  m->data[21] = ARPOP_REQUEST;
  lagopus_packet_init(&pkt, m, &port);
  add_match(&flow->match_list, 2, OFPXMT_OFB_ARP_OP << 1,
            0x00, ARPOP_REPLY);
  refresh_match(flow);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_OP mismatch error.");
  m->data[21] = ARPOP_REPLY;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_OP match error.");
}

void
test_match_basic_ARP_SPA(void) {
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
            0x08, 0x06);
  m->data[12] = 0x08;
  m->data[13] = 0x06;
  m->data[20] = 0x00;
  m->data[21] = ARPOP_REQUEST;
  m->data[28] = 172;
  m->data[29] = 21;
  m->data[30] = 0;
  m->data[31] = 1;
  lagopus_packet_init(&pkt, m, &port);
  add_match(&flow->match_list, 4, OFPXMT_OFB_ARP_SPA << 1,
            192, 168, 1, 2);
  refresh_match(flow);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_SPA mismatch error.");
  m->data[28] = 192;
  m->data[29] = 168;
  m->data[30] = 1;
  m->data[31] = 2;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_SPA match error.");
}

void
test_match_basic_ARP_SPA_W(void) {
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
            0x08, 0x06);
  m->data[12] = 0x08;
  m->data[13] = 0x06;
  m->data[20] = 0x00;
  m->data[21] = ARPOP_REQUEST;
  m->data[28] = 172;
  m->data[29] = 21;
  m->data[30] = 0;
  m->data[31] = 1;

  lagopus_packet_init(&pkt, m, &port);
  add_match(&flow->match_list, 8, (OFPXMT_OFB_ARP_SPA << 1) + 1,
            192, 168, 1, 0, 255, 255, 255, 0);
  refresh_match(flow);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_SPA_W mismatch error.");
  m->data[28] = 2;
  m->data[29] = 1;
  m->data[30] = 168;
  m->data[31] = 192;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_SPA_W mismatch(2) error.");
  m->data[28] = 192;
  m->data[29] = 168;
  m->data[30] = 1;
  m->data[31] = 2;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_SPA_W match error.");
}

void
test_match_basic_ARP_TPA(void) {
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
            0x08, 0x06);
  m->data[12] = 0x08;
  m->data[13] = 0x06;
  m->data[20] = 0x00;
  m->data[21] = ARPOP_REQUEST;
  m->data[38] = 172;
  m->data[39] = 21;
  m->data[40] = 0;
  m->data[41] = 1;
  lagopus_packet_init(&pkt, m, &port);
  add_match(&flow->match_list, 4, OFPXMT_OFB_ARP_TPA << 1,
            192, 168, 1, 2);
  refresh_match(flow);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_TPA mismatch error.");
  m->data[38] = 192;
  m->data[39] = 168;
  m->data[40] = 1;
  m->data[41] = 2;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_TPA match error.");
}

void
test_match_basic_ARP_TPA_W(void) {
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
            0x08, 0x06);
  m->data[12] = 0x08;
  m->data[13] = 0x06;
  m->data[20] = 0x00;
  m->data[21] = ARPOP_REQUEST;
  m->data[38] = 172;
  m->data[39] = 21;
  m->data[40] = 0;
  m->data[41] = 1;

  lagopus_packet_init(&pkt, m, &port);
  add_match(&flow->match_list, 8, (OFPXMT_OFB_ARP_TPA << 1) + 1,
            192, 168, 1, 0, 255, 255, 255, 0);
  refresh_match(flow);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_TPA_W mismatch error.");
  m->data[38] = 2;
  m->data[39] = 1;
  m->data[40] = 168;
  m->data[41] = 192;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_TPA_W mismatch(2) error.");
  m->data[38] = 192;
  m->data[39] = 168;
  m->data[40] = 1;
  m->data[41] = 2;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_TPA_W match error.");
}

void
test_match_basic_ARP_SHA(void) {
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
            0x08, 0x06);
  m->data[12] = 0x08;
  m->data[13] = 0x06;
  m->data[20] = 0x00;
  m->data[21] = ARPOP_REQUEST;
  m->data[22] = 0xaa;
  m->data[23] = 0x55;
  m->data[24] = 0xaa;
  m->data[25] = 0x55;
  m->data[26] = 0xaa;
  m->data[27] = 0x55;

  lagopus_packet_init(&pkt, m, &port);
  add_match(&flow->match_list, 6, OFPXMT_OFB_ARP_SHA << 1,
            0xe0, 0x4d, 0x01, 0x34, 0x56, 0x78);
  refresh_match(flow);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_SHA mismatch error.");
  m->data[22] = 0xe0;
  m->data[23] = 0x4d;
  m->data[24] = 0x01;
  m->data[25] = 0x34;
  m->data[26] = 0x56;
  m->data[27] = 0x78;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_SHA match error.");
}

void
test_match_basic_ARP_SHA_W(void) {
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
            0x08, 0x06);
  m->data[12] = 0x08;
  m->data[13] = 0x06;
  m->data[20] = 0x00;
  m->data[21] = ARPOP_REQUEST;
  m->data[22] = 0xaa;
  m->data[23] = 0x55;
  m->data[24] = 0xaa;
  m->data[25] = 0x55;
  m->data[26] = 0xaa;
  m->data[27] = 0x55;

  lagopus_packet_init(&pkt, m, &port);
  add_match(&flow->match_list, 12, (OFPXMT_OFB_ARP_SHA << 1) + 1,
            0xe0, 0x4d, 0x01, 0x00, 0x00, 0x00,
            0xff, 0xff, 0xff, 0x00, 0x00, 0x00);
  refresh_match(flow);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_SHA_W mismatch error.");
  m->data[22] = 0x78;
  m->data[23] = 0x56;
  m->data[24] = 0x34;
  m->data[25] = 0x01;
  m->data[26] = 0x4d;
  m->data[27] = 0xe0;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_SHA_W mismatch(2) error.");
  m->data[22] = 0xe0;
  m->data[23] = 0x4d;
  m->data[24] = 0x01;
  m->data[25] = 0x34;
  m->data[26] = 0x56;
  m->data[27] = 0x78;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_SHA_W match error.");
}

void
test_match_basic_ARP_THA(void) {
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
            0x08, 0x06);
  m->data[12] = 0x08;
  m->data[13] = 0x06;
  m->data[20] = 0x00;
  m->data[21] = ARPOP_REQUEST;
  m->data[32] = 0xaa;
  m->data[33] = 0x55;
  m->data[34] = 0xaa;
  m->data[35] = 0x55;
  m->data[36] = 0xaa;
  m->data[37] = 0x55;

  lagopus_packet_init(&pkt, m, &port);
  add_match(&flow->match_list, 6, OFPXMT_OFB_ARP_THA << 1,
            0xe0, 0x4d, 0x01, 0x34, 0x56, 0x78);
  refresh_match(flow);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_THA mismatch error.");
  m->data[32] = 0xe0;
  m->data[33] = 0x4d;
  m->data[34] = 0x01;
  m->data[35] = 0x34;
  m->data[36] = 0x56;
  m->data[37] = 0x78;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_THA match error.");
}

void
test_match_basic_ARP_THA_W(void) {
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
            0x08, 0x06);
  m->data[12] = 0x08;
  m->data[13] = 0x06;
  m->data[20] = 0x00;
  m->data[21] = ARPOP_REQUEST;
  m->data[32] = 0xaa;
  m->data[33] = 0x55;
  m->data[34] = 0xaa;
  m->data[35] = 0x55;
  m->data[36] = 0xaa;
  m->data[37] = 0x55;

  lagopus_packet_init(&pkt, m, &port);
  add_match(&flow->match_list, 12, (OFPXMT_OFB_ARP_THA << 1) + 1,
            0xe0, 0x4d, 0x01, 0x00, 0x00, 0x00,
            0xff, 0xff, 0xff, 0x00, 0x00, 0x00);
  refresh_match(flow);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_THA_W mismatch error.");
  m->data[32] = 0x78;
  m->data[33] = 0x56;
  m->data[34] = 0x34;
  m->data[35] = 0x01;
  m->data[36] = 0x4d;
  m->data[37] = 0xe0;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_THA_W mismatch(2) error.");
  m->data[32] = 0xe0;
  m->data[33] = 0x4d;
  m->data[34] = 0x01;
  m->data[35] = 0x34;
  m->data[36] = 0x56;
  m->data[37] = 0x78;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_THA_W match error.");
}
