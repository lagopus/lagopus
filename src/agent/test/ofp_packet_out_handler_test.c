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
#include "../ofp_packet_out_handler.h"
#include "handler_test_utils.h"
#include "lagopus/pbuf.h"
#include "lagopus/ofp_dp_apis.h"
#include "../channel_mgr.h"

#define PUT_TIMEOUT 100LL * 1000LL * 1000LL * 1000LL
#define GET_TIMEOUT 100LL * 1000LL * 1000LL * 1000LL

/* for request data in ofp_error. */
struct pbuf *req_pbuf = NULL;

void
setUp(void) {
  /* for request data in ofp_error. */
  req_pbuf = pbuf_alloc(OFP_ERROR_MAX_SIZE);
  if (req_pbuf == NULL) {
    TEST_FAIL_MESSAGE("pbuf_alloc error.");
  }
}

void
tearDown(void) {
  /* for request data in ofp_error. */
  if (req_pbuf != NULL) {
    pbuf_free(req_pbuf);
    req_pbuf = NULL;
  }
}

static lagopus_result_t
ofp_packet_out_handle_wrap(struct channel *channel,
                           struct pbuf *pbuf,
                           struct ofp_header *xid_header,
                           struct ofp_error *error) {
  /* Copy request data for ofp_error. */
  if (pbuf_copy_with_length(req_pbuf, pbuf, OFP_ERROR_MAX_SIZE) !=
      LAGOPUS_RESULT_OK) {
    TEST_FAIL_MESSAGE("pbuf_copy_with_length error.");
  }
  error->req = req_pbuf;

  return ofp_packet_out_handle(channel, pbuf, xid_header, error);
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
test_ofp_packet_out_handle_normal_pattern_no_actions(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  struct ofp_header *header = NULL;
  struct ofp_packet_out *packet_out = NULL;
  struct pbuf *req_data;
  char req[] = "04 0d 00 18 00 00 00 10 "
               "ff ff ff ff 00 00 00 0e 00 00 00 00 00 00 00 00";

  create_packet(req, &req_data);
  ret = check_packet_parse_with_dequeue(
          ofp_packet_out_handle_wrap, req,
          (void **)&eventq_data);
  TEST_ASSERT_EQUAL_MESSAGE(
    LAGOPUS_RESULT_OK, ret,
    "ofp_packet_out_handle(normal) error.");

  /* get result */
  TEST_ASSERT_EQUAL_MESSAGE(
    LAGOPUS_RESULT_OK, ret,
    "lagopus_bbq_get error.");

  /* check results. */
  packet_out = &(eventq_data->packet_out.ofp_packet_out);
  header = &(packet_out->header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_EVENTQ_PACKET_OUT,
                            eventq_data->type, "type error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x04,       header->version,       "version error.");
  TEST_ASSERT_EQUAL_MESSAGE(OFPT_PACKET_OUT, header->type,     "type error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x0018,     header->length,        "length error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x00000010, header->xid,           "xid error.");
  TEST_ASSERT_EQUAL_MESSAGE(0xffffffff, packet_out->buffer_id,
                            "buffer_id error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x0000000e, packet_out->in_port,   "in_port error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x0000, packet_out->actions_len,
                            "actions_len error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x00, eventq_data->packet_out.channel_id,
                            "channel_id error");
  TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(req_data->data,
                                      eventq_data->packet_out.req->data,
                                      req_data->plen),
                            "request data error.");

  /* free */
  pbuf_free(req_data);
  ofp_packet_out_free(eventq_data);
}

void
test_ofp_packet_out_handle_normal_pattern_no_data(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  uint32_t out_port[] = {0x0000000d, 0x0000010d};
  struct ofp_header *header = NULL;
  struct ofp_packet_out *packet_out = NULL;
  struct action *action;
  struct ofp_action_output *output;
  int i = 0;
  int action_num = 2;

  ret = check_packet_parse_with_dequeue(
          ofp_packet_out_handle_wrap,
          "04 0d 00 38 00 00 00 10 "
          "ff ff ff ff 00 00 00 0e 00 20 " "00 00 00 00 00 00 "
          "00 00 00 10 00 00 00 0d 00 01 " "00 00 00 00 00 00 "
          "00 00 00 10 00 00 01 0d 00 02 " "00 00 00 00 00 00 ",
          (void **)&eventq_data);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_packet_out_handle(normal) error.");

  /* get result */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "lagopus_bbq_get error.");

  /* check header. */
  packet_out = &(eventq_data->packet_out.ofp_packet_out);
  header = &(packet_out->header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_EVENTQ_PACKET_OUT,
                            eventq_data->type, "type error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x04,       header->version,       "version error.");
  TEST_ASSERT_EQUAL_MESSAGE(OFPT_PACKET_OUT, header->type,     "type error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x0038,     header->length,        "length error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x00000010, header->xid,           "xid error.");
  /* check packet_out */
  TEST_ASSERT_EQUAL_MESSAGE(0xffffffff, packet_out->buffer_id,
                            "buffer_id error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x0000000e, packet_out->in_port,   "in_port error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x0020, packet_out->actions_len,
                            "actions_len error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x01, eventq_data->packet_out.channel_id,
                            "channel_id error");
  /* check action_list */
  TAILQ_FOREACH(action, &eventq_data->packet_out.action_list, entry) {
    if (i < action_num) {
      output = (struct ofp_action_output *) &action->ofpat;
      TEST_ASSERT_EQUAL_MESSAGE(OFPAT_OUTPUT, output->type,    "type error.");
      TEST_ASSERT_EQUAL_MESSAGE(0x0010,       output->len,     "len error.");
      TEST_ASSERT_EQUAL_MESSAGE(out_port[i],  output->port,    "port error.");
      TEST_ASSERT_EQUAL_MESSAGE(i + 1,        output->max_len, "max_len error.");
    }
    i++;
  }
  TEST_ASSERT_EQUAL_MESSAGE(2, i, "num of action_list error.");
  /* check data */
  TEST_ASSERT_NULL(eventq_data->packet_out.data);
  /* free */
  ofp_packet_out_free(eventq_data);
}

void
test_ofp_packet_out_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  uint8_t out_data[] = {0x68, 0x6f, 0x67, 0x65};
  uint32_t out_port[] = {0x0000000d, 0x0000010d};
  struct ofp_header *header = NULL;
  struct ofp_packet_out *packet_out = NULL;
  struct action *action;
  struct ofp_action_output *output;
  int i = 0;
  int action_num = 2;

  ret = check_packet_parse_with_dequeue(
          ofp_packet_out_handle_wrap,
          "04 0d 00 3c 00 00 00 10 "
          /* <---------------------> ofp header
           */
          "ff ff ff ff 00 00 00 0e 00 20 00 00 00 00 00 00 "
          /* <----------> buffer_id
           *              <---------> in_port
           *                          <---> actions_len [bytes]
           *                                <---------------> padding
           */
          "00 00 00 10 00 00 00 0d 00 01 00 00 00 00 00 00"
          /* <----------------------> ofp_action_header
           * <----------------------------------------------> ofp_action_output
           * <----> action type (0 -> OFPAT_OUTPUT
           *        <---> action length = 16
           *              <---------> port
           *                          <---> max_len
           *                                <---------------> padding
           */
          "00 00 00 10 00 00 01 0d 00 02 00 00 00 00 00 00"
          /* <----------------------> ofp_action_header
           * <----------------------------------------------> ofp_action_output
           * <----> action type (0 -> OFPAT_OUTPUT
           *        <---> action length = 16
           *              <---------> port
           *                          <---> max_len
           *                                <---------------> padding
           */
          "68 6f 67 65",
          /* <----------> data
           *              length of the data is 3c - 8 - 20 = 4
           */
          (void **)&eventq_data);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_packet_out_handle(normal) error.");

  /* get result */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "lagopus_bbq_get error.");

  /* check header. */
  packet_out = &(eventq_data->packet_out.ofp_packet_out);
  header = &(packet_out->header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_EVENTQ_PACKET_OUT,
                            eventq_data->type, "type error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x04,       header->version,       "version error.");
  TEST_ASSERT_EQUAL_MESSAGE(OFPT_PACKET_OUT, header->type,     "type error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x003c,     header->length,        "length error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x00000010, header->xid,           "xid error.");
  /* check packet_out */
  TEST_ASSERT_EQUAL_MESSAGE(0xffffffff, packet_out->buffer_id,
                            "buffer_id error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x0000000e, packet_out->in_port,   "in_port error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x0020, packet_out->actions_len,
                            "actions_len error.");
  /* check action_list */
  TAILQ_FOREACH(action, &eventq_data->packet_out.action_list, entry) {
    if (i < action_num) {
      output = (struct ofp_action_output *) &action->ofpat;
      TEST_ASSERT_EQUAL_MESSAGE(OFPAT_OUTPUT, output->type,    "type error.");
      TEST_ASSERT_EQUAL_MESSAGE(0x0010,       output->len,     "len error.");
      TEST_ASSERT_EQUAL_MESSAGE(out_port[i],  output->port,    "port error.");
      TEST_ASSERT_EQUAL_MESSAGE(i + 1,        output->max_len, "max_len error.");
    }
    i++;
  }
  TEST_ASSERT_EQUAL_MESSAGE(2, i, "num of action_list error.");
  /* check data */
  TEST_ASSERT_NOT_NULL(eventq_data->packet_out.data);
  TEST_ASSERT_EQUAL_MESSAGE(0,
                            memcmp((uint8_t *)eventq_data->packet_out.data->getp,
                                   (uint8_t *)&out_data, 4),
                            "data compare error.");
  TEST_ASSERT_EQUAL_MESSAGE(4, eventq_data->packet_out.data->plen,
                            "packet length error.");
  TEST_ASSERT_EQUAL_MESSAGE(4,
                            eventq_data->packet_out.data->putp -
                            eventq_data->packet_out.data->getp,
                            "packet data length error.");

  /* free */
  ofp_packet_out_free(eventq_data);
}

