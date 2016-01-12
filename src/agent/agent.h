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

/* OpenFlow agent. */
lagopus_result_t agent_initialize(void *arg, lagopus_thread_t **thdptr);
void agent_finalize(void);
lagopus_result_t agent_start(void);
lagopus_result_t agent_shutdown(shutdown_grace_level_t);
lagopus_result_t agent_stop(void);

/* Thread shutdown parameters. */
#define TIMEOUT_SHUTDOWN_RIGHT_NOW     (100*1000*1000) /* 100msec */
#define TIMEOUT_SHUTDOWN_GRACEFULLY    (1500*1000*1000) /* 1.5sec */

/* Unused argument. */
#define __UNUSED __attribute__((unused))
