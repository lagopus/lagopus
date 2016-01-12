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

#ifndef __DATAPLANE_APIS_H__
#define __DATAPLANE_APIS_H__

struct port_stat;

struct bridge_stat;

enum mib2_port_status {
  up = 1,
  down = 2,
  testing = 3,
  unknown = 4,
  dormant = 5,
  notPresent = 6,
  lowerLayerDown = 7,
};

/* SNMP module expects that this function provides ifTable.ifEntry */
/**
 * Get and hold the port statistics in Datapath.
 *
 *	@param[out]	port_stat	The statistics of the ports.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If the argument 'port_stat' is NULL.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 *
 *	@details This function also ensure that port_stat_get_* functions can be called safely
 *	until port_stat_release() is called.
 *	Finally caller should call bridge_stat_release() and free() to release resource.
 *
 */
lagopus_result_t dp_get_port_stat(
  struct port_stat **port_stat);

/* SNMP module expects that this function provides ifNumber */
/**
 * Count a number of the ports in the port_stat statistics.
 *
 *	@param[in]	port_stat		The statistics of the ports.
 *	@param[out]	number	a number of the ports in the statistics.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If the argument 'port_stat' is NULL.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_count(
  const struct port_stat *port_stat,
  size_t *number);

/**
 * Release the port_stat in Datapath.
 *
 *	@param[in]	port_stat	The statistics of the ports.
 */
void port_stat_release(
  struct port_stat *port_stat);

/**
 * Get the index of the port in the dataplane.
 *
 *	@param[in]	port_stat	The statistics of the ports.
 *	@param[in]	local_index	Specify a port in the port statistics.
 *	@param[out]	inside_dataplane_index	The index of the port in the data plane.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port does not exists in port_stat or dataplane.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t dp_get_index_of_port(
  const struct port_stat *port_stat,
  size_t local_index,
  size_t *inside_dataplane_index);

/* SNMP module expects that this function provides ifTable.ifEntry.ifDescr */
/**
 * Get the name of the port.
 *
 * This function don't move the memory ownership of port_name,
 * port_name shouldn't be freed by the caller.
 *
 *	@param[in]	port_stat	The statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	port_name	The name of the port.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_get_name(
  const struct port_stat *port_stat,
  size_t index,
  const char **port_name);

/* SNMP module expects that this function provides ifTable.ifEntry.ifType */
/**
 * Get the type of the port.
 *
 *	@param[in]	port_stat	The statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	type	The type of the port.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_get_type(
  const struct port_stat *port_stat,
  size_t index,
  int32_t *type);

/**
 * Get the MTU of the port.
 *
 *	@param[in]	port_stat	The statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	mtu	The MTU of the port.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_get_mtu(
  const struct port_stat *port_stat,
  size_t index,
  int64_t *mtu);

/* SNMP module expects that this function provides ifTable.ifEntry.ifSpeed */
/**
 * Get the bps of the port.
 *
 *	@param[in]	port_stat	The statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	port_name	The name of the port.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_get_bps(
  const struct port_stat *port_stat,
  size_t index,
  uint64_t *bps);

/* SNMP module expects that this function provides ifTable.ifEntry.ifPhysAddress */
/**
 * Get the hardware address of the port.
 *
 *	@param[in]	port_stat	The statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	hw_addr	The name of the port.
 *	@param[out] length The length of hw_addr.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_get_hw_addr(
  const struct port_stat *port_stat,
  size_t index,
  const uint8_t **hw_addr,
  size_t *length);

/* SNMP module expects that this function provides ifTable.ifEntry.ifAdminStatus */
/**
 * Get the administration status of the port
 *
 *	@param[in]	port_stat	The statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	administration_status	The administration status of the port.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 *
 *	@details administration_status is up, down, or testing.
 */
lagopus_result_t port_stat_get_administration_status(
  const struct port_stat *port_stat,
  size_t index,
  enum mib2_port_status *administration_status);

/* SNMP module expects that this function provides ifTable.ifEntry.ifOperStatus */
/**
 * Get the operation status of the port
 *
 *	@param[in]	port_stat	The statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	operation_status	The operation status of the port.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 *
 *	@details operation_status is up, down, testing, unknown, dormant, notPresent, lowerLayerDown.
 */
lagopus_result_t port_stat_get_operation_status(
  const struct port_stat *port_stat,
  size_t index,
  enum mib2_port_status *operation_status);

