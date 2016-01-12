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
#include "../ofp_apis.h"
#include "handler_test_utils.h"
#include "lagopus/pbuf.h"
#include "lagopus/ofp_handler.h"
#include "../ofp_band_stats.h"
#include "../ofp_instruction.h"
#include "../ofp_action.h"
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

static struct table_features *
s_table_features_alloc(void) {
  struct table_features *table_features = NULL;
  table_features = (struct table_features *)
                   calloc(1, sizeof(struct table_features));
  if (table_features != NULL) {
    TAILQ_INIT(&table_features->table_property_list);
  } else {
    TEST_FAIL_MESSAGE("table_features is NULL.");
  }

  return table_features;
}

static struct table_property *
s_table_property_alloc(void) {
  struct table_property *table_property = NULL;
  table_property = (struct table_property *)
                   calloc(1,
                          sizeof(struct table_property));
  if (table_property != NULL) {
    TAILQ_INIT(&table_property->instruction_list);
    TAILQ_INIT(&table_property->next_table_id_list);
    TAILQ_INIT(&table_property->action_list);
    TAILQ_INIT(&table_property->oxm_id_list);
    TAILQ_INIT(&table_property->experimenter_data_list);
  }
  return table_property;
}

static struct next_table_id *
s_next_table_id_alloc(void) {
  struct next_table_id *next_table_id = NULL;
  next_table_id = (struct next_table_id *) calloc(1,
                  sizeof(struct next_table_id));
  return next_table_id;
}

#define INS_LEN 0x04
#define ACT_LEN 0x08

static void
create_data(struct table_features_list *table_features_list,
            int test_num)  {
  struct table_features *table_features;
  struct table_property *table_property;
  struct next_table_id  *next_table_id;
  struct instruction *instruction;
  struct action *action;
  uint16_t length = 0x74;
  int i;

  TAILQ_INIT(table_features_list);
  for (i = 0; i < test_num; i++) {
    table_features = s_table_features_alloc();

    table_features->ofp.length         = length;
    table_features->ofp.table_id       = (uint8_t )(0x01 + i);
    table_features->ofp.name[0]        = (char    )'a';
    table_features->ofp.name[1]        = (char    )'b';
    table_features->ofp.name[2]        = (char    )'c';
    table_features->ofp.metadata_match = (uint64_t)(0x02 + i);
    table_features->ofp.metadata_write = (uint64_t)(0x03 + i);
    table_features->ofp.config         = (uint32_t)(0x04 + i);
    table_features->ofp.max_entries    = (uint32_t)(0x05 + i);

    /* OFPTFPT_NEXT_TABLES */
    {
      table_property = s_table_property_alloc();
      table_property->ofp.type = OFPTFPT_NEXT_TABLES;
      table_property->ofp.length = 0x06;

      next_table_id = s_next_table_id_alloc();
      next_table_id->ofp.id = (uint8_t)1;
      TAILQ_INSERT_TAIL(&table_property->next_table_id_list,
                        next_table_id,
                        entry);

      next_table_id = s_next_table_id_alloc();
      next_table_id->ofp.id = (uint8_t)2;
      TAILQ_INSERT_TAIL(&table_property->next_table_id_list,
                        next_table_id,
                        entry);

      TAILQ_INSERT_TAIL(&table_features->table_property_list,
                        table_property,
                        entry);
    }

    /* OFPTFPT_INSTRUCTIONS */
    {
      table_property = s_table_property_alloc();
      table_property->ofp.type = OFPTFPT_INSTRUCTIONS;
      table_property->ofp.length = (uint16_t) (INS_LEN * 2 +
                                   sizeof(struct ofp_table_feature_prop_header));

      /* WRITE_ACTIONS */
      instruction = instruction_alloc();
      TAILQ_INSERT_TAIL(&table_property->instruction_list, instruction, entry);
      instruction->ofpit.type = OFPIT_WRITE_ACTIONS;
      instruction->ofpit.len = INS_LEN;

      /* GOTO_TABLE */
      instruction = instruction_alloc();
      TAILQ_INSERT_TAIL(&table_property->instruction_list, instruction, entry);
      instruction->ofpit.type = OFPIT_GOTO_TABLE;
      instruction->ofpit.len = INS_LEN;

      TAILQ_INSERT_TAIL(&table_features->table_property_list,
                        table_property,
                        entry);
    }

    /* OFPTFPT_WRITE_ACTIONS */
    {
      table_property = s_table_property_alloc();
      table_property->ofp.type = OFPTFPT_WRITE_ACTIONS;
      table_property->ofp.length = (uint16_t) (ACT_LEN * 3 +
                                   sizeof(struct ofp_table_feature_prop_header));

      /* OUTPUT */
      action = action_alloc(0);
      TAILQ_INSERT_TAIL(&table_property->action_list, action, entry);
      action->ofpat.type = OFPAT_OUTPUT;
      action->ofpat.len = ACT_LEN;

      /* GROUP */
      action = action_alloc(0);
      TAILQ_INSERT_TAIL(&table_property->action_list, action, entry);
      action->ofpat.type = OFPAT_GROUP;
      action->ofpat.len = ACT_LEN;

      /* SET_FIELD */
      action = action_alloc(0);
      TAILQ_INSERT_TAIL(&table_property->action_list, action, entry);
      action->ofpat.type = OFPAT_SET_FIELD;
      action->ofpat.len = ACT_LEN;

      TAILQ_INSERT_TAIL(&table_features->table_property_list,
                        table_property,
                        entry);
    }

    TAILQ_INSERT_TAIL(table_features_list, table_features, entry);
  }
}

