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
#include "../ofp_hello_handler.h"
#include "handler_test_utils.h"
#include "openflow.h"
#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_dp_apis.h"
#include "../channel_mgr.h"

uint8_t s_require_version = OPENFLOW_VERSION_0_0;

static lagopus_result_t
s_ofp_hello_handle_wrap(struct channel *channel,
                        struct pbuf *pbuf,
                        struct ofp_header *xid_header,
                        struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) xid_header;
  channel_version_set(channel, OPENFLOW_VERSION_0_0);

  ret = ofp_hello_handle(channel, pbuf, error);
  /* check version */
  TEST_ASSERT_EQUAL_MESSAGE(s_require_version, channel_version_get(channel),
                            "version error.");
  return ret;
}

static lagopus_result_t
s_ofp_hello_send_wrap(struct channel *channel,
                      struct ofp_header *xid_header) {
  (void) xid_header;
  return ofp_hello_send(channel);
}

static lagopus_result_t
s_ofp_hello_create_wrap(struct channel *channel,
                        struct pbuf **pbuf,
                        struct ofp_header *xid_header) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) xid_header;

  ret = ofp_hello_create(channel, pbuf);

  return ret;
}

static void
s_ofp_hello_create_null_wrap(struct channel **channels,
                             struct ofp_header *xid_headers,
                             struct pbuf *pbuf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void) xid_headers;

  ret = ofp_hello_create(NULL, &pbuf);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_hello_create(null) error.");

  ret = ofp_hello_create(channels[0], NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_hello_create(null) error.");
}

void
setUp(void) {
  s_require_version = OPENFLOW_VERSION_0_0;
}

void
tearDown(void) {
}

void
test_prologue(void) {
  lagopus_result_t r;
  const char *argv0 =
      ((IS_VALID_STRING(lagopus_get_command_name()) == true) ?
       lagopus_get_command_name() : "callout_test");
  const char * const argv[] = {
    argv0, NULL
  };

#define N_CALLOUT_WORKERS	1
  (void)lagopus_mainloop_set_callout_workers_number(N_CALLOUT_WORKERS);
  r = lagopus_mainloop_with_callout(1, argv, NULL, NULL,
                                    false, false, true);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  channel_mgr_initialize();
}
void
test_ofp_hello_send(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  s_require_version = OPENFLOW_VERSION_1_3;
  res = check_packet_send(s_ofp_hello_send_wrap);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res,
                            "ofp_hello_send error.");
}

void
test_hello_handle_normal_pattern(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  s_require_version = OPENFLOW_VERSION_1_3;
  res = check_packet_parse(s_ofp_hello_handle_wrap,
                           "01 00 00 10 00 00 00 64"
                           "00 01 00 08 00 00 00 10");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res,
                            "hello_handle error.");
}

void
test_hello_handle_normal_pattern_without_version_bitmaps(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  s_require_version = OPENFLOW_VERSION_1_3;
  res = check_packet_parse(s_ofp_hello_handle_wrap,
                           "04 00 00 08 00 00 00 64");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res,
                            "hello_handle without version bitmaps error.");
}

void
test_hello_handle_incompatible_version_without_version_bitmaps(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  s_require_version = OPENFLOW_VERSION_0_0;
  ofp_error_set(&expected_error, OFPET_HELLO_FAILED, OFPHFC_INCOMPATIBLE);
  res = check_packet_parse_expect_error(s_ofp_hello_handle_wrap,
                                        "05 00 00 08 00 00 00 64",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, res,
                            "hello_handle without version bitmaps error.");
}

void
test_hello_handle_many_normal_pattern(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  s_require_version = OPENFLOW_VERSION_1_3;
  res = check_packet_parse(s_ofp_hello_handle_wrap,
                           "01 00 00 14 00 00 00 64"
                           "00 01 00 0c 00 00 00 10"
                           "00 00 00 01 00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res,
                            "hello_handle(many) error.");
}

