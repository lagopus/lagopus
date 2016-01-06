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
#include "../ofp_flow_mod_handler.h"
#include "handler_test_utils.h"
#include "../channel_mgr.h"

void
setUp(void) {
}

void
tearDown(void) {
}

/* TODO remove */
static lagopus_result_t
ofp_flow_mod_handle_wrap(struct channel *channel,
                         struct pbuf *pbuf,
                         struct ofp_header *xid_header,
                         struct ofp_error *error) {

  (void) error;
  return ofp_flow_mod_handle(channel, pbuf, xid_header, error);
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
test_flow_mod_handle_add_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          ofp_flow_mod_handle_wrap,
          "04 0e 00 60 00 00 00 10 00 00 00 00 00 00 00 00"
          /*<-     ofp_header    --> <--     cookie      -->*/
          "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 64"
          /*<-     cookie mask   -->
           *                         <> table id
           *                            <> command
           *                               <---> idle timeout
           *                                     <---> hard timeout
           *                                           <---> priority
           */
          "00 00 ff ff ff ff ff ff ff ff ff ff 00 00 00 00"
          /* <-       -> buffer id
           *             <-       -> out port
           *                          <-       -> out group
           *                                     <- -> flags
           *                                           <-  -> padding[2]
           */
          "00 01 00 16 80 00 00 04 00 00 00 01 80 00 08 06"
          /* <-     ofp_match                            --> (cont
           * <- -> type
           *       <- -> length (exclude padding) = 22 (+ 2 as padding)
           *             <           oxm fields          --> (cont
           *             <---------------------> oxm tlv [0]
           *             <---------> oxm tlv header
           *              oxm_class=OFPXMC_OPENFLOW_BASIC
           *              oxm_field=0 -> OFPXMT_OFB_IN_PORT
           *              oxm_hasmask=0
           *              oxm_length=4 -> sizeof(this field) = 4 + 4 = 8
           *                         <---------> value = 1
           *                                     <---------> oxm tlv [1]
           *                                      oxm_class=OFPXMC_OPENFLOW_BASIC
           *                                      oxm_field=4 -> OFPXMT_OFB_ETH_SRC (4 = 8 >> 1)
           *                                      oxm_hasmask=0
           *                                      oxm_length=4 -> sizeof(this field) = 4 + 6 = 10
           */
          "00 0c 29 7a 90 b3 00 00 00 04 00 18 00 00 00 00"
          /* <-  ofp_match  ->
           * <-  oxm field  ->
           * value = 00:0c:29:71:90:b3 (MAC Address)
           *                   <- -> padding
           *                         <-       -> ofp_instruction
           *                         <-                   -> ofp_instruction_actions (cont
           *                         <- -> type (4 -> OFPIT_APPLY_ACTIONS)
           *                               <- -> length
           *                                     <-       -> padding
           */
          "00 00 00 10 00 00 00 00 00 00 00 00 00 00 00 00");
  /* <-                                           -> ofp_instruction_actions
   * <-                                           -> ofp_action_output
   * <- -> type (0 -> OFPAT_OUTPUT)
   *       <- -> length
   *             <-       -> port
   *                         <- -> max len = 0 (no output)
   *                               <-             -> padding
   */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_flow_mod_handle(normal) error.");
}

void
test_flow_mod_handle_add_any_match(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          ofp_flow_mod_handle_wrap,
          "04 0e 00 50 00 00 00 10 00 00 00 00 00 00 00 00"
          /*<-     ofp_header    --> <--     cookie      -->*/
          "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 64"
          /*<-     cookie mask   -->
           *                         <> table id
           *                            <> command
           *                               <---> idle timeout
           *                                     <---> hard timeout
           *                                           <---> priority
           */
          "00 00 ff ff ff ff ff ff ff ff ff ff 00 00 00 00"
          /* <-       -> buffer id
           *             <-       -> out port
           *                          <-       -> out group
           *                                     <- -> flags
           *                                           <-  -> padding[2]
           */
          "00 01 00 04 00 00 00 00 00 04 00 18 00 00 00 00"
          /* <-     ofp_match    -->
           * <- -> type
           *       <- -> length
           *             <--------> pad
           *
           *                         <-       -> ofp_instruction
           *                         <-                   -> ofp_instruction_actions (cont
           *                         <- -> type (4 -> OFPIT_APPLY_ACTIONS)
           *                               <- -> length
           *                                     <-       -> padding
           */
          "00 00 00 10 00 00 00 00 00 00 00 00 00 00 00 00");
  /* <-                                           -> ofp_instruction_actions
   * <-                                           -> ofp_action_output
   * <- -> type (0 -> OFPAT_OUTPUT)
   *       <- -> length
   *             <-       -> port
   *                         <- -> max len = 0 (no output)
   *                               <-             -> padding
   */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_flow_mod_handle(normal) error.");
}

