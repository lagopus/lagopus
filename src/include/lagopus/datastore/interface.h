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

#ifndef __LAGOPUS_DATASTORE_INTERFACE_H__
#define __LAGOPUS_DATASTORE_INTERFACE_H__

#include "lagopus_ip_addr.h"

typedef enum datastore_interface_type {
  DATASTORE_INTERFACE_TYPE_UNKNOWN = 0,
  DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY,
  DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV,
  DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK,
  DATASTORE_INTERFACE_TYPE_GRE,
  DATASTORE_INTERFACE_TYPE_NVGRE,
  DATASTORE_INTERFACE_TYPE_VXLAN,
  DATASTORE_INTERFACE_TYPE_VHOST_USER,
  DATASTORE_INTERFACE_TYPE_MIN = DATASTORE_INTERFACE_TYPE_UNKNOWN,
  DATASTORE_INTERFACE_TYPE_MAX = DATASTORE_INTERFACE_TYPE_VHOST_USER,
} datastore_interface_type_t;

/**
 * @brief	datastore_interface_eth_dpdk_phy
 */
typedef struct datastore_interface_eth_dpdk_phy {
  uint32_t port_number;
  const char *device;
  lagopus_ip_address_t *ip_addr;
  uint16_t mtu;
} datastore_interface_eth;

/**
 * @brief	datastore_interface_eth_rawsock
 */
struct datastore_interface_eth_rawsock {
  uint32_t port_number;
  const char *device;
  lagopus_ip_address_t *ip_addr;
  uint16_t mtu;
};

/**
 * @brief	datastore_interface_vxla
 */
struct datastore_interface_vxlan {
  uint32_t port_number;
  const char *device;
  lagopus_ip_address_t *dst_addr;
  uint32_t dst_port;
  lagopus_ip_address_t *mcast_group;
  lagopus_ip_address_t *src_addr;
  lagopus_ip_address_t *ip_addr;
  uint32_t src_port;
  uint32_t ni;
  uint8_t ttl;
  uint16_t mtu;
};

/**
 * @brief	datastore_interface_info_t
 */
typedef struct datastore_interface_info {
  datastore_interface_type_t type;
  union {
    /* TODO: delete datastore_interface_eth. */
    datastore_interface_eth eth;
    struct datastore_interface_eth_dpdk_phy eth_dpdk_phy;
    struct datastore_interface_eth_rawsock eth_rawsock;
    struct datastore_interface_vxlan vxlan;
    /*
     * TODO: Add gre, nvgre, vhost-user.
     */
  };
} datastore_interface_info_t;

/**
 * @brief	datastore_interface_stats_t
 */
typedef struct datastore_interface_stats {
  uint64_t rx_packets;
  uint64_t rx_bytes;
  uint64_t rx_dropped;
  uint64_t rx_errors;
  uint64_t tx_packets;
  uint64_t tx_bytes;
  uint64_t tx_dropped;
  uint64_t tx_errors;
} datastore_interface_stats_t;

/**
 * Get the value to attribute 'enabled' of the interface table record'
 *
 *  @param[in] name
 *  @param[out] enabled the value of attribute 'enabled'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'enabled' getted sucessfully.
 */
lagopus_result_t
datastore_interface_is_enabled(const char *name, bool *enabled);

/**
 * Get the value to attribute 'used' of the interface table record'
 *
 *  @param[in] name
 *  @param[out] used the value of attribute 'used'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'used' getted sucessfully.
 */
lagopus_result_t
datastore_interface_is_used(const char *name, bool *used);

/**
 * Get the value to attribute 'port_number' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] port_number the value of attribute 'port_number'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'port_number' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_port_number(const char *name, bool current,
                                    uint32_t *port_number);

/**
 * Get the value to attribute 'device' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] device the value of attribute 'device'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'device' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_device(const char *name, bool current,
                               char **device);

/**
 * Get the value to attribute 'type' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] type the value of attribute 'type'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'type' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_type(const char *name, bool current,
                             datastore_interface_type_t *type);

/**
 * Get the value to attribute 'dst_addr' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] dst_addr the value of attribute 'dst_addr'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'dst_addr' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_dst_addr(const char *name, bool current,
                                 lagopus_ip_address_t **dst_addr);

/**
 * Get the value to attribute 'dst_addr' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] dst_addr the value of attribute 'dst_addr'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'dst_addr' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_dst_addr_str(const char *name, bool current,
                                     char **dst_addr);

/**
 * Get the value to attribute 'dst_port' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] dst_port the value of attribute 'dst_port'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'dst_port' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_dst_port(const char *name, bool current,
                                 uint32_t *dst_port);

/**
 * Get the value to attribute 'mcast_group' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] mcast_group the value of attribute 'mcast_group'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'mcast_group' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_mcast_group(const char *name, bool current,
                                    lagopus_ip_address_t **mcast_group);

/**
 * Get the value to attribute 'mcast_group' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] mcast_group the value of attribute 'mcast_group'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'mcast_group' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_mcast_group_str(const char *name, bool current,
                                        char **mcast_group);

/**
 * Get the value to attribute 'src_addr' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] src_addr the value of attribute 'src_addr'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'src_addr' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_src_addr(const char *name, bool current,
                                 lagopus_ip_address_t **src_addr);

/**
 * Get the value to attribute 'src_addr' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] src_addr the value of attribute 'src_addr'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'src_addr' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_src_addr_str(const char *name, bool current,
                                     char **src_addr);

/**
 * Get the value to attribute 'src_port' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] src_port the value of attribute 'src_port'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'src_port' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_src_port(const char *name, bool current,
                                 uint32_t *src_port);

/**
 * Get the value to attribute 'ni' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] ni the value of attribute 'ni'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'ni' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_ni(const char *name, bool current, uint32_t *ni);

/**
 * Get the value to attribute 'ttl' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] ttl the value of attribute 'ttl'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'ttl' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_ttl(const char *name, bool current, uint8_t *ttl);

/**
 * Get the value to attribute 'dst_hw_addr' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] dst_hw_addr the value of attribute 'dst_hw_addr'
 *  @param[in] len
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'dst_hw_addr' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_dst_hw_addr(const char *name, bool current,
                                    uint8_t *dst_hw_addr, size_t len);

/**
 * Get the value to attribute 'src_hw_addr' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] src_hw_addr the value of attribute 'src_hw_addr'
 *  @param[in] len
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'src_hw_addr' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_src_hw_addr(const char *name, bool current,
                                    uint8_t *src_hw_addr, size_t len);

/**
 * Get the value to attribute 'mtu' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] mtu the value of attribute 'mtu'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'mtu' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_mtu(const char *name, bool current, uint16_t *mtu);

/**
 * Get the value to attribute 'ip_addr' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] ip_addr the value of attribute 'ip_addr'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'ip_addr' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_ip_addr(const char *name, bool current,
                                lagopus_ip_address_t **ip_addr);

/**
 * Get the value to attribute 'ip_addr' of the interface table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] ip_addr the value of attribute 'ip_addr'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'ip_addr' getted sucessfully.
 */
lagopus_result_t
datastore_interface_get_ip_addr_str(const char *name, bool current,
                                    char **ip_addr);

#endif /* ! __LAGOPUS_DATASTORE_INTERFACE_H__ */

