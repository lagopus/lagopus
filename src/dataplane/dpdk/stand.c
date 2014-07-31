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
 *	@file	stand.c
 *	@brief	for Standalone test use with DPDK
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
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_tailq.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ethdev.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_kni.h>

#include "lagopus/flowdb.h"
#include "lagopus/meter.h"
#include "lagopus/dpmgr.h"
#include "lagopus/port.h"
#include "lagopus_gstate.h"

#include "lagopus/dataplane.h"
#include "pktbuf.h"
#include "packet.h"

/* Print out statistics on packets dropped */
static void
print_stats(void) {
  uint64_t total_packets_dropped, total_packets_tx, total_packets_rx;
  unsigned portid;

  total_packets_dropped = 0;
  total_packets_tx = 0;
  total_packets_rx = 0;

  const char clr[] = { 27, '[', '2', 'J', '\0' };
  const char topLeft[] = { 27, '[', '1', ';', '1', 'H','\0' };

  /* Clear screen and move to top left */
  printf("%s%s", clr, topLeft);

  printf("\nPort statistics ====================================");

  for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++) {
    /* skip disabled ports */
    if (lagopus_is_portid_enabled(portid) != true) {
      continue;
    }
    printf("\nStatistics for port %u ------------------------------"
           "\nPackets sent: %24"PRIu64
           "\nPackets received: %20"PRIu64
           "\nPackets dropped: %21"PRIu64,
           portid,
           port_statistics[portid].tx,
           port_statistics[portid].rx,
           port_statistics[portid].dropped);

    total_packets_dropped += port_statistics[portid].dropped;
    total_packets_tx += port_statistics[portid].tx;
    total_packets_rx += port_statistics[portid].rx;
  }
  printf("\nAggregate statistics ==============================="
         "\nTotal packets sent: %18"PRIu64
         "\nTotal packets received: %14"PRIu64
         "\nTotal packets dropped: %15"PRIu64,
         total_packets_tx,
         total_packets_rx,
         total_packets_dropped);
  printf("\n====================================================\n");
}

int
MAIN(int argc, char *argv[]) {
  struct datapath_arg dparg;
  struct dpmgr *dpmgr;
  lagopus_thread_t *datapath;
  struct bridge *bridge;
  int portid;
  void *rv;
  int ret;

  dpmgr = dpmgr_alloc();
  if (dpmgr == NULL) {
    rte_exit(EXIT_FAILURE, "failed to initialize datapath manager.\n");
  }

  /* Create default bridge. */
  ret = dpmgr_bridge_add(dpmgr, "br0", 0);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "Adding br0 failed\n");
  }

  /* register flow entries */
  bridge = dpmgr_bridge_lookup(dpmgr, "br0");
  register_flow(bridge->flowdb, 100000);
  flowdb_switch_mode_set(bridge->flowdb, SWITCH_MODE_OPENFLOW);

  /* Start datapath. */
  dparg.dpmgr = dpmgr;
  dparg.argc = argc;
  dparg.argv = argv;
  datapath_initialize(&dparg, &datapath);

  /* register all physical ports */
  for (portid = 0; portid < nb_ports; portid++) {
    if (lagopus_is_portid_enabled(portid) == true) {
      struct port nport;

      /* New port API. */
      nport.ofp_port.port_no = (uint32_t)portid + 1;
      nport.ifindex = (uint32_t)portid;
      printf("port id %u\n", nport.ofp_port.port_no);

      snprintf(nport.ofp_port.name , sizeof(nport.ofp_port.name),
               "eth%d", portid);
      dpmgr_port_add(dpmgr, &nport);
    }
  }

  /* assign all ports to br0 */
  for (portid = 0; portid < nb_ports; portid++) {
    dpmgr_bridge_port_add(dpmgr, "br0", (uint32_t)portid + 1);
  }
  datapath_start();

  while (lagopus_thread_wait(datapath, 1000000) == LAGOPUS_RESULT_TIMEDOUT) {
    sleep(1);
  }

  return 0;
}
