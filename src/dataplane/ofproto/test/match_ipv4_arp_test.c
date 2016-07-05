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


void
setUp(void) {
}

void
tearDown(void) {
}


void
test_match_basic_ARP_OP(void) {
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
            0x08, 0x06);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x06;
  OS_MTOD(m, uint8_t *)[20] = 0x00;
  OS_MTOD(m, uint8_t *)[21] = ARPOP_REQUEST;
  lagopus_packet_init(pkt, m, &port);
  add_match(&flow->match_list, 2, OFPXMT_OFB_ARP_OP << 1,
            0x00, ARPOP_REPLY);
  refresh_match(flow);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_OP mismatch error.");
  OS_MTOD(m, uint8_t *)[21] = ARPOP_REPLY;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_OP match error.");
}

void
test_match_basic_ARP_SPA(void) {
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
            0x08, 0x06);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x06;
  OS_MTOD(m, uint8_t *)[20] = 0x00;
  OS_MTOD(m, uint8_t *)[21] = ARPOP_REQUEST;
  OS_MTOD(m, uint8_t *)[28] = 172;
  OS_MTOD(m, uint8_t *)[29] = 21;
  OS_MTOD(m, uint8_t *)[30] = 0;
  OS_MTOD(m, uint8_t *)[31] = 1;
  lagopus_packet_init(pkt, m, &port);
  add_match(&flow->match_list, 4, OFPXMT_OFB_ARP_SPA << 1,
            192, 168, 1, 2);
  refresh_match(flow);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_SPA mismatch error.");
  OS_MTOD(m, uint8_t *)[28] = 192;
  OS_MTOD(m, uint8_t *)[29] = 168;
  OS_MTOD(m, uint8_t *)[30] = 1;
  OS_MTOD(m, uint8_t *)[31] = 2;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_SPA match error.");
}

void
test_match_basic_ARP_SPA_W(void) {
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
            0x08, 0x06);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x06;
  OS_MTOD(m, uint8_t *)[20] = 0x00;
  OS_MTOD(m, uint8_t *)[21] = ARPOP_REQUEST;
  OS_MTOD(m, uint8_t *)[28] = 172;
  OS_MTOD(m, uint8_t *)[29] = 21;
  OS_MTOD(m, uint8_t *)[30] = 0;
  OS_MTOD(m, uint8_t *)[31] = 1;

  lagopus_packet_init(pkt, m, &port);
  add_match(&flow->match_list, 8, (OFPXMT_OFB_ARP_SPA << 1) + 1,
            192, 168, 1, 0, 255, 255, 255, 0);
  refresh_match(flow);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_SPA_W mismatch error.");
  OS_MTOD(m, uint8_t *)[28] = 2;
  OS_MTOD(m, uint8_t *)[29] = 1;
  OS_MTOD(m, uint8_t *)[30] = 168;
  OS_MTOD(m, uint8_t *)[31] = 192;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_SPA_W mismatch(2) error.");
  OS_MTOD(m, uint8_t *)[28] = 192;
  OS_MTOD(m, uint8_t *)[29] = 168;
  OS_MTOD(m, uint8_t *)[30] = 1;
  OS_MTOD(m, uint8_t *)[31] = 2;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_SPA_W match error.");
}

void
test_match_basic_ARP_TPA(void) {
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
            0x08, 0x06);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x06;
  OS_MTOD(m, uint8_t *)[20] = 0x00;
  OS_MTOD(m, uint8_t *)[21] = ARPOP_REQUEST;
  OS_MTOD(m, uint8_t *)[38] = 172;
  OS_MTOD(m, uint8_t *)[39] = 21;
  OS_MTOD(m, uint8_t *)[40] = 0;
  OS_MTOD(m, uint8_t *)[41] = 1;
  lagopus_packet_init(pkt, m, &port);
  add_match(&flow->match_list, 4, OFPXMT_OFB_ARP_TPA << 1,
            192, 168, 1, 2);
  refresh_match(flow);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_TPA mismatch error.");
  OS_MTOD(m, uint8_t *)[38] = 192;
  OS_MTOD(m, uint8_t *)[39] = 168;
  OS_MTOD(m, uint8_t *)[40] = 1;
  OS_MTOD(m, uint8_t *)[41] = 2;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_TPA match error.");
}

