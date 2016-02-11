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
#include "handler_test_utils.h"
#include "../channel_mgr.h"
#include "../ofp_action.h"

void
setUp(void) {
}

void
tearDown(void) {
}

#define OUT_LEN 0x10
#define GROUP_LEN 0x08
#define SET_FIELD_LEN 0x10
#define ACT_LEN 0x08
#define ENCAP_LEN 0x38
#define DECAP_LEN 0x40

static void
create_data(struct action_list *action_list) {
  struct action *action;
  struct ed_prop *ed_prop;
  struct ofp_action_output *act_output;
  struct ofp_action_encap *act_encap;
  struct ofp_action_decap *act_decap;
  struct ofp_action_group *act_group;
  struct ofp_action_set_field *act_set_field;
  uint8_t field[4] = {0x80, 0x00, 0x08, 0x06};
  uint8_t val[6] = {0x00, 0x0c, 0x29, 0x7a, 0x90, 0xb3};
  struct ofp_ed_prop_portname props[] ={
    {OFPPPT_PROP_PORT_NAME,
     0x18,
     OFPPPT_PROP_PORT_FLAGS_CREATE | OFPPPT_PROP_PORT_FLAGS_SET_INPORT,
     {0x0, 0x0},
     "hoge1"
    },
    {OFPPPT_PROP_PORT_NAME,
     0x18,
     OFPPPT_PROP_PORT_FLAGS_CREATE | OFPPPT_PROP_PORT_FLAGS_SET_INPORT,
     {0x0, 0x0},
     "hoge2"
    }
  };
  int prop_num = 2;
  int i;

  TAILQ_INIT(action_list);

  /* OUTPUT */
  action = action_alloc(OUT_LEN);
  TAILQ_INSERT_TAIL(action_list, action, entry);
  act_output = (struct ofp_action_output *)&action->ofpat;
  act_output->type = OFPAT_OUTPUT;
  act_output->len = OUT_LEN;
  act_output->port = 0x100;
  act_output->max_len = 0x20;

  /* ENCAP */
  action = action_alloc(ENCAP_LEN);
  TAILQ_INSERT_TAIL(action_list, action, entry);
  act_encap = (struct ofp_action_encap *)&action->ofpat;
  act_encap->type = OFPAT_ENCAP;
  act_encap->len = ENCAP_LEN;
  act_encap->packet_type = 0x01;
  /* ed_prop */
  for (i = 0; i < prop_num; i++) {
    ed_prop = ed_prop_alloc();
    TAILQ_INSERT_TAIL(&action->ed_prop_list, ed_prop, entry);
    ed_prop->ofp_ed_prop_portname = props[i];
  }

  /* DECAP */
  action = action_alloc(DECAP_LEN);
  TAILQ_INSERT_TAIL(action_list, action, entry);
  act_decap = (struct ofp_action_decap *)&action->ofpat;
  act_decap->type = OFPAT_DECAP;
  act_decap->len = DECAP_LEN;
  act_decap->cur_pkt_type = 0x02;
  act_decap->new_pkt_type = 0x03;
  /* ed_prop */
  for (i = 0; i < prop_num; i++) {
    ed_prop = ed_prop_alloc();
    TAILQ_INSERT_TAIL(&action->ed_prop_list, ed_prop, entry);
    ed_prop->ofp_ed_prop_portname = props[i];
  }

  /* GROUP */
  action = action_alloc(GROUP_LEN);
  TAILQ_INSERT_TAIL(action_list, action, entry);
  act_group = (struct ofp_action_group *)&action->ofpat;
  act_group->type = OFPAT_GROUP;
  act_group->len = GROUP_LEN;
  act_group->group_id = 0x03;

  /* SET_FIELD */
  action = action_alloc(SET_FIELD_LEN);
  TAILQ_INSERT_TAIL(action_list, action, entry);
  act_set_field = (struct ofp_action_set_field *)&action->ofpat;
  act_set_field->type = OFPAT_SET_FIELD;
  act_set_field->len = SET_FIELD_LEN;
  memcpy(act_set_field->field, field, 4);
  memcpy(act_set_field->field + 4, val, 6);
}

