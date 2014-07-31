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
#include "lagopus/dataplane.h"
#include "match.h"
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
test_set_field_IPV6_IP_DSCP(void) {
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 1, OFPXMT_OFB_IP_DSCP << 1,
            0x3f);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[14], 0x0f,
                            "SET_FIELD IPV6_IP_DSCP[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[15], 0xc0,
                            "SET_FIELD IPV6_IP_DSCP[1] error.");
}

void
test_set_field_IPV6_IP_ECN(void) {
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 1, OFPXMT_OFB_IP_ECN << 1,
            0x03);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[14], 0x00,
                            "SET_FIELD IPV6_IP_ECN[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[15], 0x30,
                            "SET_FIELD IPV6_IP_ECN[1] error.");
}

void
test_set_field_IPV6_IP_PROTO(void) {
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_TCP;
  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_ICMPV6);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[20], IPPROTO_ICMPV6,
                            "SET_FIELD IPV6_IP_PROTO error");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_TCP;
  m->data[55] = 0;
  lagopus_packet_init(&pkt, m);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[20], IPPROTO_DSTOPTS,
                            "SET_FIELD IPV6_IP_PROTO(next hdr) opt error");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[54], IPPROTO_ICMPV6,
                            "SET_FIELD IPV6_IP_PROTO(next hdr) error");
}

void
test_set_field_IPV6_SRC(void) {
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 16, OFPXMT_OFB_IPV6_SRC << 1,
            0x20, 0x01, 0x00, 0x00, 0xe0, 0x45, 0x22, 0xeb,
            0x09, 0x00, 0x00, 0x08, 0xdc, 0x18, 0x94, 0xad);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[22], 0x20,
                            "SET_FIELD IPV6_SRC[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[23], 0x01,
                            "SET_FIELD IPV6_SRC[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[24], 0x00,
                            "SET_FIELD IPV6_SRC[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[25], 0x00,
                            "SET_FIELD IPV6_SRC[3] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[26], 0xe0,
                            "SET_FIELD IPV6_SRC[4] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[27], 0x45,
                            "SET_FIELD IPV6_SRC[5] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[28], 0x22,
                            "SET_FIELD IPV6_SRC[6] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[29], 0xeb,
                            "SET_FIELD IPV6_SRC[7] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[30], 0x09,
                            "SET_FIELD IPV6_SRC[8] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[31], 0x00,
                            "SET_FIELD IPV6_SRC[9] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[32], 0x00,
                            "SET_FIELD IPV6_SRC[10] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[33], 0x08,
                            "SET_FIELD IPV6_SRC[11] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[34], 0xdc,
                            "SET_FIELD IPV6_SRC[12] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[35], 0x18,
                            "SET_FIELD IPV6_SRC[13] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[36], 0x94,
                            "SET_FIELD IPV6_SRC[14] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[37], 0xad,
                            "SET_FIELD IPV6_SRC[15] error.");
}

void
test_set_field_IPV6_DST(void) {
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;

  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 16, OFPXMT_OFB_IPV6_DST << 1,
            0x20, 0x01, 0x00, 0x00, 0xe0, 0x45, 0x22, 0xeb,
            0x09, 0x00, 0x00, 0x08, 0xdc, 0x18, 0x94, 0xad);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[38], 0x20,
                            "SET_FIELD IPV6_DST[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[39], 0x01,
                            "SET_FIELD IPV6_DST[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[40], 0x00,
                            "SET_FIELD IPV6_DST[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[41], 0x00,
                            "SET_FIELD IPV6_DST[3] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[42], 0xe0,
                            "SET_FIELD IPV6_DST[4] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[43], 0x45,
                            "SET_FIELD IPV6_DST[5] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[44], 0x22,
                            "SET_FIELD IPV6_DST[6] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[45], 0xeb,
                            "SET_FIELD IPV6_DST[7] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[46], 0x09,
                            "SET_FIELD IPV6_DST[8] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[47], 0x00,
                            "SET_FIELD IPV6_DST[9] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[48], 0x00,
                            "SET_FIELD IPV6_DST[10] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[49], 0x08,
                            "SET_FIELD IPV6_DST[11] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[50], 0xdc,
                            "SET_FIELD IPV6_DST[12] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[51], 0x18,
                            "SET_FIELD IPV6_DST[13] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[52], 0x94,
                            "SET_FIELD IPV6_DST[14] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[53], 0xad,
                            "SET_FIELD IPV6_DST[15] error.");
}

void
test_set_field_IPV6_FLABEL(void) {
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[14] = 0x6f;
  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 4, OFPXMT_OFB_IPV6_FLABEL << 1,
            0x00, 0x0e, 0xab, 0xcd);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[14], 0x6f,
                            "SET_FIELD IPV6_IPV6_FLABEL[0] error");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[15], 0x0e,
                            "SET_FIELD IPV6_IPV6_FLABEL[1] error");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[16], 0xab,
                            "SET_FIELD IPV6_IPV6_FLABEL[2] error");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[17], 0xcd,
                            "SET_FIELD IPV6_IPV6_FLABEL[3] error");
}

void
test_set_field_IPV6_TCP_SRC(void) {
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 128;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_TCP;

  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 2, OFPXMT_OFB_TCP_SRC << 1,
            0x80, 0x21);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[54], 0x80,
                            "SET_FIELD IPV6_TCP_SRC[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[55], 0x21,
                            "SET_FIELD IPV6_TCP_SRC[1] error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_TCP;
  m->data[55] = 0;
  lagopus_packet_init(&pkt, m);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[54], IPPROTO_TCP,
                            "SET_FIELD IPV6_TCP_SRC proto error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[62], 0x80,
                            "SET_FIELD IPV6_TCP_SRC[0](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[63], 0x21,
                            "SET_FIELD IPV6_TCP_SRC[1](next hdr) error.");
}

