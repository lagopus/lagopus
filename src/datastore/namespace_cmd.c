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

#include "namespace_cmd.h"

#define NAMESPACE_CMD_NAME "namespace"

static lagopus_hashmap_t s_namespace_table = NULL;

static inline lagopus_result_t
s_namespace_add(const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  void *val = NULL;

  if (s_namespace_table == NULL) {
    return LAGOPUS_RESULT_NOT_STARTED;
  } else if (namespace == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  } else if (strstr(namespace, DATASTORE_NAMESPACE_DELIMITER) != NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  } else if (strcmp(namespace, DATASTORE_DRYRUN_NAMESPACE) == 0) {
    return LAGOPUS_RESULT_INVALID_NAMESPACE;
  } else if (strlen(namespace) > DATASTORE_NAMESPACE_MAX) {
    return LAGOPUS_RESULT_TOO_LONG;
  }

  ret = lagopus_hashmap_find(&s_namespace_table,
                             (void *) namespace, &val);

  if (ret == LAGOPUS_RESULT_NOT_FOUND) {
    ret = lagopus_hashmap_add(&s_namespace_table,
                              (void *) namespace,
                              (void **)&namespace,
                              false);
  } else if (ret == LAGOPUS_RESULT_OK) {
    ret = LAGOPUS_RESULT_ALREADY_EXISTS;
  }

  return ret;
}

typedef struct {
  char **m_objs;
  size_t m_n_objs;
  size_t m_idx;
} namespace_iter_ctx_t;

static bool
s_namespace_iterate(void *key, void *val, lagopus_hashentry_t he,
                    void *arg) {
  namespace_iter_ctx_t *ctx = (namespace_iter_ctx_t *) arg;

  (void)val;
  (void)he;

  if (ctx->m_idx < ctx->m_n_objs) {
    ctx->m_objs[ctx->m_idx++] = (char *) key;
    return true;
  } else {
    return false;
  }
}

