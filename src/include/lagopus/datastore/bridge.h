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

#ifndef __LAGOPUS_DATASTORE_BRIDGE_H__
#define __LAGOPUS_DATASTORE_BRIDGE_H__

#include "openflow.h"
#include "lagopus/meter.h"
#include "lagopus/group.h"
#include "lagopus/ofp_multipart_apis.h"
#include "lagopus/datastore/common.h"

#define TYPE_BIT_GET(_opt) (1 << (_opt))

typedef enum datastore_bridge_fail_mode {
  DATASTORE_BRIDGE_FAIL_MODE_UNKNOWN = 0,
  DATASTORE_BRIDGE_FAIL_MODE_SECURE,
  DATASTORE_BRIDGE_FAIL_MODE_STANDALONE,
  DATASTORE_BRIDGE_FAIL_MODE_MIN = DATASTORE_BRIDGE_FAIL_MODE_UNKNOWN,
  DATASTORE_BRIDGE_FAIL_MODE_MAX = DATASTORE_BRIDGE_FAIL_MODE_STANDALONE,
} datastore_bridge_fail_mode_t;

typedef enum datastore_bridge_action_type {
  DATASTORE_BRIDGE_ACTION_TYPE_UNKNOWN = 0,
  DATASTORE_BRIDGE_ACTION_TYPE_COPY_TTL_OUT,
  DATASTORE_BRIDGE_ACTION_TYPE_COPY_TTL_IN,
  DATASTORE_BRIDGE_ACTION_TYPE_SET_MPLS_TTL,
  DATASTORE_BRIDGE_ACTION_TYPE_DEC_MPLS_TTL,
  DATASTORE_BRIDGE_ACTION_TYPE_PUSH_VLAN,
  DATASTORE_BRIDGE_ACTION_TYPE_POP_VLAN,
  DATASTORE_BRIDGE_ACTION_TYPE_PUSH_MPLS,
  DATASTORE_BRIDGE_ACTION_TYPE_POP_MPLS,
  DATASTORE_BRIDGE_ACTION_TYPE_SET_QUEUE,
  DATASTORE_BRIDGE_ACTION_TYPE_GROUP,
  DATASTORE_BRIDGE_ACTION_TYPE_SET_NW_TTL,
  DATASTORE_BRIDGE_ACTION_TYPE_DEC_NW_TTL,
  DATASTORE_BRIDGE_ACTION_TYPE_SET_FIELD,
  DATASTORE_BRIDGE_ACTION_TYPE_MIN = DATASTORE_BRIDGE_ACTION_TYPE_UNKNOWN,
  DATASTORE_BRIDGE_ACTION_TYPE_MAX = DATASTORE_BRIDGE_ACTION_TYPE_SET_FIELD,
} datastore_bridge_action_type_t;

#define DATASTORE_BRIDGE_BIT_ACTION_TYPE_UNKNOWN            \
  TYPE_BIT_GET(DATASTORE_BRIDGE_ACTION_TYPE_UNKNOWN)
#define DATASTORE_BRIDGE_BIT_ACTION_TYPE_COPY_TTL_OUT       \
  TYPE_BIT_GET(DATASTORE_BRIDGE_ACTION_TYPE_COPY_TTL_OUT)
#define DATASTORE_BRIDGE_BIT_ACTION_TYPE_COPY_TTL_IN        \
  TYPE_BIT_GET(DATASTORE_BRIDGE_ACTION_TYPE_COPY_TTL_IN)
#define DATASTORE_BRIDGE_BIT_ACTION_TYPE_SET_MPLS_TTL       \
  TYPE_BIT_GET(DATASTORE_BRIDGE_ACTION_TYPE_SET_MPLS_TTL)
#define DATASTORE_BRIDGE_BIT_ACTION_TYPE_DEC_MPLS_TTL       \
  TYPE_BIT_GET(DATASTORE_BRIDGE_ACTION_TYPE_DEC_MPLS_TTL)
#define DATASTORE_BRIDGE_BIT_ACTION_TYPE_PUSH_VLAN          \
  TYPE_BIT_GET(DATASTORE_BRIDGE_ACTION_TYPE_PUSH_VLAN)
