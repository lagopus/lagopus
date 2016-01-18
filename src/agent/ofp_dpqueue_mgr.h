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

#ifndef SRC_AGENT_OFP_DPQUEUE_MGR_H_
#define SRC_AGENT_OFP_DPQUEUE_MGR_H_

lagopus_result_t
ofp_dpqueue_mgr_initialize(int argc,
                           const char *const argv[],
                           void *extarg,
                           lagopus_thread_t **thdptr);
lagopus_result_t ofp_dpqueue_mgr_start(void);
void ofp_dpqueue_mgr_finalize(void);
lagopus_result_t ofp_dpqueue_mgr_shutdown(shutdown_grace_level_t level);
lagopus_result_t ofp_dpqueue_mgr_stop(void);

#endif /* SRC_AGENT_OFP_DPQUEUE_MGR_H_ */
