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





#define NTHDS	8


static lagopus_bbq_t s_q = NULL;
static lagopus_thread_t s_thds[NTHDS] = { 0 };


static lagopus_result_t
s_main(const lagopus_thread_t *tptr, void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)arg;

  if (tptr != NULL) {
    lagopus_result_t r;
    void *val;

    while (true) {
      val = (void *)false;
      if ((r = lagopus_bbq_get(&s_q, &val, bool, -1LL)) !=
          LAGOPUS_RESULT_OK) {
        if (r == LAGOPUS_RESULT_NOT_OPERATIONAL) {
          r = LAGOPUS_RESULT_OK;
        } else {
          lagopus_perror(r);
        }
        break;
      }
    }

    ret = r;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static void
s_finalize(const lagopus_thread_t *tptr, bool is_canceled, void *arg) {
  (void)arg;
  (void)tptr;

  lagopus_msg_debug(1, "called. %s.\n",
                    (is_canceled == false) ? "finished" : "canceled");
  if (is_canceled == true) {
    lagopus_bbq_cancel_janitor(&s_q);
  }
}


int
main(int argc, const char *const argv[]) {
  int ret = 1;
  lagopus_result_t r;

  (void)argc;
  (void)argv;

  /*
   * Following both two cases must succeeded.
   */

  /*
   * 1) Shutdown the queue makes threads exit normally.
   */
  if ((r = lagopus_bbq_create(&s_q, bool, 20, NULL)) == LAGOPUS_RESULT_OK) {
    size_t i;

    for (i = 0; i < NTHDS; i++) {
      s_thds[i] = NULL;
      if ((r = lagopus_thread_create(&s_thds[i],
                                     s_main,
                                     s_finalize,
                                     NULL,
                                     "getter", NULL)) != LAGOPUS_RESULT_OK) {
        lagopus_perror(r);
        goto done;
      }
    }

    for (i = 0; i < NTHDS; i++) {
      if ((r = lagopus_thread_start(&s_thds[i], false)) != LAGOPUS_RESULT_OK) {
        lagopus_perror(r);
        goto done;
      }
    }

    sleep(1);

    /*
     * Shutdown the queue.
     */
    lagopus_bbq_shutdown(&s_q, true);

    /*
     * Then threads exit. Delete all of them.
     */
    for (i = 0; i < NTHDS; i++) {
      if ((r = lagopus_thread_wait(&s_thds[i], -1LL)) == LAGOPUS_RESULT_OK) {
        lagopus_thread_destroy(&s_thds[i]);
      }
    }

    /*
     * Delete the queue.
     */
    lagopus_bbq_destroy(&s_q, true);
  }

  /*
   * 2) Cancel all the threads and delete the queue.
   */
  s_q = NULL;
  if ((r = lagopus_bbq_create(&s_q, bool, 20, NULL)) == LAGOPUS_RESULT_OK) {
    size_t i;

    for (i = 0; i < NTHDS; i++) {
      s_thds[i] = NULL;
      if ((r = lagopus_thread_create(&s_thds[i],
                                     s_main,
                                     s_finalize,
                                     NULL,
                                     "getter", NULL)) != LAGOPUS_RESULT_OK) {
        lagopus_perror(r);
        goto done;
      }
    }

    for (i = 0; i < NTHDS; i++) {
      if ((r = lagopus_thread_start(&s_thds[i], false)) != LAGOPUS_RESULT_OK) {
        lagopus_perror(r);
        goto done;
      }
    }

    sleep(1);

    /*
     * Cancel and destroy all the thread.
     */
    for (i = 0; i < NTHDS; i++) {
      if ((r = lagopus_thread_cancel(&s_thds[i])) == LAGOPUS_RESULT_OK) {
        lagopus_thread_destroy(&s_thds[i]);
      }
    }

    /*
     * Delete the queue.
     */
    lagopus_bbq_destroy(&s_q, true);
  }

  ret = 0;

done:
  return ret;
}
