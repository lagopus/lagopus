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
 *      @file   mactable_cmd_dump.c
 *      @brief  DSL dump command parser for MAC address table management.
 */

#include "lagopus_apis.h"
#include "datastore_apis.h"
#include "datastore_internal.h"
#include "cmd_common.h"
#include "cmd_dump.h"
#include "mactable_cmd.h"
#include "mactable_cmd_internal.h"
#include "conv_json.h"
#include "lagopus/dp_apis.h"
#include "lagopus/datastore/mactable_cmd.h"

#define TMP_FILE "/.lagopus_mactable_XXXXXX"
#define PORT_STR_BUF_SIZE 32

/* defines of mactable */
#define ETH_LEN 6

static lagopus_hashmap_t thread_table = NULL;
static lagopus_mutex_t lock = NULL;
static char dump_tmp_dir[PATH_MAX] = DATASTORE_TMP_DIR;

typedef struct args {
  mactable_conf_t *conf;
  datastore_interp_t *iptr;
  datastore_config_type_t ftype;
  void *stream_out;
  datastore_printf_proc_t printf_proc;
  char file_name[PATH_MAX];
} args_t;

/* Convert string. */
static void
mac_addr_to_str(uint64_t mac, char *macstr) {
  int i, j;
  uint8_t str[ETH_LEN];
  for (i = ETH_LEN - 1, j = 0; i >= 0; i--, j++)
    str[j] = 0xff & mac >> (8 * i);
  sprintf(macstr, "%02x:%02x:%02x:%02x:%02x:%02x",
          str[5], str[4], str[3], str[2], str[1], str[0]);
}
/**
 * Create mac entry list for dump.
 */
