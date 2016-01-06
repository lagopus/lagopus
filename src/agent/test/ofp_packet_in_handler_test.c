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
#include "../ofp_match.h"
#include "../ofp_apis.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
}

static uint32_t test_buffer_id = 0x00;
static uint16_t test_data_length = 0x00;

static struct eventq_data *
create_data(bool has_long_data) {
  int i;
  struct eventq_data *ed;
  struct match *match;
  int match_num = 2;
  uint16_t class = OFPXMC_OPENFLOW_BASIC;
  uint8_t field[] = {OFPXMT_OFB_IN_PORT << 1,
                     OFPXMT_OFB_ETH_SRC << 1
                    };
  uint8_t length[] = {0x04, 0x06};
  uint8_t value[2][6] = {{0x00, 0x00, 0x00, 0x10},
    {0x00, 0x0c, 0x29, 0x7a, 0x90, 0xb3}
  };
  uint32_t buffer_id = test_buffer_id;
  uint8_t reason = 0x02;
  uint8_t table_id = 0x30;
  uint64_t cookie = 0x40;
  uint8_t data[] = {0x12, 0x34, 0x56, 0x78, 0x90};
  uint16_t data_length = test_data_length;
  uint16_t miss_send_len = 0x80;
  ed = (struct eventq_data *) calloc(1, sizeof(struct eventq_data));

  /* data. */
  ed->packet_in.miss_send_len = miss_send_len;
  ed->packet_in.ofp_packet_in.buffer_id = buffer_id;
  ed->packet_in.ofp_packet_in.reason = reason;
  ed->packet_in.ofp_packet_in.table_id = table_id;
  ed->packet_in.ofp_packet_in.cookie = cookie;

  TAILQ_INIT(&ed->packet_in.match_list);
  for (i = 0; i < match_num; i++) {
    match = match_alloc(length[i]);
    match->oxm_class = class;
    match->oxm_field = field[i];
    match->oxm_length = length[i];
    memcpy(match->oxm_value, value[i], match->oxm_length);
    TAILQ_INSERT_TAIL(&ed->packet_in.match_list, match, entry);
  }

  ed->packet_in.data = pbuf_alloc(data_length);
  if (has_long_data == true) {
    memset(ed->packet_in.data->data, 0, data_length);
  } else {
    memcpy(ed->packet_in.data->data, data, data_length);
  }
  ed->packet_in.data->putp += data_length;
  ed->packet_in.data->plen = data_length;

  return ed;
}

static
lagopus_result_t
ofp_packet_in_create_wrap(struct channel *channel,
                          struct pbuf **pbuf,
                          struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct eventq_data *ed;
  (void) channel;
  (void) xid_header;

  ed = create_data(false);
  /* Call func (2 match encode). */
  ret = ofp_packet_in_create(&ed->packet_in, pbuf);

  /* after. */
  ofp_packet_in_free(ed);

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
test_ofp_packet_in_create(void) {
  lagopus_result_t ret;

  test_buffer_id = 0x12;
  test_data_length = 0x05;
  /* version, length, xid and buffer_id is 0. */
  ret = check_packet_create(ofp_packet_in_create_wrap,
                            "00 0a 00 00 00 00 00 00"
                            "00 00 00 12 00 05 02 30"
                            "00 00 00 00 00 00 00 40"
                            "00 01 00 16"
                            "80 00 00 04 00 00 00 10"
                            "80 00 08 06 00 0c 29 7a 90 b3 00 00"
                            "00 00"
                            "12 34 56 78 90");

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_packet_in_create error.");
}

static
lagopus_result_t
ofp_packet_in_create_long_data_wrap(struct channel *channel,
                                    struct pbuf **pbuf,
                                    struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct eventq_data *ed;
  (void) channel;
  (void) xid_header;

  ed = create_data(true);
  /* Call func (2 match encode). */
  ret = ofp_packet_in_create(&ed->packet_in, pbuf);

  /* after. */
  ofp_packet_in_free(ed);

  return ret;
}

void
test_ofp_packet_in_create_long_data_buffer_id(void) {
  lagopus_result_t ret;

  test_buffer_id = 0x12;
  test_data_length = 0x82;
  /* version, length, xid and buffer_id is 0. */
  ret = check_packet_create(ofp_packet_in_create_long_data_wrap,
                            "00 0a 00 00 00 00 00 00"
                            "00 00 00 12 00 82 02 30"
                            "00 00 00 00 00 00 00 40"
                            "00 01 00 16"
                            "80 00 00 04 00 00 00 10"
                            "80 00 08 06 00 0c 29 7a 90 b3 00 00"
                            "00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00");

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_packet_in_create error.");
}

void
test_ofp_packet_in_create_long_data_no_buffer_id(void) {
  lagopus_result_t ret;

  test_buffer_id = OFP_NO_BUFFER;
  test_data_length = 0x82;
  /* version, length, xid and buffer_id is 0. */
  ret = check_packet_create(ofp_packet_in_create_long_data_wrap,
                            "00 0a 00 00 00 00 00 00"
                            "ff ff ff ff 00 82 02 30"
                            "00 00 00 00 00 00 00 40"
                            "00 01 00 16"
                            "80 00 00 04 00 00 00 10"
                            "80 00 08 06 00 0c 29 7a 90 b3 00 00"
                            "00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 00"
                            "00 00");

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_packet_in_create error.");
}

void
test_ofp_packet_in_create_null(void) {
  lagopus_result_t ret;
  struct packet_in packet_in;
  struct pbuf *pbuf;

  ret = ofp_packet_in_create(&packet_in, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_packet_in_create error.");

  ret = ofp_packet_in_create(NULL, &pbuf);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_packet_in_create error.");

  packet_in.data = NULL;
  ret = ofp_packet_in_create(&packet_in, &pbuf);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_packet_in_create error.");
}

static lagopus_result_t
ofp_packet_in_handle_wrap(struct channel **channels,
                          struct ofp_header *xid_headers) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *ed;
  uint64_t dpid = 0xabc;
  (void) channels;
  (void) xid_headers;

  /* data */
  ed = create_data(false);

  /* Call func (2 match encode). */
  ret = ofp_packet_in_handle(&ed->packet_in, dpid);

  /* after. */
  ofp_packet_in_free(ed);

  return ret;
}

void
test_ofp_packet_in_handle(void) {
  lagopus_result_t ret;

  test_buffer_id = 0x12;
  test_data_length = 0x05;
  ret = check_use_channels_write(ofp_packet_in_handle_wrap,
                                 "00 0a 00 00 00 00 00 00"
                                 "00 00 00 12 00 05 02 30"
                                 "00 01 00 16"
                                 "80 00 00 04 00 00 00 10"
                                 "80 00 08 06 00 0c 29 7a 90 b3"
                                 "12 34 56 78 90");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_packet_in_handle error.");
}

void
test_ofp_packet_in_handle_null(void) {
  lagopus_result_t ret;
  struct packet_in packet_in;
  uint64_t dpid = 0x01;

  ret = ofp_packet_in_handle(NULL, dpid);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_packet_in_handle error.");

  packet_in.data = NULL;
  ret = ofp_packet_in_handle(&packet_in, dpid);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_packet_in_handle error.");
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
