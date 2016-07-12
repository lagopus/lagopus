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

#include <inttypes.h>
#include <string.h>
#include <sys/queue.h>

#include <lagopus_apis.h>
#include <lagopus/dp_apis.h>
#include <lagopus/flowdb.h>

#include "City.h"
#include "pktbuf.h"
#include "packet.h"

#include "thtable.h"

#define DEBUG 0
#if DEBUG > 0
#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...)
#endif /* DEBUG */
#if DEBUG > 1
#define DDPRINTF(...) printf(__VA_ARGS__)
#else
#define DDPRINTF(...)
#endif /* DEBUG */
#if DEBUG > 2
#define DDDPRINTF(...) printf(__VA_ARGS__)
#else
#define DDDPRINTF(...)
#endif /* DEBUG */

#define OXM_FIELD_TYPE(field) ((field) >> 1)
#define OXM_FIELD_HAS_MASK(field) (((field) & 1) != 0)
#define OXM_MATCH_VALUE_LEN(match) \
  ((match)->oxm_length >> ((match)->oxm_field & 1))

struct htable {
  struct match_list match_list; /* for flow */
  struct match_list mask_list; /* for packet */
  lagopus_hashmap_t hashmap;
#if DEBUG > 1
  int nflow;
#endif /* DEBUG */
};

struct htlist {
  uint16_t priority;
  int ntable;
  struct htable *tables[8]; /* preliminary */
};

#define calc_hash CityHash64WithSeed

static struct htable *
htable_alloc(void) {
  struct htable *htable;

  htable = calloc(1, sizeof(struct htable));
  if (htable != NULL) {
    lagopus_hashmap_create(&htable->hashmap, LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                           NULL);
    TAILQ_INIT(&htable->match_list);
    TAILQ_INIT(&htable->mask_list);
  }
  return htable;
}

static void
htable_free(struct htable *htable) {
  lagopus_hashmap_destroy(&htable->hashmap, false);
  match_list_entry_free(&htable->match_list);
  match_list_entry_free(&htable->mask_list);
  free(htable);
}

static struct htlist *
htlist_alloc(uint16_t priority) {
  struct htlist *htlist;

  htlist = calloc(1, sizeof(struct htlist));
  if (htlist != NULL) {
    htlist->priority = priority;
  }
  return htlist;
}

static void
htlist_free(struct htlist *htlist) {
  int i;

  for (i = 0; i < htlist->ntable; i++) {
    htable_free(htlist->tables[i]);
  }
  free(htlist);
}

thtable_t
thtable_alloc(void) {
  thtable_t thtable;

  thtable = calloc(THTABLE_INDEX_MAX + 1, sizeof(struct htlist *));
  return thtable;
}

void
thtable_free(thtable_t thtable) {
  int i;

  if (thtable != NULL) {
    for (i = 0; i <= THTABLE_INDEX_MAX; i++) {
      struct htlist *htlist;

      htlist = thtable[i];
      if (htlist != NULL) {
        htlist_free(htlist);
      }
    }
    free(thtable);
  }
}

static void
thtable_add_htlist(thtable_t thtable, struct htlist *htlist) {
  thtable[htlist->priority] = htlist;
}

static struct htlist *
thtable_get_htlist(thtable_t thtable, int priority) {
  return thtable[priority];
}

static int
thtable_cmp(void *a, void *b) {
  struct htlist *ta, *tb;

  ta = *(struct htlist **)a;
  tb = *(struct htlist **)b;
  if (ta == NULL) {
    return 1;
  }
  if (tb == NULL) {
    return -1;
  }
  return tb->priority - ta->priority;
}

static void
thtable_sort(thtable_t thtable) {
  qsort(thtable, THTABLE_INDEX_MAX + 1, sizeof(struct htlist *), thtable_cmp);
}

struct match_idx {
  int base;
  int off;
  int size;
  uint8_t mask[16];
  int shift;
};

/*
 * pkt value: &base[idx])[off], sizeof(member), masked, right shifted
 * match value: oxm_value, OXM_MATCH_VALUE_LEN
 */
#define MAKE_IDX(base, type, member)        \
  (base), offsetof(struct type, member), sizeof(((struct type *)0)->member)

/*
 * 1. classify packet.
 * 2. pick pkt->base[n] + offset, N bytes
 * 3. mask
 * 4. compare with match->oxm_value, N bytes
 */
