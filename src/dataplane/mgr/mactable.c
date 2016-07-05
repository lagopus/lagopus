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
#include <pthread.h>

#if defined HYBRID && defined PIPELINER
#include "pipeline.h"
#endif /* HYBRID && PIPELINER */

#define NR_MAX_ENTRIES 1024  /**< max number that can be registered
                                  in the bbq. */

/**
 * Struct mac entry args for get all entries from mactable.
 */
struct macentry_args {
  unsigned int num;  /**< number of entries. */
  unsigned int no;  /**< current entry's index. */
  struct macentry *entries;  /**< mac entries. */
};

/**
 * Convert type of mac address.
 * @param[in] inteth MAC address.
 */
static inline uint64_t
array_to_uint64(const uint8_t ethaddr[]) {
  uint64_t inteth = (uint64_t)ethaddr[5] << 40 |
                    (uint64_t)ethaddr[4] << 32 |
                    (uint64_t)ethaddr[3] << 24 |
                    (uint64_t)ethaddr[2] << 16 |
                    (uint64_t)ethaddr[1] << 8 |
                    (uint64_t)ethaddr[0];
  return inteth;
}

/**
 * Get local data.
 * @param[in] mactable MAC address table object.
 */
static struct local_data *
get_local_data (struct mactable *mactable) {
  uint32_t wid = 0;
#if defined PIPELINER
  wid = pipeline_worker_id;
#elif defined HAVE_DPDK
  wid = dpdk_get_worker_id();
  if (wid < 0 || wid >= UPDATER_LOCALDATA_MAX_NUM || wid == UINT32_MAX) {
    wid = 0;
  }
#endif
  return &mactable->local[wid];
}

/**
 * Create mac entry.
 * mac entry is struct macentry object.
 * @param[in] inteth MAC address.
 * @param[in] portid In port number.
 * @param[in] address_type Type(static or dynamic) of mac address learning.
 */
