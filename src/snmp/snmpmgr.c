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

#include <stdint.h>
#include <stdbool.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/library/snmp_logging.h>  // for the snmp log handler

#include "ifTable.h"
#include "ifNumber.h"
#include "dot1dBaseBridgeAddress.h"
#include "dot1dBaseNumPorts.h"
#include "dot1dBasePortTable.h"
#include "dot1dBaseType.h"

#include "coldStart.h"
#include "warmStart.h"
#include "linkUp.h"
#include "linkDown.h"

#include "lagopus_apis.h"
#include "lagopus_gstate.h"
#include "lagopus/snmpmgr.h"

#include "dataplane_apis.h"
#include "ifTable_type.h"
#include "dataplane_interface.h"

#define SNMP_TYPE "lagopus"

#define __STR__(x)      #x

#define DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC 10
#define DEFAULT_SNMPMGR_AGENTX_SOCKET "tcp:localhost:705"
#define DEFAULT_SNMPMGR_LOOP_INTERVAL_NSEC 10000000
#define DEFAULT_SNMPMGR_RIGHTNOW_SHUTDOWN_NSEC 1000
#define DEFAULT_SNMPMGR_GRACEFUL_SHUTDOWN_NSEC 3000000000

/* NOTE: state transition of SNMP module
 *                         | none  <-> runnable <-> running
 * initialize              |   x    ->     o
 * start                   |               x     ->    o
 * shutdown                |               o    <-     x
 * stop (already shutdown) |                                (no state change)
 * stop (forcibly)         |   o    <------------      x
 * finalize                |   o    <-     x
 *
 * XXX: Restarting SNMP module (call start, shutdown, then start) does not work
 */

enum snmpmgr_state {
  SNMPMGR_RUNNABLE,
  SNMPMGR_RUNNING,
  SNMPMGR_NONE,
};

static pthread_once_t initialized = PTHREAD_ONCE_INIT;
static volatile bool keep_running = false;
static bool is_thread = false;
static bool is_thread_dummy = false;
static lagopus_thread_t **thdptr_dummy = NULL;

static lagopus_result_t initialize_ret = LAGOPUS_RESULT_NOT_STARTED;
static enum snmpmgr_state state = SNMPMGR_NONE;
static lagopus_thread_t snmpmgr = NULL;
static lagopus_mutex_t snmp_lock = NULL;
static lagopus_mutex_t snmp_state_lock = NULL;
static lagopus_hashmap_t iftable_entry_hash = NULL;

static SNMPCallback snmp_log_callback_wrapper;

static int
snmp_log_callback_wrapper(int major,
                          int minor,
                          void *server_arg,
                          void *client_arg) {
  if (server_arg != NULL) {
    struct snmp_log_message *slm = (struct snmp_log_message *)server_arg;
    int lagopus_log_level = 0;
    (void)major;
    (void)minor;
    (void)client_arg;
    switch (slm->priority) {
      case LOG_EMERG:  /* fall through */
      case LOG_ALERT:  /* fall through */
      case LOG_CRIT:
        lagopus_log_level = LAGOPUS_LOG_LEVEL_FATAL;
        break;
      case LOG_ERR:
        lagopus_log_level = LAGOPUS_LOG_LEVEL_ERROR;
        break;
      case LOG_WARNING:
        lagopus_log_level = LAGOPUS_LOG_LEVEL_WARNING;
        break;
      case LOG_NOTICE:
        lagopus_log_level = LAGOPUS_LOG_LEVEL_NOTICE;
        break;
      case LOG_INFO:
        lagopus_log_level = LAGOPUS_LOG_LEVEL_INFO;
        break;
      case LOG_DEBUG:
        lagopus_log_level = LAGOPUS_LOG_LEVEL_DEBUG;
        break;
      default:
        lagopus_log_level = LAGOPUS_LOG_LEVEL_UNKNOWN;
        break;
    }

    lagopus_log_emit(lagopus_log_level, -1,
                     "(inside net-snmp)", 0,
                     "", "%s", slm->msg);
  }
  return 0;
}

