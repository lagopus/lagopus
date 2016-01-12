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
#include "lagopus_apis.h"
#include "lagopus_session.h"
#include "lagopus/pbuf.h"
#include "lagopus/flowdb.h"
#include "openflow13.h"
#include "handler_test_utils.h"
#include "../ofp_bucket_counter.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
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
test_bucket_counter_alloc_list_free(void) {
  struct bucket_counter_list bucket_counter_list;
  struct bucket_counter *bucket_counter;
  uint64_t packet_counts[] = {0x01, 0x03};
  uint64_t byte_counts[] = {0x02, 0x04};
  int test_num = 2;
  int i;

  TAILQ_INIT(&bucket_counter_list);

  for (i = 0; i < test_num; i++) {
    bucket_counter = bucket_counter_alloc();
    if (bucket_counter != NULL) {
      bucket_counter->ofp.packet_count = packet_counts[i];
      bucket_counter->ofp.byte_count = byte_counts[i];
      TAILQ_INSERT_TAIL(&bucket_counter_list, bucket_counter, entry);
    } else {
      TEST_FAIL_MESSAGE("bucket_counter is NULL.");
    }
  }
  bucket_counter_list_elem_free(&bucket_counter_list);
}

#define BUCKET_CNT_LENGTH 0x10

static int bucket_cnt_num = 0;

static void
create_data(struct bucket_counter_list *bucket_counter_list) {
  struct bucket_counter *bucket_counter;
  int i;

  TAILQ_INIT(bucket_counter_list);

  /* data. */
  for (i = 0; i < bucket_cnt_num; i++) {
    bucket_counter = bucket_counter_alloc();
    if (bucket_counter != NULL) {
      TAILQ_INSERT_TAIL(bucket_counter_list, bucket_counter, entry);

      bucket_counter->ofp.packet_count = (uint64_t) (0x01 + i);
      bucket_counter->ofp.byte_count = (uint64_t) (0x02 + i);
    } else {
      TEST_FAIL_MESSAGE("bucket_counter is NULL.");
    }
  }
}

static
lagopus_result_t
ofp_bucket_counter_list_encode_wrap(struct channel *channel,
                                    struct pbuf **pbuf,
                                    struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct bucket_counter_list bucket_counter_list;
  uint16_t total_length = 0;
  uint16_t length;
  (void) channel;
  (void) xid_header;

  create_data(&bucket_counter_list);
  if (bucket_cnt_num == 0) {
    length = 0;
  } else {
    length = BUCKET_CNT_LENGTH * 2;
  }
  *pbuf = pbuf_alloc(length);
  (*pbuf)->plen = length;

  ret = ofp_bucket_counter_list_encode(NULL, pbuf,
                                       &bucket_counter_list,
                                       &total_length);

  TEST_ASSERT_EQUAL_MESSAGE(length, total_length,
                            "total_length error.");

  /* after. */
  bucket_counter_list_elem_free(&bucket_counter_list);

  return ret;
}

void
test_ofp_bucket_counter_list_encode(void) {
  lagopus_result_t ret;

  bucket_cnt_num = 2;
  ret = check_packet(ofp_bucket_counter_list_encode_wrap,
                     "00 00 00 00 00 00 00 01"
                     "00 00 00 00 00 00 00 02"
                     "00 00 00 00 00 00 00 02"
                     "00 00 00 00 00 00 00 03");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bucket_counter_list_encode(normal) error.");
}

void
test_ofp_bucket_counter_list_encode_bucket_counter_list_empty(void) {
  lagopus_result_t ret;

  bucket_cnt_num = 0;
  ret = check_packet(ofp_bucket_counter_list_encode_wrap,
                     "");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bucket_counter_list_encode(bucket_counter_list empty) error.");
}

void
test_ofp_bucket_counter_list_encode_null(void) {
  lagopus_result_t ret;
  struct bucket_counter_list bucket_counter_list;
  struct pbuf *pbuf;
  uint16_t total_length;

  ret = ofp_bucket_counter_list_encode(NULL, NULL,
                                       &bucket_counter_list,
                                       &total_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bucket_counter_list_encode(NULL) error.");

  ret = ofp_bucket_counter_list_encode(NULL, &pbuf,
                                       NULL,
                                       &total_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bucket_counter_list_encode(NULL) error.");

  ret = ofp_bucket_counter_list_encode(NULL, &pbuf,
                                       &bucket_counter_list,
                                       NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bucket_counter_list_encode(NULL) error.");
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