static inline lagopus_result_t
s_namespace_list(char ***listp) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t n_objs = 0;

  if (s_namespace_table == NULL) {
    return LAGOPUS_RESULT_NOT_STARTED;
  }

  if (listp != NULL) {
    *listp = NULL;

    n_objs = (size_t) lagopus_hashmap_size(&s_namespace_table);
    if (n_objs > 0) {
      if ((*listp = (char **)malloc(sizeof(char *) * n_objs)) != NULL) {
        namespace_iter_ctx_t ctx;
        ctx.m_objs = *listp;
        ctx.m_n_objs = n_objs;
        ctx.m_idx= 0;
        ret = lagopus_hashmap_iterate(&s_namespace_table,
                                      s_namespace_iterate,
                                      (void *) &ctx);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = (lagopus_result_t) n_objs;
        } else {
          free((void *) *listp);
          *listp = NULL;
        }
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
s_namespace_list_str(char **list_str, bool json_escape) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int i = 0;
  char **listp = NULL;
  int n_objs = 0;
  lagopus_dstring_t ds = NULL;
  char *escaped_namespace = NULL;
  char *json_escaped_namespace = NULL;
  bool is_escaped = false;

  ret = s_namespace_list(&listp);
  if (ret > 0) {
    n_objs = (int) ret;

    ret = lagopus_dstring_create(&ds);
    if (ret == LAGOPUS_RESULT_OK) {
      for (i = 0; i < n_objs; i++) {
        if (json_escape == true) {
          if ((ret = lagopus_str_escape(listp[i],
                                        "\"'",
                                        &is_escaped,
                                        &escaped_namespace))
              == LAGOPUS_RESULT_OK) {
            if ((ret = datastore_json_string_escape(escaped_namespace,
                                                    &json_escaped_namespace)) ==
                LAGOPUS_RESULT_OK) {
              if ((i + 1) != n_objs) {
                ret = lagopus_dstring_appendf(&ds,
                                              "\"%s\", ",
                                              json_escaped_namespace);
              } else {
                ret = lagopus_dstring_appendf(&ds,
                                              "\"%s\"",
                                              json_escaped_namespace);
              }
            } else {
              goto done;
            }
          } else {
            goto done;
          }
        } else {
          if ((i + 1) != n_objs) {
            ret = lagopus_dstring_appendf(&ds, "\"%s\", ", listp[i]);
          } else {
            ret = lagopus_dstring_appendf(&ds, "\"%s\"", listp[i]);
          }
        }

        if (ret != LAGOPUS_RESULT_OK) {
          goto done;
        }

        free((void *) escaped_namespace);
        escaped_namespace = NULL;
        free((void *) json_escaped_namespace);
        json_escaped_namespace = NULL;
      }
    } else {
      goto done;
    }

    ret = lagopus_dstring_str_get(&ds, list_str);
  } else if (ret == 0) {
    *list_str = (char *) malloc(sizeof(char));
    if (*list_str != NULL) {
      *list_str[0] = '\0';
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }
  }

done:
  free((void *) escaped_namespace);
  free((void *) json_escaped_namespace);
  free((void *) listp);
  if (ds != NULL) {
    (void)lagopus_dstring_clear(&ds);
    (void)lagopus_dstring_destroy(&ds);
  }
  return ret;
}

static void
s_namespace_val_freeup(void *val) {
  free((char *) val);
}

static inline lagopus_result_t
s_namespace_current_namespace_opt_parse(datastore_interp_state_t state,
                                        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *namespace = NULL;
  char *escaped_namespace = NULL;
  char *json_escaped_namespace = NULL;
  bool is_escaped = false;
  char *list_str = NULL;

  (void)lagopus_dstring_clear(result);

  if (state == DATASTORE_INTERP_STATE_DRYRUN) {
    ret = namespace_get_saved_namespace(&namespace);
  } else {
    ret = namespace_get_current_namespace(&namespace);
  }

  if (ret == LAGOPUS_RESULT_OK && namespace != NULL) {
    if (namespace[0] == '\0') {
      ret = lagopus_dstring_appendf(result,
                                    "{\"ret\":\"OK\",\n"
                                    "\"current\":\"\", ");
      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_setf(result,
                                         LAGOPUS_RESULT_ANY_FAILURES,
                                         "dstring append failed.");
        goto done;
      }
    } else {
      if ((ret = lagopus_str_escape(namespace,
                                    "\"'",
                                    &is_escaped,
                                    &escaped_namespace)) == LAGOPUS_RESULT_OK) {
        if ((ret = datastore_json_string_escape(escaped_namespace,
                                                &json_escaped_namespace)) ==
            LAGOPUS_RESULT_OK) {
          ret = lagopus_dstring_appendf(result,
                                        "{\"ret\":\"OK\",\n"
                                        "\"current\":\"%s\", ",
                                        json_escaped_namespace);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_setf(result,
                                             LAGOPUS_RESULT_ANY_FAILURES,
                                             "dstring append failed.");
            goto done;
          }
        } else {
          ret = datastore_json_result_setf(result,
                                           LAGOPUS_RESULT_NOT_DEFINED,
                                           "Escape processing failed.");
          goto done;
        }
      } else {
        ret = datastore_json_result_setf(result,
                                         LAGOPUS_RESULT_NOT_DEFINED,
                                         "Escape processing failed.");
        goto done;
      }
    }
  } else {
    ret = datastore_json_result_setf(result,
                                     LAGOPUS_RESULT_ANY_FAILURES,
                                     "Can't get namespace.");
    goto done;
  }

  ret = s_namespace_list_str(&list_str, true);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = lagopus_dstring_appendf(result,
                                  "\"all\":[%s]}",
                                  list_str);
  } else {
    ret = datastore_json_result_setf(result,
                                     ret,
                                     "Can't get namespace list.");
    goto done;
  }

done:
  free((void *) namespace);
  free((void *) escaped_namespace);
  free((void *) json_escaped_namespace);
  free((void *) list_str);

  return ret;
}

