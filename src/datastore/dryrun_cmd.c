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

static inline lagopus_result_t
s_parse_dryrun(datastore_interp_t *iptr,
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
  (void)hptr;
  (void)u_proc;
  (void)e_proc;
  (void)s_proc;
  (void)d_proc;

  for (i = 0; i < argc; i++) {
    lagopus_msg_debug(1, "argv[" PFSZS(4, u) "]:\t'%s'\n", i, argv[i]);
  }

  if (argc == 1) {
    (void)lagopus_dstring_clear(result);

    ret = lagopus_dstring_appendf(result,
                                  "{\"ret\":\"OK\",\n"
                                  "\"data\":{\"dryrun\": ");

    if (ret == LAGOPUS_RESULT_OK) {
      if (state == DATASTORE_INTERP_STATE_DRYRUN) {
        ret = lagopus_dstring_appendf(result, "true}}");
      } else {
        ret = lagopus_dstring_appendf(result, "false}}");
      }

      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result,
                                                LAGOPUS_RESULT_ANY_FAILURES,
                                                "dstring append failed.");
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_ANY_FAILURES,
                                              "dstring append failed.");
    }
  } else if (argc == 2) {
    argv++;

    if (strcmp(*argv, "begin") == 0) {
      ret = dryrun_begin();

      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result,
                                                ret,
                                                "dryrun begin failed.");
      }
    } else if (strcmp(*argv, "end") == 0) {
      ret = dryrun_end();

      if (ret != LAGOPUS_RESULT_OK) {
        ret = datastore_json_result_string_setf(result,
                                                ret,
                                                "dryrun end failed.");
      }
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "Unknown option '%s'.",
                                              *argv);
    }

    if (ret != LAGOPUS_RESULT_DATASTORE_INTERP_ERROR) {
      ret = datastore_json_result_set(result, ret, NULL);
    }
  } else {
    ret = datastore_json_result_string_setf(result,
                                            LAGOPUS_RESULT_INVALID_ARGS,
                                            "Unknown option.");
  }

  return ret;
}

