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

/**
 *      @file   dp_apis.h
 *      @brief  Dataplane public APIs.
 */

#ifndef SRC_INCLUDE_LAGOPUS_DP_APIS_H_
#define SRC_INCLUDE_LAGOPUS_DP_APIS_H_

#include "lagopus_types.h"
#include "lagopus/datastore.h"

/**
 * Dataplane API initialization.
 */
lagopus_result_t dp_api_init(void);

/**
 * Dataplane API finish.
 */
void dp_api_fini(void);


/*
 * interface API
 */

/**
 * Create interface.
 *
 * @param[in]   name    Name of interface.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_ALREADY_EXISTS   Same name interface is exist.
 * @retval      LAGOPUS_RESULT_NO_MEMORY        Memory exhausted.
 *
 * Interface object is physical layer of packet I/O interface.
 * Using interface, port and bridge:
 * 1. create interface
 * 2. associate interface with physical port by dp_interface_info_set
 * 3. create port
 * 4. associate interface with port dy dp_port_interface_set
 * 5. create bridge
 * 6. associate port with bridge by dp_bridge_port_set
 */
lagopus_result_t
dp_interface_create(const char *name);

/**
 * Destroy interface.
 *
 * @param[in]   name    Name of interface.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Interface is not exist.
 */
lagopus_result_t
dp_interface_destroy(const char *name);

/**
 * Start interface.
 *
 * @param[in]   name    Name of interface.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Interface is not exist.
 */
lagopus_result_t
dp_interface_start(const char *name);

/**
 * Stop interface.
 *
 * @param[in]   name    Name of interface.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Interface is not exist.
 */
lagopus_result_t
dp_interface_stop(const char *name);

/**
 * Set interface information such as type, id or mtu.
 *
 * @param[in]   name            Name of interface.
 * @param[in]   interface_info  Information.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Interface is not exist.
 */
lagopus_result_t
dp_interface_info_set(const char *name,
                      datastore_interface_info_t *interface_info);

/**
 * Get hardware address associated with interface.
 *
 * @param[in]   name            Name of interface.
 * @param[out]  hw_addr         Place holder of hardware address.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Interface is not exist.
 *
 * dp_interface_hw_addr_get writes MAC address to hw_addr.
 * at least 6 bytes needed for hw_addr.
 */
lagopus_result_t
dp_interface_hw_addr_get(const char *name, uint8_t *hw_addr);

/**
 * Get statistics of interface.
 *
 * @param[in]   name            Name of interface.
 * @param[in]   stats           Place holder of statistics information.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Interface is not exist.
 */
lagopus_result_t
dp_interface_stats_get(const char *name,
                       datastore_interface_stats_t *stats);

/**
 * Clear (reset to zero) statistics of interface.
 *
 * @param[in]   name            Name of interface.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Interface is not exist.
 */
lagopus_result_t
dp_interface_stats_clear(const char *name);

/*
 * port API
 */

/**
 * Create port.
 *
 * @param[in]   name    Name of interface.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_ALREADY_EXISTS   Same name port is exist.
 * @retval      LAGOPUS_RESULT_NO_MEMORY        Memory exhausted.
 *
 * Port object is protocol layer of packet I/O interface.
 */
lagopus_result_t
dp_port_create(const char *name);

/**
 * Destroy port.
 *
 * @param[in]   name    Name of port.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Port is not exist.
 */
lagopus_result_t
dp_port_destroy(const char *name);

/**
 * Start port.
 *
 * @param[in]   name    Name of port.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Port is not exist.
 */
lagopus_result_t
dp_port_start(const char *name);

/**
 * Stop port.
 *
 * @param[in]   name    Name of port.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Port is not exist.
 */
lagopus_result_t
dp_port_stop(const char *name);

/**
 * Get port config of OpenFlow.
 *
 * @param[in]   name    Name of port.
 * @param[out]  config  Config.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Port is not exist.
 */
lagopus_result_t
dp_port_config_get(const char *name, uint32_t *config);

