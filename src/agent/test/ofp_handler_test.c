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
#include "lagopus_apis.h"
#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_bridge.h"
#include "lagopus/eventq_data.h"
#include "lagopus/ofp_bridgeq_mgr.h"
#include "handler_test_utils.h"
#include "../channel_mgr.h"

#define PUT_TIMEOUT 100LL * 1000LL * 1000LL * 1000LL
#define SHUTDOWN_TIMEOUT 100LL * 1000LL * 1000LL * 1000LL

#define SLEEP_SHORT()  usleep(10000)

static bool s_finalize_ofp_handler = false;
static lagopus_thread_t *th = NULL;

struct eventq_data *
new_eventq_data(void) {
  struct eventq_data *ret;
  ret = (struct eventq_data *)malloc(sizeof(struct eventq_data));
  lagopus_msg_debug(10, "malloc(%p)\n", ret);
  return ret;
}

void
setUp(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;

  s_finalize_ofp_handler = false;
  /*
   * create ofp_handler
   */
  res = ofp_handler_initialize(NULL, &th);
  res = ofp_handler_start();
#if 0
  if (res != LAGOPUS_RESULT_OK) {
    lagopus_perror(res);
    TEST_FAIL_MESSAGE("handler creation error");
    return;
  }
  /*
   * start ofp_handler
   */
  res = ofp_handler_start();
  if (res != LAGOPUS_RESULT_OK) {
    lagopus_perror(res);
    TEST_FAIL_MESSAGE("handler start error");
    return;
  }
  SLEEP_SHORT();
  res = global_state_set(GLOBAL_STATE_STARTED);
  TEST_ASSERT_EQUAL(res, LAGOPUS_RESULT_OK);
#endif
}

void
tearDown(void) {
#if 0
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  res = ofp_handler_shutdown(SHUTDOWN_GRACEFULLY);
  if (res != LAGOPUS_RESULT_OK) {
    lagopus_perror(res);
    TEST_FAIL_MESSAGE("handler shutdown error");
    return;
  }
#endif
  if (s_finalize_ofp_handler == true) {
    ofp_handler_finalize();
  }
  lagopus_msg_debug(10, "========================================\n");
}

/*
 * create & register ofp_bridge
 */
static struct ofp_bridge *
s_register_bridge(uint64_t dpid, lagopus_result_t required) {
  struct ofp_bridge *ofpb = NULL;
  struct ofp_bridgeq *bridgeq;
  const char *bridge_name = "test_bridge01";
  datastore_bridge_info_t info = {0};
  datastore_bridge_queue_info_t q_info =
      {1000LL, 1000LL, 1000LL, 1000LL, 1000LL, 1000LL};
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;

  res = ofp_bridgeq_mgr_bridge_register(dpid,
                                        bridge_name,
                                        &info,
                                        &q_info);
  if (res == LAGOPUS_RESULT_OK) {
    res = ofp_bridgeq_mgr_bridge_lookup(dpid, &bridgeq);
    ofpb = ofp_bridgeq_mgr_bridge_get(bridgeq);
    ofp_bridgeq_mgr_bridgeq_free(bridgeq);
  } else if (ofpb != NULL) {
    /* if register failed, destroy it. */
    ofp_bridge_destroy(ofpb);
    ofpb = NULL;
  }
  TEST_ASSERT_EQUAL_MESSAGE(required, res,
                            "bridge register error");

  return ofpb;
}

static void
s_unregister_bridge(uint64_t dpid, lagopus_result_t required) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  res = ofp_bridgeq_mgr_bridge_unregister(dpid);
  TEST_ASSERT_EQUAL_MESSAGE(required, res,
                            "bridge unregister error");
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
test_creation(void) {
  SLEEP_SHORT();
}

void
test_restart_fail(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;

  SLEEP_SHORT();

  res = ofp_handler_start();
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_ALREADY_EXISTS,
                            res, "restart error");
}

