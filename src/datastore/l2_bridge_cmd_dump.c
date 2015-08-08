/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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
#include "lagopus/dp_apis.h"
#include "datastore_apis.h"
#include "cmd_common.h"
#include "cmd_dump.h"
#include "l2_bridge_cmd.h"
#include "l2_bridge_cmd_internal.h"
#include "conv_json.h"

/* dump num. */
enum l2b_dumps {
  DUMPS_NAME = 0,
  DUMPS_ENTRIES,
  DUMPS_HW_ADDR,
  DUMPS_PORT_NUMBER,
  DUMPS_AGEING_TIMER,

  DUMPS_MAX,
};

/* dumps name. */
static const char *const dump_strs[DUMPS_MAX] = {
  "*name",               /* DUMPS_NAME (not option) */
  "*entries",            /* DUMPS_ENTRIES (not option) */
  "*hw-addr",            /* DUMPS_HW_ADDR (not option) */
  "*port-number",        /* DUMPS_PORT_NUMBER (not option) */
  "*ageing-timer",       /* DUMPS_AGEING_TIMER (not option) */
};

#define TMP_FILE "/.lagopus_l2b_XXXXXX"

static lagopus_hashmap_t thread_table = NULL;
static lagopus_mutex_t lock = NULL;

typedef struct args {
  l2_bridge_dump_conf_t *conf;
  datastore_interp_t *iptr;
  datastore_config_type_t ftype;
  void *stream_out;
  datastore_printf_proc_t printf_proc;
  char file_name[PATH_MAX];
} args_t;

