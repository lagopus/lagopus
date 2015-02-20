/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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

#include <netinet/icmp6.h>

#include "lagopus/dpmgr.h"
#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "match.h"
#include "pktbuf.h"
#include "packet.h"
#include "lagopus/dataplane.h"
#include "lagopus/flowinfo.h"
#include "lagopus/ptree.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"
#include "datapath_test_match.h"
#include "datapath_test_match_macros.h"
#include "flowinfo_test.h"


#define FLOW_ADD_IPV6_ND_UT_PREREQUISITE(_fl)   \
  FLOW_ADD_IPV6_ND_NA_PREREQUISITE((_fl))
#define TEST_ASSERT_FLOW_MATCH_IPV6_ND_UT_PREREQUISITE(_fl)     \
  TEST_ASSERT_FLOW_MATCH_IPV6_ND_NA_PREREQUISITE((_fl))


#include "flowinfo_ipv6_nd_test.c"


/*
 * The IPv6 ND field matches are stored in the leaf basic flowinfo.
 * del -> 2*add -> 3*del is hence sufficient.
 */

void
test_flowinfo_ipv6_nd_na_target_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add the IPv6 ND NS target address match. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_IPV6_ND_TARGET_MATCH(test_flow[s], &ipv6tgt[s]);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_IPV6_ND_UT_PREREQUISITE(test_flow[s]);
  }
}


void
test_flowinfo_ipv6_nd_na_tll_adddel(void) {
  size_t s;
  uint8_t addr[OFP_ETH_ALEN];

  TEST_ASSERT_OBJECTS();

  /* Add the IPv6 ND NA target link-layer address match. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    OS_MEMCPY(addr, mactgt, sizeof(addr));
    addr[sizeof(addr) - 1] = TEST_ETH_ADDR_LSB(s);
    FLOW_ADD_IPV6_ND_TLL_MATCH(test_flow[s], addr);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Reset the matches.  Mind the prerequisite. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
    FLOW_ADD_IPV6_ND_UT_PREREQUISITE(test_flow[s]);
  }
}