/**
 * Get port state of OpenFlow.
 *
 * @param[in]   name    Name of port.
 * @param[out]  state   State.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Port is not exist.
 */
lagopus_result_t
dp_port_state_get(const char *name, uint32_t *state);

/**
 * Associate port and interface.
 *
 * @param[in]   name    Name of port.
 * @param[in]   ifname  Name of interfaec.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Port or interface is not exist.
 *
 * if call dp_port_interface_set with different interface, overriden interface.
 */
lagopus_result_t
dp_port_interface_set(const char *name, const char *ifname);

/**
 * Disassociate port and interface.
 *
 * @param[in]   name    Name of port.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Port is not exist.
 */
lagopus_result_t
dp_port_interface_unset(const char *name);

/**
 * Get cookie from port name.
 */
lagopus_result_t
dp_port_cookie_get(const char *name, void **cookiep);

/**
 * Receive packet from specified port.
 */
lagopus_result_t
dp_port_rx_burst(const void *cookie, void *mbufs[], size_t nb);

#ifdef HYBRID
/**
 * Set ip address to interface object.
 * @param[in]  name   Name of port.
 * @param[in]  ip     IP address.
 * @param[in]  family INET family.
 *
 * @retval LAGOPUS_RESULT_OK Succeeded.
 */
lagopus_result_t
dp_interface_ip_set(const char *in_name, int family, const struct in_addr *ip,
                    const struct in_addr *broad, const uint8_t prefixlen);
lagopus_result_t
dp_interface_ip_unset(const char *in_name);
lagopus_result_t
dp_interface_ip_get(struct interface *ifp, int family, struct in_addr *addr,
                    struct in_addr *broad, uint8_t *prefixlen);
#endif /* HYBRID */

/**
 * Lookup internel port structure.
 *
 * @param[in]   type    Type of port.
 * @param[in]   portid  Port ID.
 *
 * @retval      != NULL         internal port structure.
 * @retval      ==NULL          Port is not found.
 */
struct port *
dp_port_lookup(int type, uint32_t portid);

/**
 * Count number of port in the system.
 *
 * @retval      Number of port.
 */
uint32_t
dp_port_count(void);

/**
 * Get port statictics.
 *
 * @param[in]   name    Name of port.
 * @param[out]  stats   Statistics.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Port is not exist.
 */
lagopus_result_t
dp_port_stats_get(const char *name,
                  datastore_port_stats_t *stats);

/**
 * Add queue to port.
 *
 * @param[in]   name            Name of port.
 * @param[in]   queue_name      Name of queue.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Port or queue is not exist.
 *
 * multiple queues can associate with port.
 */
lagopus_result_t
dp_port_queue_add(const char *name,
                  const char *queue_name);

/**
 * Delete queue from port.
 *
 * @param[in]   name            Name of port.
 * @param[in]   queue_name      Name of queue.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Port or queue is not exist.
 */
lagopus_result_t
dp_port_queue_delete(const char *name,
                     const char *queue_name);

lagopus_result_t
dp_port_policer_set(const char *name,
                    const char *policer_name);

lagopus_result_t
dp_port_policer_unset(const char *name);

/*
 * bridge API
 */

/**
 * Create bridge.
 *
 * @param[in]   name    Name of bridge.
 * @param[in]   info    Property of bridge.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_ALREADY_EXISTS   Same name bridge is exist.
 * @retval      LAGOPUS_RESULT_NO_MEMORY        Memory exhausted.
 */
lagopus_result_t
dp_bridge_create(const char *name,
                 datastore_bridge_info_t *info);

/**
 * Destroy bridge.
 *
 * @param[in]   name    Name of bridge.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Bridge is not exist.
 */
lagopus_result_t
dp_bridge_destroy(const char *name);

/**
 * Start bridge.
 *
 * @param[in]   name    Name of bridge.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Bridge is not exist.
 */
lagopus_result_t
dp_bridge_start(const char *name);

/**
 * Stop bridge.
 *
 * @param[in]   name    Name of bridge.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Bridge is not exist.
 *
 * Stop forwarding function of the bridge.
 */
