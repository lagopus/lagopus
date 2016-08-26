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
#include "lagopus/dp_apis.h"
#include "lagopus/interface.h"
#include "lagopus/datastore/bridge.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"


/*
 * Test port number.
 * (extra: for the negative deletion tests only)
 */
#define TEST_PORT_NUM		(3)
#define TEST_PORT_EXTRA_NUM	(1)

/* Test DPID. */
#define TEST_DPID(_i)	((uint64_t)((_i) + dpid_base))

/* Shift the test bridge index for the controller overwriting test. */
#define TEST_SHIFT_BRIDGE_INDEX(_i)	(((_i) + 1) % TEST_PORT_NUM)

/* Test port numbers. */
#define TEST_PORT_OFPNO(_i)	((uint32_t)((_i) + 1))
#define TEST_PORT_IFINDEX(_i)	((uint32_t)(_i + 100))


/*
 * Global variables.
 */
static struct bridge *bridge[TEST_PORT_NUM + TEST_PORT_EXTRA_NUM];
static struct flowdb *flowdb[TEST_PORT_NUM + TEST_PORT_EXTRA_NUM];
static const char bridge_name_format[] = "br%d";
static char *bridge_name[TEST_PORT_NUM + TEST_PORT_EXTRA_NUM];
static const char port_name_format[] = "Port%d";
static char *port_name[TEST_PORT_NUM + TEST_PORT_EXTRA_NUM];
static const char ifname_format[] = "Interface%d";
static char *ifname[TEST_PORT_NUM + TEST_PORT_EXTRA_NUM];
static uint64_t dpid_base;


/* Make the assertion message string for each step. */
#define TEST_SCENARIO_MSG_STEP(_buf, _step)		\
  do {							\
    snprintf((_buf), sizeof(_buf), "Step %d", (_step));	\
  } while (0)

/* Positively assert the port creation. */
#define TEST_ASSERT_PORT_ADD_OK(_b, _e, _msg)                     \
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    datastore_interface_info_t ifinfo;                                  \
    struct port *_port;                                                 \
    \
    snprintf(_buf, sizeof(_buf), "%s, add ports", (_msg));		\
    memset(&ifinfo, 0, sizeof(ifinfo));                                 \
    \
    for (size_t _s = (_b); _s < (_e); _s++) {				\
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dp_port_create(port_name[_s]), _buf); \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dp_interface_create(ifname[_s]), _buf); \
      ifinfo.eth.port_number = TEST_PORT_IFINDEX(_s);                   \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dp_interface_info_set(ifname[_s], &ifinfo), _buf); \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dp_port_interface_set(port_name[_s], ifname[_s]), _buf); \
      TEST_ASSERT_NOT_NULL_MESSAGE((_port = dp_port_lookup(0, TEST_PORT_IFINDEX(_s))), _buf); \
    }                                                                   \
  } while (0);

/* Negatively assert the port creation. */
#define TEST_ASSERT_PORT_ADD_NG(_b, _e, _pi, _msg)		\
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(_buf, sizeof(_buf), "%s, negatively add ports", (_msg));		\
    \
    for (size_t _s = (_b); _s < (_e); _s++)				\
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK != dp_port_create(port_name[_s]), _buf); \
  } while (0);

/* Positively assert the port deletion. */
#define TEST_ASSERT_PORT_DELETE_OK(_b, _e, _msg)			\
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(_buf, sizeof(_buf), "%s, delete ports", (_msg));		\
    \
    for (size_t _s = (_b); _s < (_e); _s++)					\
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dp_port_destroy(port_name[_s]), _buf); \
  } while (0);

/* Assert the port count in the system. */
#define TEST_ASSERT_PORT_COUNT(_count, _msg)		\
  do {								\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];			\
    \
    snprintf(_buf, sizeof(_buf), "%s, count ports", (_msg));	\
    \
    TEST_ASSERT_EQUAL_INT_MESSAGE((_count), dp_port_count(), (_msg)); \
  } while (0);

/* Positively assert the ports in the system. */
#define TEST_ASSERT_PORT_FIND_OK(_b, _e, _msg)			\
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(_buf, sizeof(_buf), "%s, find ports", (_msg));		\
    \
    for (size_t _s = (_b); _s < (_e); _s++) {				\
      TEST_ASSERT_TRUE_MESSAGE(NULL != dp_port_lookup(0, TEST_PORT_IFINDEX(_s)), _buf); \
    }									\
  } while (0);

