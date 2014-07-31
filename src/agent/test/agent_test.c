/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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
#include "lagopus/dpmgr.h"
#include "lagopus_gstate.h"
#include "../agent.h"
#include "confsys.h"

struct dpmgr *dpmgr = NULL;

void
setUp(void) {
  if (dpmgr != NULL) {
    return;
  }

  /* Datapath manager alloc. */
  dpmgr = dpmgr_alloc();
  if (dpmgr == NULL) {
    fprintf(stderr, "Datapath manager allocation failed\n");
    exit(-1);
  }
  (void)global_state_set(GLOBAL_STATE_STARTED);
}

void
tearDown(void) {
}

void
test_init_confsys(void) {
  lagopus_thread_t *cserver;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* Init Confsys (Singleton).  */
  ret = config_handle_initialize(NULL, &cserver);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = config_handle_initialize(NULL, &cserver);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ALREADY_EXISTS, ret);
}

void
test_agent_start_shutdown(void) {
  lagopus_thread_t *t;
  lagopus_result_t ret;
  ret = agent_initialize((void *)dpmgr, &t);
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
  ret = agent_initialize((void *)dpmgr, &t);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ALREADY_EXISTS, ret);
  ret = agent_start();
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ALREADY_EXISTS, ret);
  ret = agent_stop();
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
}

void
test_free_confsys(void) {
  lagopus_result_t ret;

  ret = config_handle_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  config_handle_finalize();
}
