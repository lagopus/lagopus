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
#include "../ofp_match.h"
#include "../ofp_instruction.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
}

static struct ofp_aggregate_stats_reply s_aggre_reply;

static lagopus_result_t
s_ofp_aggregate_reply_create_wrap(struct channel *channel,
                                  struct pbuf_list **pbuf_list,
                                  struct ofp_header *xid_header) {
  return ofp_aggregate_stats_reply_create(channel, pbuf_list,
                                          &s_aggre_reply, xid_header);
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
test_ofp_aggregate_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00"
          /* <--------------------------------------------->  ofp_multipart_request
           * <---------------------> ofp_header
           * <> version
           *    <> type
           *       <---> length
           *             <---------> xid
           *                         <---> type = 2
           *                               <---> flags
           *                                     <---------> padding
           */
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00"
          /* <--------------------------------------------... ofp_aggregate_stats_request
           * <> table_id
           *    <------> padding
           *             <---------> out_port
           *                         <---------> out_group
           *                                     <---------> padding
           */
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05"
          /*...-------------------------------------------... ofp_aggregate_stats_request
           * <---------------------> cookie
           *                         <---------------------> cookie mask
           */
          "00 01 00 0c 80 00 00 04 00 00 00 10 00 00 00 00");
  /*...-------------------------------------------->  ofp_aggregate_stats_request
   * <--------------------------------------------->  ofp_match
   * <---> type = 1 OXM match
   *       <---> length = 12
   *             <---------> OXM TLV header (oxm_class = 0x8000 -> OFPXMC_OPENFLOW_BASIC
   *                                         oxm_field = 0 -> OFPXMT_OFB_IN_PORT,
   *                                         oxm_hashmask = 0,
   *                                         oxm_length = 8)
   *                         <---------> OXML TLV payload ( value = 1)
   *                                     <---------> padding
   */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_aggregate_request_handle(normal) error.");
}

void
test_ofp_aggregate_normal_pattern_divide_two_handle(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *s[2] = {
    "04 12 00 20 00 00 00 10 00 02 00 01 00 00 00 00"
    /* <---------------------------------------------> ofp_multipart_request
     * <---------------------> ofp_header
     * <> version
     *    <> type
     *       <---> length
     *             <---------> xid
     *                         <---> type = 2
     *                               <---> flags = 0x0001 -> OFPMPF_REQ_MORE
     *                                     <---------> padding
     */
    "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00",
    /* <---------------------------------------------> ofp_aggregate_stats_request
     * <> table_id = 1
     *    <------> padding
     *             <---------> out_port = 2
     *                         <---------> out_group = 3
     *                                     <---------> padding
     */
    "04 12 00 30 00 00 00 10 00 02 00 00 00 00 00 00"
    /* <---------------------------------------------> ofp_multipart_request
     * <---------------------> ofp_header
     * <> version
     *    <> type
     *       <---> length
     *             <---------> xid
     *                         <---> type = 2
     *                               <---> flags = 0x0000
     *                                     <---------> padding
     */
    "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05"
    /*...-------------------------------------------... ofp_aggregate_stats_request
     * <---------------------> cookie
     *                         <---------------------> cookie mask
     */
    "00 01 00 0c 80 00 00 04 00 00 00 10 00 00 00 00"
  };
  /*...-------------------------------------------->  ofp_aggregate_stats_request
   * <--------------------------------------------->  ofp_match
   * <---> type = 1 OXM match
   *       <---> length = 12
   *             <---------> OXM TLV header (oxm_class = 0x8000 -> OFPXMC_OPENFLOW_BASIC
   *                                         oxm_field = 0 -> OFPXMT_OFB_IN_PORT,
   *                                         oxm_hashmask = 0,
   *                                         oxm_length = 8)
   *                         <---------> OXML TLV payload ( value = 1)
   *                                     <---------> padding
   */

  ret = check_packet_parse_array_for_multipart(
          ofp_multipart_request_handle, s, 2);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_aggregate_request_handle(normal) error.");
}

void
test_ofp_aggregate_handle_error_invalid_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* no body. */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "no-body error");
}

void
test_ofp_aggregate_handle_invalid_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_length2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_length3(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_length4(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00 "
          "01",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_length5(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00 "
          "01 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_length6(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_length7(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_length8(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_length9(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_length10(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_length11(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
          "00 01",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_length12(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
          "00 01 00 0c",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_length13(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
          "00 01 00 0c 80 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_length14(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
          "00 01 00 0c 80 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_length15(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
          "00 01 00 0c 80 00 00 04",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_length16(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body (over size) */
  ret = check_packet_parse_expect_error
        (ofp_multipart_request_handle,
         "04 12 00 48 00 00 00 10 00 02 00 00 00 00 00 00 "
         "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
         "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
         "00 01 00 0c 80 00 00 04 00 00 00 10 00 00 00 00 "
         "ff ff ff ff ff ff ff ff",
         &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_invalid_match_unknown_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_TYPE);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00"
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00"
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05"
          "ff fe 00 0c 80 00 00 04 00 00 00 10 00 00 00 00",
          /*...-------------------------------------------->  ofp_aggregate_stats_request
           * <--------------------------------------------->  ofp_match
           * <---> type = 0xfffe unknown match type
           */
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid match error.");
}

void
test_ofp_aggregate_handle_invalid_match_length_too_short(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 02 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
          "00 01 00 0c 80 00 00 04 00 00 00 10",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_aggregate_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header ofp_header;
  struct ofp_error error;

  ret = ofp_aggregate_stats_request_handle(NULL, pbuf, &ofp_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_aggregate_stats_request_handle(channel, NULL, &ofp_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_aggregate_stats_request_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (ofp_header)");

  ret = ofp_aggregate_stats_request_handle(channel, pbuf, &ofp_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (ofp_error)");
  channel_free(channel);
  pbuf_free(pbuf);
}

void
test_ofp_aggregate_reply_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *require[1] = {
    "04 13 00 28 00 00 00 10 00 02 00 00 00 00 00 00 "
    "00 00 00 00 00 00 00 01 00 00 00 00 00 00 00 02 "
    "00 00 00 03 00 00 00 00"
  };

  s_aggre_reply.packet_count = 0x0000000000000001;
  s_aggre_reply.byte_count   = 0x0000000000000002;
  s_aggre_reply.flow_count   = 0x00000003;

  ret = check_pbuf_list_packet_create(s_ofp_aggregate_reply_create_wrap, require,
                                      1);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "create port 0 error.");
}

void
test_ofp_aggregate_reply_create_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf_list *pbuf_list = NULL;
  struct ofp_aggregate_stats_reply aggre_reply;
  struct ofp_header ofp_header;

  ret = ofp_aggregate_stats_reply_create(NULL, &pbuf_list,
                                         &aggre_reply, &ofp_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_aggregate_stats_reply_create(channel, NULL,
                                         &aggre_reply, &ofp_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_aggregate_stats_reply_create(channel, &pbuf_list,
                                         NULL, &ofp_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (aggregate_stats_reply)");

  ret = ofp_aggregate_stats_reply_create(channel, &pbuf_list,
                                         &aggre_reply, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (ofp_header)");
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
