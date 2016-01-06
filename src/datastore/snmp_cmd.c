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
#include "lagopus/datastore.h"
#include "datastore_internal.h"
#include "conv_json.h"
#define RESULT_NULL	"\"\""

#define SNMP_CMD_NAME "snmp"

#define DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC 10
#define MINIMUM_PING_SEC 1
#define MAXIMUM_PING_SEC 255
#define DEFAULT_SNMPMGR_AGENTX_SOCKET "tcp:localhost:705"
#define IS_ENABLED(x) (x == true ? "true" : "false")



typedef struct snmp_conf {
  lagopus_rwlock_t s_lck;
  uint16_t ping_interval;
  char *agentx_sock;
  bool is_enabled;
} snmp_conf_t;

static lagopus_result_t
snmp_initialize(void);
static void
snmp_finalize(void);
static void
s_rlock_snmp_conf(snmp_conf_t *conf);
static void
s_wlock_snmp_conf(snmp_conf_t *conf);
static void
s_unlock_snmp_conf(snmp_conf_t *conf);

lagopus_result_t
lagopus_snmp_set_agentx_sock(const char *new_agentx);
lagopus_result_t
lagopus_snmp_get_agentx_sock(char **agentx_sock);
lagopus_result_t
lagopus_snmp_set_ping_interval(const uint16_t new_ping);
lagopus_result_t
lagopus_snmp_get_ping_interval(uint16_t *ping_interval);
lagopus_result_t
lagopus_snmp_set_enable(bool enable);
lagopus_result_t
lagopus_snmp_get_enable(bool *enable);



static snmp_conf_t conf;

static lagopus_result_t
snmp_initialize(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = lagopus_rwlock_create(&(conf.s_lck));
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    lagopus_exit_fatal("Can't initialize the configurator lock.\n");
    free((void *)(conf.s_lck));
  } else {
    if (ret == LAGOPUS_RESULT_OK) {

      s_wlock_snmp_conf(&conf);
      {
        conf.ping_interval = DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC;
        conf.agentx_sock = strdup(DEFAULT_SNMPMGR_AGENTX_SOCKET);
        conf.is_enabled = false;
      }
      s_unlock_snmp_conf(&conf);

      return LAGOPUS_RESULT_OK;
    }
  }
  return ret;
}

static void
snmp_finalize(void) {
  s_wlock_snmp_conf(&conf);
  {
    free((void *)conf.agentx_sock);
  }
  s_unlock_snmp_conf(&conf);
  free((void *)conf.s_lck);
}

static void
s_rlock_snmp_conf(snmp_conf_t *snmp) {
  if (snmp != NULL) {
    (void)lagopus_rwlock_reader_lock(&snmp->s_lck);
  }
}

static void
s_wlock_snmp_conf(snmp_conf_t *snmp) {
  if (snmp != NULL) {
    (void)lagopus_rwlock_writer_lock(&snmp->s_lck);
  }
}

static void
s_unlock_snmp_conf(snmp_conf_t *snmp) {
  if (snmp != NULL) {
    (void)lagopus_rwlock_unlock(&snmp->s_lck);
  }
}


static inline lagopus_result_t
s_parse_snmp_show(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool enable = false;
  uint16_t ping_interval;
  char *agentx_sock = NULL;
  char *esc_sock = NULL;

  if ((ret = lagopus_snmp_get_agentx_sock(&agentx_sock))
      == LAGOPUS_RESULT_OK) {
    ret = datastore_json_string_escape(agentx_sock,
                                       (char **)&esc_sock);
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't convert "
                                              "to an unescaped strings.");
      goto done;
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't get "
                                            "master-agentx-socket.");
    goto done;
  }

  if ((ret = lagopus_snmp_get_ping_interval(&ping_interval))
      == LAGOPUS_RESULT_OK) {
    if (ping_interval > MAXIMUM_PING_SEC) {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_TOO_LONG,
                                              "ping-interval-second "
                                              "must be smaller than 256.");
      goto done;
    } else if (ping_interval < MINIMUM_PING_SEC) {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_TOO_SHORT,
                                              "ping-interval-second "
                                              "must be greater than zero.");
      goto done;
    } else {
      if ((ret = lagopus_snmp_get_enable(&enable))
          == LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_setf(result, ret, "[{"
                                         "\"master-agentx-socket\":\"%s\",\n"
                                         "\"ping-interval-second\":%d,\n"
                                         "\"is-enabled\":%s"
                                         "}]",
                                         esc_sock,
                                         ping_interval,
                                         IS_ENABLED(enable));
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't get enable.");
      }
    }
  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't get "
                                            "ping-interval-second.");
  }

done:
  free((void *)agentx_sock);
  free((void *)esc_sock);

  return ret;
}

