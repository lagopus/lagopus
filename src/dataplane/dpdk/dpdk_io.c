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
#ifdef __SSE4_2__
#include <rte_hash_crc.h>
#else
#include "City.h"
#endif /* __SSE4_2__ */
#include <rte_string_fns.h>
#include <rte_kni.h>

#include "lagopus/ethertype.h"
#include "lagopus/flowdb.h"
#include "lagopus/meter.h"
#include "lagopus/dpmgr.h"
#include "lagopus/port.h"
#include "lagopus_gstate.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/vector.h"

#include "lagopus/dataplane.h"
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
#define APP_LCORE_IO_FLUSH           10000
#endif

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

/* Max size of a single packet */
#define MAX_PACKET_SZ           2048

/* Total octets in ethernet header */
#define KNI_ENET_HEADER_SIZE    14

/* Total octets in the FCS */
#define KNI_ENET_FCS_SIZE       4

/* ethernet addresses of ports */
static struct ether_addr lagopus_ports_eth_addr[RTE_MAX_ETHPORTS];

/* mask of enabled ports */
static unsigned long lagopus_port_mask = 0;

static struct rte_kni *lagopus_kni[RTE_MAX_ETHPORTS];

static const struct rte_eth_conf port_conf = {
  .rxmode = {
    .split_hdr_size = 0,
    .header_split   = 0, /**< Header Split disabled */
    .hw_ip_checksum = 0, /**< IP checksum offload enabled */
    .hw_vlan_filter = 0, /**< VLAN filtering disabled */
    .hw_vlan_strip  = 0, /**< VLAN strip disabled */
    .hw_vlan_extend = 0, /**< Extended VLAN disabled */
    .jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
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
};

static const struct rte_eth_rxconf rx_conf = {
  .rx_thresh = {
    .pthresh = APP_DEFAULT_NIC_RX_PTHRESH,
    .hthresh = APP_DEFAULT_NIC_RX_HTHRESH,
    .wthresh = APP_DEFAULT_NIC_RX_WTHRESH,
  },
  .rx_free_thresh = APP_DEFAULT_NIC_RX_FREE_THRESH,
  .rx_drop_en = APP_DEFAULT_NIC_RX_DROP_EN,
};

#if !defined(RTE_VERSION_NUM) || RTE_VERSION < RTE_VERSION_NUM(1, 8, 0, 0)
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

static int kni_change_mtu(uint8_t port_id, unsigned new_mtu);
static int kni_config_network_interface(uint8_t port_id, uint8_t if_up);

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

  if (unlikely(ret < bsz)) {
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
  for (i = 0; i < lp->rx.n_nic_queues; i ++) {
    uint8_t port = lp->rx.nic_queues[i].port;
    uint8_t queue = lp->rx.nic_queues[i].queue;
    uint32_t n_mbufs, j;

    n_mbufs = rte_eth_rx_burst(port,
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

      rte_eth_stats_get(port, &stats);

      printf("I/O RX %u in (NIC port %u): NIC drop ratio = %.2f avg burst size = %.2f\n",
             lcore,
             (unsigned) port,
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
                                  sizeof(ETHER_HDR) + 2, port) % n_workers;
          worker_1 = rte_hash_crc(rte_pktmbuf_mtod(mbuf_0_1, void *),
                                  sizeof(ETHER_HDR) + 2, port) % n_workers;
#else
          worker_0 = CityHash64WithSeed(rte_pktmbuf_mtod(mbuf_0_0, void *),
                                  sizeof(ETHER_HDR) + 2, port) % n_workers;
          worker_1 = CityHash64WithSeed(rte_pktmbuf_mtod(mbuf_0_1, void *),
                                  sizeof(ETHER_HDR) + 2, port) % n_workers;
#endif /* __SSE4_2__ */
          break;
        case FIFONESS_PORT:
          worker_0 = worker_1 = port % n_workers;
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
                                sizeof(ETHER_HDR) + 2, port) % n_workers;
#else
          worker = CityHash64WithSeed(rte_pktmbuf_mtod(mbuf, void *),
                                sizeof(ETHER_HDR) + 2, port) % n_workers;
#endif /* __SSE4_2__ */
          break;
        case FIFONESS_PORT:
          worker = port % n_workers;
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
app_lcore_io_tx_kni(struct app_lcore_params_io *lp, uint32_t bsz) {
  struct rte_mbuf *pkts_burst[bsz];
  unsigned num;
  uint8_t portid;
  uint16_t nb_tx;
  unsigned i, j;

  return;
  for (i = 0; i < lp->tx.n_nic_ports; i++) {

    portid = lp->tx.nic_ports[i];
    if (lagopus_kni[portid] == NULL) {
      continue;
    }
    num = rte_kni_rx_burst(lagopus_kni[portid], pkts_burst, bsz);
    if (num == 0 || (uint32_t)num > bsz) {
      continue;
    }
    nb_tx = rte_eth_tx_burst(portid, 0, pkts_burst, (uint16_t)num);
    if (unlikely(nb_tx < (uint16_t)num)) {
      /* Free mbufs not tx to NIC */
      for (j = nb_tx; j < num; j++) {
        rte_pktmbuf_free(pkts_burst[j]);
      }
    }
  }
}

static inline void
app_lcore_io_tx_flush(struct app_lcore_params_io *lp, void *arg) {
  struct dpmgr *dpmgr = arg;
  uint8_t portid, i;

  for (i = 0; i < lp->tx.n_nic_ports; i++) {
    uint32_t n_pkts;
    struct port *port;

    portid = lp->tx.nic_ports[i];
    rte_kni_handle_request(lagopus_kni[portid]);
    port = port_lookup(dpmgr->ports, portid);
    if (port != NULL) {
      update_port_link_status(port);
    }

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
  app_lcore_io_tx_kni(lp, bsz_tx_wr);
}

void
app_lcore_main_loop_io(void *arg) {
  uint32_t lcore = rte_lcore_id();
  struct app_lcore_params_io *lp = &app.lcore_params[lcore].io;
  uint32_t n_workers = app_get_lcores_worker();
  uint64_t i = 0;

  uint32_t bsz_rx_rd = app.burst_size_io_rx_read;
  uint32_t bsz_rx_wr = app.burst_size_io_rx_write;
  uint32_t bsz_tx_rd = app.burst_size_io_tx_read;
  uint32_t bsz_tx_wr = app.burst_size_io_tx_write;

  if (lp->rx.n_nic_queues > 0 && lp->tx.n_nic_ports == 0) {
    /* receive loop */
    for (;;) {
      if (APP_LCORE_IO_FLUSH && (unlikely(i == APP_LCORE_IO_FLUSH))) {
        if (rte_atomic32_read(&dpdk_stop) != 0) {
          break;
        }
        app_lcore_io_rx_flush(lp, n_workers);
        i = 0;
      }
      app_lcore_io_rx(lp, n_workers, bsz_rx_rd, bsz_rx_wr);
      i++;
    }
  } else if (lp->rx.n_nic_queues == 0 && lp->tx.n_nic_ports > 0) {
    /* transimit loop */
    for (;;) {
      if (APP_LCORE_IO_FLUSH && (unlikely(i == APP_LCORE_IO_FLUSH))) {
        if (rte_atomic32_read(&dpdk_stop) != 0) {
          break;
        }
        app_lcore_io_tx_flush(lp, arg);
        i = 0;
      }
      app_lcore_io_tx(lp, n_workers, bsz_tx_rd, bsz_tx_wr);
      app_lcore_io_tx_kni(lp, bsz_tx_wr);
      i++;
    }
  } else {
    for (;;) {
      if (APP_LCORE_IO_FLUSH && (unlikely(i == APP_LCORE_IO_FLUSH))) {
        if (rte_atomic32_read(&dpdk_stop) != 0) {
          break;
        }
        app_lcore_io_rx_flush(lp, n_workers);
        app_lcore_io_tx_flush(lp, arg);
        i = 0;
      }
      app_lcore_io_rx(lp, n_workers, bsz_rx_rd, bsz_rx_wr);
      app_lcore_io_tx(lp, n_workers, bsz_tx_rd, bsz_tx_wr);
      app_lcore_io_tx_kni(lp, bsz_tx_wr);
      i++;
    }
  }
  /* cleanup */
  if (likely(lp->tx.n_nic_ports > 0)) {
    app_lcore_io_tx_cleanup(lp);
  }
}
/* Check the link status of all ports in up to 9s, and print them finally */
static uint16_t
check_port_link_status(uint8_t portid) {
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 100 /* 10s (100 * 100ms) in total */
  uint8_t count;
  struct rte_eth_link link;

  printf("\nChecking link status");
  fflush(stdout);
  for (count = 0; count <= MAX_CHECK_TIME; count++) {
    memset(&link, 0, sizeof(link));
    rte_eth_link_get_nowait(portid, &link);
    /* print link status if flag set */
    /* delay and retry */
    if (link.link_status != 0) {
      break;
    }
    printf(".");
    fflush(stdout);
    rte_delay_ms(CHECK_INTERVAL);
  }
  if (link.link_status) {
    printf("Port %d Link Up - speed %u Mbps - %s\n",
           (uint8_t)portid,
           (unsigned)link.link_speed,
           (link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
           ("full-duplex") : ("half-duplex\n"));
  } else {
    printf("Port %d Link Down\n",
           (uint8_t)portid);
  }
  return link.link_speed;
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
    lagopus_msg_debug("Creating the mbuf pool for socket %u ...\n", socket);
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

      lagopus_msg_debug(
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

      lagopus_msg_debug("Creating ring to connect "
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

void
app_init_nics(void) {
  struct rte_eth_dev_info devinfo;
  unsigned socket;
  uint32_t lcore;
  uint8_t port, queue;
  int ret;
  uint32_t n_rx_queues, n_tx_queues;

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
#endif /* RTE_VERSION */

  /* Init NIC ports and queues, then start the ports */
  for (port = 0; port < APP_MAX_NIC_PORTS; port ++) {
    struct rte_mempool *pool;

    n_rx_queues = app_get_nic_rx_queues_per_port(port);
    n_tx_queues = app.nic_tx_port_mask[port];

    if ((n_rx_queues == 0) && (n_tx_queues == 0)) {
      continue;
    }

    memset(&devinfo, 0, sizeof(devinfo));
    rte_eth_dev_info_get((uint8_t)port, &devinfo);

    /* Init port */
    printf("Initializing NIC port %u ...\n", (unsigned) port);
    ret = rte_eth_dev_configure(
            port,
            (uint8_t) n_rx_queues,
            (uint8_t) n_tx_queues,
            &port_conf);
    if (ret < 0) {
      rte_panic("Cannot init NIC port %u (%s)\n",
                (unsigned) port, strerror(-ret));
    }
    rte_eth_promiscuous_enable(port);

    /* Init RX queues */
    for (queue = 0; queue < APP_MAX_RX_QUEUES_PER_NIC_PORT; queue ++) {
      if (app.nic_rx_queue_mask[port][queue] == 0) {
        continue;
      }

      app_get_lcore_for_nic_rx(port, queue, &lcore);
      socket = rte_lcore_to_socket_id(lcore);
      pool = app.lcore_params[lcore].pool;

      printf("Initializing NIC port %u RX queue %u ...\n",
             (unsigned) port,
             (unsigned) queue);
      ret = rte_eth_rx_queue_setup(
              port,
              queue,
              (uint16_t) app.nic_rx_ring_size,
              socket,
              &rx_conf,
              pool);
      if (ret < 0) {
        rte_panic("Cannot init RX queue %u for port %u (%d)\n",
                  (unsigned) queue,
                  (unsigned) port,
                  ret);
      }
    }

    /* Init TX queues */
    if (app.nic_tx_port_mask[port] == 1) {
      app_get_lcore_for_nic_tx(port, &lcore);
      socket = rte_lcore_to_socket_id(lcore);
      printf("Initializing NIC port %u TX queue 0 ...\n",
             (unsigned) port);
      ret = rte_eth_tx_queue_setup(
              port,
              0,
              (uint16_t) app.nic_tx_ring_size,
              socket,
#if defined(RTE_VERSION_NUM) && RTE_VERSION >= RTE_VERSION_NUM(1, 8, 0, 0)
                                 &devinfo.default_txconf
#else
                                 &tx_conf
#endif /* RTE_VERSION_NUM */
                                   );
      if (ret < 0) {
        rte_panic("Cannot init TX queue 0 for port %d (%d)\n",
                  port,
                  ret);
      }
    }

    /* Start port */
    ret = rte_eth_dev_start(port);
    if (ret < 0) {
      rte_panic("Cannot start port %d (%d)\n", port, ret);
    }
    check_port_link_status(port);
  }
}

void
app_init_kni(void) {
  uint8_t portid;

  for (portid = 0; portid < rte_eth_dev_count(); portid++) {
    struct rte_kni *kni;
    struct rte_kni_ops ops;
    struct rte_kni_conf conf;
    struct rte_eth_dev_info dev_info;

    continue; /* XXX */
    /* Clear conf at first */
    memset(&conf, 0, sizeof(conf));
    memset(&dev_info, 0, sizeof(dev_info));
    memset(&ops, 0, sizeof(ops));
    snprintf(conf.name, RTE_KNI_NAMESIZE, "vEth%u", portid);
    conf.group_id = (uint16_t)portid;
    conf.mbuf_size = MAX_PACKET_SZ;
    rte_eth_dev_info_get(portid, &dev_info);
    conf.addr = dev_info.pci_dev->addr;
    conf.id = dev_info.pci_dev->id;
    ops.port_id = portid;
    ops.change_mtu = kni_change_mtu;
    ops.config_network_if = kni_config_network_interface,
        /* XXX socket 0 */
        kni = rte_kni_alloc(app.pools[0], &conf, &ops);
    if (kni == NULL) {
      lagopus_msg_error("Fail to create kni dev for port: %d\n", portid);
      continue;
    }
    lagopus_kni[portid] = kni;
    printf("KNI: %s is configured.\n", conf.name);

    printf("Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
           (unsigned) portid,
           lagopus_ports_eth_addr[portid].addr_bytes[0],
           lagopus_ports_eth_addr[portid].addr_bytes[1],
           lagopus_ports_eth_addr[portid].addr_bytes[2],
           lagopus_ports_eth_addr[portid].addr_bytes[3],
           lagopus_ports_eth_addr[portid].addr_bytes[4],
           lagopus_ports_eth_addr[portid].addr_bytes[5]);

    /* initialize port stats */
    memset(&port_statistics, 0, sizeof(port_statistics));
  }
}

/* --------------------------Lagopus code start ----------------------------- */
static struct port_stats *
port_stats(struct port *port) {
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
update_port_link_status(struct port *port) {
  struct rte_eth_link link;
  uint8_t portid;
  bool changed = false;

  portid = (uint8_t)port->ifindex;
  /* ofp_port.config is configured by the controller. */

  /* other port status. */
  /* OFPPF_* for OpenFlow 1.3 */
  rte_eth_link_get_nowait(portid, &link);
  switch (link.link_speed) {
    case ETH_LINK_SPEED_10:
      port->ofp_port.curr_speed = 10* 1000;
      port->ofp_port.curr = OFPPF_10MB_FD;
      break;
    case ETH_LINK_SPEED_100:
      port->ofp_port.curr_speed = 100 * 1000;
      port->ofp_port.curr = OFPPF_100MB_FD;
      break;
    case ETH_LINK_SPEED_1000:
      port->ofp_port.curr_speed = 1000 * 1000;
      port->ofp_port.curr = OFPPF_1GB_FD;
      break;
    case ETH_LINK_SPEED_10000:
      port->ofp_port.curr_speed = 10000 * 1000;
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
      rte_eth_dev_stop((uint8_t)port->ifindex);
      rte_eth_dev_start((uint8_t)port->ifindex);
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
lagopus_configure_physical_port(struct port *port) {
  struct rte_eth_dev_info devinfo;

  /* DPDK engine */
  if (app_get_nic_rx_queues_per_port((uint8_t)port->ifindex) == 0) {
    /* Do not configured by DPDK, nothing to do. */
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  lagopus_port_mask |= (unsigned long)(1 << port->ifindex);
  port->stats = port_stats;
  rte_eth_macaddr_get((uint8_t)port->ifindex,
                      (struct ether_addr *)port->ofp_port.hw_addr);
  memset(&devinfo, 0, sizeof(devinfo));
  rte_eth_dev_info_get((uint8_t)port->ifindex, &devinfo);
  if (devinfo.pci_dev != NULL) {
    port->eth.pci_addr.domain = devinfo.pci_dev->addr.domain;
    port->eth.pci_addr.bus = devinfo.pci_dev->addr.bus;
    port->eth.pci_addr.devid = devinfo.pci_dev->addr.devid;
    port->eth.pci_addr.function = devinfo.pci_dev->addr.function;
    port->eth.pci_id.vendor_id = devinfo.pci_dev->id.vendor_id;
    port->eth.pci_id.device_id = devinfo.pci_dev->id.device_id;
  }
  rte_eth_dev_start((uint8_t)port->ifindex);

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
lagopus_unconfigure_physical_port(struct port *port) {
  /* DPDK engine */
  lagopus_port_mask &= (unsigned long)~(1 << port->ifindex);
  rte_eth_dev_stop((uint8_t)port->ifindex);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
lagopus_change_physical_port(struct port *port) {
  struct rte_eth_conf conf;
  uint16_t link_speed, link_duplex;
  lagopus_result_t rv;

  if (port->ofp_port.advertised != 0) {
    /* Change link speed setup and reconfigure port. */
    memcpy(&conf, &port_conf, sizeof(conf));
    switch ((port->ofp_port.advertised & 0x0000207f)) {
      case OFPPF_10MB_HD:
        link_speed = ETH_LINK_SPEED_10;
        link_duplex = ETH_LINK_HALF_DUPLEX;
        break;
      case OFPPF_10MB_FD:
        link_speed = ETH_LINK_SPEED_10;
        link_duplex = ETH_LINK_FULL_DUPLEX;
        break;
      case OFPPF_100MB_HD:
        link_speed = ETH_LINK_SPEED_100;
        link_duplex = ETH_LINK_HALF_DUPLEX;
        break;
      case OFPPF_100MB_FD:
        link_speed = ETH_LINK_SPEED_100;
        link_duplex = ETH_LINK_FULL_DUPLEX;
        break;
      case OFPPF_1GB_HD:
        link_speed = ETH_LINK_SPEED_1000;
        link_duplex = ETH_LINK_HALF_DUPLEX;
        break;
      case OFPPF_1GB_FD:
        link_speed = ETH_LINK_SPEED_1000;
        link_duplex = ETH_LINK_FULL_DUPLEX;
        break;
      case OFPPF_10GB_FD:
        link_speed = ETH_LINK_SPEED_10000;
        link_duplex = ETH_LINK_FULL_DUPLEX;
        break;
      case OFPPF_40GB_FD:
        link_speed = 40000; /*ETH_LINK_SPEED_40000*/
        link_duplex = ETH_LINK_FULL_DUPLEX;
        break;
#if 0 /* currently not suppored by DPDK because link_speed as uint16_t */
      case OFPPF_100GB_FD:
        link_speed = ETH_LINK_SPEED_100000;
        link_duplex = ETH_LINK_FULL_DUPLEX;
        break;
      case OFPPF_1TB_FD:
        link_speed = ETH_LINK_SPEED_1000000;
        link_duplex = ETH_LINK_FULL_DUPLEX;
        break;
      case OFPPF_OTHER:
        break;
#endif /* 0 */
      case OFPPF_AUTONEG:
        link_speed = ETH_LINK_SPEED_AUTONEG;
        link_duplex = ETH_LINK_AUTONEG_DUPLEX;
        break;
      default:
        /* invalid advertise. */
        lagopus_msg_error("Invalid advertisement 0x%x for port %d\n",
                          port->ofp_port.advertised,
                          port->ifindex);
        return LAGOPUS_RESULT_INVALID_ARGS;
    }
  }

  /* Down physical port. */
  rte_eth_dev_stop((uint8_t)port->ifindex);

  if (port->ofp_port.advertised != 0) {
    conf.link_speed = link_speed;
    conf.link_duplex = link_duplex;
    rv = rte_eth_dev_configure((uint8_t)port->ifindex, 1, 1, &conf);
    if (rv < 0) {
      lagopus_msg_error("Fail to reconfigure port %d (%s)\n",
                        port->ifindex, strerror(-(int)rv));
      return LAGOPUS_RESULT_ANY_FAILURES;
    }
  }

  if ((port->ofp_port.config & OFPPC_PORT_DOWN) == 0) {
    /* Up physical port. */
    rte_eth_dev_start((uint8_t)port->ifindex);
  }
  send_port_status(port, OFPPR_MODIFY);
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
  OS_M_FREE(pkt->mbuf);
}

int
lagopus_send_packet_normal(struct lagopus_packet *pkt, uint32_t portid) {
  struct rte_kni *kni;

  /* So far, output only one pakcet. */
  kni = lagopus_kni[portid];
  if (kni != NULL) {
    rte_kni_tx_burst(kni, &pkt->mbuf, 1);
  }

  return 0;
}

/* --------------------------Lagopus code end---------------------------- */

/* Callback for request of changing MTU */
static int
kni_change_mtu(uint8_t port_id, unsigned new_mtu) {
  int ret;
  struct rte_eth_conf conf;

  if (port_id >= rte_eth_dev_count()) {
    lagopus_msg_error("Invalid port id %d\n", port_id);
    return -EINVAL;
  }

  lagopus_msg_info("Change MTU of port %d to %u\n", port_id, new_mtu);

  /* Stop specific port */
  rte_eth_dev_stop(port_id);

  memcpy(&conf, &port_conf, sizeof(conf));
  /* Set new MTU */
  if (new_mtu > ETHER_MAX_LEN) {
    conf.rxmode.jumbo_frame = 1;
  } else {
    conf.rxmode.jumbo_frame = 0;
  }

  /* mtu + length of header + length of FCS = max pkt length */
  conf.rxmode.max_rx_pkt_len = new_mtu + KNI_ENET_HEADER_SIZE +
                               KNI_ENET_FCS_SIZE;
  ret = rte_eth_dev_configure(port_id, 1, 1, &conf);
  if (ret < 0) {
    lagopus_msg_error("Fail to reconfigure port %d\n", port_id);
    return ret;
  }

  /* Restart specific port */
  ret = rte_eth_dev_start(port_id);
  if (ret < 0) {
    lagopus_msg_error("Fail to restart port %d\n", port_id);
    return ret;
  }

  return 0;
}

/* Callback for request of configuring network interface up/down */
static int
kni_config_network_interface(uint8_t port_id, uint8_t if_up) {
  int ret = 0;

  if (port_id >= rte_eth_dev_count() || port_id >= RTE_MAX_ETHPORTS) {
    lagopus_msg_error("Invalid port id %d\n", port_id);
    return -EINVAL;
  }

  lagopus_msg_info("Configure network interface of %d %s\n",
                   port_id, if_up ? "up" : "down");

  if (if_up != 0) { /* Configure network interface up */
    rte_eth_dev_stop(port_id);
    ret = rte_eth_dev_start(port_id);
  } else { /* Configure network interface down */
    rte_eth_dev_stop(port_id);
  }

  if (ret < 0) {
    lagopus_msg_error("Failed to start port %d\n", port_id);
  }

  return ret;
}
