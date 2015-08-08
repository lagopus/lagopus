/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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

#define TEST_STR(_ret, _str, _test_str) {                            \
    if (_str != NULL) {                                              \
      TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(_test_str, _str),          \
                                "string compare error.");            \
    } else {                                                         \
      TEST_FAIL_MESSAGE("string is null.");                          \
    }                                                                \
    if (_str != NULL){                                               \
      free(_str);                                                      \
      _str = NULL;                                                     \
    }                                                                \
  }

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_lagopus_str_escape_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_escaped = false;
  char *str = NULL;
  char test_str[] = "hoge\"foo'bar";
  char test_ret_str[] = "hoge\\\"foo\\\'bar";

  ret = lagopus_str_escape(test_str, "\"'", &is_escaped, &str);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "lagopus_str_escap error.");
  TEST_STR(ret, str, test_ret_str);
  TEST_ASSERT_EQUAL_MESSAGE(true, is_escaped,
                            "is_escaped error.");
}

void
test_lagopus_str_escape_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  bool is_escaped = false;
  char *str = NULL;
  char test_str[] = "hogefoobar";
  char test_ret_str[] = "hogefoobar";

  ret = lagopus_str_escape(test_str, "\"'", &is_escaped, &str);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "lagopus_str_escap error.");
  TEST_STR(ret, str, test_ret_str);
  TEST_ASSERT_EQUAL_MESSAGE(false, is_escaped,
                            "is_escaped error.");
}

void
test_lagopus_str_escape_03(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str[] = "hoge\"foo'bar";
  char test_ret_str[] = "hoge\\\"foo\\\'bar";

  ret = lagopus_str_escape(test_str, "\"'", NULL, &str);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "lagopus_str_escap error.");
  TEST_STR(ret, str, test_ret_str);
}

void
test_lagopus_str_escape_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  bool is_escaped = false;
  char test_str[] = "hoge\"foo'bar";

  ret = lagopus_str_escape((const char *) NULL, "\"'", &is_escaped, &str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "lagopus_str_escap error (null).");

  ret = lagopus_str_escape(test_str, (const char *) NULL, &is_escaped, &str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "lagopus_str_escap error (null).");

  ret = lagopus_str_escape(test_str, "\"'", &is_escaped, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "lagopus_str_escap error (null).");
}
