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
#include "event.h"
#include "string.h"
#include "lagopus_apis.h"
#include "lagopus/pbuf.h"
#include "lagopus/session.h"
#include "lagopus/dpmgr.h"
#include "lagopus/port.h"
#include "lagopus/ofp_bridgeq_mgr.h"
#include "../channel.h"
#include "../channel_mgr.h"
#include "handler_test_utils.h"

/* Create packet data. */
#define BUF_SIZE 65535
#define TIMEOUT 100LL * 1000LL * 1000LL * 1000LL


#define TEST_ASSERT_EQUAL_OFP_ERROR(expected, actual)                       \
  {                                                                         \
    if ((expected) != NULL) {                                               \
      TEST_ASSERT_EQUAL_MESSAGE((expected)->type, (actual)->type,           \
                                "the expected error type is not matched."); \
      TEST_ASSERT_EQUAL_MESSAGE((expected)->code, (actual)->code,           \
                                "the expected error code is not matched."); \
    } else {                                                                \
      TEST_FAIL_MESSAGE("invalid usage of TEST_ASSERT_EQUAL_OFP_ERROR");    \
    }                                                                       \
  }

lagopus_result_t
ofp_header_decode_sneak_test(struct pbuf *pbuf,
                             struct ofp_header *packet) {
  /* Size check. */
  if (pbuf->plen < sizeof(struct ofp_header)) {
    return LAGOPUS_RESULT_OUT_OF_RANGE;
  }

  /* Decode packet. */
  DECODE_GETC(packet->version);
  DECODE_GETC(packet->type);
  DECODE_GETW(packet->length);
  DECODE_GETL(packet->xid);
  DECODE_REWIND(sizeof(struct ofp_header));

  return LAGOPUS_RESULT_OK;
}

static bool
s_start_ofp_handler(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  /* set gstate started */
  res = global_state_set(GLOBAL_STATE_STARTED);
  if (res != LAGOPUS_RESULT_OK) {
    lagopus_perror(res);
    TEST_FAIL_MESSAGE("handler_test_utils.c: gstate start error");
    return false;
  }
  /* create ofp_handler */
  res = ofp_handler_initialize(NULL, NULL);
  if (res != LAGOPUS_RESULT_OK) {
    lagopus_perror(res);
    TEST_FAIL_MESSAGE("handler_test_utils.c: handler creation error");
    return false;
  }
  /* start ofp_handler */
  res = ofp_handler_start();
  if (res != LAGOPUS_RESULT_OK) {
    lagopus_perror(res);
    TEST_FAIL_MESSAGE("handler_test_utils.c: handler start error");
    return false;
  }
  return true;
}

static lagopus_result_t
s_shutdown_ofp_handler(void) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  res = ofp_handler_shutdown(SHUTDOWN_GRACEFULLY);
  if (res != LAGOPUS_RESULT_OK) {
    lagopus_perror(res);
    TEST_FAIL_MESSAGE("handler_test_utils.c: handler shutdown error");
  }
  return res;
}

static int
s_is_hex(char c) {
  if ((c >= '0' && c <= '9') ||
      (c >= 'a' && c <= 'f') ||
      (c >= 'A' && c <= 'F')) {
    return 1;
  }
  return 0;
}

void
create_packet(const char in_data[], struct pbuf **pbuf) {
  int i;
  int j;
  int len = 0;
  char num[3] = {0};

  *pbuf = pbuf_alloc(BUF_SIZE);

  for (i = 0; i < (int) strlen(in_data) - 1; ) {
    j = 0;
    while (j < 2 && i < (int) strlen(in_data)) {
      if (s_is_hex(in_data[i])) {
        num[j] = in_data[i];
        j++;
      }
      i++;
    }
    *((*pbuf)->putp) = (uint8_t) strtol(num, NULL, 16);
    (*pbuf)->putp++;
    (*pbuf)->plen--;
    len++;
  }
  (*pbuf)->plen = (size_t) len;
}
/* END : Create packet data. */


static ssize_t
s_write_tcp(struct session *s, void *buf, size_t n) {
  (void) s;
  (void) buf;
  return (ssize_t) n;
}

static struct dpmgr *s_dpmgr = NULL;
static struct bridge *s_bridge = NULL;
static uint32_t s_xid = 0x10;
static struct event_manager *s_event_manager = NULL;

