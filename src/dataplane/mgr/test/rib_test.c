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
#include "route.c"
#include "arp.c"
#include "lagopus/mactable.h"
#include "../dataplane/ofproto/packet.h"
#endif /* HYBRID */

#ifdef HAVE_DPDK
#include "../dpdk/lib/librte_eal/common/include/rte_lcore.h"
#endif /* HAVE_DPDK */

#define ETH_LEN  6

#ifdef HYBRID
static struct rib rib;
#endif /* HYBRID */

void
setUp(void) {
#ifdef HYBRID
  rib_init(&rib);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
tearDown(void) {
#ifdef HYBRID
  rib_fini(&rib);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID*/
}

void
test_add_fib_entry(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr dst;
  struct fib_entry *fib_entry;
  uint8_t src_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
  uint8_t dst_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
  uint32_t port = 1;

  /* preparation */
  dst.s_addr = inet_addr("192.168.1.1");

  /* add entry */
  rv = add_fib_entry(&rib.fib[0], dst, src_mac, dst_mac, port);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check entry */
  rv = lagopus_hashmap_find(&rib.fib[0].localcache,
                            (void *)(dst.s_addr),
                            (void **)&fib_entry);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL_MEMORY(fib_entry->src_mac, src_mac, ETH_LEN);
  TEST_ASSERT_EQUAL_MEMORY(fib_entry->dst_mac, dst_mac, ETH_LEN);
  TEST_ASSERT_EQUAL(fib_entry->output_port, port);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID*/
}

void
test_find_fib_entry(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr dst1, dst2;
  struct fib_entry *fib_entry;
  uint8_t src_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
  uint8_t dst_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
  uint32_t port = 1;

  /* preparation */
  dst1.s_addr = inet_addr("192.168.1.1");
  dst2.s_addr = inet_addr("192.168.2.2");
  add_fib_entry(&rib.fib[0], dst1, src_mac, dst_mac, port);

  /* find entry */
  rv = find_fib_entry(&rib.fib[0], dst1, &fib_entry);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check entry */
  TEST_ASSERT_EQUAL_MEMORY(fib_entry->src_mac, src_mac, ETH_LEN);
  TEST_ASSERT_EQUAL_MEMORY(fib_entry->dst_mac, dst_mac, ETH_LEN);
  TEST_ASSERT_EQUAL(fib_entry->output_port, port);

  /* find entry */
  rv = find_fib_entry(&rib.fib[0], dst2, &fib_entry);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID*/
}

void
test_rib_lookup1(void) {
#if defined HYBRID && !defined PIPELINER
  lagopus_result_t rv;
  struct lagopus_packet *pkt;
  struct in_addr dst1;
  struct port *port;
  uint8_t src_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
  uint8_t dst_mac1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
  uint32_t portid = 1;
  char bridge_name1[] = "br1";

  /* preparation */
#ifdef HAVE_DPDK
  RTE_PER_LCORE(_lcore_id) = 0;
#endif /* HAVE_DPDK */

  /* create lagopus packet */
  port = port_alloc();
  port->bridge = bridge_alloc(bridge_name1);
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL(pkt);
  lagopus_packet_init(pkt, NULL, port);
  pkt->ipv4->ip_dst.s_addr = inet_addr("192.168.1.1");

  /* add fib entry */
  dst1.s_addr = inet_addr("192.168.1.1");
  add_fib_entry(&pkt->bridge->rib.fib[0], dst1,
                src_mac, dst_mac1, portid);

  /* lookup from fib */
  rv = rib_lookup(pkt);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check packet */
  TEST_ASSERT_EQUAL(pkt->output_port, portid);
  TEST_ASSERT_EQUAL_MEMORY(ETHER_DST(pkt->eth), dst_mac1, ETH_LEN);
  TEST_ASSERT_EQUAL_MEMORY(ETHER_SRC(pkt->eth), src_mac, ETH_LEN);

  /* lookup with no arp entry */
  pkt->ipv4->ip_dst.s_addr = inet_addr("192.168.3.3");
  rv = rib_lookup(pkt);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);

  /* cleanup */
  bridge_free(port->bridge);
  port_free(port);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID*/
}

