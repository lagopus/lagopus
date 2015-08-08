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
 *      @file   mbtree.c
 *      @brief  multiple branch tree for Openflow
 */

#include <inttypes.h>
#include <stddef.h>
#include <sys/types.h>

#include <sys/queue.h>

#ifndef HAVE_DPDK
#include <net/ethernet.h>
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
#include "lagopus/ptree.h"
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
  DECISION8,
  DECISION16,
  DECISION32,
  DECISION64,
  RADIXTREE,
};

#define DPRINTF(...)

#undef ALWAYS_DECISION64

#define MAKE_MATCH_IDX(base, type, member,  mask, shift)        \
  { base, offsetof(struct type, member), sizeof(((struct type *)0)->member), mask, shift }
#define OXM_FIELD_TYPE(field) ((field) >> 1)

#define BRANCH8i(val, t)        (((val) - flows->min8) / (t))
#define BRANCH16i(val, t)       (((val) - flows->min16) / (t))
#define BRANCH32i(val, t)       (((val) - flows->min32) / (t))
#define BRANCH64i(val, t)       (((val) - flows->min64) / (t))
#define BRANCH8t(val, t)        flows->branch[BRANCH8i(val, t)]
#define BRANCH16t(val, t)       flows->branch[BRANCH16i(val, t)]
#define BRANCH32t(val, t)       flows->branch[BRANCH32i(val, t)]
#define BRANCH64t(val, t)       flows->branch[BRANCH64i(val, t)]
#define BRANCH8(val)    BRANCH8t(val, flows->threshold8)
#define BRANCH16(val)   BRANCH16t(val, flows->threshold16)
#define BRANCH32(val)   BRANCH32t(val, flows->threshold32)
#define BRANCH64(val)   BRANCH64t(val, flows->threshold64)

#define BRANCH_NUM 16

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
  MAKE_MATCH_IDX(OOB_BASE, oob_data, tunnel_id, UINT64_MAX, 0),  /* 38 TUNNEL_ID */
  MAKE_MATCH_IDX(OOB_BASE, oob_data, ipv6_exthdr, UINT16_MAX, 0) /* 39 IPV6_EXTHDR */
};

static void build_mbtree_child(struct flow_list *flows);
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

void
build_mbtree(struct flow_list *flows) {
  struct flow_list **childp;
  struct flow *flow;
  struct match *match;
  uint16_t eth_type;
  int i;

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
      *childp = calloc(1, sizeof(struct flow_list));
    }
    flow_add_sub(flow, *childp);
  }
  for (i = 0; i < 0xffff; i++) {
    if (flows->branch[i] != NULL) {
      build_mbtree_child(flows->branch[i]);
    }
  }
  if (flows->flows_dontcare != NULL) {
    build_mbtree_child(flows->flows_dontcare);
  }
}

static void
build_mbtree_decision8(struct flow_list *flows,
                       uint8_t min_value,
                       uint8_t threshold) {
  struct flow *flow;
  struct match *match;
  uint8_t val8;
  int i;

  flows->type = DECISION8;
  flows->match_mask8 = match_idx[flows->oxm_field >> 1].mask;
  flows->min8 = min_value << flows->shift;
  flows->threshold8 = threshold << flows->shift;
  for (i = 0; i < flows->nflow; i++) {
    flow = flows->flows[i];
    TAILQ_FOREACH(match, &flow->match_list, entry) {
      if (match->oxm_field == flows->oxm_field) {
        break;
      }
    }
    if (match == NULL) {
      DPRINTF("add to dontcare");
      if (flow_add_sub(flow, flows->flows_dontcare) != LAGOPUS_RESULT_OK) {
        DPRINTF("dontcare: flow_add_sub error\n");
      }
    } else {
      memcpy(&val8, match->oxm_value, sizeof(uint8_t));
      val8 <<= flows->shift;
      DPRINTF("flows[%d] = %d -> branch[%d]\n",
              i, val8, BRANCH8i(val8, flows->threshold8));
      if (BRANCH8(val8) == NULL) {
        BRANCH8(val8) = calloc(1, sizeof(struct flow_list));
      }
      if (flow_add_sub(flow, BRANCH8(val8)) != LAGOPUS_RESULT_OK) {
        DPRINTF("true: flow_add_sub error\n");
      }
    }
  }
}

