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
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <string.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <errno.h>
#include <getopt.h>

#include <rte_config.h>
#include <rte_common.h>
#include <rte_byteorder.h>
#include <rte_log.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_string_fns.h>

#include "lagopus_apis.h"
#include "lagopus/dataplane.h"
#include "lagopus/ofcache.h"

#include "dpdk.h"

struct app_params app;
static bool portid_specified = false;
static unsigned lcore_count;

static const char usage[] =
  "Application manadatory parameters:                                             \n"
  "    -w NWORKERS : number of worker lcore (default is assign as possible)       \n"
  "                                                                               \n"
  "    or                                                                         \n"
  "                                                                               \n"
  "    --rx \"(PORT, QUEUE, LCORE), ...\" : List of NIC RX ports and queues       \n"
  "           handled by the I/O RX lcores                                        \n"
  "    --tx \"(PORT, LCORE), ...\" : List of NIC TX ports handled by the I/O TX   \n"
  "           lcores                                                              \n"
  "    --w \"LCORE, ...\" : List of the worker lcores                             \n"
  "                                                                               \n"
  "Application optional parameters:                                               \n"
  "    --core-assign TYPE: select core assignment algorithm, use with -p          \n"
  "           performance  Don't use HT core (default)                            \n"
  "           balance      Use HT core                                            \n"
  "           minimum      Use only 2 core                                        \n"
  "    --show-core-config : Print core assignment configuration and exit          \n"
  "    --no-cache : Don't use flow cache                                          \n"
  "    --kvstype TYPE: Select key-value store type for flow cache                 \n"
  "           hashmap_nolock  Use hashmap without rwlock (default)                \n"
  "           hashmap         Use hashmap                                         \n"
#ifdef __SSE4_2__
  "           rte_hash        Use DPDK hash table                                 \n"
  "    --hashtype TYPE: Select hash type for flow cache                           \n"
  "           city64   CityHash(64bit) (default)                                  \n"
  "           intel64  Intel_hash64                                               \n"
  "           murmur3  MurmurHash3 (32bit)                                        \n"
#endif /* __SSE4_2__ */
  "    --fifoness MODE: Select FIFOness mode, MODE is one of none, port, or flow  \n"
  "           flow : FIFOness per each flow (default.)                            \n"
  "           port : FIFOness per each port.                                      \n"
  "           none : FIFOness is disabled.                                        \n"
  "    --rsz \"A, B, C, D\" : Ring sizes                                          \n"
  "           A = Size (in number of buffer descriptors) of each of the NIC RX    \n"
  "               rings read by the I/O RX lcores (default value is %u)           \n"
  "           B = Size (in number of elements) of each of the SW rings used by the\n"
  "               I/O RX lcores to send packets to worker lcores (default value is\n"
  "               %u)                                                             \n"
  "           C = Size (in number of elements) of each of the SW rings used by the\n"
  "               worker lcores to send packets to I/O TX lcores (default value is\n"
  "               %u)                                                             \n"
  "           D = Size (in number of buffer descriptors) of each of the NIC TX    \n"
  "               rings written by I/O TX lcores (default value is %u)            \n"
  "    --bsz \"(A, B), (C, D), (E, F)\" :  Burst sizes                            \n"
  "           A = I/O RX lcore read burst size from NIC RX (default value is %u)  \n"
  "           B = I/O RX lcore write burst size to output SW rings (default value \n"
  "               is %u)                                                          \n"
  "           C = Worker lcore read burst size from input SW rings (default value \n"
  "               is %u)                                                          \n"
  "           D = Worker lcore write burst size to output SW rings (default value \n"
  "               is %u)                                                          \n"
  "           E = I/O TX lcore read burst size from input SW rings (default value \n"
  "               is %u)                                                          \n"
  "           F = I/O TX lcore write burst size to NIC TX (default value is %u)   \n";

bool
dp_dpdk_is_portid_specified(void) {
  return portid_specified;
}

unsigned
dp_dpdk_lcore_count(void) {
  return lcore_count;
}

static unsigned lcores[RTE_MAX_LCORE];

struct app_lcore_params *
dp_dpdk_get_lcore_param(unsigned lcore) {
  return &app.lcore_params[lcores[lcore]];
}

void
dp_dpdk_thread_usage(FILE *fp) {
  //  eal_common_usage(); /* XXX stdout */
  fprintf(fp, usage,
          APP_DEFAULT_NIC_RX_RING_SIZE,
          APP_DEFAULT_RING_RX_SIZE,
          APP_DEFAULT_RING_TX_SIZE,
          APP_DEFAULT_NIC_TX_RING_SIZE,
          APP_DEFAULT_BURST_SIZE_IO_RX_READ,
          APP_DEFAULT_BURST_SIZE_IO_RX_WRITE,
          APP_DEFAULT_BURST_SIZE_WORKER_READ,
          APP_DEFAULT_BURST_SIZE_WORKER_WRITE,
          APP_DEFAULT_BURST_SIZE_IO_TX_READ,
          APP_DEFAULT_BURST_SIZE_IO_TX_WRITE);
}

