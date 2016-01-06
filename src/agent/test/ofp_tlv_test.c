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

#include <sys/queue.h>
#include "unity.h"
#include "lagopus_apis.h"
#include "lagopus/pbuf.h"
#include "openflow.h"
#include "openflow13.h"
#include "handler_test_utils.h"
#include "../ofp_tlv.h"

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_ofp_tlv_length_set(void) {
  struct pbuf *test_pbuf;
  struct pbuf *ret_pbuf;
  char normal_input_data[] = "00 03 00 00";
  char normal_output_data[] = "00 03 00 04";
  uint16_t length = 0x04;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* normal */
  create_packet(normal_input_data, &ret_pbuf);
  create_packet(normal_output_data, &test_pbuf);

  ret = ofp_tlv_length_set(ret_pbuf->data, length);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_tlv_length_set error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(ret_pbuf->data, test_pbuf->data, length),
                            "data compare error.");

  pbuf_free(test_pbuf);
  pbuf_free(ret_pbuf);
}

void
test_ofp_tlv_length_sum_normal_pattern0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t total_length = 0;
  uint16_t ret_total_length = total_length;
  uint16_t length = 1;

  ret = ofp_tlv_length_sum(&ret_total_length, length);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_tlv_length_sum error.");
  TEST_ASSERT_EQUAL_MESSAGE(total_length + length, ret_total_length,
                            "length error.");
}

void
test_ofp_tlv_length_sum_normal_pattern1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t total_length = UINT16_MAX - 1;
  uint16_t ret_total_length = total_length;
  uint16_t length = 1;

  ret = ofp_tlv_length_sum(&ret_total_length, length);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_tlv_length_sum error.");
  TEST_ASSERT_EQUAL_MESSAGE(UINT16_MAX, ret_total_length,
                            "length error.");
}

void
test_ofp_tlv_length_sum_normal_pattern2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t total_length = UINT16_MAX;
  uint16_t ret_total_length = total_length;
  uint16_t length = 0;

  ret = ofp_tlv_length_sum(&ret_total_length, length);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_tlv_length_sum error.");
  TEST_ASSERT_EQUAL_MESSAGE(UINT16_MAX, ret_total_length,
                            "length error.");
}

void
test_ofp_tlv_length_sum_normal_pattern3(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t total_length = 0;
  uint16_t ret_total_length = total_length;
  uint16_t length = UINT16_MAX;;

  ret = ofp_tlv_length_sum(&ret_total_length, length);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_tlv_length_sum error.");
  TEST_ASSERT_EQUAL_MESSAGE(UINT16_MAX, ret_total_length,
                            "length error.");
}

void
test_ofp_tlv_length_sum_over_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t total_length = UINT16_MAX;
  uint16_t length = 1;
  uint16_t ret_total_length = total_length;

  ret = ofp_tlv_length_sum(&ret_total_length, length);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "ofp_tlv_length_sum error.");
}

void
test_ofp_tlv_length_sum_over_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t total_length = 1;
  uint16_t length = UINT16_MAX;
  uint16_t ret_total_length = total_length;

  ret = ofp_tlv_length_sum(&ret_total_length, length);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "ofp_tlv_length_sum error.");
}

void
test_ofp_tlv_length_sum_over_length2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t total_length = UINT16_MAX;
  uint16_t length = UINT16_MAX;
  uint16_t ret_total_length = total_length;

  ret = ofp_tlv_length_sum(&ret_total_length, length);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "ofp_tlv_length_sum error.");
}
