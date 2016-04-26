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

/* TODO remove */
static lagopus_result_t
ofp_multipart_request_handle_wrap(struct channel *channel,
                                  struct pbuf *pbuf,
                                  struct ofp_header *xid_header,
                                  struct ofp_error *error) {
  return ofp_multipart_request_handle(channel, pbuf, xid_header, error);
}

static struct flow_stats *
s_flow_stats_alloc(void) {
  struct flow_stats *flow_stats;

  flow_stats = calloc(1, sizeof(struct flow_stats));
  if (flow_stats != NULL) {
    TAILQ_INIT(&flow_stats->match_list);
    TAILQ_INIT(&flow_stats->instruction_list);
  }

  return flow_stats;
}

static struct flow_stats_list s_flow_stats_list;

static lagopus_result_t
s_ofp_flow_reply_create_wrap(struct channel *channel,
                             struct pbuf_list **pbuf_list,
                             struct ofp_header *xid_header) {
  return ofp_flow_stats_reply_create(channel, pbuf_list,
                                     &s_flow_stats_list,
                                     xid_header);
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
test_ofp_flow_handle(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(ofp_multipart_request_handle_wrap,
                           "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
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
                           "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
                           /* <--------------------------------------------... ofp_flow_stats_request
                            * <> table_id
                            *    <------> pad
                            *             <---------> out_port
                            *                         <---------> out_group
                            *                                     <---------> pad2
                            */
                           "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
                           /* <---------------------> cookie
                            *                         <---------------------> cookie_mask
                            */
                           "00 01 00 0c 80 00 00 04 00 00 00 10 00 00 00 00");
  /* <---------------------------------------------> ofp_match
   * <---> type = 1 OXM match
   *       <---> length = 12
   *             <---------> OXM TLV header (oxm_class = 0x8000
   *                                          -> OFPXMC_OPENFLOW_BASIC
   *                                         oxm_field = 0
   *                                          -> OFPXMT_OFB_IN_PORT,
   *                                         oxm_hashmask = 0,
   *                                         oxm_length = 8)
   *                         <---------> OXML TLV payload ( value = 1)
   *                                     <---------> padding
   */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_flow_request_handle(normal) error.");
}

void
test_ofp_flow_handle_error_length0(void) {
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
test_ofp_flow_handle_error_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 01 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_flow_handle_error_length2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_flow_handle_error_length3(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
          "01",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_flow_handle_error_length4(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
          "01 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_flow_handle_error_length5(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_flow_handle_error_length6(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_flow_handle_error_length7(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_flow_handle_error_length8(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_flow_handle_error_length9(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_flow_handle_error_length10(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
          "00 01",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_flow_handle_error_length11(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
          "00 01 00 0c",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_flow_handle_error_length12(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
          "00 01 00 0c 80 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_flow_handle_error_length13(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
          "00 01 00 0c 80 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_flow_handle_error_length14(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
          "00 01 00 0c 80 00 00 04",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_flow_handle_error_length15(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_LEN);
  /* invalid body */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
          "00 01 00 0c 80 00 00 04 00 00 00 10",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_flow_handle_error_length16(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid body (over size) */
  ret = check_packet_parse_expect_error(
          ofp_multipart_request_handle,
          "04 12 00 48 00 00 00 10 00 01 00 00 00 00 00 00 "
          "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
          "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
          "00 01 00 0c 80 00 00 04 00 00 00 10 00 00 00 00 "
          "ff ff ff ff ff ff ff ff",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}


void
test_ofp_flow_handle_error_bad_mach_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_TYPE);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
                                        "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
                                        "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
                                        "00 02 00 0c 80 00 00 04 00 00 00 10 00 00 00 00",
                                        /* <---------------------------------------------> ofp_match
                                         * <---> type = 2 : bad match type
                                         *       <---> length = 12
                                         *             <---------> OXM TLV header (oxm_class = 0x8000
                                         *                                          -> OFPXMC_OPENFLOW_BASIC
                                         *                                         oxm_field = 0
                                         *                                          -> OFPXMT_OFB_IN_PORT,
                                         *                                         oxm_hashmask = 0,
                                         *                                         oxm_length = 8)
                                         *                         <---------> OXML TLV payload ( value = 1)
                                         *                                     <---------> padding
                                         */
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "bad mach type error.");
}

void
test_ofp_flow_handle_error_bad_oxm_class(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_TYPE);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
                                        "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
                                        "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
                                        "00 01 00 0c 80 10 00 04 00 00 00 10 00 00 00 00",
                                        /* <---------------------------------------------> ofp_match
                                         * <---> type = 1
                                         *       <---> length = 12
                                         *             <---------> OXM TLV header (oxm_class = 0x8010 : bad class
                                         *                                          -> OFPXMC_OPENFLOW_BASIC
                                         *                                         oxm_field = 0
                                         *                                          -> OFPXMT_OFB_IN_PORT,
                                         *                                         oxm_hashmask = 0,
                                         *                                         oxm_length = 8)
                                         *                         <---------> OXML TLV payload ( value = 1)
                                         *                                     <---------> padding
                                         */
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "bad oxm class error.");
}

void
test_ofp_flow_handle_error_bad_match_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  /* FIXME : ideal value is OFPBMC_BAD_LEN. */
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
                                        "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
                                        "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
                                        "00 01 00 11 80 00 00 04 00 00 00 10 00 00 00 00",
                                        /* <---------------------------------------------> ofp_match
                                         * <---> type = 1
                                         *       <---> length = 17 : bad length (12)
                                         *             <---------> OXM TLV header (oxm_class = 0x8000
                                         *                                          -> OFPXMC_OPENFLOW_BASIC
                                         *                                         oxm_field = 0
                                         *                                          -> OFPXMT_OFB_IN_PORT,
                                         *                                         oxm_hashmask = 0,
                                         *                                         oxm_length = 8)
                                         *                         <---------> OXML TLV payload ( value = 1)
                                         *                                     <---------> padding
                                         */
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "bad match length error.");
}

