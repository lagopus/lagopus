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

#include "cmd_common.h"

#define LAGOPUS_TMP_DIR "/tmp"
#define TMP_FILE "/.lagopus_save_XXXXXX"

/* option num. */
enum atomic_opts {
  OPT_TMP_DIR = 0,

  OPT_MAX,
};

/* option name. */
static const char *const opt_strs[OPT_MAX] = {
  "-tmp-dir",            /* OPT_TMP_DIR */
};

static char save_tmp_dir[PATH_MAX] = LAGOPUS_TMP_DIR;
static lagopus_mutex_t lock = NULL;

static inline lagopus_result_t
s_parse_begin_call(datastore_interp_t *iptr,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char file_name[PATH_MAX];

  /* copy file name. */
  lagopus_mutex_lock(&lock);
  if (strlen(save_tmp_dir) + strlen(TMP_FILE) < PATH_MAX) {
    snprintf(file_name,
             sizeof(file_name),
             "%s%s", save_tmp_dir, TMP_FILE);

    ret = datastore_interp_atomic_begin(iptr, file_name, result);
    if (ret != LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_set(result, ret, NULL);
    }
  } else {
    ret = datastore_json_result_string_setf(
        result, LAGOPUS_RESULT_TOO_LONG,
        "Bad file name.");
  }
  lagopus_mutex_unlock(&lock);

  return ret;
}

static inline lagopus_result_t
s_parse_begin_show(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_dstring_t show_result = NULL;
  char *str = NULL;

  /* show. */
  if ((ret = lagopus_dstring_create(&show_result)) ==
      LAGOPUS_RESULT_OK) {
    if ((ret = lagopus_dstring_appendf(&show_result, "{")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
              }
    if ((ret = datastore_json_string_append(
            &show_result, ATTR_NAME_GET(opt_strs, OPT_TMP_DIR),
            save_tmp_dir, false)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
    if ((ret = lagopus_dstring_appendf(&show_result, "}")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    ret = lagopus_dstring_str_get(&show_result, &str);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = datastore_json_result_set(result, LAGOPUS_RESULT_OK, str);
    }
  } else {
    lagopus_perror(ret);
  }

done:
  free(str);
  lagopus_dstring_destroy(&show_result);

  return  ret;
}

static inline lagopus_result_t
s_parse_begin(datastore_interp_t *iptr,
              datastore_interp_state_t state,
              size_t argc, const char *const argv[],
              bool *is_show,
              lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) argc;

  if (*argv != NULL) {
    argv++;
    if (*argv != NULL) {
      if (IS_VALID_STRING(*argv) == true) {
        if (strcmp(*argv, opt_strs[OPT_TMP_DIR]) == 0) {
          argv++;
          if (*argv != NULL) {
            if (IS_VALID_STRING(*argv) == true) {
              if (strlen(*argv) < PATH_MAX) {
                if (state == DATASTORE_INTERP_STATE_DRYRUN) {
                  ret = LAGOPUS_RESULT_OK;
                } else {
                  lagopus_mutex_lock(&lock);
                  strncpy(save_tmp_dir, *argv, PATH_MAX - 1);
                  lagopus_mutex_unlock(&lock);

                  /* call begin func.*/
                  ret = s_parse_begin_call(iptr, result);
                }
              } else {
                ret = datastore_json_result_string_setf(
                    result,
                    LAGOPUS_RESULT_OUT_OF_RANGE,
                    CMD_ERR_MSG_INVALID_OPT_VALUE,
                    *argv);
              }
            } else {
              ret = datastore_json_result_string_setf(
                  result,
                  LAGOPUS_RESULT_INVALID_ARGS,
                  CMD_ERR_MSG_INVALID_OPT_VALUE,
                  *argv);
            }
          } else {
            /* show */
            *is_show = true;
            ret = s_parse_begin_show(result);
          }
        } else {
          ret = datastore_json_result_string_setf(
              result,
              LAGOPUS_RESULT_INVALID_ARGS,
              CMD_ERR_MSG_INVALID_OPT,
              *argv);
        }
      } else {
        ret = datastore_json_result_string_setf(
            result,
            LAGOPUS_RESULT_INVALID_ARGS,
            CMD_ERR_MSG_INVALID_OPT,
            *argv);
      }
    }  else {
      if (state == DATASTORE_INTERP_STATE_DRYRUN) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        /* call begin func.*/
        ret = s_parse_begin_call(iptr, result);
      }
    }
  } else {
    ret = datastore_json_result_string_setf(
        result,
        LAGOPUS_RESULT_INVALID_ARGS,
        "Bad opt value.");
  }

  return ret;
}

static inline lagopus_result_t
s_parse_atomic(datastore_interp_t *iptr,
               datastore_interp_state_t state,
               size_t argc, const char *const argv[],
               lagopus_hashmap_t *hptr,
               datastore_update_proc_t u_proc,
               datastore_enable_proc_t e_proc,
               datastore_serialize_proc_t s_proc,
               datastore_destroy_proc_t d_proc,
               lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_show = false;
  size_t i;

  (void)state;
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
    return datastore_json_result_set(result,
                                     LAGOPUS_RESULT_OK,
                                     NULL);
  } else {
    if (strcmp(*argv, "begin") == 0) {
      ret = s_parse_begin(iptr, state, argc, argv, &is_show, result);
    } else if (strcmp(*argv, "abort") == 0) {
      if (state == DATASTORE_INTERP_STATE_DRYRUN) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = datastore_interp_atomic_abort(iptr, result);
      }
    } else if (strcmp(*argv, "commit") == 0) {
      if (state == DATASTORE_INTERP_STATE_DRYRUN) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = datastore_interp_atomic_commit(iptr, result);
      }
    } else if (strcmp(*argv, "rollback-force") == 0) {
      if (state == DATASTORE_INTERP_STATE_DRYRUN) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = datastore_interp_atomic_rollback(iptr, result, true);
      }
    } else {
      return datastore_json_result_string_setf(result,
             LAGOPUS_RESULT_INVALID_ARGS,
             "Unknown option '%s'",
             *argv);
    }

    if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR &&
        is_show == false) {
      ret = datastore_json_result_set(result, ret, NULL);
    }

    return ret;
  }
}

static inline lagopus_result_t
atomic_cmd_initialize(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create lock. */
  if ((ret = lagopus_mutex_create(&lock)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
  }

  return ret;
}

static inline void
atomic_cmd_finalize(void) {
  lagopus_mutex_destroy(&lock);
}
