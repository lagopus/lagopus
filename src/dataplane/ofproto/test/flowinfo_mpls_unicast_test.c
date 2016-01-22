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

#define ETHER_TYPE_MPLS_UT ETHER_TYPE_MPLS

#include "flowinfo_mpls_test.c"


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
test_flowinfo_mpls_unicast_label_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();
  /* The ptree should be clean. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TEST_ASSERT_FLOWINFO_NOLABEL(flowinfo, TEST_LABEL(s), MPLS_LABEL_BITLEN,
                                 __func__);
  }

  /* Add MPLS label matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_MPLS_LABEL_MATCH(test_flow[s], TEST_LABEL(s));
  }

  /* Run the sideeffect-prone scenario. */
  TEST_SCENARIO_FLOWINFO_SEP(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_MPLS_UT_PREREQUISITE(test_flow[s]);
  }
}

/*
 * The TC and BOS tests do not add any label matches; skip the label
 * assertions.
 */
#undef TEST_ASSERT_FLOWINFO_HASLABEL
#undef TEST_ASSERT_FLOWINFO_NOLABEL
#define TEST_ASSERT_FLOWINFO_HASLABEL(_fl, _label, _l, _msg)	\
  do {								\
  } while (0)
#define TEST_ASSERT_FLOWINFO_NOLABEL(_fl, _label, _l, _msg)	\
  do {								\
  } while (0)

/*
 * The MPLS non-label matches are stored in the leaf basic flowinfo.
 * del -> 2*add -> 3*del is hence sufficient.
 */

void
test_flowinfo_mpls_unicast_tc_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add MPLS TC matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_MPLS_TC_MATCH(test_flow[s], TEST_TC(s));
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_MPLS_UT_PREREQUISITE(test_flow[s]);
  }
}

void
test_flowinfo_mpls_unicast_bos_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add MPLS BOS matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_MPLS_BOS_MATCH(test_flow[s], TEST_BOS(s));
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_MPLS_UT_PREREQUISITE(test_flow[s]);
  }
}
