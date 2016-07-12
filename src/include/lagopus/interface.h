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
 * @file        interface.h
 * @brief       interface definition.
 */

#ifndef SRC_INCLUDE_LAGOPUS_INTERFACE_H_
#define SRC_INCLUDE_LAGOPUS_INTERFACE_H_

#include "lagopus_config.h"
#include "lagopus/datastore/interface.h"
#include "lagopus/datastore/queue.h"

#ifdef HAVE_DPDK
#include "rte_config.h"
#include "rte_ethdev.h"
#include "rte_sched.h"
#include "rte_ether.h"
#include "rte_meter.h"
#else
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#else
#include <net/if.h>
#include <net/if_ether.h>
#endif /* HAVE_NET_ETHERNET_H */
#endif /* HAVE_DPDK */

struct port;
struct dp_tap_interface;
struct lagopus_packet;

typedef datastore_queue_info_t dp_queue_info_t;

#define DP_MAX_QUEUES     4

#ifdef HYBRID
#define INTERFACE_IP_DEFAULT "127.0.0.1"
#define INTERFACE_IP6_DEFAULT "::1"
#define INTERFACE_L2PORT 1
#define INTERFACE_L3PORT 2
#define INTERFACE_INVALID_PORT -1
#endif /* HYBRID */
#define INTERFACE_NAME_DELIMITER '+'  /* used by tap io. */

/**
 * @brief Output queues associated with interface.
 */
struct dp_ifqueue {
  int nqueue;
  dp_queue_info_t *queues[DP_MAX_QUEUES];
#ifdef HAVE_DPDK
  struct rte_meter_trtcm meters[RTE_SCHED_QUEUES_PER_TRAFFIC_CLASS];
#endif /* HAVE_DPDK */
};

struct ip_address_info{
  bool set;
  int family;
  struct in_addr ip;
  struct in_addr broad;
  uint8_t prefixlen;
};
/**
 * @brief Interface structure.
 */
struct interface {
  datastore_interface_info_t info;
  char *name;
#ifdef HAVE_DPDK
  struct rte_eth_dev_info devinfo;
  struct rte_sched_port *sched_port;
#endif /* HAVE_DPDK */
  struct dp_ifqueue ifqueue;
  struct port_stats *(*stats)(struct port *);
  struct port *port;
  uint8_t hw_addr[ETHER_ADDR_LEN];
  struct dp_tap_interface *tap;
  struct ip_address_info addr_info; /* ip address informations. */
  struct interface **link_timer;
};

struct interface *dp_interface_alloc(void);
lagopus_result_t dp_interface_free(struct interface *interface);
lagopus_result_t dp_interface_start_internal(struct interface *ifp);
lagopus_result_t dp_interface_stop_internal(struct interface *ifp);
lagopus_result_t dp_interface_configure_internal(struct interface *ifp);
lagopus_result_t dp_interface_unconfigure_internal(struct interface *ifp);

lagopus_result_t
dp_interface_rx_burst_internal(struct interface *ifp,
                               void *mbufs[], size_t nb);

/**
 * Send packet to tap interface.
 * @param[in]   pkt     packet.
 */
lagopus_result_t
dp_interface_send_packet_kernel(struct lagopus_packet *pkt,
                                struct interface *ifp);
/**
 * Send packet to kernel normal path related with physical port.
 *
 * @param[in]   pkt     packet.
 * @param[in]   ifp     interface.
 *
 */
lagopus_result_t
dp_interface_send_packet_normal(struct lagopus_packet *pkt,
                                struct interface *ifp,
                                bool l2_bridge);
lagopus_result_t
dp_interface_change_config(struct interface *ifp,
                           uint32_t advertised,
                           uint32_t config);
lagopus_result_t
dp_interface_hw_addr_get_internal(struct interface *ifp, uint8_t *hw_addr);
lagopus_result_t
dp_interface_stats_get_internal(struct interface *ifp,
                                datastore_interface_stats_t *stats);
lagopus_result_t
dp_interface_stats_clear_internal(struct interface *ifp);

lagopus_result_t dp_interface_queue_configure(struct interface *ifp);
lagopus_result_t
dp_interface_queue_add(struct interface *ifp, dp_queue_info_t *queue);
lagopus_result_t
dp_interface_queue_delete(struct interface *ifp, uint32_t queue_id);

lagopus_result_t dpdk_configure_interface(struct interface *ifp);
lagopus_result_t dpdk_unconfigure_interface(struct interface *ifp);
lagopus_result_t dpdk_start_interface(uint8_t portid);
lagopus_result_t dpdk_stop_interface(uint8_t portid);
lagopus_result_t dpdk_get_hwaddr(uint8_t portid, uint8_t hw_addr[]);
lagopus_result_t
dpdk_get_stats(uint8_t portid, datastore_interface_stats_t *stats);
lagopus_result_t dpdk_clear_stats(uint8_t portid);
lagopus_result_t
dpdk_change_config(uint8_t portid, uint32_t advertised, uint32_t config);
lagopus_result_t
dpdk_queue_configure(struct interface *ifp, struct dp_ifqueue *ifqueue);
lagopus_result_t
dpdk_interface_queue_add(struct interface *ifp, dp_queue_info_t *queue);
lagopus_result_t
dpdk_interface_queue_delete(struct interface *ifp, uint32_t queue_id);
void dpdk_queue_unconfigure(struct interface *ifp);

lagopus_result_t rawsock_configure_interface(struct interface *ifp);
lagopus_result_t rawsock_unconfigure_interface(struct interface *ifp);
lagopus_result_t rawsock_start_interface(struct interface *ifp);
lagopus_result_t rawsock_stop_interface(struct interface *ifp);
lagopus_result_t rawsock_get_hwaddr(struct interface *ifp, uint8_t hw_addr[]);
lagopus_result_t
rawsock_get_stats(struct interface *ifp, datastore_interface_stats_t *stats);
lagopus_result_t rawsock_clear_stats(struct interface *ifp);
lagopus_result_t
rawsock_change_config(struct interface *ifp,
                      uint32_t advertised,
                      uint32_t config);


#endif /* SRC_INCLUDE_LAGOPUS_INTERFACE_H_ */
