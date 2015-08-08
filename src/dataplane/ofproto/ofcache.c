/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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

#include <sys/queue.h>
#include <stdlib.h>
#include <inttypes.h>

#include "lagopus_apis.h"
#include "lagopus/ptree.h"
#include "lagopus/flowdb.h"

#include "pktbuf.h"
#include "packet.h"

#define FLOWCACHE_BITLEN 32

#undef CACHE_DEBUG
#ifdef CACHE_DEBUG
#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...)
#endif

TAILQ_HEAD(cache_entry_list, cache_entry);

struct cache_list {
  int nentries;
  struct cache_entry_list entries;
};

struct flowcache *
init_flowcache(int kvs_type) {
  struct flowcache *cache;

  cache = calloc(1, sizeof(struct flowcache));
  if (cache == NULL) {
    return NULL;
  }
  cache->kvs_type = kvs_type;
  switch (cache->kvs_type) {
    case FLOWCACHE_PTREE:
      cache->ptree = ptree_init(FLOWCACHE_BITLEN);
      break;
    case FLOWCACHE_HASHMAP:
    case FLOWCACHE_HASHMAP_NOLOCK:
    default:
      lagopus_hashmap_create(&cache->hashmap,
                             LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                             free);
      break;
  }
  DPRINTF("%s: cache %p created\n", __func__, cache);
  return cache;
}

void
register_cache(struct flowcache *cache,
               uint64_t hash64,
               unsigned nmatched,
               const struct flow **flow) {
  struct cache_entry *cache_entry, *remove_entry;
  struct cache_list *list;
  uint32_t hash32_h;
  struct ptree_node *node;

  DPRINTF("register cache (nmatched %d) to %p\n", nmatched, cache);
  cache_entry = calloc(1, sizeof(struct cache_entry) +
                       sizeof(struct flow *) * nmatched);
  cache_entry->hash64 = hash64;
  cache_entry->nmatched = nmatched;
  memcpy(cache_entry->flow, flow, nmatched * sizeof(struct flow *));
  hash32_h = cache_entry->hash32_h;

  switch (cache->kvs_type) {
    case FLOWCACHE_PTREE:
      node = ptree_node_get(cache->ptree,
                            (uint8_t *)&hash32_h,
                            FLOWCACHE_BITLEN);
      if (node == NULL) {
        DPRINTF("node_get failed\n");
        free(cache_entry);
        return;
      }
      if (node->info == NULL) {
        list = calloc(1, sizeof(struct cache_list));
        node->info = list;
        list->nentries = 0;
        TAILQ_INIT(&list->entries);
      } else {
        list = node->info;
        if (list->nentries == CACHE_NODE_MAX_ENTRIES) {
          /* so far, remove old entry */
          remove_entry = TAILQ_FIRST(&list->entries);
          TAILQ_REMOVE(&list->entries, remove_entry, next);
          free(remove_entry);
          list->nentries--;
        }
      }
      break;
    case FLOWCACHE_HASHMAP:
      if (lagopus_hashmap_find(&cache->hashmap,
                               (void *)(uintptr_t)hash32_h,
                               (void **)&list) == LAGOPUS_RESULT_OK &&
          list != NULL) {
        if (list->nentries == CACHE_NODE_MAX_ENTRIES) {
          /* so far, remove old entry */
          remove_entry = TAILQ_FIRST(&list->entries);
          TAILQ_REMOVE(&list->entries, remove_entry, next);
          free(remove_entry);
          list->nentries--;
        }
      } else {
        list = calloc(1, sizeof(struct cache_list));
        list->nentries = 0;
        TAILQ_INIT(&list->entries);
        lagopus_hashmap_add(&cache->hashmap,
                            (void *)(uintptr_t)hash32_h,
                            (void **)&list,
                            false);
        lagopus_hashmap_find(&cache->hashmap,
                             (void *)(uintptr_t)hash32_h,
                             (void **)&list);
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
          TAILQ_REMOVE(&list->entries, remove_entry, next);
          free(remove_entry);
          list->nentries--;
        }
      } else {
        list = calloc(1, sizeof(struct cache_list));
        list->nentries = 0;
        TAILQ_INIT(&list->entries);
        lagopus_hashmap_add(&cache->hashmap,
                            (void *)(uintptr_t)hash32_h,
                            (void **)&list,
                            false);
        lagopus_hashmap_find_no_lock(&cache->hashmap,
                                     (void *)(uintptr_t)hash32_h,
                                     (void **)&list);
      }
      break;
  }
  TAILQ_INSERT_TAIL(&list->entries, cache_entry, next);
  list->nentries++;
  cache->nentries++;
}

void
clear_all_cache(struct flowcache *cache) {
  struct cache_entry *cache_entry;
  struct ptree_node *node;

  switch (cache->kvs_type) {
    case FLOWCACHE_PTREE:
      node = ptree_top(cache->ptree);
      while (node != NULL) {
        if (node->info != NULL) {
          struct cache_list *list;

          list = node->info;
          if (list != NULL) {
            while ((cache_entry = TAILQ_FIRST(&list->entries)) != NULL) {
              TAILQ_REMOVE(&list->entries, cache_entry, next);
              free(cache_entry);
            }
            free(list);
            node->info = NULL;
          }
        }
        node = ptree_next(node);
      }
      break;
    case  FLOWCACHE_HASHMAP:
    case  FLOWCACHE_HASHMAP_NOLOCK:
    default:
      lagopus_hashmap_clear(&cache->hashmap, true);
      break;
  }
  cache->nentries = 0;
  cache->hit = 0;
  cache->miss = 0;
}

struct cache_entry *
cache_lookup(struct flowcache *cache, struct lagopus_packet *pkt) {
  struct cache_entry *cache_entry;
  struct cache_list *list;
  struct ptree_node *node;

  if (unlikely(cache == NULL)) {
    DPRINTF("cache_lookup: cache disabled\n");
    return NULL;
  }
  DPRINTF("cache_lookup (hit %lu, miss %lu)\n", cache->hit, cache->miss);
  switch (cache->kvs_type) {
    case FLOWCACHE_PTREE:
      node = ptree_node_lookup(cache->ptree, (uint8_t *)&pkt->hash32_h,
                               FLOWCACHE_BITLEN);
      if (node != NULL && node->info != NULL) {
        list = node->info;
      } else {
        list = NULL;
      }
      break;
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

void
fini_flowcache(struct flowcache *cache) {
  clear_all_cache(cache);

  switch (cache->kvs_type) {
    case  FLOWCACHE_PTREE:
      ptree_free(cache->ptree);
      break;
    case  FLOWCACHE_HASHMAP:
    case  FLOWCACHE_HASHMAP_NOLOCK:
      lagopus_hashmap_destroy(&cache->hashmap, false);
      break;
    default:
      break;
  }
  free(cache);
}
