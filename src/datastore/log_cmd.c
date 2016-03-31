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



#define LOG_CMD_NAME "log"
#define MINIMUM_DBGLVL 0
#define MAXIMUM_DBGLVL UINT16_MAX
#define RESULT_NULL	"\"\""




static void
log_child_at_fork(void);

static lagopus_result_t
log_initialize(void);

static lagopus_result_t
log_ofpt_table_add(void);

static void
log_finalize(void);

static lagopus_result_t
str_to_typ(const char *str, uint32_t **typ);

static lagopus_result_t
typ_to_str(const uint32_t *typ, char **str);

static lagopus_result_t
ofpt_typespecs(const uint32_t ofpt_typ, char **str);

static struct log_conf {
  const char *str;
  uint32_t typ;
} ofpt_table[] = {
  {"hello", TRACE_OFPT_HELLO},
  {"error", TRACE_OFPT_ERROR},
  {"echo_request", TRACE_OFPT_ECHO_REQUEST},
  {"echo_reply", TRACE_OFPT_ECHO_REPLY},
  {"experimenter", TRACE_OFPT_EXPERIMENTER},
  {"features_request", TRACE_OFPT_FEATURES_REQUEST},
  {"features_reply", TRACE_OFPT_FEATURES_REPLY},
  {"get_config_request", TRACE_OFPT_GET_CONFIG_REQUEST},
  {"get_config_reply", TRACE_OFPT_GET_CONFIG_REPLY},
  {"set_config", TRACE_OFPT_SET_CONFIG},
  {"packet_in", TRACE_OFPT_PACKET_IN},
  {"flow_removed", TRACE_OFPT_FLOW_REMOVED},
  {"port_status", TRACE_OFPT_PORT_STATUS},
  {"packet_out", TRACE_OFPT_PACKET_OUT},
  {"flow_mod", TRACE_OFPT_FLOW_MOD},
  {"group_mod", TRACE_OFPT_GROUP_MOD},
  {"port_mod", TRACE_OFPT_PORT_MOD},
  {"table_mod", TRACE_OFPT_TABLE_MOD},
  {"multipart_request", TRACE_OFPT_MULTIPART_REQUEST},
  {"multipart_reply", TRACE_OFPT_MULTIPART_REPLY},
  {"barrier_request", TRACE_OFPT_BARRIER_REQUEST},
  {"barrier_reply", TRACE_OFPT_BARRIER_REPLY},
  {"queue_get_config_request", TRACE_OFPT_QUEUE_GET_CONFIG_REQUEST},
  {"queue_get_config_reply", TRACE_OFPT_QUEUE_GET_CONFIG_REPLY},
  {"role_request", TRACE_OFPT_ROLE_REQUEST},
  {"role_reply", TRACE_OFPT_ROLE_REPLY},
  {"get_async_request", TRACE_OFPT_GET_ASYNC_REQUEST},
  {"get_async_reply", TRACE_OFPT_GET_ASYNC_REPLY},
  {"set_async", TRACE_OFPT_SET_ASYNC},
  {"meter_mod", TRACE_OFPT_METER_MOD},
};

static const size_t
array_len = sizeof(ofpt_table) / sizeof(struct log_conf);

static lagopus_hashmap_t log_str_to_typ_table = NULL;
static lagopus_hashmap_t log_typ_to_str_table = NULL;

static void
log_child_at_fork(void) {
  lagopus_hashmap_atfork_child(&log_str_to_typ_table);
  lagopus_hashmap_atfork_child(&log_typ_to_str_table);
}

