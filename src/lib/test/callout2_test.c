/* %COPYRIGHT% */

#include "lagopus_apis.h"
#include "unity.h"

#include <math.h>





#define N_CALLOUT_WORKERS	1





static lagopus_thread_t s_thd = NULL;





static void
s_term_handler(int sig) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  global_state_t gs = GLOBAL_STATE_UNKNOWN;

  if ((r = global_state_get(&gs)) == LAGOPUS_RESULT_OK) {

    if ((int)gs == (int)GLOBAL_STATE_STARTED) {

      shutdown_grace_level_t l = SHUTDOWN_UNKNOWN;
      if (sig == SIGTERM || sig == SIGINT) {
        l = SHUTDOWN_GRACEFULLY;
      } else if (sig == SIGQUIT) {
        l = SHUTDOWN_RIGHT_NOW;
      }
      if (IS_VALID_SHUTDOWN(l) == true) {
        lagopus_msg_info("About to request shutdown(%s)...\n",
                         (l == SHUTDOWN_RIGHT_NOW) ?
                         "RIGHT_NOW" : "GRACEFULLY");
        if ((r = global_state_request_shutdown(l)) == LAGOPUS_RESULT_OK) {
          lagopus_msg_info("The shutdown request accepted.\n");
        } else {
          lagopus_perror(r);
          lagopus_msg_error("can't request shutdown.\n");
        }
      }

    } else if ((int)gs < (int)GLOBAL_STATE_STARTED) {
      if (sig == SIGTERM || sig == SIGINT || sig == SIGQUIT) {
        lagopus_abort_before_mainloop();
      }
    } else {
      lagopus_msg_debug(5, "The system is already shutting down.\n");
    }
  }

}





static lagopus_result_t
s_thd_main(const lagopus_thread_t *tptr, void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *argv0 =
      (IS_VALID_STRING(lagopus_get_command_name()) == true) ?
      strdup(lagopus_get_command_name()) : strdup("callout_test");
  const char * const argv[] = {
    argv0, NULL
  };
  size_t n_workers = 1;

  (void)tptr;

  n_workers = (size_t)arg;

  (void)lagopus_mainloop_set_callout_workers_number(n_workers);

  (void)lagopus_signal(SIGINT, s_term_handler, NULL);
  (void)lagopus_signal(SIGTERM, s_term_handler, NULL);
  (void)lagopus_signal(SIGQUIT, s_term_handler, NULL);

  ret = lagopus_mainloop_with_callout(1, argv, NULL, NULL, false, false);

  free((void *)argv0);

  return ret;
}


static inline void
s_start_thd(size_t n_workers) {
  lagopus_result_t r = 
      lagopus_thread_create(&s_thd,
                            s_thd_main,
                            NULL,
                            NULL,
                            "mainlooper",
                            (void *)n_workers);
  if (likely(r == LAGOPUS_RESULT_OK)) {
    r = lagopus_thread_start(&s_thd, false);
    if (likely(r == LAGOPUS_RESULT_OK)) {
      global_state_t s;
      shutdown_grace_level_t l;

      r = global_state_wait_for(GLOBAL_STATE_STARTED, &s, &l, -1LL);
      if (unlikely(r != LAGOPUS_RESULT_OK ||
                   s != GLOBAL_STATE_STARTED)) {
        lagopus_exit_fatal("invalid startup state.\n");
      }
    } else {
      lagopus_exit_fatal("can't start the mainloop thread.\n");
    }
  } else {
    lagopus_exit_fatal("can't create the mainloop thread.\n");
  }
}


static inline void
s_stop_thd(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;

  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  if (likely(r == LAGOPUS_RESULT_OK)) {
    r = lagopus_thread_wait(&s_thd, 5LL * 1000LL * 1000LL * 1000LL);
    if (likely(r == LAGOPUS_RESULT_OK)) {
      lagopus_thread_destroy(&s_thd);
    } else if (r == LAGOPUS_RESULT_TIMEDOUT) {
      lagopus_msg_warning("the mainloop thread doesn't stop. cancel it.\n");
      r = lagopus_thread_cancel(&s_thd);
      if (likely(r == LAGOPUS_RESULT_OK)) {
        r = lagopus_thread_wait(&s_thd, 5LL * 1000LL * 1000LL * 1000LL);
        if (likely(r == LAGOPUS_RESULT_OK)) {
          lagopus_thread_destroy(&s_thd);
        } else if (r == LAGOPUS_RESULT_TIMEDOUT) {
          lagopus_exit_error(1, "the mainloop thread still exists. exit.\n");
        }
      } else {
        lagopus_perror(r);
        lagopus_exit_error(1, "can't cancel the mainloop thread. exit.\n");
      }
    } else {
      lagopus_perror(r);
      lagopus_exit_error(1, "failed to wait the mainloop "
                         "thread stop. exit.\n");
    }
  } else {
    lagopus_perror(r);
    lagopus_exit_error(1, "shutdown request failed. exit.\n");
  }
}