#define DATASTORE_BRIDGE_BIT_ACTION_TYPE_POP_VLAN           \
  TYPE_BIT_GET(DATASTORE_BRIDGE_ACTION_TYPE_POP_VLAN)
#define DATASTORE_BRIDGE_BIT_ACTION_TYPE_PUSH_MPLS          \
  TYPE_BIT_GET(DATASTORE_BRIDGE_ACTION_TYPE_PUSH_MPLS)
#define DATASTORE_BRIDGE_BIT_ACTION_TYPE_POP_MPLS           \
  TYPE_BIT_GET(DATASTORE_BRIDGE_ACTION_TYPE_POP_MPLS)
#define DATASTORE_BRIDGE_BIT_ACTION_TYPE_SET_QUEUE          \
  TYPE_BIT_GET(DATASTORE_BRIDGE_ACTION_TYPE_SET_QUEUE)
#define DATASTORE_BRIDGE_BIT_ACTION_TYPE_GROUP              \
  TYPE_BIT_GET(DATASTORE_BRIDGE_ACTION_TYPE_GROUP)
#define DATASTORE_BRIDGE_BIT_ACTION_TYPE_SET_NW_TTL         \
  TYPE_BIT_GET(DATASTORE_BRIDGE_ACTION_TYPE_SET_NW_TTL)
#define DATASTORE_BRIDGE_BIT_ACTION_TYPE_DEC_NW_TTL         \
  TYPE_BIT_GET(DATASTORE_BRIDGE_ACTION_TYPE_DEC_NW_TTL)
#define DATASTORE_BRIDGE_BIT_ACTION_TYPE_SET_FIELD          \
  TYPE_BIT_GET(DATASTORE_BRIDGE_ACTION_TYPE_SET_FIELD)

typedef enum datastore_bridge_instruction_type {
  DATASTORE_BRIDGE_INSTRUCTION_TYPE_UNKNOWN = 0,
  DATASTORE_BRIDGE_INSTRUCTION_TYPE_APPLY_ACTIONS,
  DATASTORE_BRIDGE_INSTRUCTION_TYPE_CLEAR_ACTIONS,
  DATASTORE_BRIDGE_INSTRUCTION_TYPE_WRITE_ACTIONS,
  DATASTORE_BRIDGE_INSTRUCTION_TYPE_WRITE_METADATA,
  DATASTORE_BRIDGE_INSTRUCTION_TYPE_GOTO_TABLE,
  DATASTORE_BRIDGE_INSTRUCTION_TYPE_MIN = DATASTORE_BRIDGE_INSTRUCTION_TYPE_UNKNOWN,
  DATASTORE_BRIDGE_INSTRUCTION_TYPE_MAX = DATASTORE_BRIDGE_INSTRUCTION_TYPE_GOTO_TABLE,
} datastore_bridge_instruction_type_t;

#define DATASTORE_BRIDGE_BIT_INSTRUCTION_TYPE_UNKNOWN           \
  TYPE_BIT_GET(DATASTORE_BRIDGE_INSTRUCTION_TYPE_UNKNOWN)
#define DATASTORE_BRIDGE_BIT_INSTRUCTION_TYPE_APPLY_ACTIONS     \
  TYPE_BIT_GET(DATASTORE_BRIDGE_INSTRUCTION_TYPE_APPLY_ACTIONS)
#define DATASTORE_BRIDGE_BIT_INSTRUCTION_TYPE_CLEAR_ACTIONS     \
  TYPE_BIT_GET(DATASTORE_BRIDGE_INSTRUCTION_TYPE_CLEAR_ACTIONS)
#define DATASTORE_BRIDGE_BIT_INSTRUCTION_TYPE_WRITE_ACTIONS     \
  TYPE_BIT_GET(DATASTORE_BIT_BRIDGE_INSTRUCTION_TYPE_WRITE_ACTIONS)
#define DATASTORE_BRIDGE_BIT_INSTRUCTION_TYPE_WRITE_METADATA    \
  TYPE_BIT_GET(DATASTORE_BRIDGE_INSTRUCTION_TYPE_WRITE_METADATA)
