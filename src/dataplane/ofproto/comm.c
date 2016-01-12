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
 *      @file   comm.c
 *      @brief  Dataplane communicator thread functions.
 */

#ifdef HAVE_DPDK
#include <rte_config.h>
#include <rte_lcore.h>
#include <rte_atomic.h>
#include <rte_launch.h>
#endif /* HAVE_DPDK */

#include "lagopus_apis.h"
#include "lagopus/ofp_handler.h"
#include "lagopus/flowdb.h"
#include "lagopus/bridge.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
#include "lagopus/ofcache.h"
#include "lagopus/ofp_bridge.h"
#include "lagopus/ofp_bridgeq_mgr.h"
#include "lagopus/dp_apis.h"

#include "pktbuf.h"
#include "packet.h"
#include "thread.h"
#include "mbtree.h"

#include "City.h"
#include "murmurhash3.h"

#ifdef HAVE_DPDK
#include "dpdk/dpdk.h"

rte_atomic32_t dpdk_stop;
#endif /* HAVE_DPDK */

#define MUXER_TIMEOUT 100LL * 1000LL * 1000LL
#define PUT_TIMEOUT 100LL * 1000LL * 1000LL

static lagopus_qmuxer_poll_t *polls = NULL;
static lagopus_qmuxer_t muxer = NULL;

/* process event_dataq. */
static inline lagopus_result_t
process_event_dataq_entry(struct flowcache *cache,
                          struct ofp_bridge *ofp_bridge,
                          struct eventq_data *data) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  struct bridge *bridge = NULL;

  lagopus_msg_debug(10, "get item. %p\n", data);
  if (ofp_bridge == NULL || data == NULL) {
    lagopus_msg_warning("process_event_dataq_entry arg is NULL\n");
    rv = LAGOPUS_RESULT_INVALID_ARGS;
    goto done;
  }

  flowdb_check_update(NULL);
  flowdb_rdlock(NULL);
  bridge = dp_bridge_lookup_by_dpid(ofp_bridge->dpid);
  if (bridge != NULL) {
    struct eventq_data *reply;
    struct lagopus_packet *pkt;
    struct port *port;
    uint64_t dpid = bridge->dpid;
    struct pbuf *pbuf;
    uint16_t data_len;

    switch (data->type) {
      case LAGOPUS_EVENTQ_PACKET_OUT:
        pbuf = data->packet_out.data;
        if (pbuf == NULL) {
          break;
        }
        /* create packet structure and send to specified port. */
        pkt = alloc_lagopus_packet();
        if (pkt != NULL) {
          struct port cport;
          struct ofp_packet_out *ofp_packet_out;

          port = NULL;
          ofp_packet_out = &data->packet_out.ofp_packet_out;
          if (ofp_packet_out->in_port != OFPP_CONTROLLER &&
              ofp_packet_out->in_port != OFPP_ANY) {
            port = port_lookup(bridge->ports,
                               ofp_packet_out->in_port);
          }
          if (port == NULL) {
            /**
             * silly hack.
             * bridge should have complete struct port for controller port.
             */
            port = &cport;
            cport.ifindex = 0xff;
            cport.bridge = bridge;
            cport.ofp_port.port_no = ofp_packet_out->in_port;
          }
          rv = pbuf_length_get(pbuf, &data_len);
          if (rv != LAGOPUS_RESULT_OK) {
            lagopus_msg_error("pbuf_length_get error (%s).\n",
                              lagopus_error_get_string(rv));
            break;
          }

          (void)OS_M_APPEND(pkt->mbuf, data_len);
          DECODE_GET(OS_MTOD(pkt->mbuf, char *), data_len);
          lagopus_packet_init(pkt, pkt->mbuf, port);
          pkt->cache = NULL;
          pkt->hash64 = 0;
          if (lagopus_register_action_hook != NULL) {
            struct action *action;

            TAILQ_FOREACH(action, &data->packet_out.action_list, entry) {
              lagopus_register_action_hook(action);
            }
          }
          /* XXX so far, call from comm thread directly. */
          OS_M_ADDREF(pkt->mbuf);
          execute_action(pkt, &data->packet_out.action_list);
          lagopus_packet_free(pkt);
        }
        break;

      case LAGOPUS_EVENTQ_BARRIER_REQUEST:
#ifdef USE_MBTREE
        /* rebuild branch tree. */
        {
          struct flowdb *flowdb;
          struct table *table;
          int i;

          flowdb = bridge->flowdb;
          if (flowdb->tables != NULL) {
            for (i = 0; i < flowdb->table_size; i++) {
              table = flowdb->tables[i];
              if (table != NULL) {
                cleanup_mbtree(table->flow_list);
                build_mbtree(table->flow_list);
              }
            }
          }
        }
#endif /* USE_MBTREE */
        /* flush pending requests from OFC, and reply. */
        if (cache != NULL) {
          /* clear my own cache */
          clear_all_cache(cache);
        }
#ifdef HAVE_DPDK
        /* and worker cache */
        clear_worker_flowcache(true);
#endif /* HAVE_DPDK */
        reply = malloc(sizeof(*reply));
        if (reply == NULL) {
          break;
        }
        reply->type = LAGOPUS_EVENTQ_BARRIER_REPLY;
        reply->free = ofp_barrier_free;
        reply->barrier.req = NULL;
        reply->barrier.xid = data->barrier.xid;
        reply->barrier.channel_id = data->barrier.channel_id;
        (void) ofp_handler_eventq_data_put(dpid, &reply, PUT_TIMEOUT);
        break;
      default:
        break;
    }
  } else {
    rv = LAGOPUS_RESULT_INVALID_OBJECT;
  }
  flowdb_rdunlock(NULL);

