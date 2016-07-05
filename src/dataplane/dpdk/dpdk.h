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
 *      @file   dpdk.h
 *      @brief  Parameter, structure and function for dataplane w/ DPDK.
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

#ifndef SRC_DATAPLANE_DPDK_DPDK_H_
#define SRC_DATAPLANE_DPDK_DPDK_H_

#include <rte_config.h>
#include "lagopus/interface.h"

/* Logical cores */
#ifndef APP_MAX_SOCKETS
#define APP_MAX_SOCKETS 2
#endif

#ifndef APP_MAX_LCORES
#define APP_MAX_LCORES       RTE_MAX_LCORE
#endif

#ifndef APP_MAX_NIC_PORTS
#define APP_MAX_NIC_PORTS    RTE_MAX_ETHPORTS
#endif

#ifndef APP_MAX_RX_QUEUES_PER_NIC_PORT
#define APP_MAX_RX_QUEUES_PER_NIC_PORT 128
#endif

#ifndef APP_MAX_TX_QUEUES_PER_NIC_PORT
#define APP_MAX_TX_QUEUES_PER_NIC_PORT 128
#endif

#ifndef APP_MAX_IO_LCORES
#define APP_MAX_IO_LCORES 32
#endif
#if (APP_MAX_IO_LCORES > APP_MAX_LCORES)
#error "APP_MAX_IO_LCORES is too big"
#endif

#ifndef APP_MAX_NIC_RX_QUEUES_PER_IO_LCORE
#define APP_MAX_NIC_RX_QUEUES_PER_IO_LCORE 32
#endif

#ifndef APP_MAX_NIC_TX_PORTS_PER_IO_LCORE
#define APP_MAX_NIC_TX_PORTS_PER_IO_LCORE 32
#endif
#if (APP_MAX_NIC_TX_PORTS_PER_IO_LCORE > APP_MAX_NIC_PORTS)
#error "APP_MAX_NIC_TX_PORTS_PER_IO_LCORE too big"
#endif

#ifndef APP_MAX_WORKER_LCORES
#define APP_MAX_WORKER_LCORES 32
#endif
#if (APP_MAX_WORKER_LCORES > APP_MAX_LCORES)
#error "APP_MAX_WORKER_LCORES is too big"
#endif

#ifndef APP_STATS
#define APP_STATS                    10000000
#endif


/* Mempools */
#ifndef APP_DEFAULT_MBUF_SIZE
#define APP_DEFAULT_MBUF_SIZE                                   \
  (MAX_PACKET_SZ +                                              \
   sizeof(struct rte_mbuf) +                                    \
   sizeof(struct lagopus_packet) +                              \
   RTE_PKTMBUF_HEADROOM)
#endif

#ifndef APP_DEFAULT_MBUF_LOCALDATA_OFFSET
#define APP_DEFAULT_MBUF_LOCALDATA_OFFSET       \
  (APP_DEFAULT_MBUF_SIZE - sizeof(struct lagopus_packet))
#endif

#ifndef APP_DEFAULT_MEMPOOL_BUFFERS
#define APP_DEFAULT_MEMPOOL_BUFFERS   8192 * 4
#endif

#ifndef APP_DEFAULT_MEMPOOL_CACHE_SIZE
#define APP_DEFAULT_MEMPOOL_CACHE_SIZE  256
#endif

/* NIC RX */
#ifndef APP_DEFAULT_NIC_RX_RING_SIZE
#define APP_DEFAULT_NIC_RX_RING_SIZE 1024
#endif

/*
 * RX and TX Prefetch, Host, and Write-back threshold values should be
 * carefully set for optimal performance. Consult the network
 * controller's datasheet and supporting DPDK documentation for guidance
 * on how these parameters should be set.
 */
#ifndef APP_DEFAULT_NIC_RX_PTHRESH
#define APP_DEFAULT_NIC_RX_PTHRESH  8
#endif

#ifndef APP_DEFAULT_NIC_RX_HTHRESH
#define APP_DEFAULT_NIC_RX_HTHRESH  8
#endif

#ifndef APP_DEFAULT_NIC_RX_WTHRESH
#define APP_DEFAULT_NIC_RX_WTHRESH  4
#endif

#ifndef APP_DEFAULT_NIC_RX_FREE_THRESH
#define APP_DEFAULT_NIC_RX_FREE_THRESH  64
#endif

