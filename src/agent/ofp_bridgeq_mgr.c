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

#include "lagopus/ofp_bridge.h"
#include "lagopus_apis.h"
#include "lagopus/eventq_data.h"
#include "lagopus/ofp_bridgeq_mgr.h"

static pthread_once_t initialized = PTHREAD_ONCE_INIT;
/* queues to datapath */
static lagopus_hashmap_t bridgeq_table = NULL;
static struct ofp_bridgeq *bridgeqs[MAX_BRIDGES];
static uint64_t n_bridgeqs = 0;
static lagopus_mutex_t lock = NULL;

/* bridge queue entry. */
struct ofp_bridgeq {
  struct ofp_bridge *ofp_bridge;
  /* reference counter */
  volatile uint64_t refs;
  lagopus_mutex_t lock;
  /* poll objs for Agent */
  lagopus_qmuxer_poll_t polls[MAX_BRIDGE_POLLS];
  /* poll objs for DataPlane */
  lagopus_qmuxer_poll_t dp_polls[MAX_BRIDGE_DP_POLLS];
};

static inline void
bridgeq_mgr_lock(void) {
  assert(lock != NULL);
  if (lock != NULL) {
    lagopus_mutex_lock(&lock);
  }
}

static inline void
bridgeq_mgr_unlock(void) {
  assert(lock != NULL);
  if (lock != NULL) {
    lagopus_mutex_unlock(&lock);
  }
}

static inline void
bridgeq_lock(struct ofp_bridgeq *bridgeq) {
  if (bridgeq != NULL && bridgeq->lock != NULL) {
    lagopus_mutex_lock(&bridgeq->lock);
  }
}

static inline void
bridgeq_unlock(struct ofp_bridgeq *bridgeq) {
  if (bridgeq != NULL && bridgeq->lock != NULL) {
    lagopus_mutex_unlock(&bridgeq->lock);
  }
}

/* destroy bridgeq. */
static void
bridgeq_freeup_proc(void *val) {
  if (val != NULL) {
    struct ofp_bridgeq *bridgeq = (struct ofp_bridgeq *)val;
    ofp_bridgeq_free(bridgeq, false);
  }
}

static inline void
bridgeqs_clear(void) {
  memset(bridgeqs, 0, sizeof(bridgeqs));
  n_bridgeqs = 0;
}

static void
initialize_internal(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create hashmap */
  ret = lagopus_hashmap_create(&bridgeq_table,
                               LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                               bridgeq_freeup_proc);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't create bridgeq table.\n");
  }

  ret = lagopus_mutex_create(&lock);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("Can't create lock.\n");
  }
}

void
ofp_bridgeq_mgr_initialize(void *arg) {
  (void) arg;
  pthread_once(&initialized, initialize_internal);
}

void
ofp_bridgeq_refs_get(struct ofp_bridgeq *bridgeq) {
  assert(bridgeq != NULL);
  if (bridgeq != NULL) {
    bridgeq_lock(bridgeq);
    bridgeq->refs++;
    bridgeq_unlock(bridgeq);
  }
}

void
ofp_bridgeq_refs_put(struct ofp_bridgeq *bridgeq) {
  assert(bridgeq != NULL);
  if (bridgeq != NULL) {
    bridgeq_lock(bridgeq);
    bridgeq->refs--;
    bridgeq_unlock(bridgeq);
  }
}

void
ofp_bridgeq_free(struct ofp_bridgeq *bridgeq,
                 bool force) {
  uint64_t i;

  if (bridgeq != NULL) {
    bridgeq_lock(bridgeq);

    if (bridgeq->refs > 1 && force == false) {
      /* bridgeq is busy. */
      lagopus_msg_debug(500, "bridgeq(dpid = %"PRIu64") is busy.\n",
                        bridgeq->ofp_bridge->dpid);
      bridgeq->refs--;
      bridgeq_unlock(bridgeq);
    } else {
      if (bridgeq->ofp_bridge != NULL) {
        lagopus_msg_debug(500, "bridgeq(dpid = %"PRIu64") is free.\n",
                          bridgeq->ofp_bridge->dpid);
        ofp_bridge_destroy(bridgeq->ofp_bridge);
        bridgeq->ofp_bridge = NULL;
      }

      /* destroy polls. */
      for (i = 0; i < MAX_BRIDGE_POLLS; i++) {
        if (bridgeq->polls[i] != NULL) {
          lagopus_qmuxer_poll_destroy(&(bridgeq->polls[i]));
          bridgeq->polls[i] = NULL;
        }
      }
      for (i = 0; i < MAX_BRIDGE_DP_POLLS; i++) {
        if (bridgeq->dp_polls[i] != NULL) {
          lagopus_qmuxer_poll_destroy(&(bridgeq->dp_polls[i]));
          bridgeq->dp_polls[i] = NULL;
        }
      }
      bridgeq_unlock(bridgeq);
      if (bridgeq->lock != NULL) {
        lagopus_mutex_destroy(&(bridgeq->lock));
        bridgeq->lock = NULL;
      }
      free(bridgeq);
    }
  }
}