lagopus_result_t
ofp_table_features_reply_create_wrap(struct channel *channel,
                                     struct pbuf_list **pbuf_list,
                                     struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct table_features_list table_features_list;

  create_data(&table_features_list, 1);

  ret = ofp_table_features_reply_create(channel, pbuf_list,
                                        &table_features_list,
                                        xid_header);

  /* after. */
  table_features_list_elem_free(&table_features_list);

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
test_ofp_table_features_reply_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[1] = {
    // header
    "04 13 00 88 00 00 00 10 "
    "00 0C 00 00 00 00 00 00 "
    // ofp_table_features
    "00 78"                                   // uint16_t length;
    "01 "                                     // uint8_t table_id;
    "00 00 00 00 00 "                         // uint8_t pad[5];
    "61 62 63 00 00 00 00 00 "                // char name[32];
    "00 00 00 00 00 00 00 00 "                // char name[32];
    "00 00 00 00 00 00 00 00 "                // char name[32];
    "00 00 00 00 00 00 00 00 "                // char name[32];
    "00 00 00 00 00 00 00 02 "                // uint64_t metadata_match;
    "00 00 00 00 00 00 00 03 "                // uint64_t metadata_write;
    "00 00 00 04 "                            // uint32_t config;
    "00 00 00 05 "                            // uint32_t max_entries;
    // struct ofp_table_feature_prop_next_tables
    "00 02 "                                  // uint16_t type
    "00 06 "                                  // uint16_t length
    "01 02 "                                  // uint8_t next_table_ids
    "00 00 "                                  // 64 bit padding
    // struct ofp_table_feature_prop_instructions
    "00 00 "                  // uint16_t type
    "00 0c "                  // uint16_t length
    "00 03 00 04"             // struct ofp_instruction[0]
    "00 01 00 04"             // struct ofp_instruction[1]
    "00 00 00 00"             // padding
    // struct ofp_table_feature_prop_actions
    "00 04 "                  // uint16_t type
    "00 1c "                  // uint16_t length
    "00 00 00 08"             // struct ofp_action_header[0]
    "00 00 00 00"
    "00 16 00 08"             // struct ofp_action_header[1]
    "00 00 00 00"
    "00 19 00 08"             // struct ofp_action_header[2]
    "00 00 00 00"
    "00 00 00 00"             // padding
  };

  ret = check_pbuf_list_packet_create(ofp_table_features_reply_create_wrap,
                                      data, 1);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_table_features_reply_create(normal) error.");
}

lagopus_result_t
ofp_table_features_reply_create_null_wrap(struct channel *channel,
    struct pbuf_list **pbuf_list,
    struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct table_features_list table_features_list;

  create_data(&table_features_list, 1);

  ret = ofp_table_features_reply_create(NULL,
                                        pbuf_list,
                                        &table_features_list,
                                        xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            ret,
                            "ofp_table_features_reply_create error.");

  ret = ofp_table_features_reply_create(channel,
                                        NULL,
                                        &table_features_list,
                                        xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            ret,
                            "ofp_table_features_reply_create error.");

  ret = ofp_table_features_reply_create(channel,
                                        pbuf_list,
                                        NULL,
                                        xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            ret,
                            "ofp_table_features_reply_create error.");

  ret = ofp_table_features_reply_create(channel,
                                        pbuf_list,
                                        &table_features_list,
                                        NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                            ret,
                            "ofp_table_features_reply_create error.");

  /* after. */
  table_features_list_elem_free(&table_features_list);

  /* Not check return value. */
  return -9999;
}

