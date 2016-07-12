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
 *      @file   bridge_cmd.c
 *      @brief  DSL command parser for bridge management.
 */

#include "lagopus_apis.h"
#include "lagopus/ofp_bridgeq_mgr.h"
#include "datastore_apis.h"
#include "cmd_common.h"
#include "bridge_cmd.h"
#include "controller_cmd.h"
#include "port_cmd.h"
#include "bridge_cmd_internal.h"
#include "bridge.c"
#include "conv_json.h"
#include "lagopus/dp_apis.h"
#include "../agent/channel_mgr.h"
#include "../agent/ofp_table_handler.h"

/* command name. */
#define CMD_NAME "bridge"

/* CONTROLLER_DEL_RETRY_MAX * CONTROLLER_DEL_TIMEOUT = 5(sec) */
#define CONTROLLER_DEL_RETRY_MAX 50
#define CONTROLLER_DEL_TIMEOUT   1LL * 1000LL * 1000LL * 100LL

#define ACTION_TYPE_MIN (DATASTORE_BRIDGE_ACTION_TYPE_MIN + 1)
#define ACTION_TYPE_MAX DATASTORE_BRIDGE_ACTION_TYPE_MAX
#define INSTRUCTION_TYPE_MIN (DATASTORE_BRIDGE_INSTRUCTION_TYPE_MIN + 1)
#define INSTRUCTION_TYPE_MAX DATASTORE_BRIDGE_INSTRUCTION_TYPE_MAX
#define RESERVED_PORT_TYPE_MIN (DATASTORE_BRIDGE_RESERVED_PORT_TYPE_MIN + 1)
#define RESERVED_PORT_TYPE_MAX DATASTORE_BRIDGE_RESERVED_PORT_TYPE_MAX
#define GROUP_TYPE_MIN (DATASTORE_BRIDGE_GROUP_TYPE_MIN + 1)
#define GROUP_TYPE_MAX DATASTORE_BRIDGE_GROUP_TYPE_MAX
#define GROUP_CAPABILITY_MIN (DATASTORE_BRIDGE_GROUP_CAPABILITY_MIN + 1)
#define GROUP_CAPABILITY_MAX DATASTORE_BRIDGE_GROUP_CAPABILITY_MAX

/* option num. */
enum bridge_opts {
  OPT_NAME = 0,
  OPT_CONTROLLERS,
  OPT_CONTROLLER,
  OPT_PORTS,
  OPT_PORT,
  OPT_DPID,
  OPT_FAIL_MODE,
  OPT_FLOW_STATISTICS,
  OPT_GROUP_STATISTICS,
  OPT_PORT_STATISTICS,
  OPT_QUEUE_STATISTICS,
  OPT_TABLE_STATISTICS,
  OPT_REASSEMBLE_IP_FRAGMENTS,
  OPT_MAX_BUFFERED_PACKETS,
  OPT_MAX_PORTS,
  OPT_MAX_TABLES,
  OPT_MAX_FLOWS,
  OPT_BLOCK_LOOPING_PORTS,
  OPT_ACTION_TYPES,
  OPT_ACTION_TYPE,
  OPT_INSTRUCTION_TYPES,
  OPT_INSTRUCTION_TYPE,
  OPT_RESERVED_PORT_TYPES,
  OPT_RESERVED_PORT_TYPE,
  OPT_GROUP_TYPES,
  OPT_GROUP_TYPE,
  OPT_GROUP_CAPABILITIES,
  OPT_GROUP_CAPABILITY,
  OPT_PACKET_INQ_SIZE,
  OPT_PACKET_INQ_MAX_BATCHES,
  OPT_UP_STREAMQ_SIZE,
  OPT_UP_STREAMQ_MAX_BATCHES,
  OPT_DOWN_STREAMQ_SIZE,
  OPT_DOWN_STREAMQ_MAX_BATCHES,
  OPT_IS_USED,
  OPT_IS_ENABLED,
  OPT_L2_BRIDGE,
  OPT_MACTABLE_AGEING_TIME,
  OPT_MACTABLE_MAX_ENTRIES,
  OPT_MACTABLE_ENTRY,

  OPT_MAX,
};

/* clear queue option num. */
enum bridge_clear_q_opts {
  OPT_CLEAR_Q_PACKET_INQ = 0,
  OPT_CLEAR_Q_UP_STREAMQ,
  OPT_CLEAR_Q_DOWN_STREAMQ,

  OPT_CLEAR_Q_MAX,
};

/* stats num. */
enum bridge_stats {
  STATS_PACKET_INQ_ENTRIES = 0,
  STATS_UP_STREAMQ_ENTRIES,
  STATS_DOWN_STREAMQ_ENTRIES,
  STATS_FLOWCACHE_ENTRIES,
  STATS_FLOWCACHE_HIT,
  STATS_FLOWCACHE_MISS,
  STATS_FLOW_ENTRIES,
  STATS_FLOW_LOOKUP_COUNT,
  STATS_FLOW_MATCHED_COUNT,
  STATS_TABLES,
  STATS_TABLE_ID,

  STATS_MAX,
};

/* option name. */
static const char *const opt_strs[OPT_MAX] = {
  "*name",                     /* OPT_NAME (not option) */
  "*controllers",              /* OPT_CONTROLLERS (not option) */
  "-controller",               /* OPT_CONTROLLER */
  "*ports",                    /* OPT_PORTS (not option) */
  "-port",                     /* OPT_PORT  */
  "-dpid",                     /* OPT_DPID */
  "-fail-mode",                /* OPT_FAIL_MODE */
  "-flow-statistics",          /* OPT_FLOW_STATISTICS */
  "-group-statistics",         /* OPT_GROUP_STATISTICS */
  "-port-statistics",          /* OPT_PORT_STATISTICS */
  "-queue-statistics",         /* OPT_QUEUE_STATISTICS */
  "-table-statistics",         /* OPT_TABLE_STATISTICS */
  "-reassemble-ip-fragments",  /* OPT_REASSEMBLE_IP_FRAGMENTS */
  "-max-buffered-packets",     /* OPT_MAX_BUFFERED_PACKETS */
  "-max-ports",                /* OPT_MAX_PORTS */
  "-max-tables",               /* OPT_MAX_TABLES */
  "-max-flows",                /* OPT_MAX_FLOWS */
  "-block-looping-ports",      /* OPT_BLOCK_LOOPING_PORTS */
  "*action-types",             /* OPT_ACTION_TYPES (not option) */
  "-action-type",              /* OPT_ACTION_TYPE */
  "*instruction-types",        /* OPT_INSTRUCTION_TYPES (not option) */
  "-instruction-type",         /* OPT_INSTRUCTION_TYPE */
  "*reserved-port-types",      /* OPT_RESERVED_PORT_TYPES (not option) */
  "-reserved-port-type",       /* OPT_RESERVED_PORT_TYPE */
  "*group-types",              /* OPT_GROUP_TYPES (not option)  */
  "-group-type",               /* OPT_GROUP_TYPE */
  "*group-capabilities",       /* OPT_GROUP_CAPABILITIES (not option) */
  "-group-capability",         /* OPT_GROUP_CAPABILITY */
  "-packet-inq-size",          /* OPT_PACKET_INQ_SIZE */
  "-packet-inq-max-batches",   /* OPT_PACKET_INQ_MAX_BATCHES */
  "-up-streamq-size",          /* OPT_UP_STREAMQ_SIZE */
  "-up-streamq-max-batches",   /* OPT_UP_STREAMQ_MAX_BATCHES */
  "-down-streamq-size",        /* OPT_DOWN_STREAMQ_SIZE */
  "-down-streamq-max-batches", /* OPT_DOWN_STREAMQ_MAX_BATCHES */

  "*is-used",                  /* OPT_IS_USED (not option) */
  "*is-enabled",               /* OPT_IS_ENABLED (not option) */

#ifdef HYBRID
  "-l2-bridge",                /* OPT_L2_BRIDGE*/
  "-mactable-ageing-time",     /* OPT_MACTABLE_AGEING_TIME */
  "-mactable-max-entries",     /* OPT_MACTABLE_MAX_ENTRIES*/
  "-mactable-entry",           /* OPT_MACTABLE_ENTRY */
#endif /* HYBRID */
};

/* clear queue option name. */
static const char *const clear_q_opt_strs[OPT_CLEAR_Q_MAX] = {
  "-packet-inq",
  "-up-streamq",
  "-down-streamq",
};

/* stats name. */
static const char *const stat_strs[STATS_MAX] = {
  "*packet-inq-entries",      /* STATS_PACKET_INQ_ENTRIES (not option) */
  "*up-streamq-entries",      /* STATS_UP_STREAMQ_ENTRIES (not option) */
  "*down-streamq-entries",    /* STATS_DOWN_STREAMQ_ENTRIES (not option) */
  "*flowcache-entries",       /* STATS_FLOWCACHE_ENTRIES (not option) */
  "*flowcache-hit",           /* STATS_FLOWCACHE_HIT (not option) */
  "*flowcache-miss",          /* STATS_FLOWCACHE_MISS (not option) */
  "*flow-entries",            /* STATS_FLOW_ENTRIES (not option) */
  "*flow-lookup-count",       /* STATS_FLOW_LOOKUP_COUNT (not option) */
  "*flow-matched-count",      /* STATS_FLOW_MATCHED_COUNT (not option) */
  "*tables",                  /* STATS_TABLES (not option) */
  "*table-id",                /* STATS_TABLE_ID (not option) */
};

typedef struct configs {
  size_t size;
  uint64_t flags;
  bool is_config;
  bool is_show_modified;
  bool is_show_stats;
  datastore_bridge_stats_t stats;
  bridge_conf_t **list;
} configs_t;

struct names_info {
  datastore_name_info_t *not_change_names;
  datastore_name_info_t *add_names;
  datastore_name_info_t *del_names;
};

typedef lagopus_result_t
(*bool_set_proc_t)(bridge_attr_t *attr, const bool b);
typedef lagopus_result_t
(*uint64_set_proc_t)(bridge_attr_t *attr, const uint64_t num);
typedef lagopus_result_t
(*uint32_set_proc_t)(bridge_attr_t *attr, const uint32_t num);
typedef lagopus_result_t
(*uint16_set_proc_t)(bridge_attr_t *attr, const uint16_t num);
typedef lagopus_result_t
(*uint8_set_proc_t)(bridge_attr_t *attr, const uint8_t num);
typedef lagopus_result_t
(*clear_q_proc_t)(uint64_t dpid);

static lagopus_hashmap_t sub_cmd_table = NULL;
static lagopus_hashmap_t opt_table = NULL;
static lagopus_hashmap_t opt_clear_queue_table = NULL;
static lagopus_hashmap_t port_table = NULL;

static inline lagopus_result_t
bri_names_disable(const char *name,
                  bridge_attr_t *attr,
                  struct names_info *p_names_info,
                  struct names_info *c_names_info,
                  datastore_interp_t *iptr,
                  datastore_interp_state_t state,
                  bool is_propagation,
                  bool is_unset_used,
                  lagopus_dstring_t *result);

static inline lagopus_result_t
bri_port_add(const char *name,
             uint32_t port_no,
             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  void *val = (void *) &port_no;

  if (port_table != NULL && name != NULL &&
      result != NULL) {
    ret = lagopus_hashmap_add(&port_table, (void *) name, &val, true);
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add port (name = %s).",
                                              name);
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static inline lagopus_result_t
bri_port_del(const char *name,
             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (port_table != NULL && name != NULL &&
      result != NULL) {
    ret = lagopus_hashmap_delete(&port_table,
                                 (void *) name, NULL, false);
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add port (name = %s).",
                                              name);
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static inline bool
bri_port_is_modified(const char *name) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool r = false;
  uint32_t port_no;
  uint32_t *bri_port_no;

  if (port_table != NULL && name != NULL) {
    if ((ret = datastore_port_get_port_number(name, true,
               &port_no)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_hashmap_find(&port_table,
                                      (void *) name,
                                      (void **) &bri_port_no)) ==
          LAGOPUS_RESULT_OK) {
        if (port_no != *bri_port_no) {
          r = true;
        }
      } else if (ret != LAGOPUS_RESULT_NOT_FOUND) {
        r = true;
      }
    }
  }

  return r;
}

static inline bool
bri_ports_is_modified(struct names_info *names_info) {
  bool ret = false;
  struct datastore_name_entry *name = NULL;

  if (port_table != NULL && names_info != NULL) {
    if (TAILQ_EMPTY(&names_info->not_change_names->head) == false) {
      TAILQ_FOREACH(name, &names_info->not_change_names->head, name_entries) {
        if (bri_port_is_modified(name->str) == true) {
          ret = true;
          goto done;
        }
      }
    }

    if (TAILQ_EMPTY(&names_info->add_names->head) == false) {
      TAILQ_FOREACH(name, &names_info->add_names->head, name_entries) {
        if (bri_port_is_modified(name->str) == true) {
          ret = true;
          goto done;
        }
      }
    }

    if (TAILQ_EMPTY(&names_info->del_names->head) == false) {
      TAILQ_FOREACH(name, &names_info->del_names->head, name_entries) {
        if (bri_port_is_modified(name->str) == true) {
          ret = true;
          goto done;
        }
      }
    }
  }

done:
  return ret;
}

