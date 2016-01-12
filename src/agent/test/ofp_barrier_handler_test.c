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

static struct eventq_data *
create_data(uint8_t t) {
  struct eventq_data *ed;
  uint8_t type = t;
  uint32_t xid = 0x12345678;
  uint32_t channel_id = 0x00;

  ed = (struct eventq_data *) calloc(1, sizeof(struct eventq_data));

  /* data. */
  ed->barrier.type = type;
  ed->barrier.xid = xid;
  ed->barrier.channel_id = channel_id;

  return ed;
}

lagopus_result_t
ofp_barrier_request_handle_wrap(struct channel *channel,
                                struct pbuf *pbuf,
                                struct ofp_header *xid_header,
                                struct ofp_error *error) {
  /* Copy request data for ofp_error. */
  if (pbuf_copy_with_length(req_pbuf, pbuf, OFP_ERROR_MAX_SIZE) !=
      LAGOPUS_RESULT_OK) {
    TEST_FAIL_MESSAGE("pbuf_copy_with_length error.");
  }
  error->req = req_pbuf;

  return ofp_barrier_request_handle(channel, pbuf, xid_header, error);
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
test_ofp_barrier_request_handle_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  struct barrier *barrier = NULL;
  struct pbuf *req_data;
  uint8_t type = OFPT_BARRIER_REQUEST;
  uint32_t xid = 0x12345678;
  uint32_t channel_id = 0x00;
  char req[] = "04 14 00 08 12 34 56 78";

  create_packet(req, &req_data);
  ret = check_packet_parse_with_dequeue(
          ofp_barrier_request_handle_wrap, req,
          (void **)&eventq_data);

  /* get result */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_barrier_request_handle(normal) error.");
  /* check queue_data. */
  barrier = &(eventq_data->barrier);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_EVENTQ_BARRIER_REQUEST,
                            eventq_data->type, "type error.");
  /* check barrier. */
  TEST_ASSERT_EQUAL_MESSAGE(type, barrier->type, "type error.");
  TEST_ASSERT_EQUAL_MESSAGE(xid, barrier->xid, "xid error.");
  TEST_ASSERT_EQUAL_MESSAGE(channel_id, barrier->channel_id,
                            "channel_id error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(req_data->data,
                                      barrier->req->data,
                                      req_data->plen),
                            "request data error.");
  pbuf_free(req_data);
  ofp_barrier_free(eventq_data);
}

void
test_ofp_barrier_request_invalid_length_too_long(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct eventq_data *eventq_data = NULL;
  struct ofp_error expected_error = {0, 0, {NULL}};
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
  ret = check_packet_parse_with_dequeue_expect_error(
          ofp_barrier_request_handle_wrap,
          "04 14 00 09 12 34 56 78 00",
          (void **)&eventq_data,
          &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "over len error.");
}

void
test_ofp_barrier_request_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header header;
  struct ofp_error error;

  ret = ofp_barrier_request_handle(NULL, pbuf, &header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_barrier_request_handle(channel, NULL, &header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_barrier_request_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (xid_header)");

  ret = ofp_barrier_request_handle(channel, pbuf, &header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (ofp_error)");

  channel_free(channel);
  pbuf_free(pbuf);
}

static
lagopus_result_t
ofp_barrier_reply_create_wrap(struct channel *channel,
                              struct pbuf **pbuf,
                              struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct eventq_data *ed;
  (void) xid_header;

  ed = create_data(OFPT_BARRIER_REPLY);

  /* Call func. */
  ret = ofp_barrier_reply_create(channel, &ed->barrier,
                                 pbuf);

  /* after. */
  ofp_barrier_free(ed);

  return ret;
}

void
test_ofp_barrier_reply_create(void) {
  lagopus_result_t ret;
  ret = check_packet_create(ofp_barrier_reply_create_wrap,
                            "04 15 00 08 12 34 56 78");

  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_OK,
                            "ofp_ofp_barrier_reply_create_create error.");
}

void
test_ofp_barrier_reply_create_null(void) {
  lagopus_result_t ret;
  struct barrier barrier;
  struct channel *channel =
    channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf;

  ret = ofp_barrier_reply_create(NULL, &barrier,
                                 &pbuf);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_INVALID_ARGS,
                            "ofp_barrier_reply_create error.");

  ret = ofp_barrier_reply_create(channel, NULL,
                                 &pbuf);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_INVALID_ARGS,
                            "ofp_barrier_reply_create error.");

  ret = ofp_barrier_reply_create(channel, &barrier,
                                 NULL);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_INVALID_ARGS,
                            "ofp_barrier_reply_create error.");
  /* after. */
  channel_free(channel);
}

static lagopus_result_t
ofp_barrier_reply_send_wrap(struct channel *channel,
                            struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct eventq_data *ed;
  uint64_t dpid = 0x10;
  (void) xid_header;

  ed = create_data(OFPT_BARRIER_REPLY);
  ret = ofp_barrier_reply_send(channel, &ed->barrier,
                               dpid);

  /* after. */
  ofp_barrier_free(ed);

  return ret;
}

void
test_ofp_barrier_reply_send(void) {
  lagopus_result_t ret;
  ret = check_packet_send(ofp_barrier_reply_send_wrap);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_barrier_reply_send(normal) error.");
}

void
test_ofp_barrier_reply_send_null(void) {
  lagopus_result_t ret;
  struct eventq_data *ed;
  uint64_t dpid = 0x10;
  struct channel *channel =
    channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);

  ed = create_data(OFPT_BARRIER_REPLY);

  ret = ofp_barrier_reply_send(NULL, &ed->barrier, dpid);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_INVALID_ARGS,
                            "ofp_barrier_reply_send error.");

  ret = ofp_barrier_reply_send(channel, NULL, dpid);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_INVALID_ARGS,
                            "ofp_barrier_reply_send error.");

  /* after. */
  ofp_barrier_free(ed);
  channel_free(channel);
}

void
test_ofp_barrier_reply_handle_null(void) {
  lagopus_result_t ret;
  uint64_t dpid = 0x10;

  ret = ofp_barrier_reply_handle(NULL, dpid);
  TEST_ASSERT_EQUAL_MESSAGE(ret, LAGOPUS_RESULT_INVALID_ARGS,
                            "ofp_barrier_reply_handle error.");
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
