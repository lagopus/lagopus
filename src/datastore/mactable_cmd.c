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
 *      @file   mactable_cmd.c
 *      @brief  DSL command parser for MAC address table management.
 */

#include "lagopus_apis.h"
#include "datastore_apis.h"
#include "cmd_common.h"
#include "mactable_cmd.h"
#include "mactable_cmd_internal.h"
#include "conv_json.h"

/* command name. */
#define CMD_NAME "mactable"

/* option num. */
enum mactable_opts {
  OPT_NAME = 0,
  OPT_TABLE_ID,
  OPT_TMP_DIR,

  OPT_MAX,
};

/* option name. */
static const char *const opt_strs[OPT_MAX] = {
  "*name",               /* OPT_NAME (not option) */
  "-table-id",           /* OPT_TABLE_ID */
  "-tmp-dir",            /* OPT_TMP_DIR */
};

typedef struct mactable_conf {
  char *name;
  uint8_t table_id;
} mactable_conf_t;

typedef struct configs {
  bool is_dump;
} configs_t;

static lagopus_hashmap_t sub_cmd_table = NULL;
static lagopus_hashmap_t sub_cmd_not_name_table = NULL;
static lagopus_hashmap_t dump_opt_table = NULL;
static lagopus_hashmap_t config_opt_table = NULL;

#include "mactable_cmd_dump.c"

