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

#include "openflow.h"

#include "lagopus_apis.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/dp_apis.h"

void
setUp(void) {
  dp_api_init();
}

void
tearDown(void) {
  dp_api_fini();
}

void
test_dp_interface_free(void) {
  struct interface *ifp;
  lagopus_result_t rv;

  ifp = dp_interface_alloc();
  TEST_ASSERT_NOT_NULL(ifp);
  rv = dp_interface_free(ifp);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_free(NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
}

void
test_interface_create(void) {
  lagopus_result_t rv;

  rv = dp_interface_create("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_create("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_ALREADY_EXISTS);
  rv = dp_interface_create(NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
}

void
test_interface_destroy(void) {
  lagopus_result_t rv;

  rv = dp_interface_create("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_destroy("test2");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_interface_destroy("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_destroy(NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
}

void
test_interface_info_set(void) {
  datastore_interface_info_t info;
  lagopus_result_t rv;

  rv = dp_interface_create("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  memset(&info, 0, sizeof(info));
  info.type = DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY;
  info.eth_dpdk_phy.device = strdup("ffff:ff:ff.f");
  info.eth_dpdk_phy.ip_addr = strdup("192.168.0.1");
  rv = dp_interface_info_set("test", &info);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
  rv = dp_interface_info_set("test", NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  info.type = DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK;
  info.eth_rawsock.device = strdup("lo00");
  info.eth_rawsock.ip_addr = strdup("192.168.0.1");
  rv = dp_interface_info_set("test", &info);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_POSIX_API_ERROR);
  rv = dp_interface_info_set("test", NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  info.type = DATASTORE_INTERFACE_TYPE_VXLAN;
  info.vxlan.dst_addr = strdup("192.168.0.1");
  info.vxlan.src_addr = strdup("192.168.0.1");
  info.vxlan.ip_addr = strdup("192.168.0.1");
  rv = dp_interface_info_set("test", &info);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_info_set("test", NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  info.type = DATASTORE_INTERFACE_TYPE_UNKNOWN;
  rv = dp_interface_info_set("test", &info);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_info_set("test", NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  info.type = 0xffff;
  rv = dp_interface_info_set("test", &info);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);

  rv = dp_interface_destroy("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  info.type = 0xffff;
  rv = dp_interface_info_set("test", &info);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
}

void
test_dp_interface_start_stop(void) {
  lagopus_result_t rv;

  rv = dp_interface_create("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_start("test2");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_interface_start("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_stop(NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
  rv = dp_interface_stop("test2");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_interface_stop("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_dp_interface_stats(void) {
  datastore_interface_stats_t stats;
  lagopus_result_t rv;

  rv = dp_interface_create("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_stats_get("test", &stats);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_stats_get("test2", &stats);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = dp_interface_stats_clear("test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_interface_stats_clear("test2");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
}
