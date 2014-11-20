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

#include "lagopus/dpmgr.h"
#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"


/*
 * Test port number.
 * (extra: for the negative deletion tests only)
 */
#define TEST_PORT_NUM           (3)
#define TEST_PORT_EXTRA_NUM     (1)

/* Test DPID. */
#define TEST_DPID(_i)   ((uint64_t)((_i) + dpid_base))

/* Shift the test bridge index for the controller overwriting test. */
#define TEST_SHIFT_BRIDGE_INDEX(_i)     (((_i) + 1) % TEST_PORT_NUM)

/* Test port numbers. */
#define TEST_PORT_OFPNO(_i)     ((uint32_t)((_i) + 1))
#define TEST_PORT_IFINDEX(_i)   ((uint32_t)(_i + 100))


/*
 * Global variables.
 */
static struct dpmgr *dpmgr;
static struct bridge *bridge[TEST_PORT_NUM + TEST_PORT_EXTRA_NUM];
static struct flowdb *flowdb[TEST_PORT_NUM + TEST_PORT_EXTRA_NUM];
static const char bridge_name_format[] = "br%d";
static char *bridge_name[TEST_PORT_NUM + TEST_PORT_EXTRA_NUM];
struct port nports[TEST_PORT_NUM + TEST_PORT_EXTRA_NUM];
static uint64_t dpid_base;


/* XXX */
struct port *port_lookup_number(struct vector *v, uint32_t port_no);


/* Make the assertion message string for each step. */
#define TEST_SCENARIO_MSG_STEP(_buf, _step)             \
  do {                                                  \
    snprintf((_buf), sizeof(_buf), "Step %d", (_step)); \
  } while (0)

/* Positively assert the port creation. */
#define TEST_ASSERT_DPMGR_PORT_ADD_OK(_b, _e, _pi, _msg)                \
  do {                                                                  \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    struct port *_port;                                                 \
    \
    snprintf(_buf, sizeof(_buf), "%s, add ports", (_msg));              \
    \
    for (size_t _s = (_b); _s < (_e); _s++) {                           \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dpmgr_port_add(dpmgr, &(_pi)[_s]), _buf); \
      TEST_ASSERT_NOT_NULL_MESSAGE((_port = port_lookup(dpmgr->ports, TEST_PORT_IFINDEX(_s))), _buf); \
      _port->ofp_port.hw_addr[0] = 0xff;                                \
    }                                                                   \
  } while (0);