/* for hash table */
static void
delete_entry(void *p) {
  if (p != NULL) {
    struct ifTable_entry *e = (struct ifTable_entry *)p;
    free(e);
  }
}

static lagopus_result_t
check_ping_interval(const char *ping_interval, uint16_t *val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  if (ping_interval != NULL && val != NULL) {
    if ((ret = lagopus_str_parse_uint16(ping_interval, val))
        == LAGOPUS_RESULT_OK) {
      if (*val > 0) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = LAGOPUS_RESULT_OUT_OF_RANGE;
      }
    }
  }
  return ret;
}

#ifdef THE_CONFSYS_ERA

CALLBACK(show_master_agent_socket) {
  char *master_agentx_socket = NULL;
  (void) argc;
  (void) argv;
  if (confsys != NULL) {
    if ((master_agentx_socket = config_get("snmp master-agentx-socket WORD"))
        != NULL) {
      show(confsys, "master is %s\n", master_agentx_socket);
      return CONFIG_SUCCESS;
    } else {
      return CONFIG_DOES_NOT_EXIST;
    }
  } else {
    return CONFIG_FAILURE;
  }
}

CALLBACK(show_ping_interval) {
  char *ping_interval_string = NULL;
  (void) argc;
  (void) argv;

  /* FOR DEBUG */
  /* fprintf(stderr, */
  /*         "argc is %d, argv[0] is %s, argv[1] is %s, argv[2] is %s, argv[3] is %s\n", */
  /*         argc, argv[0], argv[1], argv[2]); */

  if (confsys != NULL) {
    if ((ping_interval_string = config_get("snmp ping-interval-second 1-255"))
        != NULL) {
      show(confsys, "ping interval is %s second\n", ping_interval_string);
      return CONFIG_SUCCESS;
    } else {
      return CONFIG_DOES_NOT_EXIST;
    }
  } else {
    return CONFIG_FAILURE;
  }
}

CALLBACK(set_master_agentx_socket) {
  char *master_agentx_socket;
  (void) confsys;
  (void) argc;
  (void) argv;
  (void) master_agentx_socket;
#if 0
  if (argc == 3 && argv != NULL && argv[2] != NULL) {
    master_agentx_socket = argv[2];
    if (netsnmp_ds_set_string(NETSNMP_DS_APPLICATION_ID,
                              NETSNMP_DS_AGENT_X_SOCKET,
                              master_agentx_socket)
        == SNMPERR_SUCCESS) {
      return CONFIG_SUCCESS;
    }
  }
  /* require restart netsnmp, but not implemented yet */
  return CONFIG_FAILURE;
#else
  return CONFIG_SUCCESS;
#endif
}

CALLBACK(set_ping_interval_second) {
  lagopus_result_t ret;
  uint16_t val;
  char *ping_interval_string = NULL;
  (void) confsys;
  (void) argc;
  if (argc == 3 && argv != NULL && argv[2] != NULL) {
    ping_interval_string = (char *)argv[2];
    if ((ret = check_ping_interval(ping_interval_string, &val))
        == LAGOPUS_RESULT_OK) {
#if 0
      /* require restart netsnmp, but not implemented yet */
      if (netsnmp_ds_set_int(NETSNMP_DS_APPLICATION_ID,
                             NETSNMP_DS_AGENT_AGENTX_PING_INTERVAL,
                             (int)val)
          == SNMPERR_SUCCESS) {
        return CONFIG_SUCCESS;
      }
#else
      return CONFIG_SUCCESS;
#endif
    } else {
      show(confsys, "invalid ping interval: %s", lagopus_error_get_string(ret));
    }
  }
  return CONFIG_FAILURE;
}

#endif /* THE_CONFSYS_ERA */

