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
#include "datapath_test_match.h"
#include "datapath_test_match_macros.h"
#include "flowinfo_test.h"


FLOWINFO_TEST_DECLARE_DATA;


/* Test addresses. */
static const char testsrcstr[] = "3ffe:1:2:3:4:5:6:0";
static const char testdststr[] = "3ffe:7:8:9:a:b:c:0";
static const char testmaskstr[] = "ffff:ffff:ffff:ffff::";
static struct sockaddr_in6 testsrc[ARRAY_LEN(test_flow)];
static struct sockaddr_in6 testdst[ARRAY_LEN(test_flow)];
static struct sockaddr_in6 testmask[ARRAY_LEN(test_flow)];


/* IPv6 extension header mask. */
static const uint16_t exthdrmask = 0x1f0;


/* Compute an IPv6 DSCP. */
#define TEST_IPV6_DSCP(_i)	((uint8_t)((_i) + 1))

/* Compute an IPv6 ECN. */
#define TEST_IPV6_ECN(_i)	((uint8_t)(_i))

/* Compute an IPv6 protocol. */
#define TEST_IPV6_PROTO(_i)	((uint8_t)((_i) + 1))

/* Compute an IPv6 flowlabel. */
#define TEST_IPV6_FLOWLABEL(_i)	((uint32_t)((_i) + 1))

/* Compute an IPv6 extension headers. */
#define TEST_IPV6_EXTHDRS(_i)	((uint16_t)((_i) + 1))

/* Compute the LSB octet of an IPv6 address. */
#define TEST_IPV6_ADDR_LSB(_i)	((uint8_t)((_i) + 1))


/*
 * Assert the test objects.
 *
 * The test flows must have the prerequisite matches only.
 */
#define TEST_ASSERT_OBJECTS()						\
  do {									\
    size_t _s, _c;							\
    TEST_ASSERT_NOT_NULL(flowinfo);					\
    for (_s = 0; _s < ARRAY_LEN(test_flow); _s++) {			\
      TEST_ASSERT_NOT_NULL(test_flow[_s]);				\
      _c = 0;								\
      TAILQ_COUNT(_c, struct match, &test_flow[_s]->match_list, entry); \
      TEST_ASSERT_EQUAL_INT(1, _c);					\
      TEST_ASSERT_FLOW_MATCH_IPV6_PREREQUISITE(test_flow[_s]);		\
    }									\
  } while (0)

/* Positively assert flow addition. */
#define TEST_ASSERT_FLOWINFO_ADDFLOW_OK(_fl, _bi, _ei, _flnum, _msg)	\
  do {									\
    size_t _s;								\
    for (_s = (_bi); _s < (_ei); _s++)					\
      TEST_ASSERT_FLOWINFO_ADD_OK((_fl), test_flow[_s], (_msg));	\
    TEST_ASSERT_FLOWINFO_FLOW_NUM((_fl), (_flnum), (_msg));		\
    if (NULL != (_fl)->misc->next[ETHERTYPE_IPV6])			\
      for (_s = (_bi); _s < (_ei); _s++)				\
        TEST_ASSERT_FLOWINFO_HASIPV6PROTO((_fl)->misc->next[ETHERTYPE_IPV6], TEST_IPV6_PROTO(_s), (_msg)); \
    else {								\
      char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
      \
      snprintf(__buf, sizeof(__buf), "%s, positive flow addition", (_msg)); \
      TEST_ASSERT_EQUAL_INT_MESSAGE((_flnum), 0, __buf);		\
    }									\
  } while (0)

/* Positively assert flow deletion. */
#define TEST_ASSERT_FLOWINFO_DELFLOW_OK(_fl, _bi, _ei, _flnum, _msg)	\
  do {									\
    size_t _s;								\
    for (_s = (_bi); _s < (_ei); _s++)					\
      TEST_ASSERT_FLOWINFO_DEL_OK((_fl), test_flow[_s], (_msg));	\
    TEST_ASSERT_FLOWINFO_FLOW_NUM((_fl), (_flnum), (_msg));		\
    if (NULL != (_fl)->misc->next[ETHERTYPE_IPV6])			\
      for (_s = (_bi); _s < (_ei); _s++)				\
        TEST_ASSERT_FLOWINFO_HASIPV6PROTO((_fl)->misc->next[ETHERTYPE_IPV6], TEST_IPV6_PROTO(_s), (_msg)); \
    else {								\
      char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
      \
      snprintf(__buf, sizeof(__buf), "%s, positive flow deletion", (_msg)); \
      TEST_ASSERT_EQUAL_INT_MESSAGE((_flnum), 0, __buf);		\
    }									\
  } while (0)

/* Negatively assert flow deletion without the IPv6 protocol check. */
#define _TEST_ASSERT_FLOWINFO_DELFLOW_NG(_fl, _bi, _ei, _flnum, _msg)	\
  do {									\
    size_t __s;								\
    for (__s = (_bi); __s < (_ei); __s++)				\
      TEST_ASSERT_FLOWINFO_DEL_NG((_fl), test_flow[__s], (_msg));	\
    TEST_ASSERT_FLOWINFO_FLOW_NUM((_fl), (_flnum), (_msg));		\
  } while (0)

