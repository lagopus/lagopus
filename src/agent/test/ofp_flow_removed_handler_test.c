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

static struct eventq_data *
create_data(void) {
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
  uint64_t cookie = 0x12;
  uint16_t priority = 0x34;
  uint8_t reason = 0x02;
  uint8_t table_id = 0x30;
  uint32_t duration_sec = 0x56;
  uint32_t duration_nsec = 0x78;
  uint16_t idle_timeout = 0x90;
  uint16_t hard_timeout = 0x23;
  uint64_t packet_count = 0x45;
  uint64_t byte_count = 0x67;

  ed = (struct eventq_data *) calloc(1, sizeof(struct eventq_data));

  /* data. */
  ed->flow_removed.ofp_flow_removed.cookie = cookie;
  ed->flow_removed.ofp_flow_removed.priority = priority;
  ed->flow_removed.ofp_flow_removed.reason = reason;
  ed->flow_removed.ofp_flow_removed.table_id = table_id;
  ed->flow_removed.ofp_flow_removed.duration_sec = duration_sec;
  ed->flow_removed.ofp_flow_removed.duration_nsec = duration_nsec;
  ed->flow_removed.ofp_flow_removed.idle_timeout = idle_timeout;
  ed->flow_removed.ofp_flow_removed.hard_timeout = hard_timeout;
  ed->flow_removed.ofp_flow_removed.packet_count = packet_count;
  ed->flow_removed.ofp_flow_removed.byte_count = byte_count;


  TAILQ_INIT(&ed->flow_removed.match_list);
  for (i = 0; i < match_num; i++) {
    match = match_alloc(length[i]);
    match->oxm_class = class;
    match->oxm_field = field[i];
    match->oxm_length = length[i];
    memcpy(match->oxm_value, value[i], match->oxm_length);
    TAILQ_INSERT_TAIL(&ed->flow_removed.match_list, match, entry);
  }

  return ed;
}

static
lagopus_result_t
ofp_flow_removed_create_wrap(struct channel *channel,
                             struct pbuf **pbuf,
                             struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct eventq_data *ed;
  (void) channel;
  (void) xid_header;

  ed = create_data();
  /* Call func (2 match encode). */
  ret = ofp_flow_removed_create(&ed->flow_removed, pbuf);

  /* after. */
  ofp_flow_removed_free(ed);

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
test_ofp_flow_removed_create(void) {
  lagopus_result_t ret;
  /* version, length, xid and buffer_id is 0. */
  ret = check_packet_create(ofp_flow_removed_create_wrap,
                            "00 0b 00 00 00 00 00 00"
                            "00 00 00 00 00 00 00 12"
                            "00 34 02 30"
                            "00 00 00 56 00 00 00 78"
                            "00 90 00 23"
                            "00 00 00 00 00 00 00 45"
                            "00 00 00 00 00 00 00 67"
                            "00 01 00 16"
                            "80 00 00 04 00 00 00 10"
                            "80 00 08 06 00 0c 29 7a 90 b3 00 00");

  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "ofp_packet_in_create error.");
}

void
test_ofp_flow_removed_create_null(void) {
  lagopus_result_t ret;
  struct flow_removed flow_removed;
  struct pbuf *pbuf;

  ret = ofp_flow_removed_create(&flow_removed, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_INVALID_ARGS,
                            "ofp_flow_removed_create error.");

  ret = ofp_flow_removed_create(NULL, &pbuf);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_INVALID_ARGS,
                            "ofp_flow_removed_create error.");
}


static lagopus_result_t
ofp_flow_removed_handle_wrap(struct channel **channels,
                             struct ofp_header *xid_headers) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *ed;
  uint64_t dpid = 0xabc;
  (void) channels;
  (void) xid_headers;

  ed = create_data();

  /* Call func (2 match encode). */
  ret = ofp_flow_removed_handle(&ed->flow_removed, dpid);

  ofp_flow_removed_free(ed);

  return ret;
}

void
test_ofp_flow_removed_handle(void) {
  lagopus_result_t ret;
  ret = check_use_channels_write(ofp_flow_removed_handle_wrap,
                                 "00 0b 00 00 00 00 00 00"
                                 "00 00 00 00 00 00 00 12"
                                 "00 34 02 30"
                                 "00 00 00 56 00 00 00 78"
                                 "00 90 00 23"
                                 "00 00 00 00 00 00 00 45"
                                 "00 00 00 00 00 00 00 67"
                                 "00 01 00 16"
                                 "80 00 00 04 00 00 00 10"
                                 "80 00 08 06 00 0c 29 7a 90 b3");

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_flow_removed_handle error.");
}

void
test_ofp_flow_removed_handle_null(void) {
  lagopus_result_t ret;
  uint64_t dpid = 0x01;

  ret = ofp_flow_removed_handle(NULL, dpid);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_INVALID_ARGS,
                            "ofp_flow_removed_handle error.");
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