static lagopus_result_t
log_initialize(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;

  if ((r = lagopus_hashmap_create(&log_str_to_typ_table,
                                  LAGOPUS_HASHMAP_TYPE_STRING,
                                  NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    goto done;
  }
  if ((r = lagopus_hashmap_create(&log_typ_to_str_table,
                                  LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                                  NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    goto done;
  }

  (void)pthread_atfork(NULL, NULL, log_child_at_fork);

done:
  return r;
}

static lagopus_result_t
log_ofpt_table_add(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t ofpt;

  if (log_str_to_typ_table != NULL && log_typ_to_str_table != NULL) {
    for (ofpt = 0; ofpt < array_len; ofpt++) {
      void *typ = (void *)&ofpt_table[ofpt].typ;
      void **str = (void **)&ofpt_table[ofpt].str;

      ret = lagopus_hashmap_add(&log_str_to_typ_table,
                                (void *)ofpt_table[ofpt].str, &typ,
                                false);
      if (ret == LAGOPUS_RESULT_OK) {
        ret = lagopus_hashmap_add(&log_typ_to_str_table,
                                  (void *)&ofpt_table[ofpt].typ, str,
                                  false);
        if (ret != LAGOPUS_RESULT_OK) {
          return ret;
        }
      } else {
        return ret;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_NOT_STARTED;
  }
  return ret;
}

static void
log_finalize(void) {
  lagopus_hashmap_destroy(&log_str_to_typ_table, true);
  lagopus_hashmap_destroy(&log_typ_to_str_table, true);
  log_str_to_typ_table = NULL;
  log_typ_to_str_table = NULL;
}

static lagopus_result_t
str_to_typ(const char *str, uint32_t **typ) {
  if (log_str_to_typ_table != NULL) {
    if (IS_VALID_STRING(str) == true) {
      return lagopus_hashmap_find(&log_str_to_typ_table,
                                  (void *)str, (void **)typ);
    } else {
      return LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    return LAGOPUS_RESULT_NOT_STARTED;
  }
}

static lagopus_result_t
typ_to_str(const uint32_t *typ, char **str) {
  if (log_typ_to_str_table != NULL) {
    if (typ != NULL && *typ > 0) {
      return lagopus_hashmap_find(&log_typ_to_str_table,
                                  (void *)typ, (void **)str);
    } else {
      return LAGOPUS_RESULT_INVALID_ARGS;
    }
  } else {
    return LAGOPUS_RESULT_NOT_STARTED;
  }
}

static lagopus_result_t
ofpt_typespecs(const uint32_t ofpt_typ, char **ofpt_str) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_dstring_t ds;

  ret = lagopus_dstring_create(&ds);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = lagopus_dstring_appendf(&ds, "[");
    if (ret == LAGOPUS_RESULT_OK) {
      if (ofpt_typ == 0) {
        ret = lagopus_dstring_appendf(&ds, "\"\"");
        if (ret != LAGOPUS_RESULT_OK) {
          lagopus_dstring_destroy(&ds);
          return ret;
        }
      } else {
        size_t ofpt;
        char *typespec = NULL;
        bool first_arg = true;
        for (ofpt = 0; ofpt < array_len; ofpt++) {
          if (ofpt_table[ofpt].typ == (ofpt_table[ofpt].typ & ofpt_typ)) {
            ret = typ_to_str(&ofpt_table[ofpt].typ, &typespec);
            if (ret == LAGOPUS_RESULT_OK && IS_VALID_STRING(typespec) == true) {
              if (first_arg == true) {
                ret = lagopus_dstring_appendf(&ds, "\"%s\"", typespec);
                first_arg = false;
              } else {
                ret = lagopus_dstring_appendf(&ds, ",\"%s\"", typespec);
              }
              if (ret != LAGOPUS_RESULT_OK) {
                lagopus_dstring_destroy(&ds);
                return ret;
              }
            } else {
              lagopus_dstring_destroy(&ds);
              return ret;
            }
          }
        }
      }

      ret = lagopus_dstring_appendf(&ds, "]");
      if (ret == LAGOPUS_RESULT_OK) {
        if (IS_VALID_STRING(*ofpt_str) == true) {
          free(*ofpt_str);
          *ofpt_str = NULL;
        }
        ret = lagopus_dstring_str_get(&ds, ofpt_str);
      }
    }
  }
  lagopus_dstring_destroy(&ds);

  return ret;
}

static lagopus_result_t
ofpt_typespecs_serialize(const uint32_t ofpt_typ, char **ofpt_str) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_dstring_t ds;

  ret = lagopus_dstring_create(&ds);
  if (ret == LAGOPUS_RESULT_OK) {
    if (ret == LAGOPUS_RESULT_OK) {
      if (ofpt_typ == 0) {
        ret = lagopus_dstring_appendf(&ds, "\"\"");
      } else {
        size_t ofpt;
        char *typespec = NULL;
        bool first_arg = true;
        for (ofpt = 0; ofpt < array_len; ofpt++) {
          if (ofpt_table[ofpt].typ == (ofpt_table[ofpt].typ & ofpt_typ)) {
            ret = typ_to_str(&ofpt_table[ofpt].typ, &typespec);
            if (ret == LAGOPUS_RESULT_OK && IS_VALID_STRING(typespec) == true) {
              if (first_arg == true) {
                ret = lagopus_dstring_appendf(&ds, "\"%s\"", typespec);
                first_arg = false;
              } else {
                ret = lagopus_dstring_appendf(&ds, " \"%s\"", typespec);
              }
              if (ret != LAGOPUS_RESULT_OK) {
                lagopus_dstring_destroy(&ds);
                return ret;
              }
            } else {
              lagopus_dstring_destroy(&ds);
              return ret;
            }
          }
        }
      }

      if (ret == LAGOPUS_RESULT_OK) {
        if (IS_VALID_STRING(*ofpt_str) == true) {
          free(*ofpt_str);
          *ofpt_str = NULL;
        }
        ret = lagopus_dstring_str_get(&ds, ofpt_str);
      }
    }
    lagopus_dstring_destroy(&ds);
  }

  return ret;
}

static inline lagopus_result_t
s_parse_log_show(const char *cur_arg,
                 lagopus_log_destination_t cur_dst,
                 uint16_t cur_dbglvl,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint32_t trace_flags = 0;
  char *arg_esc = NULL;
  char *json_str = NULL;

  ret = datastore_json_string_escape(cur_arg, (char **)&arg_esc);
  if (ret == LAGOPUS_RESULT_OK) {

    trace_flags = lagopus_log_get_trace_flags();
    ret = ofpt_typespecs(trace_flags, &json_str);
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't convert uint32_t "
                                              "to json strings.");
      goto done;
    }

  } else {
    ret = datastore_json_result_string_setf(result, ret,
                                            "Can't convert cur_arg"
                                            "to an escaped strings.");
    goto done;
  }

  switch (cur_dst) {
    case LAGOPUS_LOG_EMIT_TO_UNKNOWN: {
      ret = datastore_json_result_setf(result, ret, "{"
                                       "\"debuglevel\":%u,\n"
                                       "\"packetdump\":%s"
                                         "}",
                                       cur_dbglvl,
                                       json_str);
      break;
    }
    case LAGOPUS_LOG_EMIT_TO_FILE: {
      ret = datastore_json_result_setf(result, ret, "{"
                                       "\"file\":\"%s\",\n"
                                       "\"debuglevel\":%u,\n"
                                       "\"packetdump\":%s"
                                       "}",
                                       arg_esc,
                                       cur_dbglvl,
                                       json_str);
      break;
    }
    case LAGOPUS_LOG_EMIT_TO_SYSLOG: {
      ret = datastore_json_result_setf(result, ret, "{"
                                       "\"ident\":\"%s\",\n"
                                       "\"debuglevel\":%u,\n"
                                       "\"packetdump\":%s"
                                       "}",
                                       arg_esc,
                                       cur_dbglvl,
                                       json_str);
      break;
    }
  }

done:
  free((void *)arg_esc);
  free((void *)json_str);

  return ret;
}

static inline lagopus_result_t
s_parse_log_internal(datastore_interp_t *iptr,
                     datastore_interp_state_t state,
                     const char *const argv[],
                     const char *cur_arg,
                     lagopus_log_destination_t cur_dst,
                     uint16_t cur_dbglvl,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *new_arg = (IS_VALID_STRING(cur_arg) == true) ?
                        strdup(cur_arg) : NULL;
  uint16_t new_dbglvl = cur_dbglvl;
  lagopus_log_destination_t new_dst = cur_dst;
  uint32_t trace_flags = 0;
  char c;

  if (IS_VALID_STRING(cur_arg) == true &&
      IS_VALID_STRING(new_arg) == false) {
    if (cur_dst != LAGOPUS_LOG_EMIT_TO_UNKNOWN) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Must not happen. "
                                              "The seems not"
                                              "initialized properly.");
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_NO_MEMORY,
                                              "Can't get log_destination_t.");
    }
    goto done;
  }

  while (IS_VALID_STRING(*argv) == true) {

    if (strcmp(*argv, "-syslog") == 0) {
      new_dst = LAGOPUS_LOG_EMIT_TO_SYSLOG;
      ret = LAGOPUS_RESULT_OK;

    } else if (strcmp(*argv, "-file") == 0) {

      if (IS_VALID_STRING(*(argv + 1)) == true) {
        argv++;
        free((void *)new_arg);
        new_arg = NULL;
        if ((ret = lagopus_str_unescape(*argv, "\"'",
                                        (char **)&new_arg)) > 0) {
          if (new_arg != NULL && strlen(new_arg) < PATH_MAX) {
            new_dst = LAGOPUS_LOG_EMIT_TO_FILE;
            ret = LAGOPUS_RESULT_OK;
          } else {
            ret = datastore_json_result_string_setf(result,
                                                    LAGOPUS_RESULT_TOO_LONG,
                                                    "Can't replace "
                                                    "to new file.");
            free((void *)new_arg);
            new_arg = NULL;
            goto done;
          }
        } else if (ret == 0) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "Bad opt val = %s.",
                                                  *argv);
          goto done;
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't convert to an "
                                                  "unescaped strings.");
          goto done;
        }
      } else {
        if (cur_dst == LAGOPUS_LOG_EMIT_TO_FILE) {
          ret = datastore_json_result_setf(result, LAGOPUS_RESULT_OK,
                                           "{\"file\":\"%s\"}",
                                           cur_arg);
        } else {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "A file is not specified, or "
                                                  "current log destination is not "
                                                  "a file.");
        }
        goto done;
      }

    } else if (strcmp(*argv, "-ident") == 0) {

      if (IS_VALID_STRING(*(argv + 1)) == true) {
        if (new_dst == LAGOPUS_LOG_EMIT_TO_SYSLOG) {
          argv++;
          free((void *)new_arg);
          new_arg = NULL;
          if ((ret = lagopus_str_unescape(*argv, "\"'",
                                          (char **)&new_arg)) > 0) {
            if (strlen(new_arg) < PATH_MAX) {
              ret = LAGOPUS_RESULT_OK;
            } else {
              ret = datastore_json_result_string_setf(result,
                                                      LAGOPUS_RESULT_TOO_LONG,
                                                      "Can't set ident.");
              free((void *)new_arg);
              new_arg = NULL;
              goto done;
            }
          } else if (ret == 0) {
            ret = datastore_json_result_string_setf(result,
                                                    LAGOPUS_RESULT_INVALID_ARGS,
                                                    "Bad opt val = %s.",
                                                    *argv);
            goto done;
          } else {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "Can't convert to an "
                                                    "unescaped strings.");
            goto done;
          }
        } else {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "The log destination is not the "
                                                  "syslog.");
          goto done;
        }
      } else {
        if (cur_dst == LAGOPUS_LOG_EMIT_TO_SYSLOG) {
          ret = datastore_json_result_setf(result, LAGOPUS_RESULT_OK,
                                           "{\"ident\":\"%s\"}",
                                           cur_arg);
        } else {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_INVALID_ARGS,
                                                  "An ident is not specified, or "
                                                  "current log destination is not "
                                                  "the syslog.");
        }
        goto done;
      }

    } else if (strcmp(*argv, "-debuglevel") == 0) {
      if (IS_VALID_STRING(*(argv + 1)) == true) {
        union cmd_uint cmd_uint;
        argv++;
        if ((ret = cmd_uint_parse(*argv, CMD_UINT64, &cmd_uint)) ==
            LAGOPUS_RESULT_OK) {
          if (cmd_uint.uint64 <= MAXIMUM_DBGLVL) {
            new_dbglvl = (uint16_t) cmd_uint.uint64;
          } else {
            ret = datastore_json_result_string_setf(result, LAGOPUS_RESULT_TOO_LONG,
                                                    "Can't set debuglevel.");
            goto done;
          }
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't parse %s as a "
                                                  "uint16_t integer.",
                                                  *argv);
          goto done;
        }
      } else {
        ret = datastore_json_result_setf(result, LAGOPUS_RESULT_OK,
                                         "{\"debuglevel\":%u}",
                                         cur_dbglvl);
        goto done;
      }

    } else if (strcmp(*argv, "-packetdump") == 0) {

      if (IS_VALID_STRING(*(argv + 1)) == true) {
        argv++;
        trace_flags = lagopus_log_get_trace_flags();
        while (IS_VALID_STRING(*argv) == true) {
          uint32_t *ofpt_typ = NULL;
          ret = str_to_typ(*argv, &ofpt_typ);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = str_to_typ(*argv + 1, &ofpt_typ);
          }

          if (ret == LAGOPUS_RESULT_OK) {
            c = **argv;
            if (c == '-' || c == '~') {
              trace_flags &= ~(*ofpt_typ);
            } else {
              trace_flags |= (*ofpt_typ);
            }
          } else {
            ret = datastore_json_result_string_setf(result, ret,
                                                    "packetdump option:%s",
                                                    *argv);
            goto done;
          }
          argv++;
        }

        if (state != DATASTORE_INTERP_STATE_DRYRUN) {
          lagopus_log_unset_trace_flags(TRACE_FULL);
          lagopus_log_set_trace_flags(trace_flags);
        }

        ret = lagopus_dstring_appendf(result, "{\"ret\":\"OK\"}");
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't add strings to dstring.");
        }
        argv--;

      } else {

        char *json_str = NULL;
        trace_flags = lagopus_log_get_trace_flags();
        ret = ofpt_typespecs(trace_flags, &json_str);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_setf(result, LAGOPUS_RESULT_OK,
                                           "%s", json_str);
        } else {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't convert uint32_t "
                                                  "to json strings.");
        }
        free((void *)json_str);
      }

      goto done;

    } else if (strcmp(*argv, "emit") == 0) {
      /*
       * Emit a message to the log, as a bookmark.
       */
      const char *filename = NULL;
      size_t lineno = 0;
      const char *msg = "";
      datastore_config_type_t type = DATASTORE_CONFIG_TYPE_UNKNOWN;

      if (IS_VALID_STRING(*(argv + 1)) == true) {
        argv++;
        msg = *argv;
      } else if (*(argv + 1) == NULL) {
        ret = datastore_json_result_string_setf(result, LAGOPUS_RESULT_INVALID_ARGS,
                                                "A message is not specified.");
        goto done;
      }

      if (state != DATASTORE_INTERP_STATE_DRYRUN) {
        if (datastore_interp_get_current_file_context(iptr,
                                                      &filename,
                                                      &lineno,
                                                      NULL,
                                                      NULL,
                                                      NULL,
                                                      NULL,
                                                      &type) ==
            LAGOPUS_RESULT_OK &&
            type == DATASTORE_CONFIG_TYPE_FILE) {
          if (IS_VALID_STRING(filename) == true) {
            lagopus_msg("%s: line " PFSZ(u) ": %s\n", filename, lineno, msg);
          } else {
            lagopus_msg("%s\n", msg);
          }
        } else {
          lagopus_msg("%s\n", msg);
        }
      }
      ret = LAGOPUS_RESULT_OK;

    } else {
      ret = datastore_json_result_string_setf(result, LAGOPUS_RESULT_NOT_FOUND,
                                              "option = %s",
                                              *argv);
      goto done;

    }

    argv++;
  }

  if (ret == LAGOPUS_RESULT_OK) {
    /*
     * Final validation.
     */
    if (IS_VALID_STRING(new_arg) == false) {
      if (new_dst != LAGOPUS_LOG_EMIT_TO_UNKNOWN) {
        /*
         * The new_arg must be set.
         */
        if (new_dst == LAGOPUS_LOG_EMIT_TO_SYSLOG) {
          /*
           * Try to fetch the command name and use it as the syslog ident.
           */
          const char *argv0 = lagopus_get_command_name();
          if (IS_VALID_STRING(argv0) == true) {
            free((void *)new_arg);
            new_arg = strdup(argv0);
            if (new_arg == NULL) {
              ret = datastore_json_result_string_setf(result,
                                                      LAGOPUS_RESULT_NO_MEMORY,
                                                      "Can't duplicate a string.");
              goto done;
            }
          } else {
            ret = datastore_json_result_string_setf(result, LAGOPUS_RESULT_INVALID_ARGS,
                                                    "An identifier for ths syslog "
                                                    "must be set. Use the -ident "
                                                    "option to set it.");
            goto done;
          }

        } else {
          ret = datastore_json_result_string_setf(result, LAGOPUS_RESULT_INVALID_ARGS,
                                                  "A logfile name "
                                                  "must be set. Use the -file "
                                                  "option to set it.");
          goto done;
        }
      }
    }

    /*
     * For now, the parsed result always causes side effects immediately.
     */
    if (state == DATASTORE_INTERP_STATE_DRYRUN) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      if ((cur_dst != new_dst) ||
          (cur_arg != NULL && new_arg != NULL &&
           strcmp(cur_arg, new_arg) != 0) ||
          (cur_dbglvl != new_dbglvl)) {
        ret = lagopus_log_initialize(new_dst, new_arg,
                                     false, true, new_dbglvl);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't (re-)initialize the "
                                                  "logger.");
          goto done;
        }
      }
    }
  }

  /*
   * Failsafe.
   */
  if (ret == LAGOPUS_RESULT_OK) {
    ret = lagopus_dstring_appendf(result, "{\"ret\":\"OK\"}");
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't add strings to dstring.");
    }
  }

