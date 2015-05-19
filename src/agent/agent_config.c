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


/* OpenFlow agent configuration. */
#include "lagopus_apis.h"
#include "lagopus_version.h"
#include "confsys.h"
#include "agent_config.h"
#include "lagopus/dpmgr.h"
#include "lagopus/port.h"
#include "lagopus_gstate.h"
#include "lagopus/ofp_handler.h"
#include "channel.h"
#include "channel_mgr.h"
#include "lagopus/ofp_bridgeq_mgr.h"
#include "lagopus/session_tls.h"
#include "checkcert.h"

#define TIMEOUT_REGISTER_BRIDGE        (1500*1000*1000) /* 1.5sec */
#define DEFAULT_TLS_CERTIFICATE_STORE  CONFDIR "/ca"
#define DEFAULT_TLS_CERTIFICATE_FILE   CONFDIR "/ca/cacert.pem"
#define DEFAULT_TLS_PRIVATE_KEY        CONFDIR "/ca/key.pem"
#define DEFAULT_TLS_TRUST_POINT_CONF   CONFDIR "/ca/check.conf"

extern struct dpmgr *dpmgr;

/**
 * Initialize the configuration tree related to the agent.
 */
void
agent_initialize_config_tree(void) {
  char *value;
  /* TLS */
  if ((value = config_get("tls certificate-store WORD")) == NULL) {
    config_set_default("tls certificate-store WORD",
                       (char *) DEFAULT_TLS_CERTIFICATE_STORE);
    value = config_get("tls certificate-store WORD");
  }
  session_tls_ca_dir_set(value);

  if ((value = config_get("tls certificate-file WORD")) == NULL) {
    config_set_default("tls certificate-file WORD",
                       (char *) DEFAULT_TLS_CERTIFICATE_FILE);
    value = config_get("tls certificate-file WORD");
  }
  session_tls_cert_set(value);

  if ((value = config_get("tls private-key WORD")) == NULL) {
    config_set_default("tls private-key WORD",
                       (char *) DEFAULT_TLS_PRIVATE_KEY);
    value = config_get("tls private-key WORD");
  }
  session_tls_key_set(value);

  if ((value = config_get("tls trust-point-conf WORD")) == NULL) {
    config_set_default("tls trust-point-conf WORD",
                       (char *) DEFAULT_TLS_TRUST_POINT_CONF);
    value = config_get("tls trust-point-conf WORD");
  }
  agent_check_certificates_set_config_file(value);
}


CALLBACK(show_version_func) {
  ARG_USED();
  show(confsys, "%s version %d.%d.%d%s\n",
       LAGOPUS_PRODUCT_NAME,
       LAGOPUS_VERSION_MAJOR,
       LAGOPUS_VERSION_MINOR,
       LAGOPUS_VERSION_PATCH,
       LAGOPUS_VERSION_RELEASE);
  return CONFIG_SUCCESS;
}

CALLBACK(config_default_func) {
  ARG_USED();
  return CONFIG_SUCCESS;
}

/* Bridge add and delete callback. */
CALLBACK(bridge_domain_func) {
  lagopus_result_t result;
  uint64_t dpid;
  ARG_USED();

  /* Bridge name must start from "br". */
  if (strncmp(argv[1], "br", 2) != 0) {
    return CONFIG_FAILURE;
  }

  /* Same name bridge check. */
  result = dpmgr_bridge_dpid_lookup(dpmgr, argv[1], &dpid);
  if (confsys->type == CONFSYS_MSG_TYPE_SET) {

    if (result != LAGOPUS_RESULT_NOT_FOUND) {
      return CONFIG_ALREADY_EXIST;
    }

    /* Generate dpid. */
    dpid = dpmgr_dpid_generate();

    /* Add agent's bridge. */
    result = ofp_bridgeq_mgr_bridge_register(dpid);
    if (result != LAGOPUS_RESULT_OK) {
      lagopus_msg_fatal("FAILED (%s).\n",
                        lagopus_error_get_string(result));
      return CONFIG_FAILURE;
    }

    /* Add dpmgr's bridge. */
    result = dpmgr_bridge_add(dpmgr, argv[1], dpid);
    if (result != LAGOPUS_RESULT_OK) {
      lagopus_msg_fatal("FAILED (%s).\n",
                        lagopus_error_get_string(result));
      return CONFIG_FAILURE;
    }
    lagopus_msg_info("Add bridge(dpid: %"PRIx64")\n", dpid);
  } else if (confsys->type == CONFSYS_MSG_TYPE_UNSET) {
    if (result == LAGOPUS_RESULT_NOT_FOUND) {
      return CONFIG_SUCCESS;
    }
    /* Delete bridge. */
    result = dpmgr_bridge_delete(dpmgr, argv[1]);
    if (result != LAGOPUS_RESULT_OK) {
      lagopus_msg_fatal("FAILED (%s).\n",
                        lagopus_error_get_string(result));
      return CONFIG_FAILURE;
    }

    /* Delete agent's bridge. */
    result = ofp_bridgeq_mgr_bridge_unregister(dpid);
    if (result != LAGOPUS_RESULT_OK) {
      lagopus_msg_fatal("FAILED (%s).\n",
                        lagopus_error_get_string(result));
      return CONFIG_FAILURE;
    }
    lagopus_msg_info("Delete bridge(dpid: %"PRIx64")\n", dpid);
  }

  return CONFIG_SUCCESS;
}