/* SNMP module expects that this function provides ifTable.ifEntry.ifLastChange */
/**
 * Get the time when the port_stat turns into current state.
 *
 *	@param[in]	port_stat	The statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	last_change the time when the port_stat turns into current state.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_get_last_change(
  const struct port_stat *port_stat,
  size_t index,
  uint32_t *last_change);

/* SNMP module expects that this function provides ifTable.ifEntry.ifInDiscards */
/**
 * Get the bytes of inband packets dropped at the port.
 *
 *	@param[in]	port_stat	Statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	in_discards	the bytes of inband packets discarded.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_get_in_discards(
  const struct port_stat *port_stat,
  size_t index,
  uint64_t *in_discards);

/* SNMP module expects that this function provides ifTable.ifEntry.ifOutDiscards */
/**
 * Get the bytes of outband packets dropped at the port.
 *
 *	@param[in]	port_stat	The statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	in_discards	The name of the port.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_get_out_discards(
  const struct port_stat *port_stat,
  size_t index,
  uint64_t *out_discards);

/* SNMP module expects that this function provides ifTable.ifEntry.ifInOctets */
/**
 * Get the bytes of inband packets through the port.
 *
 *	@param[in]	port_stat	Statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	in_octets	the bytes of inband packets discarded.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_get_in_octets(
  const struct port_stat *port_stat,
  size_t index,
  uint64_t *in_octets);

/* SNMP module expects that this function provides ifTable.ifEntry.ifOutOctets */
/**
 * Get the bytes of outband packets through the port.
 *
 *	@param[in]	port_stat	Statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	out_octets	the bytes of inband packets discarded.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_get_out_octets(
  const struct port_stat *port_stat,
  size_t index,
  uint64_t *out_octets);

/* SNMP module expects that this function provides ifTable.ifEntry.ifInErrors */
/**
 * Get the bytes of inband packets dropped by the error at the port.
 *
 *	@param[in]	port_stat	Statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	in_errors	the bytes of inband packets discarded.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_get_in_errors(
  const struct port_stat *port_stat,
  size_t index,
  uint64_t *in_errors);

/* SNMP module expects that this function provides ifTable.ifEntry.ifOutErrors */
/**
 * Get the bytes of outband packets dropped by the error at the port.
 *
 *	@param[in]	port_stat	Statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	out_errors	the bytes of inband packets discarded.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_get_out_errors(
  const struct port_stat *port_stat,
  size_t index,
  uint64_t *out_errors);


/* SNMP module expects that this function provides ifTable.ifEntry.ifInUcast_Packets */
/**
 * Get the bytes of inband packets dropped at the port.
 *
 *	@param[in]	port_stat	Statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	in_ucast_packets	the bytes of inband packets discarded.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_get_in_ucast_packets(
  const struct port_stat *port_stat,
  size_t index,
  uint64_t *in_ucast_packets);

/* SNMP module expects that this function provides ifTable.ifEntry.ifOutUcast_Packets */
/**
 * Get the bytes of outband packets dropped at the port.
 *
 *	@param[in]	port_stat	The statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	in_ucast_packets	The name of the port.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_get_out_ucast_packets(
  const struct port_stat *port_stat,
  size_t index,
  uint64_t *out_ucast_packets);

/**
 * Get the bytes of packets discarded by MTU exceeded at the port.
 *
 *	@param[in]	port_stat	The statistics of the ports.
 *	@param[in]	index	The index of the port.
 *	@param[out]	mtu_exceeded_discards the bytes of packets discarded by MTU exceeded at the port
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t port_stat_get_mtu_exceeded_discards(
  const struct port_stat *port_stat,
  size_t index,
  uint64_t *mtu_exceeded_discards);

/**
 * Get and hold the bridge statistics.
 *
 *	@param[out]	bridge_stat	The statistics of the bridges.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 *
 *	@details This function also ensure that bridge_stat_get_* functions can be called safely
 *	until bridge_stat_release() is called.
 *	Finally caller should call bridge_stat_release() and free() to release resource.
 */
lagopus_result_t dp_get_bridge_stat(
  struct bridge_stat **bridge_stat);

/**
 * Count a number of bridges in Datapath.
 *
 *	@param[in]	bridge_stat	The instance of bridge.
 *	@param[out]	number	The number of the bridges.
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 */
lagopus_result_t bridge_stat_get_number(
  struct bridge_stat **bridge_stat,
  size_t *number);

/**
 * Get and hold the bridge.
 *
 *	@param[in]	bridge_stat	The instance of bridge.
 */
void bridge_stat_release(
  struct bridge_stat *bridge_stat);

/**
 * Get and hold the port_stat of the bridge.
 *
 *	@param[in]	bridge_stat	The instance of bridge.
 *	@param[out]	port	The instance of the port.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the port_stat does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t bridge_stat_get_port_stat(
  const struct bridge_stat *bridge_stat,
  size_t index,
  struct port_stat **port_stat);

/* SNMP module expects that this function provides dot1dBase.dot1dBaseBridgeAddress */
/**
 * Get the hardware address of the bridge.
 *
 *	@param[in]	bridge_stat	The instance of bridge.
 *	@param[out]	hw_addr	The name of the bridge.
 *	@param[out] length The length of hw_addr.
 *
 *	@retval	LAGOPUS_RESULT_OK		Succeeded.
 *	@retval LAGOPUS_RESULT_NOT_FOUND		If the bridge does not exists.
 *	@retval LAGOPUS_RESULT_INVALID_ARGS		If some arguments are NULL or invalid.
 *	@retval	LAGOPUS_RESULT_ANY_FAILURES		Failed.
 */
lagopus_result_t bridge_stat_get_hw_addr(
  const struct bridge_stat *bridge_stat,
  size_t number,
  const uint8_t **hw_addr,
  size_t *length);

#endif /* ! __DATAPLANE_APIS_H__ */