static inline lagopus_result_t
bri_queue_max_batches_set(bridge_attr_t *attr,
                          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;
  uint16_t packet_inq_max_batches;
  uint16_t up_streamq_max_batches;
  uint16_t down_streamq_max_batches;

  /* get items. */
  if (((ret = bridge_get_dpid(attr, &dpid)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_packet_inq_max_batches(attr,
                                                &packet_inq_max_batches)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_up_streamq_max_batches(attr,
                                                &up_streamq_max_batches)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_down_streamq_max_batches(attr,
                                                  &down_streamq_max_batches)) ==
       LAGOPUS_RESULT_OK)) {
    if ((ret = ofp_bridgeq_mgr_dataq_max_batches_set(
            dpid,
            packet_inq_max_batches)) != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(
          result, ret,
          "Can't set packet_inq_max_batches.");
      goto done;
    }
    if ((ofp_bridgeq_mgr_eventq_max_batches_set(
            dpid,
            up_streamq_max_batches)) != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(
          result, ret,
          "Can't set up_streamq_max_batches.");
      goto done;
    }
    if ((ofp_bridgeq_mgr_event_dataq_max_batches_set(
            dpid,
            down_streamq_max_batches)) != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(
          result, ret,
          "Can't set down_streamq_max_batches.");
      goto done;
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't get queue max batches.");
  }

done:
  return ret;
}

static inline uint64_t
capabilities_get(bool flow_statistics,
                 bool group_statistics,
                 bool port_statistics,
                 bool queue_statistics,
                 bool table_statistics,
                 bool reassemble_ip_fragments,
                 bool block_looping_ports) {
  uint64_t capabilities = 0LL;

  if (flow_statistics == true) {
    capabilities |= DATASTORE_BRIDGE_BIT_CAPABILITY_FLOW_STATS;
  }

  if (group_statistics == true) {
    capabilities |= DATASTORE_BRIDGE_BIT_CAPABILITY_GROUP_STATS;
  }

  if (port_statistics == true) {
    capabilities |= DATASTORE_BRIDGE_BIT_CAPABILITY_PORT_STATS;
  }

  if (queue_statistics == true) {
    capabilities |= DATASTORE_BRIDGE_BIT_CAPABILITY_QUEUE_STATS;
  }

  if (table_statistics == true) {
    capabilities |= DATASTORE_BRIDGE_BIT_CAPABILITY_TABLE_STATS;
  }

  if (reassemble_ip_fragments == true) {
    capabilities |= DATASTORE_BRIDGE_BIT_CAPABILITY_REASSEMBLE_IP_FRGS;
  }

  if (block_looping_ports == true) {
    capabilities |= DATASTORE_BRIDGE_BIT_CAPABILITY_BLOCK_LOOPING_PORTS;
  }

  return capabilities;
}

/* add/delete bridge. */
/**
 * Create bridge.
 */
static inline lagopus_result_t
bri_bridge_create(const char *name,
                  bridge_attr_t *attr,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_bridge_info_t info;
  datastore_bridge_queue_info_t q_info;
  uint64_t dpid;
  datastore_bridge_fail_mode_t fail_mode;
  bool flow_statistics;
  bool group_statistics;
  bool port_statistics;
  bool queue_statistics;
  bool table_statistics;
  bool reassemble_ip_fragments;
  bool block_looping_ports;
  uint32_t max_buffered_packets;
  uint32_t max_flows;
  uint16_t packet_inq_size;
  uint16_t packet_inq_max_batches;
  uint16_t up_streamq_size;
  uint16_t up_streamq_max_batches;
  uint16_t down_streamq_size;
  uint16_t down_streamq_max_batches;
  uint16_t max_ports;
  uint8_t max_tables;
  uint64_t action_types;
  uint64_t instruction_types;
  uint64_t reserved_port_types;
  uint64_t group_types;
  uint64_t group_capabilities;
#ifdef HYBRID
  bool l2_bridge;
  uint32_t mactable_ageing_time;
  uint32_t mactable_max_entries;
#endif /* HYBRID */
  /* get items. */
  if (((ret = bridge_get_dpid(attr, &dpid)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_fail_mode(attr,
                                   &fail_mode)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_is_flow_statistics(attr,
                                        &flow_statistics)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_is_group_statistics(attr,
                                         &group_statistics)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_is_port_statistics(attr,
                                        &port_statistics)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_is_queue_statistics(attr,
                                         &queue_statistics)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_is_table_statistics(attr,
                                         &table_statistics)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_is_reassemble_ip_fragments(attr,
              &reassemble_ip_fragments)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_is_block_looping_ports(attr,
              &block_looping_ports)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_max_buffered_packets(attr,
              &max_buffered_packets)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_max_flows(attr,
                                   &max_flows)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_max_ports(attr,
                                   &max_ports)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_max_tables(attr,
                                    &max_tables)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_action_types(attr,
                                      &action_types)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_instruction_types(attr,
              &instruction_types)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_reserved_port_types(attr,
              &reserved_port_types)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_group_types(attr,
                                     &group_types)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_group_capabilities(attr,
              &group_capabilities)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_packet_inq_size(attr,
                                         &packet_inq_size)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_packet_inq_max_batches(attr,
                                                &packet_inq_max_batches)) ==
       LAGOPUS_RESULT_OK) &&
#ifdef HYBRID
      ((ret = bridge_is_l2_bridge(attr,
                                   &l2_bridge)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_mactable_ageing_time(attr,
                                              &mactable_ageing_time)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_mactable_max_entries(attr,
                                              &mactable_max_entries)) ==
       LAGOPUS_RESULT_OK) &&
#endif /* HYBRID */
      ((ret = bridge_get_up_streamq_size(attr,
                                         &up_streamq_size)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_up_streamq_max_batches(attr,
                                                &up_streamq_max_batches)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_down_streamq_size(attr,
                                           &down_streamq_size)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = bridge_get_down_streamq_max_batches(attr,
                                                  &down_streamq_max_batches)) ==
       LAGOPUS_RESULT_OK)
    ) {
    info.dpid = dpid;
    info.fail_mode = fail_mode;
    info.max_buffered_packets = max_buffered_packets;
    info.max_ports = max_ports;
    info.max_tables = max_tables;
    info.max_flows = max_flows;
    info.capabilities = capabilities_get(flow_statistics,
                                         group_statistics,
                                         port_statistics,
                                         queue_statistics,
                                         table_statistics,
                                         reassemble_ip_fragments,
                                         block_looping_ports);
    info.action_types = action_types;
    info.instruction_types = instruction_types;
    info.reserved_port_types = reserved_port_types;
    info.group_types = group_types;
    info.group_capabilities = group_capabilities;

#ifdef HYBRID
    info.l2_bridge = l2_bridge;
    info.mactable_ageing_time = mactable_ageing_time;
    info.mactable_max_entries = mactable_max_entries;
#endif /* HYBRID */

    q_info.packet_inq_size = packet_inq_size;
    q_info.packet_inq_max_batches = packet_inq_max_batches;
    q_info.up_streamq_size = up_streamq_size;
    q_info.up_streamq_max_batches = up_streamq_max_batches;
    q_info.down_streamq_size = down_streamq_size;
    q_info.down_streamq_max_batches = down_streamq_max_batches;

    lagopus_msg_info("create bridge. name = %s.\n", name);

    if ((ret = ofp_bridgeq_mgr_bridge_register(dpid,
                                               name,
                                               &info,
                                               &q_info)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = dp_bridge_create(name, &info)) !=
          LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't add bridge.");
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add bridge."
                                              "dpid = %"PRIu64".", dpid);
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't add bridge.");
  }

  return ret;
}

static inline lagopus_result_t
bri_bridge_destroy(const char *name,
                   bridge_attr_t *attr,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;

  lagopus_msg_info("destroy bridge. name = %s.\n", name);

  /* get items. */
  if ((ret = bridge_get_dpid(attr, &dpid)) ==
      LAGOPUS_RESULT_OK) {
    if ((ret = dp_bridge_destroy(name)) ==
        LAGOPUS_RESULT_OK) {
      if ((ofp_bridgeq_mgr_bridge_unregister(dpid)) !=
          LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't delete bridge."
                                                "dpid = %"PRIu64".", dpid);
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't delete bridge.");
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't delete bridge.");
  }

  return ret;
}

static inline lagopus_result_t
bri_bridge_start(const char *name,
                 bridge_attr_t *attr,
                 struct names_info *p_names_info,
                 struct names_info *c_names_info,
                 datastore_interp_t *iptr,
                 datastore_interp_state_t state,
                 bool is_propagation,
                 bool is_modified_without_names,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *pc_name = NULL;
  datastore_name_info_t *p_names = NULL;
  datastore_name_info_t *c_names = NULL;
  struct datastore_name_head *head;

  /* propagation port. */
  if (TAILQ_EMPTY(&p_names_info->add_names->head) == true) {
    if ((ret = bridge_get_port_names(attr, &p_names)) ==
        LAGOPUS_RESULT_OK) {
      head = &p_names->head;
    } else {
      ret = datastore_json_result_set(result, ret, NULL);
      goto done;
    }
  } else {
    head = &p_names_info->add_names->head;
  }

  if (TAILQ_EMPTY(head) == false) {
    TAILQ_FOREACH(pc_name, head, name_entries) {
      if (is_propagation == true) {
        if ((ret = port_cmd_enable_propagation(iptr, state,
                                               pc_name->str, result)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result,
                                                  ret,
                                                  "Can't start port.");
          goto done;
        }
      }
    }
  }

  /* propagation controller. */
  if (TAILQ_EMPTY(&c_names_info->add_names->head) == true) {
    if ((ret = bridge_get_controller_names(attr, &c_names)) ==
        LAGOPUS_RESULT_OK) {
      head = &c_names->head;
    } else {
      ret = datastore_json_result_set(result, ret, NULL);
      goto done;
    }
  } else {
    head = &c_names_info->add_names->head;
  }

  if (TAILQ_EMPTY(head) == false) {
    TAILQ_FOREACH(pc_name, head, name_entries) {
      if (is_propagation == true) {
        if ((ret = controller_cmd_enable_propagation(iptr, state,
                   pc_name->str, result)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result,
                                                  ret,
                                                  "Can't start controller.");
          goto done;
        }
      }
    }
  }

  if (is_modified_without_names == true) {
    lagopus_msg_info("start bridge. name = %s.\n", name);

    if ((ret = dp_bridge_start(name)) !=
        LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't start bridge.");
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

done:
  if (p_names != NULL) {
    datastore_names_destroy(p_names);
  }
  if (c_names != NULL) {
    datastore_names_destroy(c_names);
  }

  return ret;
}

static inline lagopus_result_t
bri_bridge_stop(const char *name,
                bridge_attr_t *attr,
                datastore_interp_t *iptr,
                datastore_interp_state_t state,
                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) attr;

  lagopus_msg_info("stop bridge. name = %s.\n", name);

  /* disable ports, controllers. */
  if ((ret = bri_names_disable(name, attr,
                               NULL, NULL,
                               iptr, state, true, false, result)) ==
      LAGOPUS_RESULT_OK) {
    if ((ret = dp_bridge_stop(name)) !=
        LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't stop bridge.");
    }
  } else if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't stop bridge.");
  }

  return ret;
}

/* add/delete ports. */
static inline lagopus_result_t
bri_ports_add(const char *name,
              bridge_attr_t *attr,
              struct names_info *names_info,
              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *p_name = NULL;
  uint32_t port_no;
  datastore_name_info_t *p_names = NULL;
  struct datastore_name_head *head;

  if (names_info == NULL) {
    if ((ret = bridge_get_port_names(attr, &p_names)) ==
        LAGOPUS_RESULT_OK) {
      head = &p_names->head;
    } else {
      ret = datastore_json_result_set(result, ret, NULL);
      goto done;
    }
  } else {
    head = &names_info->add_names->head;
  }

  if (TAILQ_EMPTY(head) == false) {
    TAILQ_FOREACH(p_name, head, name_entries) {
      /* get items. */
      if ((ret = datastore_port_get_port_number(p_name->str, false,
                 &port_no)) ==
          LAGOPUS_RESULT_INVALID_OBJECT) {
        ret = datastore_port_get_port_number(p_name->str, true,
                                             &port_no);
      }

      if (ret == LAGOPUS_RESULT_OK) {
        lagopus_msg_info("Add bridge port(%s). "
                         "port name = %s, port_no = %"PRIu32".\n",
                         name, p_name->str, port_no);
        ret = dp_bridge_port_set(name, p_name->str, port_no);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = bri_port_add(name, port_no, result);
        } else {
          ret = datastore_json_result_string_setf(result,
                                                  ret,
                                                  "Can't add port.");
          goto done;
        }
      } else {
        ret = datastore_json_result_string_setf(result,
                                                ret,
                                                "Can't add port.");
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

done:
  if (p_names != NULL) {
    datastore_names_destroy(p_names);
  }

  return ret;
}

static inline lagopus_result_t
bri_ports_delete(const char *name,
                 bridge_attr_t *attr,
                 struct names_info *names_info,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *p_name = NULL;
  datastore_name_info_t *p_names = NULL;
  struct datastore_name_head *head;
  uint32_t port_no;

  if (names_info == NULL) {
    if ((ret = bridge_get_port_names(attr, &p_names)) ==
        LAGOPUS_RESULT_OK) {
      head = &p_names->head;
    } else {
      ret = datastore_json_result_set(result, ret, NULL);
      goto done;
    }
  } else {
    head = &names_info->del_names->head;
  }

  if (TAILQ_EMPTY(head) == false) {
    TAILQ_FOREACH(p_name, head, name_entries) {
      /* get items. */
      if ((ret = datastore_port_get_port_number(p_name->str, false,
                 &port_no)) ==
          LAGOPUS_RESULT_INVALID_OBJECT) {
        ret = datastore_port_get_port_number(p_name->str, true,
                                             &port_no);
      }

      if (ret == LAGOPUS_RESULT_OK) {
        lagopus_msg_info("Delete bridge port(%s). "
                         "port name = %s.\n",
                         name, p_name->str);
        ret = dp_bridge_port_unset(name, p_name->str);
        if (ret == LAGOPUS_RESULT_OK ||
            ret == LAGOPUS_RESULT_NOT_FOUND) {
          /* ignore LAGOPUS_RESULT_NOT_FOUND. */
          ret = bri_port_del(name, result);
        } else {
          ret = datastore_json_result_string_setf(result,
                                                  ret,
                                                  "Can't delete port.");
          goto done;
        }
      } else {
        ret = datastore_json_result_string_setf(result,
                                                ret,
                                                "Can't delete port.");
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

done:
  if (p_names != NULL) {
    datastore_names_destroy(p_names);
  }

  return ret;
}

/* add/delete controllers. */
static inline lagopus_result_t
bri_controllers_add(const char *name,
                    bridge_attr_t *attr,
                    struct names_info *names_info,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *c_name = NULL;
  uint64_t dpid;
  char *channel_name = NULL;
  datastore_name_info_t *c_names = NULL;
  struct datastore_name_head *head;

  /* get items. */
  if ((ret = bridge_get_dpid(attr, &dpid)) ==
      LAGOPUS_RESULT_OK) {
    if (names_info == NULL) {
      if ((ret = bridge_get_controller_names(attr, &c_names)) ==
          LAGOPUS_RESULT_OK) {
        head = &c_names->head;
      } else {
        ret = datastore_json_result_set(result, ret, NULL);
        goto done;
      }
    } else {
      head = &names_info->add_names->head;
    }

    if (TAILQ_EMPTY(head) == false) {
      TAILQ_FOREACH(c_name, head, name_entries) {
        /* get items. */
        if ((ret = datastore_controller_get_channel_name(c_name->str, false,
                   &channel_name)) ==
            LAGOPUS_RESULT_INVALID_OBJECT) {
          ret = datastore_controller_get_channel_name(c_name->str, true,
                                                      &channel_name);
        }

        if (ret == LAGOPUS_RESULT_OK) {
          lagopus_msg_info("Add bridge controller(%s). "
                           "controller name = %s, channel_name = %s.\n",
                           name, c_name->str, channel_name);
          ret = channel_mgr_channel_dpid_set(channel_name, dpid);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't add controller.");
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't add controller.");
          goto free;
        }

      free:
        free(channel_name);
        channel_name = NULL;
        if (ret != LAGOPUS_RESULT_OK) {
          break;
        }
      }
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = datastore_json_result_set(result, ret, NULL);
  }

done:
  if (c_names != NULL) {
    datastore_names_destroy(c_names);
  }

  return ret;
}

static inline lagopus_result_t
bri_controllers_delete(const char *name,
                       bridge_attr_t *attr,
                       struct names_info *names_info,
                       lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *c_name = NULL;
  uint64_t dpid;
  char *channel_name = NULL;
  uint64_t retry = 0;
  datastore_name_info_t *c_names = NULL;
  struct datastore_name_head *head;

  /* get items. */
  if ((ret = bridge_get_dpid(attr, &dpid)) ==
      LAGOPUS_RESULT_OK) {
    if (names_info == NULL) {
      if ((ret = bridge_get_controller_names(attr, &c_names)) ==
          LAGOPUS_RESULT_OK) {
        head = &c_names->head;
      } else {
        ret = datastore_json_result_set(result, ret, NULL);
        goto done;
      }
    } else {
      head = &names_info->del_names->head;
    }

    if (TAILQ_EMPTY(head) == false) {
      TAILQ_FOREACH(c_name, head, name_entries) {
        /* get items. */
        if ((ret = datastore_controller_get_channel_name(c_name->str, false,
                                                         &channel_name)) ==
            LAGOPUS_RESULT_INVALID_OBJECT) {
          ret = datastore_controller_get_channel_name(c_name->str, true,
                                                      &channel_name);
        }

        if (ret == LAGOPUS_RESULT_OK) {
          lagopus_msg_info("Delete bridge controller(%s). "
                           "controller name = %s, channel_name = %s.\n",
                           name, c_name->str, channel_name);
          /*
           * TODO: Current situation, BUSY will be returned.
           *       Deal by looping deletion.
           *       Fundamental solution to improve the event.
           *       If an error occurs, it is necessary to rollback.
           */
          retry = 0;
          while (retry < CONTROLLER_DEL_RETRY_MAX) {
            ret = channel_mgr_channel_dpid_unset(channel_name);

            if (ret == LAGOPUS_RESULT_OK) {
              break;
            } else if (ret == LAGOPUS_RESULT_BUSY) {
              (void) lagopus_chrono_nanosleep(CONTROLLER_DEL_TIMEOUT, false);
              retry++;
              lagopus_msg_debug(1, "delete controller: retry = %"PRIu64"\n",
                                retry);
            } else {
              break;
            }
          }

          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't delete channel.");
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't delete controller.");
          goto free;
        }

      free:
        free(channel_name);
        channel_name = NULL;
        if (ret != LAGOPUS_RESULT_OK) {
          break;
        }
      }
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = datastore_json_result_set(result, ret, NULL);
  }

done:
  if (c_names != NULL) {
    datastore_names_destroy(c_names);
  }

  return ret;
}

#ifdef HYBRID
/* add/delete mactable entries. */
/**
 * Add mac entry to mactable by dp_apis.
 * @param[in] name bridge name.
 */
static inline lagopus_result_t
bri_mactable_entry_add(const char *name,
              bridge_attr_t *attr,
              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bridge_mactable_entry *entry = NULL;
  struct mactable_entry_head *head;

  /* get mactable entries list */
  head = &attr->mactable_entries->head;
  /* list loop */
  if (TAILQ_EMPTY(head) == false) {
    TAILQ_FOREACH(entry, head, mactable_entries) {
      ret = dp_bridge_mactable_entry_set(name,
                                         entry->mac_address,
                                         entry->port_no);
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result,
                                      ret, "Failed add entry to mactable.");

      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

  return ret;
}
#endif /* HYBRID */

/* create/destroy bridge, port, controller. */
static inline lagopus_result_t
bri_create(const char *name,
           bridge_attr_t *attr,
           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = bri_bridge_create(name, attr, result);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = bri_ports_add(name, attr, NULL, result);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = bri_controllers_add(name, attr, NULL, result);
#ifdef HYBRID
      if (ret == LAGOPUS_RESULT_OK) {
        /* add static entries to mactable. */
        ret = bri_mactable_entry_add(name, attr, result);
      }
#endif /* HYBRID */
    }
  }

  return ret;
}

static inline lagopus_result_t
bri_destroy(const char *name,
            bridge_attr_t *attr,
            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = bri_controllers_delete(name, attr, NULL, result);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = bri_ports_delete(name, attr, NULL, result);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = bri_bridge_destroy(name, attr, result);
    }
  }

  return ret;
}

/* add/delete ports, controllers. */
static inline lagopus_result_t
bri_names_add(const char *name,
              bridge_attr_t *attr,
              struct names_info *p_names_info,
              struct names_info *c_names_info,
              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void)c_names_info;

  ret = bri_ports_add(name, attr, p_names_info, result);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = bri_controllers_add(name, attr, c_names_info, result);
#ifdef HYBRID
    if (ret == LAGOPUS_RESULT_OK) {
      ret = bri_mactable_entry_add(name, attr, result);
    }
#endif /* HYBRID */
  }

  return ret;
}

static inline lagopus_result_t
bri_names_delete(const char *name,
                 bridge_attr_t *attr,
                 struct names_info *p_names_info,
                 struct names_info *c_names_info,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void)c_names_info;

  ret = bri_controllers_delete(name, attr, c_names_info, result);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = bri_ports_delete(name, attr, p_names_info, result);
  }

  return ret;
}

static inline lagopus_result_t
bri_port_used_set_internal(const char *name, bool b,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = port_set_used(name, b);
  /* ignore : LAGOPUS_RESULT_NOT_FOUND */
  if (ret == LAGOPUS_RESULT_OK || ret == LAGOPUS_RESULT_NOT_FOUND) {
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "port name = %s.",
                                            name);
  }

  return ret;
}

static inline lagopus_result_t
bri_ports_used_set(bridge_attr_t *attr, bool b,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_name_info_t *p_names = NULL;
  struct datastore_name_entry *name = NULL;

  if ((ret = bridge_get_port_names(attr, &p_names)) ==
      LAGOPUS_RESULT_OK) {
    TAILQ_FOREACH(name, &p_names->head, name_entries) {
      ret = bri_port_used_set_internal(name->str, b, result);
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "port name = %s.",
                                                name->str);
        break;
      }
    }
  } else {
    ret = datastore_json_result_set(result, ret, NULL);
  }

  if (p_names != NULL) {
    datastore_names_destroy(p_names);
  }

  return ret;
}

/* set is_used for controller. */
static inline lagopus_result_t
bri_controller_used_set_internal(const char *name, bool b,
                                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = controller_set_used(name, b);
  /* ignore : LAGOPUS_RESULT_NOT_FOUND */
  if (ret == LAGOPUS_RESULT_OK || ret == LAGOPUS_RESULT_NOT_FOUND) {
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "controller name = %s.",
                                            name);
  }

  return ret;
}

static inline lagopus_result_t
bri_controllers_used_set(bridge_attr_t *attr, bool b,
                         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_name_info_t *c_names = NULL;
  struct datastore_name_entry *name = NULL;

  if ((ret = bridge_get_controller_names(attr, &c_names)) ==
      LAGOPUS_RESULT_OK) {
    TAILQ_FOREACH(name, &c_names->head, name_entries) {
      ret = bri_controller_used_set_internal(name->str, b, result);
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "controller name = %s.",
                                                name->str);
        break;
      }
    }
  } else {
    ret = datastore_json_result_set(result, ret, NULL);
  }

  if (c_names != NULL) {
    datastore_names_destroy(c_names);
  }

  return ret;
}

/* set is_used for port/controller. */
static inline lagopus_result_t
bri_names_used_set(const char *name,
                   bridge_attr_t *attr, bool b,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) name;

  if ((ret = bri_ports_used_set(attr, b, result)) ==
      LAGOPUS_RESULT_OK) {
    ret = bri_controllers_used_set(attr, b, result);
  }

  return ret;
}

