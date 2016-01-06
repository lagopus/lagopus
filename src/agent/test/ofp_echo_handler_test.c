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
#include "../ofp_echo_handler.h"
#include "handler_test_utils.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
}

static lagopus_result_t
s_ofp_echo_request_send_wrap(struct channel *channel,
                             struct ofp_header *xid_header) {
  (void) xid_header;
  return ofp_echo_request_send(channel);
}

static lagopus_result_t
s_ofp_echo_reply_handle_wrap(struct channel *channel,
                             struct pbuf *pbuf,
                             struct ofp_header *xid_header,
                             struct ofp_error *error) {
  return ofp_echo_reply_handle(channel, pbuf, xid_header, error);
}

/* TODO: remove this */
static lagopus_result_t
ofp_echo_request_handle_wrap(struct channel *channel,
                             struct pbuf *pbuf,
                             struct ofp_header *header,
                             struct ofp_error *error) {
  return ofp_echo_request_handle(channel, pbuf, header, error);
}

static lagopus_result_t
ofp_echo_reply_create_wrap(struct channel *channel,
                           struct pbuf **pbuf,
                           struct ofp_header *xid_header) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *req_pbuf = pbuf_alloc(sizeof(struct ofp_header));
  uint8_t req_data[8] = {0x04, 0x02, 0x00, 0x08,
                         0x00, 0x00, 0x00, 0x10
                        };
  size_t req_length = 8;

  /* set request data. */
  memcpy(req_pbuf->data, req_data, req_length);
  req_pbuf->plen = 0;
  req_pbuf->putp += req_length;
  req_pbuf->getp = req_pbuf->putp;

  ret = ofp_echo_reply_create(channel, pbuf,
                              req_pbuf, xid_header);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_echo_reply_create(normal) error.");
  pbuf_free(req_pbuf);

  return ret;
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
test_ofp_echo_reply_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_create(ofp_echo_reply_create_wrap,
                            "04 03 00 08 00 00 00 10");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_echo_reply_create(normal) error.");
}

static lagopus_result_t
ofp_echo_reply_create_with_data_wrap(struct channel *channel,
                                     struct pbuf **pbuf,
                                     struct ofp_header *xid_header) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *req_pbuf = pbuf_alloc(sizeof(struct ofp_header));
  uint8_t req_data[10] = {0x04, 0x02, 0x00, 0x0a,
                          0x00, 0x00, 0x00, 0x10,
                          0xab, 0xcd
                         };
  size_t req_length = 10;
  size_t data_length = 2;

  /* set request data. */
  memcpy(req_pbuf->data, req_data, req_length);
  req_pbuf->plen = data_length;
  req_pbuf->putp += req_length;
  req_pbuf->getp = req_pbuf->putp - data_length;

  ret = ofp_echo_reply_create(channel, pbuf,
                              req_pbuf, xid_header);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_echo_reply_create(normal) error.");
  pbuf_free(req_pbuf);

  return ret;
}

void
test_ofp_echo_reply_create_with_data(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_create(ofp_echo_reply_create_with_data_wrap,
                            "04 03 00 0a 00 00 00 10"
                            "ab cd");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_echo_reply_create_with_data(normal) error.");
}

void
test_ofp_echo_request_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_create(ofp_echo_request_create,
                            "04 02 00 08 00 00 00 10");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_echo_request_create(normal) error.");
}

void
test_ofp_echo_request_send(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_send(s_ofp_echo_request_send_wrap);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_echo_request_send(normal) error.");
}

void
test_ofp_echo_request_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(ofp_echo_request_handle_wrap,
                           "04 02 00 08 00 00 00 10");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_echo_request_handle(normal) error.");
}

void
test_ofp_echo_request_handle_header_length_too_short(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(ofp_echo_request_handle_wrap,
                                        // packets says "My length is 8 bytes"
                                        // but the real length is 7 bytes.
                                        "04 02 00 08 00 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_echo_request_handle(error) error.");
}

void
test_ofp_echo_reply_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(s_ofp_echo_reply_handle_wrap,
                           "04 03 00 08 00 00 00 10");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_echo_reply_handle(normal) error.");
}

void
test_ofp_echo_reply_handle_invalid_length_too_short(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(s_ofp_echo_reply_handle_wrap,
                                        // packets says "My length is 8 bytes"
                                        // but the real length is 7 bytes.
                                        "04 02 00 08 00 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_echo_reply_handle(error) error.");
}

void
test_ofp_echo_reply_handle_invalid_version_too_large(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_VERSION);
  ret = check_packet_parse_expect_error(s_ofp_echo_reply_handle_wrap,
                                        // set verion 1.4
                                        "05 02 00 08 00 00 00 10",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_echo_reply_handle(error) error.");
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
