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
 *      @file   rib-notifier.c
 *      @brief  Notify nelink data to rib.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include "lagopus/dp_apis.h"
#include "lagopus/rib.h"
#include "lagopus/updater.h"
#include "rib_notifier.h"

#undef RIB_DEBUG
#ifdef RIB_DEBUG
#define PRINTF(...)   printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/* management the interface info */
static lagopus_hashmap_t ifinfo_hashmap;
lagopus_rwlock_t ifinfo_lock;
int cstate;

/**
 * Structure of the interface infromation.
 */
struct ifinfo_entry {
  int ifindex;                  /* interface index of tap interface. */
  char ifname[IFNAMSIZ];        /* interface name of the interface. */
  uint8_t hwaddr[UPDATER_ETH_LEN];  /* mac address of the interface. */
  struct rib *rib;              /* pointer to rib object in the bridge. */
};

/*** static apis ***/
/**
 * Free entry of hashmap.
 * @param[in] entry Interface information.
 */
static lagopus_result_t
ifinfo_entry_free(struct ifinfo_entry *entry) {
  if (entry == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  free(entry);
  return LAGOPUS_RESULT_OK;
}

/**
 * Get reference of the rib.
 * @param[in] ifindex Interface index.
 * @param[in] rib RIB structure.
 */
static lagopus_result_t
ifinfo_rib_get(int ifindex, struct rib **rib) {
  struct ifinfo_entry *entry;
  lagopus_result_t rv;

  rv = lagopus_hashmap_find(&ifinfo_hashmap, (void *)ifindex, (void **)&entry);
  if (rv == LAGOPUS_RESULT_OK) {
    *rib = entry->rib;
  }

  return rv;
}

/**
 * Output log for ipv4 addr information that notified from netlink.
 * @param[in] type_str String of the message type.
 * @param[in] ifindex Interface index.
 * @param[in] addr IP address.
 * @param[in] prefixlen Prefix length.
 * @param[in] broad Broadcast address.
 * @param[in] lable Interface name.
 */
static void
addr_ipv4_log(const char *type_str, int ifindex, struct in_addr *addr,
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

/**
 * Output log for ipv6 addr information that notified from netlink.
 * @param[in] type_str String of the message type.
 * @param[in] ifindex Interface index.
 * @param[in] addr IP address.
 * @param[in] prefixlen Prefix length.
 * @param[in] broad Broadcast address.
 * @param[in] lable Interface name.
 */
static void
addr_ipv6_log(const char *type_str, int ifindex, struct in6_addr *addr,
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

/*** public apis ***/
/**
 * Initialize.
 * Create table for interface information.
 */
lagopus_result_t
rib_notifier_init() {
  lagopus_result_t rv;
  lagopus_rwlock_create(&ifinfo_lock);
  rv = lagopus_hashmap_create(&ifinfo_hashmap,
                              LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                              ifinfo_entry_free);
  return rv;
}

/**
 * Finalize.
 */
void
rib_notifier_fini() {
  if (&ifinfo_hashmap != NULL) {
    lagopus_hashmap_destroy(&ifinfo_hashmap, true);
  }
  lagopus_rwlock_destroy(&ifinfo_lock);
}

/**
 * Add ipv4 addr information notified from netlink.
 */
void
rib_notifier_ipv4_addr_add(int ifindex, struct in_addr *addr, int prefixlen,
                           struct in_addr *broad, char *label) {
  lagopus_result_t rv;
  struct ifinfo_entry *entry;
  struct ifinfo_entry *dentry;
  uint8_t hwaddr[UPDATER_ETH_LEN];
  struct bridge *bridge;
  struct notification_entry *nentry = NULL;

  addr_ipv4_log("add", ifindex, addr, prefixlen, broad, label);

  /* new ifinfo entry to registered to ifinfo_hashmap. */
  entry = calloc(1, sizeof(struct ifinfo_entry));
  if (entry == NULL) {
    lagopus_msg_warning("no memory.\n");
    return;
  }
  memset(entry, 0, sizeof(struct ifinfo_entry));
  memcpy(entry->ifname, label, strlen(label));

  if (dp_tapio_interface_info_get(label, hwaddr, &bridge)
      != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("get interface info failed.\n");
    ifinfo_entry_free(entry);
    return;
  }
  memcpy(entry->hwaddr, hwaddr, UPDATER_ETH_LEN);
  entry->rib = &(bridge->rib);
  dentry = entry;
  lagopus_hashmap_add(&ifinfo_hashmap, (void *)ifindex, (void **)&dentry, true);

  /* create and set notification entry. */
  nentry = rib_create_notification_entry(NOTIFICATION_TYPE_IFADDR,
                                         NOTIFICATION_ACTION_TYPE_ADD);
  if (nentry) {
    /* add notification entry to queue. */
    nentry->ifaddr.ifindex = ifindex;
    rv = rib_add_notification_entry(&bridge->rib, nentry);
  } else {
    lagopus_msg_warning("create notification entry failed\n");
    ifinfo_entry_free(entry);
  }

  return;
}

/**
 * Delete ipv4 addr information notifeid from netlink.
 */
void
rib_notifier_ipv4_addr_delete(int ifindex, struct in_addr *addr, int prefixlen,
                              struct in_addr *broad, char *label) {
  lagopus_result_t rv;
  struct ifinfo_entry *entry;

  addr_ipv4_log("del", ifindex, addr, prefixlen, broad, label);

  lagopus_rwlock_reader_enter_critical(&ifinfo_lock, &cstate);
  rv = lagopus_hashmap_find_no_lock(&ifinfo_hashmap,
                            (void *)ifindex, (void **)&entry);
  if (entry != NULL || rv == LAGOPUS_RESULT_OK) {
    lagopus_hashmap_delete_no_lock(&ifinfo_hashmap,
                           (void *)ifindex, (void **)&entry, true);
  }
  (void)lagopus_rwlock_leave_critical(&ifinfo_lock, cstate);
}

/**
 * Get interface info.
 * Called by rib-updater.
 */
lagopus_result_t
rib_notifier_ipv4_addr_get(int ifindex, uint8_t *hwaddr) {
  struct ifinfo_entry *entry;
  lagopus_result_t rv;
  lagopus_rwlock_reader_enter_critical(&ifinfo_lock, &cstate);
  rv = lagopus_hashmap_find_no_lock(&ifinfo_hashmap,
                                    (void *)ifindex, (void **)&entry);
  if (entry != NULL || rv == LAGOPUS_RESULT_OK) {
    memcpy(hwaddr, entry->hwaddr, UPDATER_ETH_LEN);
  }
  (void)lagopus_rwlock_leave_critical(&ifinfo_lock, cstate);

  return rv;
}

/**
 * Add ipv6 addr info (not supported).
 */
void
rib_notifier_ipv6_addr_add(int ifindex, struct in6_addr *addr, int prefixlen,
                           struct in6_addr *broad, char *label) {
  addr_ipv6_log("add", ifindex, addr, prefixlen, broad, label);
}

/**
 * Delete ipv6 addr info (not supported).
 */
void
rib_notifier_ipv6_addr_delete(int ifindex, struct in6_addr *addr, int prefixlen,
                              struct in6_addr *broad, char *label) {
  addr_ipv6_log("del", ifindex, addr, prefixlen, broad, label);
}


/**
 * Add arp information notified from netlink.
 */
void
rib_notifier_arp_add(int ifindex, struct in_addr *dst_addr, char *ll_addr) {
  struct rib *rib;
  lagopus_result_t rv;
  struct notification_entry *entry = NULL;

  rv = ifinfo_rib_get(ifindex, &rib);
  if (rv == LAGOPUS_RESULT_OK && rib != NULL) {
    /* create and set notification entry. */
    entry = rib_create_notification_entry(NOTIFICATION_TYPE_ARP,
                                          NOTIFICATION_ACTION_TYPE_ADD);
    if (entry) {
      /* add notification entry to queue. */
      entry->arp.ifindex = ifindex;
      entry->arp.ip = *dst_addr;
      memcpy(entry->arp.mac, ll_addr, UPDATER_ETH_LEN);
      rv = rib_add_notification_entry(rib, entry);
    } else {
      lagopus_msg_warning("create notification entry failed\n");
    }
  }

  return;
}

/**
 * Delete arp information notified from netlink.
 */
void
rib_notifier_arp_delete(int ifindex, struct in_addr *dst_addr, char *ll_addr) {
  struct rib *rib;
  lagopus_result_t rv;
  struct notification_entry *entry = NULL;

  rv = ifinfo_rib_get(ifindex, &rib);
  if (rv == LAGOPUS_RESULT_OK && rib != NULL) {
    entry = rib_create_notification_entry(NOTIFICATION_TYPE_ARP,
                                          NOTIFICATION_ACTION_TYPE_DEL);
    if (entry) {
      /* add notification entry to queue. */
      entry->arp.ifindex = ifindex;
      entry->arp.ip = *dst_addr;
      memcpy(entry->arp.mac, ll_addr, UPDATER_ETH_LEN);
      rv = rib_add_notification_entry(rib, entry);
    } else {
      lagopus_msg_warning("create notification entry failed\n");
    }
  }

  return;
}

/**
 * Add route information notified from netlink.
 */
void
rib_notifier_ipv4_route_add(struct in_addr *dest, int prefixlen,
                            struct in_addr *gate, int ifindex, uint8_t scope) {
  struct rib *rib;
  lagopus_result_t rv;
  struct notification_entry *entry = NULL;
  struct ifinfo_entry *ientry = NULL;

  rv = ifinfo_rib_get(ifindex, &rib);
  if (rv == LAGOPUS_RESULT_OK && rib != NULL) {
    entry = rib_create_notification_entry(NOTIFICATION_TYPE_ROUTE,
                                          NOTIFICATION_ACTION_TYPE_ADD);
    if (entry) {
      /* get mac address of the interface. */
      rv = lagopus_hashmap_find(&ifinfo_hashmap,
                                (void *)ifindex, (void **)&ientry);
      if (ientry == NULL || rv != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("get interface info failed.\n");
        return;
      }
      /* set data to notification entry object. */
      entry->route.ifindex = ifindex;
      entry->route.dest = *dest;
      entry->route.gate = *gate;
      entry->route.scope = scope;
      entry->route.prefixlen = prefixlen;
      memcpy(entry->route.mac, ientry->hwaddr, UPDATER_ETH_LEN);
      /* add notification entry to queue. */
      rv = rib_add_notification_entry(rib, entry);
    } else {
      lagopus_msg_warning("create notification entry failed\n");
    }
  }

  return;
}

/**
 * Delete route information notified from netlink.
 */
void
rib_notifier_ipv4_route_delete(struct in_addr *dest, int prefixlen,
                               struct in_addr *gate, int ifindex)   {
  struct rib *rib;
  lagopus_result_t rv;
  struct notification_entry *entry = NULL;

  rv = ifinfo_rib_get(ifindex, &rib);
  if (rv == LAGOPUS_RESULT_OK && rib != NULL) {
    entry = rib_create_notification_entry(NOTIFICATION_TYPE_ROUTE,
                                          NOTIFICATION_ACTION_TYPE_DEL);
    if (entry) {
      /* add notification entry to queue. */
      entry->route.ifindex = ifindex;
      entry->route.dest = *dest;
      entry->route.gate = *gate;
      entry->route.scope = 0;
      entry->route.prefixlen = prefixlen;
      rv = rib_add_notification_entry(rib, entry);
    } else {
      lagopus_msg_warning("create notification entry failed\n");
    }
  }

  return;
}

/**
 * Add ipv6 route information notified from netlink.
 */
void
rib_notifier_ipv6_route_add(struct in6_addr *dest, int prefixlen,
                            struct in6_addr *gate, int ifindex) {
  route_ipv6_add(dest, prefixlen, gate, ifindex);
}

/**
 * Delete ipv6 route information notified from netlink.
 */
void
rib_notifier_ipv6_route_delete(struct in6_addr *dest, int prefixlen,
                               struct in6_addr *gate, int ifindex) {
  route_ipv6_delete(dest, prefixlen, gate, ifindex);
}

/** interface apis(not supported) **/
void
rib_notifier_interface_update(int ifindex, struct ifparam_t *param) {
  PRINTF("Interface update: ifindex %u ifname %s\n", ifindex, param->name);
}

void
rib_notifier_interface_delete(int ifindex) {
  PRINTF("Interface del: ifindex %u\n", ifindex);
}

/** ndp apis(not supported) **/
static void
rib_notifier_ndp_log(const char *type_str, int ifindex,
                     struct in6_addr *dst_addr, char *ll_addr) {
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
rib_notifier_ndp_add(int ifindex, struct in6_addr *dst_addr, char *ll_addr) {
  rib_notifier_ndp_log("add", ifindex, dst_addr, ll_addr);
}

void
rib_notifier_ndp_delete(int ifindex, struct in6_addr *dst_addr, char *ll_addr) {
  rib_notifier_ndp_log("del", ifindex, dst_addr, ll_addr);
}

