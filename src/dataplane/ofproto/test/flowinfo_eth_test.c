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


/* Compute an Ethernet type. */
#define TEST_ETH_TYPE(_i)	((uint16_t)((_i) + 1))


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
    for (_s = (_bi); _s < (_ei); _s++)					\
      TEST_ASSERT_FLOWINFO_HASETHTYPE((_fl)->misc, TEST_ETH_TYPE(_s), (_msg)); \
  } while (0)

/* Positively assert flow deletion. */
#define TEST_ASSERT_FLOWINFO_DELFLOW_OK(_fl, _bi, _ei, _flnum, _msg)	\
  do {									\
    size_t _s;								\
    for (_s = (_bi); _s < (_ei); _s++)					\
      TEST_ASSERT_FLOWINFO_DEL_OK((_fl), test_flow[_s], (_msg));	\
    TEST_ASSERT_FLOWINFO_FLOW_NUM((_fl), (_flnum), (_msg));		\
    for (_s = (_bi); _s < (_ei); _s++)					\
      TEST_ASSERT_FLOWINFO_HASETHTYPE((_fl)->misc, TEST_ETH_TYPE(_s), (_msg)); \
  } while (0)

/* Negatively assert flow deletion without the Ethernet type check. */
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
    _TEST_ASSERT_FLOWINFO_DELFLOW_NG(_fl, _bi, _ei, _flnum, (_msg));	\
    for (_s = (_bi); _s < (_ei); _s++)					\
      TEST_ASSERT_FLOWINFO_HASETHTYPE((_fl)->misc, TEST_ETH_TYPE(_s), (_msg)); \
  } while (0)

/* Negatively assert flow deletion and non-existence of an Ethernet type. */
#undef TEST_ASSERT_FLOWINFO_DELFLOW_NG_CLEAN
#define TEST_ASSERT_FLOWINFO_DELFLOW_NG_CLEAN(_fl, _bi, _ei, _flnum, _msg) \
  do {									\
    size_t _s;								\
    _TEST_ASSERT_FLOWINFO_DELFLOW_NG(_fl, _bi, _ei, _flnum, (_msg));	\
    for (_s = (_bi); _s < (_ei); _s++)					\
      TEST_ASSERT_FLOWINFO_NOETHTYPE((_fl)->misc, TEST_ETH_TYPE(_s), (_msg)); \
  } while (0)

/* Assert flow numbers. */
#define TEST_ASSERT_FLOWINFO_FLOW_NUM(_fl, _flnum, _msg)	\
  do {								\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl), (_flnum), (_msg));	\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc, (_flnum), (_msg));	\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc->misc, 0, (_msg));	\
  } while (0)

/* Assert the existence of an Ethernet type in the next table. */
#define TEST_ASSERT_FLOWINFO_HASETHTYPE(_fl, _type, _msg)		\
  do {									\
    char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(__buf, sizeof(__buf), "%s, has Ethernet type", (_msg));	\
    TEST_ASSERT_NOT_NULL_MESSAGE((_fl)->next[_type], __buf);		\
  } while (0)

