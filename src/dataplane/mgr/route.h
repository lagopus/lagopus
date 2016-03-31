/* %COPYRIGHT% */

/**
 *      @file   route.h
 *      @brief  Routing table.
 */

#ifndef SRC_DATAPLANE_MGR_ROUTE_H_
#define SRC_DATAPLANE_MGR_ROUTE_H_

#include <net/if.h>

void route_init(void);
void route_fini(void);

/* ROUTE APIs. */
int
route_ipv4_add(struct in_addr *dest, int prefixlen, struct in_addr *gate,
               int ifindex, uint8_t scope);

int
route_ipv4_delete(struct in_addr *dest, int prefixlen, struct in_addr *gate,
                  int ifindex);

void
route_ipv6_add(struct in6_addr *dest, int prefixlen, struct in6_addr *gate,
               int ifindex);

void
route_ipv6_delete(struct in6_addr *dest, int prefixlen,
                  struct in6_addr *gate, int ifindex);

/* for rib */
lagopus_result_t
route_ipv4_nexthop_get(const struct in_addr *ip_dst, int prefixlen,
                       struct in_addr *nexthop, uint8_t *scope);

/* for datastore */
lagopus_result_t
route_rule_get(struct in_addr *dest, struct in_addr *gate,
               int *prefixlen, uint32_t *ifindex, void **item);
#endif /* SRC_DATAPLANE_MGR_ROUTE_H_ */
