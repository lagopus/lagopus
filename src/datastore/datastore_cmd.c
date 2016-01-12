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



#define DATASTORE_CMD_NAME "datastore"
#define RESULT_NULL	"\"\""





static inline lagopus_result_t
s_current_all(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *bindaddr_str = NULL;
  const char *protocol = NULL;
  const char *tls_str = NULL;

  ret = lagopus_ip_address_str_get(s_bindaddr, &bindaddr_str);
  if (ret == LAGOPUS_RESULT_OK) {
    if (session_type_is_tcp(s_protocol) == true) {
      protocol = "tcp";
    } else if (session_type_is_tcp6(s_protocol) == true) {
      protocol = "tcp6";
    } else {
      return datastore_json_result_setf(result,
                                        LAGOPUS_RESULT_NOT_DEFINED,
                                        "Can't get protocol value: %x",
                                        s_protocol);
    }

    if (s_tls == true) {
      tls_str = "true";
    } else {
      tls_str = "false";
    }

    ret = datastore_json_result_setf(result,
                                     LAGOPUS_RESULT_OK,
                                     "[{\"addr\":\"%s\",\n"
                                     "\"port\":\"%u\",\n"
                                     "\"protocol\":\"%s\",\n"
                                     "\"tls\":\"%s\"}]",
                                     bindaddr_str,
                                     s_port,
                                     protocol,
                                     tls_str);

    free((void *) bindaddr_str);
    return ret;
  } else {
    return datastore_json_result_setf(result,
                                      ret,
                                      "Can't get addr value.");
  }
}

static inline lagopus_result_t
s_current_addr(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *bindaddr_str = NULL;

  ret = lagopus_ip_address_str_get(s_bindaddr, &bindaddr_str);
  if (ret != LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_setf(result,
                                     ret,
                                     "Can't get addr.");
  }
  ret = datastore_json_result_setf(result, ret,
                                   "[{\"addr\":\"%s\"}]",
                                   bindaddr_str);

  free((void *) bindaddr_str);
  return ret;
}

static inline lagopus_result_t
s_current_port(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = datastore_json_result_setf(result,
                                   LAGOPUS_RESULT_OK,
                                   "[{\"port\":%u}]",
                                   s_port);

  return ret;
}

static inline lagopus_result_t
s_current_protocol(lagopus_dstring_t *result) {
  const char *protocol = NULL;

  if (session_type_is_tcp(s_protocol) == true) {
    protocol = "tcp";
  } else if (session_type_is_tcp6(s_protocol) == true) {
    protocol = "tcp6";
  } else {
    return datastore_json_result_setf(result,
                                      LAGOPUS_RESULT_NOT_DEFINED,
                                      "Can't get protocol value: %x",
                                      s_protocol);
  }

  return datastore_json_result_setf(result,
                                    LAGOPUS_RESULT_OK,
                                    "[{\"protocol\":\"%s\"}]",
                                    protocol);
}

static inline lagopus_result_t
s_current_tls(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *tls_str = NULL;

  if (s_tls == true) {
    tls_str = "true";
  } else {
    tls_str = "false";
  }
  ret = datastore_json_result_setf(result,
                                   LAGOPUS_RESULT_OK,
                                   "[{\"tls\":\"%s\"}]",
                                   tls_str);

  return ret;
}

static inline lagopus_result_t
s_opt_parse_addr(datastore_interp_state_t state,
                 const char *const argv[], lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(*argv) == true) {
    if (state == DATASTORE_INTERP_STATE_DRYRUN) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = s_set_addr(*argv);
    }

    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Bad opt value = %s",
                                              *argv);
    }
  } else {
    ret = datastore_json_result_string_setf(result,
                                            LAGOPUS_RESULT_INVALID_ARGS,
                                            "Bad opt value = %s",
                                            *argv);
  }
  return ret;
}

static inline lagopus_result_t
s_opt_parse_port(datastore_interp_state_t state,
                 const char *const argv[], lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t tmp = 0;

  if (IS_VALID_STRING(*argv) == true) {
    if ((ret = lagopus_str_parse_uint16(*argv, &tmp)) ==
        LAGOPUS_RESULT_OK) {
      if (state != DATASTORE_INTERP_STATE_DRYRUN) {
        s_port = tmp;
      }
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "\"can't parse '%s' as a "
                                              "uint16_t integer.\"",
                                              *argv);
    }
  } else {
    ret = datastore_json_result_string_setf(result,
                                            LAGOPUS_RESULT_INVALID_ARGS,
                                            "Bad opt value = %s",
                                            *argv);
  }
  return ret;
}

static inline lagopus_result_t
s_opt_parse_protocol(datastore_interp_state_t state,
                     const char *const argv[], lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(*argv) == true) {
    if (state == DATASTORE_INTERP_STATE_DRYRUN) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = s_set_protocol(*argv);
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_INVALID_ARGS,
                                                "Bad opt value = %s",
                                                *argv);
      }
    }
  } else {
    ret = datastore_json_result_string_setf(result,
                                            LAGOPUS_RESULT_INVALID_ARGS,
                                            "Bad opt value = %s",
                                            *argv);
  }
  return ret;
}

