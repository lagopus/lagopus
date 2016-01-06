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
test_copy_ttl_out_IPV4_to_MPLS(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action->ofpat.type = OFPAT_COPY_TTL_OUT;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x88;
  m->data[13] = 0x47;
  m->data[16] = 0x01; /* BOS */
  m->data[17] = 100;  /* MPLS TTL */
  m->data[18] = 0x45;
  m->data[26] = 10;   /* IPv4 TTL */

  lagopus_packet_init(&pkt, m, &port);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[26], 10,
                            "COPY_TTL_OUT_IPv4_to_MPLS(inner) error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[17], 10,
                            "COPY_TTL_OUT_IPv4_to_MPLS(outer) error.");
}

void
test_copy_ttl_out_IPV6_to_MPLS(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action->ofpat.type = OFPAT_COPY_TTL_OUT;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x88;
  m->data[13] = 0x47;
  m->data[16] = 0x01; /* BOS */
  m->data[17] = 100;  /* MPLS TTL */
  m->data[18] = 0x60;
  m->data[25] = 10;   /* IPv6 TTL */

  lagopus_packet_init(&pkt, m, &port);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[25], 10,
                            "COPY_TTL_OUT_IPv6_to_MPLS(inner) error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[17], 10,
                            "COPY_TTL_OUT_IPv6_to_MPLS(outer) error.");
}

void
test_copy_ttl_out_MPLS_to_MPLS(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action->ofpat.type = OFPAT_COPY_TTL_OUT;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x88;
  m->data[13] = 0x47;
  m->data[17] = 100; /* outer MPLS TTL */
  m->data[21] = 10;  /* inner MPLS TTL */

  lagopus_packet_init(&pkt, m, &port);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[21], 10,
                            "COPY_TTL_OUT_MPLS_to_MPLS(inner) error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[17], 10,
                            "COPY_TTL_OUT_MPLS_to_MPLS(outer) error.");
}

void
test_copy_ttl_in_MPLS_to_IPV4(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action->ofpat.type = OFPAT_COPY_TTL_IN;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x88;
  m->data[13] = 0x47;
  m->data[16] = 0x01; /* BOS */
  m->data[17] = 100;  /* MPLS TTL */
  m->data[18] = 0x45;
  m->data[26] = 10;   /* IPv4 TTL */

  lagopus_packet_init(&pkt, m, &port);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[17], 100,
                            "COPY_TTL_IN_MPLS_to_IPv4(outer) error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[26], 100,
                            "COPY_TTL_IN_MPLS_to_IPv4(inner) error.");
}

void
test_copy_ttl_in_MPLS_to_IPV6(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action->ofpat.type = OFPAT_COPY_TTL_IN;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x88;
  m->data[13] = 0x47;
  m->data[16] = 0x01; /* BOS */
  m->data[17] = 100;  /* MPLS TTL */
  m->data[18] = 0x60;
  m->data[25] = 10;   /* IPv6 TTL */

  lagopus_packet_init(&pkt, m, &port);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[17], 100,
                            "COPY_TTL_IN_MPLS_to_IPv6(outer) error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[25], 100,
                            "COPY_TTL_IN_MPLS_to_IPv6(inner) error.");
}

void
test_copy_ttl_in_MPLS_to_MPLS(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action->ofpat.type = OFPAT_COPY_TTL_IN;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x88;
  m->data[13] = 0x47;
  m->data[17] = 100; /* outer MPLS TTL */
  m->data[21] = 10;  /* inner MPLS TTL */

  lagopus_packet_init(&pkt, m, &port);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[17], 100,
                            "COPY_TTL_IN_MPLS_to_MPLS(inner) error.");
  TEST_ASSERT_EQUAL_MESSAGE(m->data[21], 100,
                            "COPY_TTL_IN_MPLS_to_MPLS(outer) error.");
}

void
test_set_mpls_ttl(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_mpls_ttl *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_mpls_ttl *)&action->ofpat;
  action_set->type = OFPAT_SET_MPLS_TTL;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x88;
  m->data[13] = 0x47;

  lagopus_packet_init(&pkt, m, &port);
  action_set->mpls_ttl = 240;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[17], 240,
                            "SET_MPLS_TTL error.");
}

void
test_dec_mpls_ttl(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action->ofpat.type = OFPAT_DEC_MPLS_TTL;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x88;
  m->data[13] = 0x47;
  m->data[17] = 100;

  lagopus_packet_init(&pkt, m, &port);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[17], 99,
                            "DEC_MPLS_TTL error.");

  m->data[12] = 0x88;
  m->data[13] = 0x48;
  m->data[17] = 0;

  lagopus_packet_init(&pkt, m, &port);
  pkt.in_port = NULL;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[17], 0,
                            "DEC_MPLS_TTL(0) error.");
}

void
test_set_nw_ttl_IPV4(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_nw_ttl *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_nw_ttl *)&action->ofpat;
  action_set->type = OFPAT_SET_NW_TTL;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);


  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x08;
  m->data[13] = 0x00;
  m->data[14] = 0x45;

  lagopus_packet_init(&pkt, m, &port);
  action_set->nw_ttl = 240;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[22], 240,
                            "SET_NW_TTL_IPV4 error.");
}

void
test_set_nw_ttl_IPV6(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_nw_ttl *action_set;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action_set = (struct ofp_action_nw_ttl *)&action->ofpat;
  action_set->type = OFPAT_SET_NW_TTL;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[20] = IPPROTO_TCP;

  lagopus_packet_init(&pkt, m, &port);
  action_set->nw_ttl = 240;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[21], 240,
                            "SET_NW_TTL_IPV6 error.");
}

void
test_dec_nw_ttl_IPV4(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action->ofpat.type = OFPAT_DEC_NW_TTL;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x08;
  m->data[13] = 0x00;
  m->data[22] = 100;

  lagopus_packet_init(&pkt, m, &port);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[22], 99,
                            "DEC_NW_TTL_IPV4 error.");

  m->data[12] = 0x08;
  m->data[13] = 0x00;
  m->data[22] = 0;

  lagopus_packet_init(&pkt, m, &port);
  pkt.in_port = NULL;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[22], 0,
                            "DEC_NW_TTL_IPV4(0) error.");
}

void
test_dec_nw_ttl_IPV6(void) {
  struct port port;
  struct action_list action_list;
  struct action *action;
  struct lagopus_packet pkt;
  OS_MBUF *m;

  TAILQ_INIT(&action_list);
  action = calloc(1, sizeof(*action) + 64);
  action->ofpat.type = OFPAT_DEC_NW_TTL;
  lagopus_set_action_function(action);
  TAILQ_INSERT_TAIL(&action_list, action, entry);

  m = calloc(1, sizeof(*m));
  TEST_ASSERT_NOT_EQUAL_MESSAGE(m, NULL, "calloc error.");
  m->data = &m->dat[128];

  OS_M_PKTLEN(m) = 64;
  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[21] = 100;

  lagopus_packet_init(&pkt, m, &port);
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[21], 99,
                            "DEC_NW_TTL_IPV6 error.");

  m->data[12] = 0x86;
  m->data[13] = 0xdd;
  m->data[21] = 0;

  lagopus_packet_init(&pkt, m, &port);
  pkt.in_port = NULL;
  execute_action(&pkt, &action_list);
  TEST_ASSERT_EQUAL_MESSAGE(m->data[21], 0,
                            "DEC_NW_TTL_IPV6(0) error.");
}
