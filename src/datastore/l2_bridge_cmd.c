/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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
#include "l2_bridge_cmd.h"
#include "l2_bridge_cmd_internal.h"
#include "l2_bridge.c"
#include "conv_json.h"

/* command name. */
#define CMD_NAME "l2-bridge"

/* option num. */
enum l2_bridge_opts {
  OPT_NAME = 0,
  OPT_EXPIRE,
  OPT_MAX_ENTRIES,
  OPT_TMP_DIR,
  OPT_BRIDGE,
  OPT_IS_USED,
  OPT_IS_ENABLED,

  OPT_MAX,
};

/* stats num. */
enum l2b_stats {
  STATS_ENTRIES = 0,

  STATS_MAX,
};

/* option name. */
static const char *const opt_strs[OPT_MAX] = {
  "*name",               /* OPT_NAME (not option) */
  "-expire",             /* OPT_EXPIRE */
  "-max-entries",        /* OPT_MAX_ENTRIES */
  "-tmp-dir",            /* OPT_TMP_DIR */
  "*bridge",             /* OPT_BRIDGE (not option) */
  "*is-used",            /* OPT_IS_USED (not option) */
  "*is-enabled",         /* OPT_IS_ENABLED (not option) */
};

/* stats name. */
static const char *const stat_strs[STATS_MAX] = {
  "*entries",            /* STATS_ENTRIES (not option) */
};

typedef struct l2_bridge_dump_conf {
  const char *name;
  const char *bridge_name;
  const char *tmp_dir;
} l2_bridge_dump_conf_t;

typedef struct configs {
  size_t size;
  uint64_t flags;
  bool is_config;
  bool is_show_modified;
  bool is_show_stats;
  bool is_dump;
  datastore_l2_bridge_stats_t stats;
  l2_bridge_dump_conf_t dump_conf;
  l2_bridge_conf_t **list;
} configs_t;

typedef lagopus_result_t
(*ip_addr_set_proc_t)(l2_bridge_attr_t *attr,
                      const char *addr, const bool prefer);
typedef lagopus_result_t
(*uint64_set_proc_t)(l2_bridge_attr_t *attr, const uint64_t num);
typedef lagopus_result_t
(*uint32_set_proc_t)(l2_bridge_attr_t *attr, const uint32_t num);
typedef lagopus_result_t
(*uint16_set_proc_t)(l2_bridge_attr_t *attr, const uint16_t num);
typedef lagopus_result_t
(*uint8_set_proc_t)(l2_bridge_attr_t *attr, const uint8_t num);

static lagopus_hashmap_t sub_cmd_table = NULL;
static lagopus_hashmap_t opt_table = NULL;

#include "l2_bridge_cmd_dump.c"

static inline lagopus_result_t
l2_bridge_create(const char *name,
                 l2_bridge_attr_t *attr,
                 lagopus_dstring_t *result) {
  (void) name;
  (void) attr;
  (void) result;

  return LAGOPUS_RESULT_OK;
}

static inline lagopus_result_t
l2_bridge_destroy(const char *name, lagopus_dstring_t *result) {
  (void) name;
  (void) result;

  return LAGOPUS_RESULT_OK;
}

static inline lagopus_result_t
l2_bridge_start(const char *name,
                l2_bridge_attr_t *attr,
                l2_bridge_conf_t *conf,
                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t expire;
  uint64_t max_entries;
  (void) name;

  if (((ret = l2_bridge_get_expire(attr,
                                  &expire)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = l2_bridge_get_max_entries(attr,
                                        &max_entries)) ==
       LAGOPUS_RESULT_OK)) {
    if (bridge_exists(conf->bridge_name) == true) {
      lagopus_msg_info("start l2_bridge. bridge name = %s.\n",
                       conf->bridge_name);
      /* TODO:
       * if (((ret = dp_bridge_l2_expire_set(conf->bridge_name, expire)) !=
       *   LAGOPUS_RESULT_OK) ||
       *  ((ret = dp_bridge_l2_max_entries_set(conf->bridge_name, max_entries)) !=
       *   LAGOPUS_RESULT_OK)) {
       * ret = datastore_json_result_string_setf(result, ret,
       *                                         "Can't start l2_bridge.");
       * }
       */
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_NOT_FOUND,
                                              "bridge name = %s",
                                              conf->bridge_name);
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't start l2_bridge.");
  }

  return ret;
}

