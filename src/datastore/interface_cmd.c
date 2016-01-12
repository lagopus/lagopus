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

#include "lagopus_apis.h"
#include "datastore_apis.h"
#include "cmd_common.h"
#include "lagopus/dp_apis.h"
#include "interface_cmd.h"
#include "interface_cmd_internal.h"
#include "interface.c"
#include "conv_json.h"
#include "lagopus/port.h"
#include "lagopus/dp_apis.h"

/* command name. */
#define CMD_NAME "interface"

/* option num. */
enum interface_opts {
  OPT_NAME = 0,
  OPT_TYPE,
  OPT_PORT_NO,
  OPT_DEVICE,
  OPT_DST_ADDR,
  OPT_DST_PORT,
  OPT_MCAST_GROUP,
  OPT_SRC_ADDR,
  OPT_SRC_PORT,
  OPT_NI,
  OPT_TTL,
  OPT_DST_HW_ADDR,
  OPT_SRC_HW_ADDR,
  OPT_MTU,
  OPT_IP_ADDR,
  OPT_CLEAR,
  OPT_IS_USED,
  OPT_IS_ENABLED,

  OPT_MAX,
};

/* stats num. */
enum interface_stats {
  STATS_RX_PACKETS = 0,
  STATS_RX_BYTES,
  STATS_RX_DROPPED,
  STATS_RX_ERRORS,
  STATS_TX_PACKETS,
  STATS_TX_BYTES,
  STATS_TX_DROPPED,
  STATS_TX_ERRORS,

  STATS_MAX,
};

/* option flags. */
#define OPT_COMMON   (OPT_BIT_GET(OPT_NAME) |                 \
                      OPT_BIT_GET(OPT_TYPE) |                 \
                      OPT_BIT_GET(OPT_IS_USED) |              \
                      OPT_BIT_GET(OPT_IS_ENABLED))
#define OPT_ETHERNET_RAWSOCK (OPT_COMMON | \
                              OPT_BIT_GET(OPT_DEVICE) |               \
                              OPT_BIT_GET(OPT_MTU) |                  \
                              OPT_BIT_GET(OPT_IP_ADDR))
#define OPT_ETHERNET_DPDK_PHY (OPT_COMMON | OPT_BIT_GET(OPT_PORT_NO) | \
                               OPT_BIT_GET(OPT_DEVICE) |               \
                               OPT_BIT_GET(OPT_MTU) |                  \
                               OPT_BIT_GET(OPT_IP_ADDR))
#define OPT_GRE      (OPT_COMMON | OPT_BIT_GET(OPT_PORT_NO) | \
                      OPT_BIT_GET(OPT_DST_ADDR) |             \
                      OPT_BIT_GET(OPT_SRC_ADDR) |             \
                      OPT_BIT_GET(OPT_MTU) |                  \
                      OPT_BIT_GET(OPT_IP_ADDR))
#define OPT_NVGRE    (OPT_COMMON | OPT_BIT_GET(OPT_PORT_NO) | \
                      OPT_BIT_GET(OPT_DST_ADDR) |             \
                      OPT_BIT_GET(OPT_MCAST_GROUP) |          \
                      OPT_BIT_GET(OPT_SRC_ADDR) |             \
                      OPT_BIT_GET(OPT_NI) |                   \
                      OPT_BIT_GET(OPT_MTU) |                  \
                      OPT_BIT_GET(OPT_IP_ADDR))
#define OPT_VXLAN    (OPT_COMMON | OPT_BIT_GET(OPT_PORT_NO) | \
                      OPT_BIT_GET(OPT_DEVICE)|                \
                      OPT_BIT_GET(OPT_DST_ADDR) |             \
                      OPT_BIT_GET(OPT_DST_PORT) |             \
                      OPT_BIT_GET(OPT_MCAST_GROUP) |          \
                      OPT_BIT_GET(OPT_SRC_ADDR) |             \
                      OPT_BIT_GET(OPT_SRC_PORT) |             \
                      OPT_BIT_GET(OPT_NI) |                   \
                      OPT_BIT_GET(OPT_TTL) |                  \
                      OPT_BIT_GET(OPT_MTU) |                  \
                      OPT_BIT_GET(OPT_IP_ADDR))
#define OPT_VHOST_USER    (OPT_COMMON | OPT_BIT_GET(OPT_PORT_NO) |  \
                           OPT_BIT_GET(OPT_DST_HW_ADDR) |           \
                           OPT_BIT_GET(OPT_SRC_HW_ADDR) |           \
                           OPT_BIT_GET(OPT_MTU) |                   \
                           OPT_BIT_GET(OPT_IP_ADDR))
#define OPT_UNKNOWN  (OPT_COMMON)

/* option name. */
static const char *const opt_strs[OPT_MAX] = {
  "*name",               /* OPT_NAME (not option) */
  "-type",               /* OPT_TYPE */
  "-port-number",        /* OPT_PORT_NO */
  "-device",             /* OPT_DEVICE */
  "-dst-addr",           /* OPT_DST_ADDR */
  "-dst-port",           /* OPT_DST_PORT */
  "-mcast-group",        /* OPT_MCAST_GROUP */
  "-src-addr",           /* OPT_SRC_ADDR */
  "-src-port",           /* OPT_SRC_PORT */
  "-ni",                 /* OPT_NI */
  "-ttl",                /* OPT_TTL */
  "-dst-hw-addr",        /* OPT_DST_HW_ADDR */
  "-src-hw-addr",        /* OPT_SRC_HW_ADDR */
  "-mtu",                /* OPT_MTU */
  "-ip-addr",            /* OPT_IP_ADDR */
  "-clear",              /* OPT_CLEAR (stats option)*/
  "*is-used",            /* OPT_IS_USED (not option) */
  "*is-enabled",         /* OPT_IS_ENABLED (not option) */
};

/* stats name. */
static const char *const stat_strs[STATS_MAX] = {
  "*rx-packets",         /* STATS_RX_PACKETS (not option) */
  "*rx-bytes",           /* STATS_RX_BYTES (not option) */
  "*rx-dropped",         /* STATS_RX_DROPPED (not option) */
  "*rx-errors",          /* STATS_RX_ERRORS (not option) */
  "*tx-packets",         /* STATS_TX_PACKETS (not option) */
  "*tx-bytes",           /* STATS_TX_BYTES (not option) */
  "*tx-dropped",         /* STATS_TX_DROPPED (not option) */
  "*tx-errors",          /* STATS_TX_ERRORS (not option) */
};

typedef struct configs {
  size_t size;
  uint64_t flags;
  bool is_config;
  bool is_show_modified;
  bool is_show_stats;
  datastore_interface_stats_t stats;
  interface_conf_t **list;
} configs_t;

typedef lagopus_result_t
(*ip_addr_set_proc_t)(interface_attr_t *attr,
                      const char *addr, const bool prefer);
typedef lagopus_result_t
(*hw_addr_set_proc_t)(interface_attr_t *attr,
                      const mac_address_t hw_addr);
typedef lagopus_result_t
(*uint64_set_proc_t)(interface_attr_t *attr, const uint64_t num);
typedef lagopus_result_t
(*uint32_set_proc_t)(interface_attr_t *attr, const uint32_t num);
typedef lagopus_result_t
(*uint16_set_proc_t)(interface_attr_t *attr, const uint16_t num);
typedef lagopus_result_t
(*uint8_set_proc_t)(interface_attr_t *attr, const uint8_t num);

static lagopus_hashmap_t sub_cmd_table = NULL;
static lagopus_hashmap_t ethernet_opt_table = NULL;
static lagopus_hashmap_t gre_opt_table = NULL;
static lagopus_hashmap_t nvgre_opt_table = NULL;
static lagopus_hashmap_t vxlan_opt_table = NULL;
static lagopus_hashmap_t vhost_user_opt_table = NULL;
static lagopus_hashmap_t stats_opt_table = NULL;

static inline lagopus_result_t
ethernet_port_create(const char *name,
                     interface_attr_t *attr,
                     datastore_interface_type_t type) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *device = NULL;
  uint32_t port_no;
  lagopus_ip_address_t *dst_addr = NULL;
  uint32_t dst_port;
  lagopus_ip_address_t *mcast_group = NULL;
  lagopus_ip_address_t *src_addr = NULL;
  lagopus_ip_address_t *ip_addr = NULL;
  uint32_t src_port;
  uint32_t ni;
  uint8_t ttl;
  uint16_t mtu;
  datastore_interface_info_t interface_info;

  if (((ret = interface_get_port_number(attr, &port_no)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = interface_get_device(attr, &device)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = interface_get_mtu(attr, &mtu)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = interface_get_ip_addr(attr, &ip_addr)) ==
       LAGOPUS_RESULT_OK)) {
    lagopus_msg_info("create interface. if = %s, port no = %u.\n",
                     device, port_no);
    interface_info.type = type;
    switch (type) {
      case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
        interface_info.eth_dpdk_phy.port_number = port_no;
        interface_info.eth_dpdk_phy.device = device;
        interface_info.eth_dpdk_phy.mtu = mtu;
        interface_info.eth_dpdk_phy.ip_addr = ip_addr;
        break;
      case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
        interface_info.eth_rawsock.port_number = port_no;
        interface_info.eth_rawsock.device = device;
        interface_info.eth_rawsock.mtu = mtu;
        interface_info.eth_rawsock.ip_addr = ip_addr;
        break;
      case DATASTORE_INTERFACE_TYPE_VXLAN:
        if (((ret = interface_get_dst_addr(attr, &dst_addr)) ==
             LAGOPUS_RESULT_OK) &&
            ((ret = interface_get_dst_port(attr, &dst_port)) ==
             LAGOPUS_RESULT_OK) &&
            ((ret = interface_get_mcast_group(attr,
                                              &mcast_group)) ==
             LAGOPUS_RESULT_OK) &&
            ((ret = interface_get_src_addr(attr, &src_addr)) ==
             LAGOPUS_RESULT_OK) &&
            ((ret = interface_get_src_port(attr, &src_port)) ==
             LAGOPUS_RESULT_OK) &&
            ((ret = interface_get_ni(attr, &ni)) ==
             LAGOPUS_RESULT_OK) &&
            ((ret = interface_get_ttl(attr, &ttl)) ==
             LAGOPUS_RESULT_OK)) {
          interface_info.vxlan.dst_addr = dst_addr;
          interface_info.vxlan.dst_port = dst_port;
          interface_info.vxlan.mcast_group = mcast_group;
          interface_info.vxlan.src_addr = src_addr;
          interface_info.vxlan.src_port = src_port;
          interface_info.vxlan.ni = ni;
          interface_info.vxlan.ttl = ttl;
          interface_info.vxlan.mtu = mtu;
          interface_info.vxlan.ip_addr = ip_addr;
        }
        break;
      default:
        ret = LAGOPUS_RESULT_OUT_OF_RANGE;
        goto done;
        break;
    }
    if ((ret = dp_interface_create(name)) ==
        LAGOPUS_RESULT_OK) {
      ret = dp_interface_info_set(name,
                                  &interface_info);
#ifdef __UNIT_TESTING__
      /*
       * Ignore LAGOPUS_RESULT_POSIX_API_ERROR for unit test.
       * Not check exists interface devices (eth0, eth1, ...).
       */
      if (ret == LAGOPUS_RESULT_POSIX_API_ERROR) {
        lagopus_msg_debug(1, "Ignore LAGOPUS_RESULT_POSIX_API_ERROR!!\n");
        ret = LAGOPUS_RESULT_OK;
      }
#endif /* __UNIT_TESTING__ */
    }
  }

