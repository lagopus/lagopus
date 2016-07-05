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
 *      @file   dpdk_io.c
 *      @brief  Datapath I/O driver use with Intel DPDK
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
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

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
#include <rte_pci.h>
#ifdef __SSE4_2__
#include <rte_hash_crc.h>
#else
#include "City.h"
#endif /* __SSE4_2__ */
#include <rte_string_fns.h>

#include "lagopus/ethertype.h"
#include "lagopus/flowdb.h"
#include "lagopus/meter.h"
#include "lagopus/port.h"
#include "lagopus_gstate.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/dp_apis.h"

#include "lagopus/datastore/interface.h"
#include "lagopus/interface.h"

#include "lagopus/dataplane.h"
#include "dp_timer.h"
#include "pktbuf.h"
#include "packet.h"
#include "dpdk/dpdk.h"

#undef IO_DEBUG
#ifdef IO_DEBUG
#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...)
#endif

#ifndef APP_LCORE_IO_FLUSH
#define APP_LCORE_IO_FLUSH           100
#endif

#define DP_UPDATE_COUNT		     (200 * 10000)

#define APP_IO_RX_DROP_ALL_PACKETS   0
#define APP_IO_TX_DROP_ALL_PACKETS   0

#ifndef APP_IO_RX_PREFETCH_ENABLE
#define APP_IO_RX_PREFETCH_ENABLE    1
#endif

#ifndef APP_IO_TX_PREFETCH_ENABLE
#define APP_IO_TX_PREFETCH_ENABLE    1
#endif

#if APP_IO_RX_PREFETCH_ENABLE
#define APP_IO_RX_PREFETCH0(p)       rte_prefetch0(p)
#define APP_IO_RX_PREFETCH1(p)       rte_prefetch1(p)
#else
#define APP_IO_RX_PREFETCH0(p)
#define APP_IO_RX_PREFETCH1(p)
#endif

#if APP_IO_TX_PREFETCH_ENABLE
#define APP_IO_TX_PREFETCH0(p)       rte_prefetch0(p)
#define APP_IO_TX_PREFETCH1(p)       rte_prefetch1(p)
#else
#define APP_IO_TX_PREFETCH0(p)
#define APP_IO_TX_PREFETCH1(p)
#endif

/* optimized tx write threshold for igb only. */
#define APP_IGB_NIC_TX_WTHRESH  16

/* ethernet addresses of ports */
static struct ether_addr lagopus_ports_eth_addr[RTE_MAX_ETHPORTS];

/* mask of enabled ports */
static unsigned long lagopus_port_mask = 0;

static struct rte_eth_conf port_conf = {
  .rxmode = {
    .split_hdr_size = 0,
    .header_split   = 0, /**< Header Split disabled */
    .hw_ip_checksum = 0, /**< IP checksum offload enabled */
    .hw_vlan_filter = 0, /**< VLAN filtering disabled */
    .hw_vlan_strip  = 0, /**< VLAN strip disabled */
    .hw_vlan_extend = 0, /**< Extended VLAN disabled */
#if MAX_PACKET_SZ > 2048
    .jumbo_frame    = 1, /**< Jumbo Frame Support enabled */
    .max_rx_pkt_len = 9000, /**< Max RX packet length */
#else
    .jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
#endif /* MAX_PACKET_SZ */
    .hw_strip_crc   = 0, /**< CRC stripped by hardware */
  },
  .rx_adv_conf = {
    .rss_conf = {
      .rss_key = NULL,
      .rss_hf = ETH_RSS_IPV4 | ETH_RSS_IPV6,
    },
  },
  .txmode = {
    .mq_mode = ETH_MQ_TX_NONE,
  },
  .intr_conf = {
    .lsc = 1,
  },
};

#if !defined(RTE_VERSION_NUM) || RTE_VERSION < RTE_VERSION_NUM(1, 8, 0, 0)
static const struct rte_eth_rxconf rx_conf = {
  .rx_thresh = {
    .pthresh = APP_DEFAULT_NIC_RX_PTHRESH,
    .hthresh = APP_DEFAULT_NIC_RX_HTHRESH,
    .wthresh = APP_DEFAULT_NIC_RX_WTHRESH,
  },
  .rx_free_thresh = APP_DEFAULT_NIC_RX_FREE_THRESH,
  .rx_drop_en = APP_DEFAULT_NIC_RX_DROP_EN,
};

static struct rte_eth_txconf tx_conf = {
  .tx_thresh = {
    .pthresh = APP_DEFAULT_NIC_TX_PTHRESH,
    .hthresh = APP_DEFAULT_NIC_TX_HTHRESH,
    .wthresh = APP_DEFAULT_NIC_TX_WTHRESH,
  },
  .tx_free_thresh = APP_DEFAULT_NIC_TX_FREE_THRESH,
  .tx_rs_thresh = APP_DEFAULT_NIC_TX_RS_THRESH,
  .txq_flags = ETH_TXQ_FLAGS_NOMULTSEGS & ETH_TXQ_FLAGS_NOOFFLOADS,
};
#endif /* !RTE_VESRION_NUM */

/* Per-port statistics struct */
struct lagopus_port_statistics {
  uint64_t tx;
  uint64_t rx;
  uint64_t dropped;
} __rte_cache_aligned;
struct lagopus_port_statistics port_statistics[RTE_MAX_ETHPORTS];

static struct port_stats *dpdk_port_stats(struct port *port);

struct interface *ifp_table[UINT8_MAX + 1];

static void
dpdk_interface_set_index(struct interface *ifp) {
  ifp_table[ifp->info.eth_dpdk_phy.port_number] = ifp;
}

static void
dpdk_interface_unset_index(struct interface *ifp) {
  ifp_table[ifp->info.eth_dpdk_phy.port_number] = NULL;
}

static inline int
dpdk_interface_queue_id_to_index(struct interface *ifp, uint32_t queue_id) {
  int i;

  for (i = 0; i < ifp->ifqueue.nqueue; i++) {
    if (ifp->ifqueue.queues[i]->id == queue_id) {
      return i;
    }
  }
  return 0;
}

/**
 * Put mbuf (bsz packets) into worker queue.
 * The function is called from I/O (Input) thread.
 */
