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
#include "lagopus/ethertype.h"
#include "lagopus/flowinfo.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"
#include "datapath_test_match.h"
#include "datapath_test_match_macros.h"
#include "flowinfo_test.h"


FLOWINFO_TEST_DECLARE_DATA;


/* Compute a PBB I-SID. */
#define TEST_PBB_ISID(_i)	((uint32_t)((_i) + 1))

#ifdef notyet
/* Compute a PBB UCA. */
#define TEST_PBB_UCA(_i)	(((uint8_t)((_i) + 1)) % 1)
#endif /* notyet */

/* The test PBB I-SID mask. */
static const uint32_t isidmask = 0x00ffff00;


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
      TEST_ASSERT_FLOW_MATCH_PBB_PREREQUISITE(test_flow[_s]);		\
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
    if (NULL != (_fl)->misc->next[ETHERTYPE_PBB])			\
      TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc->next[ETHERTYPE_PBB], (_flnum), (_msg)); \
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

  /* Make the root flowinfo. */
  TEST_ASSERT_NULL(flowinfo);
  flowinfo = new_flowinfo_vlan_vid();
  TEST_ASSERT_NOT_NULL(flowinfo);

  TEST_ASSERT_FLOWINFO_FLOW_NUM(flowinfo, 0, __func__);

  /*
   * Make the test flows.
   *
   * PBB matches have the prerequisite.
   */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TEST_ASSERT_NULL(test_flow[s]);
    test_flow[s] = allocate_test_flow(10 * sizeof(struct match));
    TEST_ASSERT_NOT_NULL(test_flow[s]);
    test_flow[s]->priority = (int)s;
    FLOW_ADD_PBB_PREREQUISITE(test_flow[s]);
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
 * The PBB I-SID matches are stored in the leaf basic flowinfo.  del
 * -> 2*add -> 3*del is hence sufficient.
 */

void
test_flowinfo_pbb_isid_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the PBB ISID matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_PBB_ISID_MATCH(test_flow[s], TEST_PBB_ISID(s));
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_PBB_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_pbb_isid_w_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the PBB ISID matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_PBB_ISID_W_MATCH(test_flow[s], TEST_PBB_ISID(s), isidmask);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_PBB_PREREQUISITE(test_flow[s]);
  }
}

#ifdef notyet
void
xtest_flowinfo_pbb_uca_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the PBB UCA matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_PBB_UCA_MATCH(test_flow[s], TEST_PBB_UCA(s));
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_PBB_PREREQUISITE(test_flow[s]);
  }
}
#endif /* notyet */
