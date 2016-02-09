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

#include "lagopus_apis.h"
#include "datastore_apis.h"
#include "datastore_internal.h"
#include "cmd_common.h"
#include "cmd_dump.h"
#include "flow_cmd.h"
#include "flow_cmd_internal.h"
#include "conv_json.h"
#include "lagopus/flowdb.h"
#include "lagopus/ethertype.h"
#include "../agent/ofp_instruction.h"
#include "../agent/ofp_action.h"
#include "../agent/ofp_match.h"
#include "../agent/ofp_oxm.h"
#include "../agent/ofp_error_handler.h"

static lagopus_hashmap_t flow_mod_cmd_table = NULL;
static lagopus_hashmap_t flow_mod_action_cmd_table = NULL;
static lagopus_hashmap_t flow_mod_action_set_field_cmd_table = NULL;
static lagopus_hashmap_t flow_mod_port_str_table = NULL;
static lagopus_hashmap_t flow_mod_dl_type_str_table = NULL;
static lagopus_hashmap_t flow_mod_group_str_table = NULL;
static lagopus_hashmap_t flow_mod_table_str_table = NULL;

typedef lagopus_result_t
(*flow_mod_cmd_proc_t)(const char *const *argv[],
                       struct ofp_flow_mod *flow_mod,
                       struct match_list *match_list,
                       struct instruction_list *instruction_list,
                       enum flow_cmd_type ftype,
                       lagopus_dstring_t *result);

typedef lagopus_result_t
(*value_hook_proc_t)(uint8_t oxm_length,
                     void *oxm_value,
                     lagopus_dstring_t *result);

#define TOKEN_MAX 8192
#define ACTION_TOKEN_LIMIT 1
#define FIELD_VALUE_WITH_MASK_NUM 2

/*
 * parse hooks.
 */
static lagopus_result_t
value_uint16_hook(uint8_t oxm_length,
                  void *oxm_value,
                  lagopus_dstring_t *result) {
  uint16_t *p = (uint16_t *) oxm_value;
  uint16_t val = *p;
  (void) oxm_length;
  (void) result;

  *p = htons(val);

  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
value_uint32_hook(uint8_t oxm_length,
                  void *oxm_value,
                  lagopus_dstring_t *result) {
  uint32_t *p = (uint32_t *) oxm_value;
  uint32_t val = *p;
  (void) oxm_length;
  (void) result;

  *p = htonl(val);

  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
value_uint24_hook(uint8_t oxm_length,
                  void *oxm_value,
                  lagopus_dstring_t *result) {
  uint32_t *p = (uint32_t *) oxm_value;
  uint32_t val = *p;
  (void) oxm_length;
  (void) result;

  *p = hton24(val);

  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
value_uint64_hook(uint8_t oxm_length,
                  void *oxm_value,
                  lagopus_dstring_t *result) {
  uint64_t *p = (uint64_t *) oxm_value;
  uint64_t val = (uint64_t) *p;
  (void) oxm_length;
  (void) result;

  *p = htonll(val);

  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
value_vlan_vid_hook(uint8_t oxm_length,
                    void *oxm_value,
                    lagopus_dstring_t *result) {
  uint16_t *p = (uint16_t *) oxm_value;
  uint16_t val = *p;
  (void) oxm_length;
  (void) result;

  val |= OFPVID_PRESENT;
  *p = htons(val);

  return LAGOPUS_RESULT_OK;
}

/* add flow match. */
static inline lagopus_result_t
flow_cmd_match_add_mask(struct match *match,
                        uint8_t oxm_length,
                        size_t index,
                        void *value,
                        value_hook_proc_t hook_proc,
                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (hook_proc != NULL) {
    ret = (hook_proc)(oxm_length, value, result);
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_set(result, ret,
                                      NULL);
      goto done;
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }
  memcpy(&match->oxm_value[index], value, oxm_length);

done:
  return ret;
}

static inline lagopus_result_t
flow_cmd_match_add(struct match_list *match_list,
                   uint8_t oxm_field,
                   uint8_t oxm_length,
                   void *oxm_value,
                   void *oxm_mask,
                   value_hook_proc_t hook_proc,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t field = (uint8_t) (oxm_field << 1);
  uint8_t len = oxm_length;
  struct match *match = NULL;
  struct match *mp = NULL;

  if (oxm_mask != NULL) {
    field |= 1;
    len = (uint8_t) (len + oxm_length);
  }

  match = match_alloc(len);
  if (match != NULL) {
    match->oxm_class = OFPXMC_OPENFLOW_BASIC;
    match->oxm_field = field;
    match->oxm_length = len;

    /* Add to match list.  We sort match entry from the smallest
     * to the largest. */
    for (mp = TAILQ_FIRST(match_list); mp != NULL;
         mp = TAILQ_NEXT(mp, entry)) {
      if (OXM_FIELD_TYPE(match->oxm_field) <
          OXM_FIELD_TYPE(mp->oxm_field)) {
        break;
      }
    }
    if (mp != NULL) {
      TAILQ_INSERT_BEFORE(mp, match, entry);
    } else {
      TAILQ_INSERT_TAIL(match_list, match, entry);
    }

    /* value. */
    if ((ret = flow_cmd_match_add_mask(match, oxm_length,
                                       0, oxm_value,
                                       hook_proc, result)) !=
        LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add oxm value.");
      goto done;
    }

    /* mask */
    if (oxm_mask != NULL) {
      if ((ret = flow_cmd_match_add_mask(match, oxm_length,
                                         oxm_length, oxm_mask,
                                         hook_proc, result)) !=
          LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't add oxm mask.");
        goto done;
      }
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_NO_MEMORY,
                                    NULL);
  }

done:
  return ret;
}

static inline void
action_output_set(struct ofp_action_output *action,
                  void *value) {
  action->port = *((uint32_t *) value);
  action->max_len = UINT16_MAX;
}

static inline void
action_mpls_ttl_set(struct ofp_action_mpls_ttl *action,
                        void *value) {
  action->mpls_ttl = *((uint8_t *) value);
}

static inline void
action_push_set(struct ofp_action_push *action,
                void *value) {
  action->ethertype = *((uint16_t *) value);
}

static inline void
action_pop_mpls_set(struct ofp_action_pop_mpls *action,
                    void *value) {
  action->ethertype = *((uint16_t *) value);
}

static inline void
action_set_queue_set(struct ofp_action_set_queue *action,
                     void *value) {
  action->queue_id = *((uint32_t *) value);
}

static inline void
action_group_set(struct ofp_action_group *action,
                 void *value) {
  action->group_id = *((uint32_t *) value);
}

static inline void
action_nw_ttl_set(struct ofp_action_nw_ttl *action,
                  void *value) {
  action->nw_ttl = *((uint8_t *) value);
}

/* add flow mod field. */
static inline lagopus_result_t
flow_cmd_flow_field_add(struct ofp_flow_mod *flow_mod,
                        uint16_t field,
                        void *value,
                        void *mask,
                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  switch (field) {
    case FLOW_FIELD_COOKIE:
      flow_mod->cookie = *((uint64_t *) value);
      if (mask != NULL) {
        flow_mod->cookie_mask = *((uint64_t *) mask);
      }
      break;
    case FLOW_FIELD_TABLE:
      flow_mod->table_id = *((uint8_t *) value);
      break;
    case FLOW_FIELD_IDLE_TIMEOUT:
      flow_mod->idle_timeout = *((uint16_t *) value);
      break;
    case FLOW_FIELD_HARD_TIMEOUT:
      flow_mod->hard_timeout = *((uint16_t *) value);
      break;
    case FLOW_FIELD_PRIORITY:
      flow_mod->priority = *((uint16_t *) value);
      break;
    case FLOW_FIELD_OUT_PORT:
      flow_mod->out_port = *((uint32_t *) value);
      break;
    case FLOW_FIELD_OUT_GROUP:
      flow_mod->out_group = *((uint32_t *) value);
      break;
    case FLOW_FIELD_SEND_FLOW_REM:
      flow_mod->flags |= OFPFF_SEND_FLOW_REM;
      break;
    case FLOW_FIELD_CHECK_OVERLAP:
      flow_mod->flags |= OFPFF_CHECK_OVERLAP;
      break;
    default:
      ret = datastore_json_result_string_setf(
          result,
          LAGOPUS_RESULT_NOT_FOUND,
          "Not found flow mod field type(%"PRIu16").",
          field);
      goto done;
  }
  ret = LAGOPUS_RESULT_OK;

done:
  return ret;
}

/* add flow action (without set_field). */
static inline lagopus_result_t
flow_cmd_action_add(struct instruction_list *instruction_list,
                    uint16_t field,
                    uint16_t length,
                    void *value,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct instruction *instruction = NULL;
  struct action *action = NULL;

  instruction = TAILQ_LAST(instruction_list, instruction_list);
  if (instruction != NULL) {
    action = action_alloc(length);

    if (action != NULL) {
      TAILQ_INSERT_TAIL(&instruction->action_list, action, entry);

      action->ofpat.type = field;
      action->ofpat.len = (uint16_t) (sizeof(struct ofp_action_header) +
                                      length);

      switch (action->ofpat.type) {
        case OFPAT_OUTPUT:
          action_output_set((struct ofp_action_output *) &action->ofpat,
                            value);
          break;
        case OFPAT_COPY_TTL_OUT:
          /* nothing. */
          break;
        case OFPAT_COPY_TTL_IN:
          /* nothing. */
          break;
        case OFPAT_SET_MPLS_TTL:
          action_mpls_ttl_set((struct ofp_action_mpls_ttl *) &action->ofpat,
                              value);
          break;
        case OFPAT_DEC_MPLS_TTL:
          /* nothing. */
          break;
        case OFPAT_PUSH_VLAN:
          action_push_set((struct ofp_action_push *) &action->ofpat,
                          value);
          break;
        case OFPAT_POP_VLAN:
          /* nothing. */
          break;
        case OFPAT_PUSH_MPLS:
          action_push_set((struct ofp_action_push *) &action->ofpat,
                          value);
          break;
        case OFPAT_POP_MPLS:
          action_pop_mpls_set((struct ofp_action_pop_mpls *) &action->ofpat,
                              value);
          break;
        case OFPAT_SET_QUEUE:
          action_set_queue_set((struct ofp_action_set_queue *) &action->ofpat,
                               value);
          break;
        case OFPAT_GROUP:
          action_group_set((struct ofp_action_group *) &action->ofpat,
                           value);
          break;
        case OFPAT_SET_NW_TTL:
          action_nw_ttl_set((struct ofp_action_nw_ttl *) &action->ofpat,
                            value);
          break;
        case OFPAT_DEC_NW_TTL:
          /* nothing. */
          break;
        case OFPAT_PUSH_PBB:
          action_push_set((struct ofp_action_push *) &action->ofpat,
                          value);
          break;
        case OFPAT_POP_PBB:
          /* nothing. */
          break;
        case OFPAT_SET_FIELD:
        case OFPAT_EXPERIMENTER:
        default:
          ret = datastore_json_result_string_setf(
              result,
              LAGOPUS_RESULT_NOT_FOUND,
              "Not found action type(%"PRIu16").",
              action->ofpat.type);
          goto done;
      }
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_set(result,
                                      LAGOPUS_RESULT_NO_MEMORY,
                                      NULL);
    }
  } else {
    ret = datastore_json_result_string_setf(
        result,
        LAGOPUS_RESULT_NOT_FOUND,
        "Not found instruction.");
  }

done:
  return ret;
}

/* add flow action (for set_field). */
static inline lagopus_result_t
flow_cmd_action_set_field_add(struct action_list *action_list,
                              uint8_t oxm_field,
                              uint8_t oxm_length,
                              void *oxm_value,
                              value_hook_proc_t hook_proc,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_action_set_field *ofp_action = NULL;
  struct action *action = NULL;
  struct ofp_oxm *oxm;

  action = action_alloc(oxm_length);

  if (action != NULL) {
    TAILQ_INSERT_TAIL(action_list, action, entry);

    action->ofpat.type = OFPAT_SET_FIELD;
    action->ofpat.len = (uint16_t) (sizeof(struct ofp_action_header) +
                                    oxm_length);

    ofp_action = (struct ofp_action_set_field *) &action->ofpat;
    oxm = (struct ofp_oxm *) ofp_action->field;

    oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
    oxm->oxm_field = (uint8_t) (oxm_field << 1);
    oxm->oxm_length = oxm_length;

    if (hook_proc != NULL) {
      ret = (hook_proc)(oxm_length, oxm_value, result);
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_set(result, ret,
                                        NULL);
        goto done;
      }
    }
    memcpy(oxm->oxm_value, oxm_value, oxm_length);

    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_NO_MEMORY,
                                    NULL);
  }

done:
  return ret;
}

/* add flow instruction. */
static inline lagopus_result_t
flow_cmd_instruction_add(struct instruction_list *instruction_list,
                         uint16_t field,
                         void *value,
                         void *mask,
                         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct instruction *instruction = NULL;

  instruction = TAILQ_LAST(instruction_list, instruction_list);
  if (instruction != NULL) {
    switch (instruction->ofpit.type) {
      case OFPIT_CLEAR_ACTIONS:
        /* nothing. */
        break;
      case OFPIT_GOTO_TABLE:
        instruction->ofpit_goto_table.table_id =
            *((uint8_t *) value);
        break;
      case OFPIT_METER:
        instruction->ofpit_meter.meter_id =
            *((uint32_t *) value);
        break;
      case OFPIT_WRITE_METADATA:
        instruction->ofpit_write_metadata.metadata =
            *((uint64_t *) value);
        if (mask !=NULL) {
          instruction->ofpit_write_metadata.metadata_mask =
              *((uint64_t *) mask);
        }
        break;
      case OFPIT_WRITE_ACTIONS:
      case OFPIT_APPLY_ACTIONS:
      case OFPIT_EXPERIMENTER:
      default:
        ret = datastore_json_result_string_setf(
            result,
            LAGOPUS_RESULT_NOT_FOUND,
            "Not found instruction type(%"PRIu16").",
            field);
        goto done;
    }
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = datastore_json_result_string_setf(
        result,
        LAGOPUS_RESULT_NOT_FOUND,
        "Not found instruction.");
  }

done:
  return ret;
}

/* add action/match with oxm. */
static inline lagopus_result_t
flow_cmd_oxm_add(struct match_list *match_list,
                 struct instruction_list *instruction_list,
                 enum flow_cmd_type ftype,
                 uint16_t field,
                 uint16_t length,
                 void *oxm_value,
                 void *oxm_mask,
                 value_hook_proc_t hook_proc,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct instruction *instruction = NULL;
  uint8_t oxm_field;
  uint8_t oxm_length;

  if (field <= UINT8_MAX || length <= UINT8_MAX) {
    oxm_field = (uint8_t) field;
    oxm_length = (uint8_t) length;

    switch (ftype) {
      case FLOW_CMD_TYPE_ACTION_SET_FIELD:
        /* action. */
        instruction = TAILQ_LAST(instruction_list, instruction_list);
        if (instruction != NULL) {
          ret = flow_cmd_action_set_field_add(&instruction->action_list,
                                              oxm_field,
                                              oxm_length,
                                              oxm_value,
                                              hook_proc,
                                              result);
        } else {
          ret = datastore_json_result_string_setf(
              result,
              LAGOPUS_RESULT_NOT_FOUND,
              "Not found instruction.");
        }
        break;
      case FLOW_CMD_TYPE_MATCH:
        /* match. */
        ret = flow_cmd_match_add(match_list,
                                 oxm_field,
                                 oxm_length,
                                 oxm_value,
                                 oxm_mask,
                                 hook_proc,
                                 result);
        break;
      default:
        ret = datastore_json_result_string_setf(
            result,
            LAGOPUS_RESULT_INVALID_ARGS,
            "Bad flow mod type.");
    }
  } else {
    ret = datastore_json_result_string_setf(
        result,
        LAGOPUS_RESULT_TOO_LONG,
        "Bad oxm_type or oxm_length.");
  }

  return ret;
}

/* set flow mod. */
static inline lagopus_result_t
flow_cmd_mod_set(struct ofp_flow_mod *flow_mod,
                 struct match_list *match_list,
                 struct instruction_list *instruction_list,
                 enum flow_cmd_type ftype,
                 uint16_t field,
                 uint16_t length,
                 void *value,
                 void *mask,
                 value_hook_proc_t hook_proc,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  switch (ftype) {
    case FLOW_CMD_TYPE_MATCH:
    case FLOW_CMD_TYPE_ACTION_SET_FIELD:
      ret = flow_cmd_oxm_add(
          match_list,
          instruction_list,
          ftype,
          field,
          length,
          value,
          mask,
          hook_proc,
          result);
      break;
    case FLOW_CMD_TYPE_ACTION:
      ret = flow_cmd_action_add(instruction_list,
                                field,
                                length,
                                value,
                                result);
      break;
    case FLOW_CMD_TYPE_FLOW_FIELD:
      ret = flow_cmd_flow_field_add(flow_mod,
                                    field,
                                    value,
                                    mask,
                                    result);
      break;
    case FLOW_CMD_TYPE_INSTRUCTION:
      ret = flow_cmd_instruction_add(instruction_list,
                                     field,
                                     value,
                                     mask,
                                     result);
      break;
    default:
      ret = datastore_json_result_string_setf(
          result,
          LAGOPUS_RESULT_INVALID_ARGS,
          "Bad flow mod type.");
  }

  return ret;
}

/*
 * parse tools.
 */
static inline lagopus_result_t
uint_opt_parse(const char *str,
               size_t min,
               size_t max,
               lagopus_hashmap_t *to_num_table,
               union cmd_uint *cmd_uint,
               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *key = NULL;
  uint64_t *val;

  if (str != NULL && cmd_uint != NULL && result != NULL) {
    if ((ret = cmd_uint_parse(str, CMD_UINT64,
                              cmd_uint)) ==
        LAGOPUS_RESULT_OK) {
      if (min != 0 &&
          cmd_uint->uint64 < min) {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_TOO_SHORT,
                                                "Bad value (%"PRIu64").",
                                                cmd_uint->uint64);
        goto done;
      }
      if (max != UINT64_MAX &&
          cmd_uint->uint64 > max) {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_TOO_LONG,
                                                "Bad value (%"PRIu64").",
                                                cmd_uint->uint64);
        goto done;
      }
      ret = LAGOPUS_RESULT_OK;
    } else if (ret == LAGOPUS_RESULT_INVALID_ARGS &&
               to_num_table != NULL) {
      if ((ret = lagopus_str_trim(str, " \t\r\n", &key)) <= 0) {
        if (ret == 0) {
          ret = LAGOPUS_RESULT_INVALID_ARGS;
        }
        ret = datastore_json_result_string_setf(result,
                                                ret,
                                                "Bad value (%s).",
                                                str);
        goto done;
      }

      if ((ret = lagopus_hashmap_find(
              to_num_table,
              (void *) key,
              (void **) &val)) == LAGOPUS_RESULT_OK) {
        cmd_uint->uint64 = *val;
      } else {
        /* overwrite. */
        if (ret == LAGOPUS_RESULT_NOT_FOUND) {
          ret = LAGOPUS_RESULT_INVALID_ARGS;
        }
        ret = datastore_json_result_string_setf(result,
                                                ret,
                                                "Bad value (%s).",
                                                key);
        goto done;
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              ret,
                                              "Bad value (%s).",
                                              str);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  /* free. */
  free(key);

  return ret;
}

static inline lagopus_result_t
ip_opt_parse(const char *str,
             bool is_ipv4_addr,
             struct sockaddr **saddr,
             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_ip_address_t *addr = NULL;

  if ((ret = lagopus_ip_address_create(str, is_ipv4_addr, &addr)) ==
      LAGOPUS_RESULT_OK) {
    if ((ret = lagopus_ip_address_sockaddr_get(addr, saddr)) !=
        LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result,
                                              ret,
                                              "Bad value (%s).",
                                              str);
    }
  } else {
    ret = datastore_json_result_string_setf(result,
                                            ret,
                                            "Bad value (%s).",
                                            str);
  }

  if (addr != NULL) {
    lagopus_ip_address_destroy(addr);
  }

  return ret;
}

static inline lagopus_result_t
mac_addr_opt_parse(const char *str,
                   mac_address_t *addr,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((ret = cmd_mac_addr_parse(str, *addr)) !=
      LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result,
                                            ret,
                                            "Bad value (%s).",
                                            str);
  }

  return ret;
}