void
test_creation_loop(void) {
  int i;
  for (i = 0; i < 3; i++) {
    lagopus_msg_debug(10, "%d start\n", i);
    SLEEP_SHORT();
    tearDown();
    SLEEP_SHORT();
    setUp();
  }
}

void
test_start_shutdown_loop(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t i;
  struct ofp_bridge *ofpb = NULL;
  for (i = 0; i < 3; i++) {
    SLEEP_SHORT();
    // register
    ofpb = s_register_bridge(i, LAGOPUS_RESULT_OK);
    TEST_ASSERT_NOT_NULL_MESSAGE(ofpb, "ofp_bridge null");
    // shutdown
    lagopus_msg_debug(10, "%lu stop\n", i);
    ofp_handler_shutdown(SHUTDOWN_GRACEFULLY);
    res = lagopus_thread_wait((lagopus_thread_t *) th,
                              SHUTDOWN_TIMEOUT);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "wait error");
    SLEEP_SHORT();
    // create
    lagopus_msg_debug(10, "%lu initialize\n", i);
    res = ofp_handler_initialize(NULL, &th);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "init error");
    SLEEP_SHORT();
    // start
    lagopus_msg_debug(10, "%lu start\n", i);
    res = ofp_handler_start();
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "start error");
  }
}

void
test_start_stop_loop(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t i;
  struct ofp_bridge *ofpb = NULL;
  for (i = 0; i < 3; i++) {
    SLEEP_SHORT();
    // register
    ofpb = s_register_bridge(i, LAGOPUS_RESULT_OK);
    TEST_ASSERT_NOT_NULL_MESSAGE(ofpb, "ofp_bridge null");
    SLEEP_SHORT();
    // cancel
    lagopus_msg_debug(10, "%lu stop\n", i);
    res = ofp_handler_stop();
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "stop error");
    SLEEP_SHORT();
    // create
    lagopus_msg_debug(10, "%lu initialize\n", i);
    res = ofp_handler_initialize(NULL, &th);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "init error");
    SLEEP_SHORT();
    // start
    lagopus_msg_debug(10, "%lu start\n", i);
    res = ofp_handler_start();
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "start error");
  }
}

void
test_register_bridge(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t i;
  struct ofp_bridge *ofpb[3];
  for (i = 0; i < 3; i++) {
    SLEEP_SHORT();
    /* register first, OK. */
    ofpb[i] = s_register_bridge(i, LAGOPUS_RESULT_OK);
    TEST_ASSERT_NOT_NULL_MESSAGE(ofpb[i], "ofp_bridge null");
    /* register second, fail with ALREADY_EXIST */
    s_register_bridge(i, LAGOPUS_RESULT_ALREADY_EXISTS);
  }
  SLEEP_SHORT();
  ofp_handler_shutdown(SHUTDOWN_GRACEFULLY);
  res = lagopus_thread_wait((lagopus_thread_t *) th,
                            SHUTDOWN_TIMEOUT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "wait error");
}

void
test_unregister_bridge(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t i;
  struct ofp_bridge *ofpb[3];
  for (i = 0; i < 3; i++) {
    SLEEP_SHORT();
    /* register first, OK. */
    ofpb[i] = s_register_bridge(i, LAGOPUS_RESULT_OK);
    TEST_ASSERT_NOT_NULL_MESSAGE(ofpb[i], "ofp_bridge null");
  }
  SLEEP_SHORT();
  for (i = 0; i < 3; i++) {
    SLEEP_SHORT();
    /* register first, OK. */
    s_unregister_bridge(i, LAGOPUS_RESULT_OK);
    SLEEP_SHORT();
    /* register again, OK. */
    s_unregister_bridge(i, LAGOPUS_RESULT_OK);
  }
  SLEEP_SHORT();
  ofp_handler_shutdown(SHUTDOWN_GRACEFULLY);
  res = lagopus_thread_wait((lagopus_thread_t *) th,
                            SHUTDOWN_TIMEOUT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "wait error");
}

#define TIME_OUT_COUNTER 100

