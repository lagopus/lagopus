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
#include "channel_cmd.h"
#include "channel_cmd_internal.h"
#include "channel.c"
#include "conv_json.h"
#include "../agent/channel_mgr.h"

/* command name. */
#define CMD_NAME "channel"

/* option num. */
enum channel_opts {
  OPT_NAME = 0,
  OPT_DST_ADDR,
  OPT_DST_PORT,
  OPT_LOCAL_ADDR,
  OPT_LOCAL_PORT,
  OPT_PROTOCOL,
  OPT_IS_USED,
  OPT_IS_ENABLED,

  OPT_MAX,
};

/* option name. */
static const char *const opt_strs[OPT_MAX] = {
  "*name",               /* OPT_NAME (not option) */
  "-dst-addr",           /* OPT_DST_ADDR */
  "-dst-port",           /* OPT_DST_PORT */
  "-local-addr",         /* OPT_LOCAL_ADDR */
  "-local-port",         /* OPT_LOCAL_PORT */
  "-protocol",           /* OPT_PROTOCOL */
  "*is-used",            /* OPT_IS_USED (not option) */
  "*is-enabled",         /* OPT_IS_ENABLED (not option) */
};

typedef struct configs {
  size_t size;
  uint64_t flags;
  bool is_config;
  bool is_show_modified;
  channel_conf_t **list;
} configs_t;

typedef lagopus_result_t
(*ip_addr_set_proc_t)(channel_attr_t *attr,
                      const char *addr, const bool prefer);
typedef lagopus_result_t
(*uint64_set_proc_t)(channel_attr_t *attr, const uint64_t num);
typedef lagopus_result_t
(*uint32_set_proc_t)(channel_attr_t *attr, const uint32_t num);
typedef lagopus_result_t
(*uint16_set_proc_t)(channel_attr_t *attr, const uint16_t num);
typedef lagopus_result_t
(*uint8_set_proc_t)(channel_attr_t *attr, const uint8_t num);

static lagopus_hashmap_t sub_cmd_table = NULL;
static lagopus_hashmap_t opt_table = NULL;

static inline lagopus_result_t
chan_create(const char *name,
            channel_attr_t *attr,
            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_channel_protocol_t protocol;
  lagopus_ip_address_t *dst_addr = NULL;
  lagopus_ip_address_t *local_addr = NULL;
  uint16_t dst_port;
  uint16_t local_port;

  if (((ret = channel_get_dst_addr(attr,
                                   &dst_addr)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = channel_get_dst_port(attr,
                                   &dst_port)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = channel_get_local_addr(attr,
                                     &local_addr)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = channel_get_local_port(attr,
                                     &local_port)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = channel_get_protocol(attr,
                                   &protocol)) ==
       LAGOPUS_RESULT_OK)) {
    lagopus_msg_info("create channel. name = %s.\n", name);
    ret = channel_mgr_channel_create(name, dst_addr, dst_port,
                                     local_addr, local_port, protocol);
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't create channel.");
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't create channel.");
  }

  free(dst_addr);
  free(local_addr);

  return ret;
}

static inline lagopus_result_t
chan_destroy(const char *name, lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_info("destroy channel. name = %s.\n", name);

  ret = channel_mgr_channel_destroy(name);
  if (ret != LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't destroy channel.");
  }

  return ret;
}

