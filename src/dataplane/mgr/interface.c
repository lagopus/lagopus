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
 * @file        interface.c
 * @brief       "interface" resource internal management routines.
 */

#include "lagopus_config.h"

#include "lagopus_apis.h"
#include "lagopus/interface.h"

#include "lagopus/ofp_dp_apis.h" /* for port_stats */
#ifdef HAVE_DPDK
#include "dpdk.h"
#endif /* HAVE_DPDK */

#ifdef HYBRID
#include "lagopus/mactable.h"
#include "tap_io.h"
#include "lagopus/rib.h"
#include "lagopus/ethertype.h"
#include "pktbuf.h"
#include "packet.h"

#include "lagopus/bridge.h"
#include "packet.h"

#define IPV4(a,b,c,d) ((uint32_t)(((a) & 0xff) << 24) | \
                       (((b) & 0xff) << 16) | \
                       (((c) & 0xff) << 8)  | \
                       ((d) & 0xff))
#define IPV4_BROADCAST ((uint32_t)0xFFFFFFFF)
#define IS_IPV4_BROADCAST(x) \
        ((x) == IPV4_BROADCAST)
#define IPV4_MULTICAST_MIN IPV4(224, 0, 0, 0)
#define IPV4_MULTICAST_MAX IPV4(239, 255, 255, 255)
#define IS_IPV4_MULTICAST(x) \
        ((x) >= IPV4_MULTICAST_MIN && (x) <= IPV4_MULTICAST_MAX)

#endif /* HYBRID */

static struct port_stats *
unknown_port_stats(struct port *port) {
  return calloc(1, sizeof(struct port_stats));
}

struct interface *
dp_interface_alloc(void) {
  struct interface *ifp;
  ifp = calloc(1, sizeof(struct interface));
  if (ifp != NULL) {
    ifp->stats = unknown_port_stats;
    ifp->addr_info.set = false;
  }
  return ifp;
}

lagopus_result_t
dp_interface_free(struct interface *ifp) {
  if (ifp == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  free(ifp);
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_interface_configure_internal(struct interface *ifp) {
  lagopus_result_t rv;

  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV:
#ifdef HAVE_DPDK
      rv = dpdk_configure_interface(ifp);
#else
      rv = LAGOPUS_RESULT_INVALID_ARGS;
#endif /* HAVE_DPDK */
      break;

    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      rv = rawsock_configure_interface(ifp);
      break;

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      rv = LAGOPUS_RESULT_OK;
      break;

    case DATASTORE_INTERFACE_TYPE_GRE:
    case DATASTORE_INTERFACE_TYPE_NVGRE:
    case DATASTORE_INTERFACE_TYPE_VXLAN:
    case DATASTORE_INTERFACE_TYPE_VHOST_USER:
      /* TODO */
      rv = LAGOPUS_RESULT_OK;
      break;

    default:
      rv = LAGOPUS_RESULT_INVALID_ARGS;
      break;
  }
  if (rv == LAGOPUS_RESULT_OK) {
    dp_interface_hw_addr_get_internal(ifp, ifp->hw_addr);
#ifdef HYBRID
    rv = dp_tap_interface_create(ifp->name, ifp);
#endif /* HYBRID */
  } else {
    ifp->info.type = DATASTORE_INTERFACE_TYPE_UNKNOWN;
  }

  return rv;
}

lagopus_result_t
dp_interface_unconfigure_internal(struct interface *ifp) {
  lagopus_result_t rv;

  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV:
#ifdef HAVE_DPDK
      rv = dpdk_unconfigure_interface(ifp);
#else
      rv = LAGOPUS_RESULT_OK;
#endif /* HAVE_DPDK */
      break;

    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      rv = rawsock_unconfigure_interface(ifp);
      break;

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      /* nothing to do. */
      rv = LAGOPUS_RESULT_OK;
      break;

    case DATASTORE_INTERFACE_TYPE_GRE:
    case DATASTORE_INTERFACE_TYPE_NVGRE:
    case DATASTORE_INTERFACE_TYPE_VXLAN:
    case DATASTORE_INTERFACE_TYPE_VHOST_USER:
      /* TODO */
      rv = LAGOPUS_RESULT_OK;
      break;

    default:
      rv = LAGOPUS_RESULT_OK;
      break;
  }
#ifdef HYBRID
  if (rv == LAGOPUS_RESULT_OK) {
    dp_tap_interface_destroy(ifp->name);
  }
#endif /* HYBRID */

  return rv;
}

