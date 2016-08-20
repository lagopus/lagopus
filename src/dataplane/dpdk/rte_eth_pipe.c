/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
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

/*
 * Usage:
 *  --vdev eth_pipe0[,socket=N] --vdev eth_pipe1[,socket=N]
 *  N: numa node (0 or 1) default:0
 *  single --vdev creates twa virtual ports and connected via ring.
 */

#include <rte_config.h>
#include <rte_mbuf.h>
#include <rte_ethdev.h>
#include <rte_malloc.h>
#include <rte_memcpy.h>
#include <rte_string_fns.h>
#include <rte_dev.h>
#include <rte_kvargs.h>
#include <rte_errno.h>

#include "rte_eth_pipe.h"

#define ETH_PIPE_ATTACH_ARG     "attach"

#ifndef RTE_PMD_PIPE_MAX_RX_RING
#define RTE_PMD_PIPE_MAX_RX_RINGS 16
#endif
#ifndef RTE_PMD_PIPE_MAX_TX_RING
#define RTE_PMD_PIPE_MAX_TX_RINGS 16
#endif

static const char *valid_arguments[] = {
  ETH_PIPE_ATTACH_ARG,
  NULL
};

struct ring_queue {
  struct rte_ring *rng;
  rte_atomic64_t rx_pkts;
  rte_atomic64_t tx_pkts;
  rte_atomic64_t err_pkts;
  rte_atomic64_t rx_bytes;
  rte_atomic64_t tx_bytes;
  uint8_t if_index;
};

struct pmd_internals {
  unsigned nb_rx_queues;
  unsigned nb_tx_queues;

  struct ring_queue rx_ring_queues[RTE_PMD_PIPE_MAX_RX_RINGS];
  struct ring_queue tx_ring_queues[RTE_PMD_PIPE_MAX_TX_RINGS];
};


static struct ether_addr eth_addr = { .addr_bytes = {0} };
static const char *drivername = "Pipe PMD";
static struct rte_eth_link pmd_link = {
  .link_speed = 10000,
  .link_duplex = ETH_LINK_FULL_DUPLEX,
  .link_status = 0
};

static uint16_t
eth_ring_rx(void *q, struct rte_mbuf **bufs, uint16_t nb_bufs) {
  void **ptrs = (void *)&bufs[0];
  struct ring_queue *r = q;
  const uint16_t nb_rx = (uint16_t)rte_ring_dequeue_burst(r->rng,
                         ptrs, nb_bufs);
  uint16_t cnt;
  if (r->rng->flags & RING_F_SC_DEQ) {
    r->rx_pkts.cnt += nb_rx;
    for (cnt = 0; cnt < nb_rx; cnt++) {
      r->rx_bytes.cnt += rte_pktmbuf_pkt_len(bufs[cnt]);
    }
  } else {
    rte_atomic64_add(&(r->rx_pkts), nb_rx);
    for (cnt = 0; cnt < nb_rx; cnt++) {
      rte_atomic64_add(&(r->rx_bytes), rte_pktmbuf_pkt_len(bufs[cnt]));
    }
  }
  for (cnt = 0; cnt < nb_rx; cnt++) {
#ifdef RTE_MBUF_HAS_PKT
    bufs[cnt]->pkt.in_port = r->if_index;
#else
    bufs[cnt]->port = r->if_index;
#endif /* RTE_MBUF_HAS_PKT */
  }
  return nb_rx;
}

static uint16_t
eth_ring_tx(void *q, struct rte_mbuf **bufs, uint16_t nb_bufs) {
  void **ptrs = (void *)&bufs[0];
  struct ring_queue *r = q;
  const uint16_t nb_tx = (uint16_t)rte_ring_enqueue_burst(r->rng,
                         ptrs, nb_bufs);
  uint16_t cnt;
  if (r->rng->flags & RING_F_SP_ENQ) {
    r->tx_pkts.cnt += nb_tx;
    r->err_pkts.cnt += nb_bufs - nb_tx;
    for (cnt = 0; cnt < nb_tx; cnt++) {
      r->tx_bytes.cnt += rte_pktmbuf_pkt_len(bufs[cnt]);
    }
  } else {
    rte_atomic64_add(&(r->tx_pkts), nb_tx);
    rte_atomic64_add(&(r->err_pkts), nb_bufs - nb_tx);
    for (cnt = 0; cnt < nb_tx; cnt++) {
      rte_atomic64_add(&(r->tx_bytes),rte_pktmbuf_pkt_len(bufs[cnt]));
    }
  }
  return nb_tx;
}

static int
eth_dev_configure(struct rte_eth_dev *dev __rte_unused) { return 0; }

