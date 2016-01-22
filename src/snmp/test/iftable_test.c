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

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_IfTable_get_ifIndex_normal_pattern(void) {
  /*
   * expect:
   *  - number of interfaces is 2
   *  -
   */
  netsnmp_variable_list put_index_data = {0};
  netsnmp_variable_list *ret;
  void *loop_context = NULL;
  void *data_context1 = NULL;
  void *data_context2 = NULL;
  void *data_context3 = NULL;
  size_t ret_len;
  int32_t *ifIndex;

  put_index_data.type = ASN_INTEGER;

  ret = ifTable_get_first_data_point(&loop_context, &data_context1,
                                     &put_index_data, NULL);
  TEST_ASSERT_NOT_NULL_MESSAGE(
    ret,
    "a return value shouldn't be NULL.");
  TEST_ASSERT_NOT_NULL_MESSAGE(
    loop_context,
    "loop context shouldn't be NULL.");
  TEST_ASSERT_EQUAL_INT32_MESSAGE(
    1, *(put_index_data.val.integer),
    "the first index");

  ret = ifTable_get_next_data_point(&loop_context, &data_context2,
                                    &put_index_data, NULL);

  TEST_ASSERT(ret == &put_index_data);
  TEST_ASSERT_EQUAL_INT32_MESSAGE(2, *(put_index_data.val.integer),
                                  "the second index");

  ifTable_data_free(data_context1, NULL);

  ret = ifTable_get_next_data_point(&loop_context, &data_context3,
                                    &put_index_data, NULL);

  TEST_ASSERT_NULL_MESSAGE(
    ret,
    "at end of table iteration, get_next_data_point should return NULL.");

  ifTable_loop_free(loop_context, NULL);

  ifIndex = get_ifIndex(data_context2, &ret_len);

  TEST_ASSERT_EQUAL_INT64_MESSAGE(2, *ifIndex,
                                  "second ifIndex should be 2");

  ifTable_data_free(data_context2, NULL);
}

void
test_IfTable_index_counting(void) {
  netsnmp_variable_list data = {0};
  netsnmp_variable_list *ret;
  void *lctx = NULL;
  void *dctx1 = NULL;
  void *dctx2 = NULL;
  void *dctx3 = NULL;

  data.type = ASN_INTEGER;

  ret = ifTable_get_first_data_point(&lctx, &dctx1, &data, NULL);
  TEST_ASSERT_NOT_NULL(ret);
  TEST_ASSERT_NOT_NULL(lctx);
  TEST_ASSERT_EQUAL_INT32(1, *(data.val.integer));

  ifTable_data_free(dctx1, NULL);

  ret = ifTable_get_next_data_point(&lctx, &dctx2, &data, NULL);
  TEST_ASSERT_NOT_NULL(ret);
  TEST_ASSERT_NOT_NULL(lctx);
  TEST_ASSERT_EQUAL_INT32(2, *(data.val.integer));

  ifTable_data_free(dctx2, NULL);

  ret = ifTable_get_next_data_point(&lctx, &dctx3, &data, NULL);
  TEST_ASSERT_NULL(ret);
  TEST_ASSERT_NULL(dctx3);
  ifTable_loop_free(lctx, NULL);
}