static void
create_header_data(struct action_list *action_list) {
  struct action *action;

  TAILQ_INIT(action_list);

  /* OUTPUT */
  action = action_alloc(ACT_LEN);
  TAILQ_INSERT_TAIL(action_list, action, entry);
  action->ofpat.type = OFPAT_OUTPUT;
  action->ofpat.len = ACT_LEN;

  /* GROUP */
  action = action_alloc(ACT_LEN);
  TAILQ_INSERT_TAIL(action_list, action, entry);
  action->ofpat.type = OFPAT_GROUP;
  action->ofpat.len = ACT_LEN;

  /* SET_FIELD */
  action = action_alloc(ACT_LEN);
  TAILQ_INSERT_TAIL(action_list, action, entry);
  action->ofpat.type = OFPAT_SET_FIELD;
  action->ofpat.len = ACT_LEN;
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
test_action_alloc(void) {
  uint16_t size = 10;
  struct action *action;

  /* call func. */
  action = action_alloc(size);

  TEST_ASSERT_NOT_NULL(action);
  free(action);
}

void
test_action_free(void) {
  uint16_t size = 10;
  struct action *action;

  action = action_alloc(size);
  TEST_ASSERT_NOT_NULL(action);

  /* call func. */
  action_free(action);
}

static size_t action_len;
static lagopus_result_t
ofp_action_parse_wrap(struct channel *channel,
                      struct pbuf *pbuf,
                      struct ofp_header *xid_header,
                      struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct action_list action_list;
  struct action *action;
  struct ofp_action_output *act_output;
  struct ofp_action_group *act_group;
  struct ofp_action_encap *act_encap;
  struct ofp_action_decap *act_decap;
  struct ed_prop *ed_prop;
  struct ofp_ed_prop_portname *p;
  struct ofp_ed_prop_portname props[] ={
    {OFPPPT_PROP_PORT_NAME,
     0x18,
     OFPPPT_PROP_PORT_FLAGS_CREATE | OFPPPT_PROP_PORT_FLAGS_SET_INPORT,
     {0x0, 0x0},
     "hoge1"
    },
    {OFPPPT_PROP_PORT_NAME,
     0x18,
     OFPPPT_PROP_PORT_FLAGS_CREATE | OFPPPT_PROP_PORT_FLAGS_SET_INPORT,
     {0x0, 0x0},
     "hoge2"
    }
  };
  int i, j;
  int action_num = 4;
  int prop_num = 2;
  uint32_t port = 0x100;
  uint16_t max_len = 0x20;
  uint32_t group_id = 0x03;
  uint32_t packet_type = 0x1;
  uint32_t cur_pkt_type = 0x02;
  uint32_t new_pkt_type = 0x03;
  (void) channel;
  (void) xid_header;

  TAILQ_INIT(&action_list);

  ret = ofp_action_parse(pbuf, action_len, &action_list, error);

  if (ret == LAGOPUS_RESULT_OK) {
    TEST_ASSERT_EQUAL_MESSAGE(false, TAILQ_EMPTY(&action_list),
                              "not action_list error.");
    i = 0;
    TAILQ_FOREACH(action, &action_list, entry) {
      switch (i) {
        case 0:
          act_output = (struct ofp_action_output *) &action->ofpat;
          TEST_ASSERT_EQUAL_MESSAGE(OFPAT_OUTPUT, act_output->type,
                                    "action type error.");
          TEST_ASSERT_EQUAL_MESSAGE(OUT_LEN, act_output->len,
                                    "action len error.");
          TEST_ASSERT_EQUAL_MESSAGE(port, act_output->port,
                                    "action port error.");
          TEST_ASSERT_EQUAL_MESSAGE(max_len, act_output->max_len,
                                    "action max_len error.");
          break;
        case 1:
          act_encap = (struct ofp_action_encap *) &action->ofpat;
          TEST_ASSERT_EQUAL_MESSAGE(OFPAT_ENCAP, act_encap->type,
                                    "action type error.");
          TEST_ASSERT_EQUAL_MESSAGE(ENCAP_LEN, act_encap->len,
                                    "action len error.");
          TEST_ASSERT_EQUAL_MESSAGE(packet_type, act_encap->packet_type,
                                    "packet type error.");
          j = 0;
          TAILQ_FOREACH(ed_prop, &action->ed_prop_list, entry) {
            p = &ed_prop->ofp_ed_prop_portname;
            TEST_ASSERT_EQUAL_MESSAGE(props[j].type, p->type,
                                      "prop type error.");
            TEST_ASSERT_EQUAL_MESSAGE(props[j].len, p->len,
                                      "prop len error.");
            TEST_ASSERT_EQUAL_MESSAGE(props[j].port_flags, p->port_flags,
                                      "prop port flags error.");
            TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(props[j].name, p->name,
                                                OFP_MAX_PORT_NAME_LEN),
                                      "prop port name error.");
            j++;
          }
          TEST_ASSERT_EQUAL_MESSAGE(prop_num, j, "prop length error.");
          break;
        case 2:
          act_decap = (struct ofp_action_decap *) &action->ofpat;
          TEST_ASSERT_EQUAL_MESSAGE(OFPAT_DECAP, act_decap->type,
                                    "action type error.");
          TEST_ASSERT_EQUAL_MESSAGE(DECAP_LEN, act_decap->len,
                                    "action len error.");
          TEST_ASSERT_EQUAL_MESSAGE(cur_pkt_type, act_decap->cur_pkt_type,
                                    "cur packet type error.");
          TEST_ASSERT_EQUAL_MESSAGE(new_pkt_type, act_decap->new_pkt_type,
                                    "new packet type error.");
          j = 0;
          TAILQ_FOREACH(ed_prop, &action->ed_prop_list, entry) {
            p = &ed_prop->ofp_ed_prop_portname;
            TEST_ASSERT_EQUAL_MESSAGE(props[j].type, p->type,
                                      "prop type error.");
            TEST_ASSERT_EQUAL_MESSAGE(props[j].len, p->len,
                                      "prop len error.");
            TEST_ASSERT_EQUAL_MESSAGE(props[j].port_flags, p->port_flags,
                                      "prop port flags error.");
            TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(props[j].name, p->name,
                                                OFP_MAX_PORT_NAME_LEN),
                                      "prop port name error.");
            j++;
          }
          TEST_ASSERT_EQUAL_MESSAGE(prop_num, j, "prop length error.");
          break;
        case 3:
          act_group = (struct ofp_action_group *) &action->ofpat;
          TEST_ASSERT_EQUAL_MESSAGE(OFPAT_GROUP, act_group->type,
                                    "action type error.");
          TEST_ASSERT_EQUAL_MESSAGE(GROUP_LEN, act_group->len,
                                    "action len error.");
          TEST_ASSERT_EQUAL_MESSAGE(group_id, act_group->group_id,
                                    "action group_id error.");
          break;
      }
      i++;
    }
    TEST_ASSERT_EQUAL_MESSAGE(action_num, i, "action length error.");
  }

  /* after. */
  ofp_action_list_elem_free(&action_list);

  return ret;
}

