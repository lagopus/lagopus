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

#include <stdint.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>

#include "lagopus_apis.h"
// TODO replace
#include "dataplane_apis.h"

#include "dataplane_interface.h"
#include "ifTable_enums.h"
#include "ifTable_type.h"
#include "port_table_common.h"

#define TO_COUNTER32(x) ((uint32_t)((x) - (((x) / UINT32_MAX) * UINT32_MAX)))
#define TO_GAUGE32(x) (((x) > UINT32_MAX)?UINT32_MAX:((uint32_t)(x)))

static int32_t
ifType_mapping(sa_family_t raw_device_type) {
  int32_t ret;
  switch (raw_device_type) {
      /* #ifdef ARPHRD_NETROM */
      /*     case ARPHRD_NETROM: */
      /* #endif */
#ifdef ARPHRD_ETHER
    case ARPHRD_ETHER:
      ret = IFTYPE_ETHERNETCSMACD;
      break;
#endif
      /* #ifdef ARPHRD_EETHER */
      /*     case ARPHRD_EETHER: */
      /* #endif */
      /* #ifdef ARPHRD_AX25 */
      /*     case ARPHRD_AX25: */
      /* #endif */
      /* #ifdef ARPHRD_PRONET */
      /*     case ARPHRD_PRONET: */
      /* #endif */
      /* #ifdef ARPHRD_CHAOS */
      /*     case ARPHRD_CHAOS: */
      /* #endif */
#ifdef ARPHRD_IEEE802
    case ARPHRD_IEEE802:
      ret = IFTYPE_IEEE80212;
      break;
#endif
#ifdef ARPHRD_ARCNET
    case ARPHRD_ARCNET:
      ret = IFTYPE_ARCNET;
      break;
#endif
      /* #ifdef ARPHRD_APPLETLK */
      /*     case ARPHRD_APPLETLK: */
      /* #endif */
      /* #ifdef ARPHRD_DLCI */
      /*     case ARPHRD_DLCI: */
      /* #endif */
#ifdef ARPHRD_ATM
    case ARPHRD_ATM:
      ret = IFTYPE_ATM;
      break;
#endif
      /* #ifdef ARPHRD_METRICOM */
      /*     case ARPHRD_METRICOM: */
      /* #endif */
#ifdef ARPHRD_IEEE1394
    case ARPHRD_IEEE1394:
      ret = IFTYPE_IEEE1394;
      break;
#endif
      /* #ifdef ARPHRD_EUI64 */
      /*     case ARPHRD_EUI64: */
      /* #endif */
#ifdef ARPHRD_INFINIBAND
    case ARPHRD_INFINIBAND:
      ret = IFTYPE_INFINIBAND;
      break;
#endif
#ifdef ARPHRD_SLIP
    case ARPHRD_SLIP:
      ret = IFTYPE_SLIP;
      break;
#endif
#ifdef ARPHRD_CSLIP
    case ARPHRD_CSLIP:
      ret = IFTYPE_SLIP;
      break;
#endif
#ifdef ARPHRD_SLIP6
    case ARPHRD_SLIP6:
      ret = IFTYPE_SLIP;
      break;
#endif
#ifdef ARPHRD_CSLIP6
    case ARPHRD_CSLIP6:
      ret = IFTYPE_SLIP;
      break;
#endif
      /* #ifdef ARPHRD_RSRVD */
      /*     case ARPHRD_RSRVD: */
      /* #endif */
      /* #ifdef ARPHRD_ADAPT */
      /*     case ARPHRD_ADAPT: */
      /* #endif */
      /* #ifdef ARPHRD_ROSE */
      /*     case ARPHRD_ROSE: */
      /* #endif */
#ifdef ARPHRD_X25
    case ARPHRD_X25:
      ret = IFTYPE_RFC877X25;
      break;
#endif
      /* #ifdef ARPHRD_HWX25 */
      /*     case ARPHRD_HWX25: */
      /* #endif */
#ifdef ARPHRD_PPP
    case ARPHRD_PPP:
      ret = IFTYPE_PPP;
      break;
#endif
      /* #ifdef ARPHRD_CISCO */
      /*     case ARPHRD_CISCO: */
      /* #endif */
#ifdef ARPHRD_HDLC
    case ARPHRD_HDLC:
      ret = IFTYPE_HDLC;
      break;
#endif
#ifdef ARPHRD_LAPB
    case ARPHRD_LAPB:
      ret = IFTYPE_LAPB;
      break;
#endif
      /* #ifdef ARPHRD_DDCMP */
      /*     case ARPHRD_DDCMP: */
      /* #endif */
#ifdef ARPHRD_RAWHDLC
    case ARPHRD_RAWHDLC:
      ret = IFTYPE_HDLC;
      break;
#endif
#ifdef ARPHRD_TUNNEL
    case ARPHRD_TUNNEL:
      ret = IFTYPE_TUNNEL;
      break;
#endif
#ifdef ARPHRD_TUNNEL6
    case ARPHRD_TUNNEL6:
      ret = IFTYPE_TUNNEL;
      break;
#endif
      /* #ifdef ARPHRD_FRAD */
      /*     case ARPHRD_FRAD: */
      /* #endif */
      /* #ifdef ARPHRD_SKIP */
      /*     case ARPHRD_SKIP: */
      /* #endif */
#ifdef ARPHRD_LOOPBACK
    case ARPHRD_LOOPBACK:
      ret = IFTYPE_SOFTWARELOOPBACK;
      break;
#endif
#ifdef ARPHRD_LOCALTLK
    case ARPHRD_LOCALTLK:
      ret = IFTYPE_LOCALTALK;
      break;
#endif
#ifdef ARPHRD_FDDI
    case ARPHRD_FDDI:
      ret = IFTYPE_FDDI;
      break;
#endif
      /* #ifdef ARPHRD_BIF */
      /*     case ARPHRD_BIF: */
      /* #endif */
#ifdef ARPHRD_SIT
    case ARPHRD_SIT:
      ret = IFTYPE_TUNNEL;
      break;
#endif
      /* #ifdef ARPHRD_IPDDP */
      /*     case ARPHRD_IPDDP: */
      /* #endif */
#ifdef ARPHRD_IPGRE
    case ARPHRD_IPGRE:
      ret = IFTYPE_TUNNEL;
      break;
#endif
      /* #ifdef ARPHRD_PIMREG */
      /*     case ARPHRD_PIMREG: */
      /* #endif */
#ifdef ARPHRD_HIPPI
    case ARPHRD_HIPPI:
      ret = IFTYPE_HIPPI;
      break;
#endif
      /* #ifdef ARPHRD_ASH */
      /*     case ARPHRD_ASH: */
      /* #endif */
#ifdef ARPHRD_ECONET
    case ARPHRD_ECONET:
      ret = IFTYPE_ECONET;
      break;
#endif
      /* #ifdef ARPHRD_IRDA */
      /*     case ARPHRD_IRDA: */
      /* #endif */
      /* #ifdef ARPHRD_FCPP */
      /*     case ARPHRD_FCPP: */
      /* #endif */
      /* #ifdef ARPHRD_FCAL */
      /*     case ARPHRD_FCAL: */
      /* #endif */
      /* #ifdef ARPHRD_FCPL */
      /*     case ARPHRD_FCPL: */
      /* #endif */
      /* #ifdef ARPHRD_FCFABRIC */
      /*     case ARPHRD_FCFABRIC: */
      /* #endif */
#ifdef ARPHRD_IEEE802_TR
    case ARPHRD_IEEE802_TR:
#endif
#ifdef ARPHRD_IEEE80211
    case ARPHRD_IEEE80211:
      ret = IFTYPE_IEEE80211;
      break;
#endif
      /* #ifdef ARPHRD_IEEE80211_PRISM */
      /*     case ARPHRD_IEEE80211_PRISM: */
      /* #endif */
      /* #ifdef ARPHRD_IEEE80211_RADIOTAP */
      /*     case ARPHRD_IEEE80211_RADIOTAP: */
      /* #endif */
      /* #ifdef ARPHRD_IEEE802154 */
      /*     case ARPHRD_IEEE802154: */
      /* #endif */
      /* #ifdef ARPHRD_IEEE802154_PHY */
      /*     case ARPHRD_IEEE802154_PHY: */
      /* #endif */
    default:
      ret = IFTYPE_OTHER;
  }
  return ret;
}