CALLBACK(bridge_domain_dpid_func) {
  lagopus_result_t result;
  int num;
  unsigned int digit;
  unsigned int m1, m2, m3, m4, m5, m6;
  uint64_t old_dpid, dpid = 0;
  ARG_USED();

  if (confsys->type == CONFSYS_MSG_TYPE_SET) {
    num = sscanf(argv[3], "%u.%02x:%02x:%02x:%02x:%02x:%02x",
                 &digit, &m1, &m2, &m3, &m4, &m5, &m6);

    if (num != 7) {
      return CONFIG_FAILURE;
    }

    dpid = digit;
    dpid <<= 8;
    dpid += m1;
    dpid <<= 8;
    dpid += m2;
    dpid <<= 8;
    dpid += m3;
    dpid <<= 8;
    dpid += m4;
    dpid <<= 8;
    dpid += m5;
    dpid <<= 8;
    dpid += (m6);
  } else if (confsys->type == CONFSYS_MSG_TYPE_UNSET) {
    dpid = dpmgr_dpid_generate();
  }

  result = dpmgr_bridge_dpid_lookup(dpmgr, argv[1], &old_dpid);
  if (result == LAGOPUS_RESULT_OK) {
    result = ofp_bridgeq_mgr_bridge_unregister(old_dpid);
    if (result != LAGOPUS_RESULT_OK) {
      lagopus_msg_fatal("FAILED (%s).\n",
                        lagopus_error_get_string(result));
      return CONFIG_FAILURE;
    }

    result = dpmgr_bridge_dpid_set(dpmgr, argv[1], dpid);
    if (result != LAGOPUS_RESULT_OK) {
      lagopus_msg_fatal("FAILED (%s).\n",
                        lagopus_error_get_string(result));
      return CONFIG_FAILURE;
    }
    result = ofp_bridgeq_mgr_bridge_register(dpid);
    if (result != LAGOPUS_RESULT_OK) {
      lagopus_msg_fatal("FAILED (%s).\n",
                        lagopus_error_get_string(result));
      return CONFIG_FAILURE;
    }
  }

  return CONFIG_SUCCESS;
}

CALLBACK(bridge_domain_fail_secure_mode_func) {
  struct bridge *bridge;
  ARG_USED();

  bridge = dpmgr_bridge_lookup(dpmgr, argv[1]);
  if (bridge == NULL) {
    return CONFIG_FAILURE;
  }
  if (confsys->type == CONFSYS_MSG_TYPE_SET) {
    bridge_fail_mode_set(bridge, FAIL_SECURE_MODE);
  } else if (confsys->type == CONFSYS_MSG_TYPE_UNSET) {
    bridge_fail_mode_set(bridge, FAIL_STANDALONE_MODE);
  }

  return CONFIG_SUCCESS;
}

