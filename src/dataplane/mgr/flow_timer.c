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
#include "lagopus/dataplane.h"
#include "lagopus/flowdb.h"
#include "dp_timer.h"

#undef DEBUG
#ifdef DEBUG
#include <stdio.h>

#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...)
#endif

static void
flow_timer_expire(struct dp_timer *dp_timer) {
  struct flow *flow;
  struct ofp_error error;
  int reason;
  int i;

  DPRINTF("expired\n");
  for (i = 0; i < dp_timer->nentries; i++) {
    /* calculate elapsed time */
    flow = dp_timer->timer_entry[i];
    if (flow == NULL) {
      continue;
    }
    reason = -1;
    if (flow->hard_timeout != 0 &&
        now_ts.tv_sec - flow->create_time.tv_sec >= flow->hard_timeout) {
      /* hard timeout. */
      reason = OFPRR_HARD_TIMEOUT;
    }
    if (flow->idle_timeout != 0 &&
        now_ts.tv_sec - flow->update_time.tv_sec >= flow->idle_timeout) {
      /* idle timeout. */
      reason = OFPRR_IDLE_TIMEOUT;
    }
    if (reason != -1) {
      flow->flow_timer = NULL;
      flow_remove_with_reason(flow, flow->bridge, (uint8_t)reason, &error);
    } else {
      /* flow timeout is updated. */
      add_flow_timer(flow);
    }
  }
}

lagopus_result_t
add_flow_timer(struct flow *flow) {
  void *entryp;
  time_t timeout, idle_elapsed, hard_elapsed;

  idle_elapsed =
    (time_t)flow->idle_timeout - (now_ts.tv_sec - flow->update_time.tv_sec);
  hard_elapsed =
    (time_t)flow->hard_timeout - (now_ts.tv_sec - flow->create_time.tv_sec);
  if (flow->idle_timeout > 0 && flow->hard_timeout > 0) {
    timeout = MIN(idle_elapsed, hard_elapsed);
  } else if (flow->idle_timeout > 0) {
    timeout = idle_elapsed;
  } else {
    timeout = hard_elapsed;
  }
  DPRINTF("add timeout %d sec\n", timeout);
  entryp = add_dp_timer(FLOW_TIMER, timeout, flow_timer_expire, flow);
  if (entryp != NULL) {
    flow->flow_timer = entryp;
  }
  return LAGOPUS_RESULT_OK;
}