void
test_ofp_flow_handle_error_bad_match_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 40 00 00 00 10 00 01 00 00 00 00 00 00 "
                                        "01 00 00 00 00 00 00 02 00 00 00 03 00 00 00 00 "
                                        "00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 05 "
                                        "00 01 00 0b 80 00 00 04 00 00 00 10 00 00 00 00",
                                        /* <---------------------------------------------> ofp_match
                                         * <---> type = 1
                                         *       <---> length = 11 : bad length (12)
                                         *             <---------> OXM TLV header (oxm_class = 0x8000
                                         *                                          -> OFPXMC_OPENFLOW_BASIC
                                         *                                         oxm_field = 0
                                         *                                          -> OFPXMT_OFB_IN_PORT,
                                         *                                         oxm_hashmask = 0,
                                         *                                         oxm_length = 8)
                                         *                         <---------> OXML TLV payload ( value = 1)
                                         *                                     <---------> padding
                                         */
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "bad match length error.");
}

void
test_ofp_flow_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header ofp_header;
  struct ofp_error error;

  ret = ofp_flow_stats_request_handle(NULL, pbuf, &ofp_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_flow_stats_request_handle(channel, NULL, &ofp_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_flow_stats_request_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_flow_stats_request_handle(channel, pbuf, &ofp_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (ofp_error)");

  channel_free(channel);
  pbuf_free(pbuf);
}

void
test_ofp_flow_reply_create_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct flow_stats *flow_stats = NULL;
  struct instruction *instruction = NULL;
  struct match *match = NULL;
  const char *require[1] = {
    "04 13 00 58 00 00 00 10 00 01 00 00 00 00 00 00 "
    "00 48 01 00 00 00 00 02 00 00 00 03 00 04 00 05 "
    "00 06 00 07 00 00 00 00 00 00 00 00 00 00 00 08 "
    "00 00 00 00 00 00 00 09 00 00 00 00 00 00 00 0a "
    "00 01 00 10 00 00 01 08 00 00 00 00 00 00 00 00 "
    "00 01 00 08 00 00 00 00"
  };

  TAILQ_INIT(&s_flow_stats_list);
  if ((flow_stats = s_flow_stats_alloc()) != NULL) {
    /* flow_stats = 48, match = 16, instruction = 8, sum = 72 */
    flow_stats->ofp.length = 48 + 16 + 8;
    flow_stats->ofp.table_id = 0x01;
    flow_stats->ofp.duration_sec = 0x02;
    flow_stats->ofp.duration_nsec = 0x03;
    flow_stats->ofp.priority = 0x04;
    flow_stats->ofp.idle_timeout = 0x05;
    flow_stats->ofp.hard_timeout = 0x06;
    flow_stats->ofp.flags = 0x07;
    flow_stats->ofp.cookie = 0x08;
    flow_stats->ofp.packet_count = 0x09;
    flow_stats->ofp.byte_count = 0x0a;
    if ((match = match_alloc(8)) != NULL) {
      match->oxm_class = 0x00;
      match->oxm_field = 0x01;
      match->oxm_length = 0x08;
      TAILQ_INSERT_TAIL(&(flow_stats->match_list), match, entry);
    }
    if ((instruction = instruction_alloc()) != NULL) {
      instruction->ofpit_goto_table.type = OFPIT_GOTO_TABLE;
      instruction->ofpit_goto_table.len = 0x08; /* action_list empty */
      instruction->ofpit_goto_table.table_id = 0x00;
      TAILQ_INSERT_TAIL(&(flow_stats->instruction_list), instruction, entry);
    }
    TAILQ_INSERT_TAIL(&s_flow_stats_list, flow_stats, entry);
  } else {
    TEST_FAIL_MESSAGE("allocation error.");
  }

  /* port 0 */
  ret = check_pbuf_list_packet_create(s_ofp_flow_reply_create_wrap, require, 1);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "create port 0 error.");

  /* free */
  while ((flow_stats = TAILQ_FIRST(&s_flow_stats_list)) != NULL) {
    TAILQ_REMOVE(&s_flow_stats_list, flow_stats, entry);
    ofp_match_list_elem_free(&flow_stats->match_list);
    ofp_instruction_list_elem_free(&flow_stats->instruction_list);
    free(flow_stats);
  }
}