static inline lagopus_result_t
s_namespace_add_opt_parse(const char *const argv[],
                          datastore_interp_state_t state,
                          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *namespace = NULL;

  if (s_namespace_table != NULL) {
    if (IS_VALID_STRING(*argv) == true) {
      if ((ret = lagopus_str_unescape(*argv, "\"'", &namespace)) >= 0) {
        if (state == DATASTORE_INTERP_STATE_DRYRUN) {
          ret = LAGOPUS_RESULT_OK;
          return datastore_json_result_set(result, ret, NULL);
        } else {
          ret = s_namespace_add(namespace);

          if (ret == LAGOPUS_RESULT_OK) {
            return datastore_json_result_set(result, ret, NULL);
          } else {
            ret = datastore_json_result_string_setf(result,
                                                    ret,
                                                    "Can't add = %s",
                                                    *argv);
            goto error;
          }
        }
      } else {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_INVALID_ARGS,
                                                "Unescape processing failed.");
        goto error;
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Bad opt value = %s",
                                              *argv);
      goto error;
    }
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_NOT_STARTED, NULL);
    goto error;
  }

error:
  free((void *) namespace);
  namespace = NULL;
  return ret;
}

static inline lagopus_result_t
s_namespace_set_opt_parse(const char *const argv[],
                          datastore_interp_state_t state,
                          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *namespace = NULL;
  void *val = NULL;

  if (s_namespace_table == NULL) {
    return datastore_json_result_setf(result,
                                      LAGOPUS_RESULT_NOT_STARTED, NULL);
  }

  if (IS_VALID_STRING(*argv) == true) {
    if ((ret = lagopus_str_unescape(*argv, "\"'", &namespace)) >= 0) {
      ret = lagopus_hashmap_find(&s_namespace_table,
                                 (void *) namespace, &val);

      if (ret == LAGOPUS_RESULT_OK) {
        if (state == DATASTORE_INTERP_STATE_DRYRUN) {
          ret = LAGOPUS_RESULT_OK;
          return datastore_json_result_set(result, ret, NULL);
        } else {
          ret = s_set_current_namespace(namespace);

          if (ret == LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_set(result, ret, NULL);
          } else {
            ret = datastore_json_result_string_setf(result,
                                                    LAGOPUS_RESULT_INVALID_ARGS,
                                                    "Bad opt value = %s",
                                                    *argv);
          }
        }
      } else if (ret == LAGOPUS_RESULT_NOT_FOUND) {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_NOT_FOUND,
                                                "Bad opt value = %s",
                                                *argv);
      } else {
        ret = datastore_json_result_setf(result, ret, NULL);
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Unescape processing failed.");
    }
  } else {
    ret = datastore_json_result_string_setf(result,
                                            LAGOPUS_RESULT_INVALID_ARGS,
                                            "Bad opt value = %s",
                                            *argv);
  }

  free((void *) namespace);
  return ret;
}