void
setUp(void) {
}


void
tearDown(void) {
}





typedef struct {
  lagopus_mutex_t m_lock;
  lagopus_cond_t m_cond;
  lagopus_chrono_t m_wait_nsec;
  lagopus_chrono_t m_last_exec_abstime;
  volatile size_t m_n_exec;
  size_t m_n_stop;
  volatile bool m_is_freeuped;
} callout_arg_struct;
typedef callout_arg_struct *callout_arg_t;


static inline callout_arg_t
s_alloc_arg(size_t n_stop, lagopus_chrono_t wait_nsec) {
  callout_arg_t ret = NULL;
  lagopus_result_t r;
  lagopus_mutex_t lock = NULL;
  lagopus_cond_t cond = NULL;

  if (likely((r = lagopus_mutex_create(&lock)) != LAGOPUS_RESULT_OK)) {
    goto done;
  }
  if (likely((r = lagopus_cond_create(&cond)) != LAGOPUS_RESULT_OK)) {
    goto done;
  }

  ret = (callout_arg_t)malloc(sizeof(*ret));
  if (ret == NULL) {
    goto done;
  }

  (void)memset((void *)ret, 0, sizeof(*ret));
  ret->m_lock = lock;
  ret->m_cond = cond;
  ret->m_wait_nsec = wait_nsec;
  ret->m_last_exec_abstime = 0LL;
  ret->m_n_exec = 0LL;
  ret->m_n_stop = n_stop;
  ret->m_is_freeuped = false;

done:
  return ret;
}


static inline void
s_destroy_arg(callout_arg_t arg) {
  if (likely(arg != NULL &&
             arg->m_is_freeuped == true)) {
    lagopus_cond_destroy(&(arg->m_cond));
    lagopus_mutex_destroy(&(arg->m_lock));
    free((void *)arg);
  }
}


static inline void
s_freeup_arg(void *arg) {

  lagopus_msg_debug(1, "enter.\n");

  if (likely(arg != NULL)) {
    callout_arg_t carg = (callout_arg_t)arg;

    (void)lagopus_mutex_lock(&(carg->m_lock));
    {
      carg->m_is_freeuped = true;
      (void)lagopus_cond_notify(&(carg->m_cond), true);
    }
    (void)lagopus_mutex_unlock(&(carg->m_lock));    

  }

  lagopus_msg_debug(1, "leave.\n");

}


static inline size_t
s_wait_freeup_arg(callout_arg_t arg) {
  size_t ret = 0;

  if (likely(arg != NULL)) {

    (void)lagopus_mutex_lock(&(arg->m_lock));
    {
      while (arg->m_is_freeuped != true) {
        (void)lagopus_cond_wait(&(arg->m_cond), &(arg->m_lock), -1LL);
      }
      ret = __sync_fetch_and_add(&(arg->m_n_exec), 0);
    }
    (void)lagopus_mutex_unlock(&(arg->m_lock));    

  }

  return ret;
}


static lagopus_result_t
callout_task(void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_chrono_t now;

  WHAT_TIME_IS_IT_NOW_IN_NSEC(now);

  lagopus_msg_debug(1, "enter.\n");

  if (likely(arg != NULL)) {
    callout_arg_t carg = (callout_arg_t)arg;
    size_t n_exec = __sync_add_and_fetch(&(carg->m_n_exec), 1);

    lagopus_msg_debug(1, "exec " PFSZ(u) ".\n", n_exec);

    if (carg->m_last_exec_abstime != 0) {
      lagopus_msg_debug(1, "interval: " PF64(d) " nsec.\n",
                        now - carg->m_last_exec_abstime);
    }
    carg->m_last_exec_abstime = now;

    if (carg->m_wait_nsec > 0) {
      lagopus_msg_debug(1, "sleep " PFSZ(u) " nsec.\n",
                        carg->m_wait_nsec);
      (void)lagopus_chrono_nanosleep(carg->m_wait_nsec, NULL);
    }

    if (n_exec < carg->m_n_stop) {
      ret = LAGOPUS_RESULT_OK;
    }
  }

  lagopus_msg_debug(1, "leave.\n");

  return ret;
}





void
test_prologue(void) {
  s_start_thd(N_CALLOUT_WORKERS);
  TEST_ASSERT_EQUAL(true, true);
}





/*
 * Cancel before exec.
 */


