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
#include "lagopus/pbuf.h"
#include "lagopus/meter.h"
#include "handler_test_utils.h"
#include "../ofp_band_stats.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
}

#define BAND_STATS_LENGTH 0x10

static int band_stats_num = 0;

static void
create_data(struct meter_band_stats_list *band_stats_list) {
  struct meter_band_stats *meter_band_stats;
  int i;

  TAILQ_INIT(band_stats_list);
  for (i = 0; i < band_stats_num; i++) {
    meter_band_stats = meter_band_stats_alloc();
    TAILQ_INSERT_TAIL(band_stats_list, meter_band_stats, entry);

    meter_band_stats->ofp.packet_band_count = (uint64_t) (0x01 + i);
    meter_band_stats->ofp.byte_band_count = (uint64_t) (0x02 + i);
  }
}

static lagopus_result_t
ofp_meter_band_stats_list_encode_wrap(struct channel *channel,
                                      struct pbuf **pbuf,
                                      struct ofp_header *xid_header) {
  struct meter_band_stats_list band_stats_list;
  lagopus_result_t ret;
  uint16_t total_length;
  uint16_t length;
  (void) channel;
  (void) xid_header;

  create_data(&band_stats_list);
  if (band_stats_num == 0) {
    length = 0;
  } else {
    length = BAND_STATS_LENGTH * 2;
  }
  *pbuf = pbuf_alloc(length);
  (*pbuf)->plen = length;

  ret = ofp_meter_band_stats_list_encode(NULL, pbuf, &band_stats_list,
                                         &total_length);

  TEST_ASSERT_EQUAL_MESSAGE(length, total_length,
                            "band_stats length error.");

  /* afetr. */
  meter_band_stats_list_elem_free(&band_stats_list);

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
test_ofp_meter_band_stats_list_encode(void) {
  lagopus_result_t ret;

  band_stats_num = 2;
  ret = check_packet_create(ofp_meter_band_stats_list_encode_wrap,
                            "00 00 00 00 00 00 00 01"
                            "00 00 00 00 00 00 00 02"
                            "00 00 00 00 00 00 00 02"
                            "00 00 00 00 00 00 00 03");

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_meter_band_stats_list_encode(normal) error.");
}

void
test_ofp_meter_band_stats_list_encode_band_stats_list_empty(void) {
  lagopus_result_t ret;

  band_stats_num = 0;
  ret = check_packet_create(ofp_meter_band_stats_list_encode_wrap,
                            "");

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_meter_band_stats_list_encode(band_stats_list empty) error.");
}

void
test_ofp_meter_band_stats_list_encode_null(void) {
  struct meter_band_stats_list band_stats_list;
  struct pbuf *pbuf;
  lagopus_result_t ret;
  uint16_t total_length;

  ret = ofp_meter_band_stats_list_encode(NULL, NULL, &band_stats_list,
                                         &total_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_meter_band_stats_list_encode(NULL) error.");

  ret = ofp_meter_band_stats_list_encode(NULL, &pbuf, NULL,
                                         &total_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_meter_band_stats_list_encode(NULL) error.");

  ret = ofp_meter_band_stats_list_encode(NULL, &pbuf, &band_stats_list,
                                         NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_meter_band_stats_list_encode(NULL) error.");
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