#ifndef APP_ARG_RX_MAX_CHARS
#define APP_ARG_RX_MAX_CHARS     4096
#endif

#ifndef APP_ARG_RX_MAX_TUPLES
#define APP_ARG_RX_MAX_TUPLES    128
#endif

static int
str_to_unsigned_array(
  const char *s, size_t sbuflen,
  char separator,
  unsigned num_vals,
  unsigned *vals) {
  char str[sbuflen+1];
  char *splits[num_vals];
  char *endptr = NULL;
  int i, num_splits = 0;

  /* copy s so we don't modify original string */
  snprintf(str, sizeof(str), "%s", s);
  num_splits = rte_strsplit(str, (int)sizeof(str),
                            splits, (int)num_vals, separator);

  errno = 0;
  for (i = 0; i < num_splits; i++) {
    vals[i] = (unsigned)strtoul(splits[i], &endptr, 0);
    if (errno != 0 || *endptr != '\0') {
      return -1;
    }
  }

  return num_splits;
}

static int
str_to_unsigned_vals(
  const char *s,
  size_t sbuflen,
  char separator,
  unsigned num_vals, ...) {
  unsigned i, vals[num_vals];
  va_list ap;

  if (str_to_unsigned_array(s, sbuflen, separator, num_vals, vals) !=
      (int)num_vals) {
    return -1;
  }

  va_start(ap, num_vals);
  for (i = 0; i < num_vals; i++) {
    unsigned *u = va_arg(ap, unsigned *);
    *u = vals[i];
  }
  va_end(ap);
  return (int)num_vals;
}

static int
parse_arg_rx(const char *arg) {
  const char *p0 = arg, *p = arg;
  uint32_t n_tuples;

  if (strnlen(arg, APP_ARG_RX_MAX_CHARS + 1) == APP_ARG_RX_MAX_CHARS + 1) {
    return -1;
  }

  n_tuples = 0;
  while ((p = strchr(p0,'(')) != NULL) {
    struct app_lcore_params *lp;
    const char *p1, *p2;
    uint32_t port, queue, lcore, i, queue_min, queue_max;

    p0 = strchr(p++, ')');
    if (p0 == NULL) {
      return -2;
    }
    if (str_to_unsigned_vals(p, (size_t)(p0 - p), ',', 3,
                             &port, &queue, &lcore) !=  3) {
      p1 = strchr(p, ',');
      if (p1++ == NULL) {
        return -2;
      }
      port = atoi(p);
      p2 = strchr(p1, ',');
      if (p2 == NULL ||
          str_to_unsigned_vals(p1, (size_t)(p2 - p1), '-', 2,
                               &queue_min, &queue_max) != 2) {
        return -2;
      }
      p2++;
      lcore = atoi(p2);
      if (queue_min > queue_max) {
        return -2;
      }
    } else {
      queue_min = queue_max = queue;
    }

    /* Enable port and queue for later initialization */
    for (queue = queue_min; queue <= queue_max; queue++) {
      if ((port >= APP_MAX_NIC_PORTS) ||
          (queue >= APP_MAX_RX_QUEUES_PER_NIC_PORT)) {
        return -3;
      }
      if (app.nic_rx_queue_mask[port][queue] != NIC_RX_QUEUE_UNCONFIGURED) {
        return -4;
      }
      app.nic_rx_queue_mask[port][queue] = NIC_RX_QUEUE_ENABLED;

      /* Check and assign (port, queue) to I/O lcore */
      if (rte_lcore_is_enabled(lcore) == 0) {
        return -5;
      }

      if (lcore >= APP_MAX_LCORES) {
        return -6;
      }
      lp = &app.lcore_params[lcore];
      if (lp->type == e_APP_LCORE_WORKER) {
        return -7;
      }
      lp->type = e_APP_LCORE_IO;
      for (i = 0; i < lp->io.rx.n_nic_queues; i ++) {
        if ((lp->io.rx.nic_queues[i].port == port) &&
            (lp->io.rx.nic_queues[i].queue == queue)) {
          return -8;
        }
      }
      if (lp->io.rx.n_nic_queues >= APP_MAX_NIC_RX_QUEUES_PER_IO_LCORE) {
        return -9;
      }
      lp->io.rx.nic_queues[lp->io.rx.n_nic_queues].port = (uint8_t)port;
      lp->io.rx.nic_queues[lp->io.rx.n_nic_queues].queue = (uint8_t)queue;
      lp->io.rx.n_nic_queues++;
    }
    n_tuples ++;
    if (n_tuples > APP_ARG_RX_MAX_TUPLES) {
      return -10;
    }
  }

  if (n_tuples == 0) {
    return -11;
  }

  return 0;
}

