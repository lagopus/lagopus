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

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "ifTable_access.h"
#include "ifTable_enums.h"

#include "lagopus_apis.h"

#include "stub_values.h"

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_IfTable_get_ifOutOctets(void) {
  netsnmp_variable_list data = {0};
  void *lctx = NULL;
  void *dctx1 = NULL;
  void *dctx2 = NULL;
  void *dctx3 = NULL;
  size_t ret_len;

  uint32_t *ret_val;

  data.type = ASN_INTEGER;

  ifTable_get_first_data_point(&lctx, &dctx1, &data, NULL);
  ifTable_get_next_data_point(&lctx, &dctx2, &data, NULL);
  ifTable_data_free(dctx2, NULL);
  ifTable_get_next_data_point(&lctx, &dctx3, &data, NULL);
  TEST_ASSERT_NULL(dctx3);
  ifTable_loop_free(lctx, NULL);
  ret_val = get_ifOutOctets(dctx1, &ret_len);
  TEST_ASSERT_NOT_NULL(ret_val);
  TEST_ASSERT_EQUAL_UINT32(VALUE_ifOutOctets_1, *ret_val);
  TEST_ASSERT_EQUAL_UINT64(sizeof(*ret_val), ret_len);
  ifTable_data_free(dctx1, NULL);
  ifTable_get_first_data_point(&lctx, &dctx1, &data, NULL);
  ifTable_data_free(dctx1, NULL);
  ifTable_get_next_data_point(&lctx, &dctx2, &data, NULL);
  ifTable_get_next_data_point(&lctx, &dctx3, &data, NULL);
  TEST_ASSERT_NULL(dctx3);
  ifTable_loop_free(lctx, NULL);
  ret_val = get_ifOutOctets(dctx2, &ret_len);
  TEST_ASSERT_NOT_NULL(ret_val);
  TEST_ASSERT_EQUAL_UINT32(VALUE_ifOutOctets_2, *ret_val);
  TEST_ASSERT_EQUAL_UINT64(sizeof(*ret_val), ret_len);
  ifTable_data_free(dctx2, NULL);
}