done:
  free((void *)new_arg);

  return ret;
}

static lagopus_result_t
s_parse_log(datastore_interp_t *iptr,
            datastore_interp_state_t state,
            size_t argc, const char *const argv[],
            lagopus_hashmap_t *hptr,
            datastore_update_proc_t u_proc,
            datastore_enable_proc_t e_proc,
            datastore_serialize_proc_t s_proc,
            datastore_destroy_proc_t d_proc,
            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *cur_arg = NULL;
  lagopus_log_destination_t cur_dst = lagopus_log_get_destination(&cur_arg);
  uint16_t cur_dbglvl = lagopus_log_get_debug_level();
  size_t i;
  (void)iptr;
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
    /*
     * Returns the current log parameters.
     */
    ret = s_parse_log_show(cur_arg, cur_dst, cur_dbglvl, result);
  } else {
    ret = s_parse_log_internal(iptr, state, argv, cur_arg,
                               cur_dst, cur_dbglvl, result);
  }

  return ret;
}

lagopus_result_t
log_cmd_serialize(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* syslog/file */
  const char *dst_str = NULL;
  lagopus_log_destination_t dst = lagopus_log_get_destination(&dst_str);
  char *escaped_dst_str = NULL;

  /* debuglevel */
  uint16_t debug_level = lagopus_log_get_debug_level();

  /* packetdump */
  uint32_t trace_flags = lagopus_log_get_trace_flags();
  char *trace_flags_str = NULL;

  bool is_escaped = false;

  if (result != NULL) {
    /* cmmand name. */
    if ((ret = lagopus_dstring_appendf(result, LOG_CMD_NAME)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    /* syslog/file opt. */
    switch (dst) {
      case LAGOPUS_LOG_EMIT_TO_FILE: {
        if ((ret = lagopus_dstring_appendf(result, " -file")) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_str_escape(dst_str, "\"",
                                        &is_escaped,
                                        &escaped_dst_str)) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(
                         result,
                         ESCAPE_NAME_FMT(is_escaped, escaped_dst_str),
                         escaped_dst_str)) !=
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

        break;
      }
      case LAGOPUS_LOG_EMIT_TO_SYSLOG: {
        if ((ret = lagopus_dstring_appendf(result, " -syslog -ident")) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_str_escape(dst_str, "\"",
                                        &is_escaped,
                                        &escaped_dst_str)) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(
                         result,
                         ESCAPE_NAME_FMT(is_escaped, escaped_dst_str),
                         escaped_dst_str)) !=
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

        break;
      }
      default: {
        break;
      }
    }

    /* debuglevel opt. */
    if ((ret = lagopus_dstring_appendf(result, " -debuglevel")) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %d",
                                         debug_level)) !=
          LAGOPUS_RESULT_OK) {
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

    /* packetdump opt. */
    if ((ret = ofpt_typespecs_serialize(trace_flags, &trace_flags_str)) ==
        LAGOPUS_RESULT_OK) {
      if (IS_VALID_STRING(trace_flags_str) == true) {
        /* cmmand name. */
        if ((ret = lagopus_dstring_appendf(result, LOG_CMD_NAME)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        if ((ret = lagopus_dstring_appendf(result, " -packetdump")) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " %s",
                                             trace_flags_str)) !=
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
    if ((ret = lagopus_dstring_appendf(result, "\n\n")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

  done:
    free((void *) escaped_dst_str);
    free((void *) trace_flags_str);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





#undef RESULT_NULL