static lagopus_result_t
internal_snmpmgr_poll(lagopus_chrono_t nsec) {
  int numfds = 0;
  fd_set fdset;
  struct timespec timeout = { 0, 0 };
  struct timeval timeout_dummy;
  int count = 0;
  int block = true;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  NSEC_TO_TS(nsec, timeout);

  FD_ZERO(&fdset);
  if (snmp_select_info(&numfds, &fdset, &timeout_dummy, &block) > 0) {
    if ((count = pselect(numfds, &fdset, NULL, NULL, &timeout, NULL)) > 0) {
      snmp_read(&fdset);
    } else {
      if (count == 0) {
        snmp_timeout();
      } else {
        perror("pselect");
        return LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    }

    /* run callbacks includes that checking connetion to AgetnX master agent */
    run_alarms();

    netsnmp_check_outstanding_agent_requests();

    if (count == 0) {
      ret = LAGOPUS_RESULT_TIMEDOUT;
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
  } else {  /* snmp_select_info */
    ret = LAGOPUS_RESULT_SNMP_API_ERROR;
  }
  return ret;
}

lagopus_result_t
snmpmgr_poll(lagopus_chrono_t nsec) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void)lagopus_mutex_lock(&snmp_state_lock);
  if (state == SNMPMGR_RUNNING) {
    (void)lagopus_mutex_unlock(&snmp_state_lock);
    (void)lagopus_mutex_lock(&snmp_lock);
    ret = internal_snmpmgr_poll(nsec);
    (void)lagopus_mutex_unlock(&snmp_lock);
    return ret;
  } else {
    (void)lagopus_mutex_unlock(&snmp_state_lock);
    lagopus_msg_error("SNMP agent is not started.\n");
    return LAGOPUS_RESULT_NOT_STARTED;
  }
}

static void
check_status_and_send_traps(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct port_stat *port_stat;
  size_t index;
  char ifname[IFNAMSIZ + 1];
  size_t len = 0;
  struct ifTable_entry *entry = NULL;
  int32_t ifAdminStatus, ifOperStatus;
  int32_t old_ifAdminStatus, old_ifOperStatus;

  if ((ret = dp_get_port_stat(&port_stat)) == LAGOPUS_RESULT_OK) {
    /* check each port_stat, send trap if needed! */
    for (index = 0;
         dataplane_interface_get_ifDescr(
           port_stat, index, ifname, &len) == LAGOPUS_RESULT_OK;
         index++) {
      if (len > 0) {
        int32_t ifIndex = (int32_t) (index + 1);
        if (dataplane_interface_get_ifAdminStatus(
              port_stat, index, &ifAdminStatus) == LAGOPUS_RESULT_OK
            && dataplane_interface_get_ifOperStatus(
              port_stat, index, &ifOperStatus) == LAGOPUS_RESULT_OK) {
          /* Note:
           *  index may be changed if a port is attached/detacched to vSwitch,
           *  so use ifName as key.
           */
          ret = lagopus_hashmap_find(&iftable_entry_hash,
                                     (void *)ifname,
                                     (void *)&entry);
          if (ret == LAGOPUS_RESULT_OK) {
            old_ifAdminStatus = entry->ifAdminStatus;
            old_ifOperStatus = entry->ifOperStatus;
            entry->ifAdminStatus = ifAdminStatus;
            entry->ifOperStatus = ifOperStatus;
            if (ifOperStatus != old_ifOperStatus) {
              /* operation status prior to administration status */
              if (ifOperStatus == up) { /* is up */
                (void)send_linkUp_trap(ifIndex, ifAdminStatus, ifOperStatus);
              } else if (ifOperStatus != up && old_ifOperStatus == up) {
                (void)send_linkDown_trap(ifIndex, ifAdminStatus, ifOperStatus);
              }
            } else if (ifAdminStatus != old_ifAdminStatus) {
              if (ifAdminStatus == up) {
                (void)send_linkUp_trap(ifIndex, ifAdminStatus, ifOperStatus);
              } else if (ifAdminStatus != up && old_ifAdminStatus == up) {
                (void)send_linkDown_trap(ifIndex, ifAdminStatus, ifOperStatus);
              }
            }
          } else {
            /* The entry does not exist in previous status checking, */
            /* so the port_stat is attached recentry. */
            if (ifOperStatus == up) {
              (void)send_linkUp_trap(ifIndex, ifAdminStatus, ifOperStatus);
            }
            if ((entry = (struct ifTable_entry *)
                         malloc (sizeof(*entry))) != NULL) {
              entry->ifAdminStatus = ifAdminStatus;
              entry->ifOperStatus = ifOperStatus;
              if ((ret = lagopus_hashmap_add(&iftable_entry_hash,
                                             (void *)ifname,
                                             (void **)&entry, false))
                  != LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                free(entry);
              }
            } else {
              lagopus_perror(LAGOPUS_RESULT_NO_MEMORY);
            }
          }
        }
      }
    }
  }
}

