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

static struct port_desc *
port_desc_alloc(void) {
  struct port_desc *port_desc;
  port_desc = (struct port_desc *)
              calloc(1, sizeof(struct port_desc));
  return port_desc;
}

static int test_num = 0;

static void
create_data(struct port_desc_list *port_desc_list) {
  struct port_desc *port_desc;
  int i;

  TAILQ_INIT(port_desc_list);
  for (i = 0; i < test_num; i++) {
    port_desc = port_desc_alloc();

    port_desc->ofp.port_no    = (uint32_t) (0x01 + i);
    port_desc->ofp.hw_addr[0] = (uint8_t ) (0x02 + i);
    port_desc->ofp.hw_addr[1] = (uint8_t ) (0x03 + i);
    port_desc->ofp.hw_addr[2] = (uint8_t ) (0x04 + i);
    port_desc->ofp.hw_addr[3] = (uint8_t ) (0x05 + i);
    port_desc->ofp.hw_addr[4] = (uint8_t ) (0x06 + i);
    port_desc->ofp.hw_addr[5] = (uint8_t ) (0x07 + i);
    port_desc->ofp.name[0]    = (char) ('a' + i);
    port_desc->ofp.name[1]    = (char) ('b' + i);
    port_desc->ofp.name[2]    = (char) ('c' + i);
    port_desc->ofp.name[3]    = (char) ('d' + i);
    port_desc->ofp.config     = (uint32_t) (0x03 + i);
    port_desc->ofp.state      = (uint32_t) (0x04 + i);
    port_desc->ofp.curr       = (uint32_t) (0x05 + i);
    port_desc->ofp.advertised = (uint32_t) (0x06 + i);
    port_desc->ofp.supported  = (uint32_t) (0x07 + i);
    port_desc->ofp.peer       = (uint32_t) (0x08 + i);
    port_desc->ofp.curr_speed = (uint32_t) (0x09 + i);
    port_desc->ofp.max_speed  = (uint32_t) (0x0a + i);

    TAILQ_INSERT_TAIL(port_desc_list, port_desc, entry);
  }
}

static lagopus_result_t
s_ofp_port_desc_reply_create_wrap(struct channel *channel,
                                  struct pbuf_list **pbuf_list,
                                  struct ofp_header *xid_header) {
  struct port_desc_list port_desc_list;
  create_data(&port_desc_list);

  return ofp_port_desc_reply_create(channel,
                                    pbuf_list,
                                    &port_desc_list,
                                    xid_header);
}

/* TODO remove */
static lagopus_result_t
ofp_multipart_request_handle_wrap(struct channel *channel,
                                  struct pbuf *pbuf,
                                  struct ofp_header *xid_header,
                                  struct ofp_error *error) {
  return ofp_multipart_request_handle(channel, pbuf, xid_header, error);
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
test_ofp_port_desc_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(ofp_multipart_request_handle_wrap,
                           "04 12 00 10 00 00 00 10 "
                           "00 0D 00 00 00 00 00 00 ");
  /* The request body is empty. */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_port_desc_request_handle(normal) error.");
}

void
test_ofp_port_desc_handle_error_invalid_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  /* invalid body (no flags, pad) */
  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 08 00 00 00 10 "
                                        "00 0D",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR,
                            ret,
                            "invalid-body error");
}

void
test_ofp_port_desc_handle_error_invalid_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  /* invalid body (no pad) */
  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 08 00 00 00 10 "
                                        "00 0D 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR,
                            ret,
                            "invalid-body error");
}

void
test_ofp_port_desc_handle_error_invalid_length2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  /* invalid body (over size) */
  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 12 00 00 00 10 "
                                        "00 0D 00 00 00 00 00 00 "
                                        "00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR,
                            ret,
                            "invalid-body error");
}

