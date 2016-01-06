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
#include "../ofp_apis.h"
#include "handler_test_utils.h"
#include "lagopus/pbuf.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
}

/* TODO remove */
static lagopus_result_t
ofp_multipart_request_handle_wrap(struct channel *channel,
                                  struct pbuf *pbuf,
                                  struct ofp_header *xid_header,
                                  struct ofp_error *error) {
  return ofp_multipart_request_handle(channel, pbuf, xid_header, error);
}

static struct port_stats *
s_port_stats_alloc(void) {
  struct port_stats *port_stats;

  port_stats = calloc(1, sizeof(struct port_stats));

  return port_stats;
}

static struct port_stats_list s_port_stats_list;

static lagopus_result_t
s_ofp_port_reply_create_wrap(struct channel *channel,
                             struct pbuf_list **pbuf_list,
                             struct ofp_header *xid_header) {
  return ofp_port_stats_reply_create(channel,
                                     pbuf_list,
                                     &s_port_stats_list,
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
test_ofp_port_stats_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(ofp_multipart_request_handle_wrap,
                           "04 12 00 40 00 00 00 10 "
                           "00 04 00 00 00 00 00 00 "
                           // body
                           "00 00 00 00 "             /* uint32_t port_no;     */
                           "00 00 00 00 "             /* uint8_t  pad[4];      */
                          );
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_port_request_handle(normal) error.");
}

void
test_ofp_port_stats_handle_error_invalid_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  /* invalid body (no flags, pad) */
  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 08 00 00 00 10 "
                                        "00 04",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR,
                            ret,
                            "invalid-body error");
}

void
test_ofp_port_stats_handle_error_invalid_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  /* invalid body (no pad) */
  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 08 00 00 00 10 "
                                        "00 04 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR,
                            ret,
                            "invalid-body error");
}

void
test_ofp_port_stats_handle_error_invalid_length2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  /* invalid body (over size) */
  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 12 00 00 00 10 "
                                        "00 04 00 00 00 00 00 00 "
                                        "00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR,
                            ret,
                            "invalid-body error");
}

void
test_ofp_port_stats_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header xid_header;
  struct ofp_error error;

  ret = ofp_port_stats_request_handle(NULL, pbuf, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            ret,
                            "NULL-check error. (channel)");

  ret = ofp_port_stats_request_handle(channel, NULL, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_port_stats_request_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            ret,
                            "NULL-check error. (xid_header)");

  ret = ofp_port_stats_request_handle(channel, pbuf, &xid_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            ret,
                            "NULL-check error. (ofp_error)");
  channel_free(channel);
  pbuf_free(pbuf);
}

void
test_ofp_port_stats_reply_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct port_stats *port_stats = NULL;
  const char *require[1] = {
    "04 13 00 80 00 00 00 10 "
    "00 04 00 00 00 00 00 00 "
    // body
    "00 00 00 01 "             //  uint32_t port_no;
    "00 00 00 00 "             //  uint8_t pad[4];
    "00 00 00 00 00 00 00 02 " //  uint64_t rx_packets;
    "00 00 00 00 00 00 00 03 " //  uint64_t tx_packets;
    "00 00 00 00 00 00 00 04 " //  uint64_t rx_bytes;
    "00 00 00 00 00 00 00 05 " //  uint64_t tx_bytes;
    "00 00 00 00 00 00 00 06 " //  uint64_t rx_dropped;
    "00 00 00 00 00 00 00 07 " //  uint64_t tx_dropped;
    "00 00 00 00 00 00 00 08 " //  uint64_t rx_errors;
    "00 00 00 00 00 00 00 09 " //  uint64_t tx_errors;
    "00 00 00 00 00 00 00 0a " //  uint64_t rx_frame_err;
    "00 00 00 00 00 00 00 0b " //  uint64_t rx_over_err;
    "00 00 00 00 00 00 00 0c " //  uint64_t rx_crc_err;
    "00 00 00 00 00 00 00 0d " //  uint64_t collisions;
    "00 00 00 0e"              // uint32_t duration_sec;
    "00 00 00 0f"              // uint32_t duration_nsec;
  };

  TAILQ_INIT(&s_port_stats_list);
  if ((port_stats = s_port_stats_alloc()) != NULL) {
    port_stats->ofp.port_no       = 0x01;
    port_stats->ofp.rx_packets    = 0x02;
    port_stats->ofp.tx_packets    = 0x03;
    port_stats->ofp.rx_bytes      = 0x04;
    port_stats->ofp.tx_bytes      = 0x05;
    port_stats->ofp.rx_dropped    = 0x06;
    port_stats->ofp.tx_dropped    = 0x07;
    port_stats->ofp.rx_errors     = 0x08;
    port_stats->ofp.tx_errors     = 0x09;
    port_stats->ofp.rx_frame_err  = 0x0a;
    port_stats->ofp.rx_over_err   = 0x0b;
    port_stats->ofp.rx_crc_err    = 0x0c;
    port_stats->ofp.collisions    = 0x0d;
    port_stats->ofp.duration_sec  = 0x0e;
    port_stats->ofp.duration_nsec = 0x0f;
    TAILQ_INSERT_TAIL(&s_port_stats_list, port_stats, entry);
  } else {
    TEST_FAIL_MESSAGE("allocation error.");
  }

  /* port 0 */
  ret = check_pbuf_list_packet_create(s_ofp_port_reply_create_wrap, require, 1);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "create port 0 error.");

  /* free */
  while ((port_stats = TAILQ_FIRST(&s_port_stats_list)) != NULL) {
    TAILQ_REMOVE(&s_port_stats_list, port_stats, entry);
    free(port_stats);
  }
}

void
test_ofp_port_stats_reply_create_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf_list *pbuf_list = NULL;
  struct port_stats_list port_stats_list;
  struct ofp_header xid_header;

  ret = ofp_port_stats_reply_create(NULL,
                                    &pbuf_list,
                                    &port_stats_list,
                                    &xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_port_stats_reply_create(channel,
                                    NULL,
                                    &port_stats_list,
                                    &xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_port_stats_reply_create(channel,
                                    &pbuf_list,
                                    NULL,
                                    &xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            ret,
                            "NULL-check error. (port_stats_list)");

  ret = ofp_port_stats_reply_create(channel,
                                    &pbuf_list,
                                    &port_stats_list,
                                    NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            ret,
                            "NULL-check error. (xid-header)");
  channel_free(channel);
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
