/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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
#include "event.h"
#include "lagopus/pbuf.h"
#include "lagopus/ofp_handler.h"

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

static void
create_data(struct ofp_group_features *group_features) {
  uint32_t types = 0x01;
  uint32_t capabilities = 0x02;
  uint32_t max_groups[] = {0x03, 0x04, 0x05, 0x06};
  uint32_t actions[] = {0x07, 0x08, 0x09, 0x00};

  group_features->types = types;
  group_features->capabilities = capabilities;
  memcpy(group_features->max_groups, max_groups, sizeof(max_groups));
  memcpy(group_features->actions, actions, sizeof(actions));
}

lagopus_result_t
ofp_group_features_reply_create_wrap(struct channel *channel,
                                     struct pbuf_list **pbuf_list,
                                     struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct ofp_group_features group_features;

  create_data(&group_features);

  ret = ofp_group_features_reply_create(channel, pbuf_list,
                                        &group_features,
                                        xid_header);

  return ret;
}

void
test_ofp_group_features_reply_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[1] = {"04 13 00 38 00 00 00 10"
                         "00 08 00 00 00 00 00 00"
                         "00 00 00 01 00 00 00 02"
                         "00 00 00 03 00 00 00 04"
                         "00 00 00 05 00 00 00 06"
                         "00 00 00 07 00 00 00 08"
                         "00 00 00 09 00 00 00 00"
                        };

  ret = check_pbuf_list_packet_create(ofp_group_features_reply_create_wrap,
                                      data, 1);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_group_features_reply_create(normal) error.");
}

lagopus_result_t
ofp_group_features_reply_create_null_wrap(struct channel *channel,
    struct pbuf_list **pbuf_list,
    struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct ofp_group_features group_features;

  ret = ofp_group_features_reply_create(NULL, pbuf_list,
                                        &group_features,
                                        xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_features_reply_create error.");

  ret = ofp_group_features_reply_create(channel, NULL,
                                        &group_features,
                                        xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_features_reply_create error.");

  ret = ofp_group_features_reply_create(channel, pbuf_list,
                                        NULL,
                                        xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_features_reply_create error.");

  ret = ofp_group_features_reply_create(channel, pbuf_list,
                                        &group_features,
                                        NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_features_reply_create error.");

  /* Not check return value. */
  return -9999;
}

void
test_ofp_group_features_reply_create_null(void) {
  const char *data[1] = {"04 13 00 a0 00 00 00 10"};

  /* Not check return value. */
  (void ) check_pbuf_list_packet_create(
    ofp_group_features_reply_create_null_wrap,
    data, 1);
}

void
test_ofp_group_features_request_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = check_packet_parse(ofp_multipart_request_handle_wrap,
                           "04 12 00 10 00 00 00 10"
                           "00 08 00 00 00 00 00 00");
  /* The request body is empty. */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_group_features_request_handle(normal) error.");
}

void
test_ofp_group_features_request_handle_error_invalid_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 10 00 00 00 10"
                                        "00 08 00 00 00 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
}

void
test_ofp_group_features_request_handle_error_invalid_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 10 00 00 00 10"
                                        "00 08 00 00 00 00 00 00"
                                        "00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
}

void
test_ofp_group_features_request_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header xid_header;
  struct ofp_error error;
  struct event_manager *em = event_manager_alloc();
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000",
                            em, 0x01);

  ret = ofp_group_features_request_handle(NULL, pbuf, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_features_request_handle error.");

  ret = ofp_group_features_request_handle(channel, NULL, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_features_request_handle error.");

  ret = ofp_group_features_request_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_features_request_handle error.");

  ret = ofp_group_features_request_handle(channel, pbuf, &xid_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_group_features_request_handle error.");

  /* after. */
  channel_free(channel);
  event_manager_free(em);
  pbuf_free(pbuf);
}
