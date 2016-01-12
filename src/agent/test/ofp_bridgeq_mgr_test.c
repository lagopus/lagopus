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
#include "lagopus/ofp_bridgeq_mgr.h"
#include "openflow.h"
#include "openflow13.h"
#include "handler_test_utils.h"

void
setUp(void) {
  ofp_bridgeq_mgr_initialize(NULL);
}

void
tearDown(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  ret = ofp_bridgeq_mgr_clear();
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bridgeq_mgr_clear error.");
}

void
test_ofp_bridgeq_mgr_bridge(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid = 0x01;
  const char *bridge_name = "test_bridge01";
  char *test_bridge_name = NULL;
  struct ofp_bridgeq *bridgeq = NULL;
  lagopus_qmuxer_poll_t poll = NULL;
  datastore_bridge_info_t info =
      {1LL, 2LL, 3LL, 4LL, 5LL,
       6LL, 7LL, 8LL, 9LL, 10LL,
       11LL, 12LL};
  datastore_bridge_info_t test_info;
  datastore_bridge_queue_info_t q_info =
      {1000LL, 1000LL, 1000LL, 1000LL, 1000LL, 1000LL};

  /* register bridge. */
  ret = ofp_bridgeq_mgr_bridge_register(dpid, bridge_name, &info, &q_info);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bridgeq_mgr_bridge_register error.");

  /* register same bridge. */
  ret = ofp_bridgeq_mgr_bridge_register(dpid, bridge_name, &info, &q_info);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_ALREADY_EXISTS, ret,
                            "ofp_bridgeq_mgr_bridge_register(same bridge) error.");

  /* lookup. */
  ret = ofp_bridgeq_mgr_bridge_lookup(dpid, &bridgeq);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bridgeq_mgr_bridge_lookup error.");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, bridgeq,
                                "bridgeq error.");

  /* get bridge name. */
  ret = ofp_bridgeq_mgr_name_get(dpid, &test_bridge_name);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bridgeq_mgr_name_get error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(bridge_name, test_bridge_name),
                            "bad bridge name.");
  free(test_bridge_name);
  test_bridge_name = NULL;

  /* get bridge name (not found). */
  ret = ofp_bridgeq_mgr_name_get(0x999, &test_bridge_name);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_NOT_FOUND, ret,
                            "ofp_bridgeq_mgr_name_get error.");
  TEST_ASSERT_EQUAL_MESSAGE(NULL, test_bridge_name,
                            "bad bridge name.");

  /* get bridge info. */
  ret = ofp_bridgeq_mgr_info_get(dpid, &test_info);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bridgeq_mgr_info_get error.");
  TEST_ASSERT_EQUAL_MESSAGE(1, ((info.dpid == test_info.dpid) &&
                                (info.fail_mode == test_info.fail_mode) &&
                                (info.max_buffered_packets ==
                                 test_info.max_buffered_packets) &&
                                (info.max_ports == test_info.max_ports) &&
                                (info.max_tables == test_info.max_tables) &&
                                (info.max_flows == test_info.max_flows) &&
                                (info.capabilities == test_info.capabilities) &&
                                (info.action_types == test_info.action_types) &&
                                (info.instruction_types ==
                                 test_info.instruction_types) &&
                                (info.reserved_port_types ==
                                 test_info.reserved_port_types) &&
                                (info.group_types == test_info.group_types) &&
                                (info.group_capabilities ==
                                 test_info.group_capabilities)),
                            "bad bridge info.");

  /* get bridge info (not found). */
  ret = ofp_bridgeq_mgr_info_get(0x999, &test_info);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_NOT_FOUND, ret,
                            "ofp_bridgeq_mgr_info_get error.");

  /* check getter. */
  poll = NULL;
  poll = ofp_bridgeq_mgr_eventq_poll_get(bridgeq);
  TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, poll,
                                "ofp_bridgeq_mgr_eventq_poll_get error.");
  poll = NULL;
  poll = ofp_bridgeq_mgr_dataq_poll_get(bridgeq);
  TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, poll,
                                "ofp_bridgeq_mgr_dataq_poll_get error.");

  /* decrement refs. */
  ofp_bridgeq_mgr_bridgeq_free(bridgeq);

  /* unregister bridge. */
  ret = ofp_bridgeq_mgr_bridge_unregister(dpid);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bridgeq_mgr_bridge_unregister error.");

  /* lookup. */
  bridgeq = NULL;
  ret = ofp_bridgeq_mgr_bridge_lookup(dpid, &bridgeq);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_NOT_FOUND, ret,
                            "ofp_bridgeq_mgr_bridge_lookup error.");
}

