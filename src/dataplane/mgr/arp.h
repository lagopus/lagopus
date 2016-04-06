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