#ifndef APP_DEFAULT_NIC_RX_DROP_EN
#define APP_DEFAULT_NIC_RX_DROP_EN 0
#endif

/* NIC TX */
#ifndef APP_DEFAULT_NIC_TX_RING_SIZE
#define APP_DEFAULT_NIC_TX_RING_SIZE 1024
#endif

/*
 * These default values are optimized for use with the Intel(R) 82599 10 GbE
 * Controller and the DPDK ixgbe PMD. Consider using other values for other
 * network controllers and/or network drivers.
 */
#ifndef APP_DEFAULT_NIC_TX_PTHRESH
#define APP_DEFAULT_NIC_TX_PTHRESH  36
#endif

#ifndef APP_DEFAULT_NIC_TX_HTHRESH
#define APP_DEFAULT_NIC_TX_HTHRESH  0
#endif

#ifndef APP_DEFAULT_NIC_TX_WTHRESH
#define APP_DEFAULT_NIC_TX_WTHRESH  0
#endif

#ifndef APP_DEFAULT_NIC_TX_FREE_THRESH
#define APP_DEFAULT_NIC_TX_FREE_THRESH  0
#endif

#ifndef APP_DEFAULT_NIC_TX_RS_THRESH
#define APP_DEFAULT_NIC_TX_RS_THRESH  0
#endif

/* Software Rings */
#ifndef APP_DEFAULT_RING_RX_SIZE
#define APP_DEFAULT_RING_RX_SIZE 1024
#endif

#ifndef APP_DEFAULT_RING_TX_SIZE
#define APP_DEFAULT_RING_TX_SIZE 1024
#endif

/* Bursts */
#ifndef APP_MBUF_ARRAY_SIZE
#define APP_MBUF_ARRAY_SIZE   1024
#endif

#ifndef APP_DEFAULT_BURST_SIZE_IO_RX_READ
#define APP_DEFAULT_BURST_SIZE_IO_RX_READ  144
#endif
#if (APP_DEFAULT_BURST_SIZE_IO_RX_READ > APP_MBUF_ARRAY_SIZE)
#error "APP_DEFAULT_BURST_SIZE_IO_RX_READ is too big"
#endif

#ifndef APP_DEFAULT_BURST_SIZE_IO_RX_WRITE
#define APP_DEFAULT_BURST_SIZE_IO_RX_WRITE  144
#endif
#if (APP_DEFAULT_BURST_SIZE_IO_RX_WRITE > APP_MBUF_ARRAY_SIZE)
#error "APP_DEFAULT_BURST_SIZE_IO_RX_WRITE is too big"
#endif

#ifndef APP_DEFAULT_BURST_SIZE_IO_TX_READ
#define APP_DEFAULT_BURST_SIZE_IO_TX_READ  144
#endif
#if (APP_DEFAULT_BURST_SIZE_IO_TX_READ > APP_MBUF_ARRAY_SIZE)
#error "APP_DEFAULT_BURST_SIZE_IO_TX_READ is too big"
#endif

#ifndef APP_DEFAULT_BURST_SIZE_IO_TX_WRITE
#define APP_DEFAULT_BURST_SIZE_IO_TX_WRITE  144
#endif
#if (APP_DEFAULT_BURST_SIZE_IO_TX_WRITE > APP_MBUF_ARRAY_SIZE)
#error "APP_DEFAULT_BURST_SIZE_IO_TX_WRITE is too big"
#endif

#ifndef APP_DEFAULT_BURST_SIZE_WORKER_READ
#define APP_DEFAULT_BURST_SIZE_WORKER_READ  144
#endif
#if ((2 * APP_DEFAULT_BURST_SIZE_WORKER_READ) > APP_MBUF_ARRAY_SIZE)
#error "APP_DEFAULT_BURST_SIZE_WORKER_READ is too big"
#endif

#ifndef APP_DEFAULT_BURST_SIZE_WORKER_WRITE
#define APP_DEFAULT_BURST_SIZE_WORKER_WRITE  144
#endif
#if (APP_DEFAULT_BURST_SIZE_WORKER_WRITE > APP_MBUF_ARRAY_SIZE)
#error "APP_DEFAULT_BURST_SIZE_WORKER_WRITE is too big"
#endif

