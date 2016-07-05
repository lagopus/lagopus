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

#ifndef SRC_DATAPLANE_MGR_PIPELINE_H_
#define SRC_DATAPLANE_MGR_PIPELINE_H_

extern __thread uint32_t pipeline_worker_id;

void pipeline_process(struct lagopus_packet *pkt);
void pipeline_process_stacked_packets(void);

#endif /* SRC_DATAPLANE_MGR_PIPELINE_LEGACY_SW_H_ */
