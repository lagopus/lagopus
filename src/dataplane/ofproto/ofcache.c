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
 *      @file   ofcache.c
 *      @brief  OpenFlow flowcache implementation.
 */

#include "lagopus_config.h"

#include <sys/queue.h>
#include <stdlib.h>
#include <inttypes.h>

#include "lagopus_apis.h"
#include "lagopus/flowdb.h"

#include "pktbuf.h"
#include "packet.h"

#ifdef HAVE_DPDK
#include <rte_version.h>
#if RTE_VERSION >= RTE_VERSION_NUM(2, 1, 0, 0)
#include <rte_hash.h>
#ifdef __SSE4_2__
#include <rte_hash_crc.h>
#define HASH_FUNC       rte_hash_crc
#else
#include <rte_jhash.h>
#define HASH_FUNC       rte_jhash
#endif
#endif /* RTE_VERSION */
#endif /* HAVE_DPDK */

#define FLOWCACHE_BITLEN 32

#undef CACHE_DEBUG
#ifdef CACHE_DEBUG
#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...)
#endif

#define NBANK   2
#define FLOWCACHE_BITLEN 32
#define FLOWCACHE_MAX_ENTRIES 100000

struct flowcache_bank {
  int kvs_type;
  union {
    lagopus_hashmap_t hashmap;
#ifdef HAVE_DPDK
#if RTE_VERSION >= RTE_VERSION_NUM(2, 1, 0, 0)
    struct rte_hash *hash;
#endif /* RTE_VERSION */
#endif /* HAVE_DPDK */
  };
  /* statistics */
  uint64_t nentries;
  uint64_t hit;
  uint64_t miss;
  int bank;
};

/**
 * per thread cache object.
 */
struct flowcache {
  int used_bank;
  uint64_t max_entries;
  struct flowcache_bank *bank[NBANK];
};

TAILQ_HEAD(cache_entry_list, cache_entry);

struct cache_list {
  int nentries;
  struct cache_entry_list entries;
};

static struct cache_list *
init_cache_list(void) {
  struct cache_list *list;

  list = calloc(1, sizeof(struct cache_list));
  if (list == NULL) {
    goto out;
  }
  TAILQ_INIT(&list->entries);
out:
  return list;
}

static void
add_cache_list(struct cache_list *list, struct cache_entry *cache_entry) {
  TAILQ_INSERT_TAIL(&list->entries, cache_entry, next);
  list->nentries++;
}

static void
remove_cache_list(struct cache_list *list, struct cache_entry *cache_entry) {
  TAILQ_REMOVE(&list->entries, cache_entry, next);
  free(cache_entry);
  list->nentries--;
}

static void
fini_cache_list(struct cache_list *list) {
  struct cache_entry *entry;

  while ((entry = TAILQ_FIRST(&list->entries)) != NULL) {
    TAILQ_REMOVE(&list->entries, entry, next);
    free(entry);
  }
  free(list);
}

static struct flowcache_bank *
init_flowcache_bank(int kvs_type, int bank) {
  struct flowcache_bank *cache;

  cache = calloc(1, sizeof(struct flowcache_bank));
  if (cache == NULL) {
    return NULL;
  }
  cache->kvs_type = kvs_type;
  cache->bank = bank;
  switch (cache->kvs_type) {
#ifdef HAVE_DPDK
#if RTE_VERSION >= RTE_VERSION_NUM(2, 1, 0, 0)
    case FLOWCACHE_RTE_HASH:
      {
        struct rte_hash_parameters params;
        char name[80];

        memset(&params, 0, sizeof(params));
        snprintf(name, sizeof(name), "lagopus_fc_%d:%d",
                 pthread_self(), bank);
        params.name = name;
        params.entries = FLOWCACHE_MAX_ENTRIES;
        params.key_len = FLOWCACHE_BITLEN >> 3;
        params.hash_func = HASH_FUNC;
        cache->hash = rte_hash_create(&params);
      }
      break;
#endif /* RTE_VERSION */
#endif /* HAVE_DPDK */

    case FLOWCACHE_HASHMAP:
    case FLOWCACHE_HASHMAP_NOLOCK:
    default:
      lagopus_hashmap_create(&cache->hashmap,
                             LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                             fini_cache_list);
      break;
  }
  DPRINTF("%s: cache %p created\n", __func__, cache);
  return cache;
}

