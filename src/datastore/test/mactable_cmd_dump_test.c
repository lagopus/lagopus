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
#include "lagopus_apis.h"
#include "cmd_test_utils.h"
#include "../mactable_cmd_internal.h"
#include "../bridge_cmd_internal.h"

#include "../datastore_apis.h"
#include "../datastore_internal.h"
#include "../channel_cmd_internal.h"
#include "../controller_cmd_internal.h"
#include "../interface_cmd_internal.h"
#include "../port_cmd_internal.h"

#ifdef HAVE_DPDK
#include "../../dpdk/build/include/rte_config.h"
#include "../../dpdk/lib/librte_eal/common/include/rte_lcore.h"
#endif /* HAVE_DPDK */

#define TMP_DIR "/tmp"
#define TMP_FILE ".lagopus_mactable_"

static lagopus_dstring_t ds = NULL;
static lagopus_hashmap_t tbl = NULL;
static datastore_interp_t interp = NULL;
static bool destroy = false;
static bool is_init = false;
static const char bridge_name[] = ":br0";
static bool is_bridge_first = true;

void
setUp(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;

#ifdef HAVE_DPDK
  RTE_PER_LCORE(_lcore_id) = 0;
#endif /* HAVE DPDK */

  /* create interp. */
  INTERP_CREATE(ret, NULL, interp, tbl, ds);

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE(ret, &interp, state, &tbl, &ds, str,
                     "br0", "1", "cha1", "c1",
                     "test_if01", "test_port01", "1");
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
tearDown(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY(ret, &interp, state, &tbl, &ds, str,
                      "br0", "cha1", "c1",
                      "test_if01", "test_port01");

  /* destroy interp. */
  INTERP_DESTROY(NULL, interp, tbl, ds, destroy);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_mactable_cmd_dump_01(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char format_str[] =
    "{\"name\":\":br0\",\n"
    "\"mactable\":[{\"num_entries\":1,\n"
    "\"max_entries\":8192,\n"
    "\"ageing_time\":300,\n"
    "\"entries\":[{\"mac_addr\":\"aa:bb:cc:dd:ee:ff\",\n"
    "\"port_no\":1,\n"
    "\"update_time\":%ld,\n"
    "\"address_type\":\"static\"}]}]}";
  char test_str[1000];
  uint8_t eth[6];
  uint32_t port = 1;
  struct timespec now;
  struct bridge *bridge;

  /* add mac entries */
  eth[0] = 0xaa;
  eth[1] = 0xbb;
  eth[2] = 0xcc;
  eth[3] = 0xdd;
  eth[4] = 0xee;
  eth[5] = 0xff;

  /* preparation */
  ret = dp_bridge_mactable_entry_set(bridge_name, eth, port);
  TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);
  now = get_current_time();
  sprintf(test_str, format_str, now.tv_sec);
  bridge = dp_bridge_lookup(bridge_name);
  mactable_update(&bridge->mactable);

  /* test target. */
  (void) lagopus_dstring_clear(&ds);
  ret = dump_bridge_mactable(bridge_name, is_bridge_first, &ds);
  TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);
  TEST_DSTRING(ret, &ds, str, test_str, true);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_mactable_cmd_dump_02(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char format_str[] =
    "{\"name\":\":br0\",\n"
    "\"mactable\":[{\"num_entries\":2,\n"
    "\"max_entries\":8192,\n"
    "\"ageing_time\":300,\n"
    "\"entries\":[{\"mac_addr\":\"1a:2b:3c:4d:5e:6f\",\n"
    "\"port_no\":2,\n"
    "\"update_time\":%ld,\n"
    "\"address_type\":\"static\"},\n"
    "{\"mac_addr\":\"aa:bb:cc:dd:ee:ff\",\n"
    "\"port_no\":1,\n"
    "\"update_time\":%ld,\n"
    "\"address_type\":\"static\"}]}]}";
  char test_str[1000];
  uint8_t eth1[6];
  uint8_t eth2[6];
  uint32_t port1 = 1;
  uint32_t port2 = 2;
  struct timespec now;
  struct bridge *bridge;

  /* add mac entries */
  eth1[0] = 0xaa;
  eth1[1] = 0xbb;
  eth1[2] = 0xcc;
  eth1[3] = 0xdd;
  eth1[4] = 0xee;
  eth1[5] = 0xff;
  eth2[0] = 0x1a;
  eth2[1] = 0x2b;
  eth2[2] = 0x3c;
  eth2[3] = 0x4d;
  eth2[4] = 0x5e;
  eth2[5] = 0x6f;

  /* preparation */
  ret = dp_bridge_mactable_entry_set(bridge_name, eth1, port1);
  TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);
  ret = dp_bridge_mactable_entry_set(bridge_name, eth2, port2);
  TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);
  now = get_current_time();
  sprintf(test_str, format_str, now.tv_sec, now.tv_sec);
  bridge = dp_bridge_lookup(bridge_name);
  mactable_update(&bridge->mactable);

  /* test target. */
  (void) lagopus_dstring_clear(&ds);
  ret = dump_bridge_mactable(bridge_name, is_bridge_first, &ds);
  TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);
  TEST_DSTRING(ret, &ds, str, test_str, true);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_mactable_cmd_dump_no_entry(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char format_str[] =
    "{\"name\":\":br0\",\n"
    "\"mactable\":[{\"num_entries\":0,\n"
    "\"max_entries\":8192,\n"
    "\"ageing_time\":300,\n"
    "\"entries\":[";
  char test_str[1000];

  sprintf(test_str, format_str);

  /* test target. */
  (void) lagopus_dstring_clear(&ds);
  ret = dump_bridge_mactable(bridge_name, is_bridge_first, &ds);
  TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_ANY_FAILURES);
  TEST_DSTRING(ret, &ds, str, test_str, false);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_mactable_cmd_dump_not_found(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char format_str[] =
    "{\"name\":\":hoge\",\n"
    "\"mactable\":[{";
  char test_str[1000];

  uint8_t eth[6];
  uint32_t port = 1;

  /* add mac entries */
  eth[0] = 0xaa;
  eth[1] = 0xbb;
  eth[2] = 0xcc;
  eth[3] = 0xdd;
  eth[4] = 0xee;
  eth[5] = 0xff;

  ret = dp_bridge_mactable_entry_set(bridge_name, eth, port);
  TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);
  sprintf(test_str, format_str);

  /* test target. */
  (void) lagopus_dstring_clear(&ds);
  ret = dump_bridge_mactable(":hoge", is_bridge_first, &ds);
  TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_NOT_FOUND);
  TEST_DSTRING(ret, &ds, str, test_str, false);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_destroy(void) {
#ifdef HYBRID
  destroy = true;
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}
