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
 *      @file   flowdb.c
 *      @brief  Flow Database.
 */

#include "lagopus_config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "openflow.h"
#include "lagopus/dp_apis.h"
#include "lagopus/flowdb.h"
#include "lagopus/group.h"
#include "lagopus/port.h"
#include "lagopus/flowinfo.h"
#include "lagopus/ethertype.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/ofcache.h"

#include "../agent/ofp_instruction.h"
#include "../agent/ofp_action.h"
#include "../agent/ofp_match.h"
#include "../agent/openflow13packet.h"

#include "lock.h"

#include "callback.h"

/**
 * @brief Flow database.
 */
struct flowdb {
#ifdef HAVE_DPDK
  rte_rwlock_t rwlock;          /** Read-write lock. */
#else
  pthread_rwlock_t rwlock;      /** Read-write lock. */
#endif /* HAVE_DPDK */
  uint8_t table_size;           /** Flow table size. */
  struct table **tables;        /** Flow table. */
  enum switch_mode switch_mode; /** Switch mode. */

};

#define UPDATE_TIMEOUT 2

#define PUT_TIMEOUT 100LL * 1000LL * 1000LL

#define OXM_FIELD_TYPE(X)       ((X) >> 1)
#define OXM_DECODE_GETL(X)      (ntohs(*(uint16_t *)(X)))

#define CHECK_FLAG(V, F)        ((V) & (F))
#define SET_FLAG(V, F)          (V) = (V) | (F)
#define FIELD_TYPE_BIT(T)       ((uint64_t)1 << T)

#define GET_OXM_FIELD(ofpat) \
  ((((const struct ofp_action_set_field *)(ofpat))->field[2] >> 1) & 0x7f)

static lagopus_result_t
send_flow_removed(uint64_t dpid, struct flow *flow, uint8_t reason);

static void
flow_del_from_meter(struct meter_table *meter_table, struct flow *flow);

static void
flow_del_from_group(struct group_table *group_table, struct flow *flow);

void
match_list_entry_free(struct match_list *match_list) {
  struct match *match;

  while ((match = TAILQ_FIRST(match_list)) != NULL) {
    TAILQ_REMOVE(match_list, match, entry);
    free(match);
  }
}

/**
 * Return host byte order MPLS value.
 */
uint32_t
match_mpls_label_host_get(struct match *match) {
  int i;
  uint32_t val;

  val = 0;
  for (i = 0; i < match->oxm_length; i++) {
    val <<= 8;
    val += match->oxm_value[i];
  }
  return val;
}

void
action_list_entry_free(struct action_list *action_list) {
  struct action *action;

  while ((action = TAILQ_FIRST(action_list)) != NULL) {
    TAILQ_REMOVE(action_list, action, entry);
    if (action != NULL) {
      ed_prop_list_free(&action->ed_prop_list);
    }
    free(action);
  }
}

void
instruction_list_entry_free(struct instruction_list *instruction_list) {
  struct instruction *instruction;

  while ((instruction = TAILQ_FIRST(instruction_list)) != NULL) {
    action_list_entry_free(&instruction->action_list);
    TAILQ_REMOVE(instruction_list, instruction, entry);
    free(instruction);
  }
}

static inline int
instruction_index(struct instruction *instruction) {
  int idx;

  switch (instruction->ofpit.type) {
    case OFPIT_METER:
      idx = INSTRUCTION_INDEX_METER;
      break;
    case OFPIT_APPLY_ACTIONS:
      idx = INSTRUCTION_INDEX_APPLY_ACTIONS;
      break;
    case OFPIT_CLEAR_ACTIONS:
      idx = INSTRUCTION_INDEX_CLEAR_ACTIONS;
      break;
    case OFPIT_WRITE_ACTIONS:
      idx = INSTRUCTION_INDEX_WRITE_ACTIONS;
      break;
    case OFPIT_WRITE_METADATA:
      idx = INSTRUCTION_INDEX_WRITE_METADATA;
      break;
    case OFPIT_GOTO_TABLE:
      idx = INSTRUCTION_INDEX_GOTO_TABLE;
      break;
    default:
      idx = INSTRUCTION_INDEX_MAX;
      break;
  }
  return idx;
}

/**
 * map instruction in the list to array.
 */
static lagopus_result_t
map_instruction_list_to_array(struct instruction **dest,
                              struct instruction_list *list,
                              struct ofp_error *error) {
  struct instruction *inst;
  int idx;

  for (idx = 0; idx < INSTRUCTION_INDEX_MAX; idx++) {
    dest[idx] = NULL;
  }
  TAILQ_FOREACH(inst, list, entry) {
    idx = instruction_index(inst);
    if (idx == INSTRUCTION_INDEX_MAX) {
      error->type = OFPET_BAD_INSTRUCTION;
      error->code = OFPBIC_UNKNOWN_INST;
      lagopus_msg_info("%d: unknown instruction (%d:%d)",
                       inst->ofpit.type, error->type, error->code);
      return LAGOPUS_RESULT_OFP_ERROR;
    }
    if (dest[idx] != NULL) {
      /* same type instruction has already registered. */
      error->type = OFPET_FLOW_MOD_FAILED;
      error->code = OFPFMFC_UNKNOWN;
      lagopus_msg_info("%d: already specified instruction (%d:%d)",
                       inst->ofpit.type, error->type, error->code);
      return LAGOPUS_RESULT_OFP_ERROR;
    }
    dest[idx] = inst;
  }
  return LAGOPUS_RESULT_OK;
}

static void
flow_free(struct flow *flow) {
  if (flow->flow_timer != NULL) {
    /* clear relationship. */
    *flow->flow_timer = NULL;
  }
  match_list_entry_free(&flow->match_list);
  instruction_list_entry_free(&flow->instruction_list);
  free(flow);
}

static lagopus_result_t
flow_alloc(struct ofp_flow_mod *flow_mod,
           struct match_list *match_list,
           struct instruction_list *instruction_list,
           struct flow **flowp,
           struct ofp_error *error) {
  struct match *match;
  struct flow *flow;
  lagopus_result_t ret;
  size_t i;

  i = 1;
  TAILQ_FOREACH(match, match_list, entry) {
    i++;
  }
  flow = (struct flow *)calloc(1, (size_t)
                               (sizeof(struct flow) + i * sizeof(match)));
  *flowp = flow;
  if (flow == NULL) {
    ret = LAGOPUS_RESULT_NO_MEMORY;
    goto out;
  }

  flow->priority = flow_mod->priority;
  flow->flags = flow_mod->flags;
  flow->cookie = flow_mod->cookie;
  flow->idle_timeout = flow_mod->idle_timeout;
  flow->hard_timeout = flow_mod->hard_timeout;
  TAILQ_INIT(&flow->match_list);
  TAILQ_CONCAT(&flow->match_list, match_list, entry);
  TAILQ_INIT(&flow->instruction_list);
  TAILQ_CONCAT(&flow->instruction_list, instruction_list, entry);
  ret = map_instruction_list_to_array(flow->instruction,
                                      &flow->instruction_list, error);
  if (ret != LAGOPUS_RESULT_OK) {
    flow_free(flow);
    *flowp = NULL;
    goto out;
  }
  flow->create_time = get_current_time();
  flow->update_time = flow->create_time;

out:
  return ret;
}

static struct table *
table_alloc(uint8_t table_id) {
  struct table *table;

  table = (struct table *)calloc(1, sizeof(struct table));
  if (table == NULL) {
    return NULL;
  }

  table->table_id = table_id;
  table->flow_list = calloc(1, sizeof(struct flow_list)
                            + sizeof(void *) * 65536);
  table->flow_list->nbranch = 65536;
  return table;
}

struct table *
flowdb_get_table(struct flowdb *flowdb, uint8_t table_id) {
  if (flowdb->tables[table_id] == NULL) {
    flowdb->tables[table_id] = table_alloc(table_id);
  }

  return flowdb->tables[table_id];
}

struct table *
table_lookup(struct flowdb *flowdb, uint8_t table_id) {
  return flowdb->tables[table_id];
}

static void
table_free(struct table *table) {
  struct flow_list *flow_list;
  int nflow, i;

  flow_list = table->flow_list;
#ifdef USE_MBTREE
  cleanup_mbtree(flow_list);
#endif /* USE_MBTREE */
#ifdef USE_THTABLE
  thtable_free(flow_list->thtable);
#endif /* USE_THTABLE */
  nflow = flow_list->nflow;
  for (i = 0; i < nflow; i++) {
    flow_free(flow_list->flows[i]);
  }
  free(flow_list);
  free(table);
}

void
flowdb_lock_init(struct flowdb *flowdb) {
  (void) flowdb;
  FLOWDB_RWLOCK_INIT();
}

/* Allocate flowdb. */
struct flowdb *
flowdb_alloc(uint8_t initial_table_size) {
  struct flowdb *flowdb;
  size_t table_index_size;
  uint8_t i;

  /* Allocate flowdb. */
  flowdb = (struct flowdb *)calloc(1, sizeof(struct flowdb));
  if (flowdb == NULL) {
    return NULL;
  }

  /* Set default switch mode. */
  flowdb_switch_mode_set(flowdb, SWITCH_MODE_STANDALONE);

  /* Allocate table index. */
  table_index_size = sizeof(struct table *) * (FLOWDB_TABLE_SIZE_MAX + 1);
  flowdb->tables = (struct table **)calloc(1, table_index_size);
  if (flowdb->tables == NULL) {
    flowdb_free(flowdb);
    return NULL;
  }

  /* Allocate tables. */
  for (i = 0; i < initial_table_size; i++) {
    flowdb->tables[i] = table_alloc(i);
    if (flowdb->tables[i] == NULL) {
      flowdb_free(flowdb);
      return NULL;
    }
  }

  /* Set table size. */
  flowdb->table_size = FLOWDB_TABLE_SIZE_MAX;

  return flowdb;
}

void
flowdb_free(struct flowdb *flowdb) {
  int i;

  /* Free table index. */
  if (flowdb->tables != NULL) {
    for (i = 0; i < flowdb->table_size; i++) {
      if (flowdb->tables[i]) {
        table_free(flowdb->tables[i]);
        flowdb->tables[i] = NULL;
      }
    }
    free(flowdb->tables);
    flowdb->tables = NULL;
  }

  /* Free flowdb. */
  free(flowdb);
}

