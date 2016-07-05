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
 *      @file   worker.c
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
#include <rte_string_fns.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_sctp.h>

#include "lagopus/ethertype.h"
#include "lagopus/flowdb.h"
#include "lagopus/meter.h"
#include "lagopus/port.h"
#include "lagopus/interface.h"
#include "lagopus_gstate.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/ofcache.h"

#include "lagopus/dataplane.h"
#include "lagopus/dp_apis.h"
#include "pktbuf.h"
#include "packet.h"
#include "csum.h"
#include "lock.h"
#include "dpdk/dpdk.h"

#ifndef APP_LCORE_WORKER_FLUSH
#define APP_LCORE_WORKER_FLUSH       1000
#endif

#define APP_WORKER_DROP_ALL_PACKETS  0

#ifndef APP_WORKER_PREFETCH_ENABLE
#define APP_WORKER_PREFETCH_ENABLE   1
#endif

#if APP_WORKER_PREFETCH_ENABLE
#define APP_WORKER_PREFETCH0(p)      rte_prefetch0(p)
#define APP_WORKER_PREFETCH1(p)      rte_prefetch1(p)
#else
#define APP_WORKER_PREFETCH0(p)
#define APP_WORKER_PREFETCH1(p)
#endif

#define IS_TX_OFFLOAD_ENABLE(ifp) false /* preliminary */

struct worker_arg {
  struct lagopus_packet *pkt;
};

void
dp_bulk_match_and_action(OS_MBUF *mbufs[], size_t n_mbufs,
                         struct flowcache *cache) {
  struct interface *ifp;
  struct lagopus_packet *pkt;
  enum switch_mode mode;
  size_t i;

  APP_WORKER_PREFETCH1(rte_pktmbuf_mtod(mbufs[0], unsigned char *));
  APP_WORKER_PREFETCH0(mbufs[1]);
  flowdb_rdlock(NULL);

  for (i = 0; i < n_mbufs; i++) {
    OS_MBUF *m;

    if (likely(i < n_mbufs - 1)) {
      APP_WORKER_PREFETCH1(rte_pktmbuf_mtod(mbufs[i + 1],
                                            unsigned char *));
      }
      if (likely(i < n_mbufs - 2)) {
        APP_WORKER_PREFETCH0(mbufs[i + 2]);
      }
      m = mbufs[i];
      pkt = MBUF2PKT(m);
#ifdef RTE_MBUF_HAS_PKT
      ifp = dpdk_interface_lookup(m->pkt.in_port);
#else
      ifp = dpdk_interface_lookup(m->port);
#endif /* RTE_MBUF_HAS_PKT */
      if (ifp == NULL ||
          ifp->port == NULL ||
          ifp->port->bridge == NULL ||
          (ifp->port->ofp_port.config & OFPPC_NO_RECV) != 0) {
        /* drop m */
        for (; i < n_mbufs; i++) {
          rte_pktmbuf_free(mbufs[i]);
        }
        n_mbufs = 0;
        break;
      }
      /*
       * If OpenFlow connection is lost, switch mode is changed.
       * if "fail standalone mode", all packets are send to
       * OFPP_NORMAL.
       */
      lagopus_packet_init(pkt, m, ifp->port);

#ifdef HYBRID
#if 0 /* temporary disable tcpdump.*/
      /* count up refcnt(packet copy). */
      OS_M_ADDREF(m);

      /* send to tap */
      dp_interface_send_packet_kernel(pkt, ifp);
#endif
#ifdef PIPELINER
      if (unlikely(i == n_mbufs - 1)) {
        pkt->pipeline_context.is_last_packet_of_bulk = true;
      } else {
        pkt->pipeline_context.is_last_packet_of_bulk = false;
      }
#endif /* PIPELINER */
#endif /* HYBRID */

      flowdb_switch_mode_get(ifp->port->bridge->flowdb, &mode);
      if (mode == SWITCH_MODE_STANDALONE) {
        lagopus_forward_packet_to_port(pkt, OFPP_NORMAL);
        mbufs[i] = NULL;
        continue;
      }
    }
    for (i = 0; i < n_mbufs; i++) {
      OS_MBUF *m;

      m = mbufs[i];
      if (unlikely(m == NULL)) {
        continue;
      }
      if (likely(i < n_mbufs - 1)) {
        APP_WORKER_PREFETCH1(rte_pktmbuf_mtod(mbufs[i + 1],
                                              unsigned char *));
      }
      if (likely(i < n_mbufs - 2)) {
        APP_WORKER_PREFETCH0(mbufs[i + 2]);
      }
      pkt = MBUF2PKT(m);
      APP_WORKER_PREFETCH0(pkt);
      pkt->cache = cache;
      lagopus_match_and_action(pkt);
    }
    flowdb_rdunlock(NULL);
}