lagopus_result_t
dp_bridge_stop(const char *name);

/**
 * Configure OpenFlow port of bridge.
 *
 * @param[in]   name            Name of bridge.
 * @param[in]   port_name       Name of port.
 * @param[in]   port_num        OpenFlow port number.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Bridge or port is not exist.
 */
lagopus_result_t
dp_bridge_port_set(const char *name, const char *port_name, uint32_t port_num);

/**
 * Unconfigure OpenFlow port of bridge by port name.
 *
 * @param[in]   name            Name of bridge.
 * @param[in]   port_name       Name of port.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Bridge or port is not exist.
 */
lagopus_result_t
dp_bridge_port_unset(const char *name, const char *port_name);

/**
 * Unconfigure OpenFlow port of bridge by port number.
 *
 * @param[in]   name            Name of bridge.
 * @param[in]   port_no         OpenFlow port number.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Bridge or port is not exist.
 */
lagopus_result_t
dp_bridge_port_unset_num(const char *name, uint32_t port_no);

/**
 * Lookup internel bridge structure by name.
 *
 * @param[in]   name    Name of bridge.
 *
 * @retval      != NULL         internal bridge structure.
 * @retval      ==NULL          Bridge is not found.
 */
struct bridge *
dp_bridge_lookup(const char *name);

/**
 * Lookup internel bridge structure by datapath id.
 *
 * @param[in]   dpid    Datapath ID of bridge.
 *
 * @retval      != NULL         internal bridge structure.
 * @retval      ==NULL          Bridge is not found.
 */
struct bridge *
dp_bridge_lookup_by_dpid(uint64_t dpid);

/**
 * Get bridge statictics.
 *
 * @param[in]   name    Name of bridge.
 * @param[out]  stats   Statistics.
 *
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Bridge is not exist.
 */
lagopus_result_t
dp_bridge_stats_get(const char *name,
                    datastore_bridge_stats_t *stats);

lagopus_result_t
dp_bridge_meter_list_get(const char *name,
                         datastore_bridge_meter_info_list_t *list);

lagopus_result_t
dp_bridge_meter_stats_list_get(const char *name,
                               datastore_bridge_meter_stats_list_t *list);

lagopus_result_t
dp_bridge_group_list_get(const char *name,
                         datastore_bridge_group_info_list_t *list);

lagopus_result_t
dp_bridge_group_stats_list_get(const char *name,
                               datastore_bridge_group_stats_list_t *list);

#ifdef HYBRID
/* mactable */
/**
 * Set the MAC address entry to MAC address table.
 * @param[in]  name      Name of bridge.
 * @param[in]  macaddr   MAC address.
 * @param[in]  port_num  Port number that corresponding to MAC address.
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 * @retval      LAGOPUS_RESULT_NOT_FOUND        Bridge is not exist.
 * @retval      LAGOPUS_RESULT_INVALID_ARGS     Arguments are invalid.
 * @retval      LAGOPUS_RESULT_NO_MEMORY        Memory exhausted.
 */
lagopus_result_t
dp_bridge_mactable_entry_set(const char *name, const uint8_t macaddr[], uint32_t port_num);

/**
 * Modify the MAC address entry to MAC address table.
 * @param[in]   name      Name of bridge.
 * @param[in]   macaddr   MAC address.
 * @param[in]   port_num  Port number that corresponding to MAC address.
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
dp_bridge_mactable_entry_modify(const char *name, const uint8_t macaddr[], uint32_t port_num);

/**
 * Delete the MAC address entry from MAC address table.
 * @param[in]   name      Name of bridge.
 * @param[in]   macaddr   MAC address.
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
dp_bridge_mactable_entry_delete(const char *name, const uint8_t macaddr[]);

/**
 * Clear all MAC address entries from MAC address table.
 * @param[in]   name      Name of bridge.
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
dp_bridge_mactable_entry_clear(const char *name);

/**
 * Get entries in MAC address table.
 * @param[in]   name      Name of bridge.
 * @param[out]  e         MAC address table entry.
 * @param[in]   num       Number of entries.
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
dp_bridge_mactable_entries_get(const char *name, datastore_macentry_t *e, unsigned int num);

/**
 * Get number of entries in MAC address table.
 * @param[in]   name      Name of bridge.
 * @param[out]  num       Number of entries.
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
dp_bridge_mactable_num_entries_get(const char *name, unsigned int *num);

/**
 * Get configs from MAC address table.
 * @param[in]   name         Name of bridge.
 * @param[out]  ageing_time  Ageing time.
 * @param[out]  max_entries  Number of max entries.
 * @retval      LAGOPUS_RESULT_OK               Succeeded.
 */