void
test_ofp_packet_out_handle_normal_buffer_id(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  uint32_t out_port[] = {0x0000000d, 0x0000010d};
  struct ofp_header *header = NULL;
  struct ofp_packet_out *packet_out = NULL;
  struct action *action;
  struct ofp_action_output *output;
  int i = 0;
  int action_num = 2;

  ret = check_packet_parse_with_dequeue(
          ofp_packet_out_handle_wrap,
          "04 0d 00 38 00 00 00 10 "
          /* <---------------------> ofp header
           */
          "00 00 00 01 00 00 00 0e 00 20 00 00 00 00 00 00 "
          /* <----------> buffer_id
           *              <---------> in_port
           *                          <---> actions_len [bytes]
           *                                <---------------> padding
           */
          "00 00 00 10 00 00 00 0d 00 01 00 00 00 00 00 00"
          /* <----------------------> ofp_action_header
           * <----------------------------------------------> ofp_action_output
           * <----> action type (0 -> OFPAT_OUTPUT
           *        <---> action length = 16
           *              <---------> port
           *                          <---> max_len
           *                                <---------------> padding
           */
          "00 00 00 10 00 00 01 0d 00 02 00 00 00 00 00 00",
          /* <----------------------> ofp_action_header
           * <----------------------------------------------> ofp_action_output
           * <----> action type (0 -> OFPAT_OUTPUT
           *        <---> action length = 16
           *              <---------> port
           *                          <---> max_len
           *                                <---------------> padding
           */
          (void **)&eventq_data);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_packet_out_handle(normal) error.");

  /* get result */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "lagopus_bbq_get error.");

  /* check header. */
  packet_out = &(eventq_data->packet_out.ofp_packet_out);
  header = &(packet_out->header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_EVENTQ_PACKET_OUT,
                            eventq_data->type, "type error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x04,       header->version,       "version error.");
  TEST_ASSERT_EQUAL_MESSAGE(OFPT_PACKET_OUT, header->type,     "type error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x0038,     header->length,        "length error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x00000010, header->xid,           "xid error.");
  /* check packet_out */
  TEST_ASSERT_EQUAL_MESSAGE(0x00000001, packet_out->buffer_id,
                            "buffer_id error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x0000000e, packet_out->in_port,   "in_port error.");
  TEST_ASSERT_EQUAL_MESSAGE(0x0020, packet_out->actions_len,
                            "actions_len error.");
  /* check action_list */
  TAILQ_FOREACH(action, &eventq_data->packet_out.action_list, entry) {
    if (i < action_num) {
      output = (struct ofp_action_output *) &action->ofpat;
      TEST_ASSERT_EQUAL_MESSAGE(OFPAT_OUTPUT, output->type,    "type error.");
      TEST_ASSERT_EQUAL_MESSAGE(0x0010,       output->len,     "len error.");
      TEST_ASSERT_EQUAL_MESSAGE(out_port[i],  output->port,    "port error.");
      TEST_ASSERT_EQUAL_MESSAGE(i + 1,        output->max_len, "max_len error.");
    }
    i++;
  }
  TEST_ASSERT_EQUAL_MESSAGE(2, i, "num of action_list error.");
  /* check data */
  TEST_ASSERT_NULL(eventq_data->packet_out.data);

  /* free */
  ofp_packet_out_free(eventq_data);
}


void
s_eventq_data_free(struct eventq_data *eventq_data) {
  if (eventq_data != NULL && eventq_data->free != NULL) {
    eventq_data->free(eventq_data);
  } else {
    free(eventq_data);
  }
}

void
test_ofp_packet_out_handle_invalid_no_body(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  struct ofp_error expected_error = {0, 0, {NULL}};
  /* no body.
   * sizeof struct ofp_packet_out is 24,
   * so 8 is too short
   */
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_with_dequeue_expect_error(
          ofp_packet_out_handle_wrap,
          "04 0d 00 08 00 00 00 10",
          (void **)&eventq_data,
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "no-body error.");
  s_eventq_data_free(eventq_data);
}

void
test_ofp_packet_out_handle_invalid_packet_out_no_body(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid packet_out body. (no in_port, actions_len, pad.) */
  ret = check_packet_parse_with_dequeue_expect_error(
          ofp_packet_out_handle_wrap,
          "04 0d 00 0c 00 00 00 10 "
          "ff ff ff ff",
          (void **)&eventq_data,
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
  s_eventq_data_free(eventq_data);
}

void
test_ofp_packet_out_handle_invalid_packet_out_no_actions_len(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid packet_out body. (no actions_len, pad.) */
  ret = check_packet_parse_with_dequeue_expect_error(
          ofp_packet_out_handle_wrap,
          "04 0d 00 10 00 00 00 10 "
          "ff ff ff ff " "00 00 00 0e",
          (void **)&eventq_data,
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
  s_eventq_data_free(eventq_data);
}

void
test_ofp_packet_out_handle_invalid_packet_out_no_padding(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  /* invalid packet_out body. (no pad.) */
  ret = check_packet_parse_with_dequeue_expect_error(
          ofp_packet_out_handle_wrap,
          "04 0d 00 12 00 00 00 10 "
          "ff ff ff ff " "00 00 00 0e " "00 00",
          (void **)&eventq_data,
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid-body error.");
  s_eventq_data_free(eventq_data);
}

void
test_ofp_packet_out_handle_invalid_packet_out_no_action_header(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
  /* invalid actions_len (no ofp_action_header) */
  ret = check_packet_parse_with_dequeue_expect_error(
          ofp_packet_out_handle_wrap,
          "04 0d 00 18 00 00 00 10 "
          "ff ff ff ff " "00 00 00 0e "
          "00 10 " "00 00 00 00 00 00",
          /* <----> actions_len is 16, but there is no action */
          (void **)&eventq_data,
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid actions_len error.");
  s_eventq_data_free(eventq_data);
}

void
test_ofp_packet_out_handle_invalic_action_unknown_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_ACTION, OFPBAC_BAD_TYPE);

  ret = check_packet_parse_with_dequeue_expect_error(
          ofp_packet_out_handle_wrap,
          "04 0d 00 3c 00 00 00 10 "
          "ff ff ff ff 00 00 00 0e 00 20 00 00 00 00 00 00 "
          "ff fe 00 10 00 00 00 0d 00 01 00 00 00 00 00 00"
          /* <----------------------> ofp_action_header
           * <----------------------------------------------> ofp_action_output
           * <----> action type (fffe -> unknown action type
           *        <---> action length = 16
           *              <---------> port
           *                          <---> max_len
           *                                <---------------> padding
           */
          "00 00 00 10 00 00 01 0d 00 02 00 00 00 00 00 00"
          "68 6f 67 65",
          (void **)&eventq_data,
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "unknown action type error");
  /* free */
  ofp_packet_out_free(eventq_data);
}

void
test_ofp_packet_out_handle_invalic_action_too_long_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_ACTION, OFPBRC_BAD_TYPE);

  ret = check_packet_parse_with_dequeue_expect_error(
          ofp_packet_out_handle_wrap,
          "04 0d 00 3c 00 00 00 10 "
          "ff ff ff ff 00 00 00 0e 00 20 00 00 00 00 00 00 "
          "00 00 00 20 00 00 00 0d 00 01 00 00 00 00 00 00"
          /* <----------------------> ofp_action_header
           * <----------------------------------------------> ofp_action_output
           * <----> action type (0 -> OFPAT_OUTPUT
           *        <---> action length = ff is not valid
           *              <---------> port
           *                          <---> max_len
           *                                <---------------> padding
           */
          "00 00 00 10 00 00 01 0d 00 02 00 00 00 00 00 00"
          "68 6f 67 65",
          (void **)&eventq_data,
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "invalid action length.");
  /* free */
  ofp_packet_out_free(eventq_data);
}

void
test_ofp_packet_out_handle_buffer_id_and_data(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BUFFER_UNKNOWN);

  ret = check_packet_parse_with_dequeue_expect_error(
          ofp_packet_out_handle_wrap,
          "04 0d 00 3c 00 00 00 10 "
          "00 00 00 01 00 00 00 0e 00 20 00 00 00 00 00 00 "
          /* <----------> buffer_id
           *              <---------> in_port
           *                          <---> actions_len [bytes]
           *                                <---------------> padding
           */
          "00 00 00 10 00 00 00 0d 00 01 00 00 00 00 00 00"
          "00 00 00 10 00 00 01 0d 00 02 00 00 00 00 00 00"
          "68 6f 67 65",
          /* <---------> data */
          (void **)&eventq_data,
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "unknown action type error");
  /* free */
  ofp_packet_out_free(eventq_data);
}

void
test_ofp_packet_out_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header header;
  struct ofp_error error;

  ret = ofp_packet_out_handle(NULL, pbuf, &header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_packet_out_handle(channel, NULL, &header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_packet_out_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (xid_header)");

  ret = ofp_packet_out_handle(channel, pbuf, &header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (ofp_error)");
  channel_free(channel);
  pbuf_free(pbuf);
}

/* freeup ofp_handler. (for valgrind warnings.) */
void
test_finalize_ofph(void) {
  ofp_handler_finalize();
}

void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
