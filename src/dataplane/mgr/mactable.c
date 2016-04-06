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
 *      @file   mactable.c
 *      @brief  MAC address table management.
 */

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include "lagopus_apis.h"
#include "lagopus/port.h"
#include "lagopus/bridge.h"
#include "pktbuf.h"
#include "packet.h"


/* Static functions. */
/**
 * Convert mac address type from uint8_t[6] to uint64_t.
 * param[in]  ethaddr uint8_t[6] mac address.
 * param[out] inteth  uint64_6 mac address.
 */
static inline void
copy_mac_address(const uint8_t ethaddr[], uint64_t *inteth) {
  (*inteth) = (uint64_t)ethaddr[5] << 40 |
              (uint64_t)ethaddr[4] << 32 |
              (uint64_t)ethaddr[3] << 24 |
              (uint64_t)ethaddr[2] << 16 |
              (uint64_t)ethaddr[1] << 8 |
              (uint64_t)ethaddr[0];
}

/* for debug */
static lagopus_result_t
print_mac(uint64_t mac) {
  int i, j;
  uint8_t str[6];
  for (i = 6 - 1, j = 0; i >= 0; i--, j++)
    str[j] = 0xff & mac >> (8 * i);
  printf("%02x:%02x:%02x:%02x:%02x:%02x", str[5], str[4], str[3], str[2], str[1], str[0]);
}

void
print_macaddr_in_ethheader(struct lagopus_packet *pkt) {
  uint64_t inteth;

  printf("[ether header] dst: ");
  copy_mac_address(pkt->eth->ether_dhost, &inteth);
  print_mac(inteth);
  printf(", src: ");
  copy_mac_address(pkt->eth->ether_shost, &inteth);
  print_mac(inteth);
  printf("\n");
}

/**
 * Allocate mac entry for mactable.
 * mac entry is struct macentry object.
 * @param[in] inteth MAC address.
 */
static struct macentry *
macentry_alloc(uint64_t inteth) {
  struct macentry *entry;

  entry = calloc(1, sizeof(struct macentry));
  if (entry != NULL) {
    entry->inteth = inteth;
  }
  return entry;
}

/**
 * Free mac entry for mactable.
 * @param[in] macentry MAC address entry.
 */
static void
macentry_free(struct macentry *macentry) {
  free(macentry);
}

/**
 * Age out.
 * Only if the address type is dynamic.
 * @param[in] mactable MAC address table.
 */
lagopus_result_t
mactable_age_out(struct mactable *mactable) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  int cstate;
  struct macentry *macentry;

  lagopus_rwlock_writer_enter_critical(&mactable->lock, &cstate);
  (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);

  lagopus_rwlock_reader_enter_critical(&mactable->lock, &cstate);
  /* check the update_time from the top of the macentry_list */
  while ((macentry = TAILQ_FIRST(&mactable->macentry_list)) != NULL) {
    /* get current time. */
    struct timespec now = get_current_time();

    if (now.tv_sec - macentry->update_time.tv_sec >= mactable->ageing_time) {
      lagopus_msg_info("mactable_age_out: expired mac entry.\n");

      (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);

      lagopus_rwlock_writer_enter_critical(&mactable->lock, &cstate);
      /* this entry is expired. */
      /* remove mac entry from macentry_list. */
      TAILQ_REMOVE(&mactable->macentry_list, macentry, next);

      /* remove mac entry from hashmap. */
      rv = lagopus_hashmap_delete(&mactable->hashmap,
                                  (void *)macentry->inteth,
                                  (void **)&macentry,
                                  true);
      mactable->nentries--;

      (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);
      lagopus_rwlock_reader_enter_critical(&mactable->lock, &cstate);

      if (rv != LAGOPUS_RESULT_OK) {
        /* remove fialure from hashmap. */
        break;
      }
    } else {
      /* nothing to do. */
      break;
    }
  }
  (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);

  return rv;
}

/**
 * Delete mac entry.
 * @param[in] mactable MAC address table.
 * @param[in] macentry MAC address entry.
 * @param[in] inteth MAC address.
 */
static lagopus_result_t
entry_delete(struct mactable *mactable,
             struct macentry *entry,
             uint64_t inteth) {
  lagopus_result_t rv;
  int cstate;

  if (entry->address_type == MACTABLE_SETTYPE_DYNAMIC) {
    lagopus_rwlock_writer_enter_critical(&mactable->lock, &cstate);
    TAILQ_REMOVE(&mactable->macentry_list, entry, next);
    (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);
  }

  lagopus_rwlock_writer_enter_critical(&mactable->lock, &cstate);
  mactable->nentries--;
  (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);

  rv = lagopus_hashmap_delete(&mactable->hashmap, (void *)inteth,
                              (void **)&entry,
                              true);
  return rv;
}

/**
 * Add mac entry.
 * @param[in] mactable MAC address table.
 * @param[in] macentry MAC address entry.
 * @param[in] inteth MAC address.
 */