#define DATASTORE_BRIDGE_BIT_INSTRUCTION_TYPE_GOTO_TABLE        \
  TYPE_BIT_GET(DATASTORE_BRIDGE_INSTRUCTION_TYPE_GOTO_TABLE)

typedef enum datastore_bridge_capability {
  DATASTORE_BRIDGE_CAPABILITY_UNKNOWN = 0,
  DATASTORE_BRIDGE_CAPABILITY_FLOW_STATS ,
  DATASTORE_BRIDGE_CAPABILITY_GROUP_STATS,
  DATASTORE_BRIDGE_CAPABILITY_PORT_STATS,
  DATASTORE_BRIDGE_CAPABILITY_QUEUE_STATS,
  DATASTORE_BRIDGE_CAPABILITY_TABLE_STATS,
  DATASTORE_BRIDGE_CAPABILITY_REASSEMBLE_IP_FRGS,
  DATASTORE_BRIDGE_CAPABILITY_BLOCK_LOOPING_PORTS,
  DATASTORE_BRIDGE_CAPABILITY_MIN = DATASTORE_BRIDGE_CAPABILITY_UNKNOWN,
  DATASTORE_BRIDGE_CAPABILITY_MAX = DATASTORE_BRIDGE_CAPABILITY_BLOCK_LOOPING_PORTS,
} datastore_bridge_capability_t;

#define DATASTORE_BRIDGE_BIT_CAPABILITY_UNKNOWN                \
  TYPE_BIT_GET(DATASTORE_BRIDGE_CAPABILITY_UNKNOWN)
#define DATASTORE_BRIDGE_BIT_CAPABILITY_FLOW_STATS             \
  TYPE_BIT_GET(DATASTORE_BRIDGE_CAPABILITY_FLOW_STATS)
#define DATASTORE_BRIDGE_BIT_CAPABILITY_GROUP_STATS            \
  TYPE_BIT_GET(DATASTORE_BRIDGE_CAPABILITY_GROUP_STATS)
#define DATASTORE_BRIDGE_BIT_CAPABILITY_PORT_STATS             \
  TYPE_BIT_GET(DATASTORE_BRIDGE_CAPABILITY_PORT_STATS)
#define DATASTORE_BRIDGE_BIT_CAPABILITY_QUEUE_STATS            \
  TYPE_BIT_GET(DATASTORE_BRIDGE_CAPABILITY_QUEUE_STATS)
#define DATASTORE_BRIDGE_BIT_CAPABILITY_TABLE_STATS            \
  TYPE_BIT_GET(DATASTORE_BRIDGE_CAPABILITY_TABLE_STATS)
#define DATASTORE_BRIDGE_BIT_CAPABILITY_REASSEMBLE_IP_FRGS     \
  TYPE_BIT_GET(DATASTORE_BRIDGE_CAPABILITY_REASSEMBLE_IP_FRGS)
#define DATASTORE_BRIDGE_BIT_CAPABILITY_BLOCK_LOOPING_PORTS    \
  TYPE_BIT_GET(DATASTORE_BRIDGE_CAPABILITY_BLOCK_LOOPING_PORTS)

typedef enum datastore_bridge_reserved_port_type {
  DATASTORE_BRIDGE_RESERVED_PORT_TYPE_UNKNOWN = 0,
  DATASTORE_BRIDGE_RESERVED_PORT_TYPE_ALL,
  DATASTORE_BRIDGE_RESERVED_PORT_TYPE_CONTROLLER,
  DATASTORE_BRIDGE_RESERVED_PORT_TYPE_TABLE,
  DATASTORE_BRIDGE_RESERVED_PORT_TYPE_INPORT,
  DATASTORE_BRIDGE_RESERVED_PORT_TYPE_ANY,
  DATASTORE_BRIDGE_RESERVED_PORT_TYPE_NORMAL,
  DATASTORE_BRIDGE_RESERVED_PORT_TYPE_FLOOD,
  DATASTORE_BRIDGE_RESERVED_PORT_TYPE_MIN = DATASTORE_BRIDGE_RESERVED_PORT_TYPE_UNKNOWN,
  DATASTORE_BRIDGE_RESERVED_PORT_TYPE_MAX = DATASTORE_BRIDGE_RESERVED_PORT_TYPE_FLOOD,
} datastore_bridge_reserved_port_type_t;

