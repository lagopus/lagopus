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
#include "unity.h"

#define N_LOOP 10
#define GET_TID_RETRY 10

#define MAIN_SLEEP_USEC 50000LL
#define WAIT_NSEC 1000LL * 1000LL * 1000LL

static int s_freeup_cnt = 0;
static int s_finalize_cnt = 0;

void setUp(void) {
  ;
}

void tearDown(void) {
  ;
}

static lagopus_result_t
main_proc(const lagopus_thread_t *selfptr, void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  pthread_t tid = LAGOPUS_INVALID_THREAD;
  (void)arg;
  ret = lagopus_thread_get_pthread_id(selfptr, &tid);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  usleep(MAIN_SLEEP_USEC);
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
main_proc_autodelete(const lagopus_thread_t *selfptr, void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  pthread_t tid = LAGOPUS_INVALID_THREAD;
  (void)arg;
  ret = lagopus_thread_get_pthread_id(selfptr, &tid);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  sleep(5);
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
main_proc_long(const lagopus_thread_t *selfptr, void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  pthread_t tid = LAGOPUS_INVALID_THREAD;
  (void)arg;
  ret = lagopus_thread_get_pthread_id(selfptr, &tid);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  sleep(1000);
  return LAGOPUS_RESULT_OK;
}

static void
freeup_proc(const lagopus_thread_t *selfptr, void *arg) {
  (void)selfptr;
  (void)arg;
  s_freeup_cnt++;
}

static void
finalize_proc(const lagopus_thread_t *selfptr,
              bool is_canceled, void *arg) {
  (void)selfptr;
  (void)is_canceled;
  (void)arg;
  s_finalize_cnt++;
}

static inline bool
s_check_get_thread_id(lagopus_thread_t *thd_ptr,
                      lagopus_result_t require_ret) {
  bool result = false;
  lagopus_result_t ret;
  pthread_t tid;
  int i;
  for (i = 0; i < GET_TID_RETRY; i++) {
    ret = lagopus_thread_get_pthread_id(thd_ptr, &tid);
    if (ret == require_ret) {
      break;
    }
  }
  if (ret == require_ret) {
    if (ret == LAGOPUS_RESULT_OK) {
      TEST_ASSERT_NOT_EQUAL(tid, LAGOPUS_INVALID_THREAD);
    }
    result = true;
  } else {
    if (require_ret == LAGOPUS_RESULT_OK
        && ret == LAGOPUS_RESULT_ALREADY_HALTED) {
      lagopus_msg_warning(
        "test requires LAGOPUS_RESULT_OK, but result is LAGOPUS_ALREADY_HALTED. "
        "Modify value of WAIT_NSEC and MAIN_SLEEP_USEC of thread_test.c, please try again.\n");
      result = true;
    } else {
      lagopus_perror(ret);
      TEST_FAIL_MESSAGE("get_thread_id failed");
      result = false;
    }
  }
  return result;
}


static inline bool
s_check_is_canceled(lagopus_thread_t *thd_ptr,
                    lagopus_result_t require_ret,
                    bool require_result) {
  bool result = !require_result;
  lagopus_result_t ret;
  int i;
  for (i = 0; i < GET_TID_RETRY; i++) {
    ret = lagopus_thread_is_canceled(thd_ptr, &result);
    if (ret == require_ret) {
      break;
    }
  }
  if (ret == require_ret) {
    if (ret == LAGOPUS_RESULT_OK) {
      result = (result == require_result);
    } else {
      result = true;
    }
  } else {
    lagopus_perror(ret);
    TEST_FAIL_MESSAGE("is_canceled failed");
    result = false;
  }
  return result;
}


static inline bool
s_check_create_common(lagopus_thread_t *thd_ptr, lagopus_result_t require_ret,
                      lagopus_thread_main_proc_t mproc, const char *name) {
  bool result = false;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = lagopus_thread_create(thd_ptr, mproc, finalize_proc, freeup_proc,
                              name, NULL);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = lagopus_thread_free_when_destroy(thd_ptr);
    if (ret == require_ret) {
      result = s_check_get_thread_id(thd_ptr, LAGOPUS_RESULT_NOT_STARTED);
    } else {
      lagopus_perror(ret);
      TEST_FAIL_MESSAGE("create failed");
      result = false;
    }
  } else {
    if (ret == require_ret) {
      result = true;
    } else {
      lagopus_perror(ret);
      TEST_FAIL_MESSAGE("create failed");
      result = false;
    }
  }
  return result;
}

static inline bool
s_check_create(lagopus_thread_t *thd_ptr, lagopus_result_t require_ret) {
  return s_check_create_common(thd_ptr, require_ret, main_proc, "create");
}

static inline bool
s_check_create_long(lagopus_thread_t *thd_ptr, lagopus_result_t require_ret) {
  return s_check_create_common(thd_ptr, require_ret, main_proc_long, "create_l");
}

static inline bool
s_check_create_autodelete(lagopus_thread_t *thd_ptr,
                          lagopus_result_t require_ret) {
  return s_check_create_common(thd_ptr, require_ret, main_proc_autodelete,
                               "create_autodelete");
}

static inline void
s_check_destroy(lagopus_thread_t *thd_ptr) {
  lagopus_thread_destroy(thd_ptr);
}

static inline bool
s_check_start_common(lagopus_thread_t *thd_ptr, lagopus_result_t require_ret,
                     bool autodelete) {
  bool result = false;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = lagopus_thread_start(thd_ptr, autodelete);
  if (ret == require_ret) {
    if (ret == LAGOPUS_RESULT_OK) {
      result = s_check_get_thread_id(thd_ptr, LAGOPUS_RESULT_OK);
    } else {
      result = true;
    }
  } else {
    lagopus_perror(ret);
    TEST_FAIL_MESSAGE("start failed");
    result = false;
  }
  return result;
}
static inline bool
s_check_start(lagopus_thread_t *thd_ptr, lagopus_result_t require_ret) {
  return s_check_start_common(thd_ptr, require_ret, false);
}

static inline bool
s_check_start_autodelete(lagopus_thread_t *thd_ptr,
                         lagopus_result_t require_ret) {
  return s_check_start_common(thd_ptr, require_ret, true);
}


static inline bool
s_check_wait(lagopus_thread_t *thd_ptr, lagopus_result_t require_ret) {
  bool result = false;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = lagopus_thread_wait(thd_ptr, WAIT_NSEC);
  if (ret != require_ret) {
    lagopus_perror(ret);
    TEST_FAIL_MESSAGE("wait failed");
    result = false;
  } else {
    result = true;
  }
  return result;
}

static inline bool
s_check_cancel(lagopus_thread_t *thd_ptr,
               lagopus_result_t require_ret) {
  bool result = false;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = lagopus_thread_cancel(thd_ptr);
  if (ret != require_ret) {
    s_check_is_canceled(thd_ptr, LAGOPUS_RESULT_OK, true);
    lagopus_perror(ret);
    TEST_FAIL_MESSAGE("cancel failed");
    result = false;
  } else {
    result = true;
  }

  return result;
}


/* tests */
void
test_thread_create(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0;
  for (i = 0; i < N_LOOP; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_create(&m_thread, LAGOPUS_RESULT_OK),
                              "creation error");
    s_check_destroy(&m_thread);
  }
}

