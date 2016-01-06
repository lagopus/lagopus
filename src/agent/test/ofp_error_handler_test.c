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
#include "../ofp_error_handler.h"
#include "handler_test_utils.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
}

struct ofp_error ofp_error;

/* TODO remove */
static lagopus_result_t
ofp_unsupported_handle_wrap(struct channel *channel,
                            struct pbuf *pbuf,
                            struct ofp_header *xid_header,
                            struct ofp_error *error) {

  (void) channel;
  (void) pbuf;
  (void) xid_header;
  return ofp_unsupported_handle(error);
}

static lagopus_result_t
s_ofp_error_msg_handle_wrap(struct channel *channel,
                            struct pbuf *pbuf,
                            struct ofp_header *xid_header,
                            struct ofp_error *error) {
  return ofp_error_msg_handle(channel, pbuf, xid_header, error);
}

static lagopus_result_t
s_ofp_error_msg_create_wrap(struct channel *channel,
                            struct pbuf **pbuf,
                            struct ofp_header *xid_header) {
  return ofp_error_msg_create(channel, &ofp_error, xid_header, pbuf);
}

static lagopus_result_t
s_ofp_error_msg_send_wrap(struct channel *channel,
                          struct ofp_header *xid_header) {
  return ofp_error_msg_send(channel, xid_header, &ofp_error);
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
test_ofp_error_msg_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(s_ofp_error_msg_handle_wrap,
                           "04 01 00 0c 00 00 00 10"
                           "00 01 00 01");
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "ofp_error_msg_handle(normal) error.");
}

void
test_ofp_error_msg_handle_bad_version(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_VERSION);
  ret = check_packet_parse_expect_error(s_ofp_error_msg_handle_wrap,
                                        "05 01 00 0c 00 00 00 10"
                                        "00 01 00 01",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OFP_ERROR,
                            "ofp_error_msg_handle(error) error.");
}

void
test_ofp_error_msg_handle_wrong_long_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(s_ofp_error_msg_handle_wrap,
                                        "04 01 00 0c 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OFP_ERROR,
                            "ofp_error_msg_handle(error) error.");
}

void
test_ofp_error_msg_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ofp_error.type = OFPET_HELLO_FAILED;
  ofp_error.code = OFPHFC_EPERM;
  ofp_error.str = "TEST";
  ret = check_packet_create(s_ofp_error_msg_create_wrap,
                            "04 01 00 11 00 00 00 10"
                            "00 00 00 01 54 45 53 54"
                            "00");
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "ofp_error_msg_handle(normal) error.");
}

void
test_ofp_error_msg_create_failed_request(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t req[] = {0x01, 0x02, 0x03};
  size_t req_len = 3;
  struct pbuf *pbuf;

  /* create failed request data. */
  pbuf = pbuf_alloc(req_len);
  memcpy(pbuf->data, req, req_len);
  pbuf->putp += req_len;
  pbuf->plen = req_len;

  ofp_error.type = OFPET_BAD_REQUEST;
  ofp_error.code = OFPBRC_BAD_TYPE;
  ofp_error.req = pbuf;
  ret = check_packet_create(s_ofp_error_msg_create_wrap,
                            "04 01 00 0f 00 00 00 10"
                            "00 01 00 01 01 02 03");
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "ofp_error_msg_handle(failed_request) error.");

  pbuf_free(pbuf);
}

void
test_ofp_error_msg_send(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ofp_error.type = OFPET_HELLO_FAILED;
  ofp_error.code = OFPHFC_EPERM;
  ofp_error.str = "TEST";
  ret = check_packet_send(s_ofp_error_msg_send_wrap);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "ofp_error_msg_send(normal) error.");
}

void
test_ofp_unsupported_handle(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_TYPE);
  ret = check_packet_parse_expect_error(
          ofp_unsupported_handle_wrap,
          "04 06 00 20 00 00 00 65 00 00 00 00 00 00 0a bc "
          "00 00 ff ff 03 00 00 00 00 00 00 00 00 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_unsupported_handle(normal) error.");
}

void
test_ofp_error_msg_from_queue_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid = 0x01;

  ret = ofp_error_msg_from_queue_handle(NULL, dpid);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_error_msg_from_queue_handle(null) error.");
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
