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
