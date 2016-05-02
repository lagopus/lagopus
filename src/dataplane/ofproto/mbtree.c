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
 *      @file   mbtree.c
 *      @brief  multiple branch tree for Openflow
 */

#include "lagopus_config.h"

#include <inttypes.h>
#include <stddef.h>
#include <sys/types.h>

#include <sys/queue.h>

#ifndef HAVE_DPDK
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#else
#include <net/if.h>
#include <net/if_ether.h>
#endif /* HAVE_NET_ETHERNET_H */
#endif /* HAVE_DPDK */

#define __FAVOR_BSD
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>

#include "lagopus/ethertype.h"
#include "lagopus/flowdb.h"
#include "lagopus/flowinfo.h"
#include "pktbuf.h"
#include "packet.h"
#include "mbtree.h"

#include <netinet/if_ether.h>

struct match_idx {
  int base;
  int off;
  int size;
  uint64_t mask;
  int shift;
};

enum {
  SEQUENCIAL,
  HASHMAP,
};

#define DPRINTF(...)

#define MAKE_MATCH_IDX(base, type, member,  mask, shift)        \
  { base, offsetof(struct type, member), sizeof(((struct type *)0)->member), mask, shift }
#define OXM_FIELD_TYPE(field) ((field) >> 1)
#define OXM_FIELD_HAS_MASK(field) (((field) & 1) != 0)
#define OXM_MATCH_VALUE_LEN(match) \
  ((match)->oxm_length >> ((match)->oxm_field & 1))