static inline lagopus_result_t
mask_split(char **ts,
           char **tokens,
           size_t tokens_size,
           bool has_mask,
           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t n_tokens = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;

  memset((void *) tokens, 0, tokens_size);
  n_tokens = lagopus_str_tokenize(*ts, tokens,
                                  tokens_size, "/");

  if (n_tokens == 1 ||
      (has_mask == true && n_tokens == 2)) {
    for (i = 0; i < (size_t) n_tokens; i++) {
      if (IS_VALID_STRING(tokens[i]) == false) {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_INVALID_ARGS,
                                                "Bad mask.");
        goto done;
      }
    }
    ret = n_tokens;
  } else if (n_tokens < 0) {
    ret = datastore_json_result_string_setf(result,
                                            ret,
                                            "Bad mask.");
  } else {
    ret = datastore_json_result_string_setf(result,
                                            LAGOPUS_RESULT_INVALID_ARGS,
                                            "Bad mask.");
  }

done:
  return ret;
}

static inline lagopus_result_t
action_split(char **ts,
             char **tokens,
             size_t tokens_size,
             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t n_tokens = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;

  memset((void *) tokens, 0, tokens_size);
  n_tokens = lagopus_str_tokenize_with_limit(*ts, tokens,
                                             tokens_size,
                                             ACTION_TOKEN_LIMIT,
                                             ":");

  if (n_tokens == 1 || n_tokens == 2) {
    for (i = 0; i < (size_t) n_tokens; i++) {
      if (IS_VALID_STRING(tokens[i]) == false) {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_INVALID_ARGS,
                                                "Bad action.");
        goto done;
      }
    }
    ret = n_tokens;
  } else if (n_tokens < 0) {
    ret = datastore_json_result_string_setf(result,
                                            ret,
                                            "Bad action.");
  } else {
    ret = datastore_json_result_string_setf(result,
                                            LAGOPUS_RESULT_INVALID_ARGS,
                                            "Bad action.");
  }

done:
  return ret;
}

static inline lagopus_result_t
comma_split(char **ts,
            char **tokens,
            size_t tokens_size,
            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t n_tokens = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;

  memset((void *) tokens, 0, tokens_size);
  n_tokens = lagopus_str_tokenize(*ts, tokens,
                                  tokens_size, ",");

  if (n_tokens > 0 && n_tokens <= TOKEN_MAX) {
    for (i = 0; i < (size_t) n_tokens; i++) {
      if (IS_VALID_STRING(tokens[i]) == false) {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_INVALID_ARGS,
                                                "Bad comma.");
        goto done;
      }
    }
    ret = n_tokens;
  } else {
    ret = datastore_json_result_string_setf(result,
                                            ret,
                                            "Bad comma.");
  }

done:
  return ret;
}

/*
 * parse fields.
 */
