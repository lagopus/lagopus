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

#include "unity.h"
#include "lagopus_apis.h"
#include "cmd_test_utils.h"
#include "../datastore_apis.h"
#include "../datastore_internal.h"
#include "../agent_cmd.c"

static lagopus_dstring_t ds = NULL;
static lagopus_hashmap_t tbl = NULL;
static datastore_interp_t interp = NULL;
static bool destroy = false;

void
setUp(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create interp. */
  INTERP_CREATE(ret, NULL, interp, tbl, ds);
}

void
tearDown(void) {
  /* destroy interp. */
  INTERP_DESTROY(NULL, interp, tbl, ds, destroy);
}

/* skip stats test. */

void
test_agent_cmd_parse_bad_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"agent", "-hoge",
                         NULL};
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Unknown option '-hoge'\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 s_parse_agent, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_agent_cmd_parse_set_opts(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"agent",
                         NULL};
  const char test_str1[] =
      "{\"ret\":\"OK\",\n"
      "\"data\":[{\"channelq-size\":1000,\n"
      "\"channelq-max-batches\":1000}]}";
  const char *argv2[] = {"agent",
                         "-channelq-size", "1",
                         NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"agent",
                         NULL};
  const char test_str3[] =
      "{\"ret\":\"OK\",\n"
      "\"data\":[{\"channelq-size\":1,\n"
      "\"channelq-max-batches\":1000}]}";
  const char *argv4[] = {"agent",
                         "-channelq-size",
                         NULL};
  const char test_str4[] =
      "{\"ret\":\"OK\",\n"
      "\"data\":[{\"channelq-size\":1}]}";
  const char *argv5[] = {"agent",
                         "-channelq-max-batches", "2",
                         NULL};
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *argv6[] = {"agent",
                         NULL};
  const char test_str6[] =
      "{\"ret\":\"OK\",\n"
      "\"data\":[{\"channelq-size\":1,\n"
      "\"channelq-max-batches\":2}]}";
  const char *argv7[] = {"agent",
                         "-channelq-max-batches",
                         NULL};
  const char test_str7[] =
      "{\"ret\":\"OK\",\n"
      "\"data\":[{\"channelq-max-batches\":2}]}";

  /* show */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, s_parse_agent, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* set channelq-size */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, s_parse_agent, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, NULL,
                 &ds, str, test_str2);

  /* show */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, s_parse_agent, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, NULL,
                 &ds, str, test_str3);

  /* show channelq-size */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, s_parse_agent, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, NULL,
                 &ds, str, test_str4);

  /* set channelq-size */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, s_parse_agent, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, NULL,
                 &ds, str, test_str5);

  /* show */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, s_parse_agent, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, NULL,
                 &ds, str, test_str6);

  /* show channelq-max-batches */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, s_parse_agent, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, NULL,
                 &ds, str, test_str7);
}

void
test_agent_cmd_parse_over_channelq_size(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"agent",
                         "-channelq-size", "65536",
                         NULL};
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"can't parse '65536' as a uint16_t integer.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 s_parse_agent, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_agent_cmd_parse_bad_channelq_size(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"agent",
                         "-channelq-size", "hoge",
                         NULL};
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"can't parse 'hoge' as a uint16_t integer.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 s_parse_agent, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_agent_cmd_parse_over_channelq_max_batches(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"agent",
                         "-channelq-max-batches", "65536",
                         NULL};
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"can't parse '65536' as a uint16_t integer.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 s_parse_agent, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_agent_cmd_parse_bad_channelq_max_batches(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"agent",
                         "-channelq-max-batches", "hoge",
                         NULL};
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"can't parse 'hoge' as a uint16_t integer.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 s_parse_agent, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_agent_cmd_parse_serialize(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"agent",
                         "-channelq-size", "2000",
                         "-channelq-max-batches", "3000",
                         NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char serialize_str1[] =
      "agent "
      "-channelq-size 2000 "
      "-channelq-max-batches 3000\n\n";

  /* set */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, s_parse_agent, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
  lagopus_dstring_clear(&ds);

  /* serialize */
  ret = agent_cmd_serialize(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "agent_cmd_serialize error.");
  TEST_DSTRING_NO_JSON(ret, &ds, str, serialize_str1, true);
}

void
test_destroy(void) {
  destroy = true;
}
