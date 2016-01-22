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
#include "cmd_common.h"
#include "policer_cmd.h"
#include "policer_cmd_internal.h"
#include "policer_action_cmd.h"
#include "policer.c"
#include "conv_json.h"
#include "lagopus/dp_apis.h"

/* command name. */
#define CMD_NAME "policer"

/* option num. */
enum policer_opts {
  OPT_NAME = 0,
  OPT_ACTIONS,
  OPT_ACTION,
  OPT_BANDWIDTH_LIMIT,
  OPT_BURST_SIZE_LIMIT,
  OPT_BANDWIDTH_PERCENT,
  OPT_IS_USED,
  OPT_IS_ENABLED,

  OPT_MAX,
};

/* option name. */
static const char *const opt_strs[OPT_MAX] = {
  "*name",                  /* OPT_NAME (not option) */
  "*actions",               /* OPT_ACTIONS (not option) */
  "-action",                /* OPT_ACTION */
  "-bandwidth-limit",       /* OPT_BANDWIDTH_LIMIT */
  "-burst-size-limit",      /* OPT_BURST_SIZE_LIMIT */
  "-bandwidth-percent",     /* OPT_BANDWIDTH_PERCENT */
  "*is-used",               /* OPT_IS_USED (not option) */
  "*is-enabled",            /* OPT_IS_ENABLED (not option) */
};

/* config name. */
static const char *const config_strs[] = {
  "policer-down",           /* OFPPC_POLICER_DOWN */
  "unknown",
  "no-recv",                /* OFPPC_NO_RECV */
  "unknown",
  "unknown",
  "no-fwd",                 /* OFPPC_NO_FWD */
  "no-packet-in",           /* OFPPC_NO_PACKET_IN */
};

static const size_t config_strs_size =
  sizeof(config_strs) / sizeof(const char *);

typedef struct configs {
  size_t size;
  uint64_t flags;
  bool is_config;
  bool is_show_modified;
  policer_conf_t **list;
} configs_t;

struct names_info {
  datastore_name_info_t *not_change_names;
  datastore_name_info_t *add_names;
  datastore_name_info_t *del_names;
};

typedef lagopus_result_t
(*uint64_set_proc_t)(policer_attr_t *attr, const uint64_t num);
typedef lagopus_result_t
(*uint32_set_proc_t)(policer_attr_t *attr, const uint32_t num);
typedef lagopus_result_t
(*uint16_set_proc_t)(policer_attr_t *attr, const uint16_t num);
typedef lagopus_result_t
(*uint8_set_proc_t)(policer_attr_t *attr, const uint8_t num);

static lagopus_hashmap_t sub_cmd_table = NULL;
static lagopus_hashmap_t opt_table = NULL;

static inline lagopus_result_t
poli_names_disable(const char *name,
                   policer_attr_t *attr,
                   struct names_info *q_names_info,
                   datastore_interp_t *iptr,
                   datastore_interp_state_t state,
                   bool is_propagation,
                   bool is_unset_used,
                   lagopus_dstring_t *result);

