/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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
 *	@file	thread.c
 *	@brief	Datapath thread control functions
 */

#ifdef HAVE_DPDK
#include <rte_config.h>
#include <rte_lcore.h>
#include <rte_atomic.h>
#include <rte_launch.h>
#endif /* HAVE_DPDK */

#include "lagopus_apis.h"
#include "lagopus_gstate.h"
#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
#include "lagopus/flowinfo.h"

/* so far, global variable. */
static struct dpmgr *my_dpmgr;
static lagopus_thread_t datapath_thread = NULL;
static bool run = false;
static lagopus_mutex_t lock = NULL;

static lagopus_thread_t comm_thread = NULL;
static bool comm_run = false;
static lagopus_mutex_t comm_lock = NULL;

static lagopus_thread_t timer_thread = NULL;
static bool timer_run = false;
static lagopus_mutex_t timer_lock = NULL;

#define TIMEOUT_SHUTDOWN_RIGHT_NOW     (100*1000*1000) /* 100msec */
#define TIMEOUT_SHUTDOWN_GRACEFULLY    (1500*1000*1000) /* 1.5sec */

#ifdef HAVE_DPDK
int app_lcore_main_loop(void *arg);
rte_atomic32_t dpdk_stop;
#endif /* HAVE_DPDK */

static void
finalproc(const lagopus_thread_t *t, bool is_canceled, void *arg) {
  bool is_valid = false;
  struct datapath_arg *dparg;
  lagopus_result_t ret;

  (void) is_canceled;
  dparg = arg;

  lagopus_msg_info("finalproc in %p\n", *dparg->threadptr);
  if (*t != *dparg->threadptr) {
    lagopus_exit_fatal("other threads worked t:%p, my_thread:%p\n",
                       *t, *dparg->threadptr);
  }

  ret = lagopus_thread_is_valid(dparg->threadptr, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    return;
  }
}

static void
freeproc(const lagopus_thread_t *t, void *arg) {
  bool is_valid = false;
  struct datapath_arg *dparg;
  lagopus_result_t ret;

  dparg = arg;

  lagopus_msg_info("freeproc in\n");
  if (*t != *dparg->threadptr) {
    lagopus_exit_fatal("other threads worked %p, %p", *t, *dparg->threadptr);
  }

  ret = lagopus_thread_is_valid(dparg->threadptr, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    return;
  }
  /* ADD delete ports? */

  lagopus_mutex_destroy(&lock);
  lock = NULL;
}

static lagopus_result_t
thread_start(lagopus_thread_t *threadptr,
             lagopus_mutex_t *lockptr,
             bool *runptr) {
  lagopus_result_t ret;

  lagopus_mutex_lock(lockptr);
  if (*runptr == true) {
    ret = LAGOPUS_RESULT_ALREADY_EXISTS;
    goto done;
  }
  *runptr = true;
  ret = lagopus_thread_start(threadptr, false);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }
done:
  lagopus_mutex_unlock(lockptr);
  return ret;
}

static void
thread_finalize(lagopus_thread_t *threadptr) {
  bool is_valid = false, is_canceled = false;
  lagopus_result_t ret;

  ret = lagopus_thread_is_valid(threadptr, &is_valid);
  if (ret != LAGOPUS_RESULT_OK || is_valid == false) {
    lagopus_perror(ret);
    return;
  }
  ret = lagopus_thread_is_canceled(threadptr, &is_canceled);
  if (ret == LAGOPUS_RESULT_OK && is_canceled == false) {
    global_state_cancel_janitor();
  }
  lagopus_thread_destroy(threadptr);

  return;
}

static lagopus_result_t
thread_shutdown(lagopus_thread_t *threadptr,
                lagopus_mutex_t *lockptr,
                bool *runptr,
                shutdown_grace_level_t level) {
  bool is_valid = false;
  lagopus_chrono_t nsec;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_info("shutdown called\n");
  lagopus_mutex_lock(lockptr);
  if (*runptr == false) {
    goto done;
  }

  ret = lagopus_thread_is_valid(threadptr, &is_valid);
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

  *runptr = false;
  ret = lagopus_thread_wait(threadptr, nsec);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

done:
  lagopus_mutex_unlock(lockptr);
  return ret;
}

static lagopus_result_t
thread_stop(lagopus_thread_t *threadptr, bool *runptr) {
  bool is_canceled = false;
  lagopus_result_t ret;

  ret = lagopus_thread_is_canceled(threadptr, &is_canceled);
  if (ret == LAGOPUS_RESULT_OK && is_canceled == false) {
    ret = lagopus_thread_cancel(threadptr);
  }
  *runptr = false;

  return ret;
}

