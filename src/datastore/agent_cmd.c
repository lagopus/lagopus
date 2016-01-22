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
#include "lagopus/ofp_handler.h"

#define AGENT_CMD_NAME "agent"
#define OPT_CHANNELQ_SIZE "-channelq-size"
#define OPT_CHANNELQ_MAX_BATCHES "-channelq-max-batches"
#define STATS_CHANNLEQ_ENTRIES "*channleq-entries"

static inline lagopus_result_t
agent_cmd_stats(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t stats = 0;

  if ((ret = ofp_handler_channelq_stats_get(&stats)) ==
      LAGOPUS_RESULT_OK) {
    ret = datastore_json_result_setf(
        result,
        LAGOPUS_RESULT_OK,
        "[{\"%s\":%"PRIu16"}]",
        ATTR_NAME_GET_FOR_STR(STATS_CHANNLEQ_ENTRIES),
        stats);
  } else {
    ret = datastore_json_result_string_setf(
        result,
        ret,
        "Can't get channelq stats.");
  }

  return ret;
}

static inline lagopus_result_t
agent_cmd_current_all(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t channelq_size = ofp_handler_channelq_size_get();
  uint16_t channelq_max_batches = ofp_handler_channelq_max_batches_get();

  ret = datastore_json_result_setf(
      result,
      LAGOPUS_RESULT_OK,
      "[{\"%s\":%"PRIu16",\n"
      "\"%s\":%"PRIu16"}]",
      ATTR_NAME_GET_FOR_STR(OPT_CHANNELQ_SIZE),
      channelq_size,
      ATTR_NAME_GET_FOR_STR(OPT_CHANNELQ_MAX_BATCHES),
      channelq_max_batches);
  return ret;
}

static inline lagopus_result_t
agent_cmd_current_channelq_size(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t channelq_size = ofp_handler_channelq_size_get();

  ret = datastore_json_result_setf(
      result,
      LAGOPUS_RESULT_OK,
      "[{\"%s\":%"PRIu16"}]",
      ATTR_NAME_GET_FOR_STR(OPT_CHANNELQ_SIZE),
      channelq_size);
  return ret;
}

static inline lagopus_result_t
agent_cmd_current_channelq_max_batches(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t channelq_max_batches = ofp_handler_channelq_max_batches_get();

  ret = datastore_json_result_setf(
      result,
      LAGOPUS_RESULT_OK,
      "[{\"%s\":%"PRIu16"}]",
      ATTR_NAME_GET_FOR_STR(OPT_CHANNELQ_MAX_BATCHES),
      channelq_max_batches);
  return ret;
}

static inline lagopus_result_t
agent_cmd_opt_parse_channelq_size(datastore_interp_state_t state,
                                  const char *const argv[],
                                  lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t val = 0;

  if (IS_VALID_STRING(*argv) == true) {
    if ((ret = lagopus_str_parse_uint16(*argv, &val)) ==
        LAGOPUS_RESULT_OK) {
      if (state != DATASTORE_INTERP_STATE_DRYRUN) {
        ofp_handler_channelq_size_set(val);
      }
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "can't parse '%s' as a "
                                              "uint16_t integer.",
                                              *argv);
    }
  } else {
    ret = datastore_json_result_string_setf(result,
                                            LAGOPUS_RESULT_INVALID_ARGS,
                                            "Bad opt value = %s",
                                            *argv);
  }
  return ret;
}

static inline lagopus_result_t
agent_cmd_opt_parse_channelq_max_batches(datastore_interp_state_t state,
                                         const char *const argv[],
                                         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t val = 0;

  if (IS_VALID_STRING(*argv) == true) {
    if ((ret = lagopus_str_parse_uint16(*argv, &val)) ==
        LAGOPUS_RESULT_OK) {
      if (state != DATASTORE_INTERP_STATE_DRYRUN) {
        ofp_handler_channelq_max_batches_set(val);
      }
      ret = LAGOPUS_RESULT_OK;
    } else {
      ret = datastore_json_result_string_setf(result,
                                              LAGOPUS_RESULT_INVALID_ARGS,
                                              "can't parse '%s' as a "
                                              "uint16_t integer.",
                                              *argv);
    }
  } else {
    ret = datastore_json_result_string_setf(result,
                                            LAGOPUS_RESULT_INVALID_ARGS,
                                            "Bad opt value = %s",
                                            *argv);
  }
  return ret;
}

static inline lagopus_result_t
s_parse_agent(datastore_interp_t *iptr,
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

  if (argc == 1) {
    return agent_cmd_current_all(result);
  } else {
    if (IS_VALID_STRING(*argv) == true &&
        strcmp(*argv, STATS_SUB_CMD) == 0) {
      return agent_cmd_stats(result);
    } else {
      while (IS_VALID_STRING(*argv) == true) {
        if (strcmp(*argv, OPT_CHANNELQ_SIZE) == 0) {
          argv++;
          if (IS_VALID_STRING(*argv) == true) {
            ret = agent_cmd_opt_parse_channelq_size(state, argv, result);
            if (ret != LAGOPUS_RESULT_OK) {
              return ret;
            }
          } else {
            return agent_cmd_current_channelq_size(result);
          }
        } else if (strcmp(*argv, OPT_CHANNELQ_MAX_BATCHES) == 0) {
          argv++;
          if (IS_VALID_STRING(*argv) == true) {
            ret = agent_cmd_opt_parse_channelq_max_batches(state, argv, result);
            if (ret != LAGOPUS_RESULT_OK) {
              return ret;
            }
          } else {
            return agent_cmd_current_channelq_max_batches(result);
          }
        } else {
          return datastore_json_result_string_setf(
              result,
              LAGOPUS_RESULT_INVALID_ARGS,
              "Unknown option '%s'",
              *argv);
        }
        argv++;
      }
    }

    // setter result
    return datastore_json_result_set(result, ret, NULL);
  }
}

lagopus_result_t
agent_cmd_serialize(lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t channelq_size = ofp_handler_channelq_size_get();
  uint16_t channelq_max_batches = ofp_handler_channelq_max_batches_get();

  if (result != NULL) {
    /* cmmand name. */
    if ((ret = lagopus_dstring_appendf(result, AGENT_CMD_NAME)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    /* channelq-size opt. */
    if ((ret = lagopus_dstring_appendf(result, " "OPT_CHANNELQ_SIZE)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %"PRIu16,
                                         channelq_size)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* channelq-max-batches opt. */
    if ((ret = lagopus_dstring_appendf(result, " "OPT_CHANNELQ_MAX_BATCHES)) ==
        LAGOPUS_RESULT_OK) {
      if ((ret = lagopus_dstring_appendf(result, " %"PRIu16,
                                         channelq_max_batches)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
    } else {
      lagopus_perror(ret);
      goto done;
    }

    /* Add newline. */
    if ((ret = lagopus_dstring_appendf(result, "\n\n")) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }
done:

  return ret;
}
