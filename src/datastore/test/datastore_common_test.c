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
#include "lagopus/datastore.h"
#include "../datastore_internal.h"

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_copy_mac_address(void) {
  lagopus_result_t rc;

  const mac_address_t src_mac_addr = {0x11, 0x22, 0x33, 0xAA, 0xBB, 0xCC};
  mac_address_t actual_mac_addr = {0, 0, 0, 0, 0, 0};
  mac_address_t expected_mac_addr = {0x11, 0x22, 0x33, 0xAA, 0xBB, 0xCC};

  rc = copy_mac_address(src_mac_addr, actual_mac_addr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_mac_addr, actual_mac_addr, 6);
}

void
test_equals_mac_address(void) {
  bool result;

  const mac_address_t mac_addr1 = {0x11, 0x22, 0x33, 0xAA, 0xBB, 0xCC};
  const mac_address_t mac_addr2 = {0x11, 0x22, 0x33, 0xAA, 0xBB, 0xCC};
  const mac_address_t mac_addr3 = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};

  // equal
  result = equals_mac_address(mac_addr1, mac_addr2);
  TEST_ASSERT_TRUE(result);

  // not equal
  result = equals_mac_address(mac_addr1, mac_addr3);
  TEST_ASSERT_FALSE(result);

  // not equal
  result = equals_mac_address(mac_addr2, mac_addr3);
  TEST_ASSERT_FALSE(result);
}