static void
build_mbtree_decision16(struct flow_list *flows,
                        uint16_t min_value,
                        uint16_t threshold) {
  struct flow *flow;
  struct match *match;
  uint16_t val16;
  int i;

  flows->type = DECISION16;
  flows->match_mask16 = match_idx[flows->oxm_field >> 1].mask;
  flows->min16 = min_value << flows->shift;
  flows->threshold16 = threshold << flows->shift;
  for (i = 0; i < flows->nflow; i++) {
    flow = flows->flows[i];
    TAILQ_FOREACH(match, &flow->match_list, entry) {
      if (match->oxm_field  == flows->oxm_field) {
        break;
      }
    }
    if (match == NULL) {
      DPRINTF("add to dontcare");
      if (flow_add_sub(flow, flows->flows_dontcare) != LAGOPUS_RESULT_OK) {
        DPRINTF("dontcare: flow_add_sub error\n");
      }
    } else {
      memcpy(&val16, match->oxm_value, sizeof(uint16_t));
      val16 = ntohs(val16) << flows->shift;
      DPRINTF("flows[%d] = %d -> branch[%d]\n",
              i, val16, BRANCH16i(val16, flows->threshold16));
      if (BRANCH16(val16) == NULL) {
        BRANCH16(val16) = calloc(1, sizeof(struct flow_list));
      }
      if (flow_add_sub(flow, BRANCH16(val16)) != LAGOPUS_RESULT_OK) {
        DPRINTF("true: flow_add_sub error\n");
      }
    }
  }
}

static void
build_mbtree_decision32(struct flow_list *flows,
                        uint32_t min_value,
                        uint32_t threshold) {
  struct flow *flow;
  struct match *match;
  uint32_t val32;
  int i;

  flows->type = DECISION32;
  flows->match_mask32 = match_idx[flows->oxm_field >> 1].mask;
  flows->min32 = min_value << flows->shift;
  flows->threshold32 = threshold << flows->shift;
  for (i = 0; i < flows->nflow; i++) {
    flow = flows->flows[i];
    TAILQ_FOREACH(match, &flow->match_list, entry) {
      if (match->oxm_field == flows->oxm_field) {
        break;
      }
    }
    if (match == NULL) {
      DPRINTF("add to dontcare");
      if (flow_add_sub(flow, flows->flows_dontcare) != LAGOPUS_RESULT_OK) {
        DPRINTF("dontcare: flow_add_sub error\n");
      }
    } else {
      memcpy(&val32, match->oxm_value, sizeof(uint32_t));
      val32 = ntohl(val32) << flows->shift;
      DPRINTF("flows[%d] = %d -> branch[%d]\n",
              i, val32, BRANCH32i(val32, flows->threshold32));
      if (BRANCH32(val32) == NULL) {
        BRANCH32(val32) = calloc(1, sizeof(struct flow_list));
      }
      if (flow_add_sub(flow, BRANCH32(val32)) != LAGOPUS_RESULT_OK) {
        DPRINTF("true: flow_add_sub error\n");
      }
    }
  }
}