done:
  return ret;
}

static inline lagopus_result_t
ethernet_port_destroy(const char *name,
                      interface_attr_t *attr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *device = NULL;
  uint32_t port_no;

  if (((ret = interface_get_port_number(attr, &port_no)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = interface_get_device(attr, &device)) ==
       LAGOPUS_RESULT_OK)) {
    lagopus_msg_info("destroy interface. if = %s, port no = %u.\n",
                     device, port_no);
    ret = dp_interface_destroy(name);
  }

  free(device);

  return ret;
}

static inline lagopus_result_t
interface_port_create(const char *name,
                      interface_attr_t *attr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interface_type_t type;

  if ((ret = interface_get_type(attr, &type)) ==
      LAGOPUS_RESULT_OK) {
    switch (type) {
      case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
      case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      case DATASTORE_INTERFACE_TYPE_VXLAN:
        ret = ethernet_port_create(name, attr, type);
        break;
      case DATASTORE_INTERFACE_TYPE_GRE:
        /* TODO: */
        ret = LAGOPUS_RESULT_OK;
        break;
      case DATASTORE_INTERFACE_TYPE_NVGRE:
        /* TODO: */
        ret = LAGOPUS_RESULT_OK;
        break;
      case DATASTORE_INTERFACE_TYPE_VHOST_USER:
        /* TODO: */
        ret = LAGOPUS_RESULT_OK;
        break;
      case DATASTORE_INTERFACE_TYPE_UNKNOWN:
        ret = LAGOPUS_RESULT_OK;
        break;
      default:
        ret = LAGOPUS_RESULT_OUT_OF_RANGE;
        break;
    }
  }

  return ret;
}

static inline lagopus_result_t
interface_port_destroy(const char *name,
                       interface_attr_t *attr) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interface_type_t type;

  if ((ret = interface_get_type(attr, &type)) ==
      LAGOPUS_RESULT_OK) {
    switch (type) {
      case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
      case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
      case DATASTORE_INTERFACE_TYPE_VXLAN:
        ret = ethernet_port_destroy(name, attr);
        break;
      case DATASTORE_INTERFACE_TYPE_GRE:
        /* TODO: */
        ret = LAGOPUS_RESULT_OK;
        break;
      case DATASTORE_INTERFACE_TYPE_NVGRE:
        /* TODO: */
        ret = LAGOPUS_RESULT_OK;
        break;
      case DATASTORE_INTERFACE_TYPE_VHOST_USER:
        /* TODO: */
        ret = LAGOPUS_RESULT_OK;
        break;
      case DATASTORE_INTERFACE_TYPE_UNKNOWN:
        ret = LAGOPUS_RESULT_OK;
        break;
      default:
        ret = LAGOPUS_RESULT_OUT_OF_RANGE;
        break;
    }
  }

  return ret;
}

static inline void
interface_cmd_update_current_attr(interface_conf_t *conf,
                                  datastore_interp_state_t state) {
  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    return;
  }

  if (conf->modified_attr != NULL) {
    if (conf->current_attr != NULL) {
      interface_attr_destroy(conf->current_attr);
    }
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = NULL;
  }
}

static inline void
interface_cmd_update_aborted(interface_conf_t *conf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (conf->modified_attr != NULL) {
    if (conf->current_attr == NULL) {
      /* create. */
      ret = interface_conf_delete(conf);
      if (ret != LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    } else {
      interface_attr_destroy(conf->modified_attr);
      conf->modified_attr = NULL;
    }
  }
}

static inline void
interface_cmd_update_switch_attr(interface_conf_t *conf) {
  interface_attr_t *attr;

  if (conf->modified_attr != NULL) {
    attr = conf->current_attr;
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = attr;
  }
}

static inline void
interface_cmd_is_enabled_set(interface_conf_t *conf) {
  if (conf->is_enabled == false) {
    if (conf->is_enabling == true) {
      conf->is_enabled = true;
    }
  } else {
    if (conf->is_disabling == true) {
      conf->is_enabled = false;
    }
  }
}

static inline void
interface_cmd_do_destroy(interface_conf_t *conf,
                         datastore_interp_state_t state) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    ret = interface_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (state == DATASTORE_INTERP_STATE_DRYRUN) {
    ret = interface_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (conf->is_destroying == true ||
             state == DATASTORE_INTERP_STATE_AUTO_COMMIT) {
    ret = dp_interface_destroy(conf->name);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }

    ret = interface_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  }
}

static inline lagopus_result_t
interface_cmd_do_update(datastore_interp_t *iptr,
                        datastore_interp_state_t state,
                        interface_conf_t *conf,
                        bool is_propagation,
                        bool is_enable_disable_cmd,
                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_modified = false;
  (void) iptr;
  (void) is_propagation;

  if (conf->modified_attr != NULL &&
      interface_attr_equals(conf->current_attr,
                            conf->modified_attr) == false) {
    if (conf->modified_attr != NULL) {
      is_modified = true;
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                      NULL);
    }
  } else {
    /*
     * no need to re-create.
     */
    ret = LAGOPUS_RESULT_OK;
  }

  if (ret == LAGOPUS_RESULT_OK) {
    /*
     * Then start/shutdown if needed.
     */
    if (is_modified == true) {
      /*
       * Update attrs.
       */
      if (conf->current_attr != NULL) {
        /* destroy interface. */
        ret = interface_port_destroy(conf->name,
                                     conf->current_attr);
        if (ret != LAGOPUS_RESULT_OK) {
          lagopus_msg_warning("Can't delete interface(port).\n");
        }
      }

      /* create interface. */
      ret = interface_port_create(conf->name,
                                  conf->modified_attr);
      if (ret == LAGOPUS_RESULT_OK) {
        if (conf->is_enabled == true) {
          /* start interface. */
          lagopus_msg_info("start interface. name = %s.\n",
                           conf->name);
          ret = dp_interface_start(conf->name);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_string_setf(
                    result, ret,
                    "Can't start interface(port).");
          }
        }
      } else {
        ret = datastore_json_result_string_setf(
                result, ret,
                "Can't add interface(port).");
      }

      /* Update attr. */
      if (ret == LAGOPUS_RESULT_OK &&
          state != DATASTORE_INTERP_STATE_COMMITING &&
          state != DATASTORE_INTERP_STATE_ROLLBACKING) {
        interface_cmd_update_current_attr(conf, state);
      }
    } else {
      if (is_enable_disable_cmd == true ||
          conf->is_enabling == true ||
          conf->is_disabling == true) {
        if (conf->is_enabled == true) {
          /* start interface. */
          lagopus_msg_info("start interface. name = %s.\n",
                           conf->name);
          ret = dp_interface_start(conf->name);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_string_setf(
                    result, ret,
                    "Can't start interface(port).");
          }
        } else {
          /* stop interface. */
          lagopus_msg_info("stop interface. name = %s.\n",
                           conf->name);
          ret = dp_interface_stop(conf->name);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_string_setf(
                    result, ret,
                    "Can't stop interface(port).");
          }
        }
      }
      conf->is_enabling = false;
      conf->is_disabling = false;
    }
  }

  return ret;
}

