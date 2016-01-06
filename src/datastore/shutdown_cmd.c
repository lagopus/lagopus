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

#define OPT_GRACE_LEVEL "-level"
#define OPT_GRACE_LEVEL_RIGHT_NOW "right-now"
#define OPT_GRACE_LEVEL_GRACEFULLY "gracefully"
#define GRACE_LEVEL_DEFAULT SHUTDOWN_GRACEFULLY

static inline void
s_shutdown(shutdown_grace_level_t level) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if ((ret = global_state_request_shutdown(level)) ==
      LAGOPUS_RESULT_OK) {
    lagopus_msg_info("The shutdown request accepted.\n");
  } else {
    lagopus_perror(ret);
    lagopus_msg_error("Can't request shutdown.\n");
  }
}

static lagopus_result_t
s_parse_shutdown(datastore_interp_t *iptr,
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
  (void)argc;
  (void)hptr;
  (void)u_proc;
  (void)e_proc;
  (void)s_proc;
  (void)d_proc;
  (void)result;

  for (i = 0; i < argc; i++) {
    lagopus_msg_debug(1, "argv[" PFSZS(4, u) "]:\t'%s'\n", i, argv[i]);
  }

  if (iptr != NULL && argv != NULL && result != NULL) {
    argv++;
    if (IS_VALID_STRING(*argv) == true) {
      if (strcmp(*argv, OPT_GRACE_LEVEL) == 0) {
        argv++;
        if (IS_VALID_STRING(*argv) == true) {
          if (strcmp(*argv, OPT_GRACE_LEVEL_RIGHT_NOW) == 0) {
            if (state != DATASTORE_INTERP_STATE_DRYRUN) {
              /* shutdown (right_now) */
              s_shutdown(SHUTDOWN_RIGHT_NOW);
            }
            ret = LAGOPUS_RESULT_OK;
          } else if (strcmp(*argv, OPT_GRACE_LEVEL_GRACEFULLY) == 0) {
            if (state != DATASTORE_INTERP_STATE_DRYRUN) {
              /* shutdown (gracefully) */
              s_shutdown(SHUTDOWN_GRACEFULLY);
            }
            ret = LAGOPUS_RESULT_OK;
          } else {
            ret = LAGOPUS_RESULT_INVALID_ARGS;
            lagopus_msg_warning(
              CMD_ERR_MSG_INVALID_OPT_VALUE". (%s)\n",
              *argv, lagopus_error_get_string(ret));
          }
        } else {
          if (*argv == NULL) {
            ret = LAGOPUS_RESULT_INVALID_ARGS;
            lagopus_msg_warning("Bad opt value. (%s)\n",
                                lagopus_error_get_string(ret));
          } else {
            ret = LAGOPUS_RESULT_INVALID_ARGS;
            lagopus_msg_warning(
              CMD_ERR_MSG_INVALID_OPT_VALUE". (%s)\n",
              *argv, lagopus_error_get_string(ret));
          }
        }
      } else {
        ret = LAGOPUS_RESULT_INVALID_ARGS;
        lagopus_msg_warning(
          CMD_ERR_MSG_INVALID_OPT". (%s)\n",
          *argv, lagopus_error_get_string(ret));
      }
    } else {
      if (*argv == NULL) {
        if (state != DATASTORE_INTERP_STATE_DRYRUN) {
          /* shutdown (default) */
          s_shutdown(GRACE_LEVEL_DEFAULT);
        }
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = LAGOPUS_RESULT_INVALID_ARGS;
        lagopus_msg_warning(
          CMD_ERR_MSG_INVALID_OPT". (%s)\n",
          *argv, lagopus_error_get_string(ret));
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