#define DATASTORE_BRIDGE_BIT_PORT_TYPE_UNKNOWN          \
  TYPE_BIT_GET(DATASTORE_BRIDGE_PORT_TYPE_UNKNOWN)
#define DATASTORE_BRIDGE_BIT_PORT_TYPE_ALL              \
  TYPE_BIT_GET(DATASTORE_BRIDGE_PORT_TYPE_ALL)
#define DATASTORE_BRIDGE_BIT_PORT_TYPE_CONTROLLER       \
  TYPE_BIT_GET(DATASTORE_BRIDGE_PORT_TYPE_CONTROLLER)
#define DATASTORE_BRIDGE_BIT_PORT_TYPE_TABLE            \
  TYPE_BIT_GET(DATASTORE_BRIDGE_PORT_TYPE_TABLE)
#define DATASTORE_BRIDGE_BIT_PORT_TYPE_INPORT           \
  TYPE_BIT_GET(DATASTORE_BRIDGE_PORT_TYPE_INPORT)
#define DATASTORE_BRIDGE_BIT_PORT_TYPE_ANY              \
  TYPE_BIT_GET(DATASTORE_BRIDGE_PORT_TYPE_ANY)
#define DATASTORE_BRIDGE_BIT_PORT_TYPE_NORMAL           \
  TYPE_BIT_GET(DATASTORE_BRIDGE_PORT_TYPE_NORMAL)
#define DATASTORE_BRIDGE_BIT_PORT_TYPE_FLOOD            \
  TYPE_BIT_GET(DATASTORE_BRIDGE_PORT_TYPE_FLOOD)

typedef enum datastore_bridge_group_type {
  DATASTORE_BRIDGE_GROUP_TYPE_UNKNOWN = 0,
  DATASTORE_BRIDGE_GROUP_TYPE_ALL,
  DATASTORE_BRIDGE_GROUP_TYPE_SELECT,
  DATASTORE_BRIDGE_GROUP_TYPE_INDIRECT,
  DATASTORE_BRIDGE_GROUP_TYPE_FAST_FAILOVER,
  DATASTORE_BRIDGE_GROUP_TYPE_MIN = DATASTORE_BRIDGE_GROUP_TYPE_UNKNOWN,
  DATASTORE_BRIDGE_GROUP_TYPE_MAX = DATASTORE_BRIDGE_GROUP_TYPE_FAST_FAILOVER,
} datastore_bridge_group_type_t;

#define DATASTORE_BRIDGE_BIT_GROUP_TYPE_UNKNOWN         \
  TYPE_BIT_GET(DATASTORE_BRIDGE_GROUP_TYPE_UNKNOWN)
#define DATASTORE_BRIDGE_BIT_GROUP_TYPE_ALL             \
  TYPE_BIT_GET(DATASTORE_BRIDGE_GROUP_TYPE_ALL)
#define DATASTORE_BRIDGE_BIT_GROUP_TYPE_SELECT          \
  TYPE_BIT_GET(DATASTORE_BRIDGE_GROUP_TYPE_SELECT)
#define DATASTORE_BRIDGE_BIT_GROUP_TYPE_INDIRECT        \
  TYPE_BIT_GET(DATASTORE_BRIDGE_GROUP_TYPE_INDIRECT)
#define DATASTORE_BRIDGE_BIT_GROUP_TYPE_FAST_FAILOVER   \
  TYPE_BIT_GET(DATASTORE_BRIDGE_GROUP_TYPE_FAST_FAILOVER)