void
test_put_eventq(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  int i, j = 0;
  struct eventq_data *put;
  struct ofp_bridge *ofpb0 = s_register_bridge(0, LAGOPUS_RESULT_OK);
  struct ofp_bridge *ofpb1 = s_register_bridge(1, LAGOPUS_RESULT_OK);
  struct ofp_bridge *ofpb2 = s_register_bridge(2, LAGOPUS_RESULT_OK);

  for (i = 0; i < 3; i++) {
    lagopus_msg_debug(10, "%d start.\n", i);

    put = (struct eventq_data *)calloc(1, sizeof(struct eventq_data));
    res = lagopus_bbq_put(&(ofpb0->eventq), &put, struct eventq_data *,
                          PUT_TIMEOUT);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "eventq0 put error");
    put = (struct eventq_data *)calloc(1, sizeof(struct eventq_data));
    res = lagopus_bbq_put(&(ofpb2->eventq), &put, struct eventq_data *,
                          PUT_TIMEOUT);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "eventq2 put error");
    put = (struct eventq_data *)calloc(1, sizeof(struct eventq_data));
    res = lagopus_bbq_put(&(ofpb2->eventq), &put, struct eventq_data *,
                          PUT_TIMEOUT);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "eventq2 put error");
    put = (struct eventq_data *)calloc(1, sizeof(struct eventq_data));
    res = lagopus_bbq_put(&(ofpb2->eventq), &put, struct eventq_data *,
                          PUT_TIMEOUT);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "eventq2 put error");

    /* wait */
    while (1) {
      if ((lagopus_bbq_size(&(ofpb0->eventq)) != 0) ||
          (lagopus_bbq_size(&(ofpb1->eventq)) != 0) ||
          (lagopus_bbq_size(&(ofpb2->eventq)) != 0)) {
        if (j < TIME_OUT_COUNTER) {
          SLEEP_SHORT();
          j++;
        } else {
          TEST_FAIL_MESSAGE("TIME OUT.");
          break;
        }
      } else {
        break;
      }
    }

    res = lagopus_bbq_size(&(ofpb0->eventq));
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "eventq0 polling error");
    res = lagopus_bbq_size(&(ofpb1->eventq));
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "eventq1 polling error");
    res = lagopus_bbq_size(&(ofpb2->eventq));
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "eventq2 polling error");
  }
  ofp_handler_shutdown(SHUTDOWN_GRACEFULLY);
  res = lagopus_thread_wait((lagopus_thread_t *) th,
                            SHUTDOWN_TIMEOUT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "wait error");
}

void
test_put_dataq(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  int i, j = 0;
  struct eventq_data *put;
  struct ofp_bridge *ofpb0 = s_register_bridge(0, LAGOPUS_RESULT_OK);
  struct ofp_bridge *ofpb1 = s_register_bridge(1, LAGOPUS_RESULT_OK);
  struct ofp_bridge *ofpb2 = s_register_bridge(2, LAGOPUS_RESULT_OK);

  for (i = 0; i < 3; i++) {
    lagopus_msg_debug(10, "%d start.\n", i);

    put = (struct eventq_data *)calloc(1, sizeof(struct eventq_data));
    res = lagopus_bbq_put(&(ofpb0->dataq), &put, struct eventq_data *,
                          PUT_TIMEOUT);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "dataq0 put error");
    put = (struct eventq_data *)calloc(1, sizeof(struct eventq_data));
    res = lagopus_bbq_put(&(ofpb2->dataq), &put, struct eventq_data *,
                          PUT_TIMEOUT);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "dataq2 put error");
    put = (struct eventq_data *)calloc(1, sizeof(struct eventq_data));
    res = lagopus_bbq_put(&(ofpb2->dataq), &put, struct eventq_data *,
                          PUT_TIMEOUT);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "dataq2 put error");
    put = (struct eventq_data *)calloc(1, sizeof(struct eventq_data));
    res = lagopus_bbq_put(&(ofpb2->dataq), &put, struct eventq_data *,
                          PUT_TIMEOUT);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "dataq2 put error");

    /* wait */
    while (1) {
      if ((lagopus_bbq_size(&(ofpb0->dataq)) != 0) ||
          (lagopus_bbq_size(&(ofpb1->dataq)) != 0) ||
          (lagopus_bbq_size(&(ofpb2->dataq)) != 0)) {
        if (j < TIME_OUT_COUNTER) {
          SLEEP_SHORT();
          j++;
        } else {
          TEST_FAIL_MESSAGE("TIME OUT.");
          break;
        }
      } else {
        break;
      }
    }

    res = lagopus_bbq_size(&(ofpb0->dataq));
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "dataq0 polling error");
    res = lagopus_bbq_size(&(ofpb1->dataq));
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "dataq1 polling error");
    res = lagopus_bbq_size(&(ofpb2->dataq));
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "dataq2 polling error");
  }
  ofp_handler_shutdown(SHUTDOWN_GRACEFULLY);
  res = lagopus_thread_wait((lagopus_thread_t *) th,
                            SHUTDOWN_TIMEOUT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "wait error");
}

