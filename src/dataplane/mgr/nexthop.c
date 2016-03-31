/* %COPYRIGHT% */

/**
 *      @file   nexthop.c
 *      @brief  Nexthop handling library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "nexthop.h"
#define _PT_PRIVATE
#include "lagopus/ptree.h"

#define IPV4_BITLEN   32
#define IPV4_SIZE      4
#define IPV6_BITLEN  128
#define IPV6_SIZE     16

static struct nexthop_ipv4 *
nexthop_ipv4_alloc(struct in_addr *gate) {
  struct nexthop_ipv4 *nexthop;

  nexthop = (struct nexthop_ipv4 *)calloc(1, sizeof(struct nexthop_ipv4));
  if (nexthop == NULL) {
    return NULL;
  }
  nexthop->gate = *gate;

  return nexthop;
}

static void
nexthop_ipv4_free(struct nexthop_ipv4 *nhop) {
  free(nhop);
}

struct nexthop_ipv4 *
nexthop_ipv4_get(struct nexthop_ipv4_table *nexthop_table,
                 struct in_addr *gate, int ifindex, uint8_t scope) {
  struct nexthop_ipv4 *nexthop = NULL;
  
  /* We don't support unnumbered link. */
  if (gate == NULL) {
    return NULL;
  }

  /* Lookup nexthop. */
  nexthop = ptree_find_node(&nexthop_table->ptree, gate);
  if (nexthop && nexthop->ifindex == ifindex) {
    /* check nexthop->ifindex also,
       because multiple interfaces may have a same nexthop. */
    nexthop->ifindex = ifindex;
    nexthop->refcnt++;
    nexthop->scope = scope;
    return nexthop;
  }

  nexthop = nexthop_ipv4_alloc(gate);
  if (nexthop == NULL) {
    return NULL;
  }
  nexthop->ifindex = ifindex;
  nexthop->scope = scope;

  ptree_insert_node(&nexthop_table->ptree, nexthop);

  return nexthop;
}

struct nexthop_ipv4 *
nexthop_ipv4_lookup(struct nexthop_ipv4_table *nexthop_table,
                    struct in_addr *gate) {
  /* struct ptree_node *node; */
  struct nexthop_ipv4 *nexthop = NULL;

  if (gate == NULL) {
    return NULL;
  }

#if 0
  node = ptree_node_lookup(nexthop_table->ptree, (uint8_t *)gate, IPV4_BITLEN);
  if (node == NULL) {
    return NULL;
  }

  nexthop = (struct nexthop_ipv4 *)node->info;
  ptree_unlock_node(node);
#endif
  
  return nexthop;
}

void
nexthop_ipv4_release(struct nexthop_ipv4_table *nexthop_table,
                     struct nexthop_ipv4 *nexthop) {
  struct ptree_node *node;
  /* struct nexthop_ipv4 *nexthop; */

#if 0
  node = ptree_node_lookup(nexthop_table->ptree, (uint8_t *)gate, IPV4_BITLEN);
  if (node == NULL) {
    return;
  }

  ptree_unlock_node(node);

  nexthop = (struct nexthop_ipv4 *)node->info;
  if (nexthop == NULL) {
    return;
  }

  if (nexthop->refcnt) {
    nexthop->refcnt--;
    if (nexthop->refcnt == 0) {
      node->info = NULL;
      nexthop_ipv4_free(nexthop);
      ptree_unlock_node(node);
    }
  }
#endif

  return;
}

struct nexthop_ipv4_table *
nexthop_ipv4_table_alloc(void) {
  struct nexthop_ipv4_table *nhop_table;

  nhop_table = (struct nexthop_ipv4_table *)
    calloc(1, sizeof(struct nexthop_ipv4_table));
  if (nhop_table == NULL) {
    return NULL;
  }

  ptree_init(&nhop_table->ptree, NULL,
             (void *)(sizeof(struct in_addr) / sizeof(uint32_t)),
             offsetof(struct nexthop_ipv4, pt_node),
             offsetof(struct nexthop_ipv4, gate));

  return nhop_table;
}

void
nexthop_ipv4_table_free(struct nexthop_ipv4_table *nhop_table)
{
  struct nexthop_ipv4 *nexthop;

  while ((nexthop = ptree_iterate(&nhop_table->ptree, NULL, PT_ASCENDING))
         != NULL) {
    ptree_remove_node(&nhop_table->ptree, nexthop);
    nexthop_ipv4_free(nexthop);
  }
  free(nhop_table);
}
