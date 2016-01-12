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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>

#include "lagopus_apis.h"

#include "dataplane_apis.h"

#define PHYSADDRESS_MAXLEN 6

struct port_stat_0 {
  int64_t ifIndex;
  char ifDescr[1024];
  size_t ifDescr_len;
  int32_t ifType;
  int64_t ifMtu;
  uint64_t ifSpeed;
  uint8_t ifPhysAddress[PHYSADDRESS_MAXLEN];
  size_t ifPhysAddress_len;
  int32_t ifAdminStatus;
  int32_t ifOperStatus;
  uint32_t ifLastChange;
  uint64_t ifInOctets;
  uint64_t ifInUcastPkts;
  uint64_t ifInNUcastPkts;
  uint64_t ifInDiscards;
  uint64_t ifInErrors;
  uint64_t ifInUnknownProtos;
  uint64_t ifOutOctets;
  uint64_t ifOutUcastPkts;
  uint64_t ifOutNUcastPkts;
  uint64_t ifOutDiscards;
  uint64_t ifOutErrors;
  uint64_t ifOutQLen;
};

struct port_stat_0 _ports[] = {
  {
    .ifIndex = 1,
    .ifDescr = "eth0",
    .ifDescr_len = 4,
    .ifType = ARPHRD_ETHER,
    .ifMtu  = 1000,
    .ifSpeed = 1000,
    .ifPhysAddress = {0xaa, 0xaa, 0xaa, 0x00, 0x00, 0x01},
    .ifPhysAddress_len = 6,
    .ifAdminStatus = 0,
    .ifOperStatus = 0,
    .ifLastChange = 0,
    .ifInOctets = 100,
    .ifInUcastPkts = 0,
    .ifInNUcastPkts = 0,
    .ifInDiscards = 103,
    .ifInErrors = 104,
    .ifInUnknownProtos = 0,
    .ifOutOctets = 106,
    .ifOutUcastPkts = 0,
    .ifOutNUcastPkts = 0,
    .ifOutDiscards = 109,
    .ifOutErrors = 110,
    .ifOutQLen = 0
  },
  {
    .ifIndex = 2,
    .ifDescr = "lo",
    .ifDescr_len = 2,
    .ifType = ARPHRD_ETHER,
    .ifMtu = 2000,
    .ifSpeed= 2000,
    .ifPhysAddress = {0xaa, 0xaa, 0xaa, 0x00, 0x00, 0x02},
    .ifPhysAddress_len = 6,
    .ifAdminStatus = 0,
    .ifOperStatus = 0,
    .ifLastChange = 0,
    .ifInOctets= 200,
    .ifInUcastPkts= 0,
    .ifInNUcastPkts= 0,
    .ifInDiscards= 203,
    .ifInErrors= 204,
    .ifInUnknownProtos = 0,
    .ifOutOctets= 206,
    .ifOutUcastPkts= 0,
    .ifOutNUcastPkts= 0,
    .ifOutDiscards= 209,
    .ifOutErrors= 210,
    .ifOutQLen = 0
  }
};

struct port_stat {
  struct port_stat_0 *ports;
};

static size_t _port_num = 2;
static struct port_stat _port_stat = {
  .ports = _ports,
};

struct bridge_stat {
  size_t index;
};

void
port_stat_release(struct port_stat *port_stat) {
  (void) port_stat;
}

