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

#include "lagopus_apis.h"

#include "../dataplane_interface.h"
#include "../dot1dBasePortTable_access.h"

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_dot1dBasePortTable_get_dot1dBasePort(void) {
  netsnmp_variable_list data = {0};
  void *lctx = NULL;
  void *dctx1 = NULL;
  void *dctx2 = NULL;
  void *dctx3 = NULL;
  size_t ret_len;

  int32_t *ret_val;

  data.type = ASN_INTEGER;

  dot1dBasePortTable_get_first_data_point(&lctx, &dctx1, &data, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx2, &data, NULL);
  dot1dBasePortTable_data_free(dctx2, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx3, &data, NULL);
  TEST_ASSERT_NULL(dctx3);
  dot1dBasePortTable_loop_free(lctx, NULL);
  ret_val = get_dot1dBasePort(dctx1, &ret_len);
  TEST_ASSERT_NOT_NULL(ret_val);
  TEST_ASSERT_EQUAL_INT32(1, *ret_val);
  TEST_ASSERT_EQUAL_UINT64(sizeof(*ret_val), ret_len);
  dot1dBasePortTable_data_free(dctx1, NULL);
  dot1dBasePortTable_get_first_data_point(&lctx, &dctx1, &data, NULL);
  dot1dBasePortTable_data_free(dctx1, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx2, &data, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx3, &data, NULL);
  TEST_ASSERT_NULL(dctx3);
  dot1dBasePortTable_loop_free(lctx, NULL);
  ret_val = get_dot1dBasePort(dctx2, &ret_len);
  TEST_ASSERT_NOT_NULL(ret_val);
  TEST_ASSERT_EQUAL_INT32(2, *ret_val);
  TEST_ASSERT_EQUAL_UINT64(sizeof(*ret_val), ret_len);
  dot1dBasePortTable_data_free(dctx2, NULL);
}

void
test_dot1dBasePortIfIndexTable_get_dot1dBasePortIfIndex(void) {
  netsnmp_variable_list data = {0};
  void *lctx = NULL;
  void *dctx1 = NULL;
  void *dctx2 = NULL;
  void *dctx3 = NULL;
  size_t ret_len;

  int32_t *ret_val;

  data.type = ASN_INTEGER;

  dot1dBasePortTable_get_first_data_point(&lctx, &dctx1, &data, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx2, &data, NULL);
  dot1dBasePortTable_data_free(dctx2, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx3, &data, NULL);
  TEST_ASSERT_NULL(dctx3);
  dot1dBasePortTable_loop_free(lctx, NULL);
  ret_val = get_dot1dBasePortIfIndex(dctx1, &ret_len);
  TEST_ASSERT_NOT_NULL(ret_val);
  TEST_ASSERT_EQUAL_INT32(1, *ret_val);
  TEST_ASSERT_EQUAL_UINT64(sizeof(*ret_val), ret_len);
  dot1dBasePortTable_data_free(dctx1, NULL);
  dot1dBasePortTable_get_first_data_point(&lctx, &dctx1, &data, NULL);
  dot1dBasePortTable_data_free(dctx1, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx2, &data, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx3, &data, NULL);
  TEST_ASSERT_NULL(dctx3);
  dot1dBasePortTable_loop_free(lctx, NULL);
  ret_val = get_dot1dBasePortIfIndex(dctx2, &ret_len);
  TEST_ASSERT_NOT_NULL(ret_val);
  TEST_ASSERT_EQUAL_INT32(2, *ret_val);
  TEST_ASSERT_EQUAL_UINT64(sizeof(*ret_val), ret_len);
  dot1dBasePortTable_data_free(dctx2, NULL);
}

void
test_dot1dBasePortCircuitTable_get_dot1dBasePortCircuit(void) {
  netsnmp_variable_list data = {0};
  void *lctx = NULL;
  void *dctx1 = NULL;
  void *dctx2 = NULL;
  void *dctx3 = NULL;
  size_t ret_len;

  uint64_t oid[] = {0,0};
  size_t oid_len = sizeof(oid)/sizeof(oid[0]);

  uint64_t *ret_val;

  data.type = ASN_INTEGER;

  dot1dBasePortTable_get_first_data_point(&lctx, &dctx1, &data, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx2, &data, NULL);
  dot1dBasePortTable_data_free(dctx2, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx3, &data, NULL);
  TEST_ASSERT_NULL(dctx3);
  dot1dBasePortTable_loop_free(lctx, NULL);
  ret_val = get_dot1dBasePortCircuit(dctx1, &ret_len);
  TEST_ASSERT_NOT_NULL(ret_val);
  TEST_ASSERT_EQUAL_UINT64(oid_len, ret_len);
  TEST_ASSERT_EQUAL_UINT64_ARRAY(oid, ret_val, oid_len);
  dot1dBasePortTable_data_free(dctx1, NULL);
  dot1dBasePortTable_get_first_data_point(&lctx, &dctx1, &data, NULL);
  dot1dBasePortTable_data_free(dctx1, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx2, &data, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx3, &data, NULL);
  TEST_ASSERT_NULL(dctx3);
  dot1dBasePortTable_loop_free(lctx, NULL);
  ret_val = get_dot1dBasePortCircuit(dctx2, &ret_len);
  TEST_ASSERT_NOT_NULL(ret_val);
  TEST_ASSERT_EQUAL_UINT64(oid_len, ret_len);
  TEST_ASSERT_EQUAL_UINT64_ARRAY(oid, ret_val, oid_len);
  dot1dBasePortTable_data_free(dctx2, NULL);
}