static lagopus_result_t
snmpmgr_thread_loop(const lagopus_thread_t *selfptr, void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_chrono_t interval = DEFAULT_SNMPMGR_LOOP_INTERVAL_NSEC;
  global_state_t global_state;
  shutdown_grace_level_t l;
  (void)selfptr;
  (void)arg;

  /* open the session to AgentX master agent here. */
  init_snmp(SNMP_TYPE);

  /* wait all modules start */
  if ((ret = global_state_wait_for(GLOBAL_STATE_STARTED,
                                   &global_state, &l,
                                   -1 /* forever! */)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  } else {
    if (global_state != GLOBAL_STATE_STARTED) {
      lagopus_exit_fatal("must not happen. die!\n");
    }
  }

  /* all modules have started, then send a coldStart trap */
  (void)send_coldStart_trap();

  lagopus_msg_info("SNMP manager started (as a thread).\n");

  /* main loop */
  while (keep_running) {
    (void)lagopus_mutex_lock(&snmp_lock);
    ret = internal_snmpmgr_poll(interval);
    (void)lagopus_mutex_unlock(&snmp_lock);
    if (ret != LAGOPUS_RESULT_OK && ret != LAGOPUS_RESULT_TIMEDOUT) {
      lagopus_msg_warning("failed to poll SNMP AgentX request: %s",
                          lagopus_error_get_string(ret));
    }
    check_status_and_send_traps();
  }

  /* stop SNMP */
  snmp_shutdown(SNMP_TYPE);

  (void)lagopus_mutex_lock(&snmp_state_lock);
  if (state != SNMPMGR_RUNNABLE) {
    state = SNMPMGR_NONE;
  }
  (void)lagopus_mutex_unlock(&snmp_state_lock);

  return LAGOPUS_RESULT_OK;
}

static void
snmpmgr_thread_finally_called(
  const lagopus_thread_t *thd, bool is_canceled,
  void *arg) {
  bool is_valid = false;
  lagopus_result_t ret;
  (void) is_canceled;
  (void) arg;

  if (*thd != snmpmgr) {
    lagopus_exit_fatal("other threads worked, argument is :%p, snmpmgr is :%p\n",
                       *thd, snmpmgr);
  }

  ret = lagopus_thread_is_valid(&snmpmgr, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    return;
  }

  (void)lagopus_mutex_unlock(&snmp_lock);
  (void)lagopus_mutex_unlock(&snmp_state_lock);

  (void)lagopus_mutex_lock(&snmp_state_lock);
  state = SNMPMGR_NONE;
  (void)lagopus_mutex_unlock(&snmp_state_lock);

  global_state_cancel_janitor();
}

static void
initialize_internal(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  void *logger_client_arg = NULL;
  char *master_agentx_socket = NULL;
  char *ping_interval_string = NULL;
  uint16_t ping_interval = 0;

  state = SNMPMGR_NONE;

  /* `is_thread` is able to be changed only once here */
  is_thread = is_thread_dummy;
  if (is_thread == false && thdptr_dummy != NULL) {
    *thdptr_dummy = NULL;
  } else if (thdptr_dummy != NULL) {
    *thdptr_dummy = &snmpmgr;
  } else {
    /* never reached! */
    return;
  }

  if (lagopus_mutex_create(&snmp_lock) != LAGOPUS_RESULT_OK) {
    return;
  }

  if (lagopus_mutex_create(&snmp_state_lock) != LAGOPUS_RESULT_OK) {
    return;
  }

  snmp_set_do_debugging(false);

  /* setup Net-SNMP logger to use the lagopus logging function */
  if (snmp_register_callback(SNMP_CALLBACK_LIBRARY,
                             SNMP_CALLBACK_LOGGING,
                             snmp_log_callback_wrapper,
                             logger_client_arg) != SNMPERR_SUCCESS) {
    return;
  }
  snmp_enable_calllog();

  /* setup the SNMP module to be Agentx subagent */
  netsnmp_enable_subagent();

#ifdef THE_CONFSYS_ERA
  master_agentx_socket = config_get("snmp master-agentx-socket WORD");
  if (master_agentx_socket == NULL) {
    config_set_default("snmp master-agentx-socket WORD",
                       (char *)DEFAULT_SNMPMGR_AGENTX_SOCKET);
    master_agentx_socket = config_get("snmp master-agentx-socket WORD");
  }
#else
  /*
   * FIXME:
   *	Fetch it from the datastore.
   */
  master_agentx_socket = (char *)DEFAULT_SNMPMGR_AGENTX_SOCKET;
#endif /* THE_CONFSYS_ERA */
  lagopus_msg_debug(25, "master agentx socket is %s\n", master_agentx_socket);

  if (netsnmp_ds_set_string(NETSNMP_DS_APPLICATION_ID,
                            NETSNMP_DS_AGENT_X_SOCKET,
                            master_agentx_socket)
      != SNMPERR_SUCCESS) {
    initialize_ret = LAGOPUS_RESULT_SNMP_API_ERROR;
    return;
  }

  /* don't read/write configuration files */
  if (netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID,
                             NETSNMP_DS_LIB_DONT_PERSIST_STATE,
                             true) != SNMPERR_SUCCESS) {
    initialize_ret = LAGOPUS_RESULT_SNMP_API_ERROR;
    return;
  }
  if (netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID,
                             NETSNMP_DS_LIB_DISABLE_PERSISTENT_LOAD,
                             true) != SNMPERR_SUCCESS) {
    initialize_ret = LAGOPUS_RESULT_SNMP_API_ERROR;
    return;
  }

  /* the interval [sec] to send ping to AgentX master agent */
