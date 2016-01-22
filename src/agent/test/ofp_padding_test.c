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
#include "../ofp_padding.h"

void
setUp(void) {
}

void
tearDown(void) {
}

static lagopus_result_t
ofp_padding_encode_wrap(size_t length,
                        size_t ret_length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *test_pbuf;
  struct pbuf *ret_pbuf;

  /* Create test data. */
  test_pbuf = pbuf_alloc(ret_length);
  memset(test_pbuf->data, 0, length);
  test_pbuf->plen = ret_length - length;
  test_pbuf->putp += length;

  ret_pbuf = pbuf_alloc(ret_length);
  memset(ret_pbuf->data, 0, ret_length);
  ret_pbuf->plen = 0;
  ret_pbuf->putp += ret_length;

  /* Call func. */
  ret = ofp_padding_encode(NULL, &test_pbuf, (uint16_t *)&length);

  if (ret == LAGOPUS_RESULT_OK) {
    TEST_ASSERT_EQUAL_MESSAGE(0, test_pbuf->plen,
                              "packet length error.");
    TEST_ASSERT_EQUAL_MESSAGE(ret_length,
                              test_pbuf->putp - test_pbuf->getp,
                              "packet data length error.");
    TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(test_pbuf->data,
                                        ret_pbuf->data,
                                        ret_length),
                              "packet data error.");
  }

  /* free. */
  pbuf_free(test_pbuf);
  pbuf_free(ret_pbuf);

  return ret;
}

void
test_ofp_padding_encode_normal_pattern0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = ofp_padding_encode_wrap(3, 8);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_padding_encode error.");
}

void
test_ofp_padding_encode_normal_pattern1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = ofp_padding_encode_wrap(0, 0);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_padding_encode error.");
}

void
test_ofp_padding_encode_normal_pattern2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = ofp_padding_encode_wrap(8, 8);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_padding_encode error.");
}

void
test_ofp_padding_encode_bad_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = ofp_padding_encode_wrap(3, 3);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "ofp_padding_encode error (bad length).");
}

void
test_ofp_padding_encode_overflow(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = ofp_padding_encode_wrap(OFP_PACKET_MAX_SIZE,
                                OFP_PACKET_MAX_SIZE + 100);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "ofp_padding_encode error (overflow).");
}

void
test_ofp_padding_encode_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t length = 0;
  struct pbuf *test_pbuf = pbuf_alloc(OFP_PACKET_MAX_SIZE);

  ret = ofp_padding_encode(NULL, NULL, &length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_padding_encode error (null).");
  ret = ofp_padding_encode(NULL, &test_pbuf, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_padding_encode error (null).");

  /* free. */
  pbuf_free(test_pbuf);
}

static lagopus_result_t
ofp_padding_add_normal_wrap(uint16_t length,
                            uint16_t pad_length,
                            uint16_t ret_length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *test_pbuf;
  struct pbuf *ret_pbuf;

  /* Create test data. */
  test_pbuf = pbuf_alloc(ret_length);
  memset(test_pbuf->data, 0, length);
  test_pbuf->plen = (size_t) (ret_length - length);
  test_pbuf->putp += length;

  ret_pbuf = pbuf_alloc(ret_length);
  memset(ret_pbuf->data, 0, ret_length);
  ret_pbuf->plen = 0;
  ret_pbuf->putp += ret_length;

  /* Call func. */
  ret = ofp_padding_add(test_pbuf, pad_length);

  if (ret == LAGOPUS_RESULT_OK) {
    TEST_ASSERT_EQUAL_MESSAGE(0, test_pbuf->plen,
                              "packet length error.");
    TEST_ASSERT_EQUAL_MESSAGE(ret_length,
                              test_pbuf->putp - test_pbuf->getp,
                              "packet data length error.");
    TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(test_pbuf->data,
                                        ret_pbuf->data,
                                        ret_length),
                              "packet data error.");
  }

  /* free. */
  pbuf_free(test_pbuf);
  pbuf_free(ret_pbuf);

  return ret;
}

void
test_ofp_padding_add_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = ofp_padding_add_normal_wrap(1, 3, 4);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_padding_add error.");
}

void
test_ofp_padding_add_bad_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = ofp_padding_add_normal_wrap(1, 5, 4);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "ofp_padding_add(bad length) error.");
}

void
test_ofp_padding_add_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t pad_length = 0;

  ret = ofp_padding_add(NULL, pad_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_padding_add(null) error.");
}
