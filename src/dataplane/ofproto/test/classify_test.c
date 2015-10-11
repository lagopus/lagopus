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

#include "lagopus/flowdb.h"
#include "lagopus/ethertype.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
#include "pktbuf.h"
#include "packet.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"


void
setUp(void) {
}

void
tearDown(void) {
}

void
test_classify_packet_IPV4_TCP(void) {
  struct port port;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  /* prepare packet */
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

  m->data[12] = 0x08;
  m->data[13] = 0x00;
  m->data[14] = 0x45;
  m->data[23] = IPPROTO_TCP;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IP,
                            "ether_type error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l3_hdr, &m->data[14],
                            "l3_hdr error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l4_hdr, &m->data[34],
                            "l4_hdr error.");
  /* with VLAN */
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  m->data[27] = IPPROTO_TCP;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IP,
                            "ether_type(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l3_hdr, &m->data[18],
                            "l3_hdr(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l4_hdr, &m->data[38],
                            "l4_hdr(vlan) error.");
}

void
test_classify_packet_IPV4_UDP(void) {
  struct port port;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  /* prepare packet */
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

  m->data[12] = 0x08;
  m->data[13] = 0x00;
  m->data[14] = 0x45;
  m->data[23] = IPPROTO_UDP;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IP,
                            "ether_type error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l3_hdr, &m->data[14],
                            "l3_hdr error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l4_hdr, &m->data[34],
                            "l4_hdr error.");
  /* with VLAN */
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  m->data[27] = IPPROTO_UDP;

  pkt.mbuf = (OS_MBUF *)m;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IP,
                            "ether_type(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l3_hdr, &m->data[18],
                            "l3_hdr(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l4_hdr, &m->data[38],
                            "l4_hdr(vlan) error.");
}

void
test_classify_packet_IPV4_SCTP(void) {
  struct port port;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  /* prepare packet */
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

  m->data[12] = 0x08;
  m->data[13] = 0x00;
  m->data[14] = 0x45;
  m->data[23] = IPPROTO_SCTP;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IP,
                            "ether_type error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l3_hdr, &m->data[14],
                            "l3_hdr error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l4_hdr, &m->data[34],
                            "l4_hdr error.");
  /* with VLAN */
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  m->data[27] = IPPROTO_SCTP;

  pkt.mbuf = (OS_MBUF *)m;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IP,
                            "ether_type(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l3_hdr, &m->data[18],
                            "l3_hdr(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l4_hdr, &m->data[38],
                            "l4_hdr(vlan) error.");
}

void
test_classify_packet_IPV4_ICMP(void) {
  struct port port;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  /* prepare packet */
  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

  m->data[12] = 0x08;
  m->data[13] = 0x00;
  m->data[14] = 0x45;
  m->data[23] = IPPROTO_ICMP;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IP,
                            "ether_type error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l3_hdr, &m->data[14],
                            "l3_hdr error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l4_hdr, &m->data[34],
                            "l4_hdr error.");
  /* with VLAN */
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  m->data[27] = IPPROTO_ICMP;

  pkt.mbuf = (OS_MBUF *)m;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IP,
                            "ether_type(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l3_hdr, &m->data[18],
                            "l3_hdr(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt.l4_hdr, &m->data[38],
                            "l4_hdr(vlan) error.");
}

void
test_classify_packet_IPV4_other(void) {
  struct port port;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

  m->data[12] = 0x08;
  m->data[13] = 0x00;
  m->data[14] = 0x45;
  m->data[23] = IPPROTO_RSVP;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IP,
                            "ether_type error.");
  /* with VLAN */
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x00;
  m->data[18] = 0x45;
  m->data[27] = IPPROTO_RSVP;

  pkt.mbuf = (OS_MBUF *)m;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IP,
                            "ether_type(vlan) error.");
}

void
test_classify_packet_ARP(void) {
  struct port port;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

  m->data[12] = 0x08;
  m->data[13] = 0x06;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_ARP,
                            "ether_type error.");
  /* with VLAN */
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x08;
  m->data[17] = 0x06;

  pkt.mbuf = (OS_MBUF *)m;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_ARP,
                            "ether_type(vlan) error.");
}

void
test_classify_packet_IPV6_TCP(void) {
  struct port port;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_TCP;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type error.");
  /* with exthdr */
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_TCP;
  m->data[55] = 0;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type(exthdr) error.");
  /* with VLAN */
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x86;
  m->data[17] = 0xdd;
  m->data[24] = IPPROTO_TCP;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan) error.");
  /* with VLAN and exthdr */
  m->data[24] = IPPROTO_DSTOPTS;
  m->data[58] = IPPROTO_TCP;
  m->data[59] = 0;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan+exthdr) error.");
}

