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

/**
 *      @file   route_cmd_dump.c
 *      @brief  DSL dump command parser for L3 route table management.
 */

#include "lagopus_apis.h"
#include "datastore_apis.h"
#include "datastore_internal.h"
#include "cmd_common.h"
#include "cmd_dump.h"
#include "route_cmd.h"
#include "route_cmd_internal.h"
#include "conv_json.h"
#include "lagopus/dp_apis.h"

#define TMP_FILE "/.lagopus_route_XXXXXX"
#define PORT_STR_BUF_SIZE 32

/* defines of route */
#define ETH_LEN 6

static lagopus_hashmap_t thread_table = NULL;
static lagopus_mutex_t lock = NULL;
static char dump_tmp_dir[PATH_MAX] = DATASTORE_TMP_DIR;

typedef struct args {
  route_conf_t *conf;
  datastore_interp_t *iptr;
  datastore_config_type_t ftype;
  void *stream_out;
  datastore_printf_proc_t printf_proc;
  char file_name[PATH_MAX];
} args_t;

static inline lagopus_result_t
dump_table_ipv4_routes(const char *name,
                       lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  void *item = NULL;
  int i = 0;
  char buf[BUFSIZ];
  char string[BUFSIZ];
  struct in_addr dest, gate;
  int prefixlen;
  uint32_t ifindex;

  while (1) {
    ret = rib_route_rule_get(name, &dest, &gate, &prefixlen, &ifindex, &item);
    if (ret != LAGOPUS_RESULT_OK || item == NULL) {
      break;
    }

    /* start */
    if ((ret = lagopus_dstring_appendf(
               result, DS_JSON_DELIMITER((i == 0), "{"))) !=
         LAGOPUS_RESULT_OK) {
       lagopus_perror(ret);
       goto done;
    }

    /* dest network */
    inet_ntop(AF_INET, &dest, buf, BUFSIZ);
    snprintf(string, BUFSIZ, "%s/%d", buf, prefixlen);
    if ((ret = datastore_json_string_append( result, "dest",
               string, false)) !=
         LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    /* gateway */
    inet_ntop(AF_INET, &gate, buf, BUFSIZ);
    snprintf(string, BUFSIZ, "%s/%d", buf, prefixlen);
    if ((ret = datastore_json_string_append( result, "gate",
               string, true)) !=
         LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    /* ifindex */
    if ((ret = datastore_json_uint32_append( result, "ifindex",
               ifindex, true)) !=
         LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    /* end */
    if ((ret = lagopus_dstring_appendf( result, "}")) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    i++;
  }

done:
  return ret;
}

/**
 * Create dump result of bridge information.
 */
STATIC lagopus_result_t
dump_bridge_route(const char *name,
                  bool is_bridge_first,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((ret = lagopus_dstring_appendf(result, "{")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* bridge name */
  if ((ret = lagopus_dstring_appendf(
               result, DS_JSON_DELIMITER(is_bridge_first, KEY_FMT "\"%s\""),
               "name", name)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* entries start. */
  if ((ret = lagopus_dstring_appendf(result, DELIMITER_INSTERN(KEY_FMT "["),
                                     "route_ipv4")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }


  /* dump a table. */
  if ((ret = dump_table_ipv4_routes(name, result)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    lagopus_dstring_appendf(result, "]}");
    goto done;
  }

  /* entries end. */
  if (ret == LAGOPUS_RESULT_OK) {
    if ((ret = lagopus_dstring_appendf(result, "]")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_dstring_appendf(result, "}");
      lagopus_perror(ret);
      goto done;
    }

    if ((ret = lagopus_dstring_appendf(result, "}")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  }

 done:

  return ret;
}

/**
 * Create dump information.
 */
static lagopus_result_t
route_cmd_dump(datastore_interp_t *iptr,
               void *c,
               FILE *fp,
               void *stream_out,
               datastore_printf_proc_t printf_proc,
               bool is_with_stats,
               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_bridge_first = true;
  datastore_name_info_t *names = NULL;
  struct datastore_name_entry *name = NULL;
  route_conf_t *conf = NULL;

  if (iptr != NULL && c != NULL && fp != NULL &&
      stream_out !=NULL && printf_proc != NULL &&
      result != NULL) {
    conf = (route_conf_t *) c;

    if ((ret = datastore_bridge_get_names(conf->name,
                                          &names)) !=
        LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't get bridge names.");
      goto done;
    }
    
    if (fputs("[", fp) == EOF) {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_OUTPUT_FAILURE,
                                              "Can't write file.");
      goto done;
    }

    /* dump bridge(s). */
    if (TAILQ_EMPTY(&names->head) == false) {
      TAILQ_FOREACH(name, &names->head, name_entries) {
        if ((ret = dump_bridge_route(name->str,
                                     is_bridge_first,
                                     result)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't get route stats.");
          goto done;
        }
        if (is_bridge_first == true) {
          is_bridge_first = false;
        }

        ret = cmd_dump_file_write(fp, result);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Can't write file.");
          goto done;
        }
      }
    }

    if (fputs("]", fp) != EOF) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_OUTPUT_FAILURE,
                                              "Can't write file.");
      goto done;
    }

  done:
    if (ret == LAGOPUS_RESULT_OK) {
      ret = cmd_dump_file_send(iptr, fp, stream_out,
                               printf_proc, result);
    } else {
      ret = cmd_dump_error_send(stream_out, printf_proc,
                                result);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  return ret;
}

/**
 * Destory conf data.
 */
static inline void
route_conf_destroy(route_conf_t **conf) {
  if (*conf != NULL) {
    free((*conf)->name);
    (*conf)->name = NULL;
    free(*conf);
    *conf = NULL;
  }
}

/**
 * Copy conf data.
 */
static inline lagopus_result_t
route_conf_copy(route_conf_t **dst, route_conf_t *src) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *name = NULL;

  if (dst != NULL && *dst == NULL && src != NULL) {
    *dst = (route_conf_t *) calloc(1, sizeof(route_conf_t));
    if (*dst != NULL) {
      if (src->name != NULL) {
        name = strdup(src->name);
        if (name == NULL) {
          (*dst)->name = NULL;
          ret = LAGOPUS_RESULT_NO_MEMORY;
          lagopus_perror(ret);
          goto done;
        }
      }
      (*dst)->name = name;
      (*dst)->table_id = src->table_id;
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      lagopus_perror(ret);
    }
  done:
    if (ret != LAGOPUS_RESULT_OK) {
      route_conf_destroy(dst);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  return ret;
}

/**
 * Destroy args data.
 */
static inline void
args_destroy(args_t **args) {
  if (args != NULL && *args != NULL) {
    if ((*args)->conf != NULL) {
      route_conf_destroy(&((*args)->conf));
    }
    free(*args);
    *args = NULL;
  }
}

/**
 * Create args data.
 */
static inline lagopus_result_t
args_create(args_t **args,
            datastore_interp_t *iptr,
            route_conf_t *conf,
            datastore_config_type_t ftype,
            void *stream_out,
            datastore_printf_proc_t printf_proc) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int pre_size, size;

  if (args != NULL && *args == NULL &&
      iptr != NULL && conf != NULL &&
      stream_out != NULL && printf_proc != NULL) {
    *args = (args_t *) calloc(1, sizeof(args_t));
    if (*args != NULL) {
      (*args)->iptr = iptr;

      /* copy file name. */
      lagopus_mutex_lock(&lock);
      if (strlen(dump_tmp_dir) + strlen(TMP_FILE) < PATH_MAX) {
        pre_size = sizeof((*args)->file_name);
        size = snprintf((*args)->file_name,
                        (size_t) pre_size,
                        "%s%s", dump_tmp_dir, TMP_FILE);
        if (size < pre_size) {
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = LAGOPUS_RESULT_OUT_OF_RANGE;
          lagopus_perror(ret);
        }
      } else {
        ret = LAGOPUS_RESULT_OUT_OF_RANGE;
        lagopus_perror(ret);
      }
      lagopus_mutex_unlock(&lock);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = route_conf_copy(&((*args)->conf), conf);
        if (ret == LAGOPUS_RESULT_OK) {
          (*args)->ftype = ftype;
          (*args)->stream_out = stream_out;
          (*args)->printf_proc = printf_proc;
        } else {
          (*args)->conf = NULL;
          (*args)->stream_out = NULL;
          (*args)->printf_proc = NULL;
        }
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      lagopus_perror(ret);
    }

    if (ret != LAGOPUS_RESULT_OK) {
      args_destroy(args);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  return ret;
}

/**
 * Main thread of MAC table dump.
 */
static lagopus_result_t
thread_main(const lagopus_thread_t *t, void *a) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  args_t *args = NULL;

  if (t != NULL && a != NULL) {
    args = (args_t *) a;

    if ((ret = cmd_dump_main((lagopus_thread_t *) t,
                             args->iptr,
                             (void *) args->conf,
                             args->stream_out,
                             args->printf_proc,
                             args->ftype,
                             args->file_name,
                             false,
                             route_cmd_dump)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  if (args != NULL) {
    args_destroy(&args);
  }

  return ret;
}

/**
 * Initialize this module.
 */
static inline lagopus_result_t
route_cmd_dump_initialize(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create lock. */
  if ((ret = lagopus_mutex_create(&lock)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

 done:
  return ret;
}

/**
 * Finalize this module.
 */
static inline void
route_cmd_dump_finalize(void) {
  lagopus_hashmap_destroy(&thread_table, true);
  thread_table = NULL;
  lagopus_mutex_destroy(&lock);
}

/**
 * Start this thread.
 */
static inline lagopus_result_t
route_cmd_dump_thread_start(route_conf_t *conf,
                            datastore_interp_t *iptr,
                            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_config_type_t ftype = DATASTORE_CONFIG_TYPE_UNKNOWN;
  args_t *args = NULL;
  void *stream_out = NULL;
  datastore_printf_proc_t printf_proc = NULL;
  lagopus_thread_t th = NULL;

  lagopus_msg_info("start.\n");
  if (conf != NULL && iptr != NULL && result != NULL) {
    if ((ret = datastore_interp_get_current_file_context(iptr,
                                                         NULL, NULL,
                                                         NULL, &stream_out,
                                                         NULL, &printf_proc,
                                                         &ftype)) ==
        LAGOPUS_RESULT_OK) {
      if (stream_out != NULL && printf_proc != NULL &&
          (ftype == DATASTORE_CONFIG_TYPE_STREAM_SESSION ||
           ftype == DATASTORE_CONFIG_TYPE_STREAM_FD)) {
        if ((ret = args_create(&args, iptr, conf, ftype,
                               stream_out, printf_proc)) ==
            LAGOPUS_RESULT_OK) {
          if ((ret = lagopus_thread_create(&th,
                                           thread_main,
                                           NULL,
                                           NULL,
                                           "route_cmd_dump",
                                           (void *) args)) ==
              LAGOPUS_RESULT_OK) {
            ret = lagopus_thread_start(&th, true);
            if (ret != LAGOPUS_RESULT_OK) {
              lagopus_perror(ret);
            }
          } else {
            lagopus_perror(ret);
          }
        } else {
          lagopus_perror(ret);
        }

        if (ret != LAGOPUS_RESULT_OK && args != NULL) {
          args_destroy(&args);
        }
      } else {
        /* ignore other datastore_config_type_t. */
        ret = LAGOPUS_RESULT_OK;
      }
    } else {
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
route_cmd_dump_tmp_dir_set(const char *path) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct stat st;

  if (path != NULL) {
    if (strlen(path) < PATH_MAX) {
      if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode) == true) {
          lagopus_mutex_lock(&lock);
          strncpy(dump_tmp_dir, path, PATH_MAX - 1);
          lagopus_mutex_unlock(&lock);
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = LAGOPUS_RESULT_INVALID_ARGS;
        }
      } else {
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = LAGOPUS_RESULT_OUT_OF_RANGE;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static inline lagopus_result_t
route_cmd_dump_tmp_dir_get(char **path) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (path != NULL) {
    lagopus_mutex_lock(&lock);
    *path = strdup(dump_tmp_dir);
    if (*path == NULL) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    } else {
      ret = LAGOPUS_RESULT_OK;
    }
    lagopus_mutex_unlock(&lock);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
