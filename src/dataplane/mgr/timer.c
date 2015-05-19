/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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
 *      @file   timer.c
 *      @brief  Routines for removing flow triggerd by timeout.
 */

#include <inttypes.h>
#include <time.h>
#include <sys/queue.h>

#include "lagopus/flowdb.h"

#undef DEBUG
#ifdef DEBUG
#include <stdio.h>

#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...)
#endif

#define MAX_TIMEOUT_FLOWS 256

struct flow_timer {
  TAILQ_ENTRY(flow_timer) next;
  time_t timeout;
  int nflow;
  struct flow *flows[MAX_TIMEOUT_FLOWS];
};

TAILQ_HEAD(flow_timer_list, flow_timer) flow_timer_list;

void
init_flow_timer(void) {
  TAILQ_INIT(&flow_timer_list);
  (void)get_current_time();
}

static struct flow_timer *
find_flow_timer(time_t timeout,
                struct flow_timer **prev,
                time_t *prev_timep) {
  struct flow_timer *flow_timer;
  time_t prev_time;

  *prev = NULL;
  prev_time = 0;
  TAILQ_FOREACH(flow_timer, &flow_timer_list, next) {
    if (prev_time + flow_timer->timeout > timeout) {
      flow_timer = NULL;
      break;
    }
    if (prev_time + flow_timer->timeout == timeout &&
        flow_timer->nflow < MAX_TIMEOUT_FLOWS) {
      /* found. */
      break;
    }
    *prev = flow_timer;
    prev_time += flow_timer->timeout;
  }
  *prev_timep = prev_time;
  return flow_timer;
}

lagopus_result_t
add_flow_timer(struct flow *flow) {
  struct flow_timer *flow_timer, *prev;
  time_t timeout, idle_elapsed, hard_elapsed, prev_time;

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
  /* XXX flowdb lock */
  flow_timer = find_flow_timer(timeout, &prev, &prev_time);
  if (flow_timer != NULL) {
    flow_timer->flows[flow_timer->nflow] = flow;
    flow->flow_timer = &flow_timer->flows[flow_timer->nflow];
    flow_timer->nflow++;
  } else {
    flow_timer = calloc(1, sizeof(struct flow_timer));
    if (flow_timer == NULL) {
      /* XXX flowdb unlock */
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    flow_timer->flows[flow_timer->nflow] = flow;
    flow->flow_timer = &flow_timer->flows[flow_timer->nflow];
    flow_timer->nflow++;
    if (prev == NULL) {
      flow_timer->timeout = timeout;
      TAILQ_INSERT_HEAD(&flow_timer_list, flow_timer, next);
    } else {
      timeout -= prev_time;
      flow_timer->timeout = timeout;
      TAILQ_INSERT_AFTER(&flow_timer_list, prev, flow_timer, next);
    }
    /* re-calculation timeout of entries after inserted entry */
    if ((flow_timer = TAILQ_NEXT(flow_timer, next)) != NULL) {
      flow_timer->timeout -= timeout;
    }
  }
  /* XXX flowdb unlock */
  return LAGOPUS_RESULT_OK;
}

static void
flow_timer_expire(struct flow_timer *flow_timer) {
  struct flow *flow;
  struct ofp_error error;
  int reason;
  int i;

  DPRINTF("expired\n");
  for (i = 0; i < flow_timer->nflow; i++) {
    /* calculate elapsed time */
    flow = flow_timer->flows[i];
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
flow_timer_loop(const lagopus_thread_t *t, void *arg) {
  struct flow_timer *flow_timer;
  lagopus_result_t rv;
  global_state_t cur_state;
  shutdown_grace_level_t cur_grace;

  (void) t;
  (void) arg;

  rv = global_state_wait_for(GLOBAL_STATE_STARTED,
                             &cur_state,
                             &cur_grace,
                             -1);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }

  for (;;) {
    /* XXX flowdb lock */
    flow_timer = TAILQ_FIRST(&flow_timer_list);
    if (flow_timer == NULL) {
      sleep(1);
      continue;
    }
    /* XXX flowdb unlock */
    do {
      time_t timeout;
      unsigned int elapsed;

      timeout = flow_timer->timeout;
      while (timeout > 0) {
        elapsed = sleep(1);
        (void)get_current_time();
        if (elapsed == 0) {
          timeout--;
        }
      }
      /* XXX flowdb lock */
      flow_timer_expire(flow_timer);
      TAILQ_REMOVE(&flow_timer_list, flow_timer, next);
      free(flow_timer);
      flow_timer = TAILQ_FIRST(&flow_timer_list);
      /* XXX flowdb unlock */
    } while (flow_timer != NULL);
    pthread_yield();
  }

  return LAGOPUS_RESULT_OK;
}