/* Negatively assert the ports in the system. */
#define TEST_ASSERT_PORT_FIND_NG(_b, _e, _msg)			\
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(_buf, sizeof(_buf), "%s, negatively find ports", (_msg));	\
    \
    for (size_t _s = (_b); _s < (_e); _s++) {				\
      TEST_ASSERT_TRUE_MESSAGE(NULL == dp_port_lookup(0, TEST_PORT_IFINDEX(_s)), _buf); \
    }									\
  } while (0);

/* Positively assert the port adddition to a bridge. */
#define TEST_ASSERT_BRIDGE_ADD_PORT_OK(_b, _e, _msg)		\
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(_buf, sizeof(_buf), "%s, add ports to the bridge", (_msg)); \
    \
    for (size_t _s = (_b); _s < (_e); _s++)				\
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dp_bridge_port_set(bridge_name[_s], port_name[_s], TEST_PORT_OFPNO(_s)), _buf); \
  } while (0);

/* Negatively assert the port adddition to a bridge. */
#define TEST_ASSERT_BRIDGE_ADD_PORT_NG(_b, _e, _msg)		\
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(_buf, sizeof(_buf), "%s, negatively add ports to the bridge", (_msg)); \
    \
    for (size_t _s = (_b); _s < (_e); _s++)				\
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK != dp_bridge_port_set(bridge_name[_s], port_name[_s], TEST_PORT_OFPNO(_s)), _buf); \
  } while (0);

/* Negatively assert the port adddition to a shifted bridge. */
#define TEST_ASSERT_SHIFTEDBRIDGE_ADD_PORT_NG(_b, _e, _msg)	\
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(_buf, sizeof(_buf), "%s, negatively add ports to the shifted bridge", (_msg)); \
    \
    for (size_t _s = (_b); _s < (_e); _s++)				\
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK != dp_bridge_port_set(bridge_name[TEST_SHIFT_BRIDGE_INDEX(_s)], port_name[_s], TEST_PORT_OFPNO(_s)), _buf); \
  } while (0);

/* Positively assert the port deletion from a bridge. */
#define TEST_ASSERT_BRIDGE_DELETE_PORT_OK(_b, _e, _msg)		\
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    for (size_t _s = (_b); _s < (_e); _s++) {				\
      snprintf(_buf, sizeof(_buf), "%s, delete ports from the bridge (%d)", (_msg),(_s)); \
      \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dp_bridge_port_unset_num(bridge_name[_s], TEST_PORT_OFPNO(_s)), _buf); \
    }                                                                   \
  } while (0);

/* Negatively assert the port deletion from a bridge. */
#define TEST_ASSERT_BRIDGE_DELETE_PORT_NG(_b, _e, _msg)		\
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(_buf, sizeof(_buf), "%s, negatively delete ports from the bridge", (_msg)); \
    \
    for (size_t _s = (_b); _s < (_e); _s++)				\
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK != dp_bridge_port_unset_num( bridge_name[_s], TEST_PORT_OFPNO(_s)), _buf); \
  } while (0);

/* Negatively assert the port deletion from a shifted bridge. */
#define TEST_ASSERT_SHIFTEDBRIDGE_DELETE_PORT_NG(_b, _e, _msg)	\
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(_buf, sizeof(_buf), "%s, negatively delete ports from the shifted bridge", (_msg)); \
    \
    for (size_t _s = (_b); _s < (_e); _s++)				\
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK != dp_bridge_port_unset_num(bridge_name[TEST_SHIFT_BRIDGE_INDEX(_s)], TEST_PORT_OFPNO(_s)), _buf); \
  } while (0);

/* Assert the port count in a bridge. */
#define TEST_ASSERT_BRIDGE_COUNT_PORT(_b, _e, _count, _msg)	\
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(_buf, sizeof(_buf), "%s, count ports in a bridge", (_msg)); \
    \
    for (size_t _s = (_b); _s < (_e); _s++)				\
      TEST_ASSERT_EQUAL_INT_MESSAGE((_count), dp_bridge_port_count(bridge[_s]->name), (_msg)); \
  } while (0);

