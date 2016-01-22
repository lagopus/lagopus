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
#include "lagopus/datastore.h"
#include "cmd_test_utils.h"
#include "../datastore_internal.h"
#include "../conv_json.h"
#include "../atomic_cmd.c"

static lagopus_dstring_t result = NULL;
static lagopus_hashmap_t tbl = NULL;
static datastore_interp_t interp = NULL;
static datastore_update_proc_t proc = NULL;
static bool destroy = false;

void
setUp(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create interp. */
  INTERP_CREATE(ret, NULL, interp, tbl, result);
}

void
tearDown(void) {
  /* destroy interp. */
  INTERP_DESTROY(NULL, interp, tbl, result, destroy);
}

void
test_atomic_cmd_parse(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const char *argv1[] = {"atomic", "begin", NULL};
  const char *argv_err1[] = {"atomic", "invalid_arg", NULL};

  // Normal case
  {
    ret = s_parse_atomic(&interp, state, ARGV_SIZE(argv_err1), argv_err1, &tbl,
                         proc, NULL, NULL, NULL,
                         &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_atomic error.");
  }

  // Abnormal case
  {
    ret = s_parse_atomic(NULL, state, ARGV_SIZE(argv1), argv1, &tbl,
                         proc, NULL, NULL, NULL,
                         &result);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);

    ret = s_parse_atomic(&interp, state, ARGV_SIZE(argv1), argv1, NULL,
                         proc, NULL, NULL, NULL,
                         &result);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);

    ret = s_parse_atomic(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                         NULL, NULL, NULL, NULL,
                         &result);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);

    ret = s_parse_atomic(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                         proc, NULL, NULL, NULL,
                         NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
  }
}

void
test_atomic_cmd_parse_tmp_dir(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"atomic", "begin", "-tmp-dir", NULL};
  const char test_str1[] =
      "{\"ret\":\"OK\",\n"
      "\"data\":{\"tmp-dir\":\"\\/tmp\"}}";
  const char *argv2[] = {"atomic", "begin", "-tmp-dir", "./", NULL};
  const char *argv3[] = {"atomic", "begin", "-tmp-dir", NULL};
  const char test_str3[] =
      "{\"ret\":\"OK\",\n"
      "\"data\":{\"tmp-dir\":\".\\/\"}}";
  const char *argv4[] = {"atomic", "begin", "hoge", NULL};
  const char test_str4[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"opt = hoge\"}";

  // Normal case
  {
    /* show tmp dir (begin cmd). */
    TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, s_parse_atomic, &interp, state,
                   ARGV_SIZE(argv1), argv1, &tbl, NULL,
                   &result, str, test_str1);

    /* set tmp dir (begin cmd). */
    ret = s_parse_atomic(NULL, state, ARGV_SIZE(argv2), argv2, &tbl,
                         proc, NULL, NULL, NULL,
                         &result);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);

    /* show tmp dir (begin cmd). */
    TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, s_parse_atomic, &interp, state,
                   ARGV_SIZE(argv3), argv3, &tbl, NULL,
                   &result, str, test_str3);

    /* bad opt (begin cmd). */
    TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                   s_parse_atomic, &interp, state,
                   ARGV_SIZE(argv4), argv4, &tbl, NULL,
                   &result, str, test_str4);
  }
}

void
test_destroy(void) {
  destroy = true;
}
