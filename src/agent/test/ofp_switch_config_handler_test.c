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
#include "../channel.h"
#include "../ofp_switch_config_handler.h"
#include "handler_test_utils.h"
#include "lagopus/pbuf.h"
#include "lagopus/ofp_dp_apis.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
}

static struct ofp_switch_config s_switch_config;

static lagopus_result_t
s_ofp_set_config_request_handle_wrap(struct channel *channel,
                                     struct pbuf *pbuf,
                                     struct ofp_header *xid_header,
                                     struct ofp_error *error) {
  /* set xid_header. */
  if (ofp_header_decode_sneak_test(pbuf, xid_header) !=
      LAGOPUS_RESULT_OK) {
    TEST_FAIL_MESSAGE("handler_test_utils.c: cannot decode header\n");
  }
  return ofp_set_config_handle(channel, pbuf, xid_header, error);
}

static lagopus_result_t
s_ofp_get_config_request_handle_wrap(struct channel *channel,
                                     struct pbuf *pbuf,
                                     struct ofp_header *xid_header,
                                     struct ofp_error *error) {
  return ofp_get_config_request_handle(channel, pbuf,
                                       xid_header, error);
}

static lagopus_result_t
s_ofp_get_config_reply_create_wrap(struct channel *channel,
                                   struct pbuf **pbuf,
                                   struct ofp_header *xid_header) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error error;

  ofp_switch_config_set(channel_dpid_get(channel), &s_switch_config, &error);
  ret = ofp_get_config_reply_create(channel, pbuf, &s_switch_config,
                                    xid_header);

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
test_ofp_set_config_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          s_ofp_set_config_request_handle_wrap,
          "04 09 00 0c 00 00 00 10"
          "00 01 ff e4");
  /*
   *   <---> flags
   *         <---> miss_send_len
   */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_get_config_request_handle(normal) error.");
}
void
test_ofp_set_config_handle_invalid_header_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          s_ofp_set_config_request_handle_wrap,
          "04 09 00 08 00 00 00 10"
          /* sizeof(ofp_switch_config) is 12, but length is 8 */
          "00 01 ff e4",
          /*
           *   <---> flags
           *         <---> miss_send_len
           */
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid length.");
}

/*
 * - tests in set config
 * -- OFPSCFC_BAD_FLAGS -> testing
 * -- OFPSCFC_BAD_LEN -> testing
 * -- OFPSCFC_EPERM -> depending on DP
 */

void
test_ofp_set_config_handle_invalid_flags(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_SWITCH_CONFIG_FAILED, OFPSCFC_BAD_FLAGS);
  ret = check_packet_parse_expect_error(
          s_ofp_set_config_request_handle_wrap,
          "04 09 00 0c 00 00 00 10"
          "ff ff ff e4",
          /*
           *   <---> flags 0xffff is undefined
           *         <---> miss_send_len
           */
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid flags.");
}

void
test_ofp_set_config_handle_invalid_too_large_miss_send_len(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_SWITCH_CONFIG_FAILED, OFPSCFC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          s_ofp_set_config_request_handle_wrap,
          "04 09 00 0c 00 00 00 10"
          "00 01 ff e6",
          /*
           *   <---> flags 0xffff is undefined
           *         <---> miss_send_len > OFPCML_MAX
           */
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid miss send len.");
}

void
test_ofp_set_config_handle_invalid_miss_send_len_no_buf(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          s_ofp_set_config_request_handle_wrap,
          "04 09 00 0c 00 00 00 10"
          "00 01 ff ff");
  /*
   *         <---> miss_send_len > OFPCML_NO_BUFFER
   */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "miss send len - no buf error.");
}

void
test_ofp_get_config_handle(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(s_ofp_get_config_request_handle_wrap,
                           "04 07 00 08 00 00 00 10");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_get_config_request_handle(normal) error.");
}

void
test_ofp_get_config_handle_invalid_size_too_short(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid header */
  ret = check_packet_parse_expect_error(
          s_ofp_get_config_request_handle_wrap,
          "04 07 00 07 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid packet error.");
}

void
test_ofp_get_config_handle_invalid_size_too_long(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* over size */
  ret = check_packet_parse_expect_error(
          s_ofp_get_config_request_handle_wrap,
          "04 07 00 10 00 00 00 10 00 80",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "over size error.");
}

void
test_ofp_get_config_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header ofp_header;
  struct ofp_error error;

  ret = ofp_get_config_request_handle(NULL, pbuf, &ofp_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_get_config_request_handle(channel, NULL, &ofp_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_get_config_request_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (ofp_header)");

  ret = ofp_get_config_request_handle(channel, pbuf, &ofp_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (ofp_error)");

  channel_free(channel);
  pbuf_free(pbuf);
}

void
test_ofp_get_config_reply_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  s_switch_config.flags = 0x0000;
  s_switch_config.miss_send_len = 0x0000;
  ret = check_packet_create(s_ofp_get_config_reply_create_wrap,
                            "04 08 00 0c 00 00 00 10 00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_get_config_reply_create(normal) error.");

  s_switch_config.flags = 0xffff;
  s_switch_config.miss_send_len = 0xffff;
  ret = check_packet_create(s_ofp_get_config_reply_create_wrap,
                            "04 08 00 0c 00 00 00 10 ff ff ff ff");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_get_config_reply_create(normal) error.");
}

void
test_ofp_get_config_reply_create_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header ofp_header;

  ret = ofp_get_config_reply_create(NULL, &pbuf, &s_switch_config, &ofp_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_get_config_reply_create(channel, NULL, &s_switch_config,
                                    &ofp_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_get_config_reply_create(channel, &pbuf, NULL, &ofp_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (switch_config)");

  ret = ofp_get_config_reply_create(channel, &pbuf, &s_switch_config, NULL);
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
