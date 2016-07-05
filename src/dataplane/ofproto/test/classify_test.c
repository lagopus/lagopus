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
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;
  OS_MTOD(m, uint8_t *)[23] = IPPROTO_TCP;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IP,
                            "ether_type error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l3_hdr, &OS_MTOD(m, uint8_t *)[14],
                            "l3_hdr error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l4_hdr, &OS_MTOD(m, uint8_t *)[34],
                            "l4_hdr error.");
  /* with VLAN */
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x08;
  OS_MTOD(m, uint8_t *)[17] = 0x00;
  OS_MTOD(m, uint8_t *)[18] = 0x45;
  OS_MTOD(m, uint8_t *)[27] = IPPROTO_TCP;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IP,
                            "ether_type(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l3_hdr, &OS_MTOD(m, uint8_t *)[18],
                            "l3_hdr(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l4_hdr, &OS_MTOD(m, uint8_t *)[38],
                            "l4_hdr(vlan) error.");
}

void
test_classify_packet_IPV4_UDP(void) {
  struct port port;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;
  OS_MTOD(m, uint8_t *)[23] = IPPROTO_UDP;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IP,
                            "ether_type error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l3_hdr, &OS_MTOD(m, uint8_t *)[14],
                            "l3_hdr error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l4_hdr, &OS_MTOD(m, uint8_t *)[34],
                            "l4_hdr error.");
  /* with VLAN */
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x08;
  OS_MTOD(m, uint8_t *)[17] = 0x00;
  OS_MTOD(m, uint8_t *)[18] = 0x45;
  OS_MTOD(m, uint8_t *)[27] = IPPROTO_UDP;

  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IP,
                            "ether_type(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l3_hdr, &OS_MTOD(m, uint8_t *)[18],
                            "l3_hdr(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l4_hdr, &OS_MTOD(m, uint8_t *)[38],
                            "l4_hdr(vlan) error.");
}

void
test_classify_packet_IPV4_SCTP(void) {
  struct port port;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;
  OS_MTOD(m, uint8_t *)[23] = IPPROTO_SCTP;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IP,
                            "ether_type error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l3_hdr, &OS_MTOD(m, uint8_t *)[14],
                            "l3_hdr error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l4_hdr, &OS_MTOD(m, uint8_t *)[34],
                            "l4_hdr error.");
  /* with VLAN */
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x08;
  OS_MTOD(m, uint8_t *)[17] = 0x00;
  OS_MTOD(m, uint8_t *)[18] = 0x45;
  OS_MTOD(m, uint8_t *)[27] = IPPROTO_SCTP;

  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IP,
                            "ether_type(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l3_hdr, &OS_MTOD(m, uint8_t *)[18],
                            "l3_hdr(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l4_hdr, &OS_MTOD(m, uint8_t *)[38],
                            "l4_hdr(vlan) error.");
}

void
test_classify_packet_IPV4_ICMP(void) {
  struct port port;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  /* prepare packet */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;
  OS_MTOD(m, uint8_t *)[23] = IPPROTO_ICMP;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IP,
                            "ether_type error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l3_hdr, &OS_MTOD(m, uint8_t *)[14],
                            "l3_hdr error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l4_hdr, &OS_MTOD(m, uint8_t *)[34],
                            "l4_hdr error.");
  /* with VLAN */
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x08;
  OS_MTOD(m, uint8_t *)[17] = 0x00;
  OS_MTOD(m, uint8_t *)[18] = 0x45;
  OS_MTOD(m, uint8_t *)[27] = IPPROTO_ICMP;

  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IP,
                            "ether_type(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l3_hdr, &OS_MTOD(m, uint8_t *)[18],
                            "l3_hdr(vlan) error.");
  TEST_ASSERT_EQUAL_MESSAGE(pkt->l4_hdr, &OS_MTOD(m, uint8_t *)[38],
                            "l4_hdr(vlan) error.");
}

void
test_classify_packet_IPV4_other(void) {
  struct port port;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;
  OS_MTOD(m, uint8_t *)[23] = IPPROTO_RSVP;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IP,
                            "ether_type error.");
  /* with VLAN */
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x08;
  OS_MTOD(m, uint8_t *)[17] = 0x00;
  OS_MTOD(m, uint8_t *)[18] = 0x45;
  OS_MTOD(m, uint8_t *)[27] = IPPROTO_RSVP;

  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IP,
                            "ether_type(vlan) error.");
}

void
test_classify_packet_ARP(void) {
  struct port port;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x06;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_ARP,
                            "ether_type error.");
  /* with VLAN */
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x08;
  OS_MTOD(m, uint8_t *)[17] = 0x06;

  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_ARP,
                            "ether_type(vlan) error.");
}

