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
#include "route.c"
#endif /* HYBRID */

#ifdef HYBRID
static struct route_table route_table;
#endif /* HYBRID */

void
setUp(void) {
#ifdef HYBRID
  route_init(&route_table);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
tearDown(void) {
#ifdef HYBRID
  route_fini(&route_table);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_entry_alloc(void) {
#ifdef HYBRID
  struct in_addr addr;

  addr.s_addr = inet_addr("192.168.1.1");
  TEST_ASSERT_NOT_NULL(route_entry_alloc(&addr));
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_entry_add(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr dest, gate;
  struct route_entry *entry;
  int prefixlen = 32;
  int ifindex = 1;
  uint8_t scope = 0;
  uint8_t mac_addr[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };

  /* preparation */
  dest.s_addr = inet_addr("192.168.1.1");
  gate.s_addr = inet_addr("0.0.0.0");

  /* add entry */
  rv = route_entry_add(&route_table, &dest, prefixlen,
                       &gate, ifindex, scope, mac_addr);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check entry */
  entry = ptree_find_node(&route_table.table, &dest);
  TEST_ASSERT_EQUAL(entry->dest.s_addr, dest.s_addr);
  TEST_ASSERT_EQUAL(entry->gate.s_addr, gate.s_addr);
  TEST_ASSERT_EQUAL_MEMORY(entry->mac, mac_addr, 6);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_entry_delete(void) {
#ifdef HYBRID[
  lagopus_result_t rv;
  struct in_addr dest, gate;
  struct route_entry *entry;
  int prefixlen = 32;
  int ifindex = 1;
  uint8_t scope = 0;
  uint8_t mac_addr[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };

  /* preparation */
  dest.s_addr = inet_addr("192.168.1.1");
  gate.s_addr = inet_addr("0.0.0.0");
  route_entry_add(&route_table, &dest, prefixlen,
                  &gate, ifindex, scope, mac_addr);

  /* delete entry */
  rv = route_entry_delete(&route_table, &dest, prefixlen, &gate, ifindex);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check entry */
  entry = ptree_find_node(&route_table.table, &dest);
  TEST_ASSERT_NULL(entry);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_entry_get(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr dest, gate, nexthop;
  struct route_entry *entry;
  int prefixlen = 32;
  int ifindex = 1;
  uint8_t set_scope = 0;
  uint8_t get_scope = 0;
  uint8_t set_mac_addr[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
  uint8_t get_mac_addr[6] = { 0 };

  /* preparation */
  dest.s_addr = inet_addr("192.168.1.0");
  gate.s_addr = inet_addr("192.168.2.100");
  route_entry_add(&route_table, &dest, prefixlen,
                  &gate, ifindex, set_scope, set_mac_addr);

  /* get entry */
  rv = route_entry_get(&route_table, &dest, prefixlen,
                       &nexthop, &get_scope, get_mac_addr);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check entry */
  TEST_ASSERT_EQUAL(get_scope, set_scope);
  TEST_ASSERT_EQUAL(nexthop.s_addr, gate.s_addr);
  TEST_ASSERT_EQUAL_MEMORY(get_mac_addr, set_mac_addr, 6);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_entry_update(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr dest, gate, nexthop;
  int prefixlen = 32;
  int ifindex = 1;
  uint8_t set_scope = 0, get_scope = 0;
  uint8_t set_mac_addr1[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
  uint8_t set_mac_addr2[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 };
  uint8_t get_mac_addr[6] = { 0 };

  /* preparation */
  dest.s_addr = inet_addr("192.168.1.1");
  gate.s_addr = inet_addr("0.0.0.0");

  /* add entry */
  rv = route_entry_update(&route_table, &dest, prefixlen,
                          &gate, ifindex, set_scope, set_mac_addr1);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check entry */
  rv = route_entry_get(&route_table, &dest, prefixlen,
                       &nexthop, &get_scope, get_mac_addr);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(get_scope, set_scope);
  TEST_ASSERT_EQUAL(nexthop.s_addr, gate.s_addr);
  TEST_ASSERT_EQUAL_MEMORY(get_mac_addr, set_mac_addr1, 6);


  /* update entry */
  rv = route_entry_update(&route_table, &dest, prefixlen,
                          &gate, ifindex, set_scope, set_mac_addr2);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check entry */
  rv = route_entry_get(&route_table, &dest, prefixlen,
                       &nexthop, &get_scope, get_mac_addr);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(get_scope, set_scope);
  TEST_ASSERT_EQUAL(nexthop.s_addr, gate.s_addr);
  TEST_ASSERT_EQUAL_MEMORY(get_mac_addr, set_mac_addr2, 6);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_entry_modify(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr dest, gate, nexthop;
  struct route_entry *entry;
  int prefixlen = 32;
  int ifindex = 1;
  uint8_t scope = 0;
  uint8_t set_mac_addr1[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
  uint8_t set_mac_addr2[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
  uint8_t get_mac_addr[6] = { 0 };

  /* preparation */
  dest.s_addr = inet_addr("192.168.1.1");
  gate.s_addr = inet_addr("0.0.0.0");
  route_entry_add(&route_table, &dest, prefixlen,
                  &gate, ifindex, scope, set_mac_addr1);

  /* modify entry */
  rv = route_entry_modify(&route_table, ifindex, set_mac_addr2);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check entry */
  route_entry_get(&route_table, &dest, prefixlen,
                  &nexthop, &scope, get_mac_addr);
  TEST_ASSERT_EQUAL_MEMORY(get_mac_addr, set_mac_addr2, 6);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_entries_all_clear(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct in_addr dest1, dest2, gate1, gate2, nexthop;
  struct route_entry *entry;
  int prefixlen = 32;
  int ifindex = 1;
  uint8_t scope = 0;
  uint8_t set_mac_addr[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
  uint8_t get_mac_addr[6] = { 0 };

  /* preparation */
  dest1.s_addr = inet_addr("192.168.1.1");
  gate1.s_addr = inet_addr("0.0.0.0");
  dest2.s_addr = inet_addr("192.168.2.2");
  gate2.s_addr = inet_addr("192.168.1.100");
  route_entry_add(&route_table, &dest1, prefixlen,
                  &gate1, ifindex, scope, set_mac_addr);
  route_entry_add(&route_table, &dest2, prefixlen,
                  &gate2, ifindex, scope, set_mac_addr);

  /* clear all entries */
  route_entries_all_clear(&route_table);

  /* check entry */
  rv = route_entry_get(&route_table, &dest1, prefixlen,
                       &nexthop, &scope, get_mac_addr);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = route_entry_get(&route_table, &dest2, prefixlen,
                       &nexthop, &scope, get_mac_addr);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_route_entries_all_copy(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct route_table route_table2;
  struct in_addr dest1, dest2, gate1, gate2, nexthop;
  struct route_entry *entry;
  int prefixlen = 32;
  int ifindex = 1;
  uint8_t scope = 0;
  uint8_t mac_addr[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };

  /* preparation */
  route_init(&route_table2);
  dest1.s_addr = inet_addr("192.168.1.1");
  gate1.s_addr = inet_addr("0.0.0.0");
  dest2.s_addr = inet_addr("192.168.1.1");
  gate2.s_addr = inet_addr("0.0.0.0");
  route_entry_add(&route_table, &dest1, prefixlen,
                  &gate1, ifindex, scope, mac_addr);
  route_entry_add(&route_table, &dest2, prefixlen,
                  &gate2, ifindex, scope, mac_addr);

  /* copy all entries */
  route_entries_all_copy(&route_table, &route_table2);

  /* check entry */
  rv = route_entry_get(&route_table2, &dest1, prefixlen,
                       &nexthop, &scope, mac_addr);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = route_entry_get(&route_table2, &dest2, prefixlen,
                       &nexthop, &scope, mac_addr);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* clean up */
  route_fini(&route_table2);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}