struct channel *
create_data_channel(void) {
  static uint8_t cnt;
  char buf[256];
  struct channel *channel;
  struct session *session;
  struct addrunion addr;
  struct port port;
  uint64_t dpid = 0xabc;
  uint8_t port_hw_addr[OFP_ETH_ALEN] = {0xff, 0xff, 0xff,
                                        0xff, 0xff, 0xff
                                       };

  if (s_dpmgr == NULL) {
    s_dpmgr = dpmgr_alloc();
    dpmgr_bridge_add(s_dpmgr, "br0", dpid);
    dpmgr_controller_add(s_dpmgr, "br0", "127.0.0.1");
    port.ofp_port.port_no = 0;
    memcpy(port.ofp_port.hw_addr, port_hw_addr, OFP_ETH_ALEN);
    port.ifindex = 0;
    dpmgr_port_add(s_dpmgr, &port);
    dpmgr_bridge_port_add(s_dpmgr, "br0", 0, 0);
  }
  if (s_bridge == NULL) {
    s_bridge = dpmgr_bridge_lookup(s_dpmgr, "br0");
  }
  if (s_event_manager == NULL) {
    s_event_manager = event_manager_alloc();
    channel_mgr_initialize(s_event_manager);
  }

  snprintf(buf, sizeof(buf), "127.0.0.%u", cnt++);//XXX
  addrunion_ipv4_set(&addr, buf);
  channel_mgr_channel_add(s_bridge, dpid, &addr);
  channel_mgr_channel_lookup(s_bridge, &addr, &channel);


  session = channel_session_get(channel);
  session_write_set(session, s_write_tcp);
  session_sockfd_set(session, 3);

  channel_version_set(channel, 0x04);
  channel_xid_set(channel, s_xid);

  return channel;
}

static void
s_destroy_channel(struct channel *channel) {
  if (channel != NULL) {
    lagopus_result_t ret;
    ret = channel_free(channel);
    if (ret != LAGOPUS_RESULT_OK) {
      lagopus_msg_fatal("channel_free error (%s)\n",
                        lagopus_error_get_string(ret));
    }
  }
}

static void
s_destroy_static_data(void) {
  channel_mgr_finalize();

  if (s_bridge != NULL) {
    bridge_free(s_bridge);
    s_bridge = NULL;
  }
  if (s_dpmgr != NULL) {
    port_delete(s_dpmgr->ports, 0);
    dpmgr_free(s_dpmgr);
    s_dpmgr = NULL;
  }
  if (s_event_manager != NULL) {
    event_manager_free(s_event_manager);
    s_event_manager = NULL;
  }
}

void
destroy_data_channel(struct channel *channel) {
  s_destroy_channel(channel);
}

lagopus_result_t
check_packet_parse_cont(ofp_handler_proc_t handler_proc,
                        const char packet[]) {
  return check_packet_parse_expect_error_cont(handler_proc, packet, NULL);
}

lagopus_result_t
check_packet_parse_expect_error_cont(ofp_handler_proc_t handler_proc,
                                     const char packet[],
                                     const struct ofp_error *expected_error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = create_data_channel();
  struct ofp_header xid_header = {0, 0, 0, 0};
  struct pbuf *pbuf;
  struct ofp_error error;

  /* create packet */
  create_packet(packet, &pbuf);
  xid_header.xid = s_xid;

  /* call func & check */
  res = (handler_proc)(channel, pbuf, &xid_header, &error);
  if (res == LAGOPUS_RESULT_OK) {
    TEST_ASSERT_EQUAL_MESSAGE(0, pbuf->plen,
                              "handler_test_utils.c: packet data len error.");
  } else if (res == LAGOPUS_RESULT_OFP_ERROR) {
    /* check error if expected */
    if (expected_error != NULL) {
      TEST_ASSERT_EQUAL_OFP_ERROR(expected_error, &error);
    }
  }

  pbuf_free(pbuf);
  /* DO NOT call s_destroy_static_data() */
  return res;
}


lagopus_result_t
check_packet_parse(ofp_handler_proc_t handler_proc,
                   const char packet[]) {
  return check_packet_parse_expect_error(handler_proc, packet, NULL);
}