static inline lagopus_result_t
chan_start(const char *name, lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_controller_role_t role;
  datastore_controller_connection_type_t type;
  char *c_name = NULL;
  uint64_t dpid;
  bool is_alive = false;
  bool is_enabled = false;

  if (((ret = channel_mgr_channel_dpid_get(name,
              &dpid)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = channel_mgr_channel_is_alive(name,
              &is_alive)) ==
       LAGOPUS_RESULT_OK)) {
    if ((ret = controller_get_name_by_channel(name,
               true,
               &c_name)) ==
        LAGOPUS_RESULT_OK) {
      if (((ret = datastore_controller_is_enabled(c_name,
                  &is_enabled)) ==
           LAGOPUS_RESULT_OK) &&
          ((ret = datastore_controller_get_role(c_name,
                  true,
                  &role)) ==
           LAGOPUS_RESULT_OK) &&
          ((ret = datastore_controller_get_connection_type(c_name,
                  true,
                  &type)) ==
           LAGOPUS_RESULT_OK)) {
        if (dpid != 0 && is_alive == false && is_enabled == true) {
          ret = channel_mgr_controller_set(name, role, type);
          if (ret == LAGOPUS_RESULT_OK) {
            lagopus_msg_info("start channel. name = %s.\n", name);
            ret = channel_mgr_channel_start(name);
            if (ret != LAGOPUS_RESULT_OK) {
              ret = datastore_json_result_string_setf(result, ret,
                                                      "Can't start channel.");
            }
          } else {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't start channel.");
          }
        }
      }
    } else if (ret == LAGOPUS_RESULT_NOT_FOUND) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't start channel.");
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't start channel.");
  }

  free(c_name);

  return ret;
}

static inline lagopus_result_t
chan_stop(const char *name, lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;
  bool is_alive = false;

  if (((ret = channel_mgr_channel_dpid_get(name,
              &dpid)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = channel_mgr_channel_is_alive(name,
              &is_alive)) ==
       LAGOPUS_RESULT_OK)) {
    if (dpid != 0 && is_alive == true) {
      lagopus_msg_info("stop channel. name = %s.\n", name);
      ret = channel_mgr_channel_stop(name);
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't stop channel.");
      }
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't stop channel.");
  }

  return ret;
}

static inline void
channel_cmd_update_current_attr(channel_conf_t *conf,
                                datastore_interp_state_t state) {
  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    return;
  }

  if (conf->modified_attr != NULL) {
    if (conf->current_attr != NULL) {
      channel_attr_destroy(conf->current_attr);
    }
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = NULL;
  }
}

