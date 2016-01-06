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
#include "../ofp_header_handler.h"
#include "handler_test_utils.h"
#include "lagopus/pbuf.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
}

static lagopus_result_t
s_ofp_header_handle_wrap(struct channel *channel,
                         struct pbuf *pbuf,
                         struct ofp_header *xid_header,
                         struct ofp_error *error) {
  (void) channel;
  (void) xid_header;
  (void) error;
  return ofp_header_handle(channel, pbuf, error);
}

static uint8_t s_type = OFPT_ECHO_REQUEST;
static uint16_t s_plen = 0x08;

static lagopus_result_t
s_ofp_header_create_wrap(struct channel *channel,
                         struct pbuf **pbuf,
                         struct ofp_header *xid_header) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_header header;
  struct pbuf *ret_pbuf = NULL;
  if (pbuf == NULL) {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  } else {
    ret_pbuf = pbuf_alloc(65535);
    if (ret_pbuf == NULL) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    } else {
      ret_pbuf->plen = s_plen;
      ret = ofp_header_create(channel, s_type, xid_header, &header, ret_pbuf);
      if (ret == LAGOPUS_RESULT_OK) {
        *pbuf = ret_pbuf;
      } else {
        pbuf_free(ret_pbuf);
        *pbuf = NULL;
      }
    }
  }
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
test_ofp_header_handle(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(s_ofp_header_handle_wrap,
                           "04 00 00 08 00 00 00 64");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_header_handle error.");
}

void
test_ofp_header_handle_error(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(s_ofp_header_handle_wrap,
                                        "04 00 00 08 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_header_handle error.");
}

void
test_ofp_header_set(void) {
  struct ofp_header header;
  uint8_t version = 0x04;
  uint8_t type = OFPT_FLOW_MOD;
  uint16_t length = 0x0101;
  uint32_t xid = 0x01;

  /* Call func. */
  ofp_header_set(&header, version, type, length, xid);

  TEST_ASSERT_EQUAL_MESSAGE(header.version, version,
                            "ofp_header_set(version) error.");
  TEST_ASSERT_EQUAL_MESSAGE(header.type, type,
                            "ofp_header_set(type) error.");
  TEST_ASSERT_EQUAL_MESSAGE(header.length, length,
                            "ofp_header_set(length) error.");
  TEST_ASSERT_EQUAL_MESSAGE(header.xid, xid,
                            "ofp_header_set(xid) error.");
}

void
test_ofp_header_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  s_type = OFPT_ECHO_REQUEST;
  s_plen = 0x08;
  ret = check_packet_create(s_ofp_header_create_wrap,
                            "04 02 00 08 00 00 00 10");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_header_create error.");
}
void
test_ofp_header_create_error(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  s_type = OFPT_ECHO_REQUEST;
  s_plen = 0x06;
  ret = check_packet_create(s_ofp_header_create_wrap,
                            "04 00 00 08 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OUT_OF_RANGE, ret,
                            "ofp_header_create error.");
}

static lagopus_result_t
ofp_header_mp_copy_wrap(struct channel *channel,
                        struct pbuf *pbuf,
                        struct ofp_header *xid_header,
                        struct ofp_error *error) {
  lagopus_result_t ret;
  struct pbuf *dst_pbuf;
  uint16_t src_len = 0x12;
  uint16_t dst_len = 0x10;
  uint8_t ret_src[] = {0x04, 0x13, 0x00, 0x12,
                       0x00, 0x00, 0x00, 0x10,
                       0x00, 0x05, 0x00, 0x01,
                       0x00, 0x00, 0x00, 0x00,
                       0x12, 0x34
                      };
  uint8_t ret_dst[] = {0x04, 0x13, 0x00, 0x10,
                       0x00, 0x00, 0x00, 0x10,
                       0x00, 0x05, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00
                      };
  (void) channel;
  (void) xid_header;
  (void) error;

  dst_pbuf = pbuf_alloc(OFP_PACKET_MAX_SIZE);
  dst_pbuf->plen = OFP_PACKET_MAX_SIZE;

  ret = ofp_header_mp_copy(dst_pbuf, pbuf);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_header_mp_copy error.");
  TEST_ASSERT_EQUAL_MESSAGE(src_len, pbuf->putp - pbuf->getp,
                            "src len error.");
  TEST_ASSERT_EQUAL_MESSAGE(src_len, pbuf->plen,
                            "src packet len error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, memcmp((uint8_t *) pbuf->data, ret_src,
                                      (size_t) (pbuf->putp - pbuf->getp)),
                            "src data len error.");

  TEST_ASSERT_EQUAL_MESSAGE(dst_len, dst_pbuf->putp - dst_pbuf->getp,
                            "dst len error.");
  TEST_ASSERT_EQUAL_MESSAGE(OFP_PACKET_MAX_SIZE - dst_len, dst_pbuf->plen,
                            "dst packet len error.");

  TEST_ASSERT_EQUAL_MESSAGE(0,
                            memcmp((uint8_t *) dst_pbuf->data, ret_dst,
                                   (size_t) (dst_pbuf->putp - dst_pbuf->getp)),
                            "dst data len error.");

  pbuf_free(dst_pbuf);

  /* Not check result.*/
  return -9999;
}

