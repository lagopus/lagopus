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


#include <stdio.h>
#include <stdint.h>

#include "confsys.h"
#include "lagopus/dpmgr.h"
#include "lagopus/port.h"

/* We must have better way to access to dpmgr information. */
extern struct dpmgr *dpmgr;

static void
show_port_config_string(struct confsys *confsys, uint32_t config) {
  if ((config & OFPPC_NO_RECV) != 0) {
    show(confsys, "NO_RECV ");
  }
  if ((config & OFPPC_NO_FWD) != 0) {
    show(confsys, "NO_FWD ");
  }
  if ((config & OFPPC_NO_PACKET_IN) != 0) {
    show(confsys, "NO_PACKET_IN ");
  }
  if (config == 0) {
    show(confsys, "no restricted");
  }
}

static void
show_port_state_string(struct confsys *confsys, uint32_t state) {
  if ((state & OFPPS_LINK_DOWN) != 0) {
    show(confsys, "LINK DOWN");
  } else {
    show(confsys, "LINK UP");
  }
  if ((state & OFPPS_BLOCKED) != 0) {
    show(confsys, ", BLOCKED");
  }
  if ((state & OFPPS_LIVE) != 0) {
    show(confsys, ", LIVE");
  }
}

static void
show_port_stats(struct confsys *confsys, struct port *port) {
  struct port_stats *stats;

  show(confsys, "%s:\n", port->ofp_port.name);
  show(confsys, " Description:\n");
  show(confsys, "  OpenFlow Port: %d\n", port->ofp_port.port_no);
  show(confsys, "  Hardware Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
       port->ofp_port.hw_addr[0],
       port->ofp_port.hw_addr[1],
       port->ofp_port.hw_addr[2],
       port->ofp_port.hw_addr[3],
       port->ofp_port.hw_addr[4],
       port->ofp_port.hw_addr[5]);
  if (port->type == LAGOPUS_PORT_TYPE_PHYSICAL) {
#ifdef HAVE_DPDK
    show(confsys, "  PCI Address: %04x:%02x:%02x.%d\n",
         port->eth.pci_addr.domain,
         port->eth.pci_addr.bus,
         port->eth.pci_addr.devid,
         port->eth.pci_addr.function);
#endif /* HAVE_DPDK */
  }
  show(confsys, "  Config: ");
  show_port_config_string(confsys, port->ofp_port.config);
  show(confsys, "\n");
  show(confsys, "  State: ");
  show_port_state_string(confsys, port->ofp_port.state);
  show(confsys, "\n");
  show(confsys, " Statistics:\n");
  stats = port->stats(port);
  show(confsys, "  rx_packets: %qd\n", port->ofp_port_stats.rx_packets);
  show(confsys, "  tx_packets: %qd\n", port->ofp_port_stats.tx_packets);
  show(confsys, "  rx_bytes:   %qd\n", port->ofp_port_stats.rx_bytes);
  show(confsys, "  tx_bytes:   %qd\n", port->ofp_port_stats.tx_bytes);
  show(confsys, "  rx_dropped: %qd\n", port->ofp_port_stats.rx_dropped);
  show(confsys, "  tx_dropped: %qd\n", port->ofp_port_stats.tx_dropped);
  show(confsys, "  rx_error:   %qd\n", port->ofp_port_stats.rx_errors);
  show(confsys, "  tx_error:   %qd\n", port->ofp_port_stats.tx_errors);
  free(stats);
}

CALLBACK(show_interface_func) {
  struct port *port;
  vindex_t i, max;

  ARG_USED();

  if (!strcmp(argv[2], "all")) {
    max = dpmgr->ports->allocated;
    for (i = 0; i < max; i++) {
      port = vector_slot(dpmgr->ports, i);
      if (port == NULL) {
        continue;
      }
      /* XXX read lock */
      show_port_stats(confsys, port);
      /* XXX unlock */
    }
  } else {
    max = dpmgr->ports->allocated;
    for (i = 0; i < max; i++) {
      port = vector_slot(dpmgr->ports, i);
      if (port == NULL) {
        continue;
      }
      if (!strcmp(argv[2], port->ofp_port.name)) {
        /* XXX read lock */
        show_port_stats(confsys, port);
        /* XXX unlock */
        break;
      }
    }
  }
  return 0;
}
