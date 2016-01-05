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
test_match_basic_IPV6_ND_TARGET(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;
  int i;

  /* prepare packet */
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

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
  m->data[54] = ND_NEIGHBOR_SOLICIT;
  add_match(&flow->match_list, 16, OFPXMT_OFB_IPV6_ND_TARGET << 1,
            0x20, 0x01, 0x00, 0x00, 0xe0, 0x45, 0x22, 0xeb,
            0x09, 0x00, 0x00, 0x08, 0xdc, 0x18, 0x94, 0xad);
  refresh_match(flow);
  i = 62;
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
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_ND_TARGET mismatch error.");
  i = 62;
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
                            "IPV6_ND_TARGET match error.");
}

void
test_match_basic_IPV6_ND_SLL(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;
  int i;

  /* prepare packet */
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

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
  m->data[19] = 0x80; /* PLEN */
  m->data[20] = IPPROTO_ICMPV6;
  m->data[54] = ND_NEIGHBOR_SOLICIT;
  m->data[78] = ND_OPT_SOURCE_LINKADDR;
  m->data[79] = 1;
  m->data[86] = ND_OPT_TARGET_LINKADDR;
  m->data[87] = 1;
  add_match(&flow->match_list, ETH_ALEN, OFPXMT_OFB_IPV6_ND_SLL << 1,
            0xe0, 0x4d, 0x01, 0x34, 0x56, 0x78);
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_ND_SLL mismatch error.");
  i = 80;
  m->data[i++] = 0xe0;
  m->data[i++] = 0x4d;
  m->data[i++] = 0x01;
  m->data[i++] = 0x34;
  m->data[i++] = 0x56;
  m->data[i++] = 0x78;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_ND_SLL match error.");
}

void
test_match_basic_IPV6_ND_TLL(void) {
  struct lagopus_packet pkt;
  struct port port;
  struct flow *flow;
  OS_MBUF *m;
  bool rv;
  int i;

  /* prepare packet */
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

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
  m->data[19] = 0x80; /* PLEN */
  m->data[20] = IPPROTO_ICMPV6;
  m->data[54] = ND_NEIGHBOR_ADVERT;
  m->data[78] = ND_OPT_SOURCE_LINKADDR;
  m->data[79] = 1;
  m->data[86] = ND_OPT_TARGET_LINKADDR;
  m->data[87] = 1;
  add_match(&flow->match_list, ETH_ALEN, OFPXMT_OFB_IPV6_ND_TLL << 1,
            0xe0, 0x4d, 0x01, 0x34, 0x56, 0x78);
  refresh_match(flow);
  lagopus_packet_init(&pkt, m, &port);
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, false,
                            "IPV6_ND_TLL mismatch error.");
  i = 88;
  m->data[i++] = 0xe0;
  m->data[i++] = 0x4d;
  m->data[i++] = 0x01;
  m->data[i++] = 0x34;
  m->data[i++] = 0x56;
  m->data[i++] = 0x78;
  rv = match_basic(&pkt, flow);
  TEST_ASSERT_EQUAL_MESSAGE(rv, true,
                            "IPV6_ND_TLL match error.");
}
