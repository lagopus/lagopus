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
#include "lagopus_session.h"
#include "lagopus/pbuf.h"
#include "lagopus/flowdb.h"
#include "lagopus/ofp_dp_apis.h"
#include "openflow13.h"
#include "handler_test_utils.h"
#include "../channel.h"
#include "../channel_mgr.h"
#include "../ofp_match.h"

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
setUp(void) {
}

void
tearDown(void) {
}

void
test_match_alloc(void) {
  uint8_t size = 10;
  struct match *match;

  /* call func. */
  match = match_alloc(size);

  TEST_ASSERT_NOT_NULL(match);
  free(match);
}

static lagopus_result_t
ofp_match_parse_wrap(struct channel *channel,
                     struct pbuf *pbuf,
                     struct ofp_header *xid_header,
                     struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct match_list match_list;
  struct match *match;
  int i;
  int match_num = 3;
  uint16_t class[3] = {OFPXMC_OPENFLOW_BASIC,
                       OFPXMC_OPENFLOW_BASIC,
                       OFPXMC_OPENFLOW_BASIC
  };
  uint8_t field[3] = {OFPXMT_OFB_IN_PORT,
                      OFPXMT_OFB_ETH_SRC,
                      OFPXMT_OFB_GRE_FLAGS,
  };
  uint8_t hasmask[3] = {0, 1, 0};
  uint8_t length[3] = {4, 6, 2};
  uint8_t value[3][6] = {{0x00, 0x00, 0x00, 0x10},
                         {0x00, 0x0c, 0x29, 0x7a, 0x90, 0xb3},
                         {0x01, 0x02}
  };
  (void) xid_header;

  TAILQ_INIT(&match_list);

  ret = ofp_match_parse(channel, pbuf, &match_list, error);

  if (ret == LAGOPUS_RESULT_OK) {
    i = 0;
    TAILQ_FOREACH(match, &match_list, entry) {
      if (i < match_num) {
        TEST_ASSERT_EQUAL_MESSAGE(class[i], match->oxm_class,
                                  "oxm_class error.");
        TEST_ASSERT_EQUAL_MESSAGE(field[i], match->oxm_field >> 1,
                                  "oxm_field error.");

        TEST_ASSERT_EQUAL_MESSAGE(hasmask[i], match->oxm_field & 0x01,
                                  "oxm_hasmask error.");

        TEST_ASSERT_EQUAL_MESSAGE(length[i], match->oxm_length,
                                  "oxm_length error.");
        TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(match->oxm_value, value[i],
                                            length[i]),
                                  "oxm_value error");
        i++;
      } else {
        TEST_FAIL_MESSAGE("match num error.");
      }
    }
    TEST_ASSERT_EQUAL_MESSAGE(match_num, i, "match num error.");

    /* free. */
    ofp_match_list_elem_free(&match_list);
  }

  return ret;
}

void
test_ofp_match_parse(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(ofp_match_parse_wrap,
                           "00 01 00 1C 80 00 00 04 00 00 00 10"
                         /* <---> type = 1 OXM match
                          *       <---> length = 28
                          *             <---------> OXM TLV header (oxm_class = 0x8000
                          *                                          -> OFPXMC_OPENFLOW_BASIC
                          *                                         oxm_field = 0
                          *                                          -> OFPXMT_OFB_IN_PORT,
                          *                                         oxm_hashmask = 0,
                          *                                         oxm_length = 4)
                          *                         <---------> OXML TLV payload
                          */
                           "80 00 09 06 00 0c 29 7a 90 b3"
                         /*
                          * <---------> OXM TLV header (oxm_class = 0x8000
                          *                              -> OFPXMC_OPENFLOW_BASIC
                          *                             oxm_field = 4
                          *                              -> OFPXMT_OFB_ETH_SRC,
                          *                             oxm_hashmask = 1,
                          *                             oxm_length = 6)
                          *             <---------------> OXML TLV payload
                          */
                           "80 00 56 02 01 02 00 00 00 00"
                         /*
                          * <---------> OXM TLV header (oxm_class = 0x8000
                          *                              -> OFPXMC_OPENFLOW_BASIC
                          *                             oxm_field = 43
                          *                              -> OFPXMT_OFB_ETH_SRC,
                          *                             oxm_hashmask = 0,
                          *                             oxm_length = 6)
                          *             <---> OXML TLV payload
                          *                   <---------> pad
                          */
                           );
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "test_ofp_match_parse(normal) error.");
}