void
test_rib_lookup2(void) {
#if defined HYBRID && !defined PIPELINER
  lagopus_result_t rv;
  struct macentry *macentry;
  struct fib_entry *fib_entry;
  struct lagopus_packet *pkt;
  struct port *port;
  struct in_addr dst1, gate;
  uint8_t scope = 0;
  uint8_t src_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
  uint8_t dst_mac1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
  uint8_t dst_mac2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
  uint64_t inteth = 0x20000000000;
  uint32_t portid = 1;
  int ifindex = 1, prefixlen = 32;
  char bridge_name1[] = "br1";

  /* preparation */
#ifdef HAVE_DPDK
  RTE_PER_LCORE(_lcore_id) = 0;
#endif /* HAVE_DPDK */

  /* create lagopus packet */
  port = port_alloc();
  port->bridge = bridge_alloc(bridge_name1);
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL(pkt);
  lagopus_packet_init(pkt, NULL, port);
  dst1.s_addr = inet_addr("192.168.1.1");
  pkt->ipv4->ip_dst.s_addr = dst1.s_addr;

  /* create macentry */
  macentry = calloc(1, sizeof(struct macentry));
  macentry->inteth = inteth;
  macentry->portid = portid;
  macentry->address_type = MACTABLE_SETTYPE_DYNAMIC;

  /* add route entry */
  gate.s_addr = inet_addr("192.168.2.100");
  route_entry_add(&pkt->bridge->rib.ribs[0].route_table,
                  &dst1, prefixlen, &gate,
                  ifindex, scope, src_mac);

  /* add arp entry */
  arp_entry_update(&pkt->bridge->rib.ribs[0].arp_table,
                   ifindex, &gate, dst_mac1);
  /* add mac entry */
  rv = lagopus_hashmap_add(&pkt->in_port->bridge->mactable.local[0].localcache,
                           (void *)inteth, (void **)&macentry, true);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* lookup from routing table */
  rv = rib_lookup(pkt);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check packet */
  TEST_ASSERT_EQUAL(pkt->output_port, portid);
  TEST_ASSERT_EQUAL_MEMORY(ETHER_DST(pkt->eth), dst_mac1, 6);
  TEST_ASSERT_EQUAL_MEMORY(ETHER_SRC(pkt->eth), src_mac, 6);

  /* check fib entry */
  rv = find_fib_entry(&pkt->bridge->rib.fib[0], dst1, &fib_entry);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL_MEMORY(fib_entry->src_mac, src_mac, 6);
  TEST_ASSERT_EQUAL_MEMORY(fib_entry->dst_mac, dst_mac1, 6);

  /* cleanup */
  bridge_free(port->bridge);
  port_free(port);
  lagopus_packet_free(pkt);
  free(macentry);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID*/
}

void
test_rib_create_notification_entry(void) {
#ifdef HYBRID
  struct notification_entry *entry = NULL;
  uint8_t type = NOTIFICATION_TYPE_ROUTE;
  uint8_t action = NOTIFICATION_ACTION_TYPE_ADD;

  /* create entry */
  entry = rib_create_notification_entry(type, action);
  TEST_ASSERT_NOT_NULL(entry);
  TEST_ASSERT_EQUAL(entry->type, type);
  TEST_ASSERT_EQUAL(entry->action, action);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID*/
}

void
test_rib_add_notification_entry(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct notification_entry *entry;
  uint8_t type = NOTIFICATION_TYPE_ROUTE;
  uint8_t action = NOTIFICATION_ACTION_TYPE_ADD;

  /* preparation */
  entry = rib_create_notification_entry(type, action);

  /* add entry */
  rv = rib_add_notification_entry(&rib, entry);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID*/
}

void
test_update_tables1(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr dst1, dst2;
  struct notification_entry *entry1, *entry2;
  struct arp_entry *arp_entry;
  uint8_t dst_mac1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
  uint8_t dst_mac2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
  uint8_t scope = 0;
  int ifindex = 1;
  int prefixlen = 32;

  /* preparation */
  rib.read_table = 0;
  dst1.s_addr = inet_addr("192.168.1.101");
  dst2.s_addr = inet_addr("192.168.2.102");

  /* create notification entry of arp */
  entry1 = rib_create_notification_entry(NOTIFICATION_TYPE_ARP,
                                         NOTIFICATION_ACTION_TYPE_DEL);
  entry1->arp.ifindex = ifindex;
  entry1->arp.ip = dst1;
  memcpy(entry1->arp.mac, dst_mac1, ETH_LEN);

  entry2 = rib_create_notification_entry(NOTIFICATION_TYPE_ARP,
                                         NOTIFICATION_ACTION_TYPE_ADD);
  entry2->arp.ifindex = ifindex;
  entry2->arp.ip = dst2;
  memcpy(entry2->arp.mac, dst_mac2, ETH_LEN);

  /* add arp entry and notification entry */
  rv = arp_entry_update(&rib.ribs[0].arp_table,
                   ifindex, &dst1, dst_mac1);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = rib_add_notification_entry(&rib, entry1);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = rib_add_notification_entry(&rib, entry2);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* update tables */
  rv = update_tables(&rib, rib.read_table);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check arp table */
  rv = lagopus_hashmap_find(&rib.ribs[1].arp_table.hashmap,
                            (void *)(dst1.s_addr),
                            (void **)&arp_entry);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = lagopus_hashmap_find(&rib.ribs[1].arp_table.hashmap,
                            (void *)(dst2.s_addr),
                            (void **)&arp_entry);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(arp_entry->ip.s_addr, dst2.s_addr);
  TEST_ASSERT_EQUAL_MEMORY(arp_entry->mac_addr, dst_mac2, ETH_LEN);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID*/
}