lagopus_result_t
dp_bridge_mactable_configs_get(const char *name, uint32_t *ageing_time, uint32_t *max_entries);
#endif /* HYBRID */

/*
 * affinition API
 */

lagopus_result_t
dp_affinition_list_get(datastore_affinition_info_list_t *list);

/*
 * queue API
 */
lagopus_result_t
dp_queue_create(const char *name,
                datastore_queue_info_t *queue_info);

void
dp_queue_destroy(const char *name);

lagopus_result_t
dp_queue_start(const char *name);

lagopus_result_t
dp_queue_stop(const char *name);

lagopus_result_t
dp_queue_stats_get(const char *name,
                   datastore_queue_stats_t *stats);

/*
 * policer API
 */
lagopus_result_t
dp_policer_create(const char *name,
                  datastore_policer_info_t *policer_info);

void
dp_policer_destroy(const char *name);

lagopus_result_t
dp_policer_start(const char *name);

lagopus_result_t
dp_policer_stop(const char *name);

lagopus_result_t
dp_policer_action_add(const char *name,
                      char *action_name);

lagopus_result_t
dp_policer_action_delete(const char *name,
                         char *action_name);

/*
 * policer-action API
 */
lagopus_result_t
dp_policer_action_create(const char *name,
                         datastore_policer_action_info_t *policer_action_info);

void
dp_policer_action_destroy(const char *name);

lagopus_result_t
dp_policer_action_start(const char *name);

lagopus_result_t
dp_policer_action_stop(const char *name);

struct bridge;
/**
 * Get flow cache statistics.
 *
 * @param[in]   bridge   Bridge.
 * @param[out]  st       Statistics of flow cache.
 */
void
dp_get_flowcache_statistics(struct bridge *bridge, struct ofcachestat *st);

struct eventq_data;

typedef lagopus_result_t (*dp_dataq_put_func_t)(uint64_t dpid,
                                                struct eventq_data **data,
                                                lagopus_chrono_t timeout);
typedef lagopus_result_t (*dp_eventq_put_func_t)(uint64_t dpid,
                                                 struct eventq_data **data,
                                                 lagopus_chrono_t timeout);

/**
 * Register data queue put function.
 *
 *      @param[in]      func    pointer of put function.
 *      @retval         Previous pointer of put function.
 *
 * Use put function for:
 * - packet-in
 */
dp_dataq_put_func_t
dp_dataq_put_func_register(dp_dataq_put_func_t func);

/**
 * Register event queue put function.
 *
 *      @param[in]      func    Pointer of put function.
 *      @retval         Previous pointer of put function.
 *
 * Use put function for:
 * - barrier reply
 * - port stats
 */
dp_eventq_put_func_t
dp_eventq_put_func_register(dp_dataq_put_func_t func);

/**
 * Process event data from agent.
 *
 *      @param[in]      dpid    Datapath id of the bridge.
 *      @param[in]      data    Data.
 *      @retval         LAGOPUS_RESULT_OK       Success.
 *
 * Event data is one of:
 * - Packet-out
 * - Barrier request
 */
lagopus_result_t
dp_process_event_data(uint64_t dpid, struct eventq_data *data);


#endif /* SRC_INCLUDE_LAGOPUS_DP_APIS_H_ */
