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
#include <linux/rtnetlink.h>

#include "lagopus/dp_apis.h"
#include "lagopus/port.h"
#include "lagopus/bridge.h"
#include "lagopus/rib.h"

#include "pktbuf.h"
#include "packet.h"

#include "rib_notifier.h"

#if defined HYBRID && defined PIPELINER
#include "pipeline.h"
#endif /* HYBRID && PIPELINER */

#undef RIB_DEBUG
#ifdef RIB_DEBUG
#define PRINTF(...)   printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define NR_MAX_ENTRIES 1024  /**< max number that can be registered
                                  in the bbq. */

/* fib entry */
struct fib_entry {
  uint8_t src_mac[UPDATER_ETH_LEN];
  uint8_t dst_mac[UPDATER_ETH_LEN];
  uint32_t output_port;
};

/*** static functions ***/
/**
 * Free fib entry for fib.
 * @param[in] fibentry fib entry.
 */
static void
fib_entry_free(struct fib_entry *entry) {
  free(entry);
}

/**
 * Free fib entry for bbq.
 * @param[in] data fib entry.
 */
static void
free_bbq_entry(void **data) {
  if (likely(data != NULL && *data != NULL)) {
    free(*data);
  }
}

/**
 * Get arp info from arp table.
 * by interface.
 */
static lagopus_result_t
rib_arp_get(struct rib *rib, struct in_addr *addr,
            uint8_t *mac, int *ifindex) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  uint32_t read_table = __sync_add_and_fetch(&rib->read_table, 0);
  rv = arp_get(&rib->ribs[read_table].arp_table, addr, mac, ifindex);

  return rv;
}

/**
 * Get nexthop info from route table.
 */
static lagopus_result_t
rib_route_nexthop_get(struct rib *rib, const struct in_addr *ip_dst,
                      struct in_addr *nexthop, uint8_t *scope, uint8_t *mac) {
  int prefixlen = 32;
  uint32_t read_table = __sync_add_and_fetch(&rib->read_table, 0);
  return route_entry_get(&rib->ribs[read_table].route_table,
                         ip_dst, prefixlen, nexthop, scope, mac);
}

/* for debug */
static const char *
convert_action(uint8_t action) {
  if (action == NOTIFICATION_ACTION_TYPE_ADD) return "ADD";
  else if (action == NOTIFICATION_ACTION_TYPE_DEL) return "DEL";
  else if (action == NOTIFICATION_ACTION_TYPE_MOD) return "MOD";
  else return "";
}
static const char *
convert_type(uint8_t type) {
  if (type == NOTIFICATION_TYPE_IFADDR) return "IFADDR";
  else if (type == NOTIFICATION_TYPE_ARP) return "ARP";
  else if (type == NOTIFICATION_TYPE_ROUTE) return "ROUTE";
  else return "";
}

/**
 * Update tables, call by rib_update().
 */
