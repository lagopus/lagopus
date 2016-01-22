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
#include "lagopus/dp_apis.h"
#include "policer_action_cmd.h"
#include "policer_action_cmd_internal.h"
#include "policer_action.c"
#include "conv_json.h"
#include "lagopus/port.h"
#include "lagopus/dp_apis.h"

/* command name. */
#define CMD_NAME "policer-action"

/* option num. */
enum policer_action_opts {
  OPT_NAME = 0,
  OPT_TYPE,
  OPT_IS_USED,
  OPT_IS_ENABLED,

  OPT_MAX,
};

/* option flags. */
#define OPT_COMMON   (OPT_BIT_GET(OPT_NAME) |                 \
                      OPT_BIT_GET(OPT_TYPE) |                 \
                      OPT_BIT_GET(OPT_IS_USED) |              \
                      OPT_BIT_GET(OPT_IS_ENABLED))
#define OPT_DISCARD (OPT_COMMON)
#define OPT_UNKNOWN  (OPT_COMMON)

/* option name. */
static const char *const opt_strs[OPT_MAX] = {
  "*name",               /* OPT_NAME (not option) */
  "-type",               /* OPT_TYPE */
  "*is-used",            /* OPT_IS_USED (not option) */
  "*is-enabled",         /* OPT_IS_ENABLED (not option) */
};

typedef struct configs {
  size_t size;
  uint64_t flags;
  bool is_config;
  bool is_show_modified;
  policer_action_conf_t **list;
} configs_t;

typedef lagopus_result_t
(*ip_addr_set_proc_t)(policer_action_attr_t *attr,
                      const char *addr, const bool prefer);
typedef lagopus_result_t
(*hw_addr_set_proc_t)(policer_action_attr_t *attr,
                      const mac_address_t hw_addr);
typedef lagopus_result_t
(*uint64_set_proc_t)(policer_action_attr_t *attr, const uint64_t num);
typedef lagopus_result_t
(*uint32_set_proc_t)(policer_action_attr_t *attr, const uint32_t num);
typedef lagopus_result_t
(*uint16_set_proc_t)(policer_action_attr_t *attr, const uint16_t num);
typedef lagopus_result_t
(*uint8_set_proc_t)(policer_action_attr_t *attr, const uint8_t num);

static lagopus_hashmap_t sub_cmd_table = NULL;
static lagopus_hashmap_t discard_opt_table = NULL;

static inline lagopus_result_t
policer_action_create(const char *name,
                      policer_action_attr_t *attr,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_policer_action_type_t type;
  datastore_policer_action_info_t info;
  (void) info;

  if (((ret = policer_action_get_type(attr, &type)) ==
       LAGOPUS_RESULT_OK)) {
    info.type = type;
    switch (type) {
      case DATASTORE_POLICER_ACTION_TYPE_DISCARD:
        /* noting */
        break;
      default:
        ret = datastore_json_result_string_setf(
                result,
                LAGOPUS_RESULT_INVALID_ARGS,
                "Bad required options(-type).");
        goto done;
        break;
    }

    lagopus_msg_info("create policer_action. name = %s.\n", name);
    ret = dp_policer_action_create(name, &info);
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't create policer_action.");
    }
  }

done:
  return ret;
}

static inline void
policer_action_destroy(const char *name) {
  lagopus_msg_info("destroy policer_action. name = %s.\n", name);
  dp_policer_action_destroy(name);
}

static inline lagopus_result_t
policer_action_start(const char *name,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_info("start policer_action. name = %s.\n", name);
  ret = dp_policer_action_start(name);
  if (ret != LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't start policer_action.");
  }

  return ret;
}

static inline lagopus_result_t
policer_action_stop(const char *name,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_info("stop policer_action. name = %s.\n", name);
  ret = dp_policer_action_stop(name);
  if (ret != LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't stop policer_action.");
  }

  return ret;
}

static inline void
policer_action_cmd_update_current_attr(policer_action_conf_t *conf,
                                       datastore_interp_state_t state) {
  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    return;
  }

  if (conf->modified_attr != NULL) {
    if (conf->current_attr != NULL) {
      policer_action_attr_destroy(conf->current_attr);
    }
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = NULL;
  }
}

