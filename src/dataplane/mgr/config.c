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
