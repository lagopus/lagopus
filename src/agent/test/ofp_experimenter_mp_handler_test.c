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
#include "handler_test_utils.h"
#include "../ofp_apis.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
}

static void
create_data(struct ofp_experimenter_multipart_header *exper_req) {
  exper_req->experimenter = 0x01;
  exper_req->exp_type = 0x02;
}

static lagopus_result_t
ofp_experimenter_mp_reply_create_wrap(struct channel *channel,
                                      struct pbuf_list **pbuf_list,
                                      struct ofp_header *xid_header) {
  struct ofp_experimenter_multipart_header exper_req;

  create_data(&exper_req);

  return ofp_experimenter_mp_reply_create(channel, pbuf_list, xid_header,
                                          &exper_req);
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
test_ofp_experimenter_mp_reply_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[1] = {"04 13 00 18 00 00 00 10"
                         "ff ff 00 00 00 00 00 00"
                         "00 00 00 01 00 00 00 02"
                        };
  ret = check_pbuf_list_packet_create(ofp_experimenter_mp_reply_create_wrap,
                                      data, 1);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_experimenter_mp_reply_create(normal) error.");
}

lagopus_result_t
ofp_experimenter_mp_reply_create_null_wrap(struct channel *channel,
    struct pbuf_list **pbuf_list,
    struct ofp_header *xid_header) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_experimenter_multipart_header exper_req;

  ret = ofp_experimenter_mp_reply_create(NULL, pbuf_list, xid_header,
                                         &exper_req);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_experimenter_mp_reply_create error.");

  ret = ofp_experimenter_mp_reply_create(channel, NULL, xid_header, &exper_req);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_experimenter_mp_reply_create error.");

  ret = ofp_experimenter_mp_reply_create(channel, pbuf_list, NULL, &exper_req);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_experimenter_mp_reply_create error.");

  ret = ofp_experimenter_mp_reply_create(channel, pbuf_list, xid_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_experimenter_mp_reply_create error.");

  /* Uncheck return value. */
  return -9999;
}

void
test_ofp_experimenter_mp_reply_create_null(void) {
  const char *data[1] = {"04 13 00 18 00 00 00 10"
                         "ff ff 00 00 00 00 00 00"
                         "00 00 00 01 00 00 00 02"
                        };
  (void) check_pbuf_list_packet_create(
    ofp_experimenter_mp_reply_create_null_wrap,
    data, 1);
}

static lagopus_result_t
ofp_experimenter_mp_request_handle_wrap(struct channel *channel,
                                        struct pbuf *pbuf,
                                        struct ofp_header *xid_header,
                                        struct ofp_error *error) {
  (void) error;
  return ofp_experimenter_mp_request_handle(channel, pbuf, xid_header);
}

void
test_ofp_experimenter_mp_request_handle(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(ofp_experimenter_mp_request_handle_wrap,
                           "00 00 00 01 00 00 00 02");

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_experimenter_mp_request_handle(normal) error.");
}

void
test_ofp_experimenter_mp_request_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header xid_header;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);

  ret = ofp_experimenter_mp_request_handle(NULL, pbuf, &xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_experimenter_mp_request_handle error.");

  ret = ofp_experimenter_mp_request_handle(channel, NULL, &xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_experimenter_mp_request_handle error.");

  ret = ofp_experimenter_mp_request_handle(channel, pbuf, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_experimenter_mp_request_handle error.");

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
