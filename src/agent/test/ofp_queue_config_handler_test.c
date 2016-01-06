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
#include "../ofp_queue_config_handler.h"
#include "handler_test_utils.h"
#include "lagopus/pbuf.h"
#include "../channel_mgr.h"

static uint32_t s_port;
static struct packet_queue_list s_packet_queue_list;

void
setUp(void) {
  TAILQ_INIT(&s_packet_queue_list);
}

void
tearDown(void) {
}

static lagopus_result_t
s_ofp_queue_get_config_request_handle_wrap(struct channel *channel,
    struct pbuf *pbuf,
    struct ofp_header *xid_header,
    struct ofp_error *error) {
  return ofp_queue_get_config_request_handle(channel, pbuf,
         xid_header, error);
}

static lagopus_result_t
s_ofp_queue_get_config_reply_create_wrap(struct channel *channel,
    struct pbuf **pbuf,
    struct ofp_header *xid_header) {
  return ofp_queue_get_config_reply_create(channel, pbuf, s_port,
         &s_packet_queue_list,
         xid_header);
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
test_ofp_queue_get_config_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          s_ofp_queue_get_config_request_handle_wrap,
          "04 16 00 10 00 00 00 10 ff ff ff ff 00 00 00 00");
  /* <---------------------> ofp_header
   * <> version
   *    <> type
   *       <---> length
   *             <---------> xid
   *                         <---------> port 0xffffffff -> OFPP_ANY
   *                                     <----------> padding
   */
  TEST_ASSERT_EQUAL_MESSAGE(
    LAGOPUS_RESULT_OK, ret,
    "ofp_queue_get_config_request_handle(normal) error.");
}

void
test_ofp_queue_get_config_handle_invalid_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* no body. */
  ret = check_packet_parse_expect_error(
          s_ofp_queue_get_config_request_handle_wrap,
          "04 16 00 08 00 00 00 10",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "no-body error");
}

void
test_ofp_queue_get_config_handle_invalid_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body (no pad) */
  ret = check_packet_parse_expect_error(
          s_ofp_queue_get_config_request_handle_wrap,
          "04 16 00 0c 00 00 00 10 ff ff ff ff",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_queue_get_config_handle_invalid_length2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body (over size) */
  ret = check_packet_parse_expect_error(
          s_ofp_queue_get_config_request_handle_wrap,
          "04 16 00 12 00 00 00 10 ff ff ff ff 00 00 00 00 "
          "ff ff",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_queue_get_config_handle_invalid_too_large_port(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_QUEUE_OP_FAILED, OFPQOFC_BAD_PORT);
  ret = check_packet_parse_expect_error(
          s_ofp_queue_get_config_request_handle_wrap,
          "04 16 00 10 00 00 00 10 ff ff ff 01 00 00 00 00",
          &expected_error);
  /* <---------------------> ofp_header
   * <> version
   *    <> type
   *       <---> length
   *             <---------> xid
   *                         <---------> port 0xffffff0x -> OFPP_MAX + 1
   *                                     <----------> padding
   */
  TEST_ASSERT_EQUAL_MESSAGE(
    LAGOPUS_RESULT_OFP_ERROR, ret,
    "too large port should case error.");
}

void
test_ofp_queue_get_config_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header ofp_header;
  struct ofp_error error;

  ret = ofp_queue_get_config_request_handle(NULL, pbuf, &ofp_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_queue_get_config_request_handle(channel, NULL, &ofp_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_queue_get_config_request_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (ofp_header)");

  ret = ofp_queue_get_config_request_handle(channel, pbuf, &ofp_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (ofp_error)");
  channel_free(channel);
  pbuf_free(pbuf);
}

void
test_ofp_queue_get_config_reply_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  /* OFPP_ANY */
  s_port = 0xffffffff;
  ret = check_packet_create(
          s_ofp_queue_get_config_reply_create_wrap,
          "04 17 00 10 00 00 00 10 ff ff ff ff 00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "create ofpp_any error.");

  /* OFPP_FLOOD */
  s_port = 0xfffffffb;
  ret = check_packet_create(
          s_ofp_queue_get_config_reply_create_wrap,
          "04 17 00 10 00 00 00 10 ff ff ff fb 00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "create ofpp_flood error.");

  /* port 0 */
  s_port = 0x00000000;
  ret = check_packet_create(
          s_ofp_queue_get_config_reply_create_wrap,
          "04 17 00 10 00 00 00 10 00 00 00 00 00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "create port 0 error.");
}

void
test_ofp_queue_get_config_reply_create_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf = pbuf_alloc(65535);
  static struct packet_queue_list packet_queue_list;
  struct ofp_header ofp_header;

  ret = ofp_queue_get_config_reply_create(NULL, &pbuf, 0,
                                          &packet_queue_list,
                                          &ofp_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_queue_get_config_reply_create(channel, NULL, 0,
                                          &packet_queue_list,
                                          &ofp_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_queue_get_config_reply_create(channel, &pbuf, 0,
                                          NULL, &ofp_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (packet_queue_list)");

  ret = ofp_queue_get_config_reply_create(channel, &pbuf, 0,
                                          &packet_queue_list,
                                          NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (ofp_header)");

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
