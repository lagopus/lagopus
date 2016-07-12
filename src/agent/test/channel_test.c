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
#include "lagopus_session.h"
#include "../channel.h"

ssize_t
write_tcp(lagopus_session_t s, void *buf, size_t n) {
  (void) s;
  (void) buf;
  return (ssize_t) n;
}

struct channel *
s_create_data_channel(void) {
  uint64_t dpid = 0x01;
  struct channel *channel;
  lagopus_session_t session;
  lagopus_ip_address_t *addr = NULL;

  lagopus_ip_address_create("127.0.0.1", true, &addr);
  channel = channel_alloc(addr, dpid);

  (void) session_create(SESSION_TCP|SESSION_ACTIVE, &session);
  session_write_set(session, write_tcp);

  channel_version_set(channel, 0x04);
  channel_session_set(channel, session);
  channel_xid_set(channel, 0x10);

  lagopus_ip_address_destroy(addr);

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

  channel_free(channel);
}

void
test_channel_session_get_set(void) {
  struct channel *channel;
  lagopus_session_t session;
  lagopus_session_t ret_session;

  channel = s_create_data_channel();

  (void) session_create(SESSION_TCP|SESSION_ACTIVE, &session);
  TEST_ASSERT_NOT_EQUAL_MESSAGE(session, channel_session_get(channel),
                                "session error.");
  session_destroy(channel_session_get(channel));

  /* Call func. */
  channel_session_set(channel, session);
  ret_session = channel_session_get(channel);

  TEST_ASSERT_EQUAL_MESSAGE(ret_session, session,
                            "session error.");

  channel_free(channel);
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

  channel_free(channel);
}

void
test_channel_alloc_default_value(void) {
  uint16_t port;
  lagopus_result_t ret;
  struct channel *channel;
  enum channel_protocol protocol;
  lagopus_ip_address_t *addr  = NULL;
  lagopus_ip_address_t *addr1 = NULL;

  channel = s_create_data_channel();

  ret = lagopus_ip_address_create("127.0.0.1", true, &addr1);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "lagopus_ip_address_create() error.");

  ret = channel_port_get(channel, &port);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "channel_port_get() error.");
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
  TEST_ASSERT_NOT_NULL(addr);
  TEST_ASSERT_NOT_NULL(addr1);
  TEST_ASSERT_EQUAL_MESSAGE(true, lagopus_ip_address_equals(addr, addr1),
                            "channel_addr_get() addr error");

  lagopus_ip_address_destroy(addr);
  lagopus_ip_address_destroy(addr1);
  channel_free(channel);
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

  channel_free(channel);
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

  channel_free(channel);
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

  channel_free(channel);
}

void
test_channel_local_addr_get_set_unset(void) {
  lagopus_result_t ret;
  struct channel *channel;
  lagopus_ip_address_t *addr  = NULL;
  lagopus_ip_address_t *addr1 = NULL;
  lagopus_ip_address_t *addr2  = NULL;
  lagopus_ip_address_t *addr3 = NULL;

  channel = s_create_data_channel();

  ret = lagopus_ip_address_create("127.0.0.1", true, &addr);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "lagopus_ip_address_create() error.");

  ret = channel_local_addr_set(channel, addr);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_local_addr_set() error.");

  ret = channel_local_addr_get(channel, &addr1);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_local_addr_get() error.");
  TEST_ASSERT_NOT_NULL(addr);
  TEST_ASSERT_NOT_NULL(addr1);
  TEST_ASSERT_EQUAL_MESSAGE(true, lagopus_ip_address_equals(addr, addr1),
                            "channel_local_addr_get() addr error");

  ret = channel_local_addr_unset(channel);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_local_addr_unset() error.");

  ret = lagopus_ip_address_create("0.0.0.0", true, &addr2);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "lagopus_ip_address_create() error.");

  ret = channel_local_addr_get(channel, &addr3);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_local_addr_get() error.");
  TEST_ASSERT_NOT_NULL(addr2);
  TEST_ASSERT_NOT_NULL(addr3);
  TEST_ASSERT_EQUAL_MESSAGE(true, lagopus_ip_address_equals(addr2, addr3),
                            "channel_local_addr_get() addr error");

  lagopus_ip_address_destroy(addr);
  lagopus_ip_address_destroy(addr1);
  lagopus_ip_address_destroy(addr2);
  lagopus_ip_address_destroy(addr3);
  channel_free(channel);
}