void
test_ofp_bridgeq_mgr_bridge_lookup_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid = 0x01;

  /* call func. */
  ret = ofp_bridgeq_mgr_bridge_lookup(dpid, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bridgeq_mgr_bridge_lookup(null) error.");
}

#define MAX_LENGTH 3

void
test_ofp_bridgeq_mgr_bridge_register_orver_length(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  int data_num = MAX_BRIDGES;
  struct {
    uint64_t dpid;
    char name[20];
  } bridges[MAX_BRIDGES + 1];
  datastore_bridge_info_t info = {0};
  datastore_bridge_queue_info_t q_info =
      {1000LL, 1000LL, 1000LL, 1000LL, 1000LL, 1000LL};
  int i;

  /* create data. */
  for (i = 0; i < data_num + 1; i++) {
    bridges[i].dpid = (uint64_t) (i + 1);
    snprintf(bridges[i].name, sizeof(bridges[i].name), "test_bridge%d", i);
  }

  /* register bridge. */
  for (i = 0; i < data_num; i++) {
    ret = ofp_bridgeq_mgr_bridge_register(bridges[i].dpid,
                                          bridges[i].name,
                                          &info,
                                          &q_info);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "ofp_bridgeq_mgr_bridge_registererror.");
  }

  ret = ofp_bridgeq_mgr_bridge_register(bridges[data_num].dpid,
                                        bridges[data_num].name,
                                        &info,
                                        &q_info);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_ITERATION_HALTED, ret,
                            "ofp_bridgeq_mgr_bridge_register(orver length) error.");
}

void
test_ofp_bridgeq_mgr_bridgeqs_to_array_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t i, j;
  uint64_t data_num = MAX_LENGTH;
  struct {
    uint64_t dpid;
    const char *name;
    bool b;
  } bridges[MAX_LENGTH] = {
    {0x01, "test_bridge01", false},
    {0x02, "test_bridge02", false},
    {0x03, "test_bridge02", false}
  };
  struct ofp_bridgeq *bridgeqs[MAX_BRIDGES];
  struct ofp_bridge *bridge;
  uint64_t poll_count = 0;
  uint64_t bridgeq_count = 0;
  lagopus_qmuxer_poll_t *polls =
    (lagopus_qmuxer_poll_t *) calloc(MAX_LENGTH * MAX_BRIDGE_POLLS,
                                     sizeof(lagopus_qmuxer_poll_t));
  lagopus_qmuxer_poll_t *dp_polls =
    (lagopus_qmuxer_poll_t *) calloc(MAX_LENGTH * MAX_BRIDGE_DP_POLLS,
                                     sizeof(lagopus_qmuxer_poll_t));
  datastore_bridge_info_t info = {0};
  datastore_bridge_queue_info_t q_info =
      {1000LL, 1000LL, 1000LL, 1000LL, 1000LL, 1000LL};

  /* register bridge. */
  for (i = 0; i < data_num; i++) {
    ret = ofp_bridgeq_mgr_bridge_register(bridges[i].dpid,
                                          bridges[i].name,
                                          &info,
                                          &q_info);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "ofp_bridgeq_mgr_bridge_register error.");
  }

  /* call func. */
  ret = ofp_bridgeq_mgr_bridgeqs_to_array(bridgeqs, &bridgeq_count, MAX_LENGTH);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bridgeq_mgr_bridgeqs_to_array(normal) error.");
  TEST_ASSERT_EQUAL_MESSAGE(MAX_LENGTH, bridgeq_count,
                            "index error.");
  for (i = 0; i < data_num; i++) {
    for (j = 0; j < data_num; j++) {
      bridge = ofp_bridgeq_mgr_bridge_get(bridgeqs[i]);
      if (bridge->dpid == bridges[j].dpid) {
        /* check duplicate. */
        if (bridges[j].b == false) {
          bridges[j].b = true;
        } else {
          TEST_FAIL_MESSAGE("duplicate error.");
        }
      }
    }
  }
  /* check dpid. */
  for (i = 0; i < data_num; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, bridges[i].b,
                              "dpid error.");
  }

  /* test ofp_bridgeq_mgr_polls_get() */
  poll_count = 0;
  ret = ofp_bridgeq_mgr_polls_get(polls, bridgeqs,
                                  &poll_count, data_num);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bridgeq_mgr_polls_get(normal) error.");
  TEST_ASSERT_EQUAL_MESSAGE(data_num * MAX_BRIDGE_POLLS, poll_count,
                            "poll_count error.");

  for (i = 0; i < data_num * MAX_BRIDGE_POLLS; i++) {
    TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL,
                                  polls[i],
                                  "polls error.");
  }

  /* test ofp_bridgeq_mgr_poll_reset() */
  ofp_bridgeq_mgr_poll_reset(polls, MAX_LENGTH * MAX_BRIDGE_POLLS);
  for (i = 0; i < data_num * MAX_BRIDGE_POLLS; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(NULL,
                              polls[i],
                              "polls error(null).");
  }

  /* test ofp_bridgeq_mgr_dp_polls_get() */
  poll_count = 0;
  ret = ofp_bridgeq_mgr_dp_polls_get(dp_polls, bridgeqs,
                                     &poll_count, data_num);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bridgeq_mgr_dp_polls_get(normal) error.");
  TEST_ASSERT_EQUAL_MESSAGE(data_num * MAX_BRIDGE_DP_POLLS, poll_count,
                            "poll_count(DP) error.");

  for (i = 0; i < data_num * MAX_BRIDGE_DP_POLLS; i++) {
    TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL,
                                  dp_polls[i],
                                  "polls error.");
  }

  /* test ofp_bridgeq_mgr_poll_reset() */
  ofp_bridgeq_mgr_poll_reset(dp_polls, MAX_LENGTH * MAX_BRIDGE_DP_POLLS);
  for (i = 0; i < data_num * MAX_BRIDGE_DP_POLLS; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(NULL,
                              dp_polls[i],
                              "dp_polls error(null).");
  }

  /* test ofp_bridgeq_mgr_bridgeqs_free. */
  /* Call func.*/
  ofp_bridgeq_mgr_bridgeqs_free(bridgeqs, bridgeq_count);

  free(polls);
  free(dp_polls);
}

