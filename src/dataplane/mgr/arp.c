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
 *      @file   arp.c
 *      @brief  ARP table.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "lagopus/dp_apis.h"
#include <net/ethernet.h>
#include "lagopus/updater.h"

#undef ARP_DEBUG
#ifdef ARP_DEBUG
#define PRINTF(...)   printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/* arp entry */
struct arp_entry {
  int ifindex;
  struct in_addr ip;
  uint8_t mac_addr[UPDATER_ETH_LEN];
};

struct arp_entry_args {
  unsigned int num;  /**< number of entries. */
  unsigned int no;  /**< current entry's index. */
  struct arp_entry *entries;  /**< arp entries. */
};

/*** static functions ***/
/**
 * Free arp entry in arp table.
 */
static lagopus_result_t
arp_entry_free(struct arp_entry *entry) {
  if (entry == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }
  free(entry);
  return LAGOPUS_RESULT_OK;
}

/**
 * Debug print for arp entry.
 */
static void
arp_log(const char *type_str, int ifindex,
        struct in_addr *dst_addr, char *ll_addr) {
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

/**
 * Copy arp entry when get all entreis from arp table.
 */
static bool
copy_arp_entry(void *key, void *val, lagopus_hashentry_t he, void *arg) {
  bool result = false;
  struct arp_entry_args *aa = (struct arp_entry_args *)arg;
  struct arp_entry *entries = (struct arp_entry *)aa->entries;
  struct arp_entry *entry = (struct arp_entry *)val;
  (void) key;
  (void) he;

  if (val != NULL && aa->no < aa->num) {
    entries[aa->no].ifindex = entry->ifindex;
    entries[aa->no].ip = entry->ip;
    memcpy(entries[aa->no].mac_addr, entry->mac_addr, UPDATER_ETH_LEN);
    aa->no++;
    result = true;
  }

  return result;
}

/*** public functions ***/
/**
 * Initialize arp table.
 */
void
arp_init(struct arp_table *arp_table) {
  lagopus_rwlock_create(&arp_table->lock);
  lagopus_hashmap_create(&arp_table->hashmap,
                         LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                         arp_entry_free);
}

/**
 * Finalize arp table.
 */
void
arp_fini(struct arp_table *arp_table) {
  if (arp_table != NULL) {
    lagopus_hashmap_destroy(&arp_table->hashmap, true);
  }
  lagopus_rwlock_destroy(&arp_table->lock);
}

/**
 * Delete arp entry.
 */
lagopus_result_t
arp_entry_delete(struct arp_table *arp_table, int ifindex,
                 struct in_addr *dst_addr, uint8_t *ll_addr) {
  lagopus_result_t rv;
  struct arp_entry *entry;
  int cstate;
  (void) ifindex;
  (void) ll_addr;

  lagopus_rwlock_writer_enter_critical(&arp_table->lock, &cstate);
  rv = lagopus_hashmap_find_no_lock(&arp_table->hashmap,
                                    (void *)(dst_addr->s_addr),
                                    (void **)&entry);

  if (entry != NULL || rv == LAGOPUS_RESULT_OK) {
    rv = lagopus_hashmap_delete_no_lock(&arp_table->hashmap,
                                        (void *)dst_addr->s_addr,
                                        (void **)&entry, true);
    if (rv != LAGOPUS_RESULT_OK) {
      (void)lagopus_rwlock_leave_critical(&arp_table->lock, cstate);
      return rv;
    }
  }
  (void)lagopus_rwlock_leave_critical(&arp_table->lock, cstate);

  return rv;
}

/**
 * Update arp entry.
 */
lagopus_result_t
arp_entry_update(struct arp_table *arp_table, int ifindex,
                 struct in_addr *dst_addr, uint8_t *ll_addr) {
  lagopus_result_t rv;
  struct arp_entry *entry;
  struct arp_entry *dentry;
  struct arp_entry *arp;

  rv = lagopus_hashmap_find_no_lock(&arp_table->hashmap,
                                    (void *)(dst_addr->s_addr),
                                    (void **)&arp);
  if (rv == LAGOPUS_RESULT_NOT_FOUND) {
    /* create new entry. */
    entry = calloc(1, sizeof(struct arp_entry));
    if (entry == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }

    /* set arp information. */
    entry->ifindex = ifindex;
    entry->ip = *dst_addr;
    memcpy(entry->mac_addr, ll_addr, UPDATER_ETH_LEN);

    /* add to hashmap. */
    dentry = entry;
    rv = lagopus_hashmap_add_no_lock(&arp_table->hashmap,
                                     (void *)dst_addr->s_addr,
                                     (void **)&dentry, true);
    if (rv != LAGOPUS_RESULT_OK) {
      arp_entry_free(entry);
    }
  } else if (rv == LAGOPUS_RESULT_OK) {
    /* if the arp entry already exists, to update the entry contents. */
    if (arp->ifindex != ifindex ||
        memcmp(arp->mac_addr, ll_addr, UPDATER_ETH_LEN) != 0) {
      arp->ifindex = ifindex;
      memcpy(arp->mac_addr, ll_addr, UPDATER_ETH_LEN);
    }
  } else {
    /* find hashmap error. */
    lagopus_msg_warning("lagopus_hashmap_find() error rv = %d.\n", (int)rv);
  }

  return rv;
}

/**
 * Get arp entry.
 */
lagopus_result_t
arp_get(struct arp_table *arp_table, struct in_addr *addr,
        uint8_t *mac, int *ifindex) {
  lagopus_result_t rv;
  struct arp_entry *arp;
  int cstate;

  lagopus_rwlock_reader_enter_critical(&arp_table->lock, &cstate);
  rv = lagopus_hashmap_find_no_lock(&arp_table->hashmap,
                                    (void *)(addr->s_addr), (void **)&arp);
  if (arp != NULL && rv == LAGOPUS_RESULT_OK) {
    mac[0] = arp->mac_addr[0];
    mac[1] = arp->mac_addr[1];
    mac[2] = arp->mac_addr[2];
    mac[3] = arp->mac_addr[3];
    mac[4] = arp->mac_addr[4];
    mac[5] = arp->mac_addr[5];
    *ifindex = arp->ifindex;
  } else {
    PRINTF("arp no entry.\n");
    *ifindex = -1;
  }
  (void)lagopus_rwlock_leave_critical(&arp_table->lock, cstate);

  return rv;
}

/**
 * Clear all entries.
 */
lagopus_result_t
arp_entries_all_clear(struct arp_table *arp_table) {
  return lagopus_hashmap_clear(&arp_table->hashmap, true);
}

/**
 * Copy all entries.
 */
lagopus_result_t
arp_entries_all_copy(struct arp_table *src, struct arp_table *dst) {
  lagopus_result_t rv;
  struct arp_entry_args aa;
  unsigned int i;

  /* clear dst table. */
  rv = arp_entries_all_clear(dst);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }

  /* get entries from reading table*/
  aa.num = (unsigned int)lagopus_hashmap_size(src);
  aa.no = 0;
  aa.entries = calloc(1, sizeof(struct arp_entry) * aa.num);
  lagopus_hashmap_iterate(src, copy_arp_entry, &aa);

  /* write entries to writing table. */
  for (i = 0; i < aa.num; i++) {
    rv = arp_entry_update(dst, aa.entries[i].ifindex,
                          &(aa.entries[i].ip), aa.entries[i].mac_addr);
    if (rv != LAGOPUS_RESULT_OK) {
      break;
    }
  }

  if (aa.entries) {
    free(aa.entries);
  }

  return rv;
}




