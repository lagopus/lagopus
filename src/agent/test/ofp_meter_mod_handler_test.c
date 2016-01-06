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
#include "../ofp_meter_mod_handler.h"
#include "handler_test_utils.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
}

static lagopus_result_t
ofp_meter_mod_handle_wrap(struct channel *channel, struct pbuf *pbuf,
                          struct ofp_header *xid_header,
                          struct ofp_error *error) {
  return ofp_meter_mod_handle(channel, pbuf, xid_header, error);
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
test_ofp_meter_mod_handle_add_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[2] = {
    /* add */
    "04 1d 00 30 00 00 00 10 00 00 00 02 00 00 00 64 "
    /* <---------------------------------------------... ofp_meter_mod
     * <---------------------> ofp_header
     * <> version
     *    <> type
     *       <---> length
     *             <---------> xid
     *                         <---> command
     *                               <---> flags
     *                                     <---------> meter_id
     */
    "00 01 00 10 00 00 00 0a 00 00 00 01 00 00 00 00"
    /*...-------------------------------------------... ofp_meter_mod
     * <---------------------------------------------> ofp_meter_band_drop
     * <---------------------------------> ofp_meter_band_header
     * <---> type = 1 -> OFPMBT_DROP
     *       <---> length = 16 [bytes]
     *             <---------> rate = 10
     *                         <---------> burst_size = 1
     *                                     <---------> padding
     */
    "00 02 00 10 00 00 00 0b 00 00 00 02 03 00 00 00"
    /*...--------------------------------------------> ofp_meter_mod
     * <---------------------------------------------> ofp_meter_band_remark
     * <---------------------------------> ofp_meter_band_header
     * <---> type = 2 -> OFPMBT_DSCP_REMARK
     *       <---> length = 16 [bytes]
     *             <---------> rate = 11
     *                         <---------> burst_size = 2
     *                                     <> prec_level = 3
     *                                        <------> padding
     */
  };
  ret = check_packet_parse_array(ofp_meter_mod_handle_wrap, data, 1);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_meter_mod_handle_add(normal) error.");
}

void
test_ofp_meter_mod_handle_add_invalid_length_too_large(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  const char *data[2] = {
    /* add */
    "04 1d 00 30 00 00 00 10 00 00 00 02 00 00 00 64 "
    /* <---------------------------------------------... ofp_meter_mod
     *       <---> length = 16 * 3, but is too large
     */
    "00 01 00 10 00 00 00 0a 00 00 00 01 00 00 00 00"
  };

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_array_expect_error(
          ofp_meter_mod_handle_wrap,
          data,
          1,
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid length.");
}


void
test_ofp_meter_mod_handle_add_invalid_band_length_too_long(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  const char *data[2] = {
    /* add */
    "04 1d 00 30 00 00 00 10 00 00 00 02 00 00 00 64" /* same as normal pattern */
    "00 01 00 20 00 00 00 0a 00 00 00 01 00 00 00 00"
    /*...-------------------------------------------... ofp_meter_mod
     * <---------------------------------> ofp_meter_band_header
     * <---> type = 1
     *       <---> length = 32 [bytes] is invalid, too long
     */
    "00 02 00 10 00 00 00 0b 00 00 00 02 03 00 00 00" /* same as normal pattern */
  };

  /* FIXME: error type and code may be wrong*/
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_array_expect_error(
          ofp_meter_mod_handle_wrap, data, 1, &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid lenght should cause an error.");
}

void
test_ofp_meter_mod_handle_add_invalid_unknown_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  const char *data[2] = {
    /* add */
    "04 1d 00 30 00 00 00 10 00 00 00 02 00 00 00 64 " /* same as normal pattern */
    "ff fe 00 10 00 00 00 0a 00 00 00 01 00 00 00 00"
    /*...-------------------------------------------... ofp_meter_mod
     * <---------------------------------> ofp_meter_band_header
     * <---> type = 0xfffe -> unknown type
     *       <---> length = 16 [bytes]
     *             <---------> rate = 10
     *                         <---------> burst_size = 1
     *                                     <---------> padding
     */
    "00 02 00 10 00 00 00 0b 00 00 00 02 03 00 00 00" /* same as normal pattern */
  };

  ofp_error_set(&expected_error, OFPET_METER_MOD_FAILED, OFPMMFC_BAD_BAND);

  ret = check_packet_parse_array_expect_error(
          ofp_meter_mod_handle_wrap, data, 1, &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "unknown command should cause an error.");
}

void
test_ofp_meter_mod_handle_add_invalid_unknown_flags(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  const char *data[2] = {
    /* add */
    "04 1d 00 30 00 00 00 10 00 00 80 00 00 00 00 64"
    /* <---------------------------------------------... ofp_meter_mod
     * <---------------------> ofp_header
     * <> version
     *    <> type
     *       <---> length
     *             <---------> xid
     *                         <---> command
     *                               <---> flags 1 << 7 is unknown flag
     *                                     <---------> meter_id
     */
    "00 01 00 10 00 00 00 0a 00 00 00 01 00 00 00 00" /* same as normal pattern */
    "00 02 00 10 00 00 00 0b 00 00 00 02 03 00 00 00" /* same as normal pattern */
  };

  ofp_error_set(&expected_error, OFPET_METER_MOD_FAILED, OFPMMFC_BAD_FLAGS);

  ret = check_packet_parse_array_expect_error(
          ofp_meter_mod_handle_wrap, data, 1, &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_meter_mod_handle_add(normal) error.");
}


void
test_ofp_meter_mod_handle_modify_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[2] = {
    /* add */
    "04 1d 00 20 00 00 00 10 00 00 00 02 00 00 00 64 "
    "00 01 00 10 00 00 00 0a 00 00 00 01 00 00 00 00",
    /* mod */
    "04 1d 00 20 00 00 00 10 00 01 00 02 00 00 00 64 "
    "00 01 00 10 00 00 00 0a 00 00 00 01 00 00 00 00"
  };
  ret = check_packet_parse_array(ofp_meter_mod_handle_wrap, data, 2);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_meter_mod_handle_modify(normal) error.");
}

void
test_ofp_meter_mod_handle_delete_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[2] = {
    /* add */
    "04 1d 00 20 00 00 00 10 00 00 00 02 00 00 00 64 "
    "00 01 00 10 00 00 00 0a 00 00 00 01 00 00 00 00",
    /* del */
    "04 1d 00 20 00 00 00 10 00 02 00 02 00 00 00 64 "
    "00 01 00 10 00 00 00 0a 00 00 00 01 00 00 00 00"
  };
  ret = check_packet_parse_array(ofp_meter_mod_handle_wrap, data, 2);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_meter_mod_handle_delete(normal) error.");
}

