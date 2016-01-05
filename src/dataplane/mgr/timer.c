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

#include "lagopus_apis.h"
#include "lagopus/dataplane.h"
#include "lagopus/flowdb.h"
#include "mbtree.h"
#include "thread.h"

#undef DEBUG
#ifdef DEBUG
#include <stdio.h>

#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...)
#endif

#define MAX_TIMEOUT_ENTRIES 256

enum {
  FLOW_TIMER,
  MBTREE_TIMER
};

struct dp_timer {
  TAILQ_ENTRY(dp_timer) next;
  time_t timeout;
  int type;
  void (*expire_func)(struct dp_timer *);
  int nentries;
  void *timer_entry[MAX_TIMEOUT_ENTRIES];
};

TAILQ_HEAD(dp_timer_list, dp_timer) dp_timer_list;

static lagopus_thread_t timer_thread = NULL;
static bool timer_run = false;
static lagopus_mutex_t timer_lock = NULL;


void
init_dp_timer(void) {
  TAILQ_INIT(&dp_timer_list);
  (void)get_current_time();
}

static struct dp_timer *
find_dp_timer(time_t timeout,
              int type,
              struct dp_timer **prev,
              time_t *prev_timep) {
  struct dp_timer *dp_timer;
  time_t prev_time;

  *prev = NULL;
  prev_time = 0;
  TAILQ_FOREACH(dp_timer, &dp_timer_list, next) {
    if (dp_timer->type != type) {
      continue;
    }
    if (prev_time + dp_timer->timeout > timeout) {
      dp_timer = NULL;
      break;
    }
    if (prev_time + dp_timer->timeout == timeout &&
        dp_timer->nentries < MAX_TIMEOUT_ENTRIES) {
      /* found. */
      break;
    }
    *prev = dp_timer;
    prev_time += dp_timer->timeout;
  }
  *prev_timep = prev_time;
  return dp_timer;
}

