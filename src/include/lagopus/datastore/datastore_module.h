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

#ifndef __DATASTORE_MODULE_H__
#define __DATASTORE_MODULE_H__


#include "lagopus_apis.h"





lagopus_result_t	datastore_initialize(int argc,
                                       const char *const argv[],
                                       void *extarg,
                                       lagopus_thread_t **thdptr);
lagopus_result_t	datastore_start(void);
lagopus_result_t	datastore_shutdown(shutdown_grace_level_t l);
lagopus_result_t	datastore_stop(void);
void			datastore_finalize(void);



lagopus_result_t
load_conf_initialize(int argc, const char *const argv[],
                     void *extarg, lagopus_thread_t **retptr);

lagopus_result_t
load_conf_start(void);

lagopus_result_t
load_conf_shutdown(shutdown_grace_level_t l);

void
load_conf_finalize(void);

#endif /* ! __DATASTORE_MODULE_H__ */
