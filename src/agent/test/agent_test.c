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
#include "lagopus_gstate.h"
#include "../agent.h"

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_prologue(void) {
  lagopus_result_t r;
  const char *argv0 =
      ((IS_VALID_STRING(lagopus_get_command_name()) == true) ?
       lagopus_get_command_name() : "callout_test");
  const char * const argv[] = {
    argv0, NULL
  };

#define N_CALLOUT_WORKERS	1
  (void)lagopus_mainloop_set_callout_workers_number(N_CALLOUT_WORKERS);
  r = lagopus_mainloop_with_callout(1, argv, NULL, NULL,
                                    false, false, true);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
}
void
test_agent_start_shutdown(void) {
  lagopus_thread_t *t;
  lagopus_result_t ret;
  ret = agent_initialize(NULL, &t);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  ret = agent_start();
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  ret = agent_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

}

void
test_agent_start_stop(void) {
  lagopus_thread_t *t;
  lagopus_result_t ret;
  ret = agent_initialize(NULL, &t);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ALREADY_EXISTS, ret);
  ret = agent_start();
  //TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ALREADY_EXISTS, ret);
  ret = agent_stop();
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
