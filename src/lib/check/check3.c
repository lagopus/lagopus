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

static lagopus_result_t
s_main_proc(lagopus_thread_t *selfptr, void *arg) {
  lagopus_msg("Called with (%p, %p)\n", *selfptr, arg);
  return LAGOPUS_RESULT_OK;
}


static lagopus_result_t
s_main_proc_with_sleep(lagopus_thread_t *selfptr, void *arg) {
  lagopus_msg("Called with (%p, %p)\n", *selfptr, arg);
  sleep(10);
  return LAGOPUS_RESULT_OK;
}

static void
s_freeup_proc(lagopus_thread_t *selfptr, void *arg) {
  lagopus_msg("Called with (%p, %p)\n", *selfptr, arg);
}

static void
s_finalize_proc(lagopus_thread_t *selfptr,
                bool is_canceled, void *arg) {
  lagopus_msg("Called with (%p, %s, %p)\n",
              *selfptr, BOOL_TO_STR(is_canceled), arg);
}

static void
s_check_cancelled(lagopus_thread_t *thread, bool require) {
  bool canceled = !require;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  if ((ret = lagopus_thread_is_canceled(thread, &canceled))
      != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    lagopus_msg_error("Can't check is_cancelled.\n");
  } else if (canceled != require) {
    lagopus_msg_error(
      "is_cancelled() required %s, but %s.\n",
      BOOL_TO_STR(require), BOOL_TO_STR(canceled));
  }
}

int
main(int argc, char const *const argv[]) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int i;
  int n = 10000;
  lagopus_thread_t thread = NULL;
  pthread_t tid = LAGOPUS_INVALID_THREAD;

  lagopus_log_set_debug_level(10);

  if (argc > 1) {
    int tmp;
    if (lagopus_str_parse_int32(argv[1], &tmp) == LAGOPUS_RESULT_OK &&
        tmp > 0) {
      n = tmp;
    }
  }

  /* create, start, wait, destroy */
  for (i = 0; i < n; i++) {
    thread = NULL;

    if ((ret = lagopus_thread_create(
                 (lagopus_thread_t *)&thread,
                 (lagopus_thread_main_proc_t)s_main_proc,
                 (lagopus_thread_finalize_proc_t)s_finalize_proc,
                 (lagopus_thread_freeup_proc_t)s_freeup_proc,
                 (const char *) "thread",
                 (void *) NULL)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      lagopus_exit_fatal("Can't create a thread.\n");
    } else {
      fprintf(stderr, "create a thread [%d]\n", i);
      if ((ret = lagopus_thread_get_pthread_id(&thread, &tid))
          != LAGOPUS_RESULT_NOT_STARTED) {
        lagopus_perror(ret);
        goto done;
      }
    }

    if ((ret = lagopus_thread_start(&thread, false)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      lagopus_exit_fatal("Can't start the thread.\n");
    } else {
      ret = lagopus_thread_get_pthread_id(&thread, &tid);
      if (ret == LAGOPUS_RESULT_OK ||
          ret == LAGOPUS_RESULT_ALREADY_HALTED) {
        fprintf(stderr, " start thread:  %lu\n", tid);
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    s_check_cancelled(&thread, false);

    if ((ret = lagopus_thread_wait(&thread, 1000LL * 1000LL * 1000LL)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      lagopus_msg_error("Can't wait the thread.\n");
    } else {
      if ((ret = lagopus_thread_get_pthread_id(&thread, &tid))
          == LAGOPUS_RESULT_OK) {
        lagopus_msg_error(
          "Thread has been completed, but I can get thread ID, %lu\n",
          tid);
      } else if (ret != LAGOPUS_RESULT_ALREADY_HALTED) {
        lagopus_perror(ret);
        goto done;
      }
    }

    s_check_cancelled(&thread, false);

    lagopus_thread_destroy(&thread);
    fprintf(stderr, "destroy the thread [%d]\n", i);

    fprintf(stderr, "\n");
  }

  /* create, start, cancel, destroy */
  for (i = 0; i < n; i++) {
    thread = NULL;

    if ((ret = lagopus_thread_create(
                 (lagopus_thread_t *)&thread,
                 (lagopus_thread_main_proc_t)s_main_proc_with_sleep,
                 (lagopus_thread_finalize_proc_t)s_finalize_proc,
                 (lagopus_thread_freeup_proc_t)s_freeup_proc,
                 (const char *) "thread",
                 (void *) NULL)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      lagopus_exit_fatal("Can't create a thread.\n");
    } else {
      fprintf(stderr, "create a thread [%d]\n", i);

      if ((ret = lagopus_thread_get_pthread_id(&thread, &tid))
          != LAGOPUS_RESULT_NOT_STARTED) {
        lagopus_perror(ret);
        goto done;
      }
    }

    if ((ret = lagopus_thread_start(&thread, false)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      lagopus_exit_fatal("Can't start the thread.\n");
    } else {
      ret = lagopus_thread_get_pthread_id(&thread, &tid);
      if (ret == LAGOPUS_RESULT_OK ||
          ret == LAGOPUS_RESULT_ALREADY_HALTED) {
        fprintf(stderr, " start thread:  %lu\n", tid);
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    s_check_cancelled(&thread, false);

    if ((ret = lagopus_thread_cancel(&thread)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      lagopus_msg_error("Can't cancel the thread.\n");
    } else {
      if ((ret = lagopus_thread_get_pthread_id(&thread, &tid))
          == LAGOPUS_RESULT_OK) {
        lagopus_msg_error(
          "Thread has been canceled, but I can get thread ID, %lu\n",
          tid);
      } else if (ret != LAGOPUS_RESULT_NOT_STARTED &&
                 ret != LAGOPUS_RESULT_ALREADY_HALTED) {
        lagopus_perror(ret);
        goto done;
      }
    }

    s_check_cancelled(&thread, true);

    lagopus_thread_destroy(&thread);
    fprintf(stderr, "destroy the thread [%d]\n", i);

    fprintf(stderr, "\n");
  }

  /* create, start, destroy */
  for (i = 0; i < n; i++) {
    thread = NULL;

    if ((ret = lagopus_thread_create(
                 (lagopus_thread_t *)&thread,
                 (lagopus_thread_main_proc_t)s_main_proc_with_sleep,
                 (lagopus_thread_finalize_proc_t)s_finalize_proc,
                 (lagopus_thread_freeup_proc_t)s_freeup_proc,
                 (const char *) "thread",
                 (void *) NULL)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      lagopus_exit_fatal("Can't create a thread.\n");
    } else {
      fprintf(stderr, "create a thread [%d]\n", i);

      if ((ret = lagopus_thread_get_pthread_id(&thread, &tid))
          != LAGOPUS_RESULT_NOT_STARTED) {
        lagopus_perror(ret);
        goto done;
      }
    }

    if ((ret = lagopus_thread_start(&thread, false)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      lagopus_exit_fatal("Can't start the thread.\n");
    } else {
      ret = lagopus_thread_get_pthread_id(&thread, &tid);
      if (ret == LAGOPUS_RESULT_OK ||
          ret == LAGOPUS_RESULT_ALREADY_HALTED) {
        fprintf(stderr, " start thread:  %lu\n", tid);
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    lagopus_thread_destroy(&thread);
    fprintf(stderr, "destroy the thread [%d]\n", i);

    fprintf(stderr, "\n");
  }

done:
  return (ret == LAGOPUS_RESULT_OK) ? 0 : 1;
}
