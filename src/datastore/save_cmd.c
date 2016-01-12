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

static inline lagopus_result_t
s_parse_save(datastore_interp_t *iptr,
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
  const char *cname = NULL;
  char *filepath = NULL;
  char *save_data = NULL;

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

  ret = datastore_interp_get_current_configurater(iptr, &cname);
  if (ret == LAGOPUS_RESULT_OK && IS_VALID_STRING(cname) == true) {
    if (IS_VALID_STRING(*argv) == true) {
      if ((ret = lagopus_str_unescape(*argv, "\"'", &filepath)) > 0) {
        if (state == DATASTORE_INTERP_STATE_DRYRUN) {
          ret = LAGOPUS_RESULT_OK;
        } else {
          ret = datastore_interp_save_file(iptr, cname, filepath, result);
        }

        if (ret == LAGOPUS_RESULT_OK) {
          ret = datastore_json_result_set(result, ret, NULL);
        }
      } else {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_INVALID_ARGS,
                                                CMD_ERR_MSG_INVALID_OPT_VALUE,
                                                *argv);
      }
    } else {
      if (state == DATASTORE_INTERP_STATE_DRYRUN) {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_OK,
                                                "%s", "");
      } else {
        ret = datastore_interp_serialize(iptr, cname, result);
        if (ret == LAGOPUS_RESULT_OK) {
          ret = lagopus_dstring_str_get(result, &save_data);
          if (ret == LAGOPUS_RESULT_OK && IS_VALID_STRING(save_data) == true) {
            ret = datastore_json_result_string_setf(result,
                                                    LAGOPUS_RESULT_OK,
                                                    "%s",
                                                    save_data);
          } else {
            ret = datastore_json_result_string_setf(result,
                                                    ret,
                                                    CMD_ERR_MSG_SAVE_FAILED,
                                                    "internal error.");
          }
        } else {
          ret = datastore_json_result_string_setf(result,
                                                  ret,
                                                  CMD_ERR_MSG_SAVE_FAILED,
                                                  "internal error.");
        }
      }
    }
  } else {
    ret = datastore_json_result_string_setf(result,
                                            ret,
                                            CMD_ERR_MSG_SAVE_FAILED,
                                            "internal error.");
  }

  free((void *) filepath);
  free((void *) save_data);
  return ret;
}