void
test_update_tables2(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr dst1, dst2, gate1, gate2;
  struct notification_entry *entry1, *entry2;
  struct route_entry *route_entry;
  uint8_t dst_mac1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
  uint8_t dst_mac2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
  uint8_t scope = 0;
  int ifindex = 1;
  int prefixlen = 32;

  /* preparation */
  rib.read_table = 0;
  dst1.s_addr = inet_addr("192.168.3.103");
  dst2.s_addr = inet_addr("192.168.4.104");
  gate1.s_addr = inet_addr("192.168.3.3");
  gate2.s_addr = inet_addr("192.168.4.4");

  /* create notification entry of route */
  entry1 = rib_create_notification_entry(NOTIFICATION_TYPE_ROUTE,
                                         NOTIFICATION_ACTION_TYPE_DEL);
  entry1->route.ifindex = ifindex;
  entry1->route.dest = dst1;
  entry1->route.gate = gate1;
  entry1->route.scope = scope;
  entry1->route.prefixlen = prefixlen;

  entry2 = rib_create_notification_entry(NOTIFICATION_TYPE_ROUTE,
                                         NOTIFICATION_ACTION_TYPE_ADD);
  entry2->route.ifindex = ifindex;
  entry2->route.dest = dst2;
  entry2->route.gate = gate2;
  entry2->route.scope = scope;
  entry2->route.prefixlen = prefixlen;
  memcpy(entry2->route.mac, dst_mac2, ETH_LEN);

  /* add route entry and notification entry */
  rv = route_entry_add(&rib.ribs[0].route_table, &dst1, prefixlen,
                       &gate1, ifindex, scope, dst_mac1);
  rv = rib_add_notification_entry(&rib, entry1);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = rib_add_notification_entry(&rib, entry2);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* update tables */
  rv = update_tables(&rib, rib.read_table);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check route table */
  route_entry = ptree_find_node(&rib.ribs[1].route_table.table, &dst1);
  TEST_ASSERT_NULL(route_entry);
  route_entry = ptree_find_node(&rib.ribs[1].route_table.table, &dst2);
  TEST_ASSERT_NOT_NULL(route_entry);
  TEST_ASSERT_EQUAL(route_entry->dest.s_addr, dst2.s_addr);
  TEST_ASSERT_EQUAL(route_entry->gate.s_addr, gate2.s_addr);
  TEST_ASSERT_EQUAL_MEMORY(route_entry->mac, dst_mac2, ETH_LEN);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID*/
}

void
test_update_tables3(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr dst1, gate1;
  struct notification_entry *entry1;
  struct route_entry *route_entry;
  uint8_t dst_mac1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
  uint8_t dst_mac2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
  uint8_t scope = 0;
  int ifindex = 1;
  int prefixlen = 32;

  /* preparation */
  rib.read_table = 0;
  dst1.s_addr = inet_addr("192.168.1.101");
  gate1.s_addr = inet_addr("192.168.3.3");

  /* create notification entry of ifinfo */
  entry1 = rib_create_notification_entry(NOTIFICATION_TYPE_IFADDR,
                                         NOTIFICATION_ACTION_TYPE_ADD);
  entry1->ifaddr.ifindex = ifindex;
  memcpy(entry1->ifaddr.mac, dst_mac2, ETH_LEN);

  /* add route entry and notification entry */
  rv = route_entry_add(&rib.ribs[0].route_table, &dst1, prefixlen,
                       &gate1, ifindex, scope, dst_mac1);
  rv = rib_add_notification_entry(&rib, entry1);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* update tables */
  rv = update_tables(&rib, rib.read_table);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check route table */
  route_entry = ptree_find_node(&rib.ribs[1].route_table.table, &dst1);
  TEST_ASSERT_NOT_NULL(route_entry);
  TEST_ASSERT_EQUAL(route_entry->dest.s_addr, dst1.s_addr);
  TEST_ASSERT_EQUAL_MEMORY(route_entry->mac, dst_mac2, ETH_LEN);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID*/
}

void
test_rib_update(void) {
#ifdef HYBRID
  lagopus_result_t rv;

  TEST_ASSERT_EQUAL(rib.read_table, 0);
  rv = rib_update(&rib);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(rib.read_table, 1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID*/
}
