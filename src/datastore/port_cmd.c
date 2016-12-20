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
#include "port_cmd.h"
#include "port_cmd_internal.h"
#include "interface_cmd.h"
#include "queue_cmd.h"
#include "policer_cmd.h"
#include "port.c"
#include "conv_json.h"
#include "lagopus/port.h"
#include "lagopus/dp_apis.h"

/* command name. */
#define CMD_NAME "port"

#define UNUSED_QUEUE_ID 0


/* option num. */
enum port_opts {
  OPT_NAME = 0,
  OPT_PORT_NO,
  OPT_INTERFACE,
  OPT_POLICER,
  OPT_QUEUES,
  OPT_QUEUE,
  OPT_IS_USED,
  OPT_IS_ENABLED,

  OPT_MAX,
};

/* stats num. */
enum p_stats {
  STATS_CONFIG = 0,
  STATS_STATE,
  STATS_CURR_FEATURES,
  STATS_SUPPORTED_FEATURES,

  STATS_MAX,
};

/* option name. */
static const char *const opt_strs[OPT_MAX] = {
  "*name",               /* OPT_NAME (not option) */
  "*port-number",        /* OPT_PORT_NO (not option) */
  "-interface",          /* OPT_INTERFACE */
  "-policer",            /* OPT_POLICER */
  "*queues",             /* OPT_QUEUES (not option) */
  "-queue",              /* OPT_QUEUE */
  "*is-used",            /* OPT_IS_USED (not option) */
  "*is-enabled",         /* OPT_IS_ENABLED (not option) */
};

/* stats name. */
static const char *const stat_strs[STATS_MAX] = {
  "*config",             /* STATS_CONFIG (not option) */
  "*state",              /* STATS_STATE (not option) */
  "*curr-features",      /* STATS_CURR_FEATURES (not option) */
  "*supported-features", /* STATS_SUPPORTED_FEATURES (not option) */
};

/* config name. */
static const char *const config_strs[] = {
  "port-down",           /* OFPPC_PORT_DOWN */
  "unknown",
  "no-recv",             /* OFPPC_NO_RECV */
  "unknown",
  "unknown",
  "no-fwd",              /* OFPPC_NO_FWD */
  "no-packet-in",        /* OFPPC_NO_PACKET_IN */
};

static const size_t config_strs_size =
  sizeof(config_strs) / sizeof(const char *);

/* state name. */
static const char *const state_strs[] = {
  "link-down",           /* OFPPS_LINK_DOWN */
  "blocked",             /* OFPPS_BLOCKED */
  "live",                /* OFPPS_LIVE */
};

static const size_t state_strs_size =
  sizeof(state_strs) / sizeof(const char *);

/* features name. */
static const char *const feature_strs[] = {
  "10mb-hd",             /* OFPPF_10MB_HD */
  "10mb-fd",             /* OFPPF_10MB_FD */
  "100mb-hd",            /* OFPPF_100MB_HD */
  "100mb-fd",            /* OFPPF_100MB_FD */
  "1gb-hd",              /* OFPPF_1GB_HD */
  "1gb-fd",              /* OFPPF_1GB_FD */
  "10gb-fd",             /* OFPPF_10GB_FD */
  "40gb-fd",             /* OFPPF_40GB_FD */
  "100gb-fd",            /* OFPPF_100GB_FD */
  "1tb-fd",              /* OFPPF_1TB_FD */
  "other",               /* OFPPF_OTHER */
  "copper",              /* OFPPF_COPPER */
  "fiber",               /* OFPPF_FIBER */
  "autoneg",             /* OFPPF_AUTONEG */
  "pause",               /* OFPPF_PAUSE */
  "pause-asym",          /* OFPPF_PAUSE_ASYM */
};

static const size_t feature_strs_size =
  sizeof(feature_strs) / sizeof(const char *);

typedef struct configs {
  size_t size;
  uint64_t flags;
  bool is_config;
  bool is_show_modified;
  bool is_show_stats;
  datastore_port_stats_t stats;
  port_conf_t **list;
  datastore_interp_t *iptr;
} configs_t;

struct names_info {
  datastore_name_info_t *not_change_names;
  datastore_name_info_t *add_names;
  datastore_name_info_t *del_names;
};

static lagopus_hashmap_t sub_cmd_table = NULL;
static lagopus_hashmap_t opt_table = NULL;

static inline lagopus_result_t
ofp_port_names_disable(const char *name,
                       port_attr_t *attr,
                       struct names_info *q_names_info,
                       datastore_interp_t *iptr,
                       datastore_interp_state_t state,
                       bool is_propagation,
                       bool is_unset_used,
                       lagopus_dstring_t *result);

/* get no change queue names. */
static inline lagopus_result_t
ofp_port_names_info_create(struct names_info *names_info,
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
ofp_port_names_info_destroy(struct names_info *names_info) {
  datastore_names_destroy(names_info->not_change_names);
  datastore_names_destroy(names_info->add_names);
  datastore_names_destroy(names_info->del_names);
}

/* port. */
static inline lagopus_result_t
ofp_port_port_create(const char *name,
                     port_attr_t *attr,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *interface_name = NULL;

  if ((ret = port_get_interface_name(attr,
                                     &interface_name)) ==
      LAGOPUS_RESULT_OK) {
    lagopus_msg_info("create port. name = %s, interface name = %s.\n",
                     name,
                     (IS_VALID_STRING(interface_name) == true) ?
                     interface_name : "<NULL>" );
    if ((ret = dp_port_create(name)) ==
        LAGOPUS_RESULT_OK) {
      if (IS_VALID_STRING(interface_name) == true) {
        if ((ret = dp_port_interface_set(name,
                                         interface_name)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't create port.");
          goto done;
        }
      }
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't create port.");
  }

done:
  free(interface_name);

  return ret;
}

static inline lagopus_result_t
ofp_port_port_destroy(const char *name,
                      port_attr_t *attr,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *interface_name = NULL;

  if ((ret = port_get_interface_name(attr,
                                     &interface_name)) ==
      LAGOPUS_RESULT_OK) {
    lagopus_msg_info("destroy port. name = %s, interface name = %s.\n",
                     name,
                     (IS_VALID_STRING(interface_name) == true) ?
                     interface_name : "<NULL>" );
    ret = dp_port_destroy(name);
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't get interface name.");
  }

  free(interface_name);

  return ret;
}

/* interface */
static inline lagopus_result_t
ofp_port_interface_used_set(port_attr_t *attr, bool b,
                            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *interface_name = NULL;

  if ((ret = port_get_interface_name(attr,
                                     &interface_name)) ==
      LAGOPUS_RESULT_OK) {
    if (IS_VALID_STRING(interface_name) == true) {
      ret = interface_set_used(interface_name, b);
      /* ignore : LAGOPUS_RESULT_NOT_FOUND */
      if (ret == LAGOPUS_RESULT_OK || ret == LAGOPUS_RESULT_NOT_FOUND) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "interface name = %s.",
                                                interface_name);
      }
    }
  } else {
    ret = datastore_json_result_set(result, ret, NULL);
  }

  free(interface_name);

  return ret;
}

static inline lagopus_result_t
ofp_port_interface_disable(const char *name,
                           port_attr_t *attr,
                           datastore_interp_t *iptr,
                           datastore_interp_state_t state,
                           bool is_propagation,
                           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *interface_name = NULL;
  (void) name;
  (void) is_propagation;

  if ((ret = port_get_interface_name(attr,
                                     &interface_name)) ==
      LAGOPUS_RESULT_OK) {
    if (IS_VALID_STRING(interface_name) == true) {
      if ((ret = interface_cmd_disable_propagation(iptr, state,
                 interface_name, result)) !=
          LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "interface name = %s.",
                                                interface_name);
      }
    }
  } else {
    ret = datastore_json_result_set(result, ret, NULL);
  }

  free(interface_name);

  return ret;
}

