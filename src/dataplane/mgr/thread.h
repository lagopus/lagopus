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
 *      @file   thread.h
 *      @brief  Dataplane common thread routines.
 */

#ifndef SRC_INCLUDE_DATAPLANE_MGR_THREAD_H_
#define SRC_INCLUDE_DATAPLANE_MGR_THREAD_H_

struct dataplane_arg {
  lagopus_thread_t *threadptr;
  lagopus_mutex_t *lock;
  int argc;
  char **argv;
  bool *running;
};

void
dp_finalproc(const lagopus_thread_t *t, bool is_canceled, void *arg);

void
dp_freeproc(const lagopus_thread_t *t, void *arg);

lagopus_result_t
dp_thread_start(lagopus_thread_t *threadptr,
                lagopus_mutex_t *lockptr,
                bool *runptr);

void dp_thread_finalize(lagopus_thread_t *threadptr);

lagopus_result_t
dp_thread_shutdown(lagopus_thread_t *threadptr,
                   lagopus_mutex_t *lockptr,
                   bool *runptr,
                   shutdown_grace_level_t level);

lagopus_result_t
dp_thread_stop(lagopus_thread_t *threadptr, bool *runptr);

#endif /* SRC_INCLUDE_DATAPLANE_MGR_THREAD_H_ */