static inline lagopus_result_t
l2_bridge_stop(const char *name, lagopus_dstring_t *result) {
  (void) name;
  (void) result;

  return LAGOPUS_RESULT_OK;
}

static inline void
l2_bridge_cmd_update_current_attr(l2_bridge_conf_t *conf,
                                  datastore_interp_state_t state) {
  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    return;
  }

  if (conf->modified_attr != NULL) {
    if (conf->current_attr != NULL) {
      l2_bridge_attr_destroy(conf->current_attr);
    }
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = NULL;
  }
}

static inline void
l2_bridge_cmd_update_aborted(l2_bridge_conf_t *conf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (conf->modified_attr != NULL) {
    if (conf->current_attr == NULL) {
      /* create. */
      ret = l2_bridge_conf_delete(conf);
      if (ret != LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    } else {
      l2_bridge_attr_destroy(conf->modified_attr);
      conf->modified_attr = NULL;
    }
  }
}

static inline void
l2_bridge_cmd_update_switch_attr(l2_bridge_conf_t *conf) {
  l2_bridge_attr_t *attr;

  if (conf->modified_attr != NULL) {
    attr = conf->current_attr;
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = attr;
  }
}

static inline void
l2_bridge_cmd_is_enabled_set(l2_bridge_conf_t *conf) {
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
l2_bridge_cmd_do_destroy(l2_bridge_conf_t *conf,
                         datastore_interp_state_t state,
                         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    ret = l2_bridge_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (conf->is_destroying == true ||
             state == DATASTORE_INTERP_STATE_AUTO_COMMIT) {
    ret = l2_bridge_destroy(conf->name, result);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }

    ret = l2_bridge_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  }
}

static inline lagopus_result_t
l2_bridge_cmd_do_update(datastore_interp_t *iptr,
                        datastore_interp_state_t state,
                        l2_bridge_conf_t *conf,
                        bool is_propagation,
                        bool is_enable_disable_cmd,
                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_modified = false;
  (void) iptr;
  (void) is_propagation;

  if (conf->modified_attr != NULL &&
      l2_bridge_attr_equals(conf->current_attr,
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
        /* destroy l2_bridge. */
        ret = l2_bridge_destroy(conf->name, result);
        if (ret != LAGOPUS_RESULT_OK) {
          lagopus_msg_warning("Can't delete l2_bridge.\n");
        }
      }

      /* create l2_bridge. */
      ret = l2_bridge_create(conf->name, conf->modified_attr,
                             result);
      if (ret == LAGOPUS_RESULT_OK) {
        if (conf->is_enabled == true) {
          ret = l2_bridge_start(conf->name, conf->modified_attr,
                                conf, result);
        }
      }

      /* Update attr. */
      if (ret == LAGOPUS_RESULT_OK &&
          state != DATASTORE_INTERP_STATE_COMMITING &&
          state != DATASTORE_INTERP_STATE_ROLLBACKING) {
        l2_bridge_cmd_update_current_attr(conf, state);
      }
    } else {
      if (is_enable_disable_cmd == true ||
          conf->is_enabling == true ||
          conf->is_disabling == true) {
        if (conf->is_enabled == true) {
          /* start l2_bridge. */
          ret = l2_bridge_start(conf->name, conf->current_attr,
                                conf, result);
        } else {
          /* stop l2_bridge. */
          ret = l2_bridge_stop(conf->name, result);
        }
      }
      conf->is_enabling = false;
      conf->is_disabling = false;
    }
  }

  return ret;
}