static inline lagopus_result_t
ofp_port_interface_update(datastore_interp_t *iptr,
                          datastore_interp_state_t state,
                          port_conf_t *conf,
                          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *interface_name = NULL;
  port_attr_t *attr;

  attr = ((conf->modified_attr == NULL) ?
          conf->current_attr : conf->modified_attr);
  if ((ret = port_get_interface_name(attr,
                                     &interface_name)) ==
      LAGOPUS_RESULT_OK) {
    if (IS_VALID_STRING(interface_name) == true) {
      ret = interface_cmd_update_propagation(iptr, state,
                                             interface_name, result);
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't update interface.");
  }

  free(interface_name);

  return ret;
}

/* policer */
static inline lagopus_result_t
ofp_port_policer_add(const char *name,
                     port_attr_t *attr,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *policer_name = NULL;

  if ((ret = port_get_policer_name(attr,
                                   &policer_name)) ==
      LAGOPUS_RESULT_OK) {
    lagopus_msg_info("add port policer(%s). policer name = %s.\n",
                     name, policer_name);

    if (IS_VALID_STRING(policer_name) == true) {
      if ((ret = dp_port_policer_set(name,
                                     policer_name)) !=
          LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't add policer.");
        goto done;
      }
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't create policer.");
  }

done:
  free(policer_name);

  return ret;
}

static inline lagopus_result_t
ofp_port_policer_delete(const char *name,
                        port_attr_t *attr,
                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *policer_name = NULL;

  if ((ret = port_get_policer_name(attr,
                                   &policer_name)) ==
      LAGOPUS_RESULT_OK) {
    lagopus_msg_info("delete port policer(%s). policer name = %s.\n",
                     name, policer_name);

    if (IS_VALID_STRING(policer_name) == true) {
      if ((ret = dp_port_policer_unset(name)) !=
          LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't delete policer.");
        goto done;
      }
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't delete policer.");
  }

done:
  free(policer_name);

  return ret;
}

static inline lagopus_result_t
ofp_port_policer_used_set(port_attr_t *attr, bool b,
                          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *policer_name = NULL;
  (void) b;

  if ((ret = port_get_policer_name(attr,
                                   &policer_name)) ==
      LAGOPUS_RESULT_OK) {
    if (IS_VALID_STRING(policer_name) == true) {
      ret = policer_set_used(policer_name, b);

      /* ignore : LAGOPUS_RESULT_NOT_FOUND */
      if (ret == LAGOPUS_RESULT_OK || ret == LAGOPUS_RESULT_NOT_FOUND) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "policer name = %s.",
                                                policer_name);
      }
    }
  } else {
    ret = datastore_json_result_set(result, ret, NULL);
  }

  free(policer_name);

  return ret;
}

static inline lagopus_result_t
ofp_port_policer_disable(const char *name,
                         port_attr_t *attr,
                         datastore_interp_t *iptr,
                         datastore_interp_state_t state,
                         bool is_propagation,
                         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *policer_name = NULL;
  (void) name;
  (void) iptr;
  (void) state;
  (void) is_propagation;

  if ((ret = port_get_policer_name(attr,
                                   &policer_name)) ==
      LAGOPUS_RESULT_OK) {
    if (IS_VALID_STRING(policer_name) == true) {
      if ((ret = policer_cmd_disable_propagation(iptr, state,
                 policer_name, result)) !=
          LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "policer name = %s.",
                                                policer_name);
        goto done;
      }
    }
  } else {
    ret = datastore_json_result_set(result, ret, NULL);
    goto done;
  }

done:
  free(policer_name);

  return ret;
}

