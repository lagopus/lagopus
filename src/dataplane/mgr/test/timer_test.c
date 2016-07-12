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

#include "dp_timer.c"

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
  struct dp_timer *dp_timer;
  lagopus_result_t rv;

  clock_gettime(CLOCK_MONOTONIC, &now_ts);
  flow.idle_timeout = 100;
  flow.hard_timeout = 100;
  flow.create_time.tv_sec = now_ts.tv_sec;
  flow.update_time.tv_sec = now_ts.tv_sec;
  rv = add_flow_timer(&flow);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  dp_timer = TAILQ_FIRST(&dp_timer_list);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 100);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NULL(dp_timer);
  flow.idle_timeout = 20;
  flow.hard_timeout = 20;
  rv = add_flow_timer(&flow);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  dp_timer = TAILQ_FIRST(&dp_timer_list);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 20);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 80);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NULL(dp_timer);
}

void
test_add_mbtree_timer(void) {
  struct flow_list flow_list;
  lagopus_result_t rv;

  rv = add_mbtree_timer(&flow_list, 100);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

static void
t_add_flow_timer(int sec) {
  struct flow flow;
  lagopus_result_t rv;

  clock_gettime(CLOCK_MONOTONIC, &now_ts);
  flow.idle_timeout = sec;
  flow.hard_timeout = sec;
  flow.create_time.tv_sec = now_ts.tv_sec;
  flow.update_time.tv_sec = now_ts.tv_sec;
  rv = add_flow_timer(&flow);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_add_multiple_timer(void) {
  struct flow_list flow_list;
  struct dp_timer *dp_timer;
  lagopus_result_t rv;

  t_add_flow_timer(100);
  dp_timer = TAILQ_FIRST(&dp_timer_list);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 100);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NULL(dp_timer);

  t_add_flow_timer(20);
  dp_timer = TAILQ_FIRST(&dp_timer_list);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 20);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 80);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NULL(dp_timer);

  rv = add_mbtree_timer(&flow_list, 5);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  dp_timer = TAILQ_FIRST(&dp_timer_list);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 5);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 15);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 80);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NULL(dp_timer);

  t_add_flow_timer(30);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  dp_timer = TAILQ_FIRST(&dp_timer_list);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 5);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 15);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 10);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 70);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NULL(dp_timer);

  t_add_flow_timer(1);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  dp_timer = TAILQ_FIRST(&dp_timer_list);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 1);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 4);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 15);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 10);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 70);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NULL(dp_timer);

  rv = add_mbtree_timer(&flow_list, 8);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  dp_timer = TAILQ_FIRST(&dp_timer_list);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 1);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 4);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 3);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 12);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 10);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NOT_NULL(dp_timer);
  TEST_ASSERT_EQUAL(dp_timer->timeout, 70);
  dp_timer = TAILQ_NEXT(dp_timer, next);
  TEST_ASSERT_NULL(dp_timer);
}