static int
controller_func_internal(const char *bridge_str, const char *controller_str,
                         uint16_t type,
                         lagopus_result_t (*func)(struct bridge *, uint64_t dpid,
                             struct addrunion *)) {
  struct bridge *bridge;
  struct addrunion addr;
  lagopus_result_t result;
  uint64_t dpid = 0x00;

  /* Lookup bridge. */
  bridge = dpmgr_bridge_lookup(dpmgr, bridge_str);
  if (! bridge) {
    return CONFIG_FAILURE;
  }

  result = dpmgr_bridge_dpid_lookup(dpmgr, bridge_str, &dpid);
  if (result != LAGOPUS_RESULT_OK ) {
    return CONFIG_FAILURE;
  }

  addrunion_ipv4_set(&addr, controller_str);

  if (type == CONFSYS_MSG_TYPE_SET) {
    struct channel *chan;

    /* Add controller to dpmgr. */
    dpmgr_controller_add(dpmgr, bridge_str, controller_str);

    /* Register channel to dpmgr. */
    result = func(bridge, dpid, &addr);
    if (result != LAGOPUS_RESULT_OK) {
      lagopus_perror(result);
      return CONFIG_FAILURE;
    }

    /* Lookup channel from channel mgr and return. */
    result = channel_mgr_channel_lookup(bridge, &addr, &chan);
    if (result != LAGOPUS_RESULT_OK) {
      lagopus_perror(result);
      return CONFIG_FAILURE;
    }

  } else if (type == CONFSYS_MSG_TYPE_UNSET) {
    /* Delete controller from dpmgr. */
    dpmgr_controller_delete(dpmgr, bridge_str, controller_str);

    /* Remove channel from channel mgr. */
    result = channel_mgr_channel_delete(bridge, &addr);
    if (result != LAGOPUS_RESULT_OK) {
      lagopus_perror(result);
      return CONFIG_FAILURE;
    }
  }

  return CONFIG_SUCCESS;
}

CALLBACK(controller_func) {
  const char *bridge_str;
  const char *controller_str;
  ARG_USED();

  if (argc != 4) {
    return CONFIG_SUCCESS;
  }

  bridge_str = argv[1];
  controller_str = argv[3];

  lagopus_msg_info("bridge: [%s], controller: [%s]\n", bridge_str,
                   controller_str);

  return controller_func_internal(bridge_str,
                                  controller_str, confsys->type, channel_mgr_channel_add);
}

CALLBACK(controller_tls_func) {
  const char *bridge_str;
  const char *controller_str;
  ARG_USED();

  if (argc != 6 || strncmp(argv[5], "tls", strlen("tls")) != 0) {
    return CONFIG_FAILURE;
  }

  bridge_str = argv[1];
  controller_str = argv[3];

  lagopus_msg_info("bridge: [%s], controller: [%s]\n", bridge_str,
                   controller_str);

  return controller_func_internal(bridge_str,
                                  controller_str, confsys->type, channel_mgr_channel_tls_add);
}

CALLBACK(controller_protocol_func) {
  struct bridge *bridge;
  struct channel *chan;
  const char *bridge_str;
  const char *controller_str;
  const char *protocol_str;
  ARG_USED();

  bridge_str = argv[1];
  controller_str = argv[3];
  protocol_str = argv[5];

  if (confsys->type == CONFSYS_MSG_TYPE_SET) {
    struct addrunion addr;
    lagopus_result_t result;

    /* Lookup bridge. */
    bridge = dpmgr_bridge_lookup(dpmgr, bridge_str);
    if (! bridge) {
      return CONFIG_FAILURE;
    }

    /* Register channel to dpmgr. */
    addrunion_ipv4_set(&addr, controller_str);
    result = channel_mgr_channel_lookup(bridge, &addr, &chan);
    if (result != LAGOPUS_RESULT_OK) {
      lagopus_perror(result);
      return CONFIG_FAILURE;
    }
    if (!strcmp(protocol_str, "tls")) {
      channel_protocol_set(chan, PROTOCOL_TLS);
    } else if (!strcmp(protocol_str, "tcp")) {
      channel_protocol_set(chan, PROTOCOL_TCP);
    } else {
      return CONFIG_FAILURE;
    }
  } else if (confsys->type == CONFSYS_MSG_TYPE_UNSET) {
    /* TODO: removing protocol. */
  }

  return CONFIG_SUCCESS;
}

CALLBACK(controller_port_func) {
  struct bridge *bridge;
  struct channel *chan;
  const char *bridge_str;
  const char *controller_str;
  const char *port_str;
  int port;
  lagopus_result_t ret;
  ARG_USED();

  bridge_str = argv[1];
  controller_str = argv[3];
  port_str = argv[5];

  if (confsys->type == CONFSYS_MSG_TYPE_SET) {
    struct addrunion addr;
    lagopus_result_t result;

    /* Lookup bridge. */
    bridge = dpmgr_bridge_lookup(dpmgr, bridge_str);
    if (! bridge) {
      return CONFIG_FAILURE;
    }

    /* Register channel to dpmgr. */
    addrunion_ipv4_set(&addr, controller_str);
    result = channel_mgr_channel_lookup(bridge, &addr, &chan);
    if (result != LAGOPUS_RESULT_OK) {
      lagopus_perror(result);
      return CONFIG_FAILURE;
    }
    ret = sscanf(port_str, "%d", &port);
    if (ret != 1) {
      return CONFIG_FAILURE;
    }
    channel_port_set(chan, (uint16_t) port);
  } else if (confsys->type == CONFSYS_MSG_TYPE_UNSET) {
    /* TODO: removing port. */
  }

  return CONFIG_SUCCESS;
}