#ifndef APP_ARG_TX_MAX_CHARS
#define APP_ARG_TX_MAX_CHARS     4096
#endif

#ifndef APP_ARG_TX_MAX_TUPLES
#define APP_ARG_TX_MAX_TUPLES    128
#endif

static int
parse_arg_tx(const char *arg) {
  const char *p0 = arg, *p = arg;
  uint32_t n_tuples;

  if (strnlen(arg, APP_ARG_TX_MAX_CHARS + 1) == APP_ARG_TX_MAX_CHARS + 1) {
    return -1;
  }

  n_tuples = 0;
  while ((p = strchr(p0,'(')) != NULL) {
    struct app_lcore_params *lp;
    uint32_t port, lcore, i;

    p0 = strchr(p++, ')');
    if ((p0 == NULL) ||
        (str_to_unsigned_vals(p, (size_t)(p0 - p), ',', 2,
                              &port, &lcore) !=  2)) {
      return -2;
    }

    /* Enable port and queue for later initialization */
    if (port >= APP_MAX_NIC_PORTS) {
      return -3;
    }
    if (app.nic_tx_port_mask[port] != 0) {
      return -4;
    }
    app.nic_tx_port_mask[port] = 1;

    /* Check and assign (port, queue) to I/O lcore */
    if (rte_lcore_is_enabled(lcore) == 0) {
      return -5;
    }

    if (lcore >= APP_MAX_LCORES) {
      return -6;
    }
    lp = &app.lcore_params[lcore];
    if (lp->type == e_APP_LCORE_WORKER) {
      return -7;
    }
    lp->type = e_APP_LCORE_IO;
    for (i = 0; i < lp->io.tx.n_nic_ports; i ++) {
      if (lp->io.tx.nic_ports[i] == port) {
        return -8;
      }
    }
    if (lp->io.tx.n_nic_ports >= APP_MAX_NIC_TX_PORTS_PER_IO_LCORE) {
      return -9;
    }
    lp->io.tx.nic_ports[lp->io.tx.n_nic_ports] = (uint8_t) port;
    lp->io.tx.n_nic_ports ++;

    n_tuples ++;
    if (n_tuples > APP_ARG_TX_MAX_TUPLES) {
      return -10;
    }
  }

  if (n_tuples == 0) {
    return -11;
  }

  return 0;
}

#ifndef APP_ARG_W_MAX_CHARS
#define APP_ARG_W_MAX_CHARS     4096
#endif

#ifndef APP_ARG_W_MAX_TUPLES
#define APP_ARG_W_MAX_TUPLES    APP_MAX_WORKER_LCORES
#endif

static int
parse_arg_w(const char *arg) {
  const char *p = arg;
  uint32_t n_tuples;

  if (strnlen(arg, APP_ARG_W_MAX_CHARS + 1) == APP_ARG_W_MAX_CHARS + 1) {
    return -1;
  }

  n_tuples = 0;
  while (*p != 0) {
    struct app_lcore_params *lp;
    uint32_t lcore;

    errno = 0;
    lcore = (uint32_t)strtoul(p, NULL, 0);
    if ((errno != 0)) {
      return -2;
    }

    /* Check and enable worker lcore */
    if (rte_lcore_is_enabled(lcore) == 0) {
      return -3;
    }

    if (lcore >= APP_MAX_LCORES) {
      return -4;
    }
    lp = &app.lcore_params[lcore];
    if (lp->type == e_APP_LCORE_IO) {
      return -5;
    }
    lp->type = e_APP_LCORE_WORKER;

    n_tuples ++;
    if (n_tuples > APP_ARG_W_MAX_TUPLES) {
      return -6;
    }

    p = strchr(p, ',');
    if (p == NULL) {
      break;
    }
    p ++;
  }

  if (n_tuples == 0) {
    return -7;
  }

  return 0;
}

static int
app_check_every_rx_port_is_tx_enabled(void) {
  uint8_t port;

  for (port = 0; port < APP_MAX_NIC_PORTS; port ++) {
    if ((app_get_nic_rx_queues_per_port(port) > 0) &&
        (app.nic_tx_port_mask[port] == 0)) {
      return -1;
    }
  }
  return 0;
}

#ifndef APP_ARG_RSZ_CHARS
#define APP_ARG_RSZ_CHARS 63
#endif