static lagopus_result_t
table_id_opt_parse(const char *const *argv[],
                   void *c, void *out_configs,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  mactable_conf_t *conf = NULL;
  union cmd_uint cmd_uint;

  if (argv != NULL && c != NULL &&
      out_configs != NULL && result != NULL) {
    conf = (mactable_conf_t *) c;

    if (*(*argv + 1) != NULL) {
      if ((ret = cmd_uint_parse(*(++(*argv)), CMD_UINT8,
                                &cmd_uint)) ==
          LAGOPUS_RESULT_OK) {
        conf->table_id = cmd_uint.uint8;
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Bad opt value = %s.",
                                                *(*argv));
      }
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
tmp_dir_opt_parse(const char *const *argv[],
                  void *c, void *out_configs,
                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *path = NULL;
  (void) out_configs;
  (void) c;

  if (argv != NULL && out_configs != NULL && result != NULL) {
    if (*(*argv + 1) != NULL) {
      (*argv)++;
      if (IS_VALID_STRING(*(*argv)) == true) {
        ret = mactable_cmd_dump_tmp_dir_set(*(*argv));
        if (ret != LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_string_setf(result, ret,
                                                  "Bad opt value = %s.",
                                                  *(*argv));
        }
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Bad opt value = %s.",
                                                *(*argv));
      }
    } else {
      if ((ret = mactable_cmd_dump_tmp_dir_get(&path)) ==
          LAGOPUS_RESULT_OK) {
        if ((ret = lagopus_dstring_appendf(result, "{")) !=
            LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
          goto done;
        }
        if ((ret = datastore_json_string_append(
                     result, ATTR_NAME_GET(opt_strs, OPT_TMP_DIR),
                     path, false)) !=
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
      } else {
        ret = datastore_json_result_string_setf(result, ret,
                                                "Can't get tmp dir.");
      }
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

done:
  free(path);

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
  configs_t *configs = NULL;
  void *opt_proc;
  (void) state;
  (void) argc;
  (void) name;
  (void) hptr;
  (void) proc;

  if (iptr != NULL && argv != NULL &&
      out_configs != NULL && result != NULL) {
    configs = (configs_t *) out_configs;

    if (*(argv + 1) != NULL) {
      argv++;
      while (*argv != NULL) {
        if (IS_VALID_STRING(*(argv)) == true) {
          if ((ret = lagopus_hashmap_find(&config_opt_table,
                                          (void *)(*argv),
                                          &opt_proc)) ==
              LAGOPUS_RESULT_OK) {
            /* parse opt. */
            if (opt_proc != NULL) {
              ret = ((opt_proc_t) opt_proc)(&argv,
                                            (void *) NULL,
                                            (void *) configs,
                                            result);
              if (ret != LAGOPUS_RESULT_OK) {
                goto done;
              }
            } else {
              ret = LAGOPUS_RESULT_NOT_FOUND;
              lagopus_perror(ret);
              goto done;
            }
          } else if (ret == LAGOPUS_RESULT_NOT_FOUND) {
            goto done;
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
      ret = tmp_dir_opt_parse(&argv,
                              (void *) NULL,
                              (void *) configs,
                              result);
    }
  } else {
    ret = datastore_json_result_set(result, LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

done:
  return ret;
}

static inline lagopus_result_t
dump_sub_cmd_parse(datastore_interp_t *iptr,
                   const char *const argv[],
                   char *name,
                   mactable_conf_t *conf,
                   configs_t *configs,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  void *opt_proc;

  if (argv != NULL && conf != NULL &&
      configs != NULL && result != NULL) {
    configs->is_dump = true;

    if (name == NULL) {
      ret = LAGOPUS_RESULT_OK;
    } else {
      if (bridge_exists(name) == true) {
        conf->name = name;
        if (*argv != NULL) {
          while (*argv != NULL) {
            if (IS_VALID_STRING(*(argv)) == true) {
              if ((ret = lagopus_hashmap_find(&dump_opt_table,
                                              (void *)(*argv),
                                              &opt_proc)) ==
                  LAGOPUS_RESULT_OK) {
                /* parse opt. */
                if (opt_proc != NULL) {
                  ret = ((opt_proc_t) opt_proc)(&argv,
                                                (void *) conf,
                                                (void *) configs,
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
                ret = datastore_json_result_string_setf(
                        result, LAGOPUS_RESULT_INVALID_ARGS,
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
          ret = LAGOPUS_RESULT_OK;
        }
      } else {
        ret = datastore_json_result_string_setf(
                result, LAGOPUS_RESULT_NOT_FOUND,
               "name = %s", name);
      }
    }

    if (ret == LAGOPUS_RESULT_OK) {
      /* dump mactable. */
      if ((ret = mactable_cmd_dump_thread_start(conf,
                                            iptr,
                                            result)) !=
          LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(
                result,ret,
                "Can't start mactable dump thread.");
      }
    }
  } else {
    ret = datastore_json_result_set(result,
                                    LAGOPUS_RESULT_INVALID_ARGS,
                                    NULL);
  }

done:
  free(str);

  return ret;
}

STATIC lagopus_result_t
mactable_cmd_parse(datastore_interp_t *iptr,
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
  char *name = NULL;
  char *fullname = NULL;
  char *str = NULL;
  mactable_conf_t conf = {NULL, OFPTT_ALL};
  configs_t out_configs = {false};
  lagopus_dstring_t conf_result = NULL;
  (void) u_proc;
  (void) e_proc;
  (void) s_proc;
  (void) d_proc;

  for (i = 0; i < argc; i++) {
    lagopus_msg_debug(1, "argv[" PFSZS(4, u) "]:\t'%s'\n", i, argv[i]);
  }

  if (iptr != NULL && argv != NULL && result != NULL) {
    if ((ret = lagopus_dstring_create(&conf_result)) == LAGOPUS_RESULT_OK) {
      argv++;

      if (IS_VALID_STRING(*argv) == true) {
        if ((ret = lagopus_hashmap_find(
                     &sub_cmd_not_name_table,
                     (void *)(*argv),
                     &sub_cmd_proc)) == LAGOPUS_RESULT_OK) {
          /* parse sub cmd. */
          if (sub_cmd_proc != NULL) {
            ret_for_json =
              ((sub_cmd_proc_t) sub_cmd_proc)(iptr, state,
                                              argc, argv,
                                              NULL, hptr,
                                              u_proc,
                                              (void *) &out_configs,
                                              result);
          }
        }

        if (ret == LAGOPUS_RESULT_NOT_FOUND &&
            ret_for_json != LAGOPUS_RESULT_OK) {
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
                  /* parse dump cmd. */
                  ret_for_json = dump_sub_cmd_parse(iptr, argv,
                                                    fullname, &conf,
                                                    &out_configs, result);
                } else {
                  ret_for_json = datastore_json_result_string_setf(
                                   result, LAGOPUS_RESULT_INVALID_ARGS,
                                   "sub_cmd = %s.", *argv);
                }
              } else {
                /* parse dump cmd. */
                ret_for_json = dump_sub_cmd_parse(iptr, argv,
                                                  fullname, &conf,
                                                  &out_configs, result);
              }
            } else {
              ret_for_json = datastore_json_result_string_setf(
                               result, ret, "Can't get fullname %s.", name);
            }
          }
        }
      } else {
        /* parse dump all cmd. */
        ret_for_json = dump_sub_cmd_parse(iptr, argv, NULL, &conf,
                                          &out_configs, result);
      }

      /* create jsno for LAGOPUS_RESULT_OK. */
      if (ret_for_json == LAGOPUS_RESULT_OK &&
          out_configs.is_dump == false) {
        ret = lagopus_dstring_str_get(result, &str);
        if (ret != LAGOPUS_RESULT_OK) {
          goto done;
        }
        if (IS_VALID_STRING(str) == true) {
          ret = datastore_json_result_set(result, LAGOPUS_RESULT_OK, str);
        } else {
          ret = datastore_json_result_set(result, LAGOPUS_RESULT_OK, NULL);
        }
      } else {
        ret = ret_for_json;
      }
    }

  done:
    /* free. */
    free(name);
    free(fullname);
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

  /* create hashmap for sub cmds. */
  if ((ret = lagopus_hashmap_create(&sub_cmd_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  /* create hashmap for sub cmds (not name). */
  if ((ret = lagopus_hashmap_create(&sub_cmd_not_name_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if (((ret = sub_cmd_add(CONFIG_SUB_CMD, config_sub_cmd_parse,
                          &sub_cmd_not_name_table)) !=
       LAGOPUS_RESULT_OK)) {
    goto done;
  }

  /* create hashmap for dump opts. */
  if ((ret = lagopus_hashmap_create(&dump_opt_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = opt_add(opt_strs[OPT_TABLE_ID], table_id_opt_parse,
                     &dump_opt_table)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  /* create hashmap for config opts. */
  if ((ret = lagopus_hashmap_create(&config_opt_table,
                                    LAGOPUS_HASHMAP_TYPE_STRING,
                                    NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = opt_add(opt_strs[OPT_TMP_DIR], tmp_dir_opt_parse,
                     &config_opt_table)) !=
      LAGOPUS_RESULT_OK) {
    goto done;
  }

  if ((ret = datastore_interp_register_command(&s_interp, CONFIGURATOR_NAME,
             CMD_NAME, mactable_cmd_parse)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    goto done;
  }

  if ((ret = mactable_cmd_dump_initialize()) != LAGOPUS_RESULT_OK) {
    goto done;
  }

done:
  return ret;
}

/*
 * Public:
 */

lagopus_result_t
mactable_cmd_initialize(void) {
  return initialize_internal();
}

void
mactable_cmd_finalize(void) {
  mactable_cmd_dump_finalize();
  lagopus_hashmap_destroy(&sub_cmd_table, true);
  sub_cmd_table = NULL;
  lagopus_hashmap_destroy(&sub_cmd_not_name_table, true);
  sub_cmd_not_name_table = NULL;
  lagopus_hashmap_destroy(&dump_opt_table, true);
  dump_opt_table = NULL;
  lagopus_hashmap_destroy(&config_opt_table, true);
  config_opt_table = NULL;
}