CALLBACK(controller_local_address_func) {
  struct bridge *bridge;
  struct channel *chan;
  const char *bridge_str;
  const char *controller_str;
  const char *localaddr_str;
  ARG_USED();

  bridge_str = argv[1];
  controller_str = argv[3];
  localaddr_str = argv[5];

  if (confsys->type == CONFSYS_MSG_TYPE_SET) {
    struct addrunion addr;
    lagopus_result_t result;

    /* Lookup bridge. */
    bridge = dpmgr_bridge_lookup(dpmgr, bridge_str);
    if (! bridge) {
      return CONFIG_FAILURE;
    }

    /* Register channel to dpmgr. */
    addrunion_ipv4_set(&addr, controller_str);
    result = channel_mgr_channel_lookup(bridge, &addr, &chan);
    if (result != LAGOPUS_RESULT_OK) {
      lagopus_perror(result);
      return CONFIG_FAILURE;
    }
    addrunion_ipv4_set(&addr, localaddr_str);
    channel_local_addr_set(chan, &addr);
  } else if (confsys->type == CONFSYS_MSG_TYPE_UNSET) {
    /* TODO: removing port. */
  }

  return CONFIG_SUCCESS;
}

CALLBACK(controller_local_port_func) {
  struct bridge *bridge;
  struct channel *chan;
  const char *bridge_str;
  const char *controller_str;
  const char *port_str;
  int port;
  lagopus_result_t ret;
  ARG_USED();

  bridge_str = argv[1];
  controller_str = argv[3];
  port_str = argv[5];

  if (confsys->type == CONFSYS_MSG_TYPE_SET) {
    struct addrunion addr;
    lagopus_result_t result;

    /* Lookup bridge. */
    bridge = dpmgr_bridge_lookup(dpmgr, bridge_str);
    if (! bridge) {
      return CONFIG_FAILURE;
    }

    /* Register channel to dpmgr. */
    addrunion_ipv4_set(&addr, controller_str);
    result = channel_mgr_channel_lookup(bridge, &addr, &chan);
    if (result != LAGOPUS_RESULT_OK) {
      lagopus_perror(result);
      return CONFIG_FAILURE;
    }
    ret = sscanf(port_str, "%d", &port);
    if (ret != 1) {
      return CONFIG_FAILURE;
    }
    channel_local_port_set(chan, (uint16_t) port);
  } else if (confsys->type == CONFSYS_MSG_TYPE_UNSET) {
    /* TODO: removing port. */
  }

  return CONFIG_SUCCESS;
}

CALLBACK(interface_ethernet_func) {
  int ret;
  uint32_t portid;
  struct port nport;
  const char *if_str;

  ARG_USED();

  /* Interface name string. */
  if_str = argv[2];

  /* Interface name must start from "eth". */
  if (strncmp(if_str, "eth", 3) != 0) {
    return CONFIG_FAILURE;
  }

  /* Acquire interface number. */
  ret = sscanf(if_str, "eth%d", &portid);
  if (ret != 1) {
    return CONFIG_FAILURE;
  }

  memset(&nport, 0, sizeof(nport));

  if (confsys->type == CONFSYS_MSG_TYPE_SET) {
    nport.ofp_port.port_no = 0; /* unassigned */
    nport.ifindex = portid;
    nport.type = LAGOPUS_PORT_TYPE_PHYSICAL;
    lagopus_msg_info("interface add : if = %s, port id = %u\n",
                     if_str, nport.ifindex);
    snprintf(nport.ofp_port.name , sizeof(nport.ofp_port.name),
             "eth%d", portid);
    if (dpmgr_port_add(dpmgr, &nport) != 0) {
      return CONFIG_FAILURE;
    }
  } else if (confsys->type == CONFSYS_MSG_TYPE_UNSET) {
    lagopus_msg_info("interface delete : if = %s, port id = %u\n",
                     if_str, portid);
    if (dpmgr_port_delete(dpmgr, portid) != 0) {
      return CONFIG_FAILURE;
    }
  }
  return CONFIG_SUCCESS;
}