static int
eth_dev_start(struct rte_eth_dev *dev) {
  dev->data->dev_link.link_status = 1;
  return 0;
}

static void
eth_dev_stop(struct rte_eth_dev *dev) {
  dev->data->dev_link.link_status = 0;
}

static int
eth_rx_queue_setup(struct rte_eth_dev *dev,uint16_t rx_queue_id,
                   uint16_t nb_rx_desc __rte_unused,
                   unsigned int socket_id __rte_unused,
                   const struct rte_eth_rxconf *rx_conf __rte_unused,
                   struct rte_mempool *mb_pool __rte_unused) {
  struct pmd_internals *internals = dev->data->dev_private;
  dev->data->rx_queues[rx_queue_id] = &internals->rx_ring_queues[rx_queue_id];
  internals->rx_ring_queues[rx_queue_id].if_index = dev->data->port_id;
  return 0;
}

static int
eth_dev_set_link_down(struct rte_eth_dev *dev) {
	dev->data->dev_link.link_status = ETH_LINK_DOWN;
	return 0;
}

static int
eth_dev_set_link_up(struct rte_eth_dev *dev) {
	dev->data->dev_link.link_status = ETH_LINK_UP;
	return 0;
}

static int
eth_tx_queue_setup(struct rte_eth_dev *dev, uint16_t tx_queue_id,
                   uint16_t nb_tx_desc __rte_unused,
                   unsigned int socket_id __rte_unused,
                   const struct rte_eth_txconf *tx_conf __rte_unused) {
  struct pmd_internals *internals = dev->data->dev_private;
  dev->data->tx_queues[tx_queue_id] = &internals->tx_ring_queues[tx_queue_id];
  return 0;
}


static void
eth_dev_info(struct rte_eth_dev *dev,
             struct rte_eth_dev_info *dev_info) {
  struct pmd_internals *internals = dev->data->dev_private;
  dev_info->driver_name = drivername;
  dev_info->max_mac_addrs = 1;
  dev_info->max_rx_pktlen = (uint32_t)-1;
  dev_info->max_rx_queues = (uint16_t)internals->nb_rx_queues;
  dev_info->max_tx_queues = (uint16_t)internals->nb_tx_queues;
  dev_info->min_rx_bufsize = 0;
  dev_info->pci_dev = NULL;
}

static void
eth_stats_get(struct rte_eth_dev *dev, struct rte_eth_stats *igb_stats) {
  unsigned i;
  unsigned long rx_total = 0, tx_total = 0, tx_err_total = 0;
  unsigned long rx_btotal = 0, tx_btotal = 0;
  const struct pmd_internals *internal = dev->data->dev_private;

  memset(igb_stats, 0, sizeof(*igb_stats));
  for (i = 0; i < RTE_ETHDEV_QUEUE_STAT_CNTRS &&
       i < internal->nb_rx_queues; i++) {
    igb_stats->q_ipackets[i] = internal->rx_ring_queues[i].rx_pkts.cnt;
    igb_stats->q_ibytes[i] = internal->rx_ring_queues[i].rx_bytes.cnt;
    rx_total += igb_stats->q_ipackets[i];
    rx_btotal += igb_stats->q_ibytes[i];
  }

  for (i = 0; i < RTE_ETHDEV_QUEUE_STAT_CNTRS &&
       i < internal->nb_tx_queues; i++) {
    igb_stats->q_opackets[i] = internal->tx_ring_queues[i].tx_pkts.cnt;
    igb_stats->q_obytes[i] = internal->tx_ring_queues[i].tx_bytes.cnt;
    igb_stats->q_errors[i] = internal->tx_ring_queues[i].err_pkts.cnt;
    tx_total += igb_stats->q_opackets[i];
    tx_btotal += igb_stats->q_obytes[i];
    tx_err_total += igb_stats->q_errors[i];
  }

  igb_stats->ipackets = rx_total;
  igb_stats->opackets = tx_total;
  igb_stats->ibytes = rx_btotal;
  igb_stats->obytes = tx_btotal;
  igb_stats->oerrors = tx_err_total;
}

static void
eth_stats_reset(struct rte_eth_dev *dev) {
  unsigned i;
  struct pmd_internals *internal = dev->data->dev_private;
  for (i = 0; i < internal->nb_rx_queues; i++) {
    internal->rx_ring_queues[i].rx_pkts.cnt = 0;
    internal->rx_ring_queues[i].rx_bytes.cnt = 0;
  }
  for (i = 0; i < internal->nb_tx_queues; i++) {
    internal->tx_ring_queues[i].tx_pkts.cnt = 0;
    internal->tx_ring_queues[i].tx_bytes.cnt = 0;
    internal->tx_ring_queues[i].err_pkts.cnt = 0;
  }
}