static lagopus_result_t
entry_add(struct mactable *mactable, struct macentry *entry, uint64_t inteth) {
  lagopus_result_t rv;
  struct macentry *dentry;
  int cstate;

  /* Add new entry. */
  dentry = entry;
  rv = lagopus_hashmap_add(&mactable->hashmap, (void *)inteth,
                           (void **)&dentry, true);
  if (rv != LAGOPUS_RESULT_OK) {
    goto out;
  }
  lagopus_rwlock_writer_enter_critical(&mactable->lock, &cstate);
  mactable->nentries++;
  (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);
  /* Update list */
  if (entry->address_type == MACTABLE_SETTYPE_DYNAMIC) {
    lagopus_rwlock_writer_enter_critical(&mactable->lock, &cstate);
    TAILQ_INSERT_TAIL(&mactable->macentry_list, entry, next);
    (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);
  }

out:
  return rv;
}
/**
 * Update mac entry.
 * @param[in] mactable MAC address table.
 * @param[in] old Pointer of registered mac entry .
 * @param[in] inteth MAC address.
 */
static struct macentry*
add_new_entry(struct mactable *mactable, uint64_t inteth,
              uint32_t portid, uint16_t address_type) {
  lagopus_result_t rv;
  struct macentry *entry;
  int cstate;

  /* add new entry */
  entry = macentry_alloc(inteth);
  if (entry == NULL) {
    rv = LAGOPUS_RESULT_NO_MEMORY;
    goto out;
  }
  entry->update_time = get_current_time();
  entry->portid = portid;
  entry->address_type = address_type;
  rv = entry_add(mactable, entry, inteth);

out:
  if (rv != LAGOPUS_RESULT_OK && entry != NULL) {
    macentry_free(entry);
    entry = NULL;
  }
  return entry;
}

/**
 * Find mac entry in mactable.
 * @param[in] mactable MAC address table.
 * @param[in] ethaddr MAC address.
 */
static struct macentry *
find_entry_in_mactable(struct mactable *mactable, const uint8_t ethaddr[]) {
  struct macentry *dentry, *entry;
  uint64_t inteth;
  int cstate;
  lagopus_result_t rv;

  copy_mac_address(ethaddr, &inteth);
  rv = lagopus_hashmap_find(&mactable->hashmap,
                            (void *)inteth,
                            (void **)&dentry);
  if (dentry == NULL || rv != LAGOPUS_RESULT_OK) {
    return NULL;
  }

  /* update update_time */
  lagopus_rwlock_writer_enter_critical(&dentry->lock, &cstate);
  dentry->update_time = get_current_time();
  (void)lagopus_rwlock_leave_critical(&dentry->lock, cstate);

  return dentry;
}

/**
 * Learning mac address.
 * @param[in] mactable MAC address table.
 * @param[in] portid Target port no.
 * @param[in] ethaddr Target mac address.
 * @param[in] address_type Type(static or dynamic) of mac address learning.
 */
static lagopus_result_t
learn_port(struct mactable *mactable,
           uint32_t portid,
           const uint8_t ethaddr[],
           uint16_t address_type) {
  struct macentry *entry, *dentry;
  uint64_t inteth = 0;
  lagopus_result_t rv;
  int cstate;

  if (ethaddr == NULL || mactable == NULL) {
    rv = LAGOPUS_RESULT_INVALID_ARGS;
    goto out;
  }

  copy_mac_address(ethaddr, &inteth);

  lagopus_rwlock_reader_enter_critical(&mactable->lock, &cstate);
  /* check max number of entries */
  if (unlikely(mactable->nentries >= mactable->maxentries)) {

    dentry = TAILQ_FIRST(&mactable->macentry_list);

    if (unlikely(dentry == NULL)) {
      lagopus_msg_info("mactable is full.all entries are static.remove any entries.\n");
      rv = LAGOPUS_RESULT_NO_MEMORY;
      goto out;
    }

    (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);
    lagopus_rwlock_writer_enter_critical(&mactable->lock, &cstate);
    mactable->nentries--;


    TAILQ_REMOVE(&mactable->macentry_list, dentry, next);
    (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);
    lagopus_rwlock_reader_enter_critical(&mactable->lock, &cstate);
    rv = lagopus_hashmap_delete(&mactable->hashmap,
                                (void *)dentry->inteth,
                                (void **)&dentry,
                                true);
    if (rv != LAGOPUS_RESULT_OK) {
      goto out;
    }
  }
  (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);

  /* check entry */
  rv = lagopus_hashmap_find(&mactable->hashmap,
                            (void *)inteth,
                            (void **)&dentry);
  if (likely(dentry != NULL)) {
    if (dentry->portid != portid) {
      /* delete old entry */
      rv = entry_delete(mactable, dentry, inteth);
      if (rv != LAGOPUS_RESULT_OK) {
        goto out;
      }
      /* add new entry */
      add_new_entry(mactable, inteth, portid, address_type);
    } else {
      /* update update_time */
      lagopus_rwlock_writer_enter_critical(&dentry->lock, &cstate);
      dentry->update_time = get_current_time();
      (void)lagopus_rwlock_leave_critical(&dentry->lock, cstate);
      rv = LAGOPUS_RESULT_OK;
    }
  } else {
    /* add new entry */
    add_new_entry(mactable, inteth, portid, address_type);
    rv = LAGOPUS_RESULT_OK;
  }

out:
  return rv;
}

