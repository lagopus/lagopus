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
#include "openflow.h"
#include "openflow13.h"
#include "handler_test_utils.h"
#include "../ofp_element.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
}

static int elem_n = 0;
static int bitmap_n = 0;
static int pad_l = 0;

static void
create_data(struct element_list *element_list,
            int elem_num, int bitmap_num, int pad_len, size_t *length) {
  struct element *element;
  struct bitmap *bitmap;
  int i;
  int j;

  *length = (sizeof(struct ofp_hello_elem_header) +
             (sizeof(uint32_t) * (size_t) bitmap_num) + (size_t) pad_len) *
            (size_t) elem_num;

  TAILQ_INIT(element_list);
  for (i = 0; i < elem_num; i++) {
    element = element_alloc();
    element->length = 0;
    element->type = OFPHET_VERSIONBITMAP;

    TAILQ_INIT(&element->bitmap_list);
    for (j = 0; j < bitmap_num; j++) {
      bitmap = bitmap_alloc();

      bitmap->bitmap = (uint32_t) (0x01 + j);
      TAILQ_INSERT_TAIL(&element->bitmap_list,
                        bitmap, entry);

    }
    TAILQ_INSERT_TAIL(element_list,
                      element, entry);
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
void
test_bitmap_alloc(void) {
  struct bitmap *bitmap;

  /* call func. */
  bitmap = bitmap_alloc();

  TEST_ASSERT_NOT_NULL(bitmap);
  free(bitmap);
}

void
test_bitmap_list_elem_free(void) {
  struct bitmap *bitmap;
  struct bitmap_list bitmap_list;
  int i;
  int max_cnt = 4;

  TAILQ_INIT(&bitmap_list);

  /* data */
  for (i = 0; i < max_cnt; i++) {
    bitmap = (struct bitmap *)
             malloc(sizeof(struct bitmap));
    TAILQ_INSERT_TAIL(&bitmap_list, bitmap, entry);
  }

  TEST_ASSERT_EQUAL_MESSAGE(TAILQ_EMPTY(&bitmap_list),
                            false, "not bitmap list error.");

  /* call func. */
  bitmap_list_elem_free(&bitmap_list);

  TEST_ASSERT_EQUAL_MESSAGE(TAILQ_EMPTY(&bitmap_list),
                            true, "bitmap list error.");
}

void
test_element_alloc(void) {
  struct element *element;

  /* call func. */
  element = element_alloc();

  TEST_ASSERT_NOT_NULL(element);
  free(element);
}

void
test_element_list_elem_free(void) {
  struct element *element;
  struct bitmap *bitmap;
  struct element_list element_list;
  int i;
  int j;
  int max_cnt = 4;

  TAILQ_INIT(&element_list);

  /* data */
  for (i = 0; i < max_cnt; i++) {
    element = element_alloc();
    for (j = 0; j < max_cnt; j++) {
      bitmap = (struct bitmap *)
               malloc(sizeof(struct bitmap));
      TAILQ_INSERT_TAIL(&element->bitmap_list, bitmap, entry);
    }
    TAILQ_INSERT_TAIL(&element_list, element, entry);
  }

  TEST_ASSERT_EQUAL_MESSAGE(TAILQ_EMPTY(&element_list),
                            false, "not element list error.");

  /* call func. */
  element_list_elem_free(&element_list);

  TEST_ASSERT_EQUAL_MESSAGE(TAILQ_EMPTY(&element_list),
                            true, "element list error.");
}

static lagopus_result_t
ofp_element_parse_normal_wrap(struct channel *channel,
                              struct pbuf *pbuf,
                              struct ofp_header *xid_header,
                              struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct element_list element_list;
  struct element *element;
  struct bitmap *bitmap;
  uint16_t type = 0x01;
  uint16_t length = 0x0c;
  uint32_t bp[] = {0x10, 0x20};
  (void) channel;
  (void) xid_header;
  (void) error;

  TAILQ_INIT(&element_list);

  /* call func.*/
  ret = ofp_element_parse(pbuf, &element_list, error);

  TAILQ_FOREACH(element, &element_list, entry) {
    TEST_ASSERT_EQUAL_MESSAGE(element->type,
                              type, "type error.");
    TEST_ASSERT_EQUAL_MESSAGE(element->length,
                              length, "type error.");
    TEST_ASSERT_EQUAL_MESSAGE(TAILQ_EMPTY(&element->bitmap_list), false,
                              "bitmap list error.");

    bitmap = TAILQ_FIRST(&element->bitmap_list);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(bitmap,
                                  NULL, "bitmap error.");
    if (bitmap != NULL) {
      TEST_ASSERT_EQUAL_MESSAGE(bitmap->bitmap,
                                bp[0], "bitmap error.");
      bitmap = TAILQ_NEXT(bitmap, entry);
    }
    TEST_ASSERT_NOT_EQUAL_MESSAGE(bitmap,
                                  NULL, "bitmap error.");
    if (bitmap != NULL) {
      TEST_ASSERT_EQUAL_MESSAGE(bitmap->bitmap,
                                bp[1], "bitmap error.");
    }
  }

  /* after */
  element_list_elem_free(&element_list);

  return ret;
}

void
test_ofp_element_parse_normal(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(ofp_element_parse_normal_wrap,
                           "00 01 00 0c"
                           "00 00 00 10"
                           "00 00 00 20"
                           "00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_element_parse_normal error.");
}

static lagopus_result_t
ofp_element_parse_error_wrap(struct channel *channel,
                             struct pbuf *pbuf,
                             struct ofp_header *xid_header,
                             struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct element_list element_list;
  (void) channel;
  (void) xid_header;

  TAILQ_INIT(&element_list);

  /* call func.*/
  ret = ofp_element_parse(pbuf, &element_list, error);

  TEST_ASSERT_EQUAL_MESSAGE(TAILQ_EMPTY(&element_list), true,
                            "element list error.");

  element_list_elem_free(&element_list);

  return ret;
}

void
test_ofp_element_parse_short_packet(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_element_parse_error_wrap,
                                        "00 01 00 0c"
                                        "00 00 00 10"
                                        "00 00 00 00"
                                        "00 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_element_parse_short_packet error.");
}

void
test_ofp_element_parse_short_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_element_parse_error_wrap,
                                        "00 01 00 02"
                                        "00 00 00 10",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_element_parse_short_length error.");
}