void
test_ofp_table_features_reply_create_null(void) {
  const char *data[1] = {"04 13 00 a0 00 00 00 10"};

  /* Not check return value. */
  (void ) check_pbuf_list_packet_create(
    ofp_table_features_reply_create_null_wrap,
    data, 1);
}

void
test_ofp_table_features_request_handle_normal_pattern0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 80 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 70"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_next_tables
                                        //   properties[0];
                                        "00 02 "                   // uint16_t type
                                        "00 06 "                   // uint16_t length
                                        "01 02 "                   // uint8_t next_table_ids
                                        "00 00 "                   // 64 bit padding
                                        // struct ofp_table_feature_prop_instructions
                                        "00 00 "                 // uint16_t type
                                        "00 0c "                 // uint16_t length
                                        "00 03 00 04"            // struct ofp_instruction[0]
                                        "00 01 00 04"            // struct ofp_instruction[1]
                                        "00 00 00 00"            // padding
                                        // struct ofp_table_feature_prop_actions
                                        "00 04 "                 // uint16_t type
                                        "00 14 "                 // uint16_t length
                                        "00 00 00 08"            // struct ofp_action_header
                                        "00 00 00 00"
                                        "00 16 00 08"            // struct ofp_action_header
                                        "00 00 00 00"
                                        "00 00 00 00",           // padding
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "table_features_request set error");
}


void
test_ofp_table_features_request_handle_normal_pattern1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 38 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 40"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 00",           // uint32_t max_entries;
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "table_features_request set error");
}

void
test_ofp_table_features_request_handle_normal_pattern2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        // struct ofp_multipart_request
                                        "04 12 01 00 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        // ofp_table_features[0]
                                        "00 78"                      // uint16_t length;
                                        "01 "                        // uint8_t table_id;
                                        "00 00 00 00 00 "            // uint8_t pad[5];
                                        "61 62 63 00 00 00 00 00 "   // char name[32];
                                        "00 00 00 00 00 00 00 00 "   // char name[32];
                                        "00 00 00 00 00 00 00 00 "   // char name[32];
                                        "00 00 00 00 00 00 00 00 "   // char name[32];
                                        "00 00 00 00 00 00 00 02 "   // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 "   // uint64_t metadata_write;
                                        "00 00 00 04 "               // uint32_t config;
                                        "00 00 00 05 "               // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_next_tables
                                        "00 02 "         // uint16_t type
                                        "00 06 "         // uint16_t length
                                        "01 02 "         // uint8_t next_table_ids
                                        "00 00 "         // 64 bit padding
                                        // struct ofp_table_feature_prop_instructions
                                        "00 00 "         // uint16_t type
                                        "00 0c "         // uint16_t length
                                        "00 03 00 04"    // struct ofp_instruction[0]
                                        "00 01 00 04"    // struct ofp_instruction[1]
                                        "00 00 00 00"    // padding
                                        // struct ofp_table_feature_prop_actions
                                        "00 04 "         // uint16_t type
                                        "00 1c "         // uint16_t length
                                        "00 00 00 08"    // struct ofp_action_header[0]
                                        "00 00 00 00"
                                        "00 16 00 08"    // struct ofp_action_header[1]
                                        "00 00 00 00"
                                        "00 19 00 08"    // struct ofp_action_header[2]
                                        "00 00 00 00"
                                        "00 00 00 00"    // padding
                                        // ofp_table_features[1]
                                        "00 78"                      // uint16_t length;
                                        "01 "                        // uint8_t table_id;
                                        "00 00 00 00 00 "            // uint8_t pad[5];
                                        "61 62 63 00 00 00 00 00 "   // char name[32];
                                        "00 00 00 00 00 00 00 00 "   // char name[32];
                                        "00 00 00 00 00 00 00 00 "   // char name[32];
                                        "00 00 00 00 00 00 00 00 "   // char name[32];
                                        "00 00 00 00 00 00 00 02 "   // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 "   // uint64_t metadata_write;
                                        "00 00 00 04 "               // uint32_t config;
                                        "00 00 00 05 "               // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_next_tables
                                        "00 02 "         // uint16_t type
                                        "00 06 "         // uint16_t length
                                        "01 02 "         // uint8_t next_table_ids
                                        "00 00 "         // 64 bit padding
                                        // struct ofp_table_feature_prop_instructions
                                        "00 00 "         // uint16_t type
                                        "00 0c "         // uint16_t length
                                        "00 03 00 04"    // struct ofp_instruction[0]
                                        "00 01 00 04"    // struct ofp_instruction[1]
                                        "00 00 00 00"    // padding
                                        // struct ofp_table_feature_prop_actions
                                        "00 04 "         // uint16_t type
                                        "00 1c "         // uint16_t length
                                        "00 00 00 08"    // struct ofp_action_header[0]
                                        "00 00 00 00"
                                        "00 16 00 08"    // struct ofp_action_header[1]
                                        "00 00 00 00"
                                        "00 19 00 08"    // struct ofp_action_header[2]
                                        "00 00 00 00"
                                        "00 00 00 00",   // padding
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "table_features_request set error");
}

