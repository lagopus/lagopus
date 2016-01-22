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

void
setUp(void) {
}

void
tearDown(void) {
}

#define PBUF_LENGTH 100

void
test_pbuf_length_get_normal(void) {
  struct pbuf *pbuf = pbuf_alloc(PBUF_LENGTH);
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t length = 0;

  /* create test data. */
  pbuf->putp = pbuf->getp + PBUF_LENGTH;

  /* call func. */
  ret = pbuf_length_get(pbuf, &length);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "pbuf_length_get error.");
  TEST_ASSERT_EQUAL_MESSAGE(PBUF_LENGTH, length,
                            "pbuf length error.");

  pbuf_free(pbuf);
}

void
test_pbuf_length_get_overflow(void) {
  struct pbuf *pbuf = pbuf_alloc(UINT16_MAX);
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t length = 0;

  /* create test data. */
  pbuf->putp = pbuf->getp + UINT16_MAX + 1;

  /* call func. */
  ret = pbuf_length_get(pbuf, &length);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "pbuf_length_get error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, length,
                            "pbuf length error.");

  pbuf_free(pbuf);
}

void
test_pbuf_length_get_bad_pointer(void) {
  struct pbuf *pbuf = pbuf_alloc(PBUF_LENGTH);
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t length = 0;

  /* create test data. */
  pbuf->putp = pbuf->getp - 1;

  /* call func. */
  ret = pbuf_length_get(pbuf, &length);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "pbuf_length_get error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, length,
                            "pbuf length error.");

  pbuf_free(pbuf);
}

void
test_pbuf_encode_normal(void) {
  struct pbuf *pbuf = pbuf_alloc(PBUF_LENGTH);
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t data_length = 2;
  uint8_t data[2] = {0x01, 0x02};

  /* create test data */
  pbuf->plen = data_length;

  /* call func. */
  ret = pbuf_encode(pbuf, data, data_length);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "pbuf_encode error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, pbuf->plen,
                            "packet length error.");
  TEST_ASSERT_EQUAL_MESSAGE(data_length, pbuf->putp - pbuf->getp,
                            "packet data length error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(pbuf->data, data, data_length),
                            "data error.");
  /* after. */
  pbuf_free(pbuf);
}

void
test_pbuf_encode_bad_length(void) {
  struct pbuf *pbuf = pbuf_alloc(PBUF_LENGTH);
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t data_length = 2;
  uint8_t data[2] = {0x01, 0x02};

  /* create test data */
  /* short length. */
  pbuf->plen = data_length - 1;

  /* call func. */
  ret = pbuf_encode(pbuf, data, data_length);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "pbuf_encode error.");
  TEST_ASSERT_EQUAL_MESSAGE(data_length - 1, pbuf->plen,
                            "packet length error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, pbuf->putp - pbuf->getp,
                            "packet data length error.");

  /* after. */
  pbuf_free(pbuf);
}

void
test_pbuf_encode_null(void) {
  struct pbuf *pbuf = pbuf_alloc(PBUF_LENGTH);
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t data_length = 2;
  uint8_t data[2] = {0x01, 0x02};

  /* call func. */
  ret = pbuf_encode(NULL, data, data_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "pbuf_encode error.");
  ret = pbuf_encode(pbuf, NULL, data_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "pbuf_encode error.");

  /* after. */
  pbuf_free(pbuf);
}

void
test_pbuf_info_store_load_normal(void) {
  struct pbuf *pbuf = pbuf_alloc(PBUF_LENGTH);
  pbuf_info_t pbuf_info;

  /* create test data */
  pbuf->getp = pbuf->data + 1;
  pbuf->putp = pbuf->data + 3;
  pbuf->plen = 2;

  /* call store func. */
  pbuf_info_store(pbuf, &pbuf_info);

  TEST_ASSERT_EQUAL_MESSAGE(pbuf->data + 1, pbuf_getp_get(&pbuf_info),
                            "getp error.");
  TEST_ASSERT_EQUAL_MESSAGE(pbuf->data + 3, pbuf_putp_get(&pbuf_info),
                            "putp error.");
  TEST_ASSERT_EQUAL_MESSAGE(2, pbuf_plen_get(&pbuf_info),
                            "plen error.");

  /* create test data */
  pbuf_reset(pbuf);
  /* check reset. */
  TEST_ASSERT_EQUAL_MESSAGE(pbuf->data, pbuf_getp_get(pbuf),
                            "getp error.");
  TEST_ASSERT_EQUAL_MESSAGE(pbuf->data, pbuf_putp_get(pbuf),
                            "putp error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, pbuf_plen_get(pbuf),
                            "plen error.");

  /* call load func. */
  pbuf_info_load(pbuf, &pbuf_info);

  TEST_ASSERT_EQUAL_MESSAGE(pbuf->data + 1, pbuf_getp_get(pbuf),
                            "getp error.");
  TEST_ASSERT_EQUAL_MESSAGE(pbuf->data + 3, pbuf_putp_get(pbuf),
                            "putp error.");
  TEST_ASSERT_EQUAL_MESSAGE(2, pbuf_plen_get(pbuf),
                            "plen error.");

  /* after. */
  pbuf_free(pbuf);
}
