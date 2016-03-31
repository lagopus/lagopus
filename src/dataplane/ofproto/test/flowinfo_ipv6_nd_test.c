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


FLOWINFO_TEST_DECLARE_DATA;


/* Compute the LSB octet of an IPv4 address. */
#define TEST_IPV6_ADDR_LSB(_i)	((uint8_t)((_i) + 1))

/* Compute the LSB octet of a MAC address. */
#define TEST_ETH_ADDR_LSB(_i)	((uint8_t)((_i) + 1))


/* Test addresses. */
static const char ipv6srcstr[] = "3ffe:1:2:3:4:5:6:0";
static const char ipv6tgtstr[] = "3ffe:7:8:9:a:b:c:0";
static const char ipv6maskstr[] = "ffff:ffff:ffff:ffff::";
static struct in6_addr ipv6src[ARRAY_LEN(test_flow)];
static struct in6_addr ipv6tgt[ARRAY_LEN(test_flow)];
static struct in6_addr ipv6mask[ARRAY_LEN(test_flow)];
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
      TEST_ASSERT_EQUAL_INT(3, _c);					\
      TEST_ASSERT_FLOW_MATCH_IPV6_ND_UT_PREREQUISITE(test_flow[_s]);	\
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
    if ((NULL != (_fl)->misc->next[ETHERTYPE_IPV6]) &&			\
        (NULL != (_fl)->misc->next[ETHERTYPE_IPV6]->next[IPPROTO_ICMPV6])) { \
      TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc->next[ETHERTYPE_IPV6], (_flnum), (_msg)); \
      TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc->next[ETHERTYPE_IPV6]->next[IPPROTO_ICMPV6], (_flnum), (_msg)); \
    } else {								\
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
  struct sockaddr_in6 *sin6;
  uint8_t *p;

  /* Make the root flowinfo. */
  TEST_ASSERT_NULL(flowinfo);
  flowinfo = new_flowinfo_vlan_vid();
  TEST_ASSERT_NOT_NULL(flowinfo);

  TEST_ASSERT_FLOWINFO_FLOW_NUM(flowinfo, 0, __func__);

  /*
   * Make the test flows.
   *
   * ND NS matches have the prerequisite.
   */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TEST_ASSERT_NULL(test_flow[s]);
    test_flow[s] = allocate_test_flow(10 * sizeof(struct match));
    TEST_ASSERT_NOT_NULL(test_flow[s]);
    test_flow[s]->priority = (int)s;
    FLOW_ADD_IPV6_ND_UT_PREREQUISITE(test_flow[s]);
  }

  /* Make the test addresses. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    lagopus_ip_address_create(ipv6srcstr, false, &addr);
    TEST_ASSERT_NOT_NULL(addr);
    sin6 = NULL;
    lagopus_ip_address_sockaddr_get(addr, (struct sockaddr **)&sin6);
    TEST_ASSERT_NOT_NULL(sin6);
    lagopus_ip_address_destroy(addr);
    p = (uint8_t *)&sin6->sin6_addr.s6_addr;
    p[sizeof(sin6->sin6_addr.s6_addr) - 1] = (uint8_t)(TEST_IPV6_ADDR_LSB(s) & 0xff);
    OS_MEMCPY(&ipv6src[s], &sin6->sin6_addr, sizeof(ipv6src[s]));

    lagopus_ip_address_create(ipv6tgtstr, true, &addr);
    TEST_ASSERT_NOT_NULL(addr);
    sin6 = NULL;
    lagopus_ip_address_sockaddr_get(addr, (struct sockaddr **)&sin6);
    TEST_ASSERT_NOT_NULL(sin6);
    lagopus_ip_address_destroy(addr);
    p = (uint8_t *)&sin6->sin6_addr.s6_addr;
    p[sizeof(sin6->sin6_addr.s6_addr) - 1] = (uint8_t)(TEST_IPV6_ADDR_LSB(s) & 0xff);
    OS_MEMCPY(&ipv6tgt[s], &sin6->sin6_addr.s6_addr, sizeof(ipv6tgt[s]));

    lagopus_ip_address_create(ipv6maskstr, true, &addr);
    TEST_ASSERT_NOT_NULL(addr);
    sin6 = NULL;
    lagopus_ip_address_sockaddr_get(addr, (struct sockaddr **)&sin6);
    TEST_ASSERT_NOT_NULL(sin6);
    lagopus_ip_address_destroy(addr);
    OS_MEMCPY(&ipv6mask[s], &sin6->sin6_addr, sizeof(ipv6mask[s]));
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
