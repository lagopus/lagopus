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

static
lagopus_result_t
ofp_port_status_create_wrap(struct channel *channel,
                            struct pbuf **pbuf,
                            struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct port_status port_status;
  (void) channel;
  (void) xid_header;

  memset(&port_status, 0xFF, sizeof(struct port_status));
  ret = ofp_port_status_create(&port_status, pbuf);

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
test_ofp_port_status_create(void) {
  lagopus_result_t ret;
  /* version and xid is 0. */
  ret = check_packet_create(ofp_port_status_create_wrap,
                            "00 0C 00 50 00 00 00 00 "
                            "FF FF FF FF FF FF FF FF "
                            "FF FF FF FF FF FF FF FF "
                            "FF FF FF FF FF FF FF FF "
                            "FF FF FF FF FF FF FF FF "
                            "FF FF FF FF FF FF FF FF "
                            "FF FF FF FF FF FF FF FF "
                            "FF FF FF FF FF FF FF FF "
                            "FF FF FF FF FF FF FF FF "
                            "FF FF FF FF FF FF FF FF "
                           );

  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "ofp_port_stats_create error.");
}

void
test_ofp_port_status_create_null(void) {
  lagopus_result_t ret;
  struct port_status port_status;
  struct pbuf *pbuf;

  ret = ofp_port_status_create(&port_status, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_INVALID_ARGS,
                            "ofp_port_status_create error.");

  ret = ofp_port_status_create(NULL, &pbuf);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_INVALID_ARGS,
                            "ofp_port_status_create error.");
}

static lagopus_result_t
ofp_port_status_handle_wrap(struct channel **channels,
                            struct ofp_header *xid_headers) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid = 0xabc;
  struct port_status port_status;
  (void) channels;
  (void) xid_headers;

  memset(&port_status, 0xFF, sizeof(struct port_status));

  /* Call func. */
  ret = ofp_port_status_handle(&port_status, dpid);

  return ret;
}

void
test_ofp_port_status_handle(void) {
  lagopus_result_t ret;
  ret = check_use_channels_write(ofp_port_status_handle_wrap,
                                 "00 0C 00 50 00 00 00 00 "
                                 "FF FF FF FF FF FF FF FF "
                                 "FF FF FF FF FF FF FF FF "
                                 "FF FF FF FF FF FF FF FF "
                                 "FF FF FF FF FF FF FF FF "
                                 "FF FF FF FF FF FF FF FF "
                                 "FF FF FF FF FF FF FF FF "
                                 "FF FF FF FF FF FF FF FF "
                                 "FF FF FF FF FF FF FF FF "
                                 "FF FF FF FF FF FF FF FF ");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_port_status_handle error.");
}

void
test_ofp_port_status_handle_null(void) {
  lagopus_result_t ret;
  uint64_t dpid = 0x01;

  ret = ofp_port_status_handle(NULL, dpid);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_INVALID_ARGS,
                            "ofp_port_status_handle error.");

}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