lagopus_result_t
ofp_bridgeq_mgr_clear(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  bridgeq_mgr_lock();
  bridgeqs_clear();
  bridgeq_mgr_unlock();

  ret = lagopus_hashmap_clear(&bridgeq_table, true);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

  return ret;
}

void
ofp_bridgeq_mgr_destroy(void) {
  lagopus_hashmap_destroy(&bridgeq_table, true);
  bridgeq_table = NULL;
  lagopus_mutex_destroy(&lock);
  lock = NULL;
}

/* iterate bridgeq_table for to_array. */
static bool
iter_to_array(void *key, void *val, lagopus_hashentry_t he, void *arg) {
  bool ret = false;
  (void) key;
  (void) he;
  (void) arg;

  if (val != NULL) {
    struct ofp_bridgeq *bridgeq = (struct ofp_bridgeq *)val;

    if (n_bridgeqs < MAX_BRIDGES) {
      bridgeqs[n_bridgeqs] = bridgeq;
      n_bridgeqs++;
      ret = true;
    } else {
      lagopus_msg_warning("over max length.\n");
      ret = false;
    }
  } else {
    lagopus_msg_warning("val is NULL.\n");
  }

  return ret;
}

static inline lagopus_result_t
bridgeq_mgr_map_to_array(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (bridgeq_table != NULL) {
    if (lagopus_hashmap_size(&bridgeq_table) != 0) {
      /* NOTE:
       *   hashmap_iterate() is very slow.
       */
      ret = lagopus_hashmap_iterate(&bridgeq_table,
                                    iter_to_array,
                                    NULL);
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_msg_warning("bridgeq_table is NULL.\n");
  }

  return ret;
}

static inline lagopus_result_t
bridge_register_internal(uint64_t dpid, struct ofp_bridgeq *bridgeq) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *brq = bridgeq;

  if (bridgeq != NULL && bridgeq->ofp_bridge != NULL
      && dpid == bridgeq->ofp_bridge->dpid) {
    /* add to bridgeq_table */
    ret = lagopus_hashmap_add(&bridgeq_table,
                              (void *)bridgeq->ofp_bridge->dpid,
                              (void **)&brq, true);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
bridge_unregister_internal(uint64_t dpid, bool force) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *brq = NULL;

  /* delete from bridgeq_table */
  ret = lagopus_hashmap_delete(&bridgeq_table,
                               (void *)dpid, (void **)&brq, false);
  if (ret == LAGOPUS_RESULT_OK && brq != NULL) {
    /* delete bridgeq. */
    lagopus_msg_debug(1, "unregister bridgeq(%p). dpid: %lu\n",
                      brq, dpid);
    ofp_bridgeq_free(brq, force);
    ret = LAGOPUS_RESULT_OK;
  } else {
    lagopus_perror(ret);
  }

  return ret;
}

static inline lagopus_result_t
bridge_register(uint64_t dpid, struct ofp_bridgeq *bridgeq) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (bridgeq != NULL) {
    bridgeq_mgr_lock();

    if ((ret = bridge_register_internal(dpid, bridgeq)) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      ofp_bridgeq_free(bridgeq, true);
    } else {
      /* update array. */
      bridgeqs_clear();
      ret = bridgeq_mgr_map_to_array();
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        (void) bridge_unregister_internal(dpid, true);
      }
    }
    bridgeq_mgr_unlock();
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
bridge_unregister(uint64_t dpid, bool force) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  bridgeq_mgr_lock();
  bridgeqs_clear();

  ret = bridge_unregister_internal(dpid, force);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  } else {
    /* update array. */
    ret = bridgeq_mgr_map_to_array();
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
    }
  }
  bridgeq_mgr_unlock();

  return ret;
}

