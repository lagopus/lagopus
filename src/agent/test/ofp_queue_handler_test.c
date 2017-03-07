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

#include <sys/queue.h>
#include "unity.h"
#include "../ofp_apis.h"
#include "handler_test_utils.h"
#include "lagopus/pbuf.h"
#include "lagopus/ofp_handler.h"
#include "../ofp_queue_handler.h"
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

static struct queue_stats *
queue_stats_alloc(void) {
  struct queue_stats *queue_stats;
  queue_stats = (struct queue_stats *)
                calloc(1, sizeof(struct queue_stats));
  return queue_stats;
}

static struct queue_stats_list qstats_list;

static void
create_data(int test_num) {
  struct queue_stats *queue_stats;
  int i;

  TAILQ_INIT(&qstats_list);
  for (i = 0; i < test_num; i++) {
    queue_stats = queue_stats_alloc();

    queue_stats->ofp.port_no = (uint32_t) (0x01 + i);
    queue_stats->ofp.queue_id = (uint32_t) (0x02 + i);
    queue_stats->ofp.tx_bytes = (uint64_t) (0x03 + i);
    queue_stats->ofp.tx_packets = (uint64_t) (0x04 + i);
    queue_stats->ofp.tx_errors = (uint64_t) (0x05 + i);
    queue_stats->ofp.duration_sec = (uint32_t) (0x06 + i);
    queue_stats->ofp.duration_nsec = (uint32_t) (0x07 + i);

    TAILQ_INSERT_TAIL(&qstats_list, queue_stats, entry);
  }
}

lagopus_result_t
ofp_queue_stats_reply_create_wrap_with_self_data(
  struct channel *channel,
  struct pbuf_list **pbuf_list,
  struct ofp_header *xid_header) {
  lagopus_result_t ret;

  ret = ofp_queue_stats_reply_create(channel, pbuf_list,
                                     &qstats_list,
                                     xid_header);

  return ret;
}

lagopus_result_t
ofp_queue_stats_reply_create_wrap(struct channel *channel,
                                  struct pbuf_list **pbuf_list,
                                  struct ofp_header *xid_header) {
  lagopus_result_t ret;

  create_data(1);

  ret = ofp_queue_stats_reply_create(channel, pbuf_list,
                                     &qstats_list,
                                     xid_header);

  /* free. */
  queue_stats_list_elem_free(&qstats_list);

  return ret;
}

void
test_prologue(void) {
  lagopus_result_t r;
  const char *argv0 =
    ((IS_VALID_STRING(lagopus_get_command_name()) == true) ?
     lagopus_get_command_name() : "callout_test");
  const char *const argv[] = {
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
test_ofp_queue_stats_reply_create_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[1] = {"04 13 00 38 00 00 00 10"
                         "00 05 00 00 00 00 00 00"
                         "00 00 00 01 00 00 00 02"
                         "00 00 00 00 00 00 00 03"
                         "00 00 00 00 00 00 00 04"
                         "00 00 00 00 00 00 00 05"
                         "00 00 00 06 00 00 00 07"
                        };

  ret = check_pbuf_list_packet_create(ofp_queue_stats_reply_create_wrap,
                                      data, 1);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_queue_stats_reply_create(normal) error.");
}

void
test_ofp_queue_stats_reply_create_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct queue_stats *queue_stats;
  const char *header_data[2] = {
    "04 13 ff d8 00 00 00 10 00 05 00 01 00 00 00 00 ",
    "04 13 00 38 00 00 00 10 00 05 00 00 00 00 00 00 "
  };
  const char *body_data[2] = {
    "00 00 00 01 00 00 00 02"
    "00 00 00 00 00 00 00 03"
    "00 00 00 00 00 00 00 04"
    "00 00 00 00 00 00 00 05"
    "00 00 00 06 00 00 00 07",
    "00 00 00 01 00 00 00 02"
    "00 00 00 00 00 00 00 03"
    "00 00 00 00 00 00 00 04"
    "00 00 00 00 00 00 00 05"
    "00 00 00 06 00 00 00 07"
  };
  size_t nums[2] = {1637, 1};
  int i;

  /* data */
  TAILQ_INIT(&qstats_list);
  for (i = 0; i < 1638; i++) {
    queue_stats = queue_stats_alloc();
    if (queue_stats != NULL) {
      queue_stats->ofp.port_no = 0x01;
      queue_stats->ofp.queue_id = 0x02;
      queue_stats->ofp.tx_bytes = 0x03;
      queue_stats->ofp.tx_packets = 0x04;
      queue_stats->ofp.tx_errors = 0x05;
      queue_stats->ofp.duration_sec = 0x06;
      queue_stats->ofp.duration_nsec = 0x07;
    } else {
      TEST_FAIL_MESSAGE("allocation error.");
    }
    TAILQ_INSERT_TAIL(&qstats_list, queue_stats, entry);
  }

  ret = check_pbuf_list_across_packet_create(
          ofp_queue_stats_reply_create_wrap_with_self_data,
          header_data, body_data, nums, 2);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "check_pbuf_list error.");

  /* free */
  queue_stats_list_elem_free(&qstats_list);
}