/*
 * These functions mediates between the Data-Plane and the SNMP module
 */

void
dataplane_count_ifNumber(
  struct port_stat *port_stat,
  size_t *interface_number) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  if (interface_number != NULL) {
    *interface_number = 0;
    if ((ret = port_stat_count(port_stat, interface_number))
        != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("cannot count ports: %s",
                          lagopus_error_get_string(ret));
    }
  }
}

bool
dataplane_interface_exist(
  struct port_stat *port_stat,
  const size_t index) {
  size_t num;
  dataplane_count_ifNumber(port_stat, &num);
  return (num > index);
}

lagopus_result_t
dataplane_interface_get_ifDescr(
  struct port_stat *port_stat,
  size_t index,
  char *interface_description,
  size_t *interface_description_len) {
  const char *port_name;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  if (port_stat != NULL &&
      interface_description != NULL &&
      interface_description_len != NULL) {
    if (port_stat_get_name(port_stat, index, &port_name)
        == LAGOPUS_RESULT_OK && port_name != NULL) {
      *interface_description_len = strlen(port_name);
      strncpy(interface_description, port_name,
              *interface_description_len);
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

lagopus_result_t
dataplane_interface_get_ifType(
  struct port_stat *port_stat,
  size_t index,
  int32_t *value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int32_t type;
  if (port_stat != NULL && value != NULL) {
    ret = port_stat_get_type(port_stat, index,
                             &type);
    if (ret == LAGOPUS_RESULT_OK) {
      type = ifType_mapping((sa_family_t)type);
      *value = (int32_t)type;
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

lagopus_result_t
dataplane_interface_get_ifInDiscards(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t in_discards;
  if (port_stat != NULL && value != NULL) {
    ret = port_stat_get_in_discards(port_stat, index,
                                    &in_discards);
    if (ret == LAGOPUS_RESULT_OK) {
      *value = TO_COUNTER32(in_discards);
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

lagopus_result_t
dataplane_interface_get_ifOutDiscards(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t out_discards;
  if (port_stat != NULL && value != NULL) {
    ret = port_stat_get_out_discards(port_stat, index,
                                     &out_discards);
    if (ret == LAGOPUS_RESULT_OK) {
      *value = TO_COUNTER32(out_discards);
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

lagopus_result_t
dataplane_interface_get_ifInUcastPkts(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t in_ucast_packets = 0;
  (void)index;
  if (port_stat != NULL && value != NULL) {
    ret = port_stat_get_in_ucast_packets(port_stat, index,
                                         &in_ucast_packets);
    if (ret == LAGOPUS_RESULT_OK) {
      *value = TO_COUNTER32(in_ucast_packets);
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

lagopus_result_t
dataplane_interface_get_ifOutUcastPkts(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t out_ucast_packets = 0;
  (void)index;
  if (port_stat != NULL && value != NULL) {
    ret = port_stat_get_out_ucast_packets(port_stat, index,
                                          &out_ucast_packets);
    if (ret == LAGOPUS_RESULT_OK) {
      *value = TO_COUNTER32(out_ucast_packets);
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

lagopus_result_t
dataplane_interface_get_ifInOctets(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t in_octets;
  if (port_stat != NULL && value != NULL) {
    ret = port_stat_get_in_octets(port_stat, index,
                                  &in_octets);
    if (ret == LAGOPUS_RESULT_OK) {
      *value = TO_COUNTER32(in_octets);
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}


lagopus_result_t
dataplane_interface_get_ifOutOctets(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t out_octets;
  if (port_stat != NULL && value != NULL) {
    ret = port_stat_get_out_octets(port_stat, index,
                                   &out_octets);
    if (ret == LAGOPUS_RESULT_OK) {
      *value = TO_COUNTER32(out_octets);
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

lagopus_result_t
dataplane_interface_get_ifInErrors(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t in_errors;
  if (port_stat != NULL && value != NULL) {
    ret = port_stat_get_in_errors(port_stat, index,
                                  &in_errors);
    if (ret == LAGOPUS_RESULT_OK) {
      *value = TO_COUNTER32(in_errors);
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

lagopus_result_t
dataplane_interface_get_ifOutErrors(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t out_errors;
  if (port_stat != NULL && value != NULL) {
    ret = port_stat_get_out_errors(port_stat, index,
                                   &out_errors);
    if (ret == LAGOPUS_RESULT_OK) {
      *value = TO_COUNTER32(out_errors);
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

lagopus_result_t
dataplane_interface_get_ifPhysAddress(
  struct port_stat *port_stat,
  size_t index,
  char *value,
  size_t *length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const uint8_t *hw_addr;
  if (port_stat != NULL && value != NULL) {
    ret = port_stat_get_hw_addr(port_stat, index,
                                &hw_addr,
                                length);
    if (ret == LAGOPUS_RESULT_OK) {
      memcpy(value, hw_addr, *length);
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

lagopus_result_t
dataplane_interface_get_ifMtu(
  struct port_stat *port_stat,
  size_t index,
  int32_t *value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int64_t mtu;
  if (port_stat != NULL && value != NULL) {
    ret = port_stat_get_mtu(port_stat, index,
                            &mtu);
    if (ret == LAGOPUS_RESULT_OK) {
      *value = TO_GAUGE32(mtu);
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

lagopus_result_t
dataplane_interface_get_ifSpeed(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t bps;
  if (port_stat != NULL && value != NULL) {
    ret = port_stat_get_bps(port_stat, index,
                            &bps);
    if (ret == LAGOPUS_RESULT_OK) {
      *value = TO_GAUGE32(bps);
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

lagopus_result_t
dataplane_interface_get_ifAdminStatus(
  struct port_stat *port_stat,
  size_t index,
  int32_t *value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  enum mib2_port_status administration_status;
  if (port_stat != NULL && value != NULL) {
    ret = port_stat_get_administration_status(port_stat, index,
          &administration_status);
    if (ret == LAGOPUS_RESULT_OK) {
      *value = administration_status;
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

lagopus_result_t
dataplane_interface_get_ifOperStatus(
  struct port_stat *port_stat,
  size_t index,
  int32_t *value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  enum mib2_port_status operation_status;
  if (port_stat != NULL && value != NULL) {
    ret = port_stat_get_operation_status(port_stat, index,
                                         &operation_status);
    if (ret == LAGOPUS_RESULT_OK) {
      *value = operation_status;
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

lagopus_result_t
dataplane_interface_get_ifLastChange(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint32_t last_change;
  if (port_stat != NULL && value != NULL) {
    ret = port_stat_get_last_change(port_stat, index,
                                    &last_change);
    if (ret == LAGOPUS_RESULT_OK) {
      *value = last_change;
      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

lagopus_result_t
dataplane_bridge_count_port(size_t *value) {
  if (value != NULL) {
    lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
    struct port_stat *port_stat = NULL;
    size_t port_num;
    struct bridge_stat *bridge_stat = NULL;
    if ((ret = dp_get_bridge_stat(&bridge_stat)) == LAGOPUS_RESULT_OK) {
      if (bridge_stat_get_port_stat(bridge_stat, 0,
                                    &port_stat) == LAGOPUS_RESULT_OK) {
        if ((ret = port_stat_count(port_stat, &port_num))
            == LAGOPUS_RESULT_OK) {
          *value = port_num;
        } else {
          *value = 0;
          lagopus_msg_warning("no ports found in the bridge\n");
        }
        port_stat_release(port_stat);
        free(port_stat);
      } else {
        lagopus_msg_warning("failed to get the port statistics in the bridge\n");
        bridge_stat_release(bridge_stat);
        free(bridge_stat);
      }
    } else {
      lagopus_msg_warning("no bridge\n");
    }
    return ret;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
dataplane_bridge_stat_get_type(int32_t *value) {
  if (value != NULL) {
#if 0
    lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
    int32_t type;
    struct bridge_stat *bridge_stat = NULL;
    if ((ret = dp_get_bridge_stat(&bridge_stat)) == LAGOPUS_RESULT_OK) {
      if (bridge_stat_get_type(bridge_stat, &type) == LAGOPUS_RESULT_OK) {
        *value = type;
        ret = LAGOPUS_RESULT_OK;
      } else {
        lagopus_msg_warning("failed to get the bridge type: %s",
                            lagopus_error_get_string(ret));
      }
      bridge_stat_release(bridge_stat);
      free(bridge_stat);
    } else {
      lagopus_msg_warning("failed to get the bridge statistics: %s",
                          lagopus_error_get_string(ret));
    }
    return ret;
#else
    *value = 2; // bridge is a transparent bridge
    return LAGOPUS_RESULT_OK;
#endif
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
dataplane_bridge_stat_get_address(uint8_t *value, size_t *length) {
  if (value != NULL && length != NULL) {
    lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
    const uint8_t *hw_addr;
    struct bridge_stat *bridge_stat = NULL;
    if ((ret = dp_get_bridge_stat(&bridge_stat)) == LAGOPUS_RESULT_OK) {
      if (bridge_stat_get_hw_addr(bridge_stat, 0, &hw_addr,
                                  length) == LAGOPUS_RESULT_OK) {
        value = (uint8_t *) malloc (sizeof(uint8_t) * (*length));
        memcpy(value, hw_addr, *length);
        ret = LAGOPUS_RESULT_OK;
      }
      bridge_stat_release(bridge_stat);
      free(bridge_stat);
    }
    return ret;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
dataplane_bridge_stat_get_port_ifIndex(
  struct port_stat *port_stat,
  size_t index,
  size_t *value) {
  if (port_stat != NULL && value != NULL) {
    lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
    size_t ifindex;
    if ((ret = dp_get_index_of_port(port_stat, index, &ifindex))
        == LAGOPUS_RESULT_OK) {
      *value = ifindex;
    }
    return ret;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
dataplane_interface_get_DelayExceededDiscards(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value) {
  if (port_stat != NULL && value != NULL) {
#if 0
    /* port_stat_get_delay_exceeded_discards will not be implemented */
    lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
    uint64_t delayExceededDiscards;
    struct port_stat *port_stat = NULL;
    struct bridge_stat *bridge_stat = NULL;
    ret = port_stat_get_delay_exceeded_discards(
            port_stat, index,
            &delayExceededDiscards);
    if (ret == LAGOPUS_RESULT_OK) {
      *value = TO_COUNTER32(delayExceededDiscards);
      return LAGOPUS_RESULT_OK;
    } else {
      return ret;
    }
#else
    (void) port_stat;
    (void) index;
    *value = 0;
    return LAGOPUS_RESULT_OK;
#endif
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
dataplane_interface_get_MtuExceededDiscards(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value) {
  if (port_stat != NULL && value != NULL) {
    lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
    uint32_t mtuExceededDiscards;
    ret = port_stat_get_mtu_exceeded_discards(
            port_stat, index,
            &mtuExceededDiscards);
    if (ret == LAGOPUS_RESULT_OK) {
      *value = TO_COUNTER32(mtuExceededDiscards);
      return LAGOPUS_RESULT_OK;
    } else {
      return ret;
    }
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}