void
test_put_event_dataq(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  int i;
  struct eventq_data *put = NULL;
  struct ofp_bridge *ofpb0 = s_register_bridge(0, LAGOPUS_RESULT_OK);
  struct ofp_bridge *ofpb1 = s_register_bridge(1, LAGOPUS_RESULT_OK);
  struct ofp_bridge *ofpb2 = s_register_bridge(2, LAGOPUS_RESULT_OK);

  for (i = 0; i < 3; i++) {
    lagopus_msg_debug(10, "%d start.\n", i);

    res = ofp_handler_event_dataq_put(0, put);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "event_dataq0 put error");

    lagopus_msg_debug(10, "%d put0 ok.\n", i);

    res = ofp_handler_event_dataq_put(2, put);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "event_dataq2 put error");
    res = ofp_handler_event_dataq_put(2, put);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "event_dataq2 put error");
    res = ofp_handler_event_dataq_put(2, put);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "event_dataq2 put error");

    SLEEP_SHORT();

    res = lagopus_bbq_size(&(ofpb0->event_dataq));
    TEST_ASSERT_EQUAL_MESSAGE((i+1), res, "event_dataq0 polling error");
    res = lagopus_bbq_size(&(ofpb1->event_dataq));
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "event_dataq1 polling error");
    res = lagopus_bbq_size(&(ofpb2->event_dataq));
    TEST_ASSERT_EQUAL_MESSAGE((i+1)*3, res, "event_dataq2 polling error");
  }
  ofp_handler_shutdown(SHUTDOWN_GRACEFULLY);
  res = lagopus_thread_wait((lagopus_thread_t *) th,
                            SHUTDOWN_TIMEOUT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "wait error");
}

void
test_put_channelq(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  int i;
  struct channelq_data *put = NULL;
  lagopus_bbq_t *channelq = NULL;
  res = ofp_handler_get_channelq(&channelq);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "get channelq error");

  for (i = 0; i < 3; i++) {
    lagopus_msg_debug(10, "%d start.\n", i);

    res = lagopus_bbq_put(channelq, &put, struct channelq_data *, PUT_TIMEOUT);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "channelq put error");
    res = lagopus_bbq_put(channelq, &put, struct channelq_data *, PUT_TIMEOUT);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "channelq put error");
    res = lagopus_bbq_put(channelq, &put, struct channelq_data *, PUT_TIMEOUT);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "channelq put error");

    SLEEP_SHORT();

    res = lagopus_bbq_size(channelq);
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "channelq polling error");
  }
}

