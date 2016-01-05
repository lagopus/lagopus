/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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

#include "lagopus_apis.h"
#include "lagopus/interface.h"

#include "lagopus/ofp_dp_apis.h" /* for port_stats */

#ifdef HYBRID
#include "tap_io.h"
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

  rv = LAGOPUS_RESULT_INVALID_ARGS;
  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PCAP:
#ifdef HAVE_DPDK
      rv = dpdk_configure_interface(ifp);
#endif
      break;
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      rv = rawsock_configure_interface(ifp);
      break;

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      rv = LAGOPUS_RESULT_OK;
      break;

    case DATASTORE_INTERFACE_TYPE_VXLAN:
      /* TODO */
      rv = LAGOPUS_RESULT_OK;
      break;

    default:
      break;
  }
  if (rv == LAGOPUS_RESULT_OK) {
    dp_interface_hw_addr_get_internal(ifp, ifp->hw_addr);
#ifdef HYBRID
    rv = dp_tap_interface_create(ifp->name, ifp);
#endif /* HYBRID */
  }

  return rv;
}

lagopus_result_t
dp_interface_unconfigure_internal(struct interface *ifp) {
  lagopus_result_t rv;

  rv = LAGOPUS_RESULT_OK;
  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PCAP:
#ifdef HAVE_DPDK
      rv = dpdk_unconfigure_interface(ifp);
#endif
      break;

    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      rv = rawsock_unconfigure_interface(ifp);
      break;

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      rv = LAGOPUS_RESULT_OK;
      break;

    case DATASTORE_INTERFACE_TYPE_VXLAN:
      /* TODO */
      rv = LAGOPUS_RESULT_OK;
      break;

    default:
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
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PCAP:
      rv = dpdk_start_interface(ifp->info.eth.port_number);
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      rv = rawsock_start_interface(ifp);
      break;

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
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
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PCAP:
#ifdef HAVE_DPDK
      return dpdk_stop_interface(ifp->info.eth.port_number);
#else
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      return rawsock_stop_interface(ifp);

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      break;

    default:
      break;
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_interface_send_packet_normal(struct lagopus_packet *pkt,
                                struct interface *ifp) {
#ifdef HYBRID
  if (ifp->tap != NULL) {
    return dp_tap_interface_send_packet(ifp->tap, pkt);
  }
#else
  (void) pkt;
  (void) ifp;
#endif /* HYBRID */
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_interface_hw_addr_get_internal(struct interface *ifp, uint8_t *hw_addr) {
  switch (ifp->info.type) {
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PCAP:
#ifdef HAVE_DPDK
      return dpdk_get_hwaddr(ifp->info.eth.port_number, hw_addr);
#else
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      return rawsock_get_hwaddr(ifp, hw_addr);

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
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
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PCAP:
#ifdef HAVE_DPDK
      return dpdk_get_stats(ifp->info.eth.port_number, stats);
#else
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      return rawsock_get_stats(ifp, stats);

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
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
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PCAP:
#ifdef HAVE_DPDK
      return dpdk_clear_stats(ifp->info.eth.port_number);
#else
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      return rawsock_clear_stats(ifp);

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
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
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PCAP:
#ifdef HAVE_DPDK
      return dpdk_change_config(ifp->info.eth.port_number, advertised, config);
#else
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      return rawsock_change_config(ifp, advertised, config);

    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
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
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PCAP:
#ifdef HAVE_DPDK
      return dpdk_queue_configure(ifp, &ifp->ifqueue);
#else
      break;
#endif

    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
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
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PCAP:
#ifdef HAVE_DPDK
      return dpdk_interface_queue_add(ifp, queue);
#else
      break;
#endif

    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
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
    case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PCAP:
#ifdef HAVE_DPDK
      return dpdk_interface_queue_delete(ifp, queue_id);
#else
      break;
#endif
    case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
    case DATASTORE_INTERFACE_TYPE_UNKNOWN:
      return LAGOPUS_RESULT_OK;

    default:
      break;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}
