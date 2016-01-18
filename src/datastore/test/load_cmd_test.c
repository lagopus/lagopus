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
#include "../load_cmd.c"

#define LAGOPUS_LOAD_CMD_TEST_FILE	"/tmp/lagopus_load_cmd_test_file"

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

lagopus_result_t
create_file(const char *path) {
  FILE *fp = NULL;
  fp = fopen(path, "w");
  if (fp != NULL) {
    fprintf(fp, "channel channel01 create\n");
  } else {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }
  fclose(fp);

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
remove_file(const char *path) {
  if (remove(path) != 0) {
    return LAGOPUS_RESULT_ANY_FAILURES;
  }
  return LAGOPUS_RESULT_OK;
}

void
test_is_readable(void) {
  const char *readable_file_path = LAGOPUS_LOAD_CMD_TEST_FILE;
  const char *unreadable_file_path = LAGOPUS_LOAD_CMD_TEST_FILE;

  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)remove_file(LAGOPUS_LOAD_CMD_TEST_FILE);
  (void)create_file(LAGOPUS_LOAD_CMD_TEST_FILE);
  ret = s_is_readable(readable_file_path);
  (void)remove_file(LAGOPUS_LOAD_CMD_TEST_FILE);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  (void)create_file(LAGOPUS_LOAD_CMD_TEST_FILE);
  (void)chmod(LAGOPUS_LOAD_CMD_TEST_FILE, 0);
  ret = s_is_readable(unreadable_file_path);
  (void)remove_file(LAGOPUS_LOAD_CMD_TEST_FILE);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
}

void
test_load_cmd_parse(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const char *argv1[] = {"load", LAGOPUS_LOAD_CMD_TEST_FILE, NULL};
  const char *argv_err1[] = {"load", "invalid_arg", NULL};

  // Normal case
  {
    ret = s_parse_load(&interp, state, ARGV_SIZE(argv_err1), argv_err1, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_load error.");
  }

  // Abnormal case
  {
    ret = s_parse_load(NULL, state, ARGV_SIZE(argv1), argv1, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);

    ret = s_parse_load(&interp, state, ARGV_SIZE(argv1), argv1, NULL,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);

    ret = s_parse_load(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                       NULL, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);

    ret = s_parse_load(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                       proc, NULL, NULL, NULL,
                       NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
  }
}

void
test_destroy(void) {
  destroy = true;
}