static void
build_mbtree_decision64(struct flow_list *flows,
                        uint64_t min_value,
                        uint64_t threshold) {
  struct flow *flow;
  struct match *match;
  uint64_t val64;
  int i;

  flows->type = DECISION64;
  flows->match_mask64 = match_idx[flows->oxm_field >> 1].mask;
  flows->min64 = min_value << flows->shift;
  flows->threshold64 = threshold << flows->shift;
  for (i = 0; i < flows->nflow; i++) {
    flow = flows->flows[i];
    TAILQ_FOREACH(match, &flow->match_list, entry) {
      if (match->oxm_field == flows->oxm_field) {
        break;
      }
    }
    if (match == NULL) {
      DPRINTF("add to dontcare");
      if (flow_add_sub(flow, flows->flows_dontcare) != LAGOPUS_RESULT_OK) {
        DPRINTF("dontcare: flow_add_sub error\n");
      }
    } else {
      memcpy(&val64, match->oxm_value, sizeof(uint64_t));
      val64 = OS_NTOHLL(val64) << flows->shift;
      DPRINTF("flows[%d] = %qd -> branch[%d]\n",
              i, val64, BRANCH64i(val64, flows->threshold64));
      if (BRANCH64(val64) == NULL) {
        BRANCH64(val64) = calloc(1, sizeof(struct flow_list));
      }
      if (flow_add_sub(flow, BRANCH64(val64)) != LAGOPUS_RESULT_OK) {
        DPRINTF("true: flow_add_sub error\n");
      }
    }
  }
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
  uint64_t value;
  int i;

  TAILQ_FOREACH(match_stats, match_stats_list, entry) {
    if (match->oxm_field == match_stats->oxm_field) {
      if ((match->oxm_field & 1) == 0 ||
          !memcmp(&match->oxm_value[match->oxm_length],
                  &match_stats->oxm_value[match->oxm_length],
                  match->oxm_length)) {
        /* found exist match_stats */
        if (match->oxm_length <= sizeof(uint64_t)) {
          value = 0;
          for (i = 0; i < match->oxm_length; i++) {
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
  }
  match_stats = calloc(1, sizeof(struct match_stats));
  memcpy(match_stats, match, sizeof(struct match) + match->oxm_length);
  ((struct match_stats *)match_stats)->count = 1;
  value = 0;
  if (match->oxm_length <= sizeof(uint64_t)) {
    for (i = 0; i < match->oxm_length; i++) {
      value <<= 8;
      value |= match->oxm_value[i];
    }
    ((struct match_stats *)match_stats)->min_value = value;
    ((struct match_stats *)match_stats)->max_value = value;
  }
  TAILQ_INSERT_TAIL(match_stats_list, match_stats, entry);
}

static void
count_flow_list_match(struct flow_list *flow_list,
                      struct match_list *match_stats_list) {
  struct flow *flow;
  struct match *match;
  int i;

  for (i = 0; i < flow_list->nflow; i++) {
    flow = flow_list->flows[i];
    TAILQ_FOREACH(match, &flow->match_list, entry) {
      if (match->oxm_field >> 1 == OFPXMT_OFB_ETH_TYPE) {
        continue;
      }
      count_match(match, match_stats_list);
    }
  }
}

static struct match_stats *
get_most_match(struct flow_list *flow_list) {
  struct match_stats nullmatch = { };
  struct match_list match_stats_list;
  struct match *match;
  struct match_stats *most_match;
  uint64_t threshold;


  TAILQ_INIT(&match_stats_list);
  count_flow_list_match(flow_list, &match_stats_list);

  most_match = &nullmatch;
  TAILQ_FOREACH(match, &match_stats_list, entry) {
    struct match_stats *match_stats = (struct match_stats *)match;

    if (match_stats->min_value == match_stats->max_value) {
      continue;
    }
    if (match_stats->count > most_match->count) {
      most_match = match_stats;
    }
  }
  if (most_match == &nullmatch) {
    return NULL;
  }
  threshold = (most_match->max_value -most_match->min_value) / BRANCH_NUM;
  if (threshold == 0) {
    threshold = 1;
  }
  most_match->threshold = threshold;
  return most_match;
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
      if (!memcmp(&match->oxm_value[match->oxm_length],
                  &match_stats->oxm_value[match->oxm_length],
                  match->oxm_length)) {
        /* field ans mask match. */
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
  struct ptree_node *node;
  void *child;

  get_shifted_value(match->oxm_value, match->oxm_length,
                    flow_list->shift, flow_list->keylen >> 3, key);
  switch (flow_list->type) {
    case DECISION8:
      build_mbtree_decision8(flow_list,
                             most_match->min_value,
                             most_match->threshold);
      child = NULL;
      break;

    case DECISION16:
      build_mbtree_decision16(flow_list,
                              most_match->min_value,
                              most_match->threshold);
      child = NULL;
      break;

    case DECISION32:
      build_mbtree_decision32(flow_list,
                              most_match->min_value,
                              most_match->threshold);
      child = NULL;
      break;

    case DECISION64:
      build_mbtree_decision64(flow_list,
                              most_match->min_value,
                              most_match->threshold);
      child = NULL;
      break;

    case RADIXTREE:
      /* use only child_array[0] */
      if (child_array[0] == NULL) {
        child_array[0] = ptree_init(flow_list->keylen);
      }
      node = ptree_node_get(child_array[0], key, flow_list->keylen);
      if (node->info == NULL) {
        node->info = calloc(1, sizeof(struct flow_list));
      }
      child = node->info;
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

  flow_list->type = RADIXTREE;
  idx = match_stats->match.oxm_field >> 1;
  flow_list->base = match_idx[idx].base;
  flow_list->match_off = match_idx[idx].off;
  flow_list->shift = match_idx[idx].shift;
  flow_list->keylen = match_idx[idx].size << 3;
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

/*
 * flow_list: already stored flows in flow_list->flows[] and nflow.
 */
static void
build_mbtree_child(struct flow_list *flow_list) {
  const int min_count = 4;
  struct match_stats *most_match;
  struct ptree_node *node;
  int i;

  if (flow_list == NULL) {
    return;
  }

  if (flow_list->nflow <= min_count) {
    build_mbtree_sequencial(flow_list);
    return;
  }

  /* get most match */
  most_match = get_most_match(flow_list);
  if (most_match == NULL) {
    build_mbtree_sequencial(flow_list);
    return;
  }
  /* distribute flow entries */
  if (flow_list->flows_dontcare == NULL) {
    flow_list->flows_dontcare = calloc(1, sizeof(struct flow_list));
  }

  /* set flow_list type and related values */
  set_flow_list_desc(flow_list, most_match);

  for (i = 0; i < flow_list->nflow; i++) {
    distribute_flow_to_child(flow_list->flows[i], most_match, flow_list);
  }
  /* build child flow list */
  switch (flow_list->type) {
    case DECISION8:
    case DECISION16:
    case DECISION32:
    case DECISION64:
      for (i = 0; i < BRANCH_NUM; i++) {
        build_mbtree_child(flow_list->branch);
      }
      break;
    case RADIXTREE:
      node = ptree_top(flow_list->branch[0]);
      while (node != NULL) {
        if (node->info != NULL) {
          build_mbtree_child(node->info);
        }
        node = ptree_next(node);
      }
      break;
  }
  if (flow_list->flows_dontcare->nflow > 0) {
    DPRINTF("build_mbtree_child dontcare\n");
    build_mbtree_child(flow_list->flows_dontcare);
  } else {
    free(flow_list->flows_dontcare);
    flow_list->flows_dontcare = NULL;
  }
}

static void
free_mbtree(int type, struct flow_list *flow_list) {
  if (flow_list != NULL) {
    int i;

    for (i = 0; i < BRANCH_NUM + 1; i++) {
      struct ptree_node *node;

      if (flow_list->branch[i] == NULL) {
        continue;
      }
      switch (type) {
        case DECISION8:
        case DECISION16:
        case DECISION32:
        case DECISION64:
          cleanup_mbtree(flow_list->branch[i]);
          break;

        case RADIXTREE:
          node = ptree_top(flow_list->branch[0]);
          while (node != NULL) {
            if (node->info != NULL) {
              cleanup_mbtree(node->info);
            }
            node = ptree_next(node);
          }
          ptree_free(flow_list->branch[0]);
          break;

        case SEQUENCIAL:
          free(flow_list->branch[i]);
          break;
      }
      flow_list->branch[i] = NULL;
    }

    if (flow_list->flows_dontcare != NULL) {
      cleanup_mbtree(flow_list->flows_dontcare);
      flow_list->flows_dontcare = NULL;
    }

    free(flow_list);
  }
}

void
cleanup_mbtree(struct flow_list *flows) {
  int i, type;

  if (flows->mbtree_timer != NULL) {
    *flows->mbtree_timer = NULL;
  }
  type = flows->type;
  flows->type = SEQUENCIAL;
  for (i = 0; i < 0xffff; i++) {
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
  struct ptree_node *node;
  uint8_t val8;
  uint16_t val16;
  uint32_t val32;
  uint64_t val64;
  uint8_t *src;

  if (flows == NULL) {
    return NULL;
  }
  while (flows->type != SEQUENCIAL) {
    uint8_t key[sizeof(uint64_t)];
    int i;

    src = pkt->base[flows->base] + flows->match_off;
    for (i = 0; i < (flows->keylen >> 3); i++) {
      key[i] = src[i] & flows->mask[i];
    }
#ifdef ALWAYS_DECISION64
    val64 = 0;
    for (i = 0; i < flows->keylen; i++) {
      val64 <<= 8;
      val64 |= ((uint8_t *)src)[i];
    }
    flows = BRANCH64((value & flows->match_mask64));
#else
    switch (flows->type) {
      case DECISION8:
        memcpy(&val8, src, sizeof(uint8_t));
        DPRINTF("min %d, th %d, val8 %d -> branch[%d]\n",
                flows->min8, flows->threshold8,
                (val8 & flows->match_mask8), BRANCH8i(val8, flows->threshold8));
        flows = BRANCH8(val8 & flows->match_mask8);
        break;
      case DECISION16:
        memcpy(&val16, src, sizeof(uint16_t));
        val16 = ntohs(val16);
        DPRINTF("min %d, th %d, val16 %d -> branch[%d]\n",
                flows->min16, flows->threshold16,
                (val16 & flows->match_mask16), BRANCH16i(val16, flows->threshold16));
        flows = BRANCH16(val16 & flows->match_mask16);
        break;
      case DECISION32:
        memcpy(&val32, src, sizeof(uint32_t));
        val32 = ntohl(val32);
        DPRINTF("min %d, th %d, val32 %d -> branch[%d]\n",
                flows->min32, flows->threshold32,
                (val32 & flows->match_mask32), BRANCH32i(val32, flows->threshold32));
        DPRINTF("shifted min %d, th %d, val32 %d -> branch[%d]\n",
                flows->min32 >> flows->shift,
                flows->threshold32 >> flows->shift,
                (val32 & flows->match_mask32) >> flows->shift,
                BRANCH32i(val32, flows->threshold32));
        flows = BRANCH32(val32 & flows->match_mask32);
        break;
      case DECISION64:
        memcpy(&val64, src, sizeof(uint64_t));
        val64 = OS_NTOHLL(val64);
        DPRINTF("min %qd, th %qd, val64 %qd -> branch[%d]\n",
                flows->min64, flows->threshold64,
                (val64 & flows->match_mask64), BRANCH64i(val64, flows->threshold64));
        flows = BRANCH64(val64 & flows->match_mask64);
        break;
      case RADIXTREE:
        node = ptree_node_lookup(flows->branch[0], key, flows->keylen);
        if (node != NULL && node->info != NULL) {
          flows = node->info;
        } else {
          return NULL;
        }
        break;
      default:
        printf("XXX\n");
        break;
    }
#endif /* ALWAYS_DECISION64 */
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