void
test_thread_start_NULL(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0;
  for (i = 0; i < N_LOOP; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_start(&m_thread,
                              LAGOPUS_RESULT_INVALID_ARGS),
                              "start error");
  }
}

void
test_thread_wait_NULL(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0;
  for (i = 0; i < N_LOOP; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_wait(&m_thread,
                              LAGOPUS_RESULT_INVALID_ARGS),
                              "wait error");
  }
}

void
test_thread_cancel_NULL(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0;
  for (i = 0; i < N_LOOP; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_cancel(&m_thread,
                              LAGOPUS_RESULT_INVALID_ARGS),
                              "cancel error");
  }
}

void
test_thread_destroy_NULL(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0;
  for (i = 0; i < N_LOOP; i++) {
    s_check_destroy(&m_thread);
  }
}

void
test_thread_create_destroy(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0;
  for (i = 0; i < N_LOOP; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_create(&m_thread, LAGOPUS_RESULT_OK),
                              "create error");
    s_check_destroy(&m_thread);
  }
}

void
test_thread_create_start_destroy(void) {
  lagopus_thread_t m_thread = NULL;
  int i;
  for (i = 0; i < N_LOOP; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_create(&m_thread, LAGOPUS_RESULT_OK),
                              "create error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_start(&m_thread, LAGOPUS_RESULT_OK),
                              "start error");
    s_check_destroy(&m_thread);
  }
}