lagopus_result_t
add_flow_timer(struct flow *flow) {
  struct dp_timer *dp_timer, *prev;
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
  dp_timer = find_dp_timer(timeout, FLOW_TIMER, &prev, &prev_time);
  if (dp_timer != NULL) {
    dp_timer->timer_entry[dp_timer->nentries] = flow;
    flow->flow_timer = &dp_timer->timer_entry[dp_timer->nentries];
    dp_timer->nentries++;
  } else {
    dp_timer = calloc(1, sizeof(struct dp_timer));
    if (dp_timer == NULL) {
      /* XXX flowdb unlock */
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    dp_timer->type = FLOW_TIMER;
    dp_timer->timer_entry[dp_timer->nentries] = flow;
    flow->flow_timer = &dp_timer->timer_entry[dp_timer->nentries];
    dp_timer->nentries++;
    if (prev == NULL) {
      dp_timer->timeout = timeout;
      TAILQ_INSERT_HEAD(&dp_timer_list, dp_timer, next);
    } else {
      timeout -= prev_time;
      dp_timer->timeout = timeout;
      TAILQ_INSERT_AFTER(&dp_timer_list, prev, dp_timer, next);
    }
    /* re-calculation timeout of entries after inserted entry */
    if ((dp_timer = TAILQ_NEXT(dp_timer, next)) != NULL) {
      dp_timer->timeout -= timeout;
    }
  }
  /* XXX flowdb unlock */
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
add_mbtree_timer(struct flow_list *flow_list, time_t timeout) {
  struct dp_timer *dp_timer, *prev;
  time_t prev_time;

  dp_timer = find_dp_timer(timeout, MBTREE_TIMER, &prev, &prev_time);
  if (dp_timer != NULL) {
    int i;

    for (i = 0; i < MAX_TIMEOUT_ENTRIES; i++) {
      if (dp_timer->timer_entry[i] == NULL) {
        break;
      }
    }
    dp_timer->timer_entry[i] = flow_list;
    flow_list->mbtree_timer = &dp_timer->timer_entry[i];
    if (i == dp_timer->nentries) {
      dp_timer->nentries++;
    }
  } else {
    dp_timer = calloc(1, sizeof(struct dp_timer));
    if (dp_timer == NULL) {
      /* XXX flowdb unlock */
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    dp_timer->type = MBTREE_TIMER;
    dp_timer->timer_entry[0] = flow_list;
    flow_list->mbtree_timer = &dp_timer->timer_entry[0];
    dp_timer->nentries++;
    if (prev == NULL) {
      dp_timer->timeout = timeout;
      TAILQ_INSERT_HEAD(&dp_timer_list, dp_timer, next);
    } else {
      timeout -= prev_time;
      dp_timer->timeout = timeout;
      TAILQ_INSERT_AFTER(&dp_timer_list, prev, dp_timer, next);
    }
    /* re-calculation timeout of entries after inserted entry */
    if ((dp_timer = TAILQ_NEXT(dp_timer, next)) != NULL) {
      dp_timer->timeout -= timeout;
    }
  }
  return LAGOPUS_RESULT_OK;
}

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

static void
mbtree_timer_expire(struct dp_timer *dp_timer) {
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
    cleanup_mbtree(flow_list);
    build_mbtree(flow_list);
    system("date");
    printf("cleanup and build end\n");
  }
}

static void
dp_timer_expire(struct dp_timer *dp_timer) {
  switch (dp_timer->type) {
    case FLOW_TIMER:
      flow_timer_expire(dp_timer);
      break;
    case MBTREE_TIMER:
      mbtree_timer_expire(dp_timer);
    default:
      break;
  }
}

static lagopus_result_t
dp_timer_thread_loop(const lagopus_thread_t *t, void *arg) {
  struct dp_timer *dp_timer;
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

  while (timer_run == true) {
    /* XXX flowdb lock */
    dp_timer = TAILQ_FIRST(&dp_timer_list);
    if (dp_timer == NULL) {
      sleep(1);
      continue;
    }
    /* XXX flowdb unlock */
    do {
      unsigned int elapsed;

      while (dp_timer->timeout > 0) {
        elapsed = sleep(1);
        (void)get_current_time();
        if (elapsed == 0) {
          dp_timer->timeout--;
        }
      }
      /* XXX flowdb lock */
      dp_timer_expire(dp_timer);
      TAILQ_REMOVE(&dp_timer_list, dp_timer, next);
      free(dp_timer);
      dp_timer = TAILQ_FIRST(&dp_timer_list);
      /* XXX flowdb unlock */
    } while (dp_timer != NULL);
    sched_yield();
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_timer_thread_init(__UNUSED int argc,
                     __UNUSED const char *const argv[],
                     __UNUSED void *extarg,
                     lagopus_thread_t **thdptr) {
  static struct dataplane_arg dparg;

  init_dp_timer();
  dparg.threadptr = &timer_thread;
  dparg.lock = &timer_lock;
  dparg.running = &timer_run;
  lagopus_thread_create(&timer_thread, dp_timer_thread_loop,
                        dp_finalproc, dp_freeproc, "dp_timer", &dparg);
  if (lagopus_mutex_create(&timer_lock) != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("lagopus_mutex_create");
  }
  *thdptr = &timer_thread;

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_timer_thread_start(void) {
  return dp_thread_start(&timer_thread, &timer_lock, &timer_run);
}

void
dp_timer_thread_fini(void) {
  dp_thread_finalize(&timer_thread);
}

lagopus_result_t
dp_timer_thread_shutdown(shutdown_grace_level_t level) {
  return dp_thread_shutdown(&timer_thread, &timer_lock, &timer_run, level);
}

lagopus_result_t
dp_timer_thread_stop(void) {
  return dp_thread_stop(&timer_thread, &timer_run);
}

#if 0
#define MODIDX_TIMER  LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE + 108

static void timer_ctors(void) __attr_constructor__(MODIDX_TIMER);
static void timer_dtors(void) __attr_constructor__(MODIDX_TIMER);

static void timer_ctors (void) {
  lagopus_result_t rv;
  const char *name = "dp_timer";

  rv = lagopus_module_register(name,
                               dp_timer_thread_init,
                               NULL,
                               dp_timer_thread_start,
                               dp_timer_thread_shutdown,
                               dp_timer_thread_stop,
                               dp_timer_thread_fini,
                               NULL);
  if (rv != LAGOPUS_RESULT_OK) {
    lagopus_perror(rv);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }
}
#endif /* 0 */