static inline void
app_lcore_io_rx_buffer_to_send (
  struct app_lcore_params_io *lp,
  uint32_t worker,
  struct rte_mbuf *mbuf,
  uint32_t bsz) {
  uint32_t pos;
  int ret;

  pos = lp->rx.mbuf_out[worker].n_mbufs;
  lp->rx.mbuf_out[worker].array[pos ++] = mbuf;
  if (likely(pos < bsz)) {
    lp->rx.mbuf_out[worker].n_mbufs = pos;
    lp->rx.mbuf_out_flush[worker] = 1;
    return;
  }

  lp->rx.mbuf_out_flush[worker] = 0;
  ret = rte_ring_sp_enqueue_burst(lp->rx.rings[worker],
                                  (void **) lp->rx.mbuf_out[worker].array,
                                  bsz);

  if (unlikely(ret < (int)bsz)) {
    uint32_t k;
    for (k = (uint32_t)ret; k < bsz; k++) {
      struct rte_mbuf *m = lp->rx.mbuf_out[worker].array[k];
      rte_pktmbuf_free(m);
    }
  }

  lp->rx.mbuf_out[worker].n_mbufs = 0;

#if APP_STATS
  lp->rx.rings_iters[worker] += bsz;
  lp->rx.rings_count[worker] += ret;
  if (unlikely(lp->rx.rings_iters[worker] == APP_STATS)) {
    unsigned lcore = rte_lcore_id();

    printf("\tI/O RX %u out (worker %u): enq success rate = %.2f\n",
           lcore,
           (unsigned)worker,
           ((double) lp->rx.rings_count[worker]) / ((double)
               lp->rx.rings_iters[worker]));
    lp->rx.rings_iters[worker] = 0;
    lp->rx.rings_count[worker] = 0;
  }
#endif
}

/**
 * Receive packet from ethernet driver and queueing into worker queue.
 * This function is called from I/O (Input) thread.
 */
static inline void
app_lcore_io_rx(
  struct app_lcore_params_io *lp,
  uint32_t n_workers,
  uint32_t bsz_rd,
  uint32_t bsz_wr) {
  struct rte_mbuf *mbuf_1_0, *mbuf_1_1, *mbuf_2_0, *mbuf_2_1;
  uint32_t i, fifoness;

  fifoness = app.fifoness;
  for (i = 0; i < lp->rx.n_nic_queues; i++) {
    uint8_t portid = lp->rx.nic_queues[i].port;
    uint8_t queue = lp->rx.nic_queues[i].queue;
    uint32_t n_mbufs, j;

    if (unlikely(lp->rx.nic_queues[i].enabled != true)) {
      continue;
    }
    n_mbufs = rte_eth_rx_burst(portid,
                               queue,
                               lp->rx.mbuf_in.array,
                               (uint16_t) bsz_rd);
    if (unlikely(n_mbufs == 0)) {
      continue;
    }

#if APP_STATS
    lp->rx.nic_queues_iters[i] ++;
    lp->rx.nic_queues_count[i] += n_mbufs;
    if (unlikely(lp->rx.nic_queues_iters[i] == APP_STATS)) {
      struct rte_eth_stats stats;
      unsigned lcore = rte_lcore_id();

      rte_eth_stats_get(portid, &stats);

      printf("I/O RX %u in (NIC port %u): NIC drop ratio = %.2f avg burst size = %.2f\n",
             lcore,
             (unsigned) portid,
             (double) stats.ierrors / (double) (stats.ierrors + stats.ipackets),
             ((double) lp->rx.nic_queues_count[i]) / ((double)
                 lp->rx.nic_queues_iters[i]));
      lp->rx.nic_queues_iters[i] = 0;
      lp->rx.nic_queues_count[i] = 0;
    }
#endif

#if APP_IO_RX_DROP_ALL_PACKETS
    for (j = 0; j < n_mbufs; j ++) {
      struct rte_mbuf *pkt = lp->rx.mbuf_in.array[j];
      rte_pktmbuf_free(pkt);
    }
    continue;
#endif

    mbuf_1_0 = lp->rx.mbuf_in.array[0];
    mbuf_1_1 = lp->rx.mbuf_in.array[1];
    mbuf_2_0 = lp->rx.mbuf_in.array[2];
    mbuf_2_1 = lp->rx.mbuf_in.array[3];
    APP_IO_RX_PREFETCH0(mbuf_2_0);
    APP_IO_RX_PREFETCH0(mbuf_2_1);

    for (j = 0; j + 3 < n_mbufs; j += 2) {
      struct rte_mbuf *mbuf_0_0, *mbuf_0_1;
      uint32_t worker_0, worker_1;

      mbuf_0_0 = mbuf_1_0;
      mbuf_0_1 = mbuf_1_1;

      mbuf_1_0 = mbuf_2_0;
      mbuf_1_1 = mbuf_2_1;

      mbuf_2_0 = lp->rx.mbuf_in.array[j+4];
      mbuf_2_1 = lp->rx.mbuf_in.array[j+5];
      APP_IO_RX_PREFETCH0(mbuf_2_0);
      APP_IO_RX_PREFETCH0(mbuf_2_1);

      switch (fifoness) {
        case FIFONESS_FLOW:
#ifdef __SSE4_2__
          worker_0 = rte_hash_crc(rte_pktmbuf_mtod(mbuf_0_0, void *),
                                  sizeof(ETHER_HDR) + 2, portid) % n_workers;
          worker_1 = rte_hash_crc(rte_pktmbuf_mtod(mbuf_0_1, void *),
                                  sizeof(ETHER_HDR) + 2, portid) % n_workers;
#else
          worker_0 = CityHash64WithSeed(rte_pktmbuf_mtod(mbuf_0_0, void *),
                                        sizeof(ETHER_HDR) + 2, portid) % n_workers;
          worker_1 = CityHash64WithSeed(rte_pktmbuf_mtod(mbuf_0_1, void *),
                                        sizeof(ETHER_HDR) + 2, portid) % n_workers;
#endif /* __SSE4_2__ */
          break;
        case FIFONESS_PORT:
          worker_0 = worker_1 = portid % n_workers;
          break;
        case FIFONESS_NONE:
        default:
          worker_0 = j % n_workers;
          worker_1 = (j + 1) % n_workers;
          break;
      }
      app_lcore_io_rx_buffer_to_send(lp, worker_0, mbuf_0_0, bsz_wr);
      app_lcore_io_rx_buffer_to_send(lp, worker_1, mbuf_0_1, bsz_wr);
    }

    /*
     * Handle the last 1, 2 (when n_mbufs is even) or
     * 3 (when n_mbufs is odd) packets
     */
    for ( ; j < n_mbufs; j += 1) {
      struct rte_mbuf *mbuf;
      uint32_t worker;

      mbuf = mbuf_1_0;
      mbuf_1_0 = mbuf_1_1;
      mbuf_1_1 = mbuf_2_0;
      mbuf_2_0 = mbuf_2_1;
      APP_IO_RX_PREFETCH0(mbuf_1_0);

      switch (fifoness) {
        case FIFONESS_FLOW:
#ifdef __SSE4_2__
          worker = rte_hash_crc(rte_pktmbuf_mtod(mbuf, void *),
                                sizeof(ETHER_HDR) + 2, portid) % n_workers;
#else
          worker = CityHash64WithSeed(rte_pktmbuf_mtod(mbuf, void *),
                                      sizeof(ETHER_HDR) + 2, portid) % n_workers;
#endif /* __SSE4_2__ */
          break;
        case FIFONESS_PORT:
          worker = portid % n_workers;
          break;
        case FIFONESS_NONE:
        default:
          worker = j % n_workers;
          break;
      }
      app_lcore_io_rx_buffer_to_send(lp, worker, mbuf, bsz_wr);
    }
  }
}