static struct macentry *
macentry_alloc(uint64_t inteth, uint32_t portid,
               uint16_t address_type) {
  struct macentry *entry;

  entry = calloc(1, sizeof(struct macentry));
  if (entry != NULL) {
    entry->inteth = inteth;
    entry->portid = portid;
    entry->address_type = address_type;
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
 * Free mac entry for bbq.
 * @param[in] data MAC address entry.
 */
static void
free_bbq_entry(void **data) {
  if (likely(data != NULL && *data != NULL)) {
    struct macentry *entry = *data;
    free(entry);
  }
}

/**
 * Copy mac entry when get all entreis from mactable.
 */
static bool
copy_macentry(void *key, void *val, lagopus_hashentry_t he, void *arg) {
  bool result = false;
  struct macentry_args *ma = (struct macentry_args *)arg;
  struct macentry *entries = (struct macentry *)ma->entries;
  struct macentry *entry = (struct macentry *)val;

  if (val != NULL && ma->no < ma->num) {
    entries[ma->no].inteth = entry->inteth;
    entries[ma->no].portid = entry->portid;
    entries[ma->no].update_time = entry->update_time;
    entries[ma->no].address_type = entry->address_type;
    ma->no++;
    result = true;
  }

  return result;
}

/**
 * Add mac entry to bbq.
 * @param[in] mactable MAC address table.
 * @param[in] inteth MAC address
 * @param[in] portid In port number.
 * @param[in] address_type Setting address type.
 * @param[in] now Update time.
 */
static inline lagopus_result_t
add_entry_bbq(struct local_data *local, uint64_t inteth,
              uint32_t portid, uint16_t address_type) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  struct macentry *entry;

  entry = macentry_alloc(inteth, portid, address_type);
  if (entry) {
    lagopus_bbq_put(&local->bbq, &entry, struct macentry *, 0);
  } else {
    rv = LAGOPUS_RESULT_ANY_FAILURES;
  }

  return rv;
}


/**
 * Add mac entry to mactable for writing.
 * @param[in] mactable MAC address table.
 * @param[in] write_table MAC address table for writing.
 * @param[in] inteth MAC address
 * @param[in] portid In port number.
 * @param[in] address_type Setting address type.
 * @param[in] now Update time.
 */
static lagopus_result_t
add_entry_write_table(struct mactable *mactable, lagopus_hashmap_t *write_table,
                      uint64_t inteth, uint32_t portid, uint16_t address_type,
                      struct timespec now) {
  lagopus_result_t rv;
  struct macentry *entry;
  struct macentry *dentry;
  struct macentry *find_entry;

  if (mactable == NULL || write_table == NULL) {
    rv = LAGOPUS_RESULT_INVALID_ARGS;
    goto out;
  }

  /* allcate new entry. */
  entry = macentry_alloc(inteth, portid, address_type);
  if (entry == NULL) {
    rv = LAGOPUS_RESULT_NO_MEMORY;
    goto out;
  }
  entry->update_time = now;
  dentry = entry;

  /* if same key(inteth) entry was already registered,
     update entry in hashmap_add(4th argument means allow overwrite). */
  rv = lagopus_hashmap_find_no_lock(write_table,
                                    (void *)inteth,
                                    (void **)&find_entry);
  if (rv == LAGOPUS_RESULT_NOT_FOUND) {
    /* new entry */
    if (entry->address_type == MACTABLE_SETTYPE_DYNAMIC) {
      TAILQ_INSERT_TAIL(&mactable->macentry_list, entry, next);
    }

    mactable->nentries++;
  } else if (find_entry != NULL && rv == LAGOPUS_RESULT_OK) {
    /* update entry */
    if (find_entry->address_type == MACTABLE_SETTYPE_DYNAMIC) {
      TAILQ_REMOVE(&mactable->macentry_list, find_entry, next);
      TAILQ_INSERT_TAIL(&mactable->macentry_list, entry, next);
    } else {
      dentry->address_type = MACTABLE_SETTYPE_STATIC;
    }
  } else {
    lagopus_msg_error("lagopus hashmap find failed\n");
    goto out;
  }

  rv = lagopus_hashmap_add_no_lock(write_table, (void *)inteth,
                                   (void **)&dentry, true);

out:
  return rv;
}

/**
 * Write entry to local cache.
 * @param[in] h Hashamap for local cache.
 * @param[in] inteth MAC address
 * @param[in] portid Port number.
 * @param[in] address_type Type(static or dynamic) of mac address learning.
 */
static lagopus_result_t
add_entry_local_cache(lagopus_hashmap_t *h, uint64_t inteth,
                  uint32_t portid, uint16_t address_type) {
  lagopus_result_t rv = LAGOPUS_RESULT_ANY_FAILURES;
  struct macentry *entry;

  entry = macentry_alloc(inteth, portid, address_type);
  rv = lagopus_hashmap_add_no_lock(h, (void *)inteth, (void **)&entry, true);

  return rv;
}

/**
 * Get output port and address type by mac address from mac address table
 * (local cache or reading mac address table).
 * @param[in] h Target hashmap(localcache or reading mactable).
 * @param[in] inteth MAC address.
 * @param[out] port Number of output port.
 */
static inline lagopus_result_t
lookup(lagopus_hashmap_t *h, uint64_t inteth,
       uint32_t *port, uint16_t *address_type) {
  lagopus_result_t rv = LAGOPUS_RESULT_ANY_FAILURES;
  struct macentry *dentry;

  rv = lagopus_hashmap_find_no_lock(h, (void *)inteth, (void **)&dentry);
  if (rv == LAGOPUS_RESULT_OK) {
    *port = dentry->portid;
    *address_type = dentry->address_type;
  }

  return rv;
}

/**
 *
 */
static lagopus_result_t
check_eth_history (struct local_data *local, uint64_t inteth, bool switched) {
  lagopus_result_t rv;
  int i;

  /* mactable were switched, clear history. */
  if (unlikely(switched)) {
    /* clear history to reset when mactables are switched. */
    for (i = 0; i < MACTABLE_HISTORY_MAX_NUM; i++) {
      local->eth_history[i] = 0;
    }
    local->history_index = 0;
  } else {
    /* check history for entry decimation. */
    for (i = 0; i < MACTABLE_HISTORY_MAX_NUM; i++) {
      /* start history_index - 1. */
      uint16_t index =
        (local->history_index + MACTABLE_HISTORY_MAX_NUM - (i + 1)) % MACTABLE_HISTORY_MAX_NUM;
      if (local->eth_history[index] == inteth) {
        /* don't write to bbq. */
        return LAGOPUS_RESULT_ALREADY_EXISTS;
      }
    }
  }

  local->eth_history[local->history_index] = inteth;
  local->history_index = (local->history_index + 1) % 10;

  return LAGOPUS_RESULT_NOT_FOUND;
}

/**
 * Reset referred flag, check and return result if switched.
 * @param[in] mactable MAC address table.
 * @param[in] local Local data for each worker.
 */
static bool
check_referred(struct mactable *mactable, struct local_data *local) {
  uint32_t referred = __sync_val_compare_and_swap(&local->referred_table,
                                                  mactable->read_table^1,
                                                  mactable->read_table);
  if (referred == local->referred_table) {
    return false;
  } else {
    return true;
  }
}

/**
 * Learning mac address to mac address table(write to local cache or bbq).
 * When writing to the bbq, thinning the duplicate entry.
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
  uint64_t inteth = array_to_uint64(ethaddr);
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  lagopus_hashmap_t *h;
  struct macentry *entry, *dentry;
  struct local_data *local;
  int i;
  bool switched;
  uint32_t read_table, referred;

  if (mactable == NULL) {
    rv = LAGOPUS_RESULT_INVALID_ARGS;
    return rv;
  }

  /* get local data. */
  local = get_local_data(mactable);

  /* check reference index. */
  switched = check_referred(mactable, local);
  if (switched) {
    /* clear local cache. */
    lagopus_hashmap_clear(&local->localcache, true);
  }

  /* find entry, then write to cache if not found. */
  rv = lagopus_hashmap_find_no_lock(&local->localcache,
                                    (void *)inteth, (void **)&dentry);
  if (rv == LAGOPUS_RESULT_NOT_FOUND) {
    rv = add_entry_local_cache(&local->localcache, inteth,
                               portid, address_type);
    if (rv != LAGOPUS_RESULT_OK) {
      /* failed lookup from local cache. */
      lagopus_msg_warning("local cache operation failed(%d).\n", rv);
      goto out;
    }
  }

  /* thinning out the entry to be added to the queue, *
   * to reduce the load on the 'updater'.             */
  if (check_eth_history(local, inteth, switched) == LAGOPUS_RESULT_NOT_FOUND) {
    /* 'updater' updates the update_time in macentry. *
     * therefore, 'worker' add an entry to the bbq,   *
     * to notify that there was reference.            */
    rv = add_entry_bbq(local, inteth, portid, address_type);
  }

out:
  return rv;
}