void
test_ofp_match_parse_error_match_short_packet(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_match_parse_wrap,
                                        "00 01 00 16 80 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "bad length(normal) error.");
}

void
test_ofp_match_parse_error_match_long_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_match_parse_wrap,
                                        "00 01 00 19 80 00 00 04 00 00 00 10"
                                        /* <---> type = 1 OXM match
                                         *       <---> length = 25 : bad length(22)
                                         *             <---------> OXM TLV header (oxm_class = 0x8000
                                         *                                          -> OFPXMC_OPENFLOW_BASIC
                                         *                                         oxm_field = 0
                                         *                                          -> OFPXMT_OFB_IN_PORT,
                                         *                                         oxm_hashmask = 0,
                                         *                                         oxm_length = 4)
                                         *                         <---------> OXML TLV payload
                                         */
                                        "80 00 08 06 00 0c 29 7a 90 b3 00 00",
                                        /*
                                         * <---------> OXM TLV header (oxm_class = 0x8000
                                         *                              -> OFPXMC_OPENFLOW_BASIC
                                         *                             oxm_field = 4
                                         *                              -> OFPXMT_OFB_ETH_SRC,
                                         *                             oxm_hashmask = 0,
                                         *                             oxm_length = 6)
                                         *             <---------------> OXML TLV payload
                                         *                               <---> pad
                                         */
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "bad length error.");
}

void
test_ofp_match_parse_error_match_short_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_match_parse_wrap,
                                        "00 01 00 15 80 00 00 04 00 00 00 10"
                                        /* <---> type = 1 OXM match
                                         *       <---> length = 21 : bad length(22)
                                         *             <---------> OXM TLV header (oxm_class = 0x8000
                                         *                                          -> OFPXMC_OPENFLOW_BASIC
                                         *                                         oxm_field = 0
                                         *                                          -> OFPXMT_OFB_IN_PORT,
                                         *                                         oxm_hashmask = 0,
                                         *                                         oxm_length = 4)
                                         *                         <---------> OXML TLV payload
                                         */
                                        "80 00 08 06 00 0c 29 7a 90 b3 00 00",
                                        /*
                                         * <---------> OXM TLV header (oxm_class = 0x8000
                                         *                              -> OFPXMC_OPENFLOW_BASIC
                                         *                             oxm_field = 4
                                         *                              -> OFPXMT_OFB_ETH_SRC,
                                         *                             oxm_hashmask = 0,
                                         *                             oxm_length = 6)
                                         *             <---------------> OXML TLV payload
                                         *                               <---> pad
                                         */
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "bad length error.");
}

void
test_ofp_match_parse_error_oxm_long_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  /* FIXME : ideal value is OFPBMC_BAD_LEN. */
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_TYPE);

  ret = check_packet_parse_expect_error(ofp_match_parse_wrap,
                                        "00 01 00 16 80 00 00 05 00 00 00 10"
                                        /* <---> type = 1 OXM match
                                         *       <---> length = 22
                                         *             <---------> OXM TLV header (oxm_class = 0x8000
                                         *                                          -> OFPXMC_OPENFLOW_BASIC
                                         *                                         oxm_field = 0
                                         *                                          -> OFPXMT_OFB_IN_PORT,
                                         *                                         oxm_hashmask = 0,
                                         *                                         oxm_length = 5 :
                                         *                                          bad length(4)
                                         *                         <---------> OXML TLV payload
                                         */
                                        "80 00 08 06 00 0c 29 7a 90 b3 00 00",
                                        /*
                                         * <---------> OXM TLV header (oxm_class = 0x8000
                                         *                              -> OFPXMC_OPENFLOW_BASIC
                                         *                             oxm_field = 4
                                         *                              -> OFPXMT_OFB_ETH_SRC,
                                         *                             oxm_hashmask = 0,
                                         *                             oxm_length = 6)
                                         *             <---------------> OXML TLV payload
                                         *                               <---> pad
                                         */
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "bad length error.");
}