/* Negatively assert the port creation. */
#define TEST_ASSERT_DPMGR_PORT_ADD_NG(_b, _e, _pi, _msg)                \
  do {                                                                  \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, negatively add ports", (_msg));           \
    \
    for (size_t _s = (_b); _s < (_e); _s++)                             \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK != dpmgr_port_add(dpmgr, &(_pi)[_s]), _buf); \
  } while (0);

/* Positively assert the port deletion. */
#define TEST_ASSERT_DPMGR_PORT_DELETE_OK(_b, _e, _msg)                  \
  do {                                                                  \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, delete ports", (_msg));           \
    \
    for (size_t _s = (_b); _s < (_e); _s++)                                     \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dpmgr_port_delete(dpmgr, TEST_PORT_IFINDEX(_s)), _buf); \
  } while (0);

/* Negatively assert the port deletion. */
#define TEST_ASSERT_DPMGR_PORT_DELETE_NG(_b, _e, _msg)                  \
  do {                                                                  \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, negatively delete ports", (_msg));                \
    \
    for (size_t _s = (_b); _s < (_e); _s++)                                     \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK != dpmgr_port_delete(dpmgr, TEST_PORT_IFINDEX(_s)), _buf); \
  } while (0);

/* Assert the port count in dpmgr. */
#define TEST_ASSERT_DPMGR_PORT_COUNT(_count, _msg)              \
  do {                                                          \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                     \
    \
    snprintf(_buf, sizeof(_buf), "%s, count ports", (_msg));    \
    \
    TEST_ASSERT_EQUAL_INT_MESSAGE((_count), num_ports(dpmgr->ports), (_msg)); \
  } while (0);

/* Positively assert the ports in dpmgr. */
#define TEST_ASSERT_DPMGR_PORT_FIND_OK(_b, _e, _msg)                    \
  do {                                                                  \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, find ports", (_msg));             \
    \
    for (size_t _s = (_b); _s < (_e); _s++) {                           \
      TEST_ASSERT_TRUE_MESSAGE(NULL != port_lookup(dpmgr->ports, TEST_PORT_IFINDEX(_s)), _buf); \
      TEST_ASSERT_TRUE_MESSAGE(NULL != port_lookup_number(dpmgr->ports, TEST_PORT_OFPNO(_s)), _buf); \
    }                                                                   \
  } while (0);

/* Negatively assert the ports in dpmgr. */
#define TEST_ASSERT_DPMGR_PORT_FIND_NG(_b, _e, _msg)                    \
  do {                                                                  \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, negatively find ports", (_msg));  \
    \
    for (size_t _s = (_b); _s < (_e); _s++) {                           \
      TEST_ASSERT_TRUE_MESSAGE(NULL == port_lookup(dpmgr->ports, TEST_PORT_IFINDEX(_s)), _buf); \
      TEST_ASSERT_TRUE_MESSAGE(NULL == port_lookup_number(dpmgr->ports, TEST_PORT_OFPNO(_s)), _buf); \
    }                                                                   \
  } while (0);

/* Positively assert the port adddition to a bridge. */
#define TEST_ASSERT_DPMGR_BRIDGE_ADD_PORT_OK(_b, _e, _msg)              \
  do {                                                                  \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, add ports to the bridge", (_msg)); \
    \
    for (size_t _s = (_b); _s < (_e); _s++)                             \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dpmgr_bridge_port_add(dpmgr, bridge_name[_s], TEST_PORT_IFINDEX(_s), TEST_PORT_OFPNO(_s)), _buf); \
  } while (0);

/* Negatively assert the port adddition to a bridge. */
#define TEST_ASSERT_DPMGR_BRIDGE_ADD_PORT_NG(_b, _e, _msg)              \
  do {                                                                  \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, negatively add ports to the bridge", (_msg)); \
    \
    for (size_t _s = (_b); _s < (_e); _s++)                             \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK != dpmgr_bridge_port_add(dpmgr, bridge_name[_s], TEST_PORT_IFINDEX(_s), TEST_PORT_OFPNO(_s)), _buf); \
  } while (0);

/* Negatively assert the port adddition to a shifted bridge. */
#define TEST_ASSERT_DPMGR_SHIFTEDBRIDGE_ADD_PORT_NG(_b, _e, _msg)       \
  do {                                                                  \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, negatively add ports to the shifted bridge", (_msg)); \
    \
    for (size_t _s = (_b); _s < (_e); _s++)                             \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK != dpmgr_bridge_port_add(dpmgr, bridge_name[TEST_SHIFT_BRIDGE_INDEX(_s)], TEST_PORT_IFINDEX(_s), TEST_PORT_OFPNO(_s)), _buf); \
  } while (0);

/* Positively assert the port deletion from a bridge. */
#define TEST_ASSERT_DPMGR_BRIDGE_DELETE_PORT_OK(_b, _e, _msg)           \
  do {                                                                  \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, delete ports from the bridge", (_msg)); \
    \
    for (size_t _s = (_b); _s < (_e); _s++)                             \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dpmgr_bridge_port_delete(dpmgr, bridge_name[_s], TEST_PORT_IFINDEX(_s)), _buf); \
  } while (0);

/* Negatively assert the port deletion from a bridge. */
#define TEST_ASSERT_DPMGR_BRIDGE_DELETE_PORT_NG(_b, _e, _msg)           \
  do {                                                                  \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, negatively delete ports from the bridge", (_msg)); \
    \
    for (size_t _s = (_b); _s < (_e); _s++)                             \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK != dpmgr_bridge_port_delete(dpmgr, bridge_name[_s], TEST_PORT_IFINDEX(_s)), _buf); \
  } while (0);

/* Negatively assert the port deletion from a shifted bridge. */
#define TEST_ASSERT_DPMGR_SHIFTEDBRIDGE_DELETE_PORT_NG(_b, _e, _msg)    \
  do {                                                                  \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, negatively delete ports from the shifted bridge", (_msg)); \
    \
    for (size_t _s = (_b); _s < (_e); _s++)                             \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK != dpmgr_bridge_port_delete(dpmgr, bridge_name[TEST_SHIFT_BRIDGE_INDEX(_s)], TEST_PORT_IFINDEX(_s)), _buf); \
  } while (0);

/* Assert the port count in a bridge. */
#define TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(_b, _e, _count, _msg)       \
  do {                                                                  \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, count ports in a bridge", (_msg)); \
    \
    for (size_t _s = (_b); _s < (_e); _s++)                             \
      TEST_ASSERT_EQUAL_INT_MESSAGE((_count), num_ports(bridge[_s]->ports), (_msg)); \
  } while (0);

/* Positively assert the ports in a bridge. */
#define TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_OK(_b, _e, _msg)             \
  do {                                                                  \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, find ports in a bridge", (_msg)); \
    \
    for (size_t _s = (_b); _s < (_e); _s++) {                           \
      TEST_ASSERT_TRUE_MESSAGE(NULL != port_lookup(bridge[_s]->ports, TEST_PORT_OFPNO(_s)), _buf); \
      TEST_ASSERT_TRUE_MESSAGE(NULL != port_lookup_number(bridge[_s]->ports, TEST_PORT_OFPNO(_s)), _buf); \
    }                                                                   \
  } while (0);

/* Negatively assert the ports in a bridge. */
#define TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_NG(_b, _e, _msg)             \
  do {                                                                  \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, find ports in a bridge", (_msg)); \
    \
    for (size_t _s = (_b); _s < (_e); _s++) {                           \
      TEST_ASSERT_TRUE_MESSAGE(NULL == port_lookup(bridge[_s]->ports, TEST_PORT_OFPNO(_s)), _buf); \
      TEST_ASSERT_TRUE_MESSAGE(NULL == port_lookup_number(bridge[_s]->ports, TEST_PORT_OFPNO(_s)), _buf); \
    }                                                                   \
  } while (0);


void
setUp(void) {
  size_t s;
  char buf[128];

  dpid_base = dpmgr_dpid_generate();
  printf("dpid_base = 0x%llx (%llu)\n", (unsigned long long)dpid_base,
         (unsigned long long)dpid_base);

  TEST_ASSERT_NULL(dpmgr);
  for (s = 0; s < ARRAY_LEN(bridge); s++) {
    TEST_ASSERT_NULL(bridge_name[s]);
    TEST_ASSERT_NULL(bridge[s]);
    TEST_ASSERT_NULL(flowdb[s]);
  }

  /* Make the bridge names and port configurations. */
  memset(nports, 0, sizeof(nports));
  for (s = 0; s < ARRAY_LEN(bridge); s++) {
    snprintf(buf, sizeof(buf), bridge_name_format, (int)s);
    bridge_name[s] = strdup(buf);
    nports[s].type = LAGOPUS_PORT_TYPE_PHYSICAL;
    nports[s].ofp_port.port_no = TEST_PORT_OFPNO(s);
    nports[s].ifindex = TEST_PORT_IFINDEX(s);
  }

  TEST_ASSERT_NOT_NULL(dpmgr = dpmgr_alloc());
  for (s = 0; s < ARRAY_LEN(bridge); s++) {
    TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == dpmgr_bridge_add(dpmgr, bridge_name[s],
                     TEST_DPID(s)));
    TEST_ASSERT_NOT_NULL(bridge[s] = dpmgr_bridge_lookup(dpmgr, bridge_name[s]));
    TEST_ASSERT_NOT_NULL(flowdb[s] = bridge[s]->flowdb);
  }
}

void
tearDown(void) {
  size_t s;

  TEST_ASSERT_NOT_NULL(dpmgr);
  for (s = 0; s < ARRAY_LEN(bridge); s++) {
    TEST_ASSERT_NOT_NULL(bridge[s]);
    TEST_ASSERT_NOT_NULL(flowdb[s]);
    TEST_ASSERT_NOT_NULL(bridge_name[s]);
  }

  for (s = 0; s < ARRAY_LEN(bridge); s++) {
    TEST_ASSERT_NULL(port_lookup(dpmgr->ports, TEST_PORT_IFINDEX(s)));
    TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == dpmgr_bridge_delete(dpmgr,
                     bridge_name[s]));
    // Free the bridge names.
    free(bridge_name[s]);
    flowdb[s] = NULL;
    bridge[s] = NULL;
    bridge_name[s] = NULL;
  }

  dpmgr_free(dpmgr);
  dpmgr = NULL;
}


/*
 * Step 0: Assert no ports on the bridge.
 * Step 1: Add the ports to dpmgr.
 * Step 2: Doubly add the ports to dpmgr. (negative)
 * Step 3: Add the ports to the bridge.
 * Step 4: Doubly add the ports to the bridge. (negative)
 * Step 5: Add the ports to the shifted bridge. (negative).
 * Step 6: Delete the ports from dpmgr. (negative)
 * Step 7: Delete the ports from the shifted bridge. (negative)
 * Step 8: Delete the ports from the bridge.
 * Step 9: Doubly delete the ports from the bridge. (negative)
 * Step 10: Delete the ports from dpmgr.
 * Step 11: Doubly delete the ports from dpmgr. (negative)
 */
void
test_flowdb_dpmgr_ports(void) {
  char buf[TEST_ASSERT_MESSAGE_BUFSIZE];

  /* Step 0. */
  TEST_SCENARIO_MSG_STEP(buf, 0);

  TEST_ASSERT_DPMGR_PORT_COUNT(0, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_NG(0, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(0, TEST_PORT_EXTRA_NUM, 0, buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_NG(0, TEST_PORT_EXTRA_NUM, buf);

  /* Step 1. */
  TEST_SCENARIO_MSG_STEP(buf, 1);
  TEST_ASSERT_DPMGR_PORT_ADD_OK(0, TEST_PORT_NUM, nports, buf);

  TEST_ASSERT_DPMGR_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(0, TEST_PORT_EXTRA_NUM, 0, buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_NG(0, TEST_PORT_EXTRA_NUM, buf);

  /* Step 2. */
  TEST_SCENARIO_MSG_STEP(buf, 2);
  TEST_ASSERT_DPMGR_PORT_ADD_NG(0, TEST_PORT_NUM, nports, buf);

  TEST_ASSERT_DPMGR_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(0, TEST_PORT_EXTRA_NUM, 0, buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_NG(0, TEST_PORT_EXTRA_NUM, buf);

  /* Step 3. */
  TEST_SCENARIO_MSG_STEP(buf, 3);
  TEST_ASSERT_DPMGR_BRIDGE_ADD_PORT_OK(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_DPMGR_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(0, TEST_PORT_NUM, 1, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, 0,
                                      buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM,
                                        buf);

  /* Step 4. */
  TEST_SCENARIO_MSG_STEP(buf, 4);
  TEST_ASSERT_DPMGR_BRIDGE_ADD_PORT_NG(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_DPMGR_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(0, TEST_PORT_NUM, 1, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, 0,
                                      buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM,
                                        buf);

  /* Step 5. */
  TEST_SCENARIO_MSG_STEP(buf, 5);
  TEST_ASSERT_DPMGR_SHIFTEDBRIDGE_ADD_PORT_NG(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_DPMGR_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(0, TEST_PORT_NUM, 1, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, 0,
                                      buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM,
                                        buf);

  /* Step 6. */
  TEST_SCENARIO_MSG_STEP(buf, 6);
  TEST_ASSERT_DPMGR_PORT_DELETE_NG(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_DPMGR_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(0, TEST_PORT_NUM, 1, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, 0,
                                      buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM,
                                        buf);

  /* Step 7. */
  TEST_SCENARIO_MSG_STEP(buf, 7);
  TEST_ASSERT_DPMGR_SHIFTEDBRIDGE_DELETE_PORT_NG(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_DPMGR_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(0, TEST_PORT_NUM, 1, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, 0,
                                      buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM,
                                        buf);

  /* Step 8. */
  TEST_SCENARIO_MSG_STEP(buf, 8);
  TEST_ASSERT_DPMGR_BRIDGE_DELETE_PORT_OK(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_DPMGR_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(0, TEST_PORT_EXTRA_NUM, 0, buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_NG(0, TEST_PORT_EXTRA_NUM, buf);

  /* Step 9. */
  TEST_SCENARIO_MSG_STEP(buf, 9);
  TEST_ASSERT_DPMGR_BRIDGE_DELETE_PORT_NG(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_DPMGR_PORT_COUNT(TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_OK(0, TEST_PORT_NUM, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_NG(TEST_PORT_NUM, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(0, TEST_PORT_EXTRA_NUM, 0, buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_NG(0, TEST_PORT_EXTRA_NUM, buf);

  /* Step 10. */
  TEST_SCENARIO_MSG_STEP(buf, 10);
  TEST_ASSERT_DPMGR_PORT_DELETE_OK(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_DPMGR_PORT_COUNT(0, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_NG(0, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(0, TEST_PORT_EXTRA_NUM, 0, buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_NG(0, TEST_PORT_EXTRA_NUM, buf);

  /* Step 11. */
  TEST_SCENARIO_MSG_STEP(buf, 11);
  TEST_ASSERT_DPMGR_PORT_DELETE_NG(0, TEST_PORT_NUM, buf);

  TEST_ASSERT_DPMGR_PORT_COUNT(0, buf);
  TEST_ASSERT_DPMGR_PORT_FIND_NG(0, TEST_PORT_EXTRA_NUM, buf);
  TEST_ASSERT_DPMGR_BRIDGE_COUNT_PORT(0, TEST_PORT_EXTRA_NUM, 0, buf);
  TEST_ASSERT_DPMGR_BRIDGE_FIND_PORT_NG(0, TEST_PORT_EXTRA_NUM, buf);
}