static inline lagopus_result_t
s_namespace_delete_opt_parse(datastore_interp_t *iptr,
                             datastore_interp_state_t state,
                             const char *const argv[],
                             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *namespace = NULL;
  char *current_namespace = NULL;
  void *val = NULL;

  if (s_namespace_table == NULL) {
    return datastore_json_result_setf(result,
                                      LAGOPUS_RESULT_NOT_STARTED, NULL);
  }

  if (IS_VALID_STRING(*argv) == true) {
    if ((ret = lagopus_str_unescape(*argv, "\"'", &namespace)) >= 0) {
      ret = lagopus_hashmap_find(&s_namespace_table,
                                 (void *) namespace, &val);

      if (state == DATASTORE_INTERP_STATE_DRYRUN) {
        ret = LAGOPUS_RESULT_OK;
        return datastore_json_result_set(result, ret, NULL);
      } else {
        if (ret == LAGOPUS_RESULT_OK) {
          ret = namespace_get_current_namespace(&current_namespace);
          if (ret != LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_setf(result,
                                             LAGOPUS_RESULT_ANY_FAILURES,
                                             "Can't get namespace.");
            goto done;
          }

          if (strcmp(namespace, current_namespace) == 0) {
            if ((ret = s_unset_current_namespace()) == LAGOPUS_RESULT_OK) {
              lagopus_msg_info("unset %s.", namespace);
            } else {
              ret = datastore_json_result_string_setf(result, ret,
                                                    "Unset processing failed.");
              goto done;
            }
          }

          ret = datastore_interp_destroy_obj(iptr, namespace, result);
          if (ret == LAGOPUS_RESULT_OK) {
            lagopus_msg_info("destroy %s objects.", namespace);
          } else {
            ret = datastore_json_result_string_setf(result,
                                                    ret,
                                                    "destroy object failed.");
            goto done;
          }

          ret = lagopus_hashmap_delete(&s_namespace_table, (void *) namespace,
                                       NULL, true);
          if (ret == LAGOPUS_RESULT_OK) {
            ret = datastore_json_result_set(result, ret, NULL);
          } else {
            ret = datastore_json_result_string_setf(result,
                                                    LAGOPUS_RESULT_INVALID_ARGS,
                                                    "Bad opt value = %s",
                                                    *argv);
          }
        } else if (ret == LAGOPUS_RESULT_NOT_FOUND) {
          ret = datastore_json_result_string_setf(result,
                                                  LAGOPUS_RESULT_NOT_FOUND,
                                                  "Bad opt value = %s",
                                                  *argv);
        } else {
          ret = datastore_json_result_setf(result, ret, NULL);
        }
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Unescape processing failed.");
    }
  } else {
    ret = datastore_json_result_string_setf(result,
                                            LAGOPUS_RESULT_INVALID_ARGS,
                                            "Bad opt value = %s",
                                            *argv);
  }

done:
  free((void *) namespace);
  free((void *) current_namespace);
  return ret;
}

static inline lagopus_result_t
s_namespace_unset_opt_parse(datastore_interp_state_t state,
                            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (state == DATASTORE_INTERP_STATE_DRYRUN) {
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = s_unset_current_namespace();
  }

  return datastore_json_result_set(result, ret, NULL);
}

static inline lagopus_result_t
s_parse_namespace(datastore_interp_t *iptr,
                  datastore_interp_state_t state,
                  size_t argc, const char *const argv[],
                  lagopus_hashmap_t *hptr,
                  datastore_update_proc_t u_proc,
                  datastore_enable_proc_t e_proc,
                  datastore_serialize_proc_t s_proc,
                  datastore_destroy_proc_t d_proc,
                  lagopus_dstring_t *result) {
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

  if (argc >= 2) {
    argv++;

    if (IS_VALID_STRING(*argv) == true) {
      if (strcmp(*argv, "add") == 0) {
        if (argc == 3) {
          argv++;
          return s_namespace_add_opt_parse(argv, state, result);
        } else {
          return datastore_json_result_string_setf(result,
                 LAGOPUS_RESULT_INVALID_ARGS,
                 "invalid option value.");
        }
      } else if (strcmp(*argv, "set") == 0) {
        if (argc == 3) {
          argv++;
          return s_namespace_set_opt_parse(argv, state, result);
        } else {
          return datastore_json_result_string_setf(result,
                 LAGOPUS_RESULT_INVALID_ARGS,
                 "invalid option value.");
        }
      } else if (strcmp(*argv, "delete") == 0) {
        if (argc == 3) {
          argv++;
          return s_namespace_delete_opt_parse(iptr, state, argv, result);
        } else {
          return datastore_json_result_string_setf(result,
                 LAGOPUS_RESULT_INVALID_ARGS,
                 "invalid option value.");
        }
      } else if (strcmp(*argv, "unset") == 0) {
        if (argc == 2) {
          argv++;
          return s_namespace_unset_opt_parse(state, result);
        } else {
          return datastore_json_result_string_setf(result,
                 LAGOPUS_RESULT_INVALID_ARGS,
                 "invalid option value.");
        }
      }
    }

    return datastore_json_result_string_setf(result,
           LAGOPUS_RESULT_INVALID_ARGS,
           "Unknown option '%s'",
           *argv);
  } else {
    return s_namespace_current_namespace_opt_parse(state, result);
  }
}

lagopus_result_t
namespace_cmd_initialize(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* set default value */
  s_unset_current_namespace();

  /* create namespace table */
  if ((ret = lagopus_hashmap_create(&s_namespace_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    s_namespace_val_freeup))
      != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    lagopus_exit_fatal("Can't create namespace table.\n");
  }

  /* register namespace command */
  if ((ret = datastore_interp_register_command(&s_interp, CONFIGURATOR_NAME,
           "namespace", s_parse_namespace)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    lagopus_exit_fatal("Can't regsiter the namespace command.\n");
  }

  return LAGOPUS_RESULT_OK;
}

void
namespace_cmd_finalize(void) {
  if (s_namespace_table != NULL) {
    lagopus_hashmap_destroy(&s_namespace_table, true);
    s_namespace_table = NULL;
  }
}

bool
namespace_exists(const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  void *val = NULL;

  if (s_namespace_table == NULL) {
    lagopus_msg_warning("namespace not initialized.");
    return false;
  } else if (namespace == NULL) {
    lagopus_msg_warning("namespace invalid args.");
    return false;
  } else {
    if (namespace[0] == '\0') {
      return true;
    } else {
      ret = lagopus_hashmap_find(&s_namespace_table,
                                 (void *) namespace, &val);

      if (ret == LAGOPUS_RESULT_NOT_FOUND) {
        return false;
      } else if (ret == LAGOPUS_RESULT_OK) {
        return true;
      } else {
        lagopus_perror(ret);
        return false;
      }
    }
  }
}

lagopus_result_t
namespace_cmd_serialize(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int i = 0;
  int n_objs = 0;

  /* namespace */
  char **listp = NULL;
  char *escaped_namespace = NULL;

  /* current */
  char *current_namespace = NULL;
  char *escaped_current_namespace = NULL;

  bool is_escaped = false;

  if (result != NULL) {
    ret = s_namespace_list(&listp);

    if (ret > 0) {
      n_objs = (int) ret;

      for (i = 0; i < n_objs; i++) {
        if ((ret = lagopus_dstring_appendf(result, NAMESPACE_CMD_NAME)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        if ((ret = lagopus_str_escape(listp[i],
                                      "\"'",
                                      &is_escaped,
                                      &escaped_namespace))
            == LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_dstring_appendf(result, " add")) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(
                         result,
                         ESCAPE_NAME_FMT(is_escaped, escaped_namespace),
                         escaped_namespace)) !=
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
        } else {
          lagopus_perror(ret);
          goto done;
        }

        free((void *) escaped_namespace);
        escaped_namespace = NULL;
      }
    }

    ret = namespace_get_current_namespace(&current_namespace);
    if (ret == LAGOPUS_RESULT_OK) {
      if (current_namespace[0] != '\0') {
        if ((ret = lagopus_dstring_appendf(result, NAMESPACE_CMD_NAME)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        if (IS_VALID_STRING(current_namespace) == true) {
          if ((ret = lagopus_str_escape(current_namespace, "\"",
                                        &is_escaped,
                                        &escaped_current_namespace)) ==
              LAGOPUS_RESULT_OK) {
            if ((ret = lagopus_dstring_appendf(result, " set")) ==
                LAGOPUS_RESULT_OK) {
              if ((ret = lagopus_dstring_appendf(
                           result,
                           ESCAPE_NAME_FMT(is_escaped, escaped_current_namespace),
                           escaped_current_namespace)) !=
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
    free((void *) listp);
    free((void *) escaped_namespace);
    free((void *) current_namespace);
    free((void *) escaped_current_namespace);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