/**
 * Put pending mbufs into worker queue and flush pending mbufs.
 * This function is called from I/O (Input) thread.
 */
static inline void
app_lcore_io_rx_flush(struct app_lcore_params_io *lp, uint32_t n_workers) {
  uint32_t worker;

  for (worker = 0; worker < n_workers; worker ++) {
    uint32_t ret, n_mbufs;

    n_mbufs = lp->rx.mbuf_out[worker].n_mbufs;
    if (likely((lp->rx.mbuf_out_flush[worker] == 0) ||
               (n_mbufs == 0))) {
      continue;
    }
    ret = rte_ring_sp_enqueue_burst(lp->rx.rings[worker],
                                    (void **) lp->rx.mbuf_out[worker].array,
                                    n_mbufs);
    if (unlikely(ret < n_mbufs)) {
      uint32_t k;
      for (k = ret; k < n_mbufs; k ++) {
        struct rte_mbuf *pkt_to_free = lp->rx.mbuf_out[worker].array[k];
        rte_pktmbuf_free(pkt_to_free);
      }
    }
    lp->rx.mbuf_out[worker].n_mbufs = 0;
    lp->rx.mbuf_out_flush[worker] = 0;
  }
}

/**
 * Dequeue mbufs from output queue and send to ethernet port.
 * This function is called from I/O (Output) thread.
 */
static inline void
app_lcore_io_tx(struct app_lcore_params_io *lp,
                uint32_t n_workers,
                uint32_t bsz_rd,
                uint32_t bsz_wr) {
  uint32_t worker;

  for (worker = 0; worker < n_workers; worker ++) {
    uint32_t i;

    for (i = 0; i < lp->tx.n_nic_ports; i ++) {
      uint8_t port = lp->tx.nic_ports[i];
      struct rte_ring *ring = lp->tx.rings[port][worker];
      struct interface *ifp;
      uint32_t n_mbufs, n_pkts;
      int ret;

      n_mbufs = lp->tx.mbuf_out[port].n_mbufs;
      ret = rte_ring_sc_dequeue_burst(ring,
                                      (void **) &lp->tx.mbuf_out[port].array[n_mbufs],
                                      bsz_rd - n_mbufs);

      if (unlikely(ret == 0)) {
        continue;
      }

      n_mbufs += (uint32_t)ret;

#if APP_IO_TX_DROP_ALL_PACKETS
      {
        uint32_t j;
        APP_IO_TX_PREFETCH0(lp->tx.mbuf_out[port].array[0]);
        APP_IO_TX_PREFETCH0(lp->tx.mbuf_out[port].array[1]);

        for (j = 0; j < n_mbufs; j ++) {
          if (likely(j < n_mbufs - 2)) {
            APP_IO_TX_PREFETCH0(lp->tx.mbuf_out[port].array[j + 2]);
          }
          rte_pktmbuf_free(lp->tx.mbuf_out[port].array[j]);
        }
        lp->tx.mbuf_out[port].n_mbufs = 0;
        continue;
      }
#endif
      if (unlikely(n_mbufs < bsz_wr)) {
        lp->tx.mbuf_out[port].n_mbufs = n_mbufs;
        lp->tx.mbuf_out_flush[port] = 1;
        continue;
      }
      ifp = dpdk_interface_lookup(port);
      if (ifp != NULL && ifp->sched_port != NULL) {
        struct lagopus_packet *pkt;
        struct rte_mbuf *m;
        int qidx, color;

        for (i = 0; i < n_mbufs; i++) {
          m = lp->tx.mbuf_out[port].array[i];
          pkt = (struct lagopus_packet *)
                (m->buf_addr + APP_DEFAULT_MBUF_LOCALDATA_OFFSET);
          if (unlikely(pkt->queue_id != 0)) {
            qidx = dpdk_interface_queue_id_to_index(ifp, pkt->queue_id);
            color = rte_meter_trtcm_color_blind_check(&ifp->ifqueue.meters[qidx],
                    rte_rdtsc(),
                    OS_M_PKTLEN(m));
            rte_sched_port_pkt_write(m, 0, 0, 0, qidx, color);
          }
        }
        n_mbufs = rte_sched_port_enqueue(ifp->sched_port,
                                         lp->tx.mbuf_out[port].array,
                                         n_mbufs);
        n_mbufs = rte_sched_port_dequeue(ifp->sched_port,
                                         lp->tx.mbuf_out[port].array,
                                         n_mbufs);
      }
      DPRINTF("send %d pkts\n", n_mbufs);
      n_pkts = rte_eth_tx_burst(port,
                                0,
                                lp->tx.mbuf_out[port].array,
                                (uint16_t) n_mbufs);
      DPRINTF("sent %d pkts\n", n_pkts);

#if APP_STATS
      lp->tx.nic_ports_iters[port] ++;
      lp->tx.nic_ports_count[port] += n_pkts;
      if (unlikely(lp->tx.nic_ports_iters[port] == APP_STATS)) {
        unsigned lcore = rte_lcore_id();

        printf("\t\t\tI/O TX %u out (port %u): avg burst size = %.2f\n",
               lcore,
               (unsigned) port,
               ((double) lp->tx.nic_ports_count[port]) / ((double)
                   lp->tx.nic_ports_iters[port]));
        lp->tx.nic_ports_iters[port] = 0;
        lp->tx.nic_ports_count[port] = 0;
      }
#endif

      if (unlikely(n_pkts < n_mbufs)) {
        uint32_t k;
        for (k = n_pkts; k < n_mbufs; k ++) {
          struct rte_mbuf *pkt_to_free = lp->tx.mbuf_out[port].array[k];
          rte_pktmbuf_free(pkt_to_free);
        }
      }
      lp->tx.mbuf_out[port].n_mbufs = 0;
      lp->tx.mbuf_out_flush[port] = 0;
    }
  }
}