/* index: OFPXMT_OFB_* */
static const struct match_idx match_idx[] = {
  MAKE_MATCH_IDX(OOB_BASE, oob_data, in_port, UINT32_MAX, 0),    /* 0 IN_PORT */
  MAKE_MATCH_IDX(OOB_BASE, oob_data, in_phy_port, UINT32_MAX, 0),/* 1 IN_PHY_PORT */
  MAKE_MATCH_IDX(OOB_BASE, oob_data, metadata, UINT64_MAX, 0),   /* 2 METADATA */
  MAKE_MATCH_IDX(ETH_BASE, ether_header, ether_dhost, 0x0000ffffffffffff, 0),    /* 3 ETH_DST */
  MAKE_MATCH_IDX(ETH_BASE, ether_header, ether_shost, 0x0000ffffffffffff, 0),    /* 4 ETH_SRC */
  MAKE_MATCH_IDX(ETH_BASE, oob_data, ether_type, UINT16_MAX, 0), /* 5 ETH_TYPE */
  MAKE_MATCH_IDX(OOB_BASE, oob_data, vlan_tci, 0x1fff, 0),      /* 6 VLAN_VID w/ VID_PRESENT */
  { OOB_BASE, 0, 0, 0, 0 },     /* 7 VLAN_PCP */
  { L3_BASE, 0, 0, 0, 0 },      /* 8 IP_DSCP */
  { L3_BASE, 0, 0, 0, 0 },      /* 9 IP_ECN */
  MAKE_MATCH_IDX(IPPROTO_BASE, ip, ip_p, UINT8_MAX, 0), /* 10 IP_PROTO */
  MAKE_MATCH_IDX(L3_BASE, ip, ip_src, UINT32_MAX, 0),    /* 11 IPV4_SRC */
  MAKE_MATCH_IDX(L3_BASE, ip, ip_dst, UINT32_MAX, 0),    /* 12 IPV4_DST */
  MAKE_MATCH_IDX(L4_BASE, tcphdr, th_sport, UINT16_MAX, 0),      /* 13 TCP_SRC */
  MAKE_MATCH_IDX(L4_BASE, tcphdr, th_dport, UINT16_MAX, 0),      /* 14 TCP_DST */
  MAKE_MATCH_IDX(L4_BASE, udphdr, uh_sport, UINT16_MAX, 0),      /* 15 UDP_SRC */
  MAKE_MATCH_IDX(L4_BASE, udphdr, uh_dport, UINT16_MAX, 0),      /* 16 UDP_DST */
  MAKE_MATCH_IDX(L4_BASE, tcphdr, th_sport, UINT16_MAX, 0),      /* 17 SCTP_SRC */
  MAKE_MATCH_IDX(L4_BASE, tcphdr, th_dport, UINT16_MAX, 0),      /* 18 SCTP_DST */
  MAKE_MATCH_IDX(L4_BASE, icmp, icmp_type, UINT8_MAX, 0),       /* 19 ICMPV4_TYPE */
  MAKE_MATCH_IDX(L4_BASE, icmp, icmp_code, UINT8_MAX, 0),       /* 20 ICMPV4_CODE */
  MAKE_MATCH_IDX(L3_BASE, ether_arp, arp_op, UINT16_MAX, 0),     /* 21 ARP_OP */
  MAKE_MATCH_IDX(L3_BASE, ether_arp, arp_spa, UINT32_MAX, 0),    /* 22 ARP_SPA */
  MAKE_MATCH_IDX(L3_BASE, ether_arp, arp_tpa, UINT32_MAX, 0),    /* 23 ARP_TPA */
  MAKE_MATCH_IDX(L3_BASE, ether_arp, arp_sha, 0x0000ffffffffffff, 0),    /* 24 ARP_SHA */
  MAKE_MATCH_IDX(L3_BASE, ether_arp, arp_tha, 0x0000ffffffffffff, 0),    /* 25 ARP_THA */
#if 1
  { L3_BASE, 0, 0, 0, 0 },
  { L3_BASE, 0, 0, 0, 0 },
#else
  MAKE_MATCH_IDX(L3_BASE, ip6_hdr, ip6_src, 0, 0),      /* 26 IPV6_SRC */
  MAKE_MATCH_IDX(L3_BASE, ip6_hdr, ip6_dst, 0, 0),      /* 27 IPV6_DST */
#endif /**/
  MAKE_MATCH_IDX(L3_BASE, ip6_hdr, ip6_flow, 0x000fffff, 0),    /* 28 IPV6_FLABEL */
  MAKE_MATCH_IDX(L4_BASE, icmp6_hdr, icmp6_type, UINT8_MAX, 0), /* 29 ICMPV6_TYPE */
  MAKE_MATCH_IDX(L4_BASE, icmp6_hdr, icmp6_code, UINT8_MAX, 0), /* 30 ICMPV6_CODE */
  { L4_BASE, 0, 0, 0, 0},       /* 31 IPV6_ND_TARGET */
  { NDSLL_BASE, 0, 0, 0, 0 },   /* 32 IPV6_ND_SLL */
  { NDTLL_BASE, 0, 0, 0, 0 },   /* 33 IPV6_ND_TLL */
  { MPLS_BASE, 0, 4, 0xfffff000, 12 },   /* 34 MPLS_LABEL */
  { MPLS_BASE, 0, 0, 0, 0 },    /* 35 MPLS_TC */
  { MPLS_BASE, 0, 0, 0, 0 },    /* 36 MPLS_BOS */
  MAKE_MATCH_IDX(PBB_BASE, pbb_hdr, i_sid, 0x00ffffffffffffff, 0),       /* 37 PBB_ISID */
  MAKE_MATCH_IDX(OOB2_BASE, oob2_data, tunnel_id, UINT64_MAX, 0),  /* 38 TUNNEL_ID */
  MAKE_MATCH_IDX(OOB2_BASE, oob2_data, ipv6_exthdr, UINT16_MAX, 0) /* 39 IPV6_EXTHDR */
};

static void
build_mbtree_child(struct flow_list *flow_list, void *arg);
static struct flow *
find_mbtree_child(struct lagopus_packet *pkt, struct flow_list *flows);

static struct match *
get_match_eth_type(struct match_list *match_list, uint16_t *eth_type) {
  struct match *match;

  TAILQ_FOREACH(match, match_list, entry) {
    if (OXM_FIELD_TYPE(match->oxm_field) == OFPXMT_OFB_ETH_TYPE) {
      OS_MEMCPY(eth_type, match->oxm_value, sizeof(*eth_type));
      break;
    }
  }
  return match;
}

struct match_stats {
  struct match match;
  uint8_t valmask[32];
  int shift;
  int count;
  uint64_t min_value;
  uint64_t max_value;
  uint64_t threshold;
};