/* Positively assert the ports in a bridge. */
#define TEST_ASSERT_BRIDGE_FIND_PORT_OK(_b, _e, _msg)		\
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(_buf, sizeof(_buf), "%s, find ports in a bridge", (_msg));	\
    \
    for (size_t _s = (_b); _s < (_e); _s++) {				\
      TEST_ASSERT_TRUE_MESSAGE(NULL != port_lookup(&bridge[_s]->ports, TEST_PORT_OFPNO(_s)), _buf); \
    }									\
  } while (0);

/* Negatively assert the ports in a bridge. */
#define TEST_ASSERT_BRIDGE_FIND_PORT_NG(_b, _e, _msg)		\
  do {									\
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(_buf, sizeof(_buf), "%s, find ports in a bridge", (_msg));	\
    \
    for (size_t _s = (_b); _s < (_e); _s++) {				\
      TEST_ASSERT_TRUE_MESSAGE(NULL == port_lookup(&bridge[_s]->ports, TEST_PORT_OFPNO(_s)), _buf); \
    }									\
  } while (0);


void
setUp(void) {
  size_t s;
  char buf[128];

  dpid_base = 0;
  printf("dpid_base = 0x%llx (%llu)\n", (unsigned long long)dpid_base,
         (unsigned long long)dpid_base);

  for (s = 0; s < ARRAY_LEN(bridge); s++) {
    TEST_ASSERT_NULL(bridge_name[s]);
    TEST_ASSERT_NULL(bridge[s]);
    TEST_ASSERT_NULL(flowdb[s]);
  }
  for (s = 0; s < ARRAY_LEN(port_name); s++) {
    TEST_ASSERT_NULL(port_name[s]);
  }
  for (s = 0; s < ARRAY_LEN(ifname); s++) {
    TEST_ASSERT_NULL(ifname[s]);
  }

  /* Make the bridge names and port configurations. */
  for (s = 0; s < ARRAY_LEN(bridge); s++) {
    snprintf(buf, sizeof(buf), bridge_name_format, (int)s);
    bridge_name[s] = strdup(buf);
    snprintf(buf, sizeof(buf), port_name_format, TEST_PORT_IFINDEX(s));
    port_name[s] = strdup(buf);
    snprintf(buf, sizeof(buf), ifname_format, TEST_PORT_IFINDEX(s));
    ifname[s] = strdup(buf);
  }

  TEST_ASSERT_EQUAL(dp_api_init(), LAGOPUS_RESULT_OK);
  for (s = 0; s < ARRAY_LEN(bridge); s++) {
    datastore_bridge_info_t info;
    memset(&info, 0, sizeof(info));
    info.fail_mode = DATASTORE_BRIDGE_FAIL_MODE_SECURE;
    info.dpid = TEST_DPID(s);
    TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == dp_bridge_create(bridge_name[s],
                     &info));
    TEST_ASSERT_NOT_NULL(bridge[s] = dp_bridge_lookup(bridge_name[s]));
    TEST_ASSERT_NOT_NULL(flowdb[s] = bridge[s]->flowdb);
  }
}

void
tearDown(void) {
  size_t s;

  for (s = 0; s < ARRAY_LEN(bridge); s++) {
    TEST_ASSERT_NOT_NULL(bridge[s]);
    TEST_ASSERT_NOT_NULL(flowdb[s]);
    TEST_ASSERT_NOT_NULL(bridge_name[s]);
  }

  for (s = 0; s < ARRAY_LEN(bridge); s++) {
    TEST_ASSERT_NULL(dp_port_lookup(0, TEST_PORT_IFINDEX(s)));
    TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == dp_bridge_destroy(bridge_name[s]));
    // Free the bridge names.
    free(port_name[s]);
    free(bridge_name[s]);
    flowdb[s] = NULL;
    bridge[s] = NULL;
    bridge_name[s] = NULL;
    port_name[s] = NULL;
  }
}


/*
 * Step 0: Assert no ports on the bridge.
 * Step 1: Add the ports to the system.
 * Step 2: Doubly add the ports to system. (negative)
 * Step 3: Add the ports to the bridge.
 * Step 4: Doubly add the ports to the bridge. (negative)
 * Step 5: Add the ports to the shifted bridge. (negative).
 * Step 6: Delete the ports from the system. (negative)
 * Step 7: Delete the ports from the shifted bridge. (negative)
 * Step 8: Delete the ports from the bridge.
 * Step 9: Doubly delete the ports from the bridge. (negative)
 * Step 10: Delete the ports from the system.
 * Step 11: Doubly delete the ports from the system. (negative)
 */
