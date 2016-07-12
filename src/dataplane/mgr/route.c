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
 *      @file   route.c
 *      @brief  Routing table.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "lagopus/dp_apis.h"
#include <net/ethernet.h>
#include "lagopus/updater.h"

#undef ROUTE_DEBUG
#ifdef ROUTE_DEBUG
#define PRINTF(...)   printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#ifdef LPM_PTREE /*** use ptree ***/
#define IPV4_BITLEN   32
#define IPV6_BITLEN  128
struct route_entry {
  pt_node_t route_node; /* Patricia tree node.  */
  struct in_addr dest;  /* Destination address. */
  struct in_addr gate;  /* Nexthop address. */
  int ifindex;          /* Nexthop interface index. */
  int prefixlen;        /* Length of prefix. */
  uint8_t scope;        /* Scope of interface. */
  uint8_t mac[UPDATER_ETH_LEN];
} __attribute__ ((aligned(128)));


/*** static functions ***/
/**
 * Allocate entry object of route information.
 */
static struct route_ipv4 *
route_entry_alloc(struct in_addr *addr) {
  struct route_entry *entry;

  entry = calloc(1, sizeof(struct route_entry));
  if (entry == NULL) {
    return NULL;
  }
  entry->dest = *addr;

  return entry;
}

/**
 * Free route entry object.
 */
static void
route_entry_free(struct route_entry *entry)
{
  free(entry);
}

/**
 * Whether or not to mask for ptree.
 */
static bool
ptree_mask_filter(void *filter_arg, const void *entry, int pt_filter_mask) {
  int *arg = (int *)filter_arg;
  int val = *arg;
  struct route_entry *re = (struct route_entry *)entry;
  pt_node_t *pt = &re->route_node;

  if (val == IPV4_BITLEN) {
    if (pt_filter_mask == false) {
      return true;
    }
  } else {
    if (pt_filter_mask == true && PTN_MASK_BITLEN(pt) == val) {
      return true;
    }
  }

  return false;
}

/**
 * Output log for route information.
 */
static void
route_entry_log(const char *type_str, struct in_addr *dest,
                int prefixlen, struct in_addr *gate, int ifindex) {
  PRINTF("Route %s:", type_str);

  if (dest) {
    char buf[BUFSIZ];
    inet_ntop(AF_INET, dest, buf, BUFSIZ);
    PRINTF(" %s/%d", buf, prefixlen);
  }

  if (gate) {
    char buf[BUFSIZ];
    inet_ntop(AF_INET, gate, buf, BUFSIZ);
    PRINTF(" gateway %s", buf);
  }

  PRINTF(" ifindex %d\n", ifindex);
}

/*** public functions ***/
/**
 * Initialize route table.
 * Create ptree.
 */
void
route_init(struct route_table *route_table) {
  ptree_init(&route_table->table, NULL,
             (void *)(sizeof(struct in_addr) / sizeof(uint32_t)),
             offsetof(struct route_entry, route_node), /* set node offset */
             offsetof(struct route_entry, dest));      /* set key offset */
}

/**
 * Finalize route table.
 * Delete all entries in ptree.
 */
void
route_fini(struct route_table *route_table) {
  route_entries_all_clear(route_table);
  /* ptree_fini does not exist. */
}

/**
 * Add a route entry to ptree.
 */