/* disable port. */
static inline lagopus_result_t
bri_ports_disable(const char *name,
                  bridge_attr_t *attr,
                  struct names_info *names_info,
                  datastore_interp_t *iptr,
                  datastore_interp_state_t state,
                  bool is_propagation,
                  bool is_unset_used,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *p_name = NULL;
  datastore_name_info_t *p_names = NULL;
  struct datastore_name_head *head;
  (void) name;

  if (names_info == NULL) {
    if ((ret = bridge_get_port_names(attr, &p_names)) ==
        LAGOPUS_RESULT_OK) {
      head = &p_names->head;
    } else {
      ret = datastore_json_result_set(result, ret, NULL);
      goto done;
    }
  } else {
    head = &names_info->del_names->head;
  }

  if (TAILQ_EMPTY(head) == false) {
    TAILQ_FOREACH(p_name, head, name_entries) {
      if (is_propagation == true ||
          state == DATASTORE_INTERP_STATE_COMMITING ||
          state == DATASTORE_INTERP_STATE_ROLLBACKING) {
        /* disable propagation. */
        if ((ret = port_cmd_disable_propagation(iptr, state,
                                                p_name->str, result)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result,
                                                  ret,
                                                  "port name = %s.",
                                                  p_name->str);
          break;
        }
      }

      if (is_unset_used == true) {
        /* unset used. */
        ret = port_set_used(p_name->str, false);
        /* ignore : LAGOPUS_RESULT_NOT_FOUND */
        if (ret == LAGOPUS_RESULT_OK || ret == LAGOPUS_RESULT_NOT_FOUND) {
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "port name = %s.",
                                                  p_name->str);
          break;
        }
      } else {
        ret = LAGOPUS_RESULT_OK;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

done:
  if (p_names != NULL) {
    datastore_names_destroy(p_names);
  }

  return ret;
}

/* disable controller. */
static inline lagopus_result_t
bri_controllers_disable(const char *name,
                        bridge_attr_t *attr,
                        struct names_info *names_info,
                        datastore_interp_t *iptr,
                        datastore_interp_state_t state,
                        bool is_propagation,
                        bool is_unset_used,
                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *c_name = NULL;
  datastore_name_info_t *c_names = NULL;
  struct datastore_name_head *head;
  (void) name;

  if (names_info == NULL) {
    if ((ret = bridge_get_controller_names(attr, &c_names)) ==
        LAGOPUS_RESULT_OK) {
      head = &c_names->head;
    } else {
      ret = datastore_json_result_set(result, ret, NULL);
      goto done;
    }
  } else {
    head = &names_info->del_names->head;
  }

  if (TAILQ_EMPTY(head) == false) {
    TAILQ_FOREACH(c_name, head, name_entries) {
      if (is_propagation == true ||
          state == DATASTORE_INTERP_STATE_COMMITING ||
          state == DATASTORE_INTERP_STATE_ROLLBACKING) {
        /* disable propagation. */
        if ((ret = controller_cmd_disable_propagation(iptr, state,
                   c_name->str, result)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_NOT_FOUND,
                                                  "controller name = %s.",
                                                  c_name->str);
          break;
        }
      }

      if (is_unset_used == true) {
        /* unset used. */
        ret = controller_set_used(c_name->str, false);
        /* ignore : LAGOPUS_RESULT_NOT_FOUND */
        if (ret == LAGOPUS_RESULT_OK || ret == LAGOPUS_RESULT_NOT_FOUND) {
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "controller name = %s.",
                                                  c_name->str);
          break;
        }
      } else {
        ret = LAGOPUS_RESULT_OK;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

done:
  if (c_names != NULL) {
    datastore_names_destroy(c_names);
  }

  return ret;
}

/* disable ports, controllers. */
static inline lagopus_result_t
bri_names_disable(const char *name,
                  bridge_attr_t *attr,
                  struct names_info *p_names_info,
                  struct names_info *c_names_info,
                  datastore_interp_t *iptr,
                  datastore_interp_state_t state,
                  bool is_propagation,
                  bool is_unset_used,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* disable ports. */
  if ((ret = bri_ports_disable(name, attr, p_names_info,
                               iptr, state, is_propagation,
                               is_unset_used, result)) ==
      LAGOPUS_RESULT_OK) {
    /* disable controllers. */
    if ((ret = bri_controllers_disable(name, attr, c_names_info,
                                       iptr, state, is_propagation,
                                       is_unset_used, result)) ==
        LAGOPUS_RESULT_OK) {
    }
  }
  return ret;
}

/* get controller names info. */
static inline lagopus_result_t
bri_controller_names_info_get(bridge_conf_t *conf,
                              struct names_info *names_info,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *name = NULL;
  datastore_name_info_t *current_names = NULL;
  datastore_name_info_t *modified_names = NULL;

  if (conf->current_attr != NULL &&
      conf->modified_attr != NULL &&
      names_info != NULL) {
    if (((ret = bridge_get_controller_names(conf->current_attr,
                                            &current_names)) ==
         LAGOPUS_RESULT_OK) &&
        ((ret = bridge_get_controller_names(conf->modified_attr,
                                            &modified_names)) ==
         LAGOPUS_RESULT_OK)) {
      TAILQ_FOREACH(name, &current_names->head, name_entries) {
        if (bridge_attr_controller_name_exists(conf->modified_attr,
                                               name->str) == true) {
          /* not_change_names. */
          if (datastore_name_exists(names_info->not_change_names,
                                    name->str) == false) {
            if ((ret = datastore_add_names(names_info->not_change_names,
                                             name->str)) !=
                LAGOPUS_RESULT_OK) {
              ret = datastore_json_result_string_setf(
                      result, ret, "Can't add controller names (not change).");
              goto done;
            }
          }
        } else {
          /* del_names. */
          if (datastore_name_exists(names_info->del_names,
                                    name->str) == false) {
            if ((ret = datastore_add_names(names_info->del_names,
                                             name->str)) !=
                LAGOPUS_RESULT_OK) {
              ret = datastore_json_result_string_setf(
                      result, ret, "Can't add controller names (del).");
              goto done;
            }
          }
        }
      }
      TAILQ_FOREACH(name, &modified_names->head, name_entries) {
        if (bridge_attr_controller_name_exists(conf->current_attr,
                                               name->str) == false) {
          /* add_names. */
          if (datastore_name_exists(names_info->add_names,
                                    name->str) == false) {
            if ((ret = datastore_add_names(names_info->add_names,
                                             name->str)) !=
                LAGOPUS_RESULT_OK) {
              ret = datastore_json_result_string_setf(
                      result, ret, "Can't add controller names (add).");
              goto done;
            }
          }
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't get controller names.");
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

done:
  if (current_names != NULL) {
    datastore_names_destroy(current_names);
  }
  if (modified_names != NULL) {
    datastore_names_destroy(modified_names);
  }

  return ret;
}

/* get port names info. */
static inline lagopus_result_t
bri_port_names_info_get(bridge_conf_t *conf,
                        struct names_info *names_info,
                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *name = NULL;
  datastore_name_info_t *current_names = NULL;
  datastore_name_info_t *modified_names = NULL;

  if (conf->modified_attr != NULL &&
      names_info != NULL) {
    if (conf->current_attr == NULL) {
      if (((ret = bridge_get_port_names(conf->modified_attr, &modified_names)) ==
           LAGOPUS_RESULT_OK)) {
        TAILQ_FOREACH(name, &modified_names->head, name_entries) {
          if (bridge_attr_port_name_exists(conf->current_attr,
                                           name->str) == false) {
            /* add_names. */
            if (datastore_name_exists(names_info->add_names,
                                      name->str) == false) {
              if ((ret = datastore_add_names(names_info->add_names,
                                               name->str)) !=
                  LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(
                        result, ret, "Can't add port names (add).");
                goto done;
              }
            }
          }
        }
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't get port names.");
      }
    } else {
      if (((ret = bridge_get_port_names(conf->current_attr, &current_names)) ==
           LAGOPUS_RESULT_OK) &&
          ((ret = bridge_get_port_names(conf->modified_attr, &modified_names)) ==
           LAGOPUS_RESULT_OK)) {
        TAILQ_FOREACH(name, &current_names->head, name_entries) {
          if (bridge_attr_port_name_exists(conf->modified_attr,
                                           name->str) == true) {
            /* not_change_names. */
            if (datastore_name_exists(names_info->not_change_names,
                                      name->str) == false) {
              if ((ret = datastore_add_names(names_info->not_change_names,
                                               name->str)) !=
                  LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(
                        result, ret, "Can't add port names (not change).");
                goto done;
              }
            }
          } else {
            /* del_names. */
            if (datastore_name_exists(names_info->del_names,
                                      name->str) == false) {
              if ((ret = datastore_add_names(names_info->del_names,
                                               name->str)) !=
                  LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(
                        result, ret, "Can't add port names (del).");
                goto done;
              }
            }
          }
        }
        TAILQ_FOREACH(name, &modified_names->head, name_entries) {
          if (bridge_attr_port_name_exists(conf->current_attr,
                                           name->str) == false) {
            /* add_names. */
            if (datastore_name_exists(names_info->add_names,
                                      name->str) == false) {
              if ((ret = datastore_add_names(names_info->add_names,
                                               name->str)) !=
                  LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(
                        result, ret, "Can't add port names (add).");
                goto done;
              }
            }
          }
        }
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't get port names.");
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

done:
  if (current_names != NULL) {
    datastore_names_destroy(current_names);
  }
  if (modified_names != NULL) {
    datastore_names_destroy(modified_names);
  }

  return ret;
}

/* get port/controller names info. */
static inline lagopus_result_t
bri_names_info_get(bridge_conf_t *conf,
                   struct names_info *p_names_info,
                   struct names_info *c_names_info,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = bri_port_names_info_get(conf, p_names_info, result);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = bri_controller_names_info_get(conf, c_names_info, result);
  }

  return ret;
}

/* get no change port/controller names. */
static inline lagopus_result_t
bri_names_info_create(struct names_info *names_info,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  names_info->not_change_names = NULL;
  names_info->add_names = NULL;
  names_info->del_names = NULL;

  if ((ret = datastore_names_create(&names_info->not_change_names)) !=
      LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_set(result, ret, NULL);
    goto done;
  }
  if ((ret = datastore_names_create(&names_info->add_names)) !=
      LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_set(result, ret, NULL);
    goto done;
  }
  if ((ret = datastore_names_create(&names_info->del_names)) !=
      LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_set(result, ret, NULL);
    goto done;
  }

done:
  return ret;
}

static inline void
bri_names_info_destroy(struct names_info *names_info) {
  datastore_names_destroy(names_info->not_change_names);
  datastore_names_destroy(names_info->add_names);
  datastore_names_destroy(names_info->del_names);
}

static inline lagopus_result_t
bri_port_update(datastore_interp_t *iptr,
                datastore_interp_state_t state,
                bridge_conf_t *conf,
                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct names_info names_info;
  struct datastore_name_entry *name = NULL;

  if ((ret = bri_names_info_create(&names_info, result)) !=
      LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't update port.");
    goto done;
  }

  if ((ret = bri_port_names_info_get(conf, &names_info, result)) !=
      LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't update port.");

  }

  if (TAILQ_EMPTY(&names_info.not_change_names->head) == false) {
    TAILQ_FOREACH(name, &names_info.not_change_names->head, name_entries) {
      if (((ret = port_cmd_update_propagation(iptr, state, name->str,
                                              result)) !=
           LAGOPUS_RESULT_OK) &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't update port.");
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

  if (TAILQ_EMPTY(&names_info.add_names->head) == false) {
    TAILQ_FOREACH(name, &names_info.add_names->head, name_entries) {
      if (((ret = port_cmd_update_propagation(iptr, state, name->str,
                                              result)) !=
           LAGOPUS_RESULT_OK) &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't update port.");
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

  if (TAILQ_EMPTY(&names_info.del_names->head) == false) {
    TAILQ_FOREACH(name, &names_info.del_names->head, name_entries) {
      if (((ret = port_cmd_update_propagation(iptr, state, name->str,
                                              result)) !=
           LAGOPUS_RESULT_OK) &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't update port.");
        lagopus_perror(ret);
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

done:
  bri_names_info_destroy(&names_info);

  return ret;
}

static inline lagopus_result_t
bri_controller_update(datastore_interp_t *iptr,
                      datastore_interp_state_t state,
                      bridge_conf_t *conf,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct names_info names_info;
  struct datastore_name_entry *name = NULL;

  if ((ret = bri_names_info_create(&names_info, result)) !=
      LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't update controller.");
    goto done;
  }

  if ((ret = bri_controller_names_info_get(conf, &names_info, result)) !=
      LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't update controller.");
  }

  if (TAILQ_EMPTY(&names_info.not_change_names->head) == false) {
    TAILQ_FOREACH(name, &names_info.not_change_names->head, name_entries) {
      if (((ret = controller_cmd_update_propagation(
                    iptr, state, name->str, result)) !=
           LAGOPUS_RESULT_OK) &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't update controller.");
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

  if (TAILQ_EMPTY(&names_info.add_names->head) == false) {
    TAILQ_FOREACH(name, &names_info.add_names->head, name_entries) {
      if (((ret = controller_cmd_update_propagation(
                    iptr, state, name->str, result)) !=
           LAGOPUS_RESULT_OK) &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't update controller.");
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

  if (TAILQ_EMPTY(&names_info.del_names->head) == false) {
    TAILQ_FOREACH(name, &names_info.del_names->head, name_entries) {
      if (((ret = controller_cmd_update_propagation(
                    iptr, state, name->str, result)) !=
           LAGOPUS_RESULT_OK) &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't update controller.");
        lagopus_perror(ret);
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

done:
  bri_names_info_destroy(&names_info);

  return ret;
}

static inline lagopus_result_t
bri_names_update(datastore_interp_t *iptr,
                 datastore_interp_state_t state,
                 bridge_conf_t *conf,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = bri_port_update(iptr, state, conf, result);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = bri_controller_update(iptr, state, conf, result);
  }

  return ret;
}

static inline void
bridge_cmd_update_current_attr(bridge_conf_t *conf,
                               datastore_interp_state_t state) {
  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    return;
  }

  if (conf->modified_attr != NULL) {
    if (conf->current_attr != NULL) {
      bridge_attr_destroy(conf->current_attr);
    }
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = NULL;
  }
}

static inline void
bridge_cmd_update_aborted(bridge_conf_t *conf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (conf->modified_attr != NULL) {
    if (conf->current_attr == NULL) {
      /* create. */
      ret = bridge_conf_delete(conf);
      if (ret != LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    } else {
      bridge_attr_destroy(conf->modified_attr);
      conf->modified_attr = NULL;
    }
  }
}

static inline void
bridge_cmd_update_switch_attr(bridge_conf_t *conf) {
  bridge_attr_t *attr;

  if (conf->modified_attr != NULL) {
    attr = conf->current_attr;
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = attr;
  }
}

static inline void
bridge_cmd_is_enabled_set(bridge_conf_t *conf) {
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
bridge_cmd_do_destroy(bridge_conf_t *conf,
                      datastore_interp_state_t state,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    ret = bridge_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (state == DATASTORE_INTERP_STATE_DRYRUN) {
    /* unset is_used. */
    if (conf->current_attr != NULL) {
      if ((ret = bri_names_used_set(conf->name,
                                    conf->current_attr,
                                    false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }
    if (conf->modified_attr != NULL) {
      if ((ret = bri_names_used_set(conf->name,
                                    conf->modified_attr,
                                    false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }

    ret = bridge_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (conf->is_destroying == true ||
             state == DATASTORE_INTERP_STATE_AUTO_COMMIT) {
    /* unset is_used. */
    if (conf->current_attr != NULL) {
      if ((ret = bri_names_used_set(conf->name,
                                    conf->current_attr,
                                    false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }
    if (conf->modified_attr != NULL) {
      if ((ret = bri_names_used_set(conf->name,
                                    conf->modified_attr,
                                    false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }

    if (conf->current_attr != NULL) {
      ret = bri_destroy(conf->name, conf->current_attr, result);
      if (ret != LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }

    ret = bridge_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  }
}

static inline lagopus_result_t
bridge_cmd_do_update(datastore_interp_t *iptr,
                     datastore_interp_state_t state,
                     bridge_conf_t *conf,
                     bool is_propagation,
                     bool is_enable_disable_cmd,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_modified = false;
  bool is_modified_without_names = false;
  bool is_modified_without_qmax_batches = false;
  struct names_info c_names_info;
  struct names_info p_names_info;
  (void) iptr;

  /* create names info. */
  if ((ret = bri_names_info_create(&c_names_info, result)) !=
      LAGOPUS_RESULT_OK) {
    goto done;

  }
  if ((ret = bri_names_info_create(&p_names_info, result)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  if ((ret = bri_names_info_get(conf,
                                &p_names_info,
                                &c_names_info,
                                result)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  if (conf->modified_attr != NULL) {
    if (bridge_attr_equals(conf->current_attr,
                           conf->modified_attr) == false) {
      if (conf->modified_attr != NULL) {
        is_modified = true;

        if (conf->current_attr == NULL &&
            conf->modified_attr != NULL) {
          is_modified_without_names = true;
          is_modified_without_qmax_batches = true;
        } else if (conf->current_attr != NULL) {
          if (bridge_attr_equals_without_names(
                  conf->current_attr,
                  conf->modified_attr) == false) {
            is_modified_without_names = true;
          }
          if (bridge_attr_equals_without_qmax_batches(
              conf->current_attr,
              conf->modified_attr) == false) {
            is_modified_without_qmax_batches = true;
          }
        }
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                        NULL);
      }
    } else if (bri_ports_is_modified(&p_names_info) == true) {
      is_modified = true;
    } else {
      /*
       * no need to re-create.
       */
      ret = LAGOPUS_RESULT_OK;
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
    /* update ports, controllers. */
    if (is_propagation == true) {
      if ((ret = bri_names_update(iptr, state, conf, result)) !=
          LAGOPUS_RESULT_OK) {
        goto done;
      }
    }

    if (is_modified == true) {
      /*
       * Update attrs.
       */
      if (conf->current_attr != NULL) {
        /* disable ports, controllers. */
        ret = bri_names_disable(conf->name,
                                conf->current_attr,
                                &p_names_info,
                                &c_names_info,
                                iptr, state,
                                is_propagation,
                                true,
                                result);
        if (ret == LAGOPUS_RESULT_OK) {
          if (is_modified_without_names == true) {
            if (is_modified_without_qmax_batches == true) {
              /* destroy bridge, ports, controllers. */
              ret = bri_destroy(conf->name, conf->current_attr,
                                result);
              if (ret != LAGOPUS_RESULT_OK) {
                lagopus_msg_warning("Can't delete bridge.\n");
              }
            } else {
              /* not destroy objs. */
              ret = LAGOPUS_RESULT_OK;
            }
          } else {
            /* delete ports, controllers. */
            ret = bri_names_delete(conf->name,
                                   conf->current_attr,
                                   &p_names_info,
                                   &c_names_info,
                                   result);
            if (ret != LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("Can't delete bridge ports/controllers.\n");
            }
          }
        }
#ifdef HYBRID
        /* mac entry delete */
#endif /* HYBRID */
      } else {
        ret = LAGOPUS_RESULT_OK;
      }

      if (ret == LAGOPUS_RESULT_OK) {
        if (is_modified_without_names == true) {
          if (is_modified_without_qmax_batches == true) {
            /* create bridge ports, controllers. */
            ret = bri_create(conf->name, conf->modified_attr, result);
          } else {
            ret = bri_queue_max_batches_set(conf->modified_attr, result);
          }
        } else {
          /* add ports, controllers. */
          ret = bri_names_add(conf->name,
                              conf->modified_attr,
                              &p_names_info,
                              &c_names_info,
                              result);
        }

        if (ret == LAGOPUS_RESULT_OK &&
            is_modified_without_qmax_batches == true) {
          /* set used. */
          if ((ret = bri_names_used_set(conf->name,
                                        conf->modified_attr,
                                        true, result)) ==
              LAGOPUS_RESULT_OK) {
            if (conf->is_enabled == true) {
              /* start bridge. */
              ret = bri_bridge_start(conf->name,
                                     conf->modified_attr,
                                     &p_names_info,
                                     &c_names_info,
                                     iptr, state,
                                     is_propagation,
                                     is_modified_without_names,
                                     result);
            }
          }
        }
      }

      /* Update attr. */
      if (ret == LAGOPUS_RESULT_OK &&
          state != DATASTORE_INTERP_STATE_COMMITING &&
          state != DATASTORE_INTERP_STATE_ROLLBACKING) {
        bridge_cmd_update_current_attr(conf, state);
      }
    } else {
      if (is_enable_disable_cmd == true ||
          conf->is_enabling == true ||
          conf->is_disabling == true) {
        if (conf->is_enabled == true) {
          /* start bridge. */
          ret = bri_bridge_start(conf->name,
                                 conf->current_attr,
                                 &p_names_info,
                                 &c_names_info,
                                 iptr, state,
                                 is_propagation,
                                 true,
                                 result);
        } else {
          /* stop bridge. */
          ret = bri_bridge_stop(conf->name,
                                conf->current_attr,
                                iptr,
                                state,
                                result);
        }
      }
      conf->is_enabling = false;
      conf->is_disabling = false;
    }
  }

done:
  bri_names_info_destroy(&c_names_info);
  bri_names_info_destroy(&p_names_info);

  return ret;
}

static inline lagopus_result_t
bridge_cmd_update_internal(datastore_interp_t *iptr,
                           datastore_interp_state_t state,
                           bridge_conf_t *conf,
                           bool is_propagation,
                           bool is_enable_disable_cmd,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int i;

  switch (state) {
    case DATASTORE_INTERP_STATE_AUTO_COMMIT: {
      i = 0;
      while (i < UPDATE_CNT_MAX) {
        ret = bridge_cmd_do_update(iptr, state, conf,
                                   is_propagation,
                                   is_enable_disable_cmd,
                                   result);
        if (ret == LAGOPUS_RESULT_OK ||
            is_enable_disable_cmd == true) {
          break;
        } else if (conf->current_attr == NULL &&
                   conf->modified_attr != NULL) {
          bridge_cmd_do_destroy(conf, state, result);
          break;
        } else {
          bridge_cmd_update_switch_attr(conf);
          lagopus_msg_warning("FAILED auto_comit (%s): rollbacking....\n",
                              lagopus_error_get_string(ret));
        }
        i++;
      }
      break;
    }
    case DATASTORE_INTERP_STATE_COMMITING: {
      bridge_cmd_is_enabled_set(conf);
      ret = bridge_cmd_do_update(iptr, state, conf,
                                 is_propagation,
                                 is_enable_disable_cmd,
                                 result);
      break;
    }
    case DATASTORE_INTERP_STATE_ATOMIC: {
      /* set is_used. */
      if (conf->modified_attr != NULL) {
        /* unset is_used. */
        if (bri_names_used_set(conf->name,
                               conf->modified_attr,
                               true, result) !=
            LAGOPUS_RESULT_OK) {
          /* ignore error. */
          lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
        }
      }
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_COMMITED:
    case DATASTORE_INTERP_STATE_ROLLBACKED: {
      bridge_cmd_update_current_attr(conf, state);
      bridge_cmd_do_destroy(conf, state, result);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_ROLLBACKING: {
      if (conf->current_attr == NULL &&
          conf->modified_attr != NULL) {
        /* case create. */
        /* unset is_used. */
        if (bri_names_used_set(conf->name,
                               conf->modified_attr,
                               false, result) !=
            LAGOPUS_RESULT_OK) {
          /* ignore error. */
          lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
        }
        ret = LAGOPUS_RESULT_OK;
      } else {
        bridge_cmd_update_switch_attr(conf);
        bridge_cmd_is_enabled_set(conf);
        ret = bridge_cmd_do_update(iptr, state, conf,
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
      if (conf->modified_attr != NULL) {
        /* unset is_used. */
        if ((ret = bri_names_used_set(conf->name,
                                      conf->modified_attr,
                                      false, result)) !=
            LAGOPUS_RESULT_OK) {
          /* ignore error. */
          lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
        }
      }
      if (conf->current_attr != NULL) {
        /* unset is_used. */
        if ((ret = bri_names_used_set(conf->name,
                                      conf->current_attr,
                                      true, result)) !=
            LAGOPUS_RESULT_OK) {
          /* ignore error. */
          lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
        }
      }
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_ABORTED: {
      bridge_cmd_update_aborted(conf);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_DRYRUN: {
      if (conf->modified_attr != NULL) {
        if (conf->current_attr != NULL) {
          bridge_attr_destroy(conf->current_attr);
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
bridge_cmd_update(datastore_interp_t *iptr,
                  datastore_interp_state_t state,
                  void *o,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) iptr;
  (void) result;

  if (iptr != NULL && *iptr != NULL && o != NULL) {
    bridge_conf_t *conf = (bridge_conf_t *)o;
    ret = bridge_cmd_update_internal(iptr, state, conf, false, false, result);
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

/* parse. */
static lagopus_result_t
uint_opt_parse(const char *const *argv[],
               bridge_conf_t *conf,
               configs_t *configs,
               void *uint_set_proc,
               enum bridge_opts opt,
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
bool_opt_parse(const char *const *argv[],
               bridge_conf_t *conf,
               configs_t *configs,
               bool_set_proc_t bool_set_proc,
               enum bridge_opts opt,
               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool b;

  if (argv != NULL && conf != NULL &&
      configs != NULL && bool_set_proc != NULL &&
      result != NULL) {
    if (*(*argv + 1) != NULL) {
      if ((ret = lagopus_str_parse_bool(*(++(*argv)),
                                        &b)) ==
          LAGOPUS_RESULT_OK) {
        ret = (bool_set_proc)(conf->modified_attr, b);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't add %s.",
                                                  ATTR_NAME_GET(opt_strs, opt));
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

static inline lagopus_result_t
clear_q_opt_parse(const char *const *argv[],
                  bridge_conf_t *conf,
                  configs_t *configs,
                  void *proc,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_attr_t *attr = NULL;
  uint64_t dpid;

  if (argv != NULL && conf != NULL &&
      configs != NULL && proc != NULL &&
      result != NULL) {
    attr = conf->current_attr == NULL ?
        conf->modified_attr : conf->current_attr;

    if (attr != NULL) {
      if ((ret = bridge_get_dpid(attr, &dpid)) ==
          LAGOPUS_RESULT_OK) {
        ret = ((clear_q_proc_t) proc)(dpid);
      } else {
        ret = datastore_json_result_set(result, ret, NULL);
      }
    } else {
      ret = datastore_json_result_string_setf(
          result,
          LAGOPUS_RESULT_INVALID_OBJECT,
          "Bad attr (%s).", conf->name);
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static lagopus_result_t
controller_opt_parse(const char *const *argv[],
                     void *c, void *out_configs,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_conf_t *conf = NULL;
  configs_t *configs = NULL;
  char *name = NULL;
  char *name_str = NULL;
  char *fullname = NULL;
  bool is_added = false;
  bool is_exists = false;
  bool is_used = false;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (bridge_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = lagopus_str_unescape(*(*argv), "\"'", &name_str)) >= 0) {
          if ((ret = cmd_opt_name_get(name_str, &name, &is_added)) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = namespace_get_fullname(name, &fullname))
                == LAGOPUS_RESULT_OK) {
              is_exists =
                bridge_attr_controller_name_exists(conf->modified_attr,
                                                   fullname);
            } else {
              ret = datastore_json_result_string_setf(result,
                                                      ret,
                                                      "Can't get fullname %s.",
                                                      name);
              goto done;
            }
          } else {
            ret = datastore_json_result_string_setf(
                result, ret,
                "Can't get controller_name.");
            goto done;
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "controller name = %s.",
                                                  *(*argv));
          goto done;
        }

        if (is_added == true) {
          /* add. */
          /* check exists. */
          if (is_exists == true) {
            ret = datastore_json_result_string_setf(
                result,
                LAGOPUS_RESULT_ALREADY_EXISTS,
                "controller name = %s.", fullname);
            goto done;
          }

          if (controller_exists(fullname) == false) {
            ret = datastore_json_result_string_setf(
                result,
                LAGOPUS_RESULT_NOT_FOUND,
                "controller name = %s.", fullname);
            goto done;
          }

          /* check is_used. */
          if ((ret =
               datastore_controller_is_used(fullname, &is_used)) ==
              LAGOPUS_RESULT_OK) {
            if (is_used == false) {
              ret = bridge_attr_add_controller_name(
                  conf->modified_attr,
                  fullname);
              if (ret != LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(
                    result, ret,
                    "controller name = %s.", fullname);
              }
            } else {
              ret = datastore_json_result_string_setf(
                  result,
                  LAGOPUS_RESULT_NOT_OPERATIONAL,
                  "controller name = %s.", fullname);
            }
          } else {
            ret = datastore_json_result_string_setf(
                result, ret,
                "controller name = %s.", fullname);
          }
        } else {
          /* delete. */
          if (is_exists == false) {
            ret = datastore_json_result_string_setf(
                result, LAGOPUS_RESULT_NOT_FOUND,
                "controller name = %s.", fullname);
            goto done;
          }

          ret = bridge_attr_remove_controller_name(conf->modified_attr,
                                                   fullname);
          if (ret == LAGOPUS_RESULT_OK) {
            /* unset is_used. */
            ret = bri_controller_used_set_internal(fullname,
                                                   false,
                                                   result);
          } else {
            ret = datastore_json_result_string_setf(
                result, ret,
                "controller name = %s.", fullname);
          }
        }
      } else {
        if (*(*argv) == NULL) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt value.");
        } else {
          ret = datastore_json_result_string_setf(
                  result,
                  LAGOPUS_RESULT_INVALID_ARGS,
                  "Bad opt value = %s.", *(*argv));
        }
      }
    } else if (configs->is_config == true) {
      configs->flags = OPT_BIT_GET(OPT_CONTROLLERS);
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

done:
  free(name);
  free(name_str);
  free(fullname);

  return ret;
}

static inline bool
port_opt_number_is_exists(bridge_conf_t *conf,
                          uint32_t port_no) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool rv = false;
  struct datastore_name_entry *p_name = NULL;
  datastore_name_info_t *p_c_names = NULL;
  datastore_name_info_t *p_m_names = NULL;

  if ((ret = bridge_get_port_names(conf->current_attr,
                                   &p_c_names)) ==
      LAGOPUS_RESULT_OK) {
    if (TAILQ_EMPTY(&p_c_names->head) == false) {
      TAILQ_FOREACH(p_name, &p_c_names->head, name_entries) {
        ret = port_cmd_port_number_is_exists(p_name->str,
                                             port_no,
                                             &rv);
        if (ret == LAGOPUS_RESULT_OK) {
          if (rv == true) {
            goto done;
          }
        } else {
          goto done;
        }
      }
    }
  }
  if ((ret = bridge_get_port_names(conf->modified_attr,
                                   &p_m_names)) ==
      LAGOPUS_RESULT_OK) {
    if (TAILQ_EMPTY(&p_m_names->head) == false) {
      TAILQ_FOREACH(p_name, &p_m_names->head, name_entries) {
        ret = port_cmd_port_number_is_exists(p_name->str,
                                             port_no,
                                             &rv);
        if (ret == LAGOPUS_RESULT_OK) {
          if (rv == true) {
            goto done;
          }
        } else {
          goto done;
        }
      }
    }
  }

done:
  if (p_c_names != NULL) {
    datastore_names_destroy(p_c_names);
  }
  if (p_m_names != NULL) {
    datastore_names_destroy(p_m_names);
  }

  return rv;
}

static lagopus_result_t
port_opt_parse(const char *const *argv[],
               void *c, void *out_configs,
               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_conf_t *conf = NULL;
  configs_t *configs = NULL;
  char *name = NULL;
  char *name_str = NULL;
  char *fullname = NULL;
  bool is_added = false;
  bool is_exists = false;
  bool is_used = false;
  union cmd_uint cmd_uint;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (bridge_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = lagopus_str_unescape(*(*argv), "\"'", &name_str)) >= 0) {
          if ((ret = cmd_opt_name_get(name_str, &name, &is_added)) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = namespace_get_fullname(name, &fullname))
                == LAGOPUS_RESULT_OK) {
              is_exists = bridge_attr_port_name_exists(conf->modified_attr,
                                                       fullname);
            } else {
              ret = datastore_json_result_string_setf(
                  result,
                  ret,
                  "Can't get fullname %s.",
                  name);
              goto done;
            }
          } else {
            ret = datastore_json_result_string_setf(
                result, ret,
                "Can't get port_name.");
            goto done;
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "port name = %s.",
                                                  *(*argv));
          goto done;
        }

        if (is_added == true) {
          /* add. */
          /* check exists. */
          if (is_exists == true) {
            ret = datastore_json_result_string_setf(
                result,
                LAGOPUS_RESULT_ALREADY_EXISTS,
                "port name = %s.", fullname);
            goto done;
          }

          if (port_exists(fullname) == false) {
            ret = datastore_json_result_string_setf(
                result,
                LAGOPUS_RESULT_NOT_FOUND,
                "port name = %s.", fullname);
            goto done;
          }

          /* parse port number. */
          if (*(*argv + 1) == NULL) {
            ret = datastore_json_result_string_setf(
                result,
                LAGOPUS_RESULT_INVALID_ARGS,
                "Bad opt value.");
            goto done;
          }

          if (IS_VALID_OPT(*(*argv + 1)) == true) {
            /* argv + 1 equals option string(-XXX). */
            ret = datastore_json_result_string_setf(
                result, LAGOPUS_RESULT_INVALID_ARGS,
                "Bad opt value = %s.",
                *(*argv + 1));
            goto done;
          }

          if ((ret = cmd_uint_parse(*(++(*argv)), CMD_UINT32,
                                    &cmd_uint)) !=
              LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_string_setf(
                result, ret,
                "Bad opt value = %s.",
                *(*argv));
            goto done;
          }

          if (port_opt_number_is_exists(conf,
                                        cmd_uint.uint32) ==
              true) {
            ret = datastore_json_result_string_setf(
                result, LAGOPUS_RESULT_ALREADY_EXISTS,
                "port name = %s, port number = %"PRIu32".",
                fullname, cmd_uint.uint32);
            goto done;
          }

          if (cmd_uint.uint32 == 0) {
            ret = datastore_json_result_string_setf(
                result, LAGOPUS_RESULT_OUT_OF_RANGE,
                "port name = %s, port number = %"PRIu32".",
                fullname, cmd_uint.uint32);
            goto done;
          }

          /* set port. */
          ret = port_cmd_set_port_number(fullname,
                                         cmd_uint.uint32,
                                         result);
          if (ret == LAGOPUS_RESULT_OK) {
            /* check is_used. */
            if ((ret = datastore_port_is_used(fullname,
                                              &is_used)) ==
                LAGOPUS_RESULT_OK) {
              if (is_used == false) {
                /* add name. */
                ret = bridge_attr_add_port_name(
                    conf->modified_attr, fullname);

                if (ret != LAGOPUS_RESULT_OK) {
                  ret = datastore_json_result_string_setf(
                      result, ret,
                      "port name = %s.", fullname);
                }
              } else {
                ret = datastore_json_result_string_setf(
                    result,
                    LAGOPUS_RESULT_NOT_OPERATIONAL,
                    "port name = %s.", fullname);
              }
            } else {
              ret = datastore_json_result_string_setf(
                  result, ret,
                  "port name = %s.",
                  fullname);
            }
          } else if (ret !=
                     LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
            ret = datastore_json_result_string_setf(
                result, ret,
                "Can't add port_number.");
          }
        } else {
          /* delete. */
          if (is_exists == false) {
            ret = datastore_json_result_string_setf(
                result, LAGOPUS_RESULT_NOT_FOUND,
                "port name = %s.", fullname);
            goto done;
          }

          if (*(*argv + 1) != NULL) {
            if (IS_VALID_OPT(*(*argv + 1)) == false) {
              /* argv + 1 equals option string(-XXX). */
              ret = datastore_json_result_string_setf(
                  result, LAGOPUS_RESULT_INVALID_ARGS,
                  "Bad opt value = %s. "
                  "Do not specify the port number.",
                  *(*argv + 1));
              goto done;
            }
          }

          /* 0 means unassigned port. */
          ret = port_cmd_set_port_number(fullname, 0, result);
          if (ret == LAGOPUS_RESULT_OK) {
            ret = bridge_attr_remove_port_name(conf->modified_attr,
                                               fullname);
            if (ret == LAGOPUS_RESULT_OK) {
              /* unset is_used. */
              ret = bri_port_used_set_internal(
                  fullname, false, result);
            } else {
              ret = datastore_json_result_string_setf(result, ret,
                                                      "port name = %s.",
                                                      fullname);
            }
          } else if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
            ret = datastore_json_result_string_setf(
                result, ret,
                "Can't delete port_number.");
          }
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
      configs->flags = OPT_BIT_GET(OPT_PORTS);
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

done:
  free(name);
  free(name_str);
  free(fullname);

  return ret;
}

static lagopus_result_t
dpid_opt_parse(const char *const *argv[],
               void *c, void *out_configs,
               lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (bridge_conf_t *)c, (configs_t *) out_configs,
                        &bridge_set_dpid, OPT_DPID, CMD_UINT64, result);
}

static lagopus_result_t
fail_mode_opt_parse(const char *const *argv[],
                    void *c, void *out_configs,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_conf_t *conf = NULL;
  configs_t *configs = NULL;
  datastore_bridge_fail_mode_t fail_mode;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (bridge_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = bridge_fail_mode_to_enum(*(*argv), &fail_mode)) ==
            LAGOPUS_RESULT_OK) {
          ret = bridge_set_fail_mode(conf->modified_attr,
                                     fail_mode);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't set fail_mode.");
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Bad opt value = %s.",
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
      configs->flags = OPT_BIT_GET(OPT_FAIL_MODE);
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
flow_statistics_opt_parse(const char *const *argv[],
                          void *c, void *out_configs,
                          lagopus_dstring_t *result) {
  return bool_opt_parse(argv, (bridge_conf_t *) c, (configs_t *) out_configs,
                        &bridge_set_flow_statistics, OPT_FLOW_STATISTICS,
                        result);
}

static lagopus_result_t
group_statistics_opt_parse(const char *const *argv[],
                           void *c, void *out_configs,
                           lagopus_dstring_t *result) {
  return bool_opt_parse(argv, (bridge_conf_t *) c, (configs_t *) out_configs,
                        &bridge_set_group_statistics, OPT_GROUP_STATISTICS,
                        result);
}

static lagopus_result_t
port_statistics_opt_parse(const char *const *argv[],
                          void *c, void *out_configs,
                          lagopus_dstring_t *result) {
  return bool_opt_parse(argv, (bridge_conf_t *) c, (configs_t *) out_configs,
                        &bridge_set_port_statistics, OPT_PORT_STATISTICS,
                        result);
}

static lagopus_result_t
queue_statistics_opt_parse(const char *const *argv[],
                           void *c, void *out_configs,
                           lagopus_dstring_t *result) {
  return bool_opt_parse(argv, (bridge_conf_t *) c, (configs_t *) out_configs,
                        &bridge_set_queue_statistics, OPT_QUEUE_STATISTICS,
                        result);
}

static lagopus_result_t
table_statistics_opt_parse(const char *const *argv[],
                           void *c, void *out_configs,
                           lagopus_dstring_t *result) {
  return bool_opt_parse(argv, (bridge_conf_t *) c, (configs_t *) out_configs,
                        &bridge_set_table_statistics, OPT_TABLE_STATISTICS,
                        result);
}

static lagopus_result_t
reassemble_ip_fragments_opt_parse(const char *const *argv[],
                                  void *c, void *out_configs,
                                  lagopus_dstring_t *result) {
  return bool_opt_parse(argv, (bridge_conf_t *) c, (configs_t *) out_configs,
                        &bridge_set_reassemble_ip_fragments,
                        OPT_REASSEMBLE_IP_FRAGMENTS,
                        result);
}

static lagopus_result_t
max_buffered_packets_opt_parse(const char *const *argv[],
                               void *c, void *out_configs,
                               lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (bridge_conf_t *)c, (configs_t *) out_configs,
                        &bridge_set_max_buffered_packets,
                        OPT_MAX_BUFFERED_PACKETS, CMD_UINT64, result);
}

static lagopus_result_t
max_ports_opt_parse(const char *const *argv[],
                    void *c, void *out_configs,
                    lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (bridge_conf_t *)c, (configs_t *) out_configs,
                        &bridge_set_max_ports, OPT_MAX_PORTS, CMD_UINT64,
                        result);
}

static lagopus_result_t
max_tables_opt_parse(const char *const *argv[],
                     void *c, void *out_configs,
                     lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (bridge_conf_t *)c, (configs_t *) out_configs,
                        &bridge_set_max_tables, OPT_MAX_TABLES, CMD_UINT64,
                        result);
}

static lagopus_result_t
max_flows_opt_parse(const char *const *argv[],
                    void *c, void *out_configs,
                    lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (bridge_conf_t *)c, (configs_t *) out_configs,
                        &bridge_set_max_flows, OPT_MAX_FLOWS, CMD_UINT64,
                        result);
}

static lagopus_result_t
packet_inq_size_opt_parse(const char *const *argv[],
                          void *c, void *out_configs,
                          lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (bridge_conf_t *)c, (configs_t *) out_configs,
                        &bridge_set_packet_inq_size,
                        OPT_PACKET_INQ_SIZE, CMD_UINT64,
                        result);
}

static lagopus_result_t
packet_inq_max_batches_opt_parse(const char *const *argv[],
                                 void *c, void *out_configs,
                                 lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (bridge_conf_t *)c, (configs_t *) out_configs,
                        &bridge_set_packet_inq_max_batches,
                        OPT_PACKET_INQ_MAX_BATCHES, CMD_UINT64,
                        result);
}

static lagopus_result_t
up_streamq_size_opt_parse(const char *const *argv[],
                          void *c, void *out_configs,
                          lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (bridge_conf_t *)c, (configs_t *) out_configs,
                        &bridge_set_up_streamq_size,
                        OPT_UP_STREAMQ_SIZE, CMD_UINT64,
                        result);
}

static lagopus_result_t
up_streamq_max_batches_opt_parse(const char *const *argv[],
                                 void *c, void *out_configs,
                                 lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (bridge_conf_t *)c, (configs_t *) out_configs,
                        &bridge_set_up_streamq_max_batches,
                        OPT_UP_STREAMQ_MAX_BATCHES, CMD_UINT64,
                        result);
}

static lagopus_result_t
down_streamq_size_opt_parse(const char *const *argv[],
                            void *c, void *out_configs,
                            lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (bridge_conf_t *)c, (configs_t *) out_configs,
                        &bridge_set_down_streamq_size,
                        OPT_DOWN_STREAMQ_SIZE, CMD_UINT64,
                        result);
}

static lagopus_result_t
down_streamq_max_batches_opt_parse(const char *const *argv[],
                                   void *c, void *out_configs,
                                   lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (bridge_conf_t *)c, (configs_t *) out_configs,
                        &bridge_set_down_streamq_max_batches,
                        OPT_DOWN_STREAMQ_MAX_BATCHES, CMD_UINT64,
                        result);
}

static lagopus_result_t
block_looping_ports_opt_parse(const char *const *argv[],
                              void *c, void *out_configs,
                              lagopus_dstring_t *result) {
  return bool_opt_parse(argv, (bridge_conf_t *) c, (configs_t *) out_configs,
                        &bridge_set_block_looping_ports,
                        OPT_BLOCK_LOOPING_PORTS,
                        result);
}

static lagopus_result_t
packet_inq_opt_parse(const char *const *argv[],
                     void *c, void *out_configs,
                     lagopus_dstring_t *result) {
  return clear_q_opt_parse(argv, (bridge_conf_t *) c, (configs_t *) out_configs,
                           &ofp_bridgeq_mgr_dataq_clear,
                           result);
}

static lagopus_result_t
up_streamq_opt_parse(const char *const *argv[],
                     void *c, void *out_configs,
                     lagopus_dstring_t *result) {
  return clear_q_opt_parse(argv, (bridge_conf_t *) c, (configs_t *) out_configs,
                           &ofp_bridgeq_mgr_eventq_clear,
                           result);
}

static lagopus_result_t
down_streamq_opt_parse(const char *const *argv[],
                       void *c, void *out_configs,
                       lagopus_dstring_t *result) {
  return clear_q_opt_parse(argv, (bridge_conf_t *) c, (configs_t *) out_configs,
                           &ofp_bridgeq_mgr_event_dataq_clear,
                           result);
}

static lagopus_result_t
action_type_opt_parse(const char *const *argv[],
                      void *c, void *out_configs,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_conf_t *conf = NULL;
  configs_t *configs = NULL;
  datastore_bridge_action_type_t type;
  uint64_t types = 0LL;
  char *type_str = NULL;
  bool is_added = false;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (bridge_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = cmd_opt_type_get(*(*argv), &type_str, &is_added)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get type.");
          goto done;
        }

        if ((ret = bridge_action_type_to_enum(type_str, &type)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result,
              LAGOPUS_RESULT_INVALID_ARGS,
              "Bad opt value = %s.", *(*argv));
          goto done;
        }

        if ((ret = bridge_get_action_types(conf->modified_attr,
                                           &types)) !=
              LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get action_types.");
          goto done;
        }

        if (is_added == true) {
          /* add type. */
          types |= (uint64_t) OPT_BIT_GET(type);
        } else {
          /* delete type. */
          types &= (uint64_t) ~OPT_BIT_GET(type);
        }
        ret = bridge_set_action_types(conf->modified_attr,
                                      types);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't add action_types.");
        }
      } else {
        if (*(*argv) == NULL) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt value.");
        } else {
          ret = datastore_json_result_string_setf(
                  result,
                  LAGOPUS_RESULT_INVALID_ARGS,
                  "Bad opt value = %s.", *(*argv));
        }
      }
    } else if (configs->is_config == true) {
      configs->flags = OPT_BIT_GET(OPT_ACTION_TYPES);
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

done:
  free(type_str);

  return ret;
}

static lagopus_result_t
instruction_type_opt_parse(const char *const *argv[],
                           void *c, void *out_configs,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_conf_t *conf = NULL;
  configs_t *configs = NULL;
  datastore_bridge_instruction_type_t type;
  uint64_t types = 0LL;
  char *type_str = NULL;
  bool is_added = false;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (bridge_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = cmd_opt_type_get(*(*argv), &type_str, &is_added)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get type.");
          goto done;
        }

        if ((ret = bridge_instruction_type_to_enum(type_str, &type)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result,
              LAGOPUS_RESULT_INVALID_ARGS,
              "Bad opt value = %s.", *(*argv));
          goto done;
        }

        if ((ret = bridge_get_instruction_types(conf->modified_attr,
                                                &types)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result, ret,
              "Can't get instruction_types.");
          goto done;
        }

        if (is_added == true) {
          /* add type. */
          types |= (uint64_t) OPT_BIT_GET(type);
        } else {
          /* delete type. */
          types &= (uint64_t) ~OPT_BIT_GET(type);
        }
        ret = bridge_set_instruction_types(conf->modified_attr,
                                           types);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result, ret,
              "Can't add instruction_types.");
        }
      } else {
        if (*(*argv) == NULL) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt value.");
        } else {
          ret = datastore_json_result_string_setf(
                  result,
                  LAGOPUS_RESULT_INVALID_ARGS,
                  "Bad opt value = %s.", *(*argv));
        }
      }
    } else if (configs->is_config == true) {
      configs->flags = OPT_BIT_GET(OPT_INSTRUCTION_TYPES);
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

done:
  free(type_str);

  return ret;
}

static lagopus_result_t
reserved_port_type_opt_parse(const char *const *argv[],
                             void *c, void *out_configs,
                             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_conf_t *conf = NULL;
  configs_t *configs = NULL;
  datastore_bridge_reserved_port_type_t type;
  uint64_t types = 0LL;
  char *type_str = NULL;
  bool is_added = false;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (bridge_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = cmd_opt_type_get(*(*argv), &type_str, &is_added)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get type.");
          goto done;
        }

        if ((ret = bridge_reserved_port_type_to_enum(type_str, &type)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result,
              LAGOPUS_RESULT_INVALID_ARGS,
              "Bad opt value = %s.", *(*argv));
          goto done;
        }

        if ((ret = bridge_get_reserved_port_types(conf->modified_attr,
                                                  &types)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result, ret,
              "Can't get reserved_port_types.");
          goto done;
        }

        if (is_added == true) {
          /* add type. */
          types |= (uint64_t) OPT_BIT_GET(type);
        } else {
          /* delete type. */
          types &= (uint64_t) ~OPT_BIT_GET(type);
        }
        ret = bridge_set_reserved_port_types(conf->modified_attr,
                                             types);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result, ret,
              "Can't add reserved_port_types.");
        }
      } else {
        if (*(*argv) == NULL) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt value.");
        } else {
          ret = datastore_json_result_string_setf(
                  result,
                  LAGOPUS_RESULT_INVALID_ARGS,
                  "Bad opt value = %s.", *(*argv));
        }
      }
    } else if (configs->is_config == true) {
      configs->flags = OPT_BIT_GET(OPT_RESERVED_PORT_TYPES);
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

done:
  free(type_str);

  return ret;
}

static lagopus_result_t
group_type_opt_parse(const char *const *argv[],
                     void *c, void *out_configs,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_conf_t *conf = NULL;
  configs_t *configs = NULL;
  datastore_bridge_group_type_t type;
  uint64_t types = 0LL;
  char *type_str = NULL;
  bool is_added = false;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (bridge_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = cmd_opt_type_get(*(*argv), &type_str, &is_added)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get type.");
          goto done;
        }

        if ((ret = bridge_group_type_to_enum(type_str, &type)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result,
              LAGOPUS_RESULT_INVALID_ARGS,
              "Bad opt value = %s.", *(*argv));
          goto done;
        }

        if ((ret = bridge_get_group_types(conf->modified_attr,
                                          &types)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result, ret,
              "Can't get group_types.");
          goto done;
        }

        if (is_added == true) {
          /* add type. */
          types |= (uint64_t) OPT_BIT_GET(type);
        } else {
          /* delete type. */
          types &= (uint64_t) ~OPT_BIT_GET(type);
        }
        ret = bridge_set_group_types(conf->modified_attr,
                                     types);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result, ret,
              "Can't add group_types.");
        }
      } else {
        if (*(*argv) == NULL) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt value.");
        } else {
          ret = datastore_json_result_string_setf(
                  result,
                  LAGOPUS_RESULT_INVALID_ARGS,
                  "Bad opt value = %s.", *(*argv));
        }
      }
    } else if (configs->is_config == true) {
      configs->flags = OPT_BIT_GET(OPT_GROUP_TYPES);
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

done:
  free(type_str);

  return ret;
}

static lagopus_result_t
group_capability_opt_parse(const char *const *argv[],
                           void *c, void *out_configs,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_conf_t *conf = NULL;
  configs_t *configs = NULL;
  datastore_bridge_group_capability_t type;
  uint64_t types = 0LL;
  char *type_str = NULL;
  bool is_added = false;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (bridge_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = cmd_opt_type_get(*(*argv), &type_str, &is_added)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get type.");
          goto done;
        }

        if ((ret = bridge_group_capability_to_enum(type_str, &type)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result,
              LAGOPUS_RESULT_INVALID_ARGS,
              "Bad opt value = %s.", *(*argv));
          goto done;
        }

        if ((ret = bridge_get_group_capabilities(conf->modified_attr,
                       &types)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result, ret,
              "Can't get group_capabilities.");
          goto done;
        }

        if (is_added == true) {
          /* add type. */
          types |= (uint64_t) OPT_BIT_GET(type);
        } else {
          /* delete type. */
          types &= (uint64_t) ~OPT_BIT_GET(type);
        }
        ret = bridge_set_group_capabilities(conf->modified_attr,
                                            types);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result, ret,
              "Can't add group_capabilities.");
        }
      } else {
        if (*(*argv) == NULL) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt value.");
        } else {
          ret = datastore_json_result_string_setf(
                  result,
                  LAGOPUS_RESULT_INVALID_ARGS,
                  "Bad opt value = %s.", *(*argv));
        }
      }
    } else if (configs->is_config == true) {
      configs->flags = OPT_BIT_GET(OPT_GROUP_CAPABILITIES);
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

done:
  free(type_str);

  return ret;
}

static inline uint64_t
dpid_generate(void) {
  uint64_t dpid;

  dpid = (uint64_t) random();
  dpid <<= 32;
  dpid |= (uint64_t) random();

  return dpid;
}

static inline lagopus_result_t
opt_parse(const char *const argv[],
          bridge_conf_t *conf,
          configs_t *out_configs,
          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_bridge_fail_mode_t fail_mode;
  void *opt_proc;
  uint64_t dpid = 0;

  argv++;
  if (*argv != NULL) {
    while (*argv != NULL) {
      if (IS_VALID_STRING(*(argv)) == true) {
        if (lagopus_hashmap_find(&opt_table,
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
                                                  "opt = %s.", *argv);
          goto done;
        }
      } else {
        ret = datastore_json_result_set(result,
                                        LAGOPUS_RESULT_INVALID_ARGS,
                                        NULL);
        goto done;
      }
      argv++;
    }
  } else {
    /* config: all items show. */
    if (out_configs->is_config == true) {
      out_configs->flags = OPTS_MAX;
    }
    ret =  LAGOPUS_RESULT_OK;
  }

  /* set required opts. */
  if (conf->modified_attr != NULL) {
    /* generate dpid. */
    if ((ret = bridge_get_dpid(conf->modified_attr,
                               &dpid)) ==
        LAGOPUS_RESULT_OK) {
      if (dpid == 0) {
        dpid = dpid_generate();

        ret = bridge_set_dpid(conf->modified_attr,
                              dpid);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't add dpid.");
          goto done;
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't generate dpid.");
      goto done;
    }

    /* set fail_mode */
    if ((ret = bridge_get_fail_mode(conf->modified_attr,
                                    &fail_mode)) ==
        LAGOPUS_RESULT_OK) {
      if (fail_mode == DATASTORE_BRIDGE_FAIL_MODE_UNKNOWN) {
        ret = bridge_set_fail_mode(conf->modified_attr,
                                   DATASTORE_BRIDGE_FAIL_MODE_SECURE);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't set fail_mode.");
          goto done;
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't get fail_mode.");
      goto done;
    }
  }

done:
  return ret;
}

static inline lagopus_result_t
clear_queue_opt_parse(const char *const argv[],
                      bridge_conf_t *conf,
                      configs_t *out_configs,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  void *opt_proc;

  argv++;
  if (*argv != NULL) {
    while (*argv != NULL) {
      if (IS_VALID_STRING(*(argv)) == true) {
        if (lagopus_hashmap_find(&opt_clear_queue_table,
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
                                                  "opt = %s.", *argv);
          goto done;
        }
      } else {
        ret = datastore_json_result_set(result,
                                        LAGOPUS_RESULT_INVALID_ARGS,
                                        NULL);
        goto done;
      }
      argv++;
    }
  } else {
    /* clear all queue. */
    if ((ret = packet_inq_opt_parse(&argv,
                                    (void *) conf,
                                    (void *) out_configs,
                                    result)) != LAGOPUS_RESULT_OK) {
      goto done;
    }
    if ((ret = up_streamq_opt_parse(&argv,
                                    (void *) conf,
                                    (void *) out_configs,
                                    result)) != LAGOPUS_RESULT_OK) {
      goto done;
    }
    if ((ret = down_streamq_opt_parse(&argv,
                                      (void *) conf,
                                      (void *) out_configs,
                                      result)) != LAGOPUS_RESULT_OK) {
      goto done;
    }
  }

done:
  return ret;
}

#ifdef HYBRID
/* mactable configurations */
/**
 * Parse l2-bridge option.
 */
static lagopus_result_t
l2_bridge_opt_parse(const char *const *argv[],
                    void *c, void *out_configs,
                    lagopus_dstring_t *result) {
  return bool_opt_parse(argv, (bridge_conf_t *) c, (configs_t *) out_configs,
                        &bridge_set_l2_bridge, OPT_L2_BRIDGE,
                        result);
}

/**
 * Parse ageing-time option.
 */
static lagopus_result_t
mactable_ageing_time_opt_parse(const char *const *argv[],
                    void *c, void *out_configs,
                    lagopus_dstring_t *result) {
  return uint_opt_parse(argv,
                        (bridge_conf_t *)c,
                        (configs_t *) out_configs,
                        &bridge_set_mactable_ageing_time,
                        OPT_MACTABLE_AGEING_TIME,
                        CMD_UINT64,
                        result);
}

/**
 * Parse max-entries option.
 */
static lagopus_result_t
mactable_max_entries_opt_parse(const char *const *argv[],
                    void *c, void *out_configs,
                    lagopus_dstring_t *result) {
  return uint_opt_parse(argv,
                        (bridge_conf_t *)c,
                        (configs_t *) out_configs,
                        &bridge_set_mactable_max_entries,
                        OPT_MACTABLE_MAX_ENTRIES,
                        CMD_UINT64,
                        result);
}

/**
 * Parse mac entry options.
 */
static lagopus_result_t
mactable_entry_opt_parse(const char *const *argv[],
               void *c, void *out_configs,
               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_conf_t *conf = NULL;
  configs_t *configs = NULL;
  mac_address_t addr;
  union cmd_uint cmd_uint;
  bool is_added = false;
  char *mac = NULL;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (bridge_conf_t *) c;
    configs = (configs_t *) out_configs;

    /* command parameters.
       *(*argv) : attribute.
       *(*argv + 1) : argument of attribute.
     */
    if (*(*argv + 1) != NULL) {
      (*argv)++;

      /* Invalid arguremnt for lagopus config. */
      if (IS_VALID_STRING(*(*argv)) == false) {
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

        goto done;
      }


      if ((ret = cmd_opt_macaddr_get(*(*argv), &mac, &is_added))
           != LAGOPUS_RESULT_OK ||
          (ret = cmd_mac_addr_parse(mac, addr)) != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Invalid mac address = %s.",
                                                mac);
        goto done;
      }

      if (is_added == true) {
        /* add or modify */

        if (*(*argv + 1) == NULL) {
          /* no port number. */
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt value.");
          goto done;
        }

        if (IS_VALID_OPT(*(*argv + 1)) == true) {
          /* (argv + 1) equals option string(-XXX). */
          ret = datastore_json_result_string_setf(result, LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt value = %s.",
                                                  *(*argv + 1));
          goto done;
        }

        if ((ret = cmd_uint_parse(*(++(*argv)), CMD_UINT32, &cmd_uint)) !=
            LAGOPUS_RESULT_OK) {
          /* invalid value. */
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Bad opt value = %s.",
                                                  *(*argv));
          goto done;
        }

        if (cmd_uint.uint32 == 0) {
          /* not support 0. */
          ret = datastore_json_result_string_setf(result,
                                LAGOPUS_RESULT_OUT_OF_RANGE,
                                "port number = %"PRIu32".", cmd_uint.uint32);
          goto done;
        }

        /* set entry to mactable list */
        ret = bridge_mactable_add_entries(
                conf->modified_attr->mactable_entries,
                addr, cmd_uint.uint32);

        if (ret != LAGOPUS_RESULT_OK &&
            ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't add port_number.");
        }
      } else {
        /* delete */
        ret = bridge_mactable_remove_entry(
                conf->modified_attr->mactable_entries,
                addr);
      }

    } else if (configs->is_config == true) {
      configs->flags = OPT_BIT_GET(OPT_MACTABLE_ENTRY);
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Bad opt value.");
    }
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS, NULL);
  }

done:
  return ret;
}
#endif /* HYBRID */

static lagopus_result_t
create_sub_cmd_parse_internal(datastore_interp_t *iptr,
                              datastore_interp_state_t state,
                              size_t argc, const char *const argv[],
                              char *name,
                              datastore_update_proc_t proc,
                              configs_t *out_configs,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_conf_t *conf = NULL;
  (void) argc;
  (void) proc;

  ret = bridge_conf_create(&conf, name);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = bridge_conf_add(conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = opt_parse(argv, conf, out_configs, result);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = bridge_cmd_update_internal(iptr, state, conf,
                                         true, false, result);

        if (ret != LAGOPUS_RESULT_OK &&
            ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_set(result, ret, NULL);
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add bridge_conf.");
    }

    if (ret != LAGOPUS_RESULT_OK) {
      (void) bridge_conf_delete(conf);
      goto done;
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't create bridge_conf.");
  }

  if (ret != LAGOPUS_RESULT_OK && conf != NULL) {
    bridge_conf_destroy(conf);
  }

done:
  return ret;
}

static lagopus_result_t
config_sub_cmd_parse_internal(datastore_interp_t *iptr,
                              datastore_interp_state_t state,
                              size_t argc, const char *const argv[],
                              bridge_conf_t *conf,
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
      ret = bridge_attr_duplicate(conf->current_attr,
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
      ret = bridge_cmd_update_internal(iptr, state, conf,
                                       true, false, result);

      if (ret != LAGOPUS_RESULT_OK &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_set(result, ret, NULL);
      }
    } else {
      /* show. */
      ret = bridge_conf_one_list(&out_configs->list, conf);

      if (ret >= 0) {
        out_configs->size = (size_t) ret;
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = datastore_json_result_string_setf(
                result, ret,
                "Can't create list of bridge_conf.");
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
  bridge_conf_t *conf = NULL;
  char *namespace = NULL;
  (void) hptr;
  (void) proc;

  if (name != NULL) {
    ret = bridge_find(name, &conf);

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
  bridge_conf_t *conf = NULL;
  (void) hptr;
  (void) argc;
  (void) proc;

  if (name != NULL) {
    ret = bridge_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = config_sub_cmd_parse_internal(iptr, state, argc, argv,
                                          conf, proc, out_configs, result);

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
                              bridge_conf_t *conf,
                              bool is_propagation,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && conf != NULL && result != NULL) {
    if (conf->is_enabled == false) {
      if (state == DATASTORE_INTERP_STATE_ATOMIC) {
        conf->is_enabling = true;
        conf->is_disabling = false;
        ret = LAGOPUS_RESULT_OK;
      } else {
        conf->is_enabled = true;
        ret = bridge_cmd_update_internal(iptr, state, conf,
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
  bridge_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = bridge_find(name, &conf);

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
                               bridge_conf_t *conf,
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
      ret = bridge_cmd_update_internal(iptr, state, conf,
                                       is_propagation, true, result);

      if (ret == LAGOPUS_RESULT_OK) {
        /* disable propagation. */
        if (is_propagation == true) {
          ret = bri_names_disable(conf->name, conf->current_attr,
                                  NULL, NULL,
                                  iptr, state, true, true, result);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_set(result, ret, NULL);
          }
        }
      } else {
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
  bridge_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = bridge_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      ret = disable_sub_cmd_parse_internal(iptr, state,
                                           conf, false,
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

static lagopus_result_t
destroy_sub_cmd_parse_internal(datastore_interp_t *iptr,
                               datastore_interp_state_t state,
                               bridge_conf_t *conf,
                               bool is_propagation,
                               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && conf != NULL && result != NULL) {
    if (state == DATASTORE_INTERP_STATE_ATOMIC) {
      conf->is_destroying = true;
      conf->is_enabling = false;
      conf->is_disabling = true;
    } else {
      /* disable ports, controllers. */
      ret = bri_names_disable(conf->name, conf->current_attr,
                              NULL, NULL,
                              iptr, state, true, true, result);
      if (ret != LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }

      if (conf->is_enabled == true) {
        conf->is_enabled = false;
        ret = bridge_cmd_update_internal(iptr, state, conf,
                                         is_propagation, true, result);

        if (ret != LAGOPUS_RESULT_OK) {
          conf->is_enabled = true;
          if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
            ret = datastore_json_result_set(result, ret, NULL);
          }
          goto done;
        }
      }
      bridge_cmd_do_destroy(conf, state, result);
    }
    ret = LAGOPUS_RESULT_OK;
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
  bridge_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = bridge_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      ret = destroy_sub_cmd_parse_internal(iptr, state,
                                           conf, true, result);
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
  bridge_conf_t *conf = NULL;
  configs_t *configs = NULL;
  uint64_t dpid;
  (void) iptr;
  (void) state;
  (void) argc;
  (void) hptr;
  (void) proc;

  if (argv != NULL && name != NULL &&
      out_configs != NULL && result != NULL) {
    configs = (configs_t *) out_configs;
    ret = bridge_find(name, &conf);
    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      if (*(argv + 1) == NULL) {
        configs->is_show_stats = true;
        ret = dp_bridge_stats_get(conf->name, &configs->stats);
        if (ret == LAGOPUS_RESULT_OK) {
          if (conf->current_attr != NULL) {
            if ((ret = bridge_get_dpid(conf->current_attr,
                                       &dpid)) ==
                LAGOPUS_RESULT_OK) {
              ret = ofp_bridgeq_mgr_stats_get(dpid, &configs->stats);
            }
          }

          if (ret == LAGOPUS_RESULT_OK) {
            ret = bridge_conf_one_list(&configs->list, conf);

            if (ret >= 0) {
              configs->size = (size_t) ret;
              ret = LAGOPUS_RESULT_OK;
            } else {
              ret = datastore_json_result_string_setf(
                  result, ret,
                  "Can't create list of bridge_conf.");
            }
          } else {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't get stats.");
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get stats.");
        }
      } else {
        ret = datastore_json_result_string_setf(
            result,
            LAGOPUS_RESULT_INVALID_ARGS,
            "Bad opt = %s.", *(argv + 1));
        goto done;
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

static lagopus_result_t
clear_queue_sub_cmd_parse(datastore_interp_t *iptr,
                          datastore_interp_state_t state,
                          size_t argc, const char *const argv[],
                          char *name,
                          lagopus_hashmap_t *hptr,
                          datastore_update_proc_t proc,
                          void *out_configs,
                          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_conf_t *conf = NULL;
  (void) iptr;
  (void) state;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = bridge_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      ret = clear_queue_opt_parse(argv,
                                  conf,
                                  out_configs,
                                  result);
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

static inline lagopus_result_t
show_parse(const char *name,
           configs_t *out_configs,
           bool is_show_modified,
           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_conf_t *conf = NULL;
  char *target = NULL;

  if (name == NULL) {
    ret = namespace_get_current_namespace(&target);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = bridge_conf_list(&out_configs->list, target);
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
      ret = bridge_conf_list(&out_configs->list, "");
    } else if (ret == NS_NAMESPACE) {
      /* namespace + delim */
      ret = bridge_conf_list(&out_configs->list, target);
    } else if (ret == NS_FULLNAME) {
      /* namespace + name or delim + name */
      ret = bridge_find(target, &conf);

      if (ret == LAGOPUS_RESULT_OK) {
        if (conf->is_destroying == false) {
          ret = bridge_conf_one_list(&out_configs->list, conf);
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
            "Can't create list of bridge_conf.");
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

static inline lagopus_result_t
names_show(lagopus_dstring_t *ds,
           const char *key,
           datastore_name_info_t *names,
           bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *name = NULL;
  char *name_str = NULL;
  size_t i = 0;

  if (key != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"[", key);
    if (ret == LAGOPUS_RESULT_OK) {
      TAILQ_FOREACH(name, &names->head, name_entries) {
        ret = datastore_json_string_escape(name->str, &name_str);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = lagopus_dstring_appendf(ds, DS_JSON_LIST_ITEM_FMT(i),
                                        name_str);
          if (ret == LAGOPUS_RESULT_OK) {
            i++;
          } else {
            goto done;
          }
        }
        free(name_str);
        name_str = NULL;
      }
    } else {
      goto done;
    }
    ret = lagopus_dstring_appendf(ds, "]");
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  free(name_str);

  return ret;
}

static inline lagopus_result_t
port_names_show(lagopus_dstring_t *ds,
                const char *key,
                datastore_name_info_t *names,
                bool delimiter,
                bool is_show_current) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *name = NULL;
  char *name_str = NULL;
  uint32_t port_no;
  bool is_first = true;

  if (key != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"{", key);
    if (ret == LAGOPUS_RESULT_OK) {
      TAILQ_FOREACH(name, &names->head, name_entries) {
        if (((ret = datastore_port_get_port_number(name->str, is_show_current,
                    &port_no)) ==
             LAGOPUS_RESULT_INVALID_OBJECT) &&
            is_show_current == false) {
          ret = datastore_port_get_port_number(name->str, true,
                                               &port_no);
        }

        if (ret == LAGOPUS_RESULT_OK) {
          ret = datastore_json_string_escape(name->str, &name_str);
          if (ret == LAGOPUS_RESULT_OK) {
            ret = datastore_json_uint32_append(ds, name_str,
                                               port_no, !(is_first));
            if (ret == LAGOPUS_RESULT_OK) {
              if (is_first == true) {
                is_first = false;
              }
            } else {
              goto done;
            }
          }
        } else {
          goto done;
        }
        free(name_str);
        name_str = NULL;
      }
    } else {
      goto done;
    }
    ret = lagopus_dstring_appendf(ds, "}");
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  free(name_str);

  return ret;
}

static inline lagopus_result_t
action_types_show(lagopus_dstring_t *ds,
                  const char *key,
                  uint64_t types,
                  bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *type_str = NULL;
  size_t count = 0;
  uint64_t i;

  if (key != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"[", key);
    if (ret == LAGOPUS_RESULT_OK) {
      for (i = ACTION_TYPE_MIN; i <= ACTION_TYPE_MAX; i++) {
        if (IS_BIT_SET(types, (uint64_t) OPT_BIT_GET(i)) == true) {
          ret = bridge_action_type_to_str((datastore_bridge_action_type_t) i,
                                          &type_str);
          if (ret == LAGOPUS_RESULT_OK) {
            ret = lagopus_dstring_appendf(ds, DS_JSON_LIST_ITEM_FMT(count),
                                          type_str);
            if (ret == LAGOPUS_RESULT_OK) {
              count++;
            } else {
              goto done;
            }
          } else {
            goto done;
          }
        }
        type_str = NULL;
      }
    } else {
      goto done;
    }
    ret = lagopus_dstring_appendf(ds, "]");
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

static inline lagopus_result_t
instruction_types_show(lagopus_dstring_t *ds,
                       const char *key,
                       uint64_t types,
                       bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *type_str = NULL;
  size_t count = 0;
  uint64_t i;

  if (key != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"[", key);
    if (ret == LAGOPUS_RESULT_OK) {
      for (i = INSTRUCTION_TYPE_MIN; i <= INSTRUCTION_TYPE_MAX; i++) {
        if (IS_BIT_SET(types, (uint64_t) OPT_BIT_GET(i)) == true) {
          ret = bridge_instruction_type_to_str(
                  (datastore_bridge_instruction_type_t) i,
                  &type_str);
          if (ret == LAGOPUS_RESULT_OK) {
            ret = lagopus_dstring_appendf(ds, DS_JSON_LIST_ITEM_FMT(count),
                                          type_str);
            if (ret == LAGOPUS_RESULT_OK) {
              count++;
            } else {
              goto done;
            }
          } else {
            goto done;
          }
        }
        type_str = NULL;
      }
    } else {
      goto done;
    }
    ret = lagopus_dstring_appendf(ds, "]");
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

static inline lagopus_result_t
reserved_port_types_show(lagopus_dstring_t *ds,
                         const char *key,
                         uint64_t types,
                         bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *type_str = NULL;
  size_t count = 0;
  uint64_t i;

  if (key != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"[", key);
    if (ret == LAGOPUS_RESULT_OK) {
      for (i = RESERVED_PORT_TYPE_MIN; i <= RESERVED_PORT_TYPE_MAX; i++) {
        if (IS_BIT_SET(types, (uint64_t) OPT_BIT_GET(i)) == true) {
          ret = bridge_reserved_port_type_to_str(
                  (datastore_bridge_reserved_port_type_t) i,
                  &type_str);
          if (ret == LAGOPUS_RESULT_OK) {
            ret = lagopus_dstring_appendf(ds, DS_JSON_LIST_ITEM_FMT(count),
                                          type_str);
            if (ret == LAGOPUS_RESULT_OK) {
              count++;
            } else {
              goto done;
            }
          } else {
            goto done;
          }
        }
        type_str = NULL;
      }
    } else {
      goto done;
    }
    ret = lagopus_dstring_appendf(ds, "]");
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

static inline lagopus_result_t
group_types_show(lagopus_dstring_t *ds,
                 const char *key,
                 uint64_t types,
                 bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *type_str = NULL;
  size_t count = 0;
  uint64_t i;

  if (key != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"[", key);
    if (ret == LAGOPUS_RESULT_OK) {
      for (i = GROUP_TYPE_MIN; i <= GROUP_TYPE_MAX; i++) {
        if (IS_BIT_SET(types, (uint64_t) OPT_BIT_GET(i)) == true) {
          ret = bridge_group_type_to_str((datastore_bridge_group_type_t) i,
                                         &type_str);
          if (ret == LAGOPUS_RESULT_OK) {
            ret = lagopus_dstring_appendf(ds, DS_JSON_LIST_ITEM_FMT(count),
                                          type_str);
            if (ret == LAGOPUS_RESULT_OK) {
              count++;
            } else {
              goto done;
            }
          } else {
            goto done;
          }
        }
        type_str = NULL;
      }
    } else {
      goto done;
    }
    ret = lagopus_dstring_appendf(ds, "]");
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

static inline lagopus_result_t
group_capabilities_show(lagopus_dstring_t *ds,
                        const char *key,
                        uint64_t types,
                        bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *type_str = NULL;
  size_t count = 0;
  uint64_t i;

  if (key != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"[", key);
    if (ret == LAGOPUS_RESULT_OK) {
      for (i = GROUP_CAPABILITY_MIN; i <= GROUP_CAPABILITY_MAX; i++) {
        if (IS_BIT_SET(types, (uint64_t) OPT_BIT_GET(i)) == true) {
          ret = bridge_group_capability_to_str(
                  (datastore_bridge_group_capability_t) i,
                  &type_str);
          if (ret == LAGOPUS_RESULT_OK) {
            ret = lagopus_dstring_appendf(ds, DS_JSON_LIST_ITEM_FMT(count),
                                          type_str);
            if (ret == LAGOPUS_RESULT_OK) {
              count++;
            } else {
              goto done;
            }
          } else {
            goto done;
          }
        }
        type_str = NULL;
      }
    } else {
      goto done;
    }
    ret = lagopus_dstring_appendf(ds, "]");
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

static lagopus_result_t
bridge_cmd_json_create(lagopus_dstring_t *ds,
                       configs_t *configs,
                       lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;
  bool b;
  uint32_t max_buffs;
  uint16_t max_ports;
  uint8_t max_tables;
  uint32_t max_flows;
#ifdef HYBRID
  bool l2_bridge;
  uint32_t mactable_ageing_time;
  uint32_t mactable_max_entries;
#endif /* HYBRID */
  uint16_t packet_inq_size;
  uint16_t packet_inq_max_batches;
  uint16_t up_streamq_size;
  uint16_t up_streamq_max_batches;
  uint16_t down_streamq_size;
  uint16_t down_streamq_max_batches;
  uint64_t types;
  datastore_name_info_t *controller_names = NULL;
  datastore_name_info_t *port_names = NULL;
  datastore_bridge_fail_mode_t fail_mode;
  const char *fail_mode_str = NULL;
  bridge_attr_t *attr = NULL;
  bool is_show_current = false;
  size_t i;

  ret = lagopus_dstring_appendf(ds, "[");
  if (ret == LAGOPUS_RESULT_OK) {
    for (i = 0; i < configs->size; i++) {
      if (configs->is_config == true) {
        /* config cmd. */
        if (configs->list[i]->modified_attr != NULL) {
          attr = configs->list[i]->modified_attr;
          is_show_current = false;
        } else {
          attr = configs->list[i]->current_attr;
          is_show_current = true;
        }
      } else {
        /* show cmd. */
        if (configs->is_show_modified == true) {
          if (configs->list[i]->modified_attr != NULL) {
            attr = configs->list[i]->modified_attr;
            is_show_current = false;
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
            is_show_current = true;
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

          /* controller names */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_CONTROLLERS)) == true) {
            if ((ret = bridge_get_controller_names(attr,
                                                   &controller_names)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = names_show(ds,
                                    ATTR_NAME_GET(opt_strs,
                                                  OPT_CONTROLLERS),
                                    controller_names, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* port names */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_PORTS)) == true) {
            if ((ret = bridge_get_port_names(attr,
                                             &port_names)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = port_names_show(ds,
                                         ATTR_NAME_GET(opt_strs,
                                             OPT_PORTS),
                                         port_names, true, is_show_current)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* dpid */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_DPID)) == true) {
            if ((ret = bridge_get_dpid(attr, &dpid)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(ds,
                                                      ATTR_NAME_GET(opt_strs,
                                                          OPT_DPID),
                                                      dpid, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* fail_mode */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_FAIL_MODE)) == true) {
            if ((ret = bridge_get_fail_mode(attr,
                                            &fail_mode)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = bridge_fail_mode_to_str(fail_mode, &fail_mode_str)) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = datastore_json_string_append(
                             ds, ATTR_NAME_GET(opt_strs, OPT_FAIL_MODE),
                             fail_mode_str, true)) !=
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

          /* flow statistics */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_FLOW_STATISTICS)) == true) {
            if ((ret = bridge_is_flow_statistics(attr,
                                                 &b)) == LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_bool_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_FLOW_STATISTICS),
                           b, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* group statistics */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_GROUP_STATISTICS)) == true) {
            if ((ret = bridge_is_group_statistics(attr,
                                                  &b)) == LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_bool_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_GROUP_STATISTICS),
                           b, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* port statistics */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_PORT_STATISTICS)) == true) {
            if ((ret = bridge_is_port_statistics(attr,
                                                 &b)) == LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_bool_append(ds,
                                                    ATTR_NAME_GET(opt_strs,
                                                        OPT_PORT_STATISTICS),
                                                    b, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* queue statistics */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_QUEUE_STATISTICS)) == true) {
            if ((ret = bridge_is_queue_statistics(attr,
                                                  &b)) == LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_bool_append(ds,
                                                    ATTR_NAME_GET(opt_strs,
                                                        OPT_QUEUE_STATISTICS),
                                                    b, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* table statistics */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_TABLE_STATISTICS)) == true) {
            if ((ret = bridge_is_table_statistics(attr,
                                                  &b)) == LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_bool_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_TABLE_STATISTICS),
                           b, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* reassemble ip fragments */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_REASSEMBLE_IP_FRAGMENTS)) == true) {
            if ((ret = bridge_is_reassemble_ip_fragments(
                         attr, &b)) == LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_bool_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_REASSEMBLE_IP_FRAGMENTS),
                           b, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* max buffered packets */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_MAX_BUFFERED_PACKETS)) == true) {
            if ((ret = bridge_get_max_buffered_packets(attr,
                       &max_buffs)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_MAX_BUFFERED_PACKETS),
                           max_buffs, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* max ports */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_MAX_PORTS)) == true) {
            if ((ret = bridge_get_max_ports(attr,
                                            &max_ports)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_MAX_PORTS),
                           max_ports, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* max tables */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_MAX_TABLES)) == true) {
            if ((ret = bridge_get_max_tables(attr,
                                             &max_tables)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_MAX_TABLES),
                           max_tables, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* max flows */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_MAX_FLOWS)) == true) {
            if ((ret = bridge_get_max_flows(attr,
                                            &max_flows)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_MAX_FLOWS),
                           max_flows, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

#ifdef HYBRID
          /* l2-bridge opt */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_L2_BRIDGE)) == true) {
            if ((ret = bridge_is_l2_bridge(attr, &l2_bridge)) ==
                 LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_bool_append(
                           ds,
                           ATTR_NAME_GET(opt_strs, OPT_L2_BRIDGE),
                           l2_bridge,
                           true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* mactable-ageing-time opt */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_MACTABLE_AGEING_TIME)) == true) {
            if ((ret =
                   bridge_get_mactable_ageing_time(attr,
                                                   &mactable_ageing_time)) ==
                 LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds,
                           ATTR_NAME_GET(opt_strs, OPT_MACTABLE_AGEING_TIME),
                           mactable_ageing_time,
                           true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* mactable-max-entries opt */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_MACTABLE_MAX_ENTRIES)) == true) {
            if ((ret =
                   bridge_get_mactable_max_entries(attr,
                                                   &mactable_max_entries)) ==
                 LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds,
                           ATTR_NAME_GET(opt_strs, OPT_MACTABLE_MAX_ENTRIES),
                           mactable_max_entries,
                           true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }
#endif /* HYBRID */

          /* packet-inq size */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_PACKET_INQ_SIZE)) == true) {
            if ((ret = bridge_get_packet_inq_size(attr,
                                                  &packet_inq_size)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_PACKET_INQ_SIZE),
                           packet_inq_size, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* packet-inq max batches */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_PACKET_INQ_MAX_BATCHES)) == true) {
            if ((ret = bridge_get_packet_inq_max_batches(attr,
                                                         &packet_inq_max_batches)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_PACKET_INQ_MAX_BATCHES),
                           packet_inq_max_batches, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* up streamq size */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_UP_STREAMQ_SIZE)) == true) {
            if ((ret = bridge_get_up_streamq_size(attr,
                                                  &up_streamq_size)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_UP_STREAMQ_SIZE),
                           up_streamq_size, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* up streamq max batches */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_UP_STREAMQ_MAX_BATCHES)) == true) {
            if ((ret = bridge_get_up_streamq_max_batches(attr,
                                                         &up_streamq_max_batches)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_UP_STREAMQ_MAX_BATCHES),
                           up_streamq_max_batches, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* down streamq size */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_DOWN_STREAMQ_SIZE)) == true) {
            if ((ret = bridge_get_down_streamq_size(attr,
                                                    &down_streamq_size)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_DOWN_STREAMQ_SIZE),
                           down_streamq_size, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* down streamq max batches */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_DOWN_STREAMQ_MAX_BATCHES)) == true) {
            if ((ret = bridge_get_down_streamq_max_batches(attr,
                                            &down_streamq_max_batches)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_DOWN_STREAMQ_MAX_BATCHES),
                           down_streamq_max_batches, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* block-looping-ports */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_BLOCK_LOOPING_PORTS)) == true) {
            if ((ret = bridge_is_block_looping_ports(
                         attr, &b)) == LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_bool_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_BLOCK_LOOPING_PORTS),
                           b, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* action types */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_ACTION_TYPES)) == true) {
            if ((ret = bridge_get_action_types(attr, &types)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = action_types_show(ds,
                                           ATTR_NAME_GET(opt_strs,
                                               OPT_ACTION_TYPES),
                                           types, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* instruction types */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_INSTRUCTION_TYPES)) == true) {
            if ((ret = bridge_get_instruction_types(attr, &types)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = instruction_types_show(ds,
                                                ATTR_NAME_GET(opt_strs,
                                                    OPT_INSTRUCTION_TYPES),
                                                types, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* reserved_port_types */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_RESERVED_PORT_TYPES)) == true) {
            if ((ret = bridge_get_reserved_port_types(attr, &types)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = reserved_port_types_show(ds,
                                                  ATTR_NAME_GET(
                                                    opt_strs,
                                                    OPT_RESERVED_PORT_TYPES),
                                                  types, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* group types */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_GROUP_TYPES)) == true) {
            if ((ret = bridge_get_group_types(attr, &types)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = group_types_show(ds,
                                          ATTR_NAME_GET(opt_strs,
                                              OPT_GROUP_TYPES),
                                          types, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* group_capabilities */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_GROUP_CAPABILITIES)) == true) {
            if ((ret = bridge_get_group_capabilities(attr, &types)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = group_capabilities_show(ds,
                                                 ATTR_NAME_GET(
                                                   opt_strs,
                                                   OPT_GROUP_CAPABILITIES),
                                                 types, true)) !=
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
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_IS_USED)) == true) {
            if ((ret = datastore_json_bool_append(
                         ds, ATTR_NAME_GET(opt_strs, OPT_IS_USED),
                         configs->list[i]->is_used, true)) !=
                LAGOPUS_RESULT_OK) {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* enabled */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_IS_ENABLED)) == true) {
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
      if (controller_names != NULL) {
        datastore_names_destroy(controller_names);
        controller_names = NULL;
      }
      if (port_names != NULL) {
        datastore_names_destroy(port_names);
        port_names = NULL;
      }

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

static inline lagopus_result_t
bridge_cmd_stats_table(lagopus_dstring_t *ds,
                       configs_t *configs) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct table_stats *table_stats = NULL;
  bool is_table_first = true;

  if (TAILQ_EMPTY(&configs->stats.flow_table_stats) == false) {
    TAILQ_FOREACH(table_stats, &configs->stats.flow_table_stats, entry) {
      if ((ret = lagopus_dstring_appendf(
              ds, DS_JSON_DELIMITER(is_table_first, "{"))) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      /* table_id */
      if ((ret = datastore_json_uint8_append(
              ds, ATTR_NAME_GET(stat_strs, STATS_TABLE_ID),
              table_stats->ofp.table_id, false)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      /* flow_entries */
      if ((ret = datastore_json_uint64_append(
              ds, ATTR_NAME_GET(stat_strs, STATS_FLOW_ENTRIES),
              table_stats->ofp.active_count, true)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      /* flow_lookup_count */
      if ((ret = datastore_json_uint64_append(
              ds, ATTR_NAME_GET(stat_strs, STATS_FLOW_LOOKUP_COUNT),
              table_stats->ofp.lookup_count, true)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      /* flow_matched_count */
      if ((ret = datastore_json_uint64_append(
              ds, ATTR_NAME_GET(stat_strs, STATS_FLOW_MATCHED_COUNT),
              table_stats->ofp.matched_count, true)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if ((ret = lagopus_dstring_appendf(
              ds, "}")) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }

      if (is_table_first == true) {
        is_table_first = false;
      }
    }
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

done:
  return ret;
}

static lagopus_result_t
bridge_cmd_stats_json_create(lagopus_dstring_t *ds,
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

        /* packet_inq_entries */
        if ((ret = datastore_json_uint16_append(
                ds, ATTR_NAME_GET(stat_strs, STATS_PACKET_INQ_ENTRIES),
                configs->stats.packet_inq_entries, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* up_streamq_entries */
        if ((ret = datastore_json_uint16_append(
                ds, ATTR_NAME_GET(stat_strs, STATS_UP_STREAMQ_ENTRIES),
                configs->stats.up_streamq_entries, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* down_streamq_entries */
        if ((ret = datastore_json_uint16_append(
                ds, ATTR_NAME_GET(stat_strs, STATS_DOWN_STREAMQ_ENTRIES),
                configs->stats.down_streamq_entries, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* flowcache_entries */
        if ((ret = datastore_json_uint64_append(
                ds, ATTR_NAME_GET(stat_strs, STATS_FLOWCACHE_ENTRIES),
                configs->stats.flowcache_entries, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* flowcache_hit */
        if ((ret = datastore_json_uint64_append(
                ds, ATTR_NAME_GET(stat_strs, STATS_FLOWCACHE_HIT),
                configs->stats.flowcache_hit, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* flowcache_miss */
        if ((ret = datastore_json_uint64_append(
                ds, ATTR_NAME_GET(stat_strs, STATS_FLOWCACHE_MISS),
                configs->stats.flowcache_miss, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* flow_entries */
        if ((ret = datastore_json_uint64_append(
                ds, ATTR_NAME_GET(stat_strs, STATS_FLOW_ENTRIES),
                configs->stats.flow_entries, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* flow_lookup_count */
        if ((ret = datastore_json_uint64_append(
                ds, ATTR_NAME_GET(stat_strs, STATS_FLOW_LOOKUP_COUNT),
                configs->stats.flow_lookup_count, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* flow_matched_count */
        if ((ret = datastore_json_uint64_append(
                ds, ATTR_NAME_GET(stat_strs, STATS_FLOW_MATCHED_COUNT),
                configs->stats.flow_matched_count, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* tables */
        if ((ret = lagopus_dstring_appendf(
                ds, DELIMITER_INSTERN(KEY_FMT "["),
                ATTR_NAME_GET(stat_strs, STATS_TABLES))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* table */
        if ((ret = bridge_cmd_stats_table(ds, configs)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        if ((ret = lagopus_dstring_appendf(
                ds, "]")) !=
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
bridge_cmd_parse(datastore_interp_t *iptr,
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
  configs_t out_configs = {0, 0LL, false, false, false,
                           {0LL, 0LL, 0LL, 0LL, 0LL, 0LL, 0LL, 0LL, 0LL, {0LL}},
                           NULL};
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
    TAILQ_INIT(&out_configs.stats.flow_table_stats);

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
            ret = bridge_cmd_stats_json_create(&conf_result, &out_configs,
                                               result);
          } else {
            ret = bridge_cmd_json_create(&conf_result, &out_configs,
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
    table_stats_list_elem_free(&out_configs.stats.flow_table_stats);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
bridge_cmd_enable(datastore_interp_t *iptr,
                  datastore_interp_state_t state,
                  void *obj,
                  bool *currentp,
                  bool enabled,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (currentp == NULL) {
    if (enabled == true) {
      ret = enable_sub_cmd_parse_internal(iptr, state,
                                          (bridge_conf_t *) obj,
                                          false,
                                          result);
    } else {
      ret = disable_sub_cmd_parse_internal(iptr, state,
                                           (bridge_conf_t *) obj,
                                           false,
                                           result);
    }
  } else {
    if (obj != NULL) {
      *currentp = ((bridge_conf_t *) obj)->is_enabled;
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
bridge_cmd_destroy(datastore_interp_t *iptr,
                   datastore_interp_state_t state,
                   void *obj,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = destroy_sub_cmd_parse_internal(iptr, state,
                                       (bridge_conf_t *) obj,
                                       false, result);

  return ret;
}

STATIC lagopus_result_t
bridge_cmd_serialize(datastore_interp_t *iptr,
                     datastore_interp_state_t state,
                     const void *obj,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_conf_t *conf = NULL;

  char *escaped_name = NULL;

  /* dpid */
  uint64_t dpid = 0;

  /* controller-names */
  datastore_name_info_t *controller_names = NULL;
  struct datastore_name_head *c_head = NULL;
  struct datastore_name_entry *c_entry = NULL;
  char *escaped_c_names_str = NULL;

  /* port-names */
  datastore_name_info_t *port_names = NULL;
  struct datastore_name_head *p_head = NULL;
  struct datastore_name_entry *p_entry = NULL;
  char *escaped_p_names_str = NULL;

  /* port-number. */
  uint32_t port_num = 0;

  /* fail-mode */
  datastore_bridge_fail_mode_t fail_mode = DATASTORE_BRIDGE_FAIL_MODE_UNKNOWN;
  const char *fail_mode_str = NULL;
  char *escaped_fail_mode_str = NULL;

  /* flow-statistics */
  bool flow_statistics = false;
  const char *flow_statistics_str = NULL;

  /* group-statistics */
  bool group_statistics = false;
  const char *group_statistics_str = NULL;

  /* port-statistics */
  bool port_statistics = false;
  const char *port_statistics_str = NULL;

  /* queue-statistics */
  bool queue_statistics = false;
  const char *queue_statistics_str = NULL;

  /* table-statistics */
  bool table_statistics = false;
  const char *table_statistics_str = NULL;

  /* reassemble-ip-statistics */
  bool reassemble_ip_fragments = false;
  const char *reassemble_ip_fragments_str = NULL;

  /* max-buffered-packets opt. */
  uint32_t max_buffered_packets = 0;

  /* max-ports opt. */
  uint16_t max_ports = 0;

  /* max-tables opt. */
  uint8_t max_tables = 0;

  /* max-flows opt. */
  uint32_t max_flows = 0;

  /* queue opts. */
  uint16_t packet_inq_size = 0;
  uint16_t packet_inq_max_batches = 0;
  uint16_t up_streamq_size = 0;
  uint16_t up_streamq_max_batches = 0;
  uint16_t down_streamq_size = 0;
  uint16_t down_streamq_max_batches = 0;

  /* block-looping-ports opt. */
  bool block_looping_ports = false;
  const char *block_looping_ports_str = NULL;

#ifdef HYBRID
  /* mactable */
  bool l2_bridge = false;
  uint32_t mactable_ageing_time = 0; /* mactable ageing time. */
  uint32_t mactable_max_entries = 0; /* mactable max entries. */
  bridge_mactable_info_t *mactable_entries = NULL;
  struct mactable_entry_head *me_head = NULL;
  struct bridge_mactable_entry *me_entry = NULL;
#endif /* HYBRID */

  /* action-types */
  /* instruction-types */
  /* reserved-port-types */
  /* group-types */
  /* group-capabilities */
  uint64_t types;
  const char *type_str = NULL;
  int i;

  bool is_escaped = false;

  (void) state;

  if (iptr != NULL && obj != NULL && result != NULL) {
    conf = (bridge_conf_t *) obj;

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

    /* dpid opt. */
    if ((ret = bridge_get_dpid(conf->current_attr,
                               &dpid)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_DPID])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %ld",
                                           dpid)) !=
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

    /* controller names */
    if ((ret = bridge_get_controller_names(conf->current_attr,
                                           &controller_names)) ==
        LAGOPUS_RESULT_OK) {
      c_head = &controller_names->head;
      if (TAILQ_EMPTY(c_head) == false) {
        TAILQ_FOREACH(c_entry, c_head, name_entries) {
          if ((ret = lagopus_dstring_appendf(result, " %s",
                                             opt_strs[OPT_CONTROLLER])) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_str_escape(c_entry->str, "\"",
                                          &is_escaped,
                                          &escaped_c_names_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(
                           result,
                           ESCAPE_NAME_FMT(is_escaped, escaped_c_names_str),
                           escaped_c_names_str)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }

            free((void *) escaped_c_names_str);
            escaped_c_names_str = NULL;
          } else {
            lagopus_perror(ret);
            goto done;
          }
        }
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* port names */
    if ((ret = bridge_get_port_names(conf->current_attr,
                                     &port_names)) ==
        LAGOPUS_RESULT_OK) {
      p_head = &port_names->head;
      if (TAILQ_EMPTY(p_head) == false) {
        TAILQ_FOREACH(p_entry, p_head, name_entries) {
          if ((ret = lagopus_dstring_appendf(result, " %s",
                                             opt_strs[OPT_PORT])) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_str_escape(p_entry->str, "\"",
                                          &is_escaped,
                                          &escaped_p_names_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(
                           result,
                           ESCAPE_NAME_FMT(is_escaped, escaped_p_names_str),
                           escaped_p_names_str)) == LAGOPUS_RESULT_OK) {

                if ((ret = datastore_port_get_port_number(escaped_p_names_str,
                                                          true, &port_num)) ==
                    LAGOPUS_RESULT_OK) {
                  if ((ret = lagopus_dstring_appendf(result,
                                                     " %d", port_num)) !=
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
            } else {
              lagopus_perror(ret);
              goto done;
            }

            free((void *) escaped_p_names_str);
            escaped_p_names_str = NULL;
          } else {
            lagopus_perror(ret);
            goto done;
          }
        }
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* fail-mode */
    if ((ret = bridge_get_fail_mode(conf->current_attr,
                                    &fail_mode)) ==
        LAGOPUS_RESULT_OK) {
      if (fail_mode != DATASTORE_BRIDGE_FAIL_MODE_UNKNOWN) {
        if ((ret = bridge_fail_mode_to_str(fail_mode,
                                           &fail_mode_str)) ==
            LAGOPUS_RESULT_OK) {

          if (IS_VALID_STRING(fail_mode_str) == true) {
            if ((ret = lagopus_str_escape(fail_mode_str, "\"",
                                          &is_escaped,
                                          &escaped_fail_mode_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, " %s",
                                                 opt_strs[OPT_FAIL_MODE])) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = lagopus_dstring_appendf(
                             result,
                             ESCAPE_NAME_FMT(is_escaped, escaped_fail_mode_str),
                             escaped_fail_mode_str)) !=
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

    /* flow-statistics opt. */
    if ((ret = bridge_is_flow_statistics(conf->current_attr,
                                         &flow_statistics)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_FLOW_STATISTICS])) ==
          LAGOPUS_RESULT_OK) {

        if (flow_statistics == true) {
          flow_statistics_str = "true";
        } else {
          flow_statistics_str = "false";
        }

        if ((ret = lagopus_dstring_appendf(result, " %s",
                                           flow_statistics_str)) !=
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

    /* group-statistics opt. */
    if ((ret = bridge_is_group_statistics(conf->current_attr,
                                          &group_statistics)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_GROUP_STATISTICS])) ==
          LAGOPUS_RESULT_OK) {

        if (group_statistics == true) {
          group_statistics_str = "true";
        } else {
          group_statistics_str = "false";
        }

        if ((ret = lagopus_dstring_appendf(result, " %s",
                                           group_statistics_str)) !=
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

    /* port-statistics opt. */
    if ((ret = bridge_is_port_statistics(conf->current_attr,
                                         &port_statistics)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_PORT_STATISTICS])) ==
          LAGOPUS_RESULT_OK) {

        if (port_statistics == true) {
          port_statistics_str = "true";
        } else {
          port_statistics_str = "false";
        }

        if ((ret = lagopus_dstring_appendf(result, " %s",
                                           port_statistics_str)) !=
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

    /* queue-statistics opt. */
    if ((ret = bridge_is_queue_statistics(conf->current_attr,
                                          &queue_statistics)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_QUEUE_STATISTICS])) ==
          LAGOPUS_RESULT_OK) {

        if (queue_statistics == true) {
          queue_statistics_str = "true";
        } else {
          queue_statistics_str = "false";
        }

        if ((ret = lagopus_dstring_appendf(result, " %s",
                                           queue_statistics_str)) !=
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

    /* table-statistics opt. */
    if ((ret = bridge_is_table_statistics(conf->current_attr,
                                          &table_statistics)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_TABLE_STATISTICS])) ==
          LAGOPUS_RESULT_OK) {

        if (table_statistics == true) {
          table_statistics_str = "true";
        } else {
          table_statistics_str = "false";
        }

        if ((ret = lagopus_dstring_appendf(result, " %s",
                                           table_statistics_str)) !=
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

    /* reassemble-ip-statistics opt. */
    if ((ret = bridge_is_reassemble_ip_fragments(conf->current_attr,
               &reassemble_ip_fragments)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_REASSEMBLE_IP_FRAGMENTS])) ==
          LAGOPUS_RESULT_OK) {

        if (reassemble_ip_fragments == true) {
          reassemble_ip_fragments_str = "true";
        } else {
          reassemble_ip_fragments_str = "false";
        }

        if ((ret = lagopus_dstring_appendf(result, " %s",
                                           reassemble_ip_fragments_str)) !=
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

    /* max-buffered-packets opt. */
    if ((ret = bridge_get_max_buffered_packets(conf->current_attr,
               &max_buffered_packets)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_MAX_BUFFERED_PACKETS])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %d",
                                           max_buffered_packets)) !=
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

    /* max-ports opt. */
    if ((ret = bridge_get_max_ports(conf->current_attr,
                                    &max_ports)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_MAX_PORTS])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %d",
                                           max_ports)) !=
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

    /* max-tables opt. */
    if ((ret = bridge_get_max_tables(conf->current_attr,
                                     &max_tables)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_MAX_TABLES])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %d",
                                           max_tables)) !=
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

    /* max-flows opt. */
    if ((ret = bridge_get_max_flows(conf->current_attr,
                                    &max_flows)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_MAX_FLOWS])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %"PRIu32,
                                           max_flows)) !=
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

#ifdef HYBRID
    /* l2-bridge opt */
    if ((ret = bridge_is_l2_bridge(conf->current_attr,
                                   &l2_bridge)) ==
         LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_L2_BRIDGE]))
           == LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %s",
                                   (l2_bridge == true) ? "true" : "false")) !=
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

    /* mactable-ageing-time opt */
    if ((ret = bridge_get_mactable_ageing_time(conf->current_attr,
                                               &mactable_ageing_time)) ==
         LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_MACTABLE_AGEING_TIME ]))
           == LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %"PRIu32,
                                           mactable_ageing_time)) !=
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

    /* mactable-max-entries opt */
    if ((ret = bridge_get_mactable_max_entries(conf->current_attr,
                                               &mactable_max_entries)) ==
         LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_MACTABLE_MAX_ENTRIES]))
          == LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %"PRIu32,
                                           mactable_max_entries)) !=
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

    /* mactable-entry opt */
    if ((ret = bridge_get_mactable_entry(conf->current_attr,
                                         &mactable_entries)) ==
        LAGOPUS_RESULT_OK) {
      me_head = &mactable_entries->head;
      if (TAILQ_EMPTY(me_head) == false) {
        TAILQ_FOREACH(me_entry, me_head, mactable_entries) {
          if ((ret = lagopus_dstring_appendf(result, " %s",
                                             opt_strs[OPT_MACTABLE_ENTRY]))
              == LAGOPUS_RESULT_OK) {
            for (i = 0; i < MAC_ADDR_STR_LEN; i++) {
              if (i == 0) {
                ret = lagopus_dstring_appendf(result, " %02x",
                                              me_entry->mac_address[i]);
              } else {
                ret = lagopus_dstring_appendf(result, ":%02x",
                                              me_entry->mac_address[i]);
              }
              if (ret != LAGOPUS_RESULT_OK) {
                goto done;
              }
            }

            if ((ret = lagopus_dstring_appendf(
                         result, " %d", me_entry->port_no))
                 == LAGOPUS_RESULT_OK) {
            } else {
              lagopus_perror(ret);
              goto done;
            }

          } else {
            lagopus_perror(ret);
            goto done;
          }
        }
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }
#endif /* HYBRID */

    /* packet-inq-size opt. */
    if ((ret = bridge_get_packet_inq_size(conf->current_attr,
                                          &packet_inq_size)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_PACKET_INQ_SIZE])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %"PRIu16,
                                           packet_inq_size)) !=
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

    /* packet-inq-max-batches opt. */
    if ((ret = bridge_get_packet_inq_max_batches(conf->current_attr,
                                    &packet_inq_max_batches)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_PACKET_INQ_MAX_BATCHES])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %"PRIu16,
                                           packet_inq_max_batches)) !=
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

    /* up-streamq-size opt. */
    if ((ret = bridge_get_up_streamq_size(conf->current_attr,
                                          &up_streamq_size)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_UP_STREAMQ_SIZE])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %"PRIu16,
                                           up_streamq_size)) !=
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

    /* up-streamq-max-batches opt. */
    if ((ret = bridge_get_up_streamq_max_batches(conf->current_attr,
                                                 &up_streamq_max_batches)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_UP_STREAMQ_MAX_BATCHES])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %"PRIu16,
                                           up_streamq_max_batches)) !=
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

    /* down-streamq-size opt. */
    if ((ret = bridge_get_down_streamq_size(conf->current_attr,
                                            &down_streamq_size)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_DOWN_STREAMQ_SIZE])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %"PRIu16,
                                           down_streamq_size)) !=
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

    /* down-streamq-max-batches opt. */
    if ((ret = bridge_get_down_streamq_max_batches(conf->current_attr,
                                                   &down_streamq_max_batches)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_DOWN_STREAMQ_MAX_BATCHES])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %"PRIu16,
                                           down_streamq_max_batches)) !=
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

    /* block-looping-ports opt. */
    if ((ret = bridge_is_block_looping_ports(conf->current_attr,
               &block_looping_ports)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_BLOCK_LOOPING_PORTS])) ==
          LAGOPUS_RESULT_OK) {

        if (block_looping_ports == true) {
          block_looping_ports_str = "true";
        } else {
          block_looping_ports_str = "false";
        }

        if ((ret = lagopus_dstring_appendf(result, " %s",
                                           block_looping_ports_str)) !=
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

    /* action-types opt. */
    if ((ret = bridge_get_action_types(conf->current_attr,
                                       &types)) ==
        LAGOPUS_RESULT_OK) {
      for (i = ACTION_TYPE_MIN; i <= ACTION_TYPE_MAX; i++) {
        if (IS_BIT_SET(types, (uint64_t) OPT_BIT_GET(i)) == false) {
          if ((ret = bridge_action_type_to_str(
                       (datastore_bridge_action_type_t) i, &type_str)) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(result, " %s",
                                               opt_strs[OPT_ACTION_TYPE])) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, " ~%s",
                                                 type_str)) !=
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
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* instruction-types opt. */
    if ((ret = bridge_get_instruction_types(conf->current_attr,
                                            &types)) ==
        LAGOPUS_RESULT_OK) {
      for (i = INSTRUCTION_TYPE_MIN; i <= INSTRUCTION_TYPE_MAX; i++) {
        if (IS_BIT_SET(types, (uint64_t) OPT_BIT_GET(i)) == false) {
          if ((ret = bridge_instruction_type_to_str(
                       (datastore_bridge_instruction_type_t) i, &type_str)) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(result, " %s",
                                               opt_strs[OPT_INSTRUCTION_TYPE])) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, " ~%s",
                                                 type_str)) !=
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
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* reserved-port-types_ opt. */
    if ((ret = bridge_get_reserved_port_types(conf->current_attr,
               &types)) ==
        LAGOPUS_RESULT_OK) {
      for (i = RESERVED_PORT_TYPE_MIN; i <= RESERVED_PORT_TYPE_MAX; i++) {
        if (IS_BIT_SET(types, (uint64_t) OPT_BIT_GET(i)) == false) {
          if ((ret = bridge_reserved_port_type_to_str(
                       (datastore_bridge_reserved_port_type_t) i, &type_str)) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(
                         result, " %s",
                         opt_strs[OPT_RESERVED_PORT_TYPE])) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, " ~%s",
                                                 type_str)) !=
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
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* group-types opt. */
    if ((ret = bridge_get_group_types(conf->current_attr,
                                      &types)) ==
        LAGOPUS_RESULT_OK) {
      for (i = GROUP_TYPE_MIN; i <= GROUP_TYPE_MAX; i++) {
        if (IS_BIT_SET(types, (uint64_t) OPT_BIT_GET(i)) == false) {
          if ((ret = bridge_group_type_to_str(
                       (datastore_bridge_group_type_t) i, &type_str)) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(result, " %s",
                                               opt_strs[OPT_GROUP_TYPE])) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, " ~%s",
                                                 type_str)) !=
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
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* group-capabilities opt. */
    if ((ret = bridge_get_group_capabilities(conf->current_attr,
               &types)) ==
        LAGOPUS_RESULT_OK) {
      for (i = GROUP_CAPABILITY_MIN; i <= GROUP_CAPABILITY_MAX; i++) {
        if (IS_BIT_SET(types, (uint64_t) OPT_BIT_GET(i)) == false) {
          if ((ret = bridge_group_capability_to_str(
                       (datastore_bridge_group_capability_t) i, &type_str)) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(result, " %s",
                                               opt_strs[OPT_GROUP_CAPABILITY])) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, " ~%s",
                                                 type_str)) !=
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
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* Add newline. */
    if ((ret = lagopus_dstring_appendf(result, "\n")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

  done:
    datastore_names_destroy(controller_names);
    datastore_names_destroy(port_names);
    free((void *) escaped_name);
    free((void *) escaped_fail_mode_str);
    free((void *) escaped_c_names_str);
    free((void *) escaped_p_names_str);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
bridge_cmd_compare(const void *obj1, const void *obj2, int *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj1 != NULL && obj2 != NULL && result != NULL) {
    *result = strcmp(((bridge_conf_t *) obj1)->name,
                     ((bridge_conf_t *) obj2)->name);
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
bridge_cmd_getname(const void *obj, const char **namep) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj != NULL && namep != NULL) {
    *namep = ((bridge_conf_t *)obj)->name;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
bridge_cmd_duplicate(const void *obj, const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_conf_t *dup_obj = NULL;

  if (obj != NULL) {
    ret = bridge_conf_duplicate(obj, &dup_obj, namespace);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = bridge_conf_add(dup_obj);

      if (ret != LAGOPUS_RESULT_OK && dup_obj != NULL) {
        bridge_conf_destroy(dup_obj);
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

  if ((ret = bridge_initialize()) != LAGOPUS_RESULT_OK) {
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
       LAGOPUS_RESULT_OK) ||
      ((ret = sub_cmd_add(CLEAR_QUEUE_SUB_CMD, clear_queue_sub_cmd_parse,
                          &sub_cmd_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  /* create hashmap for opts. */
  if ((ret = lagopus_hashmap_create(&opt_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if (((ret = opt_add(opt_strs[OPT_CONTROLLER], controller_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_PORT], port_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_DPID], dpid_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_FAIL_MODE], fail_mode_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_FLOW_STATISTICS], flow_statistics_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_GROUP_STATISTICS],
                      group_statistics_opt_parse, &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_PORT_STATISTICS], port_statistics_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_QUEUE_STATISTICS],
                      queue_statistics_opt_parse, &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_TABLE_STATISTICS],
                      table_statistics_opt_parse, &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_REASSEMBLE_IP_FRAGMENTS],
                      reassemble_ip_fragments_opt_parse, &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_MAX_BUFFERED_PACKETS],
                      max_buffered_packets_opt_parse, &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_MAX_PORTS], max_ports_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_MAX_TABLES], max_tables_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_MAX_FLOWS], max_flows_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_BLOCK_LOOPING_PORTS],
                      block_looping_ports_opt_parse, &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_ACTION_TYPE], action_type_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_INSTRUCTION_TYPE],
                      instruction_type_opt_parse, &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_RESERVED_PORT_TYPE],
                      reserved_port_type_opt_parse, &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_GROUP_TYPE],
                      group_type_opt_parse, &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_GROUP_CAPABILITY],
                      group_capability_opt_parse, &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_PACKET_INQ_SIZE],
                      packet_inq_size_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_PACKET_INQ_MAX_BATCHES],
                      packet_inq_max_batches_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
#ifdef HYBRID
      ((ret = opt_add(opt_strs[OPT_L2_BRIDGE],
                      l2_bridge_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_MACTABLE_AGEING_TIME],
                      mactable_ageing_time_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_MACTABLE_MAX_ENTRIES],
                      mactable_max_entries_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_MACTABLE_ENTRY],
                      mactable_entry_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
#endif /* HYBRID */
      ((ret = opt_add(opt_strs[OPT_UP_STREAMQ_SIZE],
                      up_streamq_size_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_UP_STREAMQ_MAX_BATCHES],
                      up_streamq_max_batches_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_DOWN_STREAMQ_SIZE],
                      down_streamq_size_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_DOWN_STREAMQ_MAX_BATCHES],
                      down_streamq_max_batches_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK)
  ) {
    goto done;
  }

  /* create hashmap for clear queue opt. */
  if ((ret = lagopus_hashmap_create(&opt_clear_queue_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if (((ret = opt_add(clear_q_opt_strs[OPT_CLEAR_Q_PACKET_INQ],
                      packet_inq_opt_parse,
                      &opt_clear_queue_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(clear_q_opt_strs[OPT_CLEAR_Q_UP_STREAMQ],
                      up_streamq_opt_parse,
                      &opt_clear_queue_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(clear_q_opt_strs[OPT_CLEAR_Q_DOWN_STREAMQ],
                      down_streamq_opt_parse,
                      &opt_clear_queue_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  if ((ret = datastore_register_table(CMD_NAME,
                                      &bridge_table,
                                      bridge_cmd_update,
                                      bridge_cmd_enable,
                                      bridge_cmd_serialize,
                                      bridge_cmd_destroy,
                                      bridge_cmd_compare,
                                      bridge_cmd_getname,
                                      bridge_cmd_duplicate)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = datastore_interp_register_command(&s_interp, CONFIGURATOR_NAME,
             CMD_NAME, bridge_cmd_parse)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* create hashmap for port. */
  if ((ret = lagopus_hashmap_create(&port_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  srandom(((unsigned int) time(NULL)));

done:
  return ret;
}




/*
 * Public:
 */

lagopus_result_t
bridge_cmd_initialize(void) {
  return initialize_internal();
}

void
bridge_cmd_finalize(void) {
  lagopus_hashmap_destroy(&sub_cmd_table, true);
  sub_cmd_table = NULL;
  lagopus_hashmap_destroy(&opt_table, true);
  opt_table = NULL;
  lagopus_hashmap_destroy(&opt_clear_queue_table, true);
  opt_clear_queue_table = NULL;
  lagopus_hashmap_destroy(&port_table, true);
  port_table = NULL;
  bridge_finalize();
}
