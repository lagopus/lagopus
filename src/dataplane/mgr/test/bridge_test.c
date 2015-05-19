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
#include "lagopus/bridge.h"
#include "lagopus/dpmgr.h"
#include "openflow13.h"
#include "ofp_band.h"

static struct dpmgr *dpmgr;
static struct bridge *bridge;
static const char bridge_name[] = "br0";
static const uint64_t dpid = 12345678;

void
setUp(void) {
  TEST_ASSERT_NULL(dpmgr);
  TEST_ASSERT_NULL(bridge);
  TEST_ASSERT_NOT_NULL(dpmgr = dpmgr_alloc());
  TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == dpmgr_bridge_add(dpmgr, bridge_name,
                   dpid));
  TEST_ASSERT_NOT_NULL(bridge = dpmgr_bridge_lookup(dpmgr, bridge_name));
}

void
tearDown(void) {
  TEST_ASSERT_NOT_NULL(dpmgr);
  TEST_ASSERT_NOT_NULL(bridge);
  TEST_ASSERT_TRUE(LAGOPUS_RESULT_OK == dpmgr_bridge_delete(dpmgr,
                   bridge_name));
  dpmgr_free(dpmgr);
  bridge = NULL;
  dpmgr = NULL;
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
