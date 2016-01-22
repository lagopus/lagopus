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

#include <stdbool.h>
#include <stdint.h>
#include "lagopus_apis.h"
#include "lagopus_thread_internal.h"
#include "lagopus_session.h"
#include "lagopus/eventq_data.h"
#include "lagopus/pbuf.h"
#include "lagopus/ofp_dp_apis.h"
#include "channel.h"
#include "openflow13packet.h"
#include "lagopus/ofp_bridgeq_mgr.h"
#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_bridge.h"
#include "lagopus/dp_apis.h"
#include "ofp_apis.h"

/* qmuxer timeout in thread_main */
#define MUXER_TIMEOUT 100LL * 1000LL * 1000LL
#define PUT_TIMEOUT 100LL * 1000LL * 1000LL
#define SHUTDOWN_TIMEOUT 100LL * 1000LL * 1000LL * 1000LL

#define CHANNELQ_SIZE 1000LL

#ifdef MUXER_MAX_SIZE
#define MUXER_FAIRNESS(q_size)                                  \
  q_size = (q_size <= MUXER_MAX_SIZE) ? q_size : MUXER_MAX_SIZE
#else
#define MUXER_FAIRNESS(q_size)
#endif  /* MUXER_MAX_SIZE */

/* ofp_handler's status */
enum ofp_handler_running_status {
  OFPH_RUNNING,
  OFPH_SHUTDOWN_GRACEFULLY,
  OFPH_SHUTDOWN_RIGHT_NOW,
  OFPH_SHUTDOWNED,
};

/* ofp_handler thread record */
struct ofp_handler_record {
  lagopus_thread_record m_thread; /* must be on the head. */
  channelq_t m_channelq;          /* queue to channel_mgr */
  lagopus_qmuxer_t muxer;         /* muxer */

  lagopus_qmuxer_poll_t *m_polls; /* poll objects */
  volatile uint64_t m_n_polls;                  /* num of polls */

  enum ofp_handler_running_status  m_status;
  lagopus_mutex_t m_status_lock;   /* lock of m_status */
};
typedef struct ofp_handler_record *ofp_handler_t;

/*
 * values
 */
static ofp_handler_t s_ofp_handler = NULL;
static pthread_once_t s_initialized = PTHREAD_ONCE_INIT;
static volatile bool s_is_started = false;
static volatile bool s_is_running = false;
static volatile uint16_t channelq_size = CHANNELQ_SIZE;
static volatile uint16_t channelq_max_batches = CHANNELQ_SIZE;

/*
 * prototype
 */
/* process channelq */
static inline lagopus_result_t
s_process_channelq_entry(struct channelq_data *channel);
/* process dataq */
static inline lagopus_result_t
s_process_dataq_entry(struct ofp_bridge *ofp_bridge,
                      struct eventq_data *entry);
/* process eventq */
static inline lagopus_result_t
s_process_eventq_entry(struct ofp_bridge *ofp_bridge,
                       struct eventq_data *entry);
/* check status */
static inline bool
s_validate_ofp_handler(void);
static inline bool
s_ofp_handler_is_canceled(void);
/* thread procs */
static lagopus_result_t
s_ofph_thread_main(const lagopus_thread_t *selfptr,
                   void *arg);
static void
s_ofph_thread_shutdown(const lagopus_thread_t *selfptr,
                       bool is_canceled,
                       void *arg);
static void
s_ofph_thread_freeup(const lagopus_thread_t *selfptr,
                     void *arg);
/* thread procs (internal) */
static void
s_initialize_once(void);
static inline lagopus_result_t
s_recreate(void);
static inline void
s_destroy_for_recreate(void);
/* channel_free() wrapper for bbq */
static void
s_channel_freeup_proc(void **val);
/* dequeue(or enqueue) each queues */
static inline lagopus_result_t
s_channelq_dequeue(channelq_t *q_ptr,
                   lagopus_qmuxer_poll_t qpoll);
static inline lagopus_result_t
s_eventq_dequeue(struct ofp_bridge *ofp_bridge,
                 lagopus_qmuxer_poll_t qpoll);
static inline lagopus_result_t
s_dataq_dequeue(struct ofp_bridge *ofp_bridge,
                lagopus_qmuxer_poll_t qpoll);
#ifdef OFPH_POLL_WRITING
static inline lagopus_result_t
s_event_dataq_enqueue(struct ofp_bridge *ofp_bridge,
                      lagopus_qmuxer_poll_t *poll_ptr);
#endif  /* OFPH_POLL_WRITING */
/* management m_shutdowned */
static inline enum ofp_handler_running_status
s_get_status(void);
static inline void
s_set_status(enum ofp_handler_running_status set);



/*
 * public functions
 */
