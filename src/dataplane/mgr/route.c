/* %COPYRIGHT% */

/**
 *      @file   rib.c
 *      @brief  Routing Information Base.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "lagopus/dp_apis.h"
#include <net/ethernet.h>
#include "rib.h"
#include "arp.h"
#include "route.h"
#include "ip_addr.h"

#define LPM_PTREE
//#define LPM_DIR_24_8
#ifdef LPM_PTREE
#define _PT_PRIVATE
#include "lagopus/ptree.h"
#include "nexthop.h"
#elif defined LPM_DIR_24_8
#include <rte_config.h>
#include <rte_ip.h>
#include <rte_lpm.h>
#include <rte_eal.h>
#include <rte_lcore.h>
#endif

#undef ROUTE_DEBUG
#ifdef ROUTE_DEBUG
#define PRINTF(...)   printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#ifdef LPM_PTREE
struct route_ipv4_table
{
  pt_tree_t rib;
  struct nexthop_ipv4_table *nexthop_table;
};

static struct route_ipv4_table route_ipv4;

struct route_ipv4 {
  /* Patricia tree node.  */
  pt_node_t route_node;

  /* RIB IP address.  */
  struct in_addr route_addr;

  /* Nexthop pointer.  */
  struct nexthop_ipv4 *nexthop;
};

struct route_ipv6 {
  struct nexthop_ipv6 *nexthop;
};
#elif defined LPM_DIR_24_8
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
#endif /* LPM_XXX */

#ifdef LPM_PTREE
#define IPV4_BITLEN   32
#define IPV4_SIZE      4
#define IPV6_BITLEN  128
#define IPV6_SIZE     16

static struct route_ipv4 *
route_ipv4_alloc(struct in_addr *addr)
{
  struct route_ipv4 *rib;

  rib = calloc(1, sizeof(struct route_ipv4));
  if (rib == NULL) {
    return NULL;
  }
  rib->route_addr = *addr;

  return rib;
}

static void
route_ipv4_free(struct route_ipv4 *rib)
{
  free(rib);
}

static bool
ptree_mask_filter(void *filter_arg, const void *entry, int pt_filter_mask) {
  int *arg = (int *)filter_arg;
  int val = *arg;
  struct route_ipv4 *r = (struct route_ipv4 *)entry;
  pt_node_t *pt = &r->route_node;

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
#endif /* LPM_PTREE */

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
#endif /* LPM_DIR_24_8 */

void
route_init(void) {
#ifdef LPM_PTREE
  ptree_init(&route_ipv4.rib, NULL,
             (void *)(sizeof(struct in_addr) / sizeof(uint32_t)),
             offsetof(struct route_ipv4, route_node),
             offsetof(struct route_ipv4, route_addr));

  route_ipv4.nexthop_table = nexthop_ipv4_table_alloc();
#elif defined LPM_DIR_24_8
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
#endif /* LPM_XXX */
}

#ifdef LPM_PTREE
void
route_fini(void) {
  struct route_ipv4 *rib = NULL;
  while ((rib = ptree_iterate(&route_ipv4.rib, rib, PT_ASCENDING)) != NULL) {
    if (rib) {
      if (rib->nexthop) {
        nexthop_ipv4_release(route_ipv4.nexthop_table, rib->nexthop);
      }
      ptree_remove_node(&route_ipv4.rib, rib);
      route_ipv4_free(rib);
    }
  }
}
#elif defined LPM_DIR_24_8
void
route_fini(void) {
  struct rte_lpm *lpm = ipv4_lookup_struct[get_socket_id()];
  rte_lpm_delete_all(lpm);
  rte_lpm_free(lpm);
}
#endif

static void
route_ipv4_log(const char *type_str, struct in_addr *dest,
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
    PRINTF(" via %s", buf);
  }

  PRINTF(" ifindex %d\n", ifindex);
}

#ifdef LPM_PTREE
static void
route_ipv4_dump(void)
{
  struct route_ipv4 *rib = NULL;

  while ((rib = ptree_iterate(&route_ipv4.rib, rib, PT_ASCENDING)) != NULL) {
    pt_bitlen_t blen;
    int mask = ptree_mask_node_p(&route_ipv4.rib, rib, &blen);

    if (mask) {
      PRINTF("[%s/%d]\n", inet_ntoa(rib->route_addr), blen);
    } else {
      PRINTF("[%s/32]\n", inet_ntoa(rib->route_addr));
    }
  }
}
#elif defined LPM_DIR_24_8
static void
route_ipv4_dump(void)
{
  /* TODO */
}
#endif /* LPM_XXX */