static inline void
app_lcore_worker(struct app_lcore_params_worker *lp,
                 uint32_t bsz_rd,
                 struct worker_arg *arg) {
  static const uint8_t eth_bcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  uint32_t i;


  for (i = 0; i < lp->n_rings_in; i ++) {
    struct rte_ring *ring_in = lp->rings_in[i];
    int ret, j;

    ret = rte_ring_sc_dequeue_burst(ring_in,
                                    (void **) lp->mbuf_in.array,
                                    bsz_rd);
    if (unlikely(ret == 0)) {
#if defined HYBRID && defined PIPELINER
        pipeline_process_stacked_packets();
#endif /* HYBRID && PIPELINER */
      continue;
    }
    dp_bulk_match_and_action(lp->mbuf_in.array, ret, lp->cache);
  }
}

/**
 * Queueing output packet if pending packet is available in the array.
 */
static inline void
app_lcore_worker_flush(struct app_lcore_params_worker *lp) {
  uint32_t portid;
  uint32_t n;
  int ret;

  for (portid = 0; portid < APP_MAX_NIC_PORTS; portid ++) {

    if (unlikely(lp->rings_out[portid] == NULL)) {
      continue;
    }

    if (likely((lp->mbuf_out_flush[portid] == 0) ||
               (lp->mbuf_out[portid].n_mbufs == 0))) {
      continue;
    }

    n = lp->mbuf_out[portid].n_mbufs;
    ret = rte_ring_sp_enqueue_bulk(lp->rings_out[portid],
                                   (void **) lp->mbuf_out[portid].array,
                                   n);
    if (unlikely(ret == -ENOBUFS)) {
      uint32_t k;
      for (k = 0; k < n; k ++) {
        struct rte_mbuf *pkt_to_free = lp->mbuf_out[portid].array[k];
        rte_pktmbuf_free(pkt_to_free);
      }
    }
    lp->mbuf_out[portid].n_mbufs = 0;
    lp->mbuf_out_flush[portid] = 0;
  }
}

void
app_lcore_main_loop_worker(void *arg) {
  uint32_t lcore = rte_lcore_id();
  struct app_lcore_params_worker *lp = &app.lcore_params[lcore].worker;
  uint32_t bsz_rd = app.burst_size_worker_read;
  struct worker_arg warg;
  uint64_t i;

  if (!app.no_cache) {
    lp->cache = init_flowcache(app.kvs_type);
  }
  i = 0;
  warg.pkt = NULL;
  for (;;) {
    if (APP_LCORE_WORKER_FLUSH &&
        (unlikely(i == APP_LCORE_WORKER_FLUSH))) {
      if (rte_atomic32_read(&dpdk_stop) != 0) {
        break;
      }
      flowdb_check_update(NULL);
      app_lcore_worker_flush(lp);
      i = 0;
    }
    app_lcore_worker(lp, bsz_rd, &warg);
    i++;
  }
}

/*
 * mixed I/O and worker loop
 */