void
test_set_field_IPV6_TCP_DST(void) {
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 128;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_TCP;

  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 2, OFPXMT_OFB_TCP_DST << 1,
            0x80, 0x21);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[56], 0x80,
                            "SET_FIELD IPV6_TCP_DST[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[57], 0x21,
                            "SET_FIELD IPV6_TCP_DST[1] error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_TCP;
  m->data[55] = 0;
  lagopus_packet_init(&pkt, m);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[54], IPPROTO_TCP,
                            "SET_FIELD IPV6_TCP_DST proto error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[64], 0x80,
                            "SET_FIELD IPV6_TCP_DST[0](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[65], 0x21,
                            "SET_FIELD IPV6_TCP_DST[1](next hdr) error.");
}

void
test_set_field_IPV6_UDP_SRC(void) {
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 128;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_UDP;

  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 2, OFPXMT_OFB_UDP_SRC << 1,
            0x80, 0x21);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[54], 0x80,
                            "SET_FIELD IPV6_UDP_SRC[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[55], 0x21,
                            "SET_FIELD IPV6_UDP_SRC[1] error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_UDP;
  m->data[55] = 0;
  lagopus_packet_init(&pkt, m);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[54], IPPROTO_UDP,
                            "SET_FIELD IPV6_UDP_SRC proto error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[62], 0x80,
                            "SET_FIELD IPV6_UDP_SRC[0](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[63], 0x21,
                            "SET_FIELD IPV6_UDP_SRC[1](next hdr) error.");
}

void
test_set_field_IPV6_UDP_DST(void) {
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 128;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_UDP;

  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 2, OFPXMT_OFB_UDP_DST << 1,
            0x80, 0x21);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[56], 0x80,
                            "SET_FIELD IPV6_UDP_DST[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[57], 0x21,
                            "SET_FIELD IPV6_UDP_DST[1] error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_UDP;
  m->data[55] = 0;
  lagopus_packet_init(&pkt, m);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[54], IPPROTO_UDP,
                            "SET_FIELD IPV6_UDP_DST proto error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[64], 0x80,
                            "SET_FIELD IPV6_UDP_DST[0](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[65], 0x21,
                            "SET_FIELD IPV6_UDP_DST[1](next hdr) error.");
}

void
test_set_field_IPV6_SCTP_SRC(void) {
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 128;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_SCTP;

  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 2, OFPXMT_OFB_SCTP_SRC << 1,
            0x80, 0x21);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[54], 0x80,
                            "SET_FIELD IPV6_SCTP_SRC[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[55], 0x21,
                            "SET_FIELD IPV6_SCTP_SRC[1] error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_SCTP;
  m->data[55] = 0;
  lagopus_packet_init(&pkt, m);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[54], IPPROTO_SCTP,
                            "SET_FIELD IPV6_SCTP_SRC proto error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[62], 0x80,
                            "SET_FIELD IPV6_SCTP_SRC[0](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[63], 0x21,
                            "SET_FIELD IPV6_SCTP_SRC[1](next hdr) error.");
}

void
test_set_field_IPV6_SCTP_DST(void) {
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 128;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_SCTP;

  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 2, OFPXMT_OFB_SCTP_DST << 1,
            0x80, 0x21);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[56], 0x80,
                            "SET_FIELD IPV6_SCTP_DST[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[57], 0x21,
                            "SET_FIELD IPV6_SCTP_DST[1] error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_SCTP;
  m->data[55] = 0;
  lagopus_packet_init(&pkt, m);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[54], IPPROTO_SCTP,
                            "SET_FIELD IPV6_SCTP_DST proto error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[64], 0x80,
                            "SET_FIELD IPV6_SCTP_DST[0](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[65], 0x21,
                            "SET_FIELD IPV6_SCTP_DST[1](next hdr) error.");
}

void
test_set_field_ICMPV6_TYPE(void) {
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_ICMPV6;

  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 2, OFPXMT_OFB_ICMPV6_TYPE << 1,
            ICMP6_ECHO_REQUEST);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[54], ICMP6_ECHO_REQUEST,
                            "SET_FIELD ICMPV6_TYPE error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_ICMPV6;
  m->data[55] = 0;
  lagopus_packet_init(&pkt, m);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[54], IPPROTO_ICMPV6,
                            "SET_FIELD ICMPV6_TYPE proto error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[62], ICMP6_ECHO_REQUEST,
                            "SET_FIELD ICMPV6_TYPE(next hdr) error.");
}

void
test_set_field_ICMPV6_CODE(void) {
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_NULL_MESSAGE(m, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_ICMPV6;

  lagopus_packet_init(&pkt, m);
  set_match(action_set->field, 2, OFPXMT_OFB_ICMPV6_CODE << 1,
            ICMP6_DST_UNREACH_NOPORT);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[55], ICMP6_DST_UNREACH_NOPORT,
                            "SET_FIELD ICMPV6_CODE error.");
  m->data[20] = IPPROTO_DSTOPTS;
  m->data[54] = IPPROTO_ICMPV6;
  m->data[55] = 0;
  lagopus_packet_init(&pkt, m);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[54], IPPROTO_ICMPV6,
                            "SET_FIELD ICMPV6_CODE proto error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[63], ICMP6_DST_UNREACH_NOPORT,
                            "SET_FIELD ICMPV6_CODE(next hdr) error.");
}
