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


void
setUp(void) {
}

void
tearDown(void) {
}


void
test_zero_polls(void) {
  lagopus_result_t ret = lagopus_qmuxer_poll(NULL, NULL, 0,
                         1000 * 1000 * 1000);
  TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_TIMEDOUT);
}


void
test_null_polls(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_qmuxer_t qmx = NULL;

  ret = lagopus_qmuxer_create(&qmx);
  TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);

  if (ret == LAGOPUS_RESULT_OK) {
    lagopus_bbq_t q = NULL;
    ret = lagopus_bbq_create(&q, uint32_t, 1000, NULL);
    TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);

    if (ret == LAGOPUS_RESULT_OK) {
      lagopus_qmuxer_poll_t polls[1];
      ret = lagopus_qmuxer_poll_create(&polls[0], q,
                                       LAGOPUS_QMUXER_POLL_READABLE);
      TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_OK);

      if (ret == LAGOPUS_RESULT_OK) {

        ret = lagopus_qmuxer_poll(&qmx, polls, 0, 1000 * 1000 * 1000);
        TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_TIMEDOUT);

        ret = lagopus_qmuxer_poll(&qmx, NULL, 1, 1000 * 1000 * 1000);
        TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_TIMEDOUT);

        ret = lagopus_qmuxer_poll(NULL, polls , 0, 1000 * 1000 * 1000);
        TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_TIMEDOUT);

        ret = lagopus_qmuxer_poll(NULL, NULL , 1, 1000 * 1000 * 1000);
        TEST_ASSERT_EQUAL(ret, LAGOPUS_RESULT_TIMEDOUT);

        lagopus_qmuxer_poll_destroy(&polls[0]);
      }

      lagopus_bbq_destroy(&q, true);
    }

    lagopus_qmuxer_destroy(&qmx);
  }
}
