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
#include "../ofp_get_async_handler.h"
#include "lagopus/ofp_dp_apis.h"
#include "handler_test_utils.h"
#include "../channel_mgr.h"

static bool s_is_invalid_argument = false;
static bool test_closed = false;

void
setUp(void) {
}

void
tearDown(void) {
}

static lagopus_result_t
s_ofp_get_async_request_handle_wrap(struct channel *channel,
                                    struct pbuf *pbuf,
                                    struct ofp_header *xid_header,
                                    struct ofp_error *error) {
  return ofp_get_async_request_handle(channel, pbuf, xid_header, error);
}

static lagopus_result_t
s_ofp_get_async_request_handle_wrap_error(struct channel *channel,
    struct pbuf *pbuf,
    struct ofp_header *xid_header,
    struct ofp_error *error) {
  return ofp_get_async_request_handle(channel, pbuf, xid_header, error);
}

static lagopus_result_t
s_ofp_get_async_reply_create_wrap(struct channel *channel,
                                  struct pbuf **pbuf,
                                  struct ofp_header *xid_header) {
  return ofp_get_async_reply_create(channel, pbuf, xid_header);
}

static lagopus_result_t
s_ofp_get_async_reply_create_wrap_error(struct channel *channel,
                                        struct pbuf **pbuf,
                                        struct ofp_header *xid_header) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (s_is_invalid_argument == true) {
    ret = ofp_get_async_reply_create(NULL, NULL, NULL);
  } else {
    ret = ofp_get_async_reply_create(channel, pbuf, xid_header);
  }
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
test_ofp_get_async_request_handle(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          s_ofp_get_async_request_handle_wrap,
          "04 1C 00 20 00 00 00 10 "
          /* <--------------------... ofp_async_config
           * <--------------------->  ofp_header
           * <> version
           *    <> type
           *       <---> length
           *             <---------> xid
           */
          "00 00 00 00 00 00 00 00 "
          /*...--------------------.. ofp_async_config
           * <---------------------> packet_in_mask[2]
           */
          "00 00 00 00 00 00 00 00 "
          /*...--------------------.. ofp_async_config
           * <---------------------> port_status_mask[2]
           */
          "00 00 00 00 00 00 00 00 "
          /*...-------------------->  ofp_async_config
           * <---------------------> flow_removed_mask[2]
           */
        );

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_get_async_request_handle(normal) error.");
}

void
test_ofp_get_async_request_handle_invalid_no_body(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  /* Case of decode error.*/
  ret = check_packet_parse_expect_error(
          s_ofp_get_async_request_handle_wrap_error,
          "",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_get_async_request_handle(error) error.");
}

void
test_ofp_get_async_handle_invalid_length_too_large(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(
          s_ofp_get_async_request_handle_wrap,
          "04 1C 00 30 00 00 00 10"
          /* <--------------------... ofp_async_config
           * <--------------------->  ofp_header
           *       <---> length must be 0x20, 0x30 is too large!
           */
          "00 00 00 00 00 00 00 00"
          "00 00 00 00 00 00 00 00"
          "00 00 00 00 00 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "get_async_handle error.");
}

void
test_ofp_get_async_handle_invalid_body_too_small(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(
          s_ofp_get_async_request_handle_wrap,
          "04 1C 00 20 00 00 00 10"
          /* <--------------------... ofp_async_config
           * <--------------------->  ofp_header
           */
          "00 00 00 00 00 00 00 00"
          /* "00 00 00 00 00 00 00 00" too small ! */
          "00 00 00 00 00 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "get_async_handle error.");
}

void
test_ofp_get_async_request_handle_invalid_argument(void) {
  /* Case of invalid argument.*/
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header xid_header;
  struct ofp_error ignored_error = {0, 0, {NULL}};

  ret = ofp_get_async_request_handle(NULL, pbuf, &xid_header, &ignored_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL checking is requred.");

  ret = ofp_get_async_request_handle(channel, NULL, &xid_header, &ignored_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL checking is requred.");

  ret = ofp_get_async_request_handle(channel, pbuf, NULL, &ignored_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL checking is requred.");

  ret = ofp_get_async_request_handle(channel, pbuf, &xid_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL checking is requred.");

  channel_free(channel);
  pbuf_free(pbuf);
}

void
test_ofp_get_async_reply_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_create(s_ofp_get_async_reply_create_wrap,
                            "04 1B 00 20 00 00 00 10 "
                            "ff ff ff ff ff ff ff ff "
                            "ff ff ff ff ff ff ff ff "
                            "ff ff ff ff ff ff ff ff ");

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_get_async_reply_create(normal) error.");
}

void
test_ofp_get_async_reply_create_error(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  s_is_invalid_argument = true;
  /* Case of invalid argument.*/
  ret = check_packet_create(s_ofp_get_async_reply_create_wrap_error,
                            "04 1B 00 20 00 00 00 10 "
                            "00 00 00 00 00 00 00 00 "
                            "00 00 00 00 00 00 00 00 "
                            "00 00 00 00 00 00 00 00 ");

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_get_async_reply_create(error) error.");
}

void
test_close(void) {
  test_closed = true;
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
