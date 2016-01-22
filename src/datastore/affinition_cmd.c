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
#include "cmd_common.h"
#include "affinition_cmd.h"
#include "affinition_cmd_internal.h"
#include "conv_json.h"

/* command name. */
#define CMD_NAME "affinition"

/* stats num. */
enum m_dumps {
  DUMPS_CORE_NO = 0,
  DUMPS_TYPE,
  DUMPS_INTERFACES,

  DUMPS_MAX,
};

/* dumps name. */
static const char *const dump_strs[DUMPS_MAX] = {
  "*core-no",            /* DUMPS_CORE_NO (not option) */
  "*type",               /* DUMPS_TYPE (not option) */
  "*interfaces",         /* DUMPS_INTERFACES (not option) */
};

/* type name. */
static const char *const type_strs[DATASTORE_AFFINITION_INFO_TYPE_MAX] = {
  "unknown",   /* DATASTORE_AFFINITION_INFO_TYPE_UNKNOWN */
  "worker",    /* DATASTORE_AFFINITION_INFO_TYPE_WORKER */
  "io",        /* DATASTORE_AFFINITION_INFO_TYPE_IO */
};

typedef struct configs {
  bool is_dumps;
  datastore_affinition_info_list_t list;
} configs_t;

static inline void
affinition_dumps_list_elem_free(datastore_affinition_info_list_t *list) {
  datastore_affinition_info_t *dumps = NULL;
  datastore_affinition_interface_t *interface = NULL;

  while ((dumps = TAILQ_FIRST(list)) != NULL) {
    while ((interface = TAILQ_FIRST(&dumps->interfaces)) != NULL) {
      TAILQ_REMOVE(&dumps->interfaces, interface, entry);
      free(interface->name);
      free(interface);
    }
    TAILQ_REMOVE(list, dumps, entry);
    free(dumps);
  }
}

static lagopus_result_t
dumps_sub_cmd_parse(datastore_interp_t *iptr,
                    datastore_interp_state_t dumpe,
                    size_t argc, const char *const argv[],
                    char *name,
                    lagopus_hashmap_t *hptr,
                    datastore_update_proc_t proc,
                    void *out_configs,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  configs_t *configs = NULL;
  (void) iptr;
  (void) argc;
  (void) argv;
  (void) name;
  (void) hptr;
  (void) proc;

  if (argv != NULL && out_configs != NULL &&
      result != NULL) {
    configs = (configs_t *) out_configs;
    configs->is_dumps = true;
    if (dumpe != DATASTORE_INTERP_STATE_DRYRUN) {
      /* TODO:
      ret = dp_bridge_affinition_list_get(&configs->list);
      */
    }
    ret = LAGOPUS_RESULT_OK;
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_string_setf(result, ret,
                                              "Can't get dumps.");
    }
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

  return ret;
}

static inline lagopus_result_t
names_show(lagopus_dstring_t *ds,
           const char *key,
           datastore_affinition_interface_list_t *interfaces,
           bool delimiter) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_affinition_interface_t *interface;
  char *name_str = NULL;
  size_t i = 0;

  if (key != NULL) {
    ret = DSTRING_CHECK_APPENDF(ds, delimiter, KEY_FMT"[", key);
    if (ret == LAGOPUS_RESULT_OK) {
      TAILQ_FOREACH(interface, interfaces, entry) {
        ret = datastore_json_string_escape(interface->name, &name_str);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = lagopus_dstring_appendf(ds, DS_JSON_LIST_ITEM_FMT(i),
                                        name_str);
          if (ret == LAGOPUS_RESULT_OK) {
            i++;
          } else {
            goto done;
          }
        }
        free(name_str);
        name_str = NULL;
      }
    } else {
      goto done;
    }
    ret = lagopus_dstring_appendf(ds, "]");
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

done:
  free(name_str);

  return ret;
}