static inline void
channel_cmd_update_aborted(channel_conf_t *conf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (conf->modified_attr != NULL) {
    if (conf->current_attr == NULL) {
      /* create. */
      ret = channel_conf_delete(conf);
      if (ret != LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    } else {
      channel_attr_destroy(conf->modified_attr);
      conf->modified_attr = NULL;
    }
  }
}

static inline void
channel_cmd_update_switch_attr(channel_conf_t *conf) {
  channel_attr_t *attr;

  if (conf->modified_attr != NULL) {
    attr = conf->current_attr;
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = attr;
  }
}

static inline void
channel_cmd_is_enabled_set(channel_conf_t *conf) {
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
channel_cmd_do_destroy(channel_conf_t *conf,
                       datastore_interp_state_t state,
                       lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    ret = channel_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (state == DATASTORE_INTERP_STATE_DRYRUN) {
    ret = channel_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (conf->is_destroying == true ||
             state == DATASTORE_INTERP_STATE_AUTO_COMMIT) {
    ret = chan_destroy(conf->name, result);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }

    ret = channel_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  }
}

static inline lagopus_result_t
channel_cmd_do_update(datastore_interp_t *iptr,
                      datastore_interp_state_t state,
                      channel_conf_t *conf,
                      bool is_propagation,
                      bool is_enable_disable_cmd,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_modified = false;
  uint64_t dpid = 0LL;
  (void) iptr;
  (void) is_propagation;

  if (conf->modified_attr != NULL &&
      channel_attr_equals(conf->current_attr,
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
        /* store dpid. */
        ret = channel_mgr_channel_dpid_get(conf->name, &dpid);
        if (ret != LAGOPUS_RESULT_OK) {
          lagopus_msg_warning("Can't get dpid.\n");
        }
        /* destroy channel. */
        ret = chan_destroy(conf->name, result);
        if (ret != LAGOPUS_RESULT_OK) {
          lagopus_msg_warning("Can't delete channel.\n");
        }
      }

      /* create channel. */
      ret = chan_create(conf->name, conf->modified_attr,
                        result);
      if (ret == LAGOPUS_RESULT_OK) {
        if (dpid != 0) {
          /* restore dpid. */
          ret = channel_mgr_channel_dpid_set(conf->name, dpid);
          if (ret != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("Can't set dpid.\n");
            ret = LAGOPUS_RESULT_OK;
          }
        }
        if (conf->is_enabled == true) {
          ret = chan_start(conf->name, result);
        }
      }

      /* Update attr. */
      if (ret == LAGOPUS_RESULT_OK &&
          state != DATASTORE_INTERP_STATE_COMMITING &&
          state != DATASTORE_INTERP_STATE_ROLLBACKING) {
        channel_cmd_update_current_attr(conf, state);
      }
    } else {
      if (is_enable_disable_cmd == true ||
          conf->is_enabling == true ||
          conf->is_disabling == true) {
        if (conf->is_enabled == true) {
          /* start channel. */
          ret = chan_start(conf->name, result);
        } else {
          /* stop channel. */
          ret = chan_stop(conf->name, result);
        }
      }
      conf->is_enabling = false;
      conf->is_disabling = false;
    }
  }

  return ret;
}

static inline lagopus_result_t
channel_cmd_update_internal(datastore_interp_t *iptr,
                            datastore_interp_state_t state,
                            channel_conf_t *conf,
                            bool is_propagation,
                            bool is_enable_disable_cmd,
                            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int i;

  switch (state) {
    case DATASTORE_INTERP_STATE_AUTO_COMMIT: {
      i = 0;
      while (i < UPDATE_CNT_MAX) {
        ret = channel_cmd_do_update(iptr, state, conf,
                                    is_propagation,
                                    is_enable_disable_cmd,
                                    result);
        if (ret == LAGOPUS_RESULT_OK ||
            is_enable_disable_cmd == true) {
          break;
        } else if (conf->current_attr == NULL &&
                   conf->modified_attr != NULL) {
          channel_cmd_do_destroy(conf, state, result);
          break;
        } else {
          channel_cmd_update_switch_attr(conf);
          lagopus_msg_warning("FAILED auto_comit (%s): rollbacking....\n",
                              lagopus_error_get_string(ret));
        }
        i++;
      }
      break;
    }
    case DATASTORE_INTERP_STATE_COMMITING: {
      channel_cmd_is_enabled_set(conf);
      ret = channel_cmd_do_update(iptr, state, conf,
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
      channel_cmd_update_current_attr(conf, state);
      channel_cmd_do_destroy(conf, state, result);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_ROLLBACKING: {
      if (conf->current_attr == NULL &&
          conf->modified_attr != NULL) {
        /* case create. */
        ret = LAGOPUS_RESULT_OK;
      } else {
        channel_cmd_update_switch_attr(conf);
        channel_cmd_is_enabled_set(conf);
        ret = channel_cmd_do_update(iptr, state, conf,
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
      channel_cmd_update_aborted(conf);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_DRYRUN: {
      if (conf->modified_attr != NULL) {
        if (conf->current_attr != NULL) {
          channel_attr_destroy(conf->current_attr);
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
channel_cmd_update(datastore_interp_t *iptr,
                   datastore_interp_state_t state,
                   void *o,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) iptr;
  (void) result;

  if (iptr != NULL && *iptr != NULL && o != NULL) {
    channel_conf_t *conf = (channel_conf_t *)o;
    ret = channel_cmd_update_internal(iptr, state, conf, false, false, result);
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static lagopus_result_t
uint_opt_parse(const char *const *argv[],
               channel_conf_t *conf,
               configs_t *configs,
               void *uint_set_proc,
               enum channel_opts opt,
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
ip_addr_opt_parse(const char *const *argv[],
                  channel_conf_t *conf,
                  configs_t *configs,
                  ip_addr_set_proc_t ip_addr_set_proc,
                  enum channel_opts opt,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (argv != NULL && conf != NULL &&
      configs != NULL && ip_addr_set_proc != NULL &&
      result != NULL) {
    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        ret = (ip_addr_set_proc)(conf->modified_attr,
                                 *(*argv), true);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't add %s.",
                                                  ATTR_NAME_GET(opt_strs, opt));
        }
      } else {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_INVALID_ARGS,
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
dst_addr_opt_parse(const char *const *argv[],
                   void *c, void *out_configs,
                   lagopus_dstring_t *result) {
  return ip_addr_opt_parse(argv, (channel_conf_t *) c,
                           (configs_t *) out_configs,
                           channel_set_dst_addr_str,
                           OPT_DST_ADDR, result);
}

static lagopus_result_t
dst_port_opt_parse(const char *const *argv[],
                   void *c, void *out_configs,
                   lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (channel_conf_t *) c,
                        (configs_t *) out_configs,
                        channel_set_dst_port,
                        OPT_DST_PORT, CMD_UINT32,
                        result);
}

static lagopus_result_t
local_addr_opt_parse(const char *const *argv[],
                     void *c, void *out_configs,
                     lagopus_dstring_t *result) {
  return ip_addr_opt_parse(argv, (channel_conf_t *) c,
                           (configs_t *) out_configs,
                           channel_set_local_addr_str,
                           OPT_LOCAL_ADDR, result);
}

static lagopus_result_t
local_port_opt_parse(const char *const *argv[],
                     void *c, void *out_configs,
                     lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (channel_conf_t *) c,
                        (configs_t *) out_configs,
                        channel_set_local_port,
                        OPT_LOCAL_PORT, CMD_UINT32,
                        result);
}

static lagopus_result_t
protocol_opt_parse(const char *const *argv[],
                   void *c, void *out_configs,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  channel_conf_t *conf = NULL;
  configs_t *configs = NULL;
  datastore_channel_protocol_t protocol;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (channel_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = channel_protocol_to_enum(*(*argv), &protocol)) ==
            LAGOPUS_RESULT_OK) {
          ret = channel_set_protocol(conf->modified_attr,
                                     protocol);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't add local_addr.");
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
      configs->flags = OPT_BIT_GET(OPT_PROTOCOL);
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
opt_parse(const char *const argv[],
          channel_conf_t *conf,
          configs_t *out_configs,
          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_channel_protocol_t protocol;
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

  /* set required opts. */
  if (conf->modified_attr != NULL) {
    /* set protocol. */
    if ((ret = channel_get_protocol(conf->modified_attr,
                                    &protocol)) ==
        LAGOPUS_RESULT_OK) {
      if (protocol == DATASTORE_CHANNEL_PROTOCOL_UNKNOWN) {
        ret = channel_set_protocol(conf->modified_attr,
                                   DATASTORE_CHANNEL_PROTOCOL_TCP);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't set protocol.");
          goto done;
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't get protocol.");
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
  channel_conf_t *conf = NULL;
  (void) argc;
  (void) proc;

  ret = channel_conf_create(&conf, name);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = channel_conf_add(conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = opt_parse(argv, conf, out_configs, result);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = channel_cmd_update_internal(iptr, state, conf,
                                          true, false, result);

        if (ret != LAGOPUS_RESULT_OK &&
            ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_set(result, ret, NULL);
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add channel_conf.");
    }

    if (ret != LAGOPUS_RESULT_OK) {
      (void) channel_conf_delete(conf);
      goto done;
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't create channel_conf.");
  }

  if (ret != LAGOPUS_RESULT_OK && conf != NULL) {
    channel_conf_destroy(conf);
  }

done:
  return ret;
}

static lagopus_result_t
config_sub_cmd_parse_internal(datastore_interp_t *iptr,
                              datastore_interp_state_t state,
                              size_t argc, const char *const argv[],
                              channel_conf_t *conf,
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
      ret = channel_attr_duplicate(conf->current_attr,
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
      ret = channel_cmd_update_internal(iptr, state, conf,
                                        true, false, result);

      if (ret != LAGOPUS_RESULT_OK &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_set(result, ret, NULL);
      }
    } else {
      /* show. */
      ret = channel_conf_one_list(&out_configs->list, conf);

      if (ret >= 0) {
        out_configs->size = (size_t) ret;
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = datastore_json_result_string_setf(
                result, ret,
                "Can't create list of channel_conf.");
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
  channel_conf_t *conf = NULL;
  char *namespace = NULL;
  (void) hptr;
  (void) proc;

  if (name != NULL) {
    ret = channel_find(name, &conf);

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
  channel_conf_t *conf = NULL;
  (void) hptr;
  (void) argc;
  (void) proc;

  if (name != NULL) {
    ret = channel_find(name, &conf);

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
                              channel_conf_t *conf,
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
          ret = channel_cmd_update_internal(iptr, state, conf,
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
  channel_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = channel_find(name, &conf);

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
                               channel_conf_t *conf,
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
      ret = channel_cmd_update_internal(iptr, state, conf,
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
  channel_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = channel_find(name, &conf);

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
                               channel_conf_t *conf,
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
          ret = channel_cmd_update_internal(iptr, state, conf,
                                            is_propagation, true, result);

          if (ret != LAGOPUS_RESULT_OK) {
            conf->is_enabled = true;
            if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
              ret = datastore_json_result_set(result, ret, NULL);
            }
            goto done;
          }
        }

        channel_cmd_do_destroy(conf, state, result);
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
  channel_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = channel_find(name, &conf);

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

static inline lagopus_result_t
show_parse(const char *name,
           configs_t *out_configs,
           bool is_show_modified,
           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  channel_conf_t *conf = NULL;
  char *target = NULL;

  if (name == NULL) {
    ret = namespace_get_current_namespace(&target);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = channel_conf_list(&out_configs->list, target);
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
      ret = channel_conf_list(&out_configs->list, "");
    } else if (ret == NS_NAMESPACE) {
      /* namespace + delim */
      ret = channel_conf_list(&out_configs->list, target);
    } else if (ret == NS_FULLNAME) {
      /* namespace + name or delim + name */
      ret = channel_find(target, &conf);

      if (ret == LAGOPUS_RESULT_OK) {
        if (conf->is_destroying == false) {
          ret = channel_conf_one_list(&out_configs->list, conf);
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
            "Can't create list of channel_conf.");
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
channel_cmd_json_create(lagopus_dstring_t *ds,
                        configs_t *configs,
                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_channel_protocol_t protocol;
  const char *protocol_str = NULL;
  channel_attr_t *attr = NULL;
  char *dst_addr = NULL;
  char *local_addr = NULL;
  uint16_t dst_port;
  uint16_t local_port;
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

          /* dst addr */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_DST_ADDR)) == true) {
            if ((ret = channel_get_dst_addr_str(attr,
                                                &dst_addr)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_string_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_DST_ADDR),
                           dst_addr, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* dst port */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_DST_PORT)) == true) {
            if ((ret = channel_get_dst_port(attr,
                                            &dst_port)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint32_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_DST_PORT),
                           dst_port, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* local addr */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_LOCAL_ADDR)) == true) {
            if ((ret = channel_get_local_addr_str(attr,
                                                  &local_addr)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_string_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_LOCAL_ADDR),
                           local_addr, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* local port */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_LOCAL_PORT)) == true) {
            if ((ret = channel_get_local_port(attr,
                                              &local_port)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint32_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_LOCAL_PORT),
                           local_port, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* protocol */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_PROTOCOL)) == true) {
            if ((ret = channel_get_protocol(attr,
                                            &protocol)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = channel_protocol_to_str(protocol,
                                                 &protocol_str)) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = datastore_json_string_append(
                             ds, ATTR_NAME_GET(opt_strs, OPT_PROTOCOL),
                             protocol_str, true)) !=
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
      free(dst_addr);
      dst_addr = NULL;
      free(local_addr);
      local_addr = NULL;

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
channel_cmd_parse(datastore_interp_t *iptr,
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
          ret = channel_cmd_json_create(&conf_result, &out_configs,
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
channel_cmd_enable(datastore_interp_t *iptr,
                   datastore_interp_state_t state,
                   void *obj,
                   bool *currentp,
                   bool enabled,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (currentp == NULL) {
    if (enabled == true) {
      ret = enable_sub_cmd_parse_internal(iptr, state,
                                          (channel_conf_t *) obj,
                                          false,
                                          result);
    } else {
      ret = disable_sub_cmd_parse_internal(iptr, state,
                                           (channel_conf_t *) obj,
                                           false,
                                           result);
    }
  } else {
    if (obj != NULL) {
      *currentp = ((channel_conf_t *) obj)->is_enabled;
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
channel_cmd_destroy(datastore_interp_t *iptr,
                    datastore_interp_state_t state,
                    void *obj,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = destroy_sub_cmd_parse_internal(iptr, state,
                                       (channel_conf_t *) obj,
                                       false,
                                       result);
  return ret;
}

STATIC lagopus_result_t
channel_cmd_serialize(datastore_interp_t *iptr,
                      datastore_interp_state_t state,
                      const void *obj,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  channel_conf_t *conf = NULL;

  char *escaped_name = NULL;

  /* dst-addr */
  lagopus_ip_address_t *dst_addr = NULL;
  char *dst_addr_str = NULL;
  char *escaped_dst_addr_str = NULL;

  /* dst-port */
  uint16_t dst_port = 0;

  /* dst-addr */
  lagopus_ip_address_t *local_addr = NULL;
  char *local_addr_str = NULL;
  char *escaped_local_addr_str = NULL;

  /* src-port */
  uint16_t local_port = 0;

  /* protocol */
  datastore_channel_protocol_t protocol = DATASTORE_CHANNEL_PROTOCOL_UNKNOWN;
  //const char *protocol_str = NULL;
  const char *protocol_str = NULL;
  char *escaped_protocol_str = NULL;

  bool is_escaped = false;

  (void) state;

  if (iptr != NULL && obj != NULL && result != NULL) {
    conf = (channel_conf_t *) obj;

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

    /* dst-addr opt. */
    if ((ret = channel_get_dst_addr(conf->current_attr,
                                    &dst_addr)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_ip_address_str_get(dst_addr,
                                            &dst_addr_str)) ==
          LAGOPUS_RESULT_OK) {
        if (IS_VALID_STRING(dst_addr_str) == true) {
          if ((ret = lagopus_str_escape(dst_addr_str, "\"",
                                        &is_escaped,
                                        &escaped_dst_addr_str)) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(result, " %s",
                                               opt_strs[OPT_DST_ADDR])) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(
                           result,
                           ESCAPE_NAME_FMT(is_escaped, escaped_dst_addr_str),
                           escaped_dst_addr_str)) !=
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
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* dst-port opt. */
    if ((ret = channel_get_dst_port(conf->current_attr,
                                    &dst_port)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_DST_PORT])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %d",
                                           dst_port)) !=
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

    /* local-addr opt. */
    if ((ret = channel_get_local_addr(conf->current_attr,
                                      &local_addr)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_ip_address_str_get(local_addr,
                                            &local_addr_str)) ==
          LAGOPUS_RESULT_OK) {
        if (IS_VALID_STRING(local_addr_str) == true) {
          if ((ret = lagopus_str_escape(local_addr_str, "\"",
                                        &is_escaped,
                                        &escaped_local_addr_str)) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(result, " %s",
                                               opt_strs[OPT_LOCAL_ADDR])) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(
                           result,
                           ESCAPE_NAME_FMT(is_escaped, escaped_local_addr_str),
                           escaped_local_addr_str)) !=
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
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* dst-port opt. */
    if ((ret = channel_get_local_port(conf->current_attr,
                                      &local_port)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %s",
                                         opt_strs[OPT_LOCAL_PORT])) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %d",
                                           local_port)) !=
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

    /* protocol opt. */
    if ((ret = channel_get_protocol(conf->current_attr,
                                    &protocol)) ==
        LAGOPUS_RESULT_OK) {
      if (protocol != DATASTORE_CHANNEL_PROTOCOL_UNKNOWN) {
        if ((ret = channel_protocol_to_str(protocol,
                                           &protocol_str)) ==
            LAGOPUS_RESULT_OK) {

          if (IS_VALID_STRING(protocol_str) == true) {
            if ((ret = lagopus_str_escape(protocol_str, "\"",
                                          &is_escaped,
                                          &escaped_protocol_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, " %s",
                                                 opt_strs[OPT_PROTOCOL])) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = lagopus_dstring_appendf(
                             result,
                             ESCAPE_NAME_FMT(is_escaped, escaped_protocol_str),
                             escaped_protocol_str)) !=
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

    /* Add newline. */
    if ((ret = lagopus_dstring_appendf(result, "\n")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

  done:
    free((void *) escaped_name);
    free((void *) dst_addr);
    free((void *) dst_addr_str);
    free((void *) escaped_dst_addr_str);
    free((void *) local_addr);
    free((void *) local_addr_str);
    free((void *) escaped_local_addr_str);
    free((void *) escaped_protocol_str);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
channel_cmd_compare(const void *obj1, const void *obj2, int *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj1 != NULL && obj2 != NULL && result != NULL) {
    *result = strcmp(((channel_conf_t *) obj1)->name,
                     ((channel_conf_t *) obj2)->name);
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
channel_cmd_getname(const void *obj, const char **namep) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj != NULL && namep != NULL) {
    *namep = ((channel_conf_t *)obj)->name;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
channel_cmd_duplicate(const void *obj, const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  channel_conf_t *dup_obj = NULL;

  if (obj != NULL) {
    ret = channel_conf_duplicate(obj, &dup_obj, namespace);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = channel_conf_add(dup_obj);

      if (ret != LAGOPUS_RESULT_OK && dup_obj != NULL) {
        channel_conf_destroy(dup_obj);
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

  if ((ret = channel_initialize()) != LAGOPUS_RESULT_OK) {
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

  if (((ret = opt_add(opt_strs[OPT_DST_ADDR], dst_addr_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_DST_PORT], dst_port_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_LOCAL_ADDR], local_addr_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_LOCAL_PORT], local_port_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_PROTOCOL], protocol_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  if ((ret = datastore_register_table(CMD_NAME,
                                      &channel_table,
                                      channel_cmd_update,
                                      channel_cmd_enable,
                                      channel_cmd_serialize,
                                      channel_cmd_destroy,
                                      channel_cmd_compare,
                                      channel_cmd_getname,
                                      channel_cmd_duplicate)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = datastore_interp_register_command(&s_interp, CONFIGURATOR_NAME,
             CMD_NAME, channel_cmd_parse)) !=
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
channel_cmd_enable_propagation(datastore_interp_t *iptr,
                               datastore_interp_state_t state,
                               char *name,
                               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = enable_sub_cmd_parse(iptr, state, 0, NULL,
                               name, NULL, channel_cmd_update,
                               NULL, result);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

lagopus_result_t
channel_cmd_disable_propagation(datastore_interp_t *iptr,
                                datastore_interp_state_t state,
                                char *name,
                                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = disable_sub_cmd_parse(iptr, state, 0, NULL,
                                name, NULL, channel_cmd_update,
                                NULL, result);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

lagopus_result_t
channel_cmd_update_propagation(datastore_interp_t *iptr,
                               datastore_interp_state_t state,
                               char *name,
                               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  channel_conf_t *conf = NULL;

  if (name != NULL) {
    ret = channel_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = channel_cmd_update(iptr, state,
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
channel_cmd_initialize(void) {
  return initialize_internal();
}

void
channel_cmd_finalize(void) {
  lagopus_hashmap_destroy(&sub_cmd_table, true);
  sub_cmd_table = NULL;
  lagopus_hashmap_destroy(&opt_table, true);
  opt_table = NULL;
  channel_finalize();
}