/* get no change action names. */
static inline lagopus_result_t
poli_names_info_create(struct names_info *names_info,
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
poli_names_info_destroy(struct names_info *names_info) {
  datastore_names_destroy(names_info->not_change_names);
  datastore_names_destroy(names_info->add_names);
  datastore_names_destroy(names_info->del_names);
}

/* policer. */
static inline lagopus_result_t
poli_policer_create(const char *name,
                    policer_attr_t *attr,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t bandwidth_limit;
  uint64_t burst_size_limit;
  uint8_t bandwidth_percent;
  datastore_policer_info_t info;
  (void) info;

  if (((ret = policer_get_bandwidth_limit(attr,
                                          &bandwidth_limit)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = policer_get_burst_size_limit(attr,
              &burst_size_limit)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = policer_get_bandwidth_percent(attr,
              &bandwidth_percent)) ==
       LAGOPUS_RESULT_OK)) {
    info.bandwidth_limit = bandwidth_limit;
    info.burst_size_limit = burst_size_limit;
    info.bandwidth_percent = bandwidth_percent;

    lagopus_msg_info("create policer. name = %s.\n", name);

    ret = dp_policer_create(name, &info);
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't create policer.");
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't create policer.");
  }

  return ret;
}

static inline lagopus_result_t
poli_policer_destroy(const char *name,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_info("destroy policer. name = %s.\n", name);
  /*
   * ret = dp_policer_destroy(name);
   */
  ret = LAGOPUS_RESULT_OK;
  if (ret != LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't destroy policer.");
  }

  return ret;
}

/* actions */
static inline lagopus_result_t
poli_actions_add(const char *name,
                 policer_attr_t *attr,
                 struct names_info *names_info,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *a_name = NULL;
  datastore_name_info_t *a_names = NULL;
  struct datastore_name_head *head;

  if (names_info == NULL) {
    if ((ret = policer_get_action_names(attr, &a_names)) ==
        LAGOPUS_RESULT_OK) {
      head = &a_names->head;
    } else {
      ret = datastore_json_result_set(result, ret, NULL);
      goto done;
    }
  } else {
    head = &names_info->add_names->head;
  }

  if (TAILQ_EMPTY(head) == false) {
    TAILQ_FOREACH(a_name, head, name_entries) {
      lagopus_msg_info("add policer action(%s). "
                       "action name = %s.\n",
                       name, a_name->str);
      ret = dp_policer_action_add(name, a_name->str);
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result,
                                                ret,
                                                "Can't add policer.");
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

done:
  if (a_names != NULL) {
    datastore_names_destroy(a_names);
  }

  return ret;
}

static inline lagopus_result_t
poli_actions_delete(const char *name,
                    policer_attr_t *attr,
                    struct names_info *names_info,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *a_name = NULL;
  datastore_name_info_t *a_names = NULL;
  struct datastore_name_head *head;

  if (names_info == NULL) {
    if ((ret = policer_get_action_names(attr, &a_names)) ==
        LAGOPUS_RESULT_OK) {
      head = &a_names->head;
    } else {
      ret = datastore_json_result_set(result, ret, NULL);
      goto done;
    }
  } else {
    head = &names_info->del_names->head;
  }

  if (TAILQ_EMPTY(head) == false) {
    TAILQ_FOREACH(a_name, head, name_entries) {
      lagopus_msg_info("delete policer action(%s). "
                       "action name = %s.\n",
                       name, a_name->str);
      ret = dp_policer_action_delete(name, a_name->str);
      if (ret == LAGOPUS_RESULT_OK ||
          ret == LAGOPUS_RESULT_NOT_FOUND) {
        /* ignore LAGOPUS_RESULT_NOT_FOUND. */
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = datastore_json_result_string_setf(result,
                                                ret,
                                                "Can't delete policer.");
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

done:
  if (a_names != NULL) {
    datastore_names_destroy(a_names);
  }

  return ret;
}

static inline lagopus_result_t
poli_actions_disable(const char *name,
                     policer_attr_t *attr,
                     struct names_info *names_info,
                     datastore_interp_t *iptr,
                     datastore_interp_state_t state,
                     bool is_propagation,
                     bool is_unset_used,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *a_name = NULL;
  datastore_name_info_t *a_names = NULL;
  struct datastore_name_head *head;
  (void) name;

  if (names_info == NULL) {
    if ((ret = policer_get_action_names(attr, &a_names)) ==
        LAGOPUS_RESULT_OK) {
      head = &a_names->head;
    } else {
      ret = datastore_json_result_set(result, ret, NULL);
      goto done;
    }
  } else {
    head = &names_info->del_names->head;
  }

  if (TAILQ_EMPTY(head) == false) {
    TAILQ_FOREACH(a_name, head, name_entries) {
      if (is_propagation == true ||
          state == DATASTORE_INTERP_STATE_COMMITING ||
          state == DATASTORE_INTERP_STATE_ROLLBACKING) {
        /* disable propagation. */
        if ((ret = policer_action_cmd_disable_propagation(iptr, state,
                   a_name->str, result)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result,
                                                  ret,
                                                  "action name = %s.",
                                                  a_name->str);
          break;
        }
      }

      if (is_unset_used == true) {
        /* unset used. */
        ret = policer_set_used(a_name->str, false);
        /* ignore : LAGOPUS_RESULT_NOT_FOUND */
        if (ret == LAGOPUS_RESULT_OK || ret == LAGOPUS_RESULT_NOT_FOUND) {
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "action name = %s.",
                                                  a_name->str);
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
  if (a_names != NULL) {
    datastore_names_destroy(a_names);
  }

  return ret;
}

static inline lagopus_result_t
poli_action_used_set_internal(const char *name, bool b,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = policer_action_set_used(name, b);
  /* ignore : LAGOPUS_RESULT_NOT_FOUND */
  if (ret == LAGOPUS_RESULT_OK || ret == LAGOPUS_RESULT_NOT_FOUND) {
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "action name = %s.",
                                            name);
  }

  return ret;
}

static inline lagopus_result_t
poli_actions_used_set(policer_attr_t *attr, bool b,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_name_info_t *a_names = NULL;
  struct datastore_name_entry *name = NULL;

  if ((ret = policer_get_action_names(attr, &a_names)) ==
      LAGOPUS_RESULT_OK) {
    TAILQ_FOREACH(name, &a_names->head, name_entries) {
      ret = poli_action_used_set_internal(name->str, b, result);
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "action name = %s.",
                                                name->str);
        break;
      }
    }
  } else {
    ret = datastore_json_result_set(result, ret, NULL);
  }

  if (a_names != NULL) {
    datastore_names_destroy(a_names);
  }

  return ret;
}

/* get action names info. */
static inline lagopus_result_t
poli_action_names_info_get(policer_conf_t *conf,
                           struct names_info *names_info,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *name = NULL;
  datastore_name_info_t *current_names = NULL;
  datastore_name_info_t *modified_names = NULL;

  if (conf->modified_attr != NULL &&
      names_info != NULL) {
    if (conf->current_attr == NULL) {
      if (((ret = policer_get_action_names(conf->modified_attr,
                                           &modified_names)) ==
           LAGOPUS_RESULT_OK)) {
        TAILQ_FOREACH(name, &modified_names->head, name_entries) {
          if (policer_attr_action_name_exists(conf->current_attr,
                                              name->str) == false) {
            /* add_names. */
            if (datastore_name_exists(names_info->add_names,
                                      name->str) == false) {
              if ((ret = datastore_add_names(names_info->add_names,
                                             name->str)) !=
                  LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(
                        result, ret, "Can't add action names (add).");
                goto done;
              }
            }
          }
        }
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't get action names.");
      }
    } else {
      if (((ret = policer_get_action_names(conf->current_attr, &current_names)) ==
           LAGOPUS_RESULT_OK) &&
          ((ret = policer_get_action_names(conf->modified_attr, &modified_names)) ==
           LAGOPUS_RESULT_OK)) {
        TAILQ_FOREACH(name, &current_names->head, name_entries) {
          if (policer_attr_action_name_exists(conf->modified_attr,
                                              name->str) == true) {
            /* not_change_names. */
            if (datastore_name_exists(names_info->not_change_names,
                                      name->str) == false) {
              if ((ret = datastore_add_names(names_info->not_change_names,
                                             name->str)) !=
                  LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(
                        result, ret, "Can't add action names (not change).");
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
                        result, ret, "Can't add action names (del).");
                goto done;
              }
            }
          }
        }
        TAILQ_FOREACH(name, &modified_names->head, name_entries) {
          if (policer_attr_action_name_exists(conf->current_attr,
                                              name->str) == false) {
            /* add_names. */
            if (datastore_name_exists(names_info->add_names,
                                      name->str) == false) {
              if ((ret = datastore_add_names(names_info->add_names,
                                             name->str)) !=
                  LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(
                        result, ret, "Can't add action names (add).");
                goto done;
              }
            }
          }
        }
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't get action names.");
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

static inline lagopus_result_t
poli_action_update(datastore_interp_t *iptr,
                   datastore_interp_state_t state,
                   policer_conf_t *conf,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct names_info names_info;
  struct datastore_name_entry *name = NULL;

  if ((ret = poli_names_info_create(&names_info, result)) !=
      LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't update action.");
    goto done;
  }

  if ((ret = poli_action_names_info_get(conf, &names_info, result)) !=
      LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't update action.");
    goto done;
  }

  if (TAILQ_EMPTY(&names_info.not_change_names->head) == false) {
    TAILQ_FOREACH(name, &names_info.not_change_names->head, name_entries) {
      if (((ret = policer_action_cmd_update_propagation(iptr, state, name->str,
                  result)) !=
           LAGOPUS_RESULT_OK) &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't update action.");
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

  if (TAILQ_EMPTY(&names_info.add_names->head) == false) {
    TAILQ_FOREACH(name, &names_info.add_names->head, name_entries) {
      if (((ret = policer_action_cmd_update_propagation(iptr, state, name->str,
                  result)) !=
           LAGOPUS_RESULT_OK) &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't update action.");
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

  if (TAILQ_EMPTY(&names_info.del_names->head) == false) {
    TAILQ_FOREACH(name, &names_info.del_names->head, name_entries) {
      if (((ret = policer_action_cmd_update_propagation(iptr, state, name->str,
                  result)) !=
           LAGOPUS_RESULT_OK) &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't update action.");
        lagopus_perror(ret);
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

done:
  poli_names_info_destroy(&names_info);

  return ret;
}

/* objs */
static inline lagopus_result_t
poli_create(const char *name,
            policer_attr_t *attr,
            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = poli_policer_create(name, attr, result);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = poli_actions_add(name, attr, NULL, result);
  }

  return ret;
}

static inline lagopus_result_t
poli_destroy(const char *name,
             policer_attr_t *attr,
             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = poli_actions_delete(name, attr, NULL,
                            result);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = poli_policer_destroy(name, result);
  }

  return ret;
}

static inline lagopus_result_t
poli_start(const char *name,
           policer_attr_t *attr,
           struct names_info *a_names_info,
           datastore_interp_t *iptr,
           datastore_interp_state_t state,
           bool is_propagation,
           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *a_name = NULL;
  datastore_name_info_t *a_names = NULL;
  struct datastore_name_head *head;

  /* propagation actions. */
  if (TAILQ_EMPTY(&a_names_info->add_names->head) == true) {
    if ((ret = policer_get_action_names(attr, &a_names)) ==
        LAGOPUS_RESULT_OK) {
      head = &a_names->head;
    } else {
      ret = datastore_json_result_set(result, ret, NULL);
      goto done;
    }
  } else {
    head = &a_names_info->add_names->head;
  }

  if (TAILQ_EMPTY(head) == false) {
    TAILQ_FOREACH(a_name, head, name_entries) {
      if (is_propagation == true) {
        if ((ret = policer_action_cmd_enable_propagation(iptr, state,
                   a_name->str, result)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result,
                                                  ret,
                                                  "Can't start action.");
          goto done;
        }
      }
    }
  }

  lagopus_msg_info("start policer. name = %s.\n", name);
  ret = dp_policer_start(name);
  if (ret != LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't start policer.");
    goto done;
  }

done:
  if (a_names != NULL) {
    datastore_names_destroy(a_names);
  }

  return ret;
}

static inline lagopus_result_t
poli_stop(const char *name,
          policer_attr_t *attr,
          datastore_interp_t *iptr,
          datastore_interp_state_t state,
          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((ret = poli_names_disable(name,
                                attr,
                                NULL,
                                iptr, state,
                                true,
                                false,
                                result)) ==
      LAGOPUS_RESULT_OK) {
    lagopus_msg_info("stop policer. name = %s.\n", name);
    ret = dp_policer_stop(name);
    if (ret !=LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't stop policer.");
    }
  } else if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't stop policer.");
  }

  return ret;
}

/* name objs */
static inline lagopus_result_t
poli_names_add(const char *name,
               policer_attr_t *attr,
               struct names_info *q_names_info,
               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = poli_actions_add(name, attr, q_names_info, result);

  return ret;
}

static inline lagopus_result_t
poli_names_delete(const char *name,
                  policer_attr_t *attr,
                  struct names_info *q_names_info,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = poli_actions_delete(name, attr, q_names_info, result);

  return ret;
}

/* set is_used for action. */
static inline lagopus_result_t
poli_names_used_set(policer_attr_t *attr, bool b,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = poli_actions_used_set(attr, b, result);

  return ret;
}

static inline lagopus_result_t
poli_names_disable(const char *name,
                   policer_attr_t *attr,
                   struct names_info *a_names_info,
                   datastore_interp_t *iptr,
                   datastore_interp_state_t state,
                   bool is_propagation,
                   bool is_unset_used,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = poli_actions_disable(name, attr,
                             a_names_info, iptr,
                             state, is_propagation,
                             is_unset_used,
                             result);

  return ret;
}

/* get action names info. */
static inline lagopus_result_t
poli_names_info_get(policer_conf_t *conf,
                    struct names_info *q_names_info,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = poli_action_names_info_get(conf, q_names_info, result);

  return ret;
}

static inline void
policer_cmd_update_current_attr(policer_conf_t *conf,
                                datastore_interp_state_t state) {
  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    return;
  }

  if (conf->modified_attr != NULL) {
    if (conf->current_attr != NULL) {
      policer_attr_destroy(conf->current_attr);
    }
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = NULL;
  }
}

static inline void
policer_cmd_update_aborted(policer_conf_t *conf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (conf->modified_attr != NULL) {
    if (conf->current_attr == NULL) {
      /* create. */
      ret = policer_conf_delete(conf);
      if (ret != LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    } else {
      policer_attr_destroy(conf->modified_attr);
      conf->modified_attr = NULL;
    }
  }
}

static inline void
policer_cmd_update_switch_attr(policer_conf_t *conf) {
  policer_attr_t *attr;

  if (conf->modified_attr != NULL) {
    attr = conf->current_attr;
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = attr;
  }
}

static inline void
policer_cmd_is_enabled_set(policer_conf_t *conf) {
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
policer_cmd_do_destroy(policer_conf_t *conf,
                       datastore_interp_state_t state,
                       lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    ret = policer_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (state == DATASTORE_INTERP_STATE_DRYRUN) {
    /* unset is_used. */
    if (conf->current_attr != NULL) {
      if ((ret = poli_names_used_set(conf->current_attr,
                                     false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }
    if (conf->modified_attr != NULL) {
      if ((ret = poli_names_used_set(conf->modified_attr,
                                     false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }

    ret = policer_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (conf->is_destroying == true ||
             state == DATASTORE_INTERP_STATE_AUTO_COMMIT) {
    /* unset is_used. */
    if (conf->current_attr != NULL) {
      if ((ret = poli_names_used_set(conf->current_attr,
                                     false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }
    if (conf->modified_attr != NULL) {
      if ((ret = poli_names_used_set(conf->modified_attr,
                                     false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }

    if (conf->current_attr != NULL) {
      ret = poli_destroy(conf->name, conf->current_attr, result);
      if (ret != LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }

    ret = policer_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  }
}

static inline lagopus_result_t
poli_names_update(datastore_interp_t *iptr,
                  datastore_interp_state_t state,
                  policer_conf_t *conf,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = poli_action_update(iptr, state,
                           conf, result);

  return ret;
}

static inline lagopus_result_t
policer_cmd_do_update(datastore_interp_t *iptr,
                      datastore_interp_state_t state,
                      policer_conf_t *conf,
                      bool is_propagation,
                      bool is_enable_disable_cmd,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_modified = false;
  bool is_modified_without_names = false;
  struct names_info a_names_info;
  (void) iptr;

  /* create names info. */
  if ((ret = poli_names_info_create(&a_names_info, result)) !=
      LAGOPUS_RESULT_OK) {
    goto done;

  }

  if ((ret = poli_names_info_get(conf,
                                 &a_names_info,
                                 result)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  if (conf->modified_attr != NULL &&
      policer_attr_equals(conf->current_attr,
                          conf->modified_attr) == false) {
    if (conf->modified_attr != NULL) {
      is_modified = true;

      if (conf->current_attr == NULL &&
          conf->modified_attr != NULL) {
        is_modified_without_names = true;
      } else if (conf->current_attr != NULL &&
                 policer_attr_equals_without_names(
                   conf->current_attr,
                   conf->modified_attr) == false) {
        is_modified_without_names = true;
      }
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                      NULL);
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
    /* update actions. */
    if (is_propagation == true) {
      if ((ret = poli_names_update(iptr, state, conf, result)) !=
          LAGOPUS_RESULT_OK) {
        goto done;
      }
    }

    if (is_modified == true) {
      /*
       * Update attrs.
       */
      if (conf->current_attr != NULL) {
        /* disable actions. */
        ret = poli_names_disable(conf->name,
                                 conf->current_attr,
                                 &a_names_info,
                                 iptr, state,
                                 is_propagation,
                                 true,
                                 result);

        if (ret == LAGOPUS_RESULT_OK) {
          if (is_modified_without_names == true) {
            /* destroy policer/actions. */
            ret = poli_destroy(conf->name, conf->current_attr, result);
            if (ret != LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("Can't delete policer.\n");
            }
          } else {
            /* delete actions. */
            ret = poli_names_delete(conf->name,
                                    conf->current_attr,
                                    &a_names_info,
                                    result);
            if (ret != LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("Can't delete policer actions.\n");
            }
          }
        }
      } else {
        ret = LAGOPUS_RESULT_OK;
      }

      if (ret == LAGOPUS_RESULT_OK) {
        if (is_modified_without_names == true) {
          /* create policer/actions. */
          ret = poli_create(conf->name, conf->modified_attr,
                            result);
        } else {
          /* add actions. */
          ret = poli_names_add(conf->name,
                               conf->modified_attr,
                               &a_names_info,
                               result);
        }

        if (ret == LAGOPUS_RESULT_OK) {
          /* set is_used. */
          if ((ret = poli_names_used_set(conf->modified_attr,
                                         true, result)) ==
              LAGOPUS_RESULT_OK) {
            if (conf->is_enabled == true) {
              /* start policer. */
              ret = poli_start(conf->name, conf->modified_attr,
                               &a_names_info,
                               iptr, state,
                               is_propagation, result);
            }
          }
        }
      }

      /* Update attr. */
      if (ret == LAGOPUS_RESULT_OK &&
          state != DATASTORE_INTERP_STATE_COMMITING &&
          state != DATASTORE_INTERP_STATE_ROLLBACKING) {
        policer_cmd_update_current_attr(conf, state);
      }
    } else {
      if (is_enable_disable_cmd == true ||
          conf->is_enabling == true ||
          conf->is_disabling == true) {
        if (conf->is_enabled == true) {
          /* start policer. */
          ret = poli_start(conf->name, conf->current_attr,
                           &a_names_info,
                           iptr, state,
                           is_propagation, result);
        } else {
          /* stop policer. */
          ret = poli_stop(conf->name,
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
  poli_names_info_destroy(&a_names_info);

  return ret;
}

static inline lagopus_result_t
policer_cmd_update_internal(datastore_interp_t *iptr,
                            datastore_interp_state_t state,
                            policer_conf_t *conf,
                            bool is_propagation,
                            bool is_enable_disable_cmd,
                            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int i;

  switch (state) {
    case DATASTORE_INTERP_STATE_AUTO_COMMIT: {
      i = 0;
      while (i < UPDATE_CNT_MAX) {
        ret = policer_cmd_do_update(iptr, state, conf,
                                    is_propagation,
                                    is_enable_disable_cmd,
                                    result);
        if (ret == LAGOPUS_RESULT_OK ||
            is_enable_disable_cmd == true) {
          break;
        } else if (conf->current_attr == NULL &&
                   conf->modified_attr != NULL) {
          policer_cmd_do_destroy(conf, state, result);
          break;
        } else {
          policer_cmd_update_switch_attr(conf);
          lagopus_msg_warning("FAILED auto_comit (%s): rollbacking....\n",
                              lagopus_error_get_string(ret));
        }
        i++;
      }
      break;
    }
    case DATASTORE_INTERP_STATE_COMMITING: {
      policer_cmd_is_enabled_set(conf);
      ret = policer_cmd_do_update(iptr, state, conf,
                                  is_propagation,
                                  is_enable_disable_cmd,
                                  result);
      break;
    }
    case DATASTORE_INTERP_STATE_ATOMIC: {
      /* set is_used. */
      if (conf->modified_attr != NULL) {
        if ((ret = poli_names_used_set(conf->modified_attr,
                                       true, result)) !=
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
      policer_cmd_update_current_attr(conf, state);
      policer_cmd_do_destroy(conf, state, result);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_ROLLBACKING: {
      if (conf->current_attr == NULL &&
          conf->modified_attr != NULL) {
        /* case create. */
        /* unset is_used. */
        if ((ret = poli_names_used_set(conf->modified_attr,
                                       false, result)) !=
            LAGOPUS_RESULT_OK) {
          /* ignore error. */
          lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
        }
        ret = LAGOPUS_RESULT_OK;
      } else {
        policer_cmd_update_switch_attr(conf);
        policer_cmd_is_enabled_set(conf);
        ret = policer_cmd_do_update(iptr, state, conf,
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
        if ((ret = poli_names_used_set(conf->modified_attr,
                                       false, result)) !=
            LAGOPUS_RESULT_OK) {
          /* ignore error. */
          lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
        }
      }
      if (conf->current_attr != NULL) {
        /* unset is_used. */
        if ((ret = poli_names_used_set(conf->current_attr,
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
      policer_cmd_update_aborted(conf);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_DRYRUN: {
      if (conf->modified_attr != NULL) {
        if (conf->current_attr != NULL) {
          policer_attr_destroy(conf->current_attr);
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
policer_cmd_update(datastore_interp_t *iptr,
                   datastore_interp_state_t state,
                   void *o,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) iptr;
  (void) result;

  if (iptr != NULL && *iptr != NULL && o != NULL) {
    policer_conf_t *conf = (policer_conf_t *)o;
    ret = policer_cmd_update_internal(iptr, state, conf, false, false, result);
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static lagopus_result_t
uint_opt_parse(const char *const *argv[],
               policer_conf_t *conf,
               configs_t *configs,
               void *uint_set_proc,
               enum policer_opts opt,
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
action_opt_parse(const char *const *argv[],
                 void *c, void *out_configs,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  policer_conf_t *conf = NULL;
  configs_t *configs = NULL;
  char *name = NULL;
  char *name_str = NULL;
  char *fullname = NULL;
  bool is_added = false;
  bool is_exists = false;
  bool is_used = false;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (policer_conf_t *) c;
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
                policer_attr_action_name_exists(conf->modified_attr,
                                                fullname);
            } else {
              ret = datastore_json_result_string_setf(result,
                                                      ret,
                                                      "Can't get fullname %s.",
                                                      name);
              goto done;
            }
          } else {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't get action_name.");
            goto done;
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "action name = %s.",
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
                "action name = %s.", fullname);
            goto done;
          }

          if (policer_action_exists(fullname) == false) {
            ret = datastore_json_result_string_setf(
                result,
                LAGOPUS_RESULT_NOT_FOUND,
                "action name = %s.", fullname);
            goto done;
          }

          /* check is_used. */
          if ((ret =
               datastore_policer_action_is_used(fullname, &is_used)) ==
              LAGOPUS_RESULT_OK) {
            if (is_used == false) {
              ret = policer_attr_add_action_name(
                  conf->modified_attr,
                  fullname);
              if (ret != LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(
                    result, ret,
                    "action name = %s.", fullname);
              }
            } else {
              ret = datastore_json_result_string_setf(
                  result,
                  LAGOPUS_RESULT_NOT_OPERATIONAL,
                  "action name = %s.", fullname);
            }
          } else {
             ret = datastore_json_result_string_setf(
                result, ret,
                "action name = %s.", fullname);
          }
        } else {
          /* delete. */
          if (is_exists == false) {
            ret = datastore_json_result_string_setf(
                result, LAGOPUS_RESULT_NOT_FOUND,
                "action name = %s.", fullname);
            goto done;
          }

          ret = policer_attr_remove_action_name(conf->modified_attr,
                                                fullname);
          if (ret == LAGOPUS_RESULT_OK) {
            /* unset is_used. */
            ret = poli_action_used_set_internal(fullname,
                                                false,
                                                result);
          } else {
            ret = datastore_json_result_string_setf(
                result, ret,
                "action name = %s.", fullname);
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
      configs->flags = OPT_BIT_GET(OPT_ACTIONS);
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
bandwidth_limit_opt_parse(const char *const *argv[],
                          void *c, void *out_configs,
                          lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (policer_conf_t *) c,
                        (configs_t *) out_configs,
                        policer_set_bandwidth_limit,
                        OPT_BANDWIDTH_LIMIT, CMD_UINT64,
                        result);
}

static lagopus_result_t
burst_size_limit_opt_parse(const char *const *argv[],
                           void *c, void *out_configs,
                           lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (policer_conf_t *) c,
                        (configs_t *) out_configs,
                        policer_set_burst_size_limit,
                        OPT_BURST_SIZE_LIMIT, CMD_UINT64,
                        result);
}

static lagopus_result_t
bandwidth_percent_opt_parse(const char *const *argv[],
                            void *c, void *out_configs,
                            lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (policer_conf_t *) c,
                        (configs_t *) out_configs,
                        policer_set_bandwidth_percent,
                        OPT_BANDWIDTH_PERCENT, CMD_UINT8,
                        result);
}

static inline lagopus_result_t
opt_parse(const char *const argv[],
          policer_conf_t *conf,
          configs_t *out_configs,
          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  void *opt_proc;

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
    if (out_configs->is_config) {
      out_configs->flags = OPTS_MAX;
    }
    ret = LAGOPUS_RESULT_OK;
  }

done:
  return ret;
}

static lagopus_result_t
create_sub_cmd_parse_internal(datastore_interp_t *iptr,
                              datastore_interp_state_t state,
                              size_t argc, const char *const argv[],
                              char *name,
                              datastore_update_proc_t proc,
                              configs_t *out_configs,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  policer_conf_t *conf = NULL;
  (void) argc;
  (void) proc;

  ret = policer_conf_create(&conf, name);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = policer_conf_add(conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = opt_parse(argv, conf, out_configs, result);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = policer_cmd_update_internal(iptr, state, conf,
                                          true, false, result);

        if (ret != LAGOPUS_RESULT_OK &&
            ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_set(result, ret, NULL);
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add policer_conf.");
    }

    if (ret != LAGOPUS_RESULT_OK) {
      (void) policer_conf_delete(conf);
      goto done;
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't create policer_conf.");
  }

  if (ret != LAGOPUS_RESULT_OK && conf != NULL) {
    policer_conf_destroy(conf);
  }

done:
  return ret;
}

static lagopus_result_t
policer_attr_dup_modified(policer_conf_t *conf,
                          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (conf->modified_attr == NULL) {
    if (conf->current_attr != NULL) {
      /*
       * already exists. copy it.
       */
      ret = policer_attr_duplicate(conf->current_attr,
                                   &conf->modified_attr, NULL);
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_set(result, ret, NULL);
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_NOT_FOUND,
                                              "Not found attr. : name = %s",
                                              conf->name);
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

  return ret;
}

static lagopus_result_t
config_sub_cmd_parse_internal(datastore_interp_t *iptr,
                              datastore_interp_state_t state,
                              size_t argc, const char *const argv[],
                              policer_conf_t *conf,
                              datastore_update_proc_t proc,
                              configs_t *out_configs,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) proc;
  (void) argc;

  ret = policer_attr_dup_modified(conf, result);

  if (ret == LAGOPUS_RESULT_OK) {
    conf->is_destroying = false;
    out_configs->is_config = true;
    ret = opt_parse(argv, conf, out_configs, result);

    if (ret == LAGOPUS_RESULT_OK) {
      if (out_configs->flags == 0) {
        /* update. */
        ret = policer_cmd_update_internal(iptr, state, conf,
                                          true, false, result);

        if (ret != LAGOPUS_RESULT_OK &&
            ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_set(result, ret, NULL);
        }
      } else {
        /* show. */
        ret = policer_conf_one_list(&out_configs->list, conf);

        if (ret >= 0) {
          out_configs->size = (size_t) ret;
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = datastore_json_result_string_setf(
                  result, ret,
                  "Can't create list of policer_conf.");
        }
      }
    }
  }

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
  policer_conf_t *conf = NULL;
  char *namespace = NULL;
  (void) hptr;

  if (name != NULL) {
    ret = policer_find(name, &conf);

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
  policer_conf_t *conf = NULL;
  (void) hptr;
  (void) proc;
  (void) argc;

  if (name != NULL) {
    ret = policer_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = config_sub_cmd_parse_internal(iptr, state, argc, argv,
                                          conf, proc, out_configs,
                                          result);
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
                              policer_conf_t *conf,
                              bool is_propagation,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && conf != NULL && result != NULL) {
    if (conf->is_used == true) {
      if (conf->is_enabled == false) {
        if (state == DATASTORE_INTERP_STATE_ATOMIC) {
          conf->is_enabling = true;
          conf->is_disabling = false;
          ret = LAGOPUS_RESULT_OK;
        } else {
          conf->is_enabled = true;
          ret = policer_cmd_update_internal(iptr, state, conf,
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
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_NOT_OPERATIONAL,
                                              "name = %s. is not used.",
                                              conf->name);
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
  policer_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = policer_find(name, &conf);

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
                               policer_conf_t *conf,
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
      ret = policer_cmd_update_internal(iptr, state, conf,
                                        false, true, result);

      if (ret == LAGOPUS_RESULT_OK) {
        /* disable propagation. */
        if (is_propagation == true) {
          ret = poli_names_disable(conf->name, conf->current_attr,
                                   NULL, iptr, state,
                                   true, true, result);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_set(result, ret, NULL);
            goto done;
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

done:
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
  policer_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = policer_find(name, &conf);

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

static inline lagopus_result_t
destroy_sub_cmd_parse_internal(datastore_interp_t *iptr,
                               datastore_interp_state_t state,
                               policer_conf_t *conf,
                               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && conf != NULL && result != NULL) {
    if (conf->is_used == false) {
      if (state == DATASTORE_INTERP_STATE_ATOMIC) {
        conf->is_destroying = true;
        conf->is_enabling = false;
        conf->is_disabling = true;
      } else {
        /* disable interface/policer. */
        ret = poli_names_disable(conf->name, conf->current_attr,
                                 NULL, iptr, state,
                                 true, true, result);
        if (ret != LAGOPUS_RESULT_OK) {
          /* ignore error. */
          lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
        }

        if (conf->is_enabled == true) {
          conf->is_enabled = false;
          ret = policer_cmd_update_internal(iptr, state, conf,
                                            false, true, result);

          if (ret != LAGOPUS_RESULT_OK) {
            conf->is_enabled = true;
            if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
              ret = datastore_json_result_set(result, ret, NULL);
            }
            goto done;
          }
        }

        policer_cmd_do_destroy(conf, state, result);
      }
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_NOT_OPERATIONAL,
                                              "name = %s: is used.", conf->name);
    }
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
  policer_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = policer_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      ret = destroy_sub_cmd_parse_internal(iptr, state,
                                           conf, result);
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
  policer_conf_t *conf = NULL;
  char *target = NULL;

  if (name == NULL) {
    ret = namespace_get_current_namespace(&target);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = policer_conf_list(&out_configs->list, target);
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
      ret = policer_conf_list(&out_configs->list, "");
    } else if (ret == NS_NAMESPACE) {
      /* namespace + delim */
      ret = policer_conf_list(&out_configs->list, target);
    } else if (ret == NS_FULLNAME) {
      /* namespace + name or delim + name */
      ret = policer_find(target, &conf);

      if (ret == LAGOPUS_RESULT_OK) {
        if (conf->is_destroying == false) {
          ret = policer_conf_one_list(&out_configs->list, conf);
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
            "Can't create list of policer_conf.");
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

static lagopus_result_t
policer_cmd_json_create(lagopus_dstring_t *ds,
                        configs_t *configs,
                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_name_info_t *action_names = NULL;
  policer_attr_t *attr = NULL;
  uint64_t bandwidth_limit;
  uint64_t burst_size_limit;
  uint8_t bandwidth_percent;
  size_t i;

  ret = lagopus_dstring_appendf(ds, "[");
  if (ret == LAGOPUS_RESULT_OK) {
    for (i = 0; i < configs->size; i++) {
      if (configs->is_config == true) {
        /* config cmd. */
        if (configs->list[i]->modified_attr != NULL) {
          attr = configs->list[i]->modified_attr;
        } else {
          attr = configs->list[i]->current_attr;
        }
      } else {
        /* show cmd. */
        if (configs->is_show_modified == true) {
          if (configs->list[i]->modified_attr != NULL) {
            attr = configs->list[i]->modified_attr;
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

          /* action names */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_ACTIONS)) == true) {
            if ((ret = policer_get_action_names(attr,
                                                &action_names)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = names_show(ds,
                                    ATTR_NAME_GET(opt_strs,
                                                  OPT_ACTIONS),
                                    action_names, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* bandwidth_limit */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_BANDWIDTH_LIMIT)) == true) {
            if ((ret = policer_get_bandwidth_limit(attr, &bandwidth_limit)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_BANDWIDTH_LIMIT),
                           bandwidth_limit, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* burst_size_limit */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_BURST_SIZE_LIMIT)) == true) {
            if ((ret = policer_get_burst_size_limit(attr, &burst_size_limit)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_BURST_SIZE_LIMIT),
                           burst_size_limit, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* bandwidth_percent */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_BANDWIDTH_PERCENT)) == true) {
            if ((ret = policer_get_bandwidth_percent(attr, &bandwidth_percent)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint8_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_BANDWIDTH_PERCENT),
                           bandwidth_percent, true)) !=
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
      if (action_names != NULL) {
        datastore_names_destroy(action_names);
        action_names = NULL;
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

STATIC lagopus_result_t
policer_cmd_parse(datastore_interp_t *iptr,
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
  configs_t out_configs = {0, 0LL, false, false, NULL};
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
          ret = policer_cmd_json_create(&conf_result, &out_configs,
                                        result);

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
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
policer_cmd_enable(datastore_interp_t *iptr,
                   datastore_interp_state_t state,
                   void *obj,
                   bool *currentp,
                   bool enabled,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (currentp == NULL) {
    if (enabled == true) {
      ret = enable_sub_cmd_parse_internal(iptr, state,
                                          (policer_conf_t *) obj,
                                          false,
                                          result);
    } else {
      ret = disable_sub_cmd_parse_internal(iptr, state,
                                           (policer_conf_t *) obj,
                                           false,
                                           result);
    }
  } else {
    if (obj != NULL) {
      *currentp = ((policer_conf_t *) obj)->is_enabled;
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
policer_cmd_destroy(datastore_interp_t *iptr,
                    datastore_interp_state_t state,
                    void *obj,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = destroy_sub_cmd_parse_internal(iptr, state,
                                       (policer_conf_t *) obj,
                                       result);

  return ret;
}

STATIC lagopus_result_t
policer_cmd_serialize(datastore_interp_t *iptr,
                      datastore_interp_state_t state,
                      const void *obj,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_name_info_t *action_names = NULL;
  struct datastore_name_head *a_head = NULL;
  struct datastore_name_entry *a_entry = NULL;
  char *escaped_a_names_str = NULL;
  char *escaped_name = NULL;
  policer_conf_t *conf = NULL;
  bool is_escaped = false;
  uint64_t bandwidth_limit;
  uint64_t burst_size_limit;
  uint8_t bandwidth_percent;
  (void) state;

  if (iptr != NULL && obj != NULL && result != NULL) {
    conf = (policer_conf_t *) obj;

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

    /* action names opt. */
    if ((ret = policer_get_action_names(conf->current_attr,
                                        &action_names)) ==
        LAGOPUS_RESULT_OK) {
      a_head = &action_names->head;
      if (TAILQ_EMPTY(a_head) == false) {
        TAILQ_FOREACH(a_entry, a_head, name_entries) {
          if ((ret = lagopus_dstring_appendf(result, " %s",
                                             opt_strs[OPT_ACTION])) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_str_escape(a_entry->str, "\"",
                                          &is_escaped,
                                          &escaped_a_names_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(
                           result,
                           ESCAPE_NAME_FMT(is_escaped, escaped_a_names_str),
                           escaped_a_names_str)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }

            free((void *) escaped_a_names_str);
            escaped_a_names_str = NULL;
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

    /* bandwidth-limit opt. */
    if ((ret = policer_get_bandwidth_limit(conf->current_attr,
                                           &bandwidth_limit)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_BANDWIDTH_LIMIT])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %"PRIu64,
                                           bandwidth_limit)) !=
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

    /* burst-size-limit opt. */
    if ((ret = policer_get_burst_size_limit(conf->current_attr,
                                            &burst_size_limit)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_BURST_SIZE_LIMIT])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %"PRIu64,
                                           burst_size_limit)) !=
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

    /* bandwidth-percent opt. */
    if ((ret = policer_get_bandwidth_percent(conf->current_attr,
               &bandwidth_percent)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_BANDWIDTH_PERCENT])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %d",
                                           bandwidth_percent)) !=
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

    /* Add newline. */
    if ((ret = lagopus_dstring_appendf(result, "\n")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

  done:
    datastore_names_destroy(action_names);
    free((void *) escaped_a_names_str);
    free(escaped_name);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
policer_cmd_compare(const void *obj1, const void *obj2, int *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj1 != NULL && obj2 != NULL && result != NULL) {
    *result = strcmp(((policer_conf_t *) obj1)->name,
                     ((policer_conf_t *) obj2)->name);
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
policer_cmd_getname(const void *obj, const char **namep) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj != NULL && namep != NULL) {
    *namep = ((policer_conf_t *)obj)->name;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
policer_cmd_duplicate(const void *obj, const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  policer_conf_t *dup_obj = NULL;

  if (obj != NULL) {
    ret = policer_conf_duplicate(obj, &dup_obj, namespace);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = policer_conf_add(dup_obj);

      if (ret != LAGOPUS_RESULT_OK && dup_obj != NULL) {
        policer_conf_destroy(dup_obj);
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

  if ((ret = policer_initialize()) != LAGOPUS_RESULT_OK) {
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

  if (((ret = opt_add(opt_strs[OPT_ACTION], action_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_BANDWIDTH_LIMIT], bandwidth_limit_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_BURST_SIZE_LIMIT], burst_size_limit_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_BANDWIDTH_PERCENT], bandwidth_percent_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  if ((ret = datastore_register_table(CMD_NAME,
                                      &policer_table,
                                      policer_cmd_update,
                                      policer_cmd_enable,
                                      policer_cmd_serialize,
                                      policer_cmd_destroy,
                                      policer_cmd_compare,
                                      policer_cmd_getname,
                                      policer_cmd_duplicate)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = datastore_interp_register_command(&s_interp, CONFIGURATOR_NAME,
             CMD_NAME, policer_cmd_parse)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

done:
  return ret;
}

/*
 * Public:
 */

lagopus_result_t
policer_cmd_enable_propagation(datastore_interp_t *iptr,
                               datastore_interp_state_t state,
                               char *name,
                               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = enable_sub_cmd_parse(iptr, state, 0, NULL,
                               name, NULL, policer_cmd_update,
                               NULL, result);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

lagopus_result_t
policer_cmd_disable_propagation(datastore_interp_t *iptr,
                                datastore_interp_state_t state,
                                char *name,
                                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  policer_conf_t *conf = NULL;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = policer_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = disable_sub_cmd_parse_internal(iptr, state,
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

lagopus_result_t
policer_cmd_update_propagation(datastore_interp_t *iptr,
                               datastore_interp_state_t state,
                               char *name,
                               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  policer_conf_t *conf = NULL;

  if (name != NULL) {
    ret = policer_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = policer_cmd_update(iptr, state,
                               (void *) conf, result);
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

lagopus_result_t
policer_cmd_initialize(void) {
  return initialize_internal();
}

void
policer_cmd_finalize(void) {
  lagopus_hashmap_destroy(&sub_cmd_table, true);
  sub_cmd_table = NULL;
  lagopus_hashmap_destroy(&opt_table, true);
  opt_table = NULL;
  policer_finalize();
}
