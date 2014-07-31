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


#include "unity.h"
#include "lagopus_apis.h"
#include "lagopus/session.h"
#include "event.h"
#include "../channel.h"

ssize_t
write_tcp(struct session *s, void *buf, size_t n) {
  (void) s;
  (void) buf;
  return (ssize_t) n;
}

struct channel *
s_create_data_channel(void) {
  uint64_t dpid = 0x01;
  struct channel *channel;
  struct event_manager *em;
  struct session *session;
  struct addrunion addr  = {0,{{0}}};

  em = event_manager_alloc();
  addrunion_ipv4_set(&addr, "127.0.0.1");
  channel = channel_alloc(&addr, em, dpid);

  session = session_create(SESSION_TCP);
  session_write_set(session, write_tcp);

  channel_version_set(channel, 0x04);
  channel_session_set(channel, session);
  channel_xid_set(channel, 0x10);

  event_manager_free(em);

  return channel;
}

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_channel_version_get_set(void) {
  struct channel *channel;
  uint8_t ret_version;
  uint8_t version = 0x10;

  channel = s_create_data_channel();

  /* Call func. */
  channel_version_set(channel, version);
  ret_version = channel_version_get(channel);

  TEST_ASSERT_EQUAL_MESSAGE(ret_version, version,
                            "version error.");

  session_destroy(channel_session_get(channel));
  free(channel);
}

void
test_channel_session_get_set(void) {
  struct channel *channel;
  struct session *session;
  struct session *ret_session;

  channel = s_create_data_channel();

  session = session_create(SESSION_TCP);
  TEST_ASSERT_NOT_EQUAL_MESSAGE(session, channel_session_get(channel),
                                "session error.");
  session_destroy(channel_session_get(channel));

  /* Call func. */
  channel_session_set(channel, session);
  ret_session = channel_session_get(channel);

  TEST_ASSERT_EQUAL_MESSAGE(ret_session, session,
                            "session error.");

  session_destroy(channel_session_get(channel));
  free(channel);
}

void
test_channel_xid_get_set(void) {
  struct channel *channel;
  uint32_t ret_xid;
  uint32_t xid = 0x10;

  channel = s_create_data_channel();

  /* Call func. */
  channel_xid_set(channel, xid);
  ret_xid = channel_xid_get(channel);

  TEST_ASSERT_EQUAL_MESSAGE(ret_xid, xid,
                            "xid error.");

  /* incrementation */
  /* Call func. */
  ret_xid = channel_xid_get(channel);

  TEST_ASSERT_EQUAL_MESSAGE(ret_xid, xid + 1,
                            "xid incrementation error.");

  /* max */
  /* Call func. */
  channel_xid_set(channel, UINT32_MAX);
  ret_xid = channel_xid_get(channel);

  TEST_ASSERT_EQUAL_MESSAGE(ret_xid, 100 /*START_XID*/,
                            "xid max error.");

  session_destroy(channel_session_get(channel));
  free(channel);
}

void
test_channel_alloc_default_value(void) {
  uint16_t port;
  lagopus_result_t ret;
  struct channel *channel;
  enum channel_protocol protocol;
  struct addrunion addr  = {0,{{0}}};
  struct addrunion addr1 = {0,{{0}}};

  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&addr, &addr1, sizeof(addr),
                                   "channel_addr_get() addr error");
  channel = s_create_data_channel();
  addrunion_ipv4_set(&addr1, "127.0.0.1");

  ret = channel_port_get(channel, &port);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_port_get() error.");
  TEST_ASSERT_EQUAL_MESSAGE(6633, port, "channel_port_get() port error");

  ret = channel_local_port_get(channel, &port);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_local_port_get() error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, port, "channel_local_port_get() port error");

  ret = channel_protocol_get(channel, &protocol);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_protocol_get() error.");
  TEST_ASSERT_EQUAL_MESSAGE(PROTOCOL_TCP, protocol,
                            "channel_protocol_get() proto error");

  ret = channel_addr_get(channel, &addr);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_addr_get() error.");
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&addr1, &addr, sizeof(addr),
                                   "channel_addr_get() addr error");

  session_destroy(channel_session_get(channel));
  free(channel);
}

void
test_channel_proto_get_set_unset(void) {
  lagopus_result_t ret;
  struct channel *channel;
  enum channel_protocol protocol;

  channel = s_create_data_channel();
  ret = channel_protocol_set(channel, PROTOCOL_TCP6);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_protocol_set() error.");

  ret = channel_protocol_get(channel, &protocol);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_protocol_get() error.");
  TEST_ASSERT_EQUAL_MESSAGE(PROTOCOL_TCP6, protocol,
                            "channel_protocol_get() proto error");

  ret = channel_protocol_unset(channel);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_protocol_unset() error.");

  ret = channel_protocol_get(channel, &protocol);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_protocol_get() error.");
  TEST_ASSERT_EQUAL_MESSAGE(PROTOCOL_TCP, protocol,
                            "channel_protocol_get() proto error");

  session_destroy(channel_session_get(channel));
  free(channel);
}

void
test_channel_port_get_set_unset(void) {
  uint16_t port;
  lagopus_result_t ret;
  struct channel *channel;

  channel = s_create_data_channel();
  ret = channel_port_set(channel, 10101);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_port_set() error.");

  ret = channel_port_get(channel, &port);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_port_get() error.");
  TEST_ASSERT_EQUAL_MESSAGE(10101, port,
                            "channel_port_get() port  error");

  ret = channel_port_unset(channel);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_port_unset() error.");

  ret = channel_port_get(channel, &port);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_port_get() error.");
  TEST_ASSERT_EQUAL_MESSAGE(6633, port,
                            "channel_port_get() port error");

  session_destroy(channel_session_get(channel));
  free(channel);
}

void
test_channel_local_port_get_set_unset(void) {
  uint16_t port;
  lagopus_result_t ret;
  struct channel *channel;

  channel = s_create_data_channel();
  ret = channel_local_port_set(channel, 10101);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_local_port_set() error.");

  ret = channel_local_port_get(channel, &port);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_local_port_get() error.");
  TEST_ASSERT_EQUAL_MESSAGE(10101, port,
                            "channel_local_port_get() port  error");

  ret = channel_local_port_unset(channel);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_local_port_unset() error.");

  ret = channel_local_port_get(channel, &port);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_local_port_get() error.");
  TEST_ASSERT_EQUAL_MESSAGE(0, port,
                            "channel_local_port_get() port error");

  session_destroy(channel_session_get(channel));
  free(channel);
}

void
test_channel_local_addr_get_set_unset(void) {
  lagopus_result_t ret;
  struct channel *channel;
  struct addrunion addr  = {0,{{0}}};
  struct addrunion addr1 = {0,{{0}}};

  channel = s_create_data_channel();
  addrunion_ipv4_set(&addr, "127.0.0.1");
  ret = channel_local_addr_set(channel, &addr);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_local_addr_set() error.");

  ret = channel_local_addr_get(channel, &addr1);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_local_addr_get() error.");
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&addr, &addr1, sizeof(addr),
                                   "channel_local_addr_get() addr error");

  ret = channel_local_addr_unset(channel);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_local_addr_unset() error.");

  ret = channel_local_addr_get(channel, &addr1);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_local_addr_get() error.");
  addrunion_ipv4_set(&addr, "0.0.0.0");
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&addr, &addr1, sizeof(addr),
                                   "channel_local_addr_get() addr  error");

  session_destroy(channel_session_get(channel));
  free(channel);
}
