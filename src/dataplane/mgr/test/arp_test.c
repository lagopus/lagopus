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
#include "arp.c"
#endif /* HYBRID */

#ifdef HYBRID
static struct arp_table arp_table;
#endif /* HYBRID */

void
setUp(void) {
#ifdef HYBRID
  arp_init(&arp_table);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
tearDown(void) {
#ifdef HYBRID
  arp_fini(&arp_table);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_arp_entry_update(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr dst_addr;
  struct arp_entry *arp;
  int ifindex = 1;
  uint8_t mac_addr[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };

  /* preparation */
  dst_addr.s_addr = inet_addr("192.168.1.1");

  /* create new arp entry */
  rv = arp_entry_update(&arp_table, ifindex, &dst_addr, mac_addr);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check arp entry */
  rv = lagopus_hashmap_find(&arp_table.hashmap, dst_addr.s_addr, (void **)&arp);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(arp->ifindex, ifindex);
  TEST_ASSERT_EQUAL(arp->ip.s_addr, dst_addr.s_addr);
  TEST_ASSERT_EQUAL_MEMORY(arp->mac_addr, mac_addr, 6);

  /* update arp entry */
  ifindex = 2;
  rv = arp_entry_update(&arp_table, ifindex, &dst_addr, mac_addr);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check arp entry */
  rv = lagopus_hashmap_find(&arp_table.hashmap, dst_addr.s_addr, (void **)&arp);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(arp->ifindex, ifindex);
  TEST_ASSERT_EQUAL(arp->ip.s_addr, dst_addr.s_addr);
  TEST_ASSERT_EQUAL_MEMORY(arp->mac_addr, mac_addr, 6);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_arp_get(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr addr1, addr2;
  int set_ifindex = 1;
  int get_ifindex = 0;
  uint8_t set_mac_addr[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
  uint8_t get_mac_addr[6] = { 0 };

  /* preparation */
  addr1.s_addr = inet_addr("192.168.1.1");
  addr2.s_addr = inet_addr("192.168.2.2");
  arp_entry_update(&arp_table, set_ifindex, &addr1, set_mac_addr);

  /* get arp entry */
  rv = arp_get(&arp_table, &addr1, get_mac_addr, &get_ifindex);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL_MEMORY(set_mac_addr, get_mac_addr, 6);
  TEST_ASSERT_EQUAL(set_ifindex, get_ifindex);

  /* get arp entry */
  rv = arp_get(&arp_table, &addr2, get_mac_addr, &get_ifindex);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  TEST_ASSERT_EQUAL(get_ifindex, -1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_arp_entry_delete(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr addr;
  int set_ifindex = 1;
  int get_ifindex = 0;
  uint8_t set_mac_addr[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
  uint8_t get_mac_addr[6] = { 0 };

  /* preparation */
  addr.s_addr = inet_addr("192.168.1.1");
  arp_entry_update(&arp_table, set_ifindex, &addr, set_mac_addr);

  /* delete arp entry */
  rv = arp_entry_delete(&arp_table, set_ifindex, &addr, set_mac_addr);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* get arp entry */
  rv = arp_get(&arp_table, &addr, get_mac_addr, &get_ifindex);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_arp_entries_all_copy(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct arp_table arp_table2;
  struct in_addr addr1, addr2;
  int set_ifindex1 = 1, set_ifindex2 = 2;
  int get_ifindex1 = 0, get_ifindex2 = 0;
  uint8_t set_mac_addr1[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
  uint8_t set_mac_addr2[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 };
  uint8_t get_mac_addr1[6] = { 0 }, get_mac_addr2[6] = { 0 };

  /* preparation */
  arp_init(&arp_table2);
  addr1.s_addr = inet_addr("192.168.1.1");
  addr2.s_addr = inet_addr("192.168.2.2");
  arp_entry_update(&arp_table, set_ifindex1, &addr1, set_mac_addr1);
  arp_entry_update(&arp_table, set_ifindex2, &addr2, set_mac_addr2);

  /* copy all arp entries */
  rv = arp_entries_all_copy(&arp_table, &arp_table2);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* get arp entry */
  rv = arp_get(&arp_table2, &addr1, get_mac_addr1, &get_ifindex1);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(get_ifindex1, set_ifindex1);
  TEST_ASSERT_EQUAL_MEMORY(get_mac_addr1, set_mac_addr1, 6);
  rv = arp_get(&arp_table2, &addr2, get_mac_addr2, &get_ifindex2);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(get_ifindex2, set_ifindex2);
  TEST_ASSERT_EQUAL_MEMORY(get_mac_addr2, set_mac_addr2, 6);

  /* clean up */
  arp_fini(&arp_table2);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}
