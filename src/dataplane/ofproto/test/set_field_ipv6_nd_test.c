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

#include <netinet/icmp6.h>

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
test_set_field_IPV6_ND_TARGET(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;
  int i;

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
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_ICMPV6;

  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, 16, OFPXMT_OFB_IPV6_ND_TARGET << 1,
            0x20, 0x01, 0x00, 0x00, 0xe0, 0x45, 0x22, 0xeb,
            0x09, 0x00, 0x00, 0x08, 0xdc, 0x18, 0x94, 0xad);
  execute_action(pkt, &action_list);
  i = 62;
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x20,
                            "SET_FIELD IPV6_ND_TARGET[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x01,
                            "SET_FIELD IPV6_ND_TARGET[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x00,
                            "SET_FIELD IPV6_ND_TARGET[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x00,
                            "SET_FIELD IPV6_ND_TARGET[3] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0xe0,
                            "SET_FIELD IPV6_ND_TARGET[4] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x45,
                            "SET_FIELD IPV6_ND_TARGET[5] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x22,
                            "SET_FIELD IPV6_ND_TARGET[6] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0xeb,
                            "SET_FIELD IPV6_ND_TARGET[7] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x09,
                            "SET_FIELD IPV6_ND_TARGET[8] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x00,
                            "SET_FIELD IPV6_ND_TARGET[9] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x00,
                            "SET_FIELD IPV6_ND_TARGET[10] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x08,
                            "SET_FIELD IPV6_ND_TARGET[11] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0xdc,
                            "SET_FIELD IPV6_ND_TARGET[12] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x18,
                            "SET_FIELD IPV6_ND_TARGET[13] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x94,
                            "SET_FIELD IPV6_ND_TARGET[14] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0xad,
                            "SET_FIELD IPV6_ND_TARGET[15] error.");
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_DSTOPTS;
  OS_MTOD(m, uint8_t *)[54] = IPPROTO_ICMPV6;
  OS_MTOD(m, uint8_t *)[55] = 0;
  lagopus_packet_init(pkt, m, &port);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[54], IPPROTO_ICMPV6,
                            "SET_FIELD IPV6_ND_TARGET proto error.");
  i = 70;
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x20,
                            "SET_FIELD IPV6_ND_TARGET[0](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x01,
                            "SET_FIELD IPV6_ND_TARGET[1](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x00,
                            "SET_FIELD IPV6_ND_TARGET[2](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x00,
                            "SET_FIELD IPV6_ND_TARGET[3](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0xe0,
                            "SET_FIELD IPV6_ND_TARGET[4](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x45,
                            "SET_FIELD IPV6_ND_TARGET[5](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x22,
                            "SET_FIELD IPV6_ND_TARGET[6](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0xeb,
                            "SET_FIELD IPV6_ND_TARGET[7](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x09,
                            "SET_FIELD IPV6_ND_TARGET[8](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x00,
                            "SET_FIELD IPV6_ND_TARGET[9](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x00,
                            "SET_FIELD IPV6_ND_TARGET[10](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x08,
                            "SET_FIELD IPV6_ND_TARGET[11](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0xdc,
                            "SET_FIELD IPV6_ND_TARGET[12](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x18,
                            "SET_FIELD IPV6_ND_TARGET[13](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x94,
                            "SET_FIELD IPV6_ND_TARGET[14](next hdr) error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0xad,
                            "SET_FIELD IPV6_ND_TARGET[15](next hdr) error.");
}

void
test_set_field_IPV6_ND_SLL(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;
  int i;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_PKTLEN(m) = 96;
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  OS_MTOD(m, uint8_t *)[19] = 0x80; /* PLEN */
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_ICMPV6;
  OS_MTOD(m, uint8_t *)[54] = ND_NEIGHBOR_SOLICIT;
  OS_MTOD(m, uint8_t *)[78] = ND_OPT_SOURCE_LINKADDR;
  OS_MTOD(m, uint8_t *)[79] = 1;
  OS_MTOD(m, uint8_t *)[86] = ND_OPT_TARGET_LINKADDR;
  OS_MTOD(m, uint8_t *)[87] = 1;
  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, ETH_ALEN, OFPXMT_OFB_IPV6_ND_SLL << 1,
            0xe0, 0x4d, 0x01, 0x34, 0x56, 0x78);
  execute_action(pkt, &action_list);
  i = 80;
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0xe0,
                            "SET_FIELD IPV6_ND_SLL[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x4d,
                            "SET_FIELD IPV6_ND_SLL[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x01,
                            "SET_FIELD IPV6_ND_SLL[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x34,
                            "SET_FIELD IPV6_ND_SLL[3] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x56,
                            "SET_FIELD IPV6_ND_SLL[4] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x78,
                            "SET_FIELD IPV6_ND_SLL[5] error.");
}

void
test_set_field_IPV6_ND_TLL(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_set_field *action_set;
  struct lagopus_packet *pkt;
  OS_MBUF *m;
  int i;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_set_field *)&action->ofpat;
  action_set->type = OFPAT_SET_FIELD;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_PKTLEN(m) = 128;
  OS_MTOD(m, uint8_t *)[12] = 0x86;
  OS_MTOD(m, uint8_t *)[13] = 0xdd;
  OS_MTOD(m, uint8_t *)[19] = 0x80; /* PLEN */
  OS_MTOD(m, uint8_t *)[20] = IPPROTO_ICMPV6;
  OS_MTOD(m, uint8_t *)[54] = ND_NEIGHBOR_ADVERT;
  OS_MTOD(m, uint8_t *)[78] = ND_OPT_SOURCE_LINKADDR;
  OS_MTOD(m, uint8_t *)[79] = 1;
  OS_MTOD(m, uint8_t *)[86] = ND_OPT_TARGET_LINKADDR;
  OS_MTOD(m, uint8_t *)[87] = 1;
  lagopus_packet_init(pkt, m, &port);
  set_match(action_set->field, ETH_ALEN, OFPXMT_OFB_IPV6_ND_TLL << 1,
            0xe0, 0x4d, 0x01, 0x34, 0x56, 0x78);
  execute_action(pkt, &action_list);
  i = 88;
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0xe0,
                            "SET_FIELD IPV6_ND_TLL[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x4d,
                            "SET_FIELD IPV6_ND_TLL[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x01,
                            "SET_FIELD IPV6_ND_TLL[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x34,
                            "SET_FIELD IPV6_ND_TLL[3] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x56,
                            "SET_FIELD IPV6_ND_TLL[4] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[i++], 0x78,
                            "SET_FIELD IPV6_ND_TLL[5] error.");
}