static lagopus_result_t
commthread_initialize(void *arg) {
  lagopus_thread_create(&comm_thread, comm_thread_loop,
                        finalproc, freeproc, "datapath comm", arg);
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
timerthread_initialize(void *arg) {
  lagopus_thread_create(&timer_thread, flow_timer_loop,
                        finalproc, freeproc, "datapath timer", arg);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
datapath_initialize(int argc,
                    const char *const argv[],
                    void *extarg,
                    lagopus_thread_t **thdptr) {
  static struct datapath_arg dparg;
  static struct datapath_arg commarg;
  static struct datapath_arg timerarg;
#ifdef STANDALONE
  int portid;
#endif
  lagopus_result_t nb_ports;

  dparg.dpmgr = my_dpmgr = extarg;
  nb_ports = lagopus_datapath_init(argc, argv);
  if (nb_ports < 0) {
    lagopus_msg_fatal("lagopus_datapath_init failed\n");
    return nb_ports;
  }

  dparg.threadptr = &datapath_thread;
  lagopus_thread_create(&datapath_thread, datapath_thread_loop,
                        finalproc, freeproc, "datapath", &dparg);

  if (lagopus_mutex_create(&lock) != LAGOPUS_RESULT_OK) {
    lagopus_exit_fatal("lagopus_mutex_create");
  }
  *thdptr = &datapath_thread;
  lagopus_meter_init();
  lagopus_register_action_hook = lagopus_set_action_function;
  lagopus_register_instruction_hook = lagopus_set_instruction_function;
  flowinfo_init();

#ifdef STANDALONE
  /* register all physical ports */
  for (portid = 0; portid < nb_ports; portid++) {
    if ((l2fwd_enabled_port_mask & (1 << portid)) != 0) {
      struct port nport;

      /* New port API. */
      nport.ofp_port.port_no = (uint32_t)portid + 1;
      nport.ifindex = (uint32_t)portid;
      printf("port id %u\n", nport.ofp_port.port_no);

      snprintf(nport.ofp_port.name , sizeof(nport.ofp_port.name),
               "eth%d", portid);
      dpmgr_port_add(my_dpmgr, &nport);
    }
  }

  /* assign all ports to br0 */
  for (portid = 0; portid < nb_ports; portid++) {
    dpmgr_bridge_port_add(my_dpmgr, "br0", (uint32_t)portid + 1);
  }
#endif
  commarg.dpmgr = my_dpmgr;
  commarg.threadptr = &comm_thread;
  commarg.running = &comm_run;
  commthread_initialize(&commarg);

  timerarg.dpmgr = my_dpmgr;
  timerarg.threadptr = &timer_thread;
  timerarg.running = NULL;
  timerthread_initialize(&timerarg);

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
datapath_start(void) {
  lagopus_result_t rv;
#ifdef HAVE_DPDK
  /* launch per-lcore init on every lcore */
  if (run == false) {
    rte_eal_mp_remote_launch(app_lcore_main_loop, my_dpmgr, SKIP_MASTER);
  }
#endif /* HAVE_DPDK */
  rv = thread_start(&comm_thread, &comm_lock, &comm_run);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = thread_start(&timer_thread, &timer_lock, &timer_run);
  }
  if (rv == LAGOPUS_RESULT_OK) {
    rv = thread_start(&datapath_thread, &lock, &run);
  }
  return rv;
}

void
datapath_finalize(void) {
  thread_finalize(&comm_thread);
  thread_finalize(&timer_thread);
  thread_finalize(&datapath_thread);
}

/* Datapath thread shutdown. */
lagopus_result_t
datapath_shutdown(shutdown_grace_level_t level) {
  lagopus_result_t rv, rv2;

  rv = thread_shutdown(&comm_thread, &comm_lock, &comm_run, level);
  rv2 = thread_shutdown(&timer_thread, &timer_lock, &timer_run, level);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = rv2;
  }
  rv2 = thread_shutdown(&datapath_thread, &lock, &run, level);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = rv2;
  }
  return rv;
}
/* Datapath thread stop. */
lagopus_result_t
datapath_stop(void) {
  lagopus_result_t rv, rv2;

#ifdef HAVE_DPDK
  rte_atomic32_inc(&dpdk_stop);
#endif /* HAVE_DPDK */

  rv = thread_stop(&comm_thread, &comm_run);
  rv2 = thread_stop(&timer_thread, &timer_run);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = rv2;
  }
  rv2 = thread_stop(&datapath_thread, &run);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = rv2;
  }
  return rv;
}

#if 0
#define MODIDX_DATAPLANE  LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE + 108
#define MODNAME_DATAPLANE "datapath"

static void datapath_ctors(void) __attr_constructor__(MODIDX_DATAPLANE);
static void datapath_dtors(void) __attr_constructor__(MODIDX_DATAPLANE);

static void datapath_ctors (void) {
  lagopus_module_register("datapath",
                          datapath_initialize,
                          s_dpmptr,
                          datapath_start,
                          datapath_shutdown,
                          datapath_stop,
                          datapath_finalize,
                          datapath_usage
                         );
}
#endif