void
test_ofp_element_parse_long_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_element_parse_error_wrap,
                                        "00 01 00 10"
                                        "00 00 00 10",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_element_parse_long_length error.");
}

void
test_ofp_element_parse_short_packet_tlv(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_element_parse_error_wrap,
                                        "00 01 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_element_parse_short_packet_tlv error.");
}

void
test_ofp_element_parse_not_elem_but_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_element_parse_error_wrap,
                                        "00 01 00 04"
                                        "00 00 00 10"
                                        "00 00 00 20"
                                        "00 00 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_element_parse_short_packet_tlv error.");
}

static lagopus_result_t
ofp_element_list_encode_wrap(struct channel *channel,
                             struct pbuf **pbuf,
                             struct ofp_header *xid_header) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t length;
  struct element_list element_list;
  (void) channel;
  (void) xid_header;

  create_data(&element_list, elem_n, bitmap_n, pad_l, &length);
  *pbuf = pbuf_alloc(length);
  (*pbuf)->plen = length;

  ret = ofp_element_list_encode(*pbuf, &element_list);

  element_list_elem_free(&element_list);

  return ret;
}

void
test_ofp_element_list_encode_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  elem_n = 2;
  bitmap_n = 2;
  pad_l = 4;

  ret = check_packet_create(ofp_element_list_encode_wrap,
                            "00 01 00 0C 00 00 00 01"
                            "00 00 00 02 00 00 00 00"
                            "00 01 00 0C 00 00 00 01"
                            "00 00 00 02 00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_element_list_encode(normal) error.");
}

void
test_ofp_element_list_encode_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  elem_n = 1;
  bitmap_n = 1;
  pad_l = 0;

  ret = check_packet_create(ofp_element_list_encode_wrap,
                            "00 01 00 08 00 00 00 01");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_element_list_encode(normal) error.");
}

static lagopus_result_t
ofp_element_list_encode_element_list_empty_wrap(
  struct channel *channel,
  struct pbuf **pbuf,
  struct ofp_header *xid_header) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t length;
  struct element_list element_list;
  (void) channel;
  (void) xid_header;

  create_data(&element_list, 0, 0, 0, &length);
  *pbuf = pbuf_alloc(0);
  (*pbuf)->plen = 0;

  ret = ofp_element_list_encode(*pbuf, &element_list);

  element_list_elem_free(&element_list);

  return ret;
}

void
test_ofp_element_list_encode_element_list_empty(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_create(ofp_element_list_encode_element_list_empty_wrap,
                            "");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_element_list_encode(normal) error.");
}

void
test_ofp_element_list_encode_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *pbuf = pbuf_alloc(0);
  struct element_list element_list;

  ret = ofp_element_list_encode(NULL, &element_list);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_element_list_encode(null) error.");

  ret = ofp_element_list_encode(pbuf, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_element_list_encode(null) error.");

  /* after. */
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
