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
#include "../ofp_set_async_handler.h"
#include "../ofp_apis.h"
#include "../ofp_role.h"
#include "lagopus/ofp_dp_apis.h"
#include "handler_test_utils.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
}

static lagopus_result_t
s_ofp_set_async_handle_wrap(struct channel *channel,
                            struct pbuf *pbuf,
                            struct ofp_header *xid_header,
                            struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = ofp_set_async_handle(channel, pbuf, xid_header, error);
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
test_ofp_set_async_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = check_packet_parse(
          s_ofp_set_async_handle_wrap,
          "04 1C 00 20 00 00 00 10"
          /* <--------------------... ofp_async_config
           * <--------------------->  ofp_header
           * <> version
           *    <> type
           *       <---> length
           *             <---------> xid
           */
          "00 00 00 00 00 00 00 00"
          /*...--------------------.. ofp_async_config
           * <---------------------> packet_in_mask[2]
           */
          "00 00 00 00 00 00 00 00"
          /*...--------------------.. ofp_async_config
           * <---------------------> port_status_mask[2]
           */
          "00 00 00 00 00 00 00 00"
          /*...-------------------->  ofp_async_config
           * <---------------------> flow_removed_mask[2]
           */
        );
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "set_async_handle error.");
}

void
test_ofp_set_async_handle_invalid_length_too_large(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(
          s_ofp_set_async_handle_wrap,
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
                            "set_async_handle error.");
}

void
test_ofp_set_async_handle_invalid_body_too_small(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(
          s_ofp_set_async_handle_wrap,
          "04 1C 00 20 00 00 00 10"
          /* <--------------------... ofp_async_config
           * <--------------------->  ofp_header
           */
          "00 00 00 00 00 00 00 00"
          /* "00 00 00 00 00 00 00 00" too small ! */
          "00 00 00 00 00 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "set_async_handle error.");
}

void
test_ofp_set_async_handle_invalid_arguments(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error error;
  struct ofp_header header;
  struct channel *channel =
    channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf = pbuf_alloc(65535);

  ret = ofp_set_async_handle(NULL, pbuf, &header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL check failed.");

  ret = ofp_set_async_handle(channel, NULL, &header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL check failed.");

  ret = ofp_set_async_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL check failed.");

  ret = ofp_set_async_handle(channel, pbuf, &header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL check failed.");

  channel_free(channel);
  pbuf_free(pbuf);
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
