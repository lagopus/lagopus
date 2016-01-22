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
#include "controller_cmd.h"
#include "controller_cmd_internal.h"
#include "channel_cmd.h"
#include "controller.c"
#include "conv_json.h"
#include "../agent/channel_mgr.h"

/* command name. */
#define CMD_NAME "controller"

/* option num. */
enum controller_opts {
  OPT_NAME = 0,
  OPT_CHANNEL,
  OPT_ROLE,
  OPT_CONNECTION_TYPE,
  OPT_IS_USED,
  OPT_IS_ENABLED,

  OPT_MAX,
};

/* stats num. */
enum controller_stats {
  STATS_IS_CONNECTED = 0,
  STATS_SUPPORTED_VERSIONS,
  STATS_ROLE,

  STATS_MAX,
};

/* option name. */
static const char *const opt_strs[OPT_MAX] = {
  "*name",               /* OPT_NAME (not option) */
  "-channel",            /* OPT_CHANNEL */
  "-role",               /* OPT_ROLE */
  "-connection-type",    /* OPT_CONNECTION_TYPE */
  "*is-used",            /* OPT_IS_USED (not option) */
  "*is-enabled",         /* OPT_IS_ENABLED (not option) */
};

/* stat name. */
static const char *const stat_strs[] = {
  "*is-connected",       /* STATS_IS_CONNECTED (not option) */
  "*supported-versions", /* STATS_SUPPORTED_VERSIONS (not option) */
  "*role",               /* STATS_ROLE (not option) */
};

/* version name. */
static const char *const version_strs[] = {
  "0.0",                 /* OPENFLOW_VERSION_0_0 */
  "1.0",                 /* OPENFLOW_VERSION_1_0 */
  "1.1",                 /* OPENFLOW_VERSION_1_1 */
  "1.2",                 /* OPENFLOW_VERSION_1_2 */
  "1.3",                 /* OPENFLOW_VERSION_1_3 */
  "1.4",                 /* OPENFLOW_VERSION_1_4 */
};

static ssize_t version_strs_len =
  sizeof(version_strs) / sizeof(const char *);

typedef struct configs {
  size_t size;
  uint64_t flags;
  bool is_config;
  bool is_show_modified;
  bool is_show_stats;
  datastore_controller_stats_t stats;
  controller_conf_t **list;
} configs_t;

static lagopus_hashmap_t sub_cmd_table = NULL;
static lagopus_hashmap_t opt_table = NULL;

static inline lagopus_result_t
controller_create(const char *name,
                  controller_attr_t *attr,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_controller_role_t role;
  datastore_controller_connection_type_t type;
  char *channel_name = NULL;

  if (((ret = controller_get_channel_name(attr,
                                          &channel_name)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = controller_get_role(attr,
                                  &role)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = controller_get_connection_type(attr,
              &type)) ==
       LAGOPUS_RESULT_OK)) {
    lagopus_msg_info("create controller. name = %s, channel name = %s.\n",
                     name, channel_name);

    ret = channel_mgr_controller_set(channel_name, role, type);
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add controller.");
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't add controller.");
  }

  free(channel_name);

  return ret;
}

static inline lagopus_result_t
controller_destroy(const char *name,
                   controller_attr_t *attr,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *channel_name = NULL;

  /* empty func. */
  if ((ret = controller_get_channel_name(attr,
                                         &channel_name)) ==
      LAGOPUS_RESULT_OK) {
    lagopus_msg_info("destroy controller. name = %s, channel name = %s.\n",
                     name, channel_name);
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't delete controller.");
  }

  free(channel_name);

  return ret;
}

static inline lagopus_result_t
controller_start(const char *name,
                 controller_attr_t *attr,
                 datastore_interp_t *iptr,
                 datastore_interp_state_t state,
                 bool is_propagation,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *channel_name = NULL;
  uint64_t dpid;
  bool is_alive;

  if (((ret = controller_get_channel_name(attr,
                                          &channel_name)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = channel_mgr_channel_dpid_get(channel_name,
              &dpid)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = channel_mgr_channel_is_alive(channel_name,
              &is_alive)) ==
       LAGOPUS_RESULT_OK)) {
    if (IS_VALID_STRING(channel_name) == true) {
      if (is_propagation == true) {
        if ((ret = channel_cmd_enable_propagation(iptr, state,
                   channel_name, result)) !=
            LAGOPUS_RESULT_OK) {
          if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "channel name = %s.",
                                                    channel_name);
          }
          goto done;
        }
      }

      if (dpid != 0 && is_alive == false) {
        lagopus_msg_info("start controller. name = %s, channel name = %s.\n",
                         name, channel_name);

        ret = channel_mgr_channel_start(channel_name);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't start controller.");
        }
      }
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't start controller.");
  }