void
test_flow_mod_handle_add_invalid_too_short_packet(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_flow_mod_handle_wrap,
          "04 0e 00 60 00 00 00 10 00 00 00 00 00 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_flow_mod_handle(error) error.");
}


/*
 * - instruction test in flow mod (add)
 * -- OFPBIC_UNKNOWN_INST --> testing
 * -- OFPBIC_UNSUP_INST --> testing
 * -- OFPBIC_BAD_TABLE_ID --> depending on DP
 * -- OFPBIC_UNSUP_METADATA --> depending on DP
 * -- OFPBIC_UNSUP_METADATA_MASK --> depending on DP
 * -- OFPBIC_BAD_EXPERIMENTER --> not supported (never occur)
 * -- OFPBIC_BAD_EXP_TYPE --> not supported (never occur)
 * -- OFPBIC_BAD_LEN --> testing
 * -- OFPBIC_EPERM --> depend on DP
 */

void
test_flow_mod_handle_add_invalid_instruction_unknown_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_INSTRUCTION, OFPBIC_UNKNOWN_INST);
  ret = check_packet_parse_expect_error(
          ofp_flow_mod_handle_wrap,
          "04 0e 00 60 00 00 00 10 00 00 00 00 00 00 00 00" /* same as normal pattern */
          "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 64" /* same as normal pattern */
          "00 00 ff ff ff ff ff ff ff ff ff ff 00 00 00 00" /* same as normal pattern */
          "00 01 00 16 80 00 00 04 00 00 00 01 80 00 08 06" /* same as normal pattern */
          "00 0c 29 7a 90 b3 00 00 ff fe 00 18 00 00 00 00"
          /*
           *                         <-       -> ofp_instruction
           *                         <-                   -> ofp_instruction_actions (cont
           *                         <- -> type (0xfffe -> invalid instruction)
           *                               <- -> length
           *                                     <-       -> padding
           */
          "00 00 00 10 00 00 00 00 00 00 00 00 00 00 00 00",
          /* <-                                           -> ofp_instruction_actions
           * <-                                           -> ofp_action_output
           * <- -> type (0 -> OFPAT_OUTPUT)
           *       <- -> length
           *             <-       -> port
           *                         <- -> max len = 0 (no output)
           *                               <-             -> padding
           */
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_flow_mod_handle(normal) error.");
}

void
test_flow_mod_handle_add_invalid_instruction_experimenter_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_INSTRUCTION, OFPBIC_UNSUP_INST);

  ret = check_packet_parse_expect_error(
          ofp_flow_mod_handle_wrap,
          "04 0e 00 60 00 00 00 10 00 00 00 00 00 00 00 00" /* same as normal pattern */
          "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 64" /* same as normal pattern */
          "00 00 ff ff ff ff ff ff ff ff ff ff 00 00 00 00" /* same as normal pattern */
          "00 01 00 16 80 00 00 04 00 00 00 01 80 00 08 06" /* same as normal pattern */
          "00 0c 29 7a 90 b3 00 00 ff ff 00 18 00 00 00 00"
          /*
           *                         <-       -> ofp_instruction
           *                         <-                   -> ofp_instruction_actions (cont
           *                         <- -> type (0xffff -> OFPIT_EXPERIMENTER, is not supported)
           *                               <- -> length
           *                                     <-       -> padding
           */
          "00 00 00 10 00 00 00 00 00 00 00 00 00 00 00 00",
          /* <-                                           -> ofp_instruction_actions
           * <-                                           -> ofp_action_output
           * <- -> type (0 -> OFPAT_OUTPUT)
           *       <- -> length
           *             <-       -> port
           *                         <- -> max len = 0 (no output)
           *                               <-             -> padding
           */
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_flow_mod_handle(normal) error.");
}

