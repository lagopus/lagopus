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
#include "../save_cmd.c"

#define LAGOPUS_SAVE_CMD_TEST_FILE "/tmp/lagopus_save_cmd_test_file"

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
test_save_cmd_parse(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const char *argv1[] = {"save", LAGOPUS_SAVE_CMD_TEST_FILE, NULL};

  // Abnormal case
  {
    ret = s_parse_save(NULL, state, ARGV_SIZE(argv1), argv1, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);

    ret = s_parse_save(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                       proc, NULL, NULL, NULL,
                       NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
  }
}

void
test_destroy(void) {
  destroy = true;
}