CALLBACK(bridge_domain_port_func) {
  int ret;
  const char *bridge_str;
  const char *if_str;
  const char *port_str;
  uint32_t portid, port_no;

  ARG_USED();

  bridge_str = argv[1];
  if_str = argv[3];
  port_str = argv[5];

  /* Interface name must start from "eth". */
  if (strncmp(if_str, "eth", 3) != 0) {
    return CONFIG_FAILURE;
  }

  /* Acquire interface number. */
  ret = sscanf(if_str, "eth%d", &portid);
  if (ret != 1) {
    return CONFIG_FAILURE;
  }

  if (port_str != NULL && port_str[0] != '\0') {
    /* Acquire interface number. */
    ret = sscanf(port_str, "%d", &port_no);
    if (ret != 1) {
      return CONFIG_FAILURE;
    }
  } else {
    port_no = portid + 1;
  }

  if (confsys->type == CONFSYS_MSG_TYPE_SET) {
    lagopus_msg_info("port add : bridge = %s, if = %s, port id = %u\n",
                     bridge_str, if_str, port_no);
    dpmgr_bridge_port_add(dpmgr, bridge_str, portid, port_no);
  } else if (confsys->type == CONFSYS_MSG_TYPE_UNSET) {
    lagopus_msg_info("port delete : bridge = %s, if = %s, port id = %u\n",
                     bridge_str, if_str, portid);
    dpmgr_bridge_port_delete(dpmgr, bridge_str, portid);
  }
  return CONFIG_SUCCESS;
}

CALLBACK(config_trace_file) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t dbg_lvl = lagopus_log_get_debug_level();
  ARG_USED();

  if (confsys->type == CONFSYS_MSG_TYPE_SET) {
    ret = lagopus_log_initialize(LAGOPUS_LOG_EMIT_TO_FILE, argv[4],
                                 false, true, dbg_lvl);
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
      return CONFIG_FAILURE;
    }
  } else {
    /* TODO : arg */
    ret = lagopus_log_initialize(LAGOPUS_LOG_EMIT_TO_SYSLOG, "lagopus",
                                 false, true, dbg_lvl);
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
      return CONFIG_FAILURE;
    }
  }

  return CONFIG_SUCCESS;
}

CALLBACK(config_trace_packet) {
  ARG_USED();

  if (confsys->type == CONFSYS_MSG_TYPE_SET) {
    lagopus_log_set_trace_packet_flag(true);
  } else {
    lagopus_log_set_trace_packet_flag(false);
  }
  return CONFIG_SUCCESS;
}

enum trace_ofp_type {
  TRACE_OFP_NONE,
  TRACE_OFP_HELLO,
  TRACE_OFP_ERROR,
  TRACE_OFP_ECHO,
  TRACE_OFP_EXPERIMENTER,
  TRACE_OFP_FEATURES,
  TRACE_OFP_CONFIG,
  TRACE_OFP_PACKET_IN,
  TRACE_OFP_FLOW_REMOVED,
  TRACE_OFP_PORT_STATUS,
  TRACE_OFP_PACKET_OUT,
  TRACE_OFP_FLOW_MOD,
  TRACE_OFP_GROUP_MOD,
  TRACE_OFP_PORT_MOD,
  TRACE_OFP_TABLE_MOD,
  TRACE_OFP_MULTIPART,
  TRACE_OFP_BARRIER,
  TRACE_OFP_QUEUE,
  TRACE_OFP_ROLE,
  TRACE_OFP_ASYNC,
  TRACE_OFP_METER_MOD,
};

struct trace_ofp_packet {
  const char *str;
  enum trace_ofp_type type;
} trace_ofp_packet_str[] = {
  { "hello",  TRACE_OFP_HELLO },
  { "error",  TRACE_OFP_ERROR },
  { "echo", TRACE_OFP_ECHO },
  { "experimenter", TRACE_OFP_EXPERIMENTER},
  { "features", TRACE_OFP_FEATURES},
  { "config", TRACE_OFP_CONFIG},
  { "packet-in", TRACE_OFP_PACKET_IN},
  { "flow-removed", TRACE_OFP_FLOW_REMOVED},
  { "port-status", TRACE_OFP_PORT_STATUS},
  { "packet-out", TRACE_OFP_PACKET_OUT},
  { "flow-mod", TRACE_OFP_FLOW_MOD},
  { "group-mod", TRACE_OFP_GROUP_MOD},
  { "port-mod", TRACE_OFP_PORT_MOD},
  { "table-mod", TRACE_OFP_TABLE_MOD},
  { "multipart", TRACE_OFP_MULTIPART},
  { "barrier", TRACE_OFP_BARRIER},
  { "queue", TRACE_OFP_QUEUE},
  { "role", TRACE_OFP_ROLE},
  { "async", TRACE_OFP_ASYNC},
  { "meter-mod", TRACE_OFP_METER_MOD},
};

