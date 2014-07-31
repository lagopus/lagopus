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
test_match_basic_PBB_ISID(void) {
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

#ifndef PBB_IS_VLAN
  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x88, 0xe7);
#endif
  m->data[12] = 0x88;
  m->data[13] = 0xe7;
  add_match(&flow->match_list, 3, OFPXMT_OFB_PBB_ISID << 1,
            0x5a, 0xc3, 0x3c);
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "PBB_ISID mismatch(1) error.");
  m->data[15] = 0x3c;
  m->data[16] = 0xc3;
  m->data[17] = 0x5a;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "PBB_ISID mismatch(2) error.");
  m->data[15] = 0x5a;
  m->data[16] = 0xc3;
  m->data[17] = 0x3c;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "PBB_ISID match error.");
}

void
test_match_basic_PBB_ISID_W(void) {
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

#ifndef PBB_IS_VLAN
  add_match(&flow->match_list, 2, OFPXMT_OFB_ETH_TYPE << 1,
            0x88, 0xe7);
#endif
  m->data[12] = 0x88;
  m->data[13] = 0xe7;
  add_match(&flow->match_list, 6, (OFPXMT_OFB_PBB_ISID << 1) + 1,
            0x5a, 0xc3, 0x00, 0xff, 0xff, 0x00);
  refresh_match(flow);
  lagopus_packet_init(&pkt, m);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "PBB_ISID_W mismatch(1) error.");
  m->data[15] = 0x3c;
  m->data[16] = 0xc3;
  m->data[17] = 0x5a;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "PBB_ISID_W mismatch(2) error.");
  m->data[15] = 0x5a;
  m->data[16] = 0xc3;
  m->data[17] = 0x3c;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "PBB_ISID_W match error.");
}

void
xtest_match_basic_PBB_UCA(void) {
}
