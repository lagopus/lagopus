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
test_match_basic_PBB_ISID(void) {
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

#ifndef PBB_IS_VLAN
  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x88, 0xe7);
#endif
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0xe7;
  add_match(&flow->match_list, 3, OFPXMT_OFB_PBB_ISID << 1,
            0x5a, 0xc3, 0x3c);
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "PBB_ISID mismatch(1) error.");
  OS_MTOD(m, uint8_t *)[15] = 0x3c;
  OS_MTOD(m, uint8_t *)[16] = 0xc3;
  OS_MTOD(m, uint8_t *)[17] = 0x5a;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "PBB_ISID mismatch(2) error.");
  OS_MTOD(m, uint8_t *)[15] = 0x5a;
  OS_MTOD(m, uint8_t *)[16] = 0xc3;
  OS_MTOD(m, uint8_t *)[17] = 0x3c;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "PBB_ISID match error.");
}

void
test_match_basic_PBB_ISID_W(void) {
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

#ifndef PBB_IS_VLAN
  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x88, 0xe7);
#endif
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0xe7;
  add_match(&flow->match_list, 6, (OFPXMT_OFB_PBB_ISID << 1) + 1,
            0x5a, 0xc3, 0x00, 0xff, 0xff, 0x00);
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "PBB_ISID_W mismatch(1) error.");
  OS_MTOD(m, uint8_t *)[15] = 0x3c;
  OS_MTOD(m, uint8_t *)[16] = 0xc3;
  OS_MTOD(m, uint8_t *)[17] = 0x5a;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "PBB_ISID_W mismatch(2) error.");
  OS_MTOD(m, uint8_t *)[15] = 0x5a;
  OS_MTOD(m, uint8_t *)[16] = 0xc3;
  OS_MTOD(m, uint8_t *)[17] = 0x3c;
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "PBB_ISID_W match error.");
}

void
xtest_match_basic_PBB_UCA(void) {
}