void
test_ofp_header_mp_copy(void) {
  /* Not check result.*/
  (void) check_packet_parse(ofp_header_mp_copy_wrap,
                            "04 13 00 12 00 00 00 10"
                            "00 05 00 00 00 00 00 00"
                            "12 34");
}

static lagopus_result_t
ofp_header_packet_set_wrap(struct channel **channels,
                           struct ofp_header *xid_headers,
                           struct pbuf *pbuf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t ret_data[] = {0x04, 0x0a, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x10,
                        0x00, 0x00, 0x00, 0x12, 0x00, 0x05, 0x02, 0x30,
                        0x00, 0x01, 0x00, 0x16,
                        0x80, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10,
                        0x80, 0x00, 0x08, 0x06, 0x00, 0x0c, 0x29, 0x7a,
                        0x90, 0xb3,
                        0x12, 0x34, 0x56, 0x78, 0x90
                       };
  uint8_t *putp_head;
  uint8_t *getp_head;
  size_t plen;
  (void) xid_headers;

  /* Current pbuf info. */
  putp_head = pbuf->putp;
  getp_head = pbuf->getp;
  plen = pbuf->plen;

  /* Call func. (data is packet_in) */
  ret = ofp_header_packet_set(channels[0], pbuf);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_header_packet_set error.");
  TEST_ASSERT_EQUAL_MESSAGE(putp_head, pbuf->putp,
                            "putp error.");
  TEST_ASSERT_EQUAL_MESSAGE(getp_head, pbuf->getp,
                            "getp error.");
  TEST_ASSERT_EQUAL_MESSAGE(plen, pbuf->plen,
                            "packet len error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(ret_data, pbuf->data, plen),
                            "packet data error.");

  /* Not check return val.*/
  return -9999;
}

void
test_ofp_header_packet_set(void) {
  /* Not check return val.*/
  (void) check_use_channels_send(ofp_header_packet_set_wrap,
                                 "00 0a 00 00 00 00 00 00"
                                 "00 00 00 12 00 05 02 30"
                                 "00 01 00 16"
                                 "80 00 00 04 00 00 00 10"
                                 "80 00 08 06 00 0c 29 7a 90 b3"
                                 "12 34 56 78 90");
}

static lagopus_result_t
ofp_header_length_set_wrap(struct channel **channels,
                           struct ofp_header *xid_headers,
                           struct pbuf *pbuf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t ret_data[] = {0x04, 0x0a, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x20,
                        0x00, 0x00, 0x00, 0x12, 0x00, 0x05, 0x02, 0x30,
                        0x00, 0x01, 0x00, 0x16,
                        0x80, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10,
                        0x80, 0x00, 0x08, 0x06, 0x00, 0x0c, 0x29, 0x7a,
                        0x90, 0xb3,
                        0x12, 0x34, 0x56, 0x78, 0x90
                       };
  uint16_t length = 0x2b;
  uint8_t *putp_head;
  uint8_t *getp_head;
  size_t plen;
  (void) channels;
  (void) xid_headers;

  /* Current pbuf info. */
  putp_head = pbuf->putp;
  getp_head = pbuf->getp;
  plen = pbuf->plen;

  /* Call func. (data is packet_in) */
  ret = ofp_header_length_set(pbuf, length);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_header_packet_set error.");
  TEST_ASSERT_EQUAL_MESSAGE(putp_head, pbuf->putp,
                            "putp error.");
  TEST_ASSERT_EQUAL_MESSAGE(getp_head, pbuf->getp,
                            "getp error.");
  TEST_ASSERT_EQUAL_MESSAGE(plen, pbuf->plen,
                            "packet len error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(ret_data, pbuf->data, plen),
                            "packet data error.");

  /* Not check return val.*/
  return -9999;
}

void
test_ofp_header_length_set(void) {
  /* Not check return val.*/
  (void) check_use_channels_send(ofp_header_length_set_wrap,
                                 "04 0a 00 2b 00 00 00 20"
                                 "00 00 00 12 00 05 02 30"
                                 "00 01 00 16"
                                 "80 00 00 04 00 00 00 10"
                                 "80 00 08 06 00 0c 29 7a 90 b3"
                                 "12 34 56 78 90");
}

static lagopus_result_t
ofp_header_version_check_wrap(struct channel **channels,
                              struct ofp_header *headers) {
  bool ret;
  uint8_t version1 = 0x04;
  uint8_t version2 = 0x05;

  /* create data. */
  headers[0].version = version1;
  headers[1].version = version2;

  /* call func. */
  ret = ofp_header_version_check(channels[0], &headers[0]);
  TEST_ASSERT_EQUAL_MESSAGE(true, ret, "version error.");

  ret = ofp_header_version_check(channels[0], &headers[1]);
  TEST_ASSERT_EQUAL_MESSAGE(false, ret, "version error.");

  /* NULL */
  ret = ofp_header_version_check(NULL, &headers[0]);
  TEST_ASSERT_EQUAL_MESSAGE(false, ret, "version error.");
  ret = ofp_header_version_check(channels[0], NULL);
  TEST_ASSERT_EQUAL_MESSAGE(false, ret, "version error.");

  return LAGOPUS_RESULT_OK;
}

void
test_ofp_header_version_check(void) {
  (void) check_use_channels(ofp_header_version_check_wrap);
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
