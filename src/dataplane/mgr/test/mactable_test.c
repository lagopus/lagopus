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

#include <time.h>
#include "lagopus_apis.h"
#include "lagopus/dp_apis.h"
#include "lagopus/flowdb.h"
#include "hashmap.c"

#include "../dataplane/ofproto/packet.h"

#ifdef HYBRID
#include "lagopus/mactable.h"
#include "mactable.c"
#endif /* HYBRID */

#ifdef HAVE_DPDK
#include "../dpdk/build/include/rte_config.h"
#include "../dpdk/lib/librte_eal/common/include/rte_lcore.h"
#endif /* HAVE_DPDK */

#ifdef HYBRID
static struct mactable mactable;
static uint8_t ethaddr[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xAA};
static uint8_t ethaddr2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xBB};
static uint8_t ethaddr3[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xCC};
static uint32_t portid = 1;
static uint32_t portid2 = 2;
static uint32_t portid3 = 3;
static uint64_t inteth = 0xAA0000000000;
static uint64_t inteth2 = 0xBB0000000000;
static uint64_t inteth3 = 0xCC0000000000;
static uint16_t address_type = MACTABLE_SETTYPE_DYNAMIC;
static uint16_t address_type2 = MACTABLE_SETTYPE_STATIC;
#endif /* HYBRID */