void
test_ofp_action_parse_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  action_len = (size_t) (OUT_LEN + GROUP_LEN + ENCAP_LEN + DECAP_LEN);
  ret = check_packet_parse(ofp_action_parse_wrap,
                          "00 00 00 10"
                        /* <----------... struct ofp_action_output
                         * <---> type : OFPAT_OUTPUT
                         *       <---> len : 16
                         */
                           "00 00 01 00 00 20"
                        /* <------> port : 1
                         *          <------> max_len : 32
                         */
                           "00 00 00 00 00 00"
                         /* <---------------> pad[6]
                         */
                           "00 1C 00 38"
                         /* <----------... struct ofp_action_encap
                            <---> type
                                  <---> len
                          */
                           "00 00 00 01"
                         /* <---------> packet_type
                          */
                           "00 01 00 18"
                         /* <----------... struct ofp_ed_prop_portname[0]
                            <---> type
                                  <---> len
                          */
                           "00 03 00 00"
                         /* <---> port_flags
                                  <---> pad[2]
                          */
                           "68 6f 67 65 31 00 00 00"
                           "00 00 00 00 00 00 00 00"
                         /* <---------------------> name
                          */
                           "00 01 00 18"
                         /* <----------... struct ofp_ed_prop_portname[1]
                            <---> type
                                  <---> len
                          */
                           "00 03 00 00"
                         /* <---> port_flags
                                  <---> pad[2]
                          */
                           "68 6f 67 65 32 00 00 00"
                           "00 00 00 00 00 00 00 00"
                         /* <---------------------> name
                          */
                           "00 1D 00 40"
                         /* <----------... struct ofp_action_decap
                            <---> type
                                  <---> len
                          */
                           "00 00 00 02"
                         /* <---------> cur_pkt_type
                          */
                           "00 00 00 03"
                         /* <---------> new_pkt_type
                          */
                           "00 00 00 00"
                         /* <---------> pad[4]
                          */
                           "00 01 00 18"
                         /* <----------... struct ofp_ed_prop_portname[0]
                            <---> type
                                  <---> len
                          */
                           "00 03 00 00"
                         /* <---> port_flags
                                  <---> pad[2]
                          */
                           "68 6f 67 65 31 00 00 00"
                           "00 00 00 00 00 00 00 00"
                         /* <---------------------> name
                          */
                           "00 01 00 18"
                         /* <----------... struct ofp_ed_prop_portname[1]
                            <---> type
                                  <---> len
                          */
                           "00 03 00 00"
                         /* <---> port_flags
                                  <---> pad[2]
                          */
                           "68 6f 67 65 32 00 00 00"
                           "00 00 00 00 00 00 00 00"
                         /* <---------------------> name
                          */
                           "00 16 00 08 00 00 00 03"
                         /* <----------------------> struct ofp_action_group
                          * <----> type : OFPAT_GROUP
                          *       <----> len : 8
                          *              <---------> group_id : 3
                          */
                           );
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_action_parse(normal) error.");
}

