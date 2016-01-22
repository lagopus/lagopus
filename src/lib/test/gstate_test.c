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

#include "lagopus_apis.h"
#include "lagopus_thread_internal.h"
#include "unity.h"





typedef struct null_thread_record {
  lagopus_thread_record m_thd;
  shutdown_grace_level_t m_gl;
} null_thread_record;
typedef null_thread_record 	*null_thread_t;





static lagopus_result_t
s_main(const lagopus_thread_t *tptr, void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)arg;

  if (tptr != NULL) {
    null_thread_t nptr = (null_thread_t)*tptr;

    if (nptr != NULL) {
      global_state_t s;
      shutdown_grace_level_t l;

      lagopus_msg_debug(1, "waiting for the world changes to "
                        "GLOBAL_STATE_STARTED ...\n");

      while ((ret = global_state_wait_for(GLOBAL_STATE_STARTED, &s, &l,
                                          1000LL * 1000LL * 100LL)) ==
             LAGOPUS_RESULT_TIMEDOUT) {
        lagopus_msg_debug(1, "still waiting for the world changes to "
                          "GLOBAL_STATE_STARTED ...\n");
      }

      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        if (ret == LAGOPUS_RESULT_NOT_OPERATIONAL) {
          /*
           * Whole the system is about to be shutdown. Just exit.
           */
          if (IS_GLOBAL_STATE_SHUTDOWN(s) == true) {
            goto done;
          } else {
            lagopus_exit_fatal("must not happen.\n");
          }
        }
      } else {
        if (s != GLOBAL_STATE_STARTED) {
          lagopus_exit_fatal("must not happen, too.\n");
        }
      }

      lagopus_msg_debug(1, "wait done.\n");

      sleep(1);

      if ((ret = global_state_request_shutdown(nptr->m_gl)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      lagopus_msg_debug(1, "request shutdown.\n");

      ret = LAGOPUS_RESULT_OK;
    }
  }

done:
  return ret;
}


static void
s_finalize(const lagopus_thread_t *tptr, bool is_canceled, void *arg) {
  (void)arg;

  if (tptr != NULL) {
    null_thread_t nptr = (null_thread_t)*tptr;

    if (nptr != NULL) {

      lagopus_msg_debug(1, "called, %s.\n",
                        (is_canceled == true) ? "canceled" : "exit");
      if (is_canceled == true) {
        global_state_cancel_janitor();
      }

    }
  }
}


static inline lagopus_result_t
s_null_create(null_thread_t *nptr, shutdown_grace_level_t l) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (IS_VALID_SHUTDOWN(l) == true && nptr != NULL) {
    null_thread_t nt;

    if (*nptr != NULL) {
      nt = *nptr;
    } else {
      if ((nt = (null_thread_t)malloc(sizeof(*nt))) == NULL) {
        ret = LAGOPUS_RESULT_NO_MEMORY;
        goto done;
      }
    }

    if ((ret = lagopus_thread_create((lagopus_thread_t *)&nt,
                                     s_main, s_finalize, NULL,
                                     "null", NULL)) == LAGOPUS_RESULT_OK) {
      if (*nptr == NULL) {
        (void)lagopus_thread_free_when_destroy((lagopus_thread_t *)&nt);
        *nptr = nt;
      }

      nt->m_gl = l;

      ret = LAGOPUS_RESULT_OK;

    } else {
      if (*nptr == NULL) {
        free((void *)nt);
      }
    }

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}





void
setUp(void) {
}


void
tearDown(void) {
}





void
test_set_get(void) {
  lagopus_result_t r;
  global_state_t s;
  int i;

  global_state_reset();

  for (i = (int)GLOBAL_STATE_INITIALIZING;
       i <= (int)GLOBAL_STATE_FINALIZED;
       i++) {

    r = global_state_set((global_state_t)i);
    TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

    r = global_state_get(&s);
    TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
    TEST_ASSERT_EQUAL(s, (global_state_t)i);
  }
}


void
test_sync1(void) {
  lagopus_result_t r;
  shutdown_grace_level_t l;
  null_thread_t nt = NULL;

  global_state_reset();

  r = global_state_set(GLOBAL_STATE_INITIALIZING);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = s_null_create(&nt, SHUTDOWN_RIGHT_NOW);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = lagopus_thread_start((lagopus_thread_t *)&nt, false);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = global_state_set(GLOBAL_STATE_STARTED);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  lagopus_msg_debug(1, "waiting for shutdown request ...\n");
  while ((r = global_state_wait_for_shutdown_request(
                &l, 1000LL * 1000LL * 100LL)) == LAGOPUS_RESULT_TIMEDOUT) {
    lagopus_msg_debug(1, "still waiting for shutdown request ...\n");
  }
  if (r != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
  }
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(l, SHUTDOWN_RIGHT_NOW);

  lagopus_msg_debug(1, "got a shutdown request.\n");

  r = global_state_set(GLOBAL_STATE_ACCEPT_SHUTDOWN);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  lagopus_msg_debug(1, "shutdown request accepted.\n");

  r = lagopus_thread_wait((lagopus_thread_t *)&nt, -1LL);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  lagopus_thread_destroy((lagopus_thread_t *)&nt);
}


void
test_sync2(void) {
  lagopus_result_t r;
  shutdown_grace_level_t l;
  null_thread_t nt = NULL;

  global_state_reset();

  r = global_state_set(GLOBAL_STATE_INITIALIZING);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = s_null_create(&nt, SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = lagopus_thread_start((lagopus_thread_t *)&nt, false);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = global_state_set(GLOBAL_STATE_STARTED);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  lagopus_msg_debug(1, "waiting for shutdown request ...\n");
  while ((r = global_state_wait_for_shutdown_request(
                &l, 1000LL * 1000LL * 100LL)) == LAGOPUS_RESULT_TIMEDOUT) {
    lagopus_msg_debug(1, "still waiting for shutdown request ...\n");
  }
  if (r != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
  }
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(l, SHUTDOWN_GRACEFULLY);

  lagopus_msg_debug(1, "got a shutdown request.\n");

  r = global_state_set(GLOBAL_STATE_ACCEPT_SHUTDOWN);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  lagopus_msg_debug(1, "shutdown request accepted.\n");

  r = lagopus_thread_wait((lagopus_thread_t *)&nt, -1LL);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  lagopus_thread_destroy((lagopus_thread_t *)&nt);
}


void
test_cancel1(void) {
  lagopus_result_t r;
  null_thread_t nt = NULL;

  global_state_reset();

  r = global_state_set(GLOBAL_STATE_INITIALIZING);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = s_null_create(&nt, SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = lagopus_thread_start((lagopus_thread_t *)&nt, false);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  sleep(1);

  lagopus_thread_destroy((lagopus_thread_t *)&nt);
}


void
test_cancel2(void) {
  lagopus_result_t r;
  shutdown_grace_level_t l;
  null_thread_t nt = NULL;

  global_state_reset();

  r = global_state_set(GLOBAL_STATE_INITIALIZING);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = s_null_create(&nt, SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = lagopus_thread_start((lagopus_thread_t *)&nt, false);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = global_state_set(GLOBAL_STATE_STARTED);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  lagopus_msg_debug(1, "waiting for shutdown request ...\n");
  while ((r = global_state_wait_for_shutdown_request(
                &l, 1000LL * 1000LL * 100LL)) == LAGOPUS_RESULT_TIMEDOUT) {
    lagopus_msg_debug(1, "still waiting for shutdown request ...\n");
  }
  if (r != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
  }
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(l, SHUTDOWN_GRACEFULLY);

  lagopus_thread_destroy((lagopus_thread_t *)&nt);
}
