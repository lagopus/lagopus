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





static volatile bool s_do_loop = false;
static volatile bool s_is_started = false;
static volatile bool s_is_initialized = false;
static volatile bool s_is_gracefull = false;
static lagopus_thread_t s_thd = NULL;
static lagopus_mutex_t s_lck = NULL;

static int s_exit_code = 0;


static void
s_dummy_thd_finalize(const lagopus_thread_t *tptr, bool is_canceled,
                     void *arg) {
  (void)tptr;
  (void)arg;

  if (is_canceled == true) {
    if (s_is_started == false) {
      /*
       * Means this thread is canceled while waiting for the global
       * state change.
       */
      global_state_cancel_janitor();
    }
  }

  lagopus_msg_debug(5, "called, %s.\n",
                    (is_canceled == true) ? "canceled" : "exited");
}


static void
s_dummy_thd_destroy(const lagopus_thread_t *tptr, void *arg) {
  (void)tptr;
  (void)arg;

  lagopus_msg_debug(5, "called.\n");
}


static lagopus_result_t
s_dummy_thd_main(const lagopus_thread_t *tptr, void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  global_state_t s;
  shutdown_grace_level_t l;

  (void)tptr;
  (void)arg;

  if (s_exit_code == 2) {
    exit(2);
  }

  lagopus_msg_debug(5, "waiting for the gala opening...\n");
  
  ret = global_state_wait_for(GLOBAL_STATE_STARTED, &s, &l, -1LL);
  if (ret == LAGOPUS_RESULT_OK &&
      s == GLOBAL_STATE_STARTED) {

    s_is_started = true;

    lagopus_msg_debug(5, "gala opening.\n");

    /*
     * The main task loop.
     */
    while (s_do_loop == true) {
      lagopus_msg_debug(6, "looping...\n");
      (void)lagopus_chrono_nanosleep(1000LL * 1000LL * 500LL, NULL);
      /*
       * Create an explicit cancalation point since this loop has
       * none of it.
       */
      pthread_testcancel();

      if (s_exit_code == 3) {
        exit(3);
      }
    }
    if (s_is_gracefull == true) {
      /*
       * This is just emulating/mimicking a graceful shutdown by
       * sleep().  Don't do this on actual modules.
       */
      lagopus_msg_debug(5, "mimicking gracefull shutdown...\n");
      sleep(5);
      lagopus_msg_debug(5, "mimicking gracefull shutdown done.\n");
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = 1LL;
    }
  }

  return ret;
}





static inline void
s_lock(void) {
  if (s_lck != NULL) {
    (void)lagopus_mutex_lock(&s_lck);
  }
}


static inline void
s_unlock(void) {
  if (s_lck != NULL) {
    (void)lagopus_mutex_unlock(&s_lck);
  }
}





static inline lagopus_result_t
s_parse_args(int argc, const char * const argv[]) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)argc;

  while (*argv != NULL) {
    if (strcmp("--exit", *argv) == 0) {
      if (IS_VALID_STRING(*(argv + 1)) == true) {
        int32_t tmp = 0;

        if (lagopus_str_parse_int32(*(++argv), &tmp) == LAGOPUS_RESULT_OK) {
          if (tmp >= 0) {
            s_exit_code = tmp;
          } else {
            lagopus_msg_error("The exit code must be >= 0.\n");
            ret = LAGOPUS_RESULT_INVALID_ARGS;
            goto bailout;
          }
        } else {
          lagopus_msg_error("can't parse '%s' as an integer.\n", *argv);
          ret = LAGOPUS_RESULT_INVALID_ARGS;
          goto bailout;
        }
      }
    }
    argv++;
  }
  ret = LAGOPUS_RESULT_OK;
  
bailout:
  return ret;
}