typedef enum datastore_bridge_group_capability {
  DATASTORE_BRIDGE_GROUP_CAPABILITY_UNKNOWN = 0,
  DATASTORE_BRIDGE_GROUP_CAPABILITY_SELECT_WEIGHT,
  DATASTORE_BRIDGE_GROUP_CAPABILITY_SELECT_LIVENESS,
  DATASTORE_BRIDGE_GROUP_CAPABILITY_CHAINING,
  DATASTORE_BRIDGE_GROUP_CAPABILITY_CHAINING_CHECK,
  DATASTORE_BRIDGE_GROUP_CAPABILITY_MIN = DATASTORE_BRIDGE_GROUP_CAPABILITY_UNKNOWN,
  DATASTORE_BRIDGE_GROUP_CAPABILITY_MAX = DATASTORE_BRIDGE_GROUP_CAPABILITY_CHAINING_CHECK,
} datastore_bridge_group_capability_t;

#define DATASTORE_BRIDGE_BIT_GROUP_CAPABILITY_TYPE_UNKNOWN              \
  TYPE_BIT_GET(DATASTORE_BRIDGE_GROUP_CAPABILITY_TYPE_UNKNOWN)
#define DATASTORE_BRIDGE_BIT_GROUP_CAPABILITY_TYPE_SELECT_WEIGHT        \
  TYPE_BIT_GET(DATASTORE_BRIDGE_GROUP_CAPABILITY_TYPE_SELECT_WEIGHT)
#define DATASTORE_BRIDGE_BIT_GROUP_CAPABILITY_TYPE_SELECT_LIVENESS      \
  TYPE_BIT_GET(DATASTORE_BRIDGE_GROUP_CAPABILITY_TYPE_SELECT_LIVENESS)
#define DATASTORE_BRIDGE_BIT_GROUP_CAPABILITY_TYPE_CHAINING             \
  TYPE_BIT_GET(DATASTORE_BRIDGE_GROUP_CAPABILITY_TYPE_CHAINING)
#define DATASTORE_BRIDGE_BIT_GROUP_CAPABILITY_TYPE_CHAINING_CHECK       \
  TYPE_BIT_GET(DATASTORE_BRIDGE_GROUP_CAPABILITY_TYPE_CHAINING_CHECK)

/**
 * @brief	datastore_bridge_info_t
 */
typedef struct datastore_bridge_info {
  uint64_t dpid;
  datastore_bridge_fail_mode_t fail_mode;
  uint32_t max_buffered_packets;
  uint32_t max_ports;
  uint8_t max_tables;
  uint32_t max_flows;
#ifdef HYBRID
  bool l2_bridge;
  uint32_t mactable_ageing_time;
  uint32_t mactable_max_entries;
#endif /* HYBRID */
  uint64_t capabilities;          /* flags (DATASTORE_BRIDGE_CAPABILITY_TYPE_*) */
  uint64_t action_types;          /* flags (DATASTORE_BRIDGE_ACTION_TYPE_*) */
  uint64_t instruction_types;     /* flags (DATASTORE_BRIDGE_INSTRUCTION_TYPE_*) */
  uint64_t reserved_port_types;   /* flags (DATASTORE_BRIDGE_PORT_TYPE_*) */
  uint64_t group_types;           /* flags (DATASTORE_BRIDGE_GROUP_TYPE_*) */
  uint64_t group_capabilities;    /* flags (DATASTORE_BRIDGE_GROUP_CAPABILITY_TYPE_*) */
} datastore_bridge_info_t;

/**
 * @brief	datastore_bridge_queue_info_t
 */
typedef struct datastore_bridge_queue_info {
  uint16_t packet_inq_size;
  uint16_t packet_inq_max_batches;
  uint16_t up_streamq_size;
  uint16_t up_streamq_max_batches;
  uint16_t down_streamq_size;
  uint16_t down_streamq_max_batches;
} datastore_bridge_queue_info_t;

/**
 * @brief	datastore_bridge_stats_t
 */
typedef struct datastore_bridge_stats {
  uint16_t packet_inq_entries;
  uint16_t up_streamq_entries;
  uint16_t down_streamq_entries;
  uint64_t flowcache_entries;
  uint64_t flowcache_hit;
  uint64_t flowcache_miss;
  uint64_t flow_entries;
  uint64_t flow_lookup_count;
  uint64_t flow_matched_count;
  struct table_stats_list flow_table_stats;
} datastore_bridge_stats_t;