static enum trace_ofp_type
trace_ofp_packet_type(const char *str) {
  int i;
  int array_len;

  array_len = sizeof(trace_ofp_packet_str) / sizeof(struct trace_ofp_packet);
  for (i = 0; i < array_len; i++) {
    if (strcmp(trace_ofp_packet_str[i].str, str) == 0) {
      return trace_ofp_packet_str[i].type;
    }
  }
  return TRACE_OFP_NONE;
}

CALLBACK(config_trace_ofp_packet) {
  enum trace_ofp_type type;
  uint32_t flags = 0;
  ARG_USED();

  /* Lookup trace_ofp_type. */
  type = trace_ofp_packet_type(argv[4]);
  if (type == TRACE_OFP_NONE) {
    return CONFIG_FAILURE;
  }

  switch (type) {
    case TRACE_OFP_HELLO:
      flags = TRACE_OFPT_HELLO;
      break;
    case TRACE_OFP_ERROR:
      flags = TRACE_OFPT_ERROR;
      break;
    case TRACE_OFP_ECHO:
      flags = (TRACE_OFPT_ECHO_REQUEST | \
               TRACE_OFPT_ECHO_REPLY);
      break;
    case TRACE_OFP_EXPERIMENTER:
      flags = TRACE_OFPT_EXPERIMENTER;
      break;
    case TRACE_OFP_FEATURES:
      flags = (TRACE_OFPT_FEATURES_REQUEST | \
               TRACE_OFPT_FEATURES_REPLY);
      break;
    case TRACE_OFP_CONFIG:
      flags = (TRACE_OFPT_GET_CONFIG_REQUEST | \
               TRACE_OFPT_GET_CONFIG_REPLY |   \
               TRACE_OFPT_SET_CONFIG);
      break;
    case TRACE_OFP_PACKET_IN:
      flags = TRACE_OFPT_PACKET_IN;
      break;
    case TRACE_OFP_FLOW_REMOVED:
      flags = TRACE_OFPT_FLOW_REMOVED;
      break;
    case TRACE_OFP_PORT_STATUS:
      flags = TRACE_OFPT_PORT_STATUS;
      break;
    case TRACE_OFP_PACKET_OUT:
      flags = TRACE_OFPT_PACKET_OUT;
      break;
    case TRACE_OFP_FLOW_MOD:
      flags = TRACE_OFPT_FLOW_MOD;
      break;
    case TRACE_OFP_GROUP_MOD:
      flags = TRACE_OFPT_GROUP_MOD;
      break;
    case TRACE_OFP_PORT_MOD:
      flags = TRACE_OFPT_PORT_MOD;
      break;
    case TRACE_OFP_TABLE_MOD:
      flags = TRACE_OFPT_TABLE_MOD;
      break;
    case TRACE_OFP_MULTIPART:
      flags = (TRACE_OFPT_MULTIPART_REQUEST | \
               TRACE_OFPT_MULTIPART_REPLY);
      break;
    case TRACE_OFP_BARRIER:
      flags = (TRACE_OFPT_BARRIER_REQUEST | \
               TRACE_OFPT_BARRIER_REPLY);
      break;
    case TRACE_OFP_QUEUE:
      flags = (TRACE_OFPT_QUEUE_GET_CONFIG_REQUEST | \
               TRACE_OFPT_QUEUE_GET_CONFIG_REPLY);
      break;
    case TRACE_OFP_ROLE:
      flags = (TRACE_OFPT_ROLE_REQUEST | \
               TRACE_OFPT_ROLE_REPLY);
      break;
    case TRACE_OFP_ASYNC:
      flags = (TRACE_OFPT_GET_ASYNC_REQUEST | \
               TRACE_OFPT_GET_ASYNC_REPLY |   \
               TRACE_OFPT_SET_ASYNC);
      break;
    case TRACE_OFP_METER_MOD:
      flags = TRACE_OFPT_METER_MOD;
      break;
    default:
      break;
  }
  /* Set or unset trace flag according to trace_ofp_type. */
  if (confsys->type == CONFSYS_MSG_TYPE_SET) {
    lagopus_log_set_trace_flags(flags);
  } else {
    lagopus_log_unset_trace_flags(flags);
  }

  return CONFIG_SUCCESS;
}