static inline lagopus_result_t
s_parse_snmp_internal(datastore_interp_state_t state,
                      const char *const argv[],
                      lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *agentx_sock = NULL;
  char *esc_agentx = NULL;
  char *ping = NULL;

  while (IS_VALID_STRING(*argv) == true) {
    if (strcmp(*argv, "-master-agentx-socket") == 0) {
      if (IS_VALID_STRING(*(argv + 1)) == true) {
        argv++;
        if ((ret = lagopus_str_unescape(*argv, "\"'", &agentx_sock))
            > 0) {

          if (state == DATASTORE_INTERP_STATE_DRYRUN) {
            ret = LAGOPUS_RESULT_OK;
          } else {
            if ((ret = lagopus_snmp_set_agentx_sock(agentx_sock))
                != LAGOPUS_RESULT_OK) {
              ret = datastore_json_result_string_setf(result, ret,
                                                      "Can't set %s.",
                                                      agentx_sock);
              goto done;
            }
          }
        } else if (ret == 0) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt val = %s ",
                                                  *argv);
          goto done;
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't convert to an "
                                                  "unescaped strings.");
          goto done;
        }
      } else {

        if ((ret = lagopus_snmp_get_agentx_sock(&agentx_sock))
            == LAGOPUS_RESULT_OK) {
          ret = datastore_json_string_escape(agentx_sock,
                                             (char **)&esc_agentx);
          if (ret == LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_setf(result, ret, "[{"
                                             "\"master-agentx-socket\":\"%s\""
                                             "}]", esc_agentx);
            goto done;
          } else {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't convert to "
                                                    "an escaped strings.");
            goto done;
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get "
                                                  "master-agentx-socket.");
          goto done;
        }
      }

    } else if (strcmp(*argv, "-ping-interval-second") == 0) {

      if (IS_VALID_STRING(*(argv + 1)) == true) {
        argv++;
        ret = lagopus_str_unescape(*argv, "\"'", &ping);
        if (ret > 0) {
          uint16_t new_ping;
          ret = lagopus_str_parse_uint16(ping, &new_ping);
          if (ret == LAGOPUS_RESULT_OK) {
            if (state == DATASTORE_INTERP_STATE_DRYRUN) {
              ret = LAGOPUS_RESULT_OK;
            } else {
              if ((ret = lagopus_snmp_set_ping_interval(new_ping))
                  != LAGOPUS_RESULT_OK) {
                ret = datastore_json_result_string_setf(result, ret,
                                                        "Can't set "
                                                        "ping-interval.");

                goto done;
              }
            }
          } else {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't parse string "
                                                    "to uint16_t.");
            goto done;
          }
        } else if (ret == 0) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt val = %s ",
                                                  *argv);
          goto done;
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't convert to an "
                                                  "unescaped strings.");
          goto done;
        }
      } else {
        uint16_t ping_interval;
        ret = lagopus_snmp_get_ping_interval(&ping_interval);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_setf(result, ret, "[{"
                                           "\"ping-interval-second\":%d"
                                           "}]", ping_interval);
          goto done;
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get "
                                                  "ping-interval-second.");
          goto done;
        }
      }

    } else if (strcmp(*argv, "enable") == 0) {
      if (state == DATASTORE_INTERP_STATE_DRYRUN) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        if ((ret = lagopus_snmp_set_enable(true))
            != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't set enable.");
          goto done;
        }
      }
    } else {
      char *esc_str = NULL;
      ret = datastore_json_string_escape(*argv, (char **)&esc_str);
      if (ret == LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result, LAGOPUS_RESULT_NOT_FOUND,
                                                "option = %s",
                                                esc_str);
      }
      free((void *)esc_str);
    }

 done:
    free((void *)agentx_sock);
    free((void *)esc_agentx);
    free((void *)ping);
    agentx_sock = NULL;
    esc_agentx = NULL;
    ping = NULL;

    if (ret != LAGOPUS_RESULT_OK) {
      break;
    }
    argv++;
  }

  if (ret == LAGOPUS_RESULT_OK &&
      lagopus_dstring_empty(result)) {
    ret = lagopus_dstring_appendf(result, "{\"ret\":\"OK\"}");
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add strings "
                                              "to dstring.");
    }
  }

  return ret;
}

static lagopus_result_t
s_parse_snmp(datastore_interp_t *iptr,
             datastore_interp_state_t state,
             size_t argc, const char *const argv[],
             lagopus_hashmap_t *hptr,
             datastore_update_proc_t u_proc,
             datastore_enable_proc_t e_proc,
             datastore_serialize_proc_t s_proc,
             datastore_destroy_proc_t d_proc,
             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;

  (void)iptr;
  (void)argc;
  (void)hptr;
  (void)u_proc;
  (void)e_proc;
  (void)s_proc;
  (void)d_proc;

  for (i = 0; i < argc; i++) {
    lagopus_msg_debug(1, "argv[" PFSZS(4, u) "]:\t'%s'\n", i, argv[i]);
  }

  argv++;
  if (IS_VALID_STRING(*argv) == false) {
    ret = s_parse_snmp_show(result);
  } else {
    ret = s_parse_snmp_internal(state, argv, result);
  }

  return ret;
}