const struct match_idx match_idx[]  = {
  { MAKE_IDX(OOB_BASE, oob_data, in_port), {0xff,0xff,0xff,0xff}, 0 },
  { MAKE_IDX(OOB_BASE, oob_data, in_phy_port), {0xff,0xff,0xff,0xff}, 0 },
  { MAKE_IDX(OOB_BASE, oob_data, metadata), {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
  { MAKE_IDX(ETH_BASE, ether_header, ether_dhost), {0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
  { MAKE_IDX(ETH_BASE, ether_header, ether_shost), {0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
  { MAKE_IDX(OOB_BASE, oob_data, ether_type), {0xff,0xff}, 0 },
  { MAKE_IDX(OOB_BASE, oob_data, vlan_tci), {0x1f,0xff}, 0 },
  { MAKE_IDX(OOB_BASE, oob_data, vlan_tci), {0xe0,00}, 13 },
  { MAKE_IDX(L3_BASE, ip, ip_tos), {0xfc}, 3 },
  { MAKE_IDX(L3_BASE, ip, ip_tos), {0x03}, 0 },
  { IPPROTO_BASE, 0, 1, {0xff}, 0 },
  { MAKE_IDX(L3_BASE, ip, ip_src), {0xff,0xff,0xff,0xff}, 0 },
  { MAKE_IDX(L3_BASE, ip, ip_dst), {0xff,0xff,0xff,0xff}, 0 },
  { MAKE_IDX(L4_BASE, tcphdr, th_sport), {0xff,0xff}, 0 },
  { MAKE_IDX(L4_BASE, tcphdr, th_dport), {0xff,0xff}, 0 },
  { MAKE_IDX(L4_BASE, udphdr, uh_sport), {0xff,0xff}, 0 },
  { MAKE_IDX(L4_BASE, udphdr, uh_dport), {0xff,0xff}, 0 },
  { MAKE_IDX(L4_BASE, tcphdr, th_sport), {0xff,0xff}, 0 },
  { MAKE_IDX(L4_BASE, tcphdr, th_dport), {0xff,0xff}, 0 },
  { MAKE_IDX(L4_BASE, icmp, icmp_type), {0xff}, 0 },
  { MAKE_IDX(L4_BASE, icmp, icmp_code), {0xff}, 0 },
  { MAKE_IDX(L3_BASE, ether_arp, arp_op), {0xff,0xff}, 0 },
  { MAKE_IDX(L3_BASE, ether_arp, arp_spa), {0xff,0xff,0xff,0xff}, 0 },
  { MAKE_IDX(L3_BASE, ether_arp, arp_tpa), {0xff,0xff,0xff,0xff}, 0 },
  { MAKE_IDX(L3_BASE, ether_arp, arp_sha), {0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
  { MAKE_IDX(L3_BASE, ether_arp, arp_tha), {0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
  { MAKE_IDX(L3_BASE, ip6_hdr, ip6_src), {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
  { MAKE_IDX(L3_BASE, ip6_hdr, ip6_dst), {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
  { MAKE_IDX(L3_BASE, ip6_hdr, ip6_flow), {0x00,0x0f,0xff,0xff}, 0 },
  { MAKE_IDX(L4_BASE, icmp6_hdr, icmp6_type), {0xff}, 0 },
  { MAKE_IDX(L4_BASE, icmp6_hdr, icmp6_code), {0xff}, 0 },
  { L4_BASE, 0, 0, 0, 0},
  { NDSLL_BASE, 0, 2, {0xff,0xff}, 0 },
  { NDTLL_BASE, 0, 2, {0xff,0xff}, 0 },
  { MPLS_BASE, 0, 3, {0xff,0xff,0xf0}, 12 },
  { MPLS_BASE, 2, 1, {0x01}, 0 },
  { MPLS_BASE, 3, 1, {0xff}, 0 },
  { MAKE_IDX(PBB_BASE, pbb_hdr, i_sid), {0x00,0xff,0xff,0xff}, 0 },
  { MAKE_IDX(OOB2_BASE, oob2_data, tunnel_id), {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
  { MAKE_IDX(OOB2_BASE, oob2_data, ipv6_exthdr), {0xff,0xff}, 0 }
};

static struct match *
copy_masked_match(const struct match *src_match) {
  struct match *dst_match;
  struct match_idx *idx;
  size_t len;
  uint64_t val64;
  uint32_t val32;
  uint16_t val16;

  len = OXM_MATCH_VALUE_LEN(src_match);
  dst_match = calloc(1, sizeof(struct match) + len * 2);
  if (dst_match == NULL) {
    return NULL;
  }
  memcpy(dst_match, src_match, sizeof(struct match));
  idx = &match_idx[OXM_FIELD_TYPE(src_match->oxm_field)];
  if (idx->size == len && idx->shift == 0) {
    memcpy(dst_match->oxm_value, src_match->oxm_value, len);
  } else {
    switch (len) {
      case 8:
        memcpy(&val64, src_match->oxm_value, sizeof(val64));
        val64 = OS_NTOHLL(val64);
        val64 <<= idx->shift;
        val64 = OS_HTONLL(val64);
        memcpy(dst_match->oxm_value, &val64, sizeof(val64));
        break;
      case 4:
        memcpy(&val32, src_match->oxm_value, sizeof(val32));
        val32 = OS_NTOHL(val32);
        val32 <<= idx->shift;
        val32 = OS_HTONL(val32);
        memcpy(dst_match->oxm_value, &val32, sizeof(val32));
        break;
      case 2:
        memcpy(&val16, src_match->oxm_value, sizeof(val16));
        val16 = OS_NTOHS(val16);
        val16 <<= idx->shift;
        val16 = OS_HTONS(val16);
        memcpy(dst_match->oxm_value, &val16, sizeof(val16));
        break;
      case 1:
        *dst_match->oxm_value = (*src_match->oxm_value) << idx->shift;
        break;
    }
  }
  if (!OXM_FIELD_HAS_MASK(src_match->oxm_field)) {
    memcpy(&dst_match->oxm_value[idx->size], idx->mask, idx->size);
  } else if (idx->shift == 0) {
    memcpy(&dst_match->oxm_value[idx->size],
           &src_match->oxm_value[len + (len - idx->size)],
           idx->size);
  } else {
    switch (len) {
      case 8:
        memcpy(&val64, &src_match->oxm_value[len], sizeof(val64));
        val64 = OS_NTOHLL(val64);
        val64 <<= idx->shift;
        val64 = OS_HTONLL(val64);
        memcpy(&dst_match->oxm_value[idx->size], &val64, sizeof(val64));
        break;
      case 4:
        memcpy(&val32, &src_match->oxm_value[len], sizeof(val32));
        val32 = OS_NTOHL(val32);
        val32 <<= idx->shift;
        val32 = OS_HTONL(val32);
        memcpy(&dst_match->oxm_value[idx->size], &val32, sizeof(val32));
        break;
      case 2:
        memcpy(&val16, &src_match->oxm_value[len], sizeof(val16));
        val16 = OS_NTOHS(val16);
        val16 <<= idx->shift;
        val16 = OS_HTONS(val16);
        memcpy(&dst_match->oxm_value[idx->size], &val16, sizeof(val16));
        break;
      case 1:
        dst_match->oxm_value[idx->size] =
            src_match->oxm_value[len] << idx->shift;
        break;
    }
  }
  dst_match->oxm_length = idx->size;
  return dst_match;
}

static lagopus_result_t
copy_masked_match_list(struct match_list *dst,
                       const struct match_list *src) {
  struct match *src_match;
  struct match *dst_match;

  TAILQ_FOREACH(src_match, src, entry) {
    dst_match = copy_masked_match(src_match);
    DPRINTF(" %d", dst_match->oxm_field);
    TAILQ_INSERT_TAIL(dst, dst_match, entry);
  }
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
htlist_add_flow(struct flow *flow, struct htlist *htlist) {
  struct match *match, *tmatch;
  struct htable *hash_table;
  size_t len;
  uint64_t key;
  int i, count, tcount;

  key = 0;
  count = 0;
  TAILQ_FOREACH(match, &flow->match_list, entry) {
    struct match *pmatch;

    pmatch = copy_masked_match(match);
    key = calc_hash(pmatch->oxm_value, pmatch->oxm_length, key);
    free(pmatch);
    count++;
  }
  hash_table = NULL;
  for (i = 0; i < htlist->ntable; i++) {
    hash_table = htlist->tables[i];
    if (hash_table == NULL) {
      continue;
    }
    tcount = 0;
    TAILQ_FOREACH(tmatch, &hash_table->match_list, entry) {
      tcount++;
      TAILQ_FOREACH(match, &flow->match_list, entry) {
        if (match->oxm_field == tmatch->oxm_field) {
          len = OXM_MATCH_VALUE_LEN(match);
          if (!OXM_FIELD_HAS_MASK(match->oxm_field) ||
              !memcmp(match->oxm_value + len,
                      tmatch->oxm_value + len,
                      len)) {
            break;
          }
        }
      }
      if (match == NULL) {
        /* flow match is not found in hash table, top loop */
        break;
      }
    }
    if (tmatch == NULL && count == tcount) {
      /* hash table matched */
      break;
    }
    hash_table = NULL;
  }
  if (hash_table == NULL) {
    char *p;

    /* same match hash table does not exist.  create. */
    if (htlist->ntable >= sizeof(htlist->tables) / sizeof(htlist->tables[0])) {
      DPRINTF("warning: tables is over\n");
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    DPRINTF("alloc htlist->tebles[%d] match:", htlist->ntable);
    hash_table = htable_alloc();
    copy_match_list(&hash_table->match_list, &flow->match_list);
    copy_masked_match_list(&hash_table->mask_list, &flow->match_list);
    htlist->tables[htlist->ntable++] = hash_table;
    DPRINTF("\n");
  }
#if DEBUG > 1
  hash_table->nflow++;
#endif /* DEBUG */
  DDPRINTF("registered key %d flow %p\n", key, flow);
  return lagopus_hashmap_add(&hash_table->hashmap, key, &flow, false);
}

lagopus_result_t
thtable_add_flow(struct flow *flow, thtable_t thtable) {
  struct htlist *htlist;

  htlist = thtable_get_htlist(thtable, flow->priority);
  if (htlist == NULL) {
    htlist = htlist_alloc(flow->priority);
    thtable_add_htlist(thtable, htlist);
  }
  return htlist_add_flow(flow, htlist);
}

void
thtable_update(struct flow_list *flow_list) {
  thtable_t old_table, new_table;
  int i;

  new_table = thtable_alloc();
  for (i = 0; i < flow_list->nflow; i++) {
    thtable_add_flow(flow_list->flows[i], new_table);
  }
  thtable_sort(new_table);
  old_table = flow_list->thtable;
  flow_list->thtable = new_table;
  if (old_table != NULL) {
    thtable_free(old_table);
  }
}

static inline uint64_t
key_from_packet(struct lagopus_packet *pkt,
                struct match_list *mask_list) {
  uint8_t buf[16];
  struct match *match;
  struct match_idx *idx;
  size_t len;
  uint64_t key;
  int i;

  key = 0;
  DDPRINTF("match:");
  TAILQ_FOREACH(match, mask_list, entry) {
    idx = &match_idx[OXM_FIELD_TYPE(match->oxm_field)];
    len = idx->size;
    DDDPRINTF("idx->base %d\n", idx->base);
    DDDPRINTF("idx->off %d\n", idx->off);
    DDDPRINTF("idx->size %d\n", idx->size);
    for (i = 0; i < idx->size; i++) {
      DDDPRINTF("idx->mask[%d] %d\n", i, idx->mask[i]);
    }
    DDDPRINTF("idx->shift %d\n", idx->shift);
    if (pkt->base[idx->base] == NULL) {
      DPRINTF(" unmatched\n");
      return 0;
    }
    DDPRINTF(" %d", match->oxm_field);
    for (i = 0; i < len; i++) {
      buf[i] = *(pkt->base[idx->base] + idx->off + i);
      buf[i] &= match->oxm_value[len + i];
    }
    key = calc_hash(buf, len, key);
  }
  DDPRINTF("\n");
  return key;
}

struct flow *
thtable_match(struct lagopus_packet *pkt, thtable_t thtable) {
  struct htlist *htlist;
  struct flow *flow;
  int i, j;
  lagopus_result_t rv;

  for (i = 0; i <= THTABLE_INDEX_MAX; i++) {
    htlist = thtable[i];
    if (htlist == NULL) {
      /* unmatched. */
      break;
    }
    DDPRINTF("htlist->ntable %d\n", htlist->ntable);
    for (j = 0; j < htlist->ntable; j++) {
      uint64_t key;

      key = key_from_packet(pkt, &htlist->tables[j]->mask_list);
      if (key == 0) {
        DDPRINTF("htlist->tables[%d]: not match list\n", j);
        continue;
      }
      rv = lagopus_hashmap_find_no_lock(
          &htlist->tables[j]->hashmap,
          key,
          (void **)&flow);
      if (rv == LAGOPUS_RESULT_OK) {
        return flow;
      }
      DDPRINTF("htlist->tables[%d]: (nflow:%d) key %d not found\n",
              j, htlist->tables[j]->nflow, key);
    }
  }
  return NULL;
}