static inline lagopus_result_t
uint_field_parse(char **argv[],
                 struct ofp_flow_mod *flow_mod,
                 struct match_list *match_list,
                 struct instruction_list *instruction_list,
                 enum flow_cmd_type ftype,
                 size_t min,
                 size_t max,
                 uint16_t field,
                 uint16_t length,
                 value_hook_proc_t hook_proc,
                 lagopus_hashmap_t *to_num_table,
                 bool has_mask,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t n_tokens = LAGOPUS_RESULT_ANY_FAILURES;
  char *tokens[TOKEN_MAX + 1];
  size_t i;
  union cmd_uint cmd_uint[FIELD_VALUE_WITH_MASK_NUM] = {{0}, {0}};

  if (argv != NULL && flow_mod != NULL &&
      match_list != NULL && instruction_list != NULL &&
      result != NULL) {
    if (**argv != NULL && *(*argv + 1) != NULL) {
      (*argv)++;

      n_tokens = mask_split(*argv, (char**) &tokens, TOKEN_MAX + 1,
                            (has_mask == true &&
                             (ftype == FLOW_CMD_TYPE_MATCH ||
                              ftype == FLOW_CMD_TYPE_FLOW_FIELD ||
                              ftype == FLOW_CMD_TYPE_INSTRUCTION)),
                            result);
      if (n_tokens > 0) {
        for (i = 0; i < (size_t) n_tokens; i++) {
          if ((ret = uint_opt_parse(tokens[i], min, max,
                                    (i == 0) ? to_num_table : NULL,
                                    &cmd_uint[i],
                                    result)) !=
              LAGOPUS_RESULT_OK) {
            goto done;
          }
        }

        if (ret == LAGOPUS_RESULT_OK) {
          ret = flow_cmd_mod_set(
              flow_mod,
              match_list,
              instruction_list,
              ftype,
              field,
              length,
              (void *) &cmd_uint[0].uint64,
              (n_tokens == 2) ? (void *) &cmd_uint[1].uint64 : NULL,
              hook_proc,
              result);
        }
      } else {
        ret = n_tokens;
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Bad value.");
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

done:
  return ret;
}

#define IPV4_FIELD_LEN 4
#define IPV6_FIELD_LEN 16

static inline lagopus_result_t
ipv_field_parse(char **argv[],
                struct ofp_flow_mod *flow_mod,
                struct match_list *match_list,
                struct instruction_list *instruction_list,
                enum flow_cmd_type ftype,
                uint16_t field,
                bool has_mask,
                bool is_ipv4_addr,
                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t n_tokens = LAGOPUS_RESULT_ANY_FAILURES;
  char *tokens[TOKEN_MAX + 1];
  size_t i;
  struct sockaddr *saddr[FIELD_VALUE_WITH_MASK_NUM] = {NULL, NULL};

  if (argv != NULL && flow_mod != NULL &&
      match_list != NULL && instruction_list != NULL &&
      result != NULL) {
    if (**argv != NULL && *(*argv + 1) != NULL) {
      (*argv)++;

      n_tokens = mask_split(*argv, (char**) &tokens, TOKEN_MAX + 1,
                            (has_mask == true &&
                             ftype == FLOW_CMD_TYPE_MATCH),
                            result);

      if (n_tokens > 0) {
        for (i = 0; i < (size_t) n_tokens; i++) {
          if ((ret = ip_opt_parse(tokens[i], is_ipv4_addr,
                                  &saddr[i], result)) !=
              LAGOPUS_RESULT_OK) {
            goto done;
          }
        }

        if (ret == LAGOPUS_RESULT_OK) {
          if (is_ipv4_addr == true) {
            /* IPv4. */
            ret = flow_cmd_mod_set(
                flow_mod,
                match_list,
                instruction_list,
                ftype,
                field,
                IPV4_FIELD_LEN,
                (void *) &(((struct sockaddr_in *)saddr[0])->sin_addr),
                (n_tokens == 2) ? (void *) &(((struct sockaddr_in *)
                                              saddr[1])->sin_addr) : NULL,
                NULL,
                result);
          } else {
            /* IPv6. */
            ret = flow_cmd_mod_set(
                flow_mod,
                match_list,
                instruction_list,
                ftype,
                field,
                IPV6_FIELD_LEN,
                (void *) &(((struct sockaddr_in6 *)saddr[0])->sin6_addr),
                (n_tokens == 2) ? (void *) &(((struct sockaddr_in6 *)
                                              saddr[1])->sin6_addr) : NULL,
                NULL,
                result);
          }
        }
      } else {
        ret = n_tokens;
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Bad value.");
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

done:
  /* free. */
  if (n_tokens > 0) {
    for (i = 0; i < (size_t) n_tokens; i++) {
      if (saddr[i] != NULL) {
        free(saddr[i]);
      }
    }
  }

  return ret;
}

#define MAC_ADDR_FIELD_LEN 6

static inline lagopus_result_t
mac_addr_field_parse(char **argv[],
                     struct ofp_flow_mod *flow_mod,
                     struct match_list *match_list,
                     struct instruction_list *instruction_list,
                     enum flow_cmd_type ftype,
                     uint16_t field,
                     bool has_mask,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t n_tokens = LAGOPUS_RESULT_ANY_FAILURES;
  char *tokens[TOKEN_MAX + 1];
  size_t i;
  mac_address_t addr[2];

  if (argv != NULL && flow_mod != NULL &&
      match_list != NULL && instruction_list != NULL &&
      result != NULL) {
    if (**argv != NULL && *(*argv + 1) != NULL) {
      (*argv)++;

      n_tokens = mask_split(*argv, (char**) &tokens, TOKEN_MAX + 1,
                            (has_mask == true &&
                             ftype == FLOW_CMD_TYPE_MATCH),
                            result);

      if (n_tokens > 0) {
        for (i = 0; i < (size_t) n_tokens; i++) {
          if ((ret = mac_addr_opt_parse(tokens[i], &addr[i], result)) !=
              LAGOPUS_RESULT_OK) {
            goto done;
          }
        }

        if (ret == LAGOPUS_RESULT_OK) {
          ret = flow_cmd_mod_set(
              flow_mod,
              match_list,
              instruction_list,
              ftype,
              field,
              MAC_ADDR_FIELD_LEN,
              (void *) &addr[0],
              (n_tokens == 2) ? &addr[1] : NULL,
              NULL,
              result);
        }
      } else {
        ret = n_tokens;
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Bad value.");
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

done:
  return ret;
}

static inline lagopus_result_t
one_cmd_field_parse(char **argv[],
                     struct ofp_flow_mod *flow_mod,
                     struct match_list *match_list,
                     struct instruction_list *instruction_list,
                     enum flow_cmd_type ftype,
                     uint16_t field,
                     bool has_mask,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t n_tokens = LAGOPUS_RESULT_ANY_FAILURES;
  char *tokens[TOKEN_MAX + 1];

  if (argv != NULL && flow_mod != NULL &&
      match_list != NULL && instruction_list != NULL &&
      result != NULL) {
    n_tokens = mask_split(*argv, (char**) &tokens, TOKEN_MAX + 1,
                          (has_mask == true &&
                           ftype == FLOW_CMD_TYPE_MATCH),
                          result);
    if (n_tokens == 1) {
      ret = flow_cmd_mod_set(
          flow_mod,
          match_list,
          instruction_list,
          ftype,
          field,
          0,
          NULL,
          NULL,
          NULL,
          result);
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Bad value (%s).",
                                              *(*argv));
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

/*
 * parse actions.
 */
static inline lagopus_result_t
actions_parse(char **argv[],
              struct ofp_flow_mod *flow_mod,
              struct match_list *match_list,
              struct instruction_list *instruction_list,
              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t n_tokens = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t n_act_tokens = LAGOPUS_RESULT_ANY_FAILURES;
  char *key = NULL;
  enum flow_cmd_type ftype;
  char *tokens[TOKEN_MAX + 1];
  char *act_tokens[TOKEN_MAX + 1];
  void *opt_proc;
  size_t i;

  if (argv != NULL && flow_mod != NULL &&
      match_list != NULL && instruction_list != NULL &&
      result != NULL) {

    if (**argv != NULL && *(*argv + 1) != NULL) {
      (*argv)++;
      n_tokens = comma_split(*argv, (char**) &tokens, TOKEN_MAX + 1, result);

      if (n_tokens > 0) {
        for (i = 0; i < (size_t) n_tokens; i++) {
          n_act_tokens = action_split(&tokens[i],
                                      (char**) &act_tokens,
                                      TOKEN_MAX + 1,
                                      result);

          if (n_act_tokens > 0) {
            if ((ret = lagopus_str_trim(act_tokens[0], " \t\r\n", &key)) <= 0) {
              if (ret == 0) {
                ret = LAGOPUS_RESULT_INVALID_ARGS;
              }
              ret = datastore_json_result_string_setf(result,
                                                      ret,
                                                      "Bad value (%s).",
                                                      act_tokens[0]);
              goto loop_end;
            }

            /* find in action table.*/
            if ((ret = lagopus_hashmap_find(
                    &flow_mod_action_cmd_table,
                    (void *) key,
                    &opt_proc)) == LAGOPUS_RESULT_OK) {
              ftype = FLOW_CMD_TYPE_ACTION;
            } else if (ret == LAGOPUS_RESULT_NOT_FOUND) {
              /* find in action table for set_field.*/
              if ((ret = lagopus_hashmap_find(
                      &flow_mod_action_set_field_cmd_table,
                      (void *) key,
                      &opt_proc)) == LAGOPUS_RESULT_OK) {
                ftype = FLOW_CMD_TYPE_ACTION_SET_FIELD;
              } else {
                ret = datastore_json_result_string_setf(
                    result,
                    ret,
                    "Not found cmd (%s).",
                    key);
                goto loop_end;
              }
            } else {
              ret = datastore_json_result_string_setf(
                  result,
                  ret,
                  "Not found cmd (%s).",
                  key);
              goto loop_end;
            }

            if (opt_proc != NULL) {
              ret = ((flow_mod_cmd_proc_t) opt_proc)
                  ((const char * const **)act_tokens,
                   flow_mod,
                   match_list,
                   instruction_list,
                   ftype,
                   result);
            } else {
              ret = datastore_json_result_string_setf(
                  result,
                  LAGOPUS_RESULT_NOT_FOUND,
                  "Not found parse func (%s).",
                  tokens[0]);
              goto loop_end;
            }
          }

       loop_end:
          /* free. */
          free(key);
          key = NULL;
          if (ret != LAGOPUS_RESULT_OK) {
            break;
          }
        }
      } else {
        ret = n_tokens;
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Bad value.");
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

/*
 * parse instructions/actions.
 */

/* goto_table */
#define FLOW_CMD_GOTO_TABLE_MAX 254
#define FLOW_CMD_GOTO_TABLE_MIN 0
#define FLOW_CMD_GOTO_TABLE_LEN 0

/* write_metadata */
#define FLOW_CMD_WRITE_METADATA_MAX UINT64_MAX
#define FLOW_CMD_WRITE_METADATA_MIN 0
#define FLOW_CMD_WRITE_METADATA_LEN 0

/* meter */
#define FLOW_CMD_METER_MAX UINT32_MAX
#define FLOW_CMD_METER_MIN 0
#define FLOW_CMD_METER_LEN 0

static inline lagopus_result_t
instructions_parse(char **argv[],
                   struct ofp_flow_mod *flow_mod,
                   struct match_list *match_list,
                   struct instruction_list *instruction_list,
                   uint16_t instruction_type,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct instruction *instruction = NULL;

  if (argv != NULL && flow_mod != NULL &&
      match_list != NULL && instruction_list != NULL &&
      result != NULL) {
    instruction = instruction_alloc();

    if (instruction != NULL) {
      TAILQ_INSERT_TAIL(instruction_list, instruction, entry);
      instruction->ofpit.type = instruction_type;

      switch (instruction_type) {
        case OFPIT_WRITE_ACTIONS:
        case OFPIT_APPLY_ACTIONS:
          ret = actions_parse(argv, flow_mod, match_list,
                              instruction_list, result);
          break;
        case OFPIT_CLEAR_ACTIONS:
          ret = one_cmd_field_parse(argv, flow_mod,
                                    match_list, instruction_list,
                                    FLOW_CMD_TYPE_INSTRUCTION,
                                    instruction_type,
                                    NULL,
                                    result);
          break;
        case OFPIT_GOTO_TABLE:
          ret = uint_field_parse(argv, flow_mod,
                                 match_list, instruction_list,
                                 FLOW_CMD_TYPE_INSTRUCTION,
                                 FLOW_CMD_GOTO_TABLE_MIN,
                                 FLOW_CMD_GOTO_TABLE_MAX,
                                 instruction_type,
                                 FLOW_CMD_GOTO_TABLE_LEN,
                                 NULL,
                                 NULL,
                                 false,
                                 result);
          break;
        case OFPIT_WRITE_METADATA:
          ret = uint_field_parse(argv, flow_mod,
                                 match_list, instruction_list,
                                 FLOW_CMD_TYPE_INSTRUCTION,
                                 FLOW_CMD_WRITE_METADATA_MIN,
                                 FLOW_CMD_WRITE_METADATA_MAX,
                                 instruction_type,
                                 FLOW_CMD_WRITE_METADATA_LEN,
                                 NULL,
                                 NULL,
                                 true,
                                 result);
          break;
        case OFPIT_METER:
          ret = uint_field_parse(argv, flow_mod,
                                 match_list, instruction_list,
                                 FLOW_CMD_TYPE_INSTRUCTION,
                                 FLOW_CMD_METER_MIN,
                                 FLOW_CMD_METER_MAX,
                                 instruction_type,
                                 FLOW_CMD_METER_LEN,
                                 NULL,
                                 NULL,
                                 false,
                                 result);
          break;
        case OFPIT_EXPERIMENTER:
        default:
          ret = datastore_json_result_string_setf(
              result,
              LAGOPUS_RESULT_INVALID_ARGS,
              "Bad type (%d).", instruction_type);
          break;
      }
    } else {
      ret = datastore_json_result_set(result,
                                      LAGOPUS_RESULT_NO_MEMORY,
                                      NULL);
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

/*
 * parse oxm fields.
 */

/* oxm field (in_port). */
#define FLOW_CMD_IN_PORT_MAX UINT32_MAX
#define FLOW_CMD_IN_PORT_MIN 0
#define FLOW_CMD_IN_PORT_LEN 4

static lagopus_result_t
in_port_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_IN_PORT_MIN,
                          FLOW_CMD_IN_PORT_MAX,
                          OFPXMT_OFB_IN_PORT,
                          FLOW_CMD_IN_PORT_LEN,
                          value_uint32_hook,
                          &flow_mod_port_str_table,
                          false,
                          result);
}

static lagopus_result_t
in_phy_port_cmd_parse(const char *const argv[],
                      struct ofp_flow_mod *flow_mod,
                      struct match_list *match_list,
                      struct instruction_list *instruction_list,
                      enum flow_cmd_type ftype,
                      lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_IN_PORT_MIN,
                          FLOW_CMD_IN_PORT_MAX,
                          OFPXMT_OFB_IN_PHY_PORT,
                          FLOW_CMD_IN_PORT_LEN,
                          value_uint32_hook,
                          &flow_mod_port_str_table,
                          false,
                          result);
}

/* oxm field (metadata). */
#define FLOW_CMD_METADATA_MAX UINT64_MAX
#define FLOW_CMD_METADATA_MIN 0
#define FLOW_CMD_METADATA_LEN 8

static lagopus_result_t
metadata_cmd_parse(const char *const argv[],
                   struct ofp_flow_mod *flow_mod,
                   struct match_list *match_list,
                   struct instruction_list *instruction_list,
                   enum flow_cmd_type ftype,
                   lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_METADATA_MIN,
                          FLOW_CMD_METADATA_MAX,
                          OFPXMT_OFB_METADATA,
                          FLOW_CMD_METADATA_LEN,
                          value_uint64_hook,
                          NULL,
                          true,
                          result);
}

/* oxm field (dl_dst). */
static lagopus_result_t
dl_dst_cmd_parse(const char *const argv[],
                 struct ofp_flow_mod *flow_mod,
                 struct match_list *match_list,
                 struct instruction_list *instruction_list,
                 enum flow_cmd_type ftype,
                 lagopus_dstring_t *result) {
  return mac_addr_field_parse((char ***) &argv, flow_mod,
                              match_list, instruction_list,
                              ftype,
                              OFPXMT_OFB_ETH_DST,
                              true,
                              result);
}

/* oxm field (dl_src). */
static lagopus_result_t
dl_src_cmd_parse(const char *const argv[],
                 struct ofp_flow_mod *flow_mod,
                 struct match_list *match_list,
                 struct instruction_list *instruction_list,
                 enum flow_cmd_type ftype,
                 lagopus_dstring_t *result) {
  return mac_addr_field_parse((char ***) &argv, flow_mod,
                              match_list, instruction_list,
                              ftype,
                              OFPXMT_OFB_ETH_SRC,
                              true,
                              result);
}

/* oxm field (dl_type). */
#define FLOW_CMD_DL_TYPE_MAX UINT16_MAX
#define FLOW_CMD_DL_TYPE_MIN 0
#define FLOW_CMD_DL_TYPE_LEN 2

static lagopus_result_t
dl_type_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_DL_TYPE_MIN,
                          FLOW_CMD_DL_TYPE_MAX,
                          OFPXMT_OFB_ETH_TYPE,
                          FLOW_CMD_DL_TYPE_LEN,
                          value_uint16_hook,
                          &flow_mod_dl_type_str_table,
                          false,
                          result);
}

/* oxm field (vlan_vid). */
#define FLOW_CMD_VLAN_VID_MAX UINT16_MAX
#define FLOW_CMD_VLAN_VID_MIN 0
#define FLOW_CMD_VLAN_VID_LEN 2

static lagopus_result_t
vlan_vid_cmd_parse(const char *const argv[],
                   struct ofp_flow_mod *flow_mod,
                   struct match_list *match_list,
                   struct instruction_list *instruction_list,
                   enum flow_cmd_type ftype,
                   lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_VLAN_VID_MIN,
                          FLOW_CMD_VLAN_VID_MAX,
                          OFPXMT_OFB_VLAN_VID,
                          FLOW_CMD_VLAN_VID_LEN,
                          value_vlan_vid_hook,
                          NULL,
                          true,
                          result);
}

/* oxm field (vlan_pcp). */
#define FLOW_CMD_VLAN_PCP_MAX 7
#define FLOW_CMD_VLAN_PCP_MIN 0
#define FLOW_CMD_VLAN_PCP_LEN 1

static lagopus_result_t
vlan_pcp_cmd_parse(const char *const argv[],
                   struct ofp_flow_mod *flow_mod,
                   struct match_list *match_list,
                   struct instruction_list *instruction_list,
                   enum flow_cmd_type ftype,
                   lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_VLAN_PCP_MIN,
                          FLOW_CMD_VLAN_PCP_MAX,
                          OFPXMT_OFB_VLAN_PCP,
                          FLOW_CMD_VLAN_PCP_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* oxm field (ip_dscp). */
#define FLOW_CMD_IP_DSCP_MAX 63
#define FLOW_CMD_IP_DSCP_MIN 0
#define FLOW_CMD_IP_DSCP_LEN 1

static lagopus_result_t
ip_dscp_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_IP_DSCP_MIN,
                          FLOW_CMD_IP_DSCP_MAX,
                          OFPXMT_OFB_IP_DSCP,
                          FLOW_CMD_IP_DSCP_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* oxm field (nw_ecn). */
#define FLOW_CMD_NW_ECN_MAX 3
#define FLOW_CMD_NW_ECN_MIN 0
#define FLOW_CMD_NW_ECN_LEN 1

static lagopus_result_t
nw_ecn_cmd_parse(const char *const argv[],
                 struct ofp_flow_mod *flow_mod,
                 struct match_list *match_list,
                 struct instruction_list *instruction_list,
                 enum flow_cmd_type ftype,
                 lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_NW_ECN_MIN,
                          FLOW_CMD_NW_ECN_MAX,
                          OFPXMT_OFB_IP_ECN,
                          FLOW_CMD_NW_ECN_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* oxm field (nw_proto). */
#define FLOW_CMD_NW_PROTO_MAX UINT8_MAX
#define FLOW_CMD_NW_PROTO_MIN 0
#define FLOW_CMD_NW_PROTO_LEN 1

static lagopus_result_t
nw_proto_cmd_parse(const char *const argv[],
                   struct ofp_flow_mod *flow_mod,
                   struct match_list *match_list,
                   struct instruction_list *instruction_list,
                   enum flow_cmd_type ftype,
                   lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_NW_PROTO_MIN,
                          FLOW_CMD_NW_PROTO_MAX,
                          OFPXMT_OFB_IP_PROTO,
                          FLOW_CMD_NW_PROTO_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* oxm field (nw_src). */
static lagopus_result_t
nw_src_cmd_parse(const char *const argv[],
                 struct ofp_flow_mod *flow_mod,
                 struct match_list *match_list,
                 struct instruction_list *instruction_list,
                 enum flow_cmd_type ftype,
                 lagopus_dstring_t *result) {
  return ipv_field_parse((char ***) &argv, flow_mod,
                         match_list, instruction_list,
                         ftype,
                         OFPXMT_OFB_IPV4_SRC,
                         true,
                         true,
                         result);
}

/* oxm field (nw_dst). */
static lagopus_result_t
nw_dst_cmd_parse(const char *const argv[],
                 struct ofp_flow_mod *flow_mod,
                 struct match_list *match_list,
                 struct instruction_list *instruction_list,
                 enum flow_cmd_type ftype,
                 lagopus_dstring_t *result) {
  return ipv_field_parse((char ***) &argv, flow_mod,
                         match_list, instruction_list,
                         ftype,
                         OFPXMT_OFB_IPV4_DST,
                         true,
                         true,
                         result);
}

/* oxm field (tcp_src). */
#define FLOW_CMD_TCP_SRC_MAX UINT16_MAX
#define FLOW_CMD_TCP_SRC_MIN 0
#define FLOW_CMD_TCP_SRC_LEN 2

static lagopus_result_t
tcp_src_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_TCP_SRC_MIN,
                          FLOW_CMD_TCP_SRC_MAX,
                          OFPXMT_OFB_TCP_SRC,
                          FLOW_CMD_TCP_SRC_LEN,
                          value_uint16_hook,
                          NULL,
                          false,
                          result);
}

/* oxm field (tcp_dst). */
#define FLOW_CMD_TCP_DST_MAX UINT16_MAX
#define FLOW_CMD_TCP_DST_MIN 0
#define FLOW_CMD_TCP_DST_LEN 2

static lagopus_result_t
tcp_dst_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_TCP_DST_MIN,
                          FLOW_CMD_TCP_DST_MAX,
                          OFPXMT_OFB_TCP_DST,
                          FLOW_CMD_TCP_DST_LEN,
                          value_uint16_hook,
                          NULL,
                          false,
                          result);
}

/* oxm field (udp_src). */
#define FLOW_CMD_UDP_SRC_MAX UINT16_MAX
#define FLOW_CMD_UDP_SRC_MIN 0
#define FLOW_CMD_UDP_SRC_LEN 2

static lagopus_result_t
udp_src_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_UDP_SRC_MIN,
                          FLOW_CMD_UDP_SRC_MAX,
                          OFPXMT_OFB_UDP_SRC,
                          FLOW_CMD_UDP_SRC_LEN,
                          value_uint16_hook,
                          NULL,
                          false,
                          result);
}

/* oxm field (udp_dst). */
#define FLOW_CMD_UDP_DST_MAX UINT16_MAX
#define FLOW_CMD_UDP_DST_MIN 0
#define FLOW_CMD_UDP_DST_LEN 2

static lagopus_result_t
udp_dst_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_UDP_DST_MIN,
                          FLOW_CMD_UDP_DST_MAX,
                          OFPXMT_OFB_UDP_DST,
                          FLOW_CMD_UDP_DST_LEN,
                          value_uint16_hook,
                          NULL,
                          false,
                          result);
}

/* oxm field (sctp_src). */
#define FLOW_CMD_SCTP_SRC_MAX UINT16_MAX
#define FLOW_CMD_SCTP_SRC_MIN 0
#define FLOW_CMD_SCTP_SRC_LEN 2

static lagopus_result_t
sctp_src_cmd_parse(const char *const argv[],
                   struct ofp_flow_mod *flow_mod,
                   struct match_list *match_list,
                   struct instruction_list *instruction_list,
                   enum flow_cmd_type ftype,
                   lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_SCTP_SRC_MIN,
                          FLOW_CMD_SCTP_SRC_MAX,
                          OFPXMT_OFB_SCTP_SRC,
                          FLOW_CMD_SCTP_SRC_LEN,
                          value_uint16_hook,
                          NULL,
                          false,
                          result);
}

/* oxm field (sctp_dst). */
#define FLOW_CMD_SCTP_DST_MAX UINT16_MAX
#define FLOW_CMD_SCTP_DST_MIN 0
#define FLOW_CMD_SCTP_DST_LEN 2

static lagopus_result_t
sctp_dst_cmd_parse(const char *const argv[],
                   struct ofp_flow_mod *flow_mod,
                   struct match_list *match_list,
                   struct instruction_list *instruction_list,
                   enum flow_cmd_type ftype,
                   lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_SCTP_DST_MIN,
                          FLOW_CMD_SCTP_DST_MAX,
                          OFPXMT_OFB_SCTP_DST,
                          FLOW_CMD_SCTP_DST_LEN,
                          value_uint16_hook,
                          NULL,
                          false,
                          result);
}

/* oxm field (icmp_type). */
#define FLOW_CMD_ICMP_TYPE_MAX UINT8_MAX
#define FLOW_CMD_ICMP_TYPE_MIN 0
#define FLOW_CMD_ICMP_TYPE_LEN 1

static lagopus_result_t
icmp_type_cmd_parse(const char *const argv[],
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct instruction_list *instruction_list,
                    enum flow_cmd_type ftype,
                    lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_ICMP_TYPE_MIN,
                          FLOW_CMD_ICMP_TYPE_MAX,
                          OFPXMT_OFB_ICMPV4_TYPE,
                          FLOW_CMD_ICMP_TYPE_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* oxm field (icmp_code). */
#define FLOW_CMD_ICMP_CODE_MAX UINT8_MAX
#define FLOW_CMD_ICMP_CODE_MIN 0
#define FLOW_CMD_ICMP_CODE_LEN 1

static lagopus_result_t
icmp_code_cmd_parse(const char *const argv[],
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct instruction_list *instruction_list,
                    enum flow_cmd_type ftype,
                    lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_ICMP_CODE_MIN,
                          FLOW_CMD_ICMP_CODE_MAX,
                          OFPXMT_OFB_ICMPV4_CODE,
                          FLOW_CMD_ICMP_CODE_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* oxm field (arp_op). */
#define FLOW_CMD_ARP_OP_MAX UINT16_MAX
#define FLOW_CMD_ARP_OP_MIN 0
#define FLOW_CMD_ARP_OP_LEN 2

static lagopus_result_t
arp_op_cmd_parse(const char *const argv[],
                 struct ofp_flow_mod *flow_mod,
                 struct match_list *match_list,
                 struct instruction_list *instruction_list,
                 enum flow_cmd_type ftype,
                 lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_ARP_OP_MIN,
                          FLOW_CMD_ARP_OP_MAX,
                          OFPXMT_OFB_ARP_OP,
                          FLOW_CMD_ARP_OP_LEN,
                          value_uint16_hook,
                          NULL,
                          false,
                          result);
}

/* oxm field (arp_spa). */
static lagopus_result_t
arp_spa_cmd_parse(const char *const argv[],
                 struct ofp_flow_mod *flow_mod,
                 struct match_list *match_list,
                 struct instruction_list *instruction_list,
                 enum flow_cmd_type ftype,
                 lagopus_dstring_t *result) {
  return ipv_field_parse((char ***) &argv, flow_mod,
                         match_list, instruction_list,
                         ftype,
                         OFPXMT_OFB_ARP_SPA,
                         true,
                         true,
                         result);
}

/* oxm field (arp_tpa). */
static lagopus_result_t
arp_tpa_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return ipv_field_parse((char ***) &argv, flow_mod,
                         match_list, instruction_list,
                         ftype,
                         OFPXMT_OFB_ARP_TPA,
                         true,
                         true,
                         result);
}

/* oxm field (arp_sha). */
static lagopus_result_t
arp_sha_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return mac_addr_field_parse((char ***) &argv, flow_mod,
                              match_list, instruction_list,
                              ftype,
                              OFPXMT_OFB_ARP_SHA,
                              true,
                              result);
}

/* oxm field (arp_tha). */
static lagopus_result_t
arp_tha_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return mac_addr_field_parse((char ***) &argv, flow_mod,
                              match_list, instruction_list,
                              ftype,
                              OFPXMT_OFB_ARP_THA,
                              true,
                              result);
}

/* oxm field (ipv6_src). */
static lagopus_result_t
ipv6_src_cmd_parse(const char *const argv[],
                   struct ofp_flow_mod *flow_mod,
                   struct match_list *match_list,
                   struct instruction_list *instruction_list,
                   enum flow_cmd_type ftype,
                   lagopus_dstring_t *result) {
  return ipv_field_parse((char ***) &argv, flow_mod,
                         match_list, instruction_list,
                         ftype,
                         OFPXMT_OFB_IPV6_SRC,
                         true,
                         false,
                         result);
}

/* oxm field (ipv6_dst). */
static lagopus_result_t
ipv6_dst_cmd_parse(const char *const argv[],
                   struct ofp_flow_mod *flow_mod,
                   struct match_list *match_list,
                   struct instruction_list *instruction_list,
                   enum flow_cmd_type ftype,
                   lagopus_dstring_t *result) {
  return ipv_field_parse((char ***) &argv, flow_mod,
                         match_list, instruction_list,
                         ftype,
                         OFPXMT_OFB_IPV6_DST,
                         true,
                         false,
                         result);
}

/* oxm field (ipv6_label). */
#define FLOW_CMD_IPV6_LABEL_MAX 1048575
#define FLOW_CMD_IPV6_LABEL_MIN 0
#define FLOW_CMD_IPV6_LABEL_LEN 4

static lagopus_result_t
ipv6_label_cmd_parse(const char *const argv[],
                     struct ofp_flow_mod *flow_mod,
                     struct match_list *match_list,
                     struct instruction_list *instruction_list,
                     enum flow_cmd_type ftype,
                     lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_IPV6_LABEL_MIN,
                          FLOW_CMD_IPV6_LABEL_MAX,
                          OFPXMT_OFB_IPV6_FLABEL,
                          FLOW_CMD_IPV6_LABEL_LEN,
                          value_uint32_hook,
                          NULL,
                          true,
                          result);
}

/* oxm field (icmpv6_type). */
#define FLOW_CMD_ICMPV6_TYPE_MAX UINT8_MAX
#define FLOW_CMD_ICMPV6_TYPE_MIN 0
#define FLOW_CMD_ICMPV6_TYPE_LEN 1

static lagopus_result_t
icmpv6_type_cmd_parse(const char *const argv[],
                      struct ofp_flow_mod *flow_mod,
                      struct match_list *match_list,
                      struct instruction_list *instruction_list,
                      enum flow_cmd_type ftype,
                      lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_ICMPV6_TYPE_MIN,
                          FLOW_CMD_ICMPV6_TYPE_MAX,
                          OFPXMT_OFB_ICMPV6_TYPE,
                          FLOW_CMD_ICMPV6_TYPE_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* oxm field (icmpv6_code). */
#define FLOW_CMD_ICMPV6_CODE_MAX UINT8_MAX
#define FLOW_CMD_ICMPV6_CODE_MIN 0
#define FLOW_CMD_ICMPV6_CODE_LEN 1

static lagopus_result_t
icmpv6_code_cmd_parse(const char *const argv[],
                      struct ofp_flow_mod *flow_mod,
                      struct match_list *match_list,
                      struct instruction_list *instruction_list,
                      enum flow_cmd_type ftype,
                      lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_ICMPV6_CODE_MIN,
                          FLOW_CMD_ICMPV6_CODE_MAX,
                          OFPXMT_OFB_ICMPV6_CODE,
                          FLOW_CMD_ICMPV6_CODE_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* oxm field (nd_target). */
static lagopus_result_t
nd_target_cmd_parse(const char *const argv[],
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct instruction_list *instruction_list,
                    enum flow_cmd_type ftype,
                    lagopus_dstring_t *result) {
  return ipv_field_parse((char ***) &argv, flow_mod,
                         match_list, instruction_list,
                         ftype,
                         OFPXMT_OFB_IPV6_ND_TARGET,
                         false,
                         false,
                         result);
}

/* oxm field (nd_sll). */
static lagopus_result_t
nd_sll_cmd_parse(const char *const argv[],
                 struct ofp_flow_mod *flow_mod,
                 struct match_list *match_list,
                 struct instruction_list *instruction_list,
                 enum flow_cmd_type ftype,
                 lagopus_dstring_t *result) {
  return mac_addr_field_parse((char ***) &argv, flow_mod,
                              match_list, instruction_list,
                              ftype,
                              OFPXMT_OFB_IPV6_ND_SLL,
                              false,
                              result);
}

/* oxm field (nd_tll). */
static lagopus_result_t
nd_tll_cmd_parse(const char *const argv[],
                 struct ofp_flow_mod *flow_mod,
                 struct match_list *match_list,
                 struct instruction_list *instruction_list,
                 enum flow_cmd_type ftype,
                 lagopus_dstring_t *result) {
  return mac_addr_field_parse((char ***) &argv, flow_mod,
                              match_list, instruction_list,
                              ftype,
                              OFPXMT_OFB_IPV6_ND_TLL,
                              false,
                              result);
}

/* oxm field (mpls_label). */
#define FLOW_CMD_MPLS_LABEL_MAX 1048575
#define FLOW_CMD_MPLS_LABEL_MIN 0
#define FLOW_CMD_MPLS_LABEL_LEN 4

static lagopus_result_t
mpls_label_cmd_parse(const char *const argv[],
                     struct ofp_flow_mod *flow_mod,
                     struct match_list *match_list,
                     struct instruction_list *instruction_list,
                     enum flow_cmd_type ftype,
                     lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_MPLS_LABEL_MIN,
                          FLOW_CMD_MPLS_LABEL_MAX,
                          OFPXMT_OFB_MPLS_LABEL,
                          FLOW_CMD_MPLS_LABEL_LEN,
                          value_uint32_hook,
                          NULL,
                          false,
                          result);
}

/* oxm field (mpls_tc). */
#define FLOW_CMD_MPLS_TC_MAX 7
#define FLOW_CMD_MPLS_TC_MIN 0
#define FLOW_CMD_MPLS_TC_LEN 1

static lagopus_result_t
mpls_tc_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_MPLS_TC_MIN,
                          FLOW_CMD_MPLS_TC_MAX,
                          OFPXMT_OFB_MPLS_TC,
                          FLOW_CMD_MPLS_TC_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* oxm field (mpls_bos). */
#define FLOW_CMD_MPLS_BOS_MAX 1
#define FLOW_CMD_MPLS_BOS_MIN 0
#define FLOW_CMD_MPLS_BOS_LEN 1

static lagopus_result_t
mpls_bos_cmd_parse(const char *const argv[],
                   struct ofp_flow_mod *flow_mod,
                   struct match_list *match_list,
                   struct instruction_list *instruction_list,
                   enum flow_cmd_type ftype,
                   lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_MPLS_BOS_MIN,
                          FLOW_CMD_MPLS_BOS_MAX,
                          OFPXMT_OFB_MPLS_BOS,
                          FLOW_CMD_MPLS_BOS_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* oxm field (pbb_isid). */
#define FLOW_CMD_PBB_ISID_MAX 16777215
#define FLOW_CMD_PBB_ISID_MIN 0
#define FLOW_CMD_PBB_ISID_LEN 3

static lagopus_result_t
pbb_isid_cmd_parse(const char *const argv[],
                   struct ofp_flow_mod *flow_mod,
                   struct match_list *match_list,
                   struct instruction_list *instruction_list,
                   enum flow_cmd_type ftype,
                   lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_PBB_ISID_MIN,
                          FLOW_CMD_PBB_ISID_MAX,
                          OFPXMT_OFB_PBB_ISID,
                          FLOW_CMD_PBB_ISID_LEN,
                          value_uint24_hook,
                          NULL,
                          true,
                          result);
}

/* oxm field (tunnel_id). */
#define FLOW_CMD_TUNNEL_ID_MAX UINT64_MAX
#define FLOW_CMD_TUNNEL_ID_MIN 0
#define FLOW_CMD_TUNNEL_ID_LEN 8

static lagopus_result_t
tunnel_id_cmd_parse(const char *const argv[],
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct instruction_list *instruction_list,
                    enum flow_cmd_type ftype,
                    lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_TUNNEL_ID_MIN,
                          FLOW_CMD_TUNNEL_ID_MAX,
                          OFPXMT_OFB_TUNNEL_ID,
                          FLOW_CMD_TUNNEL_ID_LEN,
                          value_uint64_hook,
                          NULL,
                          true,
                          result);
}

/* oxm field (ipv6_exthdr). */
#define FLOW_CMD_IPV6_EXTHDR_MAX 511
#define FLOW_CMD_IPV6_EXTHDR_MIN 0
#define FLOW_CMD_IPV6_EXTHDR_LEN 2

static lagopus_result_t
ipv6_exthdr_cmd_parse(const char *const argv[],
                      struct ofp_flow_mod *flow_mod,
                      struct match_list *match_list,
                      struct instruction_list *instruction_list,
                      enum flow_cmd_type ftype,
                      lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_IPV6_EXTHDR_MIN,
                          FLOW_CMD_IPV6_EXTHDR_MAX,
                          OFPXMT_OFB_IPV6_EXTHDR,
                          FLOW_CMD_IPV6_EXTHDR_LEN,
                          value_uint16_hook,
                          NULL,
                          true,
                          result);
}

