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
#include "ip_addr.h"
#include "route.h"

#undef RIB_DEBUG
#ifdef RIB_DEBUG
#define PRINTF(...)   printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

void
rib_init(void) {
  ip_addr_init();
  arp_init();
  route_init();
}

void
rib_fini(void) {
  ip_addr_fini();
  arp_fini();
  route_fini();
}

/* interface address */
void
rib_ipv4_addr_get(int ifindex, char *ifname) {
  ip_addr_ipv4_get(ifindex, ifname);
}

void
rib_ipv4_addr_add(int ifindex, struct in_addr *addr, int prefixlen,
                  struct in_addr *broad, char *label) {
  ip_addr_ipv4_add(ifindex, addr, prefixlen, broad, label);
}

void
rib_ipv4_addr_delete(int ifindex, struct in_addr *addr, int prefixlen,
                     struct in_addr *broad, char *label) {
  ip_addr_ipv4_delete(ifindex, addr, prefixlen, broad, label);
}

void
rib_ipv6_addr_add(int ifindex, struct in6_addr *addr, int prefixlen,
                  struct in6_addr *broad, char *label) {
  ip_addr_ipv6_add(ifindex, addr, prefixlen, broad, label);
}

void
rib_ipv6_addr_delete(int ifindex, struct in6_addr *addr, int prefixlen,
                     struct in6_addr *broad, char *label) {
  ip_addr_ipv6_delete(ifindex, addr, prefixlen, broad, label);
}

/* arp */
void
rib_arp_get(struct in_addr *addr, char *mac, int *ifindex) {
  arp_get(addr, mac, ifindex);
}

void
rib_arp_add(int ifindex, struct in_addr *dst_addr, char *ll_addr) {
  arp_add(ifindex, dst_addr, ll_addr);
}

void
rib_arp_delete(int ifindex, struct in_addr *dst_addr, char *ll_addr) {
  arp_delete(ifindex, dst_addr, ll_addr);
}

/* route (lpm) */
lagopus_result_t
rib_route_rule_get(struct in_addr *dest, struct in_addr *gate,
                   int *prefixlen, uint32_t *ifindex, void **item) {
  return route_rule_get(dest, gate, prefixlen, ifindex, item);
}

int
rib_ipv4_route_add(struct in_addr *dest, int prefixlen, struct in_addr *gate,
                   int ifindex, uint8_t scope) {
  return route_ipv4_add(dest, prefixlen, gate, ifindex, scope);
}

int
rib_ipv4_route_delete(struct in_addr *dest, int prefixlen,
                      struct in_addr *gate, int ifindex)   {
  return route_ipv4_delete(dest, prefixlen, gate, ifindex);
}

lagopus_result_t
rib_nexthop_ipv4_get(const struct in_addr *ip_dst,
                     struct in_addr *nexthop,
                     uint8_t *scope) {
  int prefixlen = 32;
  return route_ipv4_nexthop_get(ip_dst, prefixlen, nexthop, scope);
}

void
rib_ipv6_route_add(struct in6_addr *dest, int prefixlen,
                   struct in6_addr *gate, int ifindex) {
  route_ipv6_add(dest, prefixlen, gate, ifindex);
}

void
rib_ipv6_route_delete(struct in6_addr *dest, int prefixlen,
                      struct in6_addr *gate, int ifindex) {
  route_ipv6_delete(dest, prefixlen, gate, ifindex);
}

/* not implemented yet. */
void
rib_interface_update(int ifindex, struct ifparam_t *param) {
  PRINTF("Interface update: ifindex %u ifname %s\n", ifindex, param->name);
}

void
rib_interface_delete(int ifindex) {
  PRINTF("Interface del: ifindex %u\n", ifindex);
}

static void
rib_ndp_log(const char *type_str, int ifindex, struct in6_addr *dst_addr,
            char *ll_addr) {
  if (dst_addr) {
    char buf[BUFSIZ];

    inet_ntop(AF_INET6, dst_addr, buf, BUFSIZ);

    PRINTF("NDP %s: %s ->", type_str, buf);

    if (ll_addr) {
      PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x",
             ll_addr[0] & 0xff, ll_addr[1] & 0xff, ll_addr[2] & 0xff,
             ll_addr[3] & 0xff, ll_addr[4] & 0xff, ll_addr[5] & 0xff);
    }

    PRINTF(" ifindex: %u", ifindex);

    PRINTF("\n");
  }
}

void
rib_ndp_add(int ifindex, struct in6_addr *dst_addr, char *ll_addr) {
  rib_ndp_log("add", ifindex, dst_addr, ll_addr);
}

void
rib_ndp_delete(int ifindex, struct in6_addr *dst_addr, char *ll_addr) {
  rib_ndp_log("del", ifindex, dst_addr, ll_addr);
}