static inline void
app_lcore_io_tx_flush(struct app_lcore_params_io *lp, __UNUSED void *arg) {
  uint8_t portid, i;

  for (i = 0; i < lp->tx.n_nic_ports; i++) {
    uint32_t n_pkts;

    portid = lp->tx.nic_ports[i];
    if (likely((lp->tx.mbuf_out_flush[portid] == 0) ||
               (lp->tx.mbuf_out[portid].n_mbufs == 0))) {
      continue;
    }

    DPRINTF("flush: send %d pkts\n", lp->tx.mbuf_out[portid].n_mbufs);
    n_pkts = rte_eth_tx_burst(portid,
                              0,
                              lp->tx.mbuf_out[portid].array,
                              (uint16_t)lp->tx.mbuf_out[portid].n_mbufs);
    DPRINTF("flus: sent %d pkts\n", n_pkts);

    if (unlikely(n_pkts < lp->tx.mbuf_out[portid].n_mbufs)) {
      uint32_t k;
      for (k = n_pkts; k < lp->tx.mbuf_out[portid].n_mbufs; k ++) {
        struct rte_mbuf *pkt_to_free = lp->tx.mbuf_out[portid].array[k];
        rte_pktmbuf_free(pkt_to_free);
      }
    }
    lp->tx.mbuf_out[portid].n_mbufs = 0;
    lp->tx.mbuf_out_flush[portid] = 0;
  }
}

static void
app_lcore_io_tx_cleanup(struct app_lcore_params_io *lp) {
  unsigned i;

  for (i = 0; i < lp->tx.n_nic_ports; i ++) {
    uint8_t portid = lp->tx.nic_ports[i];

    /* Flush TX and RX queues. */
    lagopus_msg_info("Cleanup physical port %u\n", portid);
    rte_eth_dev_stop(portid);
    rte_eth_dev_close(portid);
  }
}

void
app_lcore_io_flush(struct app_lcore_params_io *lp,
                   uint32_t n_workers,
                   void *arg) {
  app_lcore_io_rx_flush(lp, n_workers);
  app_lcore_io_tx_flush(lp, arg);
}

void
app_lcore_io(struct app_lcore_params_io *lp, uint32_t n_workers) {
  uint32_t bsz_rx_rd = app.burst_size_io_rx_read;
  uint32_t bsz_rx_wr = app.burst_size_io_rx_write;
  uint32_t bsz_tx_rd = app.burst_size_io_tx_read;
  uint32_t bsz_tx_wr = app.burst_size_io_tx_write;

  app_lcore_io_rx(lp, n_workers, bsz_rx_rd, bsz_rx_wr);
  app_lcore_io_tx(lp, n_workers, bsz_tx_rd, bsz_tx_wr);
}

void
app_lcore_main_loop_io(void *arg) {
  uint32_t lcore = rte_lcore_id();
  struct app_lcore_params_io *lp = &app.lcore_params[lcore].io;
  uint32_t n_workers = app_get_lcores_worker();
  uint32_t flush_count = 0;
  uint32_t update_count = 0;

  uint32_t bsz_rx_rd = app.burst_size_io_rx_read;
  uint32_t bsz_rx_wr = app.burst_size_io_rx_write;
  uint32_t bsz_tx_rd = app.burst_size_io_tx_read;
  uint32_t bsz_tx_wr = app.burst_size_io_tx_write;

  if (lp->rx.n_nic_queues > 0 && lp->tx.n_nic_ports == 0) {
    /* receive loop */
    for (;;) {
      if (APP_LCORE_IO_FLUSH && unlikely(flush_count == APP_LCORE_IO_FLUSH)) {
        app_lcore_io_rx_flush(lp, n_workers);
        flush_count = 0;
      }
      if (update_count == DP_UPDATE_COUNT) {
        if (rte_atomic32_read(&dpdk_stop) != 0) {
          break;
        }
        update_count = 0;
      }
      app_lcore_io_rx(lp, n_workers, bsz_rx_rd, bsz_rx_wr);
      flush_count++;
      update_count++;
    }
  } else if (lp->rx.n_nic_queues == 0 && lp->tx.n_nic_ports > 0) {
    /* transimit loop */
    for (;;) {
      if (APP_LCORE_IO_FLUSH && unlikely(flush_count == APP_LCORE_IO_FLUSH)) {
        app_lcore_io_tx_flush(lp, arg);
        flush_count = 0;
      }
      if (update_count == DP_UPDATE_COUNT) {
        if (rte_atomic32_read(&dpdk_stop) != 0) {
          break;
        }
        update_count = 0;
      }
      app_lcore_io_tx(lp, n_workers, bsz_tx_rd, bsz_tx_wr);
      flush_count++;
      update_count++;
    }
  } else {
    for (;;) {
      if (APP_LCORE_IO_FLUSH && unlikely(flush_count == APP_LCORE_IO_FLUSH)) {
        app_lcore_io_rx_flush(lp, n_workers);
        app_lcore_io_tx_flush(lp, arg);
        flush_count = 0;
      }
      if (update_count == DP_UPDATE_COUNT) {
        if (rte_atomic32_read(&dpdk_stop) != 0) {
          break;
        }
        update_count = 0;
      }
      app_lcore_io_rx(lp, n_workers, bsz_rx_rd, bsz_rx_wr);
      app_lcore_io_tx(lp, n_workers, bsz_tx_rd, bsz_tx_wr);
      flush_count++;
      update_count++;
    }
  }
  /* cleanup */
  if (likely(lp->tx.n_nic_ports > 0)) {
    app_lcore_io_tx_cleanup(lp);
  }
}