CALLBACK(config_max_tables) {
  ARG_USED();
  printf("XXX configure max tables %s\n", argv[3]);
  return CONFIG_SUCCESS;
}

CALLBACK(stop_process_func) {
  ARG_USED();
  show(confsys, "Stopping process\n");
  global_state_request_shutdown(SHUTDOWN_RIGHT_NOW);
  return CONFIG_SUCCESS;
}

int show_flow_func(struct confsys *confsys, int argc, const char **argv);
int show_flowcache_func(struct confsys *confsys, int argc, const char **argv);
int show_table_func(struct confsys *confsys, int argc, const char **argv);
int show_bridge_func(struct confsys *confsys, int argc, const char **argv);
int show_logical_switch_func(struct confsys *confsys);
int show_interface_func(struct confsys *confsys, int argc, const char **argv);

CALLBACK(show_capable_switch_func) {
  ARG_USED();
  show(confsys, "<capable-switch xmlns=\"urn:onf:of111:config:yang\">\n");
  show(confsys, "<id>CapableSwitch0</id>\n");
  show(confsys, "<configuration-points>\n");
  show(confsys, "<configuration-point>\n");
  show(confsys, "<id>ConfigurationPoint1</id>\n");
  show(confsys, "<uri>uri0</uri>\n");
  show(confsys, "<protocol>ssh</protocol>\n");
  show(confsys, "</configuration-point>\n");
  show(confsys, "</configuration-points>\n");
  show_logical_switch_func(confsys);
  show(confsys, "</capable-switch>\n");
  return CONFIG_SUCCESS;
}

CALLBACK(config_tls_ca) {
  ARG_USED();
  if (argc == 3 && argv != NULL && argv[2] != NULL) {
    if (confsys->type == CONFSYS_MSG_TYPE_SET) {
      const char *ca = argv[2];
      session_tls_ca_dir_set(ca);
      return CONFIG_SUCCESS;
    } else if (confsys->type == CONFSYS_MSG_TYPE_UNSET) {
      session_tls_ca_dir_set(DEFAULT_TLS_CERTIFICATE_STORE);
      return CONFIG_SUCCESS;
    }
  }
  return CONFIG_FAILURE;
}

CALLBACK(config_tls_cert) {
  ARG_USED();
  if (argc == 3 && argv != NULL && argv[2] != NULL) {
    if (confsys->type == CONFSYS_MSG_TYPE_SET) {
      const char *cert = argv[2];
      session_tls_cert_set(cert);
      return CONFIG_SUCCESS;
    } else if (confsys->type == CONFSYS_MSG_TYPE_UNSET) {
      session_tls_cert_set(DEFAULT_TLS_CERTIFICATE_FILE);
      return CONFIG_SUCCESS;
    }
  }
  return CONFIG_FAILURE;
}

CALLBACK(config_tls_key) {
  ARG_USED();
  if (argc == 3 && argv != NULL && argv[2] != NULL) {
    if (confsys->type == CONFSYS_MSG_TYPE_SET) {
      const char *key = argv[2];
      session_tls_key_set(key);
      return CONFIG_SUCCESS;
    } else if (confsys->type == CONFSYS_MSG_TYPE_UNSET) {
      session_tls_key_set(DEFAULT_TLS_PRIVATE_KEY);
      return CONFIG_SUCCESS;
    }
  }
  return CONFIG_FAILURE;
}

CALLBACK(config_tls_check_config) {
  ARG_USED();
  if (argc == 3 && argv != NULL && argv[2] != NULL) {
    if (confsys->type == CONFSYS_MSG_TYPE_SET) {
      const char *check_config= argv[2];
      agent_check_certificates_set_config_file(check_config);
      return CONFIG_SUCCESS;
    } else if (confsys->type == CONFSYS_MSG_TYPE_UNSET) {
      agent_check_certificates_set_config_file(DEFAULT_TLS_TRUST_POINT_CONF);
      return CONFIG_SUCCESS;
    }
  }
  return CONFIG_FAILURE;
}

CALLBACK(show_controller_ipv4_func) {
  uint64_t dpid;

  ARG_USED();

  if (dpmgr_controller_dpid_find(dpmgr, argv[2], &dpid) != LAGOPUS_RESULT_OK) {
    return CONFIG_FAILURE;
  }
  show(confsys, "Controller %s\n", argv[2]);
  show(confsys, " Datapath ID:       %016x\n", dpid);
  show(confsys, " Connection status: ");
  if (channel_mgr_has_alive_channel_by_dpid(dpid) == true) {
    show(confsys, "Connected\n");
  } else {
    show(confsys, "Disonnected\n");
  }

  return CONFIG_SUCCESS;
}