static int
eth_set_mtu(struct rte_eth_dev *dev __rte_unused,
            int mtu __rte_unused) { return 0; }

static void
eth_queue_release(void *q __rte_unused) { ; }
static int
eth_link_update(struct rte_eth_dev *dev __rte_unused,
                int wait_to_complete __rte_unused) { return 0; }

static struct eth_dev_ops ops = {
  .dev_start = eth_dev_start,
  .dev_stop = eth_dev_stop,
  .dev_set_link_up = eth_dev_set_link_up,
  .dev_set_link_down = eth_dev_set_link_down,
  .dev_configure = eth_dev_configure,
  .dev_infos_get = eth_dev_info,
  .rx_queue_setup = eth_rx_queue_setup,
  .tx_queue_setup = eth_tx_queue_setup,
  .rx_queue_release = eth_queue_release,
  .tx_queue_release = eth_queue_release,
  .link_update = eth_link_update,
  .stats_get = eth_stats_get,
  .stats_reset = eth_stats_reset,
  .mtu_set = eth_set_mtu,
};

static int
rte_eth_from_rings(const char *name, struct rte_ring *const rx_queues[],
                   const unsigned nb_rx_queues,
                   struct rte_ring *const tx_queues[],
                   const unsigned nb_tx_queues,
                   const unsigned numa_node) {
  struct rte_eth_dev_data *data = NULL;
  struct pmd_internals *internals = NULL;
  struct rte_eth_dev *eth_dev = NULL;
  unsigned i;

  /* do some parameter checking */
  if (rx_queues == NULL && nb_rx_queues > 0) {
    goto error;
  }
  if (tx_queues == NULL && nb_tx_queues > 0) {
    goto error;
  }

  RTE_LOG(INFO, PMD, "Creating pipe ethdev %s on numa socket %u\n",
          name, numa_node);

  /* now do all data allocation - for eth_dev structure, dummy pci driver
   * and internal (private) data
   */
  data = rte_zmalloc_socket(name, sizeof(*data), 0, numa_node);
  if (data == NULL) {
    rte_errno = ENOMEM;
    goto error;
  }

  internals = rte_zmalloc_socket(name, sizeof(*internals), 0, numa_node);
  if (internals == NULL) {
    rte_errno = ENOMEM;
    goto error;
  }

  /* reserve an ethdev entry */
  eth_dev = rte_eth_dev_allocate(name, RTE_ETH_DEV_VIRTUAL);
  if (eth_dev == NULL) {
    rte_errno = ENOSPC;
    goto error;
  }

  /* now put it all together
   * - store queue data in internals,
   * - store numa_node info in pci_driver
   * - point eth_dev_data to internals and pci_driver
   * - and point eth_dev structure to new eth_dev_data structure
   */
  /* NOTE: we'll replace the data element, of originally allocated eth_dev
   * so the rings are local per-process */

  internals->nb_rx_queues = nb_rx_queues;
  internals->nb_tx_queues = nb_tx_queues;
  for (i = 0; i < nb_rx_queues; i++) {
    internals->rx_ring_queues[i].rng = rx_queues[i];
  }
  for (i = 0; i < nb_tx_queues; i++) {
    internals->tx_ring_queues[i].rng = tx_queues[i];
  }

  data->dev_private = internals;
  data->port_id = eth_dev->data->port_id;
  memmove(data->name, eth_dev->data->name, sizeof(data->name));
  data->nb_rx_queues = (uint16_t)nb_rx_queues;
  data->nb_tx_queues = (uint16_t)nb_tx_queues;
  data->dev_link = pmd_link;
  data->mac_addrs = &eth_addr;

  eth_dev->data = data;
  eth_dev->driver = NULL;
  eth_dev ->dev_ops = &ops;
  data->dev_flags = RTE_ETH_DEV_DETACHABLE;
  data->kdrv = RTE_KDRV_NONE;
  data->drv_name = drivername;
  data->numa_node = numa_node;

  TAILQ_INIT(&(eth_dev->link_intr_cbs));

  /* finally assign rx and tx ops */
  eth_dev->rx_pkt_burst = eth_ring_rx;
  eth_dev->tx_pkt_burst = eth_ring_tx;

  return data->port_id;

error:
  if (data) {
    rte_free(data);
  }
  if (internals) {
    rte_free(internals);
  }
  return -1;
}

enum dev_action {
  DEV_CREATE,
  DEV_ATTACH
};

struct vport_info {
  char name[PATH_MAX];
  unsigned node;
  enum dev_action action;
};