void
test_ofp_meter_mod_handle_add_normal_meter_id(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[2] = {
    /* add */
    "04 1d 00 30 00 00 00 10 00 00 00 02 ff ff ff ff "
    /* <---------------------------------------------... ofp_meter_mod
     * <---------------------> ofp_header
     * <> version
     *    <> type
     *       <---> length
     *             <---------> xid
     *                         <---> command
     *                               <---> flags
     *                                     <---------> meter_id
     */
    "00 01 00 10 00 00 00 0a 00 00 00 01 00 00 00 00"
    /*...-------------------------------------------... ofp_meter_mod
     * <---------------------------------------------> ofp_meter_band_drop
     * <---------------------------------> ofp_meter_band_header
     * <---> type = 1 -> OFPMBT_DROP
     *       <---> length = 16 [bytes]
     *             <---------> rate = 10
     *                         <---------> burst_size = 1
     *                                     <---------> padding
     */
    "00 02 00 10 00 00 00 0b 00 00 00 02 03 00 00 00"
    /*...--------------------------------------------> ofp_meter_mod
     * <---------------------------------------------> ofp_meter_band_remark
     * <---------------------------------> ofp_meter_band_header
     * <---> type = 2 -> OFPMBT_DSCP_REMARK
     *       <---> length = 16 [bytes]
     *             <---------> rate = 11
     *                         <---------> burst_size = 2
     *                                     <> prec_level = 3
     *                                        <------> padding
     */
  };
  ret = check_packet_parse_array(ofp_meter_mod_handle_wrap, data, 1);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_meter_mod_handle_add(meter_id) error.");
}

void
test_ofp_meter_mod_handle_add_invalid_meter_id(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};

  const char *data[2] = {
    /* add */
    "04 1d 00 30 00 00 00 10 00 00 00 02 ff ff 00 01 "
    /* <---------------------------------------------... ofp_meter_mod
     * <---------------------> ofp_header
     * <> version
     *    <> type
     *       <---> length
     *             <---------> xid
     *                         <---> command
     *                               <---> flags
     *                                     <---------> meter_id
     */
    "00 01 00 10 00 00 00 0a 00 00 00 01 00 00 00 00"
    "00 02 00 10 00 00 00 0b 00 00 00 02 03 00 00 00"
  };

  ofp_error_set(&expected_error, OFPET_METER_MOD_FAILED, OFPMMFC_INVALID_METER);

  ret = check_packet_parse_array_expect_error(
          ofp_meter_mod_handle_wrap, data, 1, &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_meter_mod_handle_add(invalid meter id) error.");
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
