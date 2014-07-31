/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_ports_alloc(void) {
  struct vector *ports;

  ports = ports_alloc();
  TEST_ASSERT_NOT_NULL_MESSAGE(ports, "ports_alloc error");
}

void
test_port_add(void) {
  struct vector *ports;
  struct port port;
  lagopus_result_t rv;

  ports = ports_alloc();
  TEST_ASSERT_NOT_NULL_MESSAGE(ports, "ports_alloc error");

  port.ofp_port.port_no = 1;
  port.ifindex = 120;
  rv = port_add(ports, &port);

  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK, "port_add error");
}

void
test_port_delete(void) {
  struct vector *ports;
  struct port port;
  lagopus_result_t rv;

  ports = ports_alloc();
  TEST_ASSERT_NOT_NULL_MESSAGE(ports, "ports_alloc error");

  port.ofp_port.port_no = 1;
  port.ifindex = 120;
  rv = port_add(ports, &port);

  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK, "port_add error");

  rv = port_delete(ports, 5);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_NOT_FOUND, "not found error");
  rv = port_delete(ports, 120);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK, "port_delete error");
}

void
test_lagopus_get_port_statistics(void) {
  struct vector *ports;
  struct port port;
  struct ofp_port_stats_request req;
  struct port_stats_list list;
  struct ofp_error error;
  lagopus_result_t rv;

  ports = ports_alloc();
  TEST_ASSERT_NOT_NULL_MESSAGE(ports, "ports_alloc error");

  req.port_no = 1;
  TAILQ_INIT(&list);
  rv = lagopus_get_port_statistics(ports, &req, &list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OFP_ERROR, "result error");

  req.port_no = OFPP_ANY;
  TAILQ_INIT(&list);
  rv = lagopus_get_port_statistics(ports, &req, &list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK, "result error");
  TEST_ASSERT_NULL_MESSAGE(TAILQ_FIRST(&list), "empty list error");

  port.ofp_port.port_no = 1;
  port.ifindex = 120;
  rv = port_add(ports, &port);

  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK, "port_add error");

  req.port_no = OFPP_ANY;
  TAILQ_INIT(&list);
  rv = lagopus_get_port_statistics(ports, &req, &list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK, "result error");
  TEST_ASSERT_NOT_NULL_MESSAGE(TAILQ_FIRST(&list), "list error");

  req.port_no = 1;
  TAILQ_INIT(&list);
  rv = lagopus_get_port_statistics(ports, &req, &list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK, "result error");

  req.port_no = 5;
  TAILQ_INIT(&list);
  rv = lagopus_get_port_statistics(ports, &req, &list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OFP_ERROR, "result error");
}

void
test_port_config(void) {
  struct port port;
  struct ofp_port_mod port_mod;
  struct ofp_error error;
  struct dpmgr *dpmgr;
  struct bridge *bridge;
  lagopus_result_t rv;

  dpmgr = dpmgr_alloc();
  TEST_ASSERT_NOT_NULL_MESSAGE(dpmgr, "dpmgr alloc error.");
  rv = dpmgr_bridge_add(dpmgr, "br0", 0);
  TEST_ASSERT_EQUAL_MESSAGE(rv, LAGOPUS_RESULT_OK,
                            "bridge add error");
  memset(&port, 0, sizeof(port));
  port.ofp_port.port_no = 1;
  port.ifindex = 120;
  port.ofp_port.hw_addr[0] = 0xe0;
  port.ofp_port.hw_addr[1] = 0x4d;
  port.ofp_port.hw_addr[2] = 0xff;
  port.ofp_port.hw_addr[3] = 0x00;
  port.ofp_port.hw_addr[4] = 0x10;
  port.ofp_port.hw_addr[5] = 0x04;
  dpmgr_port_add(dpmgr, &port);
  dpmgr_bridge_port_add(dpmgr, "br0", 120, 1);

  bridge = dpmgr_bridge_lookup(dpmgr, "br0");
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
