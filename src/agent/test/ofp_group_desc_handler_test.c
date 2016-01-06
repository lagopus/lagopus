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
#include "lagopus/pbuf.h"
#include "lagopus/ofp_handler.h"
#include "openflow.h"
#include "../ofp_bucket.h"
#include "../ofp_action.h"
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

static struct group_desc *
group_desc_alloc(size_t length) {
  struct group_desc *group_desc;
  group_desc = (struct group_desc *) calloc(1, length);
  return group_desc;
}

static int group_desc_num = 0;
static int bucket_num = 0;
static int action_num = 0;

static void
create_data(struct group_desc_list *group_desc_list) {
  struct group_desc *group_desc;
  struct bucket *bucket;
  struct action *action;
  struct ofp_action_output *action_output;
  uint16_t length = 0xe0;
  int i;
  int j;
  int k;

  TAILQ_INIT(group_desc_list);
  for (i = 0; i < group_desc_num; i++) {
    group_desc = group_desc_alloc(length);
    group_desc->ofp.length = length;
    group_desc->ofp.type = OFPGT_SELECT;
    group_desc->ofp.group_id = (uint32_t) (0x01 + i);

    TAILQ_INIT(&group_desc->bucket_list);
    for (j = 0; j < bucket_num; j++) {
      bucket = bucket_alloc();
      bucket->ofp.len = 0x30;
      bucket->ofp.weight = (uint16_t) (0x02 + j);
      bucket->ofp.watch_port = (uint32_t) (0x03 +j);
      bucket->ofp.watch_group = (uint32_t) (0x4 + j);

      TAILQ_INIT(&bucket->action_list);
      for (k = 0; k < action_num; k++) {
        action = action_alloc(0x10);
        action->ofpat.type = OFPAT_OUTPUT;
        action->ofpat.len = 0x10;
        action_output = (struct ofp_action_output *) &action->ofpat;
        action_output->port = (uint32_t) (0x05 + k);
        action_output->max_len = (uint16_t) (0x06 + k);
        TAILQ_INSERT_TAIL(&bucket->action_list, action, entry);
      }
      TAILQ_INSERT_TAIL(&group_desc->bucket_list, bucket, entry);
    }
    TAILQ_INSERT_TAIL(group_desc_list, group_desc, entry);
  }
}

lagopus_result_t
ofp_group_desc_reply_create_wrap(struct channel *channel,
                                 struct pbuf_list **pbuf_list,
                                 struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct group_desc_list group_desc_list;

  create_data(&group_desc_list);

  ret = ofp_group_desc_reply_create(channel, pbuf_list,
                                    &group_desc_list,
                                    xid_header);

  /* after. */
  group_desc_list_elem_free(&group_desc_list);

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
test_ofp_group_desc_reply_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[1] = {"04 13 00 e0 00 00 00 10"
                         "00 07 00 00 00 00 00 00"

                         "00 68 01 00 00 00 00 01"
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
                         "00 07 00 00 00 00 00 00"

                         "00 68 01 00 00 00 00 02"
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
                         "00 07 00 00 00 00 00 00"
                        };

  group_desc_num = 2;
  bucket_num = 2;
  action_num = 2;
  ret = check_pbuf_list_packet_create(ofp_group_desc_reply_create_wrap,
                                      data, 1);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_group_desc_reply_create(normal) error.");
}

void
test_ofp_group_desc_reply_create_bucket_list_empty(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[1] = {"04 13 00 20 00 00 00 10"
                         "00 07 00 00 00 00 00 00"

                         "00 08 01 00 00 00 00 01"

                         "00 08 01 00 00 00 00 02"
                        };

  group_desc_num = 2;
  bucket_num = 0;
  action_num = 0;
  ret = check_pbuf_list_packet_create(ofp_group_desc_reply_create_wrap,
                                      data, 1);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_group_desc_reply_create(normal) error.");
}

void
test_ofp_group_desc_reply_create_len_error(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[1] = {"04 13 00 e0 00 00 00 10"
                         "00 07 00 00 00 00 00 00"

                         "00 68 01 00 00 00 00 01"
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
                         "00 07 00 00 00 00 00 00"

                         "00 68 01 00 00 00 00 02"
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
                         "00 07 00 00 00 00 00 00"
                        };

  group_desc_num = 2;
  bucket_num = OFP_PACKET_MAX_SIZE;
  action_num = 2;
  ret = check_pbuf_list_packet_create(ofp_group_desc_reply_create_wrap,
                                      data, 1);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "ofp_group_desc_reply_create(len error) error.");
}

lagopus_result_t
ofp_group_desc_reply_create_null_wrap(struct channel *channel,
                                      struct pbuf_list **pbuf_list,
                                      struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct group_desc_list group_desc_list;

  group_desc_num = 2;
  bucket_num = 2;
  action_num = 2;
  create_data(&group_desc_list);

  ret = ofp_group_desc_reply_create(NULL, pbuf_list,
                                    &group_desc_list,
                                    xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_desc_reply_create error.");

  ret = ofp_group_desc_reply_create(channel, NULL,
                                    &group_desc_list,
                                    xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_desc_reply_create error.");

  ret = ofp_group_desc_reply_create(channel, pbuf_list,
                                    NULL,
                                    xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_desc_reply_create error.");

  ret = ofp_group_desc_reply_create(channel, pbuf_list,
                                    &group_desc_list,
                                    NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_desc_reply_create error.");

  /* after. */
  group_desc_list_elem_free(&group_desc_list);

  /* Not check return value. */
  return -9999;
}

void
test_ofp_group_desc_reply_create_null(void) {
  const char *data[1] = {"04 13 00 a0 00 00 00 10"};

  /* Not check return value. */
  (void ) check_pbuf_list_packet_create(ofp_group_desc_reply_create_null_wrap,
                                        data, 1);
}

void
test_ofp_group_desc_request_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = check_packet_parse(ofp_multipart_request_handle_wrap,
                           "04 12 00 10 00 00 00 10"
                           "00 07 00 00 00 00 00 00");
  /* The request body is empty */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_group_desc_request_handle(normal) error.");
}

void
test_ofp_group_desc_request_handle_error_invalid_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 10 00 00 00 10"
                                        "00 07 00 00 00 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
}

void
test_ofp_group_desc_request_handle_error_invalid_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 10 00 00 00 10"
                                        "00 07 00 00 00 00 00 00"
                                        "00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
}

void
test_ofp_group_desc_request_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header xid_header;
  struct ofp_error error;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);

  ret = ofp_group_desc_request_handle(NULL, pbuf, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_desc_request_handle error.");

  ret = ofp_group_desc_request_handle(channel, NULL, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_desc_request_handle error.");

  ret = ofp_group_desc_request_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_desc_request_handle error.");

  ret = ofp_group_desc_request_handle(channel, pbuf, &xid_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_desc_request_handle error.");

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
