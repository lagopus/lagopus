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
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "rib.h"

/* #define RIB_DEBUG */
#ifdef RIB_DEBUG
#define PRINTF(...)   printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

void
rib_interface_update(int ifindex, struct ifparam_t *param) {
  PRINTF("Interface update: ifindex %u ifname %s\n", ifindex, param->name);
}

void
rib_interface_delete(int ifindex) {
  PRINTF("Interface del: ifindex %u\n", ifindex);
}

static void
rib_ipv4_addr_log(const char *type_str, int ifindex, struct in_addr *addr,
                  int prefixlen, struct in_addr *broad, char *label) {
  PRINTF("Address %s:", type_str);

  if (addr) {
    char buf[BUFSIZ];

    inet_ntop(AF_INET, addr, buf, BUFSIZ);
    PRINTF(" %s/%d ifindex %u", buf, prefixlen, ifindex);
  }

  if (broad) {
    char buf[BUFSIZ];

    inet_ntop(AF_INET, broad, buf, BUFSIZ);
    PRINTF(" broadcast %s", buf);
  }

  if (label) {
    PRINTF(" label: %s", label);
  }

  PRINTF("\n");
}

void
rib_ipv4_addr_add(int ifindex, struct in_addr *addr, int prefixlen,
                  struct in_addr *broad, char *label) {
  rib_ipv4_addr_log("add", ifindex, addr, prefixlen, broad, label);
}

void
rib_ipv4_addr_delete(int ifindex, struct in_addr *addr, int prefixlen,
                     struct in_addr *broad, char *label) {
  rib_ipv4_addr_log("del", ifindex, addr, prefixlen, broad, label);
}

static void
rib_ipv6_addr_log(const char *type_str, int ifindex, struct in6_addr *addr,
                  int prefixlen, struct in6_addr *broad, char *label) {
  PRINTF("Address %s:", type_str);

  if (addr) {
    char buf[BUFSIZ];

    inet_ntop(AF_INET6, addr, buf, BUFSIZ);
    PRINTF(" %s/%d ifindex %u", buf, prefixlen, ifindex);
  }

  if (broad) {
    char buf[BUFSIZ];

    inet_ntop(AF_INET6, broad, buf, BUFSIZ);
    PRINTF(" broadcast %s", buf);
  }

  if (label) {
    PRINTF(" label: %s", label);
  }

  PRINTF("\n");
}

void
rib_ipv6_addr_add(int ifindex, struct in6_addr *addr, int prefixlen,
                  struct in6_addr *broad, char *label) {
  rib_ipv6_addr_log("add", ifindex, addr, prefixlen, broad, label);
}

void
rib_ipv6_addr_delete(int ifindex, struct in6_addr *addr, int prefixlen,
                     struct in6_addr *broad, char *label) {
  rib_ipv6_addr_log("del", ifindex, addr, prefixlen, broad, label);
}

static void
rib_ipv4_route_log(const char *type_str, struct in_addr *dest,
                   int prefixlen, struct in_addr *gate, int ifindex) {
  PRINTF("Route %s:", type_str);

  if (dest) {
    char buf[BUFSIZ];

    inet_ntop(AF_INET, dest, buf, BUFSIZ);
    PRINTF(" %s/%d", buf, prefixlen);
  }

  PRINTF(" ifindex %d\n", ifindex);
}

void
rib_ipv4_route_add(struct in_addr *dest, int prefixlen, struct in_addr *gate,
                   int ifindex) {
  rib_ipv4_route_log("add", dest, prefixlen, gate, ifindex);
}

void
rib_ipv4_route_delete(struct in_addr *dest, int prefixlen,
                      struct in_addr *gate, int ifindex) {
  rib_ipv4_route_log("del", dest, prefixlen, gate, ifindex);
}

static void
rib_ipv6_route_log(const char *type_str, struct in6_addr *dest,
                   int prefixlen, struct in6_addr *gate, int ifindex) {
  PRINTF("Route %s:", type_str);

  if (dest) {
    char buf[BUFSIZ];

    inet_ntop(AF_INET6, dest, buf, BUFSIZ);
    PRINTF(" %s/%d", buf, prefixlen);
  }

  PRINTF("\n");
}

void
rib_ipv6_route_add(struct in6_addr *dest, int prefixlen,
                   struct in6_addr *gate, int ifindex) {
  rib_ipv6_route_log("add", dest, prefixlen, gate, ifindex);
}

void
rib_ipv6_route_delete(struct in6_addr *dest, int prefixlen,
                      struct in6_addr *gate, int ifindex) {
  rib_ipv6_route_log("del", dest, prefixlen, gate, ifindex);
}

static void
rib_arp_log(const char *type_str, int ifindex, struct in_addr *dst_addr,
            char *ll_addr) {
  if (dst_addr) {
    char buf[BUFSIZ];

    inet_ntop(AF_INET, dst_addr, buf, BUFSIZ);

    PRINTF("ARP %s: %s ->", type_str, buf);

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
rib_arp_add(int ifindex, struct in_addr *dst_addr, char *ll_addr) {
  rib_arp_log("add", ifindex, dst_addr, ll_addr);
}

void
rib_arp_delete(int ifindex, struct in_addr *dst_addr, char *ll_addr) {
  rib_arp_log("del", ifindex, dst_addr, ll_addr);
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