lagopus_result_t
route_entry_add(struct route_table *route_table, struct in_addr *dest,
                int prefixlen, struct in_addr *gate, int ifindex,
                uint8_t scope, uint8_t *mac) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  struct route_entry *entry;
  bool ret;

  route_entry_log("add", dest, prefixlen, gate, ifindex);

  if (route_table == NULL || dest == NULL || gate == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  /* create a new entry and add to ptree. */
  entry = route_entry_alloc(dest);
  if (entry == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  entry->ifindex = ifindex;
  entry->scope = scope;
  entry->gate = *gate;
  entry->prefixlen = prefixlen;
  memcpy(entry->mac, mac, UPDATER_ETH_LEN);

  /* insert entry to ptree. */
  if (prefixlen == IPV4_BITLEN) {
    ret = ptree_insert_node(&route_table->table, entry);
  } else {
    ret = ptree_insert_mask_node(&route_table->table, entry, prefixlen);
  }

  /* failed to insert entry, free new entry object. */
  if (!ret) {
    route_entry_free(entry);
    lagopus_msg_warning("Failed to insert the entry to ptree\n");
    return LAGOPUS_RESULT_ANY_FAILURES;
  }
  route_table->num++;

  return rv;
}

/**
 * Delete a route entry from ptree.
 */
lagopus_result_t
route_entry_delete(struct route_table *route_table, struct in_addr *dest,
                   int prefixlen, struct in_addr *gate, int ifindex)   {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  struct route_entry *entry = NULL;
  route_entry_log("delete", dest, prefixlen, gate, ifindex);

  if (route_table == NULL || dest == NULL || gate == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  /* check if the entry is exist.*/
  if (prefixlen == IPV4_BITLEN) {
    entry = ptree_find_node(&route_table->table, dest);
  } else {
    entry = ptree_find_filtered_node(&route_table->table, dest,
                                     ptree_mask_filter, (void *)&prefixlen);
  }

  /* delete the entry from ptree. */
  if (entry) {
    ptree_remove_node(&route_table->table, entry);
    route_entry_free(entry);
  }

  return rv;
}

/**
 * Update a route entry.
 */
lagopus_result_t
route_entry_update(struct route_table *route_table, struct in_addr *dest,
                   int prefixlen, struct in_addr *gate, int ifindex,
                   uint8_t scope, uint8_t *mac) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  struct route_entry *entry = NULL;

  /* delete old entry. */
  rv = route_entry_delete(&route_table->table,
                          dest, prefixlen, gate, ifindex);
  if (rv != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("route_entry_delete failed.\n");
    goto out;
  }
  /* add new entry */
  rv = route_entry_add(&route_table->table,
                       dest, prefixlen, gate, ifindex, scope, mac);
  if (rv != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("route_entry_add failed.\n");
    goto out;
  }

out:
  return rv;
}

/**
 * Get a route entry from ptree.
 */
lagopus_result_t
route_entry_get(struct route_table *route_table,
                const struct in_addr *dest, int prefixlen,
                struct in_addr *nexthop, uint8_t *scope, uint8_t *mac) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  struct route_entry *entry;

  /* check if the entry is exist.*/
  if (prefixlen == IPV4_BITLEN) {
    entry = ptree_find_node(&route_table->table, dest);
  } else {
    entry = ptree_find_filtered_node(&route_table->table, dest,
                                     ptree_mask_filter, (void *)&prefixlen);
  }

  /* set nexthop information. */
  if (entry) {
    *scope = entry->scope;
    *nexthop = entry->gate;
    memcpy(mac, entry->mac, UPDATER_ETH_LEN);
    route_entry_log("get", dest, prefixlen, nexthop, entry->ifindex);
  } else {
    rv = LAGOPUS_RESULT_NOT_FOUND;
  }

  return rv;
}

/**
 * Modify route entries about if information.
 */
lagopus_result_t
route_entry_modify(struct route_table *route_table,
                   int in_ifindex, uint8_t *in_mac) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  struct in_addr dest, gate;
  int prefixlen;
  uint32_t ifindex;
  void *item = NULL;
  uint8_t scope;

  while (1) {
    rv = route_rule_get(&route_table->table,
                        &dest, &gate, &prefixlen, &ifindex, &scope, &item);
    if (rv != LAGOPUS_RESULT_OK || item == NULL) {
      break;
    }

    /* check ifindex. */
    if (ifindex == in_ifindex) {
      /* update entry */
      rv = route_entry_update(&route_table->table,
                              &dest, prefixlen, &gate, in_ifindex,
                              scope, in_mac);
      if (rv != LAGOPUS_RESULT_OK) {
        break;
      }
    }
  }

  return rv;
}

