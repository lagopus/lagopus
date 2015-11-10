/* %COPYRIGHT% */

/**
 *      @file   thread.c
 *      @brief  Dataplane thread control functions
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
#include "lagopus/dp_apis.h"

#include "thread.h"

/* so far, global variable. */
#ifdef HAVE_DPDK
static lagopus_thread_t dpdk_thread = NULL;
static bool dpdk_run = false;
static lagopus_mutex_t dpdk_lock = NULL;
#endif /* HAVE_DPDK */

static lagopus_thread_t timer_thread = NULL;
static bool timer_run = false;
static lagopus_mutex_t timer_lock = NULL;

static lagopus_thread_t sock_thread = NULL;
static bool sock_run = false;
static lagopus_mutex_t sock_lock = NULL;

#define TIMEOUT_SHUTDOWN_RIGHT_NOW     (100*1000*1000) /* 100msec */
#define TIMEOUT_SHUTDOWN_GRACEFULLY    (1500*1000*1000) /* 1.5sec */

#ifdef HAVE_DPDK
int app_lcore_main_loop(void *arg);
rte_atomic32_t dpdk_stop;
#endif /* HAVE_DPDK */

void
dp_finalproc(const lagopus_thread_t *t, bool is_canceled, void *arg) {
  bool is_valid = false;
  struct dataplane_arg *dparg;
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

void
dp_freeproc(const lagopus_thread_t *t, void *arg) {
  bool is_valid = false;
  struct dataplane_arg *dparg;
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

  lagopus_mutex_destroy(dparg->lock);
  *dparg->lock = NULL;
}

lagopus_result_t
dp_thread_start(lagopus_thread_t *threadptr,
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

void
dp_thread_finalize(lagopus_thread_t *threadptr) {
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

lagopus_result_t
dp_thread_shutdown(lagopus_thread_t *threadptr,
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

lagopus_result_t
dp_thread_stop(lagopus_thread_t *threadptr, bool *runptr) {
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
timerthread_initialize(void *arg) {
  lagopus_thread_create(&timer_thread, dp_timer_loop,
                        dp_finalproc, dp_freeproc, "dataplane timer", arg);
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
sockthread_initialize(void *arg) {
  lagopus_thread_create(&sock_thread, rawsock_thread_loop,
                        dp_finalproc, dp_freeproc, "dataplane rawswock", arg);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dataplane_initialize(int argc,
                     const char *const argv[],
                     __UNUSED void *extarg,
                     lagopus_thread_t **thdptr) {
#ifdef HAVE_DPDK
  static struct dataplane_arg dparg;
#endif /* HAVE_DPDK */
  static struct dataplane_arg sockarg;
  static struct dataplane_arg timerarg;
  lagopus_result_t nb_ports;

  nb_ports = lagopus_dataplane_init(argc, argv);
  if (nb_ports < 0) {
    lagopus_msg_fatal("lagopus_dataplane_init failed\n");
    return nb_ports;
  }
  dp_api_init();
#ifdef HAVE_DPDK
  if (rawsocket_only_mode != true) {
    dparg.threadptr = &dpdk_thread;
    dparg.lock = &dpdk_lock;
    lagopus_thread_create(&dpdk_thread, dpdk_thread_loop,
                          dp_finalproc, dp_freeproc, "dataplane", &dparg);

    if (lagopus_mutex_create(&dpdk_lock) != LAGOPUS_RESULT_OK) {
      lagopus_exit_fatal("lagopus_mutex_create");
    }
  }
#endif /* HAVE_DPDK */

  lagopus_meter_init();
  lagopus_register_action_hook = lagopus_set_action_function;
  lagopus_register_instruction_hook = lagopus_set_instruction_function;
  flowinfo_init();

  timerarg.threadptr = &timer_thread;
  timerarg.lock = &timer_lock;
  timerarg.running = NULL;
  timerthread_initialize(&timerarg);

  sockarg.threadptr = &sock_thread;
  sockarg.lock = &sock_lock;
  sockarg.running = NULL;
  sockthread_initialize(&sockarg);

  *thdptr = &sock_thread;

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dataplane_start(void) {
  lagopus_result_t rv;
#ifdef HAVE_DPDK
  /* launch per-lcore init on every lcore */
  if (dpdk_run == false && rawsocket_only_mode != true) {
    rte_eal_mp_remote_launch(app_lcore_main_loop, NULL, SKIP_MASTER);
  }
#endif /* HAVE_DPDK */
  rv = dp_thread_start(&sock_thread, &sock_lock, &sock_run);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = dp_thread_start(&timer_thread, &timer_lock, &timer_run);
  }
#ifdef HAVE_DPDK
  if (rv == LAGOPUS_RESULT_OK && rawsocket_only_mode != true) {
    rv = dp_thread_start(&dpdk_thread, &dpdk_lock, &dpdk_run);
  }
#endif /* HAVE_DPDK */
  return rv;
}

void
dataplane_finalize(void) {
  dp_thread_finalize(&sock_thread);
  dp_thread_finalize(&timer_thread);
#ifdef HAVE_DPDK
  if (rawsocket_only_mode != true) {
    dp_thread_finalize(&dpdk_thread);
  }
#endif /* HAVE_DPDK */
  dp_api_fini();
}

/* Dataplane thread shutdown. */
lagopus_result_t
dataplane_shutdown(shutdown_grace_level_t level) {
  lagopus_result_t rv, rv2;

  rv = dp_thread_shutdown(&timer_thread, &timer_lock, &timer_run, level);
  rv2 = dp_thread_shutdown(&sock_thread, &sock_lock, &sock_run, level);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = rv2;
  }
#ifdef HAVE_DPDK
  if (rawsocket_only_mode != true) {
    rv2 = dp_thread_shutdown(&dpdk_thread, &dpdk_lock, &dpdk_run, level);
    if (rv == LAGOPUS_RESULT_OK) {
      rv = rv2;
    }
  }
#endif /* HAVE_DPDK */
  return rv;
}
/* Dataplane thread stop. */
lagopus_result_t
dataplane_stop(void) {
  lagopus_result_t rv, rv2;

#ifdef HAVE_DPDK
  rte_atomic32_inc(&dpdk_stop);
#endif /* HAVE_DPDK */

  rv = dp_thread_stop(&timer_thread, &timer_run);
  rv2 = dp_thread_stop(&sock_thread, &sock_run);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = rv2;
  }
#ifdef HAVE_DPDK
  if (rawsocket_only_mode != true) {
    rv2 = dp_thread_stop(&dpdk_thread, &dpdk_run);
    if (rv == LAGOPUS_RESULT_OK) {
      rv = rv2;
    }
  }
#endif /* HAVE_DPDK */
  return rv;
}

#if 0
#define MODIDX_DATAPLANE  LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE + 108
#define MODNAME_DATAPLANE "dataplane"

static void dataplane_ctors(void) __attr_constructor__(MODIDX_DATAPLANE);
static void dataplane_dtors(void) __attr_constructor__(MODIDX_DATAPLANE);

static void dataplane_ctors (void) {
  lagopus_module_register("dataplane",
                          dataplane_initialize,
                          s_dpmptr,
                          dataplane_start,
                          dataplane_shutdown,
                          dataplane_stop,
                          dataplane_finalize,
                          dataplane_usage
                         );
}
#endif