static lagopus_result_t
update_tables(struct rib *rib, uint32_t read_table) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  size_t get_num;
  bool is_empty = true;
  unsigned int bbq_size = 0;
  struct notification_entry **ep = NULL;
  int i;

  /* check if the queue is empty. */
  rv = lagopus_bbq_is_empty(&rib->notification_queue, &is_empty);
  if (rv != LAGOPUS_RESULT_OK) {
    lagopus_msg_error("bbq_is_empty failed[%d].\n", (int)rv);
    return rv;
  }

  /*
   * clear writing table and
   * copy entries from reading table to writing table.
   */
  /* arp table */
  rv = arp_entries_all_copy(&rib->ribs[read_table].arp_table,
                            &rib->ribs[read_table^1].arp_table);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }
  /* route table */
  rv = route_entries_all_copy(&rib->ribs[read_table].route_table,
                              &rib->ribs[read_table^1].route_table);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }

  /* get entries from bbq(notification_queue). */
  get_num = 0;
  if (!is_empty) {
    bbq_size = lagopus_bbq_size(&rib->notification_queue);
    ep = calloc(1, sizeof(struct notification_entry *) * bbq_size);
    lagopus_bbq_get_n(&rib->notification_queue, ep, bbq_size, 0,
                      struct notification_entry *, 0, &get_num);
  }

  /* write entries to write table. */
  for (i = 0; i < get_num; i++) {
    uint8_t type = ep[i]->type;
    uint8_t action = ep[i]->action;
    if (type == NOTIFICATION_TYPE_IFADDR) {
      struct notification_ifaddr_entry *ifaddr = &(ep[i]->ifaddr);
      /*
       * modified interface information,
       * so update interface mac address in route entries.
       */
      if (action == NOTIFICATION_ACTION_TYPE_ADD) {
        route_entry_modify(&rib->ribs[read_table^1].route_table,
                           ifaddr->ifindex, ifaddr->mac);
      }
      /* does not do anything when the non-NOTIFICATION_ACTION_TYPE_ADD. */
    } else if (type == NOTIFICATION_TYPE_ARP) {
      struct notification_arp_entry *arp = &(ep[i]->arp);
      /* update arp information. */
      if (action == NOTIFICATION_ACTION_TYPE_ADD) {
        arp_entry_update(&rib->ribs[read_table^1].arp_table,
                         arp->ifindex, &arp->ip, arp->mac);
      } else if (action == NOTIFICATION_ACTION_TYPE_DEL) {
        arp_entry_delete(&rib->ribs[read_table^1].arp_table,
                         arp->ifindex, &arp->ip, arp->mac);
      }
    } else if (type == NOTIFICATION_TYPE_ROUTE) {
      struct notification_route_entry *route = &(ep[i]->route);
      /* update route information. */
      if (action == NOTIFICATION_ACTION_TYPE_ADD) {
        route_entry_update(&rib->ribs[read_table^1].route_table,
                           &route->dest, route->prefixlen, &route->gate,
                           route->ifindex, route->scope, route->mac);
      } else if (action == NOTIFICATION_ACTION_TYPE_DEL) {
        route_entry_delete(&rib->ribs[read_table^1].route_table,
                           &route->dest, route->prefixlen, &route->gate,
                           route->ifindex);
      }
    }
    free(ep[i]);
  }

  /* free temporary data. */
  if (ep) {
    free(ep);
  }

  return rv;
}

/**
 * Get local data.
 * @param[in] rib RIB object.
 */
static struct local_data *
get_fib(struct rib *rib) {
  uint32_t wid = 0;

#if defined PIPELINER
  wid = pipeline_worker_id;
#elif defined HAVE_DPDK
  wid = dpdk_get_worker_id();
  if (wid == UINT32_MAX) {
    wid = 0;
  }
#endif /* HAVE_DPDK */

  return &rib->fib[wid];
}

/**
 * Reset referred flag, check and return result if switched.
 * @param[in] rib RIB.
 * @param[in] fib Local data for each worker.
 */
static bool
check_referred(struct rib *rib, struct fib *fib) {
  uint32_t referred = __sync_val_compare_and_swap(&fib->referred_table,
                                                  rib->read_table^1,
                                                  rib->read_table);
  if (referred == fib->referred_table) {
    return false;
  } else {
    return true;
  }
}

/**
 * Find entry from fib.
 */
static lagopus_result_t
find_fib_entry(struct fib *fib, struct in_addr dst_ip,
               struct fib_entry **entry) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  struct fib_entry *fib_entry;
  rv = lagopus_hashmap_find_no_lock(&fib->localcache,
                                    (void *)(dst_ip.s_addr),
                                    (void **)&fib_entry);
  if (entry != NULL && rv == LAGOPUS_RESULT_OK) {
    *entry = fib_entry;
  }

  return rv;
}

/**
 * Add entry to fib.
 */
static lagopus_result_t
add_fib_entry(struct fib *fib, struct in_addr dst_ip,
              uint8_t *src_mac, uint8_t *dst_mac, uint32_t port ) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  struct fib_entry *entry, *dentry, *find_entry;

  rv = lagopus_hashmap_find_no_lock(&fib->localcache,
                                    (void *)(dst_ip.s_addr),
                                    (void **)&find_entry);
  if (rv == LAGOPUS_RESULT_NOT_FOUND) {
    /* new entry */
    entry = calloc(1, sizeof(struct fib_entry));
    if (entry == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }

    entry->output_port = port;
    memcpy(entry->src_mac, src_mac, UPDATER_ETH_LEN);
    memcpy(entry->dst_mac, dst_mac, UPDATER_ETH_LEN);

    dentry = entry;
    rv = lagopus_hashmap_add_no_lock(&fib->localcache,
                                     (void *)dst_ip.s_addr,
                                     (void **)&dentry, true);
  } else if (find_entry != NULL && rv == LAGOPUS_RESULT_OK) {
    /* update entry */
    find_entry->output_port = port;
    memcpy(find_entry->src_mac, src_mac, UPDATER_ETH_LEN);
    memcpy(find_entry->dst_mac, dst_mac, UPDATER_ETH_LEN);
  } else {
    lagopus_msg_error("lagopus hashmap find failed\n");
  }

  return rv;
}

