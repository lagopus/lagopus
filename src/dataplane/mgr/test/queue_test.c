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

#include "unity.h"
#include "lagopus/dp_apis.h"
#include "lagopus/datastore/queue.h"

void
setUp(void) {
  dp_api_init();
  TEST_ASSERT_EQUAL(dp_interface_create("if1"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_create("port1"), LAGOPUS_RESULT_OK);
}

void
tearDown(void) {
  TEST_ASSERT_EQUAL(dp_port_destroy("port1"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_interface_destroy("if1"), LAGOPUS_RESULT_OK);
  dp_api_fini();
}

void
test_dp_queue_create_and_destroy(void) {
  datastore_queue_info_t qinfo;

  qinfo.id = 0;
  TEST_ASSERT_EQUAL(dp_queue_create("queue01", &qinfo), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_queue_create("queue01", &qinfo),
                    LAGOPUS_RESULT_ALREADY_EXISTS);
  TEST_ASSERT_EQUAL(dp_queue_create("queue02", &qinfo),
                    LAGOPUS_RESULT_ALREADY_EXISTS);
  qinfo.id = 1;
  TEST_ASSERT_EQUAL(dp_queue_create("queue02", &qinfo), LAGOPUS_RESULT_OK);
  dp_queue_destroy("queue01");
  dp_queue_destroy("queue02");
}

void
test_dp_port_queue_add(void) {
  datastore_queue_info_t qinfo;

  qinfo.id = 0;
  TEST_ASSERT_EQUAL(dp_queue_create("queue01", &qinfo), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_interface_set("port1", "if1"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_queue_add("port1", "queue01"), LAGOPUS_RESULT_OK);
}
