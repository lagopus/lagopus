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
#include "../ofp_group_mod_handler.h"
#include "handler_test_utils.h"
#include "lagopus/pbuf.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
}

/* TODO: remove this */
static lagopus_result_t
ofp_group_mod_handle_wrap(struct channel *channel,
                          struct pbuf *pbuf,
                          struct ofp_header *xid_header,
                          struct ofp_error *error) {
  return ofp_group_mod_handle(channel, pbuf,
                              xid_header, error);
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
/*
 * add
 */
void
test_group_mod_handle_add_normal_pattern_no_buckets(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          ofp_group_mod_handle_wrap,
          "04 0f 00 10 00 00 00 10 00 00 00 00 ff ff ff 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "add no_buckets error.");
}

void
test_group_mod_handle_add_normal_pattern_no_actions(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          ofp_group_mod_handle_wrap,
          "04 0f 00 30 00 00 00 10 00 00 00 00 ff ff ff 00 "
          "00 10 00 02 00 00 00 03 00 00 00 04 00 00 00 00 "
          "00 10 00 06 00 00 00 07 00 00 00 08 00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "add no_actions error.");
}

void
test_group_mod_handle_add_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          ofp_group_mod_handle_wrap,
          "04 0f 00 80 00 00 00 10 00 00 00 00 ff ff ff 00 "
          /* <----------------------------------------------... ofp_group_mod
           * <---------------------> ofp_header
           * <> version
           *    <> type
           *       <---> length = 8 * 16 bytes
           *             <---------> xid
           *                         <---> command ( 0 -> OFPGC_ADD
           *                               <> type ( 0 -> OFPGT_ALL
           *                                  <> padding
           *                                     <---------> group id = 0xffffff00
           */
          "00 30 00 02 00 00 00 03 00 00 00 04 00 00 00 00 "
          /* <----------------------------------------------.. ofp_bucket[0]
           * <---> len = 3 * 16 bytes
           *       <---> weight = 2
           *             <---------> watch_port = 3
           *                         <---------> watch_group = 4
           *                                     <---------> padding
           */
          "00 00 00 10 00 00 00 0a 03 e8 00 00 00 00 00 00 "
          /* ...--------------------------------------------.. ofp_bucket[0]
           * <---------------------> ofp_action_output
           * <---> type = 0 ( 0 -> AFPAT_OUTPUT
           *       <---> length = 1 * 16 bytes
           *             <---------> port = 10
           *                         <---> max_len = 0x3e8
           *                               <---------------> padding
           */
          "00 00 00 10 00 00 00 14 07 d0 00 00 00 00 00 00 "
          /* ...------------------------------------------->   ofp_bucket[0]
           * <---------------------> ofp_action_output
           * <---> type = 0 ( 0 -> AFPAT_OUTPUT
           *       <---> length = 1 * 16 bytes
           *             <---------> port = 20
           *                         <---> max_len = 0x07d0
           *                               <---------------> padding
           */
          "00 30 00 06 00 00 00 07 00 00 00 08 00 00 00 00 "
          /* <----------------------------------------------.. ofp_bucket[1]
           * <---> len = 3 * 16 bytes
           *       <---> weight = 6
           *             <---------> watch_port = 7
           *                         <---------> watch_group = 8
           *                                     <---------> padding
           */
          "00 00 00 10 00 00 00 0a 03 e8 00 00 00 00 00 00 "
          /* ...-------------------------------------------... ofp_bucket[1]
           * <---------------------> ofp_action_output
           * <---> type = 0 ( 0 -> AFPAT_OUTPUT
           *       <---> length = 1 * 16 bytes
           *             <---------> port = 10
           *                         <---> max_len = 0x03e8
           *                               <---------------> padding
           */
          "00 00 00 10 00 00 00 14 07 d0 00 00 00 00 00 00 "
          /* ...------------------------------------------->   ofp_bucket[1]
           * <---------------------> ofp_action_output
           * <---> type = 0 ( 0 -> AFPAT_OUTPUT
           *       <---> length = 1 * 16 bytes
           *             <---------> port = 20
           *                         <---> max_len = 0x07d0
           *                               <---------------> padding
           */
          "00 10 00 02 00 00 00 03 00 00 00 04 00 00 00 00"
          /* <--------------------------------------------->   ofp_bucket[2]
           * <---> len = 1 * 16 bytes
           *       <---> weight = 2
           *             <---------> watch_port = 3
           *                         <---------> watch_group = 4
           *                                     <---------> padding
           */
        );
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "add error.");
}

void
test_group_mod_handle_add_invalid_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* only header, no data */
  ret = check_packet_parse_expect_error(
          ofp_group_mod_handle_wrap,
          "04 0f 00 08 00 00 00 10",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "no body error.");
}

void
test_group_mod_handle_add_invalid_length2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* only header, no data */
  /* no type, pad, group_id, buckets */
  ret = check_packet_parse_expect_error(
          ofp_group_mod_handle_wrap,
          "04 0f 00 0a 00 00 00 10 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid body error.");
}