void
test_thread_create_start_wait_destroy(void) {
  lagopus_thread_t m_thread = NULL;
  int i;
  for (i = 0; i < N_LOOP; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_create(&m_thread, LAGOPUS_RESULT_OK),
                              "create error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_start(&m_thread, LAGOPUS_RESULT_OK),
                              "start error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_wait(&m_thread, LAGOPUS_RESULT_OK),
                              "wait error");
    s_check_destroy(&m_thread);
  }
}

void
test_thread_create_start_cancel_wait(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0;
  for (i = 0; i < N_LOOP; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_create(&m_thread, LAGOPUS_RESULT_OK),
                              "create error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_start(&m_thread, LAGOPUS_RESULT_OK),
                              "start error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_cancel(&m_thread, LAGOPUS_RESULT_OK),
                              "cancel error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_wait(&m_thread, LAGOPUS_RESULT_OK),
                              "wait error");
    s_check_destroy(&m_thread);
  }
}


void
test_thread_create_start_cancel_destroy(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0;
  for (i = 0; i < N_LOOP; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_create(&m_thread, LAGOPUS_RESULT_OK),
                              "create error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_start(&m_thread, LAGOPUS_RESULT_OK),
                              "start error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_cancel(&m_thread, LAGOPUS_RESULT_OK),
                              "cancel error");
    s_check_destroy(&m_thread);
  }
}



void
test_thread_create_start_wait_cancel_destroy(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0;
  for (i = 0; i < N_LOOP; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_create(&m_thread, LAGOPUS_RESULT_OK),
                              "create error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_start(&m_thread, LAGOPUS_RESULT_OK),
                              "start error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_wait(&m_thread, LAGOPUS_RESULT_OK),
                              "wait error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_cancel(&m_thread, LAGOPUS_RESULT_OK),
                              "cancel error");
    s_check_destroy(&m_thread);
  }
}

void
test_thread_create_start_cancel_wait_destroy(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0;
  for (i = 0; i < N_LOOP; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_create(&m_thread, LAGOPUS_RESULT_OK),
                              "create error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_start(&m_thread, LAGOPUS_RESULT_OK),
                              "start error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_cancel(&m_thread, LAGOPUS_RESULT_OK),
                              "cancel error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_wait(&m_thread, LAGOPUS_RESULT_OK),
                              "wait error");
    s_check_destroy(&m_thread);
  }
}

void
test_thread_create_wait_destroy(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0;
  for (i = 0; i < N_LOOP; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_create(&m_thread, LAGOPUS_RESULT_OK),
                              "create error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_wait(&m_thread, LAGOPUS_RESULT_OK),
                              "wait error");
    s_check_destroy(&m_thread);
  }
}

void
test_thread_create_cancel_destroy(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0;
  for (i = 0; i < N_LOOP; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_create(&m_thread, LAGOPUS_RESULT_OK),
                              "create error");
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_cancel(&m_thread, LAGOPUS_RESULT_OK),
                              "cancel error");
    s_check_destroy(&m_thread);
  }
}



