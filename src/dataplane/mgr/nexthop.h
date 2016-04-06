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
 *      @file   nexthop.h
 *      @brief  Nexthop handling library.
 */

#ifndef SRC_DATAPLANE_MGR_NEXTHOP_H_

#include <stdint.h>
#include "lagopus/ptree.h"

struct nexthop_ipv4
{
  /* Patricia tree node.  */
  pt_node_t pt_node;
  
  /* Nexthop address. */
  struct in_addr gate;

  /* Reference count from RIB. */
  uint32_t refcnt;
  
  /* Nexthop interface index. */
  int ifindex;

  /* Scope of interface. */
  uint8_t scope;
};

struct nexthop_ipv6
{
  /* Reference count from RIB. */
  uint32_t refcnt;
  
  /* Patricia tree node.  */
  pt_node_t pt_node;
  
  /* Nexthop address. */
  struct in6_addr gate;

  /* Nexthop interface index. */
  int ifindex;
};

struct nexthop_ipv4_table
{
  pt_tree_t ptree;
};

struct nexthop_ipv6_table
{
  pt_tree_t ptree;
};

struct nexthop_ipv4_table *
nexthop_ipv4_table_alloc(void);

void
nexthop_ipv4_table_free(struct nexthop_ipv4_table *nhop_table);

struct nexthop_ipv4 *
nexthop_ipv4_get(struct nexthop_ipv4_table *nexthop_table,
                 struct in_addr *gate, int ifindex, uint8_t scope);

struct nexthop_ipv4 *
nexthop_ipv4_lookup(struct nexthop_ipv4_table *nexthop_table,
                    struct in_addr *gate);

void
nexthop_ipv4_release(struct nexthop_ipv4_table *nexthop_table,
                     struct nexthop_ipv4 *nexthop);

#endif /* SRC_DATAPLANE_MGR_NEXTHOP_H_ */