void
test_ofp_bridgeq_mgr_bridgeqs_to_array_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t count;
  struct ofp_bridgeq *bridgeqs[MAX_BRIDGES];

  /* call func. */
  ret = ofp_bridgeq_mgr_bridgeqs_to_array(bridgeqs, NULL, MAX_LENGTH);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bridgeq_mgr_bridgeqs_to_array(null) error.");

  /* call func. */
  ret = ofp_bridgeq_mgr_bridgeqs_to_array(NULL, &count, MAX_LENGTH);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bridgeq_mgr_bridgeqs_to_array(null) error.");
}

void
test_ofp_bridgeq_mgr_polls_get_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_qmuxer_poll_t polls;
  struct ofp_bridgeq *bridgeqs[MAX_BRIDGES];
  uint64_t count;
  uint64_t bridgeqs_size = 0;

  /* call func. */
  ret = ofp_bridgeq_mgr_polls_get(NULL, bridgeqs, &count, bridgeqs_size);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bridgeq_mgr_polls_get(null) error.");
  /* call func. */
  ret = ofp_bridgeq_mgr_polls_get(&polls, NULL, &count, bridgeqs_size);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bridgeq_mgr_polls_get(null) error.");
  /* call func. */
  ret = ofp_bridgeq_mgr_polls_get(&polls, bridgeqs, NULL, bridgeqs_size);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bridgeq_mgr_polls_get(null) error.");
}

void
test_ofp_bridgeq_mgr_dp_polls_get_null(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_qmuxer_poll_t polls;
  struct ofp_bridgeq *bridgeqs[MAX_BRIDGES];
  uint64_t count;
  uint64_t bridgeqs_size = 0;

  /* call func. */
  ret = ofp_bridgeq_mgr_dp_polls_get(NULL, bridgeqs, &count, bridgeqs_size);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bridgeq_mgr_dp_polls_get(null) error.");
  /* call func. */
  ret = ofp_bridgeq_mgr_dp_polls_get(&polls, NULL, &count, bridgeqs_size);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bridgeq_mgr_dp_polls_get(null) error.");
  /* call func. */
  ret = ofp_bridgeq_mgr_dp_polls_get(&polls, bridgeqs, NULL, bridgeqs_size);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                            "ofp_bridgeq_mgr_dp_polls_get(null) error.");
}

