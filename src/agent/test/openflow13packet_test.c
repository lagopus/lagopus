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
#include "lagopus/pbuf.h"
#include "openflow.h"
#include "openflow13.h"
#include "handler_test_utils.h"
#include "../openflow13packet.h"


void
setUp(void) {
}

void
tearDown(void) {
}

#define TEST_TRACE_MAX_SIZE 84

void
test_ofp_action_output_trace_nromal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_action_output ofp;
  char str[TEST_TRACE_MAX_SIZE];
  char ret_str[] =
    "ofp_action_output [ type: 1, len: 20, "
    "port: 300, max_len: 4000, "
    "pad: 0 0 0 0 0 0 ,]";

  /* Set test data. */
  ofp.type = 1;
  ofp.len = 20;
  ofp.port = 300;
  ofp.max_len = 4000;
  memset(ofp.pad, 0 ,sizeof(ofp.pad));

  /* Call func. */
  ret = ofp_action_output_trace(&ofp, sizeof(ofp), str, TEST_TRACE_MAX_SIZE);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_action_output_trace(normal) error.");
  TEST_ASSERT_EQUAL_MESSAGE(TEST_TRACE_MAX_SIZE, sizeof(ret_str),
                            "ret_str length error.");
  TEST_ASSERT_EQUAL_MESSAGE(TEST_TRACE_MAX_SIZE, sizeof(str),
                            "str length error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(ret_str, str, sizeof(ret_str)),
                            "String compare error.");
}

void
test_ofp_action_output_trace_over_max_len(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_action_output ofp;
  char str[TEST_TRACE_MAX_SIZE];

  /* Set test data. */
  ofp.type = 1;
  ofp.len = 20;
  ofp.port = 300;
  ofp.max_len = 4000;
  memset(ofp.pad, 0 ,sizeof(ofp.pad));

  /* Call func. */
  ret = ofp_action_output_trace(&ofp, sizeof(ofp), str, TEST_TRACE_MAX_SIZE - 1);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "ofp_action_output_trace(over max len) error.");
}

void
test_ofp_action_output_trace_bad_packet_size(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_action_output ofp;
  char str[TEST_TRACE_MAX_SIZE];

  /* Set test data. */
  ofp.type = 1;
  ofp.len = 20;
  ofp.port = 300;
  ofp.max_len = 4000;
  memset(ofp.pad, 0 ,sizeof(ofp.pad));

  /* Call func. */
  ret = ofp_action_output_trace(&ofp, sizeof(ofp) + 1, str, TEST_TRACE_MAX_SIZE);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "ofp_action_output_trace(bad_packet_size) error.");

  /* Call func. */
  ret = ofp_action_output_trace(&ofp, sizeof(ofp) - 1, str, TEST_TRACE_MAX_SIZE);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "ofp_action_output_trace(bad_packet_size) error.");
}