void
test_group_mod_handle_add_invalid_length3(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* no pad, group_id, buckets */
  ret = check_packet_parse_expect_error(
          ofp_group_mod_handle_wrap,
          "04 0f 00 0b 00 00 00 10 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid body error.");
}

void
test_group_mod_handle_add_invalid_length4(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* no group_id, buckets */
  ret = check_packet_parse_expect_error(
          ofp_group_mod_handle_wrap,
          "04 0f 00 0c 00 00 00 10 00 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid body error.");
}


void
test_group_mod_handle_add_invalid_command(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_GROUP_MOD_FAILED, OFPGMFC_BAD_COMMAND);
  ret = check_packet_parse_expect_error(
          ofp_group_mod_handle_wrap,
          "04 0f 00 80 00 00 00 10 ff fe 00 00 ff ff ff 00 "
          /* <----------------------------------------------... ofp_group_mod
           * <---------------------> ofp_header
           * <> version
           *    <> type
           *       <---> length = 8 * 16 bytes
           *             <---------> xid
           *                         <---> command ( 0xfffe -> unknown group command
           *                               <> type ( 0 -> OFPGT_ALL
           *                                  <> padding
           *                                     <---------> group id = 0xffffff00
           */
          "00 30 00 02 00 00 00 03 00 00 00 04 00 00 00 00 "
          "00 00 00 10 00 00 00 0a 03 e8 00 00 00 00 00 00 "
          "00 00 00 10 00 00 00 14 07 d0 00 00 00 00 00 00 "
          "00 30 00 06 00 00 00 07 00 00 00 08 00 00 00 00 "
          "00 00 00 10 00 00 00 0a 03 e8 00 00 00 00 00 00 "
          "00 00 00 10 00 00 00 14 07 d0 00 00 00 00 00 00 "
          "00 10 00 02 00 00 00 03 00 00 00 04 00 00 00 00",
          &expected_error
        );
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "unknow group command should cause error");
}

void
test_group_mod_handle_add_invalid_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_GROUP_MOD_FAILED, OFPGMFC_BAD_TYPE);
  ret = check_packet_parse_expect_error(
          ofp_group_mod_handle_wrap,
          "04 0f 00 80 00 00 00 10 00 00 fe 00 ff ff ff 00 "
          /* <----------------------------------------------... ofp_group_mod
           * <---------------------> ofp_header
           * <> version
           *    <> type
           *       <---> length = 8 * 16 bytes
           *             <---------> xid
           *                         <---> command ( 0 -> OFPGC_ADD
           *                               <> type ( fe -> unknown type
           *                                  <> padding
           *                                     <---------> group id = 0xffffff00
           */
          "00 30 00 02 00 00 00 03 00 00 00 04 00 00 00 00 "
          "00 00 00 10 00 00 00 0a 03 e8 00 00 00 00 00 00 "
          "00 00 00 10 00 00 00 14 07 d0 00 00 00 00 00 00 "
          "00 30 00 06 00 00 00 07 00 00 00 08 00 00 00 00 "
          "00 00 00 10 00 00 00 0a 03 e8 00 00 00 00 00 00 "
          "00 00 00 10 00 00 00 14 07 d0 00 00 00 00 00 00 "
          "00 10 00 02 00 00 00 03 00 00 00 04 00 00 00 00",
          &expected_error
        );
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "unknown type shoud case error");
}


/*
 * modify
 */
void
test_group_mod_handle_modify_no_buckets(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[2] = {
    /* add */
    "04 0f 00 10 00 00 00 10 00 00 00 00 ff ff ff 00",
    /* mod */
    "04 0f 00 10 00 00 00 10 00 01 00 00 ff ff ff 00"
  };
  ret = check_packet_parse_array(ofp_group_mod_handle_wrap, data, 2);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "modify no_buckets error.");
}



void
test_group_mod_handle_modify_no_actions(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[2] = {
    /* add */
    "04 0f 00 10 00 00 00 10 00 00 00 00 ff ff ff 00",
    /* mod */
    "04 0f 00 30 00 00 00 10 00 01 00 00 ff ff ff 00 "
    "00 10 00 02 00 00 00 03 00 00 00 04 00 00 00 00 "
    "00 10 00 06 00 00 00 07 00 00 00 08 00 00 00 00"
  };
  ret = check_packet_parse_array(ofp_group_mod_handle_wrap, data, 2);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "modify no_actions error.");
}

void
test_group_mod_handle_modify(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[2] = {
    /* add */
    "04 0f 00 10 00 00 00 10 00 00 00 00 ff ff ff 00",
    /* mod */
    "04 0f 00 80 00 00 00 10 00 01 00 00 ff ff ff 00 "
    "00 30 00 02 00 00 00 03 00 00 00 04 00 00 00 00 "
    "00 00 00 10 00 00 00 0a 03 e8 00 00 00 00 00 00 "
    "00 00 00 10 00 00 00 14 07 d0 00 00 00 00 00 00 "
    "00 30 00 06 00 00 00 07 00 00 00 08 00 00 00 00 "
    "00 00 00 10 00 00 00 0a 03 e8 00 00 00 00 00 00 "
    "00 00 00 10 00 00 00 14 07 d0 00 00 00 00 00 00 "
    "00 10 00 02 00 00 00 03 00 00 00 04 00 00 00 00"
  };
  ret = check_packet_parse_array(ofp_group_mod_handle_wrap, data, 2);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "modify error.");
}

