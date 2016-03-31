/* %COPYRIGHT% */

/**
 *      @file   arp.h
 *      @brief  ARP table.
 */

#ifndef SRC_DATAPLANE_MGR_ARP_H_
#define SRC_DATAPLANE_MGR_ARP_H_

#include <net/if.h>

/* ARP APIs. */
void
arp_init(void);

void
arp_fini(void);

/* for netlink */
void
arp_add(int ifindex, struct in_addr *dst_addr, char *ll_addr);

void
arp_delete(int ifindex, struct in_addr *dst_addr, char *ll_addr);

/* for rib */
void
arp_get(struct in_addr *addr, char *mac, int *ifindex);

#endif /* SRC_DATAPLANE_MGR_ARP_H_ */
