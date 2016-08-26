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
#include "rib.c"
#endif /* HYBRID */

#include "interface.c"
#include "bridge.c"

#ifdef HYBRID
#include "rib_notifier.c"
#endif /* HYBRID */

#define ETH_LEN  6

lagopus_thread_t *thdptr = NULL;
struct interface *ifp;
int ifindex = 1;
int prefixlen = 32;
char *label = "test";

void
setUp(void) {
#ifdef HYBRID
  lagopus_result_t rv;

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
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
tearDown(void) {
#ifdef HYBRID
  bridge_free(ifp->port->bridge);
  port_free(ifp->port);
  dp_tap_interface_destroy(":test");
  dp_interface_free(ifp);
  dp_tapio_thread_fini();

  rib_notifier_fini();
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_rib_notifier_ipv4_addr_add(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr addr, broad;
  struct rib *rib = NULL;
  struct notification_entry *entry = NULL;
  struct ifinfo_entry *ifentry = NULL;
  uint8_t hwaddr[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

  /* preparation */
  addr.s_addr = inet_addr("192.168.1.1");
  broad.s_addr = inet_addr("192.168.255.255");

  /* add interface information */
  rib_notifier_ipv4_addr_add(ifindex, &addr, prefixlen, &broad, label);

  /* check notification queue */
  ifinfo_rib_get(ifindex, &rib);
  TEST_ASSERT_NOT_NULL(rib);
  lagopus_bbq_get(&rib->notification_queue, &entry,
                  struct notification_entry *, 0);
  TEST_ASSERT_NOT_NULL(entry);
  TEST_ASSERT_EQUAL(entry->type, NOTIFICATION_TYPE_IFADDR);
  TEST_ASSERT_EQUAL(entry->action, NOTIFICATION_ACTION_TYPE_ADD);
  TEST_ASSERT_EQUAL(entry->ifaddr.ifindex, ifindex);

  /* check hashmap */
  lagopus_hashmap_find(&ifinfo_hashmap, (void *)ifindex, (void **)&ifentry);
  TEST_ASSERT_NOT_NULL(ifentry);
  TEST_ASSERT_EQUAL_MEMORY(ifentry->hwaddr, hwaddr, ETH_LEN);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_notifier_ipv4_addr_get(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr addr, broad;
  uint8_t hwaddr[ETH_LEN];
  int ifindex2 = 2;

  /* preparation */
  addr.s_addr = inet_addr("192.168.1.1");
  broad.s_addr = inet_addr("192.168.255.255");
  rib_notifier_ipv4_addr_add(ifindex, &addr, prefixlen, &broad, label);

  /* get interface information */
  rv = rib_notifier_ipv4_addr_get(ifindex, hwaddr);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL_MEMORY(ifp->hw_addr, hwaddr, ETH_LEN);

  /* get interface information */
  rv = rib_notifier_ipv4_addr_get(ifindex2, hwaddr);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_rib_notifier_ipv4_addr_delete(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr addr, broad;
  uint8_t hwaddr[ETH_LEN];

  /* preparation */
  addr.s_addr = inet_addr("192.168.1.1");
  broad.s_addr = inet_addr("192.168.255.255");

  /* add interface information */
  rib_notifier_ipv4_addr_add(ifindex, &addr, prefixlen, &broad, label);

  /* delete interface information */
  rib_notifier_ipv4_addr_delete(ifindex, &addr, prefixlen, &broad, label);

  /* check interface information */
  rv = rib_notifier_ipv4_addr_get(ifindex, hwaddr);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_rib_notifier_arp_add(void) {
#ifdef HYBRID
  struct in_addr addr, broad, dst;
  struct rib *rib = NULL;
  struct notification_entry *entry[2];
  size_t get_num;
  unsigned int bbq_size = 0;
  char hwaddr[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

  /* preparation */
  addr.s_addr = inet_addr("192.168.1.1");
  broad.s_addr = inet_addr("192.168.255.255");
  dst.s_addr = inet_addr("192.168.2.2");
  rib_notifier_ipv4_addr_add(ifindex, &addr, prefixlen, &broad, label);

  /* add arp notification entry */
  rib_notifier_arp_add(ifindex, &dst, hwaddr);

  /* check arp notification entry */
  ifinfo_rib_get(ifindex, &rib);
  TEST_ASSERT_NOT_NULL(rib);
  bbq_size = lagopus_bbq_size(&rib->notification_queue);
  TEST_ASSERT_EQUAL(bbq_size, 2);
  lagopus_bbq_get_n(&rib->notification_queue, &entry, bbq_size, 0,
                    struct notification_entry *, 0, &get_num);
  /*
   * entry[0] is ipv4 addr notification entry
   * that added by rib_notifier_ipv4_addr_add().
   */
  TEST_ASSERT_EQUAL(entry[1]->type, NOTIFICATION_TYPE_ARP);
  TEST_ASSERT_EQUAL(entry[1]->action, NOTIFICATION_ACTION_TYPE_ADD);
  TEST_ASSERT_EQUAL(entry[1]->arp.ip.s_addr, dst.s_addr);
  TEST_ASSERT_EQUAL_MEMORY(entry[1]->arp.mac, hwaddr, ETH_LEN);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_rib_notifier_arp_delete(void) {
#ifdef HYBRID
  struct in_addr addr, broad, dst;
  struct rib *rib = NULL;
  struct notification_entry *entry[2];
  size_t get_num;
  unsigned int bbq_size = 0;
  char hwaddr[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

  /* preparation */
  addr.s_addr = inet_addr("192.168.1.1");
  broad.s_addr = inet_addr("192.168.255.255");
  dst.s_addr = inet_addr("192.168.2.2");
  rib_notifier_ipv4_addr_add(ifindex, &addr, prefixlen, &broad, label);

  /* add arp notification entry */
  rib_notifier_arp_delete(ifindex, &dst, hwaddr);

  /* check arp notification entry */
  ifinfo_rib_get(ifindex, &rib);
  TEST_ASSERT_NOT_NULL(rib);
  bbq_size = lagopus_bbq_size(&rib->notification_queue);
  TEST_ASSERT_EQUAL(bbq_size, 2);
  lagopus_bbq_get_n(&rib->notification_queue, &entry, bbq_size, 0,
                    struct notification_entry *, 0, &get_num);
  /*
   * entry[0] is ipv4 addr notification entry
   * that added by rib_notifier_ipv4_addr_add().
   */
  TEST_ASSERT_EQUAL(entry[1]->type, NOTIFICATION_TYPE_ARP);
  TEST_ASSERT_EQUAL(entry[1]->action, NOTIFICATION_ACTION_TYPE_DEL);
  TEST_ASSERT_EQUAL(entry[1]->arp.ifindex, ifindex);
  TEST_ASSERT_EQUAL(entry[1]->arp.ip.s_addr, dst.s_addr);
  TEST_ASSERT_EQUAL_MEMORY(entry[1]->arp.mac, hwaddr, ETH_LEN);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_rib_notifier_route_add(void) {
#ifdef HYBRID
  struct in_addr addr, broad, dst, gate;
  struct rib *rib = NULL;
  struct notification_entry *entry[2];
  size_t get_num;
  uint8_t scope = 0;
  unsigned int bbq_size = 0;
  char hwaddr[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

  /* preparation */
  addr.s_addr = inet_addr("192.168.1.1");
  broad.s_addr = inet_addr("192.168.255.255");
  dst.s_addr = inet_addr("192.168.2.2");
  gate.s_addr = inet_addr("192.168.2.100");
  rib_notifier_ipv4_addr_add(ifindex, &addr, prefixlen, &broad, label);

  /* add route notification entry */
  rib_notifier_ipv4_route_add(&dst, prefixlen, &gate, ifindex, scope);

  /* check arp notification entry */
  ifinfo_rib_get(ifindex, &rib);
  TEST_ASSERT_NOT_NULL(rib);
  bbq_size = lagopus_bbq_size(&rib->notification_queue);
  TEST_ASSERT_EQUAL(bbq_size, 2);
  lagopus_bbq_get_n(&rib->notification_queue, &entry, bbq_size, 0,
                    struct notification_entry *, 0, &get_num);
  /*
   * entry[0] is ipv4 addr notification entry
   * that added by rib_notifier_ipv4_addr_add().
   */
  TEST_ASSERT_EQUAL(entry[1]->type, NOTIFICATION_TYPE_ROUTE);
  TEST_ASSERT_EQUAL(entry[1]->action, NOTIFICATION_ACTION_TYPE_ADD);
  TEST_ASSERT_EQUAL(entry[1]->route.dest.s_addr, dst.s_addr);
  TEST_ASSERT_EQUAL(entry[1]->route.gate.s_addr, gate.s_addr);
  TEST_ASSERT_EQUAL(entry[1]->route.ifindex, ifindex);
  TEST_ASSERT_EQUAL(entry[1]->route.scope, scope);
  TEST_ASSERT_EQUAL(entry[1]->route.prefixlen, prefixlen);
  TEST_ASSERT_EQUAL_MEMORY(entry[1]->route.mac, hwaddr, ETH_LEN);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_rib_notifier_route_delete(void) {
#ifdef HYBRID
  struct in_addr addr, broad, dst, gate;
  struct rib *rib = NULL;
  struct notification_entry *entry[2];
  size_t get_num;
  unsigned int bbq_size = 0;
  char hwaddr[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

  /* preparation */
  addr.s_addr = inet_addr("192.168.1.1");
  broad.s_addr = inet_addr("192.168.255.255");
  dst.s_addr = inet_addr("192.168.2.2");
  gate.s_addr = inet_addr("192.168.2.100");
  rib_notifier_ipv4_addr_add(ifindex, &addr, prefixlen, &broad, label);

  /* add route notification entry */
  rib_notifier_ipv4_route_delete(&dst, prefixlen, &gate, ifindex);

  /* check arp notification entry */
  ifinfo_rib_get(ifindex, &rib);
  TEST_ASSERT_NOT_NULL(rib);
  bbq_size = lagopus_bbq_size(&rib->notification_queue);
  TEST_ASSERT_EQUAL(bbq_size, 2);
  lagopus_bbq_get_n(&rib->notification_queue, &entry, bbq_size, 0,
                    struct notification_entry *, 0, &get_num);
  /*
   * entry[0] is ipv4 addr notification entry
   * that added by rib_notifier_ipv4_addr_add().
   */
  TEST_ASSERT_EQUAL(entry[1]->type, NOTIFICATION_TYPE_ROUTE);
  TEST_ASSERT_EQUAL(entry[1]->action, NOTIFICATION_ACTION_TYPE_DEL);
  TEST_ASSERT_EQUAL(entry[1]->route.dest.s_addr, dst.s_addr);
  TEST_ASSERT_EQUAL(entry[1]->route.gate.s_addr, gate.s_addr);
  TEST_ASSERT_EQUAL(entry[1]->route.ifindex, ifindex);
  TEST_ASSERT_EQUAL(entry[1]->route.scope, 0);
  TEST_ASSERT_EQUAL(entry[1]->route.prefixlen, prefixlen);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}
