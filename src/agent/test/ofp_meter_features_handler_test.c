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

static void
create_data(struct ofp_meter_features *meter_features) {
  uint32_t max_meter = 0x01;
  uint32_t band_types = 0x02;
  uint32_t capabilities = 0x05;
  uint8_t max_bands = 0x06;
  uint8_t max_color =0x07;

  memset(meter_features, 0, sizeof(struct ofp_meter_features));
  meter_features->max_meter = max_meter;
  meter_features->band_types = band_types;
  meter_features->capabilities = capabilities;
  meter_features->max_bands = max_bands;
  meter_features->max_color = max_color;
}

lagopus_result_t
ofp_meter_features_reply_create_wrap(struct channel *channel,
                                     struct pbuf_list **pbuf_list,
                                     struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct ofp_meter_features meter_features;

  create_data(&meter_features);

  ret = ofp_meter_features_reply_create(channel, pbuf_list,
                                        &meter_features,
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
test_ofp_meter_features_reply_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[1] = {"04 13 00 20 00 00 00 10"
                         "00 0b 00 00 00 00 00 00"
                         "00 00 00 01 00 00 00 02"
                         /* <---------> max_meter : 1
                          *             <---------> band_types :
                          *                          OFPMBT_DSCP_REMARK(2)
                          */
                         "00 00 00 05 06 07 00 00"
                        };
  /* <---------> capabilities :
   *              OFPMF_KBPS(1) & OFPMF_BURST(4)
   *             <> max_bands : 6
   *               <> max_color :7
   *                   <---> pad
   */

  ret = check_pbuf_list_packet_create(ofp_meter_features_reply_create_wrap,
                                      data, 1);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_meter_features_reply_create(normal) error.");
}

void
test_ofp_meter_features_reply_create_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf_list *pbuf_list = NULL;
  struct ofp_header xid_header;
  struct ofp_meter_features meter_features;

  ret = ofp_meter_features_reply_create(NULL, &pbuf_list,
                                        &meter_features,
                                        &xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_meter_features_reply_create error.");

  ret = ofp_meter_features_reply_create(channel, NULL,
                                        &meter_features,
                                        &xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_meter_features_reply_create error.");

  ret = ofp_meter_features_reply_create(channel, &pbuf_list,
                                        NULL,
                                        &xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_meter_features_reply_create error.");

  ret = ofp_meter_features_reply_create(channel, &pbuf_list,
                                        &meter_features,
                                        NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_meter_features_reply_create error.");

  /* after. */
  channel_free(channel);
}

void
test_ofp_meter_features_request_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = check_packet_parse(ofp_multipart_request_handle_wrap,
                           "04 12 00 10 00 00 00 10"
                           "00 0b 00 00 00 00 00 00");
  /* The request body is empty. */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_meter_features_request_handle(normal) error.");
}

void
test_ofp_meter_features_request_handle_error_invalid_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 10 00 00 00 10"
                                        "00 0b 00 00 00 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
}

void
test_ofp_meter_features_request_handle_error_invalid_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 10 00 00 00 10"
                                        "00 0b 00 00 00 00 00 00"
                                        "00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
}

void
test_ofp_meter_features_request_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header xid_header;
  struct ofp_error error;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);

  ret = ofp_meter_features_request_handle(NULL, pbuf, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_meter_features_request_handle error.");

  ret = ofp_meter_features_request_handle(channel, NULL, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_meter_features_request_handle error.");

  ret = ofp_meter_features_request_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_meter_features_request_handle error.");

  ret = ofp_meter_features_request_handle(channel, pbuf, &xid_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_meter_features_request_handle error.");

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