void
test_ofp_match_parse_error_oxm_short_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  /* FIXME : ideal value is OFPBMC_BAD_LEN. */
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_TYPE);

  ret = check_packet_parse_expect_error(ofp_match_parse_wrap,
                                        "00 01 00 16 80 00 00 03 00 00 00 10"
                                        /* <---> type = 1 OXM match
                                         *       <---> length = 22
                                         *             <---------> OXM TLV header (oxm_class = 0x8000
                                         *                                          -> OFPXMC_OPENFLOW_BASIC
                                         *                                         oxm_field = 0
                                         *                                          -> OFPXMT_OFB_IN_PORT,
                                         *                                         oxm_hashmask = 0,
                                         *                                         oxm_length = 3 :
                                         *                                          bad length(4)
                                         *                         <---------> OXML TLV payload
                                         */
                                        "80 00 08 06 00 0c 29 7a 90 b3 00 00",
                                        /*
                                         * <---------> OXM TLV header (oxm_class = 0x8000
                                         *                              -> OFPXMC_OPENFLOW_BASIC
                                         *                             oxm_field = 4
                                         *                              -> OFPXMT_OFB_ETH_SRC,
                                         *                             oxm_hashmask = 0,
                                         *                             oxm_length = 6)
                                         *             <---------------> OXML TLV payload
                                         *                               <---> pad
                                         */
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "bad length error.");
}

void
test_ofp_match_parse_error_oxm_short_packet(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_LEN);

  ret = check_packet_parse_expect_error(ofp_match_parse_wrap,
                                        "00 01 00 16 80 00 00 04 00 00 00 10"
                                        /* <---> type = 1 OXM match
                                         *       <---> length = 22
                                         *             <---------> OXM TLV header (oxm_class = 0x8000
                                         *                                          -> OFPXMC_OPENFLOW_BASIC
                                         *                                         oxm_field = 0
                                         *                                          -> OFPXMT_OFB_IN_PORT,
                                         *                                         oxm_hashmask = 0,
                                         *                                         oxm_length = 4
                                         *                         <---------> OXML TLV payload
                                         */
                                        "80 00 08 06 00 0c 29 7a 90 b3 00",
                                        /*
                                         * <---------> OXM TLV header (oxm_class = 0x8000
                                         *                              -> OFPXMC_OPENFLOW_BASIC
                                         *                             oxm_field = 4
                                         *                              -> OFPXMT_OFB_ETH_SRC,
                                         *                             oxm_hashmask = 0,
                                         *                             oxm_length = 6)
                                         *             <---------------> OXML TLV payload
                                         *                               <---> pad : short
                                         */
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "bad length error.");
}

void
test_ofp_match_parse_error_oxm_bad_field(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_MATCH, OFPBMC_BAD_FIELD);

  ret = check_packet_parse_expect_error(ofp_match_parse_wrap,
                                        "00 01 00 16 80 00 00 04 00 00 00 10"
                                        /* <---> type = 1 OXM match
                                         *       <---> length = 22
                                         *             <---------> OXM TLV header (oxm_class = 0x8000
                                         *                                          -> OFPXMC_OPENFLOW_BASIC
                                         *                                         oxm_field = 0
                                         *                                          -> OFPXMT_OFB_IN_PORT,
                                         *                                         oxm_hashmask = 0,
                                         *                                         oxm_length = 4
                                         *                         <---------> OXML TLV payload
                                         */
                                        "80 00 fe 06 00 0c 29 7a 90 b3 00 00",
                                        /*
                                         * <---------> OXM TLV header (oxm_class = 0x8000
                                         *                              -> OFPXMC_OPENFLOW_BASIC
                                         *                             oxm_field = 7f : bad field
                                         *                              -> OFPXMT_OFB_ETH_SRC,
                                         *                             oxm_hashmask = 0,
                                         *                             oxm_length = 6)
                                         *             <---------------> OXML TLV payload
                                         *                               <---> pad : short
                                         */
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "bad length error.");
}

