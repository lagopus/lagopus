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
 *      @file   bridge.c
 *      @brief  DSL for bridge management.
 */

#include "lagopus/datastore.h"
#include "datastore_internal.h"
#include "ns_util.h"
#include "bridge_cmd_internal.h"

#define MINIMUM_BUFFERED_PACKETS 0
#define MAXIMUM_BUFFERED_PACKETS UINT16_MAX
#define MINIMUM_PORTS 1
#define MAXIMUM_PORTS 2048
#define MINIMUM_TABLES 1
#define MAXIMUM_TABLES UINT8_MAX
#define MINIMUM_FLOWS 0
#define MAXIMUM_FLOWS UINT32_MAX
#define MINIMUM_PACKET_INQ_SIZE 0
#define MAXIMUM_PACKET_INQ_SIZE UINT16_MAX
#define MINIMUM_PACKET_INQ_MAX_BATCHES 0
#define MAXIMUM_PACKET_INQ_MAX_BATCHES UINT16_MAX
#define MINIMUM_UP_STREAMQ_SIZE 0
#define MAXIMUM_UP_STREAMQ_SIZE UINT16_MAX
#define MINIMUM_UP_STREAMQ_MAX_BATCHES 0
#define MAXIMUM_UP_STREAMQ_MAX_BATCHES UINT16_MAX
#define MINIMUM_DOWN_STREAMQ_SIZE 0
#define MAXIMUM_DOWN_STREAMQ_SIZE UINT16_MAX
#define MINIMUM_DOWN_STREAMQ_MAX_BATCHES 0
#define MAXIMUM_DOWN_STREAMQ_MAX_BATCHES UINT16_MAX
#ifdef HYBRID
/* ageing time for mactable. */
#define MINIMUM_MACTABLE_AGEING_TIME 10
#define MAXIMUM_MACTABLE_AGEING_TIME 1000000
#define DEFAULT_MACTABLE_AGEING_TIME 300
/* number of max entries for mactable. */
#define MINIMUM_MACTABLE_MAX_ENTRIES 10      /* TODO: */
#define MAXIMUM_MACTABLE_MAX_ENTRIES 65535   /* TODO: */
#define DEFAULT_MACTABLE_MAX_ENTRIES 8192
#endif /* HYBRID */

/* bridge attributes. */
typedef struct bridge_attr {
  uint64_t dpid;
  datastore_name_info_t *controller_names;
  datastore_name_info_t *port_names;
  datastore_bridge_fail_mode_t fail_mode;
  bool flow_statistics;
  bool group_statistics;
  bool port_statistics;
  bool queue_statistics;
  bool table_statistics;
  bool reassemble_ip_fragments;
  uint32_t max_buffered_packets;
  uint16_t max_ports;
  uint8_t max_tables;
  bool block_looping_ports;
  uint64_t action_types;
  uint64_t instruction_types;
  uint32_t max_flows;
  uint64_t reserved_port_types;
  uint64_t group_types;
  uint64_t group_capabilities;
  uint16_t packet_inq_size;
  uint16_t packet_inq_max_batches;
  uint16_t up_streamq_size;
  uint16_t up_streamq_max_batches;
  uint16_t down_streamq_size;
  uint16_t down_streamq_max_batches;
#ifdef HYBRID
  bool l2_bridge;
  uint32_t mactable_ageing_time;
  uint32_t mactable_max_entries;
  bridge_mactable_info_t *mactable_entries;
#endif /* HYBRID */
} bridge_attr_t;

typedef struct bridge_conf {
  const char *name;
  bridge_attr_t *current_attr;
  bridge_attr_t *modified_attr;
  bool is_used;
  bool is_enabled;
  bool is_destroying;
  bool is_enabling;
  bool is_disabling;
} bridge_conf_t;

typedef struct bridge_fail_mode {
  const datastore_bridge_fail_mode_t fail_mode;
  const char *fail_mode_str;
} bridge_fail_mode_t;

static const bridge_fail_mode_t fail_mode_arr[] = {
  {DATASTORE_BRIDGE_FAIL_MODE_UNKNOWN, "unknown"},
  {DATASTORE_BRIDGE_FAIL_MODE_SECURE, "secure"},
  {DATASTORE_BRIDGE_FAIL_MODE_STANDALONE, "standalone"}
};

typedef struct bridge_action_type {
  const datastore_bridge_action_type_t action_type;
  const char *action_type_str;
} bridge_action_type_t;

static const bridge_action_type_t action_type_arr[] = {
  {DATASTORE_BRIDGE_ACTION_TYPE_UNKNOWN, "unknown"},
  {DATASTORE_BRIDGE_ACTION_TYPE_COPY_TTL_OUT, "copy-ttl-out"},
  {DATASTORE_BRIDGE_ACTION_TYPE_COPY_TTL_IN, "copy-ttl-in"},
  {DATASTORE_BRIDGE_ACTION_TYPE_SET_MPLS_TTL, "set-mpls-ttl"},
  {DATASTORE_BRIDGE_ACTION_TYPE_DEC_MPLS_TTL, "dec-mpls-ttl"},
  {DATASTORE_BRIDGE_ACTION_TYPE_PUSH_VLAN, "push-vlan"},
  {DATASTORE_BRIDGE_ACTION_TYPE_POP_VLAN, "pop-vlan"},
  {DATASTORE_BRIDGE_ACTION_TYPE_PUSH_MPLS, "push-mpls"},
  {DATASTORE_BRIDGE_ACTION_TYPE_POP_MPLS, "pop-mpls"},
  {DATASTORE_BRIDGE_ACTION_TYPE_SET_QUEUE, "set-queue"},
  {DATASTORE_BRIDGE_ACTION_TYPE_GROUP, "group"},
  {DATASTORE_BRIDGE_ACTION_TYPE_SET_NW_TTL, "set-nw-ttl"},
  {DATASTORE_BRIDGE_ACTION_TYPE_DEC_NW_TTL, "dec-nw-ttl"},
  {DATASTORE_BRIDGE_ACTION_TYPE_SET_FIELD, "set-field"}
};

typedef struct bridge_instruction_type {
  const datastore_bridge_instruction_type_t instruction_type;
  const char *instruction_type_str;
} bridge_instruction_type_t;

static const bridge_instruction_type_t instruction_type_arr[] = {
  {DATASTORE_BRIDGE_INSTRUCTION_TYPE_UNKNOWN, "unknown"},
  {DATASTORE_BRIDGE_INSTRUCTION_TYPE_APPLY_ACTIONS, "apply-actions"},
  {DATASTORE_BRIDGE_INSTRUCTION_TYPE_CLEAR_ACTIONS, "clear-actions"},
  {DATASTORE_BRIDGE_INSTRUCTION_TYPE_WRITE_ACTIONS, "write-actions"},
  {DATASTORE_BRIDGE_INSTRUCTION_TYPE_WRITE_METADATA, "write-metadata"},
  {DATASTORE_BRIDGE_INSTRUCTION_TYPE_GOTO_TABLE, "goto-table"}
};

typedef struct bridge_reserved_port_type {
  const datastore_bridge_reserved_port_type_t reserved_port_type;
  const char *reserved_port_type_str;
} bridge_reserved_port_type_t;

static const bridge_reserved_port_type_t reserved_port_type_arr[] = {
  {DATASTORE_BRIDGE_RESERVED_PORT_TYPE_UNKNOWN, "unknown"},
  {DATASTORE_BRIDGE_RESERVED_PORT_TYPE_ALL, "all"},
  {DATASTORE_BRIDGE_RESERVED_PORT_TYPE_CONTROLLER, "controller"},
  {DATASTORE_BRIDGE_RESERVED_PORT_TYPE_TABLE, "table"},
  {DATASTORE_BRIDGE_RESERVED_PORT_TYPE_INPORT, "inport"},
  {DATASTORE_BRIDGE_RESERVED_PORT_TYPE_ANY, "any"},
  {DATASTORE_BRIDGE_RESERVED_PORT_TYPE_NORMAL, "normal"},
  {DATASTORE_BRIDGE_RESERVED_PORT_TYPE_FLOOD, "flood"}
};

typedef struct bridge_group_type {
  const datastore_bridge_group_type_t group_type;
  const char *group_type_str;
} bridge_group_type_t;

static const bridge_group_type_t group_type_arr[] = {
  {DATASTORE_BRIDGE_GROUP_TYPE_UNKNOWN, "unknown"},
  {DATASTORE_BRIDGE_GROUP_TYPE_ALL, "all"},
  {DATASTORE_BRIDGE_GROUP_TYPE_SELECT, "select"},
  {DATASTORE_BRIDGE_GROUP_TYPE_INDIRECT, "indirect"},
  {DATASTORE_BRIDGE_GROUP_TYPE_FAST_FAILOVER, "fast-failover"}
};

typedef struct bridge_group_capability {
  const datastore_bridge_group_capability_t group_capability;
  const char *group_capability_str;
} bridge_group_capability_t;

static const bridge_group_capability_t group_capability_arr[] = {
  {DATASTORE_BRIDGE_GROUP_CAPABILITY_UNKNOWN, "unknown"},
  {DATASTORE_BRIDGE_GROUP_CAPABILITY_SELECT_WEIGHT, "select-weight"},
  {DATASTORE_BRIDGE_GROUP_CAPABILITY_SELECT_LIVENESS, "select-liveness"},
  {DATASTORE_BRIDGE_GROUP_CAPABILITY_CHAINING, "chaining"},
  {DATASTORE_BRIDGE_GROUP_CAPABILITY_CHAINING_CHECK, "chaining-check"}
};

