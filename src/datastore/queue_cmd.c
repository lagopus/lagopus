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
#include "queue_cmd.h"
#include "queue_cmd_internal.h"
#include "queue.c"
#include "conv_json.h"
#include "lagopus/dp_apis.h"

/* command name. */
#define CMD_NAME "queue"

/* option num. */
enum queue_opts {
  OPT_NAME = 0,
  OPT_TYPE,
  OPT_ID,
  OPT_PRIORITY,
  OPT_COLOR,
  OPT_COMMITTED_BURST_SIZE,
  OPT_COMMITTED_INFORMATION_RATE,
  OPT_EXCESS_BURST_SIZE,
  OPT_PEAK_BURST_SIZE,
  OPT_PEAK_INFORMATION_RATE,
  OPT_IS_USED,
  OPT_IS_ENABLED,

  OPT_MAX,
};

/* stats num. */
enum q_stats {
  STATS_PORT_NO = 0,
  STATS_QUEUE_ID,
  STATS_TX_BYTES,
  STATS_TX_PACKETS,
  STATS_TX_ERRORS,
  STATS_DURATION_SEC,
  STATS_DURATION_NSEC,

  STATS_MAX,
};

/* option flags. */
#define OPT_COMMON   (OPT_BIT_GET(OPT_NAME) |                       \
                      OPT_BIT_GET(OPT_TYPE) |                       \
                      OPT_BIT_GET(OPT_ID) |                         \
                      OPT_BIT_GET(OPT_PRIORITY) |                   \
                      OPT_BIT_GET(OPT_COLOR) |                      \
                      OPT_BIT_GET(OPT_COMMITTED_BURST_SIZE) |       \
                      OPT_BIT_GET(OPT_COMMITTED_INFORMATION_RATE) | \
                      OPT_BIT_GET(OPT_IS_USED) |                    \
                      OPT_BIT_GET(OPT_IS_ENABLED))
#define OPT_SINGLE_RATE (OPT_COMMON |                           \
                         OPT_BIT_GET(OPT_EXCESS_BURST_SIZE))
#define OPT_TWO_RATE (OPT_COMMON |                              \
                      OPT_BIT_GET(OPT_PEAK_BURST_SIZE) |        \
                      OPT_BIT_GET(OPT_PEAK_INFORMATION_RATE))
#define OPT_UNKNOWN  (OPT_COMMON)

/* option name. */
static const char *const opt_strs[OPT_MAX] = {
  "*name",                       /* OPT_NAME (not option) */
  "-type",                       /* OPT_TYPE */
  "-id",                         /* OPT_ID */
  "-priority",                   /* OPT_PRIORITY */
  "-color",                      /* OPT_COLOR */
  "-committed-burst-size",       /* OPT_COMMITTED_BURST_SIZE */
  "-committed-information-rate", /* OPT_COMMITTED_INFORMATION_RATE */
  "-excess-burst-size",          /* OPT_EXCESS_BURST_SIZE */
  "-peak-burst-size",            /* OPT_PEAK_BURST_SIZE */
  "-peak-information-rate",      /* OPT_PEAK_INFORMATION_RATE */
  "*is-used",                    /* OPT_IS_USED (not option) */
  "*is-enabled",                 /* OPT_IS_ENABLED (not option) */
};

/* stats name. */
static const char *const stat_strs[STATS_MAX] = {
  "*port-no",            /* STATS_PORT_NO (not option) */
  "*queue-id",           /* STATS_QUEUE_ID (not option) */
  "*tx-bytes",           /* STATS_TX_BYTES (not option) */
  "*tx-packets",         /* STATS_TX_PACKETS (not option) */
  "*tx-errors",          /* STATS_TX_ERRORS (not option) */
  "*duration-sec",       /* STATS_DURATION_SEC (not option) */
  "*duration-nsec",      /* STATS_DURATION_NSEC(not option) */
};

typedef struct configs {
  size_t size;
  uint64_t flags;
  bool is_config;
  bool is_show_modified;
  bool is_show_stats;
  datastore_queue_stats_t stats;
  queue_conf_t **list;
  datastore_interp_t *iptr;
} configs_t;

typedef lagopus_result_t
(*uint64_set_proc_t)(queue_attr_t *attr, const uint64_t num);
typedef lagopus_result_t
(*uint32_set_proc_t)(queue_attr_t *attr, const uint32_t num);
typedef lagopus_result_t
(*uint16_set_proc_t)(queue_attr_t *attr, const uint16_t num);
typedef lagopus_result_t
(*uint8_set_proc_t)(queue_attr_t *attr, const uint8_t num);

static lagopus_hashmap_t sub_cmd_table = NULL;
static lagopus_hashmap_t single_rate_opt_table = NULL;
static lagopus_hashmap_t two_rate_opt_table = NULL;