lagopus_result_t
dp_interface_start_internal(struct interface *ifp) {
  lagopus_result_t rv;

  switch (ifp->info.type) {
#ifdef HAVE_DPDK
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV:
      rv = dpdk_start_interface(ifp->info.eth.port_number);
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      rv = rawsock_start_interface(ifp);
      break;

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      rv = LAGOPUS_RESULT_OK;
      break;

    case DATASTORE_INTERFACE_TYPE_GRE:
    case DATASTORE_INTERFACE_TYPE_NVGRE:
    case DATASTORE_INTERFACE_TYPE_VXLAN:
    case DATASTORE_INTERFACE_TYPE_VHOST_USER:
      rv = LAGOPUS_RESULT_OK;
      break;

    default:
      rv = LAGOPUS_RESULT_INVALID_ARGS;
      break;
  }
#ifdef HYBRID
  if (rv == LAGOPUS_RESULT_OK) {
    rv = dp_tap_start_interface(ifp->name);
  }
#endif /* HYBRID */

  return rv;
}

lagopus_result_t
dp_interface_stop_internal(struct interface *ifp) {
  if (ifp == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
#ifdef HYBRID
  dp_tap_stop_interface(ifp->name);
#endif /* HYBRID */
  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV:
#ifdef HAVE_DPDK
      return dpdk_stop_interface(ifp->info.eth.port_number);
#else
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      return rawsock_stop_interface(ifp);

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      break;

    case DATASTORE_INTERFACE_TYPE_GRE:
    case DATASTORE_INTERFACE_TYPE_NVGRE:
    case DATASTORE_INTERFACE_TYPE_VXLAN:
    case DATASTORE_INTERFACE_TYPE_VHOST_USER:
      /* TODO */
      break;

    default:
      break;
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_interface_rx_burst_internal(struct interface *ifp,
                               void *mbufs[], size_t nb) {
  if (ifp == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV:
#ifdef HAVE_DPDK
      return dpdk_rx_burst(ifp->info.eth.port_number, mbufs, nb);
#else
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      return rawsock_rx_burst(ifp, mbufs, nb);

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      break;

    case DATASTORE_INTERFACE_TYPE_GRE:
    case DATASTORE_INTERFACE_TYPE_NVGRE:
    case DATASTORE_INTERFACE_TYPE_VXLAN:
    case DATASTORE_INTERFACE_TYPE_VHOST_USER:
      /* TODO */
      break;

    default:
      break;
  }

  return LAGOPUS_RESULT_OK;
}

#ifdef HYBRID
lagopus_result_t
dp_interface_send_packet_kernel(struct lagopus_packet *pkt,
                                struct interface *ifp) {
  if (ifp->tap != NULL) {
    return dp_tap_interface_send_packet(ifp->tap, pkt);
  }
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
send_packet(struct lagopus_packet *pkt) {
  if (pkt->send_kernel) {
    dp_interface_send_packet_kernel(pkt, pkt->ifp);
  } else {
    lagopus_forward_packet_to_port_hybrid(pkt);
  }
}

/**
 * L2 witching main routine.
 */
static lagopus_result_t
interface_l2_switching(struct lagopus_packet *pkt,
                       struct interface *ifp) {
  uint32_t port;
  uint8_t *dst;
  uint8_t buf[ETHER_ADDR_LEN];

  dst = pkt->eth->ether_dhost;

  /* dst is self mac */
  if (!memcmp(OS_MTOD(PKT2MBUF(pkt), uint8_t *), ifp->hw_addr, ETHER_ADDR_LEN)) {
    return dp_interface_send_packet_kernel(pkt, ifp);
  }
  /* dst is broadcast or multicast */
  if ((dst[0] & 0x01) == 0x01) {
    OS_M_ADDREF(PKT2MBUF(pkt));
    dp_interface_send_packet_kernel(pkt, ifp);
  }

  /* l2 switching */
#if defined HYBRID && defined PIPELINER
  pkt->pipeline_context.pipeline_idx = L2_PIPELINE;
  pipeline_process(pkt);
#else
  mactable_port_learning(pkt);
  mactable_port_lookup(pkt);

  /* forwarding packet to output port */
  lagopus_forward_packet_to_port_hybrid(pkt);
#endif

  return LAGOPUS_RESULT_OK;
}

static bool
is_self_ip_addr(struct in_addr dst_addr, struct in_addr self_addr) {
  return (htonl(dst_addr.s_addr) == htonl(self_addr.s_addr)) ? true : false;
}

static bool
is_broadcast(struct in_addr dst_addr, struct in_addr broad_addr) {
  if (htonl(dst_addr.s_addr) == htonl(broad_addr.s_addr) ||
      IS_IPV4_BROADCAST(htonl(dst_addr.s_addr)) == true) {
    return true;
  } else {
    return false;
  }
}

/**
 * L3 routing main routine.
 */
static lagopus_result_t
interface_l3_routing(struct lagopus_packet *pkt,
                     struct interface *ifp) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  uint16_t ether_type;
  int prefix;

  ether_type = lagopus_get_ethertype(pkt);

  /* not IP packets, send to kernel. */
  if (ether_type != ETHERTYPE_IP && ether_type != ETHERTYPE_IPV6) {
    return dp_interface_send_packet_kernel(pkt, ifp);
  }

  if (ether_type == ETHERTYPE_IP) {
    struct in_addr dst_addr;
    struct in_addr self_addr;
    struct in_addr broad_addr;

    /* initialize l3 routing */
    pkt->ifp = ifp;
    pkt->send_kernel = false;

    /* get dst ip address from input packet */
    lagopus_get_ip(pkt, &dst_addr, AF_INET);

    /* check dst ip addr. */
    dp_interface_ip_get(ifp, AF_INET, &self_addr, &broad_addr, &prefix);
    if (is_self_ip_addr(dst_addr, self_addr) == true ||
        is_broadcast(dst_addr, broad_addr) == true ||
        IS_IPV4_MULTICAST(htonl(dst_addr.s_addr)) == true) {
      return dp_interface_send_packet_kernel(pkt, ifp);
    }

#if defined HYBRID && defined PIPELINER
    pkt->pipeline_context.pipeline_idx = L3_PIPELINE;
    pipeline_process(pkt);
#else
    /* learning l2 info */
    mactable_port_learning(pkt);

    /* l3 routing */
    rib_lookup(pkt);

    /* forwarding packet */
    send_packet(pkt);
#endif
  } else if (ether_type == ETHERTYPE_IPV6) {
    /* TODO: */
    lagopus_packet_free(pkt);
  } else {
    /* nothing to do. */
    lagopus_msg_info("not support packets. ethertype = %d\n", ether_type);
    lagopus_packet_free(pkt);
    return rv;
  }

  return rv;
}
#endif /* HYBRID */

lagopus_result_t
dp_interface_send_packet_normal(struct lagopus_packet *pkt,
                                struct interface *ifp,
                                bool l2_bridge) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
#ifdef HYBRID
  if (unlikely(l2_bridge != true)) {
    return rv;
  }

  /* check port type */
  if (ifp->addr_info.set) {
    /* set ip address = l3 port */
    rv = interface_l3_routing(pkt, ifp);
  } else {
    /* no ip address = l2 port */
    rv = interface_l2_switching(pkt, ifp);
  }
#else /* HYBRID */
  /* no hybrid mode, drop packets. */
  (void) pkt;
  (void) ifp;
  lagopus_packet_free(pkt);
#endif /* HYBRID */
  return rv;
}

lagopus_result_t
dp_interface_hw_addr_get_internal(struct interface *ifp, uint8_t *hw_addr) {
  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV:
#ifdef HAVE_DPDK
      return dpdk_get_hwaddr(ifp->info.eth.port_number, hw_addr);
#else
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      return rawsock_get_hwaddr(ifp, hw_addr);

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      return LAGOPUS_RESULT_OK;

    case DATASTORE_INTERFACE_TYPE_GRE:
    case DATASTORE_INTERFACE_TYPE_NVGRE:
    case DATASTORE_INTERFACE_TYPE_VXLAN:
    case DATASTORE_INTERFACE_TYPE_VHOST_USER:
      /* TODO */
      return LAGOPUS_RESULT_OK;

    default:
      break;
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

lagopus_result_t
dp_interface_stats_get_internal(struct interface *ifp,
                                datastore_interface_stats_t *stats) {
  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV:
#ifdef HAVE_DPDK
      return dpdk_get_stats(ifp->info.eth.port_number, stats);
#else
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      return rawsock_get_stats(ifp, stats);

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      return LAGOPUS_RESULT_OK;

    case DATASTORE_INTERFACE_TYPE_GRE:
    case DATASTORE_INTERFACE_TYPE_NVGRE:
    case DATASTORE_INTERFACE_TYPE_VXLAN:
    case DATASTORE_INTERFACE_TYPE_VHOST_USER:
      /* TODO */
      return LAGOPUS_RESULT_OK;

    default:
      break;
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

lagopus_result_t
dp_interface_stats_clear_internal(struct interface *ifp) {
  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV:
#ifdef HAVE_DPDK
      return dpdk_clear_stats(ifp->info.eth.port_number);
#else
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      return rawsock_clear_stats(ifp);

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      return LAGOPUS_RESULT_OK;

    case DATASTORE_INTERFACE_TYPE_GRE:
    case DATASTORE_INTERFACE_TYPE_NVGRE:
    case DATASTORE_INTERFACE_TYPE_VXLAN:
    case DATASTORE_INTERFACE_TYPE_VHOST_USER:
      /* TODO */
      return LAGOPUS_RESULT_OK;

    default:
      break;
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

lagopus_result_t
dp_interface_change_config(struct interface *ifp,
                           uint32_t advertised,
                           uint32_t config) {
  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV:
#ifdef HAVE_DPDK
      return dpdk_change_config(ifp->info.eth.port_number, advertised, config);
#else
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      return rawsock_change_config(ifp, advertised, config);

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      return LAGOPUS_RESULT_OK;

    case DATASTORE_INTERFACE_TYPE_GRE:
    case DATASTORE_INTERFACE_TYPE_NVGRE:
    case DATASTORE_INTERFACE_TYPE_VXLAN:
    case DATASTORE_INTERFACE_TYPE_VHOST_USER:
      /* TODO */
      return LAGOPUS_RESULT_OK;

    default:
      break;
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

lagopus_result_t
dp_interface_queue_configure(struct interface *ifp) {
  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV:
#ifdef HAVE_DPDK
      return dpdk_queue_configure(ifp, &ifp->ifqueue);
#else
      break;
#endif

    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      return LAGOPUS_RESULT_OK;

    case DATASTORE_INTERFACE_TYPE_GRE:
    case DATASTORE_INTERFACE_TYPE_NVGRE:
    case DATASTORE_INTERFACE_TYPE_VXLAN:
    case DATASTORE_INTERFACE_TYPE_VHOST_USER:
      /* TODO */
      return LAGOPUS_RESULT_OK;

    default:
      break;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

lagopus_result_t
dp_interface_queue_add(struct interface *ifp, dp_queue_info_t *queue) {
  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV:
#ifdef HAVE_DPDK
      return dpdk_interface_queue_add(ifp, queue);
#else
      break;
#endif

    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      return LAGOPUS_RESULT_OK;

    case DATASTORE_INTERFACE_TYPE_GRE:
    case DATASTORE_INTERFACE_TYPE_NVGRE:
    case DATASTORE_INTERFACE_TYPE_VXLAN:
    case DATASTORE_INTERFACE_TYPE_VHOST_USER:
      /* TODO */
      return LAGOPUS_RESULT_OK;

    default:
      break;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

lagopus_result_t
dp_interface_queue_delete(struct interface *ifp, uint32_t queue_id) {
  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV:
#ifdef HAVE_DPDK
      return dpdk_interface_queue_delete(ifp, queue_id);
#else
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      return LAGOPUS_RESULT_OK;

    case DATASTORE_INTERFACE_TYPE_GRE:
    case DATASTORE_INTERFACE_TYPE_NVGRE:
    case DATASTORE_INTERFACE_TYPE_VXLAN:
    case DATASTORE_INTERFACE_TYPE_VHOST_USER:
      /* TODO */
      return LAGOPUS_RESULT_OK;

    default:
      break;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}