static void
count_match(struct match *match, struct match_list *match_stats_list) {
  struct match *match_stats;
  uint64_t value, length;
  int i;

  TAILQ_FOREACH(match_stats, match_stats_list, entry) {
    if (match->oxm_field == match_stats->oxm_field) {
      length = match->oxm_length;
      if (OXM_FIELD_HAS_MASK(match->oxm_field)) {
        length >>= 1;
        if (memcmp(&match->oxm_value[length],
                   &match_stats->oxm_value[length],
                   length) != 0) {
          continue;
        }
      }
      /* found exist match_stats */
      if (length <= sizeof(uint64_t)) {
        value = 0;
        for (i = 0; i < length; i++) {
          value <<= 8;
          value |= match->oxm_value[i];
        }
        if (((struct match_stats *)match_stats)->min_value > value) {
          ((struct match_stats *)match_stats)->min_value = value;
        }
        if (((struct match_stats *)match_stats)->max_value < value) {
          ((struct match_stats *)match_stats)->max_value = value;
        }
      }
      ((struct match_stats *)match_stats)->count++;
      return;
    }
  }
  /* new match_stats add to the list */
  match_stats = calloc(1, sizeof(struct match_stats));
  memcpy(match_stats, match, sizeof(struct match) + match->oxm_length);
  ((struct match_stats *)match_stats)->count = 1;
  value = 0;
  if (OXM_MATCH_VALUE_LEN(match) <= sizeof(uint64_t)) {
    for (i = 0; i < OXM_MATCH_VALUE_LEN(match); i++) {
      value <<= 8;
      value |= match->oxm_value[i];
    }
    ((struct match_stats *)match_stats)->min_value = value;
    ((struct match_stats *)match_stats)->max_value = value;
  }
  TAILQ_INSERT_TAIL(match_stats_list, match_stats, entry);
}

static int
count_flow_list_match(struct flow_list *flow_list,
                      struct match_list *match_stats_list) {
  struct flow *flow;
  struct match *match;
  int i, count;

  for (i = 0; i < flow_list->nflow; i++) {
    flow = flow_list->flows[i];
    TAILQ_FOREACH(match, &flow->match_list, entry) {
      if (OXM_FIELD_TYPE(match->oxm_field) == OFPXMT_OFB_ETH_TYPE) {
        continue;
      }
      count_match(match, match_stats_list);
    }
  }
  count = 0;
  TAILQ_FOREACH(match, match_stats_list, entry) {
    count++;
  }
  return count;
}

static int
match_cmp(void *a, void *b) {
  struct match_stats *ma, *mb;

  ma = *(struct match_stats **)a;
  mb = *(struct match_stats **)b;

  return mb->count - ma->count;
}

static struct match_stats **
get_match_stats_array(struct flow_list *flow_list) {
  struct match_list match_stats_list;
  struct match_stats **match_array;
  int nmatch, i;

  TAILQ_INIT(&match_stats_list);
  nmatch = count_flow_list_match(flow_list, &match_stats_list);
  match_array = calloc(nmatch + 1, sizeof(struct match_stats *));
  for (i = 0; i < nmatch; i++) {
    match_array[i] = TAILQ_FIRST(&match_stats_list);
    TAILQ_REMOVE(&match_stats_list, TAILQ_FIRST(&match_stats_list), entry);
  }
  qsort(match_array, nmatch, sizeof(struct match_stats *), match_cmp);
  return match_array;
}

static struct match *
get_match_field(struct match_list *match_list, struct match *match_stats) {
  struct match *match;

  TAILQ_FOREACH(match, match_list, entry) {
    if (match->oxm_field == match_stats->oxm_field) {
      if ((match->oxm_field & 1) == 0) {
        /* field match.  match has no mask. */
        break;
      }
      if (!memcmp(&match->oxm_value[match->oxm_length >> 1],
                  &match_stats->oxm_value[match->oxm_length >> 1],
                  match->oxm_length >> 1)) {
        /* field and mask match. */
        break;
      }
    }
  }
  return match;
}

static inline void
get_mask(uint64_t val64,
         int size,
         uint8_t rv[]) {
  while (size-- > 0) {
    rv[size] = val64 & 0xff;
    val64 >>= 8;
  }
}

static inline void
get_shifted_value(uint8_t origin[],
                  int origin_len,
                  int shift,
                  int size,
                  uint8_t rv[]) {
  uint64_t val64;
  int i;

  val64 = 0;
  for (i = 0; i < origin_len; i++) {
    val64 <<= 8;
    val64 |= origin[i];
  }
  val64 <<= shift;
  while (size-- > 0) {
    rv[size] = val64 & 0xff;
    val64 >>= 8;
  }
}

