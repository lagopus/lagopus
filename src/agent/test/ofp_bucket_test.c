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
#include "../channel.h"
#include "../ofp_bucket.h"
#include "../ofp_action.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
}

#define BUCKET_LENGTH 0x30
#define ACTION_LENGTH 0x10

static int bucket_num = 0;
static int action_num = 0;

static void
create_data(struct bucket_list *bucket_list) {
  struct bucket *bucket;
  struct action *action;
  struct ofp_action_output *action_output;
  int i;
  int j;

  TAILQ_INIT(bucket_list);
  for (i = 0; i < bucket_num; i++) {
    bucket = bucket_alloc();
    TAILQ_INSERT_TAIL(bucket_list, bucket, entry);
    bucket->ofp.len = BUCKET_LENGTH;
    bucket->ofp.weight = (uint16_t) (0x02 + i);
    bucket->ofp.watch_port = (uint32_t) (0x03 +i);
    bucket->ofp.watch_group = (uint32_t) (0x4 + i);

    TAILQ_INIT(&bucket->action_list);
    for (j = 0; j < action_num; j++) {
      action = action_alloc(ACTION_LENGTH);
      TAILQ_INSERT_TAIL(&bucket->action_list, action, entry);
      action->ofpat.type = OFPAT_OUTPUT;
      action->ofpat.len = ACTION_LENGTH;
      action_output = (struct ofp_action_output *) &action->ofpat;
      action_output->port = (uint32_t) (0x05 + j);
      action_output->max_len = (uint16_t) (0x06 + j);
    }
  }
}