#ifdef THE_CONFSYS_ERA
  ping_interval_string = config_get("snmp ping-interval-second 1-255");
  if (ping_interval_string != NULL &&
      (ret = check_ping_interval(ping_interval_string, &ping_interval))
      != LAGOPUS_RESULT_OK) {
    config_set_default("snmp ping-interval-second 1-255",
                       (char *) __STR__(DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC));
    ping_interval_string = config_get("snmp ping-interval-second 1-255");
    ping_interval = DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC; /* default value */
  }
#else
  /*
   * FIXME:
   *	Fetch it from the datastore.
   */
  ping_interval = DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC;
#endif /* THE_CONFSYS_ERA */

  if (netsnmp_ds_set_int(NETSNMP_DS_APPLICATION_ID,
                         NETSNMP_DS_AGENT_AGENTX_PING_INTERVAL,
                         (int)ping_interval)
      != SNMPERR_SUCCESS) {
    initialize_ret = LAGOPUS_RESULT_SNMP_API_ERROR;
    return;
  }

  if (init_agent(SNMP_TYPE) != SNMPERR_SUCCESS) {
    initialize_ret = LAGOPUS_RESULT_SNMP_API_ERROR;
    return;
  }

  if ((ret = lagopus_hashmap_create(&iftable_entry_hash,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    delete_entry)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    exit(1);
  }

  init_ifTable();
  init_ifNumber();
  init_dot1dBaseBridgeAddress();
  init_dot1dBaseNumPorts();
  init_dot1dBasePortTable();
  init_dot1dBaseType();

  if (is_thread == false) {
    initialize_ret = LAGOPUS_RESULT_OK;
  } else {
    keep_running = true;
    initialize_ret = lagopus_thread_create(&snmpmgr,
                                           snmpmgr_thread_loop,
                                           snmpmgr_thread_finally_called,
                                           NULL,
                                           "snmpmgr",
                                           NULL);
  }

  lagopus_msg_info("SNMP manager initialized.\n");
}

