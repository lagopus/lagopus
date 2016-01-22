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
#include "../cmd_common.h"

void
setUp(void) {
}

void
tearDown(void) {
}

#define TEST_CMD_OPT_NAME(_ret, _name, _test_name, _ret_name,       \
                          _is_added, _ret_is_added) {               \
  _ret = cmd_opt_name_get((_test_name), (_name), (_is_added));    \
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, (_ret),            \
                            "cmd_opt_name_get error.");           \
  TEST_ASSERT_EQUAL_MESSAGE(0, strcmp((_ret_name), *(_name)),     \
                            "string compare error.");             \
  TEST_ASSERT_EQUAL_MESSAGE((_ret_is_added), *(_is_added),        \
                            "is_added error.");                   \
  free(*(_name));                                                 \
}

void
test_cmd_opt_name_get_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *name = NULL;
  const char *test_name = "hoge";
  const char *ret_name = "hoge";
  bool is_added;
  bool ret_is_added = true;

  TEST_CMD_OPT_NAME(ret, &name, test_name, ret_name,
                    &is_added, ret_is_added);
}

void
test_cmd_opt_name_get_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *name = NULL;
  const char *test_name = "+hoge";
  const char *ret_name = "hoge";
  bool is_added;
  bool ret_is_added = true;

  TEST_CMD_OPT_NAME(ret, &name, test_name, ret_name,
                    &is_added, ret_is_added);
}

void
test_cmd_opt_name_get_03(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *name = NULL;
  const char *test_name = "-hoge";
  const char *ret_name = "hoge";
  bool is_added;
  bool ret_is_added = false;

  TEST_CMD_OPT_NAME(ret, &name, test_name, ret_name,
                    &is_added, ret_is_added);
}

void
test_cmd_opt_name_get_04(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *name = NULL;
  const char *test_name = "~hoge";
  const char *ret_name = "hoge";
  bool is_added;
  bool ret_is_added = false;

  TEST_CMD_OPT_NAME(ret, &name, test_name, ret_name,
                    &is_added, ret_is_added);
}

void
test_cmd_opt_name_get_invalid_args(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *name = NULL;
  const char *test_name = "hoge";
  const char *null_name = NULL;
  const char *null_str_name = "\0";
  bool is_added;

  ret = cmd_opt_name_get(null_name, &name, &is_added);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "cmd_opt_name_get error (invalid_args).");

  ret = cmd_opt_name_get(null_str_name, &name, &is_added);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "cmd_opt_name_get error (invalid_args).");

  ret = cmd_opt_name_get(test_name, NULL, &is_added);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "cmd_opt_name_get error (invalid_args).");

  ret = cmd_opt_name_get(test_name, &name, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "cmd_opt_name_get error (invalid_args).");
}

void
test_cmd_mac_addr_parse(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *mac_addr = "12:34:56:78:90:af";
  mac_address_t test_addr = {0x12, 0x34, 0x56, 0x78, 0x90, 0xaf};
  mac_address_t ret_addr;

  ret = cmd_mac_addr_parse(mac_addr, ret_addr);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "cmd_mac_addr_parse error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(test_addr,
                                      ret_addr,
                                      sizeof(uint8_t) * MAC_ADDR_STR_LEN),
                            "mac addr compare error.");
}

void
test_cmd_mac_addr_parse_out_of_range(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *mac_addr1 = "12:34:56:78:90:ax";
  const char *mac_addr2 = "12:34:56:78:90:a-";
  const char *mac_addr3 = "12:34:56:78:90:ab:a";
  const char *mac_addr4 = "12:34:56:78:90:a";
  const char *mac_addr5 = "12:34:56:78:90;ab";
  const char *mac_addr6 = "12:34:56:78:90:";
  const char *mac_addr7 = "hoge";
  mac_address_t ret_addr;

  ret = cmd_mac_addr_parse(mac_addr1, ret_addr);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "cmd_mac_addr_parse error (out_of_range).");

  ret = cmd_mac_addr_parse(mac_addr2, ret_addr);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "cmd_mac_addr_parse error (out_of_range).");

  ret = cmd_mac_addr_parse(mac_addr3, ret_addr);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "cmd_mac_addr_parse error (out_of_range).");

  ret = cmd_mac_addr_parse(mac_addr4, ret_addr);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "cmd_mac_addr_parse error (out_of_range).");

  ret = cmd_mac_addr_parse(mac_addr5, ret_addr);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "cmd_mac_addr_parse error (out_of_range).");

  ret = cmd_mac_addr_parse(mac_addr6, ret_addr);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "cmd_mac_addr_parse error (out_of_range).");

  ret = cmd_mac_addr_parse(mac_addr7, ret_addr);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "cmd_mac_addr_parse error (out_of_range).");
}

void
test_cmd_mac_addr_parse_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *mac_addr = "12:34:56:78:90:ab";
  const char *mac_addr_empty = "";
  mac_address_t ret_addr;

  ret = cmd_mac_addr_parse(NULL, ret_addr);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "cmd_mac_addr_parse error (null).");

  ret = cmd_mac_addr_parse(mac_addr_empty, ret_addr);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "cmd_mac_addr_parse error (null).");

  ret = cmd_mac_addr_parse(mac_addr, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "cmd_mac_addr_parse error (null).");
}