/**
 * Get output port by the mac address
 * in mac address table(local cache or reading mac address table).
 * @param[in] mactable MAC address table.
 * @param[in] ethaddr MAC address.
 */
static uint32_t
lookup_port(struct mactable *mactable,
            const uint8_t ethaddr[]) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  lagopus_hashmap_t *h;
  uint64_t inteth = array_to_uint64(ethaddr);
  uint32_t port;
  uint16_t addr_type;
  struct local_data *local;
  bool switched = false;
  uint32_t read_table, referred;

  if (mactable == NULL) {
    rv = LAGOPUS_RESULT_INVALID_ARGS;
    return OFPP_ALL;
  }

  /* get local data. */
  local = get_local_data(mactable);

  /*
    "local->referring" is a flag that indicates that it's in operation.
    Before performing the operation on mactable, it must be turned on(1).
    Then, after the operation ends, it must be turned off(0).
    Only worker's operation(learning and lookup) to change this flag.
   */
  __sync_add_and_fetch(&local->referring, 1);

  /* check reference index. */
  switched = check_referred(mactable, local);
  if (switched) {
    /* clear local cache. */
    lagopus_hashmap_clear(&local->localcache, true);
  } else {
    /* check local cache. */
    rv = lookup(&local->localcache, inteth, &port, &addr_type);
    if (rv == LAGOPUS_RESULT_OK) {
      goto out;
    }
  }

  /* lookup from read mactable */
  read_table = __sync_add_and_fetch(&mactable->read_table, 0);
  h = &mactable->hashmap[read_table];
  rv = lookup(h, inteth, &port, &addr_type);
  if (rv == LAGOPUS_RESULT_OK) {
    /* write to local cache(no entry in local cache). */
    add_entry_local_cache(&local->localcache, inteth, port, addr_type);
  } else {
    port = OFPP_ALL;
  }

out:
  if (port != OFPP_ALL &&
      check_eth_history(local, inteth, switched) == LAGOPUS_RESULT_NOT_FOUND) {
    /* 'updater' updates the update_time in macentry. *
     * therefore, 'worker' add an entry to the bbq,   *
     * to notify that there was reference.            */
    rv = add_entry_bbq(local, inteth, port, addr_type);
  }

  __sync_sub_and_fetch(&local->referring, 1);
  return port;
}

/**
 * Age out(= remove old entries).
 * This function is called from mactable_update().
 * Utilizing the diffrence between the current time and update time
 * that the entry has.
 * Deletable entry(address_type is MACTABLE_SETTYPE_DYNAMIC) is registered
 * in the macentry_list.
 * @param[in] mactable MAC address table object.
 */