void
app_init_mbuf_pools(void) {
  unsigned socket, lcore;

  /* Init the buffer pools */
  for (socket = 0; socket < APP_MAX_SOCKETS; socket ++) {
    char name[32];
    if (app_is_socket_used(socket) == 0) {
      continue;
    }

    snprintf(name, sizeof(name), "mbuf_pool_%u", socket);
    lagopus_dprint("Creating the mbuf pool for socket %u ...\n", socket);
    app.pools[socket] = rte_mempool_create(
                          name,
                          APP_DEFAULT_MEMPOOL_BUFFERS,
                          APP_DEFAULT_MBUF_SIZE,
                          APP_DEFAULT_MEMPOOL_CACHE_SIZE,
                          sizeof(struct rte_pktmbuf_pool_private),
                          rte_pktmbuf_pool_init, NULL,
                          rte_pktmbuf_init, NULL,
                          (int)socket,
                          0);
    if (app.pools[socket] == NULL) {
      rte_panic("Cannot create mbuf pool on socket %u\n", socket);
    }
  }

  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    if (app.lcore_params[lcore].type == e_APP_LCORE_DISABLED) {
      continue;
    }

    socket = rte_lcore_to_socket_id(lcore);
    app.lcore_params[lcore].pool = app.pools[socket];
  }
}

void
app_init_rings_rx(void) {
  unsigned lcore;

  /* Initialize the rings for the RX side */
  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    struct app_lcore_params_io *lp_io = &app.lcore_params[lcore].io;
    unsigned socket_io, lcore_worker;

    if ((app.lcore_params[lcore].type != e_APP_LCORE_IO &&
         app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER) ||
        (lp_io->rx.n_nic_queues == 0)) {
      continue;
    }

    socket_io = rte_lcore_to_socket_id(lcore);

    for (lcore_worker = 0; lcore_worker < APP_MAX_LCORES; lcore_worker ++) {
      char name[32];
      struct app_lcore_params_worker *lp_worker;
      struct rte_ring *ring = NULL;

      lp_worker = &app.lcore_params[lcore_worker].worker;
      if (app.lcore_params[lcore_worker].type != e_APP_LCORE_WORKER &&
          app.lcore_params[lcore_worker].type != e_APP_LCORE_IO_WORKER) {
        continue;
      }

      lagopus_dprint(
        "Creating ring to connect I/O "
        "lcore %u (socket %u) with worker lcore %u ...\n",
        lcore,
        socket_io,
        lcore_worker);
      snprintf(name, sizeof(name),
               "app_ring_rx_s%u_io%u_w%u",
               socket_io,
               lcore,
               lcore_worker);
      ring = rte_ring_create(
               name,
               app.ring_rx_size,
               (int)socket_io,
               RING_F_SP_ENQ | RING_F_SC_DEQ);
      if (ring == NULL) {
        rte_panic("Cannot create ring to connect I/O "
                  "core %u with worker core %u\n",
                  lcore,
                  lcore_worker);
      }

      lp_io->rx.rings[lp_io->rx.n_rings] = ring;
      lp_io->rx.n_rings ++;

      lp_worker->rings_in[lp_worker->n_rings_in] = ring;
      lp_worker->n_rings_in ++;
    }
  }

  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    struct app_lcore_params_io *lp_io = &app.lcore_params[lcore].io;

    if ((app.lcore_params[lcore].type != e_APP_LCORE_IO &&
         app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER) ||
        (lp_io->rx.n_nic_queues == 0)) {
      continue;
    }

    if (lp_io->rx.n_rings != app_get_lcores_worker()) {
      rte_panic("Algorithmic error (I/O RX rings)\n");
    }
  }

  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    struct app_lcore_params_worker *lp_worker;

    lp_worker = &app.lcore_params[lcore].worker;
    if (app.lcore_params[lcore].type != e_APP_LCORE_WORKER &&
        app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER) {
      continue;
    }

    if (lp_worker->n_rings_in != app_get_lcores_io_rx()) {
      rte_panic("Algorithmic error (worker input rings)\n");
    }
  }
}

void
app_init_rings_tx(void) {
  unsigned lcore;

  /* Initialize the rings for the TX side */
  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    struct app_lcore_params_worker *lp_worker = &app.lcore_params[lcore].worker;
    unsigned port;

    if (app.lcore_params[lcore].type != e_APP_LCORE_WORKER &&
        app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER) {
      continue;
    }

    for (port = 0; port < APP_MAX_NIC_PORTS; port ++) {
      char name[32];
      struct app_lcore_params_io *lp_io = NULL;
      struct rte_ring *ring;
      uint32_t socket_io, lcore_io;

      if (app.nic_tx_port_mask[port] == 0) {
        continue;
      }

      if (app_get_lcore_for_nic_tx((uint8_t) port, &lcore_io) < 0) {
        rte_panic("Algorithmic error (no I/O core to handle TX of port %u)\n",
                  port);
      }

      lp_io = &app.lcore_params[lcore_io].io;
      socket_io = rte_lcore_to_socket_id(lcore_io);

      lagopus_dprint("Creating ring to connect "
                     "worker lcore %u with "
                     "TX port %u (through I/O lcore %u)"
                     " (socket %u) ...\n",
                     lcore,
                     port,
                     (unsigned)lcore_io,
                     (unsigned)socket_io);
      snprintf(name, sizeof(name),
               "app_ring_tx_s%u_w%u_p%u",
               socket_io, lcore, port);
      ring = rte_ring_create(
               name,
               app.ring_tx_size,
               (int)socket_io,
               RING_F_SP_ENQ | RING_F_SC_DEQ);
      if (ring == NULL) {
        rte_panic("Cannot create ring to connect"
                  " worker core %u with TX port %u\n",
                  lcore,
                  port);
      }

      lp_worker->rings_out[port] = ring;
      lp_io->tx.rings[port][lp_worker->worker_id] = ring;
    }
  }

  for (lcore = 0; lcore < APP_MAX_LCORES; lcore ++) {
    struct app_lcore_params_io *lp_io = &app.lcore_params[lcore].io;
    unsigned i;

    if ((app.lcore_params[lcore].type != e_APP_LCORE_IO &&
         app.lcore_params[lcore].type != e_APP_LCORE_IO_WORKER) ||
        (lp_io->tx.n_nic_ports == 0)) {
      continue;
    }

    for (i = 0; i < lp_io->tx.n_nic_ports; i ++) {
      unsigned port, j;

      port = lp_io->tx.nic_ports[i];
      for (j = 0; j < app_get_lcores_worker(); j ++) {
        if (lp_io->tx.rings[port][j] == NULL) {
          rte_panic("Algorithmic error (I/O TX rings)\n");
        }
      }
    }
  }
}

