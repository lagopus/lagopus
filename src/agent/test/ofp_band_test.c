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
#include "lagopus/ofp_dp_apis.h"
#include "handler_test_utils.h"
#include "../channel_mgr.h"
#include "../ofp_band.h"

void
setUp(void) {
}

void
tearDown(void) {
}

#define BAND_LENGTH 0x10

static int band_num = 0;

static void
create_data(struct meter_band_list *band_list) {
  struct ofp_meter_band_drop band;
  struct meter_band *meter_band;
  int i;

  TAILQ_INIT(band_list);
  for (i = 0; i < band_num; i++) {
    band.type = OFPMBT_DROP;
    band.len = BAND_LENGTH;
    band.rate = (uint32_t) (0x01 + i);
    band.burst_size = (uint32_t) (0x02 + i);

    meter_band = meter_band_alloc((struct ofp_meter_band_header *)&band);
    TAILQ_INSERT_TAIL(band_list, meter_band, entry);
  }
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

static lagopus_result_t
test_ofp_meter_band_drop_parse_wrap(struct channel *channel,
                                    struct pbuf *pbuf,
                                    struct ofp_header *xid_header,
                                    struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct meter_band_list band_list;
  struct meter_band *band;
  uint16_t len = 0x10;
  uint32_t rate = 0x0a;
  uint32_t burst_size = 0x01;
  int meter_band_num = 1;
  int i;
  (void) channel;
  (void) xid_header;

  /* normal */
  TAILQ_INIT(&band_list);

  ret = ofp_meter_band_drop_parse(pbuf, &band_list, error);

  if (ret == LAGOPUS_RESULT_OK) {
    i = 0;
    TAILQ_FOREACH(band, &band_list, entry) {
      TEST_ASSERT_EQUAL_MESSAGE(OFPMBT_DROP, band->type,
                                "band type error.");
      TEST_ASSERT_EQUAL_MESSAGE(len, band->len,
                                "band len error.");
      TEST_ASSERT_EQUAL_MESSAGE(rate, band->rate,
                                "band rate error.");
      TEST_ASSERT_EQUAL_MESSAGE(burst_size, band->burst_size,
                                "band burst_size error.");
      i++;
    }

    TEST_ASSERT_EQUAL_MESSAGE(meter_band_num, i, "bund num error.");
  }

  ofp_meter_band_list_elem_free(&band_list);

  return ret;
}

void
test_ofp_meter_band_drop_parse(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(test_ofp_meter_band_drop_parse_wrap,
                           "00 01"
                           "00 10"
                           "00 00 00 0a"
                           "00 00 00 01"
                           "00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_meter_band_drop_parse(normal) error.");
}

void
test_ofp_meter_band_drop_parse_bad_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(test_ofp_meter_band_drop_parse_wrap,
                                        "00 01"
                                        "00 10"
                                        "00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_meter_band_drop_parse(bat length) error.");
}

static lagopus_result_t
test_ofp_meter_band_dscp_remark_parse_wrap(struct channel *channel,
    struct pbuf *pbuf,
    struct ofp_header *xid_header,
    struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct meter_band_list band_list;
  struct meter_band *band;
  uint16_t len = 0x10;
  uint32_t rate = 0x0a;
  uint32_t burst_size = 0x01;
  uint8_t prec_level = 0x20;
  int meter_band_num = 1;
  int i;
  (void) channel;
  (void) xid_header;

  /* normal */
  TAILQ_INIT(&band_list);

  ret = ofp_meter_band_dscp_remark_parse(pbuf, &band_list, error);

  if (ret == LAGOPUS_RESULT_OK) {
    i = 0;
    TAILQ_FOREACH(band, &band_list, entry) {
      TEST_ASSERT_EQUAL_MESSAGE(OFPMBT_DSCP_REMARK, band->type,
                                "band type error.");
      TEST_ASSERT_EQUAL_MESSAGE(len, band->len,
                                "band len error.");
      TEST_ASSERT_EQUAL_MESSAGE(rate, band->rate,
                                "band rate error.");
      TEST_ASSERT_EQUAL_MESSAGE(burst_size, band->burst_size,
                                "band burst_size error.");
      TEST_ASSERT_EQUAL_MESSAGE(prec_level, band->prec_level,
                                "band prec_level error.");
      i++;
    }

    TEST_ASSERT_EQUAL_MESSAGE(meter_band_num, i, "bund num error.");
  }

  ofp_meter_band_list_elem_free(&band_list);

  return ret;
}

void
test_ofp_meter_band_dscp_remark_parse(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(test_ofp_meter_band_dscp_remark_parse_wrap,
                           "00 02"
                           "00 10"
                           "00 00 00 0a"
                           "00 00 00 01"
                           "20"
                           "00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_meter_band_dscp_remark_parse(normal) error.");
}

