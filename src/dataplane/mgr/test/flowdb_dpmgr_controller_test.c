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
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"


/*
 * Test bridge number.
 * (extra: for the negative deletion tests only)
 */
#define TEST_BRIDGE_NUM         (3)
#define TEST_BRIDGE_EXTRA_NUM   (1)

/* Test DPID. */
#define TEST_DPID(_i)   ((uint64_t)((_i) + dpid_base))

/* Shift the test bridge index for the controller overwriting test. */
#define TEST_SHIFT_BRIDGE_INDEX(_i)     (((_i) + 1) % TEST_BRIDGE_NUM)


/*
 * Global variables.
 */
static struct dpmgr *dpmgr;
static struct bridge *bridge[TEST_BRIDGE_NUM + TEST_BRIDGE_EXTRA_NUM];
static struct flowdb *flowdb[TEST_BRIDGE_NUM + TEST_BRIDGE_EXTRA_NUM];
static const char bridge_name_format[] = "br%d";
static char *bridge_name[TEST_BRIDGE_NUM + TEST_BRIDGE_EXTRA_NUM];
static const char controller_address_format[] = "10.123.45.%d";
static char *controller_address[TEST_BRIDGE_NUM + TEST_BRIDGE_EXTRA_NUM];
static uint64_t dpid_base;


/* Make the assertion message string for each step. */
#define TEST_SCENARIO_MSG_STEP(_buf, _step)             \
  do {                                                  \
    snprintf((_buf), sizeof(_buf), "Step %d", (_step)); \
  } while (0)