void
test_thread_create_params(void) {
  lagopus_thread_t m_thread = NULL;
  TEST_ASSERT_EQUAL_MESSAGE(
    LAGOPUS_RESULT_INVALID_ARGS,
    lagopus_thread_create(NULL, main_proc, finalize_proc,
                          freeup_proc, "m_thread", NULL),
    "thread is null");
  lagopus_thread_destroy(&m_thread);

  m_thread = NULL;
  TEST_ASSERT_EQUAL_MESSAGE(
    LAGOPUS_RESULT_INVALID_ARGS,
    lagopus_thread_create(&m_thread, NULL, finalize_proc,
                          freeup_proc, "m_thread", NULL),
    "main_proc is null");
  lagopus_thread_destroy(&m_thread);

  m_thread = NULL;
  TEST_ASSERT_EQUAL_MESSAGE(
    LAGOPUS_RESULT_OK,
    lagopus_thread_create(&m_thread, main_proc, NULL,
                          freeup_proc, "m_thread", NULL),
    "finalize_proc is null");
  lagopus_thread_destroy(&m_thread);

  m_thread = NULL;
  TEST_ASSERT_EQUAL_MESSAGE(
    LAGOPUS_RESULT_OK,
    lagopus_thread_create(&m_thread, main_proc, finalize_proc,
                          NULL, "m_thread", NULL),
    "freeup_proc is null");
  lagopus_thread_destroy(&m_thread);

  m_thread = NULL;
  TEST_ASSERT_EQUAL_MESSAGE(
    LAGOPUS_RESULT_OK,
    lagopus_thread_create(&m_thread, main_proc, finalize_proc,
                          freeup_proc, NULL, NULL),
    "thread_name is null");
  lagopus_thread_destroy(&m_thread);
}

void
test_thread_start_params(void) {
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            lagopus_thread_start(NULL, false),
                            "thread is NULL");

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            lagopus_thread_start(NULL, true),
                            "thread is NULL");
}


void
test_thread_wait_params(void) {
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            lagopus_thread_wait(NULL, WAIT_NSEC),
                            "thread is NULL");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            lagopus_thread_wait(NULL, -1),
                            "thread is NULL");
}

void
test_thread_cancel_params(void) {
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            lagopus_thread_cancel(NULL),
                            "thread is NULL");
}

void
test_thread_param_destroy(void) {
  lagopus_thread_t m_thread = NULL;
  lagopus_thread_destroy(NULL);
  lagopus_thread_destroy(&m_thread);
}

void
test_thread_start_wait_loop(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0, j = 0;
  for (j = 0; j < N_LOOP; j++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_create(&m_thread, LAGOPUS_RESULT_OK),
                              "create error");
    for (i = 0; i < 3; i++) {
      TEST_ASSERT_EQUAL_MESSAGE(true, s_check_start(&m_thread, LAGOPUS_RESULT_OK),
                                "start error");
      usleep(10000);
      TEST_ASSERT_EQUAL_MESSAGE(true, s_check_wait(&m_thread, LAGOPUS_RESULT_OK),
                                "wait error");
      usleep(10000);
    }
    s_check_destroy(&m_thread);
    usleep(10000);
  }
}

void
test_thread_start_wait_cancel_loop(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0, j = 0;
  for (j = 0; j < N_LOOP; j++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_create(&m_thread, LAGOPUS_RESULT_OK),
                              "create error");
    for (i = 0; i < 3; i++) {
      TEST_ASSERT_EQUAL_MESSAGE(true, s_check_start(&m_thread, LAGOPUS_RESULT_OK),
                                "start error");
      usleep(10000);
      TEST_ASSERT_EQUAL_MESSAGE(true, s_check_wait(&m_thread, LAGOPUS_RESULT_OK),
                                "wait error");
      usleep(10000);
      TEST_ASSERT_EQUAL_MESSAGE(true, s_check_cancel(&m_thread, LAGOPUS_RESULT_OK),
                                "cancel error");
      usleep(10000);
    }
    s_check_destroy(&m_thread);
    usleep(10000);
  }
}

void
test_thread_start_cancel_wait_loop(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0, j = 0;
  for (j = 0; j < N_LOOP; j++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_create(&m_thread, LAGOPUS_RESULT_OK),
                              "create error");
    for (i = 0; i < 3; i++) {
      TEST_ASSERT_EQUAL_MESSAGE(true, s_check_start(&m_thread, LAGOPUS_RESULT_OK),
                                "start error");
      usleep(10000);
      TEST_ASSERT_EQUAL_MESSAGE(true, s_check_cancel(&m_thread, LAGOPUS_RESULT_OK),
                                "cancel error");
      usleep(10000);
      TEST_ASSERT_EQUAL_MESSAGE(true, s_check_wait(&m_thread, LAGOPUS_RESULT_OK),
                                "wait error");
      usleep(10000);
    }
    s_check_destroy(&m_thread);
    usleep(10000);
  }
}