static inline lagopus_result_t
ofp_port_policer_update(datastore_interp_t *iptr,
                        datastore_interp_state_t state,
                        port_conf_t *conf,
                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *policer_name = NULL;
  port_attr_t *attr;
  (void) iptr;
  (void) state;

  attr = ((conf->modified_attr == NULL) ?
          conf->current_attr : conf->modified_attr);
  if ((ret = port_get_policer_name(attr,
                                   &policer_name)) ==
      LAGOPUS_RESULT_OK) {
    if (IS_VALID_STRING(policer_name) == true) {
      ret = policer_cmd_update_propagation(iptr, state,
                                           policer_name, result);
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't update policer.");
  }

  free(policer_name);

  return ret;
}

/* queues */
static inline lagopus_result_t
ofp_port_queues_add(const char *name,
                    port_attr_t *attr,
                    struct names_info *names_info,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *q_name = NULL;
  datastore_name_info_t *q_names = NULL;
  struct datastore_name_head *head;

  if (names_info == NULL) {
    if ((ret = port_get_queue_names(attr, &q_names)) ==
        LAGOPUS_RESULT_OK) {
      head = &q_names->head;
    } else {
      ret = datastore_json_result_set(result, ret, NULL);
      goto done;
    }
  } else {
    head = &names_info->add_names->head;
  }

  if (TAILQ_EMPTY(head) == false) {
    TAILQ_FOREACH(q_name, head, name_entries) {
      lagopus_msg_info("add port queue(%s). "
                       "queue name = %s.\n",
                       name, q_name->str);
      ret = dp_port_queue_add(name, q_name->str);
      if (ret != LAGOPUS_RESULT_OK) {
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
  if (q_names != NULL) {
    datastore_names_destroy(q_names);
  }

  return ret;
}

static inline lagopus_result_t
ofp_port_queues_delete(const char *name,
                       port_attr_t *attr,
                       struct names_info *names_info,
                       lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *q_name = NULL;
  datastore_name_info_t *q_names = NULL;
  struct datastore_name_head *head;

  if (names_info == NULL) {
    if ((ret = port_get_queue_names(attr, &q_names)) ==
        LAGOPUS_RESULT_OK) {
      head = &q_names->head;
    } else {
      ret = datastore_json_result_set(result, ret, NULL);
      goto done;
    }
  } else {
    head = &names_info->del_names->head;
  }

  if (TAILQ_EMPTY(head) == false) {
    TAILQ_FOREACH(q_name, head, name_entries) {
      lagopus_msg_info("delete port queue(%s). "
                       "queue name = %s.\n",
                       name, q_name->str);
      ret = dp_port_queue_delete(name, q_name->str);
      if (ret == LAGOPUS_RESULT_OK ||
          ret == LAGOPUS_RESULT_NOT_FOUND) {
        /* ignore LAGOPUS_RESULT_NOT_FOUND. */
        ret = LAGOPUS_RESULT_OK;
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
  if (q_names != NULL) {
    datastore_names_destroy(q_names);
  }

  return ret;
}

static inline lagopus_result_t
ofp_port_queues_disable(const char *name,
                        port_attr_t *attr,
                        struct names_info *names_info,
                        datastore_interp_t *iptr,
                        datastore_interp_state_t state,
                        bool is_propagation,
                        bool is_unset_used,
                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *q_name = NULL;
  datastore_name_info_t *q_names = NULL;
  struct datastore_name_head *head;
  (void) name;

  if (names_info == NULL) {
    if ((ret = port_get_queue_names(attr, &q_names)) ==
        LAGOPUS_RESULT_OK) {
      head = &q_names->head;
    } else {
      ret = datastore_json_result_set(result, ret, NULL);
      goto done;
    }
  } else {
    head = &names_info->del_names->head;
  }

  if (TAILQ_EMPTY(head) == false) {
    TAILQ_FOREACH(q_name, head, name_entries) {
      if (is_propagation == true ||
          state == DATASTORE_INTERP_STATE_COMMITING ||
          state == DATASTORE_INTERP_STATE_ROLLBACKING) {
        /* disable propagation. */
        if ((ret = queue_cmd_disable_propagation(iptr, state,
                   q_name->str, result)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result,
                                                  ret,
                                                  "queue name = %s.",
                                                  q_name->str);
          break;
        }
      }

      if (is_unset_used == true) {
        /* unset used. */
        ret = port_set_used(q_name->str, false);
        /* ignore : LAGOPUS_RESULT_NOT_FOUND */
        if (ret == LAGOPUS_RESULT_OK || ret == LAGOPUS_RESULT_NOT_FOUND) {
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "queue name = %s.",
                                                  q_name->str);
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
  if (q_names != NULL) {
    datastore_names_destroy(q_names);
  }

  return ret;
}

static inline lagopus_result_t
ofp_port_queue_used_set_internal(const char *name, bool b,
                                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = queue_set_used(name, b);
  /* ignore : LAGOPUS_RESULT_NOT_FOUND */
  if (ret == LAGOPUS_RESULT_OK || ret == LAGOPUS_RESULT_NOT_FOUND) {
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "queue name = %s.",
                                            name);
  }

  return ret;
}

static inline lagopus_result_t
ofp_port_queues_used_set(port_attr_t *attr, bool b,
                         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_name_info_t *q_names = NULL;
  struct datastore_name_entry *name = NULL;

  if ((ret = port_get_queue_names(attr, &q_names)) ==
      LAGOPUS_RESULT_OK) {
    TAILQ_FOREACH(name, &q_names->head, name_entries) {
      ret = ofp_port_queue_used_set_internal(name->str, b, result);
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "queue name = %s.",
                                                name->str);
        break;
      }
    }
  } else {
    ret = datastore_json_result_set(result, ret, NULL);
  }

  if (q_names != NULL) {
    datastore_names_destroy(q_names);
  }

  return ret;
}

/* get queue names info. */
static inline lagopus_result_t
ofp_port_queue_names_info_get(port_conf_t *conf,
                              struct names_info *names_info,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *name = NULL;
  datastore_name_info_t *current_names = NULL;
  datastore_name_info_t *modified_names = NULL;

  if (conf->modified_attr != NULL &&
      names_info != NULL) {
    if (conf->current_attr == NULL) {
      if (((ret = port_get_queue_names(conf->modified_attr, &modified_names)) ==
           LAGOPUS_RESULT_OK)) {
        TAILQ_FOREACH(name, &modified_names->head, name_entries) {
          if (port_attr_queue_name_exists(conf->current_attr,
                                          name->str) == false) {
            /* add_names. */
            if (datastore_name_exists(names_info->add_names,
                                      name->str) == false) {
              if ((ret = datastore_add_names(names_info->add_names,
                                             name->str)) !=
                  LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(
                        result, ret, "Can't add queue names (add).");
                goto done;
              }
            }
          }
        }
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't get queue names.");
      }
    } else {
      if (((ret = port_get_queue_names(conf->current_attr, &current_names)) ==
           LAGOPUS_RESULT_OK) &&
          ((ret = port_get_queue_names(conf->modified_attr, &modified_names)) ==
           LAGOPUS_RESULT_OK)) {
        TAILQ_FOREACH(name, &current_names->head, name_entries) {
          if (port_attr_queue_name_exists(conf->modified_attr,
                                          name->str) == true) {
            /* not_change_names. */
            if (datastore_name_exists(names_info->not_change_names,
                                      name->str) == false) {
              if ((ret = datastore_add_names(names_info->not_change_names,
                                             name->str)) !=
                  LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(
                        result, ret, "Can't add queue names (not change).");
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
                        result, ret, "Can't add queue names (del).");
                goto done;
              }
            }
          }
        }
        TAILQ_FOREACH(name, &modified_names->head, name_entries) {
          if (port_attr_queue_name_exists(conf->current_attr,
                                          name->str) == false) {
            /* add_names. */
            if (datastore_name_exists(names_info->add_names,
                                      name->str) == false) {
              if ((ret = datastore_add_names(names_info->add_names,
                                             name->str)) !=
                  LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(
                        result, ret, "Can't add queue names (add).");
                goto done;
              }
            }
          }
        }
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't get queue names.");
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
ofp_port_queue_update(datastore_interp_t *iptr,
                      datastore_interp_state_t state,
                      port_conf_t *conf,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct names_info names_info;
  struct datastore_name_entry *name = NULL;

  if ((ret = ofp_port_names_info_create(&names_info, result)) !=
      LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't update queue.");
    goto done;
  }

  if ((ret = ofp_port_queue_names_info_get(conf, &names_info, result)) !=
      LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't update queue.");
    goto done;
  }

  if (TAILQ_EMPTY(&names_info.not_change_names->head) == false) {
    TAILQ_FOREACH(name, &names_info.not_change_names->head, name_entries) {
      if (((ret = queue_cmd_update_propagation(iptr, state, name->str,
                  result)) !=
           LAGOPUS_RESULT_OK) &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't update queue.");
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

  if (TAILQ_EMPTY(&names_info.add_names->head) == false) {
    TAILQ_FOREACH(name, &names_info.add_names->head, name_entries) {
      if (((ret = queue_cmd_update_propagation(iptr, state, name->str,
                  result)) !=
           LAGOPUS_RESULT_OK) &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't update queue.");
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

  if (TAILQ_EMPTY(&names_info.del_names->head) == false) {
    TAILQ_FOREACH(name, &names_info.del_names->head, name_entries) {
      if (((ret = queue_cmd_update_propagation(iptr, state, name->str,
                  result)) !=
           LAGOPUS_RESULT_OK) &&
          ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't update queue.");
        lagopus_perror(ret);
        goto done;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

done:
  ofp_port_names_info_destroy(&names_info);

  return ret;
}

/* objs */
static inline lagopus_result_t
ofp_port_create(const char *name,
                port_attr_t *attr,
                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = ofp_port_port_create(name, attr, result);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = ofp_port_queues_add(name, attr, NULL, result);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = ofp_port_policer_add(name, attr, result);
    }
  }

  return ret;
}

static inline lagopus_result_t
ofp_port_destroy(const char *name,
                 port_attr_t *attr,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = ofp_port_policer_delete(name, attr, result);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = ofp_port_queues_delete(name, attr, NULL,
                                 result);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = ofp_port_port_destroy(name, attr, result);
    }
  }

  return ret;
}

static inline lagopus_result_t
ofp_port_start(const char *name,
               port_attr_t *attr,
               struct names_info *q_names_info,
               datastore_interp_t *iptr,
               datastore_interp_state_t state,
               bool is_propagation,
               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *q_name = NULL;
  datastore_name_info_t *q_names = NULL;
  char *interface_name = NULL;
  char *policer_name = NULL;
  struct datastore_name_head *head;

  /* propagation policer. */
  if ((ret = port_get_policer_name(attr,
                                   &policer_name)) ==
      LAGOPUS_RESULT_OK) {
    if (IS_VALID_STRING(policer_name) == true) {
      if ((ret = policer_cmd_enable_propagation(iptr, state,
                 policer_name, result)) !=
          LAGOPUS_RESULT_OK) {
        if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "policer name = %s.",
                                                  policer_name);
        }
        goto done;
      }
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't get interface name.\n");
    goto done;
  }

  /* propagation queues. */
  if (TAILQ_EMPTY(&q_names_info->add_names->head) == true) {
    if ((ret = port_get_queue_names(attr, &q_names)) ==
        LAGOPUS_RESULT_OK) {
      head = &q_names->head;
    } else {
      ret = datastore_json_result_set(result, ret, NULL);
      goto done;
    }
  } else {
    head = &q_names_info->add_names->head;
  }

  if (TAILQ_EMPTY(head) == false) {
    TAILQ_FOREACH(q_name, head, name_entries) {
      if (is_propagation == true) {
        if ((ret = queue_cmd_enable_propagation(iptr, state,
                                                q_name->str, result)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result,
                                                  ret,
                                                  "Can't start queue.");
          goto done;
        }
      }
    }
  }

  /* propagation interface. */
  if ((ret = port_get_interface_name(attr,
                                     &interface_name)) ==
      LAGOPUS_RESULT_OK) {
    if (IS_VALID_STRING(interface_name) == true) {
      if (is_propagation == true) {
        if ((ret = interface_cmd_enable_propagation(iptr, state,
                   interface_name, result)) !=
            LAGOPUS_RESULT_OK) {
          if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "interface name = %s.",
                                                    interface_name);
          }
          goto done;
        }
      }
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't get interface name.\n");
    goto done;
  }

  lagopus_msg_info("start port. name = %s, interface name = %s.\n",
                   name,
                   (IS_VALID_STRING(interface_name) == true) ?
                   interface_name : "<NULL>" );
  if ((ret = dp_port_start(name)) !=
      LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't start port.");
  }

done:
  free(interface_name);
  free(policer_name);
  if (q_names != NULL) {
    datastore_names_destroy(q_names);
  }

  return ret;
}

static inline lagopus_result_t
ofp_port_stop(const char *name,
              port_attr_t *attr,
              datastore_interp_t *iptr,
              datastore_interp_state_t state,
              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *interface_name = NULL;

  if ((ret = port_get_interface_name(attr,
                                     &interface_name)) ==
      LAGOPUS_RESULT_OK) {
    lagopus_msg_info("stop port. name = %s, interface name = %s.\n",
                     name,
                     (IS_VALID_STRING(interface_name) == true) ?
                     interface_name : "<NULL>" );

    if ((ret = ofp_port_names_disable(name,
                                      attr,
                                      NULL,
                                      iptr, state,
                                      true,
                                      false,
                                      result)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = dp_port_stop(name)) !=
          LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't stop port.");
      }
    } else if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't stop port.");
    }
  } else {
    ret = datastore_json_result_set(result, ret, NULL);
  }

  free(interface_name);

  return ret;
}

/* name objs */
static inline lagopus_result_t
ofp_port_names_add(const char *name,
                   port_attr_t *attr,
                   struct names_info *q_names_info,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = ofp_port_policer_add(name, attr, result);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = ofp_port_queues_add(name, attr, q_names_info, result);
  }

  return ret;
}

static inline lagopus_result_t
ofp_port_names_delete(const char *name,
                      port_attr_t *attr,
                      struct names_info *q_names_info,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = ofp_port_policer_delete(name, attr, result);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = ofp_port_queues_delete(name, attr, q_names_info, result);
  }

  return ret;
}

/* set is_used for interface/policer. */
static inline lagopus_result_t
ofp_port_names_used_set(port_attr_t *attr, bool b,
                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((ret = ofp_port_interface_used_set(attr, b, result)) ==
      LAGOPUS_RESULT_OK) {
    if ((ret = ofp_port_queues_used_set(attr, b, result)) ==
        LAGOPUS_RESULT_OK) {
      ret = ofp_port_policer_used_set(attr, b, result);
    }
  }

  return ret;
}

static inline lagopus_result_t
ofp_port_names_disable(const char *name,
                       port_attr_t *attr,
                       struct names_info *q_names_info,
                       datastore_interp_t *iptr,
                       datastore_interp_state_t state,
                       bool is_propagation,
                       bool is_unset_used,
                       lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((ret = ofp_port_interface_disable(name, attr, iptr,
                                        state, is_propagation,
                                        result)) ==
      LAGOPUS_RESULT_OK) {
    if ((ret = ofp_port_queues_disable(name, attr,
                                       q_names_info, iptr,
                                       state, is_propagation,
                                       is_unset_used,
                                       result)) ==
        LAGOPUS_RESULT_OK) {
      ret = ofp_port_policer_disable(name, attr, iptr,
                                     state, is_propagation,
                                     result);
    }
  }

  return ret;
}

/* get queue names info. */
static inline lagopus_result_t
ofp_port_names_info_get(port_conf_t *conf,
                        struct names_info *q_names_info,
                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = ofp_port_queue_names_info_get(conf, q_names_info, result);

  return ret;
}

static inline void
port_cmd_update_current_attr(port_conf_t *conf,
                             datastore_interp_state_t state) {
  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    return;
  }

  if (conf->modified_attr != NULL) {
    if (conf->current_attr != NULL) {
      port_attr_destroy(conf->current_attr);
    }
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = NULL;
  }
}

static inline void
port_cmd_update_aborted(port_conf_t *conf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (conf->modified_attr != NULL) {
    if (conf->current_attr == NULL) {
      /* create. */
      ret = port_conf_delete(conf);
      if (ret != LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    } else {
      port_attr_destroy(conf->modified_attr);
      conf->modified_attr = NULL;
    }
  }
}

static inline void
port_cmd_update_switch_attr(port_conf_t *conf) {
  port_attr_t *attr;

  if (conf->modified_attr != NULL) {
    attr = conf->current_attr;
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = attr;
  }
}

static inline void
port_cmd_is_enabled_set(port_conf_t *conf) {
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
port_cmd_do_destroy(port_conf_t *conf,
                    datastore_interp_state_t state,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    ret = port_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (state == DATASTORE_INTERP_STATE_DRYRUN) {
    /* unset is_used. */
    if (conf->current_attr != NULL) {
      if ((ret = ofp_port_names_used_set(conf->current_attr,
                                         false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }
    if (conf->modified_attr != NULL) {
      if ((ret = ofp_port_names_used_set(conf->modified_attr,
                                         false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }

    ret = port_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (conf->is_destroying == true ||
             state == DATASTORE_INTERP_STATE_AUTO_COMMIT) {
    /* unset is_used. */
    if (conf->current_attr != NULL) {
      if ((ret = ofp_port_names_used_set(conf->current_attr,
                                         false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }
    if (conf->modified_attr != NULL) {
      if ((ret = ofp_port_names_used_set(conf->modified_attr,
                                         false, result)) !=
          LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }

    if (conf->current_attr != NULL) {
      ret = ofp_port_destroy(conf->name, conf->current_attr, result);
      if (ret != LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    }

    ret = port_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  }
}

static inline lagopus_result_t
ofp_port_names_update(datastore_interp_t *iptr,
                      datastore_interp_state_t state,
                      port_conf_t *conf,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((ret = ofp_port_interface_update(iptr, state,
                                       conf, result)) ==
      LAGOPUS_RESULT_OK) {
    if ((ret = ofp_port_queue_update(iptr, state,
                                     conf, result)) ==
        LAGOPUS_RESULT_OK) {
      ret = ofp_port_policer_update(iptr, state,
                                    conf, result);
    }
  }

  return ret;
}

static inline lagopus_result_t
port_cmd_do_update(datastore_interp_t *iptr,
                   datastore_interp_state_t state,
                   port_conf_t *conf,
                   bool is_propagation,
                   bool is_enable_disable_cmd,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_modified = false;
  bool is_modified_without_names = false;
  struct names_info q_names_info;
  (void) iptr;

  /* create names info. */
  if ((ret = ofp_port_names_info_create(&q_names_info, result)) !=
      LAGOPUS_RESULT_OK) {
    goto done;

  }

  if ((ret = ofp_port_names_info_get(conf,
                                     &q_names_info,
                                     result)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  if (conf->modified_attr != NULL &&
      port_attr_equals(conf->current_attr,
                       conf->modified_attr) == false) {
    if (conf->modified_attr != NULL) {
      is_modified = true;

      if (conf->current_attr == NULL &&
          conf->modified_attr != NULL) {
        is_modified_without_names = true;
      } else if (conf->current_attr != NULL &&
                 port_attr_equals_without_names(
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
    /* update interface/policer. */
    if (is_propagation == true) {
      if ((ret = ofp_port_names_update(iptr, state, conf, result)) !=
          LAGOPUS_RESULT_OK) {
        goto done;
      }
    }

    if (is_modified == true) {
      /*
       * Update attrs.
       */
      if (conf->current_attr != NULL) {
        /* unset is_used for interface. */
        if ((ret = ofp_port_interface_used_set(conf->current_attr,
                                               false, result)) ==
            LAGOPUS_RESULT_OK) {
          /* disable interface/policer/queues. */
          ret = ofp_port_names_disable(conf->name,
                                       conf->current_attr,
                                       &q_names_info,
                                       iptr, state,
                                       is_propagation,
                                       true,
                                       result);
        }

        if (ret == LAGOPUS_RESULT_OK) {
          if (is_modified_without_names == true) {
            /* destroy port/policer/queues. */
            ret = ofp_port_destroy(conf->name, conf->current_attr, result);
            if (ret != LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("Can't delete port.\n");
            }
          } else {
            /* delete policer/queues. */
            ret = ofp_port_names_delete(conf->name,
                                        conf->current_attr,
                                        &q_names_info,
                                        result);
            if (ret != LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("Can't delete port policer/queues.\n");
            }
          }
        }
      } else {
        ret = LAGOPUS_RESULT_OK;
      }

      if (ret == LAGOPUS_RESULT_OK) {
        if (is_modified_without_names == true) {
          /* create port/policer/queues. */
          ret = ofp_port_create(conf->name, conf->modified_attr,
                                result);
        } else {
          /* add policer/queue. */
          ret = ofp_port_names_add(conf->name,
                                   conf->modified_attr,
                                   &q_names_info,
                                   result);
        }

        if (ret == LAGOPUS_RESULT_OK) {
          /* set is_used. */
          if ((ret = ofp_port_names_used_set(conf->modified_attr,
                                             true, result)) ==
              LAGOPUS_RESULT_OK) {
            if (conf->is_enabled == true) {
              /* start port. */
              ret = ofp_port_start(conf->name, conf->modified_attr,
                                   &q_names_info,
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
        port_cmd_update_current_attr(conf, state);
      }
    } else {
      if (is_enable_disable_cmd == true ||
          conf->is_enabling == true ||
          conf->is_disabling == true) {
        if (conf->is_enabled == true) {
          /* start port. */
          ret = ofp_port_start(conf->name, conf->current_attr,
                               &q_names_info,
                               iptr, state,
                               is_propagation, result);
        } else {
          /* stop port. */
          ret = ofp_port_stop(conf->name,
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
  ofp_port_names_info_destroy(&q_names_info);

  return ret;
}

static inline lagopus_result_t
port_cmd_update_internal(datastore_interp_t *iptr,
                         datastore_interp_state_t state,
                         port_conf_t *conf,
                         bool is_propagation,
                         bool is_enable_disable_cmd,
                         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int i;

  switch (state) {
    case DATASTORE_INTERP_STATE_AUTO_COMMIT: {
      i = 0;
      while (i < UPDATE_CNT_MAX) {
        ret = port_cmd_do_update(iptr, state, conf,
                                 is_propagation,
                                 is_enable_disable_cmd,
                                 result);
        if (ret == LAGOPUS_RESULT_OK ||
            is_enable_disable_cmd == true) {
          break;
        } else if (conf->current_attr == NULL &&
                   conf->modified_attr != NULL) {
          port_cmd_do_destroy(conf, state, result);
          break;
        } else {
          port_cmd_update_switch_attr(conf);
          lagopus_msg_warning("FAILED auto_comit (%s): rollbacking....\n",
                              lagopus_error_get_string(ret));
        }
        i++;
      }
      break;
    }
    case DATASTORE_INTERP_STATE_COMMITING: {
      port_cmd_is_enabled_set(conf);
      ret = port_cmd_do_update(iptr, state, conf,
                               is_propagation,
                               is_enable_disable_cmd,
                               result);
      break;
    }
    case DATASTORE_INTERP_STATE_ATOMIC: {
      /* set is_used. */
      if (conf->modified_attr != NULL) {
        if ((ret = ofp_port_names_used_set(conf->modified_attr,
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
      port_cmd_update_current_attr(conf, state);
      port_cmd_do_destroy(conf, state, result);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_ROLLBACKING: {
      if (conf->current_attr == NULL &&
          conf->modified_attr != NULL) {
        /* case create. */
        /* unset is_used. */
        if ((ret = ofp_port_names_used_set(conf->modified_attr,
                                           false, result)) !=
            LAGOPUS_RESULT_OK) {
          /* ignore error. */
          lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
        }
        ret = LAGOPUS_RESULT_OK;
      } else {
        port_cmd_update_switch_attr(conf);
        port_cmd_is_enabled_set(conf);
        ret = port_cmd_do_update(iptr, state, conf,
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
        if ((ret = ofp_port_names_used_set(conf->modified_attr,
                                           false, result)) !=
            LAGOPUS_RESULT_OK) {
          /* ignore error. */
          lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
        }
      }
      if (conf->current_attr != NULL) {
        /* unset is_used. */
        if ((ret = ofp_port_names_used_set(conf->current_attr,
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
      port_cmd_update_aborted(conf);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_DRYRUN: {
      if (conf->modified_attr != NULL) {
        if (conf->current_attr != NULL) {
          port_attr_destroy(conf->current_attr);
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
port_cmd_update(datastore_interp_t *iptr,
                datastore_interp_state_t state,
                void *o,
                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) iptr;
  (void) result;

  if (iptr != NULL && *iptr != NULL && o != NULL) {
    port_conf_t *conf = (port_conf_t *)o;
    ret = port_cmd_update_internal(iptr, state, conf, false, false, result);
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static lagopus_result_t
interface_opt_parse(const char *const *argv[],
                    void *c, void *out_configs,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  port_conf_t *conf = NULL;
  configs_t *configs = NULL;
  bool is_used = false;
  char *name = NULL;
  char *fullname = NULL;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (port_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = lagopus_str_unescape(*(*argv), "\"'", &name)) >= 0) {
          if ((ret = namespace_get_fullname(name, &fullname)) !=
              LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_string_setf(result,
                                                    ret,
                                                    "Can't get fullname %s.",
                                                    name);
            goto done;
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "interface name = %s.",
                                                  *(*argv));
          goto done;
        }

        /* check exists. */
        if (interface_exists(fullname) == false) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_NOT_FOUND,
                                                  "interface name = %s.",
                                                  fullname);
          goto done;
        }

        /* check is_used. */
        if ((ret = datastore_interface_is_used(fullname, &is_used)) ==
            LAGOPUS_RESULT_OK) {
          if (is_used == false) {
            /* unset is_used. */
            if (conf->current_attr != NULL) {
              if ((ret = ofp_port_interface_used_set(conf->current_attr,
                                                     false, result)) !=
                  LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(result, ret,
                                                        "interface name = "
                                                        "%s.", fullname);
                goto done;
              }
            }
            if (conf->modified_attr != NULL) {
              if ((ret = ofp_port_interface_used_set(conf->modified_attr,
                                                     false, result)) !=
                  LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(result, ret,
                                                        "interface name = "
                                                        "%s.", fullname);
                goto done;
              }
            }

            ret = port_set_interface_name(conf->modified_attr,
                                          fullname);
            if (ret != LAGOPUS_RESULT_OK) {
              ret = datastore_json_result_string_setf(result, ret,
                                                      "interface name = "
                                                      "%s.", fullname);
            }
          } else {
            ret = datastore_json_result_string_setf(
                result, LAGOPUS_RESULT_NOT_OPERATIONAL,
                "interface name = %s.", fullname);
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "interface name = %s.",
                                                  fullname);
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
      configs->flags = OPT_BIT_GET(OPT_INTERFACE);
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
policer_opt_parse(const char *const *argv[],
                  void *c, void *out_configs,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  port_conf_t *conf = NULL;
  configs_t *configs = NULL;
  char *name = NULL;
  char *name_str = NULL;
  char *fullname = NULL;
  char *exists_name = NULL;
  bool is_added = false;
  bool is_used = false;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (port_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = lagopus_str_unescape(*(*argv), "\"'", &name_str)) >= 0) {
          if ((ret = cmd_opt_name_get(name_str, &name, &is_added)) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = namespace_get_fullname(name, &fullname))
                == LAGOPUS_RESULT_OK) {
              ret = port_get_policer_name(conf->modified_attr,
                                          &exists_name);

              if (ret != LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(
                        result, ret,
                        "policer name = %s.", fullname);
                goto done;
              }
            } else {
              ret = datastore_json_result_string_setf(result,
                                                      ret,
                                                      "Can't get fullname %s.",
                                                      name);
              goto done;
            }
          } else {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't get policer_name.");
            goto done;
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "policer name = %s.",
                                                  *(*argv));
          goto done;
        }

        if (is_added == true) {
          /* add. */
          if (IS_VALID_STRING(exists_name) == true) {
            /* unset is_used. */
            ret = ofp_port_policer_used_set(conf->modified_attr,
                                            false,
                                            result);
            if (ret != LAGOPUS_RESULT_OK) {
              ret = datastore_json_result_string_setf(
                      result, ret,
                      "policer name = %s.", fullname);
              goto done;
            }
          }

          /* check exists. */
          if (policer_exists(fullname) == false) {
            ret = datastore_json_result_string_setf(
                    result,
                    LAGOPUS_RESULT_NOT_FOUND,
                    "policer name = %s.", fullname);
            goto done;
          }

          /* check is_used. */
          if ((ret =
               datastore_policer_is_used(fullname, &is_used)) ==
              LAGOPUS_RESULT_OK) {
            if (is_used == false) {
              ret = port_set_policer_name(
                  conf->modified_attr,
                  fullname);
              if (ret != LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(
                    result, ret,
                    "policer name = %s.", fullname);
              }
            } else {
              ret = datastore_json_result_string_setf(
                  result,
                  LAGOPUS_RESULT_NOT_OPERATIONAL,
                  "policer name = %s.", fullname);
            }
          } else {
            ret = datastore_json_result_string_setf(
                result, ret,
                "policer name = %s.", fullname);
          }
        } else {
          /* delete. */
          if (IS_VALID_STRING(exists_name) == false) {
            ret = datastore_json_result_string_setf(
                    result, LAGOPUS_RESULT_NOT_FOUND,
                    "policer name = %s.", fullname);
            goto done;
          }

          /* unset is_used. */
          ret = ofp_port_policer_used_set(conf->modified_attr,
                                          false,
                                          result);
          if (ret == LAGOPUS_RESULT_OK) {
            ret = port_set_policer_name(conf->modified_attr,
                                        "");
            if (ret != LAGOPUS_RESULT_OK) {
              ret = datastore_json_result_string_setf(
                  result, ret,
                  "policer name = %s.", fullname);
            }
          } else {
            ret = datastore_json_result_string_setf(
                result, ret,
                "policer name = %s.", fullname);
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
      configs->flags = OPT_BIT_GET(OPT_POLICER);
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
  free(exists_name);

  return ret;
}

static inline bool
queue_opt_id_is_exists(port_conf_t *conf,
                       uint32_t queue_id) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool rv = false;
  struct datastore_name_entry *q_name = NULL;
  datastore_name_info_t *q_c_names = NULL;
  datastore_name_info_t *q_m_names = NULL;

  if ((ret = port_get_queue_names(conf->current_attr,
                                  &q_c_names)) ==
      LAGOPUS_RESULT_OK) {
    if (TAILQ_EMPTY(&q_c_names->head) == false) {
      TAILQ_FOREACH(q_name, &q_c_names->head, name_entries) {
        ret = queue_cmd_queue_id_is_exists(q_name->str,
                                           queue_id,
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
  if ((ret = port_get_queue_names(conf->modified_attr,
                                  &q_m_names)) ==
      LAGOPUS_RESULT_OK) {
    if (TAILQ_EMPTY(&q_m_names->head) == false) {
      TAILQ_FOREACH(q_name, &q_m_names->head, name_entries) {
        ret = queue_cmd_queue_id_is_exists(q_name->str,
                                           queue_id,
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
  if (q_c_names != NULL) {
    datastore_names_destroy(q_c_names);
  }
  if (q_m_names != NULL) {
    datastore_names_destroy(q_m_names);
  }

  return rv;
}

static inline lagopus_result_t
generate_queue_cmd(char *name,
                   uint32_t queue_id,
                   char **cmd_str) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_dstring_t ds = NULL;

  if ((ret = lagopus_dstring_create(&ds)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  if ((ret = lagopus_dstring_appendf(&ds, "queue %s config -id %"PRIu32,
                                     name, queue_id)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  if ((ret = lagopus_dstring_str_get(&ds, cmd_str)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

done:
  lagopus_dstring_destroy(&ds);

  return ret;
}

static lagopus_result_t
queue_opt_parse(const char *const *argv[],
                void *c, void *out_configs,
                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  port_conf_t *conf = NULL;
  configs_t *configs = NULL;
  char *name = NULL;
  char *name_str = NULL;
  char *fullname = NULL;
  char *queue_cmd = NULL;
  bool is_added = false;
  bool is_exists = false;
  bool is_used = false;
  union cmd_uint cmd_uint;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (port_conf_t *) c;
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
                port_attr_queue_name_exists(conf->modified_attr,
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
                                                    "Can't get queue_name.");
            goto done;
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "queue name = %s.",
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
                "queue name = %s.", fullname);
            goto done;
          }

          if (queue_exists(fullname) == false) {
            ret = datastore_json_result_string_setf(
                result,
                LAGOPUS_RESULT_NOT_FOUND,
                "queue name = %s.", fullname);
            goto done;
          }

          /* parse queue id. */
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

          /* check exists id. */
          if (queue_opt_id_is_exists(conf,
                                     cmd_uint.uint32) ==
              true) {
            ret = datastore_json_result_string_setf(
                result, LAGOPUS_RESULT_ALREADY_EXISTS,
                "queue name = %s, queue id = %"PRIu32".",
                fullname, cmd_uint.uint32);
            goto done;
          }

          /* check is_used. */
          if ((ret =
               datastore_queue_is_used(fullname, &is_used)) ==
              LAGOPUS_RESULT_OK) {
            if (is_used == false) {
              ret = port_attr_add_queue_name(
                  conf->modified_attr,
                  fullname);
              if (ret != LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(
                    result, ret,
                    "queue name = %s.", fullname);
              }


              /* generate queue cmd. */
              if ((ret = generate_queue_cmd(fullname, cmd_uint.uint32,
                                            &queue_cmd)) !=
                  LAGOPUS_RESULT_OK) {
                goto done;
              }

              if ((ret = datastore_interp_eval_cmd(configs->iptr,
                                                   queue_cmd, result)) ==
                  LAGOPUS_RESULT_OK) {
                /* clear result. Delete "OK" string.*/
                (void) lagopus_dstring_clear(result);
              } else {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              ret = datastore_json_result_string_setf(
                  result,
                  LAGOPUS_RESULT_NOT_OPERATIONAL,
                  "queue name = %s.", fullname);
            }
          } else {
            ret = datastore_json_result_string_setf(
                result, ret,
                "queue name = %s.", fullname);
          }
        } else {
          /* delete. */
          if (is_exists == false) {
            ret = datastore_json_result_string_setf(
                result, LAGOPUS_RESULT_NOT_FOUND,
                "queue name = %s.", fullname);
            goto done;
          }

          if (*(*argv + 1) != NULL) {
            if (IS_VALID_OPT(*(*argv + 1)) == false) {
              /* argv + 1 equals option string(-XXX). */
              ret = datastore_json_result_string_setf(
                  result, LAGOPUS_RESULT_INVALID_ARGS,
                  "Bad opt value = %s. "
                  "Do not specify the queue id.",
                  *(*argv + 1));
              goto done;
            }
          }

          /* reset queue id. */
          ret = port_attr_remove_queue_name(conf->modified_attr,
                                            fullname);
          if (ret == LAGOPUS_RESULT_OK) {
            /* unset is_used. */
            ret = ofp_port_queue_used_set_internal(fullname,
                                                   false,
                                                   result);
            if (ret != LAGOPUS_RESULT_OK) {
              lagopus_perror(ret);
              goto done;
            }
          } else {
            ret = datastore_json_result_string_setf(
                result, ret,
                "queue name = %s.", fullname);
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
      configs->flags = OPT_BIT_GET(OPT_QUEUES);
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
  free(queue_cmd);

  return ret;
}

static inline lagopus_result_t
opt_parse(const char *const argv[],
          port_conf_t *conf,
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
  port_conf_t *conf = NULL;
  (void) argc;
  (void) proc;

  ret = port_conf_create(&conf, name);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = port_conf_add(conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = opt_parse(argv, conf, out_configs, result);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = port_cmd_update_internal(iptr, state, conf,
                                       true, false, result);

        if (ret != LAGOPUS_RESULT_OK &&
            ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_set(result, ret, NULL);
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add port_conf.");
    }

    if (ret != LAGOPUS_RESULT_OK) {
      (void) port_conf_delete(conf);
      goto done;
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't create port_conf.");
  }

  if (ret != LAGOPUS_RESULT_OK && conf != NULL) {
    port_conf_destroy(conf);
  }

done:
  return ret;
}

static lagopus_result_t
port_attr_dup_modified(port_conf_t *conf,
                       lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (conf->modified_attr == NULL) {
    if (conf->current_attr != NULL) {
      /*
       * already exists. copy it.
       */
      ret = port_attr_duplicate(conf->current_attr,
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
                              port_conf_t *conf,
                              datastore_update_proc_t proc,
                              configs_t *out_configs,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) proc;
  (void) argc;

  ret = port_attr_dup_modified(conf, result);

  if (ret == LAGOPUS_RESULT_OK) {
    conf->is_destroying = false;
    out_configs->is_config = true;
    ret = opt_parse(argv, conf, out_configs, result);

    if (ret == LAGOPUS_RESULT_OK) {
      if (out_configs->flags == 0) {
        /* update. */
        ret = port_cmd_update_internal(iptr, state, conf,
                                       true, false, result);

        if (ret != LAGOPUS_RESULT_OK &&
            ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_set(result, ret, NULL);
        }
      } else {
        /* show. */
        ret = port_conf_one_list(&out_configs->list, conf);

        if (ret >= 0) {
          out_configs->size = (size_t) ret;
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = datastore_json_result_string_setf(
                  result, ret,
                  "Can't create list of port_conf.");
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
  port_conf_t *conf = NULL;
  char *namespace = NULL;
  (void) hptr;

  if (name != NULL) {
    ret = port_find(name, &conf);

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
  port_conf_t *conf = NULL;
  (void) hptr;
  (void) proc;
  (void) argc;

  if (name != NULL) {
    ret = port_find(name, &conf);

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
                              port_conf_t *conf,
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
          ret = port_cmd_update_internal(iptr, state, conf,
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
  port_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = port_find(name, &conf);

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
                               port_conf_t *conf,
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
      ret = port_cmd_update_internal(iptr, state, conf,
                                     false, true, result);

      if (ret == LAGOPUS_RESULT_OK) {
        /* disable propagation. */
        if (is_propagation == true) {
          ret = ofp_port_names_disable(conf->name, conf->current_attr,
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
  port_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = port_find(name, &conf);

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
                               port_conf_t *conf,
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
        ret = ofp_port_names_disable(conf->name, conf->current_attr,
                                     NULL, iptr, state,
                                     true, true, result);
        if (ret != LAGOPUS_RESULT_OK) {
          /* ignore error. */
          lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
        }

        if (conf->is_enabled == true) {
          conf->is_enabled = false;
          ret = port_cmd_update_internal(iptr, state, conf,
                                         false, true, result);

          if (ret != LAGOPUS_RESULT_OK) {
            conf->is_enabled = true;
            if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
              ret = datastore_json_result_set(result, ret, NULL);
            }
            goto done;
          }
        }

        port_cmd_do_destroy(conf, state, result);
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
  port_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = port_find(name, &conf);

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
  port_conf_t *conf = NULL;
  configs_t *configs = NULL;
  (void) iptr;
  (void) state;
  (void) argc;
  (void) hptr;
  (void) proc;

  if (argv != NULL && name != NULL &&
      out_configs != NULL && result != NULL) {
    configs = (configs_t *) out_configs;
    ret = port_find(name, &conf);
    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      if (*(argv + 1) == NULL) {
        configs->is_show_stats = true;
        ret = dp_port_stats_get(conf->name, &configs->stats);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = port_conf_one_list(&configs->list, conf);

          if (ret >= 0) {
            configs->size = (size_t) ret;
            ret = LAGOPUS_RESULT_OK;
          } else {
            ret = datastore_json_result_string_setf(
                    result, ret,
                    "Can't create list of port_conf.");
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



static inline lagopus_result_t
show_parse(const char *name,
           configs_t *out_configs,
           bool is_show_modified,
           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  port_conf_t *conf = NULL;
  char *target = NULL;

  if (name == NULL) {
    ret = namespace_get_current_namespace(&target);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = port_conf_list(&out_configs->list, target);
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
      ret = port_conf_list(&out_configs->list, "");
    } else if (ret == NS_NAMESPACE) {
      /* namespace + delim */
      ret = port_conf_list(&out_configs->list, target);
    } else if (ret == NS_FULLNAME) {
      /* namespace + name or delim + name */
      ret = port_find(target, &conf);

      if (ret == LAGOPUS_RESULT_OK) {
        if (conf->is_destroying == false) {
          ret = port_conf_one_list(&out_configs->list, conf);
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
            "Can't create list of port_conf.");
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
queue_names_show(lagopus_dstring_t *ds,
                 const char *key,
                 datastore_name_info_t *names,
                 bool delimiter,
                 bool is_show_current) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct datastore_name_entry *name = NULL;
  char *name_str = NULL;
  uint32_t id;
  bool is_first = true;

  if (key != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"{", key);
    if (ret == LAGOPUS_RESULT_OK) {
      TAILQ_FOREACH(name, &names->head, name_entries) {
        if (((ret = datastore_queue_get_id(name->str, is_show_current,
                                           &id)) ==
             LAGOPUS_RESULT_INVALID_OBJECT) &&
            is_show_current == false) {
          ret = datastore_queue_get_id(name->str, true, &id);
        }

        if (ret == LAGOPUS_RESULT_OK) {
          ret = datastore_json_string_escape(name->str, &name_str);
          if (ret == LAGOPUS_RESULT_OK) {
            ret = datastore_json_uint32_append(ds, name_str,
                                               id, !(is_first));
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

static lagopus_result_t
port_cmd_json_create(lagopus_dstring_t *ds,
                     configs_t *configs,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_name_info_t *queue_names = NULL;
  char *interface_name = NULL;
  char *policer_name = NULL;
  port_attr_t *attr = NULL;
  bool is_show_current = false;
  uint32_t port_no;
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

          /* port no */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_PORT_NO)) == true) {
            if ((ret = port_get_port_number(attr, &port_no)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint32_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_PORT_NO),
                           port_no, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* interface_name */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_INTERFACE)) ==true) {
            if ((ret = port_get_interface_name(attr,
                                               &interface_name)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_string_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_INTERFACE),
                           interface_name, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* policer_name */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_POLICER)) ==true) {
            if ((ret = port_get_policer_name(attr,
                                             &policer_name)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_string_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_POLICER),
                           policer_name, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* queue names */
          if (IS_BIT_SET(configs->flags,
                         OPT_BIT_GET(OPT_QUEUES)) == true) {
            if ((ret = port_get_queue_names(attr,
                                            &queue_names)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = queue_names_show(ds,
                                          ATTR_NAME_GET(opt_strs,
                                                        OPT_QUEUES),
                                          queue_names, true,
                                          is_show_current)) !=
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
      if (queue_names != NULL) {
        datastore_names_destroy(queue_names);
        queue_names = NULL;
      }
      free(interface_name);
      interface_name = NULL;
      free(policer_name);
      policer_name = NULL;

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
port_cmd_stats_json_create(lagopus_dstring_t *ds,
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

        /* config */
        if ((ret = datastore_json_uint32_flags_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_CONFIG),
                     configs->stats.config,
                     config_strs, config_strs_size, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* state */
        if ((ret = datastore_json_uint32_flags_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_STATE),
                     configs->stats.state,
                     state_strs, state_strs_size, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* curr_features */
        if ((ret = datastore_json_uint32_flags_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_CURR_FEATURES),
                     configs->stats.curr_features,
                     feature_strs, feature_strs_size, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* supported_features */
        if ((ret = datastore_json_uint32_flags_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_SUPPORTED_FEATURES),
                     configs->stats.supported_features,
                     feature_strs, feature_strs_size, true)) !=
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
port_cmd_parse(datastore_interp_t *iptr,
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
    {0LL, 0LL, 0LL, 0LL},
    NULL, NULL,
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
      out_configs.iptr = iptr;
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
            ret = port_cmd_stats_json_create(&conf_result, &out_configs,
                                             result);
          } else {
            ret = port_cmd_json_create(&conf_result, &out_configs,
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
port_cmd_enable(datastore_interp_t *iptr,
                datastore_interp_state_t state,
                void *obj,
                bool *currentp,
                bool enabled,
                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (currentp == NULL) {
    if (enabled == true) {
      ret = enable_sub_cmd_parse_internal(iptr, state,
                                          (port_conf_t *) obj,
                                          false,
                                          result);
    } else {
      ret = disable_sub_cmd_parse_internal(iptr, state,
                                           (port_conf_t *) obj,
                                           false,
                                           result);
    }
  } else {
    if (obj != NULL) {
      *currentp = ((port_conf_t *) obj)->is_enabled;
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
port_cmd_destroy(datastore_interp_t *iptr,
                 datastore_interp_state_t state,
                 void *obj,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = destroy_sub_cmd_parse_internal(iptr, state,
                                       (port_conf_t *) obj,
                                       result);

  return ret;
}

STATIC lagopus_result_t
port_cmd_serialize(datastore_interp_t *iptr,
                   datastore_interp_state_t state,
                   const void *obj,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_name_info_t *queue_names = NULL;
  struct datastore_name_head *q_head = NULL;
  struct datastore_name_entry *q_entry = NULL;
  char *escaped_q_names_str = NULL;
  char *escaped_name = NULL;
  char *interface_name = NULL;
  char *escaped_interface_name = NULL;
  char *policer_name = NULL;
  char *escaped_policer_name = NULL;
  port_conf_t *conf = NULL;
  bool is_escaped = false;
  uint32_t queue_id;
  (void) state;

  if (iptr != NULL && obj != NULL && result != NULL) {
    conf = (port_conf_t *) obj;

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

    /* interface opt. */
    if ((ret = port_get_interface_name(conf->current_attr,
                                       &interface_name)) ==
        LAGOPUS_RESULT_OK) {
      if (IS_VALID_STRING(interface_name) == true) {
        if ((ret = lagopus_str_escape(interface_name, "\"",
                                      &is_escaped,
                                      &escaped_interface_name)) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %s",
                                             opt_strs[OPT_INTERFACE])) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(
                         result,
                         ESCAPE_NAME_FMT(is_escaped, escaped_interface_name),
                         escaped_interface_name)) !=
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

    /* policer opt. */
    if ((ret = port_get_policer_name(conf->current_attr,
                                     &policer_name)) ==
        LAGOPUS_RESULT_OK) {
      if (IS_VALID_STRING(policer_name) == true) {
        if ((ret = lagopus_str_escape(policer_name, "\"",
                                      &is_escaped,
                                      &escaped_policer_name)) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %s",
                                             opt_strs[OPT_POLICER])) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(
                         result,
                         ESCAPE_NAME_FMT(is_escaped, escaped_policer_name),
                         escaped_policer_name)) !=
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

    /* queue names opt. */
    if ((ret = port_get_queue_names(conf->current_attr,
                                    &queue_names)) ==
        LAGOPUS_RESULT_OK) {
      q_head = &queue_names->head;
      if (TAILQ_EMPTY(q_head) == false) {
        TAILQ_FOREACH(q_entry, q_head, name_entries) {
          if ((ret = lagopus_dstring_appendf(result, " %s",
                                             opt_strs[OPT_QUEUE])) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_str_escape(q_entry->str, "\"",
                                          &is_escaped,
                                          &escaped_q_names_str)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(
                           result,
                           ESCAPE_NAME_FMT(is_escaped, escaped_q_names_str),
                           escaped_q_names_str)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              if ((ret = datastore_queue_get_id(q_entry->str, true,
                                                &queue_id)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }

              if ((ret = lagopus_dstring_appendf(result, " %"PRIu32,
                                                 queue_id)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }

            free((void *) escaped_q_names_str);
            escaped_q_names_str = NULL;
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
    datastore_names_destroy(queue_names);
    free((void *) escaped_q_names_str);
    free(escaped_name);
    free(interface_name);
    free(escaped_interface_name);
    free(policer_name);
    free(escaped_policer_name);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
port_cmd_compare(const void *obj1, const void *obj2, int *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj1 != NULL && obj2 != NULL && result != NULL) {
    *result = strcmp(((port_conf_t *) obj1)->name,
                     ((port_conf_t *) obj2)->name);
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
port_cmd_getname(const void *obj, const char **namep) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj != NULL && namep != NULL) {
    *namep = ((port_conf_t *)obj)->name;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
port_cmd_duplicate(const void *obj, const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  port_conf_t *dup_obj = NULL;

  if (obj != NULL) {
    ret = port_conf_duplicate(obj, &dup_obj, namespace);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = port_conf_add(dup_obj);

      if (ret != LAGOPUS_RESULT_OK && dup_obj != NULL) {
        port_conf_destroy(dup_obj);
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

  if ((ret = port_initialize()) != LAGOPUS_RESULT_OK) {
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

  if (((ret = opt_add(opt_strs[OPT_INTERFACE], interface_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_QUEUE], queue_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_POLICER], policer_opt_parse,
                      &opt_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  if ((ret = datastore_register_table(CMD_NAME,
                                      &port_table,
                                      port_cmd_update,
                                      port_cmd_enable,
                                      port_cmd_serialize,
                                      port_cmd_destroy,
                                      port_cmd_compare,
                                      port_cmd_getname,
                                      port_cmd_duplicate)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = datastore_interp_register_command(&s_interp, CONFIGURATOR_NAME,
             CMD_NAME, port_cmd_parse)) !=
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
port_cmd_enable_propagation(datastore_interp_t *iptr,
                            datastore_interp_state_t state,
                            char *name,
                            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = enable_sub_cmd_parse(iptr, state, 0, NULL,
                               name, NULL, port_cmd_update,
                               NULL, result);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

lagopus_result_t
port_cmd_disable_propagation(datastore_interp_t *iptr,
                             datastore_interp_state_t state,
                             char *name,
                             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  port_conf_t *conf = NULL;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = port_find(name, &conf);

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
port_cmd_update_propagation(datastore_interp_t *iptr,
                            datastore_interp_state_t state,
                            char *name,
                            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  port_conf_t *conf = NULL;

  if (name != NULL) {
    ret = port_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = port_cmd_update(iptr, state,
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
port_cmd_set_port_number(char *name,
                         const uint32_t port_number,
                         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  port_conf_t *conf = NULL;

  if (name != NULL && result != NULL) {
    ret = port_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK) {
      if (port_number == 0) {
        if (conf->current_attr != NULL) {
          ret = port_set_port_number(conf->current_attr,
                                     port_number);
          if (ret != LAGOPUS_RESULT_OK) {
            goto done;
          }
        }
        if (conf->modified_attr != NULL) {
          ret = port_set_port_number(conf->modified_attr,
                                     port_number);
          if (ret != LAGOPUS_RESULT_OK) {
            goto done;
          }
        }
      } else {
        ret = port_attr_dup_modified(conf, result);

        if (ret == LAGOPUS_RESULT_OK) {
          ret = port_set_port_number(conf->modified_attr,
                                     port_number);
        }
      }

    done:
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't add port_number.");
      }
    } else if (ret != LAGOPUS_RESULT_NOT_FOUND) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "port name = %s.", name);
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
port_cmd_port_number_is_exists(char *name,
                               const uint32_t port_number,
                               bool *b) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  port_conf_t *conf = NULL;
  uint32_t port_no;

  if (name != NULL) {
    ret = port_find(name, &conf);

    *b = false;
    if (ret == LAGOPUS_RESULT_OK) {
      if (conf->current_attr != NULL) {
        if ((ret = port_get_port_number(conf->current_attr, &port_no)) ==
            LAGOPUS_RESULT_OK) {
          if (port_no == port_number) {
            *b = true;
            goto done;
          }
        }
      }
      if (conf->modified_attr != NULL) {
        if ((ret = port_get_port_number(conf->modified_attr, &port_no)) ==
            LAGOPUS_RESULT_OK) {
          if (port_no == port_number) {
            *b = true;
            goto done;
          }
        }
      }
    } else if (ret == LAGOPUS_RESULT_NOT_FOUND) {
      /* ignore LAGOPUS_RESULT_NOT_FOUND. */
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}

lagopus_result_t
port_cmd_initialize(void) {
  return initialize_internal();
}

void
port_cmd_finalize(void) {
  lagopus_hashmap_destroy(&sub_cmd_table, true);
  sub_cmd_table = NULL;
  lagopus_hashmap_destroy(&opt_table, true);
  opt_table = NULL;
  port_finalize();
}
