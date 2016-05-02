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

#include <time.h>

#include "lagopus_apis.h"
#include "lagopus/flowdb.h"
#include "thtable.h"
#include "dp_timer.h"

#undef DEBUG
#ifdef DEBUG
#include <stdio.h>

#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...)
#endif

static void
thtable_timer_expire(struct dp_timer *dp_timer) {
  struct flow_list *flow_list;
  int i;

  DPRINTF("expired\n");
  for (i = 0; i < dp_timer->nentries; i++) {
    /* calculate elapsed time */
    flow_list = dp_timer->timer_entry[i];
    if (flow_list == NULL) {
      continue;
    }
    system("date");
    printf("cleanup and build start\n");
    thtable_update(flow_list);
    system("date");
    printf("cleanup and build end\n");
  }
}

lagopus_result_t
add_thtable_timer(struct flow_list *flow_list, time_t timeout) {
  void *entryp;

  entryp = add_dp_timer(THTABLE_TIMER, timeout,
                        thtable_timer_expire, flow_list);
  if (entryp != NULL) {
    flow_list->update_timer = entryp;
  }
  return LAGOPUS_RESULT_OK;
}