/**
 * Rewrite packet header.
 */
static lagopus_result_t
rewrite_pkt_header(struct lagopus_packet *pkt,
                   uint8_t *src, uint8_t *dst) {
  lagopus_result_t rv;

  /* rewrite ether header. (pkt, src hw addr, dst hw addr) */
  rv = lagopus_rewrite_pkt_header(pkt, src, dst);
  if (rv == LAGOPUS_RESULT_STOP) {
    lagopus_msg_warning("ttl stop\n");
    lagopus_packet_free(pkt);
  } else if (rv != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("failed rewrite ether header.");
    lagopus_packet_free(pkt);
  }

  return rv;
}

/* for debug */
static void
print_l3_result(const char *str, uint8_t *src, uint8_t *dst, uint32_t port) {
  printf("%s ", str);
  printf("src[%02x:%02x:%02x:%02x:%02x:%02x] ",
         src[0], src[1], src[2], src[3], src[4], src[5]);
  printf("dst[%02x:%02x:%02x:%02x:%02x:%02x] ",
         dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]);
  printf("output[%"PRIu32"]\n", port);
}

/*** public functions ***/
/**
 * Initialize rib.
 */
lagopus_result_t
rib_init(struct rib *rib) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  int i, j;

  /* initialize fibs */
  for (i = 0; i < UPDATER_LOCALDATA_MAX_NUM; i++) {
    struct fib *fib = &rib->fib[i];
    /* initialize localcache */
    lagopus_hashmap_create(&fib->localcache,
                           LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                           fib_entry_free);

    __sync_lock_test_and_set(&fib->referring, 0);
    __sync_lock_release(&fib->referring);
  }

  /* initialize notification queue. */
  rv = lagopus_bbq_create(&rib->notification_queue,
                          struct notification_entry *,
                          NR_MAX_ENTRIES, free_bbq_entry);
  if (rv != LAGOPUS_RESULT_OK) {
    lagopus_perror(rv);
    return rv;
  }

  /* initialize ribs. */
  for (i = 0; i < 2; i++) {
    /* arp table. */
    arp_init(&rib->ribs[i].arp_table);

    /* routing table. */
    route_init(&rib->ribs[i].route_table);
  }

  return rv;
}

/**
 * Finalize rib.
 */
void
rib_fini(struct rib *rib) {
  int i;

  /* destroy bbq. */
  lagopus_bbq_shutdown(&rib->notification_queue, true);
  lagopus_bbq_destroy(&rib->notification_queue, true);

  for (i = 0; i < 2; i++) {
    /* finalize arp table. */
    arp_fini(&rib->ribs[i].arp_table);

    /* finalize routing table. */
    route_fini(&rib->ribs[i].route_table);
  }

  for (i = 0; i < UPDATER_LOCALDATA_MAX_NUM; i++) {
    struct fib *fib = &rib->fib[i];
    /* destroy local cache. */
    lagopus_hashmap_destroy(&fib->localcache, true);
  }
}

/**
 * Create entry for notification.
 */
struct notification_entry *
rib_create_notification_entry(uint8_t type, uint8_t action) {
  struct notification_entry *entry = NULL;

  entry = calloc(1, sizeof(struct notification_entry));
  if (entry != NULL) {
    entry->type = type;
    entry->action = action;
  }

  return entry;
}

/**
 * Add entry to notification queue from rib_notifier.
 */
lagopus_result_t
rib_add_notification_entry(struct rib *rib,
                           struct notification_entry *entry) {
  lagopus_result_t rv;

  if (rib == NULL || entry == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  rv = lagopus_bbq_put(&rib->notification_queue, &entry,
                  struct notification_entry *, 0);

  return rv;
}

/**
 * Update RIB by timer('updater').
 * Entry data are written to the rib(writable) by only 'updater'.
 * They are merged rib(read only) and bbq.
 */
lagopus_result_t
rib_update(struct rib *rib) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  int cnt;
  uint32_t read_table;

  /* get read table index. */
  read_table = __sync_add_and_fetch(&rib->read_table, 0);

  /*
   * check referred.
   * see mactable_update() in mactable.c for more information.
   */
  for (cnt = 0; cnt < UPDATER_LOCALDATA_MAX_NUM; cnt++) {
    uint32_t referred =
      __sync_add_and_fetch(&rib->fib[cnt].referred_table, 0);
    if (referred != read_table) {
      uint16_t referring =
        __sync_add_and_fetch(&rib->fib[cnt].referring, 0);
      if (referring == 1) {
        return LAGOPUS_RESULT_OK;
      }
    }
  }

  /* update route table. */
  rv = update_tables(rib, read_table);

  /* switch table (write rib <-> read rib). */
  __sync_val_compare_and_swap(&rib->read_table, read_table, read_table^1);

  return rv;
}