static void *
get_child_flow_list(struct flow_list *flow_list,
                    struct match *match,
                    struct match_stats *most_match,
                    void *child_array[]) {
  uint8_t key[sizeof(uint64_t)];
  void *child;
  lagopus_result_t rv;
  int i;

  get_shifted_value(match->oxm_value,
                    OXM_MATCH_VALUE_LEN(match),
                    flow_list->shift, flow_list->keylen, key);
  DPRINTF("oxm_value:");
  for (i = 0; i < flow_list->keylen; i++) {
    DPRINTF(" %d", match->oxm_value[i]);
  }
  DPRINTF(" keylen %d, key %d\n", flow_list->keylen, *(void **)key);
  switch (flow_list->type) {
    case HASHMAP:
      if (child_array[0] == NULL) {
        child_array[0] = calloc(1, sizeof(lagopus_hashmap_t));
        lagopus_hashmap_create((void *)&child_array[0],
                               LAGOPUS_HASHMAP_TYPE_ONE_WORD, cleanup_mbtree);
      }
      rv = lagopus_hashmap_find_no_lock((void *)&child_array[0], *(void **)key, &child);
      if (rv != LAGOPUS_RESULT_OK) {
        void *val;

        child = calloc(1, sizeof(struct flow_list) + sizeof(void *));
        val = child;
        lagopus_hashmap_add_no_lock((void *)&child_array[0], *(void **)key, &val, false);
      }
      break;

    default:
      child = NULL;
      break;
  }
  return child;
}

static void
set_flow_list_desc(struct flow_list *flow_list,
                   struct match_stats *match_stats) {
  int idx;

  flow_list->type = HASHMAP;
  idx = OXM_FIELD_TYPE(match_stats->match.oxm_field);
  flow_list->base = match_idx[idx].base;
  flow_list->match_off = match_idx[idx].off;
  flow_list->shift = match_idx[idx].shift;
  flow_list->keylen = match_idx[idx].size;
  get_mask(match_idx[idx].mask, match_idx[idx].size, flow_list->mask);
}

static void
distribute_match_flow(struct flow *flow,
                      struct match *match,
                      struct match_stats *match_stats,
                      struct flow_list *flow_list) {
  struct flow_list *child;

  /* lookup child flow_list */
  child = get_child_flow_list(flow_list,
                              match,
                              match_stats,
                              (void **)flow_list->branch);
  if (child != NULL) {
    /* add flow to flow_list array */
    flow_add_sub(flow, child);
  }
}

static void
distribute_flow_to_child(struct flow *flow,
                         struct match_stats *match_stats,
                         struct flow_list *flow_list) {
  struct match *match;

  match = get_match_field(&flow->match_list, &match_stats->match);
  if (match != NULL) {
    distribute_match_flow(flow, match, match_stats, flow_list);
  } else {
    /* not found. don't care case */
    flow_add_sub(flow, flow_list->flows_dontcare);
  }
}

static void
build_mbtree_sequencial(struct flow_list *flow_list) {
  struct flow *flow;
  int i;

  flow_list->type = SEQUENCIAL;
  if (flow_list->basic == NULL) {
    flow_list->basic = new_flowinfo_basic();
  }
  for (i = 0; i < flow_list->nflow; i++) {
    flow = flow_list->flows[i];
    flow_list->basic->add_func(flow_list->basic, flow);
  }
}

static bool
mbtree_do_build_iterate(void *key, void *val,
                       lagopus_hashentry_t he, void *arg) {
  struct flow_list *flow_list;
  const struct match_stats **match_array;

  (void) key;
  (void) he;
  match_array = arg;

  flow_list = val;
  build_mbtree_child(flow_list, match_array);
  return true;
}

/*
 * flow_list: already stored flows in flow_list->flows[] and nflow.
 */
