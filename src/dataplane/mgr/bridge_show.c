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
#include "../agent/channel.h"
#include "../agent/channel_mgr.h"
#include "bridge_show.h"

/* We must have better way to access to dpmgr information. */
extern struct dpmgr *dpmgr;

/* Flag check macro. */
#define CHECK_FLAG(V,F)      ((V) & (F))

#ifdef LAGOPUS_BIG_ENDIAN
#define OS_NTOHLL(x) (x)
#else /* Little endian */
#define OS_NTOHLL(x) ((uint64_t)ntohl((x) & 0xffffffffLLU) << 32 \
                      | ntohl((x) >> 32 & 0xffffffffLLU))
#endif

static void
show_bridge(struct confsys *confsys, struct bridge *bridge) {
  uint32_t i;
  uint8_t *p;
  uint16_t val16;
  uint64_t val64;
  uint32_t capabilities;

  /* Bridge name. */
  show(confsys, "bridge: %s\n", bridge->name);

  /* Datapath ID. */
  val64 = OS_NTOHLL(bridge->dpid);
  p = (uint8_t *)&val64;
  val16 = *(uint16_t *)p;
  p += 2;
  show(confsys, "  datapnath id: %u.%02x:%02x:%02x:%02x:%02x:%02x\n",
       val16, p[0], p[1], p[2], p[3], p[4], p[5]);

  /* Number of buffers and tables. */
  show(confsys, "  max packet buffers: %d,", bridge->features.n_buffers);
  show(confsys, " number of tables: %d\n", bridge->features.n_tables);

  /* Capabilities. */
  capabilities = bridge->features.capabilities;
  show(confsys, "  capabilities:");
  show(confsys, " flow_stats %s",
       CHECK_FLAG(capabilities, OFPC_FLOW_STATS) ? "on":"off");
  show(confsys, ", table_stats %s",
       CHECK_FLAG(capabilities, OFPC_TABLE_STATS) ? "on":"off");
  show(confsys, ", port_stats %s",
       CHECK_FLAG(capabilities, OFPC_PORT_STATS) ? "on":"off");
  show(confsys, ", group_stats %s",
       CHECK_FLAG(capabilities, OFPC_GROUP_STATS) ? "on":"off");
  show(confsys, "\n               ");
  show(confsys, " ip_reasm %s",
       CHECK_FLAG(capabilities, OFPC_IP_REASM) ? "on":"off");
  show(confsys, ", queue_stats %s",
       CHECK_FLAG(capabilities, OFPC_QUEUE_STATS) ? "on":"off");
  show(confsys, ", port_blocked %s",
       CHECK_FLAG(capabilities, OFPC_PORT_BLOCKED) ? "on":"off");
  show(confsys, "\n");

  /* fail-mode. */
  switch (bridge->fail_mode) {
    case FAIL_SECURE_MODE:
      show(confsys, "  fail-mode: secure-mode\n");
      break;
    case FAIL_STANDALONE_MODE:
      show(confsys, "  fail-mode: standalone-mode (default)\n");
      break;
    default:
      show(confsys, "  fail-mode: unknown\n");
      break;
  }

  /* Ports. */
  for (i = 0; i <= vector_max(bridge->ports); i++) {
    struct port *port;
    port = vector_slot(bridge->ports, i);
    if (port != NULL) {
      show(confsys, "port: %s: ifindex %d, OpenFlow Port %d\n",
           port->ofp_port.name, port->ifindex, port->ofp_port.port_no);
    }
  }
}

CALLBACK(show_bridge_func) {
  struct bridge *bridge;
  ARG_USED();

  /* Without bridge name. */
  if (argc == 2) {
    TAILQ_FOREACH(bridge, &dpmgr->bridge_list, entry) {
      show_bridge(confsys, bridge);
    }
  }

  /* With bridge name. */
  if (argc == 3) {
    bridge = bridge_lookup(&dpmgr->bridge_list, argv[2]);
    if (bridge == NULL) {
      show(confsys, "%% Can't find bridge %s\n", argv[2]);
    } else {
      show_bridge(confsys, bridge);
    }
  }

  return 0;
}

