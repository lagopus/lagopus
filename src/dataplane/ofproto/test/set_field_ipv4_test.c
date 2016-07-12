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
test_set_field_IPV4_IP_DSCP(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 1, OFPXMT_OFB_IP_DSCP << 1,
            0x3f);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[15], 0xfc,
                            "SET_FIELD IPV4_IP_DSCP error.");
}

void
test_set_field_IPV4_IP_ECN(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 1, OFPXMT_OFB_IP_ECN << 1,
            0x3);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[15], 0x03,
                            "SET_FIELD IPV4_IP_ECN error.");
}

void
test_set_field_IPV4_IP_PROTO(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 1, OFPXMT_OFB_IP_PROTO << 1,
            IPPROTO_ICMP);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[23], IPPROTO_ICMP,
                            "SET_FIELD IPV4_IP_PROTO error.");
}

void
test_set_field_IPV4_SRC(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 4, OFPXMT_OFB_IPV4_SRC << 1,
            192, 168, 1, 12);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[26], 192,
                            "SET_FIELD IPV4_SRC[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[27], 168,
                            "SET_FIELD IPV4_SRC[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[28], 1,
                            "SET_FIELD IPV4_SRC[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[29], 12,
                            "SET_FIELD IPV4_SRC[3] error.");
}
void
test_set_field_IPV4_DST(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 4, OFPXMT_OFB_IPV4_DST << 1,
            192, 168, 1, 12);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[30], 192,
                            "SET_FIELD IPV4_DST[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[31], 168,
                            "SET_FIELD IPV4_DST[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[32], 1,
                            "SET_FIELD IPV4_DST[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[33], 12,
                            "SET_FIELD IPV4_DST[3] error.");
}

void
test_set_field_IPV4_TCP_SRC(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;
  uint16_t len;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  len = (uint16_t)(OS_M_PKTLEN(m) - sizeof(ETHER_HDR));
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;
  *((uint16_t *)&OS_MTOD(m, uint8_t *)[16]) = OS_HTONS(len);
  OS_MTOD(m, uint8_t *)[23] = IPPROTO_TCP;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 2, OFPXMT_OFB_TCP_SRC << 1,
            0xaa, 0x55);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[34], 0xaa,
                            "SET_FIELD IPV4_TCP_SRC[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[35], 0x55,
                            "SET_FIELD IPV4_TCP_SRC[1] error.");
}

void
test_set_field_IPV4_TCP_DST(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;
  uint16_t len;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  len = (uint16_t)(OS_M_PKTLEN(m) - sizeof(ETHER_HDR));
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;
  *((uint16_t *)&OS_MTOD(m, uint8_t *)[16]) = OS_HTONS(len);
  OS_MTOD(m, uint8_t *)[23] = IPPROTO_TCP;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 2, OFPXMT_OFB_TCP_DST << 1,
            0xaa, 0x55);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[36], 0xaa,
                            "SET_FIELD IPV4_TCP_DST[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[37], 0x55,
                            "SET_FIELD IPV4_TCP_DST[1] error.");
}

void
test_set_field_IPV4_UDP_SRC(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;
  uint16_t len;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  len = (uint16_t)(OS_M_PKTLEN(m) - sizeof(ETHER_HDR));
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;
  *((uint16_t *)&OS_MTOD(m, uint8_t *)[16]) = OS_HTONS(len);
  OS_MTOD(m, uint8_t *)[23] = IPPROTO_UDP;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 2, OFPXMT_OFB_UDP_SRC << 1,
            0xaa, 0x55);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[34], 0xaa,
                            "SET_FIELD IPV4_UDP_SRC[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[35], 0x55,
                            "SET_FIELD IPV4_UDP_SRC[1] error.");
}

void
test_set_field_IPV4_UDP_DST(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;
  uint16_t len;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  len = (uint16_t)(OS_M_PKTLEN(m) - sizeof(ETHER_HDR));
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;
  *((uint16_t *)&OS_MTOD(m, uint8_t *)[16]) = OS_HTONS(len);
  OS_MTOD(m, uint8_t *)[23] = IPPROTO_UDP;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 2, OFPXMT_OFB_UDP_DST << 1,
            0xaa, 0x55);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[36], 0xaa,
                            "SET_FIELD IPV4_UDP_DST[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[37], 0x55,
                            "SET_FIELD IPV4_UDP_DST[1] error.");
}

void
test_set_field_IPV4_SCTP_SRC(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;
  uint16_t len;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  len = (uint16_t)(OS_M_PKTLEN(m) - sizeof(ETHER_HDR));
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;
  *((uint16_t *)&OS_MTOD(m, uint8_t *)[16]) = OS_HTONS(len);
  OS_MTOD(m, uint8_t *)[23] = IPPROTO_SCTP;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 2, OFPXMT_OFB_SCTP_SRC << 1,
            0xaa, 0x55);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[34], 0xaa,
                            "SET_FIELD IPV4_SCTP_SRC[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[35], 0x55,
                            "SET_FIELD IPV4_SCTP_SRC[1] error.");
}

void
test_set_field_IPV4_SCTP_DST(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;
  uint16_t len;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  len = (uint16_t)(OS_M_PKTLEN(m) - sizeof(ETHER_HDR));
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;
  *((uint16_t *)&OS_MTOD(m, uint8_t *)[16]) = OS_HTONS(len);
  OS_MTOD(m, uint8_t *)[23] = IPPROTO_SCTP;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 2, OFPXMT_OFB_SCTP_DST << 1,
            0xaa, 0x55);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[36], 0xaa,
                            "SET_FIELD IPV4_SCTP_DST[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[37], 0x55,
                            "SET_FIELD IPV4_SCTP_DST[1] error.");
}

void
test_set_field_ICMPV4_TYPE(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;
  uint16_t len;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  len = (uint16_t)(OS_M_PKTLEN(m) - sizeof(ETHER_HDR));
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;
  *((uint16_t *)&OS_MTOD(m, uint8_t *)[16]) = OS_HTONS(len);
  OS_MTOD(m, uint8_t *)[23] = IPPROTO_ICMP;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 1, OFPXMT_OFB_ICMPV4_TYPE << 1,
            0xaa);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[34], 0xaa,
                            "SET_FIELD ICMPV4_TYPE error.");
}

void
test_set_field_ICMPV4_CODE(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;
  uint16_t len;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  len = (uint16_t)(OS_M_PKTLEN(m) - sizeof(ETHER_HDR));
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;
  *((uint16_t *)&OS_MTOD(m, uint8_t *)[16]) = OS_HTONS(len);
  OS_MTOD(m, uint8_t *)[23] = IPPROTO_ICMP;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 1, OFPXMT_OFB_ICMPV4_CODE << 1,
            0x55);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[35], 0x55,
                            "SET_FIELD ICMPV4_CODE error.");
}