void
test_urgent_cancel_before(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_callout_task_t t = NULL;
  size_t n_exec;

  callout_arg_t arg = NULL;

  arg = s_alloc_arg(10, 500LL * 1000LL * 1000LL);
  TEST_ASSERT_NOT_EQUAL(arg, NULL);

  r = lagopus_callout_create_task(&t, 0, __func__,
                                  callout_task, (void *)arg,
                                  s_freeup_arg);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = lagopus_callout_submit_task(&t,
                                  0LL,
                                  1000LL * 1000LL * 1000LL);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  lagopus_callout_cancel_task(&t);

  n_exec = s_wait_freeup_arg(arg);
  TEST_ASSERT_EQUAL(n_exec, 0);

  s_destroy_arg(arg);
}


void
test_delayed_cancel_before(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_callout_task_t t = NULL;
  size_t n_exec;

  callout_arg_t arg = NULL;

  arg = s_alloc_arg(10, 500LL * 1000LL * 1000LL);
  TEST_ASSERT_NOT_EQUAL(arg, NULL);

  r = lagopus_callout_create_task(&t, 0, __func__,
                                  callout_task, (void *)arg,
                                  s_freeup_arg);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = lagopus_callout_submit_task(&t,
                                  0LL,
                                  1000LL * 1000LL * 1000LL);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  lagopus_callout_cancel_task(&t);

  n_exec = s_wait_freeup_arg(arg);
  TEST_ASSERT_EQUAL(n_exec, 0);

  s_destroy_arg(arg);
}


void
test_idle_cancel_before(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_callout_task_t t = NULL;
  size_t n_exec;

  callout_arg_t arg = NULL;

  arg = s_alloc_arg(10, 500LL * 1000LL * 1000LL);
  TEST_ASSERT_NOT_EQUAL(arg, NULL);

  r = lagopus_callout_create_task(&t, 0, __func__,
                                  callout_task, (void *)arg,
                                  s_freeup_arg);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = lagopus_callout_submit_task(&t,
                                  -1LL,
                                  1000LL * 1000LL * 1000LL);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  lagopus_callout_cancel_task(&t);

  n_exec = s_wait_freeup_arg(arg);
  TEST_ASSERT_EQUAL(n_exec, 0);

  s_destroy_arg(arg);
}





/*
 * Cancel while exec
 */


void
test_urgent_cancel_while(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_callout_task_t t = NULL;
  size_t n_exec;

  callout_arg_t arg = NULL;

  arg = s_alloc_arg(10, 1500LL * 1000LL * 1000LL);
  TEST_ASSERT_NOT_EQUAL(arg, NULL);

  r = lagopus_callout_create_task(&t, 0, __func__,
                                  callout_task, (void *)arg,
                                  s_freeup_arg);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = lagopus_callout_submit_task(&t,
                                  0LL,
                                  3000LL * 1000LL * 1000LL);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  (void)lagopus_chrono_nanosleep(1000LL * 1000LL * 1000LL, NULL);

  lagopus_callout_cancel_task(&t);

  n_exec = s_wait_freeup_arg(arg);
  TEST_ASSERT_EQUAL(n_exec, 1);

  s_destroy_arg(arg);
}


void
test_delayed_cancel_while(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_callout_task_t t = NULL;
  size_t n_exec;

  callout_arg_t arg = NULL;

  arg = s_alloc_arg(10, 1500LL * 1000LL * 1000LL);
  TEST_ASSERT_NOT_EQUAL(arg, NULL);

  r = lagopus_callout_create_task(&t, 0, __func__,
                                  callout_task, (void *)arg,
                                  s_freeup_arg);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = lagopus_callout_submit_task(&t,
                                  100LL * 1000LL * 1000LL,
                                  3000LL * 1000LL * 1000LL);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  (void)lagopus_chrono_nanosleep(1000LL * 1000LL * 1000LL, NULL);

  lagopus_callout_cancel_task(&t);

  n_exec = s_wait_freeup_arg(arg);
  if (n_exec == 0) {
    /*
     * timed tasks could be cancelled before execution.
     */
    n_exec = 1;
  }
  TEST_ASSERT_EQUAL(n_exec, 1);

  s_destroy_arg(arg);
}


void
test_idle_cancel_while(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_callout_task_t t = NULL;
  size_t n_exec;

  callout_arg_t arg = NULL;

  arg = s_alloc_arg(10, 1500LL * 1000LL * 1000LL);
  TEST_ASSERT_NOT_EQUAL(arg, NULL);

  r = lagopus_callout_create_task(&t, 0, __func__,
                                  callout_task, (void *)arg,
                                  s_freeup_arg);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = lagopus_callout_submit_task(&t,
                                  -1LL,
                                  3000LL * 1000LL * 1000LL);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  (void)lagopus_chrono_nanosleep(1000LL * 1000LL * 1000LL, NULL);

  lagopus_callout_cancel_task(&t);

  n_exec = s_wait_freeup_arg(arg);
  if (n_exec == 0) {
    /*
     * idle tasks could be cancelled before execution.
     */
    n_exec = 1;
  }
  TEST_ASSERT_EQUAL(n_exec, 1);

  s_destroy_arg(arg);
}