void
test_classify_packet_IPV6_UDP(void) {
  struct port port;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_UDP;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type error.");
  /* with exthdr */
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_UDP;
  m->data[55] = 0;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type(exthdr) error.");
  /* with VLAN */
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x86;
  m->data[17] = 0xdd;
  m->data[24] = IPPROTO_UDP;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan) error.");
  /* with VLAN and exthdr */
  m->data[24] = IPPROTO_DSTOPTS;
  m->data[58] = IPPROTO_UDP;
  m->data[59] = 0;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan+exthdr) error.");
}

void
test_classify_packet_IPV6_SCTP(void) {
  struct port port;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_SCTP;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type error.");
  /* with exthdr */
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_SCTP;
  m->data[55] = 0;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type(exthdr) error.");
  /* with VLAN */
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x86;
  m->data[17] = 0xdd;
  m->data[24] = IPPROTO_SCTP;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan) error.");
  /* with VLAN and exthdr */
  m->data[24] = IPPROTO_DSTOPTS;
  m->data[58] = IPPROTO_SCTP;
  m->data[59] = 0;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan+exthdr) error.");
}

void
test_classify_packet_IPV6_ICMPV6(void) {
  struct port port;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_ICMPV6;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type error.");
  /* with exthdr */
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_ICMPV6;
  m->data[55] = 0;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type(exthdr) error.");
  /* with VLAN */
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x86;
  m->data[17] = 0xdd;
  m->data[24] = IPPROTO_ICMPV6;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan) error.");
  /* with VLAN and exthdr */
  m->data[24] = IPPROTO_DSTOPTS;
  m->data[58] = IPPROTO_ICMPV6;
  m->data[59] = 0;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan+exthdr) error.");
}

void
test_classify_packet_IPV6_other(void) {
  struct port port;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 64;

  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_RSVP;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type error.");
  /* with exthdr */
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_RSVP;
  m->data[55] = 0;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type(exthdr) error.");
  /* with VLAN */
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x86;
  m->data[17] = 0xdd;
  m->data[24] = IPPROTO_RSVP;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan) error.");
  /* with VLAN and exthdr */
  m->data[24] = IPPROTO_DSTOPTS;
  m->data[58] = IPPROTO_RSVP;
  m->data[59] = 0;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan+exthdr) error.");
}

void
test_classify_packet_MPLS(void) {
  struct port port;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

  m->data[12] = 0x88;
  m->data[13] = 0x47;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_MPLS,
                            "ether_type error.");
  /* with VLAN */
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x88;
  m->data[17] = 0x47;

  pkt.mbuf = (OS_MBUF *)m;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_MPLS,
                            "ether_type(vlan) error.");
}

void
test_classify_packet_MPLSMC(void) {
  struct port port;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

  m->data[12] = 0x88;
  m->data[13] = 0x48;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_MPLS_MCAST,
                            "ether_type error.");
  /* with VLAN */
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x88;
  m->data[17] = 0x48;

  pkt.mbuf = (OS_MBUF *)m;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_MPLS_MCAST,
                            "ether_type(vlan) error.");
}

void
test_classify_packet_PBB(void) {
  struct port port;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];
  OS_M_PKTLEN(m) = 128;

#ifdef PBB_IS_VLAN
  m->data[12] = 0x88;
  m->data[13] = 0xe7;
  m->data[30] = 0x08;
  m->data[31] = 0x00;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IP,
                            "ether_type error.");
  /* with VLAN */
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x88;
  m->data[17] = 0xe7;
  m->data[34] = 0x08;
  m->data[35] = 0x00;

  pkt.mbuf = (OS_MBUF *)m;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_IP,
                            "ether_type(vlan) error.");
#else
  m->data[12] = 0x88;
  m->data[13] = 0xe7;

  lagopus_packet_init(&pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_PBB,
                            "ether_type error.");
  /* with VLAN */
  m->data[12] = 0x81;
  m->data[13] = 0x00;
  m->data[16] = 0x88;
  m->data[17] = 0xe7;

  pkt.mbuf = (OS_MBUF *)m;
  classify_packet(&pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt.ether_type, ETHERTYPE_PBB,
                            "ether_type(vlan) error.");
#endif
}
