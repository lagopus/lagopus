/* %COPYRIGHT% */

/**
 *      @file   rib.h
 *      @brief  Routing Information Base.
 */

#ifndef SRC_DATAPLANE_MGR_RIB_H_
#define SRC_DATAPLANE_MGR_RIB_H_

#include <net/if.h>

/* Interface parameters. */
struct ifparam_t {
#define   IFPARAM_HWADDR   (1 << 0)
#define   IFPARAM_METRIC   (1 << 1)
  uint32_t params;

  /* Interface index. */
  int ifindex;

  /* Interface flags. */
  uint32_t flags;

  /* MTU. */
  int mtu;

  /* Metric. */
  int metric;

  /* Interface name. */
  char name[IFNAMSIZ + 1];

  /* Hardware address (MAC address). */
  uint16_t hw_type;
#define   HW_ADDR_MAX_LEN    20
  uint8_t hw_addr[HW_ADDR_MAX_LEN];
  int hw_addr_len;
};

/* RIB APIs. */
void
rib_interface_update(int ifindex, struct ifparam_t *param);

void
rib_interface_delete(int ifidnex);

void
rib_ipv4_route_add(struct in_addr *dest, int prefixlen, struct in_addr *gate,
                   int ifindex);

void
rib_ipv4_route_delete(struct in_addr *dest, int prefixlen, struct in_addr *gate,
                      int ifindex);

void
rib_ipv6_route_add(struct in6_addr *dest, int prefixlen, struct in6_addr *gate,
                   int ifindex);

void
rib_ipv6_route_delete(struct in6_addr *dest, int prefixlen,
                      struct in6_addr *gate, int ifindex);

void
rib_ipv4_addr_add(int ifindex, struct in_addr *addr, int prefixlen,
                  struct in_addr *broad, char *label);

void
rib_ipv4_addr_delete(int ifindex, struct in_addr *addr, int prefixlen,
                     struct in_addr *broad, char *label);

void
rib_ipv6_addr_add(int ifindex, struct in6_addr *addr, int prefixlen,
                  struct in6_addr *broad, char *label);

void
rib_ipv6_addr_delete(int ifindex, struct in6_addr *addr, int prefixlen,
                     struct in6_addr *broad, char *label);

void
rib_arp_add(int ifindex, struct in_addr *dst_addr, char *ll_addr);

void
rib_arp_delete(int ifindex, struct in_addr *dst_addr, char *ll_addr);

void
rib_ndp_add(int ifindex, struct in6_addr *dst_addr, char *ll_addr);

void
rib_ndp_delete(int ifindex, struct in6_addr *dst_addr, char *ll_addr);

#endif /* SRC_DATAPLANE_MGR_RIB_H_ */