void
app_lcore_main_loop_io_worker(void *arg) {
  uint32_t lcore = rte_lcore_id();
  struct app_lcore_params_io *lp_io = &app.lcore_params[lcore].io;
  struct app_lcore_params_worker *lp = &app.lcore_params[lcore].worker;
  uint32_t n_workers = app_get_lcores_worker();
  uint32_t bsz_rd = app.burst_size_worker_read;
  struct worker_arg warg;
  uint64_t i;

  if (!app.no_cache) {
    lp->cache = init_flowcache(app.kvs_type);
  }
  i = 0;
  warg.pkt = NULL;
  for (;;) {
    if (APP_LCORE_WORKER_FLUSH &&
        (unlikely(i == APP_LCORE_WORKER_FLUSH))) {
      if (rte_atomic32_read(&dpdk_stop) != 0) {
        break;
      }
      app_lcore_io_flush(lp_io, n_workers, arg);
      flowdb_check_update(NULL);
      app_lcore_worker_flush(lp);
      i = 0;
    }
    app_lcore_io(lp_io, n_workers);
    app_lcore_worker(lp, bsz_rd, &warg);
    i++;
  }
}

void
clear_worker_flowcache(bool wait_flush) {
  uint32_t lcore;

  if (app.no_cache) {
    return;
  }
  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    struct app_lcore_params_worker *lp;

    if (app.lcore_params[lcore].type != e_APP_LCORE_WORKER &&
        app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER) {
      continue;
    }
    lp = &app.lcore_params[lcore].worker;
    clear_all_cache(lp->cache);
  }
}

void
dp_get_flowcache_statistics(struct bridge *bridge, struct ofcachestat *st) {
  uint32_t lcore;

  (void) bridge;

  st->nentries = 0;
  st->hit = 0;
  st->miss = 0;
  if (app.no_cache) {
    return;
  }
  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    struct app_lcore_params_worker *lp;
    struct ofcachestat s;

    if (app.lcore_params[lcore].type != e_APP_LCORE_WORKER &&
        app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER) {
      continue;
    }
    lp = &app.lcore_params[lcore].worker;
    get_flowcache_statistics(lp->cache, &s);
    st->nentries += s.nentries;
    st->hit += s.hit;
    st->miss += s.miss;
  }
}

void
dpdk_assign_worker_ids(void) {
  uint32_t lcore, worker_id;

  /* Assign ID for each worker */
  worker_id = 0;
  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    struct app_lcore_params_worker *lp_worker = &app.lcore_params[lcore].worker;

    if (app.lcore_params[lcore].type != e_APP_LCORE_WORKER &&
        app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER) {
      continue;
    }

    lp_worker->worker_id = worker_id;
    worker_id ++;
  }
}

/**
 * Send the packet on an output interface.
 * NOTE: Intel DPDK supports only physical port of the NIC.
 */
