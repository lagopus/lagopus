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
 *      @file   dpdk.c
 *      @brief  Datapath driver use with Intel DPDK
 */

/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2013 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>

#include <rte_config.h>
#include <rte_common.h>
#include <rte_version.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_tailq.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_mempool.h>
#include <rte_string_fns.h>

#include "lagopus/ethertype.h"
#include "lagopus/flowdb.h"
#include "lagopus/meter.h"
#include "lagopus/port.h"
#include "lagopus_gstate.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/vector.h"

#include "lagopus/dataplane.h"
#include "pktbuf.h"
#include "packet.h"
#include "dpdk/dpdk.h"
#include "mgr/thread.h"

rte_atomic32_t dpdk_stop = RTE_ATOMIC32_INIT(0);
bool rawsocket_only_mode;

int
app_lcore_main_loop(void *arg) {
  struct app_lcore_params *lp;
  unsigned lcore;

  lcore = rte_lcore_id();
  lp = &app.lcore_params[lcore];

  if (lp->type == e_APP_LCORE_IO) {
    printf("Logical core %u (I/O) main loop.\n", lcore);
#ifdef HAVE_PTHREAD_SETNAME_NP
    (void)pthread_setname_np(pthread_self(), "main_loop_io");
#endif /* HAVE_PTHREAD_SETNAME_NP */
    app_lcore_main_loop_io(arg);
  }

  if (lp->type == e_APP_LCORE_WORKER) {
#ifdef HAVE_PTHREAD_SETNAME_NP
    char name[32];

    snprintf(name, sizeof(name), "worker_%d", lp->worker.worker_id);
    (void)pthread_setname_np(pthread_self(), name);
#endif /* HAVE_PTHREAD_SETNAME_NP */
    printf("Logical core %u (worker %u) main loop.\n",
           lcore,
           (unsigned) lp->worker.worker_id);
    app_lcore_main_loop_worker(arg);
  }

  if (lp->type == e_APP_LCORE_IO_WORKER) {
#ifdef HAVE_PTHREAD_SETNAME_NP
    char name[32];

    snprintf(name, sizeof(name), "ioworker_%d", lp->worker.worker_id);
    (void)pthread_setname_np(pthread_self(), name);
#endif /* HAVE_PTHREAD_SETNAME_NP */
    printf("Logical core %u (io-worker %u) main loop.\n",
           lcore,
           (unsigned) lp->worker.worker_id);
    app_lcore_main_loop_io_worker(arg);
  }

  return 0;
}

void
app_init(void) {
  app_assign_worker_ids();
  app_init_mbuf_pools();
  app_init_rings_rx();
  app_init_rings_tx();
  app_init_nics();
#ifdef __linux__
  app_init_kni();
#endif /* __linux__ */

  printf("Initialization completed.\n");
}

/* --------------------------Lagopus code start ----------------------------- */


struct lagopus_packet *
alloc_lagopus_packet(void) {
  struct lagopus_packet *pkt;
  struct rte_mbuf *mbuf;
  unsigned sock;

  mbuf = NULL;
  if (rawsocket_only_mode != true) {
    for (sock = 0; sock < APP_MAX_SOCKETS; sock++) {
      if (app.pools[sock] != NULL) {
        mbuf = rte_pktmbuf_alloc(app.pools[sock]);
        break;
      }
    }
    if (mbuf == NULL) {
      lagopus_msg_error("rte_pktmbuf_alloc failed\n");
      return NULL;
    }
  } else {
    /* do not use rte_mempool because it is not initialized. */
    mbuf = calloc(1, sizeof(struct rte_mbuf) + APP_DEFAULT_MBUF_SIZE);
    if (mbuf == NULL) {
      lagopus_msg_error("memory exhausted\n");
      return NULL;
    }
    mbuf->buf_addr = (void *)&mbuf[1];
    mbuf->buf_len = APP_DEFAULT_MBUF_SIZE;
    rte_pktmbuf_reset(mbuf);
    rte_mbuf_refcnt_set(mbuf, 1);
  }
  pkt = (struct lagopus_packet *)
        (mbuf->buf_addr + APP_DEFAULT_MBUF_LOCALDATA_OFFSET);
  pkt->mbuf = mbuf;
  return pkt;
}

/**
 * Copy packet for output.
 */
struct lagopus_packet *
copy_packet(struct lagopus_packet *src_pkt) {
  struct rte_mbuf *mbuf;
  struct lagopus_packet *pkt;
  size_t pktlen;

  pkt = alloc_lagopus_packet();
  if (pkt == NULL) {
    lagopus_msg_error("alloc_lagopus_packet failed\n");
    return NULL;
  }
  mbuf = pkt->mbuf;
  pktlen = OS_M_PKTLEN(src_pkt->mbuf);
  OS_M_APPEND(mbuf, pktlen);
  memcpy(OS_MTOD(pkt->mbuf, char *), OS_MTOD(src_pkt->mbuf, char *), pktlen);
  pkt->in_port = src_pkt->in_port;
  pkt->flags = PKT_FLAG_CACHED_FLOW;
  /* other pkt members are not used in physical output. */
  return pkt;
}

void
lagopus_instruction_experimenter(__UNUSED struct lagopus_packet *pkt,
                                 __UNUSED uint32_t exp_id) {
}

/* --------------------------Lagopus code end---------------------------- */

/**
 * loop function wait for finish all DPDK thread.
 */