static inline lagopus_result_t
interface_cmd_update_internal(datastore_interp_t *iptr,
                              datastore_interp_state_t state,
                              interface_conf_t *conf,
                              bool is_propagation,
                              bool is_enable_disable_cmd,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int i;

  switch (state) {
    case DATASTORE_INTERP_STATE_AUTO_COMMIT: {
      i = 0;
      while (i < UPDATE_CNT_MAX) {
        ret = interface_cmd_do_update(iptr, state, conf,
                                      is_propagation,
                                      is_enable_disable_cmd,
                                      result);
        if (ret == LAGOPUS_RESULT_OK ||
            is_enable_disable_cmd == true) {
          break;
        } else if (conf->current_attr == NULL &&
                   conf->modified_attr != NULL) {
          interface_cmd_do_destroy(conf, state);
          break;
        } else {
          interface_cmd_update_switch_attr(conf);
          lagopus_msg_warning("FAILED auto_comit (%s): rollbacking....\n",
                              lagopus_error_get_string(ret));
        }
        i++;
      }
      break;
    }
    case DATASTORE_INTERP_STATE_COMMITING: {
      interface_cmd_is_enabled_set(conf);
      ret = interface_cmd_do_update(iptr, state, conf,
                                    is_propagation,
                                    is_enable_disable_cmd,
                                    result);
      break;
    }
    case DATASTORE_INTERP_STATE_ATOMIC: {
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_COMMITED:
    case DATASTORE_INTERP_STATE_ROLLBACKED: {
      interface_cmd_update_current_attr(conf, state);
      interface_cmd_do_destroy(conf, state);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_ROLLBACKING: {
      if (conf->current_attr == NULL &&
          conf->modified_attr != NULL) {
        /* case create. */
        ret = LAGOPUS_RESULT_OK;
      } else {
        interface_cmd_update_switch_attr(conf);
        interface_cmd_is_enabled_set(conf);
        ret = interface_cmd_do_update(iptr, state, conf,
                                      is_propagation,
                                      is_enable_disable_cmd,
                                      result);

      }
      break;
    }
    case DATASTORE_INTERP_STATE_ABORTING: {
      conf->is_destroying = false;
      conf->is_enabling = false;
      conf->is_disabling = false;
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_ABORTED: {
      interface_cmd_update_aborted(conf);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_DRYRUN: {
      if (conf->modified_attr != NULL) {
        if (conf->current_attr != NULL) {
          interface_attr_destroy(conf->current_attr);
          conf->current_attr = NULL;
        }

        conf->current_attr = conf->modified_attr;
        conf->modified_attr = NULL;
      }

      ret = LAGOPUS_RESULT_OK;
      break;
    }
    default: {
      ret = LAGOPUS_RESULT_NOT_FOUND;
      lagopus_perror(ret);
      break;
    }
  }

  return ret;
}

STATIC lagopus_result_t
interface_cmd_update(datastore_interp_t *iptr,
                     datastore_interp_state_t state,
                     void *o,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) iptr;
  (void) result;

  if (iptr != NULL && *iptr != NULL && o != NULL) {
    interface_conf_t *conf = (interface_conf_t *)o;
    ret = interface_cmd_update_internal(iptr, state, conf, false, false, result);
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static lagopus_result_t
uint_opt_parse(const char *const *argv[],
               interface_conf_t *conf,
               configs_t *configs,
               void *uint_set_proc,
               enum interface_opts opt,
               enum cmd_uint_type type,
               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  union cmd_uint cmd_uint;

  if (argv != NULL && conf != NULL &&
      configs != NULL && uint_set_proc &&
      result != NULL) {
    if (*(*argv + 1) != NULL) {
      if ((ret = cmd_uint_parse(*(++(*argv)), type,
                                &cmd_uint)) ==
          LAGOPUS_RESULT_OK) {
        switch (type) {
          case CMD_UINT8:
            /* uint8. */
            ret = ((uint8_set_proc_t) uint_set_proc)(conf->modified_attr,
                  cmd_uint.uint8);
            break;
          case CMD_UINT16:
            /* uint16. */
            ret = ((uint16_set_proc_t) uint_set_proc)(conf->modified_attr,
                  cmd_uint.uint16);
            break;
          case CMD_UINT32:
            /* uint32. */
            ret = ((uint32_set_proc_t) uint_set_proc)(conf->modified_attr,
                  cmd_uint.uint32);
            break;
          default:
            /* uint64. */
            ret = ((uint64_set_proc_t) uint_set_proc)(conf->modified_attr,
                  cmd_uint.uint64);
            break;
        }
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't add %s.",
                                                  ATTR_NAME_GET(opt_strs, opt));
        }
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Bad opt value = %s.",
                                                *(*argv));
      }
    } else if (configs->is_config == true) {
      configs->flags = (uint64_t) OPT_BIT_GET(opt);
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Bad opt value.");
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static lagopus_result_t
ip_opt_parse(const char *const *argv[],
             interface_conf_t *conf,
             configs_t *configs,
             ip_addr_set_proc_t ip_addr_set_proc,
             enum interface_opts opt,
             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (argv != NULL && conf != NULL &&
      configs != NULL && ip_addr_set_proc != NULL &&
      result != NULL) {
    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        ret = (ip_addr_set_proc)(conf->modified_attr,
                                 *(*argv), true);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't add %s.",
                                                  ATTR_NAME_GET(opt_strs, opt));
        }
      } else {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_INVALID_ARGS,
                                                "Bad opt value = %s.",
                                                *(*argv));
      }
    } else if (configs->is_config == true) {
      configs->flags = (uint64_t) OPT_BIT_GET(opt);
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Bad opt value.");
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static lagopus_result_t
hw_addr_opt_parse(const char *const *argv[],
                  interface_conf_t *conf,
                  configs_t *configs,
                  hw_addr_set_proc_t hw_addr_set_proc,
                  enum interface_opts opt,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  mac_address_t addr;

  if (argv != NULL && conf != NULL &&
      configs != NULL && hw_addr_set_proc != NULL &&
      result != NULL) {
    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = cmd_mac_addr_parse(*(*argv), addr)) ==
            LAGOPUS_RESULT_OK) {
          ret = (hw_addr_set_proc) (conf->modified_attr, addr);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't add %s.",
                                                    ATTR_NAME_GET(opt_strs, opt));
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Bad opt value = %s.",
                                                  *(*argv));
        }
      } else {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_INVALID_ARGS,
                                                "Bad opt value = %s.",
                                                *(*argv));
      }
    } else if (configs->is_config == true) {
      configs->flags = (uint64_t) OPT_BIT_GET(opt);
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Bad opt value.");
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static lagopus_result_t
port_no_opt_parse(const char *const *argv[],
                  void *c, void *out_configs,
                  lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (interface_conf_t *) c,
                        (configs_t *) out_configs,
                        interface_set_port_number,
                        OPT_PORT_NO, CMD_UINT32,
                        result);
}

static lagopus_result_t
dst_addr_opt_parse(const char *const *argv[],
                   void *c, void *out_configs,
                   lagopus_dstring_t *result) {
  return ip_opt_parse(argv, (interface_conf_t *) c,
                      (configs_t *) out_configs,
                      interface_set_dst_addr_str,
                      OPT_DST_ADDR, result);
}

static lagopus_result_t
dst_port_opt_parse(const char *const *argv[],
                   void *c, void *out_configs,
                   lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (interface_conf_t *) c,
                        (configs_t *) out_configs,
                        interface_set_dst_port,
                        OPT_DST_PORT, CMD_UINT32,
                        result);
}

static lagopus_result_t
mcast_group_opt_parse(const char *const *argv[],
                      void *c, void *out_configs,
                      lagopus_dstring_t *result) {
  return ip_opt_parse(argv, (interface_conf_t *) c,
                      (configs_t *) out_configs,
                      interface_set_mcast_group_str,
                      OPT_MCAST_GROUP, result);
}

static lagopus_result_t
src_addr_opt_parse(const char *const *argv[],
                   void *c, void *out_configs,
                   lagopus_dstring_t *result) {
  return ip_opt_parse(argv, (interface_conf_t *) c,
                      (configs_t *) out_configs,
                      interface_set_src_addr_str,
                      OPT_SRC_ADDR, result);
}

static lagopus_result_t
ip_addr_opt_parse(const char *const *argv[],
                  void *c, void *out_configs,
                  lagopus_dstring_t *result) {
  return ip_opt_parse(argv, (interface_conf_t *) c,
                      (configs_t *) out_configs,
                      interface_set_ip_addr_str,
                      OPT_IP_ADDR, result);
}

static lagopus_result_t
src_port_opt_parse(const char *const *argv[],
                   void *c, void *out_configs,
                   lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (interface_conf_t *) c,
                        (configs_t *) out_configs,
                        interface_set_src_port,
                        OPT_SRC_PORT, CMD_UINT32,
                        result);
}

static lagopus_result_t
ni_opt_parse(const char *const *argv[],
             void *c, void *out_configs,
             lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (interface_conf_t *) c,
                        (configs_t *) out_configs,
                        interface_set_ni,
                        OPT_NI, CMD_UINT32,
                        result);
}

static lagopus_result_t
ttl_opt_parse(const char *const *argv[],
              void *c, void *out_configs,
              lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (interface_conf_t *) c,
                        (configs_t *) out_configs,
                        interface_set_ttl,
                        OPT_TTL, CMD_UINT8,
                        result);
}

static lagopus_result_t
mtu_opt_parse(const char *const *argv[],
              void *c, void *out_configs,
              lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (interface_conf_t *) c,
                        (configs_t *) out_configs,
                        interface_set_mtu,
                        OPT_TTL, CMD_UINT16,
                        result);
}

static lagopus_result_t
dst_hw_addr_opt_parse(const char *const *argv[],
                      void *c, void *out_configs,
                      lagopus_dstring_t *result) {
  return hw_addr_opt_parse(argv, (interface_conf_t *) c,
                           (configs_t *) out_configs,
                           interface_set_dst_hw_addr,
                           OPT_DST_HW_ADDR, result);
}

static lagopus_result_t
src_hw_addr_opt_parse(const char *const *argv[],
                      void *c, void *out_configs,
                      lagopus_dstring_t *result) {
  return hw_addr_opt_parse(argv, (interface_conf_t *) c,
                           (configs_t *) out_configs,
                           interface_set_src_hw_addr,
                           OPT_SRC_HW_ADDR, result);
}

static lagopus_result_t
device_opt_parse(const char *const *argv[],
                 void *c, void *out_configs,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  interface_conf_t *conf = NULL;
  configs_t *configs = NULL;
  char *name = NULL;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (interface_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = lagopus_str_unescape(*(*argv), "\"'", &name)) >= 0) {
          ret = interface_set_device(conf->modified_attr,
                                     name);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "device name = "
                                                    "%s.", name);
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "device name = %s.",
                                                  *(*argv));
        }
      } else {
        if (*(*argv) == NULL) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt value.");
        } else {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt value = %s.",
                                                  *(*argv));
        }
      }
    } else if (configs->is_config == true) {
      configs->flags = OPT_BIT_GET(OPT_DEVICE);
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Bad opt value.");
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  free(name);

  return ret;
}

