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
#include "../ofp_table_handler.h"
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

static struct table_stats *
table_stats_alloc(void) {
  struct table_stats *table_stats;
  table_stats = (struct table_stats *)
                calloc(1, sizeof(struct table_stats));
  return table_stats;
}

static void
create_data(struct table_stats_list *table_stats_list,
            int test_num) {
  struct table_stats *table_stats;
  int i;

  TAILQ_INIT(table_stats_list);
  for (i = 0; i < test_num; i++) {
    table_stats = table_stats_alloc();
    table_stats->ofp.table_id      = (uint8_t ) (0x01 + i);
    table_stats->ofp.active_count  = (uint8_t) (0x02 + i);
    table_stats->ofp.lookup_count  = (uint64_t) (0x03 + i);
    table_stats->ofp.matched_count = (uint64_t) (0x04 + i);
    TAILQ_INSERT_TAIL(table_stats_list, table_stats, entry);
  }
}

static lagopus_result_t
s_ofp_table_stats_reply_create_wrap(struct channel *channel,
                                    struct pbuf_list **pbuf_list,
                                    struct ofp_header *xid_header) {
  lagopus_result_t ret;
  struct table_stats_list table_stats_list;

  create_data(&table_stats_list, 1);

  ret = ofp_table_stats_reply_create(channel,
                                     pbuf_list,
                                     &table_stats_list,
                                     xid_header);

  /* free. */
  table_stats_list_elem_free(&table_stats_list);

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
test_ofp_table_stats_handle(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = check_packet_parse(ofp_multipart_request_handle_wrap,
                           "04 12 00 10 00 00 00 10 "
                           /* <-----------------... ofp_multipart_request
                            * <---------------------> ofp_header
                            * <> version
                            *    <> type
                            *       <---> length
                            *             <---------> xid
                            */
                           "00 03 00 00 00 00 00 00 ");
  /* <---------------------> ofp_header
   * <---> type = 2
   *       <---> flags
   *             <---------> padding
   */
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_table_stats_request_handle(normal) error.");
}

void
test_ofp_table_stats_handle_error_length0(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  /* invalid body (no pad) */
  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 08 00 00 00 10 "
                                        "00 03 00 00 00 00 00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_table_stats_handle_error_length1(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error;
  ofp_error_set(&expected_error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);

  /* invalid body (over size) */
  ret = check_packet_parse_expect_error(ofp_multipart_request_handle_wrap,
                                        "04 12 00 12 00 00 00 10 "
                                        "00 03 00 00 00 00 00 00 "
                                        "00",
                                        &expected_error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret, "invalid-body error");
}

void
test_ofp_table_stats_handle_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf *pbuf = pbuf_alloc(65535);
  struct ofp_header xid_header;
  struct ofp_error error;

  ret = ofp_table_stats_request_handle(NULL, pbuf, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_table_stats_request_handle(channel, NULL, &xid_header, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_table_stats_request_handle(channel, pbuf, NULL, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (xid_header)");

  ret = ofp_table_stats_request_handle(channel, pbuf, &xid_header, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (ofp_error)");

  channel_free(channel);
  pbuf_free(pbuf);
}

void
test_ofp_table_stats_reply_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *data[1] = {"04 13 00 28 00 00 00 10 "
                         "00 03 00 00 00 00 00 00 "
                         // body
                         "01 "
                         "00 00 00 "
                         "00 00 00 02 "
                         "00 00 00 00 00 00 00 03 "
                         "00 00 00 00 00 00 00 04 "
                        };

  ret = check_pbuf_list_packet_create(s_ofp_table_stats_reply_create_wrap,
                                      data, 1);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "create port 0 error.");
}

void
test_ofp_table_stats_reply_create_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = channel_alloc_ip4addr("127.0.0.1", "1000", 0x01);
  struct pbuf_list *pbuf_list = NULL;
  struct table_stats_list ofp_table_stats_list;
  struct ofp_header xid_header;

  ret = ofp_table_stats_reply_create(NULL, &pbuf_list, &ofp_table_stats_list,
                                     &xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (channel)");

  ret = ofp_table_stats_reply_create(channel, NULL, &ofp_table_stats_list,
                                     &xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (pbuf)");

  ret = ofp_table_stats_reply_create(channel, &pbuf_list, NULL, &xid_header);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (ofp_table_stats_list)");

  ret = ofp_table_stats_reply_create(channel, &pbuf_list, &ofp_table_stats_list,
                                     NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "NULL-check error. (xid_header)");

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