static lagopus_result_t
age_out_write_table(struct mactable *mactable, lagopus_hashmap_t *write_table) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  struct macentry *entry;
  struct timespec now = get_current_time();

  /* check the update_time from the top of the macentry_list */
  while ((entry = TAILQ_FIRST(&mactable->macentry_list)) != NULL) {

    if (now.tv_sec - entry->update_time.tv_sec >= mactable->ageing_time) {
      lagopus_msg_info("mactable_age_out: expired mac entry.\n");

      /* this entry is expired. */
      /* remove mac entry from macentry_list. */
      TAILQ_REMOVE(&mactable->macentry_list, entry, next);

      /* remove mac entry from hashmap. */
      rv = lagopus_hashmap_delete_no_lock(write_table,
                                          (void *)entry->inteth,
                                          (void **)&entry,
                                          true);
      mactable->nentries--;

      if (rv != LAGOPUS_RESULT_OK) {
        /* remove fialure from hashmap. */
        break;
      }
    } else {
      /* nothing to do. */
      break;
    }
  }

  return rv;
}

/**
 * Initialize mactable object.
 * This function must be called when the bridge object creation.
 * @param[in] mactable MAC address table object.
 */
lagopus_result_t
mactable_init(struct mactable *mactable) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  int i, j;

  /* create read-write lock. */
  rv = lagopus_rwlock_create(&mactable->lock);
  if (rv != LAGOPUS_RESULT_OK) {
    return rv;
  }

  /* set mac table configuration */
  mactable->maxentries = 8192;
  mactable->ageing_time = 300;

  mactable->nentries = 0;
  TAILQ_INIT(&mactable->macentry_list);

  for (i = 0; i < UPDATER_LOCALDATA_MAX_NUM; i++) {
    struct local_data *local = &mactable->local[i];
    /* initialize localcache */
    lagopus_hashmap_create(&local->localcache,
                           LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                           macentry_free);

    /* create mac entry queue */
    rv = lagopus_bbq_create(&local->bbq, struct macentry *,
                            NR_MAX_ENTRIES, free_bbq_entry);
    if (rv != LAGOPUS_RESULT_OK) {
      lagopus_perror(rv);
      return rv;
    }

    for (j = 0; j < MACTABLE_HISTORY_MAX_NUM; j++) {
      local->eth_history[j] = 0;
    }
    local->history_index = 0;
    __sync_lock_test_and_set(&local->referring, 0);
    __sync_lock_release(&local->referring);
  }

  /* init read_tbl flag */
  __sync_lock_test_and_set(&mactable->read_table, 0);
  __sync_lock_release(&mactable->read_table);

  /* create hashmap for mactable */
  for (i = 0; i < 2; i++) {
    rv = lagopus_hashmap_create(&mactable->hashmap[i],
                                LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                                macentry_free);
  }

  return rv;
}

/**
 * Finalize mactable object.
 * This function must be called when the bridge object discarded.
 * @param[in] mactable MAC address table object.
 */
lagopus_result_t
mactable_fini(struct mactable *mactable) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  int i;

  /* destroy hashmap for mactable. */
  for (i = 0; i < 2; i++) {
    lagopus_hashmap_destroy(&mactable->hashmap[i], true);
  }

  for (i = 0; i < UPDATER_LOCALDATA_MAX_NUM; i++) {
    /* destroy local cache. */
    lagopus_hashmap_destroy(&mactable->local[i].localcache, true);

    /* destroy bbq. */
    lagopus_bbq_shutdown(&mactable->local[i].bbq, true);
    lagopus_bbq_destroy(&mactable->local[i].bbq, true);
  }

  /* destroy read-write lock. */
  lagopus_rwlock_destroy(&mactable->lock);

  return rv;
}

/**
 * Update mac address table by timer('updater').
 * Entry data are written to the writing mac table by only 'updater'.
 * They are merged read mac table and bbq.
 * @param[in] mactable MAC address table object.
 */