CALLBACK(show_controller_ipv6_func) {
  uint64_t dpid;

  ARG_USED();

  if (dpmgr_controller_dpid_find(dpmgr, argv[2], &dpid) != LAGOPUS_RESULT_OK) {
    return CONFIG_FAILURE;
  }
  show(confsys, "Controller %s\n", argv[2]);
  show(confsys, " Datapath ID:       %016x\n", dpid);
  show(confsys, " Connection status: ");
  if (channel_mgr_has_alive_channel_by_dpid(dpid) == true) {
    show(confsys, "Connected\n");
  } else {
    show(confsys, "Disonnected\n");
  }

  return CONFIG_SUCCESS;
}

void
agent_install_callback(void) {
  struct cnode *top;
  struct cserver *cserver;

  cserver = cserver_get();

  /* Show commands. */
  top = cserver->show;
  install_callback(top, "show version", show_version_func);
  install_callback(top, "show flow", show_flow_func);
  install_callback(top, "show flowcache", show_flowcache_func);
  install_callback(top, "show table", show_table_func);
  install_callback(top, "show bridge-domains", show_bridge_func);
  install_callback(top, "show bridge-domains WORD", show_bridge_func);

  install_callback(top, "show interface WORD", show_interface_func);
  install_callback(top, "show controller A.B.C.D", show_controller_ipv4_func);
  install_callback(top, "show controller X:X::X:X", show_controller_ipv6_func);

  /* OF-Config. */
  install_callback(top, "capable-switch", show_capable_switch_func);

  /* Configurations. */
  top = cserver->schema;
  install_callback(top,
                   "interface ethernet WORD",
                   interface_ethernet_func);
  install_callback(top,
                   "bridge-domains WORD",
                   bridge_domain_func);
  install_callback(top,
                   "bridge-domains WORD dpid WORD",
                   bridge_domain_dpid_func);
  install_callback(top,
                   "bridge-domains WORD fail-secure-mode",
                   bridge_domain_fail_secure_mode_func);
  install_callback(top,
                   "bridge-domains WORD "
                   "controller A.B.C.D",
                   controller_func);
  install_callback(top,
                   "bridge-domains WORD "
                   "controller A.B.C.D "
                   "protocols tls",
                   controller_tls_func);
  install_callback(top,
                   "bridge-domains WORD "
                   "controller A.B.C.D "
                   "port INTEGER",
                   controller_port_func);
  install_callback(top,
                   "bridge-domains WORD "
                   "controller A.B.C.D "
                   "local-address A.B.C.D",
                   controller_local_address_func);
  install_callback(top,
                   "bridge-domains WORD "
                   "controller A.B.C.D "
                   "local-port 1-65535",
                   controller_local_port_func);
  install_callback(top,
                   "bridge-domains WORD "
                   "port WORD",
                   bridge_domain_port_func);

  /* Trace option file. */
  install_callback(top, "protocols openflow traceoptions file WORD",
                   config_trace_file);
  /* Trace option packet and types. */
  install_callback(top, "protocols openflow traceoptions flag packet",
                   config_trace_packet);
  /* Trace option for ofp packet. */
  install_callback(top, "protocols openflow traceoptions flag hello",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag error",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag echo",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag experimenter",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag features",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag config",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag packet-in",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag flow-removed",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag port-status",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag packet-out",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag flow-mod",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag group-mod",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag port-mod",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag table-mod",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag multipart",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag barrier",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag queue",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag role",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag async",
                   config_trace_ofp_packet);
  install_callback(top, "protocols openflow traceoptions flag meter-mod",
                   config_trace_ofp_packet);

  /* TLS */
  install_callback(top, "tls certificate-store WORD",
                   config_tls_ca);
  install_callback(top, "tls certificate-file WORD",
                   config_tls_cert);
  install_callback(top, "tls private-key WORD",
                   config_tls_key);
  install_callback(top, "tls trust-point-conf WORD",
                   config_tls_check_config);

  /* Max tables test. */
  install_callback(top, "bridge-domains WORD max-tables 1-255",
                   config_max_tables);

  /* Exec commands. */
  top = cserver->exec;
  install_callback(top, "stop-process", stop_process_func);
}