typedef struct {
  lagopus_result_t rc;
  uint64_t dpid;
  char *name;
  bool is_exists;
} bridge_dpid_is_exists_iter_ctx_t;

static lagopus_hashmap_t bridge_table = NULL;

#ifdef HYBRID
/* mactable entry list utilities */
/**
 * Destory mac entry in mac table list.
 */
lagopus_result_t
bridge_mactable_entry_destroy(bridge_mactable_info_t *entries) {
  struct bridge_mactable_entry *entry = NULL;

  if (entries == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  while ((entry = TAILQ_FIRST(&(entries->head))) != NULL) {
    TAILQ_REMOVE(&(entries->head), entry, mactable_entries);
    free(entry);
  }
  entries->size = 0;

  free((void *) entries);

  return LAGOPUS_RESULT_OK;
}

/**
 * Create mac entry list.
 */
lagopus_result_t
bridge_mactable_entry_create(bridge_mactable_info_t **entries) {
  if (entries== NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*entries != NULL) {
    bridge_mactable_entry_destroy(*entries);
    *entries = NULL;
  }

  if ((*entries =
       (bridge_mactable_info_t *) malloc(sizeof(bridge_mactable_info_t)))
       == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  (*entries)->size = 0;
  TAILQ_INIT(&((*entries)->head));

  return LAGOPUS_RESULT_OK;
}

/**
 * Duplicate mac entry for mac entry list.
 */
lagopus_result_t
bridge_mactable_entry_duplicate(const bridge_mactable_info_t *src_entries,
                          bridge_mactable_info_t **dst_entries) {
  struct bridge_mactable_entry *src_entry = NULL;
  struct bridge_mactable_entry *dst_entry = NULL;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (src_entries == NULL || dst_entries == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*dst_entries != NULL) {
    bridge_mactable_entry_destroy(*dst_entries);
    *dst_entries = NULL;
  }

  *dst_entries =
     (bridge_mactable_info_t *) malloc(sizeof(bridge_mactable_info_t));
  if (*dst_entries == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  memset(*dst_entries, 0, sizeof(bridge_mactable_info_t));

  TAILQ_INIT(&((*dst_entries)->head));

  TAILQ_FOREACH(src_entry, &(src_entries->head), mactable_entries) {
    dst_entry =
      (struct bridge_mactable_entry *)
       malloc(sizeof(struct bridge_mactable_entry));
    if (dst_entry == NULL) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      goto error;
    }
    TAILQ_INSERT_TAIL(&((*dst_entries)->head), dst_entry, mactable_entries);
    ret = copy_mac_address(src_entry->mac_address, dst_entry->mac_address);
    if (ret != LAGOPUS_RESULT_OK) {
      goto error;
    }
    dst_entry->port_no = src_entry->port_no;
    (*dst_entries)->size++;
  }

  return LAGOPUS_RESULT_OK;

error:
  bridge_mactable_entry_destroy(*dst_entries);
  *dst_entries = NULL;
  return ret;
}

/**
 * Check if mac entry is exist in mac entry list.
 */
bool
bridge_mactable_entry_exists(const bridge_mactable_info_t *entries,
                             const struct bridge_mactable_entry *in) {
  struct bridge_mactable_entry *entry = NULL;

  if (entries != NULL && in != NULL) {
    TAILQ_FOREACH(entry, &(entries->head), mactable_entries) {
      if (equals_mac_address(entry->mac_address, in->mac_address) == true
          && entry->port_no == in->port_no) {
        return true;
      }
    }
  }

  return false;
}

/**
 * Check if mac entries are same.
 */
bool
bridge_mactable_entry_equals(const bridge_mactable_info_t *entries0,
                       const bridge_mactable_info_t *entries1) {
  struct bridge_mactable_entry *entry = NULL;

  if (entries0 == NULL || entries1 == NULL) {
    return false;
  }

  if (entries0->size != entries1->size) {
    return false;
  }

  TAILQ_FOREACH(entry, &(entries0->head), mactable_entries) {
    if (bridge_mactable_entry_exists(entries1, entry) == false) {
      return false;
    }
  }

  return true;
}

/**
 * Add new mac entry to mac entry list.
 */
lagopus_result_t
bridge_mactable_add_entries(bridge_mactable_info_t *entries,
                            const mac_address_t addr,
                            const uint32_t port_no) {
  struct bridge_mactable_entry *e = NULL;
  struct bridge_mactable_entry *entry = NULL;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (entries == NULL || addr == NULL || port_no <= 0) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  /* check entry list. */
  TAILQ_FOREACH(entry, &(entries->head), mactable_entries) {
    /* mac address the same information */
    if (equals_mac_address(entry->mac_address, addr) == true) {
      TAILQ_REMOVE(&(entries->head), entry, mactable_entries);
      free(entry);
      entries->size--;
      break;
    }
  }

  /* add new entry. */
  e = malloc(sizeof(struct bridge_mactable_entry));
  if (e == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  ret = copy_mac_address(addr, e->mac_address);
  if (ret != LAGOPUS_RESULT_OK) {
    return ret;
  }
  e->port_no = port_no;

  TAILQ_INSERT_TAIL(&(entries->head), e, mactable_entries);
  entries->size++;

  return LAGOPUS_RESULT_OK;
}

/**
 * Remove mac entry from mac entry list.
 */
lagopus_result_t
bridge_mactable_remove_entry(bridge_mactable_info_t *entries,
                             const mac_address_t addr) {
  struct bridge_mactable_entry *entry = NULL;

  if (entries == NULL || addr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  TAILQ_FOREACH(entry, &(entries->head), mactable_entries) {
    /* mac address the same information */
    if (equals_mac_address(entry->mac_address, addr) == true) {
      TAILQ_REMOVE(&(entries->head), entry, mactable_entries);
      free(entry);

      entries->size--;

      break;
    }
  }

  return LAGOPUS_RESULT_OK;
}

/**
 * Remove all entries from mac entry list.
 */
lagopus_result_t
bridge_mactable_remove_all_entries(bridge_mactable_info_t *entries) {
  struct bridge_mactable_entry *entry = NULL;

  if (entries == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  while ((entry = TAILQ_FIRST(&(entries->head))) != NULL) {
    TAILQ_REMOVE(&(entries->head), entry, mactable_entries);
    free(entry);
  }
  entries->size = 0;

  return LAGOPUS_RESULT_OK;
}

#endif /* HYBRID */

/**
 * Destroy attributes of bridge.
 */
static inline void
bridge_attr_destroy(bridge_attr_t *attr) {
  if (attr != NULL) {
    datastore_names_destroy(attr->controller_names);
    datastore_names_destroy(attr->port_names);
#ifdef HYBRID
    bridge_mactable_entry_destroy(attr->mactable_entries);
#endif /* HYBRID */
    free((void *) attr);
  }
}

/**
 * Create attributes of bridge.
 */
static inline lagopus_result_t
bridge_attr_create(bridge_attr_t **attr) {
  lagopus_result_t rc;

  if (attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*attr != NULL) {
    bridge_attr_destroy(*attr);
    *attr = NULL;
  }

  if ((*attr = (bridge_attr_t *) malloc(sizeof(bridge_attr_t)))
      == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  (*attr)->dpid = 0;
  (*attr)->controller_names = NULL;
  (*attr)->port_names = NULL;
#ifdef HYBRID
  (*attr)->mactable_entries = NULL;
#endif /* HYBRID */
  rc = datastore_names_create(&((*attr)->controller_names));
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
  rc = datastore_names_create(&((*attr)->port_names));
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
#ifdef HYBRID
  /* initialize for mactable */
  rc = bridge_mactable_entry_create(&((*attr)->mactable_entries));
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
#endif /* HYBRID */
  (*attr)->fail_mode = DATASTORE_BRIDGE_FAIL_MODE_UNKNOWN;
  (*attr)->flow_statistics = true;
  (*attr)->group_statistics = true;
  (*attr)->port_statistics = true;
  (*attr)->queue_statistics = true;
  (*attr)->table_statistics = true;
  (*attr)->reassemble_ip_fragments = false;
  (*attr)->max_buffered_packets = 65535;
  (*attr)->max_ports = 255;
  (*attr)->max_tables = 255;
  (*attr)->block_looping_ports = false;
  (*attr)->action_types = UINT64_MAX;
  (*attr)->instruction_types = UINT64_MAX;
  (*attr)->max_flows = MAXIMUM_FLOWS;
  (*attr)->reserved_port_types = UINT64_MAX;
  (*attr)->group_types = UINT64_MAX;
  (*attr)->group_capabilities = UINT64_MAX;
  (*attr)->packet_inq_size = 1000;
  (*attr)->packet_inq_max_batches = 1000;
  (*attr)->up_streamq_size = 1000;
  (*attr)->up_streamq_max_batches = 1000;
  (*attr)->down_streamq_size = 1000;
  (*attr)->down_streamq_max_batches = 1000;

#ifdef HYBRID
  /* initialize for mactable */
  (*attr)->l2_bridge = false;
  (*attr)->mactable_ageing_time = DEFAULT_MACTABLE_AGEING_TIME;
  (*attr)->mactable_max_entries = DEFAULT_MACTABLE_MAX_ENTRIES;
#endif /* HYBRID */

  return LAGOPUS_RESULT_OK;

error:
  datastore_names_destroy((*attr)->controller_names);
  datastore_names_destroy((*attr)->port_names);
#ifdef HYBRID
  if ((*attr)->mactable_entries == NULL) {
    bridge_mactable_entry_destroy((*attr)->mactable_entries);
  }
#endif /* HYBRID */
  free((void *) *attr);
  *attr = NULL;
  return rc;
}

/**
 * Duplicate attirbutes.
 */
static inline lagopus_result_t
bridge_attr_duplicate(const bridge_attr_t *src_attr,
                      bridge_attr_t **dst_attr,
                      const char *namespace) {
  lagopus_result_t rc;
  char *buf = NULL;

  if (src_attr == NULL || dst_attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*dst_attr != NULL) {
    bridge_attr_destroy(*dst_attr);
    *dst_attr = NULL;
  }

  rc = bridge_attr_create(dst_attr);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }

  (*dst_attr)->dpid = src_attr->dpid;

  rc = datastore_names_duplicate(src_attr->controller_names,
                                 &((*dst_attr)->controller_names),
                                 namespace);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }

  rc = datastore_names_duplicate(src_attr->port_names,
                                 &((*dst_attr)->port_names),
                                 namespace);
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }

  (*dst_attr)->fail_mode = src_attr->fail_mode;
  (*dst_attr)->flow_statistics = src_attr->flow_statistics;
  (*dst_attr)->group_statistics = src_attr->group_statistics;
  (*dst_attr)->port_statistics = src_attr->port_statistics;
  (*dst_attr)->queue_statistics = src_attr->queue_statistics;
  (*dst_attr)->table_statistics = src_attr->table_statistics;
  (*dst_attr)->reassemble_ip_fragments = src_attr->reassemble_ip_fragments;
  (*dst_attr)->max_buffered_packets = src_attr->max_buffered_packets;
  (*dst_attr)->max_ports = src_attr->max_ports;
  (*dst_attr)->max_tables = src_attr->max_tables;
  (*dst_attr)->block_looping_ports = src_attr->block_looping_ports;
  (*dst_attr)->action_types = src_attr->action_types;
  (*dst_attr)->instruction_types = src_attr->instruction_types;
  (*dst_attr)->max_flows = src_attr->max_flows;
  (*dst_attr)->reserved_port_types = src_attr->reserved_port_types;
  (*dst_attr)->group_types = src_attr->group_types;
  (*dst_attr)->group_capabilities = src_attr->group_capabilities;
  (*dst_attr)->packet_inq_size = src_attr->packet_inq_size;
  (*dst_attr)->packet_inq_max_batches = src_attr->packet_inq_max_batches;
  (*dst_attr)->up_streamq_size = src_attr->up_streamq_size;
  (*dst_attr)->up_streamq_max_batches = src_attr->up_streamq_max_batches;
  (*dst_attr)->down_streamq_size = src_attr->down_streamq_size;
  (*dst_attr)->down_streamq_max_batches = src_attr->down_streamq_max_batches;

#ifdef HYBRID
  /* mactable */
  rc = bridge_mactable_entry_duplicate(src_attr->mactable_entries,
                                 &((*dst_attr)->mactable_entries));
  if (rc != LAGOPUS_RESULT_OK) {
    goto error;
  }
  (*dst_attr)->l2_bridge = src_attr->l2_bridge;
  (*dst_attr)->mactable_ageing_time = src_attr->mactable_ageing_time;
  (*dst_attr)->mactable_max_entries = src_attr->mactable_max_entries;
#endif /* HYBRID */

  return LAGOPUS_RESULT_OK;

error:
  free(buf);
  bridge_attr_destroy(*dst_attr);
  *dst_attr = NULL;
  return rc;
}

static inline bool
bridge_attr_controller_name_exists(const bridge_attr_t *attr,
                                   const char *name) {
  if (attr == NULL || name == NULL) {
    return false;
  }
  return datastore_name_exists(attr->controller_names, name);
}

static inline bool
bridge_attr_port_name_exists(const bridge_attr_t *attr, const char *name) {
  if (attr == NULL || name == NULL) {
    return false;
  }
  return datastore_name_exists(attr->port_names, name);
}

/**
 * Check whether there are changes in attributes.
 */
static inline bool
bridge_attr_equals(bridge_attr_t *attr0, bridge_attr_t *attr1) {
  if (attr0 == NULL && attr1 == NULL) {
    return true;
  } else if (attr0 == NULL || attr1 == NULL) {
    return false;
  }

  if ((datastore_names_equals(attr0->controller_names,
                                attr1->controller_names) == true) &&
      (datastore_names_equals(attr0->port_names,
                              attr1->port_names) == true) &&
      (attr0->dpid == attr1->dpid) &&
      (attr0->fail_mode == attr1->fail_mode) &&
      (attr0->flow_statistics == attr1->flow_statistics) &&
      (attr0->group_statistics == attr1->group_statistics) &&
      (attr0->port_statistics == attr1->port_statistics) &&
      (attr0->queue_statistics == attr1->queue_statistics) &&
      (attr0->table_statistics == attr1->table_statistics) &&
      (attr0->reassemble_ip_fragments == attr1->reassemble_ip_fragments) &&
      (attr0->max_buffered_packets == attr1->max_buffered_packets) &&
      (attr0->max_ports == attr1->max_ports) &&
      (attr0->max_tables == attr1->max_tables) &&
      (attr0->block_looping_ports == attr1->block_looping_ports) &&
      (attr0->action_types == attr1->action_types) &&
      (attr0->instruction_types == attr1->instruction_types) &&
      (attr0->max_flows == attr1->max_flows) &&
      (attr0->reserved_port_types == attr1->reserved_port_types) &&
      (attr0->group_types == attr1->group_types) &&
      (attr0->group_capabilities == attr1->group_capabilities) &&
      (attr0->packet_inq_size == attr1->packet_inq_size) &&
      (attr0->packet_inq_max_batches == attr1->packet_inq_max_batches) &&
#ifdef HYBRID
      (attr0->l2_bridge == attr1->l2_bridge) &&
      (bridge_mactable_entry_equals(attr0->mactable_entries,
                              attr1->mactable_entries) == true) &&
      (attr0->mactable_ageing_time == attr1->mactable_ageing_time) &&
      (attr0->mactable_max_entries == attr1->mactable_max_entries) &&
#endif /* HYBRID */
      (attr0->up_streamq_size == attr1->up_streamq_size) &&
      (attr0->up_streamq_max_batches == attr1->up_streamq_max_batches) &&
      (attr0->down_streamq_size == attr1->down_streamq_size) &&
      (attr0->down_streamq_max_batches == attr1->down_streamq_max_batches)
    ) {
    return true;
  }

  return false;
}

/**
 * Check whether there are changes in attributes that namespace is specified.
 */
static inline bool
bridge_attr_equals_without_names(bridge_attr_t *attr0, bridge_attr_t *attr1) {
  if (attr0 == NULL || attr1 == NULL) {
    return false;
  }

  if ((attr0->dpid == attr1->dpid) &&
      (attr0->fail_mode == attr1->fail_mode) &&
      (attr0->flow_statistics == attr1->flow_statistics) &&
      (attr0->group_statistics == attr1->group_statistics) &&
      (attr0->port_statistics == attr1->port_statistics) &&
      (attr0->queue_statistics == attr1->queue_statistics) &&
      (attr0->table_statistics == attr1->table_statistics) &&
      (attr0->reassemble_ip_fragments == attr1->reassemble_ip_fragments) &&
      (attr0->max_buffered_packets == attr1->max_buffered_packets) &&
      (attr0->max_ports == attr1->max_ports) &&
      (attr0->max_tables == attr1->max_tables) &&
      (attr0->block_looping_ports == attr1->block_looping_ports) &&
      (attr0->action_types == attr1->action_types) &&
      (attr0->instruction_types == attr1->instruction_types) &&
      (attr0->max_flows == attr1->max_flows) &&
      (attr0->reserved_port_types == attr1->reserved_port_types) &&
      (attr0->group_types == attr1->group_types) &&
      (attr0->group_capabilities == attr1->group_capabilities) &&
      (attr0->packet_inq_size == attr1->packet_inq_size) &&
      (attr0->packet_inq_max_batches == attr1->packet_inq_max_batches) &&
#ifdef HYBRID
      (attr0->l2_bridge == attr1->l2_bridge) &&
      (bridge_mactable_entry_equals(attr0->mactable_entries,
                              attr1->mactable_entries) == true) &&
      (attr0->mactable_ageing_time == attr1->mactable_ageing_time) &&
      (attr0->mactable_max_entries == attr1->mactable_max_entries) &&
#endif /* HYBRID */
      (attr0->up_streamq_size == attr1->up_streamq_size) &&
      (attr0->up_streamq_max_batches == attr1->up_streamq_max_batches) &&
      (attr0->down_streamq_size == attr1->down_streamq_size) &&
      (attr0->down_streamq_max_batches == attr1->down_streamq_max_batches)
    ) {
    return true;
  }

  return false;
}

static inline bool
bridge_attr_equals_without_qmax_batches(bridge_attr_t *attr0, bridge_attr_t *attr1) {
  if (attr0 == NULL || attr1 == NULL) {
    return false;
  }

  if ((datastore_names_equals(attr0->controller_names,
                                attr1->controller_names) == true) &&
      (datastore_names_equals(attr0->port_names,
                              attr1->port_names) == true) &&
      (attr0->dpid == attr1->dpid) &&
      (attr0->fail_mode == attr1->fail_mode) &&
      (attr0->flow_statistics == attr1->flow_statistics) &&
      (attr0->group_statistics == attr1->group_statistics) &&
      (attr0->port_statistics == attr1->port_statistics) &&
      (attr0->queue_statistics == attr1->queue_statistics) &&
      (attr0->table_statistics == attr1->table_statistics) &&
      (attr0->reassemble_ip_fragments == attr1->reassemble_ip_fragments) &&
      (attr0->max_buffered_packets == attr1->max_buffered_packets) &&
      (attr0->max_ports == attr1->max_ports) &&
      (attr0->max_tables == attr1->max_tables) &&
      (attr0->block_looping_ports == attr1->block_looping_ports) &&
      (attr0->action_types == attr1->action_types) &&
      (attr0->instruction_types == attr1->instruction_types) &&
      (attr0->max_flows == attr1->max_flows) &&
      (attr0->reserved_port_types == attr1->reserved_port_types) &&
      (attr0->group_types == attr1->group_types) &&
      (attr0->group_capabilities == attr1->group_capabilities) &&
      (attr0->packet_inq_size == attr1->packet_inq_size) &&
#ifdef HYBRID
      (attr0->l2_bridge == attr1->l2_bridge) &&
      (bridge_mactable_entry_equals(attr0->mactable_entries,
                              attr1->mactable_entries) == true) &&
      (attr0->mactable_ageing_time == attr1->mactable_ageing_time) &&
      (attr0->mactable_max_entries == attr1->mactable_max_entries) &&
#endif /* HYBRID */
      (attr0->up_streamq_size == attr1->up_streamq_size) &&
      (attr0->down_streamq_size == attr1->down_streamq_size)
    ) {
    return true;
  }

  return false;
}

static inline lagopus_result_t
bridge_attr_add_controller_name(const bridge_attr_t *attr, const char *name) {
  if (attr == NULL || name == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  return datastore_add_names(attr->controller_names, name);
}

static inline lagopus_result_t
bridge_attr_add_port_name(const bridge_attr_t *attr, const char *name) {
  if (attr == NULL || name == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  return datastore_add_names(attr->port_names, name);
}

static inline lagopus_result_t
bridge_attr_remove_controller_name(const bridge_attr_t *attr,
                                   const char *name) {
  if (attr == NULL || name == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  return datastore_remove_names(attr->controller_names, name);
}

static inline lagopus_result_t
bridge_attr_remove_port_name(const bridge_attr_t *attr, const char *name) {
  if (attr == NULL || name == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  return datastore_remove_names(attr->port_names, name);
}

static inline lagopus_result_t
bridge_attr_remove_all_controller_name(const bridge_attr_t *attr) {
  if (attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  return datastore_remove_all_names(attr->controller_names);
}

static inline lagopus_result_t
bridge_attr_remove_all_port_name(const bridge_attr_t *attr) {
  if (attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  return datastore_remove_all_names(attr->port_names);
}

static inline lagopus_result_t
bridge_conf_create(bridge_conf_t **conf, const char *name) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_attr_t *default_modified_attr = NULL;

  if (conf != NULL && IS_VALID_STRING(name) == true) {
    char *cname = strdup(name);
    if (IS_VALID_STRING(cname) == true) {
      *conf = (bridge_conf_t *) malloc(sizeof(bridge_conf_t));
      ret = bridge_attr_create(&default_modified_attr);
      if (*conf != NULL && ret == LAGOPUS_RESULT_OK) {
        (*conf)->name = cname;
        (*conf)->current_attr = NULL;
        (*conf)->modified_attr = default_modified_attr;
        (*conf)->is_used = false;
        (*conf)->is_enabled = false;
        (*conf)->is_destroying = false;
        (*conf)->is_enabling = false;
        (*conf)->is_disabling = false;

        return LAGOPUS_RESULT_OK;
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
        goto error;
      }
    }
  error:
    free((void *) default_modified_attr);
    free((void *) *conf);
    *conf = NULL;
    free((void *) cname);
    return ret;
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline void
bridge_conf_destroy(bridge_conf_t *conf) {
  if (conf != NULL && IS_VALID_STRING(conf->name) == true) {
    free((void *) (conf->name));
    if (conf->current_attr != NULL) {
      bridge_attr_destroy(conf->current_attr);
    }
    if (conf->modified_attr != NULL) {
      bridge_attr_destroy(conf->modified_attr);
    }
  }
  free((void *) conf);
}

static inline lagopus_result_t
bridge_conf_duplicate(const bridge_conf_t *src_conf,
                      bridge_conf_t **dst_conf, const char *namespace) {
  lagopus_result_t rc;
  bridge_attr_t *dst_current_attr = NULL;
  bridge_attr_t *dst_modified_attr = NULL;
  size_t len = 0;
  char *buf = NULL;

  if (src_conf == NULL || dst_conf == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*dst_conf != NULL) {
    bridge_conf_destroy(*dst_conf);
    *dst_conf = NULL;
  }

  if (namespace == NULL) {
    rc = bridge_conf_create(dst_conf, src_conf->name);
    if (rc != LAGOPUS_RESULT_OK) {
      goto error;
    }
  } else {
    if ((len = strlen(src_conf->name)) <= DATASTORE_BRIDGE_FULLNAME_MAX) {
      rc = ns_replace_namespace(src_conf->name, namespace, &buf);
      if (rc == LAGOPUS_RESULT_OK) {
        rc = bridge_conf_create(dst_conf, buf);
        if (rc != LAGOPUS_RESULT_OK) {
          goto error;
        }
      } else {
        goto error;
      }
      free(buf);
    } else {
      rc = LAGOPUS_RESULT_TOO_LONG;
      goto error;
    }
  }

  if (src_conf->current_attr != NULL) {
    rc = bridge_attr_duplicate(src_conf->current_attr,
                               &dst_current_attr, namespace);
    if (rc != LAGOPUS_RESULT_OK) {
      goto error;
    }
  }
  (*dst_conf)->current_attr = dst_current_attr;

  if (src_conf->modified_attr != NULL) {
    rc = bridge_attr_duplicate(src_conf->modified_attr,
                               &dst_modified_attr, namespace);
    if (rc != LAGOPUS_RESULT_OK) {
      goto error;
    }
  }
  (*dst_conf)->modified_attr = dst_modified_attr;

  (*dst_conf)->is_used = src_conf->is_used;
  (*dst_conf)->is_enabled = src_conf->is_enabled;
  (*dst_conf)->is_destroying = src_conf->is_destroying;
  (*dst_conf)->is_enabling = src_conf->is_enabling;
  (*dst_conf)->is_disabling = src_conf->is_disabling;

  return LAGOPUS_RESULT_OK;

error:
  free(buf);
  bridge_conf_destroy(*dst_conf);
  *dst_conf = NULL;
  return rc;
}

static inline bool
bridge_conf_equals(const bridge_conf_t *conf0,
                   const bridge_conf_t *conf1) {
  if (conf0 == NULL && conf1 == NULL) {
    return true;
  } else if (conf0 == NULL || conf1 == NULL) {
    return false;
  }

  if ((bridge_attr_equals(conf0->current_attr,
                          conf1->current_attr) == true) &&
      (bridge_attr_equals(conf0->modified_attr,
                          conf1->modified_attr) == true) &&
      (conf0->is_used == conf1->is_used) &&
      (conf0->is_enabled == conf1->is_enabled) &&
      (conf0->is_destroying == conf1->is_destroying) &&
      (conf0->is_enabling == conf1->is_enabling) &&
      (conf0->is_disabling == conf1->is_disabling)) {
    return true;
  }

  return false;
}

static inline lagopus_result_t
bridge_conf_add(bridge_conf_t *conf) {
  if (bridge_table != NULL) {
    if (conf != NULL && IS_VALID_STRING(conf->name) == true) {
      void *val = (void *) conf;
      return lagopus_hashmap_add(&bridge_table,
                                 (char *) (conf->name),
                                 &val, false);
    } else {
      return LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    return LAGOPUS_RESULT_NOT_STARTED;
  }
}

static inline lagopus_result_t
bridge_conf_delete(bridge_conf_t *conf) {
  if (bridge_table != NULL) {
    if (conf != NULL && IS_VALID_STRING(conf->name) == true) {
      return lagopus_hashmap_delete(&bridge_table, (void *) conf->name,
                                    NULL, true);
    } else {
      return LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    return LAGOPUS_RESULT_NOT_STARTED;
  }
}

typedef struct {
  bridge_conf_t **m_configs;
  size_t m_n_configs;
  size_t m_max;
  const char *m_namespace;
} bridge_conf_iter_ctx_t;

static bool
bridge_conf_iterate(void *key, void *val, lagopus_hashentry_t he,
                    void *arg) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_dstring_t ds;
  char *prefix = NULL;
  size_t len = 0;
  bool result = false;

  char *fullname = (char *)key;
  bridge_conf_iter_ctx_t *ctx = (bridge_conf_iter_ctx_t *)arg;

  (void)he;

  if (((bridge_conf_t *) val)->is_destroying == false) {
    if (ctx->m_namespace == NULL) {
      if (ctx->m_n_configs < ctx->m_max) {
        ctx->m_configs[ctx->m_n_configs++] =
          (bridge_conf_t *)val;
        result = true;
      } else {
        result = false;
      }
    } else {
      ret = lagopus_dstring_create(&ds);
      if (ret == LAGOPUS_RESULT_OK) {
        if (ctx->m_namespace[0] == '\0') {
          ret = lagopus_dstring_appendf(&ds,
                                        "%s",
                                        DATASTORE_NAMESPACE_DELIMITER);
        } else {
          ret = lagopus_dstring_appendf(&ds,
                                        "%s%s",
                                        ctx->m_namespace,
                                        DATASTORE_NAMESPACE_DELIMITER);
        }

        if (ret == LAGOPUS_RESULT_OK) {
          ret = lagopus_dstring_str_get(&ds, &prefix);
          if (ret == LAGOPUS_RESULT_OK) {
            len = strlen(prefix);
            if (ctx->m_n_configs < ctx->m_max &&
                strncmp(fullname, prefix, len) == 0) {
              ctx->m_configs[ctx->m_n_configs++] =
                (bridge_conf_t *)val;
            }
            result = true;
          } else {
            lagopus_msg_warning("dstring get failed.\n");
            result = false;
          }
        } else {
          lagopus_msg_warning("dstring append failed.\n");
          result = false;
        }
      } else {
        lagopus_msg_warning("dstring create failed.\n");
        result = false;
      }

      free((void *) prefix);

      (void)lagopus_dstring_clear(&ds);
      (void)lagopus_dstring_destroy(&ds);
    }
  } else {
    /* skip destroying conf.*/
    result = true;
  }

  return result;
}

static inline lagopus_result_t
bridge_conf_list(bridge_conf_t ***list, const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (bridge_table == NULL) {
    return LAGOPUS_RESULT_NOT_STARTED;
  }

  if (list != NULL) {
    size_t n = (size_t) lagopus_hashmap_size(&bridge_table);
    *list = NULL;
    if (n > 0) {
      bridge_conf_t **configs =
        (bridge_conf_t **) malloc(sizeof(bridge_conf_t *) * n);
      if (configs != NULL) {
        bridge_conf_iter_ctx_t ctx;
        ctx.m_configs = configs;
        ctx.m_n_configs = 0;
        ctx.m_max = n;
        if (namespace == NULL) {
          ctx.m_namespace = NULL;
        } else if (namespace[0] == '\0') {
          ctx.m_namespace = "";
        } else {
          ctx.m_namespace = namespace;
        }

        ret = lagopus_hashmap_iterate(&bridge_table, bridge_conf_iterate,
                                      (void *) &ctx);
        if (ret == LAGOPUS_RESULT_OK) {
          *list = configs;
          ret = (ssize_t) ctx.m_n_configs;
        } else {
          free((void *) configs);
        }
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
bridge_conf_one_list(bridge_conf_t ***list,
                     bridge_conf_t *config) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (list != NULL && config != NULL) {
    bridge_conf_t **configs = (bridge_conf_t **)
                              malloc(sizeof(bridge_conf_t *));
    if (configs != NULL) {
      configs[0] = config;
      *list = configs;
      ret = 1;
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
bridge_fail_mode_to_str(const datastore_bridge_fail_mode_t fail_mode,
                        const char **fail_mode_str) {
  if (fail_mode_str == NULL ||
      (int) fail_mode < DATASTORE_BRIDGE_FAIL_MODE_MIN ||
      (int) fail_mode > DATASTORE_BRIDGE_FAIL_MODE_MAX) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  *fail_mode_str = fail_mode_arr[fail_mode].fail_mode_str;
  return LAGOPUS_RESULT_OK;
}

static inline lagopus_result_t
bridge_fail_mode_to_enum(const char *fail_mode_str,
                         datastore_bridge_fail_mode_t *fail_mode) {
  size_t i = 0;

  if (IS_VALID_STRING(fail_mode_str) != true || fail_mode == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  for (i = DATASTORE_BRIDGE_FAIL_MODE_MIN;
       i <= DATASTORE_BRIDGE_FAIL_MODE_MAX; i++) {
    if (strcmp(fail_mode_str, fail_mode_arr[i].fail_mode_str) == 0) {
      *fail_mode = fail_mode_arr[i].fail_mode;
      return LAGOPUS_RESULT_OK;
    }
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_action_type_to_str(const datastore_bridge_action_type_t action_type,
                          const char **action_type_str) {
  if (action_type_str == NULL ||
      (int) action_type < DATASTORE_BRIDGE_ACTION_TYPE_MIN ||
      (int) action_type > DATASTORE_BRIDGE_ACTION_TYPE_MAX) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  *action_type_str = action_type_arr[action_type].action_type_str;
  return LAGOPUS_RESULT_OK;
}

static inline lagopus_result_t
bridge_action_type_to_enum(const char *action_type_str,
                           datastore_bridge_action_type_t *action_type) {
  size_t i = 0;

  if (IS_VALID_STRING(action_type_str) != true || action_type == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  for (i = DATASTORE_BRIDGE_ACTION_TYPE_MIN;
       i <= DATASTORE_BRIDGE_ACTION_TYPE_MAX; i++) {
    if (strcmp(action_type_str, action_type_arr[i].action_type_str) == 0) {
      *action_type = action_type_arr[i].action_type;
      return LAGOPUS_RESULT_OK;
    }
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_instruction_type_to_str(const datastore_bridge_instruction_type_t
                               instruction_type,
                               const char **instruction_type_str) {
  if (instruction_type_str == NULL ||
      (int) instruction_type < DATASTORE_BRIDGE_INSTRUCTION_TYPE_MIN ||
      (int) instruction_type > DATASTORE_BRIDGE_INSTRUCTION_TYPE_MAX) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  *instruction_type_str =
    instruction_type_arr[instruction_type].instruction_type_str;
  return LAGOPUS_RESULT_OK;
}

static inline lagopus_result_t
bridge_instruction_type_to_enum(const char *instruction_type_str,
                                datastore_bridge_instruction_type_t *instruction_type) {
  size_t i = 0;

  if (IS_VALID_STRING(instruction_type_str) != true ||
      instruction_type == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  for (i = DATASTORE_BRIDGE_INSTRUCTION_TYPE_MIN;
       i <= DATASTORE_BRIDGE_INSTRUCTION_TYPE_MAX; i++) {
    if (strcmp(instruction_type_str,
               instruction_type_arr[i].instruction_type_str) == 0) {
      *instruction_type = instruction_type_arr[i].instruction_type;
      return LAGOPUS_RESULT_OK;
    }
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_reserved_port_type_to_str(
  const datastore_bridge_reserved_port_type_t reserved_port_type,
  const char **reserved_port_type_str) {
  if (reserved_port_type_str == NULL ||
      (int) reserved_port_type < DATASTORE_BRIDGE_RESERVED_PORT_TYPE_MIN ||
      (int) reserved_port_type > DATASTORE_BRIDGE_RESERVED_PORT_TYPE_MAX) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  *reserved_port_type_str =
    reserved_port_type_arr[reserved_port_type].reserved_port_type_str;
  return LAGOPUS_RESULT_OK;
}

static inline lagopus_result_t
bridge_reserved_port_type_to_enum(const char *reserved_port_type_str,
                                  datastore_bridge_reserved_port_type_t *reserved_port_type) {
  size_t i = 0;

  if (IS_VALID_STRING(reserved_port_type_str) != true ||
      reserved_port_type == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  for (i = DATASTORE_BRIDGE_RESERVED_PORT_TYPE_MIN;
       i <= DATASTORE_BRIDGE_RESERVED_PORT_TYPE_MAX; i++) {
    if (strcmp(reserved_port_type_str,
               reserved_port_type_arr[i].reserved_port_type_str) == 0) {
      *reserved_port_type = reserved_port_type_arr[i].reserved_port_type;
      return LAGOPUS_RESULT_OK;
    }
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_group_type_to_str(const datastore_bridge_group_type_t group_type,
                         const char **group_type_str) {
  if (group_type_str == NULL ||
      (int) group_type < DATASTORE_BRIDGE_GROUP_TYPE_MIN ||
      (int) group_type > DATASTORE_BRIDGE_GROUP_TYPE_MAX) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  *group_type_str = group_type_arr[group_type].group_type_str;
  return LAGOPUS_RESULT_OK;
}

static inline lagopus_result_t
bridge_group_type_to_enum(const char *group_type_str,
                          datastore_bridge_group_type_t *group_type) {
  size_t i = 0;

  if (IS_VALID_STRING(group_type_str) != true || group_type == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  for (i = DATASTORE_BRIDGE_GROUP_TYPE_MIN;
       i <= DATASTORE_BRIDGE_GROUP_TYPE_MAX; i++) {
    if (strcmp(group_type_str, group_type_arr[i].group_type_str) == 0) {
      *group_type = group_type_arr[i].group_type;
      return LAGOPUS_RESULT_OK;
    }
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_group_capability_to_str(
  const datastore_bridge_group_capability_t group_capability,
  const char **group_capability_str) {
  if (group_capability_str == NULL ||
      (int) group_capability < DATASTORE_BRIDGE_GROUP_CAPABILITY_MIN ||
      (int) group_capability > DATASTORE_BRIDGE_GROUP_CAPABILITY_MAX) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  *group_capability_str =
    group_capability_arr[group_capability].group_capability_str;
  return LAGOPUS_RESULT_OK;
}

static inline lagopus_result_t
bridge_group_capability_to_enum(const char *group_capability_str,
                                datastore_bridge_group_capability_t *group_capability) {
  size_t i = 0;

  if (IS_VALID_STRING(group_capability_str) != true ||
      group_capability == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  for (i = DATASTORE_BRIDGE_GROUP_CAPABILITY_MIN;
       i <= DATASTORE_BRIDGE_GROUP_CAPABILITY_MAX; i++) {
    if (strcmp(group_capability_str,
               group_capability_arr[i].group_capability_str) == 0) {
      *group_capability = group_capability_arr[i].group_capability;
      return LAGOPUS_RESULT_OK;
    }
  }

  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_find(const char *name, bridge_conf_t **conf) {
  if (bridge_table == NULL) {
    return LAGOPUS_RESULT_NOT_STARTED;
  } else if (conf == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (IS_VALID_STRING(name) == true) {
    return lagopus_hashmap_find(&bridge_table,
                                (void *)name, (void **)conf);
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

static inline lagopus_result_t
bridge_get_attr(const char *name, bool current, bridge_attr_t **attr) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;

  if (name == NULL || attr == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (*attr != NULL) {
    bridge_attr_destroy(*attr);
    *attr = NULL;
  }

  rc = bridge_find(name, &conf);
  if (rc == LAGOPUS_RESULT_OK) {
    if (current == true) {
      *attr = conf->current_attr;
    } else {
      *attr = conf->modified_attr;
    }

    if (*attr == NULL) {
      rc = LAGOPUS_RESULT_INVALID_OBJECT;
    }
  }
  return rc;
}

static inline lagopus_result_t
bridge_get_dpid(const bridge_attr_t *attr, uint64_t *dpid) {
  if (attr != NULL && dpid != NULL) {
    *dpid = attr->dpid;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_controller_names(const bridge_attr_t *attr,
                            datastore_name_info_t **controller_names) {
  if (attr != NULL && attr->controller_names != NULL &&
      controller_names != NULL) {
    return datastore_names_duplicate(attr->controller_names,
                                     controller_names, NULL);
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_port_names(const bridge_attr_t *attr,
                      datastore_name_info_t **port_names) {
  if (attr != NULL && port_names != NULL) {
    return datastore_names_duplicate(attr->port_names,
                                     port_names, NULL);
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_fail_mode(const bridge_attr_t *attr,
                     datastore_bridge_fail_mode_t *fail_mode) {
  if (attr != NULL && fail_mode != NULL) {
    *fail_mode = attr->fail_mode;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_is_flow_statistics(const bridge_attr_t *attr, bool *flow_statistics) {
  if (attr != NULL && flow_statistics != NULL) {
    *flow_statistics = attr->flow_statistics;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_is_group_statistics(const bridge_attr_t *attr, bool *group_statistics) {
  if (attr != NULL && group_statistics != NULL) {
    *group_statistics = attr->group_statistics;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_is_port_statistics(const bridge_attr_t *attr, bool *port_statistics) {
  if (attr != NULL && port_statistics != NULL) {
    *port_statistics = attr->port_statistics;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_is_queue_statistics(const bridge_attr_t *attr, bool *queue_statistics) {
  if (attr != NULL && queue_statistics != NULL) {
    *queue_statistics = attr->queue_statistics;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_is_table_statistics(const bridge_attr_t *attr, bool *table_statistics) {
  if (attr != NULL && table_statistics != NULL) {
    *table_statistics = attr->table_statistics;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_is_reassemble_ip_fragments(const bridge_attr_t *attr,
                                  bool *reassemble_ip_fragments) {
  if (attr != NULL && reassemble_ip_fragments != NULL) {
    *reassemble_ip_fragments = attr->reassemble_ip_fragments;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_max_buffered_packets(const bridge_attr_t *attr,
                                uint32_t *max_buffered_packets) {
  if (attr != NULL && max_buffered_packets != NULL) {
    *max_buffered_packets = attr->max_buffered_packets;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_max_ports(const bridge_attr_t *attr, uint16_t *max_ports) {
  if (attr != NULL && max_ports != NULL) {
    *max_ports = attr->max_ports;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_max_tables(const bridge_attr_t *attr, uint8_t *max_tables) {
  if (attr != NULL && max_tables != NULL) {
    *max_tables = attr->max_tables;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_is_block_looping_ports(const bridge_attr_t *attr,
                              bool *block_looping_ports) {
  if (attr != NULL && block_looping_ports != NULL) {
    *block_looping_ports = attr->block_looping_ports;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_action_types(const bridge_attr_t *attr, uint64_t *action_types) {
  if (attr != NULL && action_types != NULL) {
    *action_types = attr->action_types;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_instruction_types(const bridge_attr_t *attr,
                             uint64_t *instruction_types) {
  if (attr != NULL && instruction_types != NULL) {
    *instruction_types = attr->instruction_types;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_max_flows(const bridge_attr_t *attr,
                     uint32_t *max_flows) {
  if (attr != NULL && max_flows != NULL) {
    *max_flows = attr->max_flows;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

#ifdef HYBRID
/* mactable entry list utilities */
/**
 * Set l2-bridge attribute.
 */
static inline lagopus_result_t
bridge_is_l2_bridge(const bridge_attr_t *attr,
                     bool *l2_bridge) {
  if (attr != NULL && l2_bridge!= NULL) {
    *l2_bridge = attr->l2_bridge;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

/**
 * Get mac entry from mac entry list.
 */
static inline lagopus_result_t
bridge_get_mactable_entry(const bridge_attr_t *attr,
                          bridge_mactable_info_t **mactable_entries) {
  if (attr != NULL && mactable_entries != NULL) {
    /* get entry */
    return bridge_mactable_entry_duplicate(attr->mactable_entries,
                                           mactable_entries);
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

/**
 * Get ageing time of mac table.
 */
static inline lagopus_result_t
bridge_get_mactable_ageing_time(const bridge_attr_t *attr,
                     uint32_t *mactable_ageing_time) {
  if (attr != NULL && mactable_ageing_time != NULL) {
    *mactable_ageing_time = attr->mactable_ageing_time;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

/**
 * Get number of max entries of mac table.
 */
static inline lagopus_result_t
bridge_get_mactable_max_entries(const bridge_attr_t *attr,
                     uint32_t *mactable_max_entries) {
  if (attr != NULL && mactable_max_entries != NULL) {
    *mactable_max_entries = attr->mactable_max_entries;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

/**
 * Set l2-bridge flag.
 */
static inline lagopus_result_t
bridge_set_l2_bridge(bridge_attr_t *attr,
                           const bool l2_bridge) {
  if (attr != NULL) {
    attr->l2_bridge = l2_bridge;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

/**
 * Set ageing time of mac table.
 */
static inline lagopus_result_t
bridge_set_mactable_ageing_time(bridge_attr_t *attr,
                     const uint64_t mactable_ageing_time) {
  if (attr != NULL) {
    long long int min_diff =
      (long long int) (mactable_ageing_time - MINIMUM_MACTABLE_AGEING_TIME);
    long long int max_diff =
      (long long int) (mactable_ageing_time - MAXIMUM_MACTABLE_AGEING_TIME);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->mactable_ageing_time = (uint32_t) mactable_ageing_time;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

/**
 * Set number of max entries of mac table.
 */
static inline lagopus_result_t
bridge_set_mactable_max_entries(bridge_attr_t *attr,
                     const uint64_t mactable_max_entries) {
  if (attr != NULL) {
    long long int min_diff =
      (long long int) (mactable_max_entries - MINIMUM_MACTABLE_MAX_ENTRIES);
    long long int max_diff =
      (long long int) (mactable_max_entries - MAXIMUM_MACTABLE_MAX_ENTRIES);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->mactable_max_entries = (uint32_t) mactable_max_entries;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

/**
 * Get ageing time of mac table.
 */
lagopus_result_t
datastore_bridge_get_mactable_ageing_time(const char *name, bool current,
                               uint32_t *mactable_ageing_time) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && mactable_ageing_time != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_mactable_ageing_time(attr, mactable_ageing_time);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

/**
 * Get number of max entries of mac table.
 */
lagopus_result_t
datastore_bridge_get_mactable_max_entries(const char *name, bool current,
                               uint32_t *mactable_max_entries) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && mactable_max_entries != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_mactable_max_entries(attr, mactable_max_entries);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}
#endif /* HYBRID */


static inline lagopus_result_t
bridge_get_reserved_port_types(const bridge_attr_t *attr,
                               uint64_t *reserved_port_types) {
  if (attr != NULL && reserved_port_types != NULL) {
    *reserved_port_types = attr->reserved_port_types;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_group_types(const bridge_attr_t *attr,
                       uint64_t *group_types) {
  if (attr != NULL && group_types != NULL) {
    *group_types = attr->group_types;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_group_capabilities(const bridge_attr_t *attr,
                              uint64_t *group_capabilities) {
  if (attr != NULL && group_capabilities != NULL) {
    *group_capabilities = attr->group_capabilities;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_packet_inq_size(const bridge_attr_t *attr,
                           uint16_t *packet_inq_size) {
  if (attr != NULL && packet_inq_size != NULL) {
    *packet_inq_size = attr->packet_inq_size;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_packet_inq_max_batches(const bridge_attr_t *attr,
                                  uint16_t *packet_inq_max_batches) {
  if (attr != NULL && packet_inq_max_batches != NULL) {
    *packet_inq_max_batches = attr->packet_inq_max_batches;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_up_streamq_size(const bridge_attr_t *attr,
                           uint16_t *up_streamq_size) {
  if (attr != NULL && up_streamq_size != NULL) {
    *up_streamq_size = attr->up_streamq_size;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_up_streamq_max_batches(const bridge_attr_t *attr,
                                  uint16_t *up_streamq_max_batches) {
  if (attr != NULL && up_streamq_max_batches != NULL) {
    *up_streamq_max_batches = attr->up_streamq_max_batches;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_down_streamq_size(const bridge_attr_t *attr,
                             uint16_t *down_streamq_size) {
  if (attr != NULL && down_streamq_size != NULL) {
    *down_streamq_size = attr->down_streamq_size;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_get_down_streamq_max_batches(const bridge_attr_t *attr,
                                    uint16_t *down_streamq_max_batches) {
  if (attr != NULL && down_streamq_max_batches != NULL) {
    *down_streamq_max_batches = attr->down_streamq_max_batches;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static bool
bridge_dpid_is_exists_iterate(void *key, void *val,
                              lagopus_hashentry_t he,
                              void *arg) {
  bool result = false;
  bridge_dpid_is_exists_iter_ctx_t *ctx =
    (bridge_dpid_is_exists_iter_ctx_t *) arg;
  bridge_conf_t *conf = (bridge_conf_t *) val;
  (void) he;
  (void) key;

  if (conf != NULL && ctx != NULL) {
    if (conf->current_attr != NULL) {
      if (conf->current_attr->dpid == ctx->dpid) {
        ctx->name = strdup(conf->name);
        if (ctx->name != NULL) {
          ctx->rc = LAGOPUS_RESULT_OK;
          ctx->is_exists = true;
          result = false;
        } else {
          ctx->rc = LAGOPUS_RESULT_NO_MEMORY;
          result = false;
        }
        goto done;
      }
    }
    if (conf->modified_attr != NULL) {
      if (conf->modified_attr->dpid == ctx->dpid) {
        ctx->name = strdup(conf->name);
        if (ctx->name != NULL) {
          ctx->rc = LAGOPUS_RESULT_OK;
          ctx->is_exists = true;
          result = false;
        } else {
          ctx->rc = LAGOPUS_RESULT_NO_MEMORY;
          result = false;
        }
        goto done;
      }
    }

    ctx->rc = LAGOPUS_RESULT_OK;
    result = true;
  } else {
    result = false;
  }

done:
  return result;
}

static inline bool
bridge_dpid_is_exists(uint64_t dpid) {
  lagopus_result_t rc = LAGOPUS_RESULT_ANY_FAILURES;
  bool result = false;
  bridge_dpid_is_exists_iter_ctx_t ctx;

  ctx.rc = LAGOPUS_RESULT_ANY_FAILURES;
  ctx.dpid = dpid;
  ctx.name = NULL;
  ctx.is_exists = false;

  rc = lagopus_hashmap_iterate(&bridge_table,
                               bridge_dpid_is_exists_iterate,
                               (void *) &ctx);
  if ((rc == LAGOPUS_RESULT_OK ||
       rc == LAGOPUS_RESULT_ITERATION_HALTED) &&
      ctx.rc == LAGOPUS_RESULT_OK) {
    result = ctx.is_exists;
  } else {
    lagopus_perror(rc);
    lagopus_perror(ctx.rc);
  }

  free(ctx.name);

  return result;
}

static inline lagopus_result_t
bridge_set_dpid(bridge_attr_t *attr, const uint64_t dpid) {
  if (attr != NULL) {
    if (bridge_dpid_is_exists(dpid) == true) {
      return LAGOPUS_RESULT_ALREADY_EXISTS;
    } else {
      attr->dpid = dpid;
      return LAGOPUS_RESULT_OK;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_controller_names(bridge_attr_t *attr,
                            const datastore_name_info_t *controller_names) {
  if (attr != NULL && controller_names != NULL) {
    return datastore_names_duplicate(controller_names,
                                     &(attr->controller_names), NULL);
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_port_names(bridge_attr_t *attr,
                      const datastore_name_info_t *port_names) {
  if (attr != NULL && port_names != NULL) {
    return datastore_names_duplicate(port_names,
                                     &(attr->port_names), NULL);
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_fail_mode(bridge_attr_t *attr,
                     const datastore_bridge_fail_mode_t fail_mode) {
  if (attr != NULL) {
    attr->fail_mode = fail_mode;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_flow_statistics(bridge_attr_t *attr, const bool flow_statistics) {
  if (attr != NULL) {
    attr->flow_statistics = flow_statistics;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_group_statistics(bridge_attr_t *attr, const bool group_statistics) {
  if (attr != NULL) {
    attr->group_statistics = group_statistics;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_port_statistics(bridge_attr_t *attr, const bool port_statistics) {
  if (attr != NULL) {
    attr->port_statistics = port_statistics;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_queue_statistics(bridge_attr_t *attr, const bool queue_statistics) {
  if (attr != NULL) {
    attr->queue_statistics= queue_statistics;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_table_statistics(bridge_attr_t *attr, const bool table_statistics) {
  if (attr != NULL) {
    attr->table_statistics = table_statistics;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_reassemble_ip_fragments(bridge_attr_t *attr,
                                   const bool reassemble_ip_fragments) {
  if (attr != NULL) {
    attr->reassemble_ip_fragments = reassemble_ip_fragments;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_max_buffered_packets(bridge_attr_t *attr,
                                const uint64_t max_buffered_packets) {
  if (attr != NULL) {
    long long int min_diff =
      (long long int) (max_buffered_packets - MINIMUM_BUFFERED_PACKETS);
    long long int max_diff =
      (long long int) (max_buffered_packets - MAXIMUM_BUFFERED_PACKETS);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->max_buffered_packets = (uint32_t) max_buffered_packets;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_max_ports(bridge_attr_t *attr, const uint64_t max_ports) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (max_ports - MINIMUM_PORTS);
    long long int max_diff = (long long int) (max_ports - MAXIMUM_PORTS);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->max_ports = (uint16_t) max_ports;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_max_tables(bridge_attr_t *attr, const uint64_t max_tables) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (max_tables - MINIMUM_TABLES);
    long long int max_diff = (long long int) (max_tables - MAXIMUM_TABLES);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->max_tables = (uint8_t) max_tables;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_block_looping_ports(bridge_attr_t *attr,
                               const bool block_looping_ports) {
  if (attr != NULL) {
    attr->block_looping_ports = block_looping_ports;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_action_types(bridge_attr_t *attr, const uint64_t action_types) {
  if (attr != NULL) {
    attr->action_types= action_types;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_instruction_types(bridge_attr_t *attr,
                             const uint64_t instruction_types) {
  if (attr != NULL) {
    attr->instruction_types = instruction_types;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_max_flows(bridge_attr_t *attr,
                     const uint64_t max_flows) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (max_flows - MINIMUM_FLOWS);
    long long int max_diff = (long long int) (max_flows - MAXIMUM_FLOWS);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->max_flows = (uint32_t) max_flows;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_reserved_port_types(bridge_attr_t *attr,
                               const uint64_t reserved_port_types) {
  if (attr != NULL) {
    attr->reserved_port_types = reserved_port_types;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_group_types(bridge_attr_t *attr,
                       const uint64_t group_types) {
  if (attr != NULL) {
    attr->group_types = group_types;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_group_capabilities(bridge_attr_t *attr,
                              const uint64_t group_capabilities) {
  if (attr != NULL) {
    attr->group_capabilities = group_capabilities;
    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_packet_inq_size(bridge_attr_t *attr,
                           const uint64_t packet_inq_size) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (packet_inq_size -
                                              MINIMUM_PACKET_INQ_SIZE);
    long long int max_diff = (long long int) (packet_inq_size -
                                              MAXIMUM_PACKET_INQ_SIZE);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->packet_inq_size = (uint16_t) packet_inq_size;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_packet_inq_max_batches(bridge_attr_t *attr,
                                  const uint64_t packet_inq_max_batches) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (packet_inq_max_batches -
                                              MINIMUM_PACKET_INQ_MAX_BATCHES);
    long long int max_diff = (long long int) (packet_inq_max_batches -
                                              MAXIMUM_PACKET_INQ_MAX_BATCHES);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->packet_inq_max_batches = (uint16_t) packet_inq_max_batches;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_up_streamq_size(bridge_attr_t *attr,
                           const uint64_t up_streamq_size) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (up_streamq_size -
                                              MINIMUM_UP_STREAMQ_SIZE);
    long long int max_diff = (long long int) (up_streamq_size -
                                              MAXIMUM_UP_STREAMQ_SIZE);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->up_streamq_size = (uint16_t) up_streamq_size;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_up_streamq_max_batches(bridge_attr_t *attr,
                                  const uint64_t up_streamq_max_batches) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (up_streamq_max_batches -
                                              MINIMUM_UP_STREAMQ_MAX_BATCHES);
    long long int max_diff = (long long int) (up_streamq_max_batches -
                                              MAXIMUM_UP_STREAMQ_MAX_BATCHES);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->up_streamq_max_batches = (uint16_t) up_streamq_max_batches;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_down_streamq_size(bridge_attr_t *attr,
                             const uint64_t down_streamq_size) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (down_streamq_size -
                                              MINIMUM_DOWN_STREAMQ_SIZE);
    long long int max_diff = (long long int) (down_streamq_size -
                                              MAXIMUM_DOWN_STREAMQ_SIZE);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->down_streamq_size = (uint16_t) down_streamq_size;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static inline lagopus_result_t
bridge_set_down_streamq_max_batches(bridge_attr_t *attr,
                                    const uint64_t down_streamq_max_batches) {
  if (attr != NULL) {
    long long int min_diff = (long long int) (down_streamq_max_batches -
                                              MINIMUM_DOWN_STREAMQ_MAX_BATCHES);
    long long int max_diff = (long long int) (down_streamq_max_batches -
                                              MAXIMUM_DOWN_STREAMQ_MAX_BATCHES);
    if (max_diff <= 0 && min_diff >= 0) {
      attr->down_streamq_max_batches = (uint16_t) down_streamq_max_batches;
      return LAGOPUS_RESULT_OK;
    } else if (min_diff < 0) {
      return LAGOPUS_RESULT_TOO_SHORT;
    } else {
      return LAGOPUS_RESULT_TOO_LONG;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

static void
bridge_conf_freeup(void *conf) {
  bridge_conf_destroy((bridge_conf_t *) conf);
}

static void
bridge_child_at_fork(void) {
  lagopus_hashmap_atfork_child(&bridge_table);
}

static lagopus_result_t
bridge_initialize(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;

  if ((r = lagopus_hashmap_create(&bridge_table, LAGOPUS_HASHMAP_TYPE_STRING,
                                  bridge_conf_freeup)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    goto done;
  }

  (void)pthread_atfork(NULL, NULL, bridge_child_at_fork);

done:
  return r;
}

static void
bridge_finalize(void) {
  lagopus_hashmap_destroy(&bridge_table, true);
}

//
// internal datastore
//

bool
bridge_exists(const char *name) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;

  rc = bridge_find(name, &conf);
  if (rc == LAGOPUS_RESULT_OK && conf != NULL) {
    return true;
  }
  return false;
}

lagopus_result_t
bridge_set_used(const char *name, bool is_used) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true) {
    rc = bridge_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      conf->is_used = is_used;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
bridge_set_enabled(const char *name, bool is_enabled) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true) {
    rc = bridge_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      conf->is_enabled = is_enabled;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

//
// public
//

lagopus_result_t
datastore_bridge_get_dpid(const char *name, bool current,
                          uint64_t *dpid) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && dpid != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_dpid(attr, dpid);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_is_enabled(const char *name, bool *is_enabled) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true && is_enabled != NULL) {
    rc = bridge_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      *is_enabled = conf->is_enabled;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return rc;
}

lagopus_result_t
datastore_bridge_is_used(const char *name, bool *is_used) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;

  if (IS_VALID_STRING(name) == true && is_used != NULL) {
    rc = bridge_find(name, &conf);
    if (rc == LAGOPUS_RESULT_OK) {
      *is_used = conf->is_used;
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_name_by_dpid(uint64_t dpid,
                                  char **name) {
  lagopus_result_t rc = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_dpid_is_exists_iter_ctx_t ctx;

  if (name != NULL) {
    ctx.rc = LAGOPUS_RESULT_ANY_FAILURES;
    ctx.dpid = dpid;
    ctx.name = NULL;
    ctx.is_exists = false;

    rc = lagopus_hashmap_iterate(&bridge_table,
                                 bridge_dpid_is_exists_iterate,
                                 (void *) &ctx);
    if ((rc == LAGOPUS_RESULT_OK ||
         rc == LAGOPUS_RESULT_ITERATION_HALTED) &&
        ctx.rc == LAGOPUS_RESULT_OK) {
      if (ctx.is_exists == true) {
        *name = ctx.name;
        rc = LAGOPUS_RESULT_OK;
      } else {
        rc = LAGOPUS_RESULT_NOT_FOUND;
      }
    } else {
      lagopus_perror(rc);
    }

    if (rc != LAGOPUS_RESULT_OK) {
      free(ctx.name);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return rc;
}

lagopus_result_t
datastore_bridge_get_names(const char *name,
                           datastore_name_info_t **names) {
  lagopus_result_t rc = LAGOPUS_RESULT_ANY_FAILURES;
  bridge_conf_t **list = NULL;
  bridge_conf_t *conf = NULL;
  size_t size;
  size_t i;

  if (names != NULL) {
    if (name == NULL) {
      /* create list of all bridge name. */
      if ((rc = bridge_conf_list(&list, NULL)) < 0) {
        lagopus_perror(rc);
        goto done;
      }
    } else {
      /* create list of a bridge name (size = 1). */
      if ((rc = bridge_find(name, &conf)) ==
          LAGOPUS_RESULT_OK) {
        if (conf->is_destroying == false) {
          if ((rc = bridge_conf_one_list(&list, conf)) < 0) {
            lagopus_perror(rc);
            goto done;
          }
        } else {
          rc = LAGOPUS_RESULT_NOT_FOUND;
          lagopus_perror(rc);
          goto done;
        }
      } else {
        lagopus_perror(rc);
        goto done;
      }
    }

    size = (size_t) rc;
    /* add names. */
    if ((rc = datastore_names_create(names)) ==
        LAGOPUS_RESULT_OK) {
      for (i = 0; i < size; i++) {
        if ((rc = datastore_add_names(*names, list[i]->name)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(rc);
          break;
        }
      }

      if (rc != LAGOPUS_RESULT_OK) {
        datastore_names_destroy(*names);
        *names = NULL;
      }
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  free(list);

  return rc;
}

lagopus_result_t
datastore_bridge_get_controller_names(const char *name,
                                      bool current,
                                      datastore_name_info_t **controller_names) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && controller_names != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_controller_names(attr, controller_names);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_port_names(const char *name,
                                bool current,
                                datastore_name_info_t **port_names) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && port_names != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_port_names(attr, port_names);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}


lagopus_result_t
datastore_bridge_get_fail_mode(const char *name, bool current,
                               datastore_bridge_fail_mode_t *fail_mode) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && fail_mode != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_fail_mode(attr, fail_mode);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_is_flow_statistics(const char *name, bool current,
                                    bool *flow_statistics) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && flow_statistics != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_is_flow_statistics(attr, flow_statistics);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_is_group_statistics(const char *name, bool current,
                                     bool *group_statistics) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && group_statistics != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_is_group_statistics(attr, group_statistics);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_is_port_statistics(const char *name, bool current,
                                    bool *port_statistics) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && port_statistics != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_is_port_statistics(attr, port_statistics);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_is_queue_statistics(const char *name, bool current,
                                     bool *queue_statistics) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && queue_statistics != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_is_queue_statistics(attr, queue_statistics);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_is_table_statistics(const char *name, bool current,
                                     bool *table_statistics) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && table_statistics != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_is_table_statistics(attr, table_statistics);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_is_reassemble_ip_fragments(const char *name, bool current,
    bool *reassemble_ip_fragments) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && reassemble_ip_fragments != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_is_reassemble_ip_fragments(attr, reassemble_ip_fragments);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_max_buffered_packets(const char *name, bool current,
    uint32_t *max_buffered_packets) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && max_buffered_packets != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_max_buffered_packets(attr, max_buffered_packets);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_max_ports(const char *name, bool current,
                               uint16_t *max_ports) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && max_ports != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_max_ports(attr, max_ports);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_max_tables(const char *name, bool current,
                                uint8_t *max_tables) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && max_tables != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_max_tables(attr, max_tables);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_is_block_looping_ports(const char *name, bool current,
                                        bool *block_looping_ports) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && block_looping_ports != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_is_block_looping_ports(attr, block_looping_ports);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_action_types(const char *name, bool current,
                                  uint64_t *action_types) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && action_types != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_action_types(attr, action_types);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_instruction_types(const char *name, bool current,
                                       uint64_t *instruction_types) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && instruction_types != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_instruction_types(attr, instruction_types);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_max_flows(const char *name, bool current,
                               uint32_t *max_flows) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && max_flows != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_max_flows(attr, max_flows);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_reserved_port_types(const char *name, bool current,
    uint64_t *reserved_port_types) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && reserved_port_types != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_reserved_port_types(attr, reserved_port_types);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_group_types(const char *name, bool current,
                                 uint64_t *group_types) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && group_types != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_group_types(attr, group_types);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_group_capabilities(const char *name, bool current,
                                        uint64_t *group_capabilities) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && group_capabilities != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_group_capabilities(attr, group_capabilities);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_packet_inq_size(const char *name, bool current,
                                     uint16_t *packet_inq_size) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && packet_inq_size != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_packet_inq_size(attr, packet_inq_size);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_packet_inq_max_batches(const char *name, bool current,
                                            uint16_t *packet_inq_max_batches) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && packet_inq_max_batches != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_packet_inq_max_batches(attr, packet_inq_max_batches);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_up_streamq_size(const char *name, bool current,
                                     uint16_t *up_streamq_size) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && up_streamq_size != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_up_streamq_size(attr, up_streamq_size);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_up_streamq_max_batches(const char *name, bool current,
                                            uint16_t *up_streamq_max_batches) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && up_streamq_max_batches != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_up_streamq_max_batches(attr, up_streamq_max_batches);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_down_streamq_size(const char *name, bool current,
                                       uint16_t *down_streamq_size) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && down_streamq_size != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_down_streamq_size(attr, down_streamq_size);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}

lagopus_result_t
datastore_bridge_get_down_streamq_max_batches(
    const char *name, bool current,
    uint16_t *down_streamq_max_batches) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;

  if (IS_VALID_STRING(name) == true && down_streamq_max_batches != NULL) {
    rc = bridge_get_attr(name, current, &attr);
    if (rc == LAGOPUS_RESULT_OK) {
      rc = bridge_get_down_streamq_max_batches(attr, down_streamq_max_batches);
    }
  } else {
    rc = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return rc;
}