static int
eth_dev_ring_create(const char *name, struct vport_info *info) {
  /* rx and tx are so-called from point of view of first port.
   * They are inverted from the point of view of second port
   */
  struct rte_ring *rx[RTE_PMD_PIPE_MAX_RX_RINGS];
  struct rte_ring *tx[RTE_PMD_PIPE_MAX_TX_RINGS];
  unsigned i;
  char rx_rng_name[RTE_RING_NAMESIZE];
  char tx_rng_name[RTE_RING_NAMESIZE];
  char *devname;
  unsigned num_rings = RTE_MIN(RTE_PMD_PIPE_MAX_RX_RINGS,
                               RTE_PMD_PIPE_MAX_TX_RINGS);
  unsigned numa_node;
  enum dev_action action;

  if (info->action == DEV_CREATE) {
    devname = name;
  } else {
    devname = info->name;
  }
  numa_node = info->node;
  action = info->action;
  for (i = 0; i < num_rings; i++) {
    snprintf(rx_rng_name, sizeof(rx_rng_name), "ETH_RX%u_%s", i, devname);
    snprintf(tx_rng_name, sizeof(tx_rng_name), "ETH_TX%u_%s", i, devname);
    rx[i] = (action == DEV_CREATE) ?
            rte_ring_create(rx_rng_name, 1024, numa_node,
                            RING_F_SP_ENQ|RING_F_SC_DEQ) :
            rte_ring_lookup(tx_rng_name);
    if (rx[i] == NULL) {
      return -1;
    }
    tx[i] = (action == DEV_CREATE) ?
            rte_ring_create(tx_rng_name, 1024, numa_node,
                            RING_F_SP_ENQ|RING_F_SC_DEQ):
            rte_ring_lookup(rx_rng_name);
    if (tx[i] == NULL) {
      return -1;
    }
  }
  if (rte_eth_from_rings(name, rx, num_rings, tx, num_rings,
                         numa_node)) {
    return -1;
  }

  return 0;
}

static int
parse_kvlist(const char *key __rte_unused, const char *value, void *data) {
  struct vport_info *info = data;
  int ret;
  char *end;
  long node;

  ret = -EINVAL;

  node = rte_socket_id();
  if (!value) {
    info->name[0] = '\0';
    info->node = node;
    info->action = DEV_CREATE;
  } else {
    strncpy(info->name, value, PATH_MAX);
    info->node = node;
    info->action = DEV_ATTACH;
  }

  ret = 0;
out:
  return ret;
}

int
rte_pmd_pipe_devinit(const char *name, const char *params) {
  struct rte_kvargs *kvlist;
  int ret = 0;
  struct vport_info info;

  kvlist = rte_kvargs_parse(params, valid_arguments);

  if (!kvlist) {
    RTE_LOG(INFO, PMD,
            "Ignoring unsupported parameters when creating"
            " pipe ethernet device\n");
    info.node = rte_socket_id();
    info.action = DEV_CREATE;
    eth_dev_ring_create(name, &info);
    return 0;
  } else {
    ret = rte_kvargs_count(kvlist, ETH_PIPE_ATTACH_ARG);
    if (ret != 0) {
      ret = rte_kvargs_process(kvlist, ETH_PIPE_ATTACH_ARG,
                               parse_kvlist, &info);
    } else {
      ret = parse_kvlist(NULL, NULL, &info);
    }
    if (ret < 0) {
      goto out;
    }
    if (info.action == DEV_CREATE) {
      RTE_LOG(INFO, PMD, "Initializing pmd_pipe for %s\n", name);
    }
    if (info.action == DEV_ATTACH) {
      RTE_LOG(INFO, PMD, "Initializing pmd_pipe for %s (attached to %s)\n",
              name, info.name);
    }
    eth_dev_ring_create(name, &info);
  }
out:
  return ret;
}

static int
rte_pmd_pipe_devuninit(const char *name) {
  struct rte_eth_dev *eth_dev;

  RTE_LOG(INFO, PMD, "Uninitializing pmd_pipe for %s\n", name);
  if (name == NULL) {
    return -EINVAL;
  }
  /* find an ethdev entry */
  eth_dev = rte_eth_dev_allocated(name);
  if (eth_dev == NULL) {
    return -ENODEV;
  }
  eth_dev_stop(eth_dev);
  rte_free(eth_dev->data);
  rte_eth_dev_release_port(eth_dev);
  return 0;
}

static struct rte_driver pmd_pipe_drv = {
  .name = "eth_pipe",
  .type = PMD_VDEV,
  .init = rte_pmd_pipe_devinit,
  .uninit = rte_pmd_pipe_devuninit,
};

PMD_REGISTER_DRIVER(pmd_pipe_drv, eth_pipe);
DRIVER_REGISTER_PARAM_STRING(eth_pipe,
                             "iface=<string> ");
