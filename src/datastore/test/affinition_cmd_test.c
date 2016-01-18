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
#include "../affinition_cmd_internal.h"
#include "../datastore_internal.h"

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
  INTERP_DESTROY(NULL, interp, tbl, ds, destroy)
}

void
test_affinition_cmd_parse_dump_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"affinition",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[]}";

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, affinition_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_affinition_cmd_parse_bat_opt_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"affinition", "hoge",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt = hoge.\"}";

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 affinition_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_destroy(void) {
  destroy = true;
}