static void
build_mbtree_child(struct flow_list *flow_list,
                   void *arg) {
  const int min_count = 4;
  const struct match_stats *most_match;
  const struct match_stats **match_array;
  int i;

  if (flow_list == NULL) {
    return;
  }

  match_array = arg;
  if (flow_list->nflow <= min_count || *match_array == NULL) {
    build_mbtree_sequencial(flow_list);
    return;
  }

  most_match = *match_array++;

  /* distribute flow entries */
  if (flow_list->flows_dontcare == NULL) {
    flow_list->flows_dontcare = calloc(1, sizeof(struct flow_list)
                                       + sizeof(void *));
  }

  /* set flow_list type and related values */
  set_flow_list_desc(flow_list, most_match);

  DPRINTF("most_match: oxm_field %d\n", most_match->match.oxm_field);
  DPRINTF("most_match: oxm_length %d\n", most_match->match.oxm_length);
  DPRINTF("most_match: min_value %llu\n", most_match->min_value);
  DPRINTF("most_match: max_value %llu\n", most_match->max_value);
  DPRINTF("most_match: shift %d\n", most_match->shift);
  DPRINTF("most_match: count %d\n", most_match->count);
  DPRINTF("flow_list: nflow %d\n", flow_list->nflow);
  DPRINTF("flow_list: shift %d\n", flow_list->shift);
  DPRINTF("flow_list: match_off %d\n", flow_list->match_off);
  DPRINTF("flow_list: keylen %d\n", flow_list->keylen);

  for (i = 0; i < flow_list->nflow; i++) {
    distribute_flow_to_child(flow_list->flows[i], most_match, flow_list);
  }
  DPRINTF("dontcare: nflow %d\n", flow_list->flows_dontcare->nflow);
  /* build child flow list */
  switch (flow_list->type) {
    case HASHMAP:
      lagopus_hashmap_iterate(&flow_list->branch[0],
                              mbtree_do_build_iterate, match_array);
      break;
    default:
      break;
  }
  if (flow_list->flows_dontcare->nflow > 0) {
    DPRINTF("build_mbtree_child dontcare\n");
    build_mbtree_child(flow_list->flows_dontcare, match_array);
  } else {
    free(flow_list->flows_dontcare);
    flow_list->flows_dontcare = NULL;
  }
}

static void
print_indent(int indent) {
  int i;

  for (i = 0; i < indent; i++) {
    putchar(' ');
  }
}

#if 0
static void
dump_mbtree_child(int indent, struct flow_list *flow);

static bool
dump_mbtree_iterate(void *key, void *val,
                       lagopus_hashentry_t he, void *arg) {
  struct flow_list *flow_list;
  int indent;

  key;
  (void) he;
  indent = (int)arg;

  flow_list = val;
  print_indent(indent + 2);
  printf("key 0x%04.4x, %d flows\n", key, flow_list->nflow);
  dump_mbtree_child(indent + 2, flow_list);
  return true;
}

static void
dump_mbtree_child(int indent, struct flow_list *flow_list) {
  print_indent(indent);
  printf("base %d, match_off %d, keylen %d\n",
         flow_list->base, flow_list->match_off, flow_list->keylen);
  switch (flow_list->type) {
    case HASHMAP:
      lagopus_hashmap_iterate(&flow_list->branch[0],
                              dump_mbtree_iterate, indent);
      break;
    default:
      print_indent(indent + 2);
      printf("sequencial %d flows\n", flow_list->nflow);
      break;
  }
  if (flow_list->flows_dontcare != NULL) {
    print_indent(indent + 2);
    printf("dontcare: %d flows\n", flow_list->flows_dontcare->nflow);
    dump_mbtree_child(2, flow_list->flows_dontcare);
  }
}

static void
dump_mbtree(struct flow_list *flows) {
  int i;

  for (i = 0; i < 0x10000; i++) {
    if (flows->branch[i] != NULL) {
      printf("eth_type[0x%04.04x]: %d flows\n",
             i,
             ((struct flow_list *)flows->branch[i])->nflow);
      dump_mbtree_child(2, flows->branch[i]);
    }
  }
  if (flows->flows_dontcare != NULL) {
    printf("no ethertype: %d flows\n", flows->flows_dontcare->nflow);
    dump_mbtree_child(2, flows->flows_dontcare);
  }
}
#endif