lagopus_result_t
mactable_update(struct mactable *mactable) {
  lagopus_result_t rv = LAGOPUS_RESULT_OK;
  lagopus_hashmap_t *rh, *wh;
  int i, j, cnt;
  struct macentry_args ma;
  struct macentry **ep = NULL;
  bool is_empty = true;
  unsigned int bbq_size = 0;
  uint32_t read_table;
  size_t get_num = 0, entry_num = 0, tmp_num = 0;

  /*
   * update mactable entry from new entries
   * in queue and read table.
   */
  read_table = __sync_add_and_fetch(&mactable->read_table, 0);

  /* check referred */
  for (cnt = 0; cnt < UPDATER_LOCALDATA_MAX_NUM; cnt++) {
    /*
     * Here, let's describe how to manage double mactables.
     *
     * We have 2 types of threads.
     * - Updater:
     * The updater is a maintainer of mactables. All mac entries should be
     * wrriten in one of mactables by the updater.
     * Currently the updater has 2 mactables. It looks like double buffering.
     * One of mactable is only for reading, and the another is for writing new
     * mac entries. At some point, the updater replaces mactable, then all
     * readers will refer latest mactables.
     * - Worker:
     * The worker refers one of mactable to process packets. The worker needs
     * to check which mactable is currently valid before processing packets.
     *
     * The updater has a below value to specify which mactable is valid.
     * - read_table:
     * The index value of mactable that stores newest mac entries. This value
     * is managed by the updater.
     *
     * Also, the worker has below values.
     * - referred
     * The index value of mactable that the worker is curretly referring.
     * - referring:
     * The status value indicates whether the worker is in a critical section,
     * or not.
     *  0: not in critical section.
     *  1: in critical section.
     *
     * The updater should know correct timing to write new macentiries,
     * and replace mactable. To know it, check below conditions.
     * - 'referred' and 'read_table' have different values, and the workers is
     *   in critical section.
     * If one of the workers is in above condition, just give up writing and
     * replacing. The next time the updater is invoked by timer, the updater
     * will try same things.
     *
     * Here is more detaled description why the updater needs to check like
     * above. If 'referred' and 'read_table' are different, obiously it means
     * old table is still referred by the worker. Even if values are different,
     * if worker 'is not' in a critical section, next time the worker will go
     * in the critical section, reffered value will be updated to latest one.
     * So the updater can start writing.
     *
     * Anyway, only the case that above condition is true in all the workers,
     * the updater cannot start writing.
     */
    uint32_t referred =
      __sync_add_and_fetch(&mactable->local[cnt].referred_table, 0);
    if (referred != read_table) {
      uint16_t referring =
        __sync_add_and_fetch(&mactable->local[cnt].referring, 0);
      if (referring == 1) {
        return LAGOPUS_RESULT_OK;
      }
    }
  }

  /* get tables */
  wh = &mactable->hashmap[read_table^1];
  rh = &mactable->hashmap[read_table];

  /* clear write table. */
  lagopus_hashmap_clear(wh, true);
  while ((mactable->macentry_list).tqh_first != NULL) {
    TAILQ_REMOVE(&mactable->macentry_list,
                 mactable->macentry_list.tqh_first, next);
    mactable->nentries = 0;
  }

  /* get entries from read table. */
  ma.num = (unsigned int)lagopus_hashmap_size(rh);
  ma.no = 0;
  ma.entries = malloc(sizeof(struct macentry) * ma.num);
  rv = lagopus_hashmap_iterate(rh, copy_macentry, &ma);

  /* check entry num. */
  entry_num = ma.num;
  if (ma.num > mactable->maxentries) {
    entry_num = mactable->maxentries;
    lagopus_msg_warning("mactable is full, drop read table entries(%d)\n",
                        ma.num - mactable->maxentries);
  }

  /* write entries to write table. */
  for (j = 0; j < entry_num; j++) {
    add_entry_write_table(mactable, wh,
                          ma.entries[j].inteth,
                          ma.entries[j].portid,
                          ma.entries[j].address_type,
                          ma.entries[j].update_time);
  }

  /* get entries from queue. */
  for (cnt = 0; cnt < UPDATER_LOCALDATA_MAX_NUM; cnt++) {
    get_num = 0;
    rv = lagopus_bbq_is_empty(&mactable->local[cnt].bbq, &is_empty);
    if (rv != LAGOPUS_RESULT_OK) {
      lagopus_msg_error("bbq_is_empty failed[%d].\n", (int)rv);
      if (ma.entries) {
        free(ma.entries);
      }
      return rv;
    }
    if (!is_empty) {
      bbq_size = lagopus_bbq_size(&mactable->local[cnt].bbq);
      ep = malloc(sizeof(struct macentry *) * bbq_size);
      lagopus_bbq_get_n(&mactable->local[cnt].bbq, ep, bbq_size, 0,
                        struct macentry *, 0, &get_num);
    }

    tmp_num = mactable->maxentries - entry_num;
    entry_num = get_num;
    if (get_num > tmp_num) {
      entry_num = tmp_num;
      lagopus_msg_warning("mactable is full, drop bbq entries(%d)\n",
                          get_num - tmp_num);
    }
    /* write entries to write table */
    for (i = 0; i < entry_num; i++) {
      struct timespec now = get_current_time();
      add_entry_write_table(mactable, wh,
                            ep[i]->inteth, ep[i]->portid,
                            ep[i]->address_type, now);
      free(ep[i]);
    }
  }

  /* age out */
  age_out_write_table(mactable, wh);

  /* free temporary data. */
  if (ma.entries) {
    free(ma.entries);
  }
  if (ep) {
    free(ep);
  }

  /*
   * switch table (write table <-> read table).
   */
  __sync_val_compare_and_swap(&mactable->read_table, read_table, read_table^1);

  return rv;
}