static void
register_cache_bank(struct flowcache_bank *cache,
                    uint64_t hash64,
                    unsigned nmatched,
                    const struct flow **flow) {
  struct cache_entry *cache_entry, *remove_entry;
  struct cache_list *list;
  uint32_t hash32_h;

  DPRINTF("register cache (nmatched %d) to %p\n", nmatched, cache);
  cache_entry = calloc(1, sizeof(struct cache_entry) +
                       sizeof(struct flow *) * nmatched);
  cache_entry->hash64 = hash64;
  cache_entry->nmatched = nmatched;
  memcpy(cache_entry->flow, flow, nmatched * sizeof(struct flow *));
  hash32_h = cache_entry->hash32_h;

  switch (cache->kvs_type) {
#ifdef HAVE_DPDK
#if RTE_VERSION >= RTE_VERSION_NUM(2, 1, 0, 0)
    case FLOWCACHE_RTE_HASH:
      if (rte_hash_lookup_data(cache->hash,
                               (const void *)&hash32_h,
                               (void **)&list) == 0 &&
          list != NULL) {
        if (list->nentries == CACHE_NODE_MAX_ENTRIES) {
          /* so far, remove old entry */
          remove_entry = TAILQ_FIRST(&list->entries);
          remove_cache_list(list, remove_entry);
          cache->nentries--;
        }
      } else {
        list = init_cache_list();
        rte_hash_add_key_data(cache->hash,
                              (const void *)&hash32_h,
                              list);
      }
      break;
#endif /* RTE_VERSION */
#endif /* HAVE_DPDK */

    case FLOWCACHE_HASHMAP:
      if (lagopus_hashmap_find(&cache->hashmap,
                               (void *)(uintptr_t)hash32_h,
                               (void **)&list) == LAGOPUS_RESULT_OK &&
          list != NULL) {
        if (list->nentries == CACHE_NODE_MAX_ENTRIES) {
          /* so far, remove old entry */
          remove_entry = TAILQ_FIRST(&list->entries);
          remove_cache_list(list, remove_entry);
          cache->nentries--;
        }
      } else {
        void *value;

        list = init_cache_list();
        value = list;
        lagopus_hashmap_add(&cache->hashmap,
                            (void *)(uintptr_t)hash32_h,
                            (void **)&value,
                            false);
      }
      break;

    case FLOWCACHE_HASHMAP_NOLOCK:
    default:
      if (lagopus_hashmap_find_no_lock(&cache->hashmap,
                                       (void *)(uintptr_t)hash32_h,
                                       (void **)&list) == LAGOPUS_RESULT_OK &&
          list != NULL) {
        if (list->nentries == CACHE_NODE_MAX_ENTRIES) {
          /* so far, remove old entry */
          remove_entry = TAILQ_FIRST(&list->entries);
          remove_cache_list(list, remove_entry);
          cache->nentries--;
        }
      } else {
        void *value;

        list = init_cache_list();
        value = list;
        lagopus_hashmap_add(&cache->hashmap,
                            (void *)(uintptr_t)hash32_h,
                            (void **)&value,
                            false);
      }
      break;
  }
  add_cache_list(list, cache_entry);
  cache->nentries++;
}

static void
clear_all_cache_bank(struct flowcache_bank *cache) {
  if (cache->nentries == 0) {
    /* no entries.  nothing to do. */
    return;
  }
  switch (cache->kvs_type) {
#ifdef HAVE_DPDK
#if RTE_VERSION >= RTE_VERSION_NUM(2, 1, 0, 0)
    case FLOWCACHE_RTE_HASH:
      rte_hash_reset(cache->hash);
      break;
#endif /* RTE_VERSION */
#endif /* HAVE_DPDK */

    case  FLOWCACHE_HASHMAP:
      lagopus_hashmap_clear(&cache->hashmap, true);
      break;

    case  FLOWCACHE_HASHMAP_NOLOCK:
    default:
      lagopus_hashmap_clear_no_lock(&cache->hashmap, true);
      break;
  }
  cache->nentries = 0;
  cache->hit = 0;
  cache->miss = 0;
}