static lagopus_result_t
clear_opt_parse(const char *const *argv[],
                void *c, void *out_configs,
                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  interface_conf_t *conf = NULL;
  (void) out_configs;
  (void) conf;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (interface_conf_t *) c;

    if (*(*argv + 1) == NULL) {
      ret = dp_interface_stats_clear(conf->name);
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't clear stats.");
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Bad opt = %s.", *(*argv + 1));
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static inline lagopus_result_t
opt_parse(const char *const argv[],
          interface_conf_t *conf,
          configs_t *out_configs,
          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interface_type_t type;
  bool required = false;
  lagopus_hashmap_t table = NULL;
  void *opt_proc = NULL;

  argv++;
  if (*argv != NULL) {
    while (*argv != NULL) {
      if (IS_VALID_STRING(*argv) == true) {
        /* type. */
        if (OPT_CMP(*argv, opt_strs, OPT_TYPE) == 0) {
          if (*(argv + 1) != NULL) {
            if (IS_VALID_STRING(*(argv + 1)) == true) {
              ret = interface_type_to_enum(*(++argv), &type);
              if (ret == LAGOPUS_RESULT_OK) {
                ret = interface_set_type(conf->modified_attr,
                                         type);
                if (ret == LAGOPUS_RESULT_OK) {
                  required = true;
                  goto loop_end;
                } else {
                  ret = datastore_json_result_string_setf(result, ret,
                                                          "Can't add type.");
                  goto done;
                }
              } else {
                ret = datastore_json_result_string_setf(result, ret,
                                                        "Bad opt value = %s.",
                                                        *argv);
                goto done;
              }
            } else {
              ret = datastore_json_result_set(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              NULL);
              goto done;
            }
          } else if (out_configs->is_config == true) {
            out_configs->flags = OPT_BIT_GET(OPT_TYPE);
            ret = LAGOPUS_RESULT_OK;
            goto loop_end;
          } else {
            ret = datastore_json_result_set(result,
                                            LAGOPUS_RESULT_INVALID_ARGS,
                                            NULL);
            goto done;
          }
        } else if (out_configs->is_config == false && required != true) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad required options(-type),"
                                                  " opt = %s.",
                                                  *argv);
          goto done;
        }

        if (required == true || out_configs->is_config == true) {
          ret = interface_get_type(conf->modified_attr,
                                   &type);
          if (ret == LAGOPUS_RESULT_OK) {
            switch (type) {
              case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
              case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
                table = ethernet_opt_table;
                break;
              case DATASTORE_INTERFACE_TYPE_GRE:
                table = gre_opt_table;
                break;
              case DATASTORE_INTERFACE_TYPE_NVGRE:
                table = nvgre_opt_table;
                break;
              case DATASTORE_INTERFACE_TYPE_VXLAN:
                table = vxlan_opt_table;
                break;
              case DATASTORE_INTERFACE_TYPE_VHOST_USER:
                table = vhost_user_opt_table;
                break;
              default:
                ret = datastore_json_result_string_setf(result, ret,
                                                        "Bad opt value = %s.",
                                                        *argv);
                goto done;
            }
          } else {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't get type.");
            goto done;
          }


          if (lagopus_hashmap_find(&table,
                                   (void *)(*argv),
                                   &opt_proc) == LAGOPUS_RESULT_OK) {
            /* parse opt. */
            if (opt_proc != NULL) {
              ret = ((opt_proc_t) opt_proc)(&argv,
                                            (void *) conf,
                                            (void *) out_configs,
                                            result);
              if (ret != LAGOPUS_RESULT_OK) {
                goto done;
              }
            } else {
              ret = LAGOPUS_RESULT_NOT_FOUND;
              lagopus_perror(ret);
              goto done;
            }
          } else {
            ret = datastore_json_result_string_setf(result,
                                                    LAGOPUS_RESULT_INVALID_ARGS,
                                                    "Bad opt = %s.", *argv);
            goto done;
          }
        } else {
          ret = datastore_json_result_set(result,
                                          LAGOPUS_RESULT_INVALID_ARGS,
                                          NULL);
          goto done;
        }
      loop_end:
        argv++;
      } else {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_INVALID_ARGS,
                                                "Bad opt.");
        goto done;
      }
    }
  } else {
    /* config: all items show. */
    if (out_configs->is_config) {
      out_configs->flags = OPTS_MAX;
    }
    ret = LAGOPUS_RESULT_OK;
  }

  /* set required opts. */
  if (conf->modified_attr != NULL) {
    if ((ret = interface_get_type(conf->modified_attr,
                                  &type)) ==
        LAGOPUS_RESULT_OK) {
      if (type == DATASTORE_INTERFACE_TYPE_UNKNOWN) {
        ret = interface_set_type(conf->modified_attr,
                                 DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't set type.");
          goto done;
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't get type.");
      goto done;
    }
  }

done:
  return ret;
}

static lagopus_result_t
create_sub_cmd_parse_internal(datastore_interp_t *iptr,
                              datastore_interp_state_t state,
                              size_t argc, const char *const argv[],
                              char *name,
                              datastore_update_proc_t proc,
                              configs_t *out_configs,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  interface_conf_t *conf = NULL;
  (void) argc;
  (void) proc;

  ret = interface_conf_create(&conf, name);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = interface_conf_add(conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = opt_parse(argv, conf, out_configs, result);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = interface_cmd_update_internal(iptr, state, conf,
                                            true, false, result);

        if (ret != LAGOPUS_RESULT_OK &&
            ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_set(result, ret, NULL);
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add interface_conf.");
    }

    if (ret != LAGOPUS_RESULT_OK) {
      (void) interface_conf_delete(conf);
      goto done;
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't create interface_conf.");
  }

  if (ret != LAGOPUS_RESULT_OK && conf != NULL) {
    interface_conf_destroy(conf);
  }

done:
  return ret;
}

static lagopus_result_t
config_sub_cmd_parse_internal(datastore_interp_t *iptr,
                              datastore_interp_state_t state,
                              size_t argc, const char *const argv[],
                              interface_conf_t *conf,
                              datastore_update_proc_t proc,
                              configs_t *out_configs,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) argc;
  (void) proc;
  if (conf->modified_attr == NULL) {
    if (conf->current_attr != NULL) {
      /*
       * already exists. copy it.
       */
      ret = interface_attr_duplicate(conf->current_attr,
                                     &conf->modified_attr, NULL);
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_set(result, ret, NULL);
        goto done;
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_NOT_FOUND,
                                              "Not found attr. : name = %s",
                                              conf->name);
      goto done;
    }
  }

  conf->is_destroying = false;
  out_configs->is_config = true;
  ret = opt_parse(argv, conf, out_configs, result);

  if (ret == LAGOPUS_RESULT_OK) {
    if (out_configs->flags == 0) {
      /* update. */
      ret = interface_cmd_update_internal(iptr, state, conf,
                                          true, false, result);

      if (ret != LAGOPUS_RESULT_OK &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_set(result, ret, NULL);
      }
    } else {
      /* show. */
      ret = interface_conf_one_list(&out_configs->list, conf);

      if (ret >= 0) {
        out_configs->size = (size_t) ret;
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = datastore_json_result_string_setf(
                result, ret,
                "Can't create list of interface_conf.");
      }
    }
  }

done:
  return ret;
}

static lagopus_result_t
create_sub_cmd_parse(datastore_interp_t *iptr,
                     datastore_interp_state_t state,
                     size_t argc, const char *const argv[],
                     char *name,
                     lagopus_hashmap_t *hptr,
                     datastore_update_proc_t proc,
                     void *out_configs,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  interface_conf_t *conf = NULL;
  char *namespace = NULL;
  (void) hptr;
  (void) proc;

  if (name != NULL) {
    ret = interface_find(name, &conf);

    if (ret == LAGOPUS_RESULT_NOT_FOUND) {
      ret = namespace_get_namespace(name, &namespace);
      if (ret == LAGOPUS_RESULT_OK) {
        if (namespace_exists(namespace) == true ||
            state == DATASTORE_INTERP_STATE_DRYRUN) {
          ret = create_sub_cmd_parse_internal(iptr, state,
                                              argc, argv,
                                              name, proc,
                                              out_configs, result);
        } else {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_NOT_FOUND,
                                                  "namespace = %s", namespace);
        }
        free((void *) namespace);
      } else {
        ret = datastore_json_result_set(result, ret, NULL);
      }
    } else if (ret == LAGOPUS_RESULT_OK &&
               conf->is_destroying == true) {
      ret = config_sub_cmd_parse_internal(iptr, state, argc, argv,
                                          conf, proc, out_configs, result);
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_ALREADY_EXISTS,
                                              "name = %s", name);
    }
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS, NULL);
  }

  return ret;
}

static lagopus_result_t
config_sub_cmd_parse(datastore_interp_t *iptr,
                     datastore_interp_state_t state,
                     size_t argc, const char *const argv[],
                     char *name,
                     lagopus_hashmap_t *hptr,
                     datastore_update_proc_t proc,
                     void *out_configs,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  interface_conf_t *conf = NULL;
  (void) hptr;
  (void) proc;

  if (name != NULL) {
    ret = interface_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = config_sub_cmd_parse_internal(iptr, state, argc, argv,
                                          conf, proc, out_configs,
                                          result);

    } else if (ret == LAGOPUS_RESULT_NOT_FOUND) {
      /* create. */
      ret = create_sub_cmd_parse_internal(iptr, state, argc, argv,
                                          name, proc, out_configs, result);
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_ALREADY_EXISTS,
                                              "name = %s", name);
    }
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS, NULL);
  }

  return ret;
}

static inline lagopus_result_t
enable_sub_cmd_parse_internal(datastore_interp_t *iptr,
                              datastore_interp_state_t state,
                              interface_conf_t *conf,
                              bool is_propagation,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && conf != NULL && result != NULL) {
    if (conf->is_used == true) {
      if (conf->is_enabled == false) {
        if (state == DATASTORE_INTERP_STATE_ATOMIC) {
          conf->is_enabling = true;
          conf->is_disabling = false;
          ret = LAGOPUS_RESULT_OK;
        } else {
          conf->is_enabled = true;
          ret = interface_cmd_update_internal(iptr, state, conf,
                                              is_propagation, true, result);
          if (ret != LAGOPUS_RESULT_OK) {
            conf->is_enabled = false;
            if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
              ret = datastore_json_result_set(result, ret, NULL);
            }
          }
        }
      } else {
        ret = LAGOPUS_RESULT_OK;
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_NOT_OPERATIONAL,
                                              "name = %s. is not used.",
                                              conf->name);
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static lagopus_result_t
enable_sub_cmd_parse(datastore_interp_t *iptr,
                     datastore_interp_state_t state,
                     size_t argc, const char *const argv[],
                     char *name,
                     lagopus_hashmap_t *hptr,
                     datastore_update_proc_t proc,
                     void *out_configs,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  interface_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = interface_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      ret = enable_sub_cmd_parse_internal(iptr, state,
                                          conf, true,
                                          result);
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_OBJECT,
                                              "name = %s", name);
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static inline lagopus_result_t
disable_sub_cmd_parse_internal(datastore_interp_t *iptr,
                               datastore_interp_state_t state,
                               interface_conf_t *conf,
                               bool is_propagation,
                               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && conf != NULL && result != NULL) {
    if (state == DATASTORE_INTERP_STATE_ATOMIC) {
      conf->is_enabling = false;
      conf->is_disabling = true;
      ret = LAGOPUS_RESULT_OK;
    } else {
      conf->is_enabled = false;
      ret = interface_cmd_update_internal(iptr, state, conf,
                                          is_propagation, true, result);

      if (ret != LAGOPUS_RESULT_OK) {
        conf->is_enabled = true;
        if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_set(result, ret, NULL);
        }
      }
    }
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS, NULL);
  }

  return ret;
}

static lagopus_result_t
disable_sub_cmd_parse(datastore_interp_t *iptr,
                      datastore_interp_state_t state,
                      size_t argc, const char *const argv[],
                      char *name,
                      lagopus_hashmap_t *hptr,
                      datastore_update_proc_t proc,
                      void *out_configs,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  interface_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = interface_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      ret = disable_sub_cmd_parse_internal(iptr, state,
                                           conf, true,
                                           result);
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_OBJECT,
                                              "name = %s", name);
    }
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS, NULL);
  }

  return ret;
}

