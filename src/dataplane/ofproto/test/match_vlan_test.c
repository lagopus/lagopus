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
test_match_basic_VLAN_VID(void) {
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

  /* VLAN */
  add_match(&flow->match_list, 2, OFPXMT_OFB_VLAN_VID << 1,
            0x10, 0x01);
  refresh_match(flow);
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "VLAN_VID mismatch error.");
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x00;
  OS_MTOD(m, uint8_t *)[15] = 0x01;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "VLAN VID match error.");
  free(m);
}

void
test_match_basic_VLAN_VID_W(void) {
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

  /* VLAN VID_NONE */
  add_match(&flow->match_list, 4, (OFPXMT_OFB_VLAN_VID << 1) + 1,
            0x10, 0x00, 0x10, 0x00);
  refresh_match(flow);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "VLAN_VID_W mismatch error.");
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x00;
  OS_MTOD(m, uint8_t *)[15] = 0xff;
  lagopus_packet_init(pkt, m, &port);
  rv = match_basic(pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "VLAN VID_W match error.");
  free(m);
}