static int
parse_arg_rsz(const char *arg) {
  if (strnlen(arg, APP_ARG_RSZ_CHARS + 1) == APP_ARG_RSZ_CHARS + 1) {
    return -1;
  }

  if (str_to_unsigned_vals(arg, APP_ARG_RSZ_CHARS, ',', 4,
                           &app.nic_rx_ring_size,
                           &app.ring_rx_size,
                           &app.ring_tx_size,
                           &app.nic_tx_ring_size) !=  4) {
    return -2;
  }


  if ((app.nic_rx_ring_size == 0) ||
      (app.nic_tx_ring_size == 0) ||
      (app.ring_rx_size == 0) ||
      (app.ring_tx_size == 0)) {
    return -3;
  }

  return 0;
}

#ifndef APP_ARG_BSZ_CHARS
#define APP_ARG_BSZ_CHARS 63
#endif

static int
parse_arg_bsz(const char *arg) {
  const char *p = arg, *p0;
  if (strnlen(arg, APP_ARG_BSZ_CHARS + 1) == APP_ARG_BSZ_CHARS + 1) {
    return -1;
  }

  p0 = strchr(p++, ')');
  if ((p0 == NULL) ||
      (str_to_unsigned_vals(p, (size_t)(p0 - p), ',', 2,
                            &app.burst_size_io_rx_read,
                            &app.burst_size_io_rx_write) !=  2)) {
    return -2;
  }

  p = strchr(p0, '(');
  if (p == NULL) {
    return -3;
  }

  p0 = strchr(p++, ')');
  if ((p0 == NULL) ||
      (str_to_unsigned_vals(p, (size_t)(p0 - p), ',', 2,
                            &app.burst_size_worker_read,
                            &app.burst_size_worker_write) !=  2)) {
    return -4;
  }

  p = strchr(p0, '(');
  if (p == NULL) {
    return -5;
  }

  p0 = strchr(p++, ')');
  if ((p0 == NULL) ||
      (str_to_unsigned_vals(p, (size_t)(p0 - p), ',', 2,
                            &app.burst_size_io_tx_read,
                            &app.burst_size_io_tx_write) !=  2)) {
    return -6;
  }

  if ((app.burst_size_io_rx_read == 0) ||
      (app.burst_size_io_rx_write == 0) ||
      (app.burst_size_worker_read == 0) ||
      (app.burst_size_worker_write == 0) ||
      (app.burst_size_io_tx_read == 0) ||
      (app.burst_size_io_tx_write == 0)) {
    return -7;
  }

  if ((app.burst_size_io_rx_read > APP_MBUF_ARRAY_SIZE) ||
      (app.burst_size_io_rx_write > APP_MBUF_ARRAY_SIZE) ||
      (app.burst_size_worker_read > APP_MBUF_ARRAY_SIZE) ||
      (app.burst_size_worker_write > APP_MBUF_ARRAY_SIZE) ||
      ((2 * app.burst_size_io_tx_read) > APP_MBUF_ARRAY_SIZE) ||
      (app.burst_size_io_tx_write > APP_MBUF_ARRAY_SIZE)) {
    return -8;
  }

  return 0;
}

static int
parse_arg_core_assign(const char *arg) {
  if (!strcmp(arg, "performance")) {
    app.core_assign = CORE_ASSIGN_PERFORMANCE;
  } else if (!strcmp(arg, "balance")) {
    app.core_assign = CORE_ASSIGN_BALANCE;
  } else if (!strcmp(arg, "minimum")) {
    app.core_assign = CORE_ASSIGN_MINIMUM;
  } else {
    return -1;
  }
  return 0;
}

static int
parse_arg_kvstype(const char *arg) {
  if (!strcmp(arg, "hashmap_nolock")) {
    app.kvs_type = FLOWCACHE_HASHMAP_NOLOCK;
  } else if (!strcmp(arg, "hashmap")) {
    app.kvs_type = FLOWCACHE_HASHMAP;
  } else if (!strcmp(arg, "rte_hash")) {
    app.kvs_type = FLOWCACHE_RTE_HASH;
  } else {
    return -1;
  }
  return 0;
}

static int
parse_arg_hashtype(const char *arg) {
  if (!strcmp(arg, "intel64")) {
    app.hashtype = HASH_TYPE_INTEL64;
  } else if (!strcmp(arg, "city64")) {
    app.hashtype = HASH_TYPE_CITY64;
  } else if (!strcmp(arg, "murmur3")) {
    app.hashtype = HASH_TYPE_MURMUR3;
  } else {
    return -1;
  }
  return 0;
}

static int
parse_arg_fifoness(const char *arg) {
  if (!strcmp(arg, "none")) {
    app.fifoness = FIFONESS_NONE;
  } else if (!strcmp(arg, "port")) {
    app.fifoness = FIFONESS_PORT;
  } else if (!strcmp(arg, "flow")) {
    app.fifoness = FIFONESS_FLOW;
  } else {
    return -1;
  }
  return 0;
}