/* Negatively assert flow deletion. */
#define TEST_ASSERT_FLOWINFO_DELFLOW_NG(_fl, _bi, _ei, _flnum, _msg)	\
  do {									\
    size_t _s;								\
    _TEST_ASSERT_FLOWINFO_DELFLOW_NG(_fl, _bi, _ei, (_flnum), (_msg));	\
    if (NULL != (_fl)->misc->next[ETHERTYPE_IPV6])			\
      for (_s = (_bi); _s < (_ei); _s++)				\
        TEST_ASSERT_FLOWINFO_HASIPV6PROTO((_fl)->misc->next[ETHERTYPE_IPV6], TEST_IPV6_PROTO(_s), (_msg)); \
    else {								\
      char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
      \
      snprintf(__buf, sizeof(__buf), "%s, negative flow deletion", (_msg)); \
      TEST_ASSERT_EQUAL_INT_MESSAGE((_flnum), 0, __buf);		\
    }									\
  } while (0)

/* Negatively assert flow deletion and non-existence of an IPv6 protocol. */
#undef TEST_ASSERT_FLOWINFO_DELFLOW_NG_CLEAN
#define TEST_ASSERT_FLOWINFO_DELFLOW_NG_CLEAN(_fl, _bi, _ei, _flnum, _msg) \
  do {									\
    size_t _s;								\
    _TEST_ASSERT_FLOWINFO_DELFLOW_NG(_fl, _bi, _ei, (_flnum), (_msg));	\
    if (NULL != (_fl)->misc->next[ETHERTYPE_IPV6])			\
      for (_s = (_bi); _s < (_ei); _s++)				\
        TEST_ASSERT_FLOWINFO_NOIPV6PROTO((_fl)->misc->next[ETHERTYPE_IPV6], TEST_IPV6_PROTO(_s), (_msg)); \
    else {								\
      char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
      \
      snprintf(__buf, sizeof(__buf), "%s, negative flow deletion, clean", (_msg)); \
      TEST_ASSERT_EQUAL_INT_MESSAGE((_flnum), 0, __buf);		\
    }									\
  } while (0)

/* Assert flow numbers. */
#define TEST_ASSERT_FLOWINFO_FLOW_NUM(_fl, _flnum, _msg)		\
  do {									\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl), (_flnum), (_msg));		\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc, (_flnum), (_msg));		\
    if (NULL != (_fl)->misc->next[ETHERTYPE_IPV6])			\
      TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc->next[ETHERTYPE_IPV6], (_flnum), (_msg)); \
    else {								\
      char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
      \
      snprintf(__buf, sizeof(__buf), "%s, flow count", (_msg));		\
      TEST_ASSERT_EQUAL_INT_MESSAGE((_flnum), 0, __buf);		\
    }									\
  } while (0)

/* Assert the existence of an IPv6 protocol in the next table. */
#define TEST_ASSERT_FLOWINFO_HASIPV6PROTO(_fl, _proto, _msg)		\
  do {									\
    char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(__buf, sizeof(__buf), "%s, has IPv6 proto", (_msg));	\
    TEST_ASSERT_NOT_NULL_MESSAGE((_fl)->next[_proto], __buf);		\
  } while (0)

/* Assert the non-existence of an IPv6 protocol in the next table. */
#define TEST_ASSERT_FLOWINFO_NOIPV6PROTO(_fl, _proto, _msg)		\
  do {									\
    char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(__buf, sizeof(__buf), "%s, no IPv6 proto", (_msg));	\
    TEST_ASSERT_NULL_MESSAGE((_fl)->next[_proto], __buf);		\
  } while (0)

void
setUp(void) {
  size_t s;
  uint8_t *p;
  lagopus_ip_address_t *addr;

  /* Make the root flowinfo. */
  TEST_ASSERT_NULL(flowinfo);
  flowinfo = new_flowinfo_vlan_vid();
  TEST_ASSERT_NOT_NULL(flowinfo);

  TEST_ASSERT_FLOWINFO_FLOW_NUM(flowinfo, 0, __func__);

  /*
   * Make the test flows.
   *
   * IPv6 matches have prerequisite.
   */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TEST_ASSERT_NULL(test_flow[s]);
    test_flow[s] = allocate_test_flow(10 * sizeof(struct match));
    TEST_ASSERT_NOT_NULL(test_flow[s]);
    test_flow[s]->priority = (int)s;
    FLOW_ADD_IPV6_PREREQUISITE(test_flow[s]);
  }

  /* Make the test addresses. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    lagopus_ip_address_create(testsrcstr, false, &addr);
    lagopus_ip_address_sockaddr_get(addr, &testsrc[s]);
    p = (uint8_t *)&testsrc[s].sin6_addr;
    p[sizeof(testsrc[s].sin6_addr) - 1] = (uint8_t)(TEST_IPV6_ADDR_LSB(
          s) & 0xff);

    lagopus_ip_address_create(testsrcstr, false, &addr);
    lagopus_ip_address_sockaddr_get(addr, &testdst[s]);
    p = (uint8_t *)&testdst[s].sin6_addr;
    p[sizeof(testdst[s].sin6_addr) - 1] = (uint8_t)(TEST_IPV6_ADDR_LSB(
          s) & 0xff);

    lagopus_ip_address_create(testmaskstr, false, &addr);
    lagopus_ip_address_sockaddr_get(addr, &testmask[s]);
  }
}

void
tearDown(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();
  TEST_ASSERT_FLOWINFO_FINDFLOW(flowinfo, false, __func__);

  /* Free the test flows. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    free_test_flow(test_flow[s]);
    test_flow[s] = NULL;
  }

  /* The root flowinfo must be empty. */
  TEST_ASSERT_FLOWINFO_FLOW_NUM(flowinfo, 0, __func__);

  /* Free the root flowinfo. */
  flowinfo->destroy_func(flowinfo);
  flowinfo = NULL;
}

