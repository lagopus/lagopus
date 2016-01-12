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
#include "../conv_json.h"

static lagopus_dstring_t ds = NULL;

#define TEST_DSTR(_ret, _ds, _str, _test_str) {                        \
    _ret = lagopus_dstring_str_get((_ds), &_str);                      \
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, _ret,                 \
                              "lagopus_dstring_str_get error.");       \
    if (_str != NULL) {                                                \
      TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(_test_str, _str),            \
                                "string compare error.");              \
    } else {                                                           \
      TEST_FAIL_MESSAGE("string is null.");                            \
    }                                                                  \
    if (_str != NULL){                                                 \
      free(_str);                                                      \
      _str = NULL;                                                     \
    }                                                                  \
  }

#define TEST_JSON_STR(_ret, _str, _test_str) {                         \
    if (_str != NULL) {                                                \
      TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(_test_str, _str),            \
                                "string compare error.");              \
    } else {                                                           \
      TEST_FAIL_MESSAGE("string is null.");                            \
    }                                                                  \
    if (_str != NULL){                                                 \
      free(_str);                                                      \
      _str = NULL;                                                     \
    }                                                                  \
  }

void
setUp(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create. */
  ret = lagopus_dstring_create(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "lagopus_dstring_create error.");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, ds,
                                "dstring is null.");
}

