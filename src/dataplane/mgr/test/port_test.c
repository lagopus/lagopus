/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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

#include "openflow.h"

#include "lagopus_apis.h"
#include "lagopus/dpmgr.h"
#include "lagopus/port.h"
#include "lagopus/vector.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/dp_apis.h"
#include "lagopus/datastore/bridge.h"

void
setUp(void) {
  TEST_ASSERT_EQUAL(dp_api_init(), LAGOPUS_RESULT_OK);
}

void
tearDown(void) {
  dp_api_fini();
}

void
test_ports_alloc(void) {
  struct vector *ports;

  ports = ports_alloc();
  TEST_ASSERT_NOT_NULL_MESSAGE(ports, "ports_alloc error");
  ports_free(ports);
}

void
test_dp_port_create(void) {
  lagopus_result_t rv;

  rv = dp_port_create("port0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_port_create("port0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_ALREADY_EXISTS);
  dp_port_destroy("port0");
  dp_port_destroy("port0");
}

void
test_dp_port_interface_set_unset(void) {
  lagopus_result_t rv;

  rv = dp_interface_create("if0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_port_create("port0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_port_interface_set("port1", "if1");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_port_interface_set("port1", "if0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_port_interface_set("port0", "if1");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_port_interface_set("port0", "if0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_NOT_EQUAL(dp_port_lookup(0), NULL);
  rv = dp_port_interface_unset("port1");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_port_interface_unset("port0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_dp_port_count(void) {
  lagopus_result_t rv;

  TEST_ASSERT_EQUAL(dp_port_count(), 0);
  rv = dp_port_create("port0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_count(), 0);
  rv = dp_interface_create("if0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_port_interface_set("port0", "if0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_count(), 1);
  rv = dp_port_destroy("port0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_count(), 0);
}

void
test_dp_port_start_stop(void) {
  lagopus_result_t rv;

  rv = dp_interface_create("if0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_port_create("port0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_port_start("port0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
  rv = dp_port_interface_set("port0", "if0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_port_start("port0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_port_start("port1");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_port_stop("port1");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_port_stop("port0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_dp_port_config(void) {
  lagopus_result_t rv;
  uint32_t config;

  rv = dp_port_create("port0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_port_config_get("port0", &config);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_port_config_get("port1", &config);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
}

void
test_dp_port_state(void) {
  lagopus_result_t rv;
  uint32_t state;

  rv = dp_port_create("port0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_port_state_get("port0", &state);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_port_state_get("port1", &state);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
}

void
test_dp_port_stats(void) {
  datastore_port_stats_t stats;
  lagopus_result_t rv;

  rv = dp_port_create("port0");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_port_stats_get("port0", &stats);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_port_stats_get("port1", &stats);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
}

void
test_lagopus_get_port_statistics(void) {
  datastore_bridge_info_t info;
  struct bridge *bridge;
  struct ofp_port_stats_request req;
  struct port_stats_list list;
  struct ofp_error error;
  lagopus_result_t rv;

  memset(&info, 0, sizeof(info));
  info.fail_mode = DATASTORE_BRIDGE_FAIL_MODE_SECURE;
  rv = dp_bridge_create("br0", &info);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK,
                            "bridge add error");

  bridge = dp_bridge_lookup("br0");
  TEST_ASSERT_NOT_NULL_MESSAGE(bridge, "dp_bridge_lookup error");
  req.port_no = 1;
  TAILQ_INIT(&list);
  rv = lagopus_get_port_statistics(bridge->ports, &req, &list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OFP_ERROR, "result error");

  req.port_no = OFPP_ANY;
  TAILQ_INIT(&list);
  rv = lagopus_get_port_statistics(bridge->ports, &req, &list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK, "result error");
  TEST_ASSERT_NULL_MESSAGE(TAILQ_FIRST(&list), "empty list error");

  TEST_ASSERT_EQUAL(dp_port_create("port1"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_interface_create("interface1"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_interface_set("port1", "interface1"),
                    LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_bridge_port_set("br0", "port1", 1), LAGOPUS_RESULT_OK);

  req.port_no = OFPP_ANY;
  TAILQ_INIT(&list);
  rv = lagopus_get_port_statistics(bridge->ports, &req, &list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK, "result error");
  TEST_ASSERT_NOT_NULL_MESSAGE(TAILQ_FIRST(&list), "list error");

  req.port_no = 1;
  TAILQ_INIT(&list);
  rv = lagopus_get_port_statistics(bridge->ports, &req, &list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK, "result error");
  TEST_ASSERT_NOT_NULL_MESSAGE(TAILQ_FIRST(&list), "list error");

  req.port_no = 5;
  TAILQ_INIT(&list);
  rv = lagopus_get_port_statistics(bridge->ports, &req, &list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OFP_ERROR, "result error");
}

void
test_port_config(void) {
  datastore_bridge_info_t info;
  struct ofp_port_mod port_mod;
  struct ofp_error error;
  struct bridge *bridge;
  lagopus_result_t rv;

  memset(&info, 0, sizeof(info));
  info.fail_mode = DATASTORE_BRIDGE_FAIL_MODE_SECURE;
  TEST_ASSERT_EQUAL(dp_bridge_create("br0", &info), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_create("port1"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_interface_create("interface1"), LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_port_interface_set("port1", "interface1"),
                    LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(dp_bridge_port_set("br0", "port1", 1), LAGOPUS_RESULT_OK);

  bridge = dp_bridge_lookup("br0");
  TEST_ASSERT_NOT_NULL(bridge);
  port_mod.port_no = 1;
  port_mod.hw_addr[0] = 0xe0;
  port_mod.hw_addr[1] = 0x4d;
  port_mod.hw_addr[2] = 0xff;
  port_mod.hw_addr[3] = 0x00;
  port_mod.hw_addr[4] = 0x10;
  port_mod.hw_addr[5] = 0x04;
  port_mod.config = 0;
  port_mod.mask = 0;
  rv = port_config(bridge, &port_mod, &error);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK,
                            "port_config error");
}