static inline lagopus_result_t
queue_create(const char *name,
             queue_attr_t *attr,
             bool is_eval_cmd,
             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_queue_type_t type;
  datastore_queue_color_t color;
  uint32_t id;
  uint16_t priority;
  uint64_t committed_burst_size;
  uint64_t committed_information_rate;
  uint64_t excess_burst_size;
  uint64_t peak_burst_size;
  uint64_t peak_information_rate;
  datastore_queue_info_t info;
  (void) info;

  if (((ret = queue_get_type(attr, &type)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = queue_get_color(attr, &color)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = queue_get_id(attr, &id)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = queue_get_priority(attr, &priority)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = queue_get_committed_burst_size(attr, &committed_burst_size)) ==
       LAGOPUS_RESULT_OK) &&
      ((ret = queue_get_committed_information_rate(
                attr, &committed_information_rate)) ==
       LAGOPUS_RESULT_OK)) {
    info.type = type;
    info.color = color;
    info.id = id;
    info.priority = priority;
    info.committed_burst_size = committed_burst_size;
    info.committed_information_rate = committed_information_rate;

    switch (type) {
      case DATASTORE_QUEUE_TYPE_SINGLE_RATE:
        if (((ret = queue_get_excess_burst_size(attr, &excess_burst_size)) ==
             LAGOPUS_RESULT_OK)) {
          info.excess_burst_size = excess_burst_size;
        }
        break;
      case DATASTORE_QUEUE_TYPE_TWO_RATE:
        if (((ret = queue_get_peak_burst_size(attr, &peak_burst_size)) ==
             LAGOPUS_RESULT_OK) &&
            ((ret = queue_get_peak_information_rate(attr, &peak_information_rate)) ==
             LAGOPUS_RESULT_OK)) {
          info.peak_burst_size = peak_burst_size;
          info.peak_information_rate = peak_information_rate;
        }
        break;
      default:
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_INVALID_ARGS,
                                                "Bad required options(-type).");
        goto done;
        break;
    }

    lagopus_msg_info("create queue. name = %s.\n", name);
    ret = dp_queue_create(name, &info);
    if (ret == LAGOPUS_RESULT_OK) {
      if (is_eval_cmd == true) {
        ret = dp_queue_id_set(name, info.id);

        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't set queue id.");
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't create queue.");
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't create queue.");
  }

done:
  return ret;
}

static inline void
queue_destroy(const char *name) {
  lagopus_msg_info("destroy queue. name = %s.\n", name);
  dp_queue_destroy(name);
}

static inline lagopus_result_t
queue_start(const char *name,
            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_info("start queue. name = %s.\n", name);
  ret = dp_queue_start(name);
  if (ret != LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't start queue.");
  }

  return ret;
}

static inline lagopus_result_t
queue_stop(const char *name,
           lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  lagopus_msg_info("stop queue. name = %s.\n", name);
  ret = dp_queue_stop(name);
  if (ret != LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't stop queue.");
  }

  return ret;
}

static inline void
queue_cmd_update_current_attr(queue_conf_t *conf,
                              datastore_interp_state_t state) {
  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    return;
  }

  if (conf->modified_attr != NULL) {
    if (conf->current_attr != NULL) {
      queue_attr_destroy(conf->current_attr);
    }
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = NULL;
  }
}

static inline void
queue_cmd_update_aborted(queue_conf_t *conf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (conf->modified_attr != NULL) {
    if (conf->current_attr == NULL) {
      /* create. */
      ret = queue_conf_delete(conf);
      if (ret != LAGOPUS_RESULT_OK) {
        /* ignore error. */
        lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
      }
    } else {
      queue_attr_destroy(conf->modified_attr);
      conf->modified_attr = NULL;
    }
  }
}

static inline void
queue_cmd_update_switch_attr(queue_conf_t *conf) {
  queue_attr_t *attr;

  if (conf->modified_attr != NULL) {
    attr = conf->current_attr;
    conf->current_attr = conf->modified_attr;
    conf->modified_attr = attr;
  }
}

static inline void
queue_cmd_is_enabled_set(queue_conf_t *conf) {
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
queue_cmd_do_destroy(queue_conf_t *conf,
                     datastore_interp_state_t state) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (state == DATASTORE_INTERP_STATE_ROLLBACKED &&
      conf->current_attr == NULL &&
      conf->modified_attr != NULL) {
    /* case rollbacked & create. */
    ret = queue_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (state == DATASTORE_INTERP_STATE_DRYRUN) {
    ret = queue_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  } else if (conf->is_destroying == true ||
             state == DATASTORE_INTERP_STATE_AUTO_COMMIT) {
    queue_destroy(conf->name);

    ret = queue_conf_delete(conf);
    if (ret != LAGOPUS_RESULT_OK) {
      /* ignore error. */
      lagopus_msg_warning("ret = %s", lagopus_error_get_string(ret));
    }
  }
}

static inline lagopus_result_t
queue_cmd_do_update(datastore_interp_t *iptr,
                    datastore_interp_state_t state,
                    queue_conf_t *conf,
                    bool is_propagation,
                    bool is_enable_disable_cmd,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_modified = false;
  (void) iptr;
  (void) is_propagation;

  if (conf->modified_attr != NULL &&
      queue_attr_equals(conf->current_attr,
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
        /* destroy queue. */
        queue_destroy(conf->name);
      }

      /* create queue. */
      ret = queue_create(conf->name,
                         conf->modified_attr,
                         datastore_interp_is_eval_cmd(iptr),
                         result);
      if (ret == LAGOPUS_RESULT_OK) {
        if (conf->is_enabled == true) {
          /* start queue. */
          ret = queue_start(conf->name, result);
        }
      }

      /* Update attr. */
      if (ret == LAGOPUS_RESULT_OK &&
          state != DATASTORE_INTERP_STATE_COMMITING &&
          state != DATASTORE_INTERP_STATE_ROLLBACKING) {
        queue_cmd_update_current_attr(conf, state);
      }
    } else {
      if (is_enable_disable_cmd == true ||
          conf->is_enabling == true ||
          conf->is_disabling == true) {
        if (conf->is_enabled == true) {
          /* start queue. */
          ret = queue_start(conf->name, result);
        } else {
          /* stop queue. */
          ret = queue_stop(conf->name, result);
        }
      }
      conf->is_enabling = false;
      conf->is_disabling = false;
    }
  }

  return ret;
}

