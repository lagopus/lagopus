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
#include "lagopus/flowdb.h"
#include "openflow13.h"
#include "../ofp_action.h"
#include "../ofp_instruction.h"
#include "../ofp_tlv.h"
#include "handler_test_utils.h"

#define N_CALLOUT_WORKERS	1

void
test_prologue(void) {
  lagopus_result_t r;
  const char *argv0 =
      ((IS_VALID_STRING(lagopus_get_command_name()) == true) ?
       lagopus_get_command_name() : "callout_test");
  const char * const argv[] = {
    argv0, NULL
  };

  (void)lagopus_mainloop_set_callout_workers_number(N_CALLOUT_WORKERS);
  r = lagopus_mainloop_with_callout(1, argv, NULL, NULL,
                                    false, false, true);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  (void) global_state_wait_for(GLOBAL_STATE_STARTED, NULL, NULL, 1000LL * 1000LL * 100LL);
}

void
setUp(void) {
}

void
tearDown(void) {
}

#define OUT_LEN 0x10
#define GROUP_LEN 0x08
#define SET_FIELD_LEN 0x10
#define WRITE_ACT_LEN OUT_LEN + GROUP_LEN + SET_FIELD_LEN + 0x08
#define INS_LEN 0x04

static void
create_data(struct instruction_list *instruction_list) {
  struct instruction *instruction;
  struct action *action;
  struct ofp_action_output *act_output;
  struct ofp_action_group *act_group;
  struct ofp_action_set_field *act_set_field;
  uint8_t field[4] = {0x80, 0x00, 0x08, 0x06};
  uint8_t val[6] = {0x00, 0x0c, 0x29, 0x7a, 0x90, 0xb3};

  TAILQ_INIT(instruction_list);

  instruction = instruction_alloc();
  TAILQ_INSERT_TAIL(instruction_list, instruction, entry);
  instruction->ofpit_actions.type = OFPIT_WRITE_ACTIONS;
  instruction->ofpit_actions.len = WRITE_ACT_LEN;

  /* OUTPUT */
  action = action_alloc(OUT_LEN);
  TAILQ_INSERT_TAIL(&instruction->action_list, action, entry);
  act_output = (struct ofp_action_output *)&action->ofpat;
  act_output->type = OFPAT_OUTPUT;
  act_output->len = OUT_LEN;
  act_output->port = 0x100;
  act_output->max_len = 0x20;

  /* GROUP */
  action = action_alloc(GROUP_LEN);
  TAILQ_INSERT_TAIL(&instruction->action_list, action, entry);
  act_group = (struct ofp_action_group *)&action->ofpat;
  act_group->type = OFPAT_GROUP;
  act_group->len = GROUP_LEN;
  act_group->group_id = 0x03;

  /* SET_FIELD */
  action = action_alloc(SET_FIELD_LEN);
  TAILQ_INSERT_TAIL(&instruction->action_list, action, entry);
  act_set_field = (struct ofp_action_set_field *)&action->ofpat;
  act_set_field->type = OFPAT_SET_FIELD;
  act_set_field->len = SET_FIELD_LEN;
  memcpy(act_set_field->field, field, 4);
  memcpy(act_set_field->field + 4, val, 6);
}

static void
create_header_data(struct instruction_list *instruction_list) {
  struct instruction *instruction;

  TAILQ_INIT(instruction_list);

  /* WRITE_ACTIONS */
  instruction = instruction_alloc();
  TAILQ_INSERT_TAIL(instruction_list, instruction, entry);
  instruction->ofpit.type = OFPIT_WRITE_ACTIONS;
  instruction->ofpit.len = INS_LEN;

  /* GOTO_TABLE */
  instruction = instruction_alloc();
  TAILQ_INSERT_TAIL(instruction_list, instruction, entry);
  instruction->ofpit.type = OFPIT_GOTO_TABLE;
  instruction->ofpit.len = INS_LEN;
}

