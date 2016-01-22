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
#include "lagopus_dstring.h"
#include "../conv_json_result.h"

void
setUp(void) {
}

void
tearDown(void) {
}

#define TEST_NUM 2

void
test_datastore_json_result_string_get(void) {
  const char *str = NULL;
  const char *ret_str[TEST_NUM] = {"OK",
                                   "ANY_FAILURES"
                                  };
  lagopus_result_t ret[TEST_NUM] = {LAGOPUS_RESULT_OK,
                                    LAGOPUS_RESULT_ANY_FAILURES
                                   };
  int i;

  for (i = 0; i < TEST_NUM; i++) {
    str = datastore_json_result_string_get(ret[i]);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, str,
                                  "str is null.");
    if (str != NULL) {
      TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(str, ret_str[i]),
                                "str compare error.");
    } else {
      TEST_FAIL_MESSAGE("str is null.");
    }
  }
}
