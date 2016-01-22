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
 *      @file   ofp_dpqueue_mgr.c
 *      @brief  Dataplane communicator thread functions.
 */

#include "lagopus_apis.h"
#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_bridge.h"
#include "lagopus/ofp_bridgeq_mgr.h"
#include "lagopus/dp_apis.h"

#define MUXER_TIMEOUT 100LL * 1000LL * 1000LL
#define TIMEOUT_SHUTDOWN_RIGHT_NOW     (100*1000*1000) /* 100msec */
#define TIMEOUT_SHUTDOWN_GRACEFULLY    (1500*1000*1000) /* 1.5sec */

static lagopus_qmuxer_poll_t *polls = NULL;
static lagopus_qmuxer_t muxer = NULL;

/* read event_dataq */
static inline lagopus_result_t
event_dataq_dequeue(struct ofp_bridge *ofp_bridge,
                    lagopus_qmuxer_poll_t poll) {
  lagopus_result_t rv = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data **gets = NULL;
  lagopus_result_t q_size = lagopus_bbq_size(&(ofp_bridge->event_dataq));
  uint16_t max_batches;
  size_t get_num;
  size_t i;

  rv = ofp_bridge_event_dataq_max_batches_get(ofp_bridge,
       &max_batches);
  if (rv != LAGOPUS_RESULT_OK) {
    lagopus_perror(rv);
    goto done;
  }

  lagopus_msg_debug(10,
                    "called. dpid: %lu, q_size: %lu, max_batches: %"PRIu16"\n",
                    ofp_bridge->dpid, q_size, max_batches);

  if (q_size > 0) {
    gets = (struct eventq_data **)
           malloc(sizeof(struct eventq_data *) * max_batches);
    if (gets != NULL) {
      rv = lagopus_bbq_get_n(&(ofp_bridge->event_dataq), gets,
                             max_batches, 0LL,
                             struct eventq_data *, 0LL, &get_num);
      if (rv < LAGOPUS_RESULT_OK) {
        lagopus_perror(rv);
        rv = lagopus_qmuxer_poll_set_queue(&poll, NULL);
        if (rv != LAGOPUS_RESULT_OK) {
          lagopus_perror(rv);
          goto done;
        }
      } else {
        rv = LAGOPUS_RESULT_OK;
      }

      for (i = 0; i < get_num; i++) {
        rv = dp_process_event_data(ofp_bridge->dpid, gets[i]);
        if (gets[i] != NULL && gets[i]->free != NULL) {
          gets[i]->free(gets[i]);
        } else {
          free(gets[i]);
        }
      }
    } else {
      rv = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else if (q_size == 0) {
    rv = LAGOPUS_RESULT_OK;
  }

done:
  free(gets);

  return rv;
}

/* read each queues. */
static inline lagopus_result_t
dequeue(struct ofp_bridgeq *brqs) {
  lagopus_result_t rv = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridge *bridge;
  lagopus_qmuxer_poll_t poll;

  if (brqs != NULL) {
    bridge = ofp_bridgeq_mgr_bridge_get(brqs);
    poll = ofp_bridgeq_mgr_event_dataq_dp_poll_get(brqs);
    rv = event_dataq_dequeue(bridge, poll);
    if (rv != LAGOPUS_RESULT_OK) {
      lagopus_perror(rv);
      goto done;
    }
  } else {
    lagopus_msg_error("ofp_bridgeq is NULL\n");
    rv = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return rv;
}

/**
 * dataplane comm lagopus thread.
 */

static lagopus_thread_t dpqueue_thread = NULL;
static bool dpqueue_run = false;
static lagopus_mutex_t dpqueue_lock = NULL;

/**
 * Communicate with agent loop function.
 *
 * @param[in]   t       Thread object pointer.
 * @param[in]   arg     Do not used argument.
 */
static lagopus_result_t
ofp_dpqueue_mgr_loop(const lagopus_thread_t *t, void *arg) {
  lagopus_result_t rv = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *bridgeqs[MAX_BRIDGES];
  global_state_t cur_state;
  shutdown_grace_level_t cur_grace;
  int n_need_watch = 0;
  int n_valid_polls = 0;
  uint64_t i;
  uint64_t n_bridgeqs = 0;
  /* number of default polls. */
  uint64_t n_polls = 0;
  lagopus_bbq_t bbq = NULL;
  (void) t;
  (void) arg;

  rv = global_state_wait_for(GLOBAL_STATE_STARTED,
                             &cur_state,
                             &cur_grace,
                             -1);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }

  while (dpqueue_run == true) {
    n_need_watch = 0;
    n_valid_polls = 0;
    n_polls = 0;
    /* get bridgeq array. */
    rv = ofp_bridgeq_mgr_bridgeqs_to_array(bridgeqs, &n_bridgeqs,
                                           MAX_BRIDGES);
    if (rv != LAGOPUS_RESULT_OK) {
      n_bridgeqs = 0;
      lagopus_perror(rv);
      break;
    }
    /* get polls.*/
    rv = ofp_bridgeq_mgr_dp_polls_get(polls,
                                      bridgeqs, &n_polls,
                                      n_bridgeqs);
    if (rv != LAGOPUS_RESULT_OK) {
      lagopus_perror(rv);
      goto done;
    }

    for (i = 0; i < n_polls; i++) {
      /* Check if the poll has a valid queue. */
      bbq = NULL;
      rv = lagopus_qmuxer_poll_get_queue(&polls[i], &bbq);
      if (rv != LAGOPUS_RESULT_OK) {
        lagopus_perror(rv);
        goto done;
      }
      if (bbq != NULL && ofp_handler_validate_bbq(&bbq) == true) {
        n_valid_polls++;
      }
      /* Reset the poll status. */
      rv = lagopus_qmuxer_poll_reset(&polls[i]);
      if (rv != LAGOPUS_RESULT_OK) {
        lagopus_perror(rv);
        goto done;
      }
      n_need_watch++;
    }

    /* Wait for an event. */
    rv = lagopus_qmuxer_poll(&muxer,
                             (lagopus_qmuxer_poll_t *const) polls,
                             (size_t)n_need_watch, MUXER_TIMEOUT);
    if (rv > 0) {
      /* read event_dataq */
      if (n_polls > 0) {
        for (i = 0; i < n_bridgeqs; i++) {
          rv = dequeue(bridgeqs[i]);
          if (rv != LAGOPUS_RESULT_OK) {
            lagopus_perror(rv);
            goto done;
          }
        }
      }
      rv = LAGOPUS_RESULT_OK;
    } else if (rv == LAGOPUS_RESULT_TIMEDOUT) {
      lagopus_msg_debug(100, "Timedout. continue.\n");
      rv = LAGOPUS_RESULT_OK;
    } else if (rv != LAGOPUS_RESULT_OK) {
      lagopus_perror(rv);
    }

  done:
    /* free bridgeqs. */
    ofp_bridgeq_mgr_poll_reset(polls, MAX_DP_POLLS);
    if (n_bridgeqs > 0) {
      ofp_bridgeq_mgr_bridgeqs_free(bridgeqs, n_bridgeqs);
    }

    if (rv != LAGOPUS_RESULT_OK) {
      break;
    }
  }
  return rv;
}

static void
finalproc(const lagopus_thread_t *t, bool is_canceled, void *arg) {
  bool is_valid = false;
  lagopus_result_t ret;

  (void) is_canceled;
  (void) arg;

  lagopus_msg_info("finalproc in %p\n", dpqueue_thread);
  if (*t != dpqueue_thread) {
    lagopus_exit_fatal("other threads worked t:%p, my_thread:%p\n",
                       *t, dpqueue_thread);
  }

  ret = lagopus_thread_is_valid(&dpqueue_thread, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    return;
  }
}

static void
freeproc(const lagopus_thread_t *t, void *arg) {
  bool is_valid = false;
  lagopus_result_t ret;

  lagopus_msg_info("freeproc in\n");
  if (*t != dpqueue_thread) {
    lagopus_exit_fatal("other threads worked %p, %p", *t, dpqueue_thread);
  }

  ret = lagopus_thread_is_valid(&dpqueue_thread, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    return;
  }
  /* ADD delete ports? */

  lagopus_mutex_destroy(&dpqueue_lock);
}

lagopus_result_t
ofp_dpqueue_mgr_initialize(int argc,
                           const char *const argv[],
                           __UNUSED void *extarg,
                           lagopus_thread_t **thdptr) {
  lagopus_result_t rv = LAGOPUS_RESULT_ANY_FAILURES;

  (void) argc;
  (void) argv;
  (void) thdptr;

  /* allocate polls */
  polls = (lagopus_qmuxer_poll_t *)calloc(MAX_DP_POLLS, sizeof(*polls));
  if (polls == NULL) {
    lagopus_msg_error("Can't allocate polls.\n");
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  /* Create the qmuxer. */
  rv = lagopus_qmuxer_create(&muxer);
  if (rv != LAGOPUS_RESULT_OK) {
    lagopus_perror(rv);
    free(polls);
    polls = NULL;
    return rv;
  }

  return lagopus_thread_create(&dpqueue_thread, ofp_dpqueue_mgr_loop,
                               finalproc, freeproc,
                               "ofp_dpqueue", NULL);
}

lagopus_result_t
ofp_dpqueue_mgr_start(void) {
  lagopus_result_t ret;

  lagopus_mutex_lock(&dpqueue_lock);
  if (dpqueue_run == true) {
    ret = LAGOPUS_RESULT_ALREADY_EXISTS;
    goto done;
  }
  dpqueue_run = true;
  ret = lagopus_thread_start(&dpqueue_thread, false);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }
done:
  lagopus_mutex_unlock(&dpqueue_lock);
  return ret;
}

void
ofp_dpqueue_mgr_finalize(void) {
  bool is_valid = false, is_canceled = false;
  lagopus_result_t ret;

  if (polls != NULL) {
    free(polls);
    polls = NULL;
  }
  if (muxer != NULL) {
    lagopus_qmuxer_destroy(&muxer);
    muxer = NULL;
  }

  ret = lagopus_thread_is_valid(&dpqueue_thread, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    return;
  }
  ret = lagopus_thread_is_canceled(&dpqueue_thread, &is_canceled);
  if (ret == LAGOPUS_RESULT_OK && is_canceled == false) {
    global_state_cancel_janitor();
  }
  lagopus_thread_destroy(&dpqueue_thread);

  return;
}

lagopus_result_t
ofp_dpqueue_mgr_shutdown(shutdown_grace_level_t level) {
  bool is_valid = false;
  lagopus_chrono_t nsec;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_info("shutdown called\n");
  lagopus_mutex_lock(&dpqueue_lock);
  if (dpqueue_run == false) {
    goto done;
  }

  ret = lagopus_thread_is_valid(&dpqueue_thread, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    goto done;
  }

  /* XXX UP TO main() */
  switch (level) {
    case SHUTDOWN_RIGHT_NOW:
      nsec = TIMEOUT_SHUTDOWN_RIGHT_NOW;
      break;
    case SHUTDOWN_GRACEFULLY:
      nsec = TIMEOUT_SHUTDOWN_GRACEFULLY;
      break;
    default:
      lagopus_msg_fatal("unknown shutdown level %d\n", level);
      ret = LAGOPUS_RESULT_ANY_FAILURES;
      goto done;
  }

  dpqueue_run = false;
  ret = lagopus_thread_wait(&dpqueue_thread, nsec);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

done:
  lagopus_mutex_unlock(&dpqueue_lock);
  return ret;
}

lagopus_result_t
ofp_dpqueue_mgr_stop(void) {
  bool is_canceled = false;
  lagopus_result_t ret;

  ret = lagopus_thread_is_canceled(&dpqueue_thread, &is_canceled);
  if (ret == LAGOPUS_RESULT_OK && is_canceled == false) {
    ret = lagopus_thread_cancel(&dpqueue_thread);
  }
  dpqueue_run = false;

  return ret;
}