/* oxm field (pbb_uca). */
#define FLOW_CMD_PBB_UCA_MAX 1
#define FLOW_CMD_PBB_UCA_MIN 0
#define FLOW_CMD_PBB_UCA_LEN 1

static lagopus_result_t
pbb_uca_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_PBB_UCA_MIN,
                          FLOW_CMD_PBB_UCA_MAX,
                          OFPXMT_OFB_PBB_UCA,
                          FLOW_CMD_PBB_UCA_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* oxm field (packet_type). */
#define FLOW_CMD_PACKET_TYPE_MAX UINT32_MAX
#define FLOW_CMD_PACKET_TYPE_MIN 0
#define FLOW_CMD_PACKET_TYPE_LEN 4

static lagopus_result_t
packet_type_cmd_parse(const char *const argv[],
                      struct ofp_flow_mod *flow_mod,
                      struct match_list *match_list,
                      struct instruction_list *instruction_list,
                      enum flow_cmd_type ftype,
                      lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_PACKET_TYPE_MIN,
                          FLOW_CMD_PACKET_TYPE_MAX,
                          OFPXMT_OFB_PACKET_TYPE,
                          FLOW_CMD_PACKET_TYPE_LEN,
                          value_uint32_hook,
                          NULL,
                          false,
                          result);
}