#ifndef APP_ARG_NUMERICAL_SIZE_CHARS
#define APP_ARG_NUMERICAL_SIZE_CHARS 15
#endif

static int
assign_rx_lcore(uint8_t portid, uint8_t n) {
  struct app_lcore_params *lp;

  app.nic_rx_queue_mask[portid][0] = NIC_RX_QUEUE_ENABLED;
  lp = &app.lcore_params[lcores[n]];
  if (lp->type == e_APP_LCORE_WORKER) {
    lagopus_msg_error("lcore %d (%dth lcore): already assinged as worker\n",
                      lcores[n], n);
    return -5;
  }
  lp->type = e_APP_LCORE_IO;
  lp->io.rx.nic_queues[lp->io.rx.n_nic_queues].port = portid;
  lp->io.rx.nic_queues[lp->io.rx.n_nic_queues].queue = 0;
  lp->io.rx.n_nic_queues++;
  return 0;
}

static int
assign_tx_lcore(uint8_t portid, uint8_t n) {
  struct app_lcore_params *lp;

  app.nic_tx_port_mask[portid] = 1;
  lp = &app.lcore_params[lcores[n]];
  if (lp->type == e_APP_LCORE_WORKER) {
    lagopus_msg_error("lcore %d (%dth lcore): already assinged as worker\n",
                      lcores[n], n);
    return -5;
  }
  lp->type = e_APP_LCORE_IO;
  lp->io.tx.nic_ports[lp->io.tx.n_nic_ports] = portid;
  lp->io.tx.n_nic_ports++;
  return 0;
}