done:
  return rv;
}

/* read event_dataq */
static inline lagopus_result_t
event_dataq_dequeue(struct flowcache *cache,
                    struct ofp_bridge *ofp_bridge,
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
        rv = process_event_dataq_entry(cache, ofp_bridge, gets[i]);
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
dequeue(struct flowcache *cache,
        struct ofp_bridgeq *brqs) {
  lagopus_result_t rv = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridge *bridge;
  lagopus_qmuxer_poll_t poll;

  if (brqs != NULL) {
    bridge = ofp_bridgeq_mgr_bridge_get(brqs);
    poll = ofp_bridgeq_mgr_event_dataq_dp_poll_get(brqs);
    rv = event_dataq_dequeue(cache, bridge, poll);
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
 * Communicate with agent loop function.
 *
 * @param[in]   t       Thread object pointer.
 * @param[in]   arg     Do not used argument.
 */
static lagopus_result_t
comm_thread_loop(const lagopus_thread_t *t, void *arg) {
  lagopus_result_t rv = LAGOPUS_RESULT_ANY_FAILURES;
  struct dataplane_arg *dparg;
  struct flowcache *cache;
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
  bool *running = NULL;
  (void) t;

  rv = global_state_wait_for(GLOBAL_STATE_STARTED,
                             &cur_state,
                             &cur_grace,
                             -1);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }

  dparg = arg;
  running = dparg->running;

#ifdef HAVE_DPDK
  if (!app.no_cache) {
    cache = init_flowcache(app.kvs_type);
  } else {
    cache = NULL;
  }
#else
  cache = NULL;
#endif /* HAVE_DPDK */

  while (*running == true) {
#ifdef HAVE_DPDK
    if (rte_atomic32_read(&dpdk_stop) != 0) {
      rv = LAGOPUS_RESULT_OK;
      break;
    }
#endif /* HAVE_DPDK */

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
          rv = dequeue(cache, bridgeqs[i]);
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
  if (cache != NULL) {
    fini_flowcache(cache);
  }

  return rv;
}

/**
 * dataplane comm lagopus thread.
 */

static lagopus_thread_t comm_thread = NULL;
static bool comm_run = false;
static lagopus_mutex_t comm_lock = NULL;

lagopus_result_t
dp_comm_thread_init(int argc,
                    const char *const argv[],
                    __UNUSED void *extarg,
                    lagopus_thread_t **thdptr) {
  lagopus_result_t rv = LAGOPUS_RESULT_ANY_FAILURES;
  static struct dataplane_arg commarg;

  (void) argc;
  (void) argv;

  commarg.threadptr = &comm_thread;
  commarg.lock = &comm_lock;
  commarg.running = &comm_run;
  *thdptr = &comm_thread;

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

  return lagopus_thread_create(&comm_thread, comm_thread_loop,
                               dp_finalproc, dp_freeproc,
                               "dp_comm", &commarg);
}

lagopus_result_t
dp_comm_thread_start(void) {
  return dp_thread_start(&comm_thread, &comm_lock, &comm_run);
}

void
dp_comm_thread_fini(void) {
  if (polls != NULL) {
    free(polls);
    polls = NULL;
  }
  if (muxer != NULL) {
    lagopus_qmuxer_destroy(&muxer);
    muxer = NULL;
  }

  dp_thread_finalize(&comm_thread);
}

lagopus_result_t
dp_comm_thread_shutdown(shutdown_grace_level_t level) {
  return dp_thread_shutdown(&comm_thread, &comm_lock, &comm_run, level);
}

lagopus_result_t
dp_comm_thread_stop(void) {
  return dp_thread_stop(&comm_thread, &comm_run);
}

#if 0
#define MODIDX_DPCOMM  LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE + 109

static void dpcomm_ctors(void) __attr_constructor__(MODIDX_DPCOMM);
static void dpcomm_dtors(void) __attr_constructor__(MODIDX_DPCOMM);

static void dpcomm_ctors (void) {
  lagopus_result_t rv;
  const char *name = "dp_comm";

  rv = lagopus_module_register(name,
                               dp_comm_thread_init,
                               NULL,
                               dp_comm_thread_start,
                               dp_comm_thread_shutdown,
                               dp_comm_thread_stop,
                               dp_comm_thread_fini,
                               NULL);
  if (rv != LAGOPUS_RESULT_OK) {
    lagopus_perror(rv);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }
}
#endif /* 0 */