/* Positively assert the controller addition. */
#define TEST_ASSERT_DPMGR_CONTROLLER_ADD_OK(_b, _e, _msg)               \
  do {                                                                  \
    size_t _s;                                                          \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, add controller", (_msg));         \
    \
    for (_s = (_b); _s < (_e); _s++)                                    \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dpmgr_controller_add(dpmgr, bridge_name[_s], controller_address[_s]), _buf); \
  } while (0);

/*
 * Positively assert the controller addition with the shifted
 * controller indices.
 */
#define TEST_ASSERT_DPMGR_CONTROLLER_SHIFTEDADD_OK(_b, _e, _msg)        \
  do {                                                                  \
    size_t _s;                                                          \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, add shifted controller", (_msg)); \
    \
    for (_s = (_b); _s < (_e); _s++)                                    \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dpmgr_controller_add(dpmgr, bridge_name[_s], controller_address[TEST_SHIFT_BRIDGE_INDEX(_s)]), _buf); \
  } while (0);

/* Positively assert the controller deletion. */
#define TEST_ASSERT_DPMGR_CONTROLLER_DELETE_OK(_b, _e, _msg)            \
  do {                                                                  \
    size_t _s;                                                          \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, delete controller", (_msg));              \
    \
    for (_s = (_b); _s < (_e); _s++)                                    \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dpmgr_controller_delete(dpmgr, bridge_name[_s], controller_address[_s]), _buf); \
  } while (0);

/* Negatively assert the controller deletion. */
#define TEST_ASSERT_DPMGR_CONTROLLER_DELETE_NG(_b, _e, _msg)            \
  do {                                                                  \
    size_t _s;                                                          \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, negatively delete controller", (_msg));           \
    \
    for (_s = (_b); _s < (_e); _s++)                                    \
      TEST_ASSERT_FALSE_MESSAGE(LAGOPUS_RESULT_OK == dpmgr_controller_delete(dpmgr, bridge_name[_s], controller_address[_s]), _buf); \
  } while (0);

/* Assert the controllers on the bridges. */
#define TEST_ASSERT_DPMGR_HASCONTROLLER(_b, _e, _msg)                   \
  do {                                                                  \
    size_t _s;                                                          \
    uint64_t dpid;                                                      \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, has controller", (_msg));         \
    \
    for (_s = (_b); _s < (_e); _s++) {                                  \
      TEST_ASSERT_TRUE_MESSAGE(0 == strcmp(controller_address[_s], bridge[_s]->controller_address), _buf); \
      TEST_ASSERT_TRUE_MESSAGE(dpmgr_bridge_lookup_by_controller_address(dpmgr, controller_address[_s]) == bridge[_s], _buf); \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dpmgr_controller_dpid_find(dpmgr, controller_address[_s], &dpid), _buf); \
      TEST_ASSERT_EQUAL_UINT64_MESSAGE(TEST_DPID(_s), dpid, _buf);      \
    }                                                                   \
  } while (0);

/* Assert the controllers on the bridges with the shifted indices. */
#define TEST_ASSERT_DPMGR_HASSHIFTEDCONTROLLER(_b, _e, _msg)            \
  do {                                                                  \
    size_t _s;                                                          \
    uint64_t dpid;                                                      \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, has shifted controller", (_msg));         \
    \
    for (_s = (_b); _s < (_e); _s++) {                                  \
      TEST_ASSERT_TRUE_MESSAGE(0 == strcmp(controller_address[TEST_SHIFT_BRIDGE_INDEX(_s)], bridge[_s]->controller_address), _buf); \
      TEST_ASSERT_TRUE_MESSAGE(dpmgr_bridge_lookup_by_controller_address(dpmgr, controller_address[TEST_SHIFT_BRIDGE_INDEX(_s)]) == bridge[_s], _buf); \
      TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == dpmgr_controller_dpid_find(dpmgr, controller_address[TEST_SHIFT_BRIDGE_INDEX(_s)], &dpid), _buf); \
      /* A DPID is on a bridge; not affected by shifting. */            \
      TEST_ASSERT_EQUAL_UINT64_MESSAGE(TEST_DPID(_s), dpid, _buf);      \
    }                                                                   \
  } while (0);

/* Assert no controllers on the bridges. */
#define TEST_ASSERT_DPMGR_NOCONTROLLER(_b, _e, _msg)                    \
  do {                                                                  \
    size_t _s;                                                          \
    uint64_t dpid;                                                      \
    char _buf[TEST_ASSERT_MESSAGE_BUFSIZE];                             \
    \
    snprintf(_buf, sizeof(_buf), "%s, no controller", (_msg));          \
    \
    for (_s = (_b); _s < (_e); _s++) {                                  \
      TEST_ASSERT_TRUE_MESSAGE(NULL == bridge[_s]->controller_address, _buf); \
      TEST_ASSERT_NULL_MESSAGE(dpmgr_bridge_lookup_by_controller_address(dpmgr, controller_address[_s]), _buf); \
      TEST_ASSERT_FALSE_MESSAGE(LAGOPUS_RESULT_OK == dpmgr_controller_dpid_find(dpmgr, controller_address[_s], &dpid), _buf); \
    }                                                                   \
  } while (0);


/* XXX */
struct bridge *dpmgr_bridge_lookup_by_controller_address(struct dpmgr *dpmgr,
    const char *controller_address);
lagopus_result_t dpmgr_controller_dpid_find(struct dpmgr *dpmgr,
    const char *controller_str, uint64_t *dpidp);


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
    TEST_ASSERT_NULL(controller_address[s]);
    TEST_ASSERT_NULL(bridge[s]);
    TEST_ASSERT_NULL(flowdb[s]);
  }

  /* Make the bridge names and controller addresses. */
  for (s = 0; s < ARRAY_LEN(bridge); s++) {
    snprintf(buf, sizeof(buf), bridge_name_format, (int)s);
    bridge_name[s] = strdup(buf);
    snprintf(buf, sizeof(buf), controller_address_format, (int)(s + 1));
    controller_address[s] = strdup(buf);
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
    TEST_ASSERT_NOT_NULL(controller_address[s]);
  }

  for (s = 0; s < ARRAY_LEN(bridge); s++) {
    TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == dpmgr_bridge_delete(dpmgr,
                     bridge_name[s]));
    // Free the bridge names and controller addresses.
    free(bridge_name[s]);
    free(controller_address[s]);
    flowdb[s] = NULL;
    bridge[s] = NULL;
    bridge_name[s] = NULL;
    controller_address[s] = NULL;
  }

  dpmgr_free(dpmgr);
  dpmgr = NULL;
}

/*
 * Step 0: Assert no controllers.
 * Step 1: Add the controllers with the shifted indices.
 * Step 2: Overwrite the controllers with the untouched indices.
 * Step 3: Delete the controllers.
 * Step 4: Doubly delete the controllers.
 */
void
test_flowdb_dpmgr_controller(void) {
  char buf[TEST_ASSERT_MESSAGE_BUFSIZE];

  /* Step 0. */
  TEST_SCENARIO_MSG_STEP(buf, 0);
  TEST_ASSERT_DPMGR_NOCONTROLLER(0, TEST_BRIDGE_NUM + TEST_BRIDGE_EXTRA_NUM,
                                 buf);

  /* Step 1. */
  TEST_SCENARIO_MSG_STEP(buf, 1);
  TEST_ASSERT_DPMGR_CONTROLLER_SHIFTEDADD_OK(0, TEST_BRIDGE_NUM, buf);
  TEST_ASSERT_DPMGR_HASSHIFTEDCONTROLLER(0, TEST_BRIDGE_NUM, buf);
  TEST_ASSERT_DPMGR_NOCONTROLLER(TEST_BRIDGE_NUM, TEST_BRIDGE_EXTRA_NUM, buf);

  /* Step 2. */
  TEST_SCENARIO_MSG_STEP(buf, 2);
  TEST_ASSERT_DPMGR_CONTROLLER_ADD_OK(0, TEST_BRIDGE_NUM, buf);
  TEST_ASSERT_DPMGR_HASCONTROLLER(0, TEST_BRIDGE_NUM, buf);
  TEST_ASSERT_DPMGR_NOCONTROLLER(TEST_BRIDGE_NUM, TEST_BRIDGE_EXTRA_NUM, buf);

  /* Step 3. */
  TEST_SCENARIO_MSG_STEP(buf, 3);
  TEST_ASSERT_DPMGR_CONTROLLER_DELETE_OK(0, TEST_BRIDGE_NUM, buf);
  TEST_ASSERT_DPMGR_NOCONTROLLER(0, TEST_BRIDGE_NUM + TEST_BRIDGE_EXTRA_NUM,
                                 buf);

  /* Step 4. */
  TEST_SCENARIO_MSG_STEP(buf, 4);
  TEST_ASSERT_DPMGR_CONTROLLER_DELETE_NG(0, TEST_BRIDGE_NUM, buf);
  TEST_ASSERT_DPMGR_NOCONTROLLER(0, TEST_BRIDGE_NUM + TEST_BRIDGE_EXTRA_NUM,
                                 buf);
}
