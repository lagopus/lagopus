/* %COPYRIGHT% */

/**
 *      @file   ip_addr.h
 *      @brief  Interface address.
 */

#ifndef SRC_DATAPLANE_MGR_ADDR_H_
#define SRC_DATAPLANE_MGR_ADDR_H_

#include <net/if.h>

/* ADDR APIs. */
void ip_addr_init(void);
void ip_addr_fini(void);

void
ip_addr_ipv4_add(int ifindex, struct in_addr *addr, int prefixlen,
              struct in_addr *broad, char *label);

void
ip_addr_ipv4_delete(int ifindex, struct in_addr *addr, int prefixlen,
                 struct in_addr *broad, char *label);

void
ip_addr_ipv6_add(int ifindex, struct in6_addr *addr, int prefixlen,
              struct in6_addr *broad, char *label);

void
ip_addr_ipv6_delete(int ifindex, struct in6_addr *addr, int prefixlen,
                 struct in6_addr *broad, char *label);

/* for rib */
void
ip_addr_ipv4_get(int ifindex, char *ifname);
#endif /* SRC_DATAPLANE_MGR_ADDR_H_ */