static inline lagopus_result_t
queue_cmd_update_internal(datastore_interp_t *iptr,
                          datastore_interp_state_t state,
                          queue_conf_t *conf,
                          bool is_propagation,
                          bool is_enable_disable_cmd,
                          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int i;

  switch (state) {
    case DATASTORE_INTERP_STATE_AUTO_COMMIT: {
      i = 0;
      while (i < UPDATE_CNT_MAX) {
        ret = queue_cmd_do_update(iptr, state, conf,
                                  is_propagation,
                                  is_enable_disable_cmd,
                                  result);
        if (ret == LAGOPUS_RESULT_OK ||
            is_enable_disable_cmd == true) {
          break;
        } else if (conf->current_attr == NULL &&
                   conf->modified_attr != NULL) {
          queue_cmd_do_destroy(conf, state);
          break;
        } else {
          queue_cmd_update_switch_attr(conf);
          lagopus_msg_warning("FAILED auto_comit (%s): rollbacking....\n",
                              lagopus_error_get_string(ret));
        }
        i++;
      }
      break;
    }
    case DATASTORE_INTERP_STATE_COMMITING: {
      queue_cmd_is_enabled_set(conf);
      ret = queue_cmd_do_update(iptr, state, conf,
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
      queue_cmd_update_current_attr(conf, state);
      queue_cmd_do_destroy(conf, state);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_ROLLBACKING: {
      if (conf->current_attr == NULL &&
          conf->modified_attr != NULL) {
        /* case create. */
        ret = LAGOPUS_RESULT_OK;
      } else {
        queue_cmd_update_switch_attr(conf);
        queue_cmd_is_enabled_set(conf);
        ret = queue_cmd_do_update(iptr, state, conf,
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
      queue_cmd_update_aborted(conf);
      ret = LAGOPUS_RESULT_OK;
      break;
    }
    case DATASTORE_INTERP_STATE_DRYRUN: {
      if (conf->modified_attr != NULL) {
        if (conf->current_attr != NULL) {
          queue_attr_destroy(conf->current_attr);
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
queue_cmd_update(datastore_interp_t *iptr,
                 datastore_interp_state_t state,
                 void *o,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) iptr;
  (void) result;

  if (iptr != NULL && *iptr != NULL && o != NULL) {
    queue_conf_t *conf = (queue_conf_t *)o;
    ret = queue_cmd_update_internal(iptr, state, conf, false, false, result);
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static lagopus_result_t
uint_opt_parse(const char *const *argv[],
               queue_conf_t *conf,
               configs_t *configs,
               void *uint_set_proc,
               enum queue_opts opt,
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
id_opt_parse(const char *const *argv[],
             void *c, void *out_configs,
             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  configs_t *configs = (configs_t *) out_configs;

  if (datastore_interp_is_eval_cmd(configs->iptr) == true) {
    ret = uint_opt_parse(argv, (queue_conf_t *) c,
                        (configs_t *) out_configs,
                        queue_set_id,
                        OPT_ID, CMD_UINT32,
                        result);
  } else {
    ret = datastore_json_result_string_setf(result, LAGOPUS_RESULT_INVALID_ARGS,
                                            "Bad opt = %s.",
                                            *(*argv));
  }

  return ret;
}

static lagopus_result_t
priority_opt_parse(const char *const *argv[],
                   void *c, void *out_configs,
                   lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (queue_conf_t *) c,
                        (configs_t *) out_configs,
                        queue_set_priority,
                        OPT_PRIORITY, CMD_UINT16,
                        result);
}

static lagopus_result_t
committed_burst_size_opt_parse(const char *const *argv[],
                               void *c, void *out_configs,
                               lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (queue_conf_t *) c,
                        (configs_t *) out_configs,
                        queue_set_committed_burst_size,
                        OPT_COMMITTED_BURST_SIZE,
                        CMD_UINT64,
                        result);
}

static lagopus_result_t
committed_information_rate_opt_parse(const char *const *argv[],
                                     void *c, void *out_configs,
                                     lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (queue_conf_t *) c,
                        (configs_t *) out_configs,
                        queue_set_committed_information_rate,
                        OPT_COMMITTED_INFORMATION_RATE,
                        CMD_UINT64,
                        result);
}

static lagopus_result_t
excess_burst_size_opt_parse(const char *const *argv[],
                            void *c, void *out_configs,
                            lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (queue_conf_t *) c,
                        (configs_t *) out_configs,
                        queue_set_excess_burst_size,
                        OPT_EXCESS_BURST_SIZE,
                        CMD_UINT64,
                        result);
}

static lagopus_result_t
peak_burst_size_opt_parse(const char *const *argv[],
                          void *c, void *out_configs,
                          lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (queue_conf_t *) c,
                        (configs_t *) out_configs,
                        queue_set_peak_burst_size,
                        OPT_PEAK_BURST_SIZE,
                        CMD_UINT64,
                        result);
}

static lagopus_result_t
peak_information_rate_opt_parse(const char *const *argv[],
                                void *c, void *out_configs,
                                lagopus_dstring_t *result) {
  return uint_opt_parse(argv, (queue_conf_t *) c,
                        (configs_t *) out_configs,
                        queue_set_peak_information_rate,
                        OPT_PEAK_INFORMATION_RATE,
                        CMD_UINT64,
                        result);
}

static lagopus_result_t
color_opt_parse(const char *const *argv[],
                void *c, void *out_configs,
                lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  queue_conf_t *conf = NULL;
  configs_t *configs = NULL;
  datastore_queue_color_t color;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (queue_conf_t *) c;
    configs = (configs_t *) out_configs;

    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        if ((ret = queue_color_to_enum(*(*argv), &color)) ==
            LAGOPUS_RESULT_OK) {
          ret = queue_set_color(conf->modified_attr,
                                color);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't add color.");
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
      configs->flags = OPT_BIT_GET(OPT_COLOR);
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
          queue_conf_t *conf,
          configs_t *out_configs,
          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_queue_type_t type;
  datastore_queue_color_t color;
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
              ret = queue_type_to_enum(*(++argv), &type);
              if (ret == LAGOPUS_RESULT_OK) {
                ret = queue_set_type(conf->modified_attr,
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
          ret = queue_get_type(conf->modified_attr,
                               &type);
          if (ret == LAGOPUS_RESULT_OK) {
            switch (type) {
              case DATASTORE_QUEUE_TYPE_SINGLE_RATE:
                table = single_rate_opt_table;
                break;
              case DATASTORE_QUEUE_TYPE_TWO_RATE:
                table = two_rate_opt_table;
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
    if ((ret = queue_get_type(conf->modified_attr,
                              &type)) ==
        LAGOPUS_RESULT_OK) {
      if (type == DATASTORE_QUEUE_TYPE_UNKNOWN) {
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

    /* set color */
    if ((ret = queue_get_color(conf->modified_attr,
                               &color)) ==
        LAGOPUS_RESULT_OK) {
      if (color == DATASTORE_QUEUE_COLOR_UNKNOWN) {
        ret = queue_set_color(conf->modified_attr,
                              DATASTORE_BRIDGE_FAIL_MODE_SECURE);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't set color.");
          goto done;
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't get color.");
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
  queue_conf_t *conf = NULL;
  (void) argc;
  (void) proc;

  ret = queue_conf_create(&conf, name);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = queue_conf_add(conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = opt_parse(argv, conf, out_configs, result);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = queue_cmd_update_internal(iptr, state, conf,
                                        true, false, result);

        if (ret != LAGOPUS_RESULT_OK &&
            ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_set(result, ret, NULL);
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add queue_conf.");
    }

    if (ret != LAGOPUS_RESULT_OK) {
      (void) queue_conf_delete(conf);
      goto done;
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't create queue_conf.");
  }

  if (ret != LAGOPUS_RESULT_OK && conf != NULL) {
    queue_conf_destroy(conf);
  }

done:
  return ret;
}

static lagopus_result_t
queue_attr_dup_modified(queue_conf_t *conf,
                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (conf->modified_attr == NULL) {
    if (conf->current_attr != NULL) {
      /*
       * already exists. copy it.
       */
      ret = queue_attr_duplicate(conf->current_attr,
                                 &conf->modified_attr,
                                 NULL);
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
                              queue_conf_t *conf,
                              datastore_update_proc_t proc,
                              configs_t *out_configs,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) argc;
  (void) proc;

  ret = queue_attr_dup_modified(conf, result);

  if (ret == LAGOPUS_RESULT_OK) {
    conf->is_destroying = false;
    out_configs->is_config = true;
    ret = opt_parse(argv, conf, out_configs, result);

    if (ret == LAGOPUS_RESULT_OK) {
      if (out_configs->flags == 0) {
        /* update. */
        ret = queue_cmd_update_internal(iptr, state, conf,
                                        true, false, result);

        if (ret != LAGOPUS_RESULT_OK &&
            ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
          ret = datastore_json_result_set(result, ret, NULL);
        }
      } else {
        /* show. */
        ret = queue_conf_one_list(&out_configs->list, conf);

        if (ret >= 0) {
          out_configs->size = (size_t) ret;
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = datastore_json_result_string_setf(
              result, ret,
              "Can't create list of queue_conf.");
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
  queue_conf_t *conf = NULL;
  char *namespace = NULL;
  (void) hptr;
  (void) proc;

  if (name != NULL) {
    ret = queue_find(name, &conf);

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
  queue_conf_t *conf = NULL;
  (void) hptr;
  (void) proc;

  if (name != NULL) {
    ret = queue_find(name, &conf);

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
                              queue_conf_t *conf,
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
          ret = queue_cmd_update_internal(iptr, state, conf,
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
  queue_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = queue_find(name, &conf);

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
                               queue_conf_t *conf,
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
      ret = queue_cmd_update_internal(iptr, state, conf,
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
  queue_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = queue_find(name, &conf);

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
                               queue_conf_t *conf,
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
          ret = queue_cmd_update_internal(iptr, state, conf,
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

        queue_cmd_do_destroy(conf, state);
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
  queue_conf_t *conf = NULL;
  (void) argc;
  (void) argv;
  (void) hptr;
  (void) proc;
  (void) out_configs;
  (void) result;

  if (name != NULL) {
    ret = queue_find(name, &conf);

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
  queue_conf_t *conf = NULL;
  configs_t *configs = NULL;
  (void) iptr;
  (void) state;
  (void) argc;
  (void) hptr;
  (void) proc;

  if (argv != NULL && name != NULL &&
      out_configs != NULL && result != NULL) {
    configs = (configs_t *) out_configs;
    ret = queue_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK &&
        conf->is_destroying == false) {
      if (*(argv + 1) == NULL) {
        configs->is_show_stats = true;

        ret = dp_queue_stats_get(conf->name, &configs->stats);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = queue_conf_one_list(&configs->list, conf);

          if (ret >= 0) {
            configs->size = (size_t) ret;
            ret = LAGOPUS_RESULT_OK;
          } else {
            ret = datastore_json_result_string_setf(
                    result, ret,
                    "Can't create list of queue_conf.");
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
  queue_conf_t *conf = NULL;
  char *target = NULL;

  if (name == NULL) {
    ret = namespace_get_current_namespace(&target);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = queue_conf_list(&out_configs->list, target);
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
      ret = queue_conf_list(&out_configs->list, "");
    } else if (ret == NS_NAMESPACE) {
      /* namespace + delim */
      ret = queue_conf_list(&out_configs->list, target);
    } else if (ret == NS_FULLNAME) {
      /* namespace + name or delim + name */
      ret = queue_find(target, &conf);

      if (ret == LAGOPUS_RESULT_OK) {
        if (conf->is_destroying == false) {
          ret = queue_conf_one_list(&out_configs->list, conf);
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
            "Can't create list of queue_conf.");
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
queue_cmd_enable(datastore_interp_t *iptr,
                 datastore_interp_state_t state,
                 void *obj,
                 bool *currentp,
                 bool enabled,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (currentp == NULL) {
    if (enabled == true) {
      ret = enable_sub_cmd_parse_internal(iptr, state,
                                          (queue_conf_t *) obj,
                                          false,
                                          result);
    } else {
      ret = disable_sub_cmd_parse_internal(iptr, state,
                                           (queue_conf_t *) obj,
                                           false,
                                           result);
    }
  } else {
    if (obj != NULL) {
      *currentp = ((queue_conf_t *) obj)->is_enabled;
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
queue_cmd_destroy(datastore_interp_t *iptr,
                  datastore_interp_state_t state,
                  void *obj,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = destroy_sub_cmd_parse_internal(iptr, state,
                                       (queue_conf_t *) obj,
                                       false,
                                       result);
  return ret;
}

STATIC lagopus_result_t
queue_cmd_serialize(datastore_interp_t *iptr,
                    datastore_interp_state_t state,
                    const void *obj,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  queue_conf_t *conf = NULL;

  char *escaped_name = NULL;

  /* type */
  datastore_queue_type_t type = DATASTORE_QUEUE_TYPE_UNKNOWN;
  const char *type_str = NULL;
  char *escaped_type_str = NULL;

  /* color */
  datastore_queue_color_t color = DATASTORE_QUEUE_COLOR_UNKNOWN;
  const char *color_str = NULL;
  char *escaped_color_str = NULL;

  /* priority */
  uint16_t priority = 0;

  /* committed-burst-size */
  uint64_t committed_burst_size = 0;

  /* committed_information_rate */
  uint64_t committed_information_rate = 0;

  /* excess_burst_size */
  uint64_t excess_burst_size = 0;

  /* peak_burst_size */
  uint64_t peak_burst_size = 0;

  /* peak_information_rate */
  uint64_t peak_information_rate = 0;

  bool is_escaped = false;
  uint64_t flags = OPTS_MAX;

  (void) state;

  if (iptr != NULL && obj != NULL && result != NULL) {
    conf = (queue_conf_t *) obj;

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
    if ((ret = queue_get_type(conf->current_attr,
                              &type)) ==
        LAGOPUS_RESULT_OK) {
      if (type != DATASTORE_QUEUE_TYPE_UNKNOWN) {
        if ((ret = queue_type_to_str(type,
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
      case DATASTORE_QUEUE_TYPE_SINGLE_RATE:
        flags &= OPT_SINGLE_RATE;
        break;
      case DATASTORE_QUEUE_TYPE_TWO_RATE:
        flags &= OPT_TWO_RATE;
        break;
      default:
        ret = LAGOPUS_RESULT_OUT_OF_RANGE;
        lagopus_perror(ret);
        goto done;
    }

    /* priority opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_PRIORITY)) == true) {
      if ((ret = queue_get_priority(conf->current_attr,
                                    &priority)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %s",
                                           opt_strs[OPT_PRIORITY])) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %d",
                                             priority)) !=
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

    /* color opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_COLOR)) == true) {
      if ((ret = queue_get_color(conf->current_attr,
                                 &color)) ==
          LAGOPUS_RESULT_OK) {
        if (color != DATASTORE_QUEUE_COLOR_UNKNOWN) {
          if ((ret = queue_color_to_str(color,
                                        &color_str)) ==
              LAGOPUS_RESULT_OK) {
            if (IS_VALID_STRING(color_str) == true) {
              if ((ret = lagopus_str_escape(color_str, "\"",
                                            &is_escaped,
                                            &escaped_color_str)) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = lagopus_dstring_appendf(result, " %s",
                                                   opt_strs[OPT_COLOR])) ==
                    LAGOPUS_RESULT_OK) {
                  if ((ret = lagopus_dstring_appendf(
                               result,
                               ESCAPE_NAME_FMT(is_escaped, escaped_color_str),
                               escaped_color_str)) !=
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
    }

    /* committed-burst-size opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_COMMITTED_BURST_SIZE)) == true) {
      if ((ret = queue_get_committed_burst_size(conf->current_attr,
                 &committed_burst_size)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(
                     result, " %s",
                     opt_strs[OPT_COMMITTED_BURST_SIZE])) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %"PRIu64,
                                             committed_burst_size)) !=
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

    /* committed-information-rate opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_COMMITTED_INFORMATION_RATE)) ==
        true) {
      if ((ret = queue_get_committed_information_rate(
                   conf->current_attr,
                   &committed_information_rate)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(
                     result, " %s",
                     opt_strs[OPT_COMMITTED_INFORMATION_RATE])) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %"PRIu64,
                                             committed_information_rate)) !=
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

    /* excess-burst-size opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_EXCESS_BURST_SIZE)) == true) {
      if ((ret = queue_get_excess_burst_size(conf->current_attr,
                                             &excess_burst_size)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(
                     result, " %s",
                     opt_strs[OPT_EXCESS_BURST_SIZE])) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %"PRIu64,
                                             excess_burst_size)) !=
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

    /* peak-burst-size opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_PEAK_BURST_SIZE)) == true) {
      if ((ret = queue_get_peak_burst_size(conf->current_attr,
                                           &peak_burst_size)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(
                     result, " %s",
                     opt_strs[OPT_PEAK_BURST_SIZE])) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %"PRIu64,
                                             peak_burst_size)) !=
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

    /* peak-information-rate opt. */
    if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_PEAK_INFORMATION_RATE)) == true) {
      if ((ret = queue_get_peak_information_rate(conf->current_attr,
                 &peak_information_rate)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(
                     result, " %s",
                     opt_strs[OPT_PEAK_INFORMATION_RATE])) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %"PRIu64,
                                             peak_information_rate)) !=
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

    /* Add newline. */
    if ((ret = lagopus_dstring_appendf(result, "\n")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

  done:
    free((void *) escaped_name);
    free((void *) escaped_type_str);
    free((void *) escaped_color_str);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
queue_cmd_compare(const void *obj1, const void *obj2, int *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj1 != NULL && obj2 != NULL && result != NULL) {
    *result = strcmp(((queue_conf_t *) obj1)->name,
                     ((queue_conf_t *) obj2)->name);
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
queue_cmd_getname(const void *obj, const char **namep) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (obj != NULL && namep != NULL) {
    *namep = ((queue_conf_t *)obj)->name;
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
queue_cmd_duplicate(const void *obj, const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  queue_conf_t *dup_obj = NULL;

  if (obj != NULL) {
    ret = queue_conf_duplicate(obj, &dup_obj, namespace);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = queue_conf_add(dup_obj);

      if (ret != LAGOPUS_RESULT_OK && dup_obj != NULL) {
        queue_conf_destroy(dup_obj);
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

  if ((ret = queue_initialize()) != LAGOPUS_RESULT_OK) {
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

  /* create hashmap for single_rate opt. */
  if ((ret = lagopus_hashmap_create(&single_rate_opt_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* create hashmap for two_rate opt. */
  if ((ret = lagopus_hashmap_create(&two_rate_opt_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* add opts for single_rate opt. */
  if (((ret = opt_add(opt_strs[OPT_ID], id_opt_parse,
                      &single_rate_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_PRIORITY], priority_opt_parse,
                      &single_rate_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_COLOR], color_opt_parse,
                      &single_rate_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_COMMITTED_BURST_SIZE],
                      committed_burst_size_opt_parse,
                      &single_rate_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_COMMITTED_INFORMATION_RATE],
                      committed_information_rate_opt_parse,
                      &single_rate_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_EXCESS_BURST_SIZE],
                      excess_burst_size_opt_parse,
                      &single_rate_opt_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  /* add opts for two_rate opt. */
  if (((ret = opt_add(opt_strs[OPT_ID], id_opt_parse,
                      &two_rate_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_PRIORITY], priority_opt_parse,
                      &two_rate_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_COLOR], color_opt_parse,
                      &two_rate_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_COMMITTED_BURST_SIZE],
                      committed_burst_size_opt_parse,
                      &two_rate_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_COMMITTED_INFORMATION_RATE],
                      committed_information_rate_opt_parse,
                      &two_rate_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_PEAK_BURST_SIZE],
                      peak_burst_size_opt_parse,
                      &two_rate_opt_table)) !=
       LAGOPUS_RESULT_OK) ||
      ((ret = opt_add(opt_strs[OPT_PEAK_INFORMATION_RATE],
                      peak_information_rate_opt_parse,
                      &two_rate_opt_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  if ((ret = datastore_register_table(CMD_NAME,
                                      &queue_table,
                                      queue_cmd_update,
                                      queue_cmd_enable,
                                      queue_cmd_serialize,
                                      queue_cmd_destroy,
                                      queue_cmd_compare,
                                      queue_cmd_getname,
                                      queue_cmd_duplicate)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = datastore_interp_register_command(&s_interp, CONFIGURATOR_NAME,
             CMD_NAME,
             queue_cmd_parse)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

done:
  return ret;
}

static lagopus_result_t
queue_cmd_json_create(lagopus_dstring_t *ds,
                      configs_t *configs,
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *type_str = NULL;
  const char *color_str = NULL;
  queue_attr_t *attr = NULL;
  datastore_queue_type_t type;
  datastore_queue_color_t color;
  uint32_t id;
  uint16_t priority;
  uint64_t committed_burst_size;
  uint64_t committed_information_rate;
  uint64_t excess_burst_size;
  uint64_t peak_burst_size;
  uint64_t peak_information_rate;
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
          if ((ret = queue_get_type(attr, &type)) ==
              LAGOPUS_RESULT_OK) {
            /* set show flags. */
            flags = configs->flags;
            switch (type) {
              case DATASTORE_QUEUE_TYPE_SINGLE_RATE:
                flags &= OPT_SINGLE_RATE;
                break;
              case DATASTORE_QUEUE_TYPE_TWO_RATE:
                flags &= OPT_TWO_RATE;
                break;
              case DATASTORE_QUEUE_TYPE_UNKNOWN:
                flags &= OPT_UNKNOWN;
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
            if ((ret = queue_type_to_str(type, &type_str)) ==
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

          /* id */
          /* When is_used is false, queue_id will be undefined. */
          if (configs->list[i]->is_used == true) {
            if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_ID)) == true) {
              if ((ret = queue_get_id(attr,
                                      &id)) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = datastore_json_uint32_append(
                        ds, ATTR_NAME_GET(opt_strs, OPT_ID),
                        id, true)) !=
                    LAGOPUS_RESULT_OK) {
                  lagopus_perror(ret);
                  goto done;
                }
              } else {
                lagopus_perror(ret);
                goto done;
              }
            }
          }

          /* priority */
          if (IS_BIT_SET(flags, OPT_BIT_GET(OPT_PRIORITY)) == true) {
            if ((ret = queue_get_priority(attr,
                                          &priority)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint16_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_PRIORITY),
                           priority, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* color */
          if (IS_BIT_SET(configs->flags, OPT_BIT_GET(OPT_COLOR)) == true) {
            if ((ret = queue_get_color(attr,
                                       &color)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = queue_color_to_str(color,
                                            &color_str)) ==
                  LAGOPUS_RESULT_OK) {
                if ((ret = datastore_json_string_append(ds,
                                                        ATTR_NAME_GET(opt_strs,
                                                            OPT_COLOR),
                                                        color_str, true)) !=
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

          /* committed burst size */
          if (IS_BIT_SET(flags,
                         OPT_BIT_GET(OPT_COMMITTED_BURST_SIZE)) == true) {
            if ((ret = queue_get_committed_burst_size(attr,
                       &committed_burst_size)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_COMMITTED_BURST_SIZE),
                           committed_burst_size, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* committed information rate */
          if (IS_BIT_SET(flags,
                         OPT_BIT_GET(OPT_COMMITTED_INFORMATION_RATE)) == true) {
            if ((ret = queue_get_committed_information_rate(
                         attr,
                         &committed_information_rate)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs,
                                             OPT_COMMITTED_INFORMATION_RATE),
                           committed_information_rate, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* excess burst size */
          if (IS_BIT_SET(flags,
                         OPT_BIT_GET(OPT_EXCESS_BURST_SIZE)) == true) {
            if ((ret = queue_get_excess_burst_size(attr,
                                                   &excess_burst_size)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_EXCESS_BURST_SIZE),
                           excess_burst_size, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* peak burst size */
          if (IS_BIT_SET(flags,
                         OPT_BIT_GET(OPT_PEAK_BURST_SIZE)) == true) {
            if ((ret = queue_get_peak_burst_size(attr,
                                                 &peak_burst_size)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_PEAK_BURST_SIZE),
                           peak_burst_size, true)) !=
                  LAGOPUS_RESULT_OK) {
                lagopus_perror(ret);
                goto done;
              }
            } else {
              lagopus_perror(ret);
              goto done;
            }
          }

          /* peak information rate */
          if (IS_BIT_SET(flags,
                         OPT_BIT_GET(OPT_PEAK_INFORMATION_RATE)) == true) {
            if ((ret = queue_get_peak_information_rate(
                         attr,
                         &peak_information_rate)) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = datastore_json_uint64_append(
                           ds, ATTR_NAME_GET(opt_strs, OPT_PEAK_INFORMATION_RATE),
                           peak_information_rate, true)) !=
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

static lagopus_result_t
queue_cmd_stats_json_create(lagopus_dstring_t *ds,
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

        /* port_no */
        /* When port_no is 0, port no will be undefined. */
        if (configs->stats.port_no != 0) {
          if ((ret = datastore_json_uint32_append(
                  ds, ATTR_NAME_GET(stat_strs, STATS_PORT_NO),
                  configs->stats.port_no, true)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
        }

        /* queue_id */
        /* When is_used is false, queue_id will be undefined. */
        if (configs->list[0]->is_used == true) {
          if ((ret = datastore_json_uint32_append(
                  ds, ATTR_NAME_GET(stat_strs, STATS_QUEUE_ID),
                  configs->stats.queue_id, true)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
        }

        /* tx_bytes */
        if ((ret = datastore_json_uint64_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_TX_BYTES),
                     configs->stats.tx_bytes, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* tx_packets */
        if ((ret = datastore_json_uint64_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_TX_PACKETS),
                     configs->stats.tx_packets, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* tx_errors */
        if ((ret = datastore_json_uint64_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_TX_ERRORS),
                     configs->stats.tx_errors, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* duration_sec */
        if ((ret = datastore_json_uint32_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_DURATION_SEC),
                     configs->stats.duration_sec, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* duration_nsec */
        if ((ret = datastore_json_uint32_append(
                     ds, ATTR_NAME_GET(stat_strs, STATS_DURATION_NSEC),
                     configs->stats.duration_nsec, true)) !=
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
queue_cmd_parse(datastore_interp_t *iptr,
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
  configs_t out_configs = {0 , 0LL, false, false, false,
    {0LL, 0LL, 0LL, 0LL, 0LL, 0LL, 0LL},
    NULL, NULL
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
            ret = queue_cmd_stats_json_create(&conf_result, &out_configs,
                                              result);
          } else {
            ret = queue_cmd_json_create(&conf_result, &out_configs,
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

/*
 * Public:
 */

lagopus_result_t
queue_cmd_enable_propagation(datastore_interp_t *iptr,
                             datastore_interp_state_t state,
                             char *name,
                             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = enable_sub_cmd_parse(iptr, state, 0, NULL,
                               name, NULL, queue_cmd_update,
                               NULL, result);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

lagopus_result_t
queue_cmd_disable_propagation(datastore_interp_t *iptr,
                              datastore_interp_state_t state,
                              char *name,
                              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (iptr != NULL && name != NULL && result != NULL) {
    ret = disable_sub_cmd_parse(iptr, state, 0, NULL,
                                name, NULL, queue_cmd_update,
                                NULL, result);
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

lagopus_result_t
queue_cmd_update_propagation(datastore_interp_t *iptr,
                             datastore_interp_state_t state,
                             char *name,
                             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  queue_conf_t *conf = NULL;

  if (name != NULL) {
    ret = queue_find(name, &conf);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = queue_cmd_update(iptr, state,
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
queue_cmd_queue_id_is_exists(char *name,
                             const uint32_t queue_id,
                             bool *b) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  queue_conf_t *conf = NULL;
  uint32_t q_id;

  if (name != NULL && b != NULL) {
    ret = queue_find(name, &conf);

    *b = false;
    if (ret == LAGOPUS_RESULT_OK) {
      if (conf->current_attr != NULL) {
        if ((ret = queue_get_id(conf->current_attr, &q_id)) ==
            LAGOPUS_RESULT_OK) {
          if (queue_id == q_id) {
            *b = true;
            goto done;
          }
        }
      }
      if (conf->modified_attr != NULL) {
        if ((ret = queue_get_id(conf->modified_attr, &q_id)) ==
            LAGOPUS_RESULT_OK) {
          if (queue_id == q_id) {
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
queue_cmd_initialize(void) {
  return initialize_internal();
}

void
queue_cmd_finalize(void) {
  lagopus_hashmap_destroy(&sub_cmd_table, true);
  sub_cmd_table = NULL;
  lagopus_hashmap_destroy(&single_rate_opt_table, true);
  single_rate_opt_table = NULL;
  lagopus_hashmap_destroy(&two_rate_opt_table, true);
  two_rate_opt_table = NULL;

  queue_finalize();
}
