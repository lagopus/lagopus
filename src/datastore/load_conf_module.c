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
#include "lagopus/datastore.h"

static pthread_once_t initialized = PTHREAD_ONCE_INIT;

static void
initialize_internal(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = datastore_load_config();
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    lagopus_exit_fatal("can't load the configuration parameters.\n");
  }
}

lagopus_result_t
load_conf_initialize(int argc, const char *const argv[],
                     void *extarg, lagopus_thread_t **retptr) {
  (void) argc;
  (void) argv;
  (void) extarg;
  (void) retptr;

  lagopus_msg_info("load_conf_initiaize.\n");
  pthread_once(&initialized, initialize_internal);

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
load_conf_start(void) {
  /* nothing. */
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
load_conf_shutdown(shutdown_grace_level_t l) {
  (void) l;
  /* nothing. */
  return LAGOPUS_RESULT_OK;
}

void
load_conf_finalize(void) {
  /* nothing. */
}