lagopus_result_t
check_packet_parse_expect_error(ofp_handler_proc_t handler_proc,
                                const char packet[],
                                const struct ofp_error *expected_error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = create_data_channel();
  struct ofp_header xid_header = {0, 0, 0, 0};
  struct pbuf *pbuf;
  struct ofp_error error;

  /* create packet */
  create_packet(packet, &pbuf);
  /* ignore result code. */
  (void) ofp_header_decode_sneak_test(pbuf, &xid_header);

  /* call func & check */
  res = (handler_proc)(channel, pbuf, &xid_header, &error);
  if (res == LAGOPUS_RESULT_OK) {
    TEST_ASSERT_EQUAL_MESSAGE(0, pbuf->plen,
                              "handler_test_utils.c: packet data len error.");
  } else if (res == LAGOPUS_RESULT_OFP_ERROR) {
    /* check error if expected */
    if (expected_error != NULL) {
      TEST_ASSERT_EQUAL_OFP_ERROR(expected_error, &error);
    }
  }

  pbuf_free(pbuf);
  s_destroy_static_data();
  return res;
}

lagopus_result_t
check_packet_parse_array(ofp_handler_proc_t handler_proc,
                         const char *packet[],
                         int array_len) {
  return check_packet_parse_array_expect_error(handler_proc,
         packet, array_len, NULL);
}


lagopus_result_t
check_packet_parse_array_expect_error(ofp_handler_proc_t handler_proc,
                                      const char *packet[],
                                      int array_len,
                                      const struct ofp_error *expected_error) {

  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = create_data_channel();
  struct ofp_header xid_header;
  struct pbuf *pbuf;
  struct ofp_error error;
  bool error_has_occurred = false;
  int i;
  for (i = 0; i < array_len; i++) {
    lagopus_msg_debug(1, "packet[%d] start ... %s\n", i, packet[i]);
    /* create packet */
    create_packet(packet[i], &pbuf);
    /* parse header */
    if (ofp_header_decode_sneak_test(pbuf, &xid_header) !=
        LAGOPUS_RESULT_OK) {
      pbuf_free(pbuf);
      s_destroy_static_data();
      TEST_FAIL_MESSAGE("handler_test_utils.c: cannot decode header\n");
      return LAGOPUS_RESULT_OFP_ERROR;
    }
    /* call func & check */
    res = (handler_proc)(channel, pbuf, &xid_header, &error);
    lagopus_msg_debug(1, "packet[%d] done ... %s\n", i,
                      lagopus_error_get_string(res));
    if (res == LAGOPUS_RESULT_OK) {
      TEST_ASSERT_EQUAL_MESSAGE(0, pbuf->plen,
                                "handler_test_utils.c: packet data len error.");
    } else if (res == LAGOPUS_RESULT_OFP_ERROR) {
      error_has_occurred = true;
      if (expected_error != NULL) {
        TEST_ASSERT_EQUAL_OFP_ERROR(expected_error, &error);
      }
      pbuf_free(pbuf);
      break;
    }
    /* free */
    pbuf_free(pbuf);
  }

  /* free */
  s_destroy_static_data();

  if (error_has_occurred == true) {
    TEST_ASSERT_EQUAL_OFP_ERROR(expected_error, &error);
  }

  return res;
}

lagopus_result_t
check_packet_parse_array_for_multipart(ofp_handler_proc_t handler_proc,
                                       const char *packet[],
                                       int array_len) {
  return check_packet_parse_array_for_multipart_expect_error(handler_proc,
         packet,
         array_len,
         NULL);
}

lagopus_result_t
check_packet_parse_array_for_multipart_expect_error(ofp_handler_proc_t
    handler_proc,
    const char *packet[],
    int array_len,
    const struct ofp_error *expected_error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = create_data_channel();
  struct ofp_header xid_header;
  struct pbuf *pbuf;
  struct ofp_error error;
  int i;
  for (i = 0; i < array_len; i++) {
    lagopus_msg_debug(1, "packet[%d] start ... %s\n", i, packet[i]);
    /* create packet */
    create_packet(packet[i], &pbuf);
    /* parse header */
    if (ofp_header_decode_sneak_test(pbuf, &xid_header) !=
        LAGOPUS_RESULT_OK) {
      TEST_FAIL_MESSAGE("handler_test_utils.c: cannot decode header\n");
      return LAGOPUS_RESULT_OFP_ERROR;
    }
    /* call func & check */
    res = (handler_proc)(channel, pbuf, &xid_header, &error);
    lagopus_msg_debug(1, "packet[%d] done ... %s\n", i,
                      lagopus_error_get_string(res));
    if (res == LAGOPUS_RESULT_OK) {
      if (i < array_len - 1) {
        TEST_ASSERT_EQUAL_MESSAGE(1, channel_multipart_used_count_get(channel),
                                  "handler_test_utils.c: multipart count.");
      } else {
        TEST_ASSERT_EQUAL_MESSAGE(0, channel_multipart_used_count_get(channel),
                                  "handler_test_utils.c: multipart count.");
      }
    } else if (LAGOPUS_RESULT_OFP_ERROR) {
      if (expected_error != NULL) {
        TEST_ASSERT_EQUAL_OFP_ERROR(expected_error, &error);
      }
    } else {
      break;
    }
    /* free */
    pbuf_free(pbuf);
  }
  /* free */
  s_destroy_static_data();
  return res;
}

