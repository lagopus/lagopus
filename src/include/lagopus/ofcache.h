/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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


#ifndef SRC_INCLUDE_LAGOPUS_OFCACHE_H_
#define SRC_INCLUDE_LAGOPUS_OFCACHE_H_

#define HASH_TYPE_CITY64   0
#define HASH_TYPE_INTEL64  1
#define HASH_TYPE_MURMUR3  2

#define FLOWCACHE_HASHMAP_NOLOCK 0
#define FLOWCACHE_HASHMAP        1
#define FLOWCACHE_PTREE          2

#define CACHE_NODE_MAX_ENTRIES 256

struct lagopus_packet;

/**
 * per thread cache object.
 */
struct flowcache {
  int kvs_type;
  struct ptree *ptree;
  lagopus_hashmap_t hashmap;
  /* statistics */
  uint64_t nentries;
  uint64_t hit;
  uint64_t miss;
};

struct ofcachestat {
  /* statistics */
  uint64_t nentries;
  uint64_t hit;
  uint64_t miss;
};

struct cache_entry {
  TAILQ_ENTRY(cache_entry) next;
  union {
    uint64_t hash64;
    struct {
      uint32_t hash32_h;
      uint32_t hash32_l;
    };
  };
  unsigned nmatched;
  struct flow *flow[0];
};

/**
 * Initialie cache.
 */
struct flowcache *init_flowcache(int kvs_type);

/**
 * Register cache entry.
 */
void
register_cache(struct flowcache *cache,
               uint64_t hash64,
               unsigned nmatched,
               const struct flow **flow);

/**
 * Clear all cache entry.
 */
void clear_all_cache(struct flowcache *cache);

/**
 * Lookup cache.
 *
 * @param[in]	cache	Cache.
 * @param[in]	pkt	Packet.
 *
 * @retval	!=NULL	Cache entry.
 * @retval	==NULL	does not exist in cache.
 */
struct cache_entry *
cache_lookup(struct flowcache *cache, struct lagopus_packet *pkt);

/**
 * Finalize cache.
 *
 * @param[in]	cache	Cache.
 */
void
fini_flowcache(struct flowcache *cache);

struct bridge;
/**
 * Get flow cache statistics.
 *
 * @param[in]   bridge   Bridge.
 * @param[out]  st       Statistics of flow cache.
 */
void
get_flowcache_statistics(struct bridge *bridge, struct ofcachestat *st);

#endif /* SRC_INCLUDE_LAGOPUS_FLOWCACHE_H_ */