/**
 * for datastore api.
 */
lagopus_result_t
rib_route_rule_get(const char *name,
                   struct in_addr *dest, struct in_addr *gate,
                   int *prefixlen, uint32_t *ifindex, void **item) {
  struct bridge *bridge = dp_bridge_lookup(name);
  struct rib *rib = &bridge->rib;
  uint8_t scope;
  uint32_t read_table = __sync_add_and_fetch(&rib->read_table, 0);
  return route_rule_get(&rib->ribs[read_table].route_table,
                        dest, gate, prefixlen, ifindex, &scope, item);
}


/**
 * L3 routing.
 * Check arp table / routing table , rewrite header and lookup output port.
 * For IPv4 packet only.
 */
void
rib_lookup(struct lagopus_packet *pkt) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  int ifindex;
  uint8_t dst_mac[UPDATER_ETH_LEN];
  uint8_t src_mac[UPDATER_ETH_LEN];
  uint8_t scope = 0;
  struct in_addr nexthop, dst_addr;
  struct rib *rib = &(pkt->bridge->rib);
  struct fib *fib;
  struct fib_entry *entry;
  bool switched;

  /* get dst ip address from input packet. */
  lagopus_get_ip(pkt, &dst_addr, AF_INET);

  /* get fib object. */
  fib = get_fib(rib);

  /*
   * "fib->referring" is a flag that indicates that it's in operation.
   * Before performing the operation on rib, it must be turned on(1).
   * Then, after the operation ends, it must be turned off(0).
   * Only worker's operation(this rib_lookup function) to change this flag.
   */
  __sync_add_and_fetch(&fib->referring, 1);

  /* check reference index. */
  switched = check_referred(rib, fib);
  if (switched) {
    /* clear local cache. */
    lagopus_hashmap_clear(&fib->localcache, true);
    rv = LAGOPUS_RESULT_NOT_FOUND;
  } else {
    /* check local cache. */
    rv = find_fib_entry(fib, dst_addr, &entry);
  }

  if (rv == LAGOPUS_RESULT_OK) {
    /*
     * using the fib information,
     * rewrite the pakcet header and set output port.
     */
    rewrite_pkt_header(pkt, entry->src_mac, entry->dst_mac);
    pkt->output_port = entry->output_port;
  } else if (rv == LAGOPUS_RESULT_NOT_FOUND) {
    /* get nexthop info from routing table(lpm). */
    rv = rib_route_nexthop_get(rib, &dst_addr, &nexthop, &scope, src_mac);
    if (rv != LAGOPUS_RESULT_OK) {
      lagopus_msg_info("routing entry is not found.\n");
      lagopus_packet_free(pkt);
      goto out;
    }

    /*
     * if scope is the path of the link,
     * to use the dst_addr as nexthop address.
     */
    nexthop = (scope == RT_SCOPE_LINK) ? dst_addr : nexthop;

    /* get dst mac address and output port from arp table(hash table). */
    rib_arp_get(rib, &nexthop, dst_mac, &ifindex);
    if (ifindex == -1) {
      /* it is no entry on the arp table, send packet to tap(kernel). */
      lagopus_msg_info("no entry in arp table. sent to kernel.\n");
      pkt->send_kernel = true;
      rv = LAGOPUS_RESULT_OK;
      goto out;
    }

    /* rewrite packet header. */
    rv = rewrite_pkt_header(pkt, src_mac, dst_mac);

    /* lookup output port. */
    mactable_port_lookup(pkt);

    /* learning fib. */
    add_fib_entry(fib, dst_addr,
                  src_mac, dst_mac, pkt->output_port);
  } else {
    lagopus_msg_warning("hashmap error.\n");
  }

out:
  /* decrement referring flag. */
  __sync_sub_and_fetch(&fib->referring, 1);
  return rv;
}