lagopus_result_t
check_packet_parse_with_dequeue(ofp_handler_proc_t handler_proc,
                                const char packet[],
                                void **get) {
  return check_packet_parse_with_dequeue_expect_error(handler_proc,
         packet, get,
         NULL);
}

lagopus_result_t
check_packet_parse_with_dequeue_expect_error(ofp_handler_proc_t handler_proc,
    const char packet[],
    void **get,
    const struct ofp_error *expected_error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = create_data_channel();
  struct ofp_header xid_header;
  struct ofp_bridge *ofpb = NULL;
  struct pbuf *pbuf;
  struct ofp_error error;
  struct ofp_bridgeq *bridgeq;
  if (get == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  /* start ofp_handler */
  if (s_start_ofp_handler() == false) {
    goto done;
  }
  /* register ofp_bridge */
  res = ofp_bridgeq_mgr_bridge_register(channel_dpid_get(channel));
  if (res == LAGOPUS_RESULT_OK) {
    res = ofp_bridgeq_mgr_bridge_lookup(channel_dpid_get(channel), &bridgeq);
    if (res == LAGOPUS_RESULT_OK) {
      ofpb = ofp_bridgeq_mgr_bridge_get(bridgeq);
      ofp_bridgeq_mgr_bridgeq_free(bridgeq);
    } else {
      TEST_FAIL_MESSAGE("handler_test_utils.c: "
                        "ofp_bridgeq_mgr_bridge_get error.");
    }
  } else {
    lagopus_perror(res);
    TEST_FAIL_MESSAGE("handler_test_utils.c: register error.");
    goto done;
  }
  /* create packet */
  create_packet(packet, &pbuf);
  /* parse header */
  if (ofp_header_decode_sneak_test(pbuf, &xid_header) != LAGOPUS_RESULT_OK) {
    TEST_FAIL_MESSAGE("handler_test_utils.c: cannot decode header\n");
    return LAGOPUS_RESULT_OFP_ERROR;
  }
  /* call func & check */
  res = (handler_proc)(channel, pbuf, &xid_header, &error);
  if (res == LAGOPUS_RESULT_OK) {
    res = lagopus_bbq_get(&ofpb->event_dataq, get, void *, TIMEOUT);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, res,
                              "handler_test_utils.c: lagopus_bbq_get error.");
  } else if (res == LAGOPUS_RESULT_OFP_ERROR) {
    /* check error if expected */
    if (expected_error != NULL) {
      TEST_ASSERT_EQUAL_OFP_ERROR(expected_error, &error);
    } else {
      TEST_FAIL_MESSAGE("expecting ofp error,"
                        "but a handler function make no ofp_error");
    }
  }

  /* unregister & destroy ofp_bridge */
  (void) ofp_bridgeq_mgr_bridge_unregister(channel_dpid_get(channel));
  /* free */
  pbuf_free(pbuf);
done:
  s_destroy_static_data();
  (void)s_shutdown_ofp_handler();
  return res;
}

lagopus_result_t
check_packet_create(ofp_reply_create_proc_t create_proc,
                    const char packet[]) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = create_data_channel();
  struct pbuf *test_pbuf, *ret_pbuf;
  struct ofp_header xid_header;
  size_t sizeof_packet;

  /* create packet */
  create_packet(packet, &test_pbuf);
  sizeof_packet = (size_t) (test_pbuf->putp - test_pbuf->getp);

  xid_header.xid = s_xid;
  /* call func & check */
  res = (create_proc)(channel, &ret_pbuf, &xid_header);
  if (res == LAGOPUS_RESULT_OK) {
    TEST_ASSERT_EQUAL_MESSAGE(0, ret_pbuf->plen,
                              "handler_test_utils.c: packet len error.");
    TEST_ASSERT_EQUAL_MESSAGE(sizeof_packet,
                              ret_pbuf->putp - ret_pbuf->getp,
                              "handler_test_utils.c: packet data len error.");
    TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(ret_pbuf->data, test_pbuf->data,
                                        sizeof_packet),
                              "handler_test_utils.c: packet compare error.");
    pbuf_free(ret_pbuf);
  }
  /* free */
  pbuf_free(test_pbuf);
  s_destroy_static_data();
  return res;
}