void
setUp(void) {
#ifdef HYBRID
  lagopus_result_t rv;

  rv = mactable_init(&mactable);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
tearDown(void) {
#ifdef HYBRID
  lagopus_result_t rv;

  rv = mactable_fini(&mactable);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_get_local_data(void) {
#ifdef HYBRID
  struct local_data *local = NULL;

#ifdef HAVE_DPDK
  unsigned lcore_id = 0;
  RTE_PER_LCORE(_lcore_id) = lcore_id;
#endif /* HAVE_DPDK */

  /* get local data */
  local = get_local_data(&mactable);
  TEST_ASSERT_NOT_NULL(local);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_macentry_alloc(void) {
#ifdef HYBRID
  struct macentry *entry = NULL;

  /* alloc entry */
  entry = macentry_alloc(inteth, portid, address_type);

  /* check entry */
  TEST_ASSERT_NOT_NULL(entry);
  TEST_ASSERT_EQUAL(inteth, entry->inteth);
  TEST_ASSERT_EQUAL(portid, entry->portid);
  TEST_ASSERT_EQUAL(address_type, entry->address_type);

  /* clean up */
  macentry_free(entry);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_add_entry_bbq(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct macentry *entry;

  /* add entry */
  rv = add_entry_bbq(&mactable.local, inteth, portid, address_type);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check entry */
  lagopus_bbq_get(&mactable.local->bbq,
                  &entry,
                  struct macentry *, 0);
  TEST_ASSERT_EQUAL(inteth, entry->inteth);
  TEST_ASSERT_EQUAL(portid, entry->portid);
  TEST_ASSERT_EQUAL(address_type, entry->address_type);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_add_entry_write_table(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  lagopus_hashmap_t *write_table;
  struct macentry *entry;
  struct timespec update_time;
  uint32_t read_table_id;

  /* preparation */
  read_table_id = __sync_add_and_fetch(&mactable.read_table, 0);
  write_table = &mactable.hashmap[read_table_id^1];
  update_time = get_current_time();

  /* add entry */
  rv = add_entry_write_table(&mactable, write_table,
                             inteth, portid,
                             address_type, update_time);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check entry */
  rv = lagopus_hashmap_find(write_table, (void *)inteth, (void **)&entry);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(inteth, entry->inteth);
  TEST_ASSERT_EQUAL(portid, entry->portid);
  TEST_ASSERT_EQUAL(address_type, entry->address_type);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_add_entry_write_table_bad_args(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  lagopus_hashmap_t *write_table;
  struct timespec update_time;
  uint32_t read_table_id;

  /* preparation */
  read_table_id = __sync_add_and_fetch(&mactable.read_table, 0);
  write_table = &mactable.hashmap[read_table_id^1];
  update_time = get_current_time();

  /* add entry */
  rv = add_entry_write_table(NULL, write_table,
                             inteth, portid,
                             address_type, update_time);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);

  rv = add_entry_write_table(&mactable, NULL,
                             inteth, portid,
                             address_type, update_time);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_add_entry_local_cache(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct macentry *entry;
  struct local_data *local;

  /* preparation */
  local = get_local_data(&mactable);

  /* add entry */
  rv = add_entry_local_cache(&local->localcache,
                             inteth, portid, address_type);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check entry */
  rv = lagopus_hashmap_find(&local->localcache,
                            (void *)inteth, (void **)&entry);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(inteth, entry->inteth);
  TEST_ASSERT_EQUAL(portid, entry->portid);
  TEST_ASSERT_EQUAL(address_type, entry->address_type);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_add_entry_local_cache_bad_args(void) {
#ifdef HYBRID
  lagopus_result_t rv;

  /* add entry */
  rv = add_entry_local_cache(NULL, inteth, portid, address_type);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_lookup(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct local_data *local;
  uint16_t addr_type;
  uint32_t port;

  /* preparation */
  local = get_local_data(&mactable);
  add_entry_local_cache(&local->localcache,
                        inteth, portid, address_type);

  /* lookup */
  rv = lookup(&local->localcache, inteth, &port, &addr_type);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(portid, port);
  TEST_ASSERT_EQUAL(address_type, addr_type);

  /* lookup with no match entry */
  rv = lookup(&local->localcache, inteth2, &port, &addr_type);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_lookup_bad_args(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  uint16_t addr_type;
  uint32_t port;

  /* lookup */
  rv = lookup(NULL, inteth, &port, &addr_type);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_check_eth_history(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct local_data *local;

  /* preparation */
  local = get_local_data(&mactable);

  /* check eth history */
  rv = check_eth_history(local, inteth, false);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);

  rv = check_eth_history(local, inteth, false);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_ALREADY_EXISTS);

  rv = check_eth_history(local, inteth, true);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBIRD */
}

void
test_check_referred(void) {
#ifdef HYBRID
  struct local_data *local;

  /* preparation */
  local = get_local_data(&mactable);

  /* check referred */
  TEST_ASSERT_FALSE(check_referred(&mactable, local));
  TEST_ASSERT_EQUAL(local->referred_table, 0);

  mactable.read_table = 1;

  TEST_ASSERT_TRUE(check_referred(&mactable, local));
  TEST_ASSERT_EQUAL(local->referred_table, 1);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_learn_port(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct macentry *entry1, *entry2;
  struct local_data *local;

  /* preparation */
  local = get_local_data(&mactable);

  /* learn port */
  rv = learn_port(&mactable, portid, ethaddr, address_type);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check entry from local cache */
  rv = lagopus_hashmap_find(&local->localcache, (void *)inteth, (void **)&entry1);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(inteth, entry1->inteth);
  TEST_ASSERT_EQUAL(portid, entry1->portid);
  TEST_ASSERT_EQUAL(address_type, entry1->address_type);

  /* check entry from bbq */
  rv = lagopus_bbq_get(&mactable.local->bbq,
                  &entry2,
                  struct macentry *, 0);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(inteth, entry2->inteth);
  TEST_ASSERT_EQUAL(portid, entry2->portid);
  TEST_ASSERT_EQUAL(address_type, entry2->address_type);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_learn_port_bad_args(void) {
#ifdef HYBRID
  lagopus_result_t rv;

  /* learn port */
  rv = learn_port(NULL, portid, ethaddr, address_type);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_lookup_port(void) {
#ifdef HYBRID
  lagopus_hashmap_t *read_table;
  struct local_data *local;
  struct macentry *entry;
  uint32_t port;
  uint32_t read_table_id;

  /* preparation */
  local = get_local_data(&mactable);
  read_table_id = __sync_add_and_fetch(&mactable.read_table, 0);
  read_table = &mactable.hashmap[read_table_id];
  add_entry_local_cache(read_table,
                        inteth, portid, address_type);
  add_entry_local_cache(&local->localcache,
                        inteth2, portid2, address_type);

  /* lookup from read mactable */
  port = lookup_port(&mactable, ethaddr);
  TEST_ASSERT_EQUAL(port, portid);

  /* check entry from bbq */
  lagopus_bbq_get(&mactable.local[0].bbq,
                  &entry,
                  struct macentry *, 0);
  TEST_ASSERT_EQUAL(inteth, entry->inteth);
  TEST_ASSERT_EQUAL(portid, entry->portid);
  TEST_ASSERT_EQUAL(address_type, entry->address_type);

  /* bbq clear */
  lagopus_bbq_clear(&mactable.local->bbq, true);

  /* lookup from local cache */
  port = lookup_port(&mactable, ethaddr2);
  TEST_ASSERT_EQUAL(port, portid2);

  /* check entry from bbq */
  lagopus_bbq_get(&mactable.local[0].bbq,
                  &entry,
                  struct macentry *, 0);
  TEST_ASSERT_EQUAL(inteth2, entry->inteth);
  TEST_ASSERT_EQUAL(portid2, entry->portid);
  TEST_ASSERT_EQUAL(address_type, entry->address_type);

  /* lookup with no match entry */
  port = lookup_port(&mactable, ethaddr3);
  TEST_ASSERT_EQUAL(port, OFPP_ALL);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_lookup_port_bad_args(void) {
#ifdef HYBRID
  lagopus_result_t rv;

  /* lookup port */
  rv = lookup_port(NULL, ethaddr);
  TEST_ASSERT_EQUAL(rv, OFPP_ALL);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_age_out_write_table(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  lagopus_hashmap_t *write_table;
  struct macentry *entry;
  struct timespec update_time;
  uint32_t read_table_id;

  /* preparation */
  update_time = get_current_time();
  mactable_ageing_time_set(&mactable, 1);
  read_table_id = __sync_add_and_fetch(&mactable.read_table, 0);
  write_table = &mactable.hashmap[read_table_id^1];
  /* add dynamic entry. */
  add_entry_write_table(&mactable, write_table,
                        inteth, portid, address_type, update_time);
  /* add static entry. */
  add_entry_write_table(&mactable, write_table,
                        inteth2, portid2, address_type2, update_time);
  sleep(1);

  /* age out */
  rv = age_out_write_table(&mactable, write_table);

  /* check entry */
  rv = lagopus_hashmap_find(write_table, (void *)inteth, (void **)&entry);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = lagopus_hashmap_find(write_table, (void *)inteth2, (void **)&entry);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_age_out_write_table_bad_args(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  lagopus_hashmap_t *write_table;
  struct timespec update_time;
  uint32_t read_table_id;

  /* preparation */
  update_time = get_current_time();
  mactable_ageing_time_set(&mactable, 1);
  read_table_id = __sync_add_and_fetch(&mactable.read_table, 0);
  write_table = &mactable.hashmap[read_table_id^1];
  add_entry_write_table(&mactable, write_table,
                        inteth, portid, address_type, update_time);
  sleep(1);

  /* age out */
  rv = age_out_write_table(&mactable, NULL);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_INVALID_ARGS);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_mactable_update(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  lagopus_hashmap_t *new_table, *old_table;
  struct timespec update_time;
  struct macentry entry;
  uint32_t read_table_id;

  /* preparation */
  update_time = get_current_time();
  read_table_id = __sync_add_and_fetch(&mactable.read_table, 0);
  new_table = &mactable.hashmap[read_table_id^1];
  old_table = &mactable.hashmap[read_table_id];
  add_entry_write_table(&mactable, new_table,
                        inteth, portid, address_type, update_time);
  add_entry_write_table(&mactable, old_table,
                        inteth2, portid2, address_type, update_time);
  add_entry_bbq(&mactable.local, inteth3, portid3, address_type);

  /* check read table index */
  TEST_ASSERT_EQUAL(mactable.read_table, 0);

  /* mactable update */
  rv = mactable_update(&mactable);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  /* check read table index */
  TEST_ASSERT_EQUAL(mactable.read_table, 1);

  /* check entry */
  rv = lagopus_hashmap_find(new_table, (void *)inteth, (void **)&entry);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_NOT_FOUND);
  rv = lagopus_hashmap_find(new_table, (void *)inteth2, (void **)&entry);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = lagopus_hashmap_find(new_table, (void *)inteth3, (void **)&entry);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_mactable_port_learning(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct lagopus_packet *pkt;
  struct port port;
  struct macentry *entry;
  char bridge_name[] = "br0";

  /* preparation */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL(pkt);
  lagopus_packet_init(pkt, NULL, &port);
  pkt->in_port->bridge = bridge_alloc(bridge_name);
  pkt->in_port->ofp_port.port_no = portid;
  pkt->eth->ether_shost[0] = 0x00;
  pkt->eth->ether_shost[1] = 0x00;
  pkt->eth->ether_shost[2] = 0x00;
  pkt->eth->ether_shost[3] = 0x00;
  pkt->eth->ether_shost[4] = 0x00;
  pkt->eth->ether_shost[5] = 0xAA;

  /* learn port */
  mactable_port_learning(pkt);

  /* check entry */
  rv = lagopus_hashmap_find(&pkt->in_port->bridge->mactable.local[0].localcache,
                            (void *)inteth,
                            (void **)&entry);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  TEST_ASSERT_EQUAL(inteth, entry->inteth);
  TEST_ASSERT_EQUAL(portid, entry->portid);

  /* clean up */
  lagopus_packet_free(pkt);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_mactable_port_lookup(void) {
#ifdef HYBRID
  lagopus_hashmap_t *read_table;
  struct lagopus_packet *pkt;
  struct port port;
  char bridge_name[] = "br0";
  int i;
  uint32_t read_table_id;

  /* preparation */
  pkt = alloc_lagopus_packet();
  TEST_ASSERT_NOT_NULL(pkt);
  lagopus_packet_init(pkt, NULL, &port);
  pkt->in_port->bridge = bridge_alloc(bridge_name);

  read_table_id = __sync_add_and_fetch(&mactable.read_table, 0);
  read_table = &pkt->in_port->bridge->mactable.hashmap[read_table_id];

  add_entry_local_cache(read_table, inteth, portid, address_type);
  add_entry_local_cache(&pkt->in_port->bridge->mactable.local[0].localcache,
                        inteth2, portid2, address_type);

  /* lookup from read mactable */
  for (i = 0; i < 6; i++) {
    pkt->eth->ether_dhost[i] = ethaddr[i];
  }
  mactable_port_lookup(pkt);
  TEST_ASSERT_EQUAL(portid, pkt->output_port);

  /* lookup from local cache */
  for (i = 0; i < 6; i++) {
    pkt->eth->ether_dhost[i] = ethaddr2[i];
  }
  mactable_port_lookup(pkt);
  TEST_ASSERT_EQUAL(portid2, pkt->output_port);

  /* lookup with no match entry */
  for (i = 0; i < 6; i++) {
    pkt->eth->ether_dhost[i] = ethaddr3[i];
  }
  mactable_port_lookup(pkt);
  TEST_ASSERT_EQUAL(OFPP_ALL, pkt->output_port);

  /* clean up */
  lagopus_packet_free(pkt);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_mactable_entry_update(void) {
#ifdef HYBRID
  lagopus_result_t rv;
  struct macentry *entry;

  /* entry update */
  rv = mactable_entry_update(&mactable, ethaddr, portid);

  /* check entry */
  lagopus_bbq_get(&mactable.local->bbq,
                  &entry,
                  struct macentry *, 0);
  TEST_ASSERT_EQUAL(inteth, entry->inteth);
  TEST_ASSERT_EQUAL(portid, entry->portid);
  TEST_ASSERT_EQUAL(address_type2, entry->address_type);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}

void
test_mactable_configs(void) {
#ifdef HYBRID
  uint32_t ageing_time = 100;
  uint32_t max_entries = 1000;
  uint32_t ret;

  /* check default ageing time */
  TEST_ASSERT_EQUAL(300, mactable.ageing_time);
  ret = mactable_ageing_time_get(&mactable);
  TEST_ASSERT_EQUAL(300, ret);

  /* check new ageing time */
  mactable_ageing_time_set(&mactable, ageing_time);
  ret = mactable_ageing_time_get(&mactable);
  TEST_ASSERT_EQUAL(ageing_time, ret);

  /* check default max entries */
  TEST_ASSERT_EQUAL(8192, mactable.maxentries);
  ret = mactable_max_entries_get(&mactable);
  TEST_ASSERT_EQUAL(8192, ret);

  /* check new max entries */
  mactable_max_entries_set(&mactable, max_entries);
  ret = mactable_max_entries_get(&mactable);
  TEST_ASSERT_EQUAL(max_entries, ret);
#else /* HYBRID */
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif /* HYBRID */
}