void
test_match_basic_ARP_TPA_W(void) {
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
            0x08, 0x06);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x06;
  OS_MTOD(m, uint8_t *)[20] = 0x00;
  OS_MTOD(m, uint8_t *)[21] = ARPOP_REQUEST;
  OS_MTOD(m, uint8_t *)[38] = 172;
  OS_MTOD(m, uint8_t *)[39] = 21;
  OS_MTOD(m, uint8_t *)[40] = 0;
  OS_MTOD(m, uint8_t *)[41] = 1;

  lagopus_packet_init(pkt, m, &port);
  add_match(&flow->match_list, 8, (OFPXMT_OFB_ARP_TPA << 1) + 1,
            192, 168, 1, 0, 255, 255, 255, 0);
  refresh_match(flow);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_TPA_W mismatch error.");
  OS_MTOD(m, uint8_t *)[38] = 2;
  OS_MTOD(m, uint8_t *)[39] = 1;
  OS_MTOD(m, uint8_t *)[40] = 168;
  OS_MTOD(m, uint8_t *)[41] = 192;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_TPA_W mismatch(2) error.");
  OS_MTOD(m, uint8_t *)[38] = 192;
  OS_MTOD(m, uint8_t *)[39] = 168;
  OS_MTOD(m, uint8_t *)[40] = 1;
  OS_MTOD(m, uint8_t *)[41] = 2;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_TPA_W match error.");
}

void
test_match_basic_ARP_SHA(void) {
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
            0x08, 0x06);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x06;
  OS_MTOD(m, uint8_t *)[20] = 0x00;
  OS_MTOD(m, uint8_t *)[21] = ARPOP_REQUEST;
  OS_MTOD(m, uint8_t *)[22] = 0xaa;
  OS_MTOD(m, uint8_t *)[23] = 0x55;
  OS_MTOD(m, uint8_t *)[24] = 0xaa;
  OS_MTOD(m, uint8_t *)[25] = 0x55;
  OS_MTOD(m, uint8_t *)[26] = 0xaa;
  OS_MTOD(m, uint8_t *)[27] = 0x55;

  lagopus_packet_init(pkt, m, &port);
  add_match(&flow->match_list, 6, OFPXMT_OFB_ARP_SHA << 1,
            0xe0, 0x4d, 0x01, 0x34, 0x56, 0x78);
  refresh_match(flow);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_SHA mismatch error.");
  OS_MTOD(m, uint8_t *)[22] = 0xe0;
  OS_MTOD(m, uint8_t *)[23] = 0x4d;
  OS_MTOD(m, uint8_t *)[24] = 0x01;
  OS_MTOD(m, uint8_t *)[25] = 0x34;
  OS_MTOD(m, uint8_t *)[26] = 0x56;
  OS_MTOD(m, uint8_t *)[27] = 0x78;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_SHA match error.");
}

void
test_match_basic_ARP_SHA_W(void) {
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
            0x08, 0x06);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x06;
  OS_MTOD(m, uint8_t *)[20] = 0x00;
  OS_MTOD(m, uint8_t *)[21] = ARPOP_REQUEST;
  OS_MTOD(m, uint8_t *)[22] = 0xaa;
  OS_MTOD(m, uint8_t *)[23] = 0x55;
  OS_MTOD(m, uint8_t *)[24] = 0xaa;
  OS_MTOD(m, uint8_t *)[25] = 0x55;
  OS_MTOD(m, uint8_t *)[26] = 0xaa;
  OS_MTOD(m, uint8_t *)[27] = 0x55;

  lagopus_packet_init(pkt, m, &port);
  add_match(&flow->match_list, 12, (OFPXMT_OFB_ARP_SHA << 1) + 1,
            0xe0, 0x4d, 0x01, 0x00, 0x00, 0x00,
            0xff, 0xff, 0xff, 0x00, 0x00, 0x00);
  refresh_match(flow);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_SHA_W mismatch error.");
  OS_MTOD(m, uint8_t *)[22] = 0x78;
  OS_MTOD(m, uint8_t *)[23] = 0x56;
  OS_MTOD(m, uint8_t *)[24] = 0x34;
  OS_MTOD(m, uint8_t *)[25] = 0x01;
  OS_MTOD(m, uint8_t *)[26] = 0x4d;
  OS_MTOD(m, uint8_t *)[27] = 0xe0;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_SHA_W mismatch(2) error.");
  OS_MTOD(m, uint8_t *)[22] = 0xe0;
  OS_MTOD(m, uint8_t *)[23] = 0x4d;
  OS_MTOD(m, uint8_t *)[24] = 0x01;
  OS_MTOD(m, uint8_t *)[25] = 0x34;
  OS_MTOD(m, uint8_t *)[26] = 0x56;
  OS_MTOD(m, uint8_t *)[27] = 0x78;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_SHA_W match error.");
}