/* oxm field (gre_flags). */
#define FLOW_CMD_GRE_FLAGS_MAX 8191
#define FLOW_CMD_GRE_FLAGS_MIN 0
#define FLOW_CMD_GRE_FLAGS_LEN 2

static lagopus_result_t
gre_flags_cmd_parse(const char *const argv[],
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct instruction_list *instruction_list,
                    enum flow_cmd_type ftype,
                    lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_GRE_FLAGS_MIN,
                          FLOW_CMD_GRE_FLAGS_MAX,
                          OFPXMT_OFB_GRE_FLAGS,
                          FLOW_CMD_GRE_FLAGS_LEN,
                          value_uint16_hook,
                          NULL,
                          true,
                          result);
}

/* oxm field (gre_ver). */
#define FLOW_CMD_GRE_VER_MAX 7
#define FLOW_CMD_GRE_VER_MIN 0
#define FLOW_CMD_GRE_VER_LEN 1

static lagopus_result_t
gre_ver_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_GRE_VER_MIN,
                          FLOW_CMD_GRE_VER_MAX,
                          OFPXMT_OFB_GRE_VER,
                          FLOW_CMD_GRE_VER_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* oxm field (gre_protocol). */
#define FLOW_CMD_GRE_PROTOCOL_MAX UINT16_MAX
#define FLOW_CMD_GRE_PROTOCOL_MIN 0
#define FLOW_CMD_GRE_PROTOCOL_LEN 2

static lagopus_result_t
gre_protocol_cmd_parse(const char *const argv[],
                       struct ofp_flow_mod *flow_mod,
                       struct match_list *match_list,
                       struct instruction_list *instruction_list,
                       enum flow_cmd_type ftype,
                       lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_GRE_PROTOCOL_MIN,
                          FLOW_CMD_GRE_PROTOCOL_MAX,
                          OFPXMT_OFB_GRE_PROTOCOL,
                          FLOW_CMD_GRE_PROTOCOL_LEN,
                          value_uint16_hook,
                          NULL,
                          false,
                          result);
}

/* oxm field (gre_key). */
#define FLOW_CMD_GRE_KEY_MAX UINT32_MAX
#define FLOW_CMD_GRE_KEY_MIN 0
#define FLOW_CMD_GRE_KEY_LEN 4

static lagopus_result_t
gre_key_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_GRE_KEY_MIN,
                          FLOW_CMD_GRE_KEY_MAX,
                          OFPXMT_OFB_GRE_KEY,
                          FLOW_CMD_GRE_KEY_LEN,
                          value_uint32_hook,
                          NULL,
                          true,
                          result);
}

/* oxm field (gre_seqnum). */
#define FLOW_CMD_GRE_SEQNUM_MAX UINT32_MAX
#define FLOW_CMD_GRE_SEQNUM_MIN 0
#define FLOW_CMD_GRE_SEQNUM_LEN 4

static lagopus_result_t
gre_seqnum_cmd_parse(const char *const argv[],
                     struct ofp_flow_mod *flow_mod,
                     struct match_list *match_list,
                     struct instruction_list *instruction_list,
                     enum flow_cmd_type ftype,
                     lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_GRE_SEQNUM_MIN,
                          FLOW_CMD_GRE_SEQNUM_MAX,
                          OFPXMT_OFB_GRE_SEQNUM,
                          FLOW_CMD_GRE_SEQNUM_LEN,
                          value_uint32_hook,
                          NULL,
                          false,
                          result);
}

/* oxm field (lisp_flags). */
#define FLOW_CMD_LISP_FLAGS_MAX UINT8_MAX
#define FLOW_CMD_LISP_FLAGS_MIN 0
#define FLOW_CMD_LISP_FLAGS_LEN 1

static lagopus_result_t
lisp_flags_cmd_parse(const char *const argv[],
                     struct ofp_flow_mod *flow_mod,
                     struct match_list *match_list,
                     struct instruction_list *instruction_list,
                     enum flow_cmd_type ftype,
                     lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_LISP_FLAGS_MIN,
                          FLOW_CMD_LISP_FLAGS_MAX,
                          OFPXMT_OFB_LISP_FLAGS,
                          FLOW_CMD_LISP_FLAGS_LEN,
                          NULL,
                          NULL,
                          true,
                          result);
}

/* oxm field (lisp_nonce). */
#define FLOW_CMD_LISP_NONCE_MAX 16777215
#define FLOW_CMD_LISP_NONCE_MIN 0
#define FLOW_CMD_LISP_NONCE_LEN 3

static lagopus_result_t
lisp_nonce_cmd_parse(const char *const argv[],
                     struct ofp_flow_mod *flow_mod,
                     struct match_list *match_list,
                     struct instruction_list *instruction_list,
                     enum flow_cmd_type ftype,
                     lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_LISP_NONCE_MIN,
                          FLOW_CMD_LISP_NONCE_MAX,
                          OFPXMT_OFB_LISP_NONCE,
                          FLOW_CMD_LISP_NONCE_LEN,
                          value_uint24_hook,
                          NULL,
                          true,
                          result);
}

/* oxm field (lisp_id). */
#define FLOW_CMD_LISP_ID_MAX UINT32_MAX
#define FLOW_CMD_LISP_ID_MIN 0
#define FLOW_CMD_LISP_ID_LEN 4

static lagopus_result_t
lisp_id_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_LISP_ID_MIN,
                          FLOW_CMD_LISP_ID_MAX,
                          OFPXMT_OFB_LISP_ID,
                          FLOW_CMD_LISP_ID_LEN,
                          value_uint32_hook,
                          NULL,
                          false,
                          result);
}

/* oxm field (vxlan_flags). */
#define FLOW_CMD_VXLAN_FLAGS_MAX UINT8_MAX
#define FLOW_CMD_VXLAN_FLAGS_MIN 0
#define FLOW_CMD_VXLAN_FLAGS_LEN 1

static lagopus_result_t
vxlan_flags_cmd_parse(const char *const argv[],
                      struct ofp_flow_mod *flow_mod,
                      struct match_list *match_list,
                      struct instruction_list *instruction_list,
                      enum flow_cmd_type ftype,
                      lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_VXLAN_FLAGS_MIN,
                          FLOW_CMD_VXLAN_FLAGS_MAX,
                          OFPXMT_OFB_VXLAN_FLAGS,
                          FLOW_CMD_VXLAN_FLAGS_LEN,
                          NULL,
                          NULL,
                          true,
                          result);
}

/* oxm field (vxlan_vni). */
#define FLOW_CMD_VXLAN_VNI_MAX 16777215
#define FLOW_CMD_VXLAN_VNI_MIN 0
#define FLOW_CMD_VXLAN_VNI_LEN 3

static lagopus_result_t
vxlan_vni_cmd_parse(const char *const argv[],
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct instruction_list *instruction_list,
                    enum flow_cmd_type ftype,
                    lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_VXLAN_VNI_MIN,
                          FLOW_CMD_VXLAN_VNI_MAX,
                          OFPXMT_OFB_VXLAN_VNI,
                          FLOW_CMD_VXLAN_VNI_LEN,
                          value_uint24_hook,
                          NULL,
                          false,
                          result);
}

/* oxm field (mpls_data_first_nibble). */
#define FLOW_CMD_MPLS_DATA_FIRST_NIBBLE_MAX 15
#define FLOW_CMD_MPLS_DATA_FIRST_NIBBLE_MIN 0
#define FLOW_CMD_MPLS_DATA_FIRST_NIBBLE_LEN 1

static lagopus_result_t
mpls_data_first_nibble_cmd_parse(const char *const argv[],
                                 struct ofp_flow_mod *flow_mod,
                                 struct match_list *match_list,
                                 struct instruction_list *instruction_list,
                                 enum flow_cmd_type ftype,
                                 lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_MPLS_DATA_FIRST_NIBBLE_MIN,
                          FLOW_CMD_MPLS_DATA_FIRST_NIBBLE_MAX,
                          OFPXMT_OFB_MPLS_DATA_FIRST_NIBBLE,
                          FLOW_CMD_MPLS_DATA_FIRST_NIBBLE_LEN,
                          NULL,
                          NULL,
                          true,
                          result);
}

/* oxm field (mpls_ach_version). */
#define FLOW_CMD_MPLS_ACH_VERSION_MAX 15
#define FLOW_CMD_MPLS_ACH_VERSION_MIN 0
#define FLOW_CMD_MPLS_ACH_VERSION_LEN 1

static lagopus_result_t
mpls_ach_version_cmd_parse(const char *const argv[],
                           struct ofp_flow_mod *flow_mod,
                           struct match_list *match_list,
                           struct instruction_list *instruction_list,
                           enum flow_cmd_type ftype,
                           lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_MPLS_ACH_VERSION_MIN,
                          FLOW_CMD_MPLS_ACH_VERSION_MAX,
                          OFPXMT_OFB_MPLS_ACH_VERSION,
                          FLOW_CMD_MPLS_ACH_VERSION_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* oxm field (mpls_ach_channel). */
#define FLOW_CMD_MPLS_ACH_CHANNEL_MAX UINT16_MAX
#define FLOW_CMD_MPLS_ACH_CHANNEL_MIN 0
#define FLOW_CMD_MPLS_ACH_CHANNEL_LEN 2

static lagopus_result_t
mpls_ach_channel_cmd_parse(const char *const argv[],
                           struct ofp_flow_mod *flow_mod,
                           struct match_list *match_list,
                           struct instruction_list *instruction_list,
                           enum flow_cmd_type ftype,
                           lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_MPLS_ACH_CHANNEL_MIN,
                          FLOW_CMD_MPLS_ACH_CHANNEL_MAX,
                          OFPXMT_OFB_MPLS_ACH_CHANNEL,
                          FLOW_CMD_MPLS_ACH_CHANNEL_LEN,
                          value_uint16_hook,
                          NULL,
                          true,
                          result);
}

/* oxm field (mpls_pw_metadata). */
#define FLOW_CMD_MPLS_PW_METADATA_MAX 1
#define FLOW_CMD_MPLS_PW_METADATA_MIN 0
#define FLOW_CMD_MPLS_PW_METADATA_LEN 1

static lagopus_result_t
mpls_pw_metadata_cmd_parse(const char *const argv[],
                           struct ofp_flow_mod *flow_mod,
                           struct match_list *match_list,
                           struct instruction_list *instruction_list,
                           enum flow_cmd_type ftype,
                           lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_MPLS_PW_METADATA_MIN,
                          FLOW_CMD_MPLS_PW_METADATA_MAX,
                          OFPXMT_OFB_MPLS_PW_METADATA,
                          FLOW_CMD_MPLS_PW_METADATA_LEN,
                          NULL,
                          NULL,
                          true,
                          result);
}

/* oxm field (mpls_cw_flags). */
#define FLOW_CMD_MPLS_CW_FLAGS_MAX 15
#define FLOW_CMD_MPLS_CW_FLAGS_MIN 0
#define FLOW_CMD_MPLS_CW_FLAGS_LEN 1

static lagopus_result_t
mpls_cw_flags_cmd_parse(const char *const argv[],
                        struct ofp_flow_mod *flow_mod,
                        struct match_list *match_list,
                        struct instruction_list *instruction_list,
                        enum flow_cmd_type ftype,
                        lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_MPLS_CW_FLAGS_MIN,
                          FLOW_CMD_MPLS_CW_FLAGS_MAX,
                          OFPXMT_OFB_MPLS_CW_FLAGS,
                          FLOW_CMD_MPLS_CW_FLAGS_LEN,
                          NULL,
                          NULL,
                          true,
                          result);
}

/* oxm field (mpls_cw_frag). */
#define FLOW_CMD_MPLS_CW_FRAG_MAX 3
#define FLOW_CMD_MPLS_CW_FRAG_MIN 0
#define FLOW_CMD_MPLS_CW_FRAG_LEN 1

static lagopus_result_t
mpls_cw_frag_cmd_parse(const char *const argv[],
                       struct ofp_flow_mod *flow_mod,
                       struct match_list *match_list,
                       struct instruction_list *instruction_list,
                       enum flow_cmd_type ftype,
                       lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_MPLS_CW_FRAG_MIN,
                          FLOW_CMD_MPLS_CW_FRAG_MAX,
                          OFPXMT_OFB_MPLS_CW_FRAG,
                          FLOW_CMD_MPLS_CW_FRAG_LEN,
                          NULL,
                          NULL,
                          true,
                          result);
}

/* oxm field (mpls_cw_len). */
#define FLOW_CMD_MPLS_CW_LEN_MAX 63
#define FLOW_CMD_MPLS_CW_LEN_MIN 0
#define FLOW_CMD_MPLS_CW_LEN_LEN 1

static lagopus_result_t
mpls_cw_len_cmd_parse(const char *const argv[],
                      struct ofp_flow_mod *flow_mod,
                      struct match_list *match_list,
                      struct instruction_list *instruction_list,
                      enum flow_cmd_type ftype,
                      lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_MPLS_CW_LEN_MIN,
                          FLOW_CMD_MPLS_CW_LEN_MAX,
                          OFPXMT_OFB_MPLS_CW_LEN,
                          FLOW_CMD_MPLS_CW_LEN_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* oxm field (mpls_cw_seq_num). */
#define FLOW_CMD_MPLS_CW_SEQ_NUM_MAX UINT16_MAX
#define FLOW_CMD_MPLS_CW_SEQ_NUM_MIN 0
#define FLOW_CMD_MPLS_CW_SEQ_NUM_LEN 2

static lagopus_result_t
mpls_cw_seq_num_cmd_parse(const char *const argv[],
                          struct ofp_flow_mod *flow_mod,
                          struct match_list *match_list,
                          struct instruction_list *instruction_list,
                          enum flow_cmd_type ftype,
                          lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_MPLS_CW_SEQ_NUM_MIN,
                          FLOW_CMD_MPLS_CW_SEQ_NUM_MAX,
                          OFPXMT_OFB_MPLS_CW_SEQ_NUM,
                          FLOW_CMD_MPLS_CW_SEQ_NUM_LEN,
                          value_uint16_hook,
                          NULL,
                          false,
                          result);
}

/* output. */
#define FLOW_CMD_OUTPUT_MAX UINT32_MAX
#define FLOW_CMD_OUTPUT_MIN 0
#define FLOW_CMD_OUTPUT_LEN \
  sizeof(struct ofp_action_output) - sizeof(struct ofp_action_header)

static lagopus_result_t
output_cmd_parse(const char *const argv[],
                 struct ofp_flow_mod *flow_mod,
                 struct match_list *match_list,
                 struct instruction_list *instruction_list,
                 enum flow_cmd_type ftype,
                 lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_OUTPUT_MIN,
                          FLOW_CMD_OUTPUT_MAX,
                          OFPAT_OUTPUT,
                          FLOW_CMD_OUTPUT_LEN,
                          NULL,
                          &flow_mod_port_str_table,
                          false,
                          result);
}

