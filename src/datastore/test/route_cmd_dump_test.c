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
#include "../route_cmd_internal.h"
#include "../bridge_cmd_internal.h"

#include "../datastore_apis.h"
#include "../datastore_internal.h"
#include "../channel_cmd_internal.h"
#include "../controller_cmd_internal.h"
#include "../interface_cmd_internal.h"
#include "../port_cmd_internal.h"

static lagopus_dstring_t ds = NULL;
static lagopus_hashmap_t tbl = NULL;
static datastore_interp_t interp = NULL;
static bool destroy = false;
static const char bridge_name[] = ":br0";
static bool is_bridge_first = true;

void
setUp(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;

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
test_route_cmd_dump(void) {
#ifdef HYBRID
  lagopus_result_t ret;
  char *str = NULL;
  struct in_addr dst, gate;
  struct bridge *bridge;
  int prefixlen = 16;
  int ifindex = 1;
  uint8_t scope = 0;
  uint8_t mac_addr[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
  const char test_str[] =
    "{\"name\":\":br0\",\n"
    "\"route_ipv4\":[{\"dest\":\"192.168.0.0\\/16\",\n"
    "\"gate\":\"0.0.0.0\\/16\",\n"
    "\"ifindex\":1}]}";

  dst.s_addr = inet_addr("192.168.0.0");
  gate.s_addr = inet_addr("0.0.0.0");

  bridge = dp_bridge_lookup(bridge_name);
  route_entry_add(&bridge->rib.ribs[0].route_table,
                  &dst, prefixlen, &gate, ifindex, scope, mac_addr);

  /* show route cmd. */
  (void) lagopus_dstring_clear(&ds);
  ret = dump_bridge_route(bridge_name, is_bridge_first, &ds);
  TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);
  TEST_DSTRING(ret, &ds, str, test_str, true);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_cmd_dump_no_route(void) {
#ifdef HYBRID
  lagopus_result_t ret;
  char *str = NULL;
  const char test_str[] =
    "{\"name\":\":br0\",\n"
    "\"route_ipv4\":[]}";

  /* show route cmd. */
  (void) lagopus_dstring_clear(&ds);
  ret = dump_bridge_route(bridge_name, is_bridge_first, &ds);
  TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);
  TEST_DSTRING(ret, &ds, str, test_str, true);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_cmd_dump_not_found(void) {
#ifdef HYBRID
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char test_str[] =
    "{\"name\":\":hoge\",\n"
    "\"route_ipv4\":[]}";

  /* show route cmd. */
  (void) lagopus_dstring_clear(&ds);
  ret = dump_bridge_route(":hoge", is_bridge_first, &ds);
  TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_NOT_FOUND);
  TEST_DSTRING(ret, &ds, str, test_str, true);
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
