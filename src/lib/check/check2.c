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


#


typedef struct {
  lagopus_thread_record m_thd;	/* must be on the head. */

  lagopus_mutex_t m_lock;

  bool m_is_operational;
  bool m_stop_requested;
  size_t m_n_data;
  int *m_data;
} test_thread_record;
typedef test_thread_record *test_thread_t;





static bool s_is_deleted = false;





static void
s_test_thread_finalize(const lagopus_thread_t *tptr, bool is_canceled,
                       void *arg) {
  (void)arg;

  if (tptr != NULL) {
    test_thread_t tt = (test_thread_t)*tptr;

    if (tt != NULL) {
      lagopus_mutex_lock(&(tt->m_lock));
      {
        tt->m_is_operational = false;
      }
      lagopus_mutex_unlock(&(tt->m_lock));

      lagopus_msg_debug(1, "enter (%s).\n",
                        (is_canceled == true) ? "canceled" : "exited");
    }
  }
}


static void
s_test_thread_destroy(const lagopus_thread_t *tptr, void *arg) {
  (void)arg;

  if (tptr != NULL) {
    test_thread_t tt = (test_thread_t)*tptr;

    if (tt != NULL) {
      (void)lagopus_mutex_lock(&(tt->m_lock));
      {
        lagopus_msg_debug(1, "enter.\n");
        if (tt->m_is_operational == false) {
          lagopus_msg_debug(1, "freeup.\n");
          free((void *)tt->m_data);
          tt->m_data = NULL;
        }
        s_is_deleted = true;
      }
      (void)lagopus_mutex_unlock(&(tt->m_lock));
    }

    lagopus_mutex_destroy(&(tt->m_lock));
  }
}


static lagopus_result_t
s_test_thread_main(const lagopus_thread_t *tptr, void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)arg;
  if (tptr != NULL) {
    test_thread_t tt = (test_thread_t)*tptr;

    if (tt != NULL) {
      while (tt->m_stop_requested == false) {
        lagopus_msg_debug(1, "will sleep a sec...\n");
        (void)lagopus_chrono_nanosleep(1000 * 1000 * 1000, NULL);
      }
      ret = LAGOPUS_RESULT_OK;
    }
  }

  return ret;
}


static inline bool
s_initialize(test_thread_t tt, size_t n) {
  bool ret = false;

  if (tt != NULL) {
    tt->m_data = (int *)malloc(n * sizeof(int));
    if (tt->m_data != NULL) {
      lagopus_result_t r;
      if ((r = lagopus_mutex_create(&(tt->m_lock))) != LAGOPUS_RESULT_OK) {
        lagopus_perror(r);
        goto initfailure;
      }
      if ((r = lagopus_thread_create((lagopus_thread_t *)&tt,
                                     s_test_thread_main,
                                     s_test_thread_finalize,
                                     s_test_thread_destroy,
                                     "test", NULL)) != LAGOPUS_RESULT_OK) {
        lagopus_perror(r);
        goto initfailure;
      }
      tt->m_n_data = n;
      tt->m_is_operational = false;
      tt->m_stop_requested = false;

      ret = true;
    }
  initfailure:
    if (ret != true) {
      free((void *)tt->m_data);
    }
  }

  return ret;
}





static inline bool
test_thread_create(test_thread_t *ttptr, size_t n) {
  bool ret = false;

  if (ttptr != NULL) {
    test_thread_t tt;
    if (*ttptr == NULL) {
      tt = (test_thread_t)malloc(sizeof(*tt));
      if (tt == NULL) {
        goto done;
      }
    } else {
      tt = *ttptr;
    }
    ret = s_initialize(tt, n);
    if (*ttptr == NULL && ret == true) {
      *ttptr = tt;
      (void)lagopus_thread_free_when_destroy((lagopus_thread_t *)&tt);
    }
  }

done:
  return ret;
}


static inline void
test_thread_request_stop(test_thread_t tt) {
  if (tt != NULL) {
    (void)lagopus_mutex_lock(&(tt->m_lock));
    {
      tt->m_stop_requested = true;
    }
    (void)lagopus_mutex_unlock(&(tt->m_lock));
  }
}





