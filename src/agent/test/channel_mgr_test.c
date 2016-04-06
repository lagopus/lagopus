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


int s4 = -1, s6 = -1;
static datastore_interface_info_t interface_info;
static datastore_bridge_info_t bridge_info;
static bool run;
static uint64_t dpid = 0xabc;
static const char *bridge_name = "test_bridge01";
static const char *port_name = "test_port01";
static const char *interface_name = "test_if01";

#define N_CALLOUT_WORKERS	1

void
test_prologue(void) {
  lagopus_result_t r;
  const char *argv0 =
      ((IS_VALID_STRING(lagopus_get_command_name()) == true) ?
       lagopus_get_command_name() : "callout_test");
  const char * const argv[] = {
    argv0, NULL
  };

  (void)lagopus_mainloop_set_callout_workers_number(N_CALLOUT_WORKERS);
  r = lagopus_mainloop_with_callout(1, argv, NULL, NULL,
                                    false, false, true);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
}

void
setUp(void) {
  struct sockaddr_storage so = {0,0,{0}};
  struct sockaddr_in *sin = (struct sockaddr_in *)&so;
  struct sockaddr_in6 *sin6;

  if (s4 != -1) {
    return;
  }

  sin->sin_family = AF_INET;
  sin->sin_port = htons(10022);
  sin->sin_addr.s_addr = INADDR_ANY;

  s4 = socket(AF_INET, SOCK_STREAM, 0);
  if (s4 < 0) {
    perror("socket");
    exit(1);
  }

  if (bind(s4, (struct sockaddr *) sin, sizeof(*sin)) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(s4, 5) < 0) {
    perror("listen");
    exit(1);
  }

  sin6 = (struct sockaddr_in6 *)&so;
  sin6->sin6_family = AF_INET6;
  sin6->sin6_port = htons(10023);
  sin6->sin6_addr = in6addr_any;

  s6 = socket(AF_INET6, SOCK_STREAM, 0);
  if (s6 < 0) {
    perror("socket");
    exit(1);
  }

  if (bind(s6, (struct sockaddr *) sin6, sizeof(*sin6)) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(s6, 5) < 0) {
    perror("listen");
    exit(1);
  }

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


  printf("setup end\n");
}
void
tearDown(void) {
}


static lagopus_result_t
channel_count(struct channel *chan, void *val) {
  (void) chan;
  (*(int *) val)++;

  return LAGOPUS_RESULT_OK;
}

void
test_channel_tcp(void) {
  int sock4, sock6;
  socklen_t size;
  struct sockaddr_in sin;
  lagopus_ip_address_t *addr4 = NULL;
  lagopus_ip_address_t *addr6 = NULL;
  struct channel_list  *chan_list ;
  struct channel *chan4, *chan6, *chan = NULL;
  lagopus_result_t ret;
  int cnt;

  printf("test_channel_tcp in\n");
  channel_mgr_initialize();
  ret = channel_mgr_channels_lookup_by_dpid(dpid, &chan_list );
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, ret);
  TEST_ASSERT_EQUAL(NULL, chan_list );

  ret = lagopus_ip_address_create("127.0.0.1", true, &addr4);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  ret = lagopus_ip_address_create("::1", false, &addr6);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = channel_mgr_channel_add(bridge_name, dpid, addr4);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  ret = channel_mgr_channel_add(bridge_name, dpid, addr6);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = channel_mgr_channel_add(bridge_name, dpid, addr4);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ALREADY_EXISTS, ret);
  ret = channel_mgr_channel_lookup(bridge_name, addr4, &chan4);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_EQUAL(0, channel_id_get(chan4));
  ret = channel_port_set(chan4, 10022);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  ret = channel_local_port_set(chan4, 20022);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  ret = channel_local_addr_set(chan4, addr4);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = channel_mgr_channel_add(bridge_name, dpid, addr6);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ALREADY_EXISTS, ret);
  ret = channel_mgr_channel_lookup(bridge_name, addr6, &chan6);
  TEST_ASSERT_EQUAL(1, channel_id_get(chan6));
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  ret = channel_port_set(chan6, 10023);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  ret = channel_local_port_set(chan6, 20023);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  ret = channel_local_addr_set(chan6, addr6);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  ret = channel_mgr_channel_lookup_by_channel_id(dpid, 1, &chan);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_NOT_EQUAL(NULL, chan);

  ret = channel_mgr_channel_lookup_by_channel_id(dpid, 9999, &chan);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, ret);
  TEST_ASSERT_EQUAL(NULL, chan);

  cnt = 0;
  ret = channel_mgr_channels_lookup_by_dpid(dpid, &chan_list );
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  ret = channel_list_iterate(chan_list , channel_count, &cnt);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_EQUAL(2, cnt);

  cnt = 0;
  ret = channel_mgr_dpid_iterate(dpid, channel_count, &cnt);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  TEST_ASSERT_EQUAL(2, cnt);

  run = true;
  size = sizeof(sin);
  printf("accept in\n");
  sock4 = accept(s4, (struct sockaddr *)&sin, &size);
  TEST_ASSERT_NOT_EQUAL(-1, sock4);

  sock6 = accept(s6, (struct sockaddr *)&sin, &size);
  TEST_ASSERT_NOT_EQUAL(-1, sock6);


  ret = channel_mgr_channel_lookup(bridge_name, addr4, &chan4);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  channel_refs_get(chan4);

  ret = channel_mgr_channel_delete(bridge_name, addr4);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  channel_refs_put(chan4);
  ret = channel_mgr_channel_delete(bridge_name, addr4);

  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = channel_mgr_channel_lookup(bridge_name, addr4, &chan4);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, ret);

  ret = channel_mgr_channel_delete(bridge_name, addr6);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  ret = channel_mgr_channel_lookup(bridge_name, addr6, &chan6);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, ret);

  TEST_ASSERT_FALSE(channel_mgr_has_alive_channel_by_dpid(dpid));

  ret = channel_mgr_event_upcall();
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  lagopus_ip_address_destroy(addr4);
  lagopus_ip_address_destroy(addr6);
  close(sock4);
  close(sock6);
}

void
test_epilogue(void) {
  lagopus_result_t r;

  run = false;
  channel_mgr_finalize();
  r = global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL(r, LAGOPUS_RESULT_OK);
  lagopus_mainloop_wait_thread();

  close(s4);
  close(s6);


  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK,
                    dp_bridge_destroy(bridge_name));
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK,
                    dp_port_destroy(port_name));
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK,
                    dp_interface_destroy(interface_name));

  dp_api_fini();
}

