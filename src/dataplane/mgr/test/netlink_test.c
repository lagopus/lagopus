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

#ifdef HYBRID
#include "netlink.c"

#define ETH_LEN  6

lagopus_thread_t *thdptr = NULL;
struct interface *ifp = NULL;
#endif /* HYBRID */

void
setUp(void) {
#ifdef HYBRID
  lagopus_result_t rv;

  /* preparation */
  rib_notifier_init();
  /* create tap interface */
  ifp = dp_interface_alloc();
  ifp->port = port_alloc();
  ifp->port->bridge = bridge_alloc(":test");
  ifp->hw_addr[0] = 0x00;
  ifp->hw_addr[1] = 0x00;
  ifp->hw_addr[2] = 0x00;
  ifp->hw_addr[3] = 0x00;
  ifp->hw_addr[4] = 0x00;
  ifp->hw_addr[5] = 0x01;
  dp_tapio_thread_init(NULL, NULL, NULL, &thdptr);
  rv = dp_tap_interface_create(":test", ifp);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = dp_tap_start_interface(":test", ifp);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
tearDown(void) {
#ifdef HYBRID
  lagopus_result_t rv;

  bridge_free(ifp->port->bridge);
  port_free(ifp->port);
  rv = dp_tap_interface_destroy(":test");
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  dp_interface_free(ifp);
  dp_tapio_thread_fini();
  ifp = NULL;
  thdptr = NULL;

  rib_notifier_fini();
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_netlink_addr(void) {
#ifdef HYBRID
  struct test {
    struct nlmsghdr h;
    unsigned char kernel_msg[64];
  };

  lagopus_result_t rv;
  struct test test;
  struct nlmsghdr h;
  struct bridge *bridge = NULL;
  struct notification_entry *entry = NULL;
  uint8_t hwaddr[6] = {0};
  char ifaddr_msg[64] = {0x02, 0x10, 0x80, 0x00, 0x1c, 0x00, 0x00,
                         0x00, 0x08, 0x00, 0x01, 0x00, 0xc0, 0xa8,
                         0x01, 0x01, 0x08, 0x00, 0x02, 0x00, 0xc0,
                         0xa8, 0x01, 0x01, 0x09, 0x00, 0x03, 0x00,
                         0x74, 0x65, 0x73, 0x74, 0x00, 0x00, 0x00,
                         0x00, 0x08, 0x00, 0x08, 0x00, 0x80, 0x00,
                         0x00, 0x00, 0x14, 0x00, 0x06, 0x00, 0xff,
                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                         0x5f, 0x81, 0x03, 0x00, 0x5f, 0x81, 0x03,
                         0x00};

  /* preparation */
  test.h.nlmsg_len = 80;
  test.h.nlmsg_type = RTM_NEWADDR;
  test.h.nlmsg_flags = 2;
  test.h.nlmsg_seq = 1;
  test.h.nlmsg_pid = 1000;
  memcpy(test.kernel_msg, ifaddr_msg, sizeof(ifaddr_msg));

  netlink_addr(NULL, &test);

  /* check notification entry of rib */
  rv = dp_tapio_interface_info_get("test", hwaddr, &bridge);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_NOT_NULL(bridge);

  lagopus_bbq_get(&bridge->rib.notification_queue, &entry,
                  struct notification_entry *, 0);
  TEST_ASSERT_NOT_NULL(entry);
  TEST_ASSERT_EQUAL(entry->type, NOTIFICATION_TYPE_IFADDR);
  TEST_ASSERT_EQUAL(entry->action, NOTIFICATION_ACTION_TYPE_ADD);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_netlink_neigh_type_add(void) {
#ifdef HYBRID
  struct test {
    struct nlmsghdr h;
    unsigned char kernel_msg[60];
  };

  lagopus_result_t rv;
  struct test test;
  struct bridge *bridge = NULL;
  struct notification_entry *entry[2];
  struct in_addr ifaddr, broad;
  char *label = "test";
  int ifindex = 1;
  int prefixlen = 32;
  size_t get_num;
  unsigned int bbq_size = 0;
  uint8_t hwaddr[6] = {0};
  char arp_msg[60] = {0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
                      0x00, 0x80, 0x00, 0x00, 0x02, 0x08, 0x00,
                      0x01, 0x00, 0xc0, 0xa8, 0x01, 0x01, 0x0a,
                      0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x01, 0x00, 0x00, 0x08, 0x00, 0x04,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00,
                      0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00};

  /* preparation */
  test.h.nlmsg_len = 76;
  test.h.nlmsg_type = RTM_NEWNEIGH;
  test.h.nlmsg_flags = 2;
  test.h.nlmsg_seq = 2;
  test.h.nlmsg_pid = 1000;
  memcpy(test.kernel_msg, arp_msg, sizeof(arp_msg));

  ifaddr.s_addr = inet_addr("192.168.1.1");
  broad.s_addr = inet_addr("192.168.255.255");
  rib_notifier_ipv4_addr_add(ifindex, &ifaddr, prefixlen, &broad, label);

  netlink_neigh(NULL, &test);

  /* check notification entry of rib */
  rv = dp_tapio_interface_info_get("test", hwaddr, &bridge);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_NOT_NULL(bridge);
  bbq_size = lagopus_bbq_size(&bridge->rib.notification_queue);
  TEST_ASSERT_EQUAL(bbq_size, 2);
  lagopus_bbq_get_n(&bridge->rib.notification_queue, &entry, bbq_size, 0,
                    struct notification_entry *, 0, &get_num);
  TEST_ASSERT_NOT_NULL(&entry[1]);

  /* entry[0] is ipv4 addr notification entry */
  TEST_ASSERT_EQUAL(entry[1]->type, NOTIFICATION_TYPE_ARP);
  TEST_ASSERT_EQUAL(entry[1]->action, NOTIFICATION_ACTION_TYPE_ADD);
  TEST_ASSERT_EQUAL(entry[1]->arp.ip.s_addr, inet_addr("192.168.1.1"));
  TEST_ASSERT_EQUAL_MEMORY(entry[1]->arp.mac, hwaddr, ETH_LEN);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_netlink_neigh_type_del(void) {
#ifdef HYBRID
  struct test {
    struct nlmsghdr h;
    unsigned char kernel_msg[60];
  };

  lagopus_result_t rv;
  struct test test;
  struct bridge *bridge = NULL;
  struct notification_entry *entry[2];
  struct in_addr ifaddr, broad;
  char *label = "test";
  int ifindex = 1;
  int prefixlen = 32;
  size_t get_num;
  unsigned int bbq_size = 0;
  uint8_t hwaddr[6] = {0};
  char arp_msg[60] = {0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
                      0x00, 0x80, 0x00, 0x00, 0x02, 0x08, 0x00,
                      0x01, 0x00, 0xc0, 0xa8, 0x01, 0x01, 0x0a,
                      0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x01, 0x00, 0x00, 0x08, 0x00, 0x04,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00,
                      0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00};

  /* preparation */
  test.h.nlmsg_len = 76;
  test.h.nlmsg_type = RTM_DELNEIGH;
  test.h.nlmsg_flags = 2;
  test.h.nlmsg_seq = 3;
  test.h.nlmsg_pid = 1000;
  memcpy(test.kernel_msg, arp_msg, sizeof(arp_msg));

  ifaddr.s_addr = inet_addr("192.168.1.1");
  broad.s_addr = inet_addr("192.168.255.255");
  rib_notifier_ipv4_addr_add(ifindex, &ifaddr, prefixlen, &broad, label);

  netlink_neigh(NULL, &test);

  /* check notification entry of rib */
  rv = dp_tapio_interface_info_get("test", hwaddr, &bridge);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_NOT_NULL(bridge);
  bbq_size = lagopus_bbq_size(&bridge->rib.notification_queue);
  TEST_ASSERT_EQUAL(bbq_size, 2);
  lagopus_bbq_get_n(&bridge->rib.notification_queue, &entry, bbq_size, 0,
                    struct notification_entry *, 0, &get_num);
  TEST_ASSERT_NOT_NULL(&entry[1]);

  /* entry[0] is ipv4 addr notification entry */
  TEST_ASSERT_EQUAL(entry[1]->type, NOTIFICATION_TYPE_ARP);
  TEST_ASSERT_EQUAL(entry[1]->action, NOTIFICATION_ACTION_TYPE_DEL);
  TEST_ASSERT_EQUAL(entry[1]->arp.ip.s_addr, inet_addr("192.168.1.1"));
  TEST_ASSERT_EQUAL_MEMORY(entry[1]->arp.mac, hwaddr, ETH_LEN);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_netlink_route_type_add(void) {
#ifdef HYBRID
  struct test {
    struct nlmsghdr h;
    unsigned char kernel_msg[44];
  };

  lagopus_result_t rv;
  struct test test;
  struct bridge *bridge = NULL;
  struct notification_entry *entry[2];
  struct in_addr ifaddr, broad;
  char *label = "test";
  int ifindex = 1;
  int prefixlen = 32;
  size_t get_num;
  unsigned int bbq_size = 0;
  uint8_t hwaddr[6] = {0};
  char route_msg[44] = {0x02, 0x10, 0x00, 0x00, 0xfe, 0x02, 0xfd,
                        0x01, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00,
                        0x0f, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x08,
                        0x00, 0x01, 0x00, 0xc0, 0xa8, 0x00, 0x00,
                        0x08, 0x00, 0x07, 0x00, 0xc0, 0xa8, 0x01,
                        0x01, 0x08, 0x00, 0x04, 0x00, 0x01, 0x00,
                        0x00, 0x00};

  /* preparation */
  test.h.nlmsg_len = 60;
  test.h.nlmsg_type = RTM_NEWROUTE;
  test.h.nlmsg_flags = 2;
  test.h.nlmsg_seq = 4;
  test.h.nlmsg_pid = 1000;
  memcpy(test.kernel_msg, route_msg, sizeof(route_msg));

  ifaddr.s_addr = inet_addr("192.168.1.1");
  broad.s_addr = inet_addr("192.168.255.255");
  rib_notifier_ipv4_addr_add(ifindex, &ifaddr, prefixlen, &broad, label);

  netlink_route(NULL, &test);

  /* check notification entry of rib */
  rv = dp_tapio_interface_info_get("test", hwaddr, &bridge);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_NOT_NULL(bridge);
  bbq_size = lagopus_bbq_size(&bridge->rib.notification_queue);
  TEST_ASSERT_EQUAL(bbq_size, 2);
  lagopus_bbq_get_n(&bridge->rib.notification_queue, &entry, bbq_size, 0,
                    struct notification_entry *, 0, &get_num);
  TEST_ASSERT_NOT_NULL(&entry[1]);

  /* entry[0] is ipv4 addr notification entry */
  TEST_ASSERT_EQUAL(entry[1]->type, NOTIFICATION_TYPE_ROUTE);
  TEST_ASSERT_EQUAL(entry[1]->action, NOTIFICATION_ACTION_TYPE_ADD);
  TEST_ASSERT_EQUAL(entry[1]->route.dest.s_addr, inet_addr("192.168.0.0"));
  TEST_ASSERT_EQUAL(entry[1]->route.gate.s_addr, inet_addr("0.0.0.0"));
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_netlink_route_type_del(void) {
#ifdef HYBRID
  struct test {
    struct nlmsghdr h;
    unsigned char kernel_msg[44];
  };

  lagopus_result_t rv;
  struct test test;
  struct bridge *bridge = NULL;
  struct notification_entry *entry[2];
  struct in_addr ifaddr, broad;
  char *label = "test";
  int ifindex = 1;
  int prefixlen = 32;
  size_t get_num;
  unsigned int bbq_size = 0;
  uint8_t hwaddr[6] = {0};
  char route_msg[44] = {0x02, 0x10, 0x00, 0x00, 0xfe, 0x02, 0xfd,
                        0x01, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00,
                        0x0f, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x08,
                        0x00, 0x01, 0x00, 0xc0, 0xa8, 0x00, 0x00,
                        0x08, 0x00, 0x07, 0x00, 0xc0, 0xa8, 0x01,
                        0x01, 0x08, 0x00, 0x04, 0x00, 0x01, 0x00,
                        0x00, 0x00};

  /* preparation */
  test.h.nlmsg_len = 60;
  test.h.nlmsg_type = RTM_DELROUTE;
  test.h.nlmsg_flags = 2;
  test.h.nlmsg_seq = 5;
  test.h.nlmsg_pid = 1000;
  memcpy(test.kernel_msg, route_msg, sizeof(route_msg));

  ifaddr.s_addr = inet_addr("192.168.1.1");
  broad.s_addr = inet_addr("192.168.255.255");
  rib_notifier_ipv4_addr_add(ifindex, &ifaddr, prefixlen, &broad, label);

  netlink_route(NULL, &test);

  /* check notification entry of rib */
  rv = dp_tapio_interface_info_get("test", hwaddr, &bridge);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_NOT_NULL(bridge);
  bbq_size = lagopus_bbq_size(&bridge->rib.notification_queue);
  TEST_ASSERT_EQUAL(bbq_size, 2);
  lagopus_bbq_get_n(&bridge->rib.notification_queue, &entry, bbq_size, 0,
                    struct notification_entry *, 0, &get_num);
  TEST_ASSERT_NOT_NULL(&entry[1]);

  /* entry[0] is ipv4 addr notification entry */
  TEST_ASSERT_EQUAL(entry[1]->type, NOTIFICATION_TYPE_ROUTE);
  TEST_ASSERT_EQUAL(entry[1]->action, NOTIFICATION_ACTION_TYPE_DEL);
  TEST_ASSERT_EQUAL(entry[1]->route.dest.s_addr, inet_addr("192.168.0.0"));
  TEST_ASSERT_EQUAL(entry[1]->route.gate.s_addr, inet_addr("0.0.0.0"));
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}