/* Assert the non-existence of an Ethernet type in the next table. */
#define TEST_ASSERT_FLOWINFO_NOETHTYPE(_fl, _type, _msg)		\
  do {									\
    char __buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(__buf, sizeof(__buf), "%s, no Ethernet type", (_msg));	\
    TEST_ASSERT_NULL_MESSAGE((_fl)->next[_type], __buf);		\
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
 * Ethernet flowinfo does not remove a next type entry from the table
 * upon deletion.  We hence have to cover adding both from scratch and
 * to the ptree in use.
 *
 * This also covers double addition/deletion and non-existing key
 * deletion.
 */

void
test_flowinfo_eth_type_adddel(void) {
  size_t s;

  TEST_ASSERT_OBJECTS();

  /* Add Ethernet type matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    FLOW_ADD_ETH_TYPE_MATCH(test_flow[s], TEST_ETH_TYPE(s));
  }

  /* Run the sideeffect-prone scenario. */
  TEST_SCENARIO_FLOWINFO_SEP(flowinfo);

  /* Delete the matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
  }
}

/*
 * Redefine the assertion macros for the address tests.
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

/* Negatively assert flow deletion and non-existence of an Ethernet type. */
#undef TEST_ASSERT_FLOWINFO_DELFLOW_NG_CLEAN
#define TEST_ASSERT_FLOWINFO_DELFLOW_NG_CLEAN(_fl, _bi, _ei, _flnum, _msg) \
  TEST_ASSERT_FLOWINFO_DELFLOW_NG((_fl), (_bi), (_ei), (_flnum), (_msg))

/* Assert flow numbers. */
#define TEST_ASSERT_FLOWINFO_FLOW_NUM(_fl, _flnum, _msg)		\
  do {									\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl), (_flnum), (_msg));		\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc, (_flnum), (_msg));		\
    TEST_ASSERT_FLOWINFO_NFLOW((_fl)->misc->misc, (_flnum), (_msg));	\
  } while (0)


/* MAC addresses. */
static const uint8_t srcaddr[OFP_ETH_ALEN] = { 0x12, 0xfe, 0xed, 0xbe, 0xef, 0x00 };
static const uint8_t dstaddr[OFP_ETH_ALEN] = { 0x12, 0xfa, 0xce, 0xbe, 0xad, 0x00 };
static const uint8_t maskaddr[OFP_ETH_ALEN] = { 0xff, 0xff, 0xff, 0x00, 0x00, 0x00 };


void
test_flowinfo_eth_dst_adddel(void) {
  size_t s;
  uint8_t addr[OFP_ETH_ALEN];

  TEST_ASSERT_OBJECTS();

  /* Add Ethernet destination matches. */
  OS_MEMCPY(addr, srcaddr, sizeof(addr));
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    addr[OFP_ETH_ALEN - 1] = (uint8_t)s;
    FLOW_ADD_ETH_DST_MATCH(test_flow[s], addr);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Delete the matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
  }
}

void
test_flowinfo_eth_src_adddel(void) {
  size_t s;
  uint8_t addr[OFP_ETH_ALEN];

  TEST_ASSERT_OBJECTS();

  /* Add Ethernet destination matches. */
  OS_MEMCPY(addr, srcaddr, sizeof(addr));
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    addr[OFP_ETH_ALEN - 1] = (uint8_t)s;
    FLOW_ADD_ETH_SRC_MATCH(test_flow[s], addr);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Delete the matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
  }
}

void
test_flowinfo_eth_dst_w_adddel(void) {
  size_t s;
  uint8_t addr[OFP_ETH_ALEN];

  TEST_ASSERT_OBJECTS();

  /* Add Ethernet destination matches. */
  OS_MEMCPY(addr, srcaddr, sizeof(addr));
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    addr[OFP_ETH_ALEN - 1] = (uint8_t)s;
    FLOW_ADD_ETH_DST_W_MATCH(test_flow[s], addr, maskaddr);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Delete the matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
  }
}

void
test_flowinfo_eth_src_w_adddel(void) {
  size_t s;
  uint8_t addr[OFP_ETH_ALEN];

  TEST_ASSERT_OBJECTS();

  /* Add Ethernet destination matches. */
  OS_MEMCPY(addr, srcaddr, sizeof(addr));
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    addr[OFP_ETH_ALEN - 1] = (uint8_t)s;
    FLOW_ADD_ETH_SRC_W_MATCH(test_flow[s], addr, maskaddr);
  }

  /* Run the sideeffect-free scenario. */
  TEST_SCENARIO_FLOWINFO_SEF(flowinfo);

  /* Delete the matches. */
  for (s = 0; s < ARRAY_LEN(test_flow); s++) {
    TAILQ_INIT(&test_flow[s]->match_list);
  }
}