void
test_ofp_meter_band_dscp_remark_parse_bad_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          test_ofp_meter_band_dscp_remark_parse_wrap,
          "00 02"
          "00 10"
          "00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_meter_band_dscp_remark_parse(bat length) error.");
}


static lagopus_result_t
test_ofp_meter_band_experimenter_parse_wrap(struct channel *channel,
    struct pbuf *pbuf,
    struct ofp_header *xid_header,
    struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct meter_band_list band_list;
  struct meter_band *band;
  uint16_t len = 0x10;
  uint32_t rate = 0x0a;
  uint32_t burst_size = 0x01;
  uint32_t experimenter = 0x30;
  int meter_band_num = 1;
  int i;
  (void) channel;
  (void) xid_header;

  /* normal */
  TAILQ_INIT(&band_list);

  ret = ofp_meter_band_experimenter_parse(pbuf, &band_list, error);

  if (ret == LAGOPUS_RESULT_OK) {
    i = 0;
    TAILQ_FOREACH(band, &band_list, entry) {
      TEST_ASSERT_EQUAL_MESSAGE(OFPMBT_EXPERIMENTER, band->type,
                                "band type error.");
      TEST_ASSERT_EQUAL_MESSAGE(len, band->len,
                                "band len error.");
      TEST_ASSERT_EQUAL_MESSAGE(rate, band->rate,
                                "band rate error.");
      TEST_ASSERT_EQUAL_MESSAGE(burst_size, band->burst_size,
                                "band burst_size error.");
      TEST_ASSERT_EQUAL_MESSAGE(experimenter, band->experimenter,
                                "band experimenter error.");
      i++;
    }

    TEST_ASSERT_EQUAL_MESSAGE(meter_band_num, i, "bund num error.");
  }

  ofp_meter_band_list_elem_free(&band_list);

  return ret;
}

void
test_ofp_meter_band_experimenter_parse(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(test_ofp_meter_band_experimenter_parse_wrap,
                           "ff ff"
                           "00 10"
                           "00 00 00 0a"
                           "00 00 00 01"
                           "00 00 00 30");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_meter_band_experimenter_parse(normal) error.");
}

void
test_ofp_meter_band_experimenter_parse_bad_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          test_ofp_meter_band_experimenter_parse_wrap,
          "ff ff"
          "00 10"
          "00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_meter_band_experimenter_parse(bat length) error.");
}

static lagopus_result_t
ofp_band_list_encode_wrap(struct channel *channel,
                          struct pbuf **pbuf,
                          struct ofp_header *xid_header) {
  struct meter_band_list band_list;
  lagopus_result_t ret;
  uint16_t total_length;
  uint16_t length;
  (void) channel;
  (void) xid_header;

  create_data(&band_list);
  if (band_num == 0) {
    length = 0;
  } else {
    length = (uint16_t) (BAND_LENGTH * band_num);
  }
  *pbuf = pbuf_alloc(length);
  (*pbuf)->plen = length;

  ret = ofp_band_list_encode(NULL, pbuf, &band_list,
                             &total_length);

  TEST_ASSERT_EQUAL_MESSAGE(length, total_length,
                            "band length error.");

  /* after. */
  ofp_meter_band_list_elem_free(&band_list);

  return ret;
}

void
test_ofp_band_list_encode(void) {
  lagopus_result_t ret;

  band_num = 2;
  ret = check_packet_create(ofp_band_list_encode_wrap,
                            "00 01 00 10 00 00 00 01"
                            "00 00 00 02 00 00 00 00"
                            "00 01 00 10 00 00 00 02"
                            "00 00 00 03 00 00 00 00");

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_band_list_encode(normal) error.");
}

void
test_ofp_band_list_encode_band_list_empty(void) {
  lagopus_result_t ret;

  band_num = 0;
  ret = check_packet_create(ofp_band_list_encode_wrap,
                            "");

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_band_list_encode(band_list empty) error.");
}

void
test_ofp_band_list_encode_null(void) {
  struct meter_band_list band_list;
  struct pbuf *pbuf;
  lagopus_result_t ret;
  uint16_t total_length;

  ret = ofp_band_list_encode(NULL, NULL, &band_list,
                             &total_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_band_list_encode(NULL) error.");

  ret = ofp_band_list_encode(NULL, &pbuf, NULL,
                             &total_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_band_list_encode(NULL) error.");

  ret = ofp_band_list_encode(NULL, &pbuf, &band_list,
                             NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_band_list_encode(NULL) error.");
}

void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