void
test_thread_start_cancel_loop(void) {
  lagopus_thread_t m_thread = NULL;
  int i = 0, j = 0;
  for (j = 0; j < N_LOOP; j++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, s_check_create(&m_thread, LAGOPUS_RESULT_OK),
                              "create error");
    for (i = 0; i < 3; i++) {
      TEST_ASSERT_EQUAL_MESSAGE(true, s_check_start(&m_thread, LAGOPUS_RESULT_OK),
                                "start error");
      usleep(10000);
      TEST_ASSERT_EQUAL_MESSAGE(true, s_check_cancel(&m_thread, LAGOPUS_RESULT_OK),
                                "cancel error");
      usleep(10000);
    }
    s_check_destroy(&m_thread);
    usleep(10000);
  }
}

void
test_thread_shutdown_and_freeup_procs(void) {
  lagopus_thread_t m_thread = NULL;
  /* wait for previous test's thread_freeup_proc exits completely. */
  sleep(1);

  /* init counter */
  s_freeup_cnt = 0;
  s_finalize_cnt = 0;
  TEST_ASSERT_EQUAL_MESSAGE(0, s_finalize_cnt, "check finalize");
  TEST_ASSERT_EQUAL_MESSAGE(0, s_freeup_cnt, "check freeup");

  /* create & start thread */
  TEST_ASSERT_EQUAL_MESSAGE(true, s_check_create(&m_thread, LAGOPUS_RESULT_OK),
                            "create error");
  TEST_ASSERT_EQUAL_MESSAGE(true, s_check_start(&m_thread, LAGOPUS_RESULT_OK),
                            "start error");
  usleep(10000);

  /* check counter still 0. */
  TEST_ASSERT_EQUAL_MESSAGE(0, s_finalize_cnt, "check finalize");
  TEST_ASSERT_EQUAL_MESSAGE(0, s_freeup_cnt, "check freeup");

  /* shutdown. */
  TEST_ASSERT_EQUAL_MESSAGE(true, s_check_wait(&m_thread, LAGOPUS_RESULT_OK),
                            "wait error");
  usleep(10000);

  /* check counter */
  TEST_ASSERT_EQUAL_MESSAGE(1, s_finalize_cnt, "check finalize");
  TEST_ASSERT_EQUAL_MESSAGE(0, s_freeup_cnt, "check freeup");

  /* destroy. */
  s_check_destroy(&m_thread);
  usleep(10000);

  /* check counter */
  TEST_ASSERT_EQUAL_MESSAGE(1, s_finalize_cnt, "check finalize");
  TEST_ASSERT_EQUAL_MESSAGE(1, s_freeup_cnt, "check freeup");
}

void
test_thread_freeup_autodelete(void) {
  lagopus_thread_t m_thread = NULL;
  /* wait for previous test's thread_freeup_proc exits completely. */
  sleep(1);

  /* init counter */
  s_freeup_cnt = 0;
  s_finalize_cnt = 0;
  TEST_ASSERT_EQUAL_MESSAGE(0, s_finalize_cnt, "check finalize");
  TEST_ASSERT_EQUAL_MESSAGE(0, s_freeup_cnt, "check freeup");

  /* create */
  TEST_ASSERT_EQUAL_MESSAGE(true,
                            s_check_create_autodelete(&m_thread, LAGOPUS_RESULT_OK),
                            "create error");
  TEST_ASSERT_EQUAL_MESSAGE(true, s_check_start_autodelete(&m_thread,
                            LAGOPUS_RESULT_OK),
                            "start_autodelete error");
  usleep(10000);

  /* check counter */
  TEST_ASSERT_EQUAL_MESSAGE(0, s_finalize_cnt, "check finalize");
  TEST_ASSERT_EQUAL_MESSAGE(0, s_freeup_cnt, "check freeup");

  /* wait for shutdowned & destroyed. */
  sleep(10);

  /* check counter */
  TEST_ASSERT_EQUAL_MESSAGE(1, s_finalize_cnt, "check finalize");
  TEST_ASSERT_EQUAL_MESSAGE(1, s_freeup_cnt, "check freeup");
}