/* Parse the argument given in the command line of the application */
int
app_parse_args(int argc, const char *argv[]) {
  int opt, ret;
  char **argvopt;
  int option_index;
  const char *prgname = argv[0];
  static const struct option lgopts[] = {
    {"rx", 1, 0, 0},
    {"tx", 1, 0, 0},
    {"w", 1, 0, 0},
    {"rsz", 1, 0, 0},
    {"bsz", 1, 0, 0},
    {"no-cache", 0, 0, 0},
    {"core-assign", 1, 0, 0},
    {"kvstype", 1, 0, 0},
#ifdef __SSE4_2__
    {"hashtype", 1, 0, 0},
#endif /* __SSE4_2__ */
    {"fifoness", 1, 0, 0},
    {"show-core-config", 0, 0, 0},
    {NULL, 0, 0, 0}
  };
  uint32_t arg_w = 0;
  uint32_t arg_rx = 0;
  uint32_t arg_tx = 0;
  uint32_t arg_rsz = 0;
  uint32_t arg_bsz = 0;

  char *end = NULL;
  struct app_lcore_params *lp, *htlp;
  unsigned lcore, htcore, i, wk_lcore_count = 0;
  unsigned rx_lcore_start, tx_lcore_start, wk_lcore_start;
  int rx_lcore_inc, tx_lcore_inc, wk_lcore_inc;
  uint8_t portid, port_count, count, n;
  bool show_core_assign = false;

  argvopt = (char **)argv;

  while ((opt = getopt_long(argc, argvopt, "p:w:",
                            lgopts, &option_index)) != EOF) {
    switch (opt) {
      case 'w':
        if (optarg[0] == '\0') {
          printf("Require value for -w argument\n");
          return -1;
        }
        wk_lcore_count = (unsigned)strtoul(optarg, &end, 10);
        if (end == NULL || *end != '\0') {
          printf("Non-numerical value for -w argument\n");
          return -1;
        }
        if (wk_lcore_count == 0) {
          printf("Incorrect value for -w argument\n");
          return -1;
        }
        break;
      case 'p':
        /* no longer used option.  ignored. */
        break;
        /* long options */
      case 0:
        if (!strcmp(lgopts[option_index].name, "rx")) {
          arg_rx = 1;
          ret = parse_arg_rx(optarg);
          if (ret) {
            printf("Incorrect value for --rx argument (%d)\n", ret);
            return -1;
          }
        }
        if (!strcmp(lgopts[option_index].name, "tx")) {
          arg_tx = 1;
          ret = parse_arg_tx(optarg);
          if (ret) {
            printf("Incorrect value for --tx argument (%d)\n", ret);
            return -1;
          }
        }
        if (!strcmp(lgopts[option_index].name, "w")) {
          arg_w = 1;
          ret = parse_arg_w(optarg);
          if (ret) {
            printf("Incorrect value for --w argument (%d)\n", ret);
            return -1;
          }
        }
        if (!strcmp(lgopts[option_index].name, "rsz")) {
          arg_rsz = 1;
          ret = parse_arg_rsz(optarg);
          if (ret) {
            printf("Incorrect value for --rsz argument (%d)\n", ret);
            return -1;
          }
        }
        if (!strcmp(lgopts[option_index].name, "bsz")) {
          arg_bsz = 1;
          ret = parse_arg_bsz(optarg);
          if (ret) {
            printf("Incorrect value for --bsz argument (%d)\n", ret);
            return -1;
          }
        }
        if (!strcmp(lgopts[option_index].name, "core-assign")) {
          ret = parse_arg_core_assign(optarg);
          if (ret) {
            printf("Incorrect value for --core-assign argument (%d)\n", ret);
            return -1;
          }
        }
        if (!strcmp(lgopts[option_index].name, "no-cache")) {
          app.no_cache = 1;
        }
        if (!strcmp(lgopts[option_index].name, "kvstype")) {
          ret = parse_arg_kvstype(optarg);
          if (ret) {
            printf("Incorrect value for --ksvtype argument (%d)\n", ret);
            return -1;
          }
        }
        if (!strcmp(lgopts[option_index].name, "hashtype")) {
          ret = parse_arg_hashtype(optarg);
          if (ret) {
            printf("Incorrect value for --hashtype argument (%d)\n", ret);
            return -1;
          }
        }
        if (!strcmp(lgopts[option_index].name, "fifoness")) {
          ret = parse_arg_fifoness(optarg);
          if (ret) {
            printf("Incorrect value for --fifoness argument (%d)\n", ret);
            return -1;
          }
        }
        if (!strcmp(lgopts[option_index].name, "show-core-config")) {
          show_core_assign = true;
        }
        break;

      default:
        printf("Incorrect option\n");
        return -1;
    }
  }

  /* Check that all mandatory arguments are provided */
  if ((arg_rx == 0 || arg_tx == 0 || arg_w == 0) &&
      arg_rx + arg_tx + arg_w != 0) {
    lagopus_exit_error(EXIT_FAILURE,
                       "Not all mandatory arguments are present\n");
  }
  if (arg_rx + arg_tx + arg_w == 0) {
    if (is_rawsocket_only_mode() == true) {
      goto out;
    }
    for (lcore_count = 0, lcore = 0; lcore < RTE_MAX_LCORE; lcore++) {
      if (lcore == rte_get_master_lcore()) {
        continue;
      }
      if (!rte_lcore_is_enabled(lcore)) {
        continue;
      }
      lp = &app.lcore_params[lcore];
      lp->socket_id = lcore_config[lcore].socket_id;
      lp->core_id = lcore_config[lcore].core_id;
      /* add lcore id except for hyper-threading core. */
      for (htcore = 0; htcore < lcore; htcore++) {
        if (!rte_lcore_is_enabled(htcore)) {
          continue;
        }
        htlp = &app.lcore_params[htcore];
        if (app.core_assign == CORE_ASSIGN_PERFORMANCE) {
          if (lp->socket_id == htlp->socket_id &&
              lp->core_id == htlp->core_id) {
            break;
          }
        }
      }
      if (htcore == lcore) {
        lcores[lcore_count++] = lcore;
      }
    }
    /* lcore_count is calculated. */
    if (lcore_count == 0) {
      lagopus_exit_error(
          EXIT_FAILURE,
          "Not enough active core "
          "(need at least 2 active core%s)\n",
          app.core_assign == CORE_ASSIGN_PERFORMANCE ?
          " except for HTT core" : "");
    }
    if (app.core_assign == CORE_ASSIGN_MINIMUM) {
      lcore_count = 1;
    }
    if (lcore_count == 1) {
        lp = &app.lcore_params[lcores[0]];
        lp->type = e_APP_LCORE_IO_WORKER;
    } else {
      for (lcore = 0; lcore < lcore_count / 2; lcore++) {
        lp = &app.lcore_params[lcores[lcore]];
        lp->type = e_APP_LCORE_IO;
      }
      for (; lcore < lcore_count; lcore++) {
        lp = &app.lcore_params[lcores[lcore]];
        lp->type = e_APP_LCORE_WORKER;
      }
    }
  } else {
    portid_specified = true;
  }

  /* Assign default values for the optional arguments not provided */
  if (arg_rsz == 0) {
    app.nic_rx_ring_size = APP_DEFAULT_NIC_RX_RING_SIZE;
    app.nic_tx_ring_size = APP_DEFAULT_NIC_TX_RING_SIZE;
    app.ring_rx_size = APP_DEFAULT_RING_RX_SIZE;
    app.ring_tx_size = APP_DEFAULT_RING_TX_SIZE;
  }

  if (arg_bsz == 0) {
    app.burst_size_io_rx_read = APP_DEFAULT_BURST_SIZE_IO_RX_READ;
    app.burst_size_io_rx_write = APP_DEFAULT_BURST_SIZE_IO_RX_WRITE;
    app.burst_size_io_tx_read = APP_DEFAULT_BURST_SIZE_IO_TX_READ;
    app.burst_size_io_tx_write = APP_DEFAULT_BURST_SIZE_IO_TX_WRITE;
    app.burst_size_worker_read = APP_DEFAULT_BURST_SIZE_WORKER_READ;
    app.burst_size_worker_write = APP_DEFAULT_BURST_SIZE_WORKER_WRITE;
  }

  /* Check cross-consistency of arguments */
  if (app_check_every_rx_port_is_tx_enabled() < 0) {
    lagopus_msg_error("At least one RX port is not enabled for TX.\n");
    return -2;
  }

  if (show_core_assign == true) {
    printf("core assign:\n");
    for (lcore = 0; lcore < RTE_MAX_LCORE; lcore++) {
      if (lcore_config[lcore].detected != true) {
        continue;
      }
      lp = &app.lcore_params[lcore];
      printf("  lcore %d:\n", lcore);
      if (lp->type == e_APP_LCORE_IO) {
        printf("    type: I/O\n");
      } else if (lp->type == e_APP_LCORE_WORKER) {
        printf("    type: WORKER\n");
      } else if (lp->type == e_APP_LCORE_IO_WORKER) {
        printf("    type: I/O WORKER\n");
      } else {
        printf("    type: not used\n");
      }
      for (n = 0; n < lp->io.rx.n_nic_queues; n++) {
        printf("    RX port %d (queue %d)\n",
               lp->io.rx.nic_queues[n].port,
               lp->io.rx.nic_queues[n].queue);
      }
      for (n = 0; n < lp->io.tx.n_nic_ports; n++) {
        printf("    TX port %d\n",
               lp->io.tx.nic_ports[n]);
      }
    }
    exit(0);
  }
out:
  if (optind >= 0) {
    argv[optind - 1] = prgname;
  }
  ret = optind - 1;
  optind = 0; /* reset getopt lib */
  return ret;
}

