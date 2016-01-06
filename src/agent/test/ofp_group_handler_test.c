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
#include "unity.h"
#include "../ofp_apis.h"
#include "handler_test_utils.h"
#include "openflow.h"
#include "lagopus/pbuf.h"
#include "lagopus/ofp_handler.h"
#include "../ofp_bucket_counter.h"
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

static struct group_stats *
group_stats_alloc(size_t length) {
  struct group_stats *group_stats;
  group_stats = (struct group_stats *) calloc(1, length);
  return group_stats;
}

static int group_stats_num = 0;
static int bucket_counter_num = 0;

static void
create_data(struct group_stats_list *group_stats_list) {
  struct group_stats *group_stats;
  struct bucket_counter *bucket_counter;
  size_t length;
  int i;
  int j;

  TAILQ_INIT(group_stats_list);
  for (i = 0; i < group_stats_num; i++) {
    length = sizeof(struct group_stats) +
             (sizeof(struct ofp_bucket_counter) * 2);

    group_stats = group_stats_alloc(length);

    group_stats->ofp.length = (uint16_t) length;
    group_stats->ofp.group_id = (uint32_t) (0x01 + i);
    group_stats->ofp.ref_count = (uint32_t) (0x02 + i);
    group_stats->ofp.packet_count = (uint64_t) (0x03 + i);
    group_stats->ofp.byte_count = (uint64_t) (0x04 + i);
    group_stats->ofp.duration_sec = (uint32_t) (0x05 + i);
    group_stats->ofp.duration_nsec = (uint32_t) (0x06 + i);

    TAILQ_INIT(&group_stats->bucket_counter_list);
    for (j = 0; j < bucket_counter_num; j++) {
      bucket_counter = bucket_counter_alloc();
      bucket_counter->ofp.packet_count = (uint64_t) (0x07 + i + j);
      bucket_counter->ofp.byte_count = (uint64_t) (0x09 + i + j);

      TAILQ_INSERT_TAIL(&group_stats->bucket_counter_list,
                        bucket_counter, entry);
    }

    TAILQ_INSERT_TAIL(group_stats_list, group_stats, entry);
  }
}

lagopus_result_t
ofp_group_stats_reply_create_wrap(struct channel *channel,
                                  struct pbuf_list **pbuf_list,
                                  struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct group_stats_list group_stats_list;

  create_data(&group_stats_list);

  ret = ofp_group_stats_reply_create(channel, pbuf_list,
                                     &group_stats_list,
                                     xid_header);

  /* after. */
  group_stats_list_elem_free(&group_stats_list);
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
test_ofp_group_stats_reply_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[1] = {"04 13 00 a0 00 00 00 10"
                         "00 06 00 00 00 00 00 00"
                         "00 48 00 00 00 00 00 01"
                         "00 00 00 02 00 00 00 00"
                         "00 00 00 00 00 00 00 03"
                         "00 00 00 00 00 00 00 04"
                         "00 00 00 05 00 00 00 06"
                         "00 00 00 00 00 00 00 07"
                         "00 00 00 00 00 00 00 09"
                         "00 00 00 00 00 00 00 08"
                         "00 00 00 00 00 00 00 0a"
                         "00 48 00 00 00 00 00 02"
                         "00 00 00 03 00 00 00 00"
                         "00 00 00 00 00 00 00 04"
                         "00 00 00 00 00 00 00 05"
                         "00 00 00 06 00 00 00 07"
                         "00 00 00 00 00 00 00 08"
                         "00 00 00 00 00 00 00 0a"
                         "00 00 00 00 00 00 00 09"
                         "00 00 00 00 00 00 00 0b"
                        };

  group_stats_num = 2;
  bucket_counter_num = 2;
  ret = check_pbuf_list_packet_create(ofp_group_stats_reply_create_wrap,
                                      data, 1);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_group_stats_reply_create(normal) error.");
}

void
test_ofp_group_stats_reply_create_bucket_counter_list_empty(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[1] = {"04 13 00 60 00 00 00 10"
                         "00 06 00 00 00 00 00 00"

                         "00 28 00 00 00 00 00 01"
                         "00 00 00 02 00 00 00 00"
                         "00 00 00 00 00 00 00 03"
                         "00 00 00 00 00 00 00 04"
                         "00 00 00 05 00 00 00 06"

                         "00 28 00 00 00 00 00 02"
                         "00 00 00 03 00 00 00 00"
                         "00 00 00 00 00 00 00 04"
                         "00 00 00 00 00 00 00 05"
                         "00 00 00 06 00 00 00 07"
                        };

  group_stats_num = 2;
  bucket_counter_num = 0;
  ret = check_pbuf_list_packet_create(ofp_group_stats_reply_create_wrap,
                                      data, 1);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_group_stats_reply_create(normal) error.");
}

