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

#include "openflow.h"

#include "lagopus_apis.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/dp_apis.h"

void
setUp(void) {
  dp_api_init();
}

void
tearDown(void) {
  dp_api_fini();
}

void
test_interface_create(void) {
  lagopus_result_t rv;

  rv = dp_interface_create("test");
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK,
                            "dp_interface_create error");
}

void
test_interface_destroy(void) {
  lagopus_result_t rv;

  rv = dp_interface_create("test");
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK,
                            "dp_interface_create error");
  rv = dp_interface_destroy("test2");
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_NOT_FOUND,
                            "dp_interface_destroy error");
  rv = dp_interface_destroy("test");
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK,
                            "dp_interface_destroy error");
}