lagopus_result_t
ofp_bridgeq_mgr_bridge_register(uint64_t dpid,
                                const char *name,
                                datastore_bridge_info_t *info,
                                datastore_bridge_queue_info_t *q_info) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridge *ofp_bridge = NULL;
  struct ofp_bridgeq *bridgeq;

  lagopus_msg_debug(1, "called. (dpid: %"PRIu64")\n", dpid);

  if (bridgeq_table != NULL) {
    /* check not exists */
    ret = lagopus_hashmap_find(&bridgeq_table,
                               (void *)dpid, (void **)&bridgeq);
    if (ret == LAGOPUS_RESULT_NOT_FOUND) {
      /* allocate bridgeq */
      if ((bridgeq = (struct ofp_bridgeq *)malloc(sizeof(struct ofp_bridgeq)))
          == NULL) {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      } else if ((ret = ofp_bridge_create(&ofp_bridge, dpid,
                                          name, info, q_info))
                 != LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        free(bridgeq);
      } else {
        /* init bridgeq */
        bridgeq->ofp_bridge = ofp_bridge;
        bridgeq->refs = 1;
        bridgeq->lock = NULL;
        memset(bridgeq->polls, 0, sizeof(bridgeq->polls));
        memset(bridgeq->dp_polls, 0, sizeof(bridgeq->dp_polls));
        if ((ret = lagopus_mutex_create(&(bridgeq->lock)))
            != LAGOPUS_RESULT_OK) { /* create lock */
          lagopus_perror(ret);
          ofp_bridgeq_free(bridgeq, true);
        } else if ((ret = lagopus_qmuxer_poll_create(
                            &(bridgeq->polls[EVENTQ_POLL]),
                            ofp_bridge->eventq,
                            LAGOPUS_QMUXER_POLL_READABLE))
                   != LAGOPUS_RESULT_OK) { /* create eventq poll */
          lagopus_perror(ret);
          ofp_bridgeq_free(bridgeq, true);
        } else if ((ret = lagopus_qmuxer_poll_create(
                            &(bridgeq->polls[DATAQ_POLL]),
                            ofp_bridge->dataq,
                            LAGOPUS_QMUXER_POLL_READABLE))
                   != LAGOPUS_RESULT_OK) { /* create dataq poll */
          lagopus_perror(ret);
          ofp_bridgeq_free(bridgeq, true);
#ifdef OFPH_POLL_WRITING
        } else if ((ret = lagopus_qmuxer_poll_create(
                            &(bridgeq->polls[EVENT_DATAQ_POLL]),
                            ofp_bridge->event_dataq,
                            LAGOPUS_QMUXER_POLL_WRITABLE))
                   != LAGOPUS_RESULT_OK) { /* create event_dataq poll */
          lagopus_perror(ret);
          ofp_bridgeq_free(bridgeq, true);
#endif  /* OFPH_POLL_WRITING */
        } else if ((ret = lagopus_qmuxer_poll_create(
                            &(bridgeq->dp_polls[EVENT_DATAQ_DP_POLL]),
                            ofp_bridge->event_dataq,
                            LAGOPUS_QMUXER_POLL_READABLE))
                   != LAGOPUS_RESULT_OK) { /* create dataq poll for DataPlen */
          lagopus_perror(ret);
          ofp_bridgeq_free(bridgeq, true);
        } else {
          /* succeeded all create. */
          /* register. */
          if ((ret = bridge_register(dpid, bridgeq)) != LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
          }
        }
      }
    } else if (ret == LAGOPUS_RESULT_OK) {
      ret = LAGOPUS_RESULT_ALREADY_EXISTS;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_msg_warning("bridgeq_table is NULL.\n");
  }

  return ret;
}