/*
 * The bare (non-L3/L4-depending) IPv6 matches are stored in the leaf
 * basic flowinfo.  del -> 2*add -> 3*del is hence sufficient.
 */

void
test_flowinfo_ipv6_bare_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* No need to add any IPv6 matches. */

  /* Run the sideeffect-prone scenario. */
  TEST_SCENARIO_FLOWINFO_SEP(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_IPV6_PREREQUISITE(test_flow[s]);
  }
}

/*
 * The protocol test case performs 2*add -> 3*del -> add -> del
 * because the IPv4 flowinfo does not remove a next protocol entry
 * from the table upon deletion.  We hence have to cover adding both
 * from scratch and to the ptree in use.
 *
 * This also covers double addition/deletion and non-existing key
 * deletion.
 */

void
test_flowinfo_ipv6_proto_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the IPv6 protocol matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_IP_PROTO_MATCH(test_flow[s], TEST_IPV6_PROTO(s));
  }

  /* Run the sideeffect-prone scenario. */
  TEST_SCENARIO_FLOWINFO_SEP(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_IPV6_PREREQUISITE(test_flow[s]);
  }
}

/*
 * The IPv6 header matches are stored in the leaf basic flowinfo.  del
 * -> 2*add -> 3*del is hence sufficient.
 */

void
test_flowinfo_ipv6_dscp_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the IPv6 protocol and DSCP matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_IP_PROTO_MATCH(test_flow[s], TEST_IPV6_PROTO(s));
    FLOW_ADD_IP_DSCP_MATCH(test_flow[s], TEST_IPV6_DSCP(s));
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_IPV6_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_ipv6_ecn_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the IPv6 protocol and ECN matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_IP_PROTO_MATCH(test_flow[s], TEST_IPV6_PROTO(s));
    FLOW_ADD_IP_ECN_MATCH(test_flow[s], TEST_IPV6_ECN(s));
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_IPV6_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_ipv6_src_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the IPv6protocol and source address matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_IP_PROTO_MATCH(test_flow[s], TEST_IPV6_PROTO(s));
    FLOW_ADD_IP_SRC_MATCH(test_flow[s], &testsrc[s]);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_IPV6_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_ipv6_dst_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the IPv6 protocol and source address matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_IP_PROTO_MATCH(test_flow[s], TEST_IPV6_PROTO(s));
    FLOW_ADD_IP_DST_MATCH(test_flow[s], &testsrc[s]);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_IPV6_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_ipv6_src_w_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the IPv6protocol and source address matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_IP_PROTO_MATCH(test_flow[s], TEST_IPV6_PROTO(s));
    FLOW_ADD_IP_SRC_W_MATCH(test_flow[s], &testsrc[s], &testmask[s]);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_IPV6_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_ipv6_dst_w_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the IPv6 protocol and source address matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_IP_PROTO_MATCH(test_flow[s], TEST_IPV6_PROTO(s));
    FLOW_ADD_IP_DST_W_MATCH(test_flow[s], &testsrc[s], &testmask[s]);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_IPV6_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_ipv6_flowlabel_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the IPv6 protocol and flowlabel matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_IP_PROTO_MATCH(test_flow[s], TEST_IPV6_PROTO(s));
    FLOW_ADD_IPV6_FLOWLABEL_MATCH(test_flow[s], TEST_IPV6_FLOWLABEL(s));
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_IPV6_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_ipv6_exthdr_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the IPv6 extension header matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_IPV6_EXTHDR_MATCH(test_flow[s], TEST_IPV6_EXTHDRS(s));
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_IPV6_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_ipv6_exthdr_w_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the IPv6 extension header matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_IPV6_EXTHDR_W_MATCH(test_flow[s], TEST_IPV6_EXTHDRS(s), exthdrmask);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_IPV6_PREREQUISITE(test_flow[s]);
  }
}