void
test_flow_mod_handle_add_invalid_instruction_too_long_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_INSTRUCTION, OFPBIC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_flow_mod_handle_wrap,
          "04 0e 00 60 00 00 00 10 00 00 00 00 00 00 00 00"
          "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 64"
          "00 00 ff ff ff ff ff ff ff ff ff ff 00 00 00 00"
          "00 01 00 16 80 00 00 04 00 00 00 01 80 00 08 06"
          "00 0c 29 7a 90 b3 00 00 00 04 ff ff 00 00 00 00"
          /*
           *                         <-       -> ofp_instruction
           *                         <-                   -> ofp_instruction_actions (cont
           *                         <- -> type (4 -> OFPIT_APPLY_ACTIONS)
           *                               <- -> length (ffff is too long)
           *                                     <-       -> padding
           */
          "00 00 00 10 00 00 00 00 00 00 00 00 00 00 00 00",
          /* <-                                           -> ofp_instruction_actions
           * <-                                           -> ofp_action_output
           * <- -> type (0 -> OFPAT_OUTPUT)
           *       <- -> length
           *             <-       -> port
           *                         <- -> max len = 0 (no output)
           *                               <-             -> padding
           */
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_flow_mod_handle(normal) error.");
}

void
test_flow_mod_handle_add_invalid_action_unknown_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_ACTION, OFPBAC_BAD_TYPE);
  ret = check_packet_parse_expect_error(
          ofp_flow_mod_handle_wrap,
          "04 0e 00 60 00 00 00 10 00 00 00 00 00 00 00 00" /* same as normal pattern */
          "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 64" /* same as normal pattern */
          "00 00 ff ff ff ff ff ff ff ff ff ff 00 00 00 00" /* same as normal pattern */
          "00 01 00 16 80 00 00 04 00 00 00 01 80 00 08 06" /* same as normal pattern */
          "00 0c 29 7a 90 b3 00 00 00 04 00 18 00 00 00 00" /* same as normal pattern */
          "ff fe 00 10 00 00 00 00 00 00 00 00 00 00 00 00",
          /* <-                                           -> ofp_instruction_actions
           * <-                                           -> ofp_action_output
           * <- -> type (fffe -> unknown type)
           *       <- -> length
           *             <-       -> port
           *                         <- -> max len = 0 (no output)
           *                               <-             -> padding
           */
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_flow_mod_handle(normal) error.");
}

void
test_flow_mod_handle_add_invalid_action_too_long_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
  ret = check_packet_parse_expect_error(
          ofp_flow_mod_handle_wrap,
          "04 0e 00 60 00 00 00 10 00 00 00 00 00 00 00 00" /* same as normal pattern */
          "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 64" /* same as normal pattern */
          "00 00 ff ff ff ff ff ff ff ff ff ff 00 00 00 00" /* same as normal pattern */
          "00 01 00 16 80 00 00 04 00 00 00 01 80 00 08 06" /* same as normal pattern */
          "00 0c 29 7a 90 b3 00 00 00 04 00 18 00 00 00 00" /* same as normal pattern */
          "00 00 00 20 00 00 00 00 00 00 00 00 00 00 00 00",
          /* <-                                           -> ofp_instruction_actions
           * <-                                           -> ofp_action_output
           * <- -> type (0 -> OFPAT_OUTPUT)
           *       <- -> length
           *            *WRONG LENGTH*: length of ofp_action_output is 16, not 32
           *             <-       -> port
           *                         <- -> max len = 0 (no output)
           *                               <-             -> padding
           */
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_flow_mod_handle(normal) error.");
}

void
test_flow_mod_handle_add_experimenter_action_is_not_supported(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_EXPERIMENTER);
  ret = check_packet_parse_expect_error(
          ofp_flow_mod_handle_wrap,
          "04 0e 00 58 00 00 00 10 00 00 00 00 00 00 00 00"
          "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 64"
          "00 00 ff ff ff ff ff ff ff ff ff ff 00 00 00 00"
          "00 01 00 16 80 00 00 04 00 00 00 01 80 00 08 06"
          "00 0c 29 7a 90 b3 00 00 00 04 00 10 00 00 00 00"
          /* <-  ofp_match  ->
           *                         <-       -> ofp_instruction
           *                         <-                   -> ofp_instruction_actions (cont
           *                         <- -> type (4 -> OFPIT_APPLY_ACTIONS)
           *                               <- -> length
           *                                     <-       -> padding
           */
          "ff ff 00 08 00 00 00 00",
          /* <-                                           -> ofp_instruction_actions
           * <-                                           -> experimenter (not supported)
           * <- -> type (0xffff -> OFPAT_EXPERIMENTER)
           *       <- -> length = 8
           *             <-       -> experimenter = 0
           */
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_flow_mod_handle(normal) error.");
}