int
dpdk_send_packet_physical(struct lagopus_packet *pkt, struct interface *ifp) {
  struct app_lcore_params_worker *lp;
  unsigned lcore;
  struct rte_mbuf *m;
  uint32_t bsz_wr = app.burst_size_worker_write;
  uint32_t pos, plen;
  uint8_t portid;
  int ret;

  portid = ifp->info.eth.port_number;
  lcore = rte_lcore_id();
  if (unlikely(lcore == 0 || lcore == UINT_MAX)) {
    /**
     * so far, packet-out action is running on core 0 (comm thread.)
     * but comm thread not as worker, it does not have tx rings.
     * workaround: Use first worker core rings (preliminary.)
     */
    lcore = 0;
    while (app.lcore_params[lcore].type != e_APP_LCORE_WORKER &&
           app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER) {
      lcore++;
    }
  }
  lp = &app.lcore_params[lcore].worker;

  m = PKT2MBUF(pkt);
  plen = OS_M_PKTLEN(m);
  if (plen < 60) {
    memset(OS_M_APPEND(m, 60 - plen), 0, (uint32_t)(60 - plen));
  }
  if ((pkt->flags & PKT_FLAG_RECALC_CKSUM_MASK) != 0) {
    if (IS_TX_OFFLOAD_ENABLE(ifp) == false) {
      if (pkt->ether_type == ETHERTYPE_IP) {
        lagopus_update_ipv4_checksum(pkt);
      } else if (pkt->ether_type == ETHERTYPE_IPV6) {
        lagopus_update_ipv6_checksum(pkt);
      }
    } else {
      if (pkt->vlan != NULL) {
        m->ol_flags |= PKT_TX_VLAN_PKT;
      }
      if (pkt->ether_type == ETHERTYPE_IP) {
        pkt->ipv4->ip_sum = 0;
        m->ol_flags |= PKT_TX_IP_CKSUM | PKT_TX_IPV4;
        switch (IPV4_PROTO(pkt->ipv4)) {
          case IPPROTO_TCP:
            m->ol_flags |= PKT_TX_TCP_CKSUM;
            pkt->tcp[8] = rte_ipv4_phdr_cksum(pkt->ipv4, m->ol_flags);
            break;
          case IPPROTO_UDP:
            m->ol_flags |= PKT_TX_UDP_CKSUM;
            pkt->udp[3] = rte_ipv4_phdr_cksum(pkt->ipv4, m->ol_flags);
            break;
          case IPPROTO_SCTP:
            m->ol_flags |= PKT_TX_SCTP_CKSUM;
            break;
          case IPPROTO_ICMP:
            lagopus_update_icmp_checksum(pkt);
          default:
            break;
        }
        plen = OS_M_PKTLEN(m);
      } else if (pkt->ether_type == ETHERTYPE_IPV6) {
        m->ol_flags |= PKT_TX_IP_CKSUM | PKT_TX_IPV6;
        switch (IPV4_PROTO(pkt->ipv4)) {
          case IPPROTO_TCP:
            m->ol_flags |= PKT_TX_TCP_CKSUM;
            pkt->tcp[8] = rte_ipv6_phdr_cksum(pkt->ipv4, m->ol_flags);
            break;
          case IPPROTO_UDP:
            m->ol_flags |= PKT_TX_UDP_CKSUM;
            pkt->udp[3] = rte_ipv6_phdr_cksum(pkt->ipv4, m->ol_flags);
            break;
          case IPPROTO_SCTP:
            m->ol_flags |= PKT_TX_SCTP_CKSUM;
            break;
          case IPPROTO_ICMPV6:
            lagopus_update_icmpv6_checksum(pkt);
            break;
          default:
            m->l4_len = 0;
            break;
        }
      }
      m->l3_len = pkt->base[L4_BASE] - pkt->base[L3_BASE];
      m->l2_len = pkt->base[L3_BASE] - pkt->base[ETH_BASE];
    }
  }

  pos = lp->mbuf_out[portid].n_mbufs;
  lp->mbuf_out[portid].array[pos++] = m;

  if (likely(pos < bsz_wr)) {
    lp->mbuf_out[portid].n_mbufs = pos;
    lp->mbuf_out_flush[portid] = 1;
    return 0;
  }

  ret = rte_ring_sp_enqueue_bulk(
          lp->rings_out[portid],
          (void **) lp->mbuf_out[portid].array,
          bsz_wr);

  if (unlikely(ret == -ENOBUFS)) {
    uint32_t k;
    for (k = 0; k < bsz_wr; k ++) {
      struct rte_mbuf *pkt_to_free = lp->mbuf_out[portid].array[k];
      rte_pktmbuf_free(pkt_to_free);
    }
  }
  lp->mbuf_out[portid].n_mbufs = 0;
  lp->mbuf_out_flush[portid] = 0;

  return 0;
}

#ifdef HYBRID
uint32_t
dpdk_get_worker_id(void) {
  uint32_t lcore;
  struct app_lcore_params_worker *lp;
  lcore = rte_lcore_id();
  lp = &app.lcore_params[lcore].worker;
  return lp->worker_id;
}
#endif /* HYBRID */