int
main(int argc, const char *const argv[]) {
  test_thread_record ttr;
  test_thread_t tt;
  bool is_canceled;
  int ret = 1;

  (void)argc;
  (void)argv;

  fprintf(stderr, "Pthread wrapper check: start\n\n");

  /*
   * Heap object
   */
  fprintf(stderr, "Heap object, regular: start\n");
  tt = NULL;
  if (test_thread_create(&tt, 100) != true) {
    lagopus_exit_fatal("Can't create a test thread object.\n");
  }
  if (lagopus_thread_start((lagopus_thread_t *)&tt, false) !=
      LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't start a thread.\n");
  }
  lagopus_chrono_nanosleep(5LL * 1000LL * 1000LL * 1000LL, NULL);
  test_thread_request_stop(tt);
  if (lagopus_thread_wait((lagopus_thread_t *)&tt, -1) !=
      LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't wait the thread.\n");
  }
  lagopus_thread_destroy((lagopus_thread_t *)&tt);
  fprintf(stderr, "Heap object, regular: end\n\n");

  /*
   * Stack object
   */
  fprintf(stderr, "Stack object, regular: start\n");
  tt = &ttr;
  if (test_thread_create(&tt, 100) != true) {
    lagopus_exit_fatal("Can't initialize a test thread object.\n");
  }
  if (lagopus_thread_start((lagopus_thread_t *)&tt, false) !=
      LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't start a thread.\n");
  }
  lagopus_chrono_nanosleep(5LL * 1000LL * 1000LL * 1000LL, NULL);
  test_thread_request_stop(tt);
  if (lagopus_thread_wait((lagopus_thread_t *)&tt, -1) !=
      LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't wait the thread.\n");
  }
  lagopus_thread_destroy((lagopus_thread_t *)&tt);
  fprintf(stderr, "Stack object, regular: end\n\n");

  /*
   * Cancel
   */
  fprintf(stderr, "Stack object, cancel: start\n");
  tt = &ttr;
  if (test_thread_create(&tt, 100) != true) {
    lagopus_exit_fatal("Can't initialize a test thread object.\n");
  }
  if (lagopus_thread_start((lagopus_thread_t *)&tt, false) !=
      LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't start a thread.\n");
  }
  lagopus_chrono_nanosleep(5LL * 1000LL * 1000LL * 1000LL, NULL);
  if (lagopus_thread_cancel((lagopus_thread_t *)&tt) != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't cancel the thread.\n");
  }
  if (lagopus_thread_wait((lagopus_thread_t *)&tt, -1) !=
      LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't wait the thread.\n");
  }
  if (lagopus_thread_is_canceled((lagopus_thread_t *)&tt, &is_canceled) !=
      LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("The returned value must be \"canceled\".\n");
  }
  lagopus_thread_destroy((lagopus_thread_t *)&tt);
  fprintf(stderr, "Stack object, cancel: end\n\n");

  /*
   * Heap object auto deletion
   */
  fprintf(stderr, "Heap object, auto deletion: start\n");
  s_is_deleted = false;
  tt = NULL;
  if (test_thread_create(&tt, 100) != true) {
    lagopus_exit_fatal("Can't initialize a test thread object.\n");
  }
  if (lagopus_thread_start((lagopus_thread_t *)&tt, true) !=
      LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't start a thread.\n");
  }
  lagopus_chrono_nanosleep(5LL * 1000LL * 1000LL * 1000LL, NULL);
  test_thread_request_stop(tt);
  lagopus_chrono_nanosleep(2LL * 1000LL * 1000LL * 1000LL, NULL);
  if (s_is_deleted == false) {
    lagopus_exit_fatal("The thread object must be freed and "
                       "the flag must be true.\n");
  }
  fprintf(stderr, "Heap object, auto deletion: end\n\n");

  /*
   * Stack object auto deletion
   */
  fprintf(stderr, "Stack object, auto deletion: start\n");
  s_is_deleted = false;
  tt = &ttr;
  if (test_thread_create(&tt, 100) != true) {
    lagopus_exit_fatal("Can't initialize a test thread object.\n");
  }
  if (lagopus_thread_start((lagopus_thread_t *)&tt, true) !=
      LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't start a thread.\n");
  }
  lagopus_chrono_nanosleep(5LL * 1000LL * 1000LL * 1000LL, NULL);
  test_thread_request_stop(tt);
  lagopus_chrono_nanosleep(2LL * 1000LL * 1000LL * 1000LL, NULL);
  if (s_is_deleted == false) {
    lagopus_exit_fatal("The thread object must be freed and "
                       "the flag must be true.\n");
  }
  fprintf(stderr, "Stack object, auto deletion: end\n\n");

  /*
   * Heap object cancel, auto deletion
   */
  fprintf(stderr, "Heap object, cancel, auto deletion: start\n");
  s_is_deleted = false;
  tt = NULL;
  if (test_thread_create(&tt, 100) != true) {
    lagopus_exit_fatal("Can't initialize a test thread object.\n");
  }
  if (lagopus_thread_start((lagopus_thread_t *)&tt, true) !=
      LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't start a thread.\n");
  }
  lagopus_chrono_nanosleep(5LL * 1000LL * 1000LL * 1000LL, NULL);
  if (lagopus_thread_cancel((lagopus_thread_t *)&tt) != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't cancel the thread.\n");
  }
  lagopus_chrono_nanosleep(2LL * 1000LL * 1000LL * 1000LL, NULL);
  if (s_is_deleted == false) {
    lagopus_exit_fatal("The thread object must be freed and "
                       "the flag must be true.\n");
  }
  fprintf(stderr, "Heap object, cancel, auto deletion: end\n\n");

  /*
   * Stack object cancel, auto deletion
   */
  fprintf(stderr, "Stack object, cancel, auto deletion: start\n");
  s_is_deleted = false;
  tt = &ttr;
  if (test_thread_create(&tt, 100) != true) {
    lagopus_exit_fatal("Can't initialize a test thread object.\n");
  }
  if (lagopus_thread_start((lagopus_thread_t *)&tt, true) !=
      LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't start a thread.\n");
  }
  lagopus_chrono_nanosleep(5LL * 1000LL * 1000LL * 1000LL, NULL);
  if (lagopus_thread_cancel((lagopus_thread_t *)&tt) != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't cancel the thread.\n");
  }
  lagopus_chrono_nanosleep(2LL * 1000LL * 1000LL * 1000LL, NULL);
  if (s_is_deleted == false) {
    lagopus_exit_fatal("The thread object must be freed and "
                       "the flag must be true.\n");
  }
  fprintf(stderr, "Stack object, cancel, auto deletion: end\n\n");

  /*
   * Timed wait
   */
  fprintf(stderr, "Timed wait: start\n");
  tt = &ttr;
  if (test_thread_create(&tt, 100) != true) {
    lagopus_exit_fatal("Can't initialize a test thread object.\n");
  }
  if (lagopus_thread_start((lagopus_thread_t *)&tt, false) !=
      LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't start a thread.\n");
  }
  if (lagopus_thread_wait((lagopus_thread_t *)&tt,
                          5LL * 1000LL * 1000LL * 1000LL) !=
      LAGOPUS_RESULT_TIMEDOUT) {
    lagopus_exit_fatal("Must be timed out.\n");
  }
  test_thread_request_stop(tt);
  if (lagopus_thread_wait((lagopus_thread_t *)&tt, -1) !=
      LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't wait the thread.\n");
  }
  lagopus_thread_destroy((lagopus_thread_t *)&tt);
  fprintf(stderr, "Timed wait: end\n\n");

  /*
   * Force destroy
   */
  fprintf(stderr, "Force destroy: start\n");
  s_is_deleted = false;
  tt = NULL;
  if (test_thread_create(&tt, 100) != true) {
    lagopus_exit_fatal("Can't create a test thread object.\n");
  }
  if (lagopus_thread_start((lagopus_thread_t *)&tt, false) !=
      LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't start a thread.\n");
  }
  lagopus_thread_destroy((lagopus_thread_t *)&tt);
  if (s_is_deleted == false) {
    lagopus_exit_fatal("The thread object must be freed and "
                       "the flag must be true.\n");
  }
  fprintf(stderr, "Force destroy: end\n\n");

  /*
   * Force destroy, not started
   */
  fprintf(stderr, "Force destroy, not started: start\n");
  s_is_deleted = false;
  tt = NULL;
  if (test_thread_create(&tt, 100) != true) {
    lagopus_exit_fatal("Can't create a test thread object.\n");
  }
  lagopus_thread_destroy((lagopus_thread_t *)&tt);
  if (s_is_deleted == false) {
    lagopus_exit_fatal("The thread object must be freed and "
                       "the flag must be true.\n");
  }
  fprintf(stderr, "Force destroy, not started: end\n\n");

  ret = 0;
  fprintf(stderr, "Pthread wrapper check: end\n");

  return ret;
}
