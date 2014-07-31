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
 *	@file	dpdk.h
 *	@brief	Parameter, structure and function for datapath w/  Intel DPDK.
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
#define APP_MAX_NIC_RX_QUEUES_PER_IO_LCORE 16
#endif

#ifndef APP_MAX_NIC_TX_PORTS_PER_IO_LCORE
#define APP_MAX_NIC_TX_PORTS_PER_IO_LCORE 16
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
#define APP_STATS                    1000000
#endif


/* Mempools */
#ifndef APP_DEFAULT_MBUF_SIZE
#define APP_DEFAULT_MBUF_SIZE                                   \
  (2048 +                                                       \
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
#define APP_MBUF_ARRAY_SIZE   512
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

#define FIFONESS_NONE 0
#define FIFONESS_PORT 1
#define FIFONESS_FLOW 2

struct flowcache;

struct app_mbuf_array {
  struct rte_mbuf *array[APP_MBUF_ARRAY_SIZE];
  uint32_t n_mbufs;
};

enum app_lcore_type {
  e_APP_LCORE_DISABLED = 0,
  e_APP_LCORE_IO,
  e_APP_LCORE_WORKER,
  e_APP_LCORE_IO_WORKER	/* mixed */
};

struct app_lcore_params_io {
  /* I/O RX */
  struct {
    /* NIC */
    struct {
      uint8_t port;
      uint8_t queue;
    } nic_queues[APP_MAX_NIC_RX_QUEUES_PER_IO_LCORE];
    uint32_t n_nic_queues;

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

  /* flow-cache */
  uint8_t no_cache;

  /* fifoness */
  uint8_t fifoness;
} __rte_cache_aligned;

extern struct app_params app;

extern rte_atomic32_t dpdk_stop;

int app_parse_args(int argc, const char *argv[]);
void app_init(void);
void app_assign_worker_ids(void);
void app_init_mbuf_pools(void);
void app_init_rings_rx(void);
void app_init_rings_tx(void);
void app_init_nics(void);
void app_init_kni(void);
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
 * @param[in]	wait_flush	Wait for flush if true.
 */
void clear_worker_flowcache(bool);

#endif /* SRC_DATAPLANE_DPDK_DPDK_H_ */