/**
 * @brief	datastore_bridge_meter_info_list_t
 */
typedef struct meter_config_list datastore_bridge_meter_info_list_t;

/**
 * @brief	datastore_bridge_meter_stats_list_t
 */
typedef struct meter_stats_list datastore_bridge_meter_stats_list_t;

/**
 * @brief	datastore_bridge_group_info_list_t
 */
typedef struct group_desc_list   datastore_bridge_group_info_list_t;

/**
 * @brief	datastore_bridge_group_stats_list_t
 */
typedef struct group_stats_list datastore_bridge_group_stats_list_t;

/**
 * Get the value to attribute 'enabled' of the bridge table record'
 *
 *  @param[in] name
 *  @param[out] enabled the value of attribute 'enabled'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'enabled' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_is_enabled(const char *name, bool *enabled);

/**
 * Get the value to attribute 'used' of the bridge table record'
 *
 *  @param[in] name
 *  @param[out] used the value of attribute 'used'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'used' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_is_used(const char *name, bool *used);

/**
 * Get the value to attribute 'dpid' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] dpid the value of attribute 'dpid'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'dpid' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_dpid(const char *name, bool current,
                          uint64_t *dpid);

/**
 * Get the value to attribute 'controller_names' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] controller_names the value of attribute 'controller_names'
 *  @param[in] size
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'controller_names' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_controller_names(const char *name,
                                      bool current,
                                      datastore_name_info_t **controller_names);

/**
 * Get the value to attribute 'port_names' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] port_names the value of attribute 'port_names'
 *  @param[in] size
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'port_names' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_port_names(const char *name,
                                bool current,
                                datastore_name_info_t **port_names);

/**
 * Get the value to attribute 'fail_mode' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] fail_mode the value of attribute 'fail_mode'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'fail_mode' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_fail_mode(const char *name, bool current,
                               datastore_bridge_fail_mode_t *fail_mode);

/**
 * Get the value to attribute 'flow_statistics' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] flow_statistics the value of attribute 'flow_statistics'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'flow_statistics' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_is_flow_statistics(const char *name, bool current,
                                    bool *flow_statistics);

/**
 * Get the value to attribute 'group_statistics' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] group_statistics the value of attribute 'group_statistics'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'group_statistics' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_is_group_statistics(const char *name, bool current,
                                     bool *group_statistics);

/**
 * Get the value to attribute 'port_statistics' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] port_statistics the value of attribute 'port_statistics'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'port_statistics' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_is_port_statistics(const char *name, bool current,
                                    bool *port_statistics);

/**
 * Get the value to attribute 'queue_statistics' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] queue_statistics the value of attribute 'queue_statistics'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'queue_statistics' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_is_queue_statistics(const char *name, bool current,
                                     bool *queue_statistics);

/**
 * Get the value to attribute 'table_statistics' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] table_statistics the value of attribute 'table_statistics'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'table_statistics' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_is_table_statistics(const char *name, bool current,
                                     bool *table_statistics);

/**
 * Get the value to attribute 'reassemble_ip_fragments' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] reassemble_ip_fragments the value of attribute 'reassemble_ip_fragments'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'reassemble_ip_fragments' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_is_reassemble_ip_fragments(const char *name, bool current,
    bool *reassemble_ip_fragments);

/**
 * Get the value to attribute 'max_buffered_packets' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] max_buffered_packets the value of attribute 'max_buffered_packets'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'max_buffered_packets' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_max_buffered_packets(const char *name, bool current,
    uint32_t *max_buffered_packets);

/**
 * Get the value to attribute 'max_ports' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] max_ports the value of attribute 'max_ports'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'max_ports' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_max_ports(const char *name, bool current,
                               uint16_t *max_ports);

/**
 * Get the value to attribute 'max_tables' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] max_tables the value of attribute 'max_tables'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'max_tables' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_max_tables(const char *name, bool current,
                                uint8_t *max_tables);

/**
 * Get the value to attribute 'block_looping_ports' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] block_looping_ports the value of attribute 'block_looping_ports'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'block_looping_ports' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_is_block_looping_ports(const char *name, bool current,
                                        bool *block_looping_ports);

/**
 * Get the value to attribute 'action_types' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] action_types the value of attribute 'action_types'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'action_types' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_action_types(const char *name, bool current,
                                  uint64_t *action_types);

/**
 * Get the value to attribute 'queue_statistics' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] queue_statistics the value of attribute 'queue_statistics'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'queue_statistics' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_instruction_types(const char *name, bool current,
                                       uint64_t *instruction_types);

/**
 * Get the value to attribute 'max_flows' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] max_flows the value of attribute 'max_flows'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'max_flows' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_max_flows(const char *name, bool current,
                               uint32_t *max_flows);

/**
 * Get the value to attribute 'reserved_port_types' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] queue_statistics the value of attribute 'reserved_port_types'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'reserved_port_types' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_reserved_port_types(const char *name, bool current,
    uint64_t *reserved_port_types);

/**
 * Get the value to attribute 'group_types' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] queue_statistics the value of attribute 'group_types'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'group_types' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_group_types(const char *name, bool current,
                                 uint64_t *group_types);

/**
 * Get the value to attribute 'group_capabilities' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] queue_statistics the value of attribute 'group_capabilities'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'group_capabilities' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_group_capabilities(const char *name, bool current,
                                        uint64_t *group_capabilities);

/**
 * Get the value to attribute 'packet_inq_size' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] queue_statistics the value of attribute 'packet_inq_size'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'packet_inq_size' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_packet_inq_size(const char *name, bool current,
                                     uint16_t *packet_inq_size);

/**
 * Get the value to attribute 'packet_inq_max_batches' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] queue_statistics the value of attribute 'packet_inq_max_batches'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'packet_inq_max_batches' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_packet_inq_max_batches(const char *name, bool current,
                                            uint16_t *packet_inq_max_batches);

/**
 * Get the value to attribute 'up_streamq_size' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] queue_statistics the value of attribute 'up_streamq_size'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'up_streamq_size' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_up_streamq_size(const char *name, bool current,
                                     uint16_t *up_streamq_size);

/**
 * Get the value to attribute 'up_streamq_max_batches' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] queue_statistics the value of attribute 'up_streamq_max_batches'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'up_streamq_max_batches' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_up_streamq_max_batches(const char *name, bool current,
                                            uint16_t *up_streamq_max_batches);

/**
 * Get the value to attribute 'down_streamq_size' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] queue_statistics the value of attribute 'down_streamq_size'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'down_streamq_size' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_down_streamq_size(const char *name, bool current,
                                       uint16_t *down_streamq_size);

/**
 * Get the value to attribute 'down_streamq_max_batches' of the bridge table record'
 *
 *  @param[in] name
 *  @param[in] current
 *  @param[out] queue_statistics the value of attribute 'down_streamq_max_batches'
 *
 *  @retval == LAGOPUS_RESULT_OK the attribute 'down_streamq_max_batches' getted sucessfully.
 */
lagopus_result_t
datastore_bridge_get_down_streamq_max_batches(
    const char *name, bool current,
    uint16_t *down_streamq_max_batches);


/**
 * Get bridge name by dpid.
 *
 *  @param[in]	dpid	dpid
 *  @param[out]	name	A bridge name.
 *
 *  @retval	== LAGOPUS_RESULT_OK	sucessfully.
 *  @retval	!= LAGOPUS_RESULT_OK	Failed.
 */
lagopus_result_t
datastore_bridge_get_name_by_dpid(uint64_t dpid,
                                  char **name);

/**
 * Get bridge names.
 *
 *  @param[in]	name	A bridge name (\b NULL allowed).
 *  @param[out]	names	List of bridge name.
 *
 *  @retval	== LAGOPUS_RESULT_OK	sucessfully.
 *  @retval	!= LAGOPUS_RESULT_OK	Failed.
 *
 *  @details	If the name is NULL, get list of all bridge name.
 */
lagopus_result_t
datastore_bridge_get_names(const char *name,
                           datastore_name_info_t **names);



#endif /* ! __LAGOPUS_DATASTORE_BRIDGE_H__ */