lagopus_result_t
check_pbuf_list_packet_create(ofp_reply_list_create_proc_t create_proc,
                              const char *packets[],
                              int array_len) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = create_data_channel();
  struct pbuf_list *test_pbuf_list, *ret_pbuf_list;
  struct pbuf *test_pbuf;
  struct pbuf *ret_pbuf;
  struct ofp_header xid_header;
  size_t sizeof_packet;
  int i;

  s_xid = 0x10;
  test_pbuf_list = pbuf_list_alloc();
  for (i = 0; i < array_len; i++) {
    /* create packet */
    create_packet(packets[i], &test_pbuf);
    pbuf_list_add(test_pbuf_list, test_pbuf);
  }

  xid_header.xid = s_xid;

  /* call func & check */
  res = (create_proc)(channel, &ret_pbuf_list, &xid_header);

  if (res == LAGOPUS_RESULT_OK) {
    i = 0;
    TAILQ_FOREACH(ret_pbuf, &ret_pbuf_list->tailq, entry) {
      test_pbuf = TAILQ_FIRST(&test_pbuf_list->tailq);
      if (test_pbuf != NULL) {
        TEST_ASSERT_EQUAL_MESSAGE(0, ret_pbuf->plen,
                                  "handler_test_utils.c: packet len error.");
        sizeof_packet = (size_t) (test_pbuf->putp - test_pbuf->getp);
        TEST_ASSERT_EQUAL_MESSAGE(sizeof_packet,
                                  ret_pbuf->putp - ret_pbuf->getp,
                                  "handler_test_utils.c: packet data len error.");
        TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(ret_pbuf->data, test_pbuf->data,
                                            sizeof_packet),
                                  "handler_test_utils.c: packet compare error.");
        i++;
      } else {
        TEST_FAIL_MESSAGE("handler_test_utils.c: test_pbuf is NULL.")
        break;
      }
    }
    TEST_ASSERT_EQUAL_MESSAGE(array_len, i, "handler_test_utils.c: "
                              "pbuf_list len error.");
    pbuf_list_free(ret_pbuf_list);
  }

  /* free */
  pbuf_list_free(test_pbuf_list);
  s_destroy_static_data();
  return res;
}

lagopus_result_t
check_packet_send(ofp_send_proc_t send_proc) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = create_data_channel();
  struct ofp_header xid_header;
  xid_header.xid = s_xid;
  /* call func & check */
  res = (send_proc)(channel, &xid_header);
  /* free */
  s_destroy_static_data();
  return res;
}

lagopus_result_t
check_use_channels(ofp_channels_proc_t channels_proc) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  int i;
  struct channel *channels[CHANNEL_MAX_NUM];
  struct ofp_header xid_headers[CHANNEL_MAX_NUM];

  /* create channels. */
  s_xid = 0x10;
  for (i = 0; i < CHANNEL_MAX_NUM; i++) {
    channels[i] = create_data_channel();
    xid_headers[i].xid = s_xid;
    s_xid += 0x10;
  }

  /* call func & check */
  res = (channels_proc)(channels, xid_headers);
  /* free */
  s_destroy_static_data();
  return res;
}

lagopus_result_t
check_use_channels_send(ofp_channels_send_proc_t send_proc,
                        const char packet[]) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  int i;
  struct channel *channels[CHANNEL_MAX_NUM];
  struct ofp_header xid_headers[CHANNEL_MAX_NUM];
  struct pbuf *test_pbuf;
  size_t sizeof_packet;

  /* create channels. */
  s_xid = 0x10;
  for (i = 0; i < CHANNEL_MAX_NUM; i++) {
    channels[i] = create_data_channel();
    xid_headers[i].xid = s_xid;
    s_xid += 0x10;
  }

  /* create packet */
  create_packet(packet, &test_pbuf);
  sizeof_packet = (size_t) (test_pbuf->putp - test_pbuf->getp);

  /* call func & check */
  res = (send_proc)(channels, xid_headers, test_pbuf);
  if (res == LAGOPUS_RESULT_OK) {
    TEST_ASSERT_EQUAL_MESSAGE(sizeof_packet, test_pbuf->plen,
                              "handler_test_utils.c: packet len error.");

    TEST_ASSERT_EQUAL_MESSAGE(test_pbuf->putp, test_pbuf->getp,
                              "handler_test_utils.c: packet data len error.");
  }

  /* free */
  pbuf_free(test_pbuf);
  s_destroy_static_data();
  return res;
}

