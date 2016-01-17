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
#include "../port_cmd_internal.h"
#include "../interface_cmd_internal.h"
#include "../bridge_cmd_internal.h"
#include "../datastore_internal.h"
#include "../shutdown_cmd.c"

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

/* Normal case test is skip. */

void
test_shutdown_cmd_parse_bad_opts(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const char *argv1[] = {"shutdown", "hoge",
                         NULL
                        };
  const char *argv2[] = {"shutdown", "-level",
                         NULL
                        };
  const char *argv3[] = {"shutdown", "-level", "hoge",
                         NULL
                        };

  ret = s_parse_shutdown(&interp, state, ARGV_SIZE(argv1),
                         argv1, NULL, NULL, NULL, NULL, NULL, &ds);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "s_parse_shutdown() error.");

  ret = s_parse_shutdown(&interp, state, ARGV_SIZE(argv2),
                         argv2, NULL, NULL, NULL, NULL, NULL, &ds);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "s_parse_shutdown() error.");

  ret = s_parse_shutdown(&interp, state, ARGV_SIZE(argv3),
                         argv3, NULL, NULL, NULL, NULL, NULL, &ds);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "s_parse_shutdown() error.");
}

void
test_destroy(void) {
  destroy = true;
}