void
test_ofp_action_parse_error_set_field_bad_class(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  action_len = (size_t) SET_FIELD_LEN;
  ofp_error_set(&expected_error, OFPET_BAD_ACTION, OFPBAC_BAD_SET_TYPE);
  ret = check_packet_parse_expect_error(ofp_action_parse_wrap,
                                        "00 19 00 10 00 01 08 06"
                                        /* <---------------------->
                                         *      struct ofp_action_set_field
                                         * <----> type : OFPAT_SET_FIELD
                                         *       <----> len : 16
                                         *             <-----------... oxm
                                         *             <---> oxm_class : OFPXMC_NXM_1
                                         *                   <> oxm field
                                         *                         (OFPXMT_OFB_ETH_DST) +
                                         *                         oxm_hasmask(0)
                                         *                      <> oxm_length : 6
                                         */
                                        "00 0c 29 7a 90 b3 00 00",
                                        /* <---------------> oxm_data
                                         *                   <---> padding
                                         */
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
}

void
test_ofp_action_parse_error_set_field_bad_field(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  action_len = (size_t) SET_FIELD_LEN;
  ofp_error_set(&expected_error, OFPET_BAD_ACTION, OFPBAC_BAD_SET_TYPE);
  ret = check_packet_parse_expect_error(ofp_action_parse_wrap,
                                        "00 19 00 10 80 00 ff 06"
                                        /* <---------------------->
                                         *      struct ofp_action_set_field
                                         * <----> type : OFPAT_SET_FIELD
                                         *       <----> len : 16
                                         *             <-----------... oxm
                                         *             <---> oxm_class : OFPXMC_OPENFLOW_BASIC
                                         *                   <> oxm field : ff
                                         *                         ((OFPXMT_OFB_ETH_DST) +
                                         *                           oxm_hasmask(0))
                                         *                      <> oxm_length : 6
                                         */
                                        "00 0c 29 7a 90 b3 00 00",
                                        /* <---------------> oxm_data
                                         *                   <---> padding
                                         */
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
}

void
test_ofp_action_parse_error_set_field_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  action_len = (size_t) SET_FIELD_LEN;
  ofp_error_set(&expected_error, OFPET_BAD_ACTION, OFPBAC_BAD_SET_LEN);
  ret = check_packet_parse_expect_error(ofp_action_parse_wrap,
                                        "00 19 00 10 80 00 08 ff"
                                        /* <---------------------->
                                         *      struct ofp_action_set_field
                                         * <----> type : OFPAT_SET_FIELD
                                         *       <----> len
                                         *             <-----------... oxm
                                         *             <---> oxm_class : OFPXMC_OPENFLOW_BASIC
                                         *                   <> oxm field
                                         *                         (OFPXMT_OFB_ETH_DST) +
                                         *                         oxm_hasmask(0)
                                         *                      <> oxm_length : ff (long)
                                         */
                                        "00 0c 29 7a 90 b3 00 00",
                                        /* <---------------> oxm_data
                                         *                   <---> padding
                                         */
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
}

void
test_ofp_action_parse_error_set_field_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;

  action_len = (size_t) SET_FIELD_LEN;
  ofp_error_set(&expected_error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
  ret = check_packet_parse_expect_error(ofp_action_parse_wrap,
                                        "00 19 00 10 80 00 08 06"
                                        /* <---------------------->
                                         *      struct ofp_action_set_field
                                         * <----> type : OFPAT_SET_FIELD
                                         *       <----> len : 255 (long)
                                         *             <-----------... oxm
                                         *             <---> oxm_class : OFPXMC_OPENFLOW_BASIC
                                         *                   <> oxm field
                                         *                         (OFPXMT_OFB_ETH_DST) +
                                         *                         oxm_hasmask(0)
                                         *                      <> oxm_length : 6
                                         */
                                        "00 0c 29 7a 90 b3 00",
                                        /* <---------------> oxm_data
                                         *                   <---> padding : short
                                         */
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
}

static lagopus_result_t
ofp_action_list_encode_wrap(struct channel *channel,
                            struct pbuf **pbuf,
                            struct ofp_header *xid_header) {
  struct action_list action_list;
  lagopus_result_t ret;
  uint16_t total_length;
  uint16_t length;
  (void) channel;
  (void) xid_header;

  create_data(&action_list);
  length = OUT_LEN + GROUP_LEN + SET_FIELD_LEN + ENCAP_LEN + DECAP_LEN;
  *pbuf = pbuf_alloc(length);
  (*pbuf)->plen = length;

  ret = ofp_action_list_encode(NULL, pbuf, &action_list,
                               &total_length);

  TEST_ASSERT_EQUAL_MESSAGE(length,
                            total_length,
                            "action length error.");

  /* after. */
  ofp_action_list_elem_free(&action_list);

  return ret;
}

void
test_ofp_action_list_encode(void) {
  lagopus_result_t ret;

  ret = check_packet_create(ofp_action_list_encode_wrap,
                            "00 00 00 10"
                          /* <----------... struct ofp_action_output
                           * <---> type : OFPAT_OUTPUT
                           *       <---> len : 16
                           */
                            "00 00 01 00 00 20"
                          /* <------> port : 1
                           *          <------> max_len : 32
                           */
                            "00 00 00 00 00 00"
                          /* <---------------> pad[6]
                           */
                            "00 1C 00 38"
                          /* <----------... struct ofp_action_encap
                             <---> type
                                   <---> len
                           */
                            "00 00 00 01"
                          /* <---------> packet_type
                           */
                            "00 01 00 18"
                          /* <----------... struct ofp_ed_prop_portname[0]
                             <---> type
                                   <---> len
                           */
                            "00 03 00 00"
                          /* <---> port_flags
                                   <---> pad[2]
                           */
                            "68 6f 67 65 31 00 00 00"
                            "00 00 00 00 00 00 00 00"
                          /* <---------------------> name
                           */
                            "00 01 00 18"
                          /* <----------... struct ofp_ed_prop_portname[1]
                             <---> type
                                   <---> len
                           */
                            "00 03 00 00"
                          /* <---> port_flags
                                   <---> pad[2]
                           */
                            "68 6f 67 65 32 00 00 00"
                            "00 00 00 00 00 00 00 00"
                          /* <---------------------> name
                           */

                           "00 1D 00 40"
                         /* <----------... struct ofp_action_decap
                            <---> type
                                  <---> len
                          */
                           "00 00 00 02"
                         /* <---------> cur_pkt_type
                          */
                           "00 00 00 03"
                         /* <---------> new_pkt_type
                          */
                           "00 00 00 00"
                         /* <---------> pad[4]
                          */
                           "00 01 00 18"
                         /* <----------... struct ofp_ed_prop_portname[0]
                            <---> type
                                  <---> len
                          */
                           "00 03 00 00"
                         /* <---> port_flags
                                  <---> pad[2]
                          */
                           "68 6f 67 65 31 00 00 00"
                           "00 00 00 00 00 00 00 00"
                         /* <---------------------> name
                          */
                           "00 01 00 18"
                         /* <----------... struct ofp_ed_prop_portname[1]
                            <---> type
                                  <---> len
                          */
                           "00 03 00 00"
                         /* <---> port_flags
                                  <---> pad[2]
                          */
                           "68 6f 67 65 32 00 00 00"
                           "00 00 00 00 00 00 00 00"
                         /* <---------------------> name
                          */

                            "00 16 00 08 00 00 00 03"
                          /* <----------------------> struct ofp_action_group
                           * <----> type : OFPAT_GROUP
                           *       <----> len : 8
                           *              <---------> group_id : 3
                           */
                            "00 19 00 10 80 00 08 06"
                          /* <---------------------->
                           *      struct ofp_action_set_field
                           * <----> type : OFPAT_SET_FIELD
                           *       <----> len : 16
                           *             <-----------... oxm
                           *             <---> oxm_class : OFPXMC_OPENFLOW_BASIC
                           *                   <> oxm field
                           *                         (OFPXMT_OFB_ETH_DST) +
                           *                         oxm_hasmask(0)
                           *                      <> oxm_length : 6
                           */
                            "00 0c 29 7a 90 b3 00 00"
                          /* <---------------> oxm_data
                           *                   <---> padding
                           */
                            );

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_action_list_encode(normal) error.");
}

void
test_ofp_action_list_encode_null(void) {
  lagopus_result_t ret;
  uint16_t total_length;
  struct pbuf *pbuf;
  struct action_list action_list;

  ret = ofp_action_list_encode(NULL, NULL, &action_list,
                               &total_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_action_list_encode(null) error.");

  ret = ofp_action_list_encode(NULL, &pbuf, NULL, &total_length);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_action_list_encode(null) error.");

  ret = ofp_action_list_encode(NULL, &pbuf, &action_list, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_action_list_encode(null) error.");
}

static lagopus_result_t
ofp_action_header_parse_wrap(struct channel *channel,
                             struct pbuf *pbuf,
                             struct ofp_header *xid_header,
                             struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct action_list action_list;
  struct action *action;
  int i;
  int action_num = 2;
  (void) channel;
  (void) xid_header;

  TAILQ_INIT(&action_list);

  /* call func. */
  ret = ofp_action_header_parse(pbuf, (size_t) (ACT_LEN * action_num),
                                &action_list, error);

  if (ret == LAGOPUS_RESULT_OK) {
    TEST_ASSERT_EQUAL_MESSAGE(false, TAILQ_EMPTY(&action_list),
                              "not action_list error.");
    i = 0;
    TAILQ_FOREACH(action, &action_list, entry) {
      switch (i) {
        case 0:
          TEST_ASSERT_EQUAL_MESSAGE(OFPAT_OUTPUT, action->ofpat.type,
                                    "action type error.");
          TEST_ASSERT_EQUAL_MESSAGE(ACT_LEN, action->ofpat.len,
                                    "action len error.");
          break;
        case 1:
          TEST_ASSERT_EQUAL_MESSAGE(OFPAT_GROUP, action->ofpat.type,
                                    "action type error.");
          TEST_ASSERT_EQUAL_MESSAGE(ACT_LEN, action->ofpat.len,
                                    "action len error.");
          break;
      }
      i++;
    }
    TEST_ASSERT_EQUAL_MESSAGE(action_num, i, "action length error.");
  }

  /* after. */
  ofp_action_list_elem_free(&action_list);

  return ret;
}

void
test_ofp_action_header_parse_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(ofp_action_header_parse_wrap,
                           "00 00 00 08"
                           /* <----------... struct ofp_action_output
                            * <---> type : OFPAT_OUTPUT
                            *       <---> len : 8
                            */
                           "00 00 00 00"
                           /* <---------> padding
                            */
                           "00 16 00 08"
                           /* <----------------------> struct ofp_action_group
                            * <----> type : OFPAT_GROUP
                            *       <----> len : 8
                            */
                           "00 00 00 00");
  /* <---------> padding
   */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_action_header_parse(normal) error.");
}

static lagopus_result_t
ofp_action_header_list_encode_wrap(struct channel *channel,
                                   struct pbuf **pbuf,
                                   struct ofp_header *xid_header) {
  struct action_list action_list;
  lagopus_result_t ret;
  uint16_t total_length;
  (void) channel;
  (void) xid_header;

  create_header_data(&action_list);
  *pbuf = pbuf_alloc(ACT_LEN * 3);
  (*pbuf)->plen = ACT_LEN * 3;

  ret = ofp_action_header_list_encode(NULL, pbuf, &action_list,
                                      &total_length);

  TEST_ASSERT_EQUAL_MESSAGE(ACT_LEN * 3, total_length,
                            "action length error.");

  /* after. */
  ofp_action_list_elem_free(&action_list);

  return ret;
}

void
test_ofp_action_header_list_encode(void) {
  lagopus_result_t ret;

  ret = check_packet_create(ofp_action_header_list_encode_wrap,
                            "00 00 00 08"
                            /* <----------... struct ofp_action_output
                             * <---> type : OFPAT_OUTPUT
                             *       <---> len : 8
                             */
                            "00 00 00 00"
                            /* <---------> padding
                             */
                            "00 16 00 08"
                            /* <----------------------> struct ofp_action_group
                             * <----> type : OFPAT_GROUP
                             *       <----> len : 8
                             */
                            "00 00 00 00"
                            /* <---------> padding
                             */
                            "00 19 00 08"
                            /* <----------------------> struct ofp_action_set_field
                             * <----> type : OFPAT_SET_FIELD
                             *       <----> len : 8
                             */
                            "00 00 00 00");
  /* <---------> padding
   */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_action_header_list_encode(normal) error.");
}

void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