/**
 * Get rules from ptree to dump route iformations by datastore.
 */
lagopus_result_t
route_rule_get(struct route_table *route_table, struct in_addr *dest,
               struct in_addr *gate, int *prefixlen, uint32_t *ifindex,
               uint8_t *scope, void **item) {
  struct route_entry *entry = (struct route_entry *)*item;

  /* get entry */
  if ((entry = ptree_iterate(&route_table->table, entry, PT_ASCENDING))
      != NULL) {
    *dest = entry->dest;
    *gate = entry->gate;
    *ifindex = entry->ifindex;
    *prefixlen = entry->prefixlen;
    *scope = entry->scope;
  }

  *item = entry;

  return LAGOPUS_RESULT_OK;
}

/**
 * Clear all entries in route table.
 */
void
route_entries_all_clear(struct route_table *route_table) {
  struct route_entry *entry = NULL;

  while ((entry = ptree_iterate(&route_table->table, NULL, PT_ASCENDING))
         != NULL) {
    ptree_remove_node(&route_table->table, entry);
    route_entry_free(entry);
    route_table->num--;
  }
}

/**
 * Copy all entries.
 */
lagopus_result_t
route_entries_all_copy(struct route_table *src, struct route_table *dst) {
  lagopus_result_t rv;
  struct route_entry *entry = NULL;

  /* clear dst table. */
  route_entries_all_clear(dst);

  /* get entry from src table. */
  while ((entry = ptree_iterate(&src->table, entry, PT_ASCENDING))
      != NULL) {
    /* create a new entry and add to ptree. */
    rv = route_entry_add(dst, &entry->dest, entry->prefixlen, &entry->gate,
                         entry->ifindex, entry->scope, entry->mac);
    if (rv != LAGOPUS_RESULT_OK) {
      return LAGOPUS_RESULT_ANY_FAILURES;
    }
  }

  return LAGOPUS_RESULT_OK;
}


/*** not supported ipv6. ***/
static void
route_ipv6_log(const char *type_str, struct in6_addr *dest,
               int prefixlen, struct in6_addr *gate, int ifindex) {
  (void) ifindex;

  PRINTF("Route %s:", type_str);

  if (dest) {
    char buf[BUFSIZ];
    inet_ntop(AF_INET6, dest, buf, BUFSIZ);
    PRINTF(" %s/%d", buf, prefixlen);
  }

  if (gate) {
    char buf[BUFSIZ];
    inet_ntop(AF_INET6, gate, buf, BUFSIZ);
    PRINTF(" via %s", buf);
  }

  PRINTF("\n");
}

void
route_ipv6_add(struct in6_addr *dest, int prefixlen,
                   struct in6_addr *gate, int ifindex) {
  route_ipv6_log("add", dest, prefixlen, gate, ifindex);
}

void
route_ipv6_delete(struct in6_addr *dest, int prefixlen,
                      struct in6_addr *gate, int ifindex) {
  route_ipv6_log("del", dest, prefixlen, gate, ifindex);
}

/* end of ptree */
#elif defined LPM_DIR_24_8 /*** use dir-24-8 ***/
#define IPV4_LPM_MAX_RULES 1024
#define IPV4_NEXTHOPS 255
#define LAGOPUS_LCORE_SOCKETS 8

typedef struct rte_lpm lookup_struct_t;
static lookup_struct_t *ipv4_lookup_struct[LAGOPUS_LCORE_SOCKETS];

typedef struct ipv4_nexthop {
  uint8_t set;
  uint32_t ip;
  uint16_t scope;
  uint32_t ifindex;
} ipv4_nexthop_t;
static ipv4_nexthop_t nexthops[IPV4_NEXTHOPS] = {0};


/* end of dir-24-8*/
#endif /* LPM_XXXXX*/

#if 0
#ifdef LPM_DIR_24_8
static unsigned int
get_socket_id() {
  unsigned int sockid = 0;
  uint32_t lcore = rte_lcore_id();
  if (lcore != UINT32_MAX) {
    sockid = rte_lcore_to_socket_id(lcore);
  }
  return sockid;
}