done:
  free(channel_name);

  return ret;
}

static inline lagopus_result_t
controller_stop(const char *name,
                controller_attr_t *attr,
                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *channel_name = NULL;
  uint64_t dpid;
  bool is_alive;

  if (((ret = controller_get_channel_name(attr,
                                          &channel_name)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = channel_mgr_channel_dpid_get(channel_name,
              &dpid)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = channel_mgr_channel_is_alive(channel_name,
              &is_alive)) ==
       LAGOPUS_RESULT_OK)) {
    if (dpid != 0 && is_alive == true) {
      lagopus_msg_info("stop controller. name = %s, channel name = %s.\n",
                       name, channel_name);
      ret = channel_mgr_channel_stop(channel_name);
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't stop controller.");
      }
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't stop controller.");
  }

  free(channel_name);

  return ret;
}

static inline lagopus_result_t
controller_channel_used_set(controller_attr_t *attr, bool b,
                            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *channel_name = NULL;

  if ((ret = controller_get_channel_name(attr,
                                         &channel_name)) ==
      LAGOPUS_RESULT_OK) {
    if (IS_VALID_STRING(channel_name) == true) {
      ret = channel_set_used(channel_name, b);
      /* ignore : LAGOPUS_RESULT_NOT_FOUND */
      if (ret == LAGOPUS_RESULT_OK || ret == LAGOPUS_RESULT_NOT_FOUND) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "channel name = %s.",
                                                channel_name);
      }
    }
  } else {
    ret = datastore_json_result_set(result, ret, NULL);
  }

  free(channel_name);

  return ret;
}

static inline lagopus_result_t
controller_channel_disable(const char *name,
                           controller_attr_t *attr,
                           datastore_interp_t *iptr,
                           datastore_interp_state_t state,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *channel_name = NULL;
  (void) name;

  if ((ret = controller_get_channel_name(attr,
                                         &channel_name)) ==
      LAGOPUS_RESULT_OK) {
    if (IS_VALID_STRING(channel_name) == true) {
      if ((ret = channel_cmd_disable_propagation(iptr, state,
                 channel_name, result)) !=
          LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "channel name = %s.",
                                                channel_name);
      }

    }
  } else {
    ret = datastore_json_result_set(result, ret, NULL);
  }

  free(channel_name);

  return ret;
}

static inline lagopus_result_t
channel_update(datastore_interp_t *iptr,
               datastore_interp_state_t state,
               controller_conf_t *conf,
               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *channel_name = NULL;
  controller_attr_t *attr;

  attr = ((conf->modified_attr == NULL) ?
          conf->current_attr : conf->modified_attr);
  if ((ret = controller_get_channel_name(attr,
                                         &channel_name)) ==
      LAGOPUS_RESULT_OK) {
    if (IS_VALID_STRING(channel_name)) {
      ret = channel_cmd_update_propagation(iptr, state,
                                           channel_name, result);
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't update channel.");
  }

  free(channel_name);

  return ret;
}

static inline void
controller_cmd_update_current_attr(controller_conf_t *conf,
                                   datastore_interp_state_t state) {
  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    return;
  }

  if (conf->modified_attr != NULL) {
    if (conf->current_attr != NULL) {
      controller_attr_destroy(conf->current_attr);
    }
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = NULL;
  }
}