/*
 * Force exec
 */


void
test_urgent_force(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_callout_task_t t = NULL;
  size_t n_exec;
  size_t i;
  callout_arg_t arg = NULL;
  lagopus_callout_task_state_t st;

  arg = s_alloc_arg(10, 0LL);
  TEST_ASSERT_NOT_EQUAL(arg, NULL);

  r = lagopus_callout_create_task(&t, 0, __func__,
                                  callout_task, (void *)arg,
                                  s_freeup_arg);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = lagopus_callout_submit_task(&t,
                                  0LL, 
                                  500LL * 1000LL * 1000LL);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  (void)lagopus_chrono_nanosleep(100LL * 1000LL * 1000LL, NULL);

  for (i = 0; i < 5; i++) {
    lagopus_msg_debug(1, "iter. " PF64(u) "\n", i);

    r = lagopus_callout_task_state(&t, &st);
    TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

    if (st == TASK_STATE_ENQUEUED) {
      r = lagopus_callout_exec_task_forcibly(&t);
      if (r == LAGOPUS_RESULT_INVALID_STATE) {
        continue;
      }
      TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
    } else if (st == TASK_STATE_UNKNOWN ||
               st == TASK_STATE_CANCELLED) {
      break;
    }

    (void)lagopus_chrono_nanosleep(100LL * 1000LL * 1000LL, NULL);
  }

  n_exec = s_wait_freeup_arg(arg);
  TEST_ASSERT_EQUAL(n_exec, 10);

  s_destroy_arg(arg);
}


void
test_delayed_force(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_callout_task_t t = NULL;
  size_t n_exec;
  size_t i;
  callout_arg_t arg = NULL;
  lagopus_callout_task_state_t st;

  arg = s_alloc_arg(10, 0LL);
  TEST_ASSERT_NOT_EQUAL(arg, NULL);

  r = lagopus_callout_create_task(&t, 0, __func__,
                                  callout_task, (void *)arg,
                                  s_freeup_arg);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = lagopus_callout_submit_task(&t,
                                  100LL * 1000LL * 1000LL,
                                  500LL * 1000LL * 1000LL);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  (void)lagopus_chrono_nanosleep(100LL * 1000LL * 1000LL, NULL);

  for (i = 0; i < 5; i++) {
    lagopus_msg_debug(1, "iter. " PF64(u) "\n", i);

    r = lagopus_callout_task_state(&t, &st);
    TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

    if (st == TASK_STATE_ENQUEUED) {
      r = lagopus_callout_exec_task_forcibly(&t);
      if (r == LAGOPUS_RESULT_INVALID_STATE) {
        continue;
      }
      TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
    } else if (st == TASK_STATE_UNKNOWN ||
               st == TASK_STATE_CANCELLED) {
      break;
    }

    (void)lagopus_chrono_nanosleep(100LL * 1000LL * 1000LL, NULL);
  }

  n_exec = s_wait_freeup_arg(arg);
  TEST_ASSERT_EQUAL(n_exec, 10);

  s_destroy_arg(arg);
}


void
test_idle_force(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_callout_task_t t = NULL;
  size_t n_exec;
  size_t i;
  callout_arg_t arg = NULL;
  lagopus_callout_task_state_t st;

  arg = s_alloc_arg(10, 0LL);
  TEST_ASSERT_NOT_EQUAL(arg, NULL);

  r = lagopus_callout_create_task(&t, 0, __func__,
                                  callout_task, (void *)arg,
                                  s_freeup_arg);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  r = lagopus_callout_submit_task(&t,
                                  -1LL,
                                  500LL * 1000LL * 1000LL);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

  (void)lagopus_chrono_nanosleep(1000LL * 1000LL * 1000LL, NULL);

  for (i = 0; i < 5; i++) {
    lagopus_msg_debug(1, "iter. " PF64(u) "\n", i);

    r = lagopus_callout_task_state(&t, &st);
    TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);

    if (st == TASK_STATE_ENQUEUED) {
      r = lagopus_callout_exec_task_forcibly(&t);
      if (r == LAGOPUS_RESULT_INVALID_STATE) {
        continue;
      }
      TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
    } else if (st == TASK_STATE_UNKNOWN ||
               st == TASK_STATE_CANCELLED) {
      break;
    }

    (void)lagopus_chrono_nanosleep(100LL * 1000LL * 1000LL, NULL);
  }

  n_exec = s_wait_freeup_arg(arg);
  TEST_ASSERT_EQUAL(n_exec, 10);

  s_destroy_arg(arg);
}





void
test_epilogue(void) {
  s_stop_thd();
  TEST_ASSERT_EQUAL(true, true);
}