static inline lagopus_result_t
s_opt_parse_tls(datastore_interp_state_t state,
                const char *const argv[], lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool tls = false;

  if (IS_VALID_STRING(*argv) == true) {
    if (state == DATASTORE_INTERP_STATE_DRYRUN) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = lagopus_str_parse_bool(*argv, &tls);
      if (ret == LAGOPUS_RESULT_OK) {
        s_set_tls(tls);
      } else {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_INVALID_ARGS,
                                                "Bad opt value = %s",
                                                *argv);
      }
    }
  } else {
    ret = datastore_json_result_string_setf(result,
                                            LAGOPUS_RESULT_INVALID_ARGS,
                                            "Bad opt value = %s",
                                            *argv);
  }
  return ret;
}

static inline lagopus_result_t
s_parse_datastore(datastore_interp_t *iptr,
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

  if (argc == 1) {
    return s_current_all(result);
  } else {
    while (IS_VALID_STRING(*argv) == true) {
      if (strcmp(*argv, "-addr") == 0) {
        argv++;
        if (IS_VALID_STRING(*argv) == true) {
          ret = s_opt_parse_addr(state, argv, result);
          if (ret != LAGOPUS_RESULT_OK) {
            return ret;
          }
        } else {
          return s_current_addr(result);
        }
      } else if (strcmp(*argv, "-port") == 0) {
        argv++;
        if (IS_VALID_STRING(*argv) == true) {
          ret = s_opt_parse_port(state, argv, result);
          if (ret != LAGOPUS_RESULT_OK) {
            return ret;
          }
        } else {
          return s_current_port(result);
        }
      } else if (strcmp(*argv, "-protocol") == 0) {
        argv++;
        if (IS_VALID_STRING(*argv) == true) {
          ret = s_opt_parse_protocol(state, argv, result);
          if (ret != LAGOPUS_RESULT_OK) {
            return ret;
          }
        } else {
          return s_current_protocol(result);
        }
      } else if (strcmp(*argv, "-tls") == 0) {
        argv++;
        if (IS_VALID_STRING(*argv) == true) {
          ret = s_opt_parse_tls(state, argv, result);
          if (ret != LAGOPUS_RESULT_OK) {
            return ret;
          }
        } else {
          return s_current_tls(result);
        }
      } else {
        return datastore_json_result_string_setf(result,
               LAGOPUS_RESULT_INVALID_ARGS,
               "Unknown option '%s'",
               *argv);
      }
      argv++;
    }

    // setter result
    return datastore_json_result_set(result, ret, NULL);
  }
}

lagopus_result_t
datastore_cmd_serialize(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* addr */
  char *addr_str = NULL;
  char *escaped_addr_str = NULL;

  /* protocol */
  const char *protocol_str = NULL;
  char *escaped_protocol_str = NULL;

  /* tls */
  const char *tls_str = NULL;

  bool is_escaped = false;

  if (result != NULL) {
    /* cmmand name. */
    if ((ret = lagopus_dstring_appendf(result, DATASTORE_CMD_NAME)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    /* addr opt. */
    if ((ret = lagopus_ip_address_str_get(s_bindaddr,
                                          &addr_str)) ==
        LAGOPUS_RESULT_OK) {
      if (IS_VALID_STRING(addr_str) == true) {
        if ((ret = lagopus_str_escape(addr_str, "\"",
                                      &is_escaped,
                                      &escaped_addr_str)) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " -addr")) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(
                         result,
                         ESCAPE_NAME_FMT(is_escaped, escaped_addr_str),
                         escaped_addr_str)) !=
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

    /* port opt. */
    if ((ret = lagopus_dstring_appendf(result, " -port")) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %d",
                                         s_port)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* protocol opt. */
    if (session_type_is_tcp(s_protocol) == true) {
      protocol_str = "tcp";
    } else if (session_type_is_tcp6(s_protocol) == true) {
      protocol_str = "tcp6";
    } else {
      lagopus_perror(LAGOPUS_RESULT_ANY_FAILURES);
      goto done;
    }

    if (IS_VALID_STRING(protocol_str) == true) {
      if ((ret = lagopus_str_escape(protocol_str, "\"",
                                    &is_escaped,
                                    &escaped_protocol_str)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, " -protocol")) ==
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

    /* tls opt. */
    if ((ret = lagopus_dstring_appendf(result, " -tls")) ==
        LAGOPUS_RESULT_OK) {
      if (s_tls == true) {
        tls_str = "true";
      } else {
        tls_str = "false";
      }

      if ((ret = lagopus_dstring_appendf(result, " %s", tls_str)) !=
          LAGOPUS_RESULT_OK) {
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
    free(addr_str);
    free(escaped_addr_str);
    free(escaped_protocol_str);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}



#undef RESULT_NULL