/* Public functions. */
lagopus_result_t
init_mactable(struct mactable *mactable) {
  lagopus_result_t rv;

  mactable->nentries = 0;
  mactable->maxentries = 8192; /* default value */
  mactable->ageing_time = 300; /* default value */
  TAILQ_INIT(&mactable->macentry_list);
  rv = lagopus_rwlock_create(&mactable->lock);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = lagopus_hashmap_create(&mactable->hashmap,
                                LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                                macentry_free);
  }

  /* set cleanup timer. */
  add_mactable_timer(mactable, MACTABLE_CLEANUP_TIME);

  return rv;
}

lagopus_result_t
fini_mactable(struct mactable *mactable) {
  if (mactable->hashmap != NULL) {
    lagopus_hashmap_destroy(&mactable->hashmap, true);
  }
  lagopus_rwlock_destroy(&mactable->lock);
  return LAGOPUS_RESULT_OK;
}

uint32_t
find_and_learn_port_in_mac_table(struct lagopus_packet *pkt) {
  struct macentry *entry;

  mactable_age_out(&pkt->bridge->mactable);

  learn_port(&pkt->bridge->mactable,
             pkt->in_port->ofp_port.port_no,
             pkt->eth->ether_shost,
             MACTABLE_SETTYPE_DYNAMIC);
  entry = find_entry_in_mactable(&pkt->bridge->mactable,
                                 pkt->eth->ether_dhost);
  if (entry != NULL) {
    return entry->portid;
  } else {
    return OFPP_ALL;
  }
}

void
learning_port_in_mac_table(struct lagopus_packet *pkt) {
  mactable_age_out(&pkt->in_port->bridge->mactable);

  learn_port(&pkt->in_port->bridge->mactable,
             pkt->in_port->ofp_port.port_no,
             pkt->eth->ether_shost,
             MACTABLE_SETTYPE_DYNAMIC);
}

uint32_t
lookup_port_in_mac_table(struct lagopus_packet *pkt) {
  struct macentry *entry;

  entry = find_entry_in_mactable(&pkt->in_port->bridge->mactable,
                                 pkt->eth->ether_dhost);
  if (entry != NULL) {
    return entry->portid;
  } else {
    return OFPP_ALL;
  }
}

/* For configuration */
void
mactable_set_ageing_time(struct mactable *mactable, uint32_t ageing_time) {
  int cstate;

  lagopus_rwlock_writer_enter_critical(&mactable->lock, &cstate);
  mactable->ageing_time = ageing_time;
  (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);
}

void
mactable_set_max_entries(struct mactable *mactable, uint32_t max_entries) {
  int cstate;

  lagopus_rwlock_writer_enter_critical(&mactable->lock, &cstate);
  mactable->maxentries = max_entries;
  (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);
}

uint32_t
mactable_get_ageing_time(struct mactable *mactable) {
  uint32_t ret;
  int cstate;

  lagopus_rwlock_reader_enter_critical(&mactable->lock, &cstate);
  ret = mactable->ageing_time;
  (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);

  return ret;
}

uint32_t
mactable_get_max_entries(struct mactable *mactable) {
  uint32_t ret;
  int cstate;

  lagopus_rwlock_reader_enter_critical(&mactable->lock, &cstate);
  ret = mactable->maxentries;
  (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);

  return ret;
}


lagopus_result_t
mactable_entry_update(struct mactable *mactable,
                      const uint8_t ethaddr[],
                      uint32_t portid) {
  return learn_port(mactable, portid, ethaddr, MACTABLE_SETTYPE_STATIC);
}

lagopus_result_t
mactable_entry_delete(struct mactable *mactable, const uint8_t ethaddr[]) {
  struct macentry *dentry;
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  uint64_t inteth = 0;

  dentry = find_entry_in_mactable(mactable, ethaddr);
  if (dentry != NULL) {
    copy_mac_address(ethaddr, &inteth);
    rv = entry_delete(mactable, dentry, inteth);
  }
  return rv;
}

lagopus_result_t
mactable_entry_clear(struct mactable *mactable) {
  struct macentry *macentry;
  int cstate;

  /* all clear entries in macentry_list */
  lagopus_rwlock_writer_enter_critical(&mactable->lock, &cstate);
  while ((macentry = TAILQ_FIRST(&mactable->macentry_list)) != NULL) {
    TAILQ_REMOVE(&mactable->macentry_list, macentry, next);
  }
  (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);

  /* all clear entries in hashmap */
  lagopus_hashmap_clear(&mactable->hashmap, true);

  return LAGOPUS_RESULT_OK;
}