static lagopus_result_t
affinition_cmd_dumps_json_create(lagopus_dstring_t *ds,
                                 configs_t *configs,
                                 lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_affinition_info_t *dumps = NULL;

  ret = lagopus_dstring_appendf(ds, "[");
  if (ret == LAGOPUS_RESULT_OK) {
    if (TAILQ_EMPTY(&configs->list) == false) {
      TAILQ_FOREACH(dumps, &configs->list, entry) {
        if ((ret = lagopus_dstring_appendf(ds, "{")) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* core_no */
        if ((ret = datastore_json_uint64_append(
                ds, ATTR_NAME_GET(dump_strs, DUMPS_CORE_NO),
                dumps->core_no, true)) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }

        /* type */
        if (dumps->type < DATASTORE_AFFINITION_INFO_TYPE_MAX) {
          if ((ret = datastore_json_string_append(
                  ds, ATTR_NAME_GET(dump_strs, DUMPS_TYPE),
                  type_strs[dumps->type], false)) !=
              LAGOPUS_RESULT_OK) {
            lagopus_perror(ret);
            goto done;
          }
        } else {
          ret = LAGOPUS_RESULT_OUT_OF_RANGE;
          lagopus_perror(ret);
          goto done;
        }

        /* interfaces */
        if (dumps->type == DATASTORE_AFFINITION_INFO_TYPE_IO) {
          if (TAILQ_EMPTY(&dumps->interfaces) == false) {
            if ((ret = names_show(ds,
                                  ATTR_NAME_GET(dump_strs, DUMPS_INTERFACES),
                                  &dumps->interfaces,
                                  true)) !=
                LAGOPUS_RESULT_OK) {
              lagopus_perror(ret);
              goto done;
            }
          }
        }

        if ((ret = lagopus_dstring_appendf(ds, "}")) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
      }
    } else {
      /* list is empty. */
      ret = LAGOPUS_RESULT_OK;
    }

    if (ret == LAGOPUS_RESULT_OK) {
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
affinition_cmd_parse(datastore_interp_t *iptr,
                     datastore_interp_state_t dumpe,
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
  configs_t out_configs = {false, {0LL}};
  char *str = NULL;
  lagopus_dstring_t conf_result = NULL;

  (void)e_proc;
  (void)s_proc;
  (void)d_proc;

  for (i = 0; i < argc; i++) {
    lagopus_msg_debug(1, "argv[" PFSZS(4, u) "]:\t'%s'\n", i, argv[i]);
  }

  if (iptr != NULL && argv != NULL && result != NULL) {
    TAILQ_INIT(&out_configs.list);
    if ((ret = lagopus_dstring_create(&conf_result)) == LAGOPUS_RESULT_OK) {
      argv++;

      if (IS_VALID_STRING(*argv) == false) {
        ret_for_json = dumps_sub_cmd_parse(iptr, dumpe,
                                           argc, argv,
                                           NULL, hptr,
                                           u_proc,
                                           (void *) &out_configs,
                                           result);
      } else {
        ret_for_json = datastore_json_result_string_setf(
            result,
            LAGOPUS_RESULT_INVALID_ARGS,
            "Bad opt = %s.", *argv);
      }

      /* create json for conf. */
      if (ret_for_json == LAGOPUS_RESULT_OK) {
        if (out_configs.is_dumps == true) {
          ret = affinition_cmd_dumps_json_create(&conf_result, &out_configs,
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
        ret = datastore_json_result_set(result, LAGOPUS_RESULT_OK, str);
      } else {
        ret = ret_for_json;
      }
    }

  done:
    /* free. */
    affinition_dumps_list_elem_free(&out_configs.list);
    free(str);
    lagopus_dstring_destroy(&conf_result);
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

  if ((ret = datastore_interp_register_command(&s_interp, CONFIGURATOR_NAME,
             CMD_NAME, affinition_cmd_parse)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

  return ret;
}

/*
 * Public:
 */

lagopus_result_t
affinition_cmd_initialize(void) {
  return initialize_internal();
}

void
affinition_cmd_finalize(void) {
}