static inline void
policer_action_cmd_update_aborted(policer_action_conf_t *conf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (conf->modified_attr != NULL) {
    if (conf->current_attr == NULL) {
      /* create. */
      ret = policer_action_conf_delete(conf);
      if (ret != LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    } else {
      policer_action_attr_destroy(conf->modified_attr);
      conf->modified_attr = NULL;
    }
  }
}

static inline void
policer_action_cmd_update_switch_attr(policer_action_conf_t *conf) {
  policer_action_attr_t *attr;

  if (conf->modified_attr != NULL) {
    attr = conf->current_attr;
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = attr;
  }
}

static inline void
policer_action_cmd_is_enabled_set(policer_action_conf_t *conf) {
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
policer_action_cmd_do_destroy(policer_action_conf_t *conf,
                              datastore_interp_state_t state) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    ret = policer_action_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (state == DATASTORE_INTERP_STATE_DRYRUN) {
    ret = policer_action_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (conf->is_destroying == true ||
             state == DATASTORE_INTERP_STATE_AUTO_COMMIT) {
    lagopus_msg_info("destroy policer_action. name = %s.\n", conf->name);
    dp_policer_action_destroy(conf->name);

    ret = policer_action_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  }
}

static inline lagopus_result_t
policer_action_cmd_do_update(datastore_interp_t *iptr,
                             datastore_interp_state_t state,
                             policer_action_conf_t *conf,
                             bool is_propagation,
                             bool is_enable_disable_cmd,
                             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_modified = false;
  (void) iptr;
  (void) is_propagation;

  if (conf->modified_attr != NULL &&
      policer_action_attr_equals(conf->current_attr,
                                 conf->modified_attr) == false) {
    if (conf->modified_attr != NULL) {
      is_modified = true;
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
    if (is_modified == true) {
      /*
       * Update attrs.
       */
      if (conf->current_attr != NULL) {
        /* destroy policer_action. */
        policer_action_destroy(conf->name);
      }

      /* create policer_action. */
      ret = policer_action_create(conf->name,
                                  conf->modified_attr,
                                  result);
      if (ret == LAGOPUS_RESULT_OK) {
        if (conf->is_enabled == true) {
          /* start policer_action. */
          ret = policer_action_start(conf->name, result);
        }
      }

      /* Update attr. */
      if (ret == LAGOPUS_RESULT_OK &&
          state != DATASTORE_INTERP_STATE_COMMITING &&
          state != DATASTORE_INTERP_STATE_ROLLBACKING) {
        policer_action_cmd_update_current_attr(conf, state);
      }
    } else {
      if (is_enable_disable_cmd == true ||
          conf->is_enabling == true ||
          conf->is_disabling == true) {
        if (conf->is_enabled == true) {
          /* start policer_action. */
          ret = policer_action_start(conf->name, result);
        } else {
          /* stop policer_action. */
          ret = policer_action_stop(conf->name, result);
        }
      }
      conf->is_enabling = false;
      conf->is_disabling = false;
    }
  }

  return ret;
}

static inline lagopus_result_t
policer_action_cmd_update_internal(datastore_interp_t *iptr,
                                   datastore_interp_state_t state,
                                   policer_action_conf_t *conf,
                                   bool is_propagation,
                                   bool is_enable_disable_cmd,
                                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int i;

  switch (state) {
    case DATASTORE_INTERP_STATE_AUTO_COMMIT: {
      i = 0;
      while (i < UPDATE_CNT_MAX) {
        ret = policer_action_cmd_do_update(iptr, state, conf,
                                           is_propagation,
                                           is_enable_disable_cmd,
                                           result);
        if (ret == LAGOPUS_RESULT_OK ||
            is_enable_disable_cmd == true) {
          break;
        } else if (conf->current_attr == NULL &&
                   conf->modified_attr != NULL) {
          policer_action_cmd_do_destroy(conf, state);
          break;
        } else {
          policer_action_cmd_update_switch_attr(conf);
          lagopus_msg_warning("FAILED auto_comit (%s): rollbacking....\n",
                              lagopus_error_get_string(ret));
        }
        i++;
      }
      break;
    }
    case DATASTORE_INTERP_STATE_COMMITING: {
      policer_action_cmd_is_enabled_set(conf);
      ret = policer_action_cmd_do_update(iptr, state, conf,
                                         is_propagation,
                                         is_enable_disable_cmd,
                                         result);
      break;
    }
    case DATASTORE_INTERP_STATE_ATOMIC: {
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_COMMITED:
    case DATASTORE_INTERP_STATE_ROLLBACKED: {
      policer_action_cmd_update_current_attr(conf, state);
      policer_action_cmd_do_destroy(conf, state);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_ROLLBACKING: {
      if (conf->current_attr == NULL &&
          conf->modified_attr != NULL) {
        /* case create. */
        ret = LAGOPUS_RESULT_OK;
      } else {
        policer_action_cmd_update_switch_attr(conf);
        policer_action_cmd_is_enabled_set(conf);
        ret = policer_action_cmd_do_update(iptr, state, conf,
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
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_ABORTED: {
      policer_action_cmd_update_aborted(conf);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_DRYRUN: {
      if (conf->modified_attr != NULL) {
        if (conf->current_attr != NULL) {
          policer_action_attr_destroy(conf->current_attr);
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
policer_action_cmd_update(datastore_interp_t *iptr,
                          datastore_interp_state_t state,
                          void *o,
                          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) iptr;
  (void) result;

  if (iptr != NULL && *iptr != NULL && o != NULL) {
    policer_action_conf_t *conf = (policer_action_conf_t *)o;
    ret = policer_action_cmd_update_internal(iptr, state, conf, false, false,
          result);
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static inline lagopus_result_t
opt_parse(const char *const argv[],
          policer_action_conf_t *conf,
          configs_t *out_configs,
          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_policer_action_type_t type;
  bool required = false;
  lagopus_hashmap_t table = NULL;
  void *opt_proc = NULL;

  argv++;
  if (*argv != NULL) {
    while (*argv != NULL) {
      if (IS_VALID_STRING(*argv) == true) {
        /* type. */
        if (OPT_CMP(*argv, opt_strs, OPT_TYPE) == 0) {
          if (*(argv + 1) != NULL) {
            if (IS_VALID_STRING(*(argv + 1)) == true) {
              ret = policer_action_type_to_enum(*(++argv), &type);
              if (ret == LAGOPUS_RESULT_OK) {
                ret = policer_action_set_type(conf->modified_attr,
                                              type);
                if (ret == LAGOPUS_RESULT_OK) {
                  required = true;
                  goto loop_end;
                } else {
                  ret = datastore_json_result_string_setf(result, ret,
                                                          "Can't add type.");
                  goto done;
                }
              } else {
                ret = datastore_json_result_string_setf(result, ret,
                                                        "Bad opt value = %s.",
                                                        *argv);
                goto done;
              }
            } else {
              ret = datastore_json_result_set(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              NULL);
              goto done;
            }
          } else if (out_configs->is_config == true) {
            out_configs->flags = OPT_BIT_GET(OPT_TYPE);
            ret = LAGOPUS_RESULT_OK;
            goto loop_end;
          } else {
            ret = datastore_json_result_set(result,
                                            LAGOPUS_RESULT_INVALID_ARGS,
                                            NULL);
            goto done;
          }
        } else if (out_configs->is_config == false && required != true) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad required options(-type),"
                                                  " opt = %s.",
                                                  *argv);
          goto done;
        }

        if (required == true || out_configs->is_config == true) {
          ret = policer_action_get_type(conf->modified_attr,
                                        &type);
          if (ret == LAGOPUS_RESULT_OK) {
            switch (type) {
              case DATASTORE_POLICER_ACTION_TYPE_DISCARD:
                table = discard_opt_table;
                break;
              default:
                ret = datastore_json_result_string_setf(result, ret,
                                                        "Bad opt value = %s.",
                                                        *argv);
                goto done;
            }
          } else {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't get type.");
            goto done;
          }


          if (lagopus_hashmap_find(&table,
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
                                                    "Bad opt = %s.", *argv);
            goto done;
          }
        } else {
          ret = datastore_json_result_set(result,
                                          LAGOPUS_RESULT_INVALID_ARGS,
                                          NULL);
          goto done;
        }
      loop_end:
        argv++;
      } else {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_INVALID_ARGS,
                                                "Bad opt.");
        goto done;
      }
    }
  } else {
    /* config: all items show. */
    if (out_configs->is_config) {
      out_configs->flags = OPTS_MAX;
    }
    ret = LAGOPUS_RESULT_OK;
  }

  /* required opts. */
  if (conf->modified_attr != NULL) {
    /* check type */
    if ((ret = policer_action_get_type(conf->modified_attr,
                                       &type)) ==
        LAGOPUS_RESULT_OK) {
      if (type == DATASTORE_POLICER_ACTION_TYPE_UNKNOWN) {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_INVALID_ARGS,
                                                "Bad required options(-type).");
        goto done;
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't get type.");
      goto done;
    }
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
  policer_action_conf_t *conf = NULL;
  (void) argc;
  (void) proc;

  ret = policer_action_conf_create(&conf, name);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = policer_action_conf_add(conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = opt_parse(argv, conf, out_configs, result);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = policer_action_cmd_update_internal(iptr, state, conf,
              true, false, result);

        if (ret != LAGOPUS_RESULT_OK &&
            ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_set(result, ret, NULL);
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add policer_action_conf.");
    }

    if (ret != LAGOPUS_RESULT_OK) {
      (void) policer_action_conf_delete(conf);
      goto done;
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't create policer_action_conf.");
  }

  if (ret != LAGOPUS_RESULT_OK && conf != NULL) {
    policer_action_conf_destroy(conf);
  }

done:
  return ret;
}

static lagopus_result_t
config_sub_cmd_parse_internal(datastore_interp_t *iptr,
                              datastore_interp_state_t state,
                              size_t argc, const char *const argv[],
                              policer_action_conf_t *conf,
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
      ret = policer_action_attr_duplicate(conf->current_attr,
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
      ret = policer_action_cmd_update_internal(iptr, state, conf,
            true, false, result);

      if (ret != LAGOPUS_RESULT_OK &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_set(result, ret, NULL);
      }
    } else {
      /* show. */
      ret = policer_action_conf_one_list(&out_configs->list, conf);

      if (ret >= 0) {
        out_configs->size = (size_t) ret;
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = datastore_json_result_string_setf(
                result, ret,
                "Can't create list of policer_action_conf.");
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
  policer_action_conf_t *conf = NULL;
  char *namespace = NULL;
  (void) hptr;
  (void) proc;

  if (name != NULL) {
    ret = policer_action_find(name, &conf);

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
  policer_action_conf_t *conf = NULL;
  (void) hptr;
  (void) proc;

  if (name != NULL) {
    ret = policer_action_find(name, &conf);

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
                              policer_action_conf_t *conf,
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
          ret = policer_action_cmd_update_internal(iptr, state, conf,
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
  policer_action_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = policer_action_find(name, &conf);

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
                               policer_action_conf_t *conf,
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
      ret = policer_action_cmd_update_internal(iptr, state, conf,
            is_propagation, true, result);

      if (ret != LAGOPUS_RESULT_OK) {
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
  policer_action_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = policer_action_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      ret = disable_sub_cmd_parse_internal(iptr, state,
                                           conf, true,
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
                               policer_action_conf_t *conf,
                               bool is_propagation,
                               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && conf != NULL && result != NULL) {
    if (conf->is_used == false) {
      if (state == DATASTORE_INTERP_STATE_ATOMIC) {
        conf->is_destroying = true;
        conf->is_enabling = false;
        conf->is_disabling = true;
      } else {
        if (conf->is_enabled == true) {
          conf->is_enabled = false;
          ret = policer_action_cmd_update_internal(iptr, state, conf,
                is_propagation, true,
                result);

          if (ret != LAGOPUS_RESULT_OK) {
            conf->is_enabled = true;
            if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
              ret = datastore_json_result_set(result, ret, NULL);
            }
            goto done;
          }
        }

        policer_action_cmd_do_destroy(conf, state);
      }
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_NOT_OPERATIONAL,
                                              "name = %s: is used.",
                                              conf->name);
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
  policer_action_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = policer_action_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      if (conf->is_used == false) {
        ret = destroy_sub_cmd_parse_internal(iptr, state,
                                             conf, true,
                                             result);
      } else {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_NOT_OPERATIONAL,
                                                "name = %s: is used.", name);
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

  return ret;
}

static inline lagopus_result_t
show_parse(const char *name,
           configs_t *out_configs,
           bool is_show_modified,
           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  policer_action_conf_t *conf = NULL;
  char *target = NULL;

  if (name == NULL) {
    ret = namespace_get_current_namespace(&target);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = policer_action_conf_list(&out_configs->list, target);
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
      ret = policer_action_conf_list(&out_configs->list, "");
    } else if (ret == NS_NAMESPACE) {
      /* namespace + delim */
      ret = policer_action_conf_list(&out_configs->list, target);
    } else if (ret == NS_FULLNAME) {
      /* namespace + name or delim + name */
      ret = policer_action_find(target, &conf);

      if (ret == LAGOPUS_RESULT_OK) {
        if (conf->is_destroying == false) {
          ret = policer_action_conf_one_list(&out_configs->list, conf);
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
            "Can't create list of policer_action_conf.");
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

static lagopus_result_t
policer_action_cmd_enable(datastore_interp_t *iptr,
                          datastore_interp_state_t state,
                          void *obj,
                          bool *currentp,
                          bool enabled,
                          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (currentp == NULL) {
    if (enabled == true) {
      ret = enable_sub_cmd_parse_internal(iptr, state,
                                          (policer_action_conf_t *) obj,
                                          false,
                                          result);
    } else {
      ret = disable_sub_cmd_parse_internal(iptr, state,
                                           (policer_action_conf_t *) obj,
                                           false,
                                           result);
    }
  } else {
    if (obj != NULL) {
      *currentp = ((policer_action_conf_t *) obj)->is_enabled;
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
policer_action_cmd_destroy(datastore_interp_t *iptr,
                           datastore_interp_state_t state,
                           void *obj,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = destroy_sub_cmd_parse_internal(iptr, state,
                                       (policer_action_conf_t *) obj,
                                       false,
                                       result);
  return ret;
}

STATIC lagopus_result_t
policer_action_cmd_serialize(datastore_interp_t *iptr,
                             datastore_interp_state_t state,
                             const void *obj,
                             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  policer_action_conf_t *conf = NULL;

  char *escaped_name = NULL;

  /* type */
  datastore_policer_action_type_t type = DATASTORE_POLICER_ACTION_TYPE_UNKNOWN;
  const char *type_str = NULL;
  char *escaped_type_str = NULL;

  bool is_escaped = false;
  uint64_t flags = OPTS_MAX;

  (void) state;

  if (iptr != NULL && obj != NULL && result != NULL) {
    conf = (policer_action_conf_t *) obj;

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

    /* type opt. */
    if ((ret = policer_action_get_type(conf->current_attr,
                                       &type)) ==
        LAGOPUS_RESULT_OK) {
      if (type != DATASTORE_POLICER_ACTION_TYPE_UNKNOWN) {
        if ((ret = policer_action_type_to_str(type,
                                              &type_str)) == LAGOPUS_RESULT_OK) {
          if (IS_VALID_STRING(type_str) == true) {
            if ((ret = lagopus_str_escape(type_str, "\"",
                                          &is_escaped,
                                          &escaped_type_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, " %s",
                                                 opt_strs[OPT_TYPE])) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = lagopus_dstring_appendf(
                             result,
                             ESCAPE_NAME_FMT(is_escaped, escaped_type_str),
                             escaped_type_str)) !=
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

    switch (type) {
      case DATASTORE_POLICER_ACTION_TYPE_DISCARD:
        flags &= OPT_DISCARD;
        break;
      default:
        ret = LAGOPUS_RESULT_OUT_OF_RANGE;
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
    free((void *) escaped_name);
    free((void *) escaped_type_str);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
policer_action_cmd_compare(const void *obj1, const void *obj2, int *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj1 != NULL && obj2 != NULL && result != NULL) {
    *result = strcmp(((policer_action_conf_t *) obj1)->name,
                     ((policer_action_conf_t *) obj2)->name);
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
policer_action_cmd_getname(const void *obj, const char **namep) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj != NULL && namep != NULL) {
    *namep = ((policer_action_conf_t *)obj)->name;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
policer_action_cmd_duplicate(const void *obj, const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  policer_action_conf_t *dup_obj = NULL;

  if (obj != NULL) {
    ret = policer_action_conf_duplicate(obj, &dup_obj, namespace);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = policer_action_conf_add(dup_obj);

      if (ret != LAGOPUS_RESULT_OK && dup_obj != NULL) {
        policer_action_conf_destroy(dup_obj);
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

  if ((ret = policer_action_initialize()) != LAGOPUS_RESULT_OK) {
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

  /* create hashmap for ethernet opt. */
  if ((ret = lagopus_hashmap_create(&discard_opt_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* add opts for discard opt. */
  /* noting */

  if ((ret = datastore_register_table(CMD_NAME,
                                      &policer_action_table,
                                      policer_action_cmd_update,
                                      policer_action_cmd_enable,
                                      policer_action_cmd_serialize,
                                      policer_action_cmd_destroy,
                                      policer_action_cmd_compare,
                                      policer_action_cmd_getname,
                                      policer_action_cmd_duplicate)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = datastore_interp_register_command(&s_interp, CONFIGURATOR_NAME,
             CMD_NAME,
             policer_action_cmd_parse)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

done:
  return ret;
}

static lagopus_result_t
policer_action_cmd_json_create(lagopus_dstring_t *ds,
                               configs_t *configs,
                               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *type_str = NULL;
  policer_action_attr_t *attr = NULL;
  datastore_policer_action_type_t type;
  uint64_t flags;
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

          /* get type. */
          if ((ret = policer_action_get_type(attr, &type)) ==
              LAGOPUS_RESULT_OK) {
            /* set show flags. */
            flags = configs->flags;
            switch (type) {
              case DATASTORE_POLICER_ACTION_TYPE_DISCARD:
                flags &= OPT_DISCARD;
                break;
              default:
                ret = LAGOPUS_RESULT_OUT_OF_RANGE;
                lagopus_perror(ret);
                goto done;
            }
          } else {
            lagopus_perror(ret);
            goto done;
          }

          /* type */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_TYPE)) == true) {
            if ((ret = policer_action_type_to_str(type, &type_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_string_append(ds,
                                                      ATTR_NAME_GET(opt_strs,
                                                          OPT_TYPE),
                                                      type_str, true)) !=
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
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_IS_USED)) == true) {
            if ((ret = datastore_json_bool_append(
                         ds, ATTR_NAME_GET(opt_strs, OPT_IS_USED),
                         configs->list[i]->is_used, true)) !=
                LAGOPUS_RESULT_OK) {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* enabled */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_IS_ENABLED)) == true) {
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
policer_action_cmd_parse(datastore_interp_t *iptr,
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
          ret = policer_action_cmd_json_create(&conf_result, &out_configs,
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

/*
 * Public:
 */

lagopus_result_t
policer_action_cmd_enable_propagation(datastore_interp_t *iptr,
                                      datastore_interp_state_t state,
                                      char *name,
                                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = enable_sub_cmd_parse(iptr, state, 0, NULL,
                               name, NULL, policer_action_cmd_update,
                               NULL, result);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

lagopus_result_t
policer_action_cmd_disable_propagation(datastore_interp_t *iptr,
                                       datastore_interp_state_t state,
                                       char *name,
                                       lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = disable_sub_cmd_parse(iptr, state, 0, NULL,
                                name, NULL, policer_action_cmd_update,
                                NULL, result);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

lagopus_result_t
policer_action_cmd_update_propagation(datastore_interp_t *iptr,
                                      datastore_interp_state_t state,
                                      char *name,
                                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  policer_action_conf_t *conf = NULL;

  if (name != NULL) {
    ret = policer_action_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = policer_action_cmd_update(iptr, state,
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
policer_action_cmd_initialize(void) {
  return initialize_internal();
}

void
policer_action_cmd_finalize(void) {
  lagopus_hashmap_destroy(&sub_cmd_table, true);
  sub_cmd_table = NULL;
  lagopus_hashmap_destroy(&discard_opt_table, true);
  discard_opt_table = NULL;

  policer_action_finalize();
}