lagopus_result_t
check_packet(ofp_check_packet_proc_t check_packet_proc,
             const char packet[]) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = create_data_channel();
  struct pbuf *test_pbuf, *ret_pbuf;
  struct ofp_header xid_header;
  size_t packet_length;

  /* create packet */
  create_packet(packet, &test_pbuf);
  packet_length = (size_t) (test_pbuf->putp - test_pbuf->getp);
  xid_header.xid = s_xid;
  /* call func & check */
  res = (check_packet_proc)(channel, &ret_pbuf, &xid_header);
  if (res == LAGOPUS_RESULT_OK) {
    TEST_ASSERT_EQUAL_MESSAGE(0, ret_pbuf->plen,
                              "packet len error.");
    TEST_ASSERT_EQUAL_MESSAGE(packet_length,
                              ret_pbuf->putp - ret_pbuf->getp,
                              "packet data len error.");
    TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(ret_pbuf->data, test_pbuf->data,
                                        packet_length),
                              "packet data compare error.");
    pbuf_free(ret_pbuf);
  }
  /* free */
  pbuf_free(test_pbuf);
  return res;
}

lagopus_result_t
check_use_channels_write(ofp_channels_write_proc_t write_proc,
                         const char packet[]) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  int i;
  struct channel *channels[CHANNEL_MAX_NUM];
  struct ofp_header xid_headers[CHANNEL_MAX_NUM];
  struct pbuf *test_pbuf;

  /* create channels. */
  s_xid = 0x10;
  for (i = 0; i < CHANNEL_MAX_NUM; i++) {
    channels[i] = create_data_channel();
    xid_headers[i].xid = s_xid;
    s_xid += 0x10;
  }

  /* create packet */
  create_packet(packet, &test_pbuf);

  /* call func & check */
  res = (write_proc)(channels, xid_headers);

  /* free */
  pbuf_free(test_pbuf);
  s_destroy_static_data();

  return res;
}

void
data_create(ofp_data_create_proc_t create_proc,
            const char packet[]) {
  int i;
  struct channel *channels[CHANNEL_MAX_NUM];
  struct ofp_header xid_headers[CHANNEL_MAX_NUM];
  struct pbuf *test_pbuf;

  /* create channels. */
  s_xid = 0x10;
  for (i = 0; i < CHANNEL_MAX_NUM; i++) {
    channels[i] = create_data_channel();
    xid_headers[i].xid = s_xid;
    s_xid += 0x10;
  }

  /* create packet */
  create_packet(packet, &test_pbuf);

  /* call func & check */
  (create_proc)(channels, xid_headers, test_pbuf);

  /* free */
  pbuf_free(test_pbuf);
  s_destroy_static_data();
}

lagopus_result_t
check_instruction_parse_expect_error(ofp_instruction_parse_proc_t
                                     instruction_parse_proc,
                                     struct pbuf *test_pbuf,
                                     struct instruction_list *instruction_list,
                                     struct ofp_error *expected_error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error error;

  res = (instruction_parse_proc)(test_pbuf, instruction_list, &error);
  if (res == LAGOPUS_RESULT_OFP_ERROR) {
    if (expected_error != NULL) {
      TEST_ASSERT_EQUAL_OFP_ERROR(expected_error, &error);
    }
  } else {
    TEST_FAIL_MESSAGE(__FILE__
                      ": expecting ofp error,"
                      "but a handler function make no ofp_error");
  }
  return res;
}

lagopus_result_t
check_match_parse_expect_error(ofp_match_parse_proc_t match_parse_proc,
                               struct channel *channel,
                               struct pbuf *test_pbuf,
                               struct match_list *match_list,
                               struct ofp_error *expected_error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error error;

  res = (match_parse_proc)(channel,test_pbuf, match_list, &error);
  if (res == LAGOPUS_RESULT_OFP_ERROR) {
    if (expected_error != NULL) {
      TEST_ASSERT_EQUAL_OFP_ERROR(expected_error, &error);
    }
  } else {
    TEST_FAIL_MESSAGE(__FILE__
                      ": expecting ofp error,"
                      "but a handler function make no ofp_error");
  }
  return res;
}