void
test_ofp_table_features_request_handle_error_invalid_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 50 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 ",               // uint32_t max_entries;
                                        //  : short
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_ins_invalid_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 60 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_instructions
                                        "00 00 "                 // uint16_t type
                                        "00 0c "                 // uint16_t length
                                        "00 03 00 04"            // struct ofp_instruction[0]
                                        "00 01 00 04"            // struct ofp_instruction[1]
                                        "00 00 00",              // padding : short
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_ins_invalid_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 60 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_instructions
                                        "00 00 "                 // uint16_t type
                                        "00 0c "                 // uint16_t length
                                        "00 03 00 04"            // struct ofp_instruction[0]
                                        "00 01 00 04"            // struct ofp_instruction[1]
                                        "00 00 00 00"            // padding
                                        "00",                    // : long
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_ins_invalid_length2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_INSTRUCTION, OFPBIC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 60 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_instructions
                                        "00 00 "                 // uint16_t type
                                        "00 0b "                 // uint16_t length
                                        "00 03 00 04"            // struct ofp_instruction[0]
                                        "00 01 00",              // struct ofp_instruction[1]
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_ins_invalid_length3(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_INSTRUCTION, OFPBIC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 60 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_instructions
                                        "00 00 "                 // uint16_t type
                                        "00 0c "                 // uint16_t length
                                        "00 03 00 04"            // struct ofp_instruction[0]
                                        "00 01 00 0f"            // struct ofp_instruction[1]
                                        //  [length : 15 (long)]
                                        "00 00 00 00 ",          // padding
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_nexttable_invalid_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 58 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_next_tables properties[0];
                                        "00 02 "                   // uint16_t type
                                        "00 06 "                   // uint16_t length
                                        "01 02 "                   // uint8_t next_table_ids
                                        "00",                      // 64 bit padding : short
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_nexttable_invalid_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 58 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_next_tables properties[0];
                                        "00 02 "                   // uint16_t type
                                        "00 06 "                   // uint16_t length
                                        "01 02 "                   // uint8_t next_table_ids
                                        "00 00 "                   // 64 bit padding
                                        "00",                      // long
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_nexttable_invalid_length2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 58 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_next_tables properties[0];
                                        "00 02 "                   // uint16_t type
                                        "00 0f "                   // uint16_t length
                                        //  : 15 (long)
                                        "01 02 "                   // uint8_t next_table_ids
                                        "00 00 ",                  // 64 bit padding
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_nexttable_invalid_length3(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 58 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_next_tables properties[0];
                                        "00 02 "                   // uint16_t type
                                        "00 03 "                   // uint16_t length
                                        //  : 3 (short)
                                        "01 02 "                   // uint8_t next_table_ids
                                        "00 00 ",                  // 64 bit padding
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_oxm_invalid_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 60 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_oxm properties[0];
                                        "00 08 "                   // uint16_t type
                                        "00 0c "                   // uint16_t length
                                        "00 00 00 01"              // uint32_t oxm_ids[0]
                                        "00 00 00 02"              // uint32_t oxm_ids[1]
                                        "00 00 00",                // 64 bit padding : short
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_oxm_invalid_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 60 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_oxm properties[0];
                                        "00 08 "                   // uint16_t type
                                        "00 0c "                   // uint16_t length
                                        "00 00 00 01"              // uint32_t oxm_ids[0]
                                        "00 00 00 02"              // uint32_t oxm_ids[1]
                                        "00 00 00 00"              // 64 bit padding
                                        "00",                      // : long
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_oxm_invalid_length2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 60 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_oxm properties[0];
                                        "00 08 "                   // uint16_t type
                                        "00 0f "                   // uint16_t length
                                        //  : 15 (long)
                                        "00 00 00 01"              // uint32_t oxm_ids[0]
                                        "00 00 00 02"              // uint32_t oxm_ids[1]
                                        "00 00 00 00",             // 64 bit padding
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_oxm_invalid_length3(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 60 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_oxm properties[0];
                                        "00 08 "                   // uint16_t type
                                        "00 0b "                   // uint16_t length
                                        //  : 11 (short)
                                        "00 00 00 01"              // uint32_t oxm_ids[0]
                                        "00 00 00 02"              // uint32_t oxm_ids[1]
                                        "00 00 00 00",             // 64 bit padding
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_action_invalid_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 64 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_actions properties[0];
                                        "00 04"                   // uint16_t type
                                        "00 14 "                   // uint16_t length
                                        // struct ofp_action_header[0]
                                        "00 00 "                   // uint16_t type
                                        //  : OFPAT_OUTPUT
                                        "00 08 "                   // uint16_t length
                                        "00 00 00 00"              // pad
                                        // struct ofp_action_header[1]
                                        "00 0b "                   // uint16_t type
                                        //  : OFPAT_COPY_TTL_OUT
                                        "00 08 "                   // uint16_t length
                                        "00 00 00 00"              // pad[4]
                                        "00 00 00",                // padding : short
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_action_invalid_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 64 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_actions properties[0];
                                        "00 04"                   // uint16_t type
                                        "00 14 "                   // uint16_t length
                                        // struct ofp_action_header[0]
                                        "00 00 "                   // uint16_t type
                                        //  : OFPAT_OUTPUT
                                        "00 08 "                   // uint16_t length
                                        "00 00 00 00"              // pad
                                        // struct ofp_action_header[1]
                                        "00 0b "                   // uint16_t type
                                        //  : OFPAT_COPY_TTL_OUT
                                        "00 08 "                   // uint16_t length
                                        "00 00 00 00"              // pad[4]
                                        "00 00 00 00"              // padding
                                        "00",                      //long
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_action_invalid_length2(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 64 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_actions properties[0];
                                        "00 04"                   // uint16_t type
                                        "00 14 "                   // uint16_t length
                                        // struct ofp_action_header[0]
                                        "00 00 "                   // uint16_t type
                                        //  : OFPAT_OUTPUT
                                        "00 08 "                   // uint16_t length
                                        "00 00 00 00"              // pad
                                        // struct ofp_action_header[1]
                                        "00 0b "                   // uint16_t type
                                        //  : OFPAT_COPY_TTL_OUT
                                        "00 0f "                   // uint16_t length
                                        //  : 15 (long)
                                        "00 00 00 00"              // pad[4]
                                        "00 00 00 00",             // padding
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_error_action_invalid_length3(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 64 00 00 00 10 "
                                        "00 0c 00 00 00 00 00 00 "
                                        "00 52"                    // uint16_t length;
                                        "01 "                      // uint8_t table_id;
                                        "00 00 00 00 00 "          // uint8_t pad[5];
                                        "66 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 00 " // char name[32];
                                        "00 00 00 00 00 00 00 02 " // uint64_t metadata_match;
                                        "00 00 00 00 00 00 00 03 " // uint64_t metadata_write;
                                        "00 00 00 04 "             // uint32_t config;
                                        "00 00 00 05 "             // uint32_t max_entries;
                                        // struct ofp_table_feature_prop_actions properties[0];
                                        "00 04"                    // uint16_t type
                                        "00 14 "                   // uint16_t length
                                        // struct ofp_action_header[0]
                                        "00 00 "                   // uint16_t type
                                        //  : OFPAT_OUTPUT
                                        "00 08 "                   // uint16_t length
                                        "00 00 00 00"              // pad
                                        // struct ofp_action_header[1]
                                        "00 0b "                   // uint16_t type
                                        //  : OFPAT_COPY_TTL_OUT
                                        "00 07 "                   // uint16_t length
                                        //  : 7 (short)
                                        "00 00 00 00"              // pad[4]
                                        "00 00 00 00",             // padding
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error");
}

void
test_ofp_table_features_request_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header xid_header;
  struct ofp_error error;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);

  ret = ofp_table_features_request_handle(NULL, pbuf, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_table_features_request_handle error.");

  ret = ofp_table_features_request_handle(channel, NULL, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_table_features_request_handle error.");

  ret = ofp_table_features_request_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_table_features_request_handle error.");

  ret = ofp_table_features_request_handle(channel, pbuf, &xid_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_table_features_request_handle error.");

  /* after. */
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