static inline lagopus_result_t
l2_bridge_cmd_update_internal(datastore_interp_t *iptr,
                              datastore_interp_state_t state,
                              l2_bridge_conf_t *conf,
                              bool is_propagation,
                              bool is_enable_disable_cmd,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int i;

  switch (state) {
    case DATASTORE_INTERP_STATE_AUTO_COMMIT: {
      i = 0;
      while (i < UPDATE_RETRY_MAX) {
        ret = l2_bridge_cmd_do_update(iptr, state, conf,
                                      is_propagation,
                                      is_enable_disable_cmd,
                                      result);
        if (ret == LAGOPUS_RESULT_OK ||
            is_enable_disable_cmd == true) {
          break;
        } else if (conf->current_attr == NULL &&
                   conf->modified_attr != NULL) {
          l2_bridge_cmd_do_destroy(conf, state, result);
          break;
        } else {
          l2_bridge_cmd_update_switch_attr(conf);
          lagopus_msg_warning("FAILED auto_comit (%s): rollbacking....\n",
                              lagopus_error_get_string(ret));
        }
        i++;
      }
      break;
    }
    case DATASTORE_INTERP_STATE_COMMITING: {
      l2_bridge_cmd_is_enabled_set(conf);
      ret = l2_bridge_cmd_do_update(iptr, state, conf,
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
      l2_bridge_cmd_update_current_attr(conf, state);
      l2_bridge_cmd_do_destroy(conf, state, result);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_ROLLBACKING: {
      if (conf->current_attr == NULL &&
          conf->modified_attr != NULL) {
        /* case create. */
        ret = LAGOPUS_RESULT_OK;
      } else {
        l2_bridge_cmd_update_switch_attr(conf);
        l2_bridge_cmd_is_enabled_set(conf);
        ret = l2_bridge_cmd_do_update(iptr, state, conf,
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
      l2_bridge_cmd_update_aborted(conf);
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
l2_bridge_cmd_update(datastore_interp_t *iptr,
                   datastore_interp_state_t state,
                   void *o,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) iptr;
  (void) result;

  if (iptr != NULL && *iptr != NULL && o != NULL) {
    l2_bridge_conf_t *conf = (l2_bridge_conf_t *)o;
    ret = l2_bridge_cmd_update_internal(iptr, state, conf, false, false, result);
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static lagopus_result_t
uint_opt_parse(const char *const *argv[],
               l2_bridge_conf_t *conf,
               configs_t *configs,
               void *uint_set_proc,
               enum l2_bridge_opts opt,
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
expire_opt_parse(const char *const *argv[],
                 void *c, void *out_configs,
                 lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (l2_bridge_conf_t *) c,
                        (configs_t *) out_configs,
                        l2_bridge_set_expire,
                        OPT_EXPIRE, CMD_UINT64,
                        result);
}

static lagopus_result_t
max_entries_opt_parse(const char *const *argv[],
                      void *c, void *out_configs,
                      lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (l2_bridge_conf_t *) c,
                        (configs_t *) out_configs,
                        l2_bridge_set_max_entries,
                        OPT_MAX_ENTRIES, CMD_UINT64,
                        result);
}

static lagopus_result_t
tmp_dir_opt_parse(const char *const *argv[],
                  void *c, void *out_configs,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *path = NULL;
  l2_bridge_conf_t *conf = NULL;
  configs_t *configs = NULL;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (l2_bridge_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        ret = l2_bridge_set_tmp_dir(conf->modified_attr, (*(*argv)));
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Bad opt value = %s.",
                                                  *(*argv));
        }
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Bad opt value = %s.",
                                                *(*argv));
      }
    } else if (configs->is_config == true) {
      configs->flags = OPT_BIT_GET(OPT_TMP_DIR);
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

  free(path);

  return ret;
}

static inline lagopus_result_t
opt_parse(const char *const argv[],
          l2_bridge_conf_t *conf,
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
                                                  "opt = %s.",
                                                  *argv);
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
  l2_bridge_conf_t *conf = NULL;
  (void) argc;
  (void) proc;

  ret = l2_bridge_conf_create(&conf, name);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = l2_bridge_conf_add(conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = opt_parse(argv, conf, out_configs, result);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = l2_bridge_cmd_update_internal(iptr, state, conf,
                                            true, false, result);

        if (ret != LAGOPUS_RESULT_OK &&
            ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_set(result, ret, NULL);
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add l2_bridge_conf.");
    }

    if (ret != LAGOPUS_RESULT_OK) {
      (void) l2_bridge_conf_delete(conf);
      goto done;
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't create l2_bridge_conf.");
  }

  if (ret != LAGOPUS_RESULT_OK && conf != NULL) {
    l2_bridge_conf_destroy(conf);
  }

done:
  return ret;
}

static lagopus_result_t
l2_bridge_attr_dup_modified(l2_bridge_conf_t *conf,
                            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (conf->modified_attr == NULL) {
    if (conf->current_attr != NULL) {
      /*
       * already exists. copy it.
       */
      ret = l2_bridge_attr_duplicate(conf->current_attr,
                                     &conf->modified_attr);
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
                              l2_bridge_conf_t *conf,
                              datastore_update_proc_t proc,
                              configs_t *out_configs,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) argc;
  (void) proc;

  ret = l2_bridge_attr_dup_modified(conf, result);

  if (ret == LAGOPUS_RESULT_OK) {
    conf->is_destroying = false;
    out_configs->is_config = true;
    ret = opt_parse(argv, conf, out_configs, result);

    if (ret == LAGOPUS_RESULT_OK) {
      if (out_configs->flags == 0) {
        /* update. */
        ret = l2_bridge_cmd_update_internal(iptr, state, conf,
                                            true, false, result);

        if (ret != LAGOPUS_RESULT_OK &&
            ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_set(result, ret, NULL);
        }
      } else {
        /* show. */
        ret = l2_bridge_conf_one_list(&out_configs->list, conf);

        if (ret >= 0) {
          out_configs->size = (size_t) ret;
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = datastore_json_result_string_setf(
              result, ret,
              "Can't create list of l2_bridge_conf.");
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
  l2_bridge_conf_t *conf = NULL;
  char *namespace = NULL;
  (void) hptr;
  (void) proc;

  if (name != NULL) {
    ret = l2_bridge_find(name, &conf);

    if (ret == LAGOPUS_RESULT_NOT_FOUND) {
      ret = namespace_get_namespace(name, &namespace);
      if (ret == LAGOPUS_RESULT_OK) {
        if (namespace_exists(namespace) == true) {
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
  l2_bridge_conf_t *conf = NULL;
  (void) hptr;
  (void) argc;
  (void) proc;

  if (name != NULL) {
    ret = l2_bridge_find(name, &conf);

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
                              l2_bridge_conf_t *conf,
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
          ret = l2_bridge_cmd_update_internal(iptr, state, conf,
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
  l2_bridge_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = l2_bridge_find(name, &conf);

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
                               l2_bridge_conf_t *conf,
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
      ret = l2_bridge_cmd_update_internal(iptr, state, conf,
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
  l2_bridge_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = l2_bridge_find(name, &conf);

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
                               l2_bridge_conf_t *conf,
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
          ret = l2_bridge_cmd_update_internal(iptr, state, conf,
                                              is_propagation, true, result);

          if (ret != LAGOPUS_RESULT_OK) {
            conf->is_enabled = true;
            if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
              ret = datastore_json_result_set(result, ret, NULL);
            }
            goto done;
          }
        }

        l2_bridge_cmd_do_destroy(conf, state, result);
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
  l2_bridge_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = l2_bridge_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      ret = destroy_sub_cmd_parse_internal(iptr, state,
                                           conf, true,
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
  l2_bridge_conf_t *conf = NULL;
  configs_t *configs = NULL;
  (void) iptr;
  (void) state;
  (void) argc;
  (void) hptr;
  (void) proc;

  if (argv != NULL && name != NULL &&
      out_configs != NULL && result != NULL) {
    configs = (configs_t *) out_configs;
    ret = l2_bridge_find(name, &conf);
    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      if (*(argv + 1) == NULL) {
        configs->is_show_stats = true;
        /* TODO:
        ret = dp_l2_bridge_stats_get(conf->name, &configs->stats);
        */
        ret = LAGOPUS_RESULT_OK;
        if (ret == LAGOPUS_RESULT_OK) {
          ret = l2_bridge_conf_one_list(&configs->list, conf);

          if (ret >= 0) {
            configs->size = (size_t) ret;
            ret = LAGOPUS_RESULT_OK;
          } else {
            ret = datastore_json_result_string_setf(
                result, ret,
                "Can't create list of l2_bridge_conf.");
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
dump_sub_cmd_parse(datastore_interp_t *iptr,
                   datastore_interp_state_t state,
                   size_t argc, const char *const argv[],
                   char *name,
                   lagopus_hashmap_t *hptr,
                   datastore_update_proc_t proc,
                   void *out_configs,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  l2_bridge_conf_t *conf = NULL;
  configs_t *configs = NULL;
  char *tmp_dir = NULL;
  (void) iptr;
  (void) state;
  (void) argc;
  (void) hptr;
  (void) proc;

  if (argv != NULL && name != NULL &&
      out_configs != NULL && result != NULL) {
    configs = (configs_t *) out_configs;
    ret = l2_bridge_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      if (bridge_exists(conf->bridge_name) == true) {
        if (((ret = l2_bridge_get_tmp_dir(conf->current_attr,
                                          &tmp_dir)) ==
             LAGOPUS_RESULT_OK)) {
          if (*(argv + 1) == NULL) {
            configs->is_dump = true;
            configs->dump_conf.name = name;
            configs->dump_conf.bridge_name = conf->bridge_name;
            configs->dump_conf.tmp_dir = tmp_dir;

            /* dump. */
            if ((ret = l2_bridge_cmd_dump_thread_start(&configs->dump_conf,
                                                       iptr,
                                                       result)) !=
                LAGOPUS_RESULT_OK) {
              ret = datastore_json_result_string_setf(
                  result,ret,
                  "Can't start flow show thread.");
            }
          } else {
            ret = datastore_json_result_string_setf(
                result,
                LAGOPUS_RESULT_INVALID_ARGS,
                "Bad opt = %s.", *(argv + 1));
            goto done;
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "name = %s", name);
        }
      } else {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_NOT_FOUND,
                                                "bridge name = %s",
                                                conf->bridge_name);
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
  free(tmp_dir);

  return ret;
}

static lagopus_result_t
clear_sub_cmd_parse(datastore_interp_t *iptr,
                    datastore_interp_state_t state,
                    size_t argc, const char *const argv[],
                    char *name,
                    lagopus_hashmap_t *hptr,
                    datastore_update_proc_t proc,
                    void *out_configs,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  l2_bridge_conf_t *conf = NULL;
  (void) iptr;
  (void) state;
  (void) argc;
  (void) hptr;
  (void) proc;
  (void) out_configs;

  if (argv != NULL && name != NULL &&
      out_configs != NULL && result != NULL) {
    ret = l2_bridge_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      if (*(argv + 1) == NULL) {
        if (bridge_exists(conf->bridge_name) == true) {
          ret = LAGOPUS_RESULT_OK;
          /* TODO:
           * if ((ret = dp_bridge_l2_clear(conf->bridge_name)) !=
           *   LAGOPUS_RESULT_OK) {
           *    ret = datastore_json_result_string_setf(
           *      result, ret,
           *      "Can't clear l2 bridge : name = %s",
           *    name);
           * }
           */
        } else {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_NOT_FOUND,
                                                  "bridge name = %s",
                                                  conf->bridge_name);
        }
      } else {
        ret = datastore_json_result_string_setf(
            result,
            LAGOPUS_RESULT_INVALID_ARGS,
            "Bad opt = %s.", *(argv + 1));
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
  l2_bridge_conf_t *conf = NULL;
  char *target = NULL;

  if (name == NULL) {
    ret = namespace_get_current_namespace(&target);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = l2_bridge_conf_list(&out_configs->list, target);
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
      ret = l2_bridge_conf_list(&out_configs->list, "");
    } else if (ret == NS_NAMESPACE) {
      /* namespace + delim */
      ret = l2_bridge_conf_list(&out_configs->list, target);
    } else if (ret == NS_FULLNAME) {
      /* namespace + name or delim + name */
      ret = l2_bridge_find(target, &conf);

      if (ret == LAGOPUS_RESULT_OK) {
        if (conf->is_destroying == false) {
          ret = l2_bridge_conf_one_list(&out_configs->list, conf);
        } else {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_NOT_FOUND,
                                                  "name = %s", name);
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
            "Can't create list of l2_bridge_conf.");
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
l2_bridge_cmd_json_create(lagopus_dstring_t *ds,
                          configs_t *configs,
                          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  l2_bridge_attr_t *attr = NULL;
  char *tmp_dir = NULL;
  uint64_t expire;
  uint64_t max_entries;
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

          /* expire */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_EXPIRE)) == true) {
            if ((ret = l2_bridge_get_expire(attr,
                                            &expire)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_EXPIRE),
                           expire, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* max_entries */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_MAX_ENTRIES)) == true) {
            if ((ret = l2_bridge_get_max_entries(attr,
                                                 &max_entries)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_MAX_ENTRIES),
                           max_entries, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* tmp_dir */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_TMP_DIR)) ==true) {
            if ((ret = l2_bridge_get_tmp_dir(attr,
                                             &tmp_dir)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_string_append(
                      ds, ATTR_NAME_GET(opt_strs, OPT_TMP_DIR),
                      tmp_dir, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* bridge_name */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_BRIDGE)) ==true) {
            if ((ret = datastore_json_string_append(
                    ds, ATTR_NAME_GET(opt_strs, OPT_BRIDGE),
                     configs->list[i]->bridge_name, true)) !=
                LAGOPUS_RESULT_OK) {
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
            if ((ret = datastore_json_bool_append(
                         ds, ATTR_NAME_GET(opt_strs, OPT_IS_ENABLED),
                         configs->list[i]->is_enabled, true)) !=
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
      free(tmp_dir);
      tmp_dir = NULL;

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

static lagopus_result_t
l2_bridge_cmd_stats_json_create(lagopus_dstring_t *ds,
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

        /* entries */
        if ((ret = datastore_json_uint64_append(
                ds, ATTR_NAME_GET(stat_strs, STATS_ENTRIES),
                configs->stats.entries, true)) !=
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
l2_bridge_cmd_parse(datastore_interp_t *iptr,
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
  configs_t out_configs = {0, 0LL, false, false, false, false,
                           {0LL}, {NULL, NULL, NULL}, NULL};
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
      if (ret_for_json == LAGOPUS_RESULT_OK &&
          out_configs.is_dump == false) {
        if (out_configs.size != 0) {
          if (out_configs.is_show_stats == true) {
            ret = l2_bridge_cmd_stats_json_create(&conf_result, &out_configs,
                                                  result);
          } else {
            ret = l2_bridge_cmd_json_create(&conf_result, &out_configs,
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
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
l2_bridge_cmd_enable(datastore_interp_t *iptr,
                     datastore_interp_state_t state,
                     void *obj,
                     bool *currentp,
                     bool enabled,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (currentp == NULL) {
    if (enabled == true) {
      ret = enable_sub_cmd_parse_internal(iptr, state,
                                          (l2_bridge_conf_t *) obj,
                                          false,
                                          result);
    } else {
      ret = disable_sub_cmd_parse_internal(iptr, state,
                                           (l2_bridge_conf_t *) obj,
                                           false,
                                           result);
    }
  } else {
    if (obj != NULL) {
      *currentp = ((l2_bridge_conf_t *) obj)->is_enabled;
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
l2_bridge_cmd_destroy(datastore_interp_t *iptr,
                      datastore_interp_state_t state,
                      void *obj,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = destroy_sub_cmd_parse_internal(iptr, state,
                                       (l2_bridge_conf_t *) obj,
                                       false,
                                       result);
  return ret;
}

STATIC lagopus_result_t
l2_bridge_cmd_serialize(datastore_interp_t *iptr,
                      datastore_interp_state_t state,
                      const void *obj,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  l2_bridge_conf_t *conf = NULL;
  char *escaped_name = NULL;
  char *tmp_dir = NULL;
  uint64_t expire = 0;
  uint64_t max_entries = 0;
  bool is_escaped = false;
  (void) state;

  if (iptr != NULL && obj != NULL && result != NULL) {
    conf = (l2_bridge_conf_t *) obj;

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

    /* expire opt. */
    if ((ret = l2_bridge_get_expire(conf->current_attr,
                                    &expire)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_EXPIRE])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %"PRIu64,
                                           expire)) !=
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

    /* max_entries opt. */
    if ((ret = l2_bridge_get_max_entries(conf->current_attr,
                                         &max_entries)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_MAX_ENTRIES])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %"PRIu64,
                                           max_entries)) !=
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

    /* tmp_dir opt */
    if ((ret = l2_bridge_get_tmp_dir(conf->current_attr,
                                     &tmp_dir)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_TMP_DIR])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %s",
                                           tmp_dir)) !=
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
    free((void *) escaped_name);
    free((void *) tmp_dir);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
l2_bridge_cmd_compare(const void *obj1, const void *obj2, int *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj1 != NULL && obj2 != NULL && result != NULL) {
    *result = strcmp(((l2_bridge_conf_t *) obj1)->name,
                     ((l2_bridge_conf_t *) obj2)->name);
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
l2_bridge_cmd_getname(const void *obj, const char **namep) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj != NULL && namep != NULL) {
    *namep = ((l2_bridge_conf_t *)obj)->name;
    ret = LAGOPUS_RESULT_OK;
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

  if ((ret = l2_bridge_initialize()) != LAGOPUS_RESULT_OK) {
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
      ((ret = sub_cmd_add(DUMP_SUB_CMD, dump_sub_cmd_parse,
                          &sub_cmd_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = sub_cmd_add(CLEAR_SUB_CMD, clear_sub_cmd_parse,
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

  if (((ret = opt_add(opt_strs[OPT_EXPIRE], expire_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_MAX_ENTRIES], max_entries_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_TMP_DIR], tmp_dir_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  if ((ret = datastore_register_table(CMD_NAME,
                                      &l2_bridge_table,
                                      l2_bridge_cmd_update,
                                      l2_bridge_cmd_enable,
                                      l2_bridge_cmd_serialize,
                                      l2_bridge_cmd_destroy,
                                      l2_bridge_cmd_compare,
                                      l2_bridge_cmd_getname)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = datastore_interp_register_command(&s_interp, CONFIGURATOR_NAME,
             CMD_NAME, l2_bridge_cmd_parse)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

  if ((ret = l2_bridge_cmd_dump_initialize()) != LAGOPUS_RESULT_OK) {
    goto done;
  }

done:
  return ret;
}

/*
 * Public:
 */

lagopus_result_t
l2_bridge_cmd_enable_propagation(datastore_interp_t *iptr,
                                 datastore_interp_state_t state,
                                 char *name,
                                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = enable_sub_cmd_parse(iptr, state, 0, NULL,
                               name, NULL, l2_bridge_cmd_update,
                               NULL, result);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

lagopus_result_t
l2_bridge_cmd_disable_propagation(datastore_interp_t *iptr,
                                  datastore_interp_state_t state,
                                  char *name,
                                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = disable_sub_cmd_parse(iptr, state, 0, NULL,
                                name, NULL, l2_bridge_cmd_update,
                                NULL, result);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

lagopus_result_t
l2_bridge_cmd_update_propagation(datastore_interp_t *iptr,
                                 datastore_interp_state_t state,
                                 char *name,
                                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  l2_bridge_conf_t *conf = NULL;

  if (name != NULL) {
    ret = l2_bridge_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = l2_bridge_cmd_update(iptr, state,
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
l2_bridge_cmd_set_bridge(const char *name,
                         const char *bridge_name,
                         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  l2_bridge_conf_t *conf = NULL;
  size_t len;

  if (name != NULL && bridge_name != NULL &&
      result != NULL) {
    ret = l2_bridge_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK) {
      len = strlen(bridge_name);
      if (len <= DATASTORE_BRIDGE_NAME_MAX) {
        strncpy(conf->bridge_name, bridge_name, len);
        conf->bridge_name[len] = '\0';
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = LAGOPUS_RESULT_TOO_LONG;
      }
    } else if (ret != LAGOPUS_RESULT_NOT_FOUND) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "l2 bridge name = %s.", name);
    } else {
      /* ignore LAGOPUS_RESULT_NOT_FOUND. */
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS, NULL);
  }

  return ret;
}

lagopus_result_t
l2_bridge_cmd_initialize(void) {
  return initialize_internal();
}

void
l2_bridge_cmd_finalize(void) {
  l2_bridge_cmd_dump_finalize();
  lagopus_hashmap_destroy(&sub_cmd_table, true);
  sub_cmd_table = NULL;
  lagopus_hashmap_destroy(&opt_table, true);
  opt_table = NULL;
  l2_bridge_finalize();
}
