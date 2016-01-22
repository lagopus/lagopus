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

/**
 *      @file   thread.c
 *      @brief  Dataplane thread control functions
 */

#include "lagopus_config.h"

#ifdef HAVE_DPDK
#include <rte_config.h>
#include <rte_lcore.h>
#include <rte_atomic.h>
#include <rte_launch.h>
#endif /* HAVE_DPDK */

#include "lagopus_apis.h"
#include "lagopus_gstate.h"
#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
#include "lagopus/flowinfo.h"
#include "lagopus/dp_apis.h"

#include "thread.h"

#define TIMEOUT_SHUTDOWN_RIGHT_NOW     (100*1000*1000) /* 100msec */
#define TIMEOUT_SHUTDOWN_GRACEFULLY    (1500*1000*1000) /* 1.5sec */

void
dp_finalproc(const lagopus_thread_t *t, bool is_canceled, void *arg) {
  bool is_valid = false;
  struct dataplane_arg *dparg;
  lagopus_result_t ret;

  (void) is_canceled;
  dparg = arg;

  lagopus_msg_info("finalproc in %p\n", *dparg->threadptr);
  if (*t != *dparg->threadptr) {
    lagopus_exit_fatal("other threads worked t:%p, my_thread:%p\n",
                       *t, *dparg->threadptr);
  }

  ret = lagopus_thread_is_valid(dparg->threadptr, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    return;
  }
}

void
dp_freeproc(const lagopus_thread_t *t, void *arg) {
  bool is_valid = false;
  struct dataplane_arg *dparg;
  lagopus_result_t ret;

  dparg = arg;

  lagopus_msg_info("freeproc in\n");
  if (*t != *dparg->threadptr) {
    lagopus_exit_fatal("other threads worked %p, %p", *t, *dparg->threadptr);
  }

  ret = lagopus_thread_is_valid(dparg->threadptr, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    return;
  }
  /* ADD delete ports? */

  lagopus_mutex_destroy(dparg->lock);
  *dparg->lock = NULL;
}

lagopus_result_t
dp_thread_start(lagopus_thread_t *threadptr,
                lagopus_mutex_t *lockptr,
                bool *runptr) {
  lagopus_result_t ret;

  lagopus_mutex_lock(lockptr);
  if (*runptr == true) {
    ret = LAGOPUS_RESULT_ALREADY_EXISTS;
    goto done;
  }
  *runptr = true;
  ret = lagopus_thread_start(threadptr, false);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }
done:
  lagopus_mutex_unlock(lockptr);
  return ret;
}

void
dp_thread_finalize(lagopus_thread_t *threadptr) {
  bool is_valid = false, is_canceled = false;
  lagopus_result_t ret;

  ret = lagopus_thread_is_valid(threadptr, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    return;
  }
  ret = lagopus_thread_is_canceled(threadptr, &is_canceled);
  if (ret == LAGOPUS_RESULT_OK && is_canceled == false) {
    global_state_cancel_janitor();
  }
  lagopus_thread_destroy(threadptr);

  return;
}

lagopus_result_t
dp_thread_shutdown(lagopus_thread_t *threadptr,
                   lagopus_mutex_t *lockptr,
                   bool *runptr,
                   shutdown_grace_level_t level) {
  bool is_valid = false;
  lagopus_chrono_t nsec;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_info("shutdown called\n");
  lagopus_mutex_lock(lockptr);
  if (*runptr == false) {
    goto done;
  }

  ret = lagopus_thread_is_valid(threadptr, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    goto done;
  }

  /* XXX UP TO main() */
  switch (level) {
    case SHUTDOWN_RIGHT_NOW:
      nsec = TIMEOUT_SHUTDOWN_RIGHT_NOW;
      break;
    case SHUTDOWN_GRACEFULLY:
      nsec = TIMEOUT_SHUTDOWN_GRACEFULLY;
      break;
    default:
      lagopus_msg_fatal("unknown shutdown level %d\n", level);
      ret = LAGOPUS_RESULT_ANY_FAILURES;
      goto done;
  }

  *runptr = false;
  ret = lagopus_thread_wait(threadptr, nsec);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

done:
  lagopus_mutex_unlock(lockptr);
  return ret;
}

lagopus_result_t
dp_thread_stop(lagopus_thread_t *threadptr, bool *runptr) {
  bool is_canceled = false;
  lagopus_result_t ret;

  ret = lagopus_thread_is_canceled(threadptr, &is_canceled);
  if (ret == LAGOPUS_RESULT_OK && is_canceled == false) {
    ret = lagopus_thread_cancel(threadptr);
  }
  *runptr = false;

  return ret;
}