lagopus_result_t
ofp_queue_stats_reply_create_null_wrap(struct channel *channel,
                                       struct pbuf_list **pbuf_list,
                                       struct ofp_header *xid_header) {
  lagopus_result_t ret;

  ret = ofp_queue_stats_reply_create(NULL, pbuf_list,
                                     &qstats_list,
                                     xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_queue_stats_request_create error.");

  ret = ofp_queue_stats_reply_create(channel, NULL,
                                     &qstats_list,
                                     xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_queue_stats_request_create error.");

  ret = ofp_queue_stats_reply_create(channel, pbuf_list,
                                     NULL, xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_queue_stats_request_create error.");

  ret = ofp_queue_stats_reply_create(channel, pbuf_list,
                                     &qstats_list,
                                     NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_queue_stats_request_create error.");

  /* Uncheck return value. */
  return -9999;
}

void
test_ofp_queue_stats_reply_create_null(void) {
  const char *data[1] = {"04 13 00 38 00 00 00 10"
                         "00 05 00 00 00 00 00 00"
                         "00 00 00 01 00 00 00 02"
                         "00 00 00 00 00 00 00 03"
                         "00 00 00 00 00 00 00 04"
                         "00 00 00 00 00 00 00 05"
                         "00 00 00 06 00 00 00 07"
                        };

  /* Uncheck return value. */
  (void) check_pbuf_list_packet_create(ofp_queue_stats_reply_create_null_wrap,
                                       data, 1);
}

void
test_ofp_queue_stats_request_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = check_packet_parse(ofp_multipart_request_handle_wrap,
                           "04 12 00 18 00 00 00 10"
                           "00 05 00 00 00 00 00 00"
                           "00 00 00 01 00 00 00 02");
  /* <---------> port_no : 1
   *             <---------> queue_id : 2
   */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_queue_stats_request_handle(normal) error.");
}

void
test_ofp_queue_stats_request_handle_error_invalid_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 18 00 00 00 10"
                                        "00 05 00 00 00 00 00 00"
                                        "00 00 00 01 00 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
}

void
test_ofp_queue_stats_request_handle_error_invalid_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 18 00 00 00 10"
                                        "00 05 00 00 00 00 00 00"
                                        "00 00 00 01 00 00 00 02"
                                        "00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
}

void
test_ofp_queue_stats_request_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header xid_header;
  struct ofp_error error;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);

  ret = ofp_queue_stats_request_handle(NULL, pbuf, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_queue_stats_request_handle error.");

  ret = ofp_queue_stats_request_handle(channel, NULL, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_queue_stats_request_handle error.");

  ret = ofp_queue_stats_request_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_queue_stats_request_handle error.");

  ret = ofp_queue_stats_request_handle(channel, pbuf, &xid_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_queue_stats_request_handle error.");

  /* after. */
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