#define DP_MBUF_ROUNDUP(a, b) (((a) + (b) - 1) & ~((b) - 1))

#define CORE_ASSIGN_PERFORMANCE 0 /* default */
#define CORE_ASSIGN_BALANCE     1
#define CORE_ASSIGN_MINIMUM     2

#define FIFONESS_FLOW 0 /* default */
#define FIFONESS_PORT 1
#define FIFONESS_NONE 2

#define NIC_RX_QUEUE_UNCONFIGURED 0
#define NIC_RX_QUEUE_ENABLED      1
#define NIC_RX_QUEUE_CONFIGURED   2

struct flowcache;
struct interface;

struct app_mbuf_array {
  uint32_t n_mbufs;
  struct rte_mbuf *array[APP_MBUF_ARRAY_SIZE];
};

enum app_lcore_type {
  e_APP_LCORE_DISABLED = 0,
  e_APP_LCORE_IO,
  e_APP_LCORE_WORKER,
  e_APP_LCORE_IO_WORKER /* mixed */
};

struct app_lcore_params_io {
  /* I/O RX */
  struct {
    /* NIC */
    struct {
      uint8_t port;
      uint8_t queue;
      uint8_t enabled;
    } nic_queues[APP_MAX_NIC_RX_QUEUES_PER_IO_LCORE];
    uint32_t n_nic_queues;
    struct interface *ifp[APP_MAX_NIC_RX_QUEUES_PER_IO_LCORE];
    uint32_t nifs;

    /* Rings */
    struct rte_ring *rings[APP_MAX_WORKER_LCORES];
    uint32_t n_rings;

    /* Internal buffers */
    struct app_mbuf_array mbuf_in;
    struct app_mbuf_array mbuf_out[APP_MAX_WORKER_LCORES];
    uint8_t mbuf_out_flush[APP_MAX_WORKER_LCORES];

    /* Stats */
    uint32_t nic_queues_count[APP_MAX_NIC_RX_QUEUES_PER_IO_LCORE];
    uint32_t nic_queues_iters[APP_MAX_NIC_RX_QUEUES_PER_IO_LCORE];
    uint32_t rings_count[APP_MAX_WORKER_LCORES];
    uint32_t rings_iters[APP_MAX_WORKER_LCORES];
  } rx;

  /* I/O TX */
  struct {
    /* Rings */
    struct rte_ring *rings[APP_MAX_NIC_PORTS][APP_MAX_WORKER_LCORES];

    /* NIC */
    uint8_t nic_ports[APP_MAX_NIC_TX_PORTS_PER_IO_LCORE];
    uint32_t n_nic_ports;

    /* Internal buffers */
    struct app_mbuf_array mbuf_out[APP_MAX_NIC_TX_PORTS_PER_IO_LCORE];
    uint8_t mbuf_out_flush[APP_MAX_NIC_TX_PORTS_PER_IO_LCORE];

    /* Stats */
    uint32_t rings_count[APP_MAX_NIC_PORTS][APP_MAX_WORKER_LCORES];
    uint32_t rings_iters[APP_MAX_NIC_PORTS][APP_MAX_WORKER_LCORES];
    uint32_t nic_ports_count[APP_MAX_NIC_TX_PORTS_PER_IO_LCORE];
    uint32_t nic_ports_iters[APP_MAX_NIC_TX_PORTS_PER_IO_LCORE];
  } tx;
};

struct app_lcore_params_worker {
  /* Rings */
  struct rte_ring *rings_in[APP_MAX_IO_LCORES];
  uint32_t n_rings_in;
  struct rte_ring *rings_out[APP_MAX_NIC_PORTS];

  uint32_t worker_id;
  struct flowcache *cache;
  volatile uint16_t cache_flush;

  /* Internal buffers */
  struct app_mbuf_array mbuf_in;
  struct app_mbuf_array mbuf_out[APP_MAX_NIC_PORTS];
  uint8_t mbuf_out_flush[APP_MAX_NIC_PORTS];

  /* Stats */
  uint32_t rings_in_count[APP_MAX_IO_LCORES];
  uint32_t rings_in_iters[APP_MAX_IO_LCORES];
  uint32_t rings_out_count[APP_MAX_NIC_PORTS];
  uint32_t rings_out_iters[APP_MAX_NIC_PORTS];
};