void
test_put_channelq_packet_out(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_bridge *ofpb = NULL;
  struct channel *channel = NULL;
  lagopus_bbq_t *channelq = NULL;
  struct channelq_data *cdata = NULL;
  struct eventq_data *edata = NULL;

  /* get channelq */
  res = ofp_handler_get_channelq(&channelq);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "get channelq error");

  /* create channel */
  channel = create_data_channel();
  TEST_ASSERT_NOT_NULL_MESSAGE(channel, "channel alloc error");
  channel_refs_get(channel);

  /* register bridge */

  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK,
                    ofp_bridgeq_mgr_clear());
  ofpb = s_register_bridge(channel_dpid_get(channel), LAGOPUS_RESULT_OK);

  /* create cdata */
  cdata = (struct channelq_data *)malloc(sizeof(*cdata));
  if (cdata != NULL) {
    cdata->channel = channel;
  } else {
    TEST_FAIL_MESSAGE("cdata alloc error");
  }
  create_packet(
    "04 0d 00 3c 00 00 00 10 "
    "ff ff ff ff 00 00 00 0e 00 20 00 00 00 00 00 00 "
    "00 00 00 10 00 00 00 0d 00 01 00 00 00 00 00 00 "
    "00 00 00 10 00 00 01 0d 00 02 00 00 00 00 00 00 "
    "68 6f 67 65",
    &cdata->pbuf);

  /* put cdata */
  res = lagopus_bbq_put(channelq, &cdata, struct channelq_data *, PUT_TIMEOUT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "channelq put error");

  /* wait for process cdata */
  SLEEP_SHORT();
  res = lagopus_bbq_get(&ofpb->event_dataq, &edata, void *, PUT_TIMEOUT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "process packet_out error");

  /* check queue length */
  res = lagopus_bbq_size(channelq);
  TEST_ASSERT_EQUAL_MESSAGE(0, res, "channelq length error");
  res = lagopus_bbq_size(&(ofpb->event_dataq));
  TEST_ASSERT_EQUAL_MESSAGE(0, res, "event_dataq length error");
  ofp_packet_out_free(edata);

  ofp_handler_shutdown(SHUTDOWN_GRACEFULLY);
  res = lagopus_thread_wait((lagopus_thread_t *) th,
                            SHUTDOWN_TIMEOUT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res, "wait error");
}

#define TIMEOUT 100LL * 1000LL * 1000LL

void
test_ofp_handler_dataq_data_put(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid = 0x01;
  struct eventq_data *data = (struct eventq_data *)
                             calloc(1, sizeof(struct eventq_data));

  /* register_bridge. */
  (void) s_register_bridge(dpid, LAGOPUS_RESULT_OK);

  /* Call func. */
  ret = ofp_handler_dataq_data_put(dpid, &data, TIMEOUT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_handler_dataq_data_put(normal) error.");

  s_unregister_bridge(dpid, LAGOPUS_RESULT_OK);
}

void
test_ofp_handler_eventq_data_put(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid = 0x01;
  struct eventq_data *data = (struct eventq_data *)
                             calloc(1, sizeof(struct eventq_data));

  /* register_bridge. */
  (void) s_register_bridge(dpid, LAGOPUS_RESULT_OK);

  /* Call func. */
  ret = ofp_handler_eventq_data_put(dpid, &data, TIMEOUT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_handler_eventq_data_put(normal) error.");

  s_unregister_bridge(dpid, LAGOPUS_RESULT_OK);
}

void
test_ofp_handler_event_dataq_data_get(void) {
  lagopus_result_t ret= LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid = 0x01;
  struct eventq_data *data = (struct eventq_data *)
                             calloc(1, sizeof(struct eventq_data));
  struct eventq_data *ret_data = NULL;

  /* register_bridge. */
  (void) s_register_bridge(dpid, LAGOPUS_RESULT_OK);

  ret = ofp_handler_event_dataq_put(dpid, data);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "event_dataq put error");

  /* Call func.*/
  ret = ofp_handler_event_dataq_data_get(dpid, &ret_data, TIMEOUT);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_handler_event_dataq_data_get(normal) error.");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, ret_data,
                                "data error.");
  s_unregister_bridge(dpid, LAGOPUS_RESULT_OK);
  free(ret_data);
}

void
test_finalize_ofph(void) {
  /* this test must be executed last. */
  /* ofph will finalize in teardown(). */
  s_finalize_ofp_handler = true;
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