lagopus_result_t
snmpmgr_initialize(void *arg, lagopus_thread_t **thdptr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  if (state == SNMPMGR_NONE) {
    /* don't check `arg` is NULL */
    /* there may be no difference between NULL and false! */
    is_thread_dummy = (bool) arg;
    /* ONLY checking whether are they valid argments or not */
    if (is_thread_dummy == false || thdptr != NULL) {
      int pthd_once_ret;
      thdptr_dummy = thdptr;
      if ((pthd_once_ret = pthread_once(&initialized, initialize_internal))
          == 0) {
        ret = initialize_ret;
        if (ret == LAGOPUS_RESULT_OK) {
          (void)lagopus_mutex_lock(&snmp_state_lock);
          state = SNMPMGR_RUNNABLE;
          (void)lagopus_mutex_unlock(&snmp_state_lock);
        }
      } else {
        errno = pthd_once_ret;
        perror("pthread_once");
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {  /* if is_thread_dummy is true and thdptr is NULL */
      ret = LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = initialize_ret;
  }
  return ret;
}

lagopus_result_t
snmpmgr_start(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void)lagopus_mutex_lock(&snmp_state_lock);
  if (state == SNMPMGR_RUNNABLE) {
    if (is_thread == false) {
      state = SNMPMGR_RUNNING;
      (void)lagopus_mutex_unlock(&snmp_state_lock);
      /* open the session to AgentX master agent here. */
      init_snmp(SNMP_TYPE);
      /* send_*_trap always return SNMP_ERR_NOERROR */
      (void)send_coldStart_trap();
      ret = LAGOPUS_RESULT_OK;
      lagopus_msg_info("SNMP manager started.\n");
    } else {
      /* Note:
       * If is_thread is true, init_snmp and send_coldStart_trap
       * are called inside snmpmgr_thread_loop
       */
      (void)lagopus_mutex_unlock(&snmp_state_lock);
      ret = lagopus_thread_start(&snmpmgr, false);
    }
  } else {
    (void)lagopus_mutex_unlock(&snmp_state_lock);
    ret = LAGOPUS_RESULT_NOT_STARTED;
  }
  return ret;
}

lagopus_result_t
snmpmgr_shutdown(shutdown_grace_level_t level) {
  (void) level;
  if (is_thread == false) {
    snmp_shutdown(SNMP_TYPE);
    return LAGOPUS_RESULT_OK;
  } else {
    bool is_valid = false;
    lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
    if ((ret = lagopus_thread_is_valid(&snmpmgr, &is_valid))
        != LAGOPUS_RESULT_OK || is_valid == false) {
      lagopus_perror(ret);
      return ret;
    }
    keep_running = false;
    return ret;
  }
}

static inline
lagopus_result_t
snmpmgr_stop_single(void) {
  (void)lagopus_mutex_lock(&snmp_state_lock);
  if (state == SNMPMGR_RUNNING) {
    snmp_shutdown(SNMP_TYPE);
    state = SNMPMGR_RUNNABLE;
    (void)lagopus_mutex_unlock(&snmp_state_lock);
    return LAGOPUS_RESULT_OK;
  } else {
    (void)lagopus_mutex_unlock(&snmp_state_lock);
    return LAGOPUS_RESULT_NOT_STARTED;
  }
}

static inline
lagopus_result_t
snmpmgr_stop_thread(void) {
  return lagopus_thread_cancel(&snmpmgr);
}

lagopus_result_t
snmpmgr_stop(void) {
  if (is_thread == false) {
    return snmpmgr_stop_single();
  } else {
    return snmpmgr_stop_thread();
  }
}

void
snmpmgr_finalize(void) {
  if (snmp_lock == NULL) {
    lagopus_mutex_destroy(&snmp_lock);
    snmp_lock = NULL;
  }
  if (snmp_state_lock == NULL) {
    lagopus_mutex_destroy(&snmp_state_lock);
    snmp_state_lock = NULL;
  }
  if (iftable_entry_hash != NULL) {
    lagopus_hashmap_destroy(&iftable_entry_hash, true);
    iftable_entry_hash = NULL;
  }
  state = SNMPMGR_NONE;
}
