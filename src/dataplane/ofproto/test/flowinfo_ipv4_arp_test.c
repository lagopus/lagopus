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

#include "lagopus_apis.h"
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


/* Compute an ARP operation. */
#define TEST_ARP_OP(_i)	((uint16_t)((_i) + 1))

/* Compute the LSB octet of an IPv4 address. */
#define TEST_IPV4_ADDR_LSB(_i)	((uint8_t)((_i) + 1))

/* Compute the LSB octet of a MAC address. */
#define TEST_ETH_ADDR_LSB(_i)	((uint8_t)((_i) + 1))


/* Test addresses. */
static const char ipv4srcstr[] = "10.1.123.0";
static const char ipv4tgtstr[] = "10.2.123.0";
static const char ipv4maskstr[] = "255.0.0.0";
static struct in_addr ipv4src[ARRAY_LEN(test_flow)];
static struct in_addr ipv4tgt[ARRAY_LEN(test_flow)];
static struct in_addr ipv4mask[ARRAY_LEN(test_flow)];
static const uint8_t macsrc[OFP_ETH_ALEN] = { 0x12, 0xfe, 0xed, 0xbe, 0xef, 0x00 };
static const uint8_t mactgt[OFP_ETH_ALEN] = { 0x12, 0xfa, 0xce, 0xbe, 0xad, 0x00 };
static const uint8_t macmask[OFP_ETH_ALEN] = { 0xff, 0xff, 0xff, 0x00, 0x00, 0x00 };


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
      TEST_ASSERT_FLOW_MATCH_ARP_PREREQUISITE(test_flow[_s]);		\
    }									\
  } while (0)

/* Positively assert flow addition. */
#define TEST_ASSERT_FLOWINFO_ADDFLOW_OK(_fl, _bi, _ei, _flnum, _msg)	\
  do {									\
    size_t _s;								\
    for (_s = (_bi); _s < (_ei); _s++)					\
      TEST_ASSERT_FLOWINFO_ADD_OK((_fl), test_flow[_s], (_msg));	\
    TEST_ASSERT_FLOWINFO_FLOW_NUM((_fl), (_flnum), (_msg));		\
  } while (0)

/* Positively assert flow deletion. */
#define TEST_ASSERT_FLOWINFO_DELFLOW_OK(_fl, _bi, _ei, _flnum, _msg)	\
  do {									\
    size_t _s;								\
    for (_s = (_bi); _s < (_ei); _s++)					\
      TEST_ASSERT_FLOWINFO_DEL_OK((_fl), test_flow[_s], (_msg));	\
    TEST_ASSERT_FLOWINFO_FLOW_NUM((_fl), (_flnum), (_msg));		\
  } while (0)

/* Negatively assert flow deletion. */
#define TEST_ASSERT_FLOWINFO_DELFLOW_NG(_fl, _bi, _ei, _flnum, _msg)	\
  do {									\
    size_t __s;								\
    for (__s = (_bi); __s < (_ei); __s++)				\
      TEST_ASSERT_FLOWINFO_DEL_NG((_fl), test_flow[__s], (_msg));	\
    TEST_ASSERT_FLOWINFO_FLOW_NUM((_fl), (_flnum), (_msg));		\
  } while (0)

/* Assert flow numbers. */
#define TEST_ASSERT_FLOWINFO_FLOW_NUM(_fl, _flnum, _msg)		\
  do {									\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl), (_flnum), (_msg));		\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc, (_flnum), (_msg));		\
    if (NULL != (_fl)->misc->next[ETHERTYPE_ARP])			\
      TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc->next[ETHERTYPE_ARP], (_flnum), (_msg)); \
    else {								\
      char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
      \
      snprintf(__buf, sizeof(__buf), "%s, flow count", (_msg));		\
      TEST_ASSERT_EQUAL_INT_MESSAGE((_flnum), 0, __buf);		\
    }									\
  } while (0)