void
test_classify_packet_IPV6_TCP(void) {
  struct port port;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_TCP;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type error.");
  /* with exthdr */
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_TCP;
  OS_MTOD(m, uint8_t *)[55] = 0;
  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type(exthdr) error.");
  /* with VLAN */
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x86;
  OS_MTOD(m, uint8_t *)[17] = 0xdd;
  OS_MTOD(m, uint8_t *)[24] = IPPROTO_TCP;
  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan) error.");
  /* with VLAN and exthdr */
  OS_MTOD(m, uint8_t *)[24] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[58] = IPPROTO_TCP;
  OS_MTOD(m, uint8_t *)[59] = 0;
  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan+exthdr) error.");
}

void
test_classify_packet_IPV6_UDP(void) {
  struct port port;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_UDP;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type error.");
  /* with exthdr */
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_UDP;
  OS_MTOD(m, uint8_t *)[55] = 0;
  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type(exthdr) error.");
  /* with VLAN */
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x86;
  OS_MTOD(m, uint8_t *)[17] = 0xdd;
  OS_MTOD(m, uint8_t *)[24] = IPPROTO_UDP;
  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan) error.");
  /* with VLAN and exthdr */
  OS_MTOD(m, uint8_t *)[24] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[58] = IPPROTO_UDP;
  OS_MTOD(m, uint8_t *)[59] = 0;
  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan+exthdr) error.");
}

void
test_classify_packet_IPV6_SCTP(void) {
  struct port port;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_SCTP;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type error.");
  /* with exthdr */
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_SCTP;
  OS_MTOD(m, uint8_t *)[55] = 0;
  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type(exthdr) error.");
  /* with VLAN */
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x86;
  OS_MTOD(m, uint8_t *)[17] = 0xdd;
  OS_MTOD(m, uint8_t *)[24] = IPPROTO_SCTP;
  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan) error.");
  /* with VLAN and exthdr */
  OS_MTOD(m, uint8_t *)[24] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[58] = IPPROTO_SCTP;
  OS_MTOD(m, uint8_t *)[59] = 0;
  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan+exthdr) error.");
}

void
test_classify_packet_IPV6_ICMPV6(void) {
  struct port port;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_ICMPV6;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type error.");
  /* with exthdr */
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_ICMPV6;
  OS_MTOD(m, uint8_t *)[55] = 0;
  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type(exthdr) error.");
  /* with VLAN */
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x86;
  OS_MTOD(m, uint8_t *)[17] = 0xdd;
  OS_MTOD(m, uint8_t *)[24] = IPPROTO_ICMPV6;
  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan) error.");
  /* with VLAN and exthdr */
  OS_MTOD(m, uint8_t *)[24] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[58] = IPPROTO_ICMPV6;
  OS_MTOD(m, uint8_t *)[59] = 0;
  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan+exthdr) error.");
}

void
test_classify_packet_IPV6_other(void) {
  struct port port;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_APPEND(m, 64);

  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_RSVP;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type error.");
  /* with exthdr */
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_RSVP;
  OS_MTOD(m, uint8_t *)[55] = 0;
  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type(exthdr) error.");
  /* with VLAN */
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x86;
  OS_MTOD(m, uint8_t *)[17] = 0xdd;
  OS_MTOD(m, uint8_t *)[24] = IPPROTO_RSVP;
  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan) error.");
  /* with VLAN and exthdr */
  OS_MTOD(m, uint8_t *)[24] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[58] = IPPROTO_RSVP;
  OS_MTOD(m, uint8_t *)[59] = 0;
  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IPV6,
                            "ether_type(vlan+exthdr) error.");
}

void
test_classify_packet_MPLS(void) {
  struct port port;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x47;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_MPLS,
                            "ether_type error.");
  /* with VLAN */
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x88;
  OS_MTOD(m, uint8_t *)[17] = 0x47;

  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_MPLS,
                            "ether_type(vlan) error.");
}

void
test_classify_packet_MPLSMC(void) {
  struct port port;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x48;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_MPLS_MCAST,
                            "ether_type error.");
  /* with VLAN */
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x88;
  OS_MTOD(m, uint8_t *)[17] = 0x48;

  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_MPLS_MCAST,
                            "ether_type(vlan) error.");
}

void
test_classify_packet_PBB(void) {
  struct port port;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);
  OS_M_PKTLEN(m) = 128;

#ifdef PBB_IS_VLAN
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0xe7;
  OS_MTOD(m, uint8_t *)[30] = 0x08;
  OS_MTOD(m, uint8_t *)[31] = 0x00;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IP,
                            "ether_type error.");
  /* with VLAN */
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x88;
  OS_MTOD(m, uint8_t *)[17] = 0xe7;
  OS_MTOD(m, uint8_t *)[34] = 0x08;
  OS_MTOD(m, uint8_t *)[35] = 0x00;

  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_IP,
                            "ether_type(vlan) error.");
#else
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0xe7;

  lagopus_packet_init(pkt, m, &port);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_PBB,
                            "ether_type error.");
  /* with VLAN */
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[16] = 0x88;
  OS_MTOD(m, uint8_t *)[17] = 0xe7;

  classify_ether_packet(pkt);
  TEST_ASSERT_EQUAL_MESSAGE(pkt->ether_type, ETHERTYPE_PBB,
                            "ether_type(vlan) error.");
#endif
}