void
test_cmd_uint_parse_uint8(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  enum cmd_uint_type type = CMD_UINT8;
  const char *uint_max_str = "255";
  uint8_t test_uint_max = UINT8_MAX;
  const char *uint_min_str = "0";
  uint8_t test_uint_min = 0;
  union cmd_uint cmd_uint;

  /* max */
  ret = cmd_uint_parse(uint_max_str, type, &cmd_uint);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "cmd_uint_parse error.");
  TEST_ASSERT_EQUAL_MESSAGE(test_uint_max, cmd_uint.uint8,
                            "uint compare error.");

  /* min */
  ret = cmd_uint_parse(uint_min_str, type, &cmd_uint);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "cmd_uint_parse error.");
  TEST_ASSERT_EQUAL_MESSAGE(test_uint_min, cmd_uint.uint8,
                            "uint compare error.");
}

void
test_cmd_uint_parse_uint16(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  enum cmd_uint_type type = CMD_UINT16;
  const char *uint_max_str = "65535";
  uint16_t test_uint_max = UINT16_MAX;
  const char *uint_min_str = "0";
  uint16_t test_uint_min = 0;
  union cmd_uint cmd_uint;

  /* max */
  ret = cmd_uint_parse(uint_max_str, type, &cmd_uint);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "cmd_uint_parse error.");
  TEST_ASSERT_EQUAL_MESSAGE(test_uint_max, cmd_uint.uint16,
                            "uint compare error.");

  /* min */
  ret = cmd_uint_parse(uint_min_str, type, &cmd_uint);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "cmd_uint_parse error.");
  TEST_ASSERT_EQUAL_MESSAGE(test_uint_min, cmd_uint.uint16,
                            "uint compare error.");
}

void
test_cmd_uint_parse_uint32(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  enum cmd_uint_type type = CMD_UINT32;
  const char *uint_max_str = "4294967295";
  uint32_t test_uint_max = UINT32_MAX;
  const char *uint_min_str = "0";
  uint32_t test_uint_min = 0;
  union cmd_uint cmd_uint;

  /* max */
  ret = cmd_uint_parse(uint_max_str, type, &cmd_uint);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "cmd_uint_parse error.");
  TEST_ASSERT_EQUAL_MESSAGE(test_uint_max, cmd_uint.uint32,
                            "uint compare error.");

  /* min */
  ret = cmd_uint_parse(uint_min_str, type, &cmd_uint);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "cmd_uint_parse error.");
  TEST_ASSERT_EQUAL_MESSAGE(test_uint_min, cmd_uint.uint32,
                            "uint compare error.");
}

void
test_cmd_uint_parse_uint64(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  enum cmd_uint_type type = CMD_UINT64;
  const char *uint_max_str = "18446744073709551615";
  uint64_t test_uint_max = UINT64_MAX;
  const char *uint_min_str = "0";
  uint64_t test_uint_min = 0;
  union cmd_uint cmd_uint;

  /* max */
  ret = cmd_uint_parse(uint_max_str, type, &cmd_uint);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "cmd_uint_parse error.");
  TEST_ASSERT_EQUAL_MESSAGE(test_uint_max, cmd_uint.uint64,
                            "uint compare error.");

  /* min */
  ret = cmd_uint_parse(uint_min_str, type, &cmd_uint);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "cmd_uint_parse error.");
  TEST_ASSERT_EQUAL_MESSAGE(test_uint_min, cmd_uint.uint64,
                            "uint compare error.");
}

void
test_cmd_uint_parse_out_of_range(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *uint8_over_str = "256";
  const char *uint16_over_str = "65536";
  const char *uint32_over_str = "4294967296";
  const char *uint64_over_str = "18446744073709551616";
  union cmd_uint cmd_uint;

  /* uint8 */
  ret = cmd_uint_parse(uint8_over_str, CMD_UINT8, &cmd_uint);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "cmd_uint_parse error (out_of_range).");

  /* uint16 */
  ret = cmd_uint_parse(uint16_over_str, CMD_UINT16, &cmd_uint);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "cmd_uint_parse error (out_of_range).");

  /* uint32 */
  ret = cmd_uint_parse(uint32_over_str, CMD_UINT32, &cmd_uint);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "cmd_uint_parse error (out_of_range).");

  /* uint64 */
  ret = cmd_uint_parse(uint64_over_str, CMD_UINT64, &cmd_uint);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "cmd_uint_parse error (out_of_range).");
}

void
test_cmd_uint_parse_invalid_args(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *uint_str = "0";
  const char *uint_str_empty = "";
  union cmd_uint cmd_uint;

  ret = cmd_uint_parse(NULL, CMD_UINT8, &cmd_uint);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "cmd_uint_parse error (invalid_args).");

  ret = cmd_uint_parse(uint_str, CMD_UINT8, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "cmd_uint_parse error (invalid_args).");

  ret = cmd_uint_parse(uint_str_empty, CMD_UINT8, &cmd_uint);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "cmd_uint_parse error (invalid_args).");

  ret = cmd_uint_parse(uint_str_empty, CMD_UINT_MAX, &cmd_uint);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "cmd_uint_parse error (invalid_args).");
}