/* copy_ttl_out. */
static lagopus_result_t
copy_ttl_out_cmd_parse(const char *const argv[],
                       struct ofp_flow_mod *flow_mod,
                       struct match_list *match_list,
                       struct instruction_list *instruction_list,
                       enum flow_cmd_type ftype,
                       lagopus_dstring_t *result) {
  return one_cmd_field_parse((char ***) &argv, flow_mod,
                             match_list, instruction_list,
                             ftype,
                             OFPAT_COPY_TTL_OUT,
                             NULL,
                             result);
}

/* copy_ttl_in. */
static lagopus_result_t
copy_ttl_in_cmd_parse(const char *const argv[],
                      struct ofp_flow_mod *flow_mod,
                      struct match_list *match_list,
                      struct instruction_list *instruction_list,
                      enum flow_cmd_type ftype,
                      lagopus_dstring_t *result) {
  return one_cmd_field_parse((char ***) &argv, flow_mod,
                             match_list, instruction_list,
                             ftype,
                             OFPAT_COPY_TTL_IN,
                             NULL,
                             result);
}

/* set_mpls_ttl. */
#define FLOW_CMD_SET_MPLS_TTL_MAX UINT8_MAX
#define FLOW_CMD_SET_MPLS_TTL_MIN 0
#define FLOW_CMD_SET_MPLS_TTL_LEN \
  sizeof(struct ofp_action_mpls_ttl) - sizeof(struct ofp_action_header)