void
test_ofp_match_parse_any_match(void) {
  struct channel *channel;
  struct pbuf *test_pbuf;
  lagopus_result_t ret;
  struct match_list match_list;
  struct ofp_error error;
  /* match packet. */
  char normal_data[] = "00 01 00 04 00 00 00 00";

  channel = create_data_channel();

  /* normal */
  TAILQ_INIT(&match_list);
  create_packet(normal_data, &test_pbuf);

  /* call func. */
  ret = ofp_match_parse(channel, test_pbuf, &match_list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_match_parse(any match) error.");

  TEST_ASSERT_EQUAL_MESSAGE(true, TAILQ_EMPTY(&match_list),
                            "match_list error.");

  TEST_ASSERT_EQUAL_MESSAGE(0, test_pbuf->plen,
                            "plen error.");

  ofp_match_list_elem_free(&match_list);
  pbuf_free(test_pbuf);
}

static
lagopus_result_t
ofp_match_list_encode_wrap(struct channel *channel,
                           struct pbuf **pbuf,
                           struct ofp_header *xid_header) {
  lagopus_result_t ret;
  int i;
  struct match_list match_list;
  struct match *match;
  int test_num = 3;
  uint16_t total_len = 0;
  uint16_t tlv_length = 0x20;
  uint16_t class = OFPXMC_OPENFLOW_BASIC;
  uint8_t field[] = {OFPXMT_OFB_IN_PORT << 1,
                     OFPXMT_OFB_ETH_SRC << 1,
                    OFPXMT_OFB_GRE_FLAGS << 1
                    };
  uint8_t length[] = {0x04, 0x06, 0x02};
  uint8_t value[3][8] = {{0x00, 0x00, 0x00, 0x10},
                         {
                           0x00, 0x0c, 0x29, 0x7a, 0x90, 0xb3,
                         },
                         {
                           0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
                         }
  };
  (void) xid_header;
  (void) channel;

  /* data. */
  TAILQ_INIT(&match_list);
  for (i = 0; i < test_num; i++) {
    match = match_alloc(length[i]);
    match->oxm_class = class;
    match->oxm_field = field[i];
    match->oxm_length = length[i];
    memcpy(match->oxm_value, value[i], match->oxm_length);
    TAILQ_INSERT_TAIL(&match_list, match, entry);
  }

  *pbuf = pbuf_alloc(tlv_length);
  (*pbuf)->plen = tlv_length;

  /* Call func (2 match encode). */
  ret = ofp_match_list_encode(NULL, pbuf, &match_list,
                              &total_len);
  TEST_ASSERT_EQUAL_MESSAGE(total_len, tlv_length,
                            "match length error.");

  /* free. */
  ofp_match_list_elem_free(&match_list);

  return ret;
}

void
test_ofp_match_list_encode(void) {
  lagopus_result_t ret;
  ret = check_packet(ofp_match_list_encode_wrap,
                     "00 01 00 1C"
                     "80 00 00 04 00 00 00 10"
                     "80 00 08 06 00 0c 29 7a 90 b3"
                     "80 00 56 02 01 02 00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_match_list_encode error.");
}

static
lagopus_result_t
ofp_match_list_encode_match_list_empty_wrap(struct channel *channel,
    struct pbuf **pbuf,
    struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct match_list match_list;
  uint16_t total_len = 0;
  uint16_t tlv_length = 0x04;
  uint16_t padding_length = 0x04;
  (void) xid_header;
  (void) channel;

  /* data. */
  TAILQ_INIT(&match_list);
  *pbuf = pbuf_alloc((size_t) (tlv_length + padding_length));
  (*pbuf)->plen = (size_t) (tlv_length + padding_length);

  /* Call func (2 match encode). */
  ret = ofp_match_list_encode(NULL, pbuf, &match_list,
                              &total_len);

  TEST_ASSERT_EQUAL_MESSAGE(total_len, tlv_length + padding_length,
                            "match length error.");

  return ret;
}

void
test_ofp_match_list_encode_match_list_empty(void) {
  lagopus_result_t ret;
  ret = check_packet(ofp_match_list_encode_match_list_empty_wrap,
                     "00 01 00 04"
                     "00 00 00 00");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_match_list_encode error.");
}

void
test_ofp_match_list_encode_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t total_len;
  struct match_list match_list;
  struct pbuf *pbuf = pbuf_alloc(0);

  /* Call func (Arg is NULL). */
  ret = ofp_match_list_encode(NULL, &pbuf, NULL, &total_len);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_match_list_encode error.");

  ret = ofp_match_list_encode(NULL, NULL, &match_list, &total_len);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_match_list_encode error.");

  ret = ofp_match_list_encode(NULL, &pbuf, &match_list, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_match_list_encode error.");

  /* free. */
  pbuf_free(pbuf);
}

void
test_ofp_match_list_for_list_encode(void) {
  lagopus_result_t ret;
  int i;
  struct match_list match_list;
  struct match *match;
  struct pbuf_list *pbuf_list = pbuf_list_alloc();
  struct pbuf *pbuf;
  int test_num = 2;
  uint16_t total_len = 0;
  uint16_t class = OFPXMC_OPENFLOW_BASIC;
  uint8_t field[] = {OFPXMT_OFB_IN_PORT << 1,
                     OFPXMT_OFB_ETH_SRC << 1
                    };
  uint8_t length[] = {0x04, 0x06};
  uint8_t value[2][6] = {{0x00, 0x00, 0x00, 0x10},
    {0x00, 0x0c, 0x29, 0x7a, 0x90, 0xb3}
  };
  struct pbuf *ret_pbuf;
  uint8_t ret_header[2][16] = {{
      0x04, 0x13, 0xff, 0xfd,
      0x00, 0x00, 0x00, 0x64,
      0x00, 0x05, 0x00, 0x01,
      0x00, 0x00, 0x00, 0x00
    },
    {
      0x04, 0x13, 0x00, 0x10,
      0x00, 0x00, 0x00, 0x64,
      0x00, 0x05, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00
    }
  };
  uint8_t ret_value[2][20] = {{0x00, 0x01, 0x00, 0x16},
    {
      0x80, 0x00, 0x00, 0x04,
      0x00, 0x00, 0x00, 0x10,
      0x80, 0x00, 0x08, 0x06,
      0x00, 0x0c, 0x29, 0x7a,
      0x90, 0xb3, 0x00, 0x00
    }
  };
  size_t ret_data_len[] = {OFP_PACKET_MAX_SIZE - 2,
                           sizeof(struct ofp_multipart_reply) + 20
                          };
  size_t ret_packet_len[] = {2,
                             OFP_PACKET_MAX_SIZE -
                             sizeof(struct ofp_multipart_reply) - 20
                            };

  /* data. */
  TAILQ_INIT(&match_list);
  for (i = 0; i < test_num; i++) {
    match = match_alloc(length[i]);
    match->oxm_class = class;
    match->oxm_field = field[i];
    match->oxm_length = length[i];
    memcpy(match->oxm_value, value[i], match->oxm_length);
    TAILQ_INSERT_TAIL(&match_list, match, entry);
  }

  create_packet("04 13 00 10 00 00 00 64"
                "00 05 00 00 00 00 00 00",
                &pbuf);
  pbuf->putp = pbuf->data + (OFP_PACKET_MAX_SIZE - 6);
  pbuf->plen = 6;
  pbuf_list_add(pbuf_list, pbuf);

  /* Call func (2 match encode). */
  ret = ofp_match_list_encode(pbuf_list, &pbuf, &match_list,
                              &total_len);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_match_list_encode error.");

  i = 0;
  TAILQ_FOREACH(ret_pbuf, &pbuf_list->tailq, entry) {
    TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, ret_pbuf,
                                  "ret_pbuf is NULL.");
    TEST_ASSERT_EQUAL_MESSAGE(ret_pbuf->data + ret_data_len[i],
                              ret_pbuf->putp,
                              "date length error.");
    TEST_ASSERT_EQUAL_MESSAGE(ret_packet_len[i], ret_pbuf->plen,
                              "pcket length error.");

    /* check header. */
    TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(ret_pbuf->data, ret_header[i],
                                        sizeof(struct ofp_multipart_reply)),
                              "ret_pbuf header error.");
    /* check pbuf data. */
    if (i == 0) {
      /* check data. */
      ret_pbuf->getp = ret_pbuf->data + sizeof(struct ofp_multipart_reply);
      while (ret_pbuf->getp < ret_pbuf->data + OFP_PACKET_MAX_SIZE - 6) {
        TEST_ASSERT_EQUAL_MESSAGE(0x00, *(ret_pbuf->getp),
                                  "ret_pbuf data error.");
        ret_pbuf->getp++;
      }
      TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(ret_pbuf->getp, ret_value[i], 4),
                                "ret_pbuf data error");
    } else if (i == 1) {
      /* check data. */
      ret_pbuf->getp = ret_pbuf->data + sizeof(struct ofp_multipart_reply);

      TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(ret_pbuf->getp, ret_value[i], 18),
                                "ret_pbuf data error");
    }

    i++;
  }

  TEST_ASSERT_EQUAL_MESSAGE(2, i, "ret_pbuf len error");

  /* free. */
  ofp_match_list_elem_free(&match_list);
  pbuf_list_free(pbuf_list);
}

void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