void
route_init(void) {
  unsigned int sockid = 0;
  uint32_t lcore = rte_lcore_id();

  if (lcore != UINT32_MAX) {
    sockid = rte_lcore_to_socket_id(lcore);
  }

  ipv4_lookup_struct[sockid] = rte_lpm_create("lagopus_lpm", sockid,
                                              IPV4_LPM_MAX_RULES, 0); 
  if (ipv4_lookup_struct[sockid] == NULL) {
    printf("unable to create the lpm table\n");
  }
}

void
route_fini(void) {
  struct rte_lpm *lpm = ipv4_lookup_struct[get_socket_id()];
  rte_lpm_delete_all(lpm);
  rte_lpm_free(lpm);
}

static void
route_ipv4_dump(void)
{
  /* TODO */
}

int
route_ipv4_add(struct in_addr *dest, int prefixlen, struct in_addr *gate,
                   int ifindex, uint8_t scope) {
  int ret = LAGOPUS_RESULT_ANY_FAILURES;
  int i, index = 0;
  char ifname[IFNAMSIZ] = {0};
  unsigned int sockid = get_socket_id();

  route_ipv4_log("add", dest, prefixlen, gate, ifindex);
  if (prefixlen < 1) {
    return LAGOPUS_RESULT_OK;
  }

  for (i = 0; i < IPV4_NEXTHOPS; i++) {
    if (nexthops[i].set == 0) {
      index = i;
      nexthops[i].set = 1;
      nexthops[i].ip = *(uint32_t *)(dest);
      nexthops[i].scope = scope;
      nexthops[i].ifindex = ifindex;
      break;
    }
  }
  if (i == IPV4_NEXTHOPS) {
    /* TODO: */
    lagopus_msg_warning("[route(DIR-24-8)] table is full.");
    /* delete route entry from routing table on kernel. */
  }

  ret = rte_lpm_add(ipv4_lookup_struct[sockid],
                    htonl(*((uint32_t *)dest)), prefixlen, index);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "Unable to add entry to the "
             "LPM table on socket %d. ret = %d\n", sockid, ret);
  }
  return LAGOPUS_RESULT_OK;
}

int
route_ipv4_delete(struct in_addr *dest, int prefixlen,
                      struct in_addr *gate, int ifindex)   {
  int ret;
  unsigned int sockid = get_socket_id();
  uint8_t nexthop_index;
  ret = rte_lpm_lookup(ipv4_lookup_struct[sockid],
                       htonl(*((uint32_t *)ip_dst)), &nexthop_index);

  ret = rte_lpm_delete(ipv4_lookup_struct[sockid],
                       htonl(*((uint32_t *)dest)), prefixlen);
  if (ret != 0) {
    /* delete failed. */
    lagopus_msg_warning("[route(DIR-24-8)]route entry delete failed from lpm.");
  }

  if (ret == 0 && nexthop_index >= 0) {
    nexthops[nexthop_index].set     = 0;
    nexthops[nexthop_index].ip      = 0;
    nexthops[nexthop_index].scope   = 0;
    nexthops[nexthop_index].ifindex = 0;
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
route_ipv4_nexthop_get(const struct in_addr *ip_dst,
                       int prefixlen,
                       struct in_addr *nexthop,
                       uint8_t *scope) {
  unsigned int sockid = get_socket_id();
  uint8_t nexthop_index;
  char *ifname[IFNAMSIZ] = {0};
  int ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = rte_lpm_lookup(ipv4_lookup_struct[sockid],
                       htonl(*((uint32_t *)ip_dst)), &nexthop_index);
  if (ret == 0 && nexthops[nexthop_index].set == 1) {
    *scope = nexthops[nexthop_index].scope;
    *((uint32_t *)nexthop) = nexthops[nexthop_index].ip;
  }

  return LAGOPUS_RESULT_OK;
}

#endif
#endif