void
test_channel_dpid_get_set(void) {
  uint64_t dpid;
  lagopus_result_t ret;
  struct channel *channel;

  channel = s_create_data_channel();

  /* Call func. */
  ret = channel_dpid_set(channel, 0xbeef);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_dpid_set() error.");
  dpid = channel_dpid_get(channel);

  TEST_ASSERT_EQUAL_MESSAGE(dpid, 0xbeef,
                            "dpid error.");

  channel_free(channel);
}

void
test_channel_auxiliary_get_set(void) {
  bool is_auxiliary;
  lagopus_result_t ret;
  struct channel *channel;

  channel = s_create_data_channel();

  /* Call func. */
  ret = channel_auxiliary_set(channel, true);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_auxiliary_set() error.");
  ret = channel_auxiliary_get(channel, &is_auxiliary);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_auxiliary_get() error.");
  TEST_ASSERT_EQUAL_MESSAGE(is_auxiliary, true,
                            "auxiliary error.");

  channel_free(channel);
}

void
test_channel_multipart_get_put(void) {
  char buf[16];
  lagopus_result_t ret;
  struct channel *channel;
  struct ofp_header xid_header;
  struct pbuf *pbuf[2], *pbuf2 = NULL;
  
  xid_header.xid = 0;
  channel = s_create_data_channel();

  pbuf[0] = pbuf_alloc(64);
  pbuf_plen_set(pbuf[0], 64);
  pbuf_encode(pbuf[0], buf, sizeof(buf));
  pbuf[1] = pbuf_alloc(64);
  pbuf_plen_set(pbuf[1], 64);
  pbuf_encode(pbuf[1], buf, 15);
  /* Call func. */
  ret = channel_multipart_put(channel, pbuf[0], &xid_header, 0);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_multipart_put() error.");
  ret = channel_multipart_put(channel, pbuf[1], &xid_header, 0);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_multipart_put() error.");
  ret = channel_multipart_used_count_get(channel);
  TEST_ASSERT_EQUAL_MESSAGE(1, ret,
                            "channel_multipart_used_count_get() error.");
  ret = channel_multipart_get(channel, &pbuf2, &xid_header, 0);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_multipart_get() error.");
  TEST_ASSERT_EQUAL_MESSAGE(16+15, pbuf_plen_get(pbuf2),
                            "pbuf plen error.");
  ret = channel_multipart_used_count_get(channel);
  TEST_ASSERT_EQUAL_MESSAGE(0, ret,
                            "channel_multipart_used_count_get() error.");
  pbuf_free(pbuf2);

  channel_free(channel);
}

void
test_channel_multipart_get_put_err(void) {
  char buf[16];
  lagopus_result_t ret;
  struct channel *channel;
  struct ofp_header xid_header;
  struct pbuf *pbuf, *pbuf2 = NULL;
  
  xid_header.xid = 0;
  channel = s_create_data_channel();

  pbuf = pbuf_alloc(64);
  pbuf_plen_set(pbuf, 64);
  pbuf_encode(pbuf, buf, 8);
  /* Call func. */
  ret = channel_multipart_put(channel, pbuf, &xid_header, 0);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "channel_multipart_put() error.");
  pbuf->putp=NULL;
  ret = channel_multipart_used_count_get(channel);
  TEST_ASSERT_EQUAL_MESSAGE(1, ret,
                            "channel_multipart_used_count_get() error.");
  ret = channel_multipart_get(channel, &pbuf2, &xid_header, 0);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_NO_MEMORY, ret,
                            "channel_multipart_get() error.");
  ret = channel_multipart_used_count_get(channel);
  TEST_ASSERT_EQUAL_MESSAGE(0, ret,
                            "channel_multipart_used_count_get() error.");
  pbuf_free(pbuf2);

  channel_free(channel);
}