void
test_flowdb_ports(void) {
#ifdef HYBRID
  lagopus_thread_t *thdptr = NULL;
  dp_tapio_thread_init(NULL, NULL, NULL, &thdptr);
#endif /* HYBRID */
  char buf[TEST_ASSERT_MESSAGE_BUFSIZE];

  /* Step 0. */
  TEST_SCENARIO_MSG_STEP(buf, 0);

  TEST_ASSERT_PORT_COUNT(0, buf);
  TEST_ASSERT_PORT_FIND_NG(0, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(0, TEST_PORT_EXTRA_NUM, 0, buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_NG(0, TEST_PORT_EXTRA_NUM, buf);

  /* Step 1. */
  TEST_SCENARIO_MSG_STEP(buf, 1);
  TEST_ASSERT_PORT_ADD_OK(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(0, TEST_PORT_EXTRA_NUM, 0, buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_NG(0, TEST_PORT_EXTRA_NUM, buf);

  /* Step 2. */
  TEST_SCENARIO_MSG_STEP(buf, 2);
  TEST_ASSERT_PORT_ADD_NG(0, TEST_PORT_NUM, nports, buf);

  TEST_ASSERT_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(0, TEST_PORT_EXTRA_NUM, 0, buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_NG(0, TEST_PORT_EXTRA_NUM, buf);

  /* Step 3. */
  TEST_SCENARIO_MSG_STEP(buf, 3);
  TEST_ASSERT_BRIDGE_ADD_PORT_OK(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(0, TEST_PORT_NUM, 1, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, 0,
                                      buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);

  /* Step 4. */
  TEST_SCENARIO_MSG_STEP(buf, 4);
  TEST_ASSERT_BRIDGE_ADD_PORT_NG(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(0, TEST_PORT_NUM, 1, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, 0,
                                      buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);

  /* Step 5. */
  TEST_SCENARIO_MSG_STEP(buf, 5);
  TEST_ASSERT_SHIFTEDBRIDGE_ADD_PORT_NG(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(0, TEST_PORT_NUM, 1, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, 0,
                                      buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);

  /* Step 6. */
  TEST_SCENARIO_MSG_STEP(buf, 6);

  TEST_ASSERT_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(0, TEST_PORT_NUM, 1, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, 0,
                                      buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);

  /* Step 7. */
  TEST_SCENARIO_MSG_STEP(buf, 7);
  TEST_ASSERT_SHIFTEDBRIDGE_DELETE_PORT_NG(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(0, TEST_PORT_NUM, 1, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, 0,
                                      buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);

  /* Step 8. */
  TEST_SCENARIO_MSG_STEP(buf, 8);
  TEST_ASSERT_BRIDGE_DELETE_PORT_OK(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(0, TEST_PORT_EXTRA_NUM, 0, buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_NG(0, TEST_PORT_EXTRA_NUM, buf);

  /* Step 9. */
  TEST_SCENARIO_MSG_STEP(buf, 9);
  TEST_ASSERT_BRIDGE_DELETE_PORT_NG(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(0, TEST_PORT_EXTRA_NUM, 0, buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_NG(0, TEST_PORT_EXTRA_NUM, buf);

  /* Step 10. */
  TEST_SCENARIO_MSG_STEP(buf, 10);
  TEST_ASSERT_PORT_DELETE_OK(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_PORT_COUNT(0, buf);
  TEST_ASSERT_PORT_FIND_NG(0, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(0, TEST_PORT_EXTRA_NUM, 0, buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_NG(0, TEST_PORT_EXTRA_NUM, buf);

  /* Step 11. */
  TEST_SCENARIO_MSG_STEP(buf, 11);

  TEST_ASSERT_PORT_COUNT(0, buf);
  TEST_ASSERT_PORT_FIND_NG(0, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_BRIDGE_COUNT_PORT(0, TEST_PORT_EXTRA_NUM, 0, buf);
  TEST_ASSERT_BRIDGE_FIND_PORT_NG(0, TEST_PORT_EXTRA_NUM, buf);

#ifdef HYBRID
  dp_tapio_thread_fini();
#endif /* HYBRID */
}