struct app_lcore_params {
  struct {
    struct app_lcore_params_io io;
    struct app_lcore_params_worker worker;
  };
  enum app_lcore_type type;
  struct rte_mempool *pool;
  unsigned socket_id;
  unsigned core_id;
} __rte_cache_aligned;

struct app_params {
  /* lcore */
  struct app_lcore_params lcore_params[APP_MAX_LCORES];

  /* NIC */
  uint8_t nic_rx_queue_mask[APP_MAX_NIC_PORTS][APP_MAX_RX_QUEUES_PER_NIC_PORT];
  uint8_t nic_tx_port_mask[APP_MAX_NIC_PORTS];

  /* mbuf pools */
  struct rte_mempool *pools[APP_MAX_SOCKETS];

  /* rings */
  uint32_t nic_rx_ring_size;
  uint32_t nic_tx_ring_size;
  uint32_t ring_rx_size;
  uint32_t ring_tx_size;

  /* burst size */
  uint32_t burst_size_io_rx_read;
  uint32_t burst_size_io_rx_write;
  uint32_t burst_size_io_tx_read;
  uint32_t burst_size_io_tx_write;
  uint32_t burst_size_worker_read;
  uint32_t burst_size_worker_write;

  /* hash type */
  uint8_t hashtype;

  /* kvs type */
  uint8_t kvs_type;

  /* core assign algorithm */
  uint8_t core_assign;

  /* flow-cache */
  uint8_t no_cache;

  /* fifoness */
  uint8_t fifoness;
} __rte_cache_aligned;

extern struct app_params app;

extern rte_atomic32_t dpdk_stop;

struct interface;
extern struct interface *ifp_table[];

/**
 * Find interface pointer from DPDK port id.
 *
 * @param[in]   portid  DPDK port id.
 *
 * @retval      !=NULL  Pointer of interface.
 * @retval      ==NULL  interface is not assigned.
 */
static inline struct interface *
dpdk_interface_lookup(uint8_t portid) {
  return ifp_table[portid];
}

/**
 * Receive packet from specified interface.
 *
 * @param[in]	ifp	Interface.
 * @param[out]	mbufs	Buffer of packets.
 * @param[in]	nb	Buffer size.
 *
 * @retval	>=0	Number of received packets.
 * @retval	<0	Error.
 */
static inline lagopus_result_t
dpdk_rx_burst(struct interface *ifp, void *mbufs[], size_t nb) {
  return (lagopus_result_t)rte_eth_rx_burst(ifp->info.eth.port_number, 0,
                                            mbufs, nb);
}

int app_parse_args(int argc, const char *argv[]);
void dp_dpdk_init(void);

void dpdk_assign_worker_ids(void);
void dpdk_init_mbuf_pools(void);

void app_lcore_io_flush(struct app_lcore_params_io *lp,
                        uint32_t n_workers,
                        void *arg);
void app_lcore_io(struct app_lcore_params_io *lp, uint32_t n_workers);
void app_lcore_main_loop_io(void *arg);
void app_lcore_main_loop_worker(void *arg);
void app_lcore_main_loop_io_worker(void *arg);
int app_lcore_main_loop(void *arg);

uint32_t app_get_nic_rx_queues_per_port(uint8_t port);
int app_get_lcore_for_nic_rx(uint8_t port, uint8_t queue,
                             uint32_t *lcore_out);
int app_get_lcore_for_nic_tx(uint8_t port, uint32_t *lcore_out);
int app_is_socket_used(uint32_t socket);
uint32_t app_get_lcores_io_rx(void);
uint32_t app_get_lcores_worker(void);
void app_print_params(void);

/**
 * Clear flowcache for each worker.
 *
 * @param[in]   wait_flush      Wait for flush if true.
 */
void clear_worker_flowcache(bool);

bool is_rawsocket_only_mode(void);
bool set_rawsocket_only_mode(bool newval);

struct interface *dpdk_interface_lookup(uint8_t portid);

bool dp_dpdk_is_portid_specified(void);

struct app_lcore_params *dp_dpdk_get_lcore_param(unsigned lcore);

unsigned dp_dpdk_lcore_count(void);

void dp_bulk_match_and_action(struct rte_mbuf *mbufs[], size_t n_mbufs,
                              struct flowcache *cache);

#endif /* SRC_DATAPLANE_DPDK_DPDK_H_ */