lagopus_result_t
dp_get_port_stat(struct port_stat **port_stat) {
  if (port_stat != NULL) {
    struct port_stat *p;
    p = (struct port_stat *) malloc (sizeof(*p));
    *p = _port_stat;
    *port_stat = p;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
port_stat_count(const struct port_stat *port_stat, size_t *number) {
  (void) port_stat;
  *number = _port_num;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
port_stat_get_name(const struct port_stat *port_stat,
                   size_t index,
                   const char **port_name) {
  if (index >= _port_num) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  if (port_stat != NULL && port_name != NULL) {
    *port_name = port_stat->ports[index].ifDescr;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
port_stat_get_in_discards(const struct port_stat *port_stat,
                          size_t index,
                          uint64_t *in_discards) {
  if (port_stat != NULL && in_discards != NULL) {
    *in_discards = port_stat->ports[index].ifInDiscards;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
port_stat_get_out_discards(const struct port_stat *port_stat,
                           size_t index,
                           uint64_t *out_discards) {
  if (port_stat != NULL && out_discards != NULL) {
    *out_discards = port_stat->ports[index].ifOutDiscards;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
port_stat_get_in_ucast_packets(const struct port_stat *port_stat,
                               size_t index,
                               uint64_t *in_ucast_packets) {
  if (port_stat != NULL && in_ucast_packets != NULL) {
    *in_ucast_packets = port_stat->ports[index].ifInUcastPkts;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
port_stat_get_out_ucast_packets(const struct port_stat *port_stat,
                                size_t index,
                                uint64_t *out_ucast_packets) {
  if (port_stat != NULL && out_ucast_packets != NULL) {
    *out_ucast_packets = port_stat->ports[index].ifOutUcastPkts;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
port_stat_get_in_octets(const struct port_stat *port_stat,
                        size_t index,
                        uint64_t *in_octets) {
  if (port_stat != NULL && in_octets != NULL) {
    *in_octets = port_stat->ports[index].ifInOctets;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
port_stat_get_out_octets(const struct port_stat *port_stat,
                         size_t index,
                         uint64_t *out_octets) {
  if (port_stat != NULL && out_octets != NULL) {
    *out_octets = port_stat->ports[index].ifOutOctets;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
port_stat_get_in_errors(const struct port_stat *port_stat,
                        size_t index,
                        uint64_t *in_errors) {
  if (port_stat != NULL && in_errors != NULL) {
    *in_errors = port_stat->ports[index].ifInErrors;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
port_stat_get_out_errors(const struct port_stat *port_stat,
                         size_t index,
                         uint64_t *out_errors) {
  if (port_stat != NULL && out_errors != NULL) {
    *out_errors = port_stat->ports[index].ifOutErrors;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
port_stat_get_hw_addr(const struct port_stat *port_stat,
                      size_t index,
                      const uint8_t **hw_addr,
                      size_t *length) {
  if (port_stat != NULL && hw_addr != NULL) {
    *hw_addr = port_stat->ports[index].ifPhysAddress;
    *length = 6;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

/* TODO, remove this if it turns out that the type cann't be fetched from DP */
lagopus_result_t
port_stat_get_type(const struct port_stat *port_stat,
                   size_t index,
                   int32_t *type) {
  if (port_stat != NULL && type != NULL) {
    *type = port_stat->ports[index].ifType;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
port_stat_get_mtu(const struct port_stat *port_stat,
                  size_t index,
                  int64_t *mtu) {
  if (port_stat != NULL && mtu != NULL) {
    *mtu = port_stat->ports[index].ifMtu;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
port_stat_get_bps(const struct port_stat *port_stat,
                  size_t index,
                  uint64_t *bps) {
  if (port_stat != NULL && bps != NULL) {
    *bps = port_stat->ports[index].ifSpeed;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
port_stat_get_administration_status(const struct port_stat *port_stat,
                                    size_t index,
                                    enum mib2_port_status *administration_status) {
  if (port_stat != NULL && administration_status != NULL) {
    *administration_status = port_stat->ports[index].ifAdminStatus;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
port_stat_get_operation_status(const struct port_stat *port_stat,
                               size_t index,
                               enum mib2_port_status *operation_status) {
  if (port_stat != NULL && operation_status != NULL) {
    *operation_status = port_stat->ports[index].ifOperStatus;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
port_stat_get_last_change(const struct port_stat *port_stat,
                          size_t index,
                          uint32_t *last_change) {
  if (port_stat != NULL && last_change != NULL) {
    *last_change = port_stat->ports[index].ifLastChange;
    return LAGOPUS_RESULT_OK;
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}


lagopus_result_t
dp_get_bridge_stat(
  struct bridge_stat **bridge) {
  struct bridge_stat *b;
  b = (struct bridge_stat *) malloc (sizeof(*b));
  *bridge = b;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
bridge_stat_get_port_stat(
  const struct bridge_stat *bridge_stat,
  size_t index,
  struct port_stat **port_stat) {
  struct port_stat *p;
  (void) bridge_stat;
  (void) index;
  (void) port_stat;
  p = (struct port_stat *) malloc (sizeof(*p));
  *p = _port_stat;
  *port_stat = p;
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
dp_get_index_of_port(const struct port_stat *port_stat,
                     size_t local_index,
                     size_t *inside_dataplane_index) {
  (void) port_stat;
  (void) local_index;
  (void) inside_dataplane_index;
  *inside_dataplane_index = local_index;
  return LAGOPUS_RESULT_OK;
}

void
bridge_stat_release(
  struct bridge_stat *bridge_stat) {
  (void)bridge_stat;
}

#if 0
lagopus_result_t
bridge_stat_get_type(
  const struct bridge_stat *bridge_stat,
  int32_t *type) {
  (void) bridge;
  (void) type;
  return LAGOPUS_RESULT_ANY_FAILURES;
}
#endif

lagopus_result_t
bridge_stat_get_hw_addr(
  const struct bridge_stat *bridge_stat,
  size_t index,
  const uint8_t **hw_addr,
  size_t *length) {
  static uint8_t _hw_addr[PHYSADDRESS_MAXLEN] = {0};
  (void) bridge_stat;
  (void) index;
  (void) hw_addr;
  (void) length;

  if (bridge_stat != NULL && hw_addr != NULL && length != NULL) {
    *hw_addr = _hw_addr;
    *length = sizeof(_hw_addr);
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

#if 0
lagopus_result_t
port_stat_get_delay_exceeded_discards(
  const struct port_stat *port_stat,
  size_t index,
  uint64_t *delay_exceeded_discards) {
  (void) port_stat;
  if (delay_exceeded_discards != NULL) {
    *delay_exceeded_discards = 0;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}
#endif

lagopus_result_t
port_stat_get_mtu_exceeded_discards(
  const struct port_stat *port_stat,
  size_t index,
  uint64_t *mtu_exceeded_discards) {
  (void) port_stat;
  (void) index;
  /* always return 0 */
  if (mtu_exceeded_discards != NULL) {
    *mtu_exceeded_discards = 0;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}