/**
 * Learning mac address and input port number when packet handling.
 * This function is called from l3 routing function in interface.c.
 * @param[in] pkt receive packet.
 */
void
mactable_port_learning(struct lagopus_packet *pkt) {
  learn_port(&pkt->in_port->bridge->mactable,
             pkt->in_port->ofp_port.port_no,
             pkt->eth->ether_shost,
             MACTABLE_SETTYPE_DYNAMIC);
}

/**
 * Look up output port in mac address table when packet handling.
 * This function is called from l3 routing function in interface.c.
 * @param[in] pkt receive packet.
 */
void
mactable_port_lookup(struct lagopus_packet *pkt) {
  uint32_t port;
  port = lookup_port(&pkt->in_port->bridge->mactable,
                     pkt->eth->ether_dhost);
  pkt->output_port = port;
}

/**
 * Delete all mac entries from mac address table by a request from datastore.
 * @param[in] mactable MAC address table object.
 */
lagopus_result_t
mactable_entry_clear(struct mactable *mactable) {
  return LAGOPUS_RESULT_OK;
}

/**
 * Delete a mac entry from mac address table by a request from datastore.
 * @param[in] mactable MAC address table object.
 * @param[in] ethaddr MAC address.
 */
lagopus_result_t
mactable_entry_delete(struct mactable *mactable, const uint8_t ethaddr[]) {
  return LAGOPUS_RESULT_OK;
}

/**
 * Add or modify a mac entry to mac address table by a request from datastore.
 * @param[in] mactable MAC address table object.
 * @param[in] ethaddr MAC address.
 * @param[in] portid Port number.
 */
lagopus_result_t
mactable_entry_update(struct mactable *mactable,
                      const uint8_t ethaddr[],
                      uint32_t portid) {
  struct local_data *local;
  local = get_local_data(mactable);
  return add_entry_bbq(local, array_to_uint64(ethaddr),
                       portid, MACTABLE_SETTYPE_STATIC);
}

/**
 * Get number of max entries by a request from datastore.
 * @param[in] mactable MAC address table object.
 */
uint32_t
mactable_max_entries_get(struct mactable *mactable) {
  uint32_t ret;
  int cstate;

  lagopus_rwlock_reader_enter_critical(&mactable->lock, &cstate);
  ret = mactable->maxentries;
  (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);

  return ret;
}

/**
 * Get ageing time by a request from datastore.
 * @param[in] mactable MAC address table object.
 */
uint32_t
mactable_ageing_time_get(struct mactable *mactable) {
  uint32_t ret;
  int cstate;

  lagopus_rwlock_reader_enter_critical(&mactable->lock, &cstate);
  ret =  mactable->ageing_time;
  (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);

  return ret;
}

/**
 * Set number of max entries by a request from datastore.
 * @param[in] mactable MAC address table object.
 * @param[in] max_entries Number of max entries.
 */
void
mactable_max_entries_set(struct mactable *mactable, uint32_t max_entries) {
  int cstate;

  lagopus_rwlock_writer_enter_critical(&mactable->lock, &cstate);
  mactable->maxentries = max_entries;
  (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);
}

/**
 * Set ageing time by a request from datastore.
 * @param[in] mactable MAC address table object.
 * @param[in] ageing_time Ageing time.
 */
void
mactable_ageing_time_set(struct mactable *mactable, uint32_t ageing_time) {
  int cstate;

  lagopus_rwlock_writer_enter_critical(&mactable->lock, &cstate);
  mactable->ageing_time = ageing_time;
  (void)lagopus_rwlock_leave_critical(&mactable->lock, cstate);

}