lagopus_result_t
route_rule_get(struct in_addr *dest, struct in_addr *gate,
                   int *prefixlen, uint32_t *ifindex, void **item) {
#ifdef LPM_PTREE
  struct route_ipv4 *rib = (struct route_ipv4 *)*item;
  char string[BUFSIZ];

  if ((rib = ptree_iterate(&route_ipv4.rib, rib, PT_ASCENDING)) != NULL) {
    pt_bitlen_t blen;
    int mask = ptree_mask_node_p(&route_ipv4.rib, rib, &blen);

    *dest = rib->route_addr;
    if (mask) {
      *prefixlen = blen;
    } else {
      *prefixlen = 32;
    }

    if (rib->nexthop) {
      *gate = rib->nexthop->gate;
      *ifindex = rib->nexthop->ifindex;
    }
  }
  *item = rib;
#endif /* LPM_PTREE */

  return LAGOPUS_RESULT_OK;
}

#ifdef LPM_PTREE
int
route_ipv4_add(struct in_addr *dest, int prefixlen, struct in_addr *gate,
                   int ifindex, uint8_t scope) {
  int ret;
  struct route_ipv4 *rib;
  struct nexthop_ipv4 *nexthop;

  route_ipv4_log("add", dest, prefixlen, gate, ifindex);

  rib = route_ipv4_alloc(dest);
  if (! rib) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  if (prefixlen == IPV4_BITLEN) {
    ret = ptree_insert_node(&route_ipv4.rib, rib);
  } else {
    ret = ptree_insert_mask_node(&route_ipv4.rib, rib, prefixlen);
  }

  nexthop = nexthop_ipv4_get(route_ipv4.nexthop_table, gate, ifindex, scope);
  if (nexthop == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }
  rib->nexthop = nexthop;

  route_ipv4_dump();
  return LAGOPUS_RESULT_OK;
}
#elif defined LPM_DIR_24_8
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
#endif /* LPM_XXX */

#ifdef LPM_PTREE
int
route_ipv4_delete(struct in_addr *dest, int prefixlen,
                      struct in_addr *gate, int ifindex)   {
  struct route_ipv4 *rib = NULL;

  route_ipv4_log("del", dest, prefixlen, gate, ifindex);

  if (prefixlen == IPV4_BITLEN) {
    rib = ptree_find_node(&route_ipv4.rib, dest);
  } else {
    rib = ptree_find_filtered_node(&route_ipv4.rib,
                                   dest, ptree_mask_filter, (void *)&prefixlen);
  }

  if (rib) {
    if (rib->nexthop) {
      nexthop_ipv4_release(route_ipv4.nexthop_table, rib->nexthop);
    }
    ptree_remove_node(&route_ipv4.rib, rib);
    route_ipv4_free(rib);
  }

  route_ipv4_dump();

  return LAGOPUS_RESULT_OK;
}
#elif defined LPM_DIR_24_8
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
    lagopus_msg_warning("[route(DIR-24-8)] route entry delete failed from lpm.");
  }

  if (ret == 0 && nexthop_index >= 0) {
    nexthops[nexthop_index].set     = 0;
    nexthops[nexthop_index].ip      = 0;
    nexthops[nexthop_index].scope   = 0;
    nexthops[nexthop_index].ifindex = 0;
  }

  return LAGOPUS_RESULT_OK;
}
#endif /* LPM_XXX */

#ifdef LPM_PTREE
lagopus_result_t
route_ipv4_nexthop_get(const struct in_addr *ip_dst,
                       int prefixlen,
                       struct in_addr *nexthop,
                       uint8_t *scope) {
  struct route_ipv4 *rib = NULL;

  /* get nexthop and scope by dst ip addr. */
  if (prefixlen == IPV4_BITLEN) {
    rib = ptree_find_node(&route_ipv4.rib, ip_dst);
  } else {
    rib = ptree_find_filtered_node(&route_ipv4.rib, ip_dst,
                                   ptree_mask_filter,
                                   (void *)&prefixlen);
  }

  if (rib) {
    if (rib->nexthop) {
      *scope = rib->nexthop->scope;
      memcpy(nexthop, &(rib->nexthop->gate), sizeof(struct in_addr));
    }
  }

  route_ipv4_dump();

  return LAGOPUS_RESULT_OK;
}
#elif defined LPM_DIR_24_8
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
#endif /* LPM_XXX */

static void
route_ipv6_log(const char *type_str, struct in6_addr *dest,
                   int prefixlen, struct in6_addr *gate, int ifindex) {
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

