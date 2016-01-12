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

#include <inttypes.h>
#include <time.h>
#include <sys/queue.h>

#include "unity.h"
#include "lagopus_apis.h"
#include "lagopus/flowdb.h"

void
setUp(void) {
  init_dp_timer();
}

void
tearDown(void) {
}

void
test_add_flow_timer(void) {
  struct flow flow;
  lagopus_result_t rv;

  flow.idle_timeout = 100;
  flow.hard_timeout = 100;
  rv = add_flow_timer(&flow);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_add_mbtree_timer(void) {
  struct flow_list flow_list;
  lagopus_result_t rv;

  rv = add_mbtree_timer(&flow_list, 100);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}