uint32_t
app_get_nic_rx_queues_per_port(uint8_t port) {
  uint32_t i, count;

  if (port >= APP_MAX_NIC_PORTS) {
    return 0;
  }
  count = 0;
  for (i = 0; i < APP_MAX_RX_QUEUES_PER_NIC_PORT; i ++) {
    if (app.nic_rx_queue_mask[port][i] == NIC_RX_QUEUE_ENABLED) {
      count ++;
    }
  }
  return count;
}

int
app_get_lcore_for_nic_rx(uint8_t port, uint8_t queue, uint32_t *lcore_out) {
  uint32_t lcore;

  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    struct app_lcore_params_io *lp = &app.lcore_params[lcore].io;
    uint32_t i;

    if (app.lcore_params[lcore].type != e_APP_LCORE_IO &&
        app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER) {
      continue;
    }
    for (i = 0; i < lp->rx.n_nic_queues; i ++) {
      if ((lp->rx.nic_queues[i].port == port) &&
          (lp->rx.nic_queues[i].queue == queue)) {
        *lcore_out = lcore;
        return 0;
      }
    }
  }
  return -1;
}

int
app_get_lcore_for_nic_tx(uint8_t port, uint32_t *lcore_out) {
  uint32_t lcore;

  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    struct app_lcore_params_io *lp = &app.lcore_params[lcore].io;
    uint32_t i;

    if (app.lcore_params[lcore].type != e_APP_LCORE_IO &&
        app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER) {
      continue;
    }
    for (i = 0; i < lp->tx.n_nic_ports; i ++) {
      if (lp->tx.nic_ports[i] == port) {
        *lcore_out = lcore;
        return 0;
      }
    }
  }
  return -1;
}

int
app_is_socket_used(uint32_t socket) {
  uint32_t lcore;

  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    if (app.lcore_params[lcore].type == e_APP_LCORE_DISABLED) {
      continue;
    }
    if (socket == rte_lcore_to_socket_id(lcore)) {
      return 1;
    }
  }
  return 0;
}

uint32_t
app_get_lcores_io_rx(void) {
  uint32_t lcore, count;

  count = 0;
  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    struct app_lcore_params_io *lp_io = &app.lcore_params[lcore].io;

    if (app.lcore_params[lcore].type != e_APP_LCORE_IO &&
        app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER) {
      continue;
    }
    count++;
  }
  return count;
}

uint32_t
app_get_lcores_worker(void) {
  uint32_t lcore, count;

  count = 0;
  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    if (app.lcore_params[lcore].type != e_APP_LCORE_WORKER &&
        app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER) {
      continue;
    }
    count++;
  }
  if (count > APP_MAX_WORKER_LCORES) {
    rte_panic("Algorithmic error (too many worker lcores)\n");
    return 0;
  }
  return count;
}

