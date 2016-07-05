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
test_push_mpls(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_push *action_push;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) +
                  sizeof(*action_push) - sizeof(struct ofp_action_header));
  action_push = (struct ofp_action_push *)&action->ofpat;
  action_push->type = OFPAT_PUSH_MPLS;
  action_push->ethertype = 0x8847;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;
  OS_MTOD(m, uint8_t *)[22] = 240;

  lagopus_packet_init(pkt, m, &port);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_M_PKTLEN(m), 64 + 4,
                            "PUSH_MPLS length error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[12], 0x88,
                            "PUSH_MPLS ethertype[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[13], 0x47,
                            "PUSH_MPLS ethertype[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[16], 1,
                            "PUSH_MPLS BOS error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[17], 240,
                            "PUSH_MPLS TTL error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[18], 0x45,
                            "PUSH_MPLS payload error.");

  OS_MTOD(m, uint8_t *)[14] = 0x12;
  OS_MTOD(m, uint8_t *)[15] = 0x34;
  OS_MTOD(m, uint8_t *)[16] = 0x5f;

  action_push->ethertype = 0x8848;
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_M_PKTLEN(m), 64 + 8,
                            "PUSH_MPLS(2) length error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[12], 0x88,
                            "PUSH_MPLS(2) ethertype[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[13], 0x48,
                            "PUSH_MPLS(2) ethertype[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[14], 0x12,
                            "PUSH_MPLS(2) LSE[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[15], 0x34,
                            "PUSH_MPLS(2) LSE[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[16], 0x5e,
                            "PUSH_MPLS(2) LSE[2] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[17], 240,
                            "PUSH_MPLS(2) LSE[3] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[18], 0x12,
                            "PUSH_MPLS(2) payload[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[19], 0x34,
                            "PUSH_MPLS(2) payload[1] error.");
}

void
test_push_vlan(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_push *action_push;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) +
                  sizeof(*action_push) - sizeof(struct ofp_action_header));
  action_push = (struct ofp_action_push *)&action->ofpat;
  action_push->type = OFPAT_PUSH_VLAN;
  action_push->ethertype = 0x8100;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  OS_MTOD(m, uint8_t *)[12] = 0x08;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x45;

  lagopus_packet_init(pkt, m, &port);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_M_PKTLEN(m), 64 + 4,
                            "PUSH_VLAN length error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[12], 0x81,
                            "PUSH_VLAN TPID[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[13], 0x00,
                            "PUSH_VLAN TPID[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[16], 0x08,
                            "PUSH_VLAN ethertype[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[17], 0x00,
                            "PUSH_VLAN ethertype[1] error.");

  OS_MTOD(m, uint8_t *)[14] = 0xef;
  OS_MTOD(m, uint8_t *)[15] = 0xfe;
  action_push->ethertype = 0x88a8;
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_M_PKTLEN(m), 64 + 8,
                            "PUSH_VLAN(2) length error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[12], 0x88,
                            "PUSH_VLAN(2) TPID[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[13], 0xa8,
                            "PUSH_VLAN(2) TPID[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[14], 0xef,
                            "PUSH_VLAN(2) TCI[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[15], 0xfe,
                            "PUSH_VLAN(2) TCI[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[16], 0x81,
                            "PUSH_VLAN(2) ethertype[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[17], 0x00,
                            "PUSH_VLAN(2) ethertype[1] error.");
}

void
test_push_pbb(void) {
  static const uint8_t dhost[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
  static const uint8_t shost[] = { 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb };
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_push *action_push;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) +
                  sizeof(*action_push) - sizeof(struct ofp_action_header));
  action_push = (struct ofp_action_push *)&action->ofpat;
  action_push->type = OFPAT_PUSH_PBB;
  action_push->ethertype = 0x88e7;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64);
  OS_MEMCPY(&OS_MTOD(m, uint8_t *)[0], dhost, ETH_ALEN);
  OS_MEMCPY(&OS_MTOD(m, uint8_t *)[6], shost, ETH_ALEN);

  lagopus_packet_init(pkt, m, &port);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_M_PKTLEN(m), 64 + 18,
                            "PUSH_PBB length error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[12], 0x88,
                            "PUSH_PBB TPID[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[13], 0xe7,
                            "PUSH_PBB TPID[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[14], 0,
                            "PUSH_PBB pcp_dei error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[15], 0x00,
                            "PUSH_PBB i_sid[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[16], 0x00,
                            "PUSH_PBB i_sid[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[17], 0x00,
                            "PUSH_PBB i_sid[2] error.");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&OS_MTOD(m, uint8_t *)[18], dhost, ETH_ALEN,
                                        "PUSH_PBB dhost error.");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&OS_MTOD(m, uint8_t *)[24], shost, ETH_ALEN,
                                        "PUSH_PBB shost error.");
  OS_MTOD(m, uint8_t *)[15] = 0xab;
  OS_MTOD(m, uint8_t *)[16] = 0xcd;
  OS_MTOD(m, uint8_t *)[17] = 0xef;
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_M_PKTLEN(m), 64 + 18 + 18,
                            "PUSH_PBB length error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[15], 0xab,
                            "PUSH_PBB(2) i_sid[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[16], 0xcd,
                            "PUSH_PBB(2) i_sid[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[17], 0xef,
                            "PUSH_PBB(2) i_sid[2] error.");
}