void
test_ofp_port_desc_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header xid_header;
  struct ofp_error error;

  ret = ofp_port_desc_request_handle(NULL, pbuf, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_port_desc_request_handle(channel, NULL, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_port_desc_request_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (xid_header");

  ret = ofp_port_desc_request_handle(channel, pbuf, &xid_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (ofp_error)");

  /* after. */
  channel_free(channel);
  pbuf_free(pbuf);
}

void
test_ofp_port_desc_reply_create1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[1] = {"04 13 00 50 00 00 00 10 "
                         "00 0D 00 00 00 00 00 00 "
                         // body
                         "00 00 00 01 "             /* uint32_t port_no;     */
                         "00 00 00 00 "             /* uint8_t  pad[4];      */
                         "02 03 04 05 06 07 "       /* uint8_t  hw_addr[6];  */
                         "00 00 "                   /* uint8_t  pad2[2];     */
                         "61 62 63 64 00 00 00 00 " /* char     name[16];    */
                         "00 00 00 00 00 00 00 00 " /*                       */
                         "00 00 00 03 "             /* uint32_t config;      */
                         "00 00 00 04 "             /* uint32_t state;       */
                         "00 00 00 05 "             /* uint32_t curr;        */
                         "00 00 00 06 "             /* uint32_t advertised;  */
                         "00 00 00 07 "             /* uint32_t supported;   */
                         "00 00 00 08 "             /* uint32_t peer;        */
                         "00 00 00 09 "             /* uint32_t curr_speed;  */
                         "00 00 00 0a "             /* uint32_t max_speed;   */
                        };

  /* port 0 */
  test_num = 1;
  ret = check_pbuf_list_packet_create(s_ofp_port_desc_reply_create_wrap,
                                      data, 1);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "create port 0 error.");
}

void
test_ofp_port_desc_reply_create2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[1] = {"04 13 00 90 00 00 00 10 "
                         "00 0D 00 00 00 00 00 00 "
                         // body1
                         "00 00 00 01 "             /* uint32_t port_no;     */
                         "00 00 00 00 "             /* uint8_t  pad[4];      */
                         "02 03 04 05 06 07 "       /* uint8_t  hw_addr[6];  */
                         "00 00 "                   /* uint8_t  pad2[2];     */
                         "61 62 63 64 00 00 00 00 " /* char     name[16];    */
                         "00 00 00 00 00 00 00 00 " /*                       */
                         "00 00 00 03 "             /* uint32_t config;      */
                         "00 00 00 04 "             /* uint32_t state;       */
                         "00 00 00 05 "             /* uint32_t curr;        */
                         "00 00 00 06 "             /* uint32_t advertised;  */
                         "00 00 00 07 "             /* uint32_t supported;   */
                         "00 00 00 08 "             /* uint32_t peer;        */
                         "00 00 00 09 "             /* uint32_t curr_speed;  */
                         "00 00 00 0a "             /* uint32_t max_speed;   */
                         // body2
                         "00 00 00 02 "             /* uint32_t port_no;     */
                         "00 00 00 00 "             /* uint8_t  pad[4];      */
                         "03 04 05 06 07 08 "       /* uint8_t  hw_addr[6];  */
                         "00 00 "                   /* uint8_t  pad2[2];     */
                         "62 63 64 65 00 00 00 00 " /* char     name[16];    */
                         "00 00 00 00 00 00 00 00 " /*                       */
                         "00 00 00 04 "             /* uint32_t config;      */
                         "00 00 00 05 "             /* uint32_t state;       */
                         "00 00 00 06 "             /* uint32_t curr;        */
                         "00 00 00 07 "             /* uint32_t advertised;  */
                         "00 00 00 08 "             /* uint32_t supported;   */
                         "00 00 00 09 "             /* uint32_t peer;        */
                         "00 00 00 0a "             /* uint32_t curr_speed;  */
                         "00 00 00 0b "             /* uint32_t max_speed;   */
                        };

  test_num = 2;
  ret = check_pbuf_list_packet_create(s_ofp_port_desc_reply_create_wrap,
                                      data, 1);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "create port 0 error.");
}

void
test_ofp_port_desc_reply_create_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf_list *pbuf_list = NULL;
  struct port_desc_list port_desc_list;
  struct ofp_header xid_header;

  ret = ofp_port_desc_reply_create(NULL, &pbuf_list, &port_desc_list,
                                   &xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_port_desc_reply_create(channel, NULL, &port_desc_list, &xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_port_desc_reply_create(channel, &pbuf_list, NULL, &xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (port_desc_list)");

  ret = ofp_port_desc_reply_create(channel, &pbuf_list, &port_desc_list, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (xid_header)");

  /* after. */
  channel_free(channel);
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