static lagopus_result_t
show_controller_addr_func(struct channel *chan, void *arg) {
  struct confsys *confsys = arg;
  struct addrunion addr;
  enum channel_protocol protocol;
  char ipaddr[32];
  uint16_t portno;

  show(confsys, "<controller>\n");

  if (channel_addr_get(chan, &addr) == LAGOPUS_RESULT_OK &&
      addrunion_ipaddr_str_get(&addr,
                               ipaddr, sizeof(ipaddr)) != NULL &&
      strcmp(ipaddr, "0.0.0.0") != 0) {
    show(confsys, "<ip-address>%s</ip-address>\n",
         ipaddr);
  }
  if (channel_port_get(chan, &portno) == LAGOPUS_RESULT_OK) {
    show(confsys, "<port>%d</port>\n", portno);
  }
  if (channel_local_addr_get(chan, &addr) == LAGOPUS_RESULT_OK &&
      addrunion_ipaddr_str_get(&addr, ipaddr, sizeof(ipaddr))  != NULL &&
      strcmp(ipaddr, "0.0.0.0") != 0) {
    show(confsys, "<local-ip-address>%s</local-ip-address>\n",
         ipaddr);
  }
  if (channel_local_port_get(chan, &portno) == LAGOPUS_RESULT_OK &&
      portno != 0) {
    show(confsys, "<local-port>%d</local-port>\n", portno);
  }
  if (channel_protocol_get(chan, &protocol) == LAGOPUS_RESULT_OK) {
    show(confsys, "<protocol>");
    switch (protocol) {
      case PROTOCOL_TCP:
      case PROTOCOL_TCP6:
        show(confsys, "tcp");
        break;
      case PROTOCOL_TLS:
      case PROTOCOL_TLS6:
      default:
        show(confsys, "tls");
        break;
    }
    show(confsys, "</protocol>\n");
  }

  show(confsys, "</controller>\n");

  return LAGOPUS_RESULT_OK;
}

