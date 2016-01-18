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


/* XXX */
#define MPLS_LABEL_BITLEN (20)

/*
 * MPLS uses two Ethertypes; ETHER_TYPE_MPLS for unicast and
 * ETHER_TYPE_MPLS_MCAST for multicast.  We define ETHER_TYPE_MPLS_UT
 * to be one of them outside this source so that both of the Ethernet
 * types share the test cases.
 */
#if !defined(ETHER_TYPE_MPLS_UT)
#error ETHER_TYPE_MPLS_UT not defined.
#elseif (ETHER_TYPE_MPLS != ETHER_TYPE_MPLS_UT) && (ETHER_TYPE_MPLS_MCAST != ETHER_TYPE_MPLS_UT)
#error ETHER_TYPE_MPLS_UT not an expected Ethernet type.
#endif

#if ETHER_TYPE_MPLS == ETHER_TYPE_MPLS_UT
#define FLOW_ADD_MPLS_UT_PREREQUISITE(_fl)	FLOW_ADD_MPLS_PREREQUISITE(_fl)
#define TEST_ASSERT_FLOW_MATCH_MPLS_UT_PREREQUISITE(_fl)	TEST_ASSERT_FLOW_MATCH_MPLS_PREREQUISITE(_fl)
#else /* ETHER_TYPE_MPLS_MCAST == ETHER_TYPE_MPLS_UT */
#define FLOW_ADD_MPLS_UT_PREREQUISITE(_fl)	FLOW_ADD_MMPLS_PREREQUISITE(_fl)
#define TEST_ASSERT_FLOW_MATCH_MPLS_UT_PREREQUISITE(_fl)	TEST_ASSERT_FLOW_MATCH_MMPLS_PREREQUISITE(_fl)
#endif /* ETHER_TYPE_MPLS_UT */


/* Compute a MPLS label, TC and BOS value. */
#define TEST_LABEL(_i)	((uint32_t)((_i) + 1))
#define TEST_TC(_i)	((uint8_t)((_i) + 1))
#define TEST_BOS(_i)	(((uint8_t)((_i) + 1)) % 2)

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
      TEST_ASSERT_FLOW_MATCH_MPLS_UT_PREREQUISITE(test_flow[_s]);	\
    }									\
  } while (0)

/* Positively assert flow addition. */
#define TEST_ASSERT_FLOWINFO_ADDFLOW_OK(_fl, _bi, _ei, _flnum, _msg)	\
  do {									\
    size_t _s;								\
    for (_s = (_bi); _s < (_ei); _s++)					\
      TEST_ASSERT_FLOWINFO_ADD_OK((_fl), test_flow[_s], (_msg));	\
    TEST_ASSERT_FLOWINFO_FLOW_NUM((_fl), (_flnum), (_msg));		\
    if (NULL != (_fl)->misc->next[ETHER_TYPE_MPLS_UT])			\
      for (_s = (_bi); _s < (_ei); _s++)				\
        TEST_ASSERT_FLOWINFO_HASLABEL((_fl)->misc->next[ETHER_TYPE_MPLS_UT], TEST_LABEL(_s), MPLS_LABEL_BITLEN, (_msg)); \
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
    if (NULL != (_fl)->misc->next[ETHER_TYPE_MPLS_UT])			\
      for (_s = (_bi); _s < (_ei); _s++)				\
        TEST_ASSERT_FLOWINFO_HASLABEL((_fl)->misc->next[ETHER_TYPE_MPLS_UT], TEST_LABEL(_s), MPLS_LABEL_BITLEN, (_msg)); \
    else {								\
      char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
      \
      snprintf(__buf, sizeof(__buf), "%s, positive flow deletion", (_msg)); \
      TEST_ASSERT_EQUAL_INT_MESSAGE((_flnum), 0, __buf);		\
    }									\
  } while (0)

/* Negatively assert flow deletion. */
#define TEST_ASSERT_FLOWINFO_DELFLOW_NG(_fl, _bi, _ei, _flnum, _msg)	\
  do {									\
    size_t _s;								\
    for (_s = (_bi); _s < (_ei); _s++)					\
      TEST_ASSERT_FLOWINFO_DEL_NG((_fl), test_flow[_s], (_msg));	\
    TEST_ASSERT_FLOWINFO_FLOW_NUM((_fl), (_flnum), (_msg));		\
    if (NULL != (_fl)->misc->next[ETHER_TYPE_MPLS_UT])			\
      for (_s = (_bi); _s < (_ei); _s++)				\
        TEST_ASSERT_FLOWINFO_HASLABEL((_fl)->misc->next[ETHER_TYPE_MPLS_UT], TEST_LABEL(_s), MPLS_LABEL_BITLEN, (_msg)); \
    else {								\
      char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
      \
      snprintf(__buf, sizeof(__buf), "%s, negative flow deletion", (_msg)); \
      TEST_ASSERT_EQUAL_INT_MESSAGE((_flnum), 0, __buf);		\
    }									\
  } while (0)

/* Assert flow numbers. */
#define TEST_ASSERT_FLOWINFO_FLOW_NUM(_fl, _flnum, _msg)		\
  do {									\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl), (_flnum), (_msg));		\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc, (_flnum), (_msg));		\
    if (NULL != (_fl)->misc->next[ETHER_TYPE_MPLS_UT])			\
      TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc->next[ETHER_TYPE_MPLS_UT], (_flnum), (_msg)); \
    else {								\
      char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
      \
      snprintf(__buf, sizeof(__buf), "%s, flow count", (_msg));		\
      TEST_ASSERT_EQUAL_INT_MESSAGE((_flnum), 0, __buf);		\
    }									\
  } while (0)

/* Assert the existence of an MPLS label in a ptree. */
#define TEST_ASSERT_FLOWINFO_HASLABEL(_fl, _label, _l, _msg)		\
  do {									\
    uint32_t _mpls_lse;							\
    char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(__buf, sizeof(__buf), "%s, has MPLS label", (_msg));	\
    SET_MPLS_LSE(_mpls_lse, _label, 0, 0, 0);				\
  } while (0)

/* Assert the non-existence of an MPLS label in a ptree. */
#define TEST_ASSERT_FLOWINFO_NOLABEL(_fl, _label, _l, _msg)		\
  do {									\
    uint32_t _mpls_lse;							\
    char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(__buf, sizeof(__buf), "%s, no MPLS label", (_msg));	\
    SET_MPLS_LSE(_mpls_lse, _label, 0, 0, 0);				\
  } while (0)


void
setUp(void) {
  size_t s;

  /* Make the root flowinfo. */
  TEST_ASSERT_NULL(flowinfo);
  flowinfo = new_flowinfo_vlan_vid();
  TEST_ASSERT_NOT_NULL(flowinfo);

  TEST_ASSERT_FLOWINFO_FLOW_NUM(flowinfo, 0, __func__);

  /*
   * Make the test flows.
   *
   * MPLS matches have prerequisite.
   */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TEST_ASSERT_NULL(test_flow[s]);
    test_flow[s] = allocate_test_flow(10 * sizeof(struct match));
    TEST_ASSERT_NOT_NULL(test_flow[s]);
    test_flow[s]->priority = (int)s;
    FLOW_ADD_MPLS_UT_PREREQUISITE(test_flow[s]);
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