lagopus_result_t
ofp_bridgeq_mgr_bridge_unregister(uint64_t dpid) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_debug(1, "called. (dpid: %"PRIu64")\n", dpid);

  if (bridgeq_table != NULL) {
    if ((ret = bridge_unregister(dpid, false)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_msg_warning("bridgeq_table is NULL.\n");
  }

  return ret;
}

lagopus_result_t
ofp_bridgeq_mgr_bridge_lookup(uint64_t dpid,
                              struct ofp_bridgeq **bridgeq) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (bridgeq != NULL) {
    if (bridgeq_table != NULL) {
      bridgeq_mgr_lock();
      ret = lagopus_hashmap_find(&bridgeq_table,
                                 (void *)dpid, (void **)bridgeq);
      if (ret == LAGOPUS_RESULT_OK) {
        ofp_bridgeq_refs_get(*bridgeq);
      }
      bridgeq_mgr_unlock();
    } else {
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
      lagopus_msg_warning("bridgeq_table is NULL.\n");
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_bridgeq_mgr_bridgeqs_to_array(struct ofp_bridgeq *brqs[],
                                  uint64_t *count,
                                  const size_t max_size) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t i;

  if (brqs != NULL && count != NULL &&
      max_size <= MAX_BRIDGES) {
    if (bridgeq_table != NULL) {
      bridgeq_mgr_lock();
      for (i = 0; i < n_bridgeqs; i++) {
        brqs[i] = bridgeqs[i];
        /* inclement reference counter */
        ofp_bridgeq_refs_get(bridgeqs[i]);
      }
      *count = n_bridgeqs;
      bridgeq_mgr_unlock();
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
      lagopus_msg_warning("bridgeq_table is NULL.\n");
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

#define GET_POLLS(_type, _brq) (((_type) == AGENT_POLLS) ? _brq->polls : _brq->dp_polls)

static inline lagopus_result_t
bridgeq_mgr_polls_get(enum ofp_bridgeq_polls_type type,
                      lagopus_qmuxer_poll_t *polls,
                      struct ofp_bridgeq *brqs[],
                      uint64_t *count,
                      const uint64_t bridgeqs_size,
                      const uint64_t max_bridge_polls,
                      const uint64_t max_polls) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t i;

  if (polls != NULL && brqs != NULL &&
      count != NULL) {
    if (bridgeq_table != NULL) {
      if ((*count + (bridgeqs_size * max_bridge_polls)) <= max_polls) {
        for (i = 0; i < bridgeqs_size; i++) {
          bridgeq_lock(brqs[i]);
          memcpy(&polls[*count], GET_POLLS(type, brqs[i]),
                 sizeof(lagopus_qmuxer_poll_t) * max_bridge_polls);
          (*count) += max_bridge_polls;
          bridgeq_unlock(brqs[i]);
        }
        ret = LAGOPUS_RESULT_OK;
      } else {
        lagopus_msg_warning("poll over flow.\n");
        ret = LAGOPUS_RESULT_OUT_OF_RANGE;
      }
    } else {
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
      lagopus_msg_warning("bridgeq_table is NULL.\n");
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_bridgeq_mgr_polls_get(lagopus_qmuxer_poll_t *polls,
                          struct ofp_bridgeq *brqs[],
                          uint64_t *count,
                          const uint64_t bridgeqs_size) {
  return bridgeq_mgr_polls_get(AGENT_POLLS, polls, brqs, count, bridgeqs_size,
                               MAX_BRIDGE_POLLS, MAX_POLLS);
}

lagopus_result_t
ofp_bridgeq_mgr_dp_polls_get(lagopus_qmuxer_poll_t *polls,
                             struct ofp_bridgeq *brqs[],
                             uint64_t *count,
                             const uint64_t bridgeqs_size) {
  return bridgeq_mgr_polls_get(DP_POLLS, polls, brqs, count, bridgeqs_size,
                               MAX_BRIDGE_DP_POLLS, MAX_DP_POLLS);
}

void
ofp_bridgeq_mgr_bridgeq_free(struct ofp_bridgeq *brqs) {
  if (brqs != NULL) {
    bridgeq_mgr_lock();
    ofp_bridgeq_free(brqs, false);
    bridgeq_mgr_unlock();
  }
}

void
ofp_bridgeq_mgr_bridgeqs_free(struct ofp_bridgeq *brqs[],
                              const uint64_t num) {
  uint64_t i;

  if (brqs != NULL) {
    bridgeq_mgr_lock();
    for (i = 0; i < num; i++) {
      ofp_bridgeq_free(brqs[i], false);
    }
    bridgeq_mgr_unlock();
  }
}

/* getter. */
struct ofp_bridge *
ofp_bridgeq_mgr_bridge_get(struct ofp_bridgeq *bridgeq) {
  struct ofp_bridge *ofp_bridge;

  bridgeq_lock(bridgeq);
  ofp_bridge = bridgeq->ofp_bridge;
  bridgeq_unlock(bridgeq);

  return ofp_bridge;
}

static inline
lagopus_qmuxer_poll_t
poll_get(struct ofp_bridgeq *bridgeq, enum ofp_bridgeq_agent_poll_type type) {
  lagopus_qmuxer_poll_t poll;

  bridgeq_lock(bridgeq);
  poll = bridgeq->polls[type];
  bridgeq_unlock(bridgeq);

  return poll;
}

lagopus_qmuxer_poll_t
ofp_bridgeq_mgr_eventq_poll_get(struct ofp_bridgeq *bridgeq) {
  return poll_get(bridgeq, EVENTQ_POLL);
}

lagopus_qmuxer_poll_t
ofp_bridgeq_mgr_dataq_poll_get(struct ofp_bridgeq *bridgeq) {
  return poll_get(bridgeq, DATAQ_POLL);
}

#ifdef OFPH_POLL_WRITING
lagopus_qmuxer_poll_t
ofp_bridgeq_mgr_event_dataq_poll_get(struct ofp_bridgeq *bridgeq) {
  return poll_get(bridgeq, EVENT_DATAQ_POLL);
}
#endif

static inline
lagopus_qmuxer_poll_t
dp_poll_get(struct ofp_bridgeq *bridgeq, enum ofp_bridgeq_dp_poll_type type) {
  lagopus_qmuxer_poll_t poll;

  bridgeq_lock(bridgeq);
  poll = bridgeq->dp_polls[type];
  bridgeq_unlock(bridgeq);

  return poll;
}

lagopus_qmuxer_poll_t
ofp_bridgeq_mgr_event_dataq_dp_poll_get(struct ofp_bridgeq *bridgeq) {
  return poll_get(bridgeq, EVENT_DATAQ_DP_POLL);
}

void
ofp_bridgeq_mgr_poll_reset(lagopus_qmuxer_poll_t *polls,
                           const size_t size) {
  if (polls != NULL) {
    memset(polls, 0, sizeof(lagopus_qmuxer_poll_t) * size);
  }
}

lagopus_result_t
ofp_bridgeq_mgr_dataq_max_batches_set(uint64_t dpid,
                                      uint16_t val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *bridgeq = NULL;

  if (bridgeq_table != NULL) {
    bridgeq_mgr_lock();
    ret = lagopus_hashmap_find(&bridgeq_table,
                               (void *)dpid, (void **)&bridgeq);
    if (ret == LAGOPUS_RESULT_OK) {
      bridgeq_lock(bridgeq);
      if ((ret = ofp_bridge_dataq_max_batches_set(
              bridgeq->ofp_bridge, val)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
      }
      bridgeq_unlock(bridgeq);
    }
    bridgeq_mgr_unlock();
  } else {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_msg_warning("bridgeq_table is NULL.\n");
  }

  return ret;
}

lagopus_result_t
ofp_bridgeq_mgr_eventq_max_batches_set(uint64_t dpid,
                                       uint16_t val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *bridgeq = NULL;

  if (bridgeq_table != NULL) {
    bridgeq_mgr_lock();
    ret = lagopus_hashmap_find(&bridgeq_table,
                               (void *)dpid, (void **)&bridgeq);
    if (ret == LAGOPUS_RESULT_OK) {
      bridgeq_lock(bridgeq);
      if ((ret = ofp_bridge_eventq_max_batches_set(
              bridgeq->ofp_bridge, val)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
      }
      bridgeq_unlock(bridgeq);
    }
    bridgeq_mgr_unlock();
  } else {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_msg_warning("bridgeq_table is NULL.\n");
  }

  return ret;
}

lagopus_result_t
ofp_bridgeq_mgr_event_dataq_max_batches_set(uint64_t dpid,
                                            uint16_t val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *bridgeq = NULL;

  if (bridgeq_table != NULL) {
    bridgeq_mgr_lock();
    ret = lagopus_hashmap_find(&bridgeq_table,
                               (void *)dpid, (void **)&bridgeq);
    if (ret == LAGOPUS_RESULT_OK) {
      bridgeq_lock(bridgeq);
      if ((ret = ofp_bridge_event_dataq_max_batches_set(
              bridgeq->ofp_bridge, val)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
      }
      bridgeq_unlock(bridgeq);
    }
    bridgeq_mgr_unlock();
  } else {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_msg_warning("bridgeq_table is NULL.\n");
  }

  return ret;
}

lagopus_result_t
ofp_bridgeq_mgr_stats_get(uint64_t dpid,
                          datastore_bridge_stats_t *stats) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *bridgeq = NULL;
  uint16_t dataq_stats = 0;
  uint16_t eventq_stats = 0;
  uint16_t event_dataq_stats = 0;

  if (bridgeq_table != NULL) {
    bridgeq_mgr_lock();
    ret = lagopus_hashmap_find(&bridgeq_table,
                               (void *)dpid, (void **)&bridgeq);
    if (ret == LAGOPUS_RESULT_OK) {
      bridgeq_lock(bridgeq);

      if ((ret = ofp_bridge_dataq_stats_get(
              bridgeq->ofp_bridge, &dataq_stats)) != LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if ((ret = ofp_bridge_eventq_stats_get(
              bridgeq->ofp_bridge, &eventq_stats)) != LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
      if ((ret = ofp_bridge_event_dataq_stats_get(
              bridgeq->ofp_bridge, &event_dataq_stats)) != LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      stats->packet_inq_entries = dataq_stats;
      stats->up_streamq_entries = eventq_stats;
      stats->down_streamq_entries = event_dataq_stats;

   done:
      bridgeq_unlock(bridgeq);
    }
    bridgeq_mgr_unlock();
  } else {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_msg_warning("bridgeq_table is NULL.\n");
  }

  return ret;
}

lagopus_result_t
ofp_bridgeq_mgr_dataq_clear(uint64_t dpid) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *bridgeq = NULL;

  if (bridgeq_table != NULL) {
    bridgeq_mgr_lock();
    ret = lagopus_hashmap_find(&bridgeq_table,
                               (void *)dpid, (void **)&bridgeq);
    if (ret == LAGOPUS_RESULT_OK) {
      bridgeq_lock(bridgeq);
      ret = ofp_bridge_dataq_clear(bridgeq->ofp_bridge);
      bridgeq_unlock(bridgeq);
    }
    bridgeq_mgr_unlock();
  } else {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_msg_warning("bridgeq_table is NULL.\n");
  }

  return ret;
}

lagopus_result_t
ofp_bridgeq_mgr_eventq_clear(uint64_t dpid) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *bridgeq = NULL;

  if (bridgeq_table != NULL) {
    bridgeq_mgr_lock();
    ret = lagopus_hashmap_find(&bridgeq_table,
                               (void *)dpid, (void **)&bridgeq);
    if (ret == LAGOPUS_RESULT_OK) {
      bridgeq_lock(bridgeq);
      ret = ofp_bridge_eventq_clear(bridgeq->ofp_bridge);
      bridgeq_unlock(bridgeq);
    }
    bridgeq_mgr_unlock();
  } else {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_msg_warning("bridgeq_table is NULL.\n");
  }

  return ret;
}

lagopus_result_t
ofp_bridgeq_mgr_event_dataq_clear(uint64_t dpid) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *bridgeq = NULL;

  if (bridgeq_table != NULL) {
    bridgeq_mgr_lock();
    ret = lagopus_hashmap_find(&bridgeq_table,
                               (void *)dpid, (void **)&bridgeq);
    if (ret == LAGOPUS_RESULT_OK) {
      bridgeq_lock(bridgeq);
      ret = ofp_bridge_event_dataq_clear(bridgeq->ofp_bridge);
      bridgeq_unlock(bridgeq);
    }
    bridgeq_mgr_unlock();
  } else {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_msg_warning("bridgeq_table is NULL.\n");
  }

  return ret;
}

lagopus_result_t
ofp_bridgeq_mgr_name_get(uint64_t dpid,
                         char **name) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *bridgeq = NULL;

  if (bridgeq_table != NULL) {
    bridgeq_mgr_lock();
    ret = lagopus_hashmap_find(&bridgeq_table,
                               (void *)dpid, (void **)&bridgeq);
    if (ret == LAGOPUS_RESULT_OK) {
      bridgeq_lock(bridgeq);

      if ((ret = ofp_bridge_name_get(bridgeq->ofp_bridge,
                                     name)) != LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

   done:
      bridgeq_unlock(bridgeq);
    } else {
      *name = NULL;
    }
    bridgeq_mgr_unlock();
  } else {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_msg_warning("bridgeq_table is NULL.\n");
  }

  return ret;
}

lagopus_result_t
ofp_bridgeq_mgr_info_get(uint64_t dpid,
                         datastore_bridge_info_t *info) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *bridgeq = NULL;

  if (bridgeq_table != NULL) {
    bridgeq_mgr_lock();
    ret = lagopus_hashmap_find(&bridgeq_table,
                               (void *)dpid, (void **)&bridgeq);
    if (ret == LAGOPUS_RESULT_OK) {
      bridgeq_lock(bridgeq);

      if ((ret = ofp_bridge_info_get(bridgeq->ofp_bridge,
                                     info)) != LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

   done:
      bridgeq_unlock(bridgeq);
    }
    bridgeq_mgr_unlock();
  } else {
    ret = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_msg_warning("bridgeq_table is NULL.\n");
  }

  return ret;
}