void
test_match_basic_ARP_THA(void) {
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
            0x08, 0x06);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x06;
  OS_MTOD(m, uint8_t *)[20] = 0x00;
  OS_MTOD(m, uint8_t *)[21] = ARPOP_REQUEST;
  OS_MTOD(m, uint8_t *)[32] = 0xaa;
  OS_MTOD(m, uint8_t *)[33] = 0x55;
  OS_MTOD(m, uint8_t *)[34] = 0xaa;
  OS_MTOD(m, uint8_t *)[35] = 0x55;
  OS_MTOD(m, uint8_t *)[36] = 0xaa;
  OS_MTOD(m, uint8_t *)[37] = 0x55;

  lagopus_packet_init(pkt, m, &port);
  add_match(&flow->match_list, 6, OFPXMT_OFB_ARP_THA << 1,
            0xe0, 0x4d, 0x01, 0x34, 0x56, 0x78);
  refresh_match(flow);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_THA mismatch error.");
  OS_MTOD(m, uint8_t *)[32] = 0xe0;
  OS_MTOD(m, uint8_t *)[33] = 0x4d;
  OS_MTOD(m, uint8_t *)[34] = 0x01;
  OS_MTOD(m, uint8_t *)[35] = 0x34;
  OS_MTOD(m, uint8_t *)[36] = 0x56;
  OS_MTOD(m, uint8_t *)[37] = 0x78;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_THA match error.");
}

void
test_match_basic_ARP_THA_W(void) {
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
            0x08, 0x06);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x06;
  OS_MTOD(m, uint8_t *)[20] = 0x00;
  OS_MTOD(m, uint8_t *)[21] = ARPOP_REQUEST;
  OS_MTOD(m, uint8_t *)[32] = 0xaa;
  OS_MTOD(m, uint8_t *)[33] = 0x55;
  OS_MTOD(m, uint8_t *)[34] = 0xaa;
  OS_MTOD(m, uint8_t *)[35] = 0x55;
  OS_MTOD(m, uint8_t *)[36] = 0xaa;
  OS_MTOD(m, uint8_t *)[37] = 0x55;

  lagopus_packet_init(pkt, m, &port);
  add_match(&flow->match_list, 12, (OFPXMT_OFB_ARP_THA << 1) + 1,
            0xe0, 0x4d, 0x01, 0x00, 0x00, 0x00,
            0xff, 0xff, 0xff, 0x00, 0x00, 0x00);
  refresh_match(flow);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_THA_W mismatch error.");
  OS_MTOD(m, uint8_t *)[32] = 0x78;
  OS_MTOD(m, uint8_t *)[33] = 0x56;
  OS_MTOD(m, uint8_t *)[34] = 0x34;
  OS_MTOD(m, uint8_t *)[35] = 0x01;
  OS_MTOD(m, uint8_t *)[36] = 0x4d;
  OS_MTOD(m, uint8_t *)[37] = 0xe0;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "ARP_THA_W mismatch(2) error.");
  OS_MTOD(m, uint8_t *)[32] = 0xe0;
  OS_MTOD(m, uint8_t *)[33] = 0x4d;
  OS_MTOD(m, uint8_t *)[34] = 0x01;
  OS_MTOD(m, uint8_t *)[35] = 0x34;
  OS_MTOD(m, uint8_t *)[36] = 0x56;
  OS_MTOD(m, uint8_t *)[37] = 0x78;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "ARP_THA_W match error.");
}
