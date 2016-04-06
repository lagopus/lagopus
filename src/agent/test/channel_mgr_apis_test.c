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
#include "lagopus/port.h"
#include "lagopus/flowdb.h"
#include "lagopus/dp_apis.h"
#include "../channel.h"
#include "../channel_mgr.h"


static uint64_t dpid = 0xabc;
static datastore_interface_info_t interface_info;
static datastore_bridge_info_t bridge_info;
static const char *bridge_name = "test_bridge01";
static const char *port_name = "test_port01";
static const char *interface_name = "test_if01";

void
setUp(void) {

  /* Event manager allocation. */
  /* init datapath. */
  TEST_ASSERT_EQUAL(dp_api_init(), LAGOPUS_RESULT_OK);

  /* interface. */
  interface_info.type = DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK;
  interface_info.eth_rawsock.port_number = 0;
  interface_info.eth_dpdk_phy.device = strdup("eth0");
  if (interface_info.eth_dpdk_phy.device == NULL) {
    TEST_FAIL_MESSAGE("device is NULL.");
  }
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK,
                    dp_interface_create(interface_name));
  dp_interface_info_set(interface_name, &interface_info);

  /* port. */
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK,
                    dp_port_create(port_name));
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK,
                    dp_port_interface_set(port_name, interface_name));

  /* bridge. */
  bridge_info.dpid = dpid;
  bridge_info.fail_mode = DATASTORE_BRIDGE_FAIL_MODE_SECURE;
  bridge_info.max_buffered_packets = UINT32_MAX;
  bridge_info.max_ports = UINT16_MAX;
  bridge_info.max_tables = UINT8_MAX;
  bridge_info.max_flows = UINT32_MAX;
  bridge_info.capabilities = UINT64_MAX;
  bridge_info.action_types = UINT64_MAX;
  bridge_info.instruction_types = UINT64_MAX;
  bridge_info.reserved_port_types = UINT64_MAX;
  bridge_info.group_types = UINT64_MAX;
  bridge_info.group_capabilities = UINT64_MAX;
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK,
                    dp_bridge_create(bridge_name, &bridge_info));
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK,
                    dp_bridge_port_set(bridge_name, port_name, 0));
}

void
tearDown(void) {

  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK,
                    dp_bridge_destroy(bridge_name));
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK,
                    dp_port_destroy(port_name));
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK,
                    dp_interface_destroy(interface_name));

  dp_api_fini();
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
test_channel_mgr_channel_create(void) {
  lagopus_result_t ret;
  lagopus_ip_address_t *addr4 = NULL;

  lagopus_ip_address_create("127.0.0.1", true, &addr4);
  ret = channel_mgr_channel_create("channel1", addr4, 10032, NULL, 0,
                                   DATASTORE_CHANNEL_PROTOCOL_TCP);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = channel_mgr_channel_start("channel1");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);

  ret = channel_mgr_channel_destroy("channel1");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  lagopus_ip_address_destroy(addr4);
}

void
test_channel_mgr_apis_fail(void) {
  lagopus_result_t ret;
  struct channel *chan = NULL;

  ret = channel_mgr_channel_lookup_by_name("hoge", &chan);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, ret);
  TEST_ASSERT_EQUAL(NULL, chan);
}

void
test_channel_mgr_apis(void) {
  lagopus_ip_address_t *addr4 = NULL, *laddr4 = NULL;
  struct channel *chan = NULL;
  lagopus_result_t ret;
  uint16_t sport;
  datastore_controller_role_t role;
  datastore_channel_status_t status;
  uint8_t version;

  lagopus_ip_address_create("127.0.0.1", true, &addr4);

  ret = channel_mgr_channel_create("channel1", addr4, 10032, addr4, 20032,
                                   DATASTORE_CHANNEL_PROTOCOL_TCP);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  ret = channel_mgr_channel_lookup_by_name("channel1", &chan);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = channel_mgr_controller_set("channel1", DATASTORE_CONTROLLER_ROLE_MASTER,
                                   DATASTORE_CONTROLLER_CONNECTION_TYPE_AUXILIARY);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = channel_mgr_channel_local_port_get("channel1", &sport);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_EQUAL(20032, sport);

  ret = channel_mgr_channel_local_addr_get("channel1", &laddr4);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_EQUAL(true, lagopus_ip_address_equals(addr4, laddr4));

  ret = channel_mgr_channel_connection_status_get("channel1", &status);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_EQUAL(DATASTORE_CHANNEL_DISONNECTED, status);

  ret = channel_mgr_channel_role_get("channel1", &role);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_EQUAL(DATASTORE_CONTROLLER_ROLE_MASTER, role);

  ret = channel_mgr_ofp_version_get("channel1", &version);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_EQUAL(4, version);

  ret = channel_mgr_channel_dpid_set("channel1", (uint64_t) dpid);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = channel_mgr_channel_dpid_unset("channel1");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = channel_mgr_channel_dpid_set("channel1", (uint64_t) dpid);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = channel_mgr_channel_start("channel1");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = channel_mgr_channel_connection_status_get("channel1", &status);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = channel_mgr_channel_dpid_unset("channel1");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = channel_mgr_channel_stop("channel1");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = channel_mgr_channel_destroy("channel1");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = channel_mgr_channel_lookup_by_name("channel1", &chan);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, ret);

  lagopus_ip_address_destroy(addr4);
  lagopus_ip_address_destroy(laddr4);
}
void
test_epilogue(void) {
  lagopus_result_t r;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();
}
