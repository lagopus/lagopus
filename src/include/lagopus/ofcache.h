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

#ifndef SRC_INCLUDE_LAGOPUS_OFCACHE_H_
#define SRC_INCLUDE_LAGOPUS_OFCACHE_H_

#define HASH_TYPE_CITY64   0
#define HASH_TYPE_INTEL64  1
#define HASH_TYPE_MURMUR3  2

#define FLOWCACHE_HASHMAP_NOLOCK 0
#define FLOWCACHE_HASHMAP        1
#define FLOWCACHE_PTREE          2
#define FLOWCACHE_RTE_HASH       3

#define CACHE_NODE_MAX_ENTRIES 256

struct lagopus_packet;
struct rte_hash;
struct flowcache;

/**
 * @brief Flow cache statistics.
 */
struct ofcachestat {
  uint64_t nentries;                    /** number of cache entry */
  uint64_t hit;                         /** cache hit count */
  uint64_t miss;                        /** cache miss count */
};

/**
 * @brief Flow cache entry.
 */
struct cache_entry {
  TAILQ_ENTRY(cache_entry) next;        /** link for next entry */
  union {
    uint64_t hash64;                    /** 64bit hash value */
    struct {
      uint32_t hash32_h;                /** higher 32bit of hash value */
      uint32_t hash32_l;                /** lower 32bit of hash value */
    };
  };
  unsigned nmatched;                    /** number of flow. */
  struct flow *flow[0];                 /** flow entries. */
};

/**
 * Initialize cache.
 *
 * @param[in]   kvs_type        Key Value Store type.
 *
 * @retval      !=NULL          flow cache object.
 *              ==NULL          kvs_type is invalid, or memory exhausted.
 *
 * kvs_type is one of below:
 *      FLOWCACHE_HASHMAP_NOLOCK
 *      FLOWCACHE_HASHMAP
 *      FLOWCACHE_PTREE
 */
struct flowcache *init_flowcache(int kvs_type);

/**
 * Register cache entry.
 *
 * @param[in]   cache           Flow cache object.
 * @param[in]   hash64          Hash value for lookup cache entry.
 * @param[in]   nmatched        Number of flows.
 * @param[in]   flow            Flow entries.
 */
void
register_cache(struct flowcache *cache,
               uint64_t hash64,
               unsigned nmatched,
               const struct flow **flow);

/**
 * Clear all cache entry.
 *
 * @param[in]   cache           Flow cache object.
 */
void clear_all_cache(struct flowcache *cache);

/**
 * Lookup cache.
 *
 * @param[in]   cache   Flow cache object.
 * @param[in]   pkt     Packet.
 *
 * @retval      !=NULL  Cache entry.
 * @retval      ==NULL  does not exist in cache.
 */
struct cache_entry *
cache_lookup(struct flowcache *cache, struct lagopus_packet *pkt);

/**
 * Finalize cache.
 *
 * @param[in]   cache   Flow cache object.
 */
void
fini_flowcache(struct flowcache *cache);

struct bridge;
/**
 * Get flow cache statistics.
 *
 * @param[in]   cache    Flow cache object.
 * @param[out]  st       Statistics of flow cache.
 */
void
get_flowcache_statistics(struct flowcache *cache, struct ofcachestat *st);

#endif /* SRC_INCLUDE_LAGOPUS_FLOWCACHE_H_ */