void
build_mbtree(struct flow_list *flows) {
  struct flow_list **childp;
  struct flow *flow;
  struct match *match;
  struct match_stats **match_array;
  uint16_t eth_type;
  int i;

  /* store flow into child flow_list. */
  for (i = 0; i < flows->nflow; i++) {
    flow = flows->flows[i];
    match = get_match_eth_type(&flow->match_list, &eth_type);
    if (match != NULL) {
      match->except_flag = true;
      childp = &flows->branch[ntohs(eth_type)];
    } else {
      childp = &flows->flows_dontcare;
    }
    if (*childp == NULL) {
      *childp = calloc(1, sizeof(struct flow_list)
                       + sizeof(void *) * 65536);
      (*childp)->nbranch = 65536;
    }
    flow_add_sub(flow, *childp);
  }
  /* process for each child flow_list. */
  for (i = 0; i < flows->nbranch; i++) {
    if (flows->branch[i] != NULL) {
      match_array = get_match_stats_array(flows->branch[i]);
      build_mbtree_child(flows->branch[i], match_array);
      free(match_array);
    }
  }
  if (flows->flows_dontcare != NULL) {
    match_array = get_match_stats_array(flows->flows_dontcare);
    build_mbtree_child(flows->flows_dontcare, match_array);
    free(match_array);
  }
#if 0
  dump_mbtree(flows);
#endif
}

static void
free_mbtree(int type, void *arg) {
  struct flow_list *flow_list;
  int i;

  if (arg != NULL) {
    switch (type) {
      case HASHMAP:
        lagopus_hashmap_destroy(arg, true);
        break;

      case SEQUENCIAL:
        flow_list = arg;
        if (flow_list->basic != NULL) {
          flow_list->basic->destroy_func(flow_list->basic);
        }
        free(flow_list);
        break;
    }
  }
}

void
cleanup_mbtree(struct flow_list *flows) {
  int i, type;

  if (flows->update_timer != NULL) {
    *flows->update_timer = NULL;
  }
  type = flows->type;
  flows->type = SEQUENCIAL;
  for (i = 0; i < flows->nbranch; i++) {
    if (flows->branch[i] != NULL) {
      free_mbtree(type, flows->branch[i]);
      flows->branch[i] = NULL;
    }
  }
  if (flows->flows_dontcare != NULL) {
    cleanup_mbtree(flows->flows_dontcare);
    flows->flows_dontcare = NULL;
  }
}

struct flow *
find_mbtree(struct lagopus_packet *pkt, struct flow_list *flows) {
  struct flow *flow, *alt_flow;

  flow = find_mbtree_child(pkt, flows->branch[pkt->ether_type]);
  if (pkt->mpls != NULL) {
    alt_flow = find_mbtree_child(pkt,
                                 flows->branch[ntohs(*(((uint16_t *)pkt->mpls) - 1))]);
    if (alt_flow != NULL &&
        (flow == NULL || alt_flow->priority > flow->priority)) {
      flow = alt_flow;
    }
  } else if (pkt->pbb != NULL) {
    alt_flow = find_mbtree_child(pkt, flows->branch[ETHERTYPE_PBB]);
    if (alt_flow != NULL &&
        (flow == NULL || alt_flow->priority > flow->priority)) {
      flow = alt_flow;
    }
  }
  alt_flow = find_mbtree_child(pkt, flows->flows_dontcare);
  if (alt_flow != NULL &&
      (flow == NULL || alt_flow->priority > flow->priority)) {
    flow = alt_flow;
  }
  return flow;
}

static struct flow *
find_mbtree_child(struct lagopus_packet *pkt, struct flow_list *flows) {
  struct flow *flow;
  uint8_t *src;
  lagopus_result_t rv;

  if (flows == NULL) {
    return NULL;
  }
  while (flows->type == HASHMAP) {
    uint8_t key[sizeof(uint64_t)];
    int i;

    src = pkt->base[flows->base] + flows->match_off;
    for (i = 0; i < flows->keylen; i++) {
      key[i] = src[i] & flows->mask[i];
    }
    rv = lagopus_hashmap_find_no_lock(&flows->branch[0],
                                      *(void **)key, &flows);
    if (rv != LAGOPUS_RESULT_OK) {
      return NULL;
    }
  }
  if (flows->basic != NULL) {
    int32_t pri = -1;

    flow = flows->basic->match_func(flows->basic, pkt, &pri);
    DPRINTF("find: basic: flow %p (nflow %d)\n",
            flow, flows->basic->nflow);
  } else {
    flow = NULL;
  }
  if (flows->flows_dontcare != NULL) {
    struct flow *alt_flow;

    alt_flow = find_mbtree_child(pkt, flows->flows_dontcare);
    if (alt_flow != NULL &&
        (flow == NULL || alt_flow->priority > flow->priority)) {
      flow = alt_flow;
    }
  }
  return flow;
}
