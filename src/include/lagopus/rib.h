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
 *      @file   rib.h
 *      @brief  Routing Information Base.
 */

#ifndef SRC_DATAPLANE_MGR_RIB_H_
#define SRC_DATAPLANE_MGR_RIB_H_

#include <net/if.h>

#include "lagopus/route.h"
#include "lagopus/arp.h"
#include "lagopus/updater.h"

/* for queue entry(netlink notification) */
enum msg_type {
  NOTIFICATION_TYPE_IFADDR = 0,
  NOTIFICATION_TYPE_ARP,
  NOTIFICATION_TYPE_ROUTE
};

enum action_type {
  NOTIFICATION_ACTION_TYPE_ADD = 0,
  NOTIFICATION_ACTION_TYPE_DEL,
  NOTIFICATION_ACTION_TYPE_MOD
};

/* if-addr entry for queue */
struct notification_ifaddr_entry {
  int ifindex;              /* i/f index. */
  char ifname[IFNAMSIZ];    /* i/f name. */
  uint8_t mac[UPDATER_ETH_LEN]; /* i/f mac address. */
} __attribute__ ((aligned(128)));

/* arp entry for queue */
struct notification_arp_entry {
  int ifindex;              /* i/f index. */
  struct in_addr ip;        /* ip address of i/f with ifindex. */
  uint8_t mac[UPDATER_ETH_LEN]; /* mac address for ip. */
} __attribute__ ((aligned(128)));

/* route entry for queue */
struct notification_route_entry {
  struct in_addr dest;      /* Destination address. */
  struct in_addr gate;      /* Nexthop address. */
  int ifindex;              /* Nexthop interface index. */
  uint8_t scope;            /* Scope of interface. */
  uint32_t prefixlen;       /* Prefix length. */
  uint8_t mac[UPDATER_ETH_LEN]; /* mac address for i/f with ifindex. */
} __attribute__ ((aligned(128)));

/* queue entry */
struct notification_entry {
  uint8_t type;
  uint8_t action;
  union {
    struct notification_arp_entry arp;
    struct notification_route_entry route;
    struct notification_ifaddr_entry ifaddr;
  };
};

/**
 * Local data for each worker.
 */
struct fib {
  lagopus_hashmap_t localcache; /**< local cache for each worker. */

  uint32_t referred_table; /**< index of referencing rib. */
  uint16_t referring;      /**< whether it refers to the rib(reading). */
} __attribute__ ((aligned(128)));

/**
 * A combination of the arp table and the route table to manage.
 */
struct rib_tables {
  struct arp_table arp_table;     /**< arp table */
  struct route_table route_table; /**< route table */
};

/**
 * RIB(Relational Information Base)
 */
struct rib {
  lagopus_bbq_t notification_queue;  /**< queue that receives a change
                                          request for various information
                                          by notification from rib_notifier. */

  struct rib_tables ribs[2]; /**< RIBs(writing and reading). */
  uint32_t read_table;       /**< Current read table index. */

  struct fib fib[UPDATER_LOCALDATA_MAX_NUM]; /**< local cache for each workers. */
};

/* apis */
lagopus_result_t
rib_init(struct rib *);

void
rib_fini(struct rib *);

lagopus_result_t
rib_update(struct rib *rib);

/* for rib_notifier */
lagopus_result_t
rib_arp_add(struct rib *rib, int ifindex,
            struct in_addr *dst_addr, char *ll_addr);

lagopus_result_t
rib_arp_delete(struct rib *rib, int ifindex,
               struct in_addr *dst_addr, char *ll_addr);


/* for datastore */
lagopus_result_t
rib_route_rule_get(const char *name,
                   struct in_addr *dest, struct in_addr *gate,
                   int *prefixlen, uint32_t *ifindex, void **item);

/* for l3 routing */
#if 0
lagopus_result_t
rib_nexthop_ipv4_get(struct rib *rib, const struct in_addr *ip_dst,
                     struct in_addr *nexthop, uint8_t *scope, uint8_t *mac);

lagopus_result_t
rib_arp_get(struct rib *rib, struct in_addr *addr, uint8_t *mac, int *ifindex);
#endif
void
rib_ipv4_interface_get(int ifindex, uint8_t *hwaddr);

void
rib_lookup(struct lagopus_packet *pkt);

lagopus_result_t
rib_add_notification_entry(struct rib *rib, struct notification_entry *entry);

struct notification_entry *
rib_create_notification_entry(uint8_t type, uint8_t action);

#endif /* SRC_DATAPLANE_MGR_RIB_H_ */