lagopus_result_t
flowdb_switch_mode_get(struct flowdb *flowdb, enum switch_mode *switch_mode) {
  *switch_mode = flowdb->switch_mode;

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
flowdb_switch_mode_set(struct flowdb *flowdb, enum switch_mode switch_mode) {
  flowdb->switch_mode = switch_mode;

  return LAGOPUS_RESULT_OK;
}

/**
 * Pre-requisite condition check.  The match list must be sorted by
 * match field type. When duplicated match entry is found or
 * pre-requisite condition failed, return error.
 */
static lagopus_result_t
flow_pre_requisite_check(struct flow *flow,
                         struct match_list *match_list,
                         struct ofp_error *error) {
  struct match *match;
  int field_type;
  //uint64_t field_bits;

#define CHECK_FIELD_BIT(mask, bit)   CHECK_FLAG((mask), FIELD_TYPE_BIT((bit)))

  /* Set default flow type to misc. */
  flow->field_bits = 0;

  /* Iterate match entry. */
  TAILQ_FOREACH(match, match_list, entry) {
    field_type = OXM_FIELD_TYPE(match->oxm_field);

    /* Duplication field error. */
    if (CHECK_FIELD_BIT(flow->field_bits, field_type)) {
      lagopus_msg_info("Duplication field error at match type %s\n",
                       oxm_ofb_match_fields_str(field_type));
      error->type = OFPET_BAD_MATCH;
      error->code = OFPBMC_DUP_FIELD;
      return LAGOPUS_RESULT_OFP_ERROR;
    }

    /* Set field type to flow's field mask. */
    SET_FLAG(flow->field_bits, FIELD_TYPE_BIT(field_type));

    switch (field_type) {
      case OFPXMT_OFB_IN_PORT:
        /* No prerequisite. */
        break;
      case OFPXMT_OFB_IN_PHY_PORT:
        /* IN_PORT must exist. */
        if (!CHECK_FIELD_BIT(flow->field_bits, OFPXMT_OFB_IN_PORT)) {
          goto bad_out;
        }
        break;
      case OFPXMT_OFB_METADATA:
      case OFPXMT_OFB_ETH_DST:
      case OFPXMT_OFB_ETH_SRC:
        /* No prerequisite. */
        break;
      case OFPXMT_OFB_ETH_TYPE:
        /* Determine flow type based upon ether type. */
        break;
      case OFPXMT_OFB_VLAN_VID:
        /* No prerequisite. */
        break;
      case OFPXMT_OFB_VLAN_PCP:
        /* VLAN_VID must exist. */
        if (!CHECK_FIELD_BIT(flow->field_bits, OFPXMT_OFB_VLAN_VID)) {
          goto bad_out;
        }
        break;
        /* Check pre-requisites  */
      case OFPXMT_OFB_IP_DSCP:
      case OFPXMT_OFB_IP_ECN:
      case OFPXMT_OFB_IP_PROTO:
      case OFPXMT_OFB_IPV4_SRC:
      case OFPXMT_OFB_IPV4_DST:
        if (!CHECK_FIELD_BIT(flow->field_bits, OFPXMT_OFB_ETH_TYPE)) {
          /* ETH_TYPE must exist. */
          goto bad_out;
        }
        break;
      case OFPXMT_OFB_TCP_SRC:
      case OFPXMT_OFB_TCP_DST:
      case OFPXMT_OFB_UDP_SRC:
      case OFPXMT_OFB_UDP_DST:
      case OFPXMT_OFB_SCTP_SRC:
      case OFPXMT_OFB_SCTP_DST:
      case OFPXMT_OFB_ICMPV4_TYPE:
      case OFPXMT_OFB_ICMPV4_CODE:
        if (!CHECK_FIELD_BIT(flow->field_bits, OFPXMT_OFB_ETH_TYPE)) {
          /* ETH_TYPE must exist. */
          goto bad_out;
        }
        if (!CHECK_FIELD_BIT(flow->field_bits, OFPXMT_OFB_IP_PROTO)) {
          /* IP_PROTO must exist. */
          goto bad_out;
        }
        break;
      case OFPXMT_OFB_ARP_OP:
      case OFPXMT_OFB_ARP_SPA:
      case OFPXMT_OFB_ARP_TPA:
      case OFPXMT_OFB_ARP_SHA:
      case OFPXMT_OFB_ARP_THA:
      case OFPXMT_OFB_IPV6_SRC:
      case OFPXMT_OFB_IPV6_DST:
      case OFPXMT_OFB_IPV6_FLABEL:
        if (!CHECK_FIELD_BIT(flow->field_bits, OFPXMT_OFB_ETH_TYPE)) {
          /* ETH_TYPE must exist. */
          goto bad_out;
        }
        break;
      case OFPXMT_OFB_ICMPV6_TYPE:
      case OFPXMT_OFB_ICMPV6_CODE:
      case OFPXMT_OFB_IPV6_ND_TARGET:
      case OFPXMT_OFB_IPV6_ND_SLL:
      case OFPXMT_OFB_IPV6_ND_TLL:
        if (!CHECK_FIELD_BIT(flow->field_bits, OFPXMT_OFB_ETH_TYPE)) {
          /* ETH_TYPE must exist. */
          goto bad_out;
        }
        if (!CHECK_FIELD_BIT(flow->field_bits, OFPXMT_OFB_IP_PROTO)) {
          /* IP_PROTO must exist. */
          goto bad_out;
        }
        break;
      case OFPXMT_OFB_IPV6_EXTHDR:
      case OFPXMT_OFB_MPLS_LABEL:
      case OFPXMT_OFB_MPLS_TC:
      case OFPXMT_OFB_MPLS_BOS:
        if (!CHECK_FIELD_BIT(flow->field_bits, OFPXMT_OFB_ETH_TYPE)) {
          /* ETH_TYPE must exist. */
          goto bad_out;
        }
        break;
      case OFPXMT_OFB_PBB_ISID:
      case OFPXMT_OFB_TUNNEL_ID:
        /* No prerequisite. */
        break;
      default:
        break;
    }
  }
  return LAGOPUS_RESULT_OK;

bad_out:
  lagopus_msg_info("Pre-requisite error on match type %s\n",
                   oxm_ofb_match_fields_str(field_type));
  error->type = OFPET_BAD_MATCH;
  error->code = OFPBMC_BAD_PREREQ;
  return LAGOPUS_RESULT_OFP_ERROR;
}

static lagopus_result_t
flow_mask_check(struct match_list *match_list, struct ofp_error *error) {
  struct match *match;
  size_t len, i;

  /* Iterate match entry. */
  TAILQ_FOREACH(match, match_list, entry) {
    if ((match->oxm_field & 1) != 0) {
      len = match->oxm_length >> 1;
      for (i = 0; i < len; i++) {
        if ((match->oxm_value[i] & ~match->oxm_value[len + i]) != 0) {
          error->type = OFPET_BAD_MATCH;
          error->code = OFPBMC_BAD_WILDCARDS;
          return LAGOPUS_RESULT_OFP_ERROR;
        }
      }
    }
  }
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
flow_action_check(struct bridge *bridge,
                  struct flow *flow,
                  struct ofp_error *error) {
  struct group_table *group_table;
  struct meter_table *meter_table;
  struct instruction *instruction;
  struct action *action;
  struct ofp_action_output *output;
  struct group *group;
  struct meter *meter;
  uint32_t group_id;
  int i;

  group_table = bridge->group_table;
  meter_table = bridge->meter_table;

  for (i = 0; i < INSTRUCTION_INDEX_MAX; i++) {
    instruction = flow->instruction[i];
    if (instruction == NULL) {
      continue;
    }
    switch (instruction->ofpit.type) {
      case OFPIT_METER:
        /* already locked meter table */
        meter = meter_table_lookup(meter_table,
                                   instruction->ofpit_meter.meter_id);
        if (meter != NULL) {
          meter->flow_count++;
        } else {
          error->type = OFPET_METER_MOD_FAILED;
          error->code = OFPMMFC_UNKNOWN_METER;
          lagopus_msg_info("%d: no such meter (%d:%d)",
                           instruction->ofpit_meter.meter_id,
                           error->type, error->code);
          return LAGOPUS_RESULT_OFP_ERROR;
        }
        break;
      case OFPIT_GOTO_TABLE:
        if (instruction->ofpit_goto_table.table_id > OFPTT_MAX) {
          error->type = OFPET_BAD_INSTRUCTION;
          error->code = OFPBIC_BAD_TABLE_ID;
          lagopus_msg_info("%d: bad table id (%d:%d)",
                           instruction->ofpit_goto_table.table_id,
                           error->type, error->code);
          return LAGOPUS_RESULT_OFP_ERROR;
        }
        break;
      case OFPIT_WRITE_ACTIONS:
      case OFPIT_APPLY_ACTIONS:
        TAILQ_FOREACH(action, &instruction->action_list, entry) {
          switch (action->ofpat.type) {
            case OFPAT_OUTPUT:
              /* is exist specified port? */
              output = (struct ofp_action_output *)&action->ofpat;
              switch (output->port) {
                case OFPP_TABLE:
                case OFPP_NORMAL:
                case OFPP_FLOOD:
                case OFPP_ALL:
                case OFPP_CONTROLLER:
                case OFPP_LOCAL:
                  break;

                default:
                  if (port_lookup(&bridge->ports, output->port) == NULL) {
                    error->type = OFPET_BAD_ACTION;
                    error->code = OFPBAC_BAD_OUT_PORT;
                    lagopus_msg_info("%d: no such port (%d:%d)",
                                     output->port, error->type, error->code);
                    return LAGOPUS_RESULT_OFP_ERROR;
                  }
              }
              break;
            case OFPAT_SET_QUEUE:
              /* is exist specified queue? */
              break;
            case OFPAT_GROUP:
              /* is exist specified group? */
              group_id = ((struct ofp_action_group *)&action->ofpat)->group_id;
              group = group_table_lookup(group_table, group_id);
              if (group == NULL) {
                error->type = OFPET_BAD_ACTION;
                error->code = OFPBAC_BAD_OUT_GROUP;
                lagopus_msg_info("%d: group not found (%d:%d)",
                                 group_id, error->type, error->code);
                return LAGOPUS_RESULT_OFP_ERROR;
              }
              /* create relationship with specified group. */
              group_add_ref_flow(group, flow);
              break;
            default:
              break;
          }
        }
    }
  }
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
copy_match_list(struct match_list *dst,
                const struct match_list *src) {
  struct match *src_match;
  struct match *dst_match;
  TAILQ_FOREACH(src_match, src, entry) {
    dst_match = calloc(1, sizeof(struct match) + src_match->oxm_length);
    if (dst_match == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    memcpy(dst_match, src_match, sizeof(struct match) + src_match->oxm_length);
    TAILQ_INSERT_TAIL(dst, dst_match, entry);
  }
  return LAGOPUS_RESULT_OK;
}

/**
 * dislike TAILQ_CONCAT, src is not modified and copied contents.
 */
lagopus_result_t
copy_action_list(struct action_list *dst,
                 const struct action_list *src) {
  const struct action *src_action;
  struct action *dst_action;
  TAILQ_FOREACH(src_action, src, entry) {
    dst_action = calloc(1, sizeof(struct action) + src_action->ofpat.len);
    if (dst_action == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    dst_action->exec = src_action->exec;
    dst_action->cookie = src_action->cookie;
    memcpy(&dst_action->ofpat, &src_action->ofpat, src_action->ofpat.len);
    TAILQ_INSERT_TAIL(dst, dst_action, entry);
  }
  return LAGOPUS_RESULT_OK;
}

/**
 * dislike TAILQ_CONCAT, src is not modified and copied contents.
 */
static lagopus_result_t
copy_instruction_list(struct instruction_list *dst,
                      const struct instruction_list *src) {
  struct instruction *src_inst;
  struct instruction *dst_inst;
  lagopus_result_t rv;

  TAILQ_FOREACH(src_inst, src, entry) {
    dst_inst = calloc(1, sizeof(struct instruction));
    if (dst_inst == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    *dst_inst = *src_inst;
    TAILQ_INIT(&dst_inst->action_list);
    rv = copy_action_list(&dst_inst->action_list, &src_inst->action_list);
    if (rv != LAGOPUS_RESULT_OK) {
      return rv;
    }
    TAILQ_INSERT_TAIL(dst, dst_inst, entry);
  }
  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
flow_remove_with_reason(struct flow *flow,
                        struct bridge *bridge,
                        uint8_t reason,
                        struct ofp_error *error) {
  lagopus_result_t ret;

  (void) error;

  flowdb_wrlock(NULL);
  ret = flow_remove_with_reason_nolock(flow, bridge, reason, error);
  flowdb_wrunlock(NULL);

  return ret;
}

lagopus_result_t
flow_remove_with_reason_nolock(struct flow *flow,
                               struct bridge *bridge,
                               uint8_t reason,
                               struct ofp_error *error) {
  struct table *table;
  struct group_table *group_table;
  struct meter_table *meter_table;
  struct flow_list *flow_list;
  lagopus_result_t ret;
  int i;

  (void) error;

  ret = LAGOPUS_RESULT_OK;

  /* Clear flow cache */
#ifdef HAVE_DPDK
  clear_worker_flowcache(false);
#endif /* HAVE_DPDK */
  clear_rawsock_flowcache();

  group_table = bridge->group_table;
  meter_table = bridge->meter_table;
  table = flowdb_get_table(bridge->flowdb, flow->table_id);

  flow_list = table->flow_list;
  for (i = 0; i < flow_list->nflow; i++) {
    if (flow == flow_list->flows[i]) {
      /* call flowinfo cleanup. */
      if (lagopus_del_flow_hook != NULL) {
        lagopus_del_flow_hook(flow, table);
      }
      flow_del_from_group(group_table, flow);
      flow_del_from_meter(meter_table, flow);
      if ((flow->flags & OFPFF_SEND_FLOW_REM) != 0) {
        /* send OFPT_FLOW_REMOVED message */
        ret = send_flow_removed(bridge->dpid, flow, reason);
      }
      flow_free(flow);
      flow_list->nflow--;
      if (i < flow_list->nflow) {
        memmove(&flow_list->flows[i], &flow_list->flows[i + 1],
                sizeof(struct flow *) *
                (unsigned int)(flow_list->nflow - i));
      }
      goto out;
    }
  }
out:
  return ret;
}

/**
 * true if ml1 has all ml2 matches.
 */
static bool
match_compare(struct match_list *ml1, struct match_list *ml2) {
  struct match *m1, *m2;

  TAILQ_FOREACH(m2, ml2, entry) {
    TAILQ_FOREACH(m1, ml1, entry) {
      if (m1->oxm_class == m2->oxm_class &&
          m1->oxm_field == m2->oxm_field &&
          m1->oxm_length == m2->oxm_length &&
          memcmp(m1->oxm_value, m2->oxm_value, m1->oxm_length) == 0) {
        break;
      }
    }
    if (m1 == NULL) {
      return false;
    }
  }
  return true;
}

static bool
flow_compare(struct flow *f1, struct flow *f2) {
  struct match *m1;
  struct match *m2;

  /* Priority compare. */
  if (f1->priority != f2->priority) {
    return false;
  }

  /* Field bits compare. */
  if (f1->field_bits != f2->field_bits) {
    return false;
  }

  m1 = TAILQ_FIRST(&f1->match_list);
  m2 = TAILQ_FIRST(&f2->match_list);

  while (m1 && m2) {
    if (m1->oxm_class != m2->oxm_class ||
        m1->oxm_field != m2->oxm_field ||
        m1->oxm_length != m2->oxm_length ||
        memcmp(m1->oxm_value, m2->oxm_value, m1->oxm_length) != 0) {
      return false;
    }

    m1 = TAILQ_NEXT(m1, entry);
    m2 = TAILQ_NEXT(m2, entry);
  }

  if (m1 != m2) {
    return false;
  }

  return true;
}

static struct flow *
flow_exist_sub(struct flow *flow, struct flow_list *flow_list) {
  int i;
  bool same;

  for (i = 0; i < flow_list->nflow; i++) {
    same = flow_compare(flow, flow_list->flows[i]);
    if (same == true) {
      return flow_list->flows[i];
    }
  }
  return NULL;
}

static struct action *
flow_action_examination(struct flow *flow,
                        struct action_list *action_list) {
  struct action *action, *action_output, *action_push;
  struct group *group;
  struct bucket *bucket;
  uint32_t group_id;
  int flags;

  flags = 0;
  action_output = NULL;
  action_push = NULL;
  TAILQ_FOREACH(action, action_list, entry) {
    switch (action->ofpat.type) {
      case OFPAT_PUSH_VLAN:
      case OFPAT_PUSH_MPLS:
      case OFPAT_PUSH_PBB:
      case OFPAT_POP_VLAN:
      case OFPAT_POP_MPLS:
      case OFPAT_POP_PBB:
        if (action_output != NULL) {
          action_output->flags = OUTPUT_COPIED_PACKET;
        }
        action_push = action;
        flags = 0;
        break;

      case OFPAT_SET_FIELD:
        if (action_output != NULL) {
          action_output->flags = OUTPUT_COPIED_PACKET;
        }
        if (action_push != NULL) {
          if (GET_OXM_FIELD(&action->ofpat) == OFPXMT_OFB_ETH_DST) {
            flags |= SET_FIELD_ETH_DST;
          } else if (GET_OXM_FIELD(&action->ofpat) == OFPXMT_OFB_ETH_SRC) {
            flags |= SET_FIELD_ETH_SRC;
          }
          action_push->flags = flags;
        }
        break;

      case OFPAT_OUTPUT:
        if (action_output != NULL) {
          action_output->flags = OUTPUT_COPIED_PACKET;
        }
        action_output = action;
        action->cookie = flow->cookie;
        break;

      case OFPAT_GROUP:
        group_id = ((struct ofp_action_group *)&action->ofpat)->group_id;
        group = group_table_lookup(flow->bridge->group_table, group_id);
        if (group == NULL) {
          break;
        }
        TAILQ_FOREACH(bucket, &group->bucket_list, entry) {
          struct action *bucket_output;

          TAILQ_FOREACH(bucket_output,
                        &bucket->actions[LAGOPUS_ACTION_SET_ORDER_OUTPUT],
                        entry) {
            bucket_output->cookie = flow->cookie;
          }
        }
        break;

      default:
        break;
    }
  }

  return action_output;
}

/* Examine apply-action for dataplane. */
static void
flow_instruction_examination(struct flow *flow) {
  struct instruction *instruction;
  struct action *action, *output;
  int i;

  output = NULL;
  for (i = 0; i < INSTRUCTION_INDEX_MAX; i++) {
    instruction = flow->instruction[i];
    if (instruction == NULL) {
      continue;
    }
    if (i > INSTRUCTION_INDEX_APPLY_ACTIONS && output != NULL) {
      /* instructions is exist after output action. */
      output->flags = OUTPUT_COPIED_PACKET;
    }
    if (lagopus_register_instruction_hook != NULL) {
      lagopus_register_instruction_hook(instruction);
    }
    if ((instruction->ofpit.type == OFPIT_APPLY_ACTIONS ||
         instruction->ofpit.type == OFPIT_WRITE_ACTIONS) &&
        lagopus_register_action_hook != NULL) {
      TAILQ_FOREACH(action, &instruction->action_list, entry) {
        lagopus_register_action_hook(action);
      }
    }
    if (instruction->ofpit.type == OFPIT_APPLY_ACTIONS) {
      output = flow_action_examination(flow, &instruction->action_list);
    }
  }
}

lagopus_result_t
flow_add_sub(struct flow *flow, struct flow_list *flows) {
  lagopus_result_t ret;
  int i, st, ed, off;

  ret = LAGOPUS_RESULT_OK;
  if (flows->nflow + 1 > flows->alloced) {
    flows->alloced = (flows->nflow + 1) * 2;
    flows->flows = realloc(flows->flows,
                           (size_t)(flows->alloced) * sizeof(struct flow *));
  }
  if (flows->flows == NULL) {
    ret = LAGOPUS_RESULT_NO_MEMORY;
    goto out;
  }

  /* Entry is sorted by highest priority number. */
  st = 0;
  ed = flows->nflow;
  while (st < ed) {
    off = st + (ed - st) / 2;
    if (flows->flows[off]->priority >= flow->priority) {
      st = off + 1;
    } else {
      ed = off;
    }
  }
  i = ed;
  if (i < flows->nflow) {
    size_t siz;

    siz = (size_t)(flows->nflow - i) * sizeof(struct flow *);
    memmove(&flows->flows[i + 1], &flows->flows[i], siz);
  }
  /* Insert flow into the point. */
  flows->flows[i] = flow;
  flows->nflow++;
out:
  return ret;
}


/* Flow add API. */
lagopus_result_t
flowdb_flow_add(struct bridge *bridge,
                struct ofp_flow_mod *flow_mod,
                struct match_list *match_list,
                struct instruction_list *instruction_list,
                struct ofp_error *error) {
  struct flowdb *flowdb;
  struct table *table;
  struct flow *flow;
  struct flow *identical_flow;
  lagopus_result_t ret = LAGOPUS_RESULT_OK;

  /* OFPIT_ALL is invalid for add. */
  if (flow_mod->table_id == OFPTT_ALL) {
    error->type = OFPET_FLOW_MOD_FAILED;
    error->code = OFPFMFC_BAD_TABLE_ID;
    lagopus_msg_info("flow add: OFPTT_ALL: bad tabld id (%d:%d)",
                     error->type, error->code);
    return LAGOPUS_RESULT_OFP_ERROR;
  }

  flowdb = bridge->flowdb;

  /* Write lock the flowdb. */
  flowdb_wrlock(flowdb);

  /* Get table. */
  table = flowdb_get_table(flowdb, flow_mod->table_id);
  if (table == NULL) {
    error->type = OFPET_FLOW_MOD_FAILED;
    error->code = OFPFMFC_BAD_TABLE_ID;
    lagopus_msg_info("flow add: %d: table not found (%d:%d)",
                     flow_mod->table_id, error->type, error->code);
    ret = LAGOPUS_RESULT_OFP_ERROR;
    goto out;
  }

  /* Allocate a new flow. */
  ret = flow_alloc(flow_mod, match_list, instruction_list, &flow, error);
  if (flow == NULL) {
    goto out;
  }
  flow->bridge = bridge;
  flow->table_id = flow_mod->table_id;

  /* Examine each match entry. When duplicated entry or inconsistent
   * entry is found, return error. */
  ret = flow_pre_requisite_check(flow, &flow->match_list, error);
  if (ret != LAGOPUS_RESULT_OK) {
    flow_free(flow);
    goto out;
  }
  ret = flow_mask_check(&flow->match_list, error);
  if (ret != LAGOPUS_RESULT_OK) {
    flow_free(flow);
    goto out;
  }

  /* Overlapping flow check. */
  if (lagopus_find_flow_hook != NULL) {
    identical_flow = lagopus_find_flow_hook(flow, table);
  } else {
    identical_flow = flow_exist_sub(flow, table->flow_list);
  }
  if (identical_flow != NULL) {
    /* Check if overlapped entry exist.  see 6.4 Flow Table Modification
     * Messages. */
    if (CHECK_FLAG(flow_mod->flags, OFPFF_CHECK_OVERLAP)) {
      flow_free(flow);
      error->type = OFPET_FLOW_MOD_FAILED;
      error->code = OFPFMFC_OVERLAP;
      lagopus_msg_info("flow add: overlapped entry detected (%d:%d)",
                       error->type, error->code);
      ret = LAGOPUS_RESULT_OFP_ERROR;
      goto out;
    }
    ret = map_instruction_list_to_array(flow->instruction,
                                        &flow->instruction_list,
                                        error);
    if (ret != LAGOPUS_RESULT_OK) {
      flow_free(flow);
      goto out;
    }
    /* Examine apply-action for dataplane. */
    flow_instruction_examination(flow);
    /* overriden. */
    flow_del_from_meter(bridge->meter_table, identical_flow);
    flow_del_from_group(bridge->group_table, identical_flow);
    if ((flow_mod->flags & OFPFF_RESET_COUNTS) != 0) {
      identical_flow->packet_count = 0;
      identical_flow->byte_count = 0;
    }
    match_list_entry_free(&identical_flow->match_list);
    instruction_list_entry_free(&identical_flow->instruction_list);
    TAILQ_CONCAT(&identical_flow->match_list,
                 &flow->match_list,
                 entry);
    TAILQ_CONCAT(&identical_flow->instruction_list,
                 &flow->instruction_list,
                 entry);
    flow_free(flow);
    ret = map_instruction_list_to_array(identical_flow->instruction,
                                        &identical_flow->instruction_list,
                                        error);
    if (ret != LAGOPUS_RESULT_OK) {
      goto out;
    }
    ret = flow_action_check(bridge, identical_flow, error);
    if (ret != LAGOPUS_RESULT_OK) {
      goto out;
    }
    flow = identical_flow;
  } else {
    ret = flow_action_check(bridge, flow, error);
    if (ret != LAGOPUS_RESULT_OK) {
      goto out;
    }
    /* Examine apply-action for dataplane. */
    flow_instruction_examination(flow);
    ret = flow_add_sub(flow, table->flow_list);
    if (ret != LAGOPUS_RESULT_OK) {
      goto out;
    }
    if (lagopus_add_flow_hook != NULL) {
      lagopus_add_flow_hook(flow, table);
    }
    if (flow->idle_timeout > 0 || flow->hard_timeout > 0) {
      add_flow_timer(flow);
    }
#ifdef USE_MBTREE
    if (table->flow_list->update_timer != NULL) {
      *table->flow_list->update_timer = NULL;
    }
    add_mbtree_timer(table->flow_list, UPDATE_TIMEOUT);
#endif /* USE_MBTREE */
#ifdef USE_THTABLE
    if (table->flow_list->update_timer != NULL) {
      *table->flow_list->update_timer = NULL;
    }
    add_thtable_timer(table->flow_list, UPDATE_TIMEOUT);
#endif /* USE_MBTREE */
  }

  /* Clear flow cache */
#ifdef HAVE_DPDK
  clear_worker_flowcache(false);
#endif /* HAVE_DPDK */
  clear_rawsock_flowcache();

out:
  /* Unlock the flowdb then return result. */
  flowdb_wrunlock(flowdb);
  return ret;
}

static lagopus_result_t
flow_modify_sub(struct bridge *bridge,
                struct ofp_flow_mod *flow_mod,
                struct flow_list *flow_list,
                struct match_list *match_list,
                struct instruction_list *instruction_list,
                struct ofp_error *error,
                int strict) {
  struct flow *flow;
  lagopus_result_t ret;
  int i;

  ret = flow_alloc(flow_mod, match_list, instruction_list, &flow, error);
  if (flow == NULL) {
    goto out;
  }
  ret = flow_pre_requisite_check(flow, &flow->match_list, error);
  if (ret != LAGOPUS_RESULT_OK) {
    flow_free(flow);
    goto out;
  }
  ret = map_instruction_list_to_array(flow->instruction,
                                      &flow->instruction_list,
                                      error);
  if (ret != LAGOPUS_RESULT_OK) {
    flow_free(flow);
    goto out;
  }
  flow_instruction_examination(flow);
  if (strict) {
    /*
     * strict. modify identical flow specified by flow_mod.
     */
    for (i = 0; i < flow_list->nflow; i++) {
      if (flow_compare(flow, flow_list->flows[i]) == true) {
        flow_del_from_meter(bridge->meter_table, flow_list->flows[i]);
        flow_del_from_group(bridge->group_table, flow_list->flows[i]);
        if ((flow_mod->flags & OFPFF_RESET_COUNTS) != 0) {
          flow_list->flows[i]->packet_count = 0;
          flow_list->flows[i]->byte_count = 0;
        }
        instruction_list_entry_free(&flow_list->flows[i]->instruction_list);
        copy_instruction_list(&flow_list->flows[i]->instruction_list,
                              &flow->instruction_list);
        map_instruction_list_to_array(flow_list->flows[i]->instruction,
                                      &flow_list->flows[i]->instruction_list,
                                      error);
        if (ret != LAGOPUS_RESULT_OK) {
          goto out;
        }
        ret = flow_action_check(bridge, flow, error);
        if (ret != LAGOPUS_RESULT_OK) {
          goto out;
        }
        flow_instruction_examination(flow);
        break;
      }
    }
    flow_free(flow);
  } else {
    /*
     * not strict. modify all flows if matched by match_list and cookie.
     */
    TAILQ_CONCAT(match_list, &flow->match_list, entry);
    TAILQ_CONCAT(instruction_list, &flow->instruction_list, entry);
    flow_free(flow);

    for (i = 0; i < flow_list->nflow; i++) {
      flow = flow_list->flows[i];
      /* filtering by cookie */
      if (flow_mod->cookie_mask != 0) {
        if ((flow->cookie & flow_mod->cookie_mask)
            != (flow_mod->cookie & flow_mod->cookie_mask)) {
          continue;
        }
      }
      /* filtering by output port and group are not supported yet */
      if (match_compare(&flow->match_list, match_list) == true) {
        flow_del_from_meter(bridge->meter_table, flow);
        flow_del_from_group(bridge->group_table, flow);
        if ((flow_mod->flags & OFPFF_RESET_COUNTS) != 0) {
          flow->packet_count = 0;
          flow->byte_count = 0;
        }
        instruction_list_entry_free(&flow->instruction_list);
        ret = copy_instruction_list(&flow->instruction_list,
                                    instruction_list);
        if (ret != LAGOPUS_RESULT_OK) {
          break;
        }
        ret = map_instruction_list_to_array(flow->instruction,
                                            &flow->instruction_list,
                                            error);
        if (ret != LAGOPUS_RESULT_OK) {
          break;
        }
        ret = flow_action_check(bridge, flow, error);
        if (ret != LAGOPUS_RESULT_OK) {
          goto out;
        }
        flow_instruction_examination(flow);
      }
    }
    instruction_list_entry_free(instruction_list);
  }
out:
  return ret;
}

static void
flow_del_from_meter(struct meter_table *meter_table, struct flow *flow) {
  struct instruction *inst;
  struct meter *meter;

  if (meter_table == NULL) {
    /* nothing to do. */
    return;
  }
  /* remove from meter */
  inst = flow->instruction[INSTRUCTION_INDEX_METER];
  if (inst != NULL) {
    meter = meter_table_lookup(meter_table, inst->ofpit_meter.meter_id);
    if (meter != NULL) {
      meter->flow_count--;
    }
  }
}

static void
flow_del_from_group(struct group_table *group_table, struct flow *flow) {
  struct instruction *instruction;
  struct action *action;
  struct group *group;
  uint32_t group_id;
  int i;

  if (group_table == NULL) {
    /* nothing to do. */
    return;
  }

  /* remove from group */
  for (i = 0; i < INSTRUCTION_INDEX_MAX; i++) {
    instruction = flow->instruction[i];
    if (instruction == NULL) {
      continue;
    }
    switch (instruction->ofpit.type) {
      case OFPIT_WRITE_ACTIONS:
      case OFPIT_APPLY_ACTIONS:
        TAILQ_FOREACH(action, &instruction->action_list, entry) {
          if (action->ofpat.type == OFPAT_GROUP) {
            group_id = ((struct ofp_action_group *)&action->ofpat)->group_id;
            group = group_table_lookup(group_table, group_id);
            group_remove_ref_flow(group, flow);
            break;
          }
        }
        break;
    }
  }
}

static void
flow_removed_free(struct eventq_data *data) {
  if (data != NULL) {
    ofp_match_list_elem_free(&data->flow_removed.match_list);
    free(data);
  }
}

static lagopus_result_t
send_flow_removed(uint64_t dpid,
                  struct flow *flow,
                  uint8_t reason) {
  struct timespec ts;
  struct flow_removed *flow_removed;
  struct eventq_data *eventq_data;
  lagopus_result_t rv;

  eventq_data = malloc(sizeof(*eventq_data));
  if (eventq_data == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  flow_removed = &eventq_data->flow_removed;
  if (flow_removed == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  /*flow_removed->ofp_flow_removed.header;*/
  flow_removed->ofp_flow_removed.cookie = flow->cookie;
  flow_removed->ofp_flow_removed.priority = (uint16_t)flow->priority;
  flow_removed->ofp_flow_removed.reason = reason;
  flow_removed->ofp_flow_removed.table_id = flow->table_id;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  flow_removed->ofp_flow_removed.duration_sec =
    (uint32_t)(ts.tv_sec - flow->create_time.tv_sec);
  if (ts.tv_nsec < flow->create_time.tv_nsec) {
    flow_removed->ofp_flow_removed.duration_sec--;
    flow_removed->ofp_flow_removed.duration_nsec = 1 * 1000 * 1000 * 1000;
  } else {
    flow_removed->ofp_flow_removed.duration_nsec = 0;
  }
  flow_removed->ofp_flow_removed.duration_nsec += (uint32_t)ts.tv_nsec;
  flow_removed->ofp_flow_removed.duration_nsec -=
    (uint32_t)flow->create_time.tv_nsec;

  flow_removed->ofp_flow_removed.idle_timeout = flow->idle_timeout;
  flow_removed->ofp_flow_removed.hard_timeout = flow->hard_timeout;
  if ((flow->flags & OFPFF_NO_PKT_COUNTS) == 0) {
    flow_removed->ofp_flow_removed.packet_count = flow->packet_count;
  } else {
    flow_removed->ofp_flow_removed.packet_count = 0xffffffffffffffff;
  }
  if ((flow->flags & OFPFF_NO_BYT_COUNTS) == 0) {
    flow_removed->ofp_flow_removed.byte_count = flow->byte_count;
  } else {
    flow_removed->ofp_flow_removed.byte_count = 0xffffffffffffffff;
  }
  TAILQ_INIT(&flow_removed->match_list);
  TAILQ_CONCAT(&flow_removed->match_list, &flow->match_list, entry);
  eventq_data->type = LAGOPUS_EVENTQ_FLOW_REMOVED;
  eventq_data->free = flow_removed_free;
  rv = dp_eventq_data_put(dpid, &eventq_data, PUT_TIMEOUT);
  if (rv != LAGOPUS_RESULT_OK) {
    lagopus_perror(rv);
  }
  return rv;
}

static bool
find_action_output(struct flow *flow, uint32_t out_port) {
  int i;

  for (i = 0; i < INSTRUCTION_INDEX_MAX; i++) {
    struct instruction *instruction;

    instruction = flow->instruction[i];
    if (instruction == NULL) {
      continue;
    }
    if (instruction->ofpit.type == OFPIT_WRITE_ACTIONS ||
        instruction->ofpit.type == OFPIT_APPLY_ACTIONS) {
      struct action *action;

      TAILQ_FOREACH(action, &instruction->action_list, entry) {
        if (action->ofpat.type == OFPAT_OUTPUT) {
          /* is exist specified port? */
          struct ofp_action_output *output;

          output = (struct ofp_action_output *)&action->ofpat;
          if (output->port == out_port) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

static bool
find_action_group(struct flow *flow, uint32_t out_group) {
  int i;

  for (i = 0; i < INSTRUCTION_INDEX_MAX; i++) {
    struct instruction *instruction;

    instruction = flow->instruction[i];
    if (instruction == NULL) {
      continue;
    }
    if (instruction->ofpit.type == OFPIT_WRITE_ACTIONS ||
        instruction->ofpit.type == OFPIT_APPLY_ACTIONS) {
      struct action *action;

      TAILQ_FOREACH(action, &instruction->action_list, entry) {
        if (action->ofpat.type == OFPAT_GROUP) {
          /* is exist specified group? */
          uint32_t group_id;

          group_id = ((struct ofp_action_group *)&action->ofpat)->group_id;
          if (out_group == group_id) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

static lagopus_result_t
flow_del_sub(struct bridge *bridge,
             struct table *table,
             struct ofp_flow_mod *flow_mod,
             struct flow_list *flow_list,
             struct match_list *match_list,
             int strict,
             struct ofp_error *error) {
  struct group_table *group_table;
  struct meter_table *meter_table;
  struct instruction_list instruction_list;
  struct flow *flow;
  lagopus_result_t ret;
  int i;

  ret = LAGOPUS_RESULT_OK;
  group_table = bridge->group_table;
  meter_table = bridge->meter_table;

  if (strict) {
    /*
     * strict. delete identical flow specified by flow_mod.
     */
    TAILQ_INIT(&instruction_list);
    ret = flow_alloc(flow_mod, match_list, &instruction_list, &flow, error);
    if (flow == NULL) {
      goto out;
    }
    ret = flow_pre_requisite_check(flow, &flow->match_list, error);
    if (ret != LAGOPUS_RESULT_OK) {
      goto out;
    }
    for (i = 0; i < flow_list->nflow; i++) {
      if (flow_compare(flow, flow_list->flows[i]) == true) {
        if (lagopus_del_flow_hook != NULL) {
          lagopus_del_flow_hook(flow_list->flows[i], table);
        }
        flow_del_from_group(group_table, flow_list->flows[i]);
        flow_del_from_meter(meter_table, flow_list->flows[i]);
        if ((flow_list->flows[i]->flags & OFPFF_SEND_FLOW_REM) != 0) {
          /* send OFPT_FLOW_REMOVED message */
          ret = send_flow_removed(bridge->dpid, flow, OFPRR_DELETE);
        }
        flow_free(flow_list->flows[i]);
        flow_list->nflow--;
        if (i < flow_list->nflow) {
          memmove(&flow_list->flows[i], &flow_list->flows[i + 1],
                  sizeof(struct flow *) *
                  (unsigned int)(flow_list->nflow - i));
        }
        break;
      }
    }
    flow_free(flow);
#ifdef USE_MBTREE
    if (flow_list->update_timer != NULL) {
      *flow_list->update_timer = NULL;
    }
    add_mbtree_timer(flow_list, UPDATE_TIMEOUT);
#endif /* USE_MBTREE */
#ifdef USE_THTABLE
    if (flow_list->update_timer != NULL) {
      *flow_list->update_timer = NULL;
    }
    add_thtable_timer(flow_list, UPDATE_TIMEOUT);
#endif /* USE_THTABLEE */
  } else {
    int new_nflow;

    /*
     * not strict. delete all flows if matched by match_list and cookie.
     */
    flow_list = table->flow_list;
    new_nflow = flow_list->nflow;
    for (i = 0; i < flow_list->nflow; i++) {
      flow = flow_list->flows[i];
      /* filtering by cookie */
      if (flow_mod->cookie_mask != 0) {
        if ((flow->cookie & flow_mod->cookie_mask)
            != (flow_mod->cookie & flow_mod->cookie_mask)) {
          continue;
        }
      }
      if (flow_mod->out_port != OFPP_ANY) {
        if (find_action_output(flow, flow_mod->out_port) != true) {
          continue;
        }
      }
      if (flow_mod->out_group != OFPG_ANY) {
        if (find_action_group(flow, flow_mod->out_group) != true) {
          continue;
        }
      }
      /* filtering by output port and group are not supported yet */
      if (match_compare(&flow->match_list, match_list) == true) {
        if (lagopus_del_flow_hook != NULL) {
          lagopus_del_flow_hook(flow, table);
        }
        flow_del_from_group(group_table, flow);
        flow_del_from_meter(meter_table, flow);
        if ((flow->flags & OFPFF_SEND_FLOW_REM) != 0) {
          /* send OFPT_FLOW_REMOVED message */
          ret = send_flow_removed(bridge->dpid, flow, OFPRR_DELETE);
        }
        flow_list->flows[i] = NULL;
        flow_free(flow);
        new_nflow--;
#ifdef USE_MBTREE
        if (flow_list->update_timer != NULL) {
          *flow_list->update_timer = NULL;
        }
        add_mbtree_timer(flow_list, UPDATE_TIMEOUT);
#endif /* USE_MBTREE */
#ifdef USE_THTABLE
        if (flow_list->update_timer != NULL) {
          *flow_list->update_timer = NULL;
        }
        add_thtable_timer(flow_list, UPDATE_TIMEOUT);
#endif /* USE_THTABLE */
      }
    }
    /* compaction. */
    for (i = 0; i < flow_list->nflow; i++) {
      int st, ed;

      if (flow_list->flows[i] == NULL) {
        for (st = i; st < flow_list->nflow; st++) {
          if (flow_list->flows[st] != NULL) {
            break;
          }
        }
        if (st == flow_list->nflow) {
          break;
        }
        for (ed = st + 1; ed < flow_list->nflow; ed++) {
          if (flow_list->flows[ed] == NULL) {
            break;
          }
        }
        memmove(&flow_list->flows[i], &flow_list->flows[st],
                sizeof(struct flow *) * (unsigned int)(ed - st));
        i = ed - 1;
      }
    }
    flow_list->nflow = new_nflow;
  }
out:
  return ret;
}

static int
table_flow_modify(struct bridge *bridge, struct table *table,
                  struct flow *flow,
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  struct ofp_error *error,
                  int strict) {
  flow_modify_sub(bridge, flow_mod,
                  table->flow_list,
                  match_list, instruction_list,
                  error, strict);
  return LAGOPUS_RESULT_OK;
}

static void
table_flow_delete(struct bridge *bridge,
                  struct table *table, struct flow *flow,
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  int strict,
                  struct ofp_error *error) {
  flow_del_sub(bridge, table, flow_mod,
               table->flow_list, match_list,
               strict, error);
}


lagopus_result_t
flowdb_flow_modify(struct bridge *bridge,
                   struct ofp_flow_mod *flow_mod,
                   struct match_list *match_list,
                   struct instruction_list *instruction_list,
                   struct ofp_error *error) {
  struct flow flow;
  struct table *table;
  lagopus_result_t result;
  int strict;

  /* Set flow parameters. */
  flow.priority = flow_mod->priority;
  flow.flags = flow_mod->flags;

  /* Examine flow. */
  result = flow_pre_requisite_check(&flow, match_list, error);
  if (result != LAGOPUS_RESULT_OK) {
    return result;
  }

  /* Strict check. */
  strict = (flow_mod->command == OFPFC_MODIFY_STRICT ? 1 : 0);

  /* OFPIT_ALL is invalid for modify. */
  if (flow_mod->table_id == OFPTT_ALL) {
    error->type = OFPET_FLOW_MOD_FAILED;
    error->code = OFPFMFC_BAD_TABLE_ID;
    lagopus_msg_info("flow modify: OFPTT_ALL: bad table id (%d:%d)",
                     error->type, error->code);
    return LAGOPUS_RESULT_OFP_ERROR;
  }

  /* Write lock the flowdb. */
  flowdb_wrlock(bridge->flowdb);

  /* Get table. */
  table = flowdb_get_table(bridge->flowdb, flow_mod->table_id);
  if (table == NULL) {
    error->type = OFPET_FLOW_MOD_FAILED;
    error->code = OFPFMFC_BAD_TABLE_ID;
    lagopus_msg_info("flow modify: %d: table not found (%d:%d)",
                     flow_mod->table_id, error->type, error->code);
    result = LAGOPUS_RESULT_OFP_ERROR;
    goto out;
  }

  /* Modify table. */
  result = table_flow_modify(bridge, table, &flow, flow_mod,
                             match_list, instruction_list,
                             error, strict);

  /* Clear flow cache */
#ifdef HAVE_DPDK
  clear_worker_flowcache(false);
#endif /* HAVE_DPDK */
  clear_rawsock_flowcache();

  /* Unlock the flowdb and return result. */
out:
  flowdb_wrunlock(bridge->flowdb);
  return result;
}

lagopus_result_t
flowdb_flow_delete(struct bridge *bridge,
                   struct ofp_flow_mod *flow_mod,
                   struct match_list *match_list,
                   struct ofp_error *error) {
  struct flowdb *flowdb;
  int i;
  struct flow flow;
  lagopus_result_t result;
  struct table *table;
  int strict;

  /* Set flow parameters. */
  flow.priority = flow_mod->priority;
  flow.flags = flow_mod->flags;

  /* Examine flow.  fixed flow_type. */
  result = flow_pre_requisite_check(&flow, match_list, error);
  if (result != LAGOPUS_RESULT_OK) {
    return result;
  }

  /* Strict check. */
  strict = (flow_mod->command == OFPFC_DELETE_STRICT ? 1 : 0);

  flowdb = bridge->flowdb;

  /* Write lock the flowdb. */
  flowdb_wrlock(flowdb);

  /* OFPTT_ALL means targeting all tables. */
  if (flow_mod->table_id == OFPTT_ALL) {
    for (i = 0; i < flowdb->table_size; i++) {
      table = flowdb->tables[i];
      if (table == NULL) {
        continue;
      }
      table_flow_delete(bridge,
                        table, &flow, flow_mod, match_list,
                        strict, error);
    }
  } else {
    /* Lookup table using table_id. */
    table = flowdb_get_table(flowdb, flow_mod->table_id);
    if (table == NULL) {
      error->type = OFPET_FLOW_MOD_FAILED;
      error->code = OFPFMFC_BAD_TABLE_ID;
      lagopus_msg_info("flow delete: %d: table not found (%d:%d)",
                       flow_mod->table_id, error->type, error->code);
      result = LAGOPUS_RESULT_OFP_ERROR;
      goto out;
    }
    table_flow_delete(bridge,
                      table, &flow, flow_mod, match_list,
                      strict, error);
  }

  /* Clear flow cache */
#ifdef HAVE_DPDK
  clear_worker_flowcache(false);
#endif /* HAVE_DPDK */
  clear_rawsock_flowcache();

  /* Unlock the flowdb and return result. */
out:
  flowdb_wrunlock(flowdb);
  return result;
}

static lagopus_result_t
table_flow_stats(struct table *table,
                 int table_id,
                 struct ofp_flow_stats_request *request,
                 struct match_list *match_list,
                 struct flow_stats_list *flow_stats_list) {
  struct timespec ts;
  struct flow_stats *flow_stats;
  struct flow_list *flow_list;
  struct flow *flow;
  int i;
  lagopus_result_t rv;

  rv = LAGOPUS_RESULT_OK;

  flow_list = table->flow_list;
  for (i = 0; i < flow_list->nflow; i++) {
    flow = flow_list->flows[i];
    if (request->cookie_mask != 0) {
      if ((flow->cookie & request->cookie_mask) !=
          (request->cookie & request->cookie_mask)) {
        continue;
      }
    }
    if (match_compare(&flow->match_list, match_list) == true) {
      /* make flow stats. */
      flow_stats = calloc(1, sizeof(struct flow_stats));
      if (flow_stats == NULL) {
        goto out;
      }
      flow_stats->ofp.table_id = (uint8_t)table_id;
#define COPY_STATS(member) flow_stats->ofp.member = flow->member
      COPY_STATS(idle_timeout);
      COPY_STATS(hard_timeout);
      flow_stats->ofp.priority = (uint16_t)flow->priority;
      COPY_STATS(flags);
      COPY_STATS(cookie);
      if ((flow->flags & OFPFF_NO_PKT_COUNTS) == 0) {
        COPY_STATS(packet_count);
      } else {
        flow_stats->ofp.packet_count =  0xffffffffffffffff;
      }
      if ((flow->flags & OFPFF_NO_BYT_COUNTS) == 0) {
        COPY_STATS(byte_count);
      } else {
        flow_stats->ofp.byte_count = 0xffffffffffffffff;
      }
#undef COPY_STATS

      clock_gettime(CLOCK_MONOTONIC, &ts);
      flow_stats->ofp.duration_sec =
        (uint32_t)(ts.tv_sec - flow->create_time.tv_sec);
      if (ts.tv_nsec < flow->create_time.tv_nsec) {
        flow_stats->ofp.duration_sec--;
        flow_stats->ofp.duration_nsec = 1 * 1000 * 1000 * 1000;
      } else {
        flow_stats->ofp.duration_nsec = 0;
      }
      flow_stats->ofp.duration_nsec += (uint32_t)ts.tv_nsec;
      flow_stats->ofp.duration_nsec -= (uint32_t)flow->create_time.tv_nsec;

      /* copy lists. */
      TAILQ_INIT(&flow_stats->match_list);
      copy_match_list(&flow_stats->match_list, &flow->match_list);
      TAILQ_INIT(&flow_stats->instruction_list);
      copy_instruction_list(&flow_stats->instruction_list,
                            &flow->instruction_list);

      /* and link to list. */
      TAILQ_INSERT_TAIL(flow_stats_list, flow_stats, entry);
    }
  }
out:
  return rv;
}

lagopus_result_t
flowdb_flow_stats(struct flowdb *flowdb,
                  struct ofp_flow_stats_request *request,
                  struct match_list *match_list,
                  struct flow_stats_list *flow_stats_list,
                  struct ofp_error *error) {
  struct table *table;
  lagopus_result_t rv;

  rv = LAGOPUS_RESULT_OK;

  /* Write lock the flowdb. */
  flowdb_wrlock(flowdb);

  if (request->table_id == OFPTT_ALL) {
    int i;

    for (i = 0; i < flowdb->table_size; i++) {
      table = flowdb->tables[i];
      if (table != NULL) {
        rv = table_flow_stats(table, i, request, match_list, flow_stats_list);
        if (rv != LAGOPUS_RESULT_OK) {
          goto out;
        }
      }
    }
  } else {
    /* Lookup table using table_id. */
    table = flowdb_get_table(flowdb, request->table_id);
    if (table == NULL) {
      error->type = OFPET_BAD_REQUEST;
      error->code = OFPBRC_BAD_TABLE_ID;
      lagopus_msg_info("flow stats: %d: table not found (%d:%d)",
                       request->table_id, error->type, error->code);
      rv = LAGOPUS_RESULT_OFP_ERROR;
      goto out;
    }
    rv = table_flow_stats(table, request->table_id, request,
                          match_list, flow_stats_list);
  }

  /* Unlock the flowdb and return result. */
out:
  flowdb_wrunlock(flowdb);
  return rv;
}

static void
table_flow_counts(struct table *table,
                  struct ofp_flow_stats_request *request,
                  struct match_list *match_list,
                  struct ofp_aggregate_stats_reply *reply) {

  struct flow_list *flow_list;
  struct flow *flow;
  int i;

  flow_list = table->flow_list;
  for (i = 0; i < flow_list->nflow; i++) {
    flow = flow_list->flows[i];
    if (request->cookie_mask != 0) {
      if ((flow->cookie & request->cookie_mask) !=
          (request->cookie & request->cookie_mask)) {
        continue;
      }
    }
    if (match_compare(&flow->match_list, match_list) == true) {

      if ((flow->flags & OFPFF_NO_PKT_COUNTS) == 0) {
        reply->packet_count += flow->packet_count;
      }
      if ((flow->flags & OFPFF_NO_BYT_COUNTS) == 0) {
        reply->byte_count += flow->byte_count;
      }
      reply->flow_count++;
    }
  }
}

lagopus_result_t
flowdb_aggregate_stats(struct flowdb *flowdb,
                       struct ofp_aggregate_stats_request *request,
                       struct match_list *match_list,
                       struct ofp_aggregate_stats_reply *reply,
                       struct ofp_error *error) {
  struct table *table;
  lagopus_result_t result;

  result = LAGOPUS_RESULT_OK;

  /* Write lock the flowdb. */
  flowdb_wrlock(flowdb);

  reply->packet_count = 0;
  reply->byte_count = 0;
  reply->flow_count = 0;
  if (request->table_id == OFPTT_ALL) {
    int i;

    for (i = 0; i < flowdb->table_size; i++) {
      table = flowdb->tables[i];
      if (table != NULL) {
        table_flow_counts(table, request, match_list, reply);
      }
    }
  } else {
    /* Lookup table using table_id. */
    table = flowdb_get_table(flowdb, request->table_id);
    if (table == NULL) {
      error->type = OFPET_BAD_REQUEST;
      error->code = OFPBRC_BAD_TABLE_ID;
      lagopus_msg_info("flow aggr stats: %d: table not found (%d:%d)",
                       request->table_id, error->type, error->code);
      result = LAGOPUS_RESULT_OFP_ERROR;
      goto out;
    }
    table_flow_counts(table, request, match_list, reply);
  }

  /* Unlock the flowdb and return result. */
out:
  flowdb_wrunlock(flowdb);
  return result;
}

lagopus_result_t
flowdb_table_stats(struct flowdb *flowdb,
                   struct table_stats_list *list,
                   struct ofp_error *error) {
  int table_id;

  (void)error;

  for (table_id = 0; table_id <= UINT8_MAX; table_id++) {
    struct table_stats *stats;
    struct table *table;

    table = flowdb->tables[table_id];
    if (table == NULL) {
      continue;
    }
    stats = calloc(1, sizeof(struct table_stats));
    if (stats == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    stats->ofp.table_id = (uint8_t)table_id;
    stats->ofp.active_count = (uint32_t)table->flow_list->nflow;
    stats->ofp.lookup_count = table->lookup_count;
    stats->ofp.matched_count = table->matched_count;
    TAILQ_INSERT_TAIL(list, stats, entry);
  }
  return LAGOPUS_RESULT_OK;
}

static struct table_property *
alloc_table_prop(uint16_t type) {
  struct table_property *prop;

  prop = calloc(1, sizeof(struct table_property));
  if (prop == NULL) {
    return NULL;
  }
  prop->ofp.type = type;
  TAILQ_INIT(&prop->instruction_list);
  TAILQ_INIT(&prop->next_table_id_list);
  TAILQ_INIT(&prop->action_list);
  TAILQ_INIT(&prop->oxm_id_list);
  prop->experimenter = 0;
  prop->exp_type;
  TAILQ_INIT(&prop->experimenter_data_list);

  return prop;
}

static uint16_t inst_type[] = {
  OFPIT_GOTO_TABLE,
  OFPIT_WRITE_METADATA,
  OFPIT_WRITE_ACTIONS,
  OFPIT_APPLY_ACTIONS,
  OFPIT_CLEAR_ACTIONS,
  OFPIT_METER,
  0
};

static struct table_property *
get_table_prop_instructions(__UNUSED struct table *table) {
  struct table_property *prop;

  prop = alloc_table_prop(OFPTFPT_INSTRUCTIONS);
  if (prop != NULL) {
    struct instruction *inst;
    uint16_t *typep;

    for (typep = inst_type; *typep != 0; typep++) {
      inst = calloc(1, sizeof(struct instruction));
      if (inst != NULL) {
        inst->ofpit.type = *typep;
        TAILQ_INSERT_TAIL(&prop->instruction_list, inst, entry);
      }
    }
  }
  return prop;
}

static struct table_property *
get_table_prop_next_tables(struct table *table) {
  struct table_property *prop;

  prop = alloc_table_prop(OFPTFPT_NEXT_TABLES);
  if (prop != NULL) {
    /* OFPTT_MAX is reserved for internal use. */
    uint8_t id;

    for (id = table->table_id + 1; id < OFPTT_MAX; id++) {
      struct next_table_id *nxt;

      nxt = calloc(1, sizeof(struct next_table_id));
      if (nxt != NULL) {
        nxt->ofp.id = id;
        TAILQ_INSERT_TAIL(&prop->next_table_id_list, nxt, entry);
      }
    }
  }

  return prop;
}

static int action_type[] = {
  OFPAT_OUTPUT,
  OFPAT_COPY_TTL_OUT,
  OFPAT_COPY_TTL_IN,
  OFPAT_SET_MPLS_TTL,
  OFPAT_DEC_MPLS_TTL,
  OFPAT_PUSH_VLAN,
  OFPAT_POP_VLAN,
  OFPAT_PUSH_MPLS,
  OFPAT_POP_MPLS,
  /*OFPAT_SET_QUEUE,*/
  OFPAT_GROUP,
  OFPAT_SET_NW_TTL,
  OFPAT_DEC_NW_TTL,
  OFPAT_SET_FIELD,
  OFPAT_PUSH_PBB,
  OFPAT_POP_PBB,
  -1
};

static struct table_property *
get_table_prop_write_actions(__UNUSED struct table *table) {
  struct table_property *prop;

  prop = alloc_table_prop(OFPTFPT_WRITE_ACTIONS);
  if (prop != NULL) {
    struct action *act;
    int *typep;

    for (typep = action_type; *typep != -1; typep++) {
      act = calloc(1, sizeof(struct action));
      if (act != NULL) {
        act->ofpat.type = (uint16_t)*typep;
        TAILQ_INSERT_TAIL(&prop->action_list, act, entry);
      }
    }
  }
  return prop;
}

static struct table_property *
get_table_prop_apply_actions(__UNUSED struct table *table) {
  struct table_property *prop;

  prop = alloc_table_prop(OFPTFPT_APPLY_ACTIONS);
  if (prop != NULL) {
    struct action *act;
    int *typep;

    for (typep = action_type; *typep != -1; typep++) {
      act = calloc(1, sizeof(struct action));
      if (act != NULL) {
        act->ofpat.type = (uint16_t)*typep;
        TAILQ_INSERT_TAIL(&prop->action_list, act, entry);
      }
    }
  }

  return prop;
}

static uint32_t oxm_ids[] = {
  OXM_OF_IN_PORT,
  OXM_OF_IN_PHY_PORT,
  OXM_OF_METADATA,
  OXM_OF_METADATA_W,
  OXM_OF_ETH_DST,
  OXM_OF_ETH_DST_W,
  OXM_OF_ETH_SRC,
  OXM_OF_ETH_SRC_W,
  OXM_OF_ETH_TYPE,
  OXM_OF_VLAN_VID,
  OXM_OF_VLAN_VID_W,
  OXM_OF_VLAN_PCP,
  OXM_OF_IP_DSCP,
  OXM_OF_IP_ECN,
  OXM_OF_IP_PROTO,
  OXM_OF_IPV4_SRC,
  OXM_OF_IPV4_SRC_W,
  OXM_OF_IPV4_DST,
  OXM_OF_IPV4_DST_W,
  OXM_OF_TCP_SRC,
  OXM_OF_TCP_DST,
  OXM_OF_UDP_SRC,
  OXM_OF_UDP_DST,
  OXM_OF_SCTP_SRC,
  OXM_OF_SCTP_DST,
  OXM_OF_ICMPV4_TYPE,
  OXM_OF_ICMPV4_CODE,
  OXM_OF_ARP_OP,
  OXM_OF_ARP_SPA,
  OXM_OF_ARP_SPA_W,
  OXM_OF_ARP_TPA,
  OXM_OF_ARP_TPA_W,
  OXM_OF_ARP_SHA,
  OXM_OF_ARP_THA,
  OXM_OF_IPV6_SRC,
  OXM_OF_IPV6_SRC_W,
  OXM_OF_IPV6_DST,
  OXM_OF_IPV6_DST_W,
  OXM_OF_IPV6_FLABEL,
  OXM_OF_ICMPV6_TYPE,
  OXM_OF_ICMPV6_CODE,
  OXM_OF_IPV6_ND_TARGET,
  OXM_OF_IPV6_ND_SLL,
  OXM_OF_IPV6_ND_TLL,
  OXM_OF_MPLS_LABEL,
  OXM_OF_MPLS_TC,
  OXM_OF_MPLS_BOS,
  OXM_OF_PBB_ISID,
  OXM_OF_PBB_ISID_W,
  OXM_OF_TUNNEL_ID,
  OXM_OF_TUNNEL_ID_W,
  OXM_OF_IPV6_EXTHDR,
  OXM_OF_IPV6_EXTHDR_W,
  0
};

static struct table_property *
get_table_prop_match(__UNUSED struct table *table) {
  struct table_property *prop;

  prop = alloc_table_prop(OFPTFPT_MATCH);
  if (prop != NULL) {
    struct oxm_id *oxm;
    uint32_t *idp;

    for (idp = oxm_ids; *idp != 0; idp++) {
      oxm = calloc(1, sizeof(struct oxm_id));
      if (oxm != NULL) {
        oxm->ofp.id = *idp;
        TAILQ_INSERT_TAIL(&prop->oxm_id_list, oxm, entry);
      }
    }
  }

  return prop;
}

static struct table_property *
get_table_prop_wildcards(__UNUSED struct table *table) {
  struct table_property *prop;

  prop = alloc_table_prop(OFPTFPT_WILDCARDS);
  if (prop != NULL) {
    struct oxm_id *oxm;
    uint32_t *idp;

    for (idp = oxm_ids; *idp != 0; idp++) {
      oxm = calloc(1, sizeof(struct oxm_id));
      if (oxm != NULL) {
        oxm->ofp.id = *idp;
        TAILQ_INSERT_TAIL(&prop->oxm_id_list, oxm, entry);
      }
    }
  }

  return prop;
}

static uint32_t set_field_oxm_ids[] = {
  OXM_OF_IN_PORT,
  OXM_OF_METADATA,
  OXM_OF_ETH_DST,
  OXM_OF_ETH_SRC,
  OXM_OF_ETH_TYPE,
  OXM_OF_VLAN_VID,
  OXM_OF_VLAN_PCP,
  OXM_OF_IP_DSCP,
  OXM_OF_IP_ECN,
  OXM_OF_IP_PROTO,
  OXM_OF_IPV4_SRC,
  OXM_OF_IPV4_DST,
  OXM_OF_TCP_SRC,
  OXM_OF_TCP_DST,
  OXM_OF_UDP_SRC,
  OXM_OF_UDP_DST,
  OXM_OF_SCTP_SRC,
  OXM_OF_SCTP_DST,
  OXM_OF_ICMPV4_TYPE,
  OXM_OF_ICMPV4_CODE,
  OXM_OF_ARP_OP,
  OXM_OF_ARP_SPA,
  OXM_OF_ARP_TPA,
  OXM_OF_ARP_SHA,
  OXM_OF_ARP_THA,
  OXM_OF_IPV6_SRC,
  OXM_OF_IPV6_DST,
  OXM_OF_IPV6_FLABEL,
  OXM_OF_ICMPV6_TYPE,
  OXM_OF_ICMPV6_CODE,
  OXM_OF_IPV6_ND_TARGET,
  OXM_OF_IPV6_ND_SLL,
  OXM_OF_IPV6_ND_TLL,
  OXM_OF_MPLS_LABEL,
  OXM_OF_MPLS_TC,
  OXM_OF_MPLS_BOS,
  OXM_OF_PBB_ISID,
  OXM_OF_TUNNEL_ID,
  0
};

static struct table_property *
get_table_prop_write_setfield(__UNUSED struct table *table) {
  struct table_property *prop;

  prop = alloc_table_prop(OFPTFPT_WRITE_SETFIELD);
  if (prop != NULL) {
    struct oxm_id *oxm;
    uint32_t *idp;

    for (idp = set_field_oxm_ids; *idp != 0; idp++) {
      oxm = calloc(1, sizeof(struct oxm_id));
      if (oxm != NULL) {
        oxm->ofp.id = *idp;
        TAILQ_INSERT_TAIL(&prop->oxm_id_list, oxm, entry);
      }
    }
  }

  return prop;
}

static struct table_property *
get_table_prop_apply_setfield(__UNUSED struct table *table) {
  struct table_property *prop;

  prop = alloc_table_prop(OFPTFPT_APPLY_SETFIELD);
  if (prop != NULL) {
    struct oxm_id *oxm;
    uint32_t *idp;

    for (idp = set_field_oxm_ids; *idp != 0; idp++) {
      oxm = calloc(1, sizeof(struct oxm_id));
      if (oxm != NULL) {
        oxm->ofp.id = *idp;
        TAILQ_INSERT_TAIL(&prop->oxm_id_list, oxm, entry);
      }
    }
  }

  return prop;
}

#ifdef notyet
static struct table_property *
get_table_prop_experimenter(__UNUSED struct table *table) {
  struct table_property *prop;

  prop = alloc_table_prop(OFPTFPT_EXPERIMENTER);

  return prop;
}
#endif /* notyet */

typedef struct table_property *(*prop_func_t)(struct table *table);

/* *_miss functions are omitted because it is same as regular entry. */
static prop_func_t prop_funcs[] = {
  get_table_prop_instructions,
  get_table_prop_next_tables,
  get_table_prop_write_actions,
  get_table_prop_apply_actions,
  get_table_prop_match,
  get_table_prop_wildcards,
  get_table_prop_write_setfield,
  get_table_prop_apply_setfield,
#ifdef notyet
  get_table_prop_experimenter,
#endif /* notyet */
  NULL
};

static void
get_table_props(struct table *table, struct table_property_list *prop_list) {
  struct table_property *prop;
  prop_func_t *funcp;

  for (funcp = prop_funcs; *funcp != NULL; funcp++) {
    prop = (*funcp)(table);
    if (prop != NULL) {
      TAILQ_INSERT_TAIL(prop_list, prop, entry);
    }
  }
}

lagopus_result_t
flowdb_get_table_features(struct flowdb *flowdb,
                          struct table_features_list *list,
                          struct ofp_error *error) {
  struct table_features *features;
  struct table *table;
  int i;

  (void) error;

  /* Read lock the flowdb. */
  flowdb_rdlock(flowdb);

  for (i = 0; i < flowdb->table_size; i++) {
    table = flowdb->tables[i];
    if (table != NULL) {
      features = calloc(1, sizeof(struct table_features));
      if (features == NULL) {
        flowdb_rdunlock(flowdb);
        return LAGOPUS_RESULT_NO_MEMORY;
      }
      memcpy(&features->ofp,
             &table->features,
             sizeof(struct ofp_table_features));
      /* properties */
      TAILQ_INIT(&features->table_property_list);
      get_table_props(table, &features->table_property_list);

      TAILQ_INSERT_TAIL(list, features, entry);
    }
  }
  flowdb_rdunlock(flowdb);

  return LAGOPUS_RESULT_OK;
}

static void
flow_dump_match(uint8_t oxm_field, uint8_t oxm_length, uint8_t *oxm_value,
                const char *indent, FILE *fp) {
  uint8_t field, hasmask;
  char ipaddr[INET6_ADDRSTRLEN];
  const char *field_str;
  enum printer {
    PRINTER_UNKNOWN = 0,
    PRINTER_U8,
    PRINTER_U16,
    PRINTER_U24,
    PRINTER_U32,
    PRINTER_U64,
    PRINTER_X16,
    PRINTER_X24,
    PRINTER_X64,
    PRINTER_ETHADDR,
    PRINTER_IPV4ADDR,
    PRINTER_IPV6ADDR,
  } printer;
  static const struct {
    const char *field_str;
    enum printer printer;
  } printer_table[] = {
    [OFPXMT_OFB_IN_PORT]        = { "in_port",          PRINTER_U32 },
    [OFPXMT_OFB_IN_PHY_PORT]    = { "in_phy_port",      PRINTER_U32 },
    [OFPXMT_OFB_METADATA]       = { "metadata",         PRINTER_X64 },
    [OFPXMT_OFB_ETH_DST]        = { "eth_dst",          PRINTER_ETHADDR },
    [OFPXMT_OFB_ETH_SRC]        = { "eth_src",          PRINTER_ETHADDR },
    [OFPXMT_OFB_ETH_TYPE]       = { "eth_type",         PRINTER_X16 },
    [OFPXMT_OFB_VLAN_VID]       = { "vlan_vid",         PRINTER_U16 },
    [OFPXMT_OFB_VLAN_PCP]       = { "vlan_pcp",         PRINTER_U8 },
    [OFPXMT_OFB_IP_DSCP]        = { "ip_dscp",          PRINTER_U8 },
    [OFPXMT_OFB_IP_ECN]         = { "ip_ecn",           PRINTER_U8 },
    [OFPXMT_OFB_IP_PROTO]       = { "ip_proto",         PRINTER_U8 },
    [OFPXMT_OFB_IPV4_SRC]       = { "ipv4_src",         PRINTER_IPV4ADDR },
    [OFPXMT_OFB_IPV4_DST]       = { "ipv4_dst",         PRINTER_IPV4ADDR },
    [OFPXMT_OFB_TCP_SRC]        = { "tcp_src",          PRINTER_U16 },
    [OFPXMT_OFB_TCP_DST]        = { "tcp_dst",          PRINTER_U16 },
    [OFPXMT_OFB_UDP_SRC]        = { "udp_src",          PRINTER_U16 },
    [OFPXMT_OFB_UDP_DST]        = { "udp_dst",          PRINTER_U16 },
    [OFPXMT_OFB_SCTP_SRC]       = { "sctp_src",         PRINTER_U16 },
    [OFPXMT_OFB_SCTP_DST]       = { "sctp_dst",         PRINTER_U16 },
    [OFPXMT_OFB_ICMPV4_TYPE]    = { "icmpv4_type",      PRINTER_U8 },
    [OFPXMT_OFB_ICMPV4_CODE]    = { "icmpv4_code",      PRINTER_U8 },
    [OFPXMT_OFB_ARP_OP]         = { "arp_op",           PRINTER_U16 },
    [OFPXMT_OFB_ARP_SPA]        = { "arp_spa",          PRINTER_IPV4ADDR },
    [OFPXMT_OFB_ARP_TPA]        = { "arp_tpa",          PRINTER_IPV4ADDR },
    [OFPXMT_OFB_ARP_SHA]        = { "arp_sha",          PRINTER_ETHADDR },
    [OFPXMT_OFB_ARP_THA]        = { "arp_tha",          PRINTER_ETHADDR },
    [OFPXMT_OFB_IPV6_SRC]       = { "ipv6_src",         PRINTER_IPV6ADDR },
    [OFPXMT_OFB_IPV6_DST]       = { "ipv6_dst",         PRINTER_IPV6ADDR },
    [OFPXMT_OFB_IPV6_FLABEL]    = { "ipv6_flabel",      PRINTER_U24 },
    [OFPXMT_OFB_ICMPV6_TYPE]    = { "icmpv6_type",      PRINTER_U8 },
    [OFPXMT_OFB_ICMPV6_CODE]    = { "icmpv6_code",      PRINTER_U8 },
    [OFPXMT_OFB_IPV6_ND_TARGET] = { "ipv6_nd_target",   PRINTER_IPV6ADDR },
    [OFPXMT_OFB_IPV6_ND_SLL]    = { "ipv6_nd_sll",      PRINTER_ETHADDR },
    [OFPXMT_OFB_IPV6_ND_TLL]    = { "ipv6_nd_tll",      PRINTER_ETHADDR },
    [OFPXMT_OFB_MPLS_LABEL]     = { "mpls_label",       PRINTER_U24 },
    [OFPXMT_OFB_MPLS_TC]        = { "mpls_tc",          PRINTER_U8 },
    [OFPXMT_OFB_MPLS_BOS]       = { "mpls_bos",         PRINTER_U8 },
    [OFPXMT_OFB_PBB_ISID]       = { "pbb_isid",         PRINTER_X24 },
    [OFPXMT_OFB_TUNNEL_ID]      = { "tunnel_id",        PRINTER_U64 },
    [OFPXMT_OFB_IPV6_EXTHDR]    = { "ipv6_exthdr",      PRINTER_U16 },
  };

  field = oxm_field >> 1;
  hasmask = oxm_field & 1;

  if (field < (sizeof(printer_table) / sizeof(printer_table[0]))) {
    field_str = printer_table[field].field_str;
    printer = printer_table[field].printer;
  } else {
    field_str = "unknown_field";
    printer = PRINTER_UNKNOWN;
  }

  switch (printer) {
    case PRINTER_U8:
      fprintf(fp, "%s%s=%d", indent, field_str, *oxm_value);
      if (hasmask) {
        fprintf(fp, "/0x%02x", *(oxm_value + 1));
      }
      fprintf(fp, "\n");
      break;
    case PRINTER_U16:
      fprintf(fp, "%s%s=%d", indent, field_str, ntohs(*(uint16_t *)oxm_value));
      if (hasmask) {
        fprintf(fp, "/0x%04x", ntohs(*((uint16_t *)oxm_value + 1)));
      }
      fprintf(fp, "\n");
      break;
    case PRINTER_U24:
      if (hasmask) {
        fprintf(fp, "%s%s=%d/0x%06x\n", indent, field_str,
                (uint32_t)oxm_value[oxm_length / 2 - 3] << 16
                | (uint32_t)oxm_value[oxm_length / 2 - 2] << 8
                | (uint32_t)oxm_value[oxm_length / 2 - 1],
                (uint32_t)oxm_value[oxm_length - 3] << 16
                | (uint32_t)oxm_value[oxm_length - 2] << 8
                | (uint32_t)oxm_value[oxm_length - 1]);
      } else {
        fprintf(fp, "%s%s=%d\n", indent, field_str,
                (uint32_t)oxm_value[oxm_length - 3] << 16
                | (uint32_t)oxm_value[oxm_length - 2] << 8
                | (uint32_t)oxm_value[oxm_length - 1]);
      }
      break;
    case PRINTER_U32:
      fprintf(fp, "%s%s=%d", indent, field_str, ntohl(*(uint32_t *)oxm_value));
      if (hasmask) {
        fprintf(fp, "/0x%08x", ntohl(*((uint32_t *)oxm_value + 1)));
      }
      fprintf(fp, "\n");
      break;
    case PRINTER_U64:
      fprintf(fp, "%s%s=%" PRIu64, indent, field_str,
              (uint64_t)oxm_value[0] << 56
              | (uint64_t)oxm_value[1] << 48
              | (uint64_t)oxm_value[2] << 40
              | (uint64_t)oxm_value[3] << 32
              | (uint64_t)oxm_value[4] << 24
              | (uint64_t)oxm_value[5] << 16
              | (uint64_t)oxm_value[6] << 8
              | (uint64_t)oxm_value[7]);
      if (hasmask) {
        fprintf(fp, "/0x%016" PRIx64,
                (uint64_t)oxm_value[8] << 56
                | (uint64_t)oxm_value[9] << 48
                | (uint64_t)oxm_value[10] << 40
                | (uint64_t)oxm_value[11] << 32
                | (uint64_t)oxm_value[12] << 24
                | (uint64_t)oxm_value[13] << 16
                | (uint64_t)oxm_value[14] << 8
                | (uint64_t)oxm_value[15]);
      }
      fprintf(fp, "\n");
      break;
    case PRINTER_X16:
      fprintf(fp, "%s%s=0x%04x", indent, field_str,
              ntohs(*(uint16_t *)oxm_value));
      if (hasmask) {
        fprintf(fp, "/0x%04x", ntohs(*((uint16_t *)oxm_value + 1)));
      }
      fprintf(fp, "\n");
      break;
    case PRINTER_X24:
      if (hasmask) {
        fprintf(fp, "%s%s=0x%06x/0x%06x\n", indent, field_str,
                (uint32_t)oxm_value[oxm_length / 2 - 3] << 16
                | (uint32_t)oxm_value[oxm_length / 2 - 2] << 8
                | (uint32_t)oxm_value[oxm_length / 2 - 1],
                (uint32_t)oxm_value[oxm_length - 3] << 16
                | (uint32_t)oxm_value[oxm_length - 2] << 8
                | (uint32_t)oxm_value[oxm_length - 1]);
      } else {
        fprintf(fp, "%s%s=0x%06x\n", indent, field_str,
                (uint32_t)oxm_value[oxm_length - 3] << 16
                | (uint32_t)oxm_value[oxm_length - 2] << 8
                | (uint32_t)oxm_value[oxm_length - 1]);
      }
      break;
    case PRINTER_X64:
      fprintf(fp, "%s%s=0x%016" PRIx64, indent, field_str,
              (uint64_t)oxm_value[0] << 56
              | (uint64_t)oxm_value[1] << 48
              | (uint64_t)oxm_value[2] << 40
              | (uint64_t)oxm_value[3] << 32
              | (uint64_t)oxm_value[4] << 24
              | (uint64_t)oxm_value[5] << 16
              | (uint64_t)oxm_value[6] << 8
              | (uint64_t)oxm_value[7]);
      if (hasmask) {
        fprintf(fp, "/0x%016" PRIx64,
                (uint64_t)oxm_value[8] << 56
                | (uint64_t)oxm_value[9] << 48
                | (uint64_t)oxm_value[10] << 40
                | (uint64_t)oxm_value[11] << 32
                | (uint64_t)oxm_value[12] << 24
                | (uint64_t)oxm_value[13] << 16
                | (uint64_t)oxm_value[14] << 8
                | (uint64_t)oxm_value[15]);
      }
      fprintf(fp, "\n");
      break;
    case PRINTER_ETHADDR:
      fprintf(fp, "%s%s=%02x:%02x:%02x:%02x:%02x:%02x", indent, field_str,
              oxm_value[0], oxm_value[1], oxm_value[2],
              oxm_value[3], oxm_value[4], oxm_value[5]);
      if (hasmask) {
        fprintf(fp, "/%02x:%02x:%02x:%02x:%02x:%02x",
                oxm_value[6], oxm_value[7], oxm_value[8],
                oxm_value[9], oxm_value[10], oxm_value[11]);
      }
      fprintf(fp, "\n");
      break;
    case PRINTER_IPV4ADDR:
      fprintf(fp, "%s%s=%s", indent, field_str,
              inet_ntop(AF_INET, oxm_value, ipaddr, sizeof(ipaddr)));
      if (hasmask) {
        fprintf(fp, "/%s",
                inet_ntop(AF_INET, &oxm_value[4], ipaddr, sizeof(ipaddr)));
      }
      fprintf(fp, "\n");
      break;
    case PRINTER_IPV6ADDR:
      fprintf(fp, "%s%s=%s", indent, field_str,
              inet_ntop(AF_INET6, oxm_value, ipaddr, sizeof(ipaddr)));
      if (hasmask) {
        fprintf(fp, "/%s",
                inet_ntop(AF_INET6, &oxm_value[4], ipaddr, sizeof(ipaddr)));
      }
      break;
    case PRINTER_UNKNOWN:
    default:
      fprintf(fp, "%s%s\n", indent, field_str);
      break;
  }
}

void
flow_dump(struct flow *flow, FILE *fp) {
  struct instruction *instruction;
  struct action *action;
  struct match *match;

  fprintf(fp, " flow(table %d, priority %d)\n",
          flow->table_id, flow->priority);
  if (TAILQ_FIRST(&flow->match_list) != NULL) {
    fprintf(fp, "  match\n");
    TAILQ_FOREACH(match, &flow->match_list, entry) {
      if (match->oxm_class == OFPXMC_OPENFLOW_BASIC) {
        flow_dump_match(match->oxm_field, match->oxm_length, match->oxm_value,
                        "   ", fp);
      }
    }
  }

  TAILQ_FOREACH(instruction, &flow->instruction_list, entry) {
    switch (instruction->ofpit.type) {
      case OFPIT_WRITE_ACTIONS:
        fprintf(fp, "  instruction write_actions\n");
        goto dump_actions;
      case OFPIT_APPLY_ACTIONS:
        fprintf(fp, "  instruction apply_actions\n");
        goto dump_actions;
      case OFPIT_CLEAR_ACTIONS:
        fprintf(fp, "  instruction clear_actions\n");
      dump_actions:
        TAILQ_FOREACH(action, &instruction->action_list, entry) {
          struct ofp_action_header *ofpat = &action->ofpat;

          switch (ofpat->type) {
            case OFPAT_OUTPUT: {
              struct ofp_action_output *output;

              output = (struct ofp_action_output *)ofpat;
              fprintf(fp, "   action OUTPUT\n");
              fprintf(fp, "    port: %d\n", output->port);
              if (output->port == OFPP_CONTROLLER) {
                fprintf(fp, "    max_len: %d\n", output->max_len);
              }
            }
            break;
            case OFPAT_COPY_TTL_OUT:
              fprintf(fp, "   action COPY_TTL_OUT\n");
              break;
            case OFPAT_COPY_TTL_IN:
              fprintf(fp, "   action COPY_TTL_IN\n");
              break;
            case OFPAT_SET_MPLS_TTL:
              fprintf(fp, "   action SET_MPLS_TTL\n");
              fprintf(fp, "    ttl: %d\n",
                      ((struct ofp_action_mpls_ttl *)ofpat)->mpls_ttl);
              break;
            case OFPAT_DEC_MPLS_TTL:
              fprintf(fp, "   action DEC_MPLS_TTL\n");
              break;
            case OFPAT_PUSH_VLAN:
              fprintf(fp, "   action PUSH_VLAN\n");
              fprintf(fp, "    ethertype: 0x%04x\n",
                      ((struct ofp_action_push *)ofpat)->ethertype);
              break;
            case OFPAT_POP_VLAN:
              fprintf(fp, "   action POP_VLAN\n");
              break;
            case OFPAT_PUSH_MPLS:
              fprintf(fp, "   action PUSH_MPLS\n");
              fprintf(fp, "    ethertype: 0x%04x\n",
                      ((struct ofp_action_push *)ofpat)->ethertype);
              break;
            case OFPAT_POP_MPLS:
              fprintf(fp, "   action POP_MPLS\n");
              fprintf(fp, "    ethertype: 0x%04x\n",
                      ((struct ofp_action_pop_mpls *)ofpat)->ethertype);
              break;
            case OFPAT_SET_QUEUE:
              fprintf(fp, "   action SET_QUEUE\n");
              fprintf(fp, "    queue_id: %d\n",
                      ((struct ofp_action_set_queue *)ofpat)->queue_id);
              break;
            case OFPAT_GROUP:
              fprintf(fp, "   action GROUP\n");
              fprintf(fp, "    group_id: %d\n",
                      ((struct ofp_action_group *)ofpat)->group_id);
              break;
            case OFPAT_SET_NW_TTL:
              fprintf(fp, "   action SET_NW_TTL\n");
              fprintf(fp, "    nw_ttl: %d\n",
                      ((struct ofp_action_nw_ttl *)ofpat)->nw_ttl);
              break;
            case OFPAT_DEC_NW_TTL:
              fprintf(fp, "   action DEC_NW_TTL\n");
              break;
            case OFPAT_SET_FIELD: {
              struct ofp_action_set_field *set_field;
              struct ofp_oxm *oxm;

              set_field = (struct ofp_action_set_field *)ofpat;
              oxm = (struct ofp_oxm *)set_field->field;
              fprintf(fp, "   action SET_FIELD\n");
              if (oxm->oxm_class == htons(OFPXMC_OPENFLOW_BASIC)) {
                flow_dump_match(oxm->oxm_field, oxm->oxm_length,
                                oxm->oxm_value, "    ", fp);
              }
            }
            break;
            case OFPAT_PUSH_PBB:
              fprintf(fp, "   action PUSH_PBB\n");
              fprintf(fp, "    ethertype: 0x%04x\n",
                      ((struct ofp_action_push *)ofpat)->ethertype);
              break;
            case OFPAT_POP_PBB:
              fprintf(fp, "   action POP_PBB\n");
              break;
            case OFPAT_EXPERIMENTER: {
              struct ofp_action_experimenter_header *experimenter;

              experimenter = (struct ofp_action_experimenter_header *)ofpat;
              fprintf(fp, "   action EXPERIMENTER\n");
              fprintf(fp, "    experimenter: %d\n",
                      experimenter->experimenter);
            }
            break;
            default:
              break;
          }
        }
        break;
      case OFPIT_GOTO_TABLE:
        fprintf(fp, "  instruction goto_table\n");
        fprintf(fp, "   table_id: %d\n",
                instruction->ofpit_goto_table.table_id);
        break;
      case OFPIT_WRITE_METADATA:
        fprintf(fp, "  instruction write_metadata\n");
        fprintf(fp, "   metadata: 0x%016" PRIx64 "/0x%016" PRIx64 "\n",
                instruction->ofpit_write_metadata.metadata,
                instruction->ofpit_write_metadata.metadata_mask);
        break;
      case OFPIT_METER:
        fprintf(fp, "  instruction meter\n");
        fprintf(fp, "   meter_id: %d\n", instruction->ofpit_meter.meter_id);
        break;
      case OFPIT_EXPERIMENTER:
        fprintf(fp, "  instruction experimenter\n");
        fprintf(fp, "   experimenter: %d\n",
                instruction->ofpit_experimenter.experimenter);
        break;
      default:
        break;
    }
  }
}

void
flowdb_dump(struct flowdb *flowdb, FILE *fp) {
  int i, j;
  struct table *table;
  struct flow_list *flow_list;

  flowdb_rdlock(flowdb);

  for (i = 0; i < flowdb->table_size; i++) {
    table = flowdb->tables[i];
    if (table != NULL) {
      fprintf(fp, "table %d\n", i);

      flow_list = table->flow_list;
      for (j = 0; j < flow_list->nflow; j++) {
        flow_dump(flow_list->flows[j], fp);
      }
    }
  }

  flowdb_rdunlock(flowdb);
}

void
ofp_match_list_elem_free(struct match_list *match_list) {
  match_list_entry_free(match_list);
}

void
ofp_instruction_list_elem_free(struct instruction_list *instruction_list) {
  instruction_list_entry_free(instruction_list);
}

void
ofp_action_list_elem_free(struct action_list *action_list) {
  action_list_entry_free(action_list);
}

lagopus_result_t
ofp_flow_mod_check_add(uint64_t dpid,
                       struct ofp_flow_mod *flow_mod,
                       struct match_list *match_list,
                       struct instruction_list *instruction_list,
                       struct ofp_error *error) {
  struct bridge *bridge;
  lagopus_result_t ret;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  ret =  flowdb_flow_add(bridge,
                         flow_mod,
                         match_list, instruction_list,
                         error);
  return ret;
}

lagopus_result_t
ofp_flow_mod_modify(uint64_t dpid,
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct instruction_list *instruction_list,
                    struct ofp_error *error) {
  struct bridge *bridge;
  lagopus_result_t ret;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  switch (flow_mod->command) {
    case OFPFC_MODIFY:
    case OFPFC_MODIFY_STRICT:
      ret = flowdb_flow_modify(bridge, flow_mod,
                               match_list, instruction_list,
                               error);
      break;
    default:
      error->type = OFPET_FLOW_MOD_FAILED;
      error->code = OFPFMFC_BAD_COMMAND;
      lagopus_msg_info("flow mod: %d: bad command (%d:%d)",
                       flow_mod->command, error->type, error->code);
      ret = LAGOPUS_RESULT_OFP_ERROR;
  }

  return ret;
}

lagopus_result_t
ofp_flow_mod_delete(uint64_t dpid,
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct ofp_error *error) {
  struct bridge *bridge;
  lagopus_result_t ret;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  switch (flow_mod->command) {
    case OFPFC_DELETE:
    case OFPFC_DELETE_STRICT:
      ret = flowdb_flow_delete(bridge, flow_mod, match_list, error);
      break;
    default:
      error->type = OFPET_FLOW_MOD_FAILED;
      error->code = OFPFMFC_BAD_COMMAND;
      lagopus_msg_info("flow mod: %d: bad command (%d:%d)",
                       flow_mod->command, error->type, error->code);
      ret = LAGOPUS_RESULT_OFP_ERROR;
  }

  return ret;
}

/*
 * flow_stats (Agent/DP API)
 */
lagopus_result_t
ofp_flow_stats_get(uint64_t dpid,
                   struct ofp_flow_stats_request *flow_stats_request,
                   struct match_list *match_list,
                   struct flow_stats_list *flow_stats_list,
                   struct ofp_error *error) {
  struct bridge *bridge;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return flowdb_flow_stats(bridge->flowdb, flow_stats_request, match_list,
                           flow_stats_list, error);
}

/*
 * table_stats (Agent/DP API)
 */
lagopus_result_t
ofp_table_stats_get(uint64_t dpid,
                    struct table_stats_list *table_stats_list,
                    struct ofp_error *error) {
  struct bridge *bridge;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return flowdb_table_stats(bridge->flowdb,
                            table_stats_list,
                            error);
}

/*
 * aggregate_stats (Agent/DP API)
 */
lagopus_result_t
ofp_aggregate_stats_reply_get(uint64_t dpid,
                              struct ofp_aggregate_stats_request *request,
                              struct match_list *match_list,
                              struct ofp_aggregate_stats_reply *aggre_reply,
                              struct ofp_error *error) {
  struct bridge *bridge;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return flowdb_aggregate_stats(bridge->flowdb,
                                request,
                                match_list,
                                aggre_reply,
                                error);
}

/*
 * table_features (Agent/DP API)
 */
lagopus_result_t
ofp_table_features_get(uint64_t dpid,
                       struct table_features_list *table_features_list,
                       struct ofp_error *error) {
  struct bridge *bridge;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (bridge == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }

  return flowdb_get_table_features(bridge->flowdb, table_features_list, error);
}

lagopus_result_t
ofp_table_features_set(uint64_t dpid,
                       struct table_features_list *table_features_list,
                       struct ofp_error *error) {
  (void) dpid;
  (void) table_features_list;
  /* so far, we cannot modify table features. */
  error->type = OFPET_BAD_REQUEST;
  error->code = OFPBRC_BAD_LEN;
  lagopus_msg_info("So far, cannot modify table features (%d:%d)",
                   error->type, error->code);
  return LAGOPUS_RESULT_OFP_ERROR;
}

/*
 * table_mod (Agent/DP API)
 */
lagopus_result_t
ofp_table_mod_set(uint64_t dpid,
                  struct ofp_table_mod *table_mod,
                  struct ofp_error *error) {
  (void) dpid;
  (void) table_mod;
  (void) error;
  return LAGOPUS_RESULT_OK;
}