lagopus_result_t
ofp_group_stats_reply_create_len_error_wrap(struct channel *channel,
    struct pbuf_list **pbuf_list,
    struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct group_stats_list group_stats_list;

  create_data(&group_stats_list);

  ret = ofp_group_stats_reply_create(channel, pbuf_list,
                                     &group_stats_list,
                                     xid_header);

  /* after. */
  group_stats_list_elem_free(&group_stats_list);
  return ret;
}

void
test_ofp_group_stats_reply_create_len_error(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[1] = {"04 13 00 a0 00 00 00 10"
                         "00 06 00 00 00 00 00 00"
                         "00 48 00 00 00 00 00 01"
                         "00 00 00 02 00 00 00 00"
                         "00 00 00 00 00 00 00 03"
                         "00 00 00 00 00 00 00 04"
                         "00 00 00 05 00 00 00 06"
                         "00 00 00 00 00 00 00 07"
                         "00 00 00 00 00 00 00 09"
                         "00 00 00 00 00 00 00 08"
                         "00 00 00 00 00 00 00 0a"
                         "00 48 00 00 00 00 00 02"
                         "00 00 00 03 00 00 00 00"
                         "00 00 00 00 00 00 00 04"
                         "00 00 00 00 00 00 00 05"
                         "00 00 00 06 00 00 00 07"
                         "00 00 00 00 00 00 00 08"
                         "00 00 00 00 00 00 00 0a"
                         "00 00 00 00 00 00 00 09"
                         "00 00 00 00 00 00 00 0b"
                        };

  group_stats_num = 2;
  bucket_counter_num = OFP_PACKET_MAX_SIZE;
  ret = check_pbuf_list_packet_create(ofp_group_stats_reply_create_wrap,
                                      data, 1);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "ofp_group_stats_reply_create(len error) error.");
}

lagopus_result_t
ofp_group_stats_reply_create_null_wrap(struct channel *channel,
                                       struct pbuf_list **pbuf_list,
                                       struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct group_stats_list group_stats_list;

  group_stats_num = 2;
  bucket_counter_num = 2;
  create_data(&group_stats_list);

  ret = ofp_group_stats_reply_create(NULL, pbuf_list,
                                     &group_stats_list,
                                     xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_stats_reply_create error.");

  ret = ofp_group_stats_reply_create(channel, NULL,
                                     &group_stats_list,
                                     xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_stats_reply_create error.");

  ret = ofp_group_stats_reply_create(channel, pbuf_list,
                                     NULL,
                                     xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_stats_reply_create error.");

  ret = ofp_group_stats_reply_create(channel, pbuf_list,
                                     &group_stats_list,
                                     NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_stats_reply_create error.");

  /* after. */
  group_stats_list_elem_free(&group_stats_list);

  /* Not check return value. */
  return -9999;
}

void
test_ofp_group_stats_reply_create_null(void) {
  const char *data[1] = {"04 13 00 a0 00 00 00 10"};

  /* Not check return value. */
  (void )check_pbuf_list_packet_create(ofp_group_stats_reply_create_null_wrap,
                                       data, 1);
}

void
test_ofp_group_stats_request_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(ofp_multipart_request_handle_wrap,
                           "04 12 00 18 00 00 00 10"
                           "00 06 00 00 00 00 00 00"
                           "00 00 00 01 00 00 00 00");
  /* <---------> group_id
   *             <---------> pad
   */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_group_stats_request_handle(normal) error.");
}

void
test_ofp_group_stats_request_handle_error_invalid_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 18 00 00 00 10"
                                        "00 06 00 00 00 00 00 00"
                                        "00 00 00 01 00 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
}

void
test_ofp_group_stats_request_handle_error_invalid_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 18 00 00 00 10"
                                        "00 06 00 00 00 00 00 00"
                                        "00 00 00 01 00 00 00 00"
                                        "00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
}

void
test_ofp_group_stats_request_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header xid_header;
  struct ofp_error error;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  ret = ofp_group_stats_request_handle(NULL, pbuf, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_stats_request_handle error.");

  ret = ofp_group_stats_request_handle(channel, NULL, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_stats_request_handle error.");

  ret = ofp_group_stats_request_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_stats_request_handle error.");

  ret = ofp_group_stats_request_handle(channel, pbuf, &xid_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_stats_request_handle error.");

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
