/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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

#include <sys/queue.h>
#include "unity.h"
#include "lagopus_apis.h"
#include "lagopus/pbuf.h"
#include "lagopus/datastore/bridge.h"
#include "lagopus/bridge.h"
#include "lagopus/dp_apis.h"
#include "openflow13.h"
#include "ofp_band.h"

static struct bridge *bridge;
static const char bridge_name[] = "br0";
static const uint64_t dpid = 12345678;

void
setUp(void) {
  datastore_bridge_info_t info;

  memset(&info, 0, sizeof(info));
  info.dpid = dpid;
  info.fail_mode = DATASTORE_BRIDGE_FAIL_MODE_SECURE;
  TEST_ASSERT_NULL(bridge);
  TEST_ASSERT_EQUAL(dp_api_init(), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_bridge_create(bridge_name, &info), LAGOPUS_RESULT_OK);
  bridge = dp_bridge_lookup(bridge_name);
  TEST_ASSERT_NOT_NULL(bridge);
}

void
tearDown(void) {
  TEST_ASSERT_NOT_NULL(bridge);
  TEST_ASSERT_EQUAL(dp_bridge_destroy(bridge_name), LAGOPUS_RESULT_OK);
  dp_api_fini();
  bridge = NULL;
}

void
test_bridge_fail_mode(void) {
  enum fail_mode mode;
  lagopus_result_t rv;

  rv = bridge_fail_mode_get(bridge, &mode);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = bridge_fail_mode_set(bridge, 12345);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
  rv = bridge_fail_mode_set(bridge, FAIL_SECURE_MODE);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = bridge_fail_mode_set(bridge, FAIL_STANDALONE_MODE);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_bridge_ofp_version(void) {
  uint8_t ver;
  lagopus_result_t rv;

  rv = bridge_ofp_version_get(bridge, &ver);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = bridge_ofp_version_set(bridge, 0);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
  rv = bridge_ofp_version_set(bridge, OPENFLOW_VERSION_1_3);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = bridge_ofp_version_set(bridge, OPENFLOW_VERSION_1_4);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_bridge_table_id_iter(void) {
  dp_bridge_iter_t iter;
  uint8_t ids[] = { 0, 5, 10 };
  uint8_t id;
  int i;
  lagopus_result_t rv;

  iter = NULL;
  TEST_ASSERT_EQUAL(dp_bridge_table_id_iter_create(bridge_name, &iter),
                    LAGOPUS_RESULT_OK);
  TEST_ASSERT_NOT_NULL(iter);
  for (i = 0; i < sizeof(ids) / sizeof(ids[0]); i++) {
    TEST_ASSERT_NOT_NULL(table_get(bridge->flowdb, ids[i]));
  }
  i = 0;
  while ((rv = dp_bridge_table_id_iter_get(iter, &id)) == LAGOPUS_RESULT_OK) {
    TEST_ASSERT_NOT_EQUAL(i, sizeof(ids) / sizeof(ids[0]));
    TEST_ASSERT_EQUAL(id, ids[i]);
    i++;
  }
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_EOF);
  dp_bridge_table_id_iter_destroy(iter);
}

void
test_dp_bridge_flow_iter(void) {
  dp_bridge_iter_t iter;
  struct flow *flow;
  lagopus_result_t rv;

  iter = NULL;
  TEST_ASSERT_EQUAL(dp_bridge_flow_iter_create(bridge_name, 0, &iter),
                    LAGOPUS_RESULT_OK);
  TEST_ASSERT_NOT_NULL(iter);
  flow = NULL;
  while ((rv = dp_bridge_flow_iter_get(iter, &flow)) == LAGOPUS_RESULT_OK) {
    TEST_ASSERT_NOT_NULL(flow);
    flow = NULL;
  }
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_EOF);
  dp_bridge_flow_iter_destroy(iter);
}

void
test_ofp_version_bitmap(void) {
  int version;
  uint32_t bitmap;

  TEST_ASSERT_NOT_NULL(bridge);

  /* Setting versions. */
  for (version = OPENFLOW_VERSION_1_3; version <= OPENFLOW_VERSION_1_4;
       version++) {
    TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == bridge_ofp_version_bitmap_set(bridge,
                     version));
    TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == bridge_ofp_version_bitmap_get(bridge,
                     &bitmap));
    TEST_ASSERT_TRUE(0 != (bitmap & (1 << version)));
  }

  /* Clearing versions. */
  for (version = OPENFLOW_VERSION_1_4; version >= OPENFLOW_VERSION_1_3;
       version--) {
    TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == bridge_ofp_version_bitmap_unset(bridge,
                     version));
    TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == bridge_ofp_version_bitmap_get(bridge,
                     &bitmap));
    TEST_ASSERT_TRUE(0 == (bitmap & (1 << version)));
  }
}

void
test_dp_bridge_stats(void) {
  datastore_bridge_stats_t stats;
  lagopus_result_t rv;

  rv = dp_bridge_stats_get("bad", &stats);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_bridge_stats_get(bridge_name, &stats);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_dp_bridge_meter_list(void) {
  datastore_bridge_meter_info_list_t list;
  lagopus_result_t rv;

  TAILQ_INIT(&list);
  rv = dp_bridge_meter_list_get("bad", &list);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_bridge_meter_list_get(bridge_name, &list);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_dp_bridge_meter_stats(void) {
  datastore_bridge_meter_stats_list_t list;
  lagopus_result_t rv;

  TAILQ_INIT(&list);
  rv = dp_bridge_meter_stats_list_get("bad", &list);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_bridge_meter_stats_list_get(bridge_name, &list);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_dp_bridge_group_list(void) {
  datastore_bridge_group_info_list_t list;
  lagopus_result_t rv;

  TAILQ_INIT(&list);
  rv = dp_bridge_group_list_get("bad", &list);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_bridge_group_list_get(bridge_name, &list);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_dp_bridge_group_stats(void) {
  datastore_bridge_group_stats_list_t list;
  lagopus_result_t rv;

  TAILQ_INIT(&list);
  rv = dp_bridge_group_stats_list_get("bad", &list);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_bridge_group_stats_list_get(bridge_name, &list);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_dp_bridge_l2_clear(void) {
  lagopus_result_t rv;

  rv = dp_bridge_l2_clear("bad");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_bridge_l2_clear(bridge_name);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_dp_bridge_l2_expire_set(void) {
  lagopus_result_t rv;

  rv = dp_bridge_l2_expire_set("bad", 600);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_bridge_l2_expire_set(bridge_name, 600);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_dp_bridge_l2_max_entries_set(void) {
  lagopus_result_t rv;

  rv = dp_bridge_l2_max_entries_set("bad", 600);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_bridge_l2_max_entries_set(bridge_name, 600);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_dp_bridge_l2_entries_get(void) {
  uint64_t nentries;
  lagopus_result_t rv;

  rv = dp_bridge_l2_entries_get("bad", &nentries);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_bridge_l2_entries_get(bridge_name, &nentries);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}