void
show_logical_switch_func(struct confsys *confsys) {
  struct bridge *bridge;
  struct channel_list *chan_list;
  struct port *port;
  vindex_t i, max;
  uint32_t capabilities;
  lagopus_result_t ret;

  /* Logical switch. */
  show(confsys, "<logical-switches>\n");
  TAILQ_FOREACH(bridge, &dpmgr->bridge_list, entry) {
    show(confsys, "<switch>\n");
    show(confsys, "<id>%s</id>\n", bridge->name);

    /* Capabilities. */
    show(confsys, "<capabilities>\n");
    show(confsys, "<max-buffered-packets>%d</max-buffered-packets>\n",
         bridge->features.n_buffers);
    show(confsys, "<max-tables>%d</max-tables>\n",
         bridge->features.n_tables);
    show(confsys, "<max-ports>2048</max-ports>\n");

    capabilities = bridge->features.capabilities;
    if (CHECK_FLAG(capabilities, OFPC_FLOW_STATS)) {
      show(confsys, "<flow-statistics>true</flow-statistics>\n");
    } else {
      show(confsys, "<flow-statistics>false</flow-statistics>\n");
    }
    if (CHECK_FLAG(capabilities, OFPC_TABLE_STATS)) {
      show(confsys, "<table-statistics>true</table-statistics>\n");
    } else {
      show(confsys, "<table-statistics>false</table-statistics>\n");
    }
    if (CHECK_FLAG(capabilities, OFPC_PORT_STATS)) {
      show(confsys, "<port-statistics>true</port-statistics>\n");
    } else {
      show(confsys, "<port-statistics>false</port-statistics>\n");
    }
    if (CHECK_FLAG(capabilities, OFPC_GROUP_STATS)) {
      show(confsys, "<group-statistics>true</group-statistics>\n");
    } else {
      show(confsys, "<group-statistics>false</group-statistics>\n");
    }
    if (CHECK_FLAG(capabilities, OFPC_QUEUE_STATS)) {
      show(confsys, "<queue-statistics>true</queue-statistics>\n");
    } else {
      show(confsys, "<queue-statistics>false</queue-statistics>\n");
    }
    if (CHECK_FLAG(capabilities, OFPC_IP_REASM)) {
      show(confsys, "<reassemble-ip-fragments>true</reassemble-ip-fragments>\n");
    } else {
      show(confsys, "<reassemble-ip-fragments>false</reassemble-ip-fragments>\n");
    }
    if (CHECK_FLAG(capabilities, OFPC_PORT_BLOCKED)) {
      show(confsys, "<block-looping-ports>true</block-looping-ports>\n");
    } else {
      show(confsys, "<block-looping-ports>false</block-looping-ports>\n");
    }

    show(confsys, "<reserved-port-types>\n");
    show(confsys, "<type>all</type>\n");
    show(confsys, "</reserved-port-types>\n");

    show(confsys, "<group-types>\n");
    show(confsys, "<type>all</type>\n");
    show(confsys, "</group-types>\n");

    show(confsys, "<group-capabilities>\n");
    show(confsys, "<capability>select-weight</capability>\n");
    show(confsys, "</group-capabilities>\n");

    show(confsys, "<action-types>\n");
    show(confsys, "<type>output</type>\n");
    show(confsys, "<type>copy-ttl-out</type>\n");
    show(confsys, "<type>copy-ttl-in</type>\n");
    show(confsys, "<type>set-mpls-ttl</type>\n");
    show(confsys, "<type>dec-mpls-ttl</type>\n");
    show(confsys, "<type>push-vlan</type>\n");
    show(confsys, "<type>pop-vlan</type>\n");
    show(confsys, "<type>push-mpls</type>\n");
    show(confsys, "<type>pop-mpls</type>\n");
    show(confsys, "<type>set-queue</type>\n");
    show(confsys, "<type>group</type>\n");
    show(confsys, "<type>set-nw-ttl</type>\n");
    show(confsys, "<type>dec-nw-ttl</type>\n");
    show(confsys, "<type>set-field</type>\n");
    show(confsys, "</action-types>\n");

    show(confsys, "<instruction-types>\n");
    show(confsys, "<type>apply-actions</type>\n");
    show(confsys, "<type>clear-actions</type>\n");
    show(confsys, "<type>write-actions</type>\n");
    show(confsys, "<type>write-metadata</type>\n");
    show(confsys, "<type>goto-table</type>\n");
    show(confsys, "</instruction-types>\n");

    show(confsys, "</capabilities>\n");

    ret = channel_mgr_channels_lookup_by_dpid(bridge->dpid, &chan_list);
    if (ret == LAGOPUS_RESULT_OK) {
      show(confsys, "<controllers>\n");
      ret = channel_list_iterate(chan_list,
                                 show_controller_addr_func,
                                 (void *)confsys);
      show(confsys, "</controllers>\n");
    }

    show(confsys, "<resources>\n");
    max = bridge->ports->allocated;
    for (i = 0; i < max; i++) {
      port = vector_slot(bridge->ports, i);
      if (port == NULL) {
        continue;
      }
      show(confsys, "<port>\n");
      if (port->type == LAGOPUS_PORT_TYPE_PHYSICAL) {
        show(confsys, "<resource-id>Port%d</resource-id>\n",
             port->ofp_port.port_no);
      }
      show(confsys, "<number>%d</number>\n", port->ofp_port.port_no);
      show(confsys, "<name>%s</name>\n", port->ofp_port.name);
      show(confsys, "</port>\n");
    }
    show(confsys, "</resources>\n");

    show(confsys, "</switch>\n");
  }
  show(confsys, "</logical-switches>\n");
}
