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
#include "../ofp_features_handler.h"
#include "handler_test_utils.h"

void
setUp(void) {
}

void
tearDown(void) {
}

static lagopus_result_t
s_ofp_features_reply_create_wrap(struct channel *channel,
                                 struct pbuf **pbuf,
                                 struct ofp_header *xid_header) {
  return ofp_features_reply_create(channel, pbuf, xid_header);
}

void
test_ofp_features_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(ofp_features_request_handle,
                           "04 05 00 08 00 00 00 10");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_features_request_handle(normal) error.");
}

void
test_ofp_features_handle_invalid_wrong_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_features_request_handle,
          "04 05 00 08 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_features_request_handle(error) error.");
}

void
test_ofp_features_handle_invalid_version_too_large(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_VERSION);
  ret = check_packet_parse_expect_error(
          ofp_features_request_handle,
          "05 05 00 08 00 00 00 10",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_features_request_handle(error) error.");
}

void
test_ofp_features_reply_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_create(s_ofp_features_reply_create_wrap,
                            "04 06 00 20 00 00 00 10"
                            "00 00 00 00 00 00 0a bc"
                            "ff ff ff ff ff 00 00 00"
                            "00 00 01 6f 00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_features_reply_create(normal) error.");
}