void
app_print_params(void) {
  unsigned port, queue, lcore, i, j;

  /* Print NIC RX configuration */
  printf("NIC RX ports:\n");
  for (port = 0; port < APP_MAX_NIC_PORTS; port ++) {
    uint32_t n_rx_queues = app_get_nic_rx_queues_per_port((uint8_t) port);

    if (n_rx_queues == 0) {
      continue;
    }

    printf(  "  port %u (", port);
    for (queue = 0; queue < APP_MAX_RX_QUEUES_PER_NIC_PORT; queue ++) {
      if (app.nic_rx_queue_mask[port][queue] == NIC_RX_QUEUE_ENABLED) {
        printf("queue %u", queue);
      }
    }
    printf(")\n");
  }
  printf("\n");

  /* Print I/O lcore RX params */
  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    struct app_lcore_params_io *lp = &app.lcore_params[lcore].io;

    if ((app.lcore_params[lcore].type != e_APP_LCORE_IO &&
         app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER)) {
      continue;
    }

    printf("I/O lcore %u (socket %u):\n",
           lcore, rte_lcore_to_socket_id(lcore));

    printf(" RX ports:\n");
    for (i = 0; i < lp->rx.n_nic_queues; i ++) {
      printf("  port %u (queue %u)\n",
             (unsigned) lp->rx.nic_queues[i].port,
             (unsigned) lp->rx.nic_queues[i].queue);
    }

    printf(" Output rings:\n");
    for (i = 0; i < lp->rx.n_rings; i ++) {
      printf("  %p\n", lp->rx.rings[i]);
    }
  }
  printf("\n");

  /* Print worker lcore RX and TX params */
  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    struct app_lcore_params_worker *lp = &app.lcore_params[lcore].worker;

    if (app.lcore_params[lcore].type != e_APP_LCORE_WORKER &&
        app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER) {
      continue;
    }

    printf("Worker %u: lcore %u (socket %u):\n",
           (unsigned)lp->worker_id,
           lcore,
           rte_lcore_to_socket_id(lcore));

    printf(" Input rings:\n");
    for (i = 0; i < lp->n_rings_in; i ++) {
      printf("  %p\n", lp->rings_in[i]);
    }
    printf(" Output rings per TX port\n");
    for (port = 0; port < APP_MAX_NIC_PORTS; port ++) {
      if (lp->rings_out[port] != NULL) {
        printf("  port %u (%p)\n", port, lp->rings_out[port]);
      }
    }
  }
  printf("\n");

  /* Print NIC TX configuration */
  printf("NIC TX ports:\n");
  for (port = 0; port < APP_MAX_NIC_PORTS; port ++) {
    if (app.nic_tx_port_mask[port] == 1) {
      printf("  %u", port);
    }
  }
  printf("\n");
  printf("\n");

  /* Print I/O TX lcore params */
  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    struct app_lcore_params_io *lp = &app.lcore_params[lcore].io;
    uint32_t n_workers = app_get_lcores_worker();

    if ((app.lcore_params[lcore].type != e_APP_LCORE_IO &&
         app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER) ||
        (lp->tx.n_nic_ports == 0)) {
      continue;
    }

    printf("I/O lcore %u (socket %u):\n",
           lcore, rte_lcore_to_socket_id(lcore));

    printf(" Input rings per TX port\n");
    for (i = 0; i < lp->tx.n_nic_ports; i ++) {
      port = lp->tx.nic_ports[i];

      printf(" port %u\n", port);
      for (j = 0; j < n_workers; j ++) {
        printf("  worker %u, %p\n", j, lp->tx.rings[port][j]);
      }
    }
  }
  printf("\n");

  /* Rings */
  printf("Ring sizes:\n");
  printf("  NIC RX     = %u\n", (unsigned)app.nic_rx_ring_size);
  printf("  Worker in  = %u\n", (unsigned) app.ring_rx_size);
  printf("  Worker out = %u\n", (unsigned) app.ring_tx_size);
  printf("  NIC TX     = %u\n", (unsigned) app.nic_tx_ring_size);

  /* Bursts */
  printf("Burst sizes:\n");
  printf("  I/O RX (rd = %u, wr = %u)\n",
         (unsigned) app.burst_size_io_rx_read,
         (unsigned) app.burst_size_io_rx_write);
  printf("  Worker (rd = %u, wr = %u)\n",
         (unsigned) app.burst_size_worker_read,
         (unsigned) app.burst_size_worker_write);
  printf("  I/O TX (rd = %u, wr = %u)\n",
         (unsigned) app.burst_size_io_tx_read,
         (unsigned) app.burst_size_io_tx_write);
  printf("\n");
}
