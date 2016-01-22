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
#include "openflow13.h"
#include "handler_test_utils.h"
#include "../channel_mgr.h"
#include "../channel.h"
#include "../ofp_apis.h"

static
bool test_closed = false;

static
struct ofp_async_config test_role_mask;

void
setUp(void) {
}

void
tearDown(void) {
}

static lagopus_result_t
ofp_role_channel_update_find_wrap(struct channel **channels,
                                  struct ofp_header *xid_headers) {
  struct ofp_role_request role_request;
  lagopus_result_t ret;
  uint64_t dpid = 0x01;
  (void) xid_headers;


  /* Call func (update channels[1] : equal -> slave).  */
  role_request.role = OFPCR_ROLE_SLAVE;
  ret = ofp_role_channel_update(channels[1], &role_request, dpid);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_role_channel_update error.");

  /* Call func (update channels[0] : nochange).  */
  role_request.role = OFPCR_ROLE_NOCHANGE;
  ret = ofp_role_channel_update(channels[0], &role_request, dpid);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_role_channel_update error.");

  /* Call func (update channels[0] : equal -> master).  */
  role_request.role = OFPCR_ROLE_MASTER;
  ret = ofp_role_channel_update(channels[0], &role_request, dpid);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_role_channel_update error.");

  /* Call func (channel is NULL). */
  role_request.role = OFPCR_ROLE_SLAVE;
  ret = ofp_role_channel_update(NULL, &role_request, dpid);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_role_channel_update error.");

  /* Call func (role_request is NULL). */
  ret = ofp_role_channel_update(channels[0], NULL, dpid);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_role_channel_update error.");

  return LAGOPUS_RESULT_OK;
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
test_ofp_role_channel_update_find(void) {
  (void) check_use_channels(ofp_role_channel_update_find_wrap);
}

static lagopus_result_t
ofp_role_channel_check_mask_wrap(struct channel **channels,
                                 struct ofp_header *xid_headers) {
  struct ofp_async_config role_mask;
  bool rv;
  int i;
  int j;
  uint8_t reason = 0x01;
  int type_test_num = 3;
  uint8_t type[] = {OFPT_PACKET_IN,
                    OFPT_FLOW_REMOVED,
                    OFPT_PORT_STATUS
                   };
  int role_test_num = 3;
  uint32_t role[] = {OFPCR_ROLE_MASTER,
                     OFPCR_ROLE_EQUAL,
                     OFPCR_ROLE_SLAVE
                    };
  (void) xid_headers;

  for (i = 0; i < type_test_num; i++) {
    for (j = 0; j < role_test_num; j++) {
      /* Call func. (Result true.) */
      channel_role_mask_get(channels[0], &role_mask);
      rv = ofp_role_channel_check_mask(&role_mask, type[i], reason, role[j]);
      TEST_ASSERT_EQUAL_MESSAGE(true, rv,
                                "ofp_role_channel_check_mask error.");
    }
  }

  /* unset mask. */
  for (i = 0; i < 2; i++) {
    role_mask.packet_in_mask[i] = 0x00;
    role_mask.port_status_mask[i] = 0x00;
    role_mask.flow_removed_mask[i] = 0x00;
  }

  for (i = 0; i < type_test_num; i++) {
    for (j = 0; j < role_test_num; j++) {
      /* Call func. (Result false.) */
      rv = ofp_role_channel_check_mask(&role_mask, type[i],
                                       reason, role[j]);
      TEST_ASSERT_EQUAL_MESSAGE(false, rv,
                                "ofp_role_channel_check_mask error.");
    }
  }

  /* set mask packet_in_mask[master, equal] only. */
  for (i = 0; i < 2; i++) {
    role_mask.packet_in_mask[i] = 0x01;
    role_mask.port_status_mask[i] = 0x01;
    role_mask.flow_removed_mask[i] = 0x01;
  }
  role_mask.packet_in_mask[0] = 0x02;

  for (i = 0; i < type_test_num; i++) {
    for (j = 0; j < role_test_num; j++) {
      /* Call func. (Result false.) */
      rv = ofp_role_channel_check_mask(&role_mask, type[i],
                                       reason, role[j]);
      if (i == 0 && (j == 0 || j == 1)) {
        /* packet_in_mask[master, equal] */
        TEST_ASSERT_EQUAL_MESSAGE(true, rv,
                                  "ofp_role_channel_check_mask(packet_in, master) error.");
      } else {
        TEST_ASSERT_EQUAL_MESSAGE(false, rv,
                                  "ofp_role_channel_check_mask error.");
      }
    }
  }

  /* Call func (Arg is NULL). */
  rv = ofp_role_channel_check_mask(NULL, type[0], reason, role[0]);
  TEST_ASSERT_EQUAL_MESSAGE(false, rv,
                            "ofp_role_channel_check_mask error.");

  /* Call func (unsport ofp type). */
  rv = ofp_role_channel_check_mask(&role_mask, OFPT_PACKET_OUT,
                                   reason, role[0]);
  TEST_ASSERT_EQUAL_MESSAGE(false, rv,
                            "ofp_role_channel_check_mask error.");

  /* Call func (unsport role type). */
  rv = ofp_role_channel_check_mask(&role_mask, type[0],
                                   reason, OFPCR_ROLE_NOCHANGE);
  TEST_ASSERT_EQUAL_MESSAGE(false, rv,
                            "ofp_role_channel_check_mask error.");

  return LAGOPUS_RESULT_OK;
}

void
test_ofp_role_channel_check_mask(void) {
  (void) check_use_channels(ofp_role_channel_check_mask_wrap);
}

static lagopus_result_t
ofp_role_channel_write_wrap(struct channel **channels,
                            struct ofp_header *xid_headers,
                            struct pbuf *pbuf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid = 0xabc;
  uint8_t reason = 0x01;
  int i;
  (void) channels;
  (void) xid_headers;

  for (i = 0; i < CHANNEL_MAX_NUM; i++) {
    channel_role_mask_set(channels[i], &test_role_mask);
  }

  ret = ofp_role_channel_write(pbuf, dpid, OFPT_PACKET_IN, reason);

  return ret;
}

void
test_ofp_role_channel_write(void) {
  lagopus_result_t ret;
  int i;

  /* set role mask. */
  for (i = 0; i < 2; i++) {
    test_role_mask.packet_in_mask[i] = 0xff;
    test_role_mask.port_status_mask[i] = 0xff;
    test_role_mask.flow_removed_mask[i] = 0xff;
  }
  ret = check_use_channels_send(ofp_role_channel_write_wrap,
                                "04 10 00 08 00 00 00 64");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_role_channel_write error.");
}

void
test_ofp_role_channel_write_filtering(void) {
  lagopus_result_t ret;
  int i;

  /* set role mask. */
  for (i = 0; i < 2; i++) {
    test_role_mask.packet_in_mask[i] = 0x00;
    test_role_mask.port_status_mask[i] = 0x00;
    test_role_mask.flow_removed_mask[i] = 0x00;
  }
  ret = check_use_channels_send(ofp_role_channel_write_wrap,
                                "04 10 00 08 00 00 00 64");
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_role_channel_write(filtering) error.");
}

static void
ofp_role_check_wrap(struct channel **channels,
                    struct ofp_header *xid_headers,
                    struct pbuf *pbuf) {
  bool ret;
  int i;
  int ofp_type_num = 10;
  uint8_t type[] = {OFPT_SET_CONFIG, OFPT_TABLE_MOD,
                    OFPT_FLOW_MOD, OFPT_GROUP_MOD,
                    OFPT_PORT_MOD, OFPT_METER_MOD,
                    OFPT_PACKET_OUT, OFPT_PACKET_IN,
                    OFPT_FLOW_REMOVED, OFPT_FEATURES_REQUEST
                   };
  (void) pbuf;

  /* role is equal. */
  channel_role_set(channels[0], OFPCR_ROLE_EQUAL);
  for (i = 0; i < ofp_type_num; i++) {
    xid_headers[0].type = type[i];
    ret = ofp_role_check(channels[0], &xid_headers[0]);
    TEST_ASSERT_EQUAL_MESSAGE(true, ret, "ofp_role_check error.");
  }

  /* role is master. */
  channel_role_set(channels[0], OFPCR_ROLE_MASTER);
  for (i = 0; i < ofp_type_num; i++) {
    xid_headers[0].type = type[i];
    ret = ofp_role_check(channels[0], &xid_headers[0]);
    TEST_ASSERT_EQUAL_MESSAGE(true, ret, "ofp_role_check error.");
  }

  /* role is slave. */
  channel_role_set(channels[0], OFPCR_ROLE_SLAVE);
  for (i = 0; i < ofp_type_num; i++) {
    xid_headers[0].type = type[i];
    ret = ofp_role_check(channels[0], &xid_headers[0]);
    if (type[i] == OFPT_FEATURES_REQUEST) {
      TEST_ASSERT_EQUAL_MESSAGE(true, ret, "ofp_role_check error.");
    } else {
      TEST_ASSERT_EQUAL_MESSAGE(false, ret, "ofp_role_check error.");
    }
  }
}

void
test_ofp_role_check(void) {
  data_create(ofp_role_check_wrap, "");
}

static void
ofp_role_check_null_wrap(struct channel **channels,
                         struct ofp_header *xid_headers,
                         struct pbuf *pbuf) {
  bool ret;
  (void) pbuf;

  ret = ofp_role_check(NULL, &xid_headers[0]);
  TEST_ASSERT_EQUAL_MESSAGE(false, ret, "ofp_role_check error.");

  ret = ofp_role_check(channels[0], NULL);
  TEST_ASSERT_EQUAL_MESSAGE(false, ret, "ofp_role_check error.");
}

void
test_ofp_role_check_null(void) {
  data_create(ofp_role_check_null_wrap, "");
}

static void
ofp_role_mp_check_wrap(struct channel **channels,
                       struct ofp_header *xid_headers,
                       struct pbuf *pbuf) {
  bool ret;
  int i;
  int ofp_type_num = 2;
  uint8_t type[] = {OFPMP_TABLE_FEATURES,
                    OFPMP_QUEUE
                   };
  struct ofp_multipart_request multipart;
  (void) xid_headers;

  /* role is equal. */
  channel_role_set(channels[0], OFPCR_ROLE_EQUAL);
  for (i = 0; i < ofp_type_num; i++) {
    multipart.type = type[i];
    ret = ofp_role_mp_check(channels[0], pbuf,
                            &multipart);
    TEST_ASSERT_EQUAL_MESSAGE(true, ret, "ofp_role_mp_check error.");
  }

  /* role is master. */
  channel_role_set(channels[0], OFPCR_ROLE_MASTER);
  for (i = 0; i < ofp_type_num; i++) {
    multipart.type = type[i];
    ret = ofp_role_mp_check(channels[0], pbuf,
                            &multipart);
    TEST_ASSERT_EQUAL_MESSAGE(true, ret, "ofp_role_mp_check error.");
  }

  /* role is slave and body is not empty. */
  channel_role_set(channels[0], OFPCR_ROLE_SLAVE);
  for (i = 0; i < ofp_type_num; i++) {
    multipart.type = type[i];
    ret = ofp_role_mp_check(channels[0], pbuf,
                            &multipart);
    if (type[i] == OFPMP_TABLE_FEATURES) {
      TEST_ASSERT_EQUAL_MESSAGE(false, ret, "ofp_role_mp_check error.");
    } else {
      TEST_ASSERT_EQUAL_MESSAGE(true, ret, "ofp_role_mp_check error.");
    }
  }

  /* role is slave and body is empty. */
  channel_role_set(channels[0], OFPCR_ROLE_SLAVE);
  pbuf->plen = 0;
  for (i = 0; i < ofp_type_num; i++) {
    multipart.type = type[i];
    ret = ofp_role_mp_check(channels[0], pbuf,
                            &multipart);

    TEST_ASSERT_EQUAL_MESSAGE(true, ret, "ofp_role_mp_check error.");
  }
}

void
test_ofp_role_mp_check(void) {
  data_create(ofp_role_mp_check_wrap,
              "00 00 00 00 00 00 00 00");
}

static void
ofp_role_mp_check_null_wrap(struct channel **channels,
                            struct ofp_header *xid_headers,
                            struct pbuf *pbuf) {
  bool ret;
  struct ofp_multipart_request multipart;
  (void) xid_headers;

  ret = ofp_role_mp_check(NULL, pbuf,
                          &multipart);
  TEST_ASSERT_EQUAL_MESSAGE(false, ret, "ofp_role_mp_check error.");

  ret = ofp_role_mp_check(channels[0], NULL,
                          &multipart);
  TEST_ASSERT_EQUAL_MESSAGE(false, ret, "ofp_role_mp_check error.");

  ret = ofp_role_mp_check(channels[0], pbuf, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(false, ret, "ofp_role_mp_check error.");
}

void
test_role_mp_null_check(void) {
  data_create(ofp_role_mp_check_null_wrap, "");
}


static lagopus_result_t
ofp_role_generation_id_check_wrap(struct channel **channels,
                                  struct ofp_header *xid_headers) {
  bool rt;
  uint64_t dpid = 0xabc;
  (void) channels;
  (void) xid_headers;

  /* new generation_id. */
  rt = ofp_role_generation_id_check(dpid, OFPCR_ROLE_SLAVE, 0x10);
  TEST_ASSERT_EQUAL_MESSAGE(true, rt,
                            "ofp_role_generation_id_check error.");

  /* same generation_id. */
  rt = ofp_role_generation_id_check(dpid, OFPCR_ROLE_SLAVE, 0x10);
  TEST_ASSERT_EQUAL_MESSAGE(true, rt,
                            "ofp_role_generation_id_check error.");

  /* small generation_id. */
  rt = ofp_role_generation_id_check(dpid, OFPCR_ROLE_SLAVE, 0x0f);
  TEST_ASSERT_EQUAL_MESSAGE(false, rt,
                            "ofp_role_generation_id_check error.");

  /* role is OFPCR_ROLE_EQUAL (small generation_id). */
  rt = ofp_role_generation_id_check(dpid, OFPCR_ROLE_EQUAL, 0x0f);
  TEST_ASSERT_EQUAL_MESSAGE(true, rt,
                            "ofp_role_generation_id_check error.");

  /* same small generation_id. */
  rt = ofp_role_generation_id_check(dpid, OFPCR_ROLE_SLAVE, 0x0f);
  TEST_ASSERT_EQUAL_MESSAGE(false, rt,
                            "ofp_role_generation_id_check error.");

  /* large generation_id. */
  rt = ofp_role_generation_id_check(dpid, OFPCR_ROLE_SLAVE, 0x11);
  TEST_ASSERT_EQUAL_MESSAGE(true, rt,
                            "ofp_role_generation_id_check error.");

  /* minus generation_id. */
  rt = ofp_role_generation_id_check(dpid, OFPCR_ROLE_SLAVE, 0xfffffffffffffffe);
  TEST_ASSERT_EQUAL_MESSAGE(false, rt,
                            "ofp_role_generation_id_check error.");

  return LAGOPUS_RESULT_OK;
}

void
test_ofp_role_generation_id_check(void) {
  (void) check_use_channels(ofp_role_generation_id_check_wrap);
}

void
test_close(void) {
  test_closed = true;
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
