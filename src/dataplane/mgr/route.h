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
