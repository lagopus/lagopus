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

#ifndef SRC_DATAPLANE_MGR_DP_TIMER_H_
#define SRC_DATAPLANE_MGR_DP_TIMER_H_

enum {
  FLOW_TIMER,
  MBTREE_TIMER,
  MACTABLE_TIMER,
  LINK_TIMER,
};

#define MAX_TIMEOUT_ENTRIES 256

struct dp_timer {
  TAILQ_ENTRY(dp_timer) next;
  time_t timeout;
  int type;
  void (*expire_func)(struct dp_timer *);
  int nentries;
  void *timer_entry[MAX_TIMEOUT_ENTRIES];
};

void
init_dp_timer(void);

void *
add_dp_timer(int type,
             time_t timeout,
             void (*expire_func)(struct dp_timer *),
             void *arg);

struct flow;
struct flow_list;
struct mactable;

lagopus_result_t
add_flow_timer(struct flow *flow);
lagopus_result_t
add_mbtree_timer(struct flow_list *flow_list, time_t timeout);
lagopus_result_t
add_mactable_timer(struct mactable *mactable, time_t timeout);

#endif /* SRC_DATAPLANE_MGR_DP_TIMER_H_ */