void
test_hello_handle_invalid__version_is_too_small(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_HELLO_FAILED, OFPHFC_INCOMPATIBLE);
  s_require_version = OPENFLOW_VERSION_0_0;
  // remind that the default version of the bridge is OPENFLOW_VERSION_1_3
  res = check_packet_parse_expect_error(s_ofp_hello_handle_wrap,
                                        "00 00 00 10 00 00 00 64"
                                        // set hello version to 0_0
                                        "00 01 00 08 00 00 00 01",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, res,
                            "hello_handle(error) error.");
}

void
test_hello_handle_invalid_hello_version_is_too_large(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_HELLO_FAILED, OFPHFC_INCOMPATIBLE);
  s_require_version = OPENFLOW_VERSION_0_0; // expected failure
  // remind that the default version of the bridge is OPENFLOW_VERSION_1_3
  res = check_packet_parse_expect_error(s_ofp_hello_handle_wrap,
                                        "05 00 00 10 00 00 00 64"
                                        // set hello version to 1_4
                                        "00 01 00 08 00 00 00 20",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, res,
                            "hello_handle(error) error.");
}

void
test_hello_handle_normal_pattern_masked_version(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  s_require_version = OPENFLOW_VERSION_1_3;
  // remind that the default version of the bridge is OPENFLOW_VERSION_1_3
  res = check_packet_parse(s_ofp_hello_handle_wrap,
                           // set the hightest supported version to 1_4
                           "05 00 00 10 00 00 00 64"
                           // add 1_3 supported version
                           "00 01 00 08 00 00 00 30");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res,
                            "hello_handle(error) error.");
}

void
test_hello_handle_normal_pattern_odd_length(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  s_require_version = OPENFLOW_VERSION_0_0;

  res = check_packet_parse_expect_error(s_ofp_hello_handle_wrap,
                                        // set the hightest supported
                                        // version to 1_4
                                        "05 00 00 11 00 00 00 64"
                                        // add 1_3 supported version
                                        // length of version bitmap 9
                                        "00 01 00 09 00 00 00 00"
                                        "30"
                                        // (((9 + 7) / 8) * 8) - 9) = 7
                                        // padding 7 bytes
                                        "00 00 00 00 00 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, res,
                            "hello_handle(error) error.");
}

void
test_hello_handle_invalid_0_length_for_version_bitmaps(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  s_require_version = OPENFLOW_VERSION_0_0;

  res = check_packet_parse_expect_error(s_ofp_hello_handle_wrap,
                                        "05 00 00 10 00 00 00 64"
                                        // (((4 + 7) / 8) * 8) - 4) = 4
                                        "00 01 00 04 00 00 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, res,
                            "hello_handle(error) error.");
}

void
test_hello_handle_invalid_hello_length_for_version_bitmaps(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  s_require_version = OPENFLOW_VERSION_0_0;

  res = check_packet_parse_expect_error(s_ofp_hello_handle_wrap,
                                        "04 00 00 10 00 00 00 64"
                                        "00 01 00 05 00 00 00 ff",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, res,
                            "hello_handle(error) error.");
}

void
test_hello_handle_invalid_hello_too_long_length(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  s_require_version = OPENFLOW_VERSION_1_3;
  // remind that the default version of the bridge is OPENFLOW_VERSION_1_3
  res = check_packet_parse(s_ofp_hello_handle_wrap,
                           // length of this message is 16
                           "05 00 00 10 00 00 00 64"
                           // length of version bitmap is 16, but it is too long!
                           "00 01 00 10 00 00 00 00"
                           "00 00 00 00 00 00 00 10"
                          );
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res,
                            "hello_handle(error) error.");
}

void
test_hello_handle_invalid_short_length(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  s_require_version = OPENFLOW_VERSION_0_0;
  // remind that the default version of the bridge is OPENFLOW_VERSION_1_3
  res = check_packet_parse(s_ofp_hello_handle_wrap,
                           "05 00 00 10 00 00 00 64"
                           // length of version bitmap is 0!
                           "00 01 00 00 00 00 00 00"
                          );
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, res,
                            "hello_handle(error) error.");
}

void
test_hello_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = check_packet_create(s_ofp_hello_create_wrap,
                            "04 00 00 10 00 00 00 10"
                            "00 01 00 08 00 00 00 10");

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "hello_handle_create error.");
}

void
test_hello_create_null(void) {
  data_create(s_ofp_hello_create_null_wrap,
              "");
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