#if defined(RTE_VERSION_NUM) && RTE_VERSION >= RTE_VERSION_NUM(2, 0, 0, 4)
static inline uint8_t
dpdk_get_detachable_portid_by_name(const char *name) {
  uint8_t portid;
  struct rte_eth_dev *dev;
  struct rte_pci_addr *addr;
  char devname[RTE_ETH_NAME_MAX_LEN];

  for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++) {
    dev = &rte_eth_devices[portid];
    if ((dev->attached) &&
        (dev->pci_dev->driver->drv_flags & RTE_PCI_DRV_DETACHABLE)) {
      switch (dev->dev_type) {
        case RTE_ETH_DEV_PCI:
          addr = &dev->pci_dev->addr;
          snprintf(devname, RTE_ETH_NAME_MAX_LEN,
                   "%04x:%02x:%02x.%d",
                   addr->domain, addr->bus,
                   addr->devid, addr->function);
          if (strcmp(name, devname) == 0) {
            goto out;
          }
          break;
        case RTE_ETH_DEV_VIRTUAL:
          if (strncmp(name, dev->data->name, RTE_ETH_NAME_MAX_LEN) == 0) {
            goto out;
          }
          break;
        default:
          break;
      }
    }
  }
out:
  return portid;
}
#endif /* RTE_VERSION_NUM */

static void
dpdk_intr_event_callback(uint8_t portid, enum rte_eth_event_type type,
                         void *param) {
  struct interface *ifp;

  switch (type) {
    case RTE_ETH_EVENT_INTR_LSC:
      ifp = param;
      if (ifp != NULL && ifp->port != NULL) {
        dpdk_update_port_link_status(ifp->port);
      }
      break;
    default:
      break;
  }
}

static inline const char *
dpdk_remove_namespace(const char *device) {
  char *p;

  p = strchr(device, ':');
  if (p == NULL) {
    return device;
  }
  return p+1;
}