void
test_dot1dBasePortIfIndexTable_get_dot1dBasePortDelayExceededDiscards(void) {
  netsnmp_variable_list data = {0};
  void *lctx = NULL;
  void *dctx1 = NULL;
  void *dctx2 = NULL;
  void *dctx3 = NULL;
  size_t ret_len;

  uint32_t *ret_val;

  data.type = ASN_INTEGER;

  dot1dBasePortTable_get_first_data_point(&lctx, &dctx1, &data, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx2, &data, NULL);
  dot1dBasePortTable_data_free(dctx2, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx3, &data, NULL);
  TEST_ASSERT_NULL(dctx3);
  dot1dBasePortTable_loop_free(lctx, NULL);
  ret_val = get_dot1dBasePortDelayExceededDiscards(dctx1, &ret_len);
  TEST_ASSERT_NOT_NULL(ret_val);
  TEST_ASSERT_EQUAL_UINT32(0, *ret_val);
  TEST_ASSERT_EQUAL_UINT64(sizeof(*ret_val), ret_len);
  dot1dBasePortTable_data_free(dctx1, NULL);
  dot1dBasePortTable_get_first_data_point(&lctx, &dctx1, &data, NULL);
  dot1dBasePortTable_data_free(dctx1, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx2, &data, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx3, &data, NULL);
  TEST_ASSERT_NULL(dctx3);
  dot1dBasePortTable_loop_free(lctx, NULL);
  ret_val = get_dot1dBasePortDelayExceededDiscards(dctx2, &ret_len);
  TEST_ASSERT_NOT_NULL(ret_val);
  TEST_ASSERT_EQUAL_UINT32(0, *ret_val);
  TEST_ASSERT_EQUAL_UINT64(sizeof(*ret_val), ret_len);
  dot1dBasePortTable_data_free(dctx2, NULL);
}

void
test_dot1dBasePortIfIndexTable_get_dot1dBasePortMtuExceededDiscards(void) {
  netsnmp_variable_list data = {0};
  void *lctx = NULL;
  void *dctx1 = NULL;
  void *dctx2 = NULL;
  void *dctx3 = NULL;
  size_t ret_len;

  uint32_t *ret_val;

  data.type = ASN_INTEGER;

  dot1dBasePortTable_get_first_data_point(&lctx, &dctx1, &data, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx2, &data, NULL);
  dot1dBasePortTable_data_free(dctx2, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx3, &data, NULL);
  TEST_ASSERT_NULL(dctx3);
  dot1dBasePortTable_loop_free(lctx, NULL);
  ret_val = get_dot1dBasePortMtuExceededDiscards(dctx1, &ret_len);
  TEST_ASSERT_NOT_NULL(ret_val);
  TEST_ASSERT_EQUAL_UINT32(0, *ret_val);
  TEST_ASSERT_EQUAL_UINT64(sizeof(*ret_val), ret_len);
  dot1dBasePortTable_data_free(dctx1, NULL);
  dot1dBasePortTable_get_first_data_point(&lctx, &dctx1, &data, NULL);
  dot1dBasePortTable_data_free(dctx1, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx2, &data, NULL);
  dot1dBasePortTable_get_next_data_point(&lctx, &dctx3, &data, NULL);
  TEST_ASSERT_NULL(dctx3);
  dot1dBasePortTable_loop_free(lctx, NULL);
  ret_val = get_dot1dBasePortMtuExceededDiscards(dctx2, &ret_len);
  TEST_ASSERT_NOT_NULL(ret_val);
  TEST_ASSERT_EQUAL_UINT32(0, *ret_val);
  TEST_ASSERT_EQUAL_UINT64(sizeof(*ret_val), ret_len);
  dot1dBasePortTable_data_free(dctx2, NULL);
}