void
tearDown(void) {
  /* destroy. */
  lagopus_dstring_destroy(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(NULL, ds,
                            "dstring is not null.");
}

void
test_datastore_json_string_append(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str1[] = "\"key1\":\"val\\\"1\"";
  char test_str2[] = "\"key1\":\"val\\\"1\",\n\"key2\":\"val2\"";

  /* 1st. */
  ret = datastore_json_string_append(&ds, "key1", "val\"1", false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_append error.");
  TEST_DSTR(ret, &ds, str, test_str1);

  /* 2nd. */
  ret = datastore_json_string_append(&ds, "key2", "val2", true);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_append error.");
  TEST_DSTR(ret, &ds, str, test_str2);
}

void
test_datastore_json_string_append_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  static lagopus_dstring_t datastore_null = NULL;

  ret = datastore_json_string_append(NULL, "key1", "val1", false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_string_append error.");

  ret = datastore_json_string_append(&datastore_null, "key1", "val1", false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_string_append error.");

  ret = datastore_json_string_append(&ds, NULL, "val1", false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_string_append error.");

  ret = datastore_json_string_append(&ds, "key1", NULL, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_string_append error.");
}

void
test_datastore_json_uint64_append(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str1[] = "\"key1\":18446744073709551615";
  char test_str2[] = "\"key1\":18446744073709551615,\n\"key2\":0";

  /* 1st. */
  ret = datastore_json_uint64_append(&ds, "key1", UINT64_MAX, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint64_append error.");
  TEST_DSTR(ret, &ds, str, test_str1);

  /* 2nd. */
  ret = datastore_json_uint64_append(&ds, "key2", 0, true);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint64_append error.");
  TEST_DSTR(ret, &ds, str, test_str2);
}

void
test_datastore_json_uint64_append_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  static lagopus_dstring_t datastore_null = NULL;

  ret = datastore_json_uint64_append(NULL, "key1", 0, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_uint64_append error.");

  ret = datastore_json_uint64_append(&datastore_null, "key1", 0, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_uint64_append error.");

  ret = datastore_json_uint64_append(&ds, NULL, 0, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_uint64_append error.");
}

void
test_datastore_json_uint32_append(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str1[] = "\"key1\":4294967295";
  char test_str2[] = "\"key1\":4294967295,\n\"key2\":0";

  /* 1st. */
  ret = datastore_json_uint32_append(&ds, "key1", UINT32_MAX, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint32_append error.");
  TEST_DSTR(ret, &ds, str, test_str1);

  /* 2nd. */
  ret = datastore_json_uint32_append(&ds, "key2", 0, true);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint32_append error.");
  TEST_DSTR(ret, &ds, str, test_str2);
}

void
test_datastore_json_uint32_append_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  static lagopus_dstring_t datastore_null = NULL;

  ret = datastore_json_uint32_append(NULL, "key1", 0, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_uint32_append error.");

  ret = datastore_json_uint32_append(&datastore_null, "key1", 0, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_uint32_append error.");

  ret = datastore_json_uint32_append(&ds, NULL, 0, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_uint32_append error.");
}

void
test_datastore_json_uint16_append(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str1[] = "\"key1\":65535";
  char test_str2[] = "\"key1\":65535,\n\"key2\":0";

  /* 1st. */
  ret = datastore_json_uint16_append(&ds, "key1", UINT16_MAX, false);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint16_append error.");
  TEST_DSTR(ret, &ds, str, test_str1);

  /* 2nd. */
  ret = datastore_json_uint16_append(&ds, "key2", 0, true);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint16_append error.");
  TEST_DSTR(ret, &ds, str, test_str2);
}

void
test_datastore_json_uint16_append_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  static lagopus_dstring_t datastore_null = NULL;

  ret = datastore_json_uint16_append(NULL, "key1", 0, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_uint16_append error.");

  ret = datastore_json_uint16_append(&datastore_null, "key1", 0, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_uint16_append error.");

  ret = datastore_json_uint16_append(&ds, NULL, 0, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_uint16_append error.");
}

void
test_datastore_json_uint8_append(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str1[] = "\"key1\":255";
  char test_str2[] = "\"key1\":255,\n\"key2\":0";

  /* 1st. */
  ret = datastore_json_uint8_append(&ds, "key1", UINT8_MAX, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint8_append error.");
  TEST_DSTR(ret, &ds, str, test_str1);

  /* 2nd. */
  ret = datastore_json_uint8_append(&ds, "key2", 0, true);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint8_append error.");
  TEST_DSTR(ret, &ds, str, test_str2);
}

void
test_datastore_json_uint8_append_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  static lagopus_dstring_t datastore_null = NULL;

  ret = datastore_json_uint8_append(NULL, "key1", 0, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_uint8_append error.");

  ret = datastore_json_uint8_append(&datastore_null, "key1", 0, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_uint8_append error.");

  ret = datastore_json_uint8_append(&ds, NULL, 0, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_uint8_append error.");
}

void
test_datastore_json_bool_append(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str1[] = "\"key1\":true";
  char test_str2[] = "\"key1\":true,\n\"key2\":false";

  /* true. */
  ret = datastore_json_bool_append(&ds, "key1", true, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_bool_append error.");
  TEST_DSTR(ret, &ds, str, test_str1);

  /* false. */
  ret = datastore_json_bool_append(&ds, "key2", false, true);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_bool_append error.");
  TEST_DSTR(ret, &ds, str, test_str2);
}

void
test_datastore_json_bool_append_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  static lagopus_dstring_t datastore_null = NULL;

  ret = datastore_json_bool_append(NULL, "key1", true, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_bool_append error.");

  ret = datastore_json_bool_append(&datastore_null, "key1", true, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_bool_append error.");

  ret = datastore_json_bool_append(&ds, NULL, true, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_bool_append error.");
}

void
test_datastore_json_string_array_append_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str1[] = "\"key1\":[\"val1\",\"val2\",\"val3\"]";
  const char *data_str1[3] = {"val1", "val2", "val3"};
  char test_str2[] =
    "\"key1\":[\"val1\",\"val2\",\"val3\"],\n"
    "\"key2\":[\"val4\",\"val5\",\"val6\"]";
  const char *data_str2[3] = {"val4", "val5", "val6"};

  /* 1st. */
  ret = datastore_json_string_array_append(&ds, "key1", data_str1, 3, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_array_append error.");
  TEST_DSTR(ret, &ds, str, test_str1);

  /* 2nd. */
  ret = datastore_json_string_array_append(&ds, "key2", data_str2, 3, true);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_array_append error.");
  TEST_DSTR(ret, &ds, str, test_str2);
}

void
test_datastore_json_string_array_append_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str1[] = "\"key1\":[]";
  const char *data_str1[3] = {};

  /* empty item. */
  ret = datastore_json_string_array_append(&ds, "key1", data_str1, 3, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_array_append error.");
  TEST_DSTR(ret, &ds, str, test_str1);
}

void
test_datastore_json_string_array_append_03(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str1[] = "\"key1\":[]";
  const char *data_str1[3] = {"val1", "val2", "val3"};

  /* size = 0. */
  ret = datastore_json_string_array_append(&ds, "key1", data_str1, 0, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_array_append error.");
  TEST_DSTR(ret, &ds, str, test_str1);
}

void
test_datastore_json_string_array_append_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  static lagopus_dstring_t datastore_null = NULL;
  const char *data_str1[3] = {"val1", "val2", "val3"};

  ret = datastore_json_string_array_append(NULL, "key1", data_str1, 3, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_string_array_append error.");

  ret = datastore_json_string_array_append(&datastore_null, "key1", data_str1, 3,
        false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_string_array_append error.");

  ret = datastore_json_string_array_append(&ds, NULL, data_str1, 3, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_string_array_append error.");

  ret = datastore_json_string_array_append(&ds, "key1", NULL, 3, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_string_array_append error.");
}

void
test_datastore_json_result_set_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t out_ret = LAGOPUS_RESULT_OK;
  char *str = NULL;
  char test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"key1\":\"val1\"}]}";

  ret = datastore_json_result_set(&ds, out_ret, "[{\"key1\":\"val1\"}]");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_result_set error.");
  TEST_DSTR(ret, &ds, str, test_str1);
}

void
test_datastore_json_result_set_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t out_ret = LAGOPUS_RESULT_OK;
  char *str = NULL;
  char test_str1[] =
    "{\"ret\":\"OK\"}";

  ret = datastore_json_result_set(&ds, out_ret, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_result_set error.");
  TEST_DSTR(ret, &ds, str, test_str1);
}

void
test_datastore_json_result_set_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t out_ret = LAGOPUS_RESULT_OK;
  static lagopus_dstring_t ds_null = NULL;

  ret = datastore_json_result_set(NULL, out_ret, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_result_set error.");

  ret = datastore_json_result_set(&ds_null, out_ret, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_result_set error.");
}

void
test_datastore_json_result_setf(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t out_ret = LAGOPUS_RESULT_INVALID_ARGS;
  char *str = NULL;
  char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":hoge 1}";

  ret = datastore_json_result_setf(&ds, out_ret, "hoge %d", 1);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                            "datastore_json_result_setf error.");
  TEST_DSTR(ret, &ds, str, test_str1);
}

void
test_datastore_json_result_setf_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t out_ret = LAGOPUS_RESULT_OK;
  static lagopus_dstring_t ds_null = NULL;

  ret = datastore_json_result_setf(NULL, out_ret, "hoge");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_result_setf error.");

  ret = datastore_json_result_setf(&ds_null, out_ret, "hoge");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_result_setf error.");

  ret = datastore_json_result_setf(&ds, out_ret, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_result_setf error.");
}

void
test_datastore_json_result_string_setf_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t out_ret = LAGOPUS_RESULT_INVALID_ARGS;
  char *str = NULL;
  char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"hoge \\\" 1\"}";

  ret = datastore_json_result_string_setf(&ds, out_ret, "hoge \" %d", 1);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                            "datastore_json_result_string_setf error.");
  TEST_DSTR(ret, &ds, str, test_str1);
}

void
test_datastore_json_result_string_setf_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t out_ret = LAGOPUS_RESULT_INVALID_ARGS;
  char *str = NULL;
  char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"hoge\"}";

  ret = datastore_json_result_string_setf(&ds, out_ret, "hoge");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                            "datastore_json_result_string_setf error.");
  TEST_DSTR(ret, &ds, str, test_str1);
}

void
test_datastore_json_result_string_setf_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t out_ret = LAGOPUS_RESULT_OK;
  static lagopus_dstring_t ds_null = NULL;

  ret = datastore_json_result_string_setf(NULL, out_ret, "hoge");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_result_string_setf error.");

  ret = datastore_json_result_string_setf(&ds_null, out_ret, "hoge");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_result_string_setf error.");
}

void
test_datastore_json_string_escape_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str[] = "hoge\"foo";
  char test_ret_str[] = "hoge\\\"foo";

  ret = datastore_json_string_escape(test_str, &str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_escape error.");
  TEST_JSON_STR(ret, str, test_ret_str);
}

void
test_datastore_json_string_escape_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str[] = "hoge\\foo";
  char test_ret_str[] = "hoge\\\\foo";

  ret = datastore_json_string_escape(test_str, &str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_escape error.");
  TEST_JSON_STR(ret, str, test_ret_str);
}

void
test_datastore_json_string_escape_03(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str[] = "hoge/foo";
  char test_ret_str[] = "hoge\\/foo";

  ret = datastore_json_string_escape(test_str, &str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_escape error.");
  TEST_JSON_STR(ret, str, test_ret_str);
}

void
test_datastore_json_string_escape_04(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str[] = "hoge\bfoo";
  char test_ret_str[] = "hoge\\bfoo";

  ret = datastore_json_string_escape(test_str, &str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_escape error.");
  TEST_JSON_STR(ret, str, test_ret_str);
}

void
test_datastore_json_string_escape_05(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str[] = "hoge\ffoo";
  char test_ret_str[] = "hoge\\ffoo";

  ret = datastore_json_string_escape(test_str, &str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_escape error.");
  TEST_JSON_STR(ret, str, test_ret_str);
}

void
test_datastore_json_string_escape_06(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str[] = "hoge\nfoo";
  char test_ret_str[] = "hoge\\nfoo";

  ret = datastore_json_string_escape(test_str, &str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_escape error.");
  TEST_JSON_STR(ret, str, test_ret_str);
}

void
test_datastore_json_string_escape_07(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str[] = "hoge\rfoo";
  char test_ret_str[] = "hoge\\rfoo";

  ret = datastore_json_string_escape(test_str, &str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_escape error.");
  TEST_JSON_STR(ret, str, test_ret_str);
}

void
test_datastore_json_string_escape_08(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str[] = "hoge\tfoo";
  char test_ret_str[] = "hoge\\tfoo";

  ret = datastore_json_string_escape(test_str, &str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_escape error.");
  TEST_JSON_STR(ret, str, test_ret_str);
}

void
test_datastore_json_string_escape_09(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str[] = "hoge\"foo\"bar'baz";
  char test_ret_str[] = "hoge\\\"foo\\\"bar'baz";

  ret = datastore_json_string_escape(test_str, &str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_escape error.");
  TEST_JSON_STR(ret, str, test_ret_str);
}

void
test_datastore_json_string_escape_10(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str[] = "hoge&foo";
  char test_ret_str[] = "hoge&foo";

  ret = datastore_json_string_escape(test_str, &str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_string_escape error.");
  TEST_JSON_STR(ret, str, test_ret_str);
}

void
test_datastore_json_string_escape_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str[] = "hoge";

  ret = datastore_json_string_escape(NULL, &str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_string_escape error.");

  ret = datastore_json_string_escape(test_str, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "datastore_json_string_escape error.");
}

void
test_datastore_json_uint32_flags_append_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  uint32_t test_flags1 = 0x0;
  char test_str1[] =
      "\"key\":[]";
  uint32_t test_flags2 = 0x1;
  char test_str2[] =
      "\"key\":[\"hoge\"]";
  uint32_t test_flags3 = 0x2;
  char test_str3[] =
      "\"key\":[\"foo\"]";
  uint32_t test_flags4 = 0x3;
  char test_str4[] =
      "\"key\":[\"hoge\",\n"
      "\"foo\"]";
  const char *const strs[] = {
    "hoge",
    "foo",
  };
  size_t strs_size =
      sizeof(strs) / sizeof(const char *);

  ret = datastore_json_uint32_flags_append(&ds, "key", test_flags1,
                                           strs, strs_size, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint32_flags_append error.");
  TEST_DSTR(ret, &ds, str, test_str1);
  (void)lagopus_dstring_clear(&ds);

  ret = datastore_json_uint32_flags_append(&ds, "key", test_flags2,
                                           strs, strs_size, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint32_flags_append error.");
  TEST_DSTR(ret, &ds, str, test_str2);
  (void)lagopus_dstring_clear(&ds);

  ret = datastore_json_uint32_flags_append(&ds, "key", test_flags3,
                                           strs, strs_size, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint32_flags_append error.");
  TEST_DSTR(ret, &ds, str, test_str3);
  (void)lagopus_dstring_clear(&ds);

  ret = datastore_json_uint32_flags_append(&ds, "key", test_flags4,
                                           strs, strs_size, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint32_flags_append error.");
  TEST_DSTR(ret, &ds, str, test_str4);
  (void)lagopus_dstring_clear(&ds);
}

void
test_datastore_json_uint16_flags_append_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;
  uint16_t test_flags1 = 0x0;
  char test_str1[] =
      "\"key\":[]";
  uint16_t test_flags2 = 0x1;
  char test_str2[] =
      "\"key\":[\"hoge\"]";
  uint16_t test_flags3 = 0x2;
  char test_str3[] =
      "\"key\":[\"foo\"]";
  uint16_t test_flags4 = 0x3;
  char test_str4[] =
      "\"key\":[\"hoge\",\n"
      "\"foo\"]";
  const char *const strs[] = {
    "hoge",
    "foo",
  };
  size_t strs_size =
      sizeof(strs) / sizeof(const char *);

  ret = datastore_json_uint16_flags_append(&ds, "key", test_flags1,
                                           strs, strs_size, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint16_flags_append error.");
  TEST_DSTR(ret, &ds, str, test_str1);
  (void)lagopus_dstring_clear(&ds);

  ret = datastore_json_uint16_flags_append(&ds, "key", test_flags2,
                                           strs, strs_size, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint16_flags_append error.");
  TEST_DSTR(ret, &ds, str, test_str2);
  (void)lagopus_dstring_clear(&ds);

  ret = datastore_json_uint16_flags_append(&ds, "key", test_flags3,
                                           strs, strs_size, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint16_flags_append error.");
  TEST_DSTR(ret, &ds, str, test_str3);
  (void)lagopus_dstring_clear(&ds);

  ret = datastore_json_uint16_flags_append(&ds, "key", test_flags4,
                                           strs, strs_size, false);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_json_uint16_flags_append error.");
  TEST_DSTR(ret, &ds, str, test_str4);
  (void)lagopus_dstring_clear(&ds);
}
