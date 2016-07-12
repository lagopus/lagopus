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
 *      @file   route.h
 *      @brief  Routing table.
 */

#ifndef SRC_DATAPLANE_MGR_ROUTE_H_
#define SRC_DATAPLANE_MGR_ROUTE_H_

#include <net/if.h>

#define LPM_PTREE
#ifdef LPM_PTREE /* LPM_PTREE */
#define _PT_PRIVATE
#include "lagopus/ptree.h"
#elif defined LPM_DIR_24_8 /* LPM_DIR_24_8 */
#include <rte_config.h>
#include <rte_ip.h>
#include <rte_lpm.h>
#include <rte_eal.h>
#include <rte_lcore.h>
#endif /* LPM_XXX */

/**
 * Route table.
 */
struct route_table {
#ifdef LPM_PTREE
  pt_tree_t table;  /**< ptree to registered route informations. */
  uint32_t num;     /**< num of entries. */
#endif /* LPM_PTREE */
};

/* ROUTE APIs. */
void route_init(struct route_table *route_table);
void route_fini(struct route_table *route_table);

lagopus_result_t
route_entry_add(struct route_table *route_table, struct in_addr *dest,
                int prefixlen, struct in_addr *gate, int ifindex,
                uint8_t scope, uint8_t *mac);

lagopus_result_t
route_entry_delete(struct route_table *route_table, struct in_addr *dest,
                   int prefixlen, struct in_addr *gate, int ifindex);

lagopus_result_t
route_entry_update(struct route_table *route_table, struct in_addr *dest,
                   int prefixlen, struct in_addr *gate, int ifindex,
                   uint8_t scope, uint8_t *mac);

lagopus_result_t
route_entry_modify(struct route_table *route_table,
                   int in_ifindex, uint8_t *in_mac);

void
route_ipv6_add(struct in6_addr *dest, int prefixlen,
               struct in6_addr *gate, int ifindex);

void
route_ipv6_delete(struct in6_addr *dest, int prefixlen,
                  struct in6_addr *gate, int ifindex);

lagopus_result_t
route_entry_get(struct route_table *route_table,
                const struct in_addr *ip_dst, int prefixlen,
                struct in_addr *nexthop, uint8_t *scope, uint8_t *mac);

lagopus_result_t
route_rule_get(struct route_table *route_table, struct in_addr *dest,
               struct in_addr *gate, int *prefixlen, uint32_t *ifindex,
               uint8_t *scope, void **item);

void
route_entries_all_clear(struct route_table *route_table);

lagopus_result_t
route_entries_all_copy(struct route_table *src, struct route_table *dst);

#endif /* SRC_DATAPLANE_MGR_ROUTE_H_ */
