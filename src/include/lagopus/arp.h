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

/**
 *      @file   arp.h
 *      @brief  ARP table.
 */

#ifndef SRC_DATAPLANE_MGR_ARP_H_
#define SRC_DATAPLANE_MGR_ARP_H_

#include <net/if.h>

/**
 * ARP table.
 */
struct arp_table {
  lagopus_hashmap_t hashmap; /**< hashmap to registered arp informations. */
  lagopus_rwlock_t lock;
};

/* ARP APIs. */
void arp_init(struct arp_table *arp_table);
void arp_fini(struct arp_table *arp_table);

lagopus_result_t
arp_entry_delete(struct arp_table *arp_table, int ifindex,
                 struct in_addr *dst_addr, uint8_t *ll_addr);

lagopus_result_t
arp_entry_update(struct arp_table *arp_table, int ifindex,
                 struct in_addr *dst_addr, uint8_t *ll_addr);

lagopus_result_t
arp_get(struct arp_table *arp_table, struct in_addr *addr,
        uint8_t *mac, int *ifindex);

lagopus_result_t
arp_entries_all_clear(struct arp_table *arp_table);

lagopus_result_t
arp_entries_all_copy(struct arp_table *src, struct arp_table *dst);
#endif /* SRC_DATAPLANE_MGR_ARP_H_ */