void
test_flow_mod_handle_add_bad_command(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  ofp_error_set(&expected_error, OFPET_FLOW_MOD_FAILED, OFPFMFC_BAD_COMMAND);
  ret = check_packet_parse_expect_error(
          ofp_flow_mod_handle_wrap,
          "04 0e 00 60 00 00 00 10 00 00 00 00 00 00 00 00"
          "00 00 00 00 00 00 00 00 00 ff 00 00 00 00 00 64"
          /*<-     cookie mask   -->
           *                         <> table id
           *                            <> command : ff (bad command)
           *                               <---> idle timeout
           *                                     <---> hard timeout
           *                                           <---> priority
           */
          "00 00 ff ff ff ff ff ff ff ff ff ff 00 00 00 00"
          "00 01 00 16 80 00 00 04 00 00 00 01 80 00 08 06"
          "00 0c 29 7a 90 b3 00 00 00 04 00 18 00 00 00 00"
          "00 00 00 10 00 00 00 00 00 00 00 00 00 00 00 00",
          &expected_error);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_flow_mod_handle(bad command) error.");
}

void
test_flow_mod_handle_add_bad_flags(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  ofp_error_set(&expected_error, OFPET_FLOW_MOD_FAILED, OFPFMFC_BAD_FLAGS);
  ret = check_packet_parse_expect_error(
          ofp_flow_mod_handle_wrap,
          "04 0e 00 60 00 00 00 10 00 00 00 00 00 00 00 00"
          "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 64"
          "00 00 ff ff ff ff ff ff ff ff ff ff ff ff 00 00"
          /* <-       -> buffer id
           *             <-       -> out port
           *                          <-       -> out group
           *                                     <- -> flags : ff ff (bad flags)
           *                                           <-  -> padding[2]
           */
          "00 01 00 16 80 00 00 04 00 00 00 01 80 00 08 06"
          "00 0c 29 7a 90 b3 00 00 00 04 00 18 00 00 00 00"
          "00 00 00 10 00 00 00 00 00 00 00 00 00 00 00 00",
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_flow_mod_handle(bad flags) error.");
}

/*
 * NOTE: The behavior for `OFPAT_EXPERIMENTER` is not supported,
 *       so there is not tests with the error code
 *       `OFPBAC_BAD_EXPERIMENTER` and `OFPBAC_BAD_EXP_TYPE`.
 */


/*
 * - skip the test for following error code. These depends on DP
 *
 * -- OFPBAC_BAD_OUT_PORT
 * -- OFPBAC_BAD_ARGUMENT
 * -- OFPBAC_EPERM
 * -- OFPBAC_TOO_MANY
 * -- OFPBAC_BAD_QUEUE
 * -- OFPBAC_BAD_OUT_GROUP
 * -- OFPBAC_MATCH_INCONSISTENT
 * -- OFPBAC_UNSUPPORTED_ORDER
 * -- OFPBAC_BAD_TAG
 * -- OFPBAC_BAD_SET_TYPE
 * -- OFPBAC_BAD_SET_LEN
 * -- OFPBAC_BAD_SET_ARGUMENT
 */

void
test_flow_mod_handle_modify(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          ofp_flow_mod_handle_wrap,
          "04 0e 00 60 00 00 00 10 00 00 00 00 00 00 00 00"
          "00 00 00 00 00 00 00 00 00 01 00 00 00 00 00 64"
          "00 00 ff ff ff ff ff ff ff ff ff ff 00 00 00 00"
          "00 01 00 16 80 00 00 04 00 00 00 01 80 00 08 06"
          "00 0c 29 7a 90 b3 00 00 00 04 00 18 00 00 00 00"
          "00 00 00 10 00 00 00 00 00 00 00 00 00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_flow_mod_handle(normal) error.");
}

void
test_flow_mod_handle_delete(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(
          ofp_flow_mod_handle_wrap,
          "04 0e 00 60 00 00 00 10 00 00 00 00 00 00 00 00"
          "00 00 00 00 00 00 00 00 00 03 00 00 00 00 00 64"
          "00 00 ff ff ff ff ff ff ff ff ff ff 00 00 00 00"
          "00 01 00 16 80 00 00 04 00 00 00 01 80 00 08 06"
          "00 0c 29 7a 90 b3 00 00 00 04 00 18 00 00 00 00"
          "00 00 00 10 00 00 00 00 00 00 00 00 00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_flow_mod_handle(normal) error.");
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