void
test_ofp_bridgeq_mgr_bridgeqs_to_array_unregister(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t i, j, delete_target = 1;
  uint64_t data_num = MAX_LENGTH;
  struct {
    uint64_t dpid;
    const char *name;
    bool b;
  } bridges[MAX_LENGTH] = {
    {0x01, "test_bridge01", false},
    {0x02, "test_bridge02", false},
    {0x03, "test_bridge02", false}
  };
  struct ofp_bridgeq *bridgeqs[MAX_BRIDGES];
  struct ofp_bridge *bridge;
  uint64_t poll_count = 0;
  uint64_t bridgeq_count = 0;
  lagopus_qmuxer_poll_t *polls =
    (lagopus_qmuxer_poll_t *) calloc(MAX_LENGTH * MAX_BRIDGE_POLLS,
                                     sizeof(lagopus_qmuxer_poll_t));
  lagopus_qmuxer_poll_t *dp_polls =
    (lagopus_qmuxer_poll_t *) calloc(MAX_LENGTH * MAX_BRIDGE_DP_POLLS,
                                     sizeof(lagopus_qmuxer_poll_t));
  datastore_bridge_info_t info = {0};
  datastore_bridge_queue_info_t q_info =
      {1000LL, 1000LL, 1000LL, 1000LL, 1000LL, 1000LL};

  /* register bridge. */
  for (i = 0; i < data_num; i++) {
    ret = ofp_bridgeq_mgr_bridge_register(bridges[i].dpid,
                                          bridges[i].name,
                                          &info,
                                          &q_info);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "ofp_bridgeq_mgr_bridge_register error.");
  }

  ret = ofp_bridgeq_mgr_bridgeqs_to_array(bridgeqs, &bridgeq_count, MAX_LENGTH);

  /* delete entry. */
  bridge = ofp_bridgeq_mgr_bridge_get(bridgeqs[delete_target]);
  ofp_bridgeq_mgr_bridge_unregister(bridge->dpid);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bridgeq_mgr_bridgeqs_to_array(normal) error.");
  TEST_ASSERT_EQUAL_MESSAGE(MAX_LENGTH, bridgeq_count,
                            "index error.");

  for (i = 0; i < data_num; i++) {
    for (j = 0; j < data_num; j++) {
      bridge = ofp_bridgeq_mgr_bridge_get(bridgeqs[i]);
      if (bridge->dpid == bridges[j].dpid) {
        /* check duplicate. */
        if (bridges[j].b == false) {
          bridges[j].b = true;
        } else {
          TEST_FAIL_MESSAGE("duplicate error.");
        }
      }
    }
  }
  /* check dpid. */
  for (i = 0; i < data_num; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(true, bridges[i].b,
                              "dpid error.");
  }

  /* test ofp_bridgeq_mgr_polls_get() */
  poll_count = 0;
  ret = ofp_bridgeq_mgr_polls_get(polls, bridgeqs,
                                  &poll_count, data_num);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bridgeq_mgr_polls_get(normal) error.");
  TEST_ASSERT_EQUAL_MESSAGE(data_num * MAX_BRIDGE_POLLS, poll_count,
                            "poll_count error.");
  for (i = 0; i < data_num * MAX_BRIDGE_POLLS; i++) {
    TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL,
                                  polls[i],
                                  "polls error.");
  }

  /* test ofp_bridgeq_mgr_dp_polls_get() */
  poll_count = 0;
  ret = ofp_bridgeq_mgr_dp_polls_get(dp_polls, bridgeqs,
                                     &poll_count, data_num);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_bridgeq_mgr_dp_polls_get(normal) error.");
  TEST_ASSERT_EQUAL_MESSAGE(data_num * MAX_BRIDGE_DP_POLLS, poll_count,
                            "poll_count(DP) error.");
  for (i = 0; i < data_num * MAX_BRIDGE_DP_POLLS; i++) {
    TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL,
                                  dp_polls[i],
                                  "polls error.");
  }

  /* test ofp_bridgeq_mgr_bridgeqs_free. */
  /* Call func.*/
  ofp_bridgeq_mgr_bridgeqs_free(bridgeqs, bridgeq_count);

  free(polls);
  free(dp_polls);
}
