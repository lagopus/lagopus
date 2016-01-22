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
#include "../ofp_role_request_handler.h"
#include "../ofp_apis.h"
#include "../ofp_role.h"
#include "handler_test_utils.h"
#include "../channel_mgr.h"


static bool test_closed = false;

static const uint64_t dpid = 0xabc;

void
setUp(void) {
}

void
tearDown(void) {
}

static lagopus_result_t
s_ofp_role_request_handle_wrap(struct channel *channel,
                               struct pbuf *pbuf,
                               struct ofp_header *xid_header,
                               struct ofp_error *error) {
  return ofp_role_request_handle(channel, pbuf, xid_header, error);
}

static lagopus_result_t
s_ofp_role_reply_create_wrap(struct channel *channel,
                             struct pbuf **pbuf,
                             struct ofp_header *xid_header) {
  struct ofp_role_request role_request;
  role_request.role = OFPCR_ROLE_SLAVE;
  role_request.generation_id = 0x01;

  return ofp_role_reply_create(channel, pbuf, xid_header, &role_request);
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
test_ofp_role_request_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          s_ofp_role_request_handle_wrap,
          "04 19 00 18 00 00 00 10 "
          /* <--------------------... ofp_role_request
           * <---------------------> ofp_header
           * <> version
           *    <> type
           *       <---> length
           *             <---------> xid
           */
          "00 00 00 03 00 00 00 00 "
          /*...-------------------... ofp_role_request
           * <---------> role = 3 -> OFPCR_ROLE_SLAVE
           *             <---------> padding
           */
          "00 00 00 00 00 00 00 00 ");
  /*...-------------------->  ofp_role_request
   * <---------------------> generation_id = 0
   */

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_role_request_handle(normal) error.");
}

void
test_ofp_role_request_handle_invalid_unknown_role(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_ROLE_REQUEST_FAILED, OFPRRFC_BAD_ROLE);

  ret = check_packet_parse_expect_error(
          s_ofp_role_request_handle_wrap,
          "04 19 00 18 00 00 00 10 "
          "ff ff ff fe 00 00 00 00 "
          /*...-------------------... ofp_role_request
           * <---------> role = 0xfffffffe -> unknown role
           *             <---------> padding
           */
          "00 00 00 00 00 00 00 09 ",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "unknown rule should case error.");
}

void
test_ofp_role_request_handle_invalid_stale_message(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_ROLE_REQUEST_FAILED, OFPRRFC_STALE);

  ret = check_packet_parse_cont(
          s_ofp_role_request_handle_wrap,
          "04 19 00 18 00 00 00 10 "
          "00 00 00 03 00 00 00 00 "
          /*...-------------------... ofp_role_request
           * <---------> role = 3 -> OFPCR_ROLE_SLAVE
           *             <---------> padding
           */
          "00 00 00 00 00 00 00 09 ");
  /*...-------------------->  ofp_role_request
   * <---------------------> generation_id = 9
   */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_role_request_handle(normal) error.");

  ret = check_packet_parse_expect_error(
          s_ofp_role_request_handle_wrap,
          "04 19 00 18 00 00 00 10 "
          "00 00 00 03 00 00 00 00 "
          /*...-------------------... ofp_role_request
           * <---------> role = 3 -> OFPCR_ROLE_SLAVE
           *             <---------> padding
           */
          "00 00 00 00 00 00 00 01 ",
          &expected_error);
  /*...-------------------->  ofp_role_request
   * <---------------------> generation_id = 1
   */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "stale message should cause error");


}


void
test_ofp_role_request_handle_no_body(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* Case of decode error.*/
  ret = check_packet_parse(s_ofp_role_request_handle_wrap, "");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_role_request_handle(error) error.");
}


void
test_ofp_role_request_handle_with_null_agument(void) {
  /* Case of invlid argument.*/
  struct channel *channel =
    channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf pbuf;
  struct ofp_header xid_header;
  lagopus_result_t ret;
  struct ofp_error error;

  /* struct ofp_error error; */
  ret = ofp_role_request_handle(NULL, &pbuf, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL should be treated as invalid arguments");
  ret = ofp_role_request_handle(channel, NULL, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL should be treated as invalid arguments");
  ret = ofp_role_request_handle(channel, &pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL should be treated as invalid arguments");
  ret = ofp_role_request_handle(channel, &pbuf, &xid_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL should be treated as invalid arguments");

  channel_free(channel);
}

void
test_ofp_role_reply_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_create(s_ofp_role_reply_create_wrap,
                            "04 19 00 18 00 00 00 10 "
                            "00 00 00 03 00 00 00 00 "
                            "00 00 00 00 00 00 00 01 ");

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_role_reply_create(normal) error.");
}

void
test_ofp_role_reply_create_with_null_agument(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel =
    channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf;
  struct ofp_header xid_header;
  struct ofp_role_request role_request;
  role_request.role = OFPCR_ROLE_SLAVE;
  role_request.generation_id = 0x01;

  /* TODO add error as a 4th argument */
  /* struct ofp_error error; */
  ret = ofp_role_reply_create(NULL, &pbuf, &xid_header, &role_request);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL should be treated as invalid arguments");
  ret = ofp_role_reply_create(channel, NULL, &xid_header, &role_request);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL should be treated as invalid arguments");
  ret = ofp_role_reply_create(channel, &pbuf, NULL, &role_request);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL should be treated as invalid arguments");

  channel_free(channel);
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