void
test_instruction_alloc(void) {
  struct instruction *instruction = NULL;

  instruction = instruction_alloc();

  TEST_ASSERT_NOT_NULL(instruction);
  if (instruction != NULL) {    /* memo: for scan-build warning */
    TEST_ASSERT_EQUAL_MESSAGE(true, TAILQ_EMPTY(&(instruction->action_list)),
                              "action_list not empty error.");
  }

  free(instruction);
}

void
test_ofp_instruction_parse(void) {
  struct pbuf *test_pbuf;
  lagopus_result_t ret;
  struct instruction_list instruction_list;
  struct instruction *instruction;
  struct action *action;
  uint16_t ins_type = OFPIT_WRITE_ACTIONS;
  uint16_t ins_len = 0x18;
  struct ofp_action_output *act_out;
  uint16_t act_type = OFPAT_OUTPUT;
  uint16_t act_len = 0x10;
  uint32_t act_port = 0x100;
  uint32_t act_max_len = 0x30;
  struct ofp_error error;
  struct ofp_error expected_error = {0, 0, {NULL}};

  /* instruction packet. */
  char normal_data[] = "00 03 00 18 00 00 00 00"
                       "00 00 00 10 00 00 01 00 00 30 00 00 00 00 00 00";
  char error_data[] = "00 03 00 18 00";

  TAILQ_INIT(&instruction_list);

  /* normal */
  create_packet(normal_data, &test_pbuf);

  ret = ofp_instruction_parse(test_pbuf, &instruction_list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "ofp_instruction_parse(normal) error.");



  TAILQ_FOREACH(instruction, &instruction_list, entry) {
    TEST_ASSERT_EQUAL_MESSAGE(instruction->ofpit_actions.type,
                              ins_type, "type error.");
    TEST_ASSERT_EQUAL_MESSAGE(instruction->ofpit_actions.len,
                              ins_len, "len error.");

    TEST_ASSERT_EQUAL_MESSAGE(TAILQ_EMPTY(&instruction->action_list),
                              false, "not action_list error.");

    TAILQ_FOREACH(action, &instruction->action_list, entry) {
      act_out = (struct ofp_action_output *) &action->ofpat;
      TEST_ASSERT_EQUAL_MESSAGE(act_out->type,
                                act_type, "action type error.");
      TEST_ASSERT_EQUAL_MESSAGE(act_out->len,
                                act_len, "action len error.");
      TEST_ASSERT_EQUAL_MESSAGE(act_out->port,
                                act_port, "action port error.");
      TEST_ASSERT_EQUAL_MESSAGE(act_out->max_len,
                                act_max_len, "action max len error.");
    }
  }

  pbuf_free(test_pbuf);
  ofp_instruction_list_elem_free(&instruction_list);

  /* error */
  TAILQ_INIT(&instruction_list);
  create_packet(error_data, &test_pbuf);

  ofp_error_set(&expected_error, OFPET_BAD_INSTRUCTION, OFPBIC_BAD_LEN);
  ret = check_instruction_parse_expect_error(ofp_instruction_parse,
        test_pbuf,
        &instruction_list,
        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_instruction_parse(error) error.");
  free(instruction);
  pbuf_free(test_pbuf);
}

void
test_ofp_instruction_parse_clear_actions(void) {
  struct pbuf *test_pbuf;
  lagopus_result_t ret;
  struct instruction_list instruction_list;
  struct instruction *instruction;
  uint16_t ins_type = OFPIT_CLEAR_ACTIONS;
  uint16_t ins_len = 0x08;
  struct ofp_error error;

  /* instruction packet. */
  char normal_data[] = "00 05 00 08 00 00 00 00";

  TAILQ_INIT(&instruction_list);

  /* normal */
  create_packet(normal_data, &test_pbuf);

  ret = ofp_instruction_parse(test_pbuf, &instruction_list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_instruction_parse(normal) error.");

  TAILQ_FOREACH(instruction, &instruction_list, entry) {
    TEST_ASSERT_EQUAL_MESSAGE(ins_type, instruction->ofpit_actions.type,
                              "type error.");
    TEST_ASSERT_EQUAL_MESSAGE(ins_len, instruction->ofpit_actions.len,
                              "len error.");

    TEST_ASSERT_EQUAL_MESSAGE(true, TAILQ_EMPTY(&instruction->action_list),
                              "not action_list error.");
  }

  ofp_instruction_list_elem_free(&instruction_list);
  free(instruction);
  pbuf_free(test_pbuf);
}

void
test_ofp_instruction_parse_clear_actions_with_actions(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *test_pbuf;
  struct instruction_list instruction_list;
  struct ofp_error expected_error = {0, 0, {NULL}};

  /* instruction packet. */
  char normal_data[] = "00 05 00 18 00 00 00 00"
                       "00 00 00 10 00 00 01 00 00 30 00 00 00 00 00 00";

  TAILQ_INIT(&instruction_list);

  /* normal */
  create_packet(normal_data, &test_pbuf);

  ofp_error_set(&expected_error, OFPET_BAD_INSTRUCTION, OFPBIC_BAD_LEN);
  ret = check_instruction_parse_expect_error(ofp_instruction_parse,
        test_pbuf,
        &instruction_list,
        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_instruction_parse(clear_actions) error.");
  ofp_instruction_list_elem_free(&instruction_list);
  pbuf_free(test_pbuf);
}

static lagopus_result_t
ofp_instruction_list_encode_wrap(struct channel *channel,
                                 struct pbuf **pbuf,
                                 struct ofp_header *xid_header) {
  struct instruction_list instruction_list;
  uint16_t total_length = 0;
  lagopus_result_t ret;
  (void) channel;
  (void) xid_header;

  create_data(&instruction_list);
  *pbuf = pbuf_alloc(WRITE_ACT_LEN);
  (*pbuf)->plen = WRITE_ACT_LEN;

  ret = ofp_instruction_list_encode(NULL, pbuf, &instruction_list,
                                    &total_length);

  TEST_ASSERT_EQUAL_MESSAGE(total_length, 0x30,
                            "instruction length error.");

  /* after. */
  ofp_instruction_list_elem_free(&instruction_list);

  return ret;
}

void
test_ofp_instruction_list_encode(void) {
  lagopus_result_t ret;

  ret = check_packet_create(ofp_instruction_list_encode_wrap,
                            "00 03 00 30 00 00 00 00"
                            "00 00 00 10"
                            "00 00 01 00 00 20"
                            "00 00 00 00 00 00"
                            "00 16 00 08 00 00 00 03"
                            "00 19 00 10 80 00 08 06"
                            "00 0c 29 7a 90 b3 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_instruction_list_encode(normal) error.");
}

void
test_ofp_instruction_list_encode_null(void) {
  lagopus_result_t ret;
  uint16_t total_length;
  struct pbuf *pbuf = NULL;
  struct instruction_list instruction_list;

  ret = ofp_instruction_list_encode(NULL, NULL, &instruction_list,
                                    &total_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_instruction_list_encode(NULL) error.");

  ret = ofp_instruction_list_encode(NULL, &pbuf, NULL,
                                    &total_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_instruction_list_encode(NULL) error.");

  ret = ofp_instruction_list_encode(NULL, &pbuf, &instruction_list,
                                    NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_instruction_list_encode(NULL) error.");
}

void
test_ofp_instruction_header_parse(void) {
  struct pbuf *test_pbuf;
  lagopus_result_t ret;
  struct instruction_list instruction_list;
  struct instruction *instruction;
  struct ofp_error error;
  struct ofp_error expected_error = {0 ,0, {NULL}};
  uint16_t ins_type = OFPIT_WRITE_ACTIONS;
  uint16_t ins_len = 0x04;
  int ins_num = 1;
  int i;

  /* instruction packet. */
  char normal_data[] = "00 03 00 04";
  char error_data[] = "00 03 00";

  TAILQ_INIT(&instruction_list);

  /* normal */
  create_packet(normal_data, &test_pbuf);

  ret = ofp_instruction_header_parse(test_pbuf, &instruction_list,
                                     &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_instruction_header_parse(normal) error.");

  i = 0;
  TAILQ_FOREACH(instruction, &instruction_list, entry) {
    TEST_ASSERT_EQUAL_MESSAGE(ins_type, instruction->ofpit_actions.type,
                              "type error.");
    TEST_ASSERT_EQUAL_MESSAGE(ins_len, instruction->ofpit_actions.len,
                              "len error.");
    TEST_ASSERT_EQUAL_MESSAGE(true, TAILQ_EMPTY(&instruction->action_list),
                              "action_list error.");
    i++;
  }
  TEST_ASSERT_EQUAL_MESSAGE(ins_num, i,
                            "length error.");

  pbuf_free(test_pbuf);
  ofp_instruction_list_elem_free(&instruction_list);

  /* error */
  TAILQ_INIT(&instruction_list);
  create_packet(error_data, &test_pbuf);

  /*
  ret = ofp_instruction_header_parse(test_pbuf, &instruction_list,
                                     &error);
  */
  ofp_error_set(&expected_error, OFPET_BAD_INSTRUCTION, OFPBIC_BAD_LEN);
  ret = check_instruction_parse_expect_error(ofp_instruction_header_parse,
        test_pbuf,
        &instruction_list,
        &expected_error);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_instruction_header_parse(error) error.");
  free(instruction);
  pbuf_free(test_pbuf);
}

void
test_ofp_instruction_header_parse_null(void) {
  struct pbuf *pbuf = pbuf_alloc(0);
  struct instruction_list instruction_list;
  struct ofp_error error;
  lagopus_result_t ret;

  ret = ofp_instruction_header_parse(NULL, &instruction_list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_instruction_header_parse(NULL) error.");

  ret = ofp_instruction_header_parse(pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_instruction_header_parse(NULL) error.");

  /* after. */
  pbuf_free(pbuf);
}

static lagopus_result_t
ofp_instruction_header_list_encode_wrap(struct channel *channel,
                                        struct pbuf **pbuf,
                                        struct ofp_header *xid_header) {
  struct instruction_list instruction_list;
  uint16_t total_length = 0;
  lagopus_result_t ret;
  (void) channel;
  (void) xid_header;

  create_header_data(&instruction_list);
  *pbuf = pbuf_alloc(INS_LEN * 2);
  (*pbuf)->plen = INS_LEN * 2;

  ret = ofp_instruction_header_list_encode(NULL, pbuf, &instruction_list,
        &total_length);

  TEST_ASSERT_EQUAL_MESSAGE(total_length, INS_LEN * 2,
                            "instruction length error.");

  /* after. */
  ofp_instruction_list_elem_free(&instruction_list);

  return ret;
}

void
test_ofp_instruction_header_list_encode(void) {
  lagopus_result_t ret;

  ret = check_packet_create(ofp_instruction_header_list_encode_wrap,
                            "00 03 00 04"
                            "00 01 00 04");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_instruction_header_list_encode(normal) error.");
}

void
test_ofp_instruction_header_list_encode_null(void) {
  struct pbuf *pbuf;
  struct instruction_list instruction_list;
  uint16_t total_length;
  lagopus_result_t ret;

  ret = ofp_instruction_header_list_encode(NULL, NULL, &instruction_list,
        &total_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_instruction_header_list_encode(NULL) error.");

  ret = ofp_instruction_header_list_encode(NULL, &pbuf, NULL,
        &total_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_instruction_header_list_encode(NULL) error.");

  ret = ofp_instruction_header_list_encode(NULL, &pbuf, &instruction_list,
        NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_instruction_header_list_encode(NULL) error.");
}

void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
