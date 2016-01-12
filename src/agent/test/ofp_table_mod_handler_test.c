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
#include "../ofp_table_mod_handler.h"
#include "handler_test_utils.h"
#include "lagopus/pbuf.h"
#include "../channel_mgr.h"


void
setUp(void) {
}

void
tearDown(void) {
}

/* TODO: remove after error handling modification*/
lagopus_result_t
ofp_table_mod_handle_wrap(struct channel *channel, struct pbuf *pbuf,
                          struct ofp_header *xid_header,
                          struct ofp_error *error) {
  return ofp_table_mod_handle(channel, pbuf,
                              xid_header, error);
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
test_ofp_table_mod_handle_wrap(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          ofp_table_mod_handle_wrap,
          "04 11 00 10 00 00 00 10 01 00 00 00 00 00 00 03");
  /*
   * <---------------------> ofp_header
   * <> version
   *    <> type
   *       <---> length = 16 bytes
   *             <---------> xid
   *                         <> table_id
   *                            <------> padding
   *                                     <----------> config
   */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_table_mod_handle(normal) error.");
}

void
test_ofp_table_mod_handle_wrap_invalid_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* header only */
  ret = check_packet_parse_expect_error(
          ofp_table_mod_handle_wrap,
          "04 11 00 08 00 00 00 10",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "no body error.");
}

void
test_ofp_table_mod_handle_wrap_invalid_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* no pad, config */
  ret = check_packet_parse_expect_error(
          ofp_table_mod_handle_wrap,
          "04 11 00 09 00 00 00 10 01",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid body error.");
}

void
test_ofp_table_mod_handle_wrap_invalid_length2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* no config */
  ret = check_packet_parse_expect_error(
          ofp_table_mod_handle_wrap,
          "04 11 00 0c 00 00 00 10 01 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid body error.");
}

void
test_ofp_table_mod_handle_wrap_invalid_length3(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* over size */
  ret = check_packet_parse_expect_error(
          ofp_table_mod_handle_wrap,
          "04 11 00 18 00 00 00 10 01 00 00 00 00 00 00 03"
          "ff ff ff ff ff ff ff ff",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid body error.");
}

void
test_ofp_table_mod_handle_bad_config(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_TABLE_MOD_FAILED, OFPTMFC_BAD_CONFIG);

  ret = check_packet_parse_expect_error(
          ofp_table_mod_handle_wrap,
          "04 11 00 10 00 00 00 10 01 00 00 00 00 00 00 04",
          /*
           * <---------------------> ofp_header
           * <> version
           *    <> type
           *       <---> length = 16 bytes
           *             <---------> xid
           *                         <> table_id
           *                            <------> padding
           *                                     <----------> config
           *                                                   : 4 (bad config)
           */
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_table_mod_handle(bad config) error.");
}

void
test_ofp_table_mod_handle_wrap_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header xid_header;
  struct ofp_error error;

  ret = ofp_table_mod_handle(NULL, pbuf, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_table_mod_handle(channel, NULL, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_table_mod_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (xid_header)");

  ret = ofp_table_mod_handle(channel, pbuf, &xid_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (ofp_error)");
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