static struct cache_entry *
cache_lookup_bank(struct flowcache_bank *cache, struct lagopus_packet *pkt) {
  struct cache_entry *cache_entry;
  struct cache_list *list;

  if (unlikely(cache == NULL)) {
    DPRINTF("cache_lookup: cache disabled\n");
    return NULL;
  }
  DPRINTF("cache_lookup (hit %lu, miss %lu)\n", cache->hit, cache->miss);
  switch (cache->kvs_type) {
#ifdef HAVE_DPDK
#if RTE_VERSION >= RTE_VERSION_NUM(2, 1, 0, 0)
    case FLOWCACHE_RTE_HASH:
      if (rte_hash_lookup_data(cache->hash,
                               (void *)&pkt->hash32_h,
                               (void **)&list) < 0) {
        list = NULL;
      }
      break;
#endif /* RTE_VERSION */
#endif /* HAVE_DPDK */

    case FLOWCACHE_HASHMAP:
      if (lagopus_hashmap_find(&cache->hashmap,
                               (void *)(uintptr_t)pkt->hash32_h,
                               (void **)&list) != LAGOPUS_RESULT_OK) {
        list = NULL;
      }
      break;

    case FLOWCACHE_HASHMAP_NOLOCK:
    default:
      if (lagopus_hashmap_find_no_lock(&cache->hashmap,
                                       (void *)(uintptr_t)pkt->hash32_h,
                                       (void **)&list) != LAGOPUS_RESULT_OK) {
        list = NULL;
      }
      break;
  }
  if (likely(list != NULL)) {
    TAILQ_FOREACH(cache_entry, &list->entries, next) {
      if (pkt->hash32_l == cache_entry->hash32_l) {
        cache->hit++;
        return cache_entry;
      }
    }
  }
  cache->miss++;
  return NULL;
}

static void
fini_flowcache_bank(struct flowcache_bank *cache) {
  clear_all_cache_bank(cache);

  switch (cache->kvs_type) {
#ifdef HAVE_DPDK
#if RTE_VERSION >= RTE_VERSION_NUM(2, 1, 0, 0)
    case FLOWCACHE_RTE_HASH:
      {
        const void *key;
        void *data;
        uint32_t next;

        next = 0;
        while (rte_hash_iterate(cache->hash, &key, &data, &next) >= 0) {
          fini_cache_list(data);
        }
        rte_hash_free(cache->hash);
      }
      break;
#endif /* RTE_VERSION */
#endif /* HAVE_DPDK */

    case  FLOWCACHE_HASHMAP:
    case  FLOWCACHE_HASHMAP_NOLOCK:
      lagopus_hashmap_destroy(&cache->hashmap, false);
      break;
    default:
      break;
  }
  free(cache);
}

struct flowcache *
init_flowcache(int kvs_type) {
  struct flowcache *cache;
  int bank;

  cache = calloc(1, sizeof(struct flowcache));
  if (cache == NULL) {
    return NULL;
  }
  for (bank = 0; bank < NBANK; bank++) {
    cache->bank[bank] = init_flowcache_bank(kvs_type, bank);
    if (cache->bank[bank] == NULL) {
      free(cache);
      return NULL;
    }
  }
  cache->max_entries = FLOWCACHE_MAX_ENTRIES;
  return cache;
}

void
register_cache(struct flowcache *cache,
               uint64_t hash64,
               unsigned nmatched,
               const struct flow **flow) {
  struct flowcache_bank *bank, *alt_bank;

  bank = cache->bank[0];
  register_cache_bank(bank, hash64, nmatched, flow);
  if (bank->nentries >= cache->max_entries / 2) {
    alt_bank = init_flowcache_bank(bank->kvs_type, cache->bank[1]->bank + 1);
    alt_bank->hit = cache->bank[1]->hit;
    alt_bank->miss = cache->bank[1]->miss;
    fini_flowcache_bank(cache->bank[1]);
    cache->bank[0] = alt_bank;
    cache->bank[1] = bank;
  }
}

void
clear_all_cache(struct flowcache *cache) {
  int bank;

  if (cache == NULL) {
    /* flowcache is not running, nothing to do. */
    return;
  }
  for (bank = 0; bank < NBANK; bank++) {
    clear_all_cache_bank(cache->bank[bank]);
  }
}

struct cache_entry *
cache_lookup(struct flowcache *cache, struct lagopus_packet *pkt) {
  struct cache_entry *rv;

  if (cache == NULL) {
    return NULL;
  }
  rv = cache_lookup_bank(cache->bank[0], pkt);
  if (rv == NULL) {
    rv = cache_lookup_bank(cache->bank[1], pkt);
  }
  return rv;
}

void
get_flowcache_statistics(struct flowcache *cache, struct ofcachestat *st) {
  struct flowcache_bank *bank;
  int i;

  st->nentries = 0;
  st->hit = 0;
  st->miss = 0;
  for (i = 0; i < NBANK; i++) {
    bank = cache->bank[i];
    st->nentries += bank->nentries;
    st->hit += bank->hit;
    st->miss += bank->miss;
  }
}

void
fini_flowcache(struct flowcache *cache) {
  int bank;

  for (bank = 0; bank < NBANK; bank++) {
    fini_flowcache_bank(cache->bank[bank]);
  }
  free(cache);
}
