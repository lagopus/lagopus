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


/* Compute a VLAN VID. */
#define TEST_VID(_i)	((uint16_t)(OFPVID_PRESENT | (((_i) << 8) + 1)))

/* Compute a VLAN PCP. */
#define TEST_PCP(_i)	((uint8_t)((_i) + 1))

/* Test VLAN VID mask. */
#define TEST_VID_MASK	(0xff0)


/*
 * Assert the test objects.
 *
 * The test flows must not have any matches.
 */
#define TEST_ASSERT_OBJECTS()						\
  do {									\
    size_t _s, _c;							\
    TEST_ASSERT_NOT_NULL(flowinfo);					\
    for (_s = 0; _s < ARRAY_LEN(test_flow); _s++) {			\
      TEST_ASSERT_NOT_NULL(test_flow[_s]);				\
      _c = 0;								\
      TAILQ_COUNT(_c, struct match, &test_flow[_s]->match_list, entry); \
      TEST_ASSERT_EQUAL_INT(0, _c);					\
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
    size_t _s;								\
    for (_s = (_bi); _s < (_ei); _s++)					\
      TEST_ASSERT_FLOWINFO_DEL_NG((_fl), test_flow[_s], (_msg));	\
    TEST_ASSERT_FLOWINFO_FLOW_NUM((_fl), (_flnum), (_msg));		\
  } while (0)

/* Assert flow numbers. */
#define TEST_ASSERT_FLOWINFO_FLOW_NUM(_fl, _flnum, _msg)	\
  do {								\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl), (_flnum), (_msg));	\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc, 0, (_msg));		\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc->misc, 0, (_msg));	\
  } while (0)

void
setUp(void) {
  size_t s;

  /* Make the root flowinfo. */
  TEST_ASSERT_NULL(flowinfo);
  flowinfo = new_flowinfo_vlan_vid();
  TEST_ASSERT_NOT_NULL(flowinfo);

  TEST_ASSERT_FLOWINFO_FLOW_NUM(flowinfo, 0, __func__);

  /* Make the test flows. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TEST_ASSERT_NULL(test_flow[s]);
    test_flow[s] = allocate_test_flow(10 * sizeof(struct match));
    TEST_ASSERT_NOT_NULL(test_flow[s]);
    test_flow[s]->priority = (int)s;
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
 * The test case performs 2*add -> 3*del -> add -> del because the
 * VLAN flowinfo does not remove a node from the ptree upon deletion.
 * We hence have to cover adding both from scratch and to the ptree in
 * use.
 *
 * This also covers double addition/deletion and non-existing key
 * deletion.
 */

void
test_flowinfo_vlan_vid_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add VLAN VID matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_VLAN_VID_MATCH(test_flow[s], TEST_VID(s));
  }

  /* Run the sideeffect-prone scenario. */
  TEST_SCENARIO_FLOWINFO_SEP(flowinfo);

  /* Delete the matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
  }
}

/*
 * Redefine the assertion macros; a VLAN VID match with a mask is
 * handled by the basic flowinfo rather than VLAN.
 *
 * a VLAN PCP match is also on the same line.
 */
#undef TEST_ASSERT_FLOWINFO_ADDFLOW_OK
#undef TEST_ASSERT_FLOWINFO_DELFLOW_OK
#undef TEST_ASSERT_FLOWINFO_DELFLOW_NG
#undef TEST_ASSERT_FLOWINFO_FLOW_NUM

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
    size_t _s;								\
    for (_s = (_bi); _s < (_ei); _s++)					\
      TEST_ASSERT_FLOWINFO_DEL_NG((_fl), test_flow[_s], (_msg));	\
    TEST_ASSERT_FLOWINFO_FLOW_NUM((_fl), (_flnum), (_msg));		\
  } while (0)

/* Assert flow numbers. */
#define TEST_ASSERT_FLOWINFO_FLOW_NUM(_fl, _flnum, _msg)		\
  do {									\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl), (_flnum), (_msg));		\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc, (_flnum), (_msg));		\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc->misc, (_flnum), (_msg));	\
  } while (0)

void
test_flowinfo_vlan_vid_w_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add VLAN VID matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_VLAN_VID_W_MATCH(test_flow[s], TEST_VID(s), TEST_VID_MASK);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Delete the matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
  }
}

void
test_flowinfo_vlan_pcp_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the VLAN PCP matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_VLAN_PCP_PREREQUISITE(test_flow[s]);
    FLOW_ADD_VLAN_PCP_MATCH(test_flow[s], TEST_PCP(s));
    TEST_ASSERT_FLOW_MATCH_VLAN_PCP_PREREQUISITE(test_flow[s]);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Delete the matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
  }
}
