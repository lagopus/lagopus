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
s_main_proc(const lagopus_thread_t *selfptr, void *arg) {
  volatile size_t a = 0;

  (void)selfptr;
  (void)arg;

  while (true) {
    a++;
  }

  return LAGOPUS_RESULT_OK;
}


int
main(int argc, char const *const argv[]) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int cpu = 0;
  lagopus_thread_t thd = NULL;

  if (argc > 1) {
    int tmp;
    if (lagopus_str_parse_int32(argv[1], &tmp) == LAGOPUS_RESULT_OK &&
        tmp > 0) {
      cpu = tmp;
    }
  }

  if ((ret = lagopus_thread_create(&thd,
                                   s_main_proc,
                                   NULL,
                                   NULL,
                                   (const char *) "thread",
                                   (void *) NULL)) == LAGOPUS_RESULT_OK) {
    if ((ret = lagopus_thread_set_cpu_affinity(&thd,
               -1)) == LAGOPUS_RESULT_OK &&
        (ret = lagopus_thread_set_cpu_affinity(&thd,
               cpu)) == LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_thread_start(&thd, false)) == LAGOPUS_RESULT_OK) {
        ret = lagopus_thread_wait(&thd, -1LL);
      }
    }
  }

  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    return 1;
  } else {
    return 0;
  }
}