static lagopus_result_t
dp_dpdk_thread_loop(__UNUSED const lagopus_thread_t *selfptr,
                    __UNUSED void *arg) {
  unsigned lcore_id;

  lagopus_result_t rv;
  global_state_t cur_state;
  shutdown_grace_level_t cur_grace;

  rv = global_state_wait_for(GLOBAL_STATE_STARTED,
                             &cur_state,
                             &cur_grace,
                             -1);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }

  for (;;) {
    if (rte_atomic32_read(&dpdk_stop) != 0) {
      break;
    }
    sleep(1);
  }
  /* 'stop' is requested */
  RTE_LCORE_FOREACH_SLAVE(lcore_id) {
    if (rte_eal_wait_lcore(lcore_id) < 0) {
      return LAGOPUS_RESULT_STOP;
    }
  }
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dpdk_dataplane_init(int argc, const char *const argv[]) {
  int ret;
  size_t argsize;
  char **copy_argv;
  int i;

  argsize = sizeof(char *) * (argc + 1);
  copy_argv = malloc(argsize);
  if (copy_argv == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  copy_argv[0] = argv[0];
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--")) {
      memcpy(&copy_argv[1], &argv[i + 1], sizeof(char *) * (argc - i - 1));
      break;
    }
  }
  if (i == argc) {
    /* "--" is not found in argv */
    memcpy(copy_argv, argv, argsize);
    rawsocket_only_mode = true;
  } else {
    argc -= i;
    optind = 1;

    /* init EAL */
    ret = rte_eal_init(argc, copy_argv);
    if (ret < 0) {
      return LAGOPUS_RESULT_INVALID_ARGS;
    }
    optind = ret + 1;
    rawsocket_only_mode = false;
  }

  /* parse application arguments (after the EAL ones) */
  ret = app_parse_args(argc, copy_argv);
  free(copy_argv);
  if (ret < 0) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (rawsocket_only_mode != true) {
    /* Init */
    app_init();
    app_print_params();
  }

  return LAGOPUS_RESULT_OK;
}

static lagopus_thread_t dpdk_thread = NULL;
static bool dpdk_run = false;
static lagopus_mutex_t dpdk_lock = NULL;
rte_atomic32_t dpdk_stop;

lagopus_result_t
dp_dpdk_thread_init(int argc,
                    const char *const argv[],
                    __UNUSED void *extarg,
                    lagopus_thread_t **thdptr) {
  static struct dataplane_arg dparg;

  if (rawsocket_only_mode != true) {
    dparg.threadptr = &dpdk_thread;
    dparg.lock = &dpdk_lock;
    lagopus_thread_create(&dpdk_thread, dp_dpdk_thread_loop,
                          dp_finalproc, dp_freeproc, "dp_dpdk", &dparg);
    if (lagopus_mutex_create(&dpdk_lock) != LAGOPUS_RESULT_OK) {
      lagopus_exit_fatal("lagopus_mutex_create");
    }
  }
  *thdptr = &dpdk_thread;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_dpdk_thread_start(void) {
  lagopus_result_t rv;

  rv = LAGOPUS_RESULT_OK;
  /* launch per-lcore init on every lcore */
  if (dpdk_run == false && rawsocket_only_mode != true) {
    rte_eal_mp_remote_launch(app_lcore_main_loop, NULL, SKIP_MASTER);
    rv = dp_thread_start(&dpdk_thread, &dpdk_lock, &dpdk_run);
  }
  return rv;
}

void
dp_dpdk_thread_fini(void) {
  if (rawsocket_only_mode != true) {
    dp_thread_finalize(&dpdk_thread);
  }
}

lagopus_result_t
dp_dpdk_thread_shutdown(shutdown_grace_level_t level) {
  lagopus_result_t rv;

  rv = LAGOPUS_RESULT_OK;
  if (rawsocket_only_mode != true) {
    rv = dp_thread_shutdown(&dpdk_thread, &dpdk_lock, &dpdk_run, level);
  }
  return rv;
}

lagopus_result_t
dp_dpdk_thread_stop(void) {
  lagopus_result_t rv;

#ifdef HAVE_DPDK
  rte_atomic32_inc(&dpdk_stop);
#endif /* HAVE_DPDK */

  rv = LAGOPUS_RESULT_OK;
#ifdef HAVE_DPDK
  if (rawsocket_only_mode != true) {
    rv = dp_thread_stop(&dpdk_thread, &dpdk_run);
  }
#endif /* HAVE_DPDK */
  return rv;
}

#if 0
#define MODIDX_DPDK  LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE + 109
#define MODNAME_DPDK "dp_dpdk"

static void dpdk_ctors(void) __attr_constructor__(MODIDX_DPDK);
static void dpdk_dtors(void) __attr_constructor__(MODIDX_DPDK);

static void dpdk_ctors (void) {
  lagopus_result_t rv;
  const char *name = "dp_dpdk";

  rv = lagopus_module_register(name,
                               dp_dpdk_thread_init,
                               NULL,
                               dp_dpdk_thread_start,
                               dp_dpdk_thread_shutdown,
                               dp_dpdk_thread_stop,
                               dp_dpdk_thread_fini,
                               dp_dpdk_thread_usage);
  if (rv != LAGOPUS_RESULT_OK) {
    lagopus_perror(rv);
    lagopus_exit_fatal("can't register the \"%s\" module.\n", name);
  }
}
#endif /* 0 */
