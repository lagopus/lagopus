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

#ifndef __SNMP_DATAPLANE_INTERFACE_H__
#define __SNMP_DATAPLANE_INTERFACE_H__

#include "lagopus_apis.h"
#include "dataplane_apis.h"

void dataplane_count_ifNumber(struct port_stat *port_stat,
                              size_t *interface_number);

/* TODO: rename, this function name does not match this function do */
bool dataplane_interface_exist(struct port_stat *port_stat,
                               const size_t index);

lagopus_result_t dataplane_interface_get_ifDescr(
  struct port_stat *port_stat,
  size_t index,
  char *interface_description,
  size_t *interface_description_len);

lagopus_result_t dataplane_interface_get_ifType(
  struct port_stat *port_stat,
  size_t index,
  int32_t *type);

lagopus_result_t dataplane_interface_get_ifOutDiscards(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value);

lagopus_result_t dataplane_interface_get_ifInDiscards(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value);

lagopus_result_t dataplane_interface_get_ifInUcastPkts(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value);

lagopus_result_t dataplane_interface_get_ifOutUcastPkts(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value);

lagopus_result_t dataplane_interface_get_ifInOctets(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value);

lagopus_result_t dataplane_interface_get_ifInErrors(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value);

lagopus_result_t dataplane_interface_get_ifOutErrors(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value);

lagopus_result_t dataplane_interface_get_ifInUcastPkts(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value);

lagopus_result_t dataplane_interface_get_ifPhysAddress(
  struct port_stat *port_stat,
  size_t index,
  char *value,
  size_t *len);

lagopus_result_t dataplane_interface_get_ifOutOctets(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value);

lagopus_result_t dataplane_interface_get_ifMtu(
  struct port_stat *port_stat,
  size_t index,
  int32_t *value);

lagopus_result_t dataplane_interface_get_ifSpeed(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value);

lagopus_result_t dataplane_interface_get_ifAdminStatus(
  struct port_stat *port_stat,
  size_t index,
  int32_t *value);

lagopus_result_t dataplane_interface_get_ifOperStatus(
  struct port_stat *port_stat,
  size_t index,
  int32_t *value);

lagopus_result_t dataplane_interface_get_ifLastChange(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *value);

lagopus_result_t dataplane_bridge_count_port(
  size_t *port_num);

lagopus_result_t dataplane_bridge_stat_get_type(
  int32_t *type);

lagopus_result_t dataplane_bridge_stat_get_address(
  uint8_t *hw_address, size_t *length);

lagopus_result_t dataplane_bridge_stat_get_port_ifIndex(
  struct port_stat *port_stat,
  size_t index,
  size_t *ifindex);

lagopus_result_t dataplane_interface_get_DelayExceededDiscards(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *delayExceededDiscards);

lagopus_result_t dataplane_interface_get_MtuExceededDiscards(
  struct port_stat *port_stat,
  size_t index,
  uint32_t *mtuExceededDiscards);

#endif /* ! __SNMP_DATAPLANE_INTERFACE_H__ */