void
test_pop_mpls(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_pop_mpls *action_pop;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) +
                  sizeof(*action_pop) - sizeof(struct ofp_action_header));
  action_pop = (struct ofp_action_pop_mpls *)&action->ofpat;
  action_pop->type = OFPAT_POP_MPLS;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64 + 8);
  /* initial, double taged */
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0x48;
  /* outer LSE */
  OS_MTOD(m, uint8_t *)[17] = 50;
  /* innner LSE */
  OS_MTOD(m, uint8_t *)[20] = 0x01;
  OS_MTOD(m, uint8_t *)[21] = 100;
  /* IPv4 */
  OS_MTOD(m, uint8_t *)[22] = 0x45;
  OS_MTOD(m, uint8_t *)[30] = 240;

  lagopus_packet_init(pkt, m, &port);
  action_pop->ethertype = 0x8847;
  TEST_ASSERT_EQUAL(execute_action(pkt, &action_list), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL_MESSAGE(OS_M_PKTLEN(m), 64 + 4,
                            "POP_MPLS length error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[12], 0x88,
                            "POP_MPLS ethertype[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[13], 0x47,
                            "POP_MPLS ethertype[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[16], 1,
                            "POP_MPLS BOS error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[17], 100,
                            "POP_MPLS TTL error.");
  action_pop->ethertype = 0x0800;
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_M_PKTLEN(m), 64,
                            "POP_MPLS(2) length error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[12], 0x08,
                            "POP_MPLS(2) ethertype[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[13], 0x00,
                            "POP_MPLS(2) ethertype[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[14], 0x45,
                            "POP_MPLS(2) ip_vhl error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[22], 240,
                            "POP_MPLS(2) ip_ttl error.");
}

void
test_pop_vlan(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_pop_mpls *action_pop;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action));
  action_pop = (struct ofp_action_pop_mpls *)&action->ofpat;
  action_pop->type = OFPAT_POP_VLAN;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64 + 4);
  OS_MTOD(m, uint8_t *)[12] = 0x81;
  OS_MTOD(m, uint8_t *)[13] = 0x00;
  OS_MTOD(m, uint8_t *)[14] = 0x30;
  OS_MTOD(m, uint8_t *)[15] = 50;
  OS_MTOD(m, uint8_t *)[16] = 0x08;
  OS_MTOD(m, uint8_t *)[17] = 0x00;
  OS_MTOD(m, uint8_t *)[18] = 0x45;
  OS_MTOD(m, uint8_t *)[26] = 240;

  lagopus_packet_init(pkt, m, &port);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_M_PKTLEN(m), 64,
                            "POP_VLAN length error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[12], 0x08,
                            "POP_VLAN ethertype[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[13], 0x00,
                            "POP_VLAN ethertype[1] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[14], 0x45,
                            "POP_VLAN ip_vhl error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[22], 240,
                            "POP_VLAN ip_ttl error.");
}

void
test_pop_pbb(void) {
  static const uint8_t odhost[] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab };
  static const uint8_t oshost[] = { 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54 };
  static const uint8_t idhost[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
  static const uint8_t ishost[] = { 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb };
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_pop_mpls *action_pop;
  struct lagopus_packet *pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action));
  action_pop = (struct ofp_action_pop_mpls *)&action->ofpat;
  action_pop->type = OFPAT_POP_PBB;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL_MESSAGE(pkt, "lagopus_alloc_packet error.");
  m = PKT2MBUF(pkt);

  OS_M_APPEND(m, 64 + 18);
  OS_MEMCPY(&OS_MTOD(m, uint8_t *)[0], odhost, ETH_ALEN);
  OS_MEMCPY(&OS_MTOD(m, uint8_t *)[6], oshost, ETH_ALEN);
  OS_MTOD(m, uint8_t *)[12] = 0x88;
  OS_MTOD(m, uint8_t *)[13] = 0xe7;
  OS_MTOD(m, uint8_t *)[14] = 0x00;
  OS_MTOD(m, uint8_t *)[15] = 0x11;
  OS_MTOD(m, uint8_t *)[16] = 0x22;
  OS_MTOD(m, uint8_t *)[17] = 0x33;
  /* dhost and shost */
  OS_MEMCPY(&OS_MTOD(m, uint8_t *)[18], idhost, ETH_ALEN);
  OS_MEMCPY(&OS_MTOD(m, uint8_t *)[24], ishost, ETH_ALEN);
  OS_MTOD(m, uint8_t *)[30] = 0x08;
  OS_MTOD(m, uint8_t *)[31] = 0x00;
  OS_MTOD(m, uint8_t *)[32] = 0x45;

  lagopus_packet_init(pkt, m, &port);
  execute_action(pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(OS_M_PKTLEN(m), 64,
                            "POP_PBB length error.");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&OS_MTOD(m, uint8_t *)[0], idhost, ETH_ALEN,
                                        "POP_PBB ether_dhost error.");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&OS_MTOD(m, uint8_t *)[6], ishost, ETH_ALEN,
                                        "POP_PBB ether_shost error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[12], 0x08,
                            "POP_PBB ethertype[0] error.");
  TEST_ASSERT_EQUAL_MESSAGE(OS_MTOD(m, uint8_t *)[13], 0x00,
                            "POP_PBB ethertype[1] error.");
}