static lagopus_result_t
dummy_module_initialize(int argc,
                        const char *const argv[],
                        void *extarg,
                        lagopus_thread_t **thdptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)extarg;

  lagopus_msg_debug(5, "called.\n");

  if ((ret = s_parse_args(argc, argv)) != LAGOPUS_RESULT_OK) {
    return ret;
  }

  if (s_exit_code == 1) {
    exit(1);
  }

  if (thdptr != NULL) {
    *thdptr = NULL;
  }

  s_lock();
  {
    if (s_is_initialized == false) {
      int i;

      lagopus_msg_debug(5, "argc: %d\n", argc);
      for (i = 0; i < argc; i++) {
        lagopus_msg_debug(5, "%5d: '%s'\n", i, argv[i]);
      }
      
      ret = lagopus_thread_create(&s_thd,
                                  s_dummy_thd_main,
                                  s_dummy_thd_finalize,
                                  s_dummy_thd_destroy,
                                  "dummy", NULL);
      if (ret == LAGOPUS_RESULT_OK) {
        s_is_initialized = true;
        if (thdptr != NULL) {
          *thdptr = &s_thd;
        }
      }
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
  }
  s_unlock();

  return ret;
}


static lagopus_result_t
dummy_module_start(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_debug(5, "called.\n");

  if (s_thd != NULL) {

    s_lock();
    {
      if (s_is_initialized == true) {
        ret = lagopus_thread_start(&s_thd, false);
        if (ret == LAGOPUS_RESULT_OK) {
          s_do_loop = true;
        }
      } else {
        ret = LAGOPUS_RESULT_INVALID_STATE_TRANSITION;
      }
    }
    s_unlock();

  } else {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
  }

  return ret;
}


static lagopus_result_t
dummy_module_shutdown(shutdown_grace_level_t l) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_debug(5, "called.\n");

  if (s_thd != NULL) {

    s_lock();
    {
      if (s_is_initialized == true) {
        if (l == SHUTDOWN_GRACEFULLY) {
          s_is_gracefull = true;
        }
        s_do_loop = false;
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = LAGOPUS_RESULT_INVALID_STATE_TRANSITION;
      }
    }
    s_unlock();

  } else {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
  }

  return ret;
}


static lagopus_result_t
dummy_module_stop(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_debug(5, "called.\n");

  if (s_thd != NULL) {

    s_lock();
    {
      if (s_is_initialized == true) {
        ret = lagopus_thread_cancel(&s_thd);
      } else {
        ret = LAGOPUS_RESULT_INVALID_STATE_TRANSITION;
      }
    }
    s_unlock();

  } else {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
  }

  return ret;
}


static void
dummy_module_finalize(void) {
  lagopus_msg_debug(5, "called.\n");

  if (s_thd != NULL) {

    s_lock();
    {
      lagopus_thread_destroy(&s_thd);
    }
    s_unlock();

  }
}


static void
dummy_module_usage(FILE *fd) {
  if (fd != NULL) {
    fprintf(fd, "\t--exit #\tspecify the exit code/mode.\n");
  }
}





#define MY_MOD_IDX	LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE + 100
#define MY_MOD_NAME	"dummy"


static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static void	s_ctors(void) __attr_constructor__(MY_MOD_IDX);
static void	s_dtors(void) __attr_destructor__(MY_MOD_IDX);


static void
s_once_proc(void) {
  lagopus_result_t r;

  lagopus_msg_debug(5, "called.\n");

  if ((r = lagopus_mutex_create(&s_lck)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize a mutex.\n");
  }

  if ((r = lagopus_module_register(MY_MOD_NAME,
                                   dummy_module_initialize, NULL,
                                   dummy_module_start,
                                   dummy_module_shutdown,
                                   dummy_module_stop,
                                   dummy_module_finalize,
                                   dummy_module_usage)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register the %s module.\n", MY_MOD_NAME);
  }
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  lagopus_msg_debug(5, "called.\n");
}


static inline void
s_final(void) {
  if (s_lck != NULL) {
    lagopus_mutex_destroy(&s_lck);
  }
}


static void
s_dtors(void) {
  s_final();

  lagopus_msg_debug(5, "called.\n");
}
