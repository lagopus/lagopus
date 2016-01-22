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
#include "openflow.h"
#include "openflow13.h"
#include "handler_test_utils.h"
#include "../ofp_features_capabilities.h"

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_ofp_features_capabilities_convert(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t ds_capabilities =
      DATASTORE_BRIDGE_BIT_CAPABILITY_TABLE_STATS |
      DATASTORE_BRIDGE_BIT_CAPABILITY_GROUP_STATS |
      DATASTORE_BRIDGE_BIT_CAPABILITY_BLOCK_LOOPING_PORTS;
  uint32_t features_capabilities =
      OFPC_TABLE_STATS |
      OFPC_GROUP_STATS |
      OFPC_PORT_BLOCKED;
  uint32_t test_features_capabilities;

  ret = ofp_features_capabilities_convert(ds_capabilities,
                                          &test_features_capabilities);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_features_capabilities_convert error.");
  TEST_ASSERT_EQUAL_MESSAGE(features_capabilities,
                            test_features_capabilities,
                            "features compare error.");
}

void
test_ofp_features_capabilities_convert_invalid_args(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t ds_capabilities = 0LL;

  ret = ofp_features_capabilities_convert(ds_capabilities,
                                          NULL);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_features_capabilities_convert error.");
}