static inline lagopus_result_t
destroy_sub_cmd_parse_internal(datastore_interp_t *iptr,
                               datastore_interp_state_t state,
                               interface_conf_t *conf,
                               bool is_propagation,
                               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && conf != NULL && result != NULL) {
    if (conf->is_used == false) {
      if (state == DATASTORE_INTERP_STATE_ATOMIC) {
        conf->is_destroying = true;
        conf->is_enabling = false;
        conf->is_disabling = true;
      } else {
        if (conf->is_enabled == true) {
          conf->is_enabled = false;
          ret = interface_cmd_update_internal(iptr, state, conf,
                                              is_propagation, true,
                                              result);

          if (ret != LAGOPUS_RESULT_OK) {
            conf->is_enabled = true;
            if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
              ret = datastore_json_result_set(result, ret, NULL);
            }
            goto done;
          }
        }

        interface_cmd_do_destroy(conf, state);
      }
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_NOT_OPERATIONAL,
                                              "name = %s: is used.",
                                              conf->name);
    }
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

done:
  return ret;
}

static lagopus_result_t
destroy_sub_cmd_parse(datastore_interp_t *iptr,
                      datastore_interp_state_t state,
                      size_t argc, const char *const argv[],
                      char *name,
                      lagopus_hashmap_t *hptr,
                      datastore_update_proc_t proc,
                      void *out_configs,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  interface_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = interface_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      if (conf->is_used == false) {
        ret = destroy_sub_cmd_parse_internal(iptr, state,
                                             conf, true,
                                             result);
      } else {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_NOT_OPERATIONAL,
                                                "name = %s: is used.", name);
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_OBJECT,
                                              "name = %s", name);
    }
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static lagopus_result_t
stats_sub_cmd_parse(datastore_interp_t *iptr,
                    datastore_interp_state_t state,
                    size_t argc, const char *const argv[],
                    char *name,
                    lagopus_hashmap_t *hptr,
                    datastore_update_proc_t proc,
                    void *out_configs,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  interface_conf_t *conf = NULL;
  configs_t *configs = NULL;
  void *opt_proc = NULL;
  (void) iptr;
  (void) state;
  (void) argc;
  (void) hptr;
  (void) proc;

  if (argv != NULL && name != NULL &&
      out_configs != NULL && result != NULL) {
    configs = (configs_t *) out_configs;
    ret = interface_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      if (*(argv + 1) == NULL) {
        configs->is_show_stats = true;

        ret = dp_interface_stats_get(conf->name, &configs->stats);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = interface_conf_one_list(&configs->list, conf);

          if (ret >= 0) {
            configs->size = (size_t) ret;
            ret = LAGOPUS_RESULT_OK;
          } else {
            ret = datastore_json_result_string_setf(
                    result, ret,
                    "Can't create list of interface_conf.");
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get stats.");
        }
      } else {
        argv++;
        while (*argv != NULL) {
          if (IS_VALID_STRING(*argv) == true) {
            if (lagopus_hashmap_find(&stats_opt_table,
                                     (void *)(*argv),
                                     &opt_proc) == LAGOPUS_RESULT_OK) {
              /* parse opt. */
              if (opt_proc != NULL) {
                ret = ((opt_proc_t) opt_proc)(&argv,
                                              (void *) conf,
                                              (void *) out_configs,
                                              result);
                if (ret != LAGOPUS_RESULT_OK) {
                  goto done;
                }
              } else {
                ret = datastore_json_result_string_setf(result,
                                                        LAGOPUS_RESULT_INVALID_ARGS,
                                                        "Bad opt = %s.",
                                                        *argv);
                goto done;
              }
            } else {
              ret = datastore_json_result_string_setf(result,
                                                      LAGOPUS_RESULT_INVALID_ARGS,
                                                      "Bad opt = %s.", *argv);
              goto done;
            }
            argv++;
          } else {
            ret = datastore_json_result_string_setf(result,
                                                    LAGOPUS_RESULT_INVALID_ARGS,
                                                    "Bad opt.");
            goto done;
          }
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_OBJECT,
                                              "name = %s", name);
    }
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

done:
  return ret;
}

static inline lagopus_result_t
show_parse(const char *name,
           configs_t *out_configs,
           bool is_show_modified,
           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  interface_conf_t *conf = NULL;
  char *target = NULL;

  if (name == NULL) {
    ret = namespace_get_current_namespace(&target);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = interface_conf_list(&out_configs->list, target);
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_ANY_FAILURES,
                                              "Can't get namespace.");
      goto done;
    }
  } else {
    ret = namespace_get_search_target(name, &target);

    if (ret == NS_DEFAULT_NAMESPACE) {
      /* default namespace */
      ret = interface_conf_list(&out_configs->list, "");
    } else if (ret == NS_NAMESPACE) {
      /* namespace + delim */
      ret = interface_conf_list(&out_configs->list, target);
    } else if (ret == NS_FULLNAME) {
      /* namespace + name or delim + name */
      ret = interface_find(target, &conf);

      if (ret == LAGOPUS_RESULT_OK) {
        if (conf->is_destroying == false) {
          ret = interface_conf_one_list(&out_configs->list, conf);
        } else {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_NOT_FOUND, "name = %s", name);
          goto done;
        }
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "name = %s", name);
        goto done;
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't get search target.");
      goto done;
    }
  }

  if (ret >= 0) {
    out_configs->size = (size_t) ret;
    out_configs->flags = OPTS_MAX;
    out_configs->is_show_modified = is_show_modified;
    ret = LAGOPUS_RESULT_OK;
  } else if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
    ret = datastore_json_result_string_setf(
            result, ret,
            "Can't create list of interface_conf.");
  }

done:
  free((void *) target);

  return ret;
}

static inline lagopus_result_t
show_sub_cmd_parse(const char *const argv[],
                   char *name,
                   configs_t *out_configs,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_show_modified = false;

  if (IS_VALID_STRING(*argv) == true) {
    if (strcmp(*argv, SHOW_OPT_CURRENT) == 0 ||
        strcmp(*argv, SHOW_OPT_MODIFIED) == 0) {
      if (strcmp(*argv, SHOW_OPT_MODIFIED) == 0) {
        is_show_modified = true;
      }

      if (IS_VALID_STRING(*(argv + 1)) == false) {
        ret = show_parse(name, out_configs, is_show_modified, result);
      } else {
        ret = datastore_json_result_set(result,
                                        LAGOPUS_RESULT_INVALID_ARGS,
                                        NULL);
      }
    } else {
      ret = datastore_json_result_string_setf(
              result, LAGOPUS_RESULT_INVALID_ARGS,
              "sub_cmd = %s.", *argv);
    }
  } else {
    if (*argv == NULL) {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Bad opt value.");
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Bad opt value = %s.", *argv);
    }
  }

  return ret;
}

static lagopus_result_t
interface_cmd_enable(datastore_interp_t *iptr,
                     datastore_interp_state_t state,
                     void *obj,
                     bool *currentp,
                     bool enabled,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (currentp == NULL) {
    if (enabled == true) {
      ret = enable_sub_cmd_parse_internal(iptr, state,
                                          (interface_conf_t *) obj,
                                          false,
                                          result);
    } else {
      ret = disable_sub_cmd_parse_internal(iptr, state,
                                           (interface_conf_t *) obj,
                                           false,
                                           result);
    }
  } else {
    if (obj != NULL) {
      *currentp = ((interface_conf_t *) obj)->is_enabled;
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_set(result,
                                      LAGOPUS_RESULT_INVALID_ARGS,
                                      NULL);
    }
  }

  return ret;
}

static lagopus_result_t
interface_cmd_destroy(datastore_interp_t *iptr,
                      datastore_interp_state_t state,
                      void *obj,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = destroy_sub_cmd_parse_internal(iptr, state,
                                       (interface_conf_t *) obj,
                                       false,
                                       result);
  return ret;
}