void
test_ofp_flow_reply_create_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct flow_stats *flow_stats = NULL;
  struct match *match = NULL;
  const char *header_data[2] = {
    "04 13 ff d0 00 00 00 10 00 01 00 01 00 00 00 00 ",
    "04 13 00 50 00 00 00 10 00 01 00 00 00 00 00 00 "
  };
  const char *body_data[2] = {
    "00 40 01 00 00 00 00 02 00 00 00 03 00 04 00 05 "
    "00 06 00 07 00 00 00 00 00 00 00 00 00 00 00 08 "
    "00 00 00 00 00 00 00 09 00 00 00 00 00 00 00 0a "
    "00 01 00 10 00 00 01 08 00 00 00 00 00 00 00 00 ",
    "00 40 01 00 00 00 00 02 00 00 00 03 00 04 00 05 "
    "00 06 00 07 00 00 00 00 00 00 00 00 00 00 00 08 "
    "00 00 00 00 00 00 00 09 00 00 00 00 00 00 00 0a "
    "00 01 00 10 00 00 01 08 00 00 00 00 00 00 00 00 "
  };
  size_t nums[2] = {1023, 1};
  int i;

  /* data */
  TAILQ_INIT(&s_flow_stats_list);
  for (i = 0; i < 1024; i++) {
    if ((flow_stats = s_flow_stats_alloc()) != NULL) {
      /* flow_stats = 48, match = 16, sum = 64 */
      flow_stats->ofp.length = 0;
      flow_stats->ofp.table_id = 0x01;
      flow_stats->ofp.duration_sec = 0x02;
      flow_stats->ofp.duration_nsec = 0x03;
      flow_stats->ofp.priority = 0x04;
      flow_stats->ofp.idle_timeout = 0x05;
      flow_stats->ofp.hard_timeout = 0x06;
      flow_stats->ofp.flags = 0x07;
      flow_stats->ofp.cookie = 0x08;
      flow_stats->ofp.packet_count = 0x09;
      flow_stats->ofp.byte_count = 0x0a;
      if ((match = match_alloc(8)) != NULL) {
        match->oxm_class = 0x00;
        match->oxm_field = 0x01;
        match->oxm_length = 0x08;
        TAILQ_INSERT_TAIL(&(flow_stats->match_list), match, entry);
      }
      TAILQ_INSERT_TAIL(&s_flow_stats_list, flow_stats, entry);
    } else {
      TEST_FAIL_MESSAGE("allocation error.");
    }
  }

  ret = check_pbuf_list_across_packet_create(s_ofp_flow_reply_create_wrap,
                                             header_data, body_data, nums, 2);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "create port 0 error.");

  /* free */
  while ((flow_stats = TAILQ_FIRST(&s_flow_stats_list)) != NULL) {
    TAILQ_REMOVE(&s_flow_stats_list, flow_stats, entry);
    ofp_match_list_elem_free(&flow_stats->match_list);
    ofp_instruction_list_elem_free(&flow_stats->instruction_list);
    free(flow_stats);
  }
}

void
test_ofp_flow_reply_create_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf_list *pbuf_list = NULL;
  struct flow_stats_list flow_stats_list;
  struct ofp_header ofp_header;

  ret = ofp_flow_stats_reply_create(NULL, &pbuf_list,
                                    &flow_stats_list,
                                    &ofp_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_flow_stats_reply_create(channel, NULL,
                                    &flow_stats_list,
                                    &ofp_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_flow_stats_reply_create(channel, &pbuf_list,
                                    NULL, &ofp_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (flow_stats_list)");

  ret = ofp_flow_stats_reply_create(channel, &pbuf_list,
                                    &flow_stats_list, NULL);
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