lagopus_result_t
ofp_handler_initialize(void *arg,
                       lagopus_thread_t **retptr) {
  (void) arg;

  lagopus_msg_info("called. (retptr: %p)\n", retptr);
  (void)pthread_once(&s_initialized, s_initialize_once);
  if (s_validate_ofp_handler() == true) {
    if (retptr != NULL) {
      *retptr = (lagopus_thread_t *)&s_ofp_handler;
    }
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
ofp_handler_start(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_msg_info("called.\n");
  if (s_validate_ofp_handler() == true) {
    if (s_get_status() != OFPH_RUNNING) {
      mbar();
      s_is_running = true;
      s_set_status(OFPH_RUNNING);
      res = s_recreate();
      if (res == LAGOPUS_RESULT_OK) {
        res = lagopus_thread_start((lagopus_thread_t *)&s_ofp_handler, false);
        if (res != LAGOPUS_RESULT_OK) {
          lagopus_perror(res);
        }
      } else {
        lagopus_perror(res);
      }
    } else {
      res = LAGOPUS_RESULT_ALREADY_EXISTS;
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return res;
}

lagopus_result_t
ofp_handler_shutdown(shutdown_grace_level_t level) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_valid = false;

  lagopus_msg_info("called. (level:%d)\n", level);
  if (s_ofp_handler != NULL) {
    res = lagopus_thread_is_valid((const lagopus_thread_t *)&s_ofp_handler,
                                  &is_valid);
    if (res == LAGOPUS_RESULT_OK) {
      if (is_valid == true) {
        mbar();
        s_is_running = false;
        switch (level) {
          case SHUTDOWN_RIGHT_NOW:
            s_set_status(OFPH_SHUTDOWN_RIGHT_NOW);
            break;
          case SHUTDOWN_GRACEFULLY:
            s_set_status(OFPH_SHUTDOWN_GRACEFULLY);
            break;
          default:
            res = LAGOPUS_RESULT_INVALID_ARGS;
            break;
        }
        if (res != LAGOPUS_RESULT_OK) {
          lagopus_perror(res);
        }
      } else {
        res = LAGOPUS_RESULT_INVALID_OBJECT;
        lagopus_msg_error("ofp-handler thread is invalid.\n");
      }
    } else {
      lagopus_perror(res);
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_OBJECT;
    lagopus_msg_error("ofp-handler thread is NULL.\n");
  }
  return res;
}

lagopus_result_t
ofp_handler_stop(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_msg_info("called.\n");
  if (s_validate_ofp_handler() == true
      && s_ofp_handler_is_canceled() == false) {
    res = lagopus_thread_cancel((lagopus_thread_t *)&s_ofp_handler);
  }
  return res;
}

void
ofp_handler_finalize(void) {
  lagopus_msg_info("called.\n");
  if (s_validate_ofp_handler() == true) {
    lagopus_thread_destroy((lagopus_thread_t *)&s_ofp_handler);
  }
}

lagopus_result_t
ofp_handler_get_channelq(lagopus_bbq_t **retptr) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_valid = false;

  lagopus_msg_debug(1, "called. (retptr: %p)\n", retptr);
  if (retptr != NULL) {
    if (s_ofp_handler != NULL) {
      res = lagopus_thread_is_valid((const lagopus_thread_t *)&s_ofp_handler,
                                    &is_valid);
      if (res == LAGOPUS_RESULT_OK) {
        if (is_valid == true) {
          *retptr = &(s_ofp_handler->m_channelq);
          res = LAGOPUS_RESULT_OK;
        } else {
          lagopus_msg_error("ofp-handler thread is invalid.\n");
          res = LAGOPUS_RESULT_INVALID_OBJECT;
        }
      } else {
        lagopus_perror(res);
      }
    } else {
      lagopus_msg_error("ofp-handler thread is NULL.\n");
      res = LAGOPUS_RESULT_INVALID_OBJECT;
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return res;
}

lagopus_result_t
ofp_handler_dataq_data_put(uint64_t dpid,
                           struct eventq_data **data,
                           lagopus_chrono_t timeout) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *bridgeq;
  struct ofp_bridge *bridge;

  /* check params */
  if (data != NULL) {
    /* find ofp_bridge */
    ret = ofp_bridgeq_mgr_bridge_lookup(dpid, &bridgeq);
    if (ret == LAGOPUS_RESULT_OK) {
      bridge = ofp_bridgeq_mgr_bridge_get(bridgeq);
      ret = lagopus_bbq_put(&bridge->dataq,
                            data,
                            struct dataq_data *,
                            timeout);
      ofp_bridgeq_mgr_bridgeq_free(bridgeq);
    } else {
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_handler_eventq_data_put(uint64_t dpid,
                            struct eventq_data **data,
                            lagopus_chrono_t timeout) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *bridgeq;
  struct ofp_bridge *bridge;

  /* check params */
  if (data != NULL) {
    /* find ofp_bridge */
    ret = ofp_bridgeq_mgr_bridge_lookup(dpid, &bridgeq);
    if (ret == LAGOPUS_RESULT_OK) {
      bridge = ofp_bridgeq_mgr_bridge_get(bridgeq);
      ret = lagopus_bbq_put(&bridge->eventq,
                            data,
                            struct eventq_data *,
                            timeout);
      ofp_bridgeq_mgr_bridgeq_free(bridgeq);
    } else {
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_handler_event_dataq_data_get(uint64_t dpid,
                                 struct eventq_data **data,
                                 lagopus_chrono_t timeout) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *bridgeq;
  struct ofp_bridge *bridge;

  /* check params */
  if (data != NULL) {
    /* find ofp_bridge */
    ret = ofp_bridgeq_mgr_bridge_lookup(dpid, &bridgeq);
    if (ret == LAGOPUS_RESULT_OK) {
      bridge = ofp_bridgeq_mgr_bridge_get(bridgeq);
      /* get data */
      ret = lagopus_bbq_get(&bridge->event_dataq,
                            data,
                            struct eventq_data *,
                            timeout);
      ofp_bridgeq_mgr_bridgeq_free(bridgeq);
    } else {
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

void
ofp_handler_channelq_size_set(uint16_t val) {
  mbar();
  channelq_size = val;
  lagopus_msg_info("set channelq_size: %"PRIu16".\n", val);
}

void
ofp_handler_channelq_max_batches_set(uint16_t val) {
  mbar();
  channelq_max_batches = val;
  lagopus_msg_info("set channelq_max_batches: %"PRIu16".\n", val);
}

uint16_t
ofp_handler_channelq_size_get(void) {
  return channelq_size;
}

uint16_t
ofp_handler_channelq_max_batches_get(void) {
  return channelq_max_batches;
}

lagopus_result_t
ofp_handler_channelq_stats_get(uint16_t *val) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_valid = false;

  if (val != NULL) {
    if (s_ofp_handler != NULL) {
      res = lagopus_thread_is_valid((const lagopus_thread_t *)&s_ofp_handler,
                                    &is_valid);
      if (res == LAGOPUS_RESULT_OK) {
        if (is_valid == true) {
          res = lagopus_bbq_size(&(s_ofp_handler->m_channelq));
          if (res >= LAGOPUS_RESULT_OK) {
            *val = (uint16_t) res;
            res = LAGOPUS_RESULT_OK;
          }
        } else {
          lagopus_msg_error("ofp-handler thread is invalid.\n");
          res = LAGOPUS_RESULT_INVALID_OBJECT;
        }
      } else {
        lagopus_perror(res);
      }
    } else {
      lagopus_msg_error("ofp-handler thread is NULL.\n");
      res = LAGOPUS_RESULT_INVALID_OBJECT;
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return res;
}

/*
 * private functions
 */
/* initialize thread. it runs only once. */
static void
s_initialize_once(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_qmuxer_poll_t *polls = NULL;

  lagopus_msg_debug(10, "called.\n");
  /* allocate thread */
  s_ofp_handler = (ofp_handler_t)malloc(sizeof(*s_ofp_handler));
  if (s_ofp_handler == NULL) {
    lagopus_exit_fatal("ofp_handler_initialize:allocate ofp_handler");
  }

  /* allocate polls */
  polls = (lagopus_qmuxer_poll_t *)calloc(MAX_POLLS, sizeof(*polls));
  if (polls == NULL) {
    lagopus_exit_fatal("ofp_handler_initialize:allocate poll array");
  }

  /* init lagopus_thread_t */
  res = lagopus_thread_create((lagopus_thread_t *)&s_ofp_handler,
                              s_ofph_thread_main, s_ofph_thread_shutdown,
                              s_ofph_thread_freeup, "ofp_handler", NULL);
  if (res != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("ofp_handler_initialize:lagopus_thread_crate (%s)",
                       lagopus_error_get_string(res));
  }
  /* set thread_free_when_destroy */
  lagopus_thread_free_when_destroy((lagopus_thread_t *)&s_ofp_handler);

  /* create mutex */
  res = lagopus_mutex_create(&(s_ofp_handler->m_status_lock));
  if (res != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("ofp_handler_initialize:lagopus_mutex_create (%s)",
                       lagopus_error_get_string(res));
  }
  /* Create the qmuxer. */
  res = lagopus_qmuxer_create(&(s_ofp_handler->muxer));
  if (res != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("ofp_handler_initialize:lagopus_qmuxer_create (%s)",
                       lagopus_error_get_string(res));
  }

  /* Register queue put function. */
  dp_dataq_put_func_register(ofp_handler_dataq_data_put);
  dp_eventq_put_func_register(ofp_handler_eventq_data_put);

  ofp_bridgeq_mgr_initialize(NULL);

  /* init other */
  s_set_status(OFPH_SHUTDOWNED);
  s_ofp_handler->m_polls = polls;
  s_ofp_handler->m_n_polls = 0;
  lagopus_msg_debug(1, "created. (retptr: %p)\n", s_ofp_handler);

  return;
}

/* if s_ofp_handler is valid thread, return true */
static inline bool
s_validate_ofp_handler(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_valid = false;

  if (s_ofp_handler == NULL) {
    return false;
  }
  res = lagopus_thread_is_valid((const lagopus_thread_t *)&s_ofp_handler,
                                &is_valid);
  if (res == LAGOPUS_RESULT_OK && is_valid == true) {
    return true;
  } else {
    return false;
  }
}

/* if ptr is chanceled thread, return true */
static inline bool
s_ofp_handler_is_canceled(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_canceled = false;

  if (s_ofp_handler == NULL) {
    return false;
  }
  res = lagopus_thread_is_canceled((const lagopus_thread_t *)&s_ofp_handler,
                                   &is_canceled);
  if (res == LAGOPUS_RESULT_OK && is_canceled == true) {
    return true;
  } else {
    return false;
  }
}

bool
ofp_handler_validate_bbq(lagopus_bbq_t *bbq) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_operational = false;
  if (bbq != NULL) {
    res = lagopus_bbq_is_operational(bbq, &is_operational);
    if (res != LAGOPUS_RESULT_OK) {
      is_operational = false;
    }
  }
  return is_operational;
}

/* insert eventq_data to event_dataq */
lagopus_result_t
ofp_handler_event_dataq_put(uint64_t dpid,
                            struct eventq_data *dataptr) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridgeq *bridgeq;
  struct ofp_bridge *bridge;

  /* check params */
  if (s_validate_ofp_handler() != false) {
    /* find ofp_bridge */
    res = ofp_bridgeq_mgr_bridge_lookup(dpid, &bridgeq);
    if (res == LAGOPUS_RESULT_OK) {
      bridge = ofp_bridgeq_mgr_bridge_get(bridgeq);
    } else {
      lagopus_perror(res);
      goto done;
    }
#ifdef OFPH_POLL_WRITING
    {
      /* insert edq_buffer */
      struct edq_buffer_entry *qe;
      qe = (struct edq_buffer_entry *)malloc(sizeof(struct edq_buffer_entry));
      if (qe != NULL) {
        qe->qdata = dataptr;
        STAILQ_INSERT_TAIL(&(bridge->edq_buffer), qe, qentry);
        res = LAGOPUS_RESULT_OK;
      } else {
        res = LAGOPUS_RESULT_NO_MEMORY;
      }
    }
#else  /* OFPH_POLL_WRITING */
    /* insert bbq. */
    res = lagopus_bbq_put(&(bridge->event_dataq),
                          &dataptr, struct eventq_data *, PUT_TIMEOUT);
    if (res != LAGOPUS_RESULT_OK) {
      lagopus_perror(res);
    }
    ofp_bridgeq_mgr_bridgeq_free(bridgeq);
#endif  /* OFPH_POLL_WRITING */
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }
done:
  return res;
}

static bool
s_ofp_version_check(struct channel *channel,
                    struct ofp_header *header) {
  return (header->type == OFPT_HELLO || header->type == OFPT_ERROR ||
          ofp_header_version_check(channel, header) == true);
}

/* process channel */
static inline lagopus_result_t
s_process_channelq_entry(struct channelq_data *entry) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = NULL;
  struct pbuf *pbuf = NULL;
  struct pbuf *req_pbuf = NULL;
  struct ofp_header header;
  struct ofp_error error = {OFPET_BAD_REQUEST, OFPBRC_BAD_VERSION, {NULL}};

  lagopus_msg_debug(10, "get item. %p\n", entry);
  if (entry == NULL
      || entry->channel == NULL
      || entry->pbuf == NULL) {
    lagopus_msg_warning("received channelq_data is NULL\n");
    res = LAGOPUS_RESULT_INVALID_ARGS;
    goto done;
  }
  channel = entry->channel;
  pbuf = entry->pbuf;

  /* set request in ofp_error. */
  req_pbuf = channel_pbuf_list_get(channel,
                                   OFP_ERROR_MAX_SIZE);
  if (req_pbuf == NULL) {
    lagopus_msg_warning("Can't allocate pbuf.\n");
    res = LAGOPUS_RESULT_NO_MEMORY;
    goto done;
  }
  if (pbuf_copy_with_length(req_pbuf, pbuf, OFP_ERROR_MAX_SIZE) !=
      LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("Can't copy request pbuf.\n");
    ofp_error_set(&error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
    res = LAGOPUS_RESULT_OFP_ERROR;
    goto done;
  }
  error.req = req_pbuf;

  if (ofp_header_decode_sneak(pbuf, &header) != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("cannot decode header\n");
    ofp_error_set(&error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
    res = LAGOPUS_RESULT_OFP_ERROR;
    goto done;
  }

  /* check packet length. */
  if (pbuf_plen_equal_check(pbuf, (size_t) header.length) !=
      LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("bad header length.\n");
    ofp_error_set(&error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
    res = LAGOPUS_RESULT_OFP_ERROR;
    goto done;
  }

  /* State loggin. */
  lagopus_msg_debug(1, "RECV: %s (xid=%u)\n",
                    ofp_type_str(header.type), header.xid);

  /* check ofp version in header. */
  if (s_ofp_version_check(channel, &header) == false) {
    res = LAGOPUS_RESULT_OFP_ERROR;
    ofp_error_set(&error, OFPET_BAD_REQUEST, OFPBRC_BAD_VERSION);
    goto done;
  }

  /* check role. */
  if (ofp_role_check(channel, &header) == false) {
    lagopus_msg_warning("role is slave (%s).\n",
                        ofp_type_str(header.type));
    res = LAGOPUS_RESULT_OFP_ERROR;
    ofp_error_set(&error, OFPET_BAD_REQUEST, OFPBRC_IS_SLAVE);
    goto done;
  }

  switch (header.type) {
    case OFPT_HELLO:
      res = ofp_hello_handle(channel, pbuf, &error);
      break;
    case OFPT_FEATURES_REQUEST:
      res = ofp_features_request_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_SET_CONFIG:
      res = ofp_set_config_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_ECHO_REPLY:
      res = ofp_echo_reply_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_FLOW_MOD:
      res = ofp_flow_mod_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_ERROR:
      res = ofp_error_msg_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_ECHO_REQUEST:
      res = ofp_echo_request_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_EXPERIMENTER:
      /* unsupport. */
      /* res = ofp_experimenter_request_handle(channel, pbuf, &header) */
      res = ofp_bad_experimenter_handle(&error);
      break;
    case OFPT_FEATURES_REPLY:
      res = ofp_unsupported_handle(&error);
      break;
    case OFPT_GET_CONFIG_REQUEST:
      res = ofp_get_config_request_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_GET_CONFIG_REPLY:
      res = ofp_unsupported_handle(&error);
      break;
    case OFPT_PACKET_IN:
      res = ofp_unsupported_handle(&error);
      break;
    case OFPT_FLOW_REMOVED:
      res = ofp_unsupported_handle(&error);
      break;
    case OFPT_PORT_STATUS:
      res = ofp_unsupported_handle(&error);
      break;
    case OFPT_PACKET_OUT:
      res = ofp_packet_out_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_GROUP_MOD:
      res = ofp_group_mod_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_PORT_MOD:
      res = ofp_port_mod_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_TABLE_MOD:
      res = ofp_table_mod_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_MULTIPART_REQUEST:
      res = ofp_multipart_request_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_MULTIPART_REPLY:
      res = ofp_unsupported_handle(&error);
      break;
    case OFPT_BARRIER_REQUEST:
      res = ofp_barrier_request_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_BARRIER_REPLY:
      res = ofp_unsupported_handle(&error);
      break;
    case OFPT_QUEUE_GET_CONFIG_REQUEST:
      res = ofp_queue_get_config_request_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_QUEUE_GET_CONFIG_REPLY:
      res = ofp_unsupported_handle(&error);
      break;
    case OFPT_ROLE_REQUEST:
      res = ofp_role_request_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_ROLE_REPLY:
      res = ofp_unsupported_handle(&error);
      break;
    case OFPT_GET_ASYNC_REQUEST:
      res = ofp_get_async_request_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_GET_ASYNC_REPLY:
      res = ofp_unsupported_handle(&error);
      break;
    case OFPT_SET_ASYNC:
      res = ofp_set_async_handle(channel, pbuf, &header, &error);
      break;
    case OFPT_METER_MOD:
      res = ofp_meter_mod_handle(channel, pbuf, &header, &error);
      break;
    default:
      res = ofp_unsupported_handle(&error);
      break;
  }

done:
  if (res == LAGOPUS_RESULT_OFP_ERROR) {
    res = ofp_error_msg_send(channel, &header, &error);
  }

  /* free request data in error. */
  if (req_pbuf != NULL) {
    channel_pbuf_list_unget(channel, req_pbuf);
  }

  return res;
}

/* process eventq. */
static inline lagopus_result_t
s_process_eventq_entry(struct ofp_bridge *ofp_bridge,
                       struct eventq_data *entry) {
  lagopus_result_t res = LAGOPUS_RESULT_OK;

  lagopus_msg_debug(10, "get item. %p\n", entry);
  if (ofp_bridge == NULL || entry == NULL) {
    lagopus_msg_warning("s_process_eventq_entry arg is NULL\n");
    res = LAGOPUS_RESULT_INVALID_ARGS;
    goto done;
  }

  switch (entry->type) {
    case LAGOPUS_EVENTQ_BARRIER_REPLY:
      res = ofp_barrier_reply_handle(&entry->barrier,
                                     ofp_bridge->dpid);
      break;
    case LAGOPUS_EVENTQ_FLOW_REMOVED:
      res = ofp_flow_removed_handle(&entry->flow_removed,
                                    ofp_bridge->dpid);
      break;
    case LAGOPUS_EVENTQ_PORT_STATUS:
      res = ofp_port_status_handle(&entry->port_status,
                                   ofp_bridge->dpid);
      break;
    case LAGOPUS_EVENTQ_ERROR:
      res = ofp_error_msg_from_queue_handle(&entry->error,
                                            ofp_bridge->dpid);
      break;
    default:
      lagopus_msg_warning("Not found event type (%d).\n",
                          entry->type);
      break;
  }

done:
  return res;
}

/* process dataq. */
static inline lagopus_result_t
s_process_dataq_entry(struct ofp_bridge *ofp_bridge,
                      struct eventq_data *entry) {
  lagopus_result_t res = LAGOPUS_RESULT_OK;

  lagopus_msg_debug(10, "get item. %p\n", entry);
  if (ofp_bridge == NULL || entry == NULL) {
    lagopus_msg_warning("s_process_dataq_entry arg is NULL\n");
    res = LAGOPUS_RESULT_INVALID_ARGS;
    goto done;
  }

  switch (entry->type) {
    case LAGOPUS_EVENTQ_PACKET_IN:
      res = ofp_packet_in_handle(&entry->packet_in,
                                 ofp_bridge->dpid);
      break;
    default:
      lagopus_msg_warning("Not found event type (%d).\n",
                          entry->type);
      break;
  }

done:
  return res;
}

/* read each queues. */
static lagopus_result_t
s_dequeue(struct ofp_bridgeq *brqs) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridge *bridge;
  lagopus_qmuxer_poll_t qpoll;

  if (brqs != NULL) {
    bridge = ofp_bridgeq_mgr_bridge_get(brqs);
    qpoll = ofp_bridgeq_mgr_eventq_poll_get(brqs);
    res = s_eventq_dequeue(bridge, qpoll);
    if (res != LAGOPUS_RESULT_OK) {
      lagopus_perror(res);
      goto done;
    }
    qpoll = ofp_bridgeq_mgr_dataq_poll_get(brqs);
    res = s_dataq_dequeue(bridge, qpoll);
    if (res != LAGOPUS_RESULT_OK) {
      lagopus_perror(res);
      goto done;
    }
#ifdef OFPH_POLL_WRITING
    poll = ofp_bridgeq_mgr_event_dataq_poll_get(brqs);
    res = s_event_dataq_enqueue(bridge, qpoll);
    if (res != LAGOPUS_RESULT_OK) {
      lagopus_perror(res);
      goto done;
    }
#endif  /* OFPH_POLL_WRITING */
    res = LAGOPUS_RESULT_OK;
  } else {
    lagopus_msg_error("ofp_bridgeq is NULL\n");
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return res;
}

/* thread_main_proc. */
static lagopus_result_t
s_ofph_thread_main(const lagopus_thread_t *selfptr,
                   void *arg) {
  uint64_t i;
  ofp_handler_t thd;
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_bbq_t bbq = NULL;
  int n_need_watch = 0;
  int n_valid_polls = 0;
  global_state_t gstate;
  shutdown_grace_level_t level;
  uint64_t n_bridgeqs = 0;
  /* number of default polls. */
  uint64_t n_polls = 0;
  struct ofp_bridgeq *bridgeqs[MAX_BRIDGES];
  (void)arg;

  if (selfptr == NULL || *selfptr == NULL) {
    res = LAGOPUS_RESULT_INVALID_ARGS;
    goto done;
  }
  thd = (ofp_handler_t)*selfptr;

  /* wait for GLOBAL_STATE_STARTED */
  lagopus_msg_debug(1, "waiting for the world changes to "
                    "GLOBAL_STATE_STARTED ...\n");
  while ((res = global_state_wait_for(GLOBAL_STATE_STARTED, &gstate, &level,
                                      1000LL * 1000LL * 100LL)) ==
         LAGOPUS_RESULT_TIMEDOUT) {
    lagopus_msg_debug(1, "still waiting for the world changes to "
                      "GLOBAL_STATE_STARTED ...\n");
  }
  if (res != LAGOPUS_RESULT_OK) {
    lagopus_perror(res);
    goto done;
  }
  lagopus_msg_debug(10, "now GLOBAL_STATE_STARTED, start main-loop.\n");

  s_is_started = true;
  /* main loop. */
  while (s_is_running == true) {
    n_need_watch = 0;
    n_valid_polls = 0;

    if (s_get_status() == OFPH_SHUTDOWN_RIGHT_NOW) {
      goto done;
    }

    /* get bridgeq array. */
    res = ofp_bridgeq_mgr_bridgeqs_to_array(bridgeqs, &n_bridgeqs,
                                            MAX_BRIDGES);
    if (res != LAGOPUS_RESULT_OK) {
      lagopus_perror(res);
      goto done;
    }

    /* get polls.*/
    n_polls = thd->m_n_polls;
    res = ofp_bridgeq_mgr_polls_get(thd->m_polls,
                                    bridgeqs,
                                    (uint64_t *) &thd->m_n_polls,
                                    n_bridgeqs);
    if (res != LAGOPUS_RESULT_OK) {
      lagopus_perror(res);
      goto free_bridgeqs;
    }

    for (i = 0; i < thd->m_n_polls; i++) {
      /* Check if the poll has a valid queue. */
      bbq = NULL;
      res = lagopus_qmuxer_poll_get_queue(&(thd->m_polls[i]), &bbq);
      if (res != LAGOPUS_RESULT_OK) {
        lagopus_perror(res);
        s_set_status(OFPH_SHUTDOWN_RIGHT_NOW);
        goto free_bridgeqs;
      }
      if (bbq != NULL && ofp_handler_validate_bbq(&bbq) == true) {
        n_valid_polls++;
      }
      /* Reset the poll status. */
      res = lagopus_qmuxer_poll_reset(&(thd->m_polls[i]));
      if (res != LAGOPUS_RESULT_OK) {
        lagopus_perror(res);
        s_set_status(OFPH_SHUTDOWN_RIGHT_NOW);
        goto free_bridgeqs;
      }
      n_need_watch++;
    }
    if (n_valid_polls == 0) {
      lagopus_msg_error("there are no valid queues.\n");
      s_set_status(OFPH_SHUTDOWN_RIGHT_NOW);
      res = LAGOPUS_RESULT_INVALID_OBJECT;
      goto free_bridgeqs;
    }
    /* Wait for an event. */
    res = lagopus_qmuxer_poll(&(thd->muxer),
                              (lagopus_qmuxer_poll_t * const)(thd->m_polls),
                              (size_t)n_need_watch, MUXER_TIMEOUT);
    if (s_get_status() == OFPH_SHUTDOWN_RIGHT_NOW) {
      res = LAGOPUS_RESULT_NOT_OPERATIONAL;
      goto free_bridgeqs;
    }
    if (res > 0) {
      /* read channelq */
      res = s_channelq_dequeue(&(thd->m_channelq), thd->m_polls[0]);
      if (res != LAGOPUS_RESULT_OK) {
        lagopus_perror(res);
        /* Not exit. */
        res = LAGOPUS_RESULT_OK;
      }
      /* read eventq, dataq, event_dataq */
      if (thd->m_n_polls > 1) {
        for (i = 0; i < n_bridgeqs; i++) {
          res = s_dequeue(bridgeqs[i]);
          if (res != LAGOPUS_RESULT_OK) {
            lagopus_perror(res);
            /* Not exit. */
            res = LAGOPUS_RESULT_OK;
          }
        }
      }
    } else if (res == LAGOPUS_RESULT_TIMEDOUT) {
      lagopus_msg_debug(100, "Timedout. continue.\n");
      /* ignore timeout. */
      res = LAGOPUS_RESULT_OK;
    } else {
      lagopus_perror(res);
      s_set_status(OFPH_SHUTDOWN_RIGHT_NOW);
    }

  free_bridgeqs:
    /* free bridgeqs. */
    ofp_bridgeq_mgr_poll_reset(&thd->m_polls[n_polls],
                               MAX_POLLS - n_polls);
    thd->m_n_polls = n_polls;
    ofp_bridgeq_mgr_bridgeqs_free(bridgeqs, n_bridgeqs);

    if (res != LAGOPUS_RESULT_OK) {
      break;
    }
  }

done:
  lagopus_msg_debug(10, "ofp_handler breaks main-loop.\n");
  return res;
}

/* thread_shutdown_proc */
static void
s_ofph_thread_shutdown(const lagopus_thread_t *selfptr,
                       bool is_canceled, void *arg) {
  (void)arg;
  lagopus_msg_debug(10, "called. %s.\n",
                    (is_canceled == false) ? "finished" : "canceled");
  if (s_validate_ofp_handler() == true) {
    ofp_handler_t thd = (ofp_handler_t)*selfptr;
    /* if canceled, unlock all mutexes */
    if (is_canceled == true) {
      lagopus_bbq_cancel_janitor(&(thd->m_channelq));
      lagopus_qmuxer_cancel_janitor(&(thd->muxer));
    }
    /* shutdown all queues, bridges, hashmaps */
    lagopus_bbq_shutdown(&(thd->m_channelq), true);
    s_destroy_for_recreate();
    s_set_status(OFPH_SHUTDOWNED);
  }
  if (is_canceled == true && s_is_started == false) {
    global_state_cancel_janitor();
  }
  lagopus_msg_debug(10, "ok.\n");
}

/* thread_destroy_proc */
static void
s_ofph_thread_freeup(const lagopus_thread_t *selfptr,
                     void *arg) {
  (void)arg;
  lagopus_msg_debug(10, "called. %p\n", selfptr);
  if (s_validate_ofp_handler() == true) {
    ofp_handler_t thd = (ofp_handler_t)*selfptr;

    free(thd->m_polls);
    thd->m_polls = NULL;
    lagopus_qmuxer_destroy(&(thd->muxer));
    thd->muxer = NULL;
    lagopus_mutex_destroy(&(thd->m_status_lock));
    thd->m_status_lock = NULL;
  }

  ofp_bridgeq_mgr_destroy();
  lagopus_msg_debug(10, "ok.\n");
}

/* channel_free() wrapper for bbq */
static void
s_channel_freeup_proc(void **val) {
  if (val != NULL) {
    channel_free((struct channel *)val);
  }
}

/* read channelq */
static inline lagopus_result_t
s_channelq_dequeue(channelq_t *q_ptr,
                   lagopus_qmuxer_poll_t qpoll) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct channelq_data **gets = NULL;
  lagopus_result_t q_size = lagopus_bbq_size(q_ptr);
  uint16_t max_batches = channelq_max_batches;
  size_t get_num;
  size_t i;

  lagopus_msg_debug(10,
                    "called. q_size: %lu, max_batches: %"PRIu16"\n",
                    q_size, max_batches);
  if (q_size > 0) {
    int cstate;

    gets = (struct channelq_data **)
        malloc(sizeof(struct channelq_data *) * max_batches);
    if (gets != NULL) {
      res = lagopus_bbq_get_n(q_ptr, gets,
                              (size_t) channelq_max_batches, 0LL,
                              struct channelq_data *, 0LL, &get_num);
      if (res < LAGOPUS_RESULT_OK) {
        lagopus_perror(res);
        res = lagopus_qmuxer_poll_set_queue(&qpoll, NULL);
        if (res != LAGOPUS_RESULT_OK) {
          lagopus_perror(res);
          goto done;
        }
      } else {
        res = LAGOPUS_RESULT_OK;
      }

      for (i = 0; i < get_num; i++) {
        lagopus_mutex_enter_critical(&(s_ofp_handler->m_status_lock), &cstate);
        {
          s_process_channelq_entry(gets[i]);
        channelq_data_destroy(gets[i]);
        }
        lagopus_mutex_leave_critical(&(s_ofp_handler->m_status_lock), cstate);
      }
    } else {
      res = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else if (q_size == 0) {
    res = LAGOPUS_RESULT_OK;
  }

done:
  free(gets);

  return res;
}

/* read eventq */
static inline lagopus_result_t
s_eventq_dequeue(struct ofp_bridge *ofp_bridge,
                 lagopus_qmuxer_poll_t qpoll) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data **gets = NULL;
  lagopus_result_t q_size = lagopus_bbq_size(&(ofp_bridge->eventq));
  uint16_t max_batches;
  size_t get_num;
  size_t i;

  res = ofp_bridge_eventq_max_batches_get(ofp_bridge,
                                          &max_batches);
  if (res != LAGOPUS_RESULT_OK) {
    lagopus_perror(res);
    goto done;
  }

  lagopus_msg_debug(10,
                    "called. dpid: %lu, q_size: %lu, max_batches: %"PRIu16"\n",
                    ofp_bridge->dpid, q_size, max_batches);
  if (q_size > 0) {
    int cstate;

    gets = (struct eventq_data **)
        malloc(sizeof(struct eventq_data *) * max_batches);
    if (gets != NULL) {
      res = lagopus_bbq_get_n(&(ofp_bridge->eventq), gets,
                              (size_t) max_batches, 0LL,
                              struct eventq_data *, 0LL, &get_num);
      if (res < LAGOPUS_RESULT_OK) {
        lagopus_perror(res);
        res = lagopus_qmuxer_poll_set_queue(&qpoll, NULL);
        if (res != LAGOPUS_RESULT_OK) {
          lagopus_perror(res);
          goto done;
        }
      } else {
        res = LAGOPUS_RESULT_OK;
      }

      for (i = 0; i < get_num; i++) {
        lagopus_mutex_enter_critical(&(s_ofp_handler->m_status_lock), &cstate);
        {
          res = s_process_eventq_entry(ofp_bridge, gets[i]);
          if (gets[i] != NULL && gets[i]->free != NULL) {
            gets[i]->free(gets[i]);
          } else {
            free(gets[i]);
          }
        }
        lagopus_mutex_leave_critical(&(s_ofp_handler->m_status_lock), cstate);
      }
    } else {
      res = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else if (q_size == 0) {
    res = LAGOPUS_RESULT_OK;
  }
done:
  free(gets);

  return res;
}

/* read dataq */
static inline lagopus_result_t
s_dataq_dequeue(struct ofp_bridge *ofp_bridge,
                lagopus_qmuxer_poll_t qpoll) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data **gets = NULL;
  lagopus_result_t q_size = lagopus_bbq_size(&(ofp_bridge->dataq));
  uint16_t max_batches;
  size_t get_num;
  size_t i;

  res = ofp_bridge_dataq_max_batches_get(ofp_bridge,
                                          &max_batches);
  if (res != LAGOPUS_RESULT_OK) {
    lagopus_perror(res);
    goto done;
  }

  lagopus_msg_debug(10,
                    "called. dpid: %lu, q_size: %lu, max_batches: %"PRIu16"\n",
                    ofp_bridge->dpid, q_size, max_batches);
  if (q_size > 0) {
    int cstate;

    gets = (struct eventq_data **)
        malloc(sizeof(struct eventq_data *) * max_batches);
    if (gets != NULL) {
      res = lagopus_bbq_get_n(&(ofp_bridge->dataq), gets,
                              (size_t) max_batches, 0LL,
                              struct eventq_data *, 0LL, &get_num);
      if (res < LAGOPUS_RESULT_OK) {
        lagopus_perror(res);
        res = lagopus_qmuxer_poll_set_queue(&qpoll, NULL);
        if (res != LAGOPUS_RESULT_OK) {
          lagopus_perror(res);
          goto done;
        }
      } else {
        res = LAGOPUS_RESULT_OK;
      }

      for (i = 0; i < get_num; i++) {
        lagopus_mutex_enter_critical(&(s_ofp_handler->m_status_lock), &cstate);
        {
          res = s_process_dataq_entry(ofp_bridge, gets[i]);
          if (gets[i] != NULL && gets[i]->free != NULL) {
            gets[i]->free(gets[i]);
          } else {
            free(gets[i]);
          }
        }
        lagopus_mutex_leave_critical(&(s_ofp_handler->m_status_lock), cstate);
      }
    } else {
      res = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else if (q_size == 0) {
    res = LAGOPUS_RESULT_OK;
  }
done:
  free(gets);

  return res;
}

/* write event_dataq */
#ifdef OFPH_POLL_WRITING
static inline lagopus_result_t
s_event_dataq_enqueue(struct ofp_bridge *ofp_bridge,
                      lagopus_qmuxer_poll_t qpoll) {
  int i;
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t q_size
    = lagopus_bbq_remaining_capacity(&(ofp_bridge->event_dataq));

  lagopus_msg_debug(10, "called. dpid: %lu, q_size: %lu\n",
                    ofp_bridge->dpid, q_size);
  if (q_size > 0) {
    int cstate;
    MUXER_FAIRNESS(q_size);
    for (i = 0; i < q_size; i++) {
      lagopus_mutex_enter_critical(&(s_ofp_handler->m_status_lock), &cstate);
      {
        struct edq_buffer_entry *qe
          = STAILQ_FIRST(&(ofp_bridge->edq_buffer));
        if (qe != NULL) {
          lagopus_msg_debug(1, "put item. %p\n", qe->qdata);
          res = lagopus_bbq_put(&(ofp_bridge->event_dataq), &(qe->qdata),
                                struct eventq_data *, PUT_TIMEOUT);
          if (res != LAGOPUS_RESULT_OK) {
            lagopus_perror(res);
            res = lagopus_qmuxer_poll_set_queue(&qpoll, NULL);
            if (res != LAGOPUS_RESULT_OK) {
              lagopus_perror(res);
              goto done;
            }
          }
          STAILQ_REMOVE_HEAD(&(ofp_bridge->edq_buffer), qentry);
          free(qe);
        }
      }
      lagopus_mutex_leave_critical(&(s_ofp_handler->m_status_lock), cstate);
    }
    res = LAGOPUS_RESULT_OK;
  } else if (q_size == 0) {
    res = LAGOPUS_RESULT_OK;
  }
done:
  return res;
}
#endif  /* OFPH_POLL_WRITING */

static inline void
s_destroy_for_recreate(void) {
  if (s_validate_ofp_handler() == true) {
    lagopus_bbq_destroy(&(s_ofp_handler->m_channelq), true);
    s_ofp_handler->m_channelq = NULL;
    lagopus_qmuxer_poll_destroy(&(s_ofp_handler->m_polls[0]));
    s_ofp_handler->m_polls[0] = NULL;
    s_ofp_handler->m_n_polls = 0;
  }
  /* clear bridgeq hashmap */
  (void) ofp_bridgeq_mgr_clear();
}

static inline lagopus_result_t
s_recreate(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;

  if (s_validate_ofp_handler() == true) {
    /* Create channelq */
    res = lagopus_bbq_create(&(s_ofp_handler->m_channelq), struct channel *,
                             channelq_size, s_channel_freeup_proc);
    if (res != LAGOPUS_RESULT_OK) {
      lagopus_perror(res);
      goto done;
    }
    /* Create poll objects for channel queue. */
    res = lagopus_qmuxer_poll_create(&(s_ofp_handler->m_polls[0]),
                                     s_ofp_handler->m_channelq,
                                     LAGOPUS_QMUXER_POLL_READABLE);
    if (res != LAGOPUS_RESULT_OK) {
      lagopus_perror(res);
      goto done;
    }
    s_ofp_handler->m_n_polls++;
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }
done:
  return res;
}

static inline enum ofp_handler_running_status
s_get_status(void) {
  bool ret = false;
  lagopus_mutex_lock(&(s_ofp_handler->m_status_lock));
  {
    ret = s_ofp_handler->m_status;
  }
  lagopus_mutex_unlock(&(s_ofp_handler->m_status_lock));
  return ret;
}

static inline void
s_set_status(enum ofp_handler_running_status set) {
  lagopus_mutex_lock(&(s_ofp_handler->m_status_lock));
  {
    /* TODO: check, current_status < set */
    s_ofp_handler->m_status = set;
  }
  lagopus_mutex_unlock(&(s_ofp_handler->m_status_lock));
}

#if 0
/* constructor, destructor */
#define OFP_HANDLER_IDX LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE + 10

static void s_ofp_handler_ctors(void) __attr_constructor__(OFP_HANDLER_IDX);
static void s_ofp_handler_dtors(void) __attr_destructor__(OFP_HANDLER_IDX);

static void s_ofp_handler_ctors(void) {
  lagopus_msg_debug(1, "called.\n");
  /* TODO : init. */
}

static void s_ofp_handler_dtors(void) {
  lagopus_msg_debug(1, "called.\n");
  /* TODO : final. */
}
#endif