void
setUp(void) {
  size_t s;
  lagopus_ip_address_t *addr;
  struct sockaddr_in *sin;
  uint8_t *p;

  /* Make the root flowinfo. */
  TEST_ASSERT_NULL(flowinfo);
  flowinfo = new_flowinfo_vlan_vid();
  TEST_ASSERT_NOT_NULL(flowinfo);

  TEST_ASSERT_FLOWINFO_FLOW_NUM(flowinfo, 0, __func__);

  /*
   * Make the test flows.
   *
   * ARP matches have the prerequisite.
   */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TEST_ASSERT_NULL(test_flow[s]);
    test_flow[s] = allocate_test_flow(10 * sizeof(struct match));
    TEST_ASSERT_NOT_NULL(test_flow[s]);
    test_flow[s]->priority = (int)s;
    FLOW_ADD_ARP_PREREQUISITE(test_flow[s]);
  }

  /* Make the test addresses. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    lagopus_ip_address_create(ipv4srcstr, true, &addr);
    sin = NULL;
    lagopus_ip_address_sockaddr_get(addr, &sin);
    lagopus_ip_address_destroy(addr);
    p = (uint8_t *)&sin->sin_addr.s_addr;
    p[sizeof(sin->sin_addr.s_addr) - 1] = (uint8_t)(TEST_IPV4_ADDR_LSB(s) & 0xff);
    OS_MEMCPY(&ipv4src[s], &sin->sin_addr, sizeof(ipv4src[s]));

    lagopus_ip_address_create(ipv4tgtstr, true, &addr);
    sin = NULL;
    lagopus_ip_address_sockaddr_get(addr, &sin);
    lagopus_ip_address_destroy(addr);
    p = (uint8_t *)&sin->sin_addr.s_addr;
    p[sizeof(sin->sin_addr.s_addr) - 1] = (uint8_t)(TEST_IPV4_ADDR_LSB(s) & 0xff);
    OS_MEMCPY(&ipv4tgt[s], &sin->sin_addr, sizeof(ipv4tgt[s]));

    lagopus_ip_address_create(ipv4maskstr, true, &addr);
    sin = NULL;
    lagopus_ip_address_sockaddr_get(addr, &sin);
    lagopus_ip_address_destroy(addr);
    OS_MEMCPY(&ipv4mask[s], &sin->sin_addr, sizeof(ipv4mask[s]));
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
 * The ARP field matches are stored in the leaf basic flowinfo.  del
 * -> 2*add -> 3*del is hence sufficient.
 */

void
test_flowinfo_arp_op_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the ARP operation matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_ARP_OP_MATCH(test_flow[s], TEST_ARP_OP(s));
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_ARP_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_arp_spa_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the ARP IPv4 source address matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_ARP_SPA_MATCH(test_flow[s], &ipv4src[s]);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_ARP_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_arp_spa_w_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the ARP IPv4 source address matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_ARP_SPA_W_MATCH(test_flow[s], &ipv4src[s], ipv4mask);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_ARP_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_arp_tpa_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the ARP IPv4 target address matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_ARP_TPA_MATCH(test_flow[s], &ipv4tgt[s]);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_ARP_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_arp_tpa_w_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the ARP IPv4 target address matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_ARP_TPA_W_MATCH(test_flow[s], &ipv4tgt[s], ipv4mask);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_ARP_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_arp_sha_adddel(void) {
  size_t s;
  uint8_t addr[OFP_ETH_ALEN];

  TEST_ASSERT_OBJECTS();

  /* Add the ARP MAC source address matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    OS_MEMCPY(addr, macsrc, sizeof(addr));
    addr[sizeof(addr) - 1] = TEST_ETH_ADDR_LSB(s);
    FLOW_ADD_ARP_SHA_MATCH(test_flow[s], addr);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_ARP_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_arp_sha_w_adddel(void) {
  size_t s;
  uint8_t addr[OFP_ETH_ALEN];

  TEST_ASSERT_OBJECTS();

  /* Add the ARP MAC source address matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    OS_MEMCPY(addr, macsrc, sizeof(addr));
    addr[sizeof(addr) - 1] = TEST_ETH_ADDR_LSB(s);
    FLOW_ADD_ARP_SHA_W_MATCH(test_flow[s], addr, macmask);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_ARP_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_arp_tha_adddel(void) {
  size_t s;
  uint8_t addr[OFP_ETH_ALEN];

  TEST_ASSERT_OBJECTS();

  /* Add the ARP MAC target address matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    OS_MEMCPY(addr, mactgt, sizeof(addr));
    addr[sizeof(addr) - 1] = TEST_ETH_ADDR_LSB(s);
    FLOW_ADD_ARP_THA_MATCH(test_flow[s], addr);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_ARP_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_arp_tha_w_adddel(void) {
  size_t s;
  uint8_t addr[OFP_ETH_ALEN];

  TEST_ASSERT_OBJECTS();

  /* Add the ARP MAC target address matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    OS_MEMCPY(addr, mactgt, sizeof(addr));
    addr[sizeof(addr) - 1] = TEST_ETH_ADDR_LSB(s);
    FLOW_ADD_ARP_THA_W_MATCH(test_flow[s], addr, macmask);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_ARP_PREREQUISITE(test_flow[s]);
  }
}