static inline lagopus_result_t
dump_mactable_list(const char *name,
                   unsigned int num_entries,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int i;

  /* get entries. */
  datastore_macentry_t *entries =
    malloc(sizeof(datastore_macentry_t) * num_entries);
  dp_bridge_mactable_entries_get(name, entries, num_entries);

  /* create json result of mac entry list. */
  for (i = 0; i < (int)num_entries; i++) {
    char mac_str[18];
    if ((ret = lagopus_dstring_appendf(
               result, DS_JSON_DELIMITER((i == 0), "{"))) !=
       LAGOPUS_RESULT_OK) {
       lagopus_perror(ret);
       goto done;
    }

    mac_addr_to_str(entries[i].mac_addr, mac_str);
    if ((ret = datastore_json_string_append( result, "mac_addr",
               mac_str, false)) !=
         LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    if ((ret = datastore_json_uint32_append(
               result, "port_no", entries[i].port_no, true)) !=
         LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    if ((ret = datastore_json_uint32_append(
               result, "update_time", entries[i].update_time, true)) !=
         LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    if ((ret = datastore_json_string_append( result, "address_type",
               (entries[i].address_type == 0) ? "static": "dynamic", true)) !=
         LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    if ((ret = lagopus_dstring_appendf( result, "}")) != LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  }

done:
  if (entries) {
    free(entries);
  }

  return ret;
}

/**
 * Create dump information of MAC address table entries.
 */
static inline lagopus_result_t
dump_table_macentries(const char *name,
                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  unsigned int num_entries;
  uint32_t ageing_time, max_entries;

  if ((ret = lagopus_dstring_appendf(result, "{")) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* get num of mac entries for "num_entries". */
  if ((ret = dp_bridge_mactable_num_entries_get(name, &num_entries)) !=
       LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
  if ((ret = datastore_json_uint32_append( result, "num_entries",
             num_entries, false)) !=
       LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* get configs from mactable for "ageing_time" and "max_entries". */
  if ((ret = dp_bridge_mactable_configs_get(name,
                                            &ageing_time,
                                            &max_entries)) !=
       LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
  if ((ret = datastore_json_uint32_append( result, "max_entries",
             max_entries, true)) !=
       LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }
  if ((ret = datastore_json_uint32_append( result, "ageing_time",
             ageing_time, true)) !=
       LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* start "entries". */
  if ((ret = lagopus_dstring_appendf(
               result, DELIMITER_INSTERN(KEY_FMT "["),
               "entries")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* get json result of mac entry list. */
  if ((ret = dump_mactable_list(name, num_entries, result))
       != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* end "entries". */
  if ((ret = lagopus_dstring_appendf(
               result, "]")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = lagopus_dstring_appendf( result, "}")) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

done:

  return ret;
}

/**
 * Create dump result of bridge information.
 */
STATIC lagopus_result_t
dump_bridge_mactable(const char *name,
                     bool is_bridge_first,
                     lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((ret = lagopus_dstring_appendf(
               result, "{")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* bridge name */
  if ((ret = lagopus_dstring_appendf(
          result, DS_JSON_DELIMITER(is_bridge_first, KEY_FMT "\"%s\""),
          "name",
          name)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* entries start. */
  if ((ret = lagopus_dstring_appendf(
          result, DELIMITER_INSTERN(KEY_FMT "["),
          "mactable")) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }


  /* dump a table. */
  if ((ret = dump_table_macentries(name, result)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* entries end. */
  if (ret == LAGOPUS_RESULT_OK) {
    if ((ret = lagopus_dstring_appendf(
            result, "]")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    if ((ret = lagopus_dstring_appendf(
            result, "}")) !=
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
mactable_cmd_dump(datastore_interp_t *iptr,
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
  mactable_conf_t *conf = NULL;

  if (iptr != NULL && c != NULL && fp != NULL &&
      stream_out !=NULL && printf_proc != NULL &&
      result != NULL) {
    conf = (mactable_conf_t *) c;

    if ((ret = datastore_bridge_get_names(conf->name,
                                          &names)) !=
        LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(
          result, ret,
          "Can't get bridge names.");
      goto done;
    }

    if (fputs("[", fp) == EOF) {
      ret = datastore_json_result_string_setf(
          result, LAGOPUS_RESULT_OUTPUT_FAILURE,
          "Can't write file.");
      goto done;
    }

    /* dump bridge(s). */
    if (TAILQ_EMPTY(&names->head) == false) {
      TAILQ_FOREACH(name, &names->head, name_entries) {
        if ((ret = dump_bridge_mactable(name->str,
                                        is_bridge_first,
                                        result)) !=
            LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result, ret,
              "Can't get mactable stats.");
          goto done;
        }
        if (is_bridge_first == true) {
          is_bridge_first = false;
        }

        ret = cmd_dump_file_write(fp, result);
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(
              result, ret,
              "Can't write file.");
          goto done;
        }
      }
    }

    if (fputs("]", fp) != EOF) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_string_setf(
          result, LAGOPUS_RESULT_OUTPUT_FAILURE,
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
mactable_conf_destroy(mactable_conf_t **conf) {
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
mactable_conf_copy(mactable_conf_t **dst, mactable_conf_t *src) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *name = NULL;

  if (dst != NULL && *dst == NULL && src != NULL) {
    *dst = (mactable_conf_t *) calloc(1, sizeof(mactable_conf_t));
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
      mactable_conf_destroy(dst);
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
      mactable_conf_destroy(&((*args)->conf));
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
            mactable_conf_t *conf,
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
        ret = mactable_conf_copy(&((*args)->conf), conf);
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
                             mactable_cmd_dump)) !=
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
mactable_cmd_dump_initialize(void) {
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
mactable_cmd_dump_finalize(void) {
  lagopus_hashmap_destroy(&thread_table, true);
  thread_table = NULL;
  lagopus_mutex_destroy(&lock);
}

/**
 * Start this thread.
 */
static inline lagopus_result_t
mactable_cmd_dump_thread_start(mactable_conf_t *conf,
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
                                           "mactable_cmd_dump",
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
mactable_cmd_dump_tmp_dir_set(const char *path) {
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
mactable_cmd_dump_tmp_dir_get(char **path) {
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