static lagopus_result_t
set_mpls_ttl_cmd_parse(const char *const argv[],
                       struct ofp_flow_mod *flow_mod,
                       struct match_list *match_list,
                       struct instruction_list *instruction_list,
                       enum flow_cmd_type ftype,
                       lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_SET_MPLS_TTL_MIN,
                          FLOW_CMD_SET_MPLS_TTL_MAX,
                          OFPAT_SET_MPLS_TTL,
                          FLOW_CMD_SET_MPLS_TTL_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* dec_mpls_ttl. */
static lagopus_result_t
dec_mpls_ttl_cmd_parse(const char *const argv[],
                       struct ofp_flow_mod *flow_mod,
                       struct match_list *match_list,
                       struct instruction_list *instruction_list,
                       enum flow_cmd_type ftype,
                       lagopus_dstring_t *result) {
  return one_cmd_field_parse((char ***) &argv, flow_mod,
                             match_list, instruction_list,
                             ftype,
                             OFPAT_DEC_MPLS_TTL,
                             NULL,
                             result);
}

/* push_vlan. */
#define FLOW_CMD_PUSH_VLAN_MAX UINT16_MAX
#define FLOW_CMD_PUSH_VLAN_MIN 0
#define FLOW_CMD_PUSH_VLAN_LEN \
  sizeof(struct ofp_action_push) - sizeof(struct ofp_action_header)

static lagopus_result_t
push_vlan_cmd_parse(const char *const argv[],
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct instruction_list *instruction_list,
                    enum flow_cmd_type ftype,
                    lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_PUSH_VLAN_MIN,
                          FLOW_CMD_PUSH_VLAN_MAX,
                          OFPAT_PUSH_VLAN,
                          FLOW_CMD_PUSH_VLAN_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* strip_vlan. */
static lagopus_result_t
strip_vlan_cmd_parse(const char *const argv[],
                     struct ofp_flow_mod *flow_mod,
                     struct match_list *match_list,
                     struct instruction_list *instruction_list,
                     enum flow_cmd_type ftype,
                     lagopus_dstring_t *result) {
  return one_cmd_field_parse((char ***) &argv, flow_mod,
                             match_list, instruction_list,
                             ftype,
                             OFPAT_POP_VLAN,
                             NULL,
                             result);
}

/* push_mpls. */
#define FLOW_CMD_PUSH_MPLS_MAX UINT16_MAX
#define FLOW_CMD_PUSH_MPLS_MIN 0
#define FLOW_CMD_PUSH_MPLS_LEN \
  sizeof(struct ofp_action_push) - sizeof(struct ofp_action_header)

static lagopus_result_t
push_mpls_cmd_parse(const char *const argv[],
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct instruction_list *instruction_list,
                    enum flow_cmd_type ftype,
                    lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_PUSH_MPLS_MIN,
                          FLOW_CMD_PUSH_MPLS_MAX,
                          OFPAT_PUSH_MPLS,
                          FLOW_CMD_PUSH_MPLS_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* pop_mpls. */
#define FLOW_CMD_POP_MPLS_MAX UINT16_MAX
#define FLOW_CMD_POP_MPLS_MIN 0
#define FLOW_CMD_POP_MPLS_LEN \
  sizeof(struct ofp_action_pop_mpls) - sizeof(struct ofp_action_header)

static lagopus_result_t
pop_mpls_cmd_parse(const char *const argv[],
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct instruction_list *instruction_list,
                    enum flow_cmd_type ftype,
                    lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_POP_MPLS_MIN,
                          FLOW_CMD_POP_MPLS_MAX,
                          OFPAT_POP_MPLS,
                          FLOW_CMD_POP_MPLS_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* set_queue. */
#define FLOW_CMD_SET_QUEUE_MAX UINT32_MAX
#define FLOW_CMD_SET_QUEUE_MIN 0
#define FLOW_CMD_SET_QUEUE_LEN \
  sizeof(struct ofp_action_set_queue) - sizeof(struct ofp_action_header)

static lagopus_result_t
set_queue_cmd_parse(const char *const argv[],
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct instruction_list *instruction_list,
                    enum flow_cmd_type ftype,
                    lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_SET_QUEUE_MIN,
                          FLOW_CMD_SET_QUEUE_MAX,
                          OFPAT_SET_QUEUE,
                          FLOW_CMD_SET_QUEUE_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* group. */
#define FLOW_CMD_GROUP_MAX UINT32_MAX
#define FLOW_CMD_GROUP_MIN 0
#define FLOW_CMD_GROUP_LEN \
  sizeof(struct ofp_action_group) - sizeof(struct ofp_action_header)

static lagopus_result_t
group_cmd_parse(const char *const argv[],
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct instruction_list *instruction_list,
                    enum flow_cmd_type ftype,
                    lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_GROUP_MIN,
                          FLOW_CMD_GROUP_MAX,
                          OFPAT_GROUP,
                          FLOW_CMD_GROUP_LEN,
                          NULL,
                          &flow_mod_group_str_table,
                          false,
                          result);
}

/* set_nw_ttl. */
#define FLOW_CMD_SET_NW_TTL_MAX UINT8_MAX
#define FLOW_CMD_SET_NW_TTL_MIN 0
#define FLOW_CMD_SET_NW_TTL_LEN \
  sizeof(struct ofp_action_nw_ttl) - sizeof(struct ofp_action_header)

static lagopus_result_t
set_nw_ttl_cmd_parse(const char *const argv[],
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct instruction_list *instruction_list,
                    enum flow_cmd_type ftype,
                    lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_SET_NW_TTL_MIN,
                          FLOW_CMD_SET_NW_TTL_MAX,
                          OFPAT_SET_NW_TTL,
                          FLOW_CMD_SET_NW_TTL_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* dec_nw_ttl. */
static lagopus_result_t
dec_nw_ttl_cmd_parse(const char *const argv[],
                     struct ofp_flow_mod *flow_mod,
                     struct match_list *match_list,
                     struct instruction_list *instruction_list,
                     enum flow_cmd_type ftype,
                     lagopus_dstring_t *result) {
  return one_cmd_field_parse((char ***) &argv, flow_mod,
                             match_list, instruction_list,
                             ftype,
                             OFPAT_DEC_NW_TTL,
                             NULL,
                             result);
}

/* push_pbb. */
#define FLOW_CMD_PUSH_PBB_MAX UINT16_MAX
#define FLOW_CMD_PUSH_PBB_MIN 0
#define FLOW_CMD_PUSH_PBB_LEN \
  sizeof(struct ofp_action_push) - sizeof(struct ofp_action_header)

static lagopus_result_t
push_pbb_cmd_parse(const char *const argv[],
                    struct ofp_flow_mod *flow_mod,
                    struct match_list *match_list,
                    struct instruction_list *instruction_list,
                    enum flow_cmd_type ftype,
                    lagopus_dstring_t *result) {
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          ftype,
                          FLOW_CMD_PUSH_PBB_MIN,
                          FLOW_CMD_PUSH_PBB_MAX,
                          OFPAT_PUSH_PBB,
                          FLOW_CMD_PUSH_PBB_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* pop_pbb. */
static lagopus_result_t
pop_pbb_cmd_parse(const char *const argv[],
                  struct ofp_flow_mod *flow_mod,
                  struct match_list *match_list,
                  struct instruction_list *instruction_list,
                  enum flow_cmd_type ftype,
                  lagopus_dstring_t *result) {
  return one_cmd_field_parse((char ***) &argv, flow_mod,
                             match_list, instruction_list,
                             ftype,
                             OFPAT_POP_PBB,
                             NULL,
                             result);
}

/* cookie. */
#define FLOW_CMD_COOKIE_MAX UINT64_MAX
#define FLOW_CMD_COOKIE_MIN 0
#define FLOW_CMD_COOKIE_LEN 0

static lagopus_result_t
cookie_cmd_parse(const char *const argv[],
                 struct ofp_flow_mod *flow_mod,
                 struct match_list *match_list,
                 struct instruction_list *instruction_list,
                 enum flow_cmd_type ftype,
                 lagopus_dstring_t *result) {
  (void) ftype;
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          FLOW_CMD_TYPE_FLOW_FIELD,
                          FLOW_CMD_COOKIE_MIN,
                          FLOW_CMD_COOKIE_MAX,
                          FLOW_FIELD_COOKIE,
                          FLOW_CMD_COOKIE_LEN,
                          NULL,
                          NULL,
                          true,
                          result);
}

/* table. */
#define FLOW_CMD_TABLE_MAX UINT8_MAX
#define FLOW_CMD_TABLE_MIN 0
#define FLOW_CMD_TABLE_LEN 0

static lagopus_result_t
table_cmd_parse(const char *const argv[],
                struct ofp_flow_mod *flow_mod,
                struct match_list *match_list,
                struct instruction_list *instruction_list,
                enum flow_cmd_type ftype,
                lagopus_dstring_t *result) {
  (void) ftype;
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          FLOW_CMD_TYPE_FLOW_FIELD,
                          FLOW_CMD_TABLE_MIN,
                          FLOW_CMD_TABLE_MAX,
                          FLOW_FIELD_TABLE,
                          FLOW_CMD_TABLE_LEN,
                          NULL,
                          &flow_mod_table_str_table,
                          false,
                          result);
}

/* idle_timeout. */
#define FLOW_CMD_IDLE_TIMEOUT_MAX UINT16_MAX
#define FLOW_CMD_IDLE_TIMEOUT_MIN 0
#define FLOW_CMD_IDLE_TIMEOUT_LEN 0

static lagopus_result_t
idle_timeout_cmd_parse(const char *const argv[],
                       struct ofp_flow_mod *flow_mod,
                       struct match_list *match_list,
                       struct instruction_list *instruction_list,
                       enum flow_cmd_type ftype,
                       lagopus_dstring_t *result) {
  (void) ftype;
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          FLOW_CMD_TYPE_FLOW_FIELD,
                          FLOW_CMD_IDLE_TIMEOUT_MIN,
                          FLOW_CMD_IDLE_TIMEOUT_MAX,
                          FLOW_FIELD_IDLE_TIMEOUT,
                          FLOW_CMD_IDLE_TIMEOUT_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* hard_timeout. */
#define FLOW_CMD_HARD_TIMEOUT_MAX UINT16_MAX
#define FLOW_CMD_HARD_TIMEOUT_MIN 0
#define FLOW_CMD_HARD_TIMEOUT_LEN 0

static lagopus_result_t
hard_timeout_cmd_parse(const char *const argv[],
                       struct ofp_flow_mod *flow_mod,
                       struct match_list *match_list,
                       struct instruction_list *instruction_list,
                       enum flow_cmd_type ftype,
                       lagopus_dstring_t *result) {
  (void) ftype;
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          FLOW_CMD_TYPE_FLOW_FIELD,
                          FLOW_CMD_HARD_TIMEOUT_MIN,
                          FLOW_CMD_HARD_TIMEOUT_MAX,
                          FLOW_FIELD_HARD_TIMEOUT,
                          FLOW_CMD_HARD_TIMEOUT_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* priority. */
#define FLOW_CMD_PRIORITY_MAX UINT16_MAX
#define FLOW_CMD_PRIORITY_MIN 0
#define FLOW_CMD_PRIORITY_LEN 0

static lagopus_result_t
priority_cmd_parse(const char *const argv[],
                       struct ofp_flow_mod *flow_mod,
                       struct match_list *match_list,
                       struct instruction_list *instruction_list,
                       enum flow_cmd_type ftype,
                       lagopus_dstring_t *result) {
  (void) ftype;
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          FLOW_CMD_TYPE_FLOW_FIELD,
                          FLOW_CMD_PRIORITY_MIN,
                          FLOW_CMD_PRIORITY_MAX,
                          FLOW_FIELD_PRIORITY,
                          FLOW_CMD_PRIORITY_LEN,
                          NULL,
                          NULL,
                          false,
                          result);
}

/* out_port. */
#define FLOW_CMD_OUT_PORT_MAX UINT32_MAX
#define FLOW_CMD_OUT_PORT_MIN 0
#define FLOW_CMD_OUT_PORT_LEN 0

static lagopus_result_t
out_port_cmd_parse(const char *const argv[],
                       struct ofp_flow_mod *flow_mod,
                       struct match_list *match_list,
                       struct instruction_list *instruction_list,
                       enum flow_cmd_type ftype,
                       lagopus_dstring_t *result) {
  (void) ftype;
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          FLOW_CMD_TYPE_FLOW_FIELD,
                          FLOW_CMD_OUT_PORT_MIN,
                          FLOW_CMD_OUT_PORT_MAX,
                          FLOW_FIELD_OUT_PORT,
                          FLOW_CMD_OUT_PORT_LEN,
                          NULL,
                          &flow_mod_port_str_table,
                          false,
                          result);
}

/* out_group. */
#define FLOW_CMD_OUT_GROUP_MAX UINT32_MAX
#define FLOW_CMD_OUT_GROUP_MIN 0
#define FLOW_CMD_OUT_GROUP_LEN 0

static lagopus_result_t
out_group_cmd_parse(const char *const argv[],
                       struct ofp_flow_mod *flow_mod,
                       struct match_list *match_list,
                       struct instruction_list *instruction_list,
                       enum flow_cmd_type ftype,
                       lagopus_dstring_t *result) {
  (void) ftype;
  return uint_field_parse((char ***) &argv, flow_mod,
                          match_list, instruction_list,
                          FLOW_CMD_TYPE_FLOW_FIELD,
                          FLOW_CMD_OUT_GROUP_MIN,
                          FLOW_CMD_OUT_GROUP_MAX,
                          FLOW_FIELD_OUT_GROUP,
                          FLOW_CMD_OUT_GROUP_LEN,
                          NULL,
                          &flow_mod_group_str_table,
                          false,
                          result);
}

/* send_flow_rem. */
static lagopus_result_t
send_flow_rem_cmd_parse(const char *const argv[],
                        struct ofp_flow_mod *flow_mod,
                        struct match_list *match_list,
                        struct instruction_list *instruction_list,
                        enum flow_cmd_type ftype,
                        lagopus_dstring_t *result) {
  (void) ftype;
  return one_cmd_field_parse((char ***) &argv, flow_mod,
                             match_list, instruction_list,
                             FLOW_CMD_TYPE_FLOW_FIELD,
                             FLOW_FIELD_SEND_FLOW_REM,
                             NULL,
                             result);
}

/* check_overlap. */
static lagopus_result_t
check_overlap_cmd_parse(const char *const argv[],
                        struct ofp_flow_mod *flow_mod,
                        struct match_list *match_list,
                        struct instruction_list *instruction_list,
                        enum flow_cmd_type ftype,
                        lagopus_dstring_t *result) {
  (void) ftype;
  return one_cmd_field_parse((char ***) &argv, flow_mod,
                             match_list, instruction_list,
                             FLOW_CMD_TYPE_FLOW_FIELD,
                             FLOW_FIELD_CHECK_OVERLAP,
                             NULL,
                             result);
}

/* strict opt. */
static lagopus_result_t
strict_cmd_parse(const char *const argv[],
                 struct ofp_flow_mod *flow_mod,
                 struct match_list *match_list,
                 struct instruction_list *instruction_list,
                 enum flow_cmd_type ftype,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) flow_mod;
  (void) match_list;
  (void) instruction_list;
  (void) ftype;

  if (argv != NULL && flow_mod != NULL) {
    switch (flow_mod->command) {
      case OFPFC_MODIFY:
      case OFPFC_MODIFY_STRICT:
        flow_mod->command = OFPFC_MODIFY_STRICT;
        ret = LAGOPUS_RESULT_OK;
        break;
      case OFPFC_DELETE:
      case OFPFC_DELETE_STRICT:
        flow_mod->command = OFPFC_DELETE_STRICT;
        ret = LAGOPUS_RESULT_OK;
        break;
      default:
        ret = datastore_json_result_string_setf(
            result,
            LAGOPUS_RESULT_INVALID_ARGS,
            "Bad opt (%s).",
            argv[0]);
        break;
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

/*
 * parse instructions.
 */

/* goto_table. */
static lagopus_result_t
goto_table_cmd_parse(const char *const argv[],
                     struct ofp_flow_mod *flow_mod,
                     struct match_list *match_list,
                     struct instruction_list *instruction_list,
                     enum flow_cmd_type ftype,
                     lagopus_dstring_t *result) {
  (void) ftype;
  return instructions_parse((char ***) &argv, flow_mod,
                            match_list, instruction_list,
                            OFPIT_GOTO_TABLE,
                            result);
}

/* write_metadata. */
static lagopus_result_t
write_metadata_cmd_parse(const char *const argv[],
                         struct ofp_flow_mod *flow_mod,
                         struct match_list *match_list,
                         struct instruction_list *instruction_list,
                         enum flow_cmd_type ftype,
                         lagopus_dstring_t *result) {
  (void) ftype;
  return instructions_parse((char ***) &argv, flow_mod,
                            match_list, instruction_list,
                            OFPIT_WRITE_METADATA,
                            result);
}

/* write_actions. */
static lagopus_result_t
write_actions_cmd_parse(const char *const argv[],
                        struct ofp_flow_mod *flow_mod,
                        struct match_list *match_list,
                        struct instruction_list *instruction_list,
                        enum flow_cmd_type ftype,
                        lagopus_dstring_t *result) {
  (void) ftype;
  return instructions_parse((char ***) &argv, flow_mod,
                            match_list, instruction_list,
                            OFPIT_WRITE_ACTIONS,
                            result);
}

/* apply_actions. */
static lagopus_result_t
apply_actions_cmd_parse(const char *const argv[],
                        struct ofp_flow_mod *flow_mod,
                        struct match_list *match_list,
                        struct instruction_list *instruction_list,
                        enum flow_cmd_type ftype,
                        lagopus_dstring_t *result) {
  (void) ftype;
  return instructions_parse((char ***) &argv, flow_mod,
                            match_list, instruction_list,
                            OFPIT_APPLY_ACTIONS,
                            result);
}

/* clear_actions. */
static lagopus_result_t
clear_actions_cmd_parse(const char *const argv[],
                        struct ofp_flow_mod *flow_mod,
                        struct match_list *match_list,
                        struct instruction_list *instruction_list,
                        enum flow_cmd_type ftype,
                        lagopus_dstring_t *result) {
  (void) ftype;
  return instructions_parse((char ***) &argv, flow_mod,
                            match_list, instruction_list,
                            OFPIT_CLEAR_ACTIONS,
                            result);
}

/* meter. */
static lagopus_result_t
meter_cmd_parse(const char *const argv[],
                struct ofp_flow_mod *flow_mod,
                struct match_list *match_list,
                struct instruction_list *instruction_list,
                enum flow_cmd_type ftype,
                lagopus_dstring_t *result) {
  (void) ftype;
  return instructions_parse((char ***) &argv, flow_mod,
                            match_list, instruction_list,
                            OFPIT_METER,
                            result);
}

static inline void
flow_mod_init(struct ofp_flow_mod *flow_mod,
              uint8_t command) {
  memset(flow_mod, 0, sizeof(struct ofp_flow_mod));
  flow_mod->command = command;
  flow_mod->buffer_id = OFP_NO_BUFFER;
  flow_mod->out_port = OFPP_ANY;
  flow_mod->out_group = OFPG_ANY;
}

static inline lagopus_result_t
flow_cmd_mod_cmd_parse_internal(const char *const argv[],
                                struct ofp_flow_mod *flow_mod,
                                struct match_list *match_list,
                                struct instruction_list *instruction_list,
                                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t n_tokens = LAGOPUS_RESULT_ANY_FAILURES;
  void *opt_proc;
  char *co = NULL;
  char *key = NULL;
  char *tokens[TOKEN_MAX + 1];

  if (argv != NULL && result != NULL) {
    if (*argv != NULL && *(argv + 1) != NULL) {
      argv++;
      while (*argv != NULL) {
        if (IS_VALID_STRING(*argv) == true) {
          co = strdup(*argv);

          if (co == NULL) {
            ret = datastore_json_result_set(result,
                                            LAGOPUS_RESULT_NO_MEMORY,
                                            NULL);
            goto loop_end;
          }

          memset((void *) tokens, 0, sizeof(tokens));
          n_tokens = lagopus_str_tokenize(co, tokens,
                                          TOKEN_MAX + 1, "=");

          if (n_tokens <= 0 && n_tokens <= TOKEN_MAX) {
            ret = datastore_json_result_set(result,
                                            LAGOPUS_RESULT_INVALID_ARGS,
                                            NULL);
            goto loop_end;
          }

          if ((ret = lagopus_str_trim(tokens[0], " \t\r\n", &key)) <= 0) {
            if (ret == 0) {
              ret = LAGOPUS_RESULT_INVALID_ARGS;
            }
            ret = datastore_json_result_string_setf(result,
                                                    ret,
                                                    "Bad value (%s).",
                                                    tokens[0]);
            goto loop_end;
          }

          if ((ret = lagopus_hashmap_find(
                  &flow_mod_cmd_table,
                  (void *) key,
                  &opt_proc)) != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_string_setf(result,
                                                    LAGOPUS_RESULT_NOT_FOUND,
                                                    "Not found cmd (%s).",
                                                    key);
            goto loop_end;
          }

          if (opt_proc != NULL) {
            ret = ((flow_mod_cmd_proc_t) opt_proc)(
                (const char * const **)tokens,
                flow_mod,
                match_list,
                instruction_list,
                FLOW_CMD_TYPE_MATCH,
                result);
          } else {
            ret = datastore_json_result_string_setf(
                result,
                LAGOPUS_RESULT_NOT_FOUND,
                "Not found parse func (%s).",
                tokens[0]);
            goto loop_end;
          }
        } else {
          ret = datastore_json_result_set(result,
                                          LAGOPUS_RESULT_INVALID_ARGS,
                                          NULL);
        }
        argv++;

     loop_end:
        /* free. */
        free(co);
        co = NULL;
        free(key);
        key = NULL;
        if (ret != LAGOPUS_RESULT_OK) {
          break;
        }
      }
    } else {
      ret = datastore_json_result_set(result,
                                      LAGOPUS_RESULT_INVALID_ARGS,
                                      NULL);
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

/*
 * parse add cmd.
 */
static inline lagopus_result_t
flow_cmd_mod_add_cmd_parse(datastore_interp_t *iptr,
                           datastore_interp_state_t state,
                           size_t argc, const char *const argv[],
                           char *name,
                           void *out_configs,
                           uint64_t dpid,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct ofp_error error;
  (void) state;
  (void) argc;

  if (iptr != NULL && argv != NULL && name != NULL &&
      out_configs != NULL && result != NULL) {
    /* Init lists. */
    TAILQ_INIT(&match_list);
    TAILQ_INIT(&instruction_list);
    flow_mod_init(&flow_mod, OFPFC_ADD);

    if ((ret = flow_cmd_mod_cmd_parse_internal(argv,
                                               &flow_mod,
                                               &match_list,
                                               &instruction_list,
                                               result)) !=
        LAGOPUS_RESULT_OK) {
      goto done;
    }

    ret = ofp_flow_mod_check_add(dpid, &flow_mod,
                                 &match_list, &instruction_list,
                                 &error);
    if (ret != LAGOPUS_RESULT_OK) {
      if (ret == LAGOPUS_RESULT_OFP_ERROR) {
        ret = datastore_json_result_string_setf(
            result,
            ret,
            "Can't flow mod (ADD), ofp_error[type = %s, code = %s].",
            ofp_error_type_str_get(error.type),
            ofp_error_code_str_get(error.type, error.code));
      } else {
        ret = datastore_json_result_string_setf(
            result,
            ret,
            "Can't flow mod (ADD).");
      }
    }

 done:
    /* free. */
    ofp_instruction_list_elem_free(&instruction_list);
    ofp_match_list_elem_free(&match_list);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

/*
 * parse mod cmd.
 */
static inline lagopus_result_t
flow_cmd_mod_mod_cmd_parse(datastore_interp_t *iptr,
                           datastore_interp_state_t state,
                           size_t argc, const char *const argv[],
                           char *name,
                           void *out_configs,
                           uint64_t dpid,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct ofp_error error;
  (void) state;
  (void) argc;

  if (iptr != NULL && argv != NULL && name != NULL &&
      out_configs != NULL && result != NULL) {
    /* Init lists. */
    TAILQ_INIT(&match_list);
    TAILQ_INIT(&instruction_list);
    flow_mod_init(&flow_mod, OFPFC_MODIFY);

    if ((ret = flow_cmd_mod_cmd_parse_internal(argv,
                                               &flow_mod,
                                               &match_list,
                                               &instruction_list,
                                               result)) !=
        LAGOPUS_RESULT_OK) {
      goto done;
    }

    ret = ofp_flow_mod_modify(dpid, &flow_mod,
                              &match_list, &instruction_list,
                              &error);
    if (ret != LAGOPUS_RESULT_OK) {
      if (ret == LAGOPUS_RESULT_OFP_ERROR) {
        ret = datastore_json_result_string_setf(
            result,
            ret,
            "Can't flow mod (MOD), ofp_error[type = %s, code = %s].",
            ofp_error_type_str_get(error.type),
            ofp_error_code_str_get(error.type, error.code));
      } else {
        ret = datastore_json_result_string_setf(
            result,
            ret,
            "Can't flow mod (MOD).");
      }
    }

 done:
    /* free. */
    ofp_instruction_list_elem_free(&instruction_list);
    ofp_match_list_elem_free(&match_list);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

/*
 * parse del cmd.
 */
static inline lagopus_result_t
flow_cmd_mod_del_cmd_parse(datastore_interp_t *iptr,
                           datastore_interp_state_t state,
                           size_t argc, const char *const argv[],
                           char *name,
                           void *out_configs,
                           uint64_t dpid,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct ofp_error error;
  (void) state;
  (void) argc;

  if (iptr != NULL && argv != NULL && name != NULL &&
      out_configs != NULL && result != NULL) {
    /* Init lists. */
    TAILQ_INIT(&match_list);
    TAILQ_INIT(&instruction_list);
    flow_mod_init(&flow_mod, OFPFC_DELETE);

    if (*argv != NULL && *(argv + 1) != NULL) {
      if ((ret = flow_cmd_mod_cmd_parse_internal(argv,
                                                 &flow_mod,
                                                 &match_list,
                                                 &instruction_list,
                                                 result)) !=
          LAGOPUS_RESULT_OK) {
        goto done;
      }
    }

    ret = ofp_flow_mod_delete(dpid, &flow_mod,
                              &match_list,
                              &error);
    if (ret != LAGOPUS_RESULT_OK) {
      if (ret == LAGOPUS_RESULT_OFP_ERROR) {
        ret = datastore_json_result_string_setf(
            result,
            ret,
            "Can't flow mod (DEL), ofp_error[type = %s, code = %s].",
            ofp_error_type_str_get(error.type),
            ofp_error_code_str_get(error.type, error.code));
      } else {
        ret = datastore_json_result_string_setf(
            result,
            ret,
            "Can't flow mod (DEL).");
      }
    }

 done:
    /* free. */
    ofp_instruction_list_elem_free(&instruction_list);
    ofp_match_list_elem_free(&match_list);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static inline lagopus_result_t
cmd_add(const char *name, flow_mod_cmd_proc_t proc,
        lagopus_hashmap_t *table) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  void *val = (void *)proc;

  if (table != NULL && *table  != NULL &&
      name != NULL) {
    if (val != NULL) {
      ret = lagopus_hashmap_add(table, (void *) name, &val, false);
    } else {
      /* ignore proc is NULL. */
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

  return ret;
}

static inline lagopus_result_t
to_num_table_add(const char *name,
                 uint64_t *num,
                 lagopus_hashmap_t *table) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  void *val = (void *) num;

  if (table != NULL && *table  != NULL &&
      name != NULL && val != NULL) {
    ret = lagopus_hashmap_add(table, (void *) name, &val, false);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

  return ret;
}

static void*
flow_match_field_procs[FLOW_MATCH_FIELD_MAX]  = {
  in_port_cmd_parse,                 /* FLOW_MATCH_FIELD_IN_PORT */
  in_phy_port_cmd_parse,             /* FLOW_MATCH_FIELD_IN_PHY_PORT */
  metadata_cmd_parse,                /* FLOW_MATCH_FIELD_METADATA */
  dl_dst_cmd_parse,                  /* FLOW_MATCH_FIELD_DL_DST */
  dl_src_cmd_parse,                  /* FLOW_MATCH_FIELD_DL_SRC */
  dl_type_cmd_parse,                 /* FLOW_MATCH_FIELD_DL_TYPE */
  vlan_vid_cmd_parse,                /* FLOW_MATCH_FIELD_VLAN_VID */
  vlan_pcp_cmd_parse,                /* FLOW_MATCH_FIELD_VLAN_PCP */
  ip_dscp_cmd_parse,                 /* FLOW_MATCH_FIELD_IP_DSCP */
  nw_ecn_cmd_parse,                  /* FLOW_MATCH_FIELD_NW_ECN */
  nw_proto_cmd_parse,                /* FLOW_MATCH_FIELD_NW_PROTO */
  nw_src_cmd_parse,                  /* FLOW_MATCH_FIELD_NW_SRC */
  nw_dst_cmd_parse,                  /* FLOW_MATCH_FIELD_NW_DST */
  tcp_src_cmd_parse,                 /* FLOW_MATCH_FIELD_TCP_SRC */
  tcp_dst_cmd_parse,                 /* FLOW_MATCH_FIELD_TCP_DST */
  udp_src_cmd_parse,                 /* FLOW_MATCH_FIELD_UDP_SRC */
  udp_dst_cmd_parse,                 /* FLOW_MATCH_FIELD_UDP_DST */
  sctp_src_cmd_parse,                /* FLOW_MATCH_FIELD_SCTP_SRC */
  sctp_dst_cmd_parse,                /* FLOW_MATCH_FIELD_SCTP_DST */
  icmp_type_cmd_parse,               /* FLOW_MATCH_FIELD_ICMP_TYPE */
  icmp_code_cmd_parse,               /* FLOW_MATCH_FIELD_ICMP_CODE */
  arp_op_cmd_parse,                  /* FLOW_MATCH_FIELD_ARP_OP */
  arp_spa_cmd_parse,                 /* FLOW_MATCH_FIELD_ARP_SPA */
  arp_tpa_cmd_parse,                 /* FLOW_MATCH_FIELD_ARP_TPA */
  arp_sha_cmd_parse,                 /* FLOW_MATCH_FIELD_ARP_SHA */
  arp_tha_cmd_parse,                 /* FLOW_MATCH_FIELD_ARP_THA */
  ipv6_src_cmd_parse,                /* FLOW_MATCH_FIELD_IPV6_SRC */
  ipv6_dst_cmd_parse,                /* FLOW_MATCH_FIELD_IPV6_DST */
  ipv6_label_cmd_parse,              /* FLOW_MATCH_FIELD_IPV6_LABEL */
  icmpv6_type_cmd_parse,             /* FLOW_MATCH_FIELD_ICMPV6_TYPE */
  icmpv6_code_cmd_parse,             /* FLOW_MATCH_FIELD_ICMPV6_CODE */
  nd_target_cmd_parse,               /* FLOW_MATCH_FIELD_ND_TARGET */
  nd_sll_cmd_parse,                  /* FLOW_MATCH_FIELD_ND_SLL */
  nd_tll_cmd_parse,                  /* FLOW_MATCH_FIELD_ND_TLL */
  mpls_label_cmd_parse,              /* FLOW_MATCH_FIELD_MPLS_LABEL */
  mpls_tc_cmd_parse,                 /* FLOW_MATCH_FIELD_MPLS_TC */
  mpls_bos_cmd_parse,                /* FLOW_MATCH_FIELD_MPLS_BOS */
  pbb_isid_cmd_parse,                /* FLOW_MATCH_FIELD_PBB_ISID */
  tunnel_id_cmd_parse,               /* FLOW_MATCH_FIELD_TUNNEL_ID */
  ipv6_exthdr_cmd_parse,             /* FLOW_MATCH_FIELD_IPV6_EXTHDR */
  pbb_uca_cmd_parse,                 /* FLOW_MATCH_FIELD_PBB_UCA */
  packet_type_cmd_parse,             /* FLOW_MATCH_FIELD_PACKET_TYPE */
  gre_flags_cmd_parse,               /* FLOW_MATCH_FIELD_GRE_FLAGS */
  gre_ver_cmd_parse,                 /* FLOW_MATCH_FIELD_GRE_VER */
  gre_protocol_cmd_parse,            /* FLOW_MATCH_FIELD_GRE_PROTOCOL */
  gre_key_cmd_parse,                 /* FLOW_MATCH_FIELD_GRE_KEY */
  gre_seqnum_cmd_parse,              /* FLOW_MATCH_FIELD_GRE_SEQNUM */
  lisp_flags_cmd_parse,              /* FLOW_MATCH_FIELD_LISP_FLAGS */
  lisp_nonce_cmd_parse,              /* FLOW_MATCH_FIELD_LISP_NONCE */
  lisp_id_cmd_parse,                 /* FLOW_MATCH_FIELD_LISP_ID */
  vxlan_flags_cmd_parse,             /* FLOW_MATCH_FIELD_VXLAN_FLAGS */
  vxlan_vni_cmd_parse,               /* FLOW_MATCH_FIELD_VXLAN_VNI */
  mpls_data_first_nibble_cmd_parse,  /* FLOW_MATCH_FIELD_MPLS_DATA_FIRST_NIBBLE */
  mpls_ach_version_cmd_parse,        /* FLOW_MATCH_FIELD_MPLS_ACH_VERSION */
  mpls_ach_channel_cmd_parse,        /* FLOW_MATCH_FIELD_MPLS_ACH_CHANNEL */
  mpls_pw_metadata_cmd_parse,        /* FLOW_MATCH_FIELD_MPLS_PW_METADATA */
  mpls_cw_flags_cmd_parse,           /* FLOW_MATCH_FIELD_MPLS_CW_FLAGS */
  mpls_cw_frag_cmd_parse,            /* FLOW_MATCH_FIELD_MPLS_CW_FRAG */
  mpls_cw_len_cmd_parse,             /* FLOW_MATCH_FIELD_MPLS_CW_LEN */
  mpls_cw_seq_num_cmd_parse,         /* FLOW_MATCH_FIELD_MPLS_CW_SEQ_NUM */
};

static void*
flow_instruction_procs[FLOW_INSTRUCTION_MAX] = {
  goto_table_cmd_parse,              /* FLOW_INSTRUCTION_GOTO_TABLE */
  write_metadata_cmd_parse,          /* FLOW_INSTRUCTION_WRITE_METADATA */
  write_actions_cmd_parse,           /* FLOW_INSTRUCTION_WRITE_ACTIONS */
  apply_actions_cmd_parse,           /* FLOW_INSTRUCTION_APPLY_ACTIONS */
  clear_actions_cmd_parse,           /* FLOW_INSTRUCTION_CLEAR_ACTIONS */
  meter_cmd_parse,                   /* FLOW_INSTRUCTION_METER */
  NULL,                              /* FLOW_INSTRUCTION_EXPERIMENTER, nothing. */
};

static void*
flow_action_procs[FLOW_ACTION_MAX] = {
  output_cmd_parse,                  /* FLOW_ACTION_OUTPUT */
  copy_ttl_out_cmd_parse,            /* FLOW_ACTION_COPY_TTL_OUT */
  copy_ttl_in_cmd_parse,             /* FLOW_ACTION_COPY_TTL_IN */
  set_mpls_ttl_cmd_parse,            /* FLOW_ACTION_SET_MPLS_TTL */
  dec_mpls_ttl_cmd_parse,            /* FLOW_ACTION_DEC_MPLS_TTL */
  push_vlan_cmd_parse,               /* FLOW_ACTION_PUSH_VLAN */
  strip_vlan_cmd_parse,              /* FLOW_ACTION_STRIP_VLAN */
  push_mpls_cmd_parse,               /* FLOW_ACTION_PUSH_MPLS */
  pop_mpls_cmd_parse,                /* FLOW_ACTION_POP_MPLS */
  set_queue_cmd_parse,               /* FLOW_ACTION_SET_QUEUE */
  group_cmd_parse,                   /* FLOW_ACTION_GROUP */
  set_nw_ttl_cmd_parse,              /* FLOW_ACTION_SET_NW_TTL */
  dec_nw_ttl_cmd_parse,              /* FLOW_ACTION_DEC_NW_TTL */
  NULL,                              /* FLOW_ACTION_SET_FIELD, nothing. */
  push_pbb_cmd_parse,                /* FLOW_ACTION_PUSH_PBB */
  pop_pbb_cmd_parse,                 /* FLOW_ACTION_POP_PBB */
  NULL,                              /* FLOW_ACTION_EXPERIMENTER, nothing. */
};

static void*
flow_field_procs[FLOW_FIELD_MAX] = {
  cookie_cmd_parse,                  /* FLOW_FIELD_COOKIE */
  table_cmd_parse,                   /* FLOW_FIELD_TABLE */
  idle_timeout_cmd_parse,            /* FLOW_FIELD_IDLE_TIMEOUT */
  hard_timeout_cmd_parse,            /* FLOW_FIELD_HARD_TIMEOUT */
  priority_cmd_parse,                /* FLOW_FIELD_PRIORITY */
  out_port_cmd_parse,                /* FLOW_FIELD_OUT_PORT */
  out_group_cmd_parse,               /* FLOW_FIELD_OUT_GROUP */
  send_flow_rem_cmd_parse,           /* FLOW_FIELD_SEND_FLOW_REM */
  check_overlap_cmd_parse,           /* FLOW_FIELD_CHECK_OVERLAP */
};

static void*
flow_mod_opt_procs[FLOW_MOD_OPT_MAX] = {
  strict_cmd_parse,                  /* FLOW_MOD_OPT_STRICT */
};

static uint64_t
flow_port_to_num[FLOW_PORT_MAX] = {
  OFPP_IN_PORT,                      /* FLOW_PORT_IN_PORT */
  OFPP_TABLE,                        /* FLOW_PORT_TABLE */
  OFPP_NORMAL,                       /* FLOW_PORT_NORMAL*/
  OFPP_FLOOD,                        /* FLOW_PORT_FLOOD */
  OFPP_ALL,                          /* FLOW_PORT_ALL */
  OFPP_CONTROLLER,                   /* FLOW_PORT_CONTROLLER */
  OFPP_LOCAL,                        /* FLOW_PORT_LOCAL */
  OFPP_ANY,                          /* FLOW_PORT_ANY */
};

static uint64_t
flow_dl_type_to_num[FLOW_DL_TYPE_MAX] = {
  ETHERTYPE_IP,                      /* FLOW_DL_TYPE_IP */
  ETHERTYPE_ARP,                     /* FLOW_DL_TYPE_ARP */
  ETHERTYPE_VLAN,                    /* FLOW_DL_TYPE_VLAN */
  ETHERTYPE_IPV6,                    /* FLOW_DL_TYPE_IPV6 */
  ETHERTYPE_MPLS,                    /* FLOW_DL_TYPE_MPLS */
  ETHERTYPE_MPLS_MCAST,              /* FLOW_DL_TYPE_MPLS_MCAST */
  ETHERTYPE_PBB,                     /* FLOW_DL_TYPE_PBB */
};

static uint64_t
flow_group_to_num[FLOW_GROUP_MAX] = {
  OFPG_ALL,                          /* FLOW_GROUP_ALL */
  OFPG_ANY,                          /* FLOW_GROUP_ANY */
};

static uint64_t
flow_table_to_num[FLOW_TABLE_MAX] = {
  OFPTT_ALL,                        /* FLOW_TABLE_ALL */
};

static inline lagopus_result_t
flow_cmd_mod_initialize(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;

  /* create hashmap for flow mod cmds. */
  if ((ret = lagopus_hashmap_create(&flow_mod_cmd_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
     lagopus_perror(ret);
    goto done;
  }

  /* add flow mod cmds for match fields. */
  for (i = 0; i < FLOW_MATCH_FIELD_MAX; i++) {
    if ((ret = cmd_add(flow_match_field_strs[i],
                       flow_match_field_procs[i],
                       &flow_mod_cmd_table)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  }

  /* add flow mod cmds for instructions. */
  for (i = 0; i < FLOW_INSTRUCTION_MAX; i++) {
    if ((ret = cmd_add(flow_instruction_strs[i],
                       flow_instruction_procs[i],
                       &flow_mod_cmd_table)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  }

  /* add flow mod cmds for flow fields. */
  for (i = 0; i < FLOW_FIELD_MAX; i++) {
    if ((ret = cmd_add(flow_field_strs[i],
                       flow_field_procs[i],
                       &flow_mod_cmd_table)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  }

  /* add flow mod cmds for flow mod options. */
  for (i = 0; i < FLOW_MOD_OPT_MAX; i++) {
    if ((ret = cmd_add(flow_mod_opt_strs[i],
                       flow_mod_opt_procs[i],
                       &flow_mod_cmd_table)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  }

  /* create hashmap for flow mod actions cmds. */
  if ((ret = lagopus_hashmap_create(&flow_mod_action_cmd_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
     lagopus_perror(ret);
    goto done;
  }

  /* add flow mod action cmds for actions. */
  for (i = 0; i < FLOW_ACTION_MAX; i++) {
    if ((ret = cmd_add(flow_action_strs[i],
                       flow_action_procs[i],
                       &flow_mod_action_cmd_table)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  }

  /* create hashmap for flow mod actions cmds. */
  if ((ret = lagopus_hashmap_create(&flow_mod_action_set_field_cmd_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
     lagopus_perror(ret);
    goto done;
  }

  /* add flow action mod cmds for match fields. */
  for (i = 0; i < FLOW_MATCH_FIELD_MAX; i++) {
    switch (i) {
      case FLOW_MATCH_FIELD_IN_PORT:
      case FLOW_MATCH_FIELD_IN_PHY_PORT:
      case FLOW_MATCH_FIELD_METADATA:
        /* unsupported type in set_field. */
        break;
      default:
        if ((ret = cmd_add(flow_match_field_strs[i],
                           flow_match_field_procs[i],
                           &flow_mod_action_set_field_cmd_table)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
        break;
    }
  }

  /* create hashmap for port_str. */
  if ((ret = lagopus_hashmap_create(&flow_mod_port_str_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
     lagopus_perror(ret);
    goto done;
  }

  /* add port_str. */
  for (i = 0; i < FLOW_PORT_MAX; i++) {
    if ((ret = to_num_table_add(flow_port_strs[i],
                                &flow_port_to_num[i],
                                &flow_mod_port_str_table)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  }

  /* create hashmap for dl_type_str. */
  if ((ret = lagopus_hashmap_create(&flow_mod_dl_type_str_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
     lagopus_perror(ret);
    goto done;
  }

  /* add dl_tye_str. */
  for (i = 0; i < FLOW_DL_TYPE_MAX; i++) {
    if ((ret = to_num_table_add(flow_dl_type_strs[i],
                                &flow_dl_type_to_num[i],
                                &flow_mod_dl_type_str_table)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  }

  /* create hashmap for group_str. */
  if ((ret = lagopus_hashmap_create(&flow_mod_group_str_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
     lagopus_perror(ret);
    goto done;
  }

  /* add group_str. */
  for (i = 0; i < FLOW_GROUP_MAX; i++) {
    if ((ret = to_num_table_add(flow_group_strs[i],
                                &flow_group_to_num[i],
                                &flow_mod_group_str_table)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  }

  /* create hashmap for table_str. */
  if ((ret = lagopus_hashmap_create(&flow_mod_table_str_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
     lagopus_perror(ret);
    goto done;
  }

  /* add table_str. */
  for (i = 0; i < FLOW_TABLE_MAX; i++) {
    if ((ret = to_num_table_add(flow_table_strs[i],
                                &flow_table_to_num[i],
                                &flow_mod_table_str_table)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  }

done:
  return ret;
}

static inline void
flow_cmd_mod_finalize(void) {
  lagopus_hashmap_destroy(&flow_mod_cmd_table, true);
  flow_mod_cmd_table = NULL;
  lagopus_hashmap_destroy(&flow_mod_action_cmd_table, true);
  flow_mod_action_cmd_table = NULL;
  lagopus_hashmap_destroy(&flow_mod_action_set_field_cmd_table, true);
  flow_mod_action_set_field_cmd_table = NULL;
  lagopus_hashmap_destroy(&flow_mod_port_str_table, true);
  flow_mod_port_str_table = NULL;
  lagopus_hashmap_destroy(&flow_mod_dl_type_str_table, true);
  flow_mod_dl_type_str_table = NULL;
  lagopus_hashmap_destroy(&flow_mod_group_str_table, true);
  flow_mod_group_str_table = NULL;
  lagopus_hashmap_destroy(&flow_mod_table_str_table, true);
  flow_mod_table_str_table = NULL;
}
