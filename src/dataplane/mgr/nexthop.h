/* %COPYRIGHT% */

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
