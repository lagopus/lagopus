/* %COPYRIGHT% */

#include "lagopus_apis.h"





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





typedef	lagopus_result_t (*test_proc_t)(void *arg);


typedef struct {
  test_proc_t m_proc;
  void *m_arg;
} callout_test_task_record;
typedef callout_test_task_record *callout_test_task_t;


/*
 * Actuall test driver.
 */


static lagopus_thread_t s_thd = NULL;


static lagopus_result_t
s_thd_main(const lagopus_thread_t *tptr, void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  global_state_t s;
  shutdown_grace_level_t l;

  (void)tptr;

  ret = global_state_wait_for(GLOBAL_STATE_STARTED, &s, &l, -1LL);
  if (ret == LAGOPUS_RESULT_OK &&
      s == GLOBAL_STATE_STARTED) {
    callout_test_task_t t = (callout_test_task_t)arg;
    if (t != NULL) {
      ret = (t->m_proc)(t->m_arg);
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
  }

  return ret;
}





static uint64_t s_val0 = 0LL;
static uint64_t s_val1 = 0LL;


static lagopus_result_t
callout0(void *arg) {
  uint64_t *v = (uint64_t *)arg;

  lagopus_msg_debug(1, "task0 called.\n");

  (void)__sync_fetch_and_add(v, 1);

#if 0
  (void)lagopus_chrono_nanosleep(2LL * 1000LL * 1000LL * 1000LL, NULL);
#endif

  return LAGOPUS_RESULT_OK;
}


static void
callout0_freeup(void *arg) {
  (void)arg;
  lagopus_msg_debug(1, "freeup0 called.\n");
}


static lagopus_result_t
callout1(void *arg) {
  uint64_t *v = (uint64_t *)arg;

  lagopus_msg_debug(1, "task1 called.\n");

  if (__sync_fetch_and_add(v, 1) > 3) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  } else {
    return LAGOPUS_RESULT_OK;
  }
}


static void
callout1_freeup(void *arg) {
  (void)arg;
  lagopus_msg_debug(1, "freeup1 called.\n");
}


static lagopus_result_t
test_1(void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_callout_task_t t0 = NULL;
  lagopus_callout_task_t t1 = NULL;
  uint64_t val;

  (void)arg;

  s_val0 = 0LL;

  if (likely(((ret = lagopus_callout_create_task(&t0, 0, "test_task0",
                                                 callout0, (void *)&s_val0,
                                                 callout0_freeup)) ==
              LAGOPUS_RESULT_OK) &&

             ((ret = lagopus_callout_create_task(&t1, 0, "test_task1",
                                                 callout1, (void *)&s_val1,
                                                 callout1_freeup)) ==
              LAGOPUS_RESULT_OK))) {

    if (likely(((ret =
                 lagopus_callout_submit_task(&t0,
                                             0LL, 
                                             1000LL * 1000LL * 1000LL)) ==
                LAGOPUS_RESULT_OK) &&

               ((ret =
                 lagopus_callout_submit_task(&t1,
                                             10LL * 1000LL * 1000LL * 1000LL, 
                                             300LL * 1000LL * 1000LL)) ==
                LAGOPUS_RESULT_OK))) {

      (void)lagopus_chrono_nanosleep(1500LL * 1000LL * 1000LL, NULL);

      (void)lagopus_callout_exec_task_forcibly(&t0);
      (void)lagopus_callout_exec_task_forcibly(&t1);

      (void)lagopus_chrono_nanosleep(4LL * 1000LL * 1000LL * 1000LL, NULL);

      lagopus_callout_cancel_task(&t0);

      (void)lagopus_chrono_nanosleep(1LL * 1000LL * 1000LL * 1000LL, NULL);

      val = __sync_fetch_and_add(&s_val0, 0);

      fprintf(stdout, "\nval = " PFSZ(u) ".\n", val);
      ret = LAGOPUS_RESULT_OK;
    }
  }

  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

  return ret;
}


int
main(int argc, const char * const argv[]) {
  size_t n_workers = 1;
  callout_test_task_record t = {
    test_1, NULL
  };
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (argc > 1 && IS_VALID_STRING(argv[1]) == true) {
    size_t tmp = 0;
    if (lagopus_str_parse_uint64(argv[1], &tmp) == LAGOPUS_RESULT_OK) {
      if (tmp <= 4) {
        n_workers = tmp;
      }
    }
  }

  (void)lagopus_mainloop_set_callout_workers_number(n_workers);
  fprintf(stderr, "workers: " PFSZ(u) ".\n", n_workers);

  (void)lagopus_signal(SIGINT, s_term_handler, NULL);
  (void)lagopus_signal(SIGTERM, s_term_handler, NULL);
  (void)lagopus_signal(SIGQUIT, s_term_handler, NULL);

  ret = lagopus_thread_create(&s_thd,
                              s_thd_main,
                              NULL,
                              NULL,
                              "test", (void *)&t);
  if (likely(ret == LAGOPUS_RESULT_OK)) {
    ret = lagopus_thread_start(&s_thd, false);
    if (unlikely(ret != LAGOPUS_RESULT_OK)) {
      goto done;
    }
  }

  ret = lagopus_mainloop_with_callout(argc, argv, NULL, NULL,
                                      false, false, false);

done:
  return (ret == LAGOPUS_RESULT_OK) ? 0 : 1;
}