static lagopus_result_t
dump_proc(datastore_bridge_l2_info_t *entry,
          FILE *fp,
          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = lagopus_dstring_appendf(result, DS_JSON_LIST_ITEM_FMT(entry->index),
                                "{");
  if (ret == LAGOPUS_RESULT_OK) {
    /* hw_addr */
    if ((ret = lagopus_dstring_appendf(
            result,
            DELIMITER_INSTERN(KEY_FMT "\"%02x:%02x:%02x:%02x:%02x:%02x\""),
            ATTR_NAME_GET(opt_strs, DUMPS_HW_ADDR),
            entry->hw_addr[0], entry->hw_addr[1],
            entry->hw_addr[2], entry->hw_addr[3],
            entry->hw_addr[4], entry->hw_addr[5])) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    /* port_number */
    if ((ret = datastore_json_uint32_append(
            result,
            ATTR_NAME_GET(opt_strs, DUMPS_PORT_NUMBER),
            entry->port_number, true)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    /* ageing_timer */
    if ((ret = datastore_json_uint64_append(
            result,
            ATTR_NAME_GET(opt_strs, DUMPS_AGEING_TIMER),
            entry->ageing_timer, true)) !=
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

  ret = cmd_dump_file_write(fp, result);
  if (ret != LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_string_setf(
        result, ret,
        "Can't write file.");
    goto done;
  }

done:
  if (ret != LAGOPUS_RESULT_OK &&
      ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
    ret = datastore_json_result_set(result, ret, NULL);
  }

  return ret;
}

static inline lagopus_result_t
l2_bridge_cmd_dump_internal(l2_bridge_dump_conf_t *conf,
                            FILE *fp,
                            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) fp;

  ret = lagopus_dstring_appendf(result, "{");
  if (ret == LAGOPUS_RESULT_OK) {
    /* name */
    if ((ret = datastore_json_string_append(
            result,
            ATTR_NAME_GET(opt_strs, DUMPS_NAME),
            conf->name, false)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    if ((ret = DSTRING_CHECK_APPENDF(
            result, true, KEY_FMT"[",
            ATTR_NAME_GET(opt_strs, DUMPS_ENTRIES))) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    /* TODO:
     * if ((ret = dp_bridge_l2_info_get(conf->name, dump_proc, fp, result)) !=
     *     LAGOPUS_RESULT_OK) {
     *   lagopus_perror(ret);
     *   goto done;
     * }
     */
  }

done:
  return ret;
}

static lagopus_result_t
l2_bridge_cmd_dump(datastore_interp_t *iptr,
                   void *c,
                   FILE *fp,
                   void *stream_out,
                   datastore_printf_proc_t printf_proc,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  l2_bridge_dump_conf_t *conf;

  if (iptr != NULL && c != NULL && fp != NULL &&
      stream_out !=NULL && printf_proc != NULL &&
      result != NULL) {
    conf = (l2_bridge_dump_conf_t *) c;

    if (fputs("[", fp) == EOF) {
      ret = datastore_json_result_string_setf(
          result, LAGOPUS_RESULT_OUTPUT_FAILURE,
          "Can't write file.");
      goto done;
    }

    if ((ret = l2_bridge_cmd_dump_internal(conf,
                                           fp, result)) !=
        LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(
          result, ret,
          "Can't write file.");
      goto done;
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

static inline void
l2_bridge_dump_conf_destroy(l2_bridge_dump_conf_t **conf) {
  if (*conf != NULL) {
    free((void *) (*conf)->name);
    (*conf)->name = NULL;
    free((void *) (*conf)->bridge_name);
    (*conf)->bridge_name = NULL;
    free((void *) (*conf)->tmp_dir);
    (*conf)->tmp_dir = NULL;

    free(*conf);
    *conf = NULL;
  }
}

static inline lagopus_result_t
l2_bridge_dump_conf_copy(l2_bridge_dump_conf_t **dst,
                         l2_bridge_dump_conf_t *src) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *name = NULL;
  char *bridge_name = NULL;
  char *tmp_dir = NULL;

  if (dst != NULL && *dst == NULL && src != NULL) {
    *dst = (l2_bridge_dump_conf_t *) calloc(1, sizeof(l2_bridge_dump_conf_t));
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

      if (src->bridge_name != NULL) {
        bridge_name = strdup(src->bridge_name);
        if (bridge_name == NULL) {
          (*dst)->bridge_name = NULL;
          ret = LAGOPUS_RESULT_NO_MEMORY;
          lagopus_perror(ret);
          goto done;
        }
      }
      (*dst)->bridge_name = bridge_name;

      if (src->tmp_dir != NULL) {
        tmp_dir = strdup(src->tmp_dir);
        if (tmp_dir == NULL) {
          (*dst)->tmp_dir = NULL;
          ret = LAGOPUS_RESULT_NO_MEMORY;
          lagopus_perror(ret);
          goto done;
        }
      }
      (*dst)->tmp_dir = tmp_dir;

      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      lagopus_perror(ret);
    }
 done:
    if (ret != LAGOPUS_RESULT_OK) {
      l2_bridge_dump_conf_destroy(dst);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  return ret;
}


static inline void
args_destroy(args_t **args) {
  if (args != NULL && *args != NULL) {
    if ((*args)->conf != NULL) {
      l2_bridge_dump_conf_destroy(&((*args)->conf));
    }
    free(*args);
    *args = NULL;
  }
}

static inline lagopus_result_t
args_create(args_t **args,
            datastore_interp_t *iptr,
            l2_bridge_dump_conf_t *conf,
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
      if (strlen(conf->tmp_dir) + strlen(TMP_FILE) < PATH_MAX) {
        pre_size = sizeof((*args)->file_name);
        size = snprintf((*args)->file_name,
                        (size_t) pre_size,
                        "%s%s", conf->tmp_dir, TMP_FILE);
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
        ret = l2_bridge_dump_conf_copy(&((*args)->conf), conf);
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
                             l2_bridge_cmd_dump)) !=
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

static inline lagopus_result_t
l2_bridge_cmd_dump_initialize(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create lock. */
  if ((ret = lagopus_mutex_create(&lock)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

  return ret;
}

static inline void
l2_bridge_cmd_dump_finalize(void) {
  lagopus_hashmap_destroy(&thread_table, true);
  thread_table = NULL;
  lagopus_mutex_destroy(&lock);
}

static inline lagopus_result_t
l2_bridge_cmd_dump_thread_start(l2_bridge_dump_conf_t *conf,
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
                                           "l2_bridge_cmd_dump",
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