static inline void
controller_cmd_update_aborted(controller_conf_t *conf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (conf->modified_attr != NULL) {
    if (conf->current_attr == NULL) {
      /* create. */
      ret = controller_conf_delete(conf);
      if (ret != LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    } else {
      controller_attr_destroy(conf->modified_attr);
      conf->modified_attr = NULL;
    }
  }
}

static inline void
controller_cmd_update_switch_attr(controller_conf_t *conf) {
  controller_attr_t *attr;

  if (conf->modified_attr != NULL) {
    attr = conf->current_attr;
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = attr;
  }
}

static inline void
controller_cmd_is_enabled_set(controller_conf_t *conf) {
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
controller_cmd_do_destroy(controller_conf_t *conf,
                          datastore_interp_state_t state,
                          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    ret = controller_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (state == DATASTORE_INTERP_STATE_DRYRUN) {
    /* unset is_used. */
    if (conf->current_attr != NULL) {
      if ((ret = controller_channel_used_set(conf->current_attr,
                                             false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }
    if (conf->modified_attr != NULL) {
      if ((ret = controller_channel_used_set(conf->modified_attr,
                                             false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }

    ret = controller_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (conf->is_destroying == true ||
             state == DATASTORE_INTERP_STATE_AUTO_COMMIT) {
    /* unset is_used. */
    if (conf->current_attr != NULL) {
      if ((ret = controller_channel_used_set(conf->current_attr,
                                             false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }
    if (conf->modified_attr != NULL) {
      if ((ret = controller_channel_used_set(conf->modified_attr,
                                             false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }

    if (conf->current_attr != NULL) {
      ret = controller_destroy(conf->name, conf->current_attr, result);
      if (ret != LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }

    ret = controller_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  }
}

static inline lagopus_result_t
controller_cmd_do_update(datastore_interp_t *iptr,
                         datastore_interp_state_t state,
                         controller_conf_t *conf,
                         bool is_propagation,
                         bool is_enable_disable_cmd,
                         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_modified = false;
  (void) iptr;

  if (conf->modified_attr != NULL &&
      controller_attr_equals(conf->current_attr,
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
    /* update channel. */
    if (is_propagation == true) {
      if ((ret = channel_update(iptr, state, conf, result)) !=
          LAGOPUS_RESULT_OK) {
        goto done;
      }
    }
    if (is_modified == true) {
      /*
       * Update attrs.
       */
      if (conf->current_attr != NULL) {
        /* unset is_used. */
        if ((ret = controller_channel_used_set(conf->current_attr,
                                               false, result)) ==
            LAGOPUS_RESULT_OK) {
          if (is_propagation == true) {
            ret = controller_channel_disable(conf->name,
                                             conf->current_attr,
                                             iptr, state, result);
          }
          if (ret == LAGOPUS_RESULT_OK) {
            /* destroy controller */
            ret = controller_destroy(conf->name, conf->current_attr,
                                     result);
            if (ret != LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("Can't delete controller.\n");
            }
          }
        }
      } else {
        ret = LAGOPUS_RESULT_OK;
      }

      if (ret == LAGOPUS_RESULT_OK) {
        /* add controller. */
        ret = controller_create(conf->name, conf->modified_attr,
                                result);
        if (ret == LAGOPUS_RESULT_OK) {
          /* set is_used. */
          if ((ret = controller_channel_used_set(conf->modified_attr,
                                                 true, result)) ==
              LAGOPUS_RESULT_OK) {
            if (conf->is_enabled == true) {
              /* start controller. */
              ret = controller_start(conf->name, conf->modified_attr,
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
        controller_cmd_update_current_attr(conf, state);
      }
    } else {
      if (is_enable_disable_cmd == true ||
          conf->is_enabling == true ||
          conf->is_disabling == true) {
        if (conf->is_enabled == true) {
          /* start controller. */
          ret = controller_start(conf->name, conf->current_attr,
                                 iptr, state,
                                 is_propagation, result);
        } else {
          /* stop controller. */
          ret = controller_stop(conf->name, conf->current_attr,
                                result);
        }
      }
      conf->is_enabling = false;
      conf->is_disabling = false;
    }
  }

done:
  return ret;
}

static inline lagopus_result_t
controller_cmd_update_internal(datastore_interp_t *iptr,
                               datastore_interp_state_t state,
                               controller_conf_t *conf,
                               bool is_propagation,
                               bool is_enable_disable_cmd,
                               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int i;

  switch (state) {
    case DATASTORE_INTERP_STATE_AUTO_COMMIT: {
      i = 0;
      while (i < UPDATE_CNT_MAX) {
        ret = controller_cmd_do_update(iptr, state, conf,
                                       is_propagation,
                                       is_enable_disable_cmd,
                                       result);
        if (ret == LAGOPUS_RESULT_OK ||
            is_enable_disable_cmd == true) {
          break;
        } else if (conf->current_attr == NULL &&
                   conf->modified_attr != NULL) {
          controller_cmd_do_destroy(conf, state, result);
          break;
        } else {
          controller_cmd_update_switch_attr(conf);
          lagopus_msg_warning("FAILED auto_comit (%s): rollbacking....\n",
                              lagopus_error_get_string(ret));
        }
        i++;
      }
      break;
    }
    case DATASTORE_INTERP_STATE_COMMITING: {
      controller_cmd_is_enabled_set(conf);
      ret = controller_cmd_do_update(iptr, state, conf,
                                     is_propagation,
                                     is_enable_disable_cmd,
                                     result);
      break;
    }
    case DATASTORE_INTERP_STATE_ATOMIC: {
      /* set is_used. */
      if (conf->modified_attr != NULL) {
        if ((ret = controller_channel_used_set(conf->modified_attr,
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
      controller_cmd_update_current_attr(conf, state);
      controller_cmd_do_destroy(conf, state, result);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_ROLLBACKING: {
      if (conf->current_attr == NULL &&
          conf->modified_attr != NULL) {
        /* case create. */
        /* unset is_used. */
        if ((ret = controller_channel_used_set(conf->modified_attr,
                                               false, result)) !=
            LAGOPUS_RESULT_OK) {
          /* ignore error. */
          lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
        }
        ret = LAGOPUS_RESULT_OK;
      } else {
        controller_cmd_update_switch_attr(conf);
        controller_cmd_is_enabled_set(conf);
        ret = controller_cmd_do_update(iptr, state, conf,
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
        if ((ret = controller_channel_used_set(conf->modified_attr,
                                               false, result)) !=
            LAGOPUS_RESULT_OK) {
          /* ignore error. */
          lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
        }
      }
      if (conf->current_attr != NULL) {
        /* unset is_used. */
        if ((ret = controller_channel_used_set(conf->current_attr,
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
      controller_cmd_update_aborted(conf);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_DRYRUN: {
      if (conf->modified_attr != NULL) {
        if (conf->current_attr != NULL) {
          controller_attr_destroy(conf->current_attr);
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
controller_cmd_update(datastore_interp_t *iptr,
                      datastore_interp_state_t state,
                      void *o,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) iptr;
  (void) result;

  if (iptr != NULL && *iptr != NULL && o != NULL) {
    controller_conf_t *conf = (controller_conf_t *)o;
    ret = controller_cmd_update_internal(iptr, state, conf, false, false, result);
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static lagopus_result_t
channel_opt_parse(const char *const *argv[],
                  void *c, void *out_configs,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  controller_conf_t *conf = NULL;
  configs_t *configs = NULL;
  bool is_used = false;
  char *name = NULL;
  char *fullname = NULL;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (controller_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = lagopus_str_unescape(*(*argv), "\"'", &name)) >= 0) {
          if ((ret = namespace_get_fullname(name, &fullname))
              == LAGOPUS_RESULT_OK) {
            /* check exists. */
            if (channel_exists(fullname) == true) {
              /* check is_used. */
              if ((ret = datastore_channel_is_used(fullname, &is_used)) ==
                  LAGOPUS_RESULT_OK) {
                if (is_used == false) {
                  /* unset is_used. */
                  if (conf->current_attr != NULL) {
                    if ((ret = controller_channel_used_set(conf->current_attr,
                                                           false, result)) !=
                        LAGOPUS_RESULT_OK) {
                      ret = datastore_json_result_string_setf(result, ret,
                                                              "channel name = "
                                                              "%s.", fullname);
                      goto done;
                    }
                  }
                  if (conf->modified_attr != NULL) {
                    if ((ret = controller_channel_used_set(conf->modified_attr,
                                                           false, result)) !=
                        LAGOPUS_RESULT_OK) {
                      ret = datastore_json_result_string_setf(result, ret,
                                                              "channel name = "
                                                              "%s.", fullname);
                      goto done;
                    }
                  }

                  ret = controller_set_channel_name(conf->modified_attr,
                                                    fullname);
                  if (ret != LAGOPUS_RESULT_OK) {
                    ret = datastore_json_result_string_setf(result, ret,
                                                            "channel name = %s.",
                                                            fullname);
                  }
                } else {
                  ret = datastore_json_result_string_setf(
                          result, LAGOPUS_RESULT_NOT_OPERATIONAL,
                          "channel name = %s.", fullname);
                }
              } else {
                ret = datastore_json_result_string_setf(result, ret,
                                                        "channel name = %s.",
                                                        fullname);
              }
            } else {
              ret = datastore_json_result_string_setf(result,
                                                      LAGOPUS_RESULT_NOT_FOUND,
                                                      "channel name = %s.",
                                                      fullname);
            }
          } else {
            ret = datastore_json_result_string_setf(result,
                                                    ret,
                                                    "Can't get fullname %s.",
                                                    name);
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "channel name = %s.",
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
      configs->flags = OPT_BIT_GET(OPT_CHANNEL);
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
  free(fullname);

  return ret;
}

static lagopus_result_t
role_opt_parse(const char *const *argv[],
               void *c, void *out_configs,
               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  controller_conf_t *conf = NULL;
  configs_t *configs = NULL;
  datastore_controller_role_t role;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (controller_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = controller_role_to_enum(*(*argv), &role)) ==
            LAGOPUS_RESULT_OK) {
          ret = controller_set_role(conf->modified_attr,
                                    role);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't add role.");
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
      configs->flags = OPT_BIT_GET(OPT_ROLE);
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
connection_type_opt_parse(const char *const *argv[],
                          void *c, void *out_configs,
                          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  controller_conf_t *conf = NULL;
  configs_t *configs = NULL;
  datastore_controller_connection_type_t type;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (controller_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = controller_connection_type_to_enum(*(*argv), &type)) ==
            LAGOPUS_RESULT_OK) {
          ret = controller_set_connection_type(conf->modified_attr,
                                               type);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_string_setf(
                    result, ret,
                    "Can't add connection_type.");
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
      configs->flags = OPT_BIT_GET(OPT_CONNECTION_TYPE);
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
          controller_conf_t *conf,
          configs_t *out_configs,
          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_controller_role_t role;
  datastore_controller_connection_type_t type;
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

  /* set required opts. */
  if (conf->modified_attr != NULL) {
    if ((ret = controller_get_role(conf->modified_attr,
                                   &role)) ==
        LAGOPUS_RESULT_OK) {
      if (role == DATASTORE_CONTROLLER_ROLE_UNKNOWN) {
        ret = controller_set_role(conf->modified_attr,
                                  DATASTORE_CONTROLLER_ROLE_EQUAL);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't set role.");
          goto done;
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't get role.");
      goto done;
    }

    if ((ret = controller_get_connection_type(conf->modified_attr,
               &type)) ==
        LAGOPUS_RESULT_OK) {
      if (type == DATASTORE_CONTROLLER_CONNECTION_TYPE_UNKNOWN) {
        ret = controller_set_connection_type(
                conf->modified_attr,
                DATASTORE_CONTROLLER_CONNECTION_TYPE_MAIN);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't set connection type.");
          goto done;
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't get connection type.");
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
  controller_conf_t *conf = NULL;
  (void) argc;
  (void) proc;

  ret = controller_conf_create(&conf, name);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = controller_conf_add(conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = opt_parse(argv, conf, out_configs, result);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = controller_cmd_update_internal(iptr, state, conf,
                                             true, false, result);

        if (ret != LAGOPUS_RESULT_OK &&
            ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_set(result, ret, NULL);
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add controller_conf.");
    }

    if (ret != LAGOPUS_RESULT_OK) {
      (void) controller_conf_delete(conf);
      goto done;
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't create controller_conf.");
  }

  if (ret != LAGOPUS_RESULT_OK && conf != NULL) {
    controller_conf_destroy(conf);
  }

done:
  return ret;
}

static lagopus_result_t
config_sub_cmd_parse_internal(datastore_interp_t *iptr,
                              datastore_interp_state_t state,
                              size_t argc, const char *const argv[],
                              controller_conf_t *conf,
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
      ret = controller_attr_duplicate(conf->current_attr,
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
      ret = controller_cmd_update_internal(iptr, state, conf,
                                           true, false, result);

      if (ret != LAGOPUS_RESULT_OK &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_set(result, ret, NULL);
      }
    } else {
      /* show. */
      ret = controller_conf_one_list(&out_configs->list, conf);

      if (ret >= 0) {
        out_configs->size = (size_t) ret;
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = datastore_json_result_string_setf(
                result, ret,
                "Can't create list of controller_conf.");
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
  controller_conf_t *conf = NULL;
  char *namespace = NULL;
  (void) hptr;
  (void) proc;

  if (name != NULL) {
    ret = controller_find(name, &conf);

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
  controller_conf_t *conf = NULL;
  (void) hptr;
  (void) argc;
  (void) proc;

  if (name != NULL) {
    ret = controller_find(name, &conf);

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
                              controller_conf_t *conf,
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
          ret = controller_cmd_update_internal(iptr, state, conf,
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
  controller_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = controller_find(name, &conf);

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
                               controller_conf_t *conf,
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
      ret = controller_cmd_update_internal(iptr, state, conf,
                                           false, true, result);
      if (ret == LAGOPUS_RESULT_OK) {
        /* disable propagation. */
        if (is_propagation == true) {
          ret = controller_channel_disable(conf->name, conf->current_attr,
                                           iptr, state, result);
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
  controller_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = controller_find(name, &conf);

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
                               controller_conf_t *conf,
                               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && conf != NULL && result != NULL) {
    if (conf->is_used == false) {
      if (state == DATASTORE_INTERP_STATE_ATOMIC) {
        conf->is_destroying = true;
        conf->is_enabling = false;
        conf->is_disabling = true;
      } else {
        /* disable channel. */
        ret = controller_channel_disable(conf->name, conf->current_attr,
                                         iptr, state, result);
        if (ret != LAGOPUS_RESULT_OK) {
          /* ignore error. */
          lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
        }

        if (conf->is_enabled == true) {
          conf->is_enabled = false;
          ret = controller_cmd_update_internal(iptr, state, conf,
                                               false, true, result);

          if (ret != LAGOPUS_RESULT_OK) {
            conf->is_enabled = true;
            if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
              ret = datastore_json_result_set(result, ret, NULL);
            }
            goto done;
          }
        }
        controller_cmd_do_destroy(conf, state, result);
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
  controller_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = controller_find(name, &conf);

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
  controller_conf_t *conf = NULL;
  configs_t *configs = NULL;
  char *channel_name = NULL;
  datastore_channel_status_t status;
  (void) iptr;
  (void) state;
  (void) argc;
  (void) hptr;
  (void) proc;

  if (argv != NULL && name != NULL &&
      out_configs != NULL && result != NULL) {
    configs = (configs_t *) out_configs;
    ret = controller_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      if (*(argv + 1) == NULL) {
        configs->is_show_stats = true;

        if (((ret = controller_get_channel_name(conf->current_attr,
                                                &channel_name)) ==
             LAGOPUS_RESULT_OK) &&
            ((ret = channel_mgr_channel_connection_status_get(
                      channel_name, &status)) ==
             LAGOPUS_RESULT_OK) &&
            ((ret = channel_mgr_channel_role_get(channel_name, &configs->stats.role)) ==
             LAGOPUS_RESULT_OK) &&
            ((ret = channel_mgr_ofp_version_get(channel_name, &configs->stats.version)) ==
             LAGOPUS_RESULT_OK)) {
          switch (status) {
            case DATASTORE_CHANNEL_CONNECTED:
              configs->stats.is_connected = true;
              break;
            default:
              configs->stats.is_connected = false;
              break;
          }
          ret = controller_conf_one_list(&configs->list, conf);

          if (ret >= 0) {
            configs->size = (size_t) ret;
            ret = LAGOPUS_RESULT_OK;
          } else {
            ret = datastore_json_result_string_setf(
                    result, ret,
                    "Can't create list of controller_conf.");
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get stats.");
        }
      } else {
        ret = datastore_json_result_string_setf(result,
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

  free(channel_name);

  return ret;
}


static inline lagopus_result_t
show_parse(const char *name,
           configs_t *out_configs,
           bool is_show_modified,
           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  controller_conf_t *conf = NULL;
  char *target = NULL;

  if (name == NULL) {
    ret = namespace_get_current_namespace(&target);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = controller_conf_list(&out_configs->list, target);
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
      ret = controller_conf_list(&out_configs->list, "");
    } else if (ret == NS_NAMESPACE) {
      /* namespace + delim */
      ret = controller_conf_list(&out_configs->list, target);
    } else if (ret == NS_FULLNAME) {
      /* namespace + name or delim + name */
      ret = controller_find(target, &conf);

      if (ret == LAGOPUS_RESULT_OK) {
        if (conf->is_destroying == false) {
          ret = controller_conf_one_list(&out_configs->list, conf);
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
            "Can't create list of controller_conf.");
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
controller_cmd_json_create(lagopus_dstring_t *ds,
                           configs_t *configs,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_controller_role_t role;
  const char *role_str = NULL;
  datastore_controller_connection_type_t type;
  const char *type_str = NULL;
  char *channel_name = NULL;
  controller_attr_t *attr = NULL;
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

          /* channel_name */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_CHANNEL)) ==
              true) {
            if ((ret = controller_get_channel_name(attr,
                                                   &channel_name)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_string_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_CHANNEL),
                           channel_name, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* role */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_ROLE)) == true) {
            if ((ret = controller_get_role(attr,
                                           &role)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = controller_role_to_str(role,
                                                &role_str)) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = datastore_json_string_append(ds,
                                                        ATTR_NAME_GET(opt_strs,
                                                            OPT_ROLE),
                                                        role_str, true)) !=
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

          /* connection_type */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_CONNECTION_TYPE)) == true) {
            if ((ret = controller_get_connection_type(attr,
                       &type)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = controller_connection_type_to_str(type,
                         &type_str)) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = datastore_json_string_append(
                             ds, ATTR_NAME_GET(opt_strs, OPT_CONNECTION_TYPE),
                             type_str, true)) !=
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
      free(channel_name);
      channel_name = NULL;

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
controller_cmd_stats_json_create(lagopus_dstring_t *ds,
                                 configs_t *configs,
                                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *role_str = NULL;
  const char *version = NULL;

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

        /* is_connected */
        if ((ret = datastore_json_bool_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_IS_CONNECTED),
                     configs->stats.is_connected, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* supported_versions */
        if ((ret = DSTRING_CHECK_APPENDF(
                     ds, true,
                     KEY_FMT"[",
                     ATTR_NAME_GET(stat_strs,
                                   STATS_SUPPORTED_VERSIONS))) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        if (configs->stats.version < version_strs_len) {
          version = version_strs[configs->stats.version];
        } else {
          version = version_strs[0];
        }
        if ((ret = lagopus_dstring_appendf(
                     ds, "\"%s\"", version)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        if ((ret = lagopus_dstring_appendf(ds, "]")) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* role */
        if ((ret = controller_role_to_str(configs->stats.role,
                                          &role_str)) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = datastore_json_string_append(ds,
                                                  ATTR_NAME_GET(stat_strs,
                                                      STATS_ROLE),
                                                  role_str, true)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
        } else {
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
controller_cmd_parse(datastore_interp_t *iptr,
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
    {false, 0, DATASTORE_CONTROLLER_ROLE_UNKNOWN},
    NULL
  };
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
          if (out_configs.is_show_stats == true) {
            ret = controller_cmd_stats_json_create(&conf_result, &out_configs,
                                                   result);
          } else {
            ret = controller_cmd_json_create(&conf_result, &out_configs,
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
controller_cmd_enable(datastore_interp_t *iptr,
                      datastore_interp_state_t state,
                      void *obj,
                      bool *currentp,
                      bool enabled,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (currentp == NULL) {
    if (enabled == true) {
      ret = enable_sub_cmd_parse_internal(iptr, state,
                                          (controller_conf_t *) obj,
                                          false,
                                          result);
    } else {
      ret = disable_sub_cmd_parse_internal(iptr, state,
                                           (controller_conf_t *) obj,
                                           false,
                                           result);
    }
  } else {
    if (obj != NULL) {
      *currentp = ((controller_conf_t *) obj)->is_enabled;
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
controller_cmd_destroy(datastore_interp_t *iptr,
                       datastore_interp_state_t state,
                       void *obj,
                       lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = destroy_sub_cmd_parse_internal(iptr, state,
                                       (controller_conf_t *) obj,
                                       result);

  return ret;
}

STATIC lagopus_result_t
controller_cmd_serialize(datastore_interp_t *iptr,
                         datastore_interp_state_t state,
                         const void *obj,
                         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  controller_conf_t *conf = NULL;

  char *escaped_name = NULL;

  /* channel-name */
  char *channel_name = NULL;
  char *escaped_channel_name = NULL;

  /* role */
  datastore_controller_role_t role = DATASTORE_CONTROLLER_ROLE_UNKNOWN;
  const char *role_str = NULL;
  char *escaped_role_str = NULL;

  /* type */
  datastore_controller_connection_type_t type =
    DATASTORE_CONTROLLER_CONNECTION_TYPE_UNKNOWN;
  const char *type_str = NULL;
  char *escaped_type_str = NULL;

  bool is_escaped = false;

  (void) state;

  if (iptr != NULL && obj != NULL && result != NULL) {
    conf = (controller_conf_t *) obj;

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

    /* channel-name */
    if ((ret = controller_get_channel_name(conf->current_attr,
                                           &channel_name)) ==
        LAGOPUS_RESULT_OK) {
      if (IS_VALID_STRING(channel_name) == true) {
        if ((ret = lagopus_str_escape(channel_name, "\"",
                                      &is_escaped,
                                      &escaped_channel_name)) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %s",
                                             opt_strs[OPT_CHANNEL])) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(
                         result,
                         ESCAPE_NAME_FMT(is_escaped, escaped_channel_name),
                         escaped_channel_name)) !=
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

    /* role opt. */
    if ((ret = controller_get_role(conf->current_attr,
                                   &role)) ==
        LAGOPUS_RESULT_OK) {
      if (role != DATASTORE_CONTROLLER_ROLE_UNKNOWN) {
        if ((ret = controller_role_to_str(role,
                                          &role_str)) ==
            LAGOPUS_RESULT_OK) {
          if (IS_VALID_STRING(role_str) == true) {
            if ((ret = lagopus_str_escape(role_str, "\"",
                                          &is_escaped,
                                          &escaped_role_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, " %s",
                                                 opt_strs[OPT_ROLE])) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = lagopus_dstring_appendf(
                             result,
                             ESCAPE_NAME_FMT(is_escaped, escaped_role_str),
                             escaped_role_str)) !=
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

    /* connection-type opt. */
    if ((ret = controller_get_connection_type(conf->current_attr,
               &type)) ==
        LAGOPUS_RESULT_OK) {
      if (type != DATASTORE_CONTROLLER_CONNECTION_TYPE_UNKNOWN) {
        if ((ret = controller_connection_type_to_str(type,
                   &type_str)) ==
            LAGOPUS_RESULT_OK) {

          if (IS_VALID_STRING(type_str) == true) {
            if ((ret = lagopus_str_escape(type_str, "\"",
                                          &is_escaped,
                                          &escaped_type_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(result, " %s",
                                                 opt_strs[OPT_CONNECTION_TYPE])) ==
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

    /* Add newline. */
    if ((ret = lagopus_dstring_appendf(result, "\n")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

  done:
    free((void *) escaped_name);
    free((void *) channel_name);
    free((void *) escaped_channel_name);
    free((void *) escaped_role_str);
    free((void *) escaped_type_str);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
controller_cmd_compare(const void *obj1, const void *obj2, int *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj1 != NULL && obj2 != NULL && result != NULL) {
    *result = strcmp(((controller_conf_t *) obj1)->name,
                     ((controller_conf_t *) obj2)->name);
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
controller_cmd_getname(const void *obj, const char **namep) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj != NULL && namep != NULL) {
    *namep = ((controller_conf_t *)obj)->name;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
controller_cmd_duplicate(const void *obj, const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  controller_conf_t *dup_obj = NULL;

  if (obj != NULL) {
    ret = controller_conf_duplicate(obj, &dup_obj, namespace);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = controller_conf_add(dup_obj);

      if (ret != LAGOPUS_RESULT_OK && dup_obj != NULL) {
        controller_conf_destroy(dup_obj);
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

  if ((ret = controller_initialize()) != LAGOPUS_RESULT_OK) {
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

  if (((ret = opt_add(opt_strs[OPT_CHANNEL], channel_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_ROLE], role_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_CONNECTION_TYPE], connection_type_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  if ((ret = datastore_register_table(CMD_NAME,
                                      &controller_table,
                                      controller_cmd_update,
                                      controller_cmd_enable,
                                      controller_cmd_serialize,
                                      controller_cmd_destroy,
                                      controller_cmd_compare,
                                      controller_cmd_getname,
                                      controller_cmd_duplicate)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = datastore_interp_register_command(&s_interp, CONFIGURATOR_NAME,
             CMD_NAME,
             controller_cmd_parse)) !=
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
controller_cmd_enable_propagation(datastore_interp_t *iptr,
                                  datastore_interp_state_t state,
                                  char *name,
                                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = enable_sub_cmd_parse(iptr, state, 0, NULL,
                               name, NULL, controller_cmd_update,
                               NULL, result);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

lagopus_result_t
controller_cmd_disable_propagation(datastore_interp_t *iptr,
                                   datastore_interp_state_t state,
                                   char *name,
                                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  controller_conf_t *conf = NULL;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = controller_find(name, &conf);

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
controller_cmd_update_propagation(datastore_interp_t *iptr,
                                  datastore_interp_state_t state,
                                  char *name,
                                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  controller_conf_t *conf = NULL;

  if (name != NULL) {
    ret = controller_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = controller_cmd_update(iptr, state,
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
controller_cmd_initialize(void) {
  return initialize_internal();
}

void
controller_cmd_finalize(void) {
  lagopus_hashmap_destroy(&sub_cmd_table, true);
  sub_cmd_table = NULL;
  lagopus_hashmap_destroy(&opt_table, true);
  opt_table = NULL;
  controller_finalize();
}