static lagopus_result_t
ofp_bucket_parse_wrap(struct channel *channel,
                      struct pbuf *pbuf,
                      struct ofp_header *xid_header,
                      struct ofp_error *error) {
  struct bucket_list bucket_list;
  struct bucket *bucket;
  struct action *action;
  struct ofp_action_output *action_output;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int ofp_bucket_num = 1;
  int ofp_action_num = 2;
  int i;
  int j;
  (void) channel;
  (void) xid_header;

  TAILQ_INIT(&bucket_list);

  ret = ofp_bucket_parse(pbuf, &bucket_list, error);

  if (ret == LAGOPUS_RESULT_OK) {
    i = 0;
    TAILQ_FOREACH(bucket, &bucket_list, entry) {
      TEST_ASSERT_EQUAL_MESSAGE(BUCKET_LENGTH, bucket->ofp.len,
                                "bucket length error.");
      TEST_ASSERT_EQUAL_MESSAGE(0x02 + i, bucket->ofp.weight,
                                "bucket weight error.");
      TEST_ASSERT_EQUAL_MESSAGE(0x03 +i, bucket->ofp.watch_port,
                                "bucket watch_port error.");
      TEST_ASSERT_EQUAL_MESSAGE(0x4 + i, bucket->ofp.watch_group,
                                "bucket watch_group error.");

      j = 0;
      TAILQ_FOREACH(action, &bucket->action_list, entry) {
        action_output = (struct ofp_action_output *) &action->ofpat;

        TEST_ASSERT_EQUAL_MESSAGE(OFPAT_OUTPUT, action_output->type,
                                  "action length error.");
        TEST_ASSERT_EQUAL_MESSAGE(ACTION_LENGTH, action_output->len,
                                  "action length error.");
        TEST_ASSERT_EQUAL_MESSAGE(0x05 + j, action_output->port,
                                  "action port error.");
        TEST_ASSERT_EQUAL_MESSAGE(0x06 + j, action_output->max_len,
                                  "action max_len error.");
        j++;
      }
      TEST_ASSERT_EQUAL_MESSAGE(ofp_action_num, j,
                                "action length error.");
      i++;
    }
    TEST_ASSERT_EQUAL_MESSAGE(ofp_bucket_num, i,
                              "bucket length error.");
    /* afetr. */
    ofp_bucket_list_free(&bucket_list);
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
test_ofp_bucket_parse(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(ofp_bucket_parse_wrap,
                           "00 30 00 02 00 00 00 03"
                           /* <-----------------...ofp_bucket
                            * <---> len : 48
                            *       <---> weight : 2
                            *             <---------> watch_port : 3
                            */
                           "00 00 00 04 00 00 00 00"
                           /* <---------> watch_group : 4
                            *             <---------> pad
                            */
                           "00 00 00 10 00 00 00 05"
                           /* <----------------... ofp_action_output
                            * <---> type : OFPAT_OUTPUT(0)
                            *       <---> len : 16
                            *             <---------> port : 5
                            */
                           "00 06 00 00 00 00 00 00"
                           /* <---> max_len : 6
                            *       <---------------> pad
                            */
                           "00 00 00 10 00 00 00 06"
                           /* <----------------... ofp_action_output
                            * <---> type : OFPAT_OUTPUT(0)
                            *       <---> len : 16
                            *             <---------> port : 6
                            */
                           "00 07 00 00 00 00 00 00");
  /* <---> max_len : 7
   *       <---------------> pad
   */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bucket_parse(normal) error.");
}

void
test_ofp_bucket_parse_error_invalid_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_bucket_parse_wrap,
                                        "00 30 00 02 00 00 00 03"
                                        "00 00 00 04 00 00 00 00"
                                        "00 00 00 10 00 00 00 05"
                                        "00 06 00 00 00 00 00 00"
                                        "00 00 00 10 00 00 00 06"
                                        "00 07 00 00 00 00 00",
                                        /* <---> max_len : 7
                                         *       <---------------> pad : short
                                         */
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_bucket_parse_error_invalid_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_bucket_parse_wrap,
                                        "00 30 00 02 00 00 00 03"
                                        "00 00 00 04 00 00 00", // ofp_bucket : short
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_bucket_parse_error_invalid_length2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_bucket_parse_wrap,
                                        "00 31 00 02 00 00 00 03"
                                        /* <-----------------...ofp_bucket
                                         * <---> len : 49 (long)
                                         *       <---> weight : 2
                                         *             <---------> watch_port : 3
                                         */
                                        "00 00 00 04 00 00 00 00"
                                        "00 00 00 10 00 00 00 05"
                                        "00 06 00 00 00 00 00 00"
                                        "00 00 00 10 00 00 00 06"
                                        "00 07 00 00 00 00 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_bucket_parse_error_invalid_length3(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_bucket_parse_wrap,
                                        "00 2f 00 02 00 00 00 03"
                                        /* <-----------------...ofp_bucket
                                         * <---> len : 47 (short)
                                         *       <---> weight : 2
                                         *             <---------> watch_port : 3
                                         */
                                        "00 00 00 04 00 00 00 00"
                                        "00 00 00 10 00 00 00 05"
                                        "00 06 00 00 00 00 00 00"
                                        "00 00 00 10 00 00 00 06"
                                        "00 07 00 00 00 00 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_bucket_parse_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bucket_list bucket_list;
  struct ofp_error error;
  struct pbuf *pbuf = pbuf_alloc(65535);

  ret = ofp_bucket_parse(NULL, &bucket_list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bucket_parse(NULL) error.");

  ret = ofp_bucket_parse(pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bucket_parse(NULL) error.");

  ret = ofp_bucket_parse(pbuf, &bucket_list, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bucket_parse(NULL) error.");

  pbuf_free(pbuf);
}

static lagopus_result_t
ofp_bucket_list_encode_wrap(struct channel *channel,
                            struct pbuf **pbuf,
                            struct ofp_header *xid_header) {
  struct bucket_list bucket_list;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t total_length;
  uint16_t length;
  (void) channel;
  (void) xid_header;

  create_data(&bucket_list);

  if (bucket_num == 0) {
    length = 0;
  } else {
    length = BUCKET_LENGTH * 2;
  }
  *pbuf = pbuf_alloc(length);
  (*pbuf)->plen = length;

  ret = ofp_bucket_list_encode(NULL, pbuf, &bucket_list,
                               &total_length);

  TEST_ASSERT_EQUAL_MESSAGE(length, total_length,
                            "bucket length error.");

  /* afetr. */
  ofp_bucket_list_free(&bucket_list);

  return ret;
}

void
test_ofp_bucket_list_encode(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  bucket_num = 2;
  action_num = 2;
  ret = check_packet_create(ofp_bucket_list_encode_wrap,
                            "00 30 00 02 00 00 00 03"
                            "00 00 00 04 00 00 00 00"
                            "00 00 00 10 00 00 00 05"
                            "00 06 00 00 00 00 00 00"
                            "00 00 00 10 00 00 00 06"
                            "00 07 00 00 00 00 00 00"
                            "00 30 00 03 00 00 00 04"
                            "00 00 00 05 00 00 00 00"
                            "00 00 00 10 00 00 00 05"
                            "00 06 00 00 00 00 00 00"
                            "00 00 00 10 00 00 00 06"
                            "00 07 00 00 00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bucket_list_encode(normal) error.");
}

void
test_ofp_bucket_list_encode_bucket_list_empty(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  bucket_num = 0;
  action_num = 0;
  ret = check_packet_create(ofp_bucket_list_encode_wrap,
                            "");

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bucket_list_encode(normal) error.");
}

void
test_ofp_bucket_list_encode_null(void) {
  struct bucket_list bucket_list;
  struct pbuf *pbuf = pbuf_alloc(65535);
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t total_length;

  ret = ofp_bucket_list_encode(NULL, NULL, &bucket_list,
                               &total_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bucket_list_encode(NULL) error.");

  ret = ofp_bucket_list_encode(NULL, &pbuf, NULL,
                               &total_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bucket_list_encode(NULL) error.");

  ret = ofp_bucket_list_encode(NULL, &pbuf, &bucket_list,
                               NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bucket_list_encode(NULL) error.");

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
