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
#include "ip_addr.h"

#undef ADDR_DEBUG
#ifdef ADDR_DEBUG
#define PRINTF(...)   printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static lagopus_hashmap_t ifname_hashmap;
lagopus_rwlock_t ifname_lock;
int cstate;

struct ifname_entry {
  char ifname[IFNAMSIZ];
};

#define IPV4_BITLEN   32
#define IPV4_SIZE      4
#define IPV6_BITLEN  128
#define IPV6_SIZE     16

static lagopus_result_t
ifname_entry_free(struct ifname_entry *entry) {
  if (entry == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  free(entry);
  return LAGOPUS_RESULT_OK;
}

void
ip_addr_init(void) {
  lagopus_rwlock_create(&ifname_lock);
  lagopus_hashmap_create(&ifname_hashmap,
                         LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                         ifname_entry_free);
}

void
ip_addr_fini(void) {
  if (&ifname_hashmap != NULL) {
    lagopus_hashmap_destroy(&ifname_hashmap, true);
  }
  lagopus_rwlock_destroy(&ifname_lock);
}

static void
ip_addr_ipv4_log(const char *type_str, int ifindex, struct in_addr *addr,
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
ip_addr_ipv4_get(int ifindex, char *ifname) {
  struct ifname_entry *entry;
  lagopus_result_t rv;

  lagopus_rwlock_reader_enter_critical(&ifname_lock, &cstate);
  rv = lagopus_hashmap_find_no_lock(&ifname_hashmap,
                       (void *)ifindex, (void **)&entry);
  if (entry != NULL || rv == LAGOPUS_RESULT_OK) {
    strcpy(ifname, entry->ifname);
  }
  (void)lagopus_rwlock_leave_critical(&ifname_lock, cstate);
}

void
ip_addr_ipv4_add(int ifindex, struct in_addr *addr, int prefixlen,
                  struct in_addr *broad, char *label) {
  struct ifname_entry *entry;
  struct ifname_entry *dentry;

  ip_addr_ipv4_log("add", ifindex, addr, prefixlen, broad, label);

  entry = calloc(1, sizeof(struct ifname_entry));
  if (entry == NULL) {
    lagopus_msg_warning("no memory.\n");
    return;
  }
  memset(entry, 0, sizeof(struct ifname_entry));
  memcpy(entry->ifname, label, strlen(label));
  dentry = entry;
  lagopus_hashmap_add(&ifname_hashmap, (void *)ifindex, (void **)&dentry, true);
}

void
ip_addr_ipv4_delete(int ifindex, struct in_addr *addr, int prefixlen,
                     struct in_addr *broad, char *label) {
  lagopus_result_t rv;
  struct ifname_entry *entry;

  ip_addr_ipv4_log("del", ifindex, addr, prefixlen, broad, label);

  lagopus_rwlock_reader_enter_critical(&ifname_lock, &cstate);
  rv = lagopus_hashmap_find_no_lock(&ifname_hashmap,
                            (void *)ifindex, (void **)&entry);
  if (entry != NULL || rv == LAGOPUS_RESULT_OK) {
    lagopus_hashmap_delete_no_lock(&ifname_hashmap,
                           (void *)ifindex, (void **)&entry, true);
  }
  (void)lagopus_rwlock_leave_critical(&ifname_lock, cstate);
}

static void
ip_addr_ipv6_log(const char *type_str, int ifindex, struct in6_addr *addr,
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
ip_addr_ipv6_add(int ifindex, struct in6_addr *addr, int prefixlen,
                  struct in6_addr *broad, char *label) {
  ip_addr_ipv6_log("add", ifindex, addr, prefixlen, broad, label);
}

void
ip_addr_ipv6_delete(int ifindex, struct in6_addr *addr, int prefixlen,
                     struct in6_addr *broad, char *label) {
  ip_addr_ipv6_log("del", ifindex, addr, prefixlen, broad, label);
}