STATIC lagopus_result_t
interface_cmd_serialize(datastore_interp_t *iptr,
                        datastore_interp_state_t state,
                        const void *obj,
                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  interface_conf_t *conf = NULL;

  char *escaped_name = NULL;

  /* port-number */
  uint32_t port_number = 0;

  /* device */
  char *device= NULL;
  char *escaped_device = NULL;

  /* type */
  datastore_interface_type_t type = DATASTORE_INTERFACE_TYPE_UNKNOWN;
  const char *type_str = NULL;
  char *escaped_type_str = NULL;

  /* dst-addr */
  lagopus_ip_address_t *dst_addr = NULL;
  char *dst_addr_str = NULL;
  char *escaped_dst_addr_str = NULL;

  /* dst-port */
  uint32_t dst_port = 0;

  /* mcast-group */
  lagopus_ip_address_t *mcast_group = NULL;
  char *mcast_group_str = NULL;
  char *escaped_mcast_group_str = NULL;

  /* src-addr */
  lagopus_ip_address_t *src_addr = NULL;
  char *src_addr_str = NULL;
  char *escaped_src_addr_str = NULL;

  /* src-port */
  uint32_t src_port = 0;

  /* ni */
  uint32_t ni = 0;

  /* dst-hw-addr */
  mac_address_t dst_hw_addr;

  /* src-hw-addr */
  mac_address_t src_hw_addr;

  bool is_escaped = false;
  uint64_t flags = OPTS_MAX;
  size_t i = 0;

  /* ttl */
  uint8_t ttl = 0;

  /* mtu */
  uint16_t mtu = 0;

  /* src-addr */
  lagopus_ip_address_t *ip_addr = NULL;
  char *ip_addr_str = NULL;
  char *escaped_ip_addr_str = NULL;


  (void) state;

  if (iptr != NULL && obj != NULL && result != NULL) {
    conf = (interface_conf_t *) obj;

    if (conf->current_attr == NULL) {
      ret = LAGOPUS_RESULT_OK;
      goto done;
    }

    /* cmmand name. */
    if ((ret = lagopus_dstring_appendf(result, CMD_NAME)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    /* name. */
    if ((ret = lagopus_str_escape(conf->name, "\"", &is_escaped,
                                  &escaped_name)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(
                   result,
                   ESCAPE_NAME_FMT(is_escaped, escaped_name),
                   escaped_name)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* create cmd. */
    if ((ret = lagopus_dstring_appendf(result, " " CREATE_SUB_CMD)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    /* type opt. */
    if ((ret = interface_get_type(conf->current_attr,
                                  &type)) ==
        LAGOPUS_RESULT_OK) {
      if (type != DATASTORE_INTERFACE_TYPE_UNKNOWN) {
        if ((ret = interface_type_to_str(type,
                                         &type_str)) == LAGOPUS_RESULT_OK) {
          if (IS_VALID_STRING(type_str) == true) {
            if ((ret = lagopus_str_escape(type_str, "\"",
                                          &is_escaped,
                                          &escaped_type_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, " %s",
                                                 opt_strs[OPT_TYPE])) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = lagopus_dstring_appendf(
                             result,
                             ESCAPE_NAME_FMT(is_escaped, escaped_type_str),
                             escaped_type_str)) !=
                    LAGOPUS_RESULT_OK) {
                  lagopus_perror(ret);
                  goto done;
                }
              } else {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    switch (type) {
      case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
        flags &= OPT_ETHERNET_DPDK_PHY;
        break;
      case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
        flags &= OPT_ETHERNET_RAWSOCK;
        break;
      case DATASTORE_INTERFACE_TYPE_GRE:
        flags &= OPT_GRE;
        break;
      case DATASTORE_INTERFACE_TYPE_NVGRE:
        flags &= OPT_NVGRE;
        break;
      case DATASTORE_INTERFACE_TYPE_VXLAN:
        flags &= OPT_VXLAN;
        break;
      case DATASTORE_INTERFACE_TYPE_VHOST_USER:
        flags &= OPT_VHOST_USER;
        break;
      case DATASTORE_INTERFACE_TYPE_UNKNOWN:
        flags &= OPT_UNKNOWN;
        break;
      default:
        ret = LAGOPUS_RESULT_OUT_OF_RANGE;
        lagopus_perror(ret);
        goto done;
    }

    /* port-number opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_PORT_NO)) == true) {
      if ((ret = interface_get_port_number(conf->current_attr,
                                           &port_number)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %s",
                                           opt_strs[OPT_PORT_NO])) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %d",
                                             port_number)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    /* device opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_DEVICE)) == true) {
      if ((ret = interface_get_device(conf->current_attr,
                                      &device)) ==
          LAGOPUS_RESULT_OK) {
        if (IS_VALID_STRING(device) == true) {
          if ((ret = lagopus_str_escape(device, "\"",
                                        &is_escaped,
                                        &escaped_device)) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(result, " %s",
                                               opt_strs[OPT_DEVICE])) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(
                           result,
                           ESCAPE_NAME_FMT(is_escaped, escaped_device),
                           escaped_device)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          } else {
            lagopus_perror(ret);
            goto done;
          }
        }
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    /* dst-addr opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_DST_ADDR)) == true) {
      if ((ret = interface_get_dst_addr(conf->current_attr,
                                        &dst_addr)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_ip_address_str_get(dst_addr,
                                              &dst_addr_str)) ==
            LAGOPUS_RESULT_OK) {
          if (IS_VALID_STRING(dst_addr_str) == true) {
            if ((ret = lagopus_str_escape(dst_addr_str, "\"",
                                          &is_escaped,
                                          &escaped_dst_addr_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, " %s",
                                                 opt_strs[OPT_DST_ADDR])) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = lagopus_dstring_appendf(
                             result,
                             ESCAPE_NAME_FMT(is_escaped, escaped_dst_addr_str),
                             escaped_dst_addr_str)) !=
                    LAGOPUS_RESULT_OK) {
                  lagopus_perror(ret);
                  goto done;
                }
              } else {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    /* dst-port opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_DST_PORT)) == true) {
      if ((ret = interface_get_dst_port(conf->current_attr,
                                        &dst_port)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %s",
                                           opt_strs[OPT_DST_PORT])) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %d",
                                             dst_port)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    /* mcast-group opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_MCAST_GROUP)) == true) {
      if ((ret = interface_get_mcast_group(conf->current_attr,
                                           &mcast_group)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_ip_address_str_get(mcast_group,
                                              &mcast_group_str)) ==
            LAGOPUS_RESULT_OK) {
          if (IS_VALID_STRING(mcast_group_str) == true) {
            if ((ret = lagopus_str_escape(mcast_group_str, "\"",
                                          &is_escaped,
                                          &escaped_mcast_group_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, " %s",
                                                 opt_strs[OPT_MCAST_GROUP])) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = lagopus_dstring_appendf(
                             result,
                             ESCAPE_NAME_FMT(is_escaped, escaped_mcast_group_str),
                             escaped_mcast_group_str)) !=
                    LAGOPUS_RESULT_OK) {
                  lagopus_perror(ret);
                  goto done;
                }
              } else {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    /* src-addr opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_SRC_ADDR)) == true) {
      if ((ret = interface_get_src_addr(conf->current_attr,
                                        &src_addr)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_ip_address_str_get(src_addr,
                                              &src_addr_str)) ==
            LAGOPUS_RESULT_OK) {
          if (IS_VALID_STRING(src_addr_str) == true) {
            if ((ret = lagopus_str_escape(src_addr_str, "\"",
                                          &is_escaped,
                                          &escaped_src_addr_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, " %s",
                                                 opt_strs[OPT_SRC_ADDR])) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = lagopus_dstring_appendf(
                             result,
                             ESCAPE_NAME_FMT(is_escaped, escaped_src_addr_str),
                             escaped_src_addr_str)) !=
                    LAGOPUS_RESULT_OK) {
                  lagopus_perror(ret);
                  goto done;
                }
              } else {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    /* src-port opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_SRC_PORT)) == true) {
      if ((ret = interface_get_dst_port(conf->current_attr,
                                        &src_port)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %s",
                                           opt_strs[OPT_SRC_PORT])) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %d",
                                             src_port)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    /* ni opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_NI)) == true) {
      if ((ret = interface_get_ni(conf->current_attr,
                                  &ni)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %s",
                                           opt_strs[OPT_NI])) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %d",
                                             ni)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    /* ttl opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_TTL)) == true) {
      if ((ret = interface_get_ttl(conf->current_attr,
                                   &ttl)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %s",
                                           opt_strs[OPT_TTL])) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %d",
                                             ttl)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    /* dst-hw-addr opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_DST_HW_ADDR)) == true) {
      if ((ret = interface_get_dst_hw_addr(conf->current_attr,
                                           &dst_hw_addr)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %s ",
                                           opt_strs[OPT_DST_HW_ADDR])) ==
            LAGOPUS_RESULT_OK) {
          for (i = 0; i < MAC_ADDR_STR_LEN; i++) {
            if (i == 0) {
              ret = lagopus_dstring_appendf(result, "%02x",
                                            dst_hw_addr[i]);
            } else {
              ret = lagopus_dstring_appendf(result, ":%02x",
                                            dst_hw_addr[i]);
            }
            if (ret != LAGOPUS_RESULT_OK) {
              goto done;
            }
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    /* src-hw-addr opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_SRC_HW_ADDR)) == true) {
      if ((ret = interface_get_dst_hw_addr(conf->current_attr,
                                           &src_hw_addr)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %s ",
                                           opt_strs[OPT_SRC_HW_ADDR])) ==
            LAGOPUS_RESULT_OK) {
          for (i = 0; i < MAC_ADDR_STR_LEN; i++) {
            if (i == 0) {
              ret = lagopus_dstring_appendf(result, "%02x",
                                            src_hw_addr[i]);
            } else {
              ret = lagopus_dstring_appendf(result, ":%02x",
                                            src_hw_addr[i]);
            }
            if (ret != LAGOPUS_RESULT_OK) {
              goto done;
            }
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    /* mtu opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_MTU)) == true) {
      if ((ret = interface_get_mtu(conf->current_attr,
                                   &mtu)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %s",
                                           opt_strs[OPT_MTU])) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %d",
                                             mtu)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    /* ip-addr opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_IP_ADDR)) == true) {
      if ((ret = interface_get_ip_addr(conf->current_attr,
                                       &ip_addr)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_ip_address_str_get(ip_addr,
                                              &ip_addr_str)) ==
            LAGOPUS_RESULT_OK) {
          if (IS_VALID_STRING(ip_addr_str) == true) {
            if ((ret = lagopus_str_escape(ip_addr_str, "\"",
                                          &is_escaped,
                                          &escaped_ip_addr_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, " %s",
                                                 opt_strs[OPT_IP_ADDR])) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = lagopus_dstring_appendf(
                             result,
                             ESCAPE_NAME_FMT(is_escaped, escaped_ip_addr_str),
                             escaped_ip_addr_str)) !=
                    LAGOPUS_RESULT_OK) {
                  lagopus_perror(ret);
                  goto done;
                }
              } else {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }
        } else {
          lagopus_perror(ret);
          goto done;
        }
      } else {
        lagopus_perror(ret);
        goto done;
      }
    }

    /* Add newline. */
    if ((ret = lagopus_dstring_appendf(result, "\n")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

  done:
    free((void *) escaped_name);
    free((void *) device);
    free((void *) escaped_device);
    free((void *) escaped_type_str);
    free((void *) dst_addr);
    free((void *) dst_addr_str);
    free((void *) escaped_dst_addr_str);
    free((void *) mcast_group);
    free((void *) mcast_group_str);
    free((void *) escaped_mcast_group_str);
    free((void *) src_addr);
    free((void *) src_addr_str);
    free((void *) escaped_src_addr_str);
    free((void *) ip_addr);
    free((void *) ip_addr_str);
    free((void *) escaped_ip_addr_str);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
interface_cmd_compare(const void *obj1, const void *obj2, int *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj1 != NULL && obj2 != NULL && result != NULL) {
    *result = strcmp(((interface_conf_t *) obj1)->name,
                     ((interface_conf_t *) obj2)->name);
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
interface_cmd_getname(const void *obj, const char **namep) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj != NULL && namep != NULL) {
    *namep = ((interface_conf_t *)obj)->name;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
interface_cmd_duplicate(const void *obj, const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  interface_conf_t *dup_obj = NULL;

  if (obj != NULL) {
    ret = interface_conf_duplicate(obj, &dup_obj, namespace);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = interface_conf_add(dup_obj);

      if (ret != LAGOPUS_RESULT_OK && dup_obj != NULL) {
        interface_conf_destroy(dup_obj);
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

extern datastore_interp_t datastore_get_master_interp(void);

static inline lagopus_result_t
initialize_internal(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_t s_interp = datastore_get_master_interp();

  if ((ret = interface_initialize()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* create hashmap for sub cmds. */
  if ((ret = lagopus_hashmap_create(&sub_cmd_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if (((ret = sub_cmd_add(CREATE_SUB_CMD, create_sub_cmd_parse,
                          &sub_cmd_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = sub_cmd_add(CONFIG_SUB_CMD, config_sub_cmd_parse,
                          &sub_cmd_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = sub_cmd_add(ENABLE_SUB_CMD, enable_sub_cmd_parse,
                          &sub_cmd_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = sub_cmd_add(DISABLE_SUB_CMD, disable_sub_cmd_parse,
                          &sub_cmd_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = sub_cmd_add(DESTROY_SUB_CMD, destroy_sub_cmd_parse,
                          &sub_cmd_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = sub_cmd_add(STATS_SUB_CMD, stats_sub_cmd_parse,
                          &sub_cmd_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  /* create hashmap for ethernet opt. */
  if ((ret = lagopus_hashmap_create(&ethernet_opt_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* create hashmap for gre opt. */
  if ((ret = lagopus_hashmap_create(&gre_opt_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* create hashmap for nvgre opt. */
  if ((ret = lagopus_hashmap_create(&nvgre_opt_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* create hashmap for vxlan opt. */
  if ((ret = lagopus_hashmap_create(&vxlan_opt_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* create hashmap for vhost-user opt. */
  if ((ret = lagopus_hashmap_create(&vhost_user_opt_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* create hashmap for stats opt. */
  if ((ret = lagopus_hashmap_create(&stats_opt_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* add opts for ethernet opt. */
  if (((ret = opt_add(opt_strs[OPT_PORT_NO], port_no_opt_parse,
                      &ethernet_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_DEVICE], device_opt_parse,
                      &ethernet_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_MTU], mtu_opt_parse,
                      &ethernet_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_IP_ADDR], ip_addr_opt_parse,
                      &ethernet_opt_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  /* add opts for gre opt. */
  if (((ret = opt_add(opt_strs[OPT_PORT_NO], port_no_opt_parse,
                      &gre_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_DST_ADDR], dst_addr_opt_parse,
                      &gre_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_SRC_ADDR], src_addr_opt_parse,
                      &gre_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_MTU], mtu_opt_parse,
                      &gre_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_IP_ADDR], ip_addr_opt_parse,
                      &gre_opt_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  /* add opts for nvgre opt. */
  if (((ret = opt_add(opt_strs[OPT_PORT_NO], port_no_opt_parse,
                      &nvgre_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_DST_ADDR], dst_addr_opt_parse,
                      &nvgre_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_MCAST_GROUP], mcast_group_opt_parse,
                      &nvgre_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_SRC_ADDR], src_addr_opt_parse,
                      &nvgre_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_NI], ni_opt_parse,
                      &nvgre_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_MTU], mtu_opt_parse,
                      &nvgre_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_IP_ADDR], ip_addr_opt_parse,
                      &nvgre_opt_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  /* add opts for vxlan opt. */
  if (((ret = opt_add(opt_strs[OPT_PORT_NO], port_no_opt_parse,
                      &vxlan_opt_table)) != LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_DEVICE], device_opt_parse,
                      &vxlan_opt_table)) != LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_DST_ADDR], dst_addr_opt_parse,
                      &vxlan_opt_table)) != LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_DST_PORT], dst_port_opt_parse,
                      &vxlan_opt_table)) != LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_MCAST_GROUP], mcast_group_opt_parse,
                      &vxlan_opt_table)) != LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_SRC_ADDR], src_addr_opt_parse,
                      &vxlan_opt_table)) != LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_SRC_PORT], src_port_opt_parse,
                      &vxlan_opt_table)) != LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_NI], ni_opt_parse,
                      &vxlan_opt_table)) != LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_TTL], ttl_opt_parse,
                      &vxlan_opt_table)) != LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_MTU], mtu_opt_parse,
                      &vxlan_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_IP_ADDR], ip_addr_opt_parse,
                      &vxlan_opt_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  /* add opts for vhost-user opt. */
  if (((ret = opt_add(opt_strs[OPT_PORT_NO], port_no_opt_parse,
                      &vhost_user_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_DST_HW_ADDR], dst_hw_addr_opt_parse,
                      &vhost_user_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_SRC_HW_ADDR], src_hw_addr_opt_parse,
                      &vhost_user_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_MTU], mtu_opt_parse,
                      &vhost_user_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_IP_ADDR], ip_addr_opt_parse,
                      &vhost_user_opt_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  /* add opts for stats opt. */
  if (((ret = opt_add(opt_strs[OPT_CLEAR], clear_opt_parse,
                      &stats_opt_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  if ((ret = datastore_register_table(CMD_NAME,
                                      &interface_table,
                                      interface_cmd_update,
                                      interface_cmd_enable,
                                      interface_cmd_serialize,
                                      interface_cmd_destroy,
                                      interface_cmd_compare,
                                      interface_cmd_getname,
                                      interface_cmd_duplicate)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = datastore_interp_register_command(&s_interp, CONFIGURATOR_NAME,
             CMD_NAME,
             interface_cmd_parse)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

done:
  return ret;
}

static lagopus_result_t
mac_addr_show(lagopus_dstring_t *ds,
              const char *key,
              mac_address_t addr,
              bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;

  if (key != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"\"", key);
    if (ret == LAGOPUS_RESULT_OK) {
      for (i = 0; i < MAC_ADDR_STR_LEN; i++) {
        if (i == 0) {
          ret = lagopus_dstring_appendf(ds, "%02x",
                                        addr[i]);
        } else {
          ret = lagopus_dstring_appendf(ds, ":%02x",
                                        addr[i]);
        }
        if (ret != LAGOPUS_RESULT_OK) {
          goto done;
        }
      }
    } else {
      goto done;
    }
    ret = lagopus_dstring_appendf(ds, "\"");
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

static lagopus_result_t
interface_cmd_json_create(lagopus_dstring_t *ds,
                          configs_t *configs,
                          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *type_str = NULL;
  interface_attr_t *attr = NULL;
  datastore_interface_type_t type;
  char *dst_addr = NULL;
  char *src_addr = NULL;
  char *ip_addr = NULL;
  char *mcast_group = NULL;
  char *device = NULL;
  mac_address_t addr;
  uint32_t port_no;
  uint32_t ni;
  uint16_t mtu;
  uint8_t ttl;
  uint64_t flags;
  size_t i;

  ret = lagopus_dstring_appendf(ds, "[");
  if (ret == LAGOPUS_RESULT_OK) {
    for (i = 0; i < configs->size; i++) {
      if (configs->is_config == true) {
        /* config cmd. */
        if (configs->list[i]->modified_attr != NULL) {
          attr = configs->list[i]->modified_attr;
        } else {
          attr = configs->list[i]->current_attr;
        }
      } else {
        /* show cmd. */
        if (configs->is_show_modified == true) {
          if (configs->list[i]->modified_attr != NULL) {
            attr = configs->list[i]->modified_attr;
          } else {
            if (configs->size == 1) {
              ret = datastore_json_result_string_setf(
                      result, LAGOPUS_RESULT_NOT_OPERATIONAL,
                      "Not set modified.");
            } else {
              ret = LAGOPUS_RESULT_OK;
            }
            goto done;
          }
        } else {
          if (configs->list[i]->current_attr != NULL) {
            attr = configs->list[i]->current_attr;
          } else {
            if (configs->size == 1) {
              ret = datastore_json_result_string_setf(
                      result, LAGOPUS_RESULT_NOT_OPERATIONAL,
                      "Not set current.");
            } else {
              ret = LAGOPUS_RESULT_OK;
            }
            goto done;
          }
        }
      }

      if (i == 0) {
        ret = lagopus_dstring_appendf(ds, "{");
      } else {
        ret = lagopus_dstring_appendf(ds, ",\n{");
      }
      if (ret == LAGOPUS_RESULT_OK) {
        if (attr != NULL) {
          /* name */
          if ((ret = datastore_json_string_append(
                       ds, ATTR_NAME_GET(opt_strs, OPT_NAME),
                       configs->list[i]->name, false)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }

          /* get type. */
          if ((ret = interface_get_type(attr, &type)) ==
              LAGOPUS_RESULT_OK) {
            /* set show flags. */
            flags = configs->flags;
            switch (type) {
              case DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY:
                flags &= OPT_ETHERNET_DPDK_PHY;
                break;
              case DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK:
                flags &= OPT_ETHERNET_RAWSOCK;
                break;
              case DATASTORE_INTERFACE_TYPE_GRE:
                flags &= OPT_GRE;
                break;
              case DATASTORE_INTERFACE_TYPE_NVGRE:
                flags &= OPT_NVGRE;
                break;
              case DATASTORE_INTERFACE_TYPE_VXLAN:
                flags &= OPT_VXLAN;
                break;
              case DATASTORE_INTERFACE_TYPE_VHOST_USER:
                flags &= OPT_VHOST_USER;
                break;
              case DATASTORE_INTERFACE_TYPE_UNKNOWN:
                flags &= OPT_UNKNOWN;
                break;
              default:
                ret = LAGOPUS_RESULT_OUT_OF_RANGE;
                lagopus_perror(ret);
                goto done;
            }
          } else {
            lagopus_perror(ret);
            goto done;
          }

          /* type */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_TYPE)) == true) {
            if ((ret = interface_type_to_str(type, &type_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_string_append(ds,
                                                      ATTR_NAME_GET(opt_strs,
                                                          OPT_TYPE),
                                                      type_str, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* port no */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_PORT_NO)) == true) {
            if ((ret = interface_get_port_number(attr,
                                                 &port_no)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint32_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_PORT_NO),
                           port_no, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* device */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_DEVICE)) == true) {
            if ((ret = interface_get_device(attr, &device)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_string_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_DEVICE),
                           device, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* dst addr */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_DST_ADDR)) == true) {
            if ((ret = interface_get_dst_addr_str(attr,
                                                  &dst_addr)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_string_append(
                           ds, ATTR_NAME_GET(opt_strs,
                                             OPT_DST_ADDR),
                           dst_addr, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* dst port */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_DST_PORT)) == true) {
            if ((ret = interface_get_dst_port(attr,
                                              &port_no)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint32_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_DST_PORT),
                           port_no, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* src addr */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_SRC_ADDR)) == true) {
            if ((ret = interface_get_src_addr_str(attr,
                                                  &src_addr)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_string_append(
                           ds, ATTR_NAME_GET(opt_strs,
                                             OPT_SRC_ADDR),
                           src_addr, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* src port */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_SRC_PORT)) == true) {
            if ((ret = interface_get_src_port(attr,
                                              &port_no)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint32_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_SRC_PORT),
                           port_no, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* mcast group */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_MCAST_GROUP)) == true) {
            if ((ret = interface_get_mcast_group_str(attr,
                       &mcast_group)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_string_append(
                           ds, ATTR_NAME_GET(opt_strs,
                                             OPT_MCAST_GROUP),
                           mcast_group, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* ni */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_NI)) == true) {
            if ((ret = interface_get_ni(attr,
                                        &ni)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint32_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_NI),
                           ni, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* ttl */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_TTL)) == true) {
            if ((ret = interface_get_ttl(attr,
                                         &ttl)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint8_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_TTL),
                           ttl, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* dst hw addr */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_DST_HW_ADDR)) == true) {
            if ((ret = interface_get_dst_hw_addr(attr,
                                                 &addr)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = mac_addr_show(
                           ds, ATTR_NAME_GET(opt_strs,
                                             OPT_DST_HW_ADDR),
                           addr, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* src hw addr */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_SRC_HW_ADDR)) == true) {
            if ((ret = interface_get_src_hw_addr(attr,
                                                 &addr)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = mac_addr_show(
                           ds, ATTR_NAME_GET(opt_strs,
                                             OPT_SRC_HW_ADDR),
                           addr, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* mtu */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_MTU)) == true) {
            if ((ret = interface_get_mtu(attr,
                                         &mtu)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint16_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_MTU),
                           mtu, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* ip addr */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_IP_ADDR)) == true) {
            if ((ret = interface_get_ip_addr_str(attr,
                                                 &ip_addr)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_string_append(
                           ds, ATTR_NAME_GET(opt_strs,
                                             OPT_IP_ADDR),
                           ip_addr, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* used */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_IS_USED)) == true) {
            if ((ret = datastore_json_bool_append(
                         ds, ATTR_NAME_GET(opt_strs, OPT_IS_USED),
                         configs->list[i]->is_used, true)) !=
                LAGOPUS_RESULT_OK) {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* enabled */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_IS_ENABLED)) == true) {
            if ((ret = datastore_json_bool_append(ds,
                                                  ATTR_NAME_GET(opt_strs,
                                                      OPT_IS_ENABLED),
                                                  configs->list[i]->is_enabled,
                                                  true)) !=
                LAGOPUS_RESULT_OK) {
              lagopus_perror(ret);
              goto done;
            }
          }
        }
        if ((ret = lagopus_dstring_appendf(ds, "}")) != LAGOPUS_RESULT_OK) {
          goto done;
        }
      }

    done:
      /* free. */
      free(dst_addr);
      dst_addr = NULL;
      free(src_addr);
      src_addr = NULL;
      free(ip_addr);
      ip_addr = NULL;
      free(mcast_group);
      mcast_group = NULL;
      free(device);
      device = NULL;

      if (ret != LAGOPUS_RESULT_OK) {
        break;
      }
    }

    if (ret == LAGOPUS_RESULT_OK) {
      ret = lagopus_dstring_appendf(ds, "]");
    }
  }

  if (ret != LAGOPUS_RESULT_OK &&
      ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
    ret = datastore_json_result_set(result, ret, NULL);
  }

  return ret;
}

static lagopus_result_t
interface_cmd_stats_json_create(lagopus_dstring_t *ds,
                                configs_t *configs,
                                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = lagopus_dstring_appendf(ds, "[");
  if (ret == LAGOPUS_RESULT_OK) {
    ret = lagopus_dstring_appendf(ds, "{");
    if (ret == LAGOPUS_RESULT_OK) {
      if (configs->size == 1) {
        /* name */
        if ((ret = datastore_json_string_append(
                     ds, ATTR_NAME_GET(opt_strs, OPT_NAME),
                     configs->list[0]->name, false)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* rx_packets */
        if ((ret = datastore_json_uint64_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_RX_PACKETS),
                     configs->stats.rx_packets, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* rx_bytes */
        if ((ret = datastore_json_uint64_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_RX_BYTES),
                     configs->stats.rx_bytes, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* rx_dropped */
        if ((ret = datastore_json_uint64_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_RX_DROPPED),
                     configs->stats.rx_dropped, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* rx_errors */
        if ((ret = datastore_json_uint64_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_RX_ERRORS),
                     configs->stats.rx_errors, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* tx_packets */
        if ((ret = datastore_json_uint64_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_TX_PACKETS),
                     configs->stats.tx_packets, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* tx_bytes */
        if ((ret = datastore_json_uint64_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_TX_BYTES),
                     configs->stats.tx_bytes, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* tx_dropped */
        if ((ret = datastore_json_uint64_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_TX_DROPPED),
                     configs->stats.tx_dropped, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* tx_errors */
        if ((ret = datastore_json_uint64_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_TX_ERRORS),
                     configs->stats.tx_errors, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        if ((ret = lagopus_dstring_appendf(ds, "}")) != LAGOPUS_RESULT_OK) {
          goto done;
        }
      }
      if (ret == LAGOPUS_RESULT_OK) {
        ret = lagopus_dstring_appendf(ds, "]");
      }
    }
  }

done:
  if (ret != LAGOPUS_RESULT_OK &&
      ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
    ret = datastore_json_result_set(result, ret, NULL);
  }

  return ret;
}

STATIC lagopus_result_t
interface_cmd_parse(datastore_interp_t *iptr,
                    datastore_interp_state_t state,
                    size_t argc, const char *const argv[],
                    lagopus_hashmap_t *hptr,
                    datastore_update_proc_t u_proc,
                    datastore_enable_proc_t e_proc,
                    datastore_serialize_proc_t s_proc,
                    datastore_destroy_proc_t d_proc,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t ret_for_json = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;
  void *sub_cmd_proc;
  configs_t out_configs = {0 , 0LL, false, false, false,
    {0LL, 0LL, 0LL, 0LL, 0LL, 0LL, 0LL, 0LL},
    NULL
  };
  char *name = NULL;
  char *fullname = NULL;
  char *str = NULL;
  lagopus_dstring_t conf_result = NULL;

  (void)e_proc;
  (void)s_proc;
  (void)d_proc;

  for (i = 0; i < argc; i++) {
    lagopus_msg_debug(1, "argv[" PFSZS(4, u) "]:\t'%s'\n", i, argv[i]);
  }

  if (iptr != NULL && argv != NULL && hptr != NULL &&
      u_proc != NULL && result != NULL) {

    if ((ret = lagopus_dstring_create(&conf_result)) == LAGOPUS_RESULT_OK) {
      argv++;

      if (IS_VALID_STRING(*argv) == true) {
        if ((ret = lagopus_str_unescape(*argv, "\"'", &name)) < 0) {
          goto done;
        } else {
          argv++;

          if ((ret = namespace_get_fullname(name, &fullname))
              == LAGOPUS_RESULT_OK) {
            if (IS_VALID_STRING(*argv) == true) {
              if ((ret = lagopus_hashmap_find(
                           &sub_cmd_table,
                           (void *)(*argv),
                           &sub_cmd_proc)) == LAGOPUS_RESULT_OK) {
                /* parse sub cmd. */
                if (sub_cmd_proc != NULL) {
                  ret_for_json =
                    ((sub_cmd_proc_t) sub_cmd_proc)(iptr, state,
                                                    argc, argv,
                                                    fullname, hptr,
                                                    u_proc,
                                                    (void *) &out_configs,
                                                    result);
                } else {
                  ret = LAGOPUS_RESULT_NOT_FOUND;
                  lagopus_perror(ret);
                  goto done;
                }
              } else if (ret == LAGOPUS_RESULT_NOT_FOUND) {
                ret_for_json = show_sub_cmd_parse(argv, fullname,
                                                  &out_configs, result);
              } else {
                ret_for_json = datastore_json_result_string_setf(
                                 result, LAGOPUS_RESULT_INVALID_ARGS,
                                 "sub_cmd = %s.", *argv);
              }
            } else {
              /* parse show cmd. */
              ret_for_json = show_parse(fullname, &out_configs,
                                        false, result);
            }
          } else {
            ret_for_json = datastore_json_result_string_setf(
                             result, ret, "Can't get fullname %s.", name);
          }
        }
      } else {
        /* parse show all cmd. */
        ret_for_json = show_parse(NULL, &out_configs,
                                  false, result);
      }

      /* create json for conf. */
      if (ret_for_json == LAGOPUS_RESULT_OK) {
        if (out_configs.size != 0) {
          if (out_configs.is_show_stats == true) {
            ret = interface_cmd_stats_json_create(&conf_result, &out_configs,
                                                  result);
          } else {
            ret = interface_cmd_json_create(&conf_result, &out_configs,
                                            result);
          }

          if (ret == LAGOPUS_RESULT_OK) {
            ret = lagopus_dstring_str_get(&conf_result, &str);
            if (ret != LAGOPUS_RESULT_OK) {
              goto done;
            }
          } else {
            goto done;
          }
        }
        ret = datastore_json_result_set(result, LAGOPUS_RESULT_OK, str);
      } else {
        ret = ret_for_json;
      }
    }

  done:
    /* free. */
    free(name);
    free(fullname);
    free(out_configs.list);
    free(str);
    lagopus_dstring_destroy(&conf_result);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/*
 * Public:
 */

lagopus_result_t
interface_cmd_enable_propagation(datastore_interp_t *iptr,
                                 datastore_interp_state_t state,
                                 char *name,
                                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = enable_sub_cmd_parse(iptr, state, 0, NULL,
                               name, NULL, interface_cmd_update,
                               NULL, result);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

lagopus_result_t
interface_cmd_disable_propagation(datastore_interp_t *iptr,
                                  datastore_interp_state_t state,
                                  char *name,
                                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = disable_sub_cmd_parse(iptr, state, 0, NULL,
                                name, NULL, interface_cmd_update,
                                NULL, result);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

lagopus_result_t
interface_cmd_update_propagation(datastore_interp_t *iptr,
                                 datastore_interp_state_t state,
                                 char *name,
                                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  interface_conf_t *conf = NULL;

  if (name != NULL) {
    ret = interface_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = interface_cmd_update(iptr, state,
                                 (void *) conf, result);
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_OBJECT,
                                              "name = %s", name);
    }
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS, NULL);
  }

  return ret;
}

lagopus_result_t
interface_cmd_initialize(void) {
  return initialize_internal();
}

void
interface_cmd_finalize(void) {
  lagopus_hashmap_destroy(&sub_cmd_table, true);
  sub_cmd_table = NULL;
  lagopus_hashmap_destroy(&ethernet_opt_table, true);
  ethernet_opt_table = NULL;
  lagopus_hashmap_destroy(&gre_opt_table, true);
  gre_opt_table = NULL;
  lagopus_hashmap_destroy(&nvgre_opt_table, true);
  nvgre_opt_table = NULL;
  lagopus_hashmap_destroy(&vxlan_opt_table, true);
  vxlan_opt_table = NULL;
  lagopus_hashmap_destroy(&vhost_user_opt_table, true);
  vhost_user_opt_table = NULL;
  lagopus_hashmap_destroy(&stats_opt_table, true);
  stats_opt_table = NULL;

  interface_finalize();
}