lagopus_result_t
dpdk_configure_interface(struct interface *ifp) {
  unsigned socket;
  uint32_t lcore;
  uint8_t queue;
  int ret;
  uint32_t n_rx_queues, n_tx_queues;
  uint8_t portid;
  struct rte_mempool *pool;

  if (is_rawsocket_only_mode() == true) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
#if defined(RTE_VERSION_NUM) && RTE_VERSION >= RTE_VERSION_NUM(2, 0, 0, 4)
  if (strlen(ifp->info.eth_dpdk_phy.device) > 0) {
    uint8_t actual_portid;
    const char *name;

    name = dpdk_remove_namespace(ifp->info.eth_dpdk_phy.device);
    if (rte_eth_dev_attach(name, &actual_portid)) {
      return LAGOPUS_RESULT_NOT_FOUND;
    }
    /* whenever 'device' is specified, overwrite portid by actual portid. */
    ifp->info.eth.port_number = (uint32_t)actual_portid;
  }
#endif /* RTE_VERSION_NUM */
  portid = (uint8_t)ifp->info.eth.port_number;

  n_rx_queues = app_get_nic_rx_queues_per_port(portid);
  n_tx_queues = app.nic_tx_port_mask[portid];

  if ((n_rx_queues == 0) && (n_tx_queues == 0)) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (ifp->info.eth_dpdk_phy.mtu < 64 ||
      ifp->info.eth_dpdk_phy.mtu > MAX_PACKET_SZ) {
    return LAGOPUS_RESULT_OUT_OF_RANGE;
  }

  rte_eth_dev_info_get(portid, &ifp->devinfo);

  /* Init port */
  printf("Initializing NIC port %u ...\n", (unsigned) portid);
  if ((rte_eth_devices[portid].data->dev_flags & RTE_ETH_DEV_INTR_LSC) != 0) {
    port_conf.intr_conf.lsc = 1;
    ret = rte_eth_dev_configure(portid,
                                (uint8_t) n_rx_queues,
                                (uint8_t) n_tx_queues,
                                &port_conf);
    if (ret >= 0) {
      rte_eth_dev_callback_register(portid,
                                    RTE_ETH_EVENT_INTR_LSC,
                                    dpdk_intr_event_callback,
                                    ifp);
    }
  } else {
    port_conf.intr_conf.lsc = 0;
    ret = rte_eth_dev_configure(portid,
                                (uint8_t) n_rx_queues,
                                (uint8_t) n_tx_queues,
                                &port_conf);
    /* register link update periodic timer */
    if (ret >= 0) {
      add_link_timer(ifp);
    }
  }
  if (ret < 0) {
    rte_panic("Cannot init NIC port %u (%s)\n",
              (unsigned) portid, strerror(-ret));
  }
  ret = rte_eth_dev_set_mtu(portid, ifp->info.eth_dpdk_phy.mtu);
  if (ret < 0) {
    if (ret != -ENOTSUP) {
      rte_panic("Cannot set MTU(%d) for port %d (%d)\n",
                ifp->info.eth_dpdk_phy.mtu,
                portid,
                ret);
    } else {
      lagopus_msg_notice("Cannot set MTU(%d) for port %d, not supporetd\n",
                         ifp->info.eth_dpdk_phy.mtu,
                         portid);
    }
  }
  rte_eth_promiscuous_enable(portid);

  /* Init RX queues */
  for (queue = 0; queue < APP_MAX_RX_QUEUES_PER_NIC_PORT; queue ++) {
    struct app_lcore_params_io *lp;
    uint8_t i;

    if (app.nic_rx_queue_mask[portid][queue] == NIC_RX_QUEUE_UNCONFIGURED) {
      continue;
    }
    app_get_lcore_for_nic_rx(portid, queue, &lcore);
    lp = &app.lcore_params[lcore].io;
    socket = rte_lcore_to_socket_id(lcore);
    pool = app.lcore_params[lcore].pool;

    lagopus_msg_info("Initializing NIC port %u RX queue %u ...\n",
                     (unsigned) portid,
                     (unsigned) queue);
    ret = rte_eth_rx_queue_setup(portid,
                                 queue,
                                 (uint16_t) app.nic_rx_ring_size,
                                 socket,
#if defined(RTE_VERSION_NUM) && RTE_VERSION >= RTE_VERSION_NUM(1, 8, 0, 0)
                                 &ifp->devinfo.default_rxconf,
#else
                                 &rx_conf,
#endif /* RTE_VERSION_NUM */
                                 pool);
    if (ret < 0) {
      rte_panic("Cannot init RX queue %u for port %u (%d)\n",
                (unsigned) queue,
                (unsigned) portid,
                ret);
    }
    for (i = 0; i < lp->rx.n_nic_queues; i++) {
      if (lp->rx.nic_queues[i].port != portid ||
          lp->rx.nic_queues[i].queue != queue) {
        continue;
      }
      lp->rx.nic_queues[i].enabled = true;
      break;
    }
  }

  /* Init TX queues */
  if (app.nic_tx_port_mask[portid] == 1) {
    app_get_lcore_for_nic_tx(portid, &lcore);
    socket = rte_lcore_to_socket_id(lcore);
    lagopus_msg_info("Initializing NIC port %u TX queue 0 ...\n",
                     (unsigned) portid);
    ret = rte_eth_tx_queue_setup(portid,
                                 0,
                                 (uint16_t) app.nic_tx_ring_size,
                                 socket,
#if defined(RTE_VERSION_NUM) && RTE_VERSION >= RTE_VERSION_NUM(1, 8, 0, 0)
                                 &ifp->devinfo.default_txconf
#else
                                 &tx_conf
#endif /* RTE_VERSION_NUM */
                                );
    if (ret < 0) {
      rte_panic("Cannot init TX queue 0 for port %d (%d)\n",
                portid,
                ret);
    }
  }

  ifp->stats = dpdk_port_stats;
  dpdk_interface_set_index(ifp);

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dpdk_unconfigure_interface(struct interface *ifp) {
  uint8_t portid, queue;

#if defined(RTE_VERSION_NUM) && RTE_VERSION >= RTE_VERSION_NUM(2, 0, 0, 4)
  if (strlen(ifp->info.eth_dpdk_phy.device) > 0) {
    uint8_t actual_portid;
    const char *name;

    name = dpdk_remove_namespace(ifp->info.eth_dpdk_phy.device);
    actual_portid = dpdk_get_detachable_portid_by_name(name);
    if (actual_portid == RTE_MAX_ETHPORTS) {
      return LAGOPUS_RESULT_NOT_FOUND;
    }
    /* whenever 'device' is specified, overwrite portid by actual portid. */
    ifp->info.eth.port_number = (uint32_t)actual_portid;
  }
#endif /* RTE_VERSION_NUM */
  portid = (uint8_t)ifp->info.eth.port_number;

  dpdk_stop_interface(portid);
  for (queue = 0; queue < APP_MAX_RX_QUEUES_PER_NIC_PORT; queue ++) {
    struct app_lcore_params_io *lp;
    uint32_t lcore;
    uint8_t i;

    if (app.nic_rx_queue_mask[portid][queue] == NIC_RX_QUEUE_UNCONFIGURED) {
      continue;
    }
    app_get_lcore_for_nic_rx(portid, queue, &lcore);
    lp = &app.lcore_params[lcore].io;
    for (i = 0; i < lp->rx.n_nic_queues; i++) {
      if (lp->rx.nic_queues[i].port != portid ||
          lp->rx.nic_queues[i].queue != queue) {
        continue;
      }
      lp->rx.nic_queues[i].enabled = false;
      break;
    }
  }
#if defined(RTE_VERSION_NUM) && RTE_VERSION >= RTE_VERSION_NUM(2, 0, 0, 4)
  if (strlen(ifp->info.eth_dpdk_phy.device) > 0) {
    char detached_devname[RTE_ETH_NAME_MAX_LEN];

    rte_eth_dev_close(portid);
    rte_eth_dev_detach(portid, detached_devname);
  }
#endif /* RTE_VERSION_NUM */
  dpdk_interface_unset_index(ifp);
  return LAGOPUS_RESULT_OK;
}

void
app_init_nics(void) {
#ifndef RTE_VERSION_NUM
  /* Init driver */
  printf("Initializing the PMD driver ...\n");
  if (rte_pmd_init_all() < 0) {
    rte_panic("Cannot init PMD\n");
  }
#elif RTE_VERSION < RTE_VERSION_NUM(1, 8, 0, 0)
  if (rte_eal_pci_probe() < 0) {
    rte_panic("Cannot probe PCI\n");
  }
#endif /* RTE_VERSION_NUM */
}

static struct port_stats *
dpdk_port_stats(struct port *port) {
  struct rte_eth_stats rte_stats;
  struct timespec ts;
  struct port_stats *stats;

  if (unlikely(port->ifindex >= RTE_MAX_ETHPORTS)) {
    return NULL;
  }
  stats = calloc(1, sizeof(struct port_stats));
  if (stats == NULL) {
    return NULL;
  }

  rte_eth_stats_get((uint8_t)port->ifindex, &rte_stats);
  /* if counter is not supported, set all ones value. */
  stats->ofp.port_no = port->ofp_port.port_no;
  stats->ofp.rx_packets = rte_stats.ipackets;
  stats->ofp.tx_packets = rte_stats.opackets;
  stats->ofp.rx_bytes = rte_stats.ibytes;
  stats->ofp.tx_bytes = rte_stats.obytes;
  stats->ofp.rx_dropped = rte_stats.rx_nombuf;
  stats->ofp.tx_dropped = UINT64_MAX;
  stats->ofp.rx_errors = rte_stats.ierrors;
  stats->ofp.tx_errors = rte_stats.oerrors;
  stats->ofp.rx_frame_err = UINT64_MAX;
  stats->ofp.rx_over_err = UINT64_MAX;
  stats->ofp.rx_crc_err = UINT64_MAX;
  stats->ofp.collisions = UINT64_MAX;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  stats->ofp.duration_sec = (uint32_t)(ts.tv_sec - port->create_time.tv_sec);
  if (ts.tv_nsec < port->create_time.tv_nsec) {
    stats->ofp.duration_sec--;
    stats->ofp.duration_nsec = 1 * 1000 * 1000 * 1000;
  } else {
    stats->ofp.duration_nsec = 0;
  }
  stats->ofp.duration_nsec += (uint32_t)ts.tv_nsec;
  stats->ofp.duration_nsec -= (uint32_t)port->create_time.tv_nsec;
  OS_MEMCPY(&port->ofp_port_stats, &stats->ofp, sizeof(stats->ofp));

  return stats;
}

lagopus_result_t
dpdk_update_port_link_status(struct port *port) {
  struct rte_eth_link link;
  struct interface *ifp;
  uint8_t portid;
  bool changed = false;

  ifp = port->interface;
  if (ifp == NULL ||
      ifp->info.type != DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY) {
    return LAGOPUS_RESULT_OK;
  }
  portid = (uint8_t)port->ifindex;
  /* ofp_port.config is configured by the controller. */

  /* other port status. */
  /* OFPPF_* for OpenFlow 1.3 */
  rte_eth_link_get_nowait(portid, &link);
  port->ofp_port.curr_speed = link.link_speed * 1000;
  switch (link.link_speed) {
    case ETH_SPEED_NUM_10M:
      port->ofp_port.curr = OFPPF_10MB_FD;
      break;
    case ETH_SPEED_NUM_100M:
      port->ofp_port.curr = OFPPF_100MB_FD;
      break;
    case ETH_SPEED_NUM_1G:
      port->ofp_port.curr = OFPPF_1GB_FD;
      break;
    case ETH_LINK_SPEED_10G:
      port->ofp_port.curr = OFPPF_10GB_FD;
      break;
    default:
      port->ofp_port.curr_speed = 0;
      port->ofp_port.curr = OFPPF_OTHER;
      break;
  }
  if (port->ofp_port.curr_speed != 0 &&
      link.link_duplex != ETH_LINK_FULL_DUPLEX) {
    port->ofp_port.curr >>= 1;
  }
  if (link.link_status == 0) {
    if (port->ofp_port.state != OFPPS_LINK_DOWN) {
      lagopus_msg_info("Physical Port %d: link down\n", port->ifindex);
      changed = true;
    }
    port->ofp_port.state = OFPPS_LINK_DOWN;
  } else {
    if (port->ofp_port.state != OFPPS_LIVE) {
      lagopus_msg_info("Physical Port %d: link up\n", port->ifindex);
      changed = true;
    }
    port->ofp_port.state = OFPPS_LIVE;
  }
  /* all zero means the port has no available feature information. */
  port->ofp_port.supported = 0;
  port->ofp_port.peer = 0;

  if (changed == true) {
    send_port_status(port, OFPPR_MODIFY);
  }
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dpdk_start_interface(uint8_t portid) {
  rte_eth_dev_start(portid);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dpdk_stop_interface(uint8_t portid) {
  rte_eth_dev_stop(portid);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dpdk_get_hwaddr(uint8_t portid, uint8_t *hw_addr) {
  rte_eth_macaddr_get(portid,
                      (struct ether_addr *)hw_addr);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dpdk_get_stats(uint8_t portid, datastore_interface_stats_t *stats) {
  struct rte_eth_stats rte_stats;

  rte_eth_stats_get(portid, &rte_stats);
  stats->rx_packets = rte_stats.ipackets;
  stats->rx_bytes = rte_stats.ibytes;
  stats->tx_packets = rte_stats.opackets;
  stats->tx_bytes = rte_stats.obytes;
  stats->rx_errors = rte_stats.ierrors;
  stats->tx_dropped = 0;
  stats->tx_errors = rte_stats.oerrors;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dpdk_clear_stats(uint8_t portid) {
  rte_eth_stats_reset(portid);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dpdk_change_config(uint8_t portid, uint32_t advertised, uint32_t config) {
  struct rte_eth_conf conf;
  uint16_t link_speed, link_duplex;
  lagopus_result_t rv;

  if (advertised != 0) {
    /* Change link speed setup and reconfigure port. */
    memcpy(&conf, &port_conf, sizeof(conf));
    switch ((advertised & 0x0000207f)) {
      case OFPPF_10MB_HD:
        link_speed = ETH_LINK_SPEED_FIXED | ETH_LINK_SPEED_10M_HD;
        break;
      case OFPPF_10MB_FD:
        link_speed = ETH_LINK_SPEED_FIXED | ETH_LINK_SPEED_10M;
        break;
      case OFPPF_100MB_HD:
        link_speed = ETH_LINK_SPEED_FIXED | ETH_LINK_SPEED_100M_HD;
        break;
      case OFPPF_100MB_FD:
        link_speed = ETH_LINK_SPEED_FIXED | ETH_LINK_SPEED_100M;
        break;
      case OFPPF_1GB_FD:
        link_speed = ETH_LINK_SPEED_FIXED | ETH_LINK_SPEED_1G;
        break;
      case OFPPF_10GB_FD:
        link_speed = ETH_LINK_SPEED_FIXED | ETH_LINK_SPEED_10G;
        break;
      case OFPPF_40GB_FD:
        link_speed = ETH_LINK_SPEED_FIXED | ETH_LINK_SPEED_40G;
        break;
      case OFPPF_100GB_FD:
        link_speed = ETH_LINK_SPEED_FIXED | ETH_LINK_SPEED_100G;
        break;
#if 0 /* currently not suppored by DPDK */
      case OFPPF_1GB_HD:
        link_speed = ETH_LINK_SPEED_FIXED | ETH_LINK_SPEED_1G;
        break;
      case OFPPF_1TB_FD:
        link_speed = ETH_LINK_SPEED_FIXED | ETH_LINK_SPEED_1000000;
        link_duplex = ETH_LINK_FULL_DUPLEX;
        break;
      case OFPPF_OTHER:
        break;
#endif /* 0 */
      case OFPPF_AUTONEG:
        link_speed = ETH_LINK_SPEED_AUTONEG;
        break;
      default:
        /* invalid advertise. */
        lagopus_msg_error("Invalid advertisement 0x%x for port %d\n",
                          advertised,
                          portid);
        return LAGOPUS_RESULT_INVALID_ARGS;
    }
  }

  /* Down physical port. */
  rte_eth_dev_stop(portid);

  if (advertised != 0) {
    conf.link_speeds = link_speed;
    rv = rte_eth_dev_configure(portid, 1, 1, &conf);
    if (rv < 0) {
      lagopus_msg_error("Fail to reconfigure port %d (%s)\n",
                        portid, strerror(-(int)rv));
      return LAGOPUS_RESULT_ANY_FAILURES;
    }
  }
  if ((config & OFPPC_PORT_DOWN) == 0) {
    /* Up physical port. */
    rte_eth_dev_start(portid);
  }

  return LAGOPUS_RESULT_OK;
}

bool
lagopus_is_port_enabled(const struct port *port) {
  if (port == NULL || port->type != LAGOPUS_PORT_TYPE_PHYSICAL) {
    return false;
  }
  return (lagopus_port_mask & (unsigned long)(1 << port->ifindex)) != 0;
}

bool
lagopus_is_portid_enabled(int portid) {
  return (lagopus_port_mask & (unsigned long)(1 << portid)) != 0;
}

void
lagopus_packet_free(struct lagopus_packet *pkt) {
  if (is_rawsocket_only_mode() != true) {
    rte_pktmbuf_free(pkt->mbuf);
  } else {
    if (rte_mbuf_refcnt_update(pkt->mbuf, -1) == 0) {
      free(pkt->mbuf);
    }
  }
}
