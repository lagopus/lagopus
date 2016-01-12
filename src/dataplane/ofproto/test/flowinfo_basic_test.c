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


/* Compute a port number. */
#define TEST_PORT(_i)	((uint32_t)((_i) + 1))
#define TEST_PHYPORT(_i)	((uint32_t)((_i) + 1))

/* Compute a metadata. */
#define TEST_METADATA(_i)	((uint64_t)((_i) + md))

/* Compute a tunnel ID. */
#define TEST_TUNNELID(_i)	((uint64_t)((_i) + tunnelid))


/* Test metadata. */
static const uint64_t md = 0xbeefcafe12345678UL;
static const uint64_t mdmask = 0xffffffff00000000UL;

/* Test ingress port. */
static const uint32_t testiport = 0x01234567;

/* Test tunnel ID. */
static const uint64_t tunnelid = 0x12345678beefcafeUL;
static const uint64_t tunnelidmask = 0xffffffff00000000UL;


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
#define TEST_ASSERT_FLOWINFO_FLOW_NUM(_fl, _flnum, _msg)		\
  do {									\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl), (_flnum), (_msg));		\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc, (_flnum), (_msg));		\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc->misc, (_flnum), (_msg));	\
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
 * The test cases perform 2*add -> 3*del so that they cover double
 * addition and deletion.
 *
 * The deletion of a non-existing key is also tested.
 */

void
test_flowinfo_port_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add port matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_PORT_MATCH(test_flow[s], TEST_PORT(s));
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Delete the matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
  }
}

void
test_flowinfo_phyport_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the physical port matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_PHYPORT_PREREQUISITE(test_flow[s], testiport);
    FLOW_ADD_PHYPORT_MATCH(test_flow[s], TEST_PHYPORT(s));
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Delete the matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
  }
}

void
test_flowinfo_metadata_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add metadata matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_METADATA_MATCH(test_flow[s], TEST_METADATA(s));
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Delete the matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
  }
}

void
test_flowinfo_metadata_w_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add metadata matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_METADATA_W_MATCH(test_flow[s], TEST_METADATA(s), mdmask);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Delete the matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
  }
}

void
test_flowinfo_tunnelid_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the tunnel ID matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_TUNNELID_MATCH(test_flow[s], TEST_TUNNELID(s));
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Delete the matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
  }
}

void
test_flowinfo_tunnelid_w_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the tunnel ID matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_TUNNELID_W_MATCH(test_flow[s], TEST_TUNNELID(s), mdmask);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Delete the matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
  }
}