lagopus_result_t
lagopus_snmp_set_agentx_sock(const char *new_agentx) {
  if (new_agentx != NULL) {
    char *str = NULL;
    str = strdup(new_agentx);

    if (str != NULL) {

      s_wlock_snmp_conf(&conf);
      {
        free((void *)conf.agentx_sock);
        conf.agentx_sock = str;
      }
      s_unlock_snmp_conf(&conf);

      return LAGOPUS_RESULT_OK;
    } else {
      free((void *)str);
      return LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
}

lagopus_result_t
lagopus_snmp_get_agentx_sock(char **agentx_sock) {
  if (agentx_sock != NULL && *agentx_sock == NULL) {
    if (IS_VALID_STRING(conf.agentx_sock) == true) {
      char *str = NULL;

      s_rlock_snmp_conf(&conf);
      {
        str = strdup(conf.agentx_sock);
      }
      s_unlock_snmp_conf(&conf);

      if (IS_VALID_STRING(str) == true) {
        *agentx_sock = str;
        return LAGOPUS_RESULT_OK;
      } else {
        free((void *)str);
        return LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      return LAGOPUS_RESULT_NOT_DEFINED;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

lagopus_result_t
lagopus_snmp_set_ping_interval(const uint16_t new_ping) {
  if (new_ping >= MINIMUM_PING_SEC &&
      new_ping <= MAXIMUM_PING_SEC) {

    s_wlock_snmp_conf(&conf);
    {
      conf.ping_interval = new_ping;
    }
    s_unlock_snmp_conf(&conf);

    return LAGOPUS_RESULT_OK;
  } else if (new_ping > MINIMUM_PING_SEC) {
    return LAGOPUS_RESULT_TOO_LONG;
  } else {
    return LAGOPUS_RESULT_TOO_SHORT;
  }
}

lagopus_result_t
lagopus_snmp_get_ping_interval(uint16_t *ping_interval) {
  if (ping_interval != NULL) {
    if (conf.ping_interval >= MINIMUM_PING_SEC &&
        conf.ping_interval <= MAXIMUM_PING_SEC) {
      uint16_t val;

      s_rlock_snmp_conf(&conf);
      {
        val = conf.ping_interval;
      }
      s_unlock_snmp_conf(&conf);

      *ping_interval = val;
      return LAGOPUS_RESULT_OK;
    } else if (conf.ping_interval > MINIMUM_PING_SEC) {
      return LAGOPUS_RESULT_TOO_LONG;
    } else {
      return LAGOPUS_RESULT_TOO_SHORT;
    }
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

lagopus_result_t
lagopus_snmp_set_enable(bool enable) {

  s_wlock_snmp_conf(&conf);
  {
    conf.is_enabled = enable;
  }
  s_unlock_snmp_conf(&conf);

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
lagopus_snmp_get_enable(bool *enable) {
  if (enable != NULL) {

    s_wlock_snmp_conf(&conf);
    {
      *enable = conf.is_enabled;
    }
    s_unlock_snmp_conf(&conf);

    return LAGOPUS_RESULT_OK;
  }
  return LAGOPUS_RESULT_INVALID_ARGS;
}

lagopus_result_t
snmp_cmd_serialize(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* master-agentx-socket */
  char *master_agentx_socket_str = NULL;
  char *escaped_master_agentx_socket_str = NULL;

  /* ping-interval-second */
  uint16_t ping_interval_second = 0;

  bool is_escaped = false;

  if (result != NULL) {
    /* cmmand name. */
    if ((ret = lagopus_dstring_appendf(result, SNMP_CMD_NAME)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    /* master-agentx-socket opt. */
    if ((ret = lagopus_snmp_get_agentx_sock(&master_agentx_socket_str))
        == LAGOPUS_RESULT_OK) {
      if (IS_VALID_STRING(master_agentx_socket_str) == true) {
        if ((ret = lagopus_str_escape(master_agentx_socket_str, "\"",
                                      &is_escaped,
                                      &escaped_master_agentx_socket_str)) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " -master-agentx-socket")) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(
                         result,
                         ESCAPE_NAME_FMT(is_escaped, escaped_master_agentx_socket_str),
                         escaped_master_agentx_socket_str)) !=
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

    /* ping-interval-second opt. */
    if ((ret = lagopus_snmp_get_ping_interval(&ping_interval_second)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " -ping-interval-second")) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " %d",
                                           ping_interval_second)) !=
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
    if ((ret = lagopus_dstring_appendf(result, "\n\n")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

  done:
    free((void *) master_agentx_socket_str);
    free((void *) escaped_master_agentx_socket_str);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