void
test_group_mod_handle_modify_error(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  /* only header, no data */
  ret = check_packet_parse(
          ofp_group_mod_handle_wrap,
          "04 0f 00 08 00 00 00 10");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "no body error.");
  /* no type, pad, group_id, buckets */
  ret = check_packet_parse(
          ofp_group_mod_handle_wrap,
          "04 0f 00 0a 00 00 00 10 00 01");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid body error.");
  /* no pad, group_id, buckets */
  ret = check_packet_parse(
          ofp_group_mod_handle_wrap,
          "04 0f 00 0b 00 00 00 10 00 01 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid body error.");
  /* no group_id, buckets */
  ret = check_packet_parse(
          ofp_group_mod_handle_wrap,
          "04 0f 00 0c 00 00 00 10 00 01 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid body error.");
}


/*
 * delete
 */
void
test_group_mod_handle_delete(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          ofp_group_mod_handle_wrap,
          "04 0f 00 10 00 00 00 10 00 02 00 00 ff ff ff 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "delete no_buckets error.");
}

void
test_group_mod_handle_delete_with_buckets(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          ofp_group_mod_handle_wrap,
          "04 0f 00 30 00 00 00 10 00 02 00 00 ff ff ff 00 "
          "00 10 00 02 00 00 00 03 00 00 00 04 00 00 00 00 "
          "00 10 00 06 00 00 00 07 00 00 00 08 00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "delete no_actions error.");
}

void
test_group_mod_handle_delete_with_actions(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(ofp_group_mod_handle_wrap,
                           "04 0f 00 80 00 00 00 10 00 02 00 00 ff ff ff 00 "
                           "00 30 00 02 00 00 00 03 00 00 00 04 00 00 00 00 "
                           "00 00 00 10 00 00 00 0a 03 e8 00 00 00 00 00 00 "
                           "00 00 00 10 00 00 00 14 07 d0 00 00 00 00 00 00 "
                           "00 30 00 06 00 00 00 07 00 00 00 08 00 00 00 00 "
                           "00 00 00 10 00 00 00 0a 03 e8 00 00 00 00 00 00 "
                           "00 00 00 10 00 00 00 14 07 d0 00 00 00 00 00 00 "
                           "00 10 00 02 00 00 00 03 00 00 00 04 00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "delete error.");
}

void
test_group_mod_handle_delete_error(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  /* only header, no data */
  ret = check_packet_parse(
          ofp_group_mod_handle_wrap,
          "04 0f 00 08 00 00 00 10");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "no body error.");
  /* no type, pad, group_id, buckets */
  ret = check_packet_parse(
          ofp_group_mod_handle_wrap,
          "04 0f 00 0a 00 00 00 10 00 02");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid body error.");
  /* no pad, group_id, buckets */
  ret = check_packet_parse(
          ofp_group_mod_handle_wrap,
          "04 0f 00 0b 00 00 00 10 00 02 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid body error.");
  /* no group_id, buckets */
  ret = check_packet_parse(
          ofp_group_mod_handle_wrap,
          "04 0f 00 0c 00 00 00 10 00 02 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid body error.");
}


/*
 * common
 */
void
test_group_mod_handle_invlaid_command(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          ofp_group_mod_handle_wrap,
          "04 0f 00 10 00 00 00 10 ff ff 00 00 00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid command error.");
}

void
test_group_mod_handle_invalid_buckets(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  /* no weight, watch_port, watch_group, pad, actions */
  ret = check_packet_parse(ofp_group_mod_handle_wrap,
                           "04 0f 00 1c 00 00 00 10 00 01 00 00 ff ff ff 00 "
                           "00 02");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "parse buckets error.");
  /* no watch_port, watch_group, pad, actions */
  ret = check_packet_parse(ofp_group_mod_handle_wrap,
                           "04 0f 00 1c 00 00 00 10 00 01 00 00 ff ff ff 00 "
                           "00 04 00 02");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "parse buckets error.");
  /* no watch_group, pad, actions */
  ret = check_packet_parse(ofp_group_mod_handle_wrap,
                           "04 0f 00 18 00 00 00 10 00 01 00 00 ff ff ff 00 "
                           "00 08 00 02 00 00 00 03");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "parse buckets error.");
  /* no pad, actions */
  ret = check_packet_parse(ofp_group_mod_handle_wrap,
                           "04 0f 00 1c 00 00 00 10 00 01 00 00 ff ff ff 00 "
                           "00 0c 00 02 00 00 00 03 00 00 00 04");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "parse buckets error.");
}


void
test_ofp_group_mod_handle_wrap_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header xid_header;
  struct ofp_error error;

  ret = ofp_group_mod_handle(NULL, pbuf, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_group_mod_handle(channel, NULL, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_group_mod_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (xid_header)");

  ret = ofp_group_mod_handle(channel, pbuf, &xid_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (xid_header)");

  channel_free(channel);
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
