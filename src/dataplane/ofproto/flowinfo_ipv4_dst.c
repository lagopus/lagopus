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
 *      @file   flowinfo_ipv4_dst.c
 *      @brief  Optimized flow database for dataplane, for ipv4_dst
 */

#include <stdlib.h>

#include "openflow.h"
#include "lagopus_apis.h"
#include "lagopus/flowdb.h"
#include "pktbuf.h"
#include "packet.h"

#include "lagopus/flowinfo.h"

#define OXM_FIELD_TYPE(field) ((field) >> 1)
#define IPV4_DST_BITLEN (32)

static lagopus_result_t
add_flow_ipv4_dst_mask(struct flowinfo *, struct flow *);
static lagopus_result_t
del_flow_ipv4_dst_mask(struct flowinfo *, struct flow *);
static struct flow *
match_flow_ipv4_dst_mask(struct flowinfo *, struct lagopus_packet *,
                         int32_t *);
static struct flow *
find_flow_ipv4_dst_mask(struct flowinfo *, struct flow *);
static void
destroy_flowinfo_ipv4_dst_mask(struct flowinfo *);

static lagopus_result_t
add_flow_ipv4_dst(struct flowinfo *, struct flow *);
static lagopus_result_t
del_flow_ipv4_dst(struct flowinfo *, struct flow *);
static struct flow *
match_flow_ipv4_dst(struct flowinfo *, struct lagopus_packet *, int32_t *);
static struct flow *
find_flow_ipv4_dst(struct flowinfo *, struct flow *);
static void
destroy_flowinfo_ipv4_dst(struct flowinfo *);

static lagopus_result_t
get_match_ipv4_dst(const struct match_list *match_list,
                   uint32_t *ipv4_dst,
                   uint32_t *mask) {
  const struct match *match;

  TAILQ_FOREACH(match, match_list, entry) {
    if (match->oxm_field == (OFPXMT_OFB_IPV4_DST << 1) + 1) {
      OS_MEMCPY(ipv4_dst, match->oxm_value, sizeof(*ipv4_dst));
      OS_MEMCPY(mask, &match->oxm_value[4], sizeof(*mask));
      break;
    }
    if (OXM_FIELD_TYPE(match->oxm_field) == OFPXMT_OFB_IPV4_DST) {
      OS_MEMCPY(ipv4_dst, match->oxm_value, sizeof(*ipv4_dst));
      *mask = 0xffffffff;
      break;
    }
  }
  if (match == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }
  return LAGOPUS_RESULT_OK;
}

struct flowinfo *
new_flowinfo_ipv4_dst_mask(void) {
  struct flowinfo *self;

  self = calloc(1, sizeof(struct flowinfo));
  if (self != NULL) {
    self->nflow = 0;
    self->nnext = 0;
    self->next = malloc(1);
    self->misc = new_flowinfo_ipv4_src_mask();
    self->add_func = add_flow_ipv4_dst_mask;
    self->del_func = del_flow_ipv4_dst_mask;
    self->match_func = match_flow_ipv4_dst_mask;
    self->find_func = find_flow_ipv4_dst_mask;
    self->destroy_func = destroy_flowinfo_ipv4_dst_mask;
  }
  return self;
}

static void
destroy_flowinfo_ipv4_dst_mask(struct flowinfo *self) {
  struct flowinfo *flowinfo;
  unsigned int i;

  for (i = 0; i < self->nnext; i++) {
    flowinfo = self->next[i];
    flowinfo->destroy_func(flowinfo);
  }
  free(self->next);
  free(self);
}

static void
freeup_flowinfo(void *val) {
  struct flowinfo *flowinfo;

  flowinfo = val;
  flowinfo->destroy_func(flowinfo);
}

struct flowinfo *
new_flowinfo_ipv4_dst(void) {
  struct flowinfo *self;

  self = calloc(1, sizeof(struct flowinfo));
  if (self != NULL) {
    lagopus_hashmap_create(&self->hashmap, LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                           freeup_flowinfo);
    /* misc is not used */
    self->add_func = add_flow_ipv4_dst;
    self->del_func = del_flow_ipv4_dst;
    self->match_func = match_flow_ipv4_dst;
    self->find_func = find_flow_ipv4_dst;
    self->destroy_func = destroy_flowinfo_ipv4_dst;
  }
  return self;
}

static void
destroy_flowinfo_ipv4_dst(struct flowinfo *self) {
  lagopus_hashmap_destroy(&self->hashmap, true);
  free(self);
}

static lagopus_result_t
add_flow_ipv4_dst_mask(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  uint32_t ipv4_dst, mask;
  lagopus_result_t rv;
  unsigned int i;

  rv = get_match_ipv4_dst(&flow->match_list, &ipv4_dst, &mask);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = LAGOPUS_RESULT_NOT_FOUND;
    for (i = 0; i < self->nnext; i++) {
      if (self->next[i]->userdata == mask) {
        flowinfo = self->next[i];
        rv = LAGOPUS_RESULT_OK;
        break;
      }
    }
    if (rv == LAGOPUS_RESULT_NOT_FOUND) {
      /* new node. */
      flowinfo = new_flowinfo_ipv4_dst();
      flowinfo->userdata = mask;
      self->next = realloc(self->next,
                           (unsigned long)(self->nnext + 1) *
                           sizeof(struct flowinfo *));
      self->next[self->nnext] = flowinfo;
      self->nnext++;
    }
    rv = flowinfo->add_func(flowinfo, flow);
  } else {
    rv = self->misc->add_func(self->misc, flow);
  }
  if (rv == LAGOPUS_RESULT_OK) {
    self->nflow++;
  }
  return rv;
}

static lagopus_result_t
del_flow_ipv4_dst_mask(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  uint32_t ipv4_dst, mask;
  lagopus_result_t rv;
  unsigned int i;

  rv = get_match_ipv4_dst(&flow->match_list, &ipv4_dst, &mask);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = LAGOPUS_RESULT_NOT_FOUND;
    for (i = 0; i < self->nnext; i++) {
      if (self->next[i]->userdata == mask) {
        flowinfo = self->next[i];
        rv = LAGOPUS_RESULT_OK;
        break;
      }
    }
    if (rv == LAGOPUS_RESULT_NOT_FOUND) {
      return LAGOPUS_RESULT_NOT_FOUND;
    }
    rv = flowinfo->del_func(flowinfo, flow);
    if (flowinfo->nflow == 0) {
      flowinfo->destroy_func(flowinfo);
      self->nnext--;
      memmove(&self->next[i], &self->next[i + 1], (size_t)(self->nnext - i));
    }
  } else {
    rv = self->misc->del_func(self->misc, flow);
  }
  if (rv == LAGOPUS_RESULT_OK) {
    self->nflow--;
  }
  return rv;
}

static struct flow *
match_flow_ipv4_dst_mask(struct flowinfo *self, struct lagopus_packet *pkt,
                         int32_t *pri) {
  struct flowinfo *flowinfo;
  struct flow *flow[self->nnext], *matched, *alt_flow;
  struct flow mismatched = {
    .priority = 0,
    .flags = 0,
    .idle_timeout = 0,
    .hard_timeout = 0,
    .match_list = {NULL, NULL},
    .instruction_list = {NULL, NULL},
    .field_bits = 0
  };
  unsigned int i;

  matched = &mismatched;
  //#pragma omp parallel for
  for (i = 0; i < self->nnext; i++) {
    flowinfo = self->next[i];
    flow[i] = flowinfo->match_func(flowinfo, pkt, pri);
  }
  for (i = 0; i < self->nnext; i++) {
    if (flow[i] != NULL && flow[i]->priority > matched->priority) {
      matched = flow[i];
    }
  }
  alt_flow = self->misc->match_func(self->misc, pkt, pri);
  if (alt_flow != NULL) {
    matched = alt_flow;
  }
  if (matched == &mismatched) {
    matched = NULL;
  }
  return matched;
}

static struct flow *
find_flow_ipv4_dst_mask(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  uint32_t ipv4_dst, mask;
  lagopus_result_t rv;
  unsigned int i;

  rv = get_match_ipv4_dst(&flow->match_list, &ipv4_dst, &mask);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = LAGOPUS_RESULT_NOT_FOUND;
    for (i = 0; i < self->nnext; i++) {
      if (self->next[i]->userdata == mask) {
        flowinfo = self->next[i];
        rv = LAGOPUS_RESULT_OK;
        break;
      }
    }
    if (rv == LAGOPUS_RESULT_NOT_FOUND) {
      return NULL;
    }
  } else {
    flowinfo = self->misc;
  }
  return flowinfo->find_func(flowinfo, flow);
}

static lagopus_result_t
add_flow_ipv4_dst(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  uint32_t ipv4_dst, mask;
  lagopus_result_t rv;

  rv = get_match_ipv4_dst(&flow->match_list, &ipv4_dst, &mask);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = lagopus_hashmap_find_no_lock(&self->hashmap,
                                      (void *)ipv4_dst, (void *)&flowinfo);
    if (rv != LAGOPUS_RESULT_OK) {
      void *val;

      flowinfo = new_flowinfo_ipv4();
      val = flowinfo;
      lagopus_hashmap_add_no_lock(&self->hashmap, (void *)ipv4_dst,
                                  (void *)&val, false);
    }
    rv = flowinfo->add_func(flowinfo, flow);
    if (rv == LAGOPUS_RESULT_OK) {
      self->nflow++;
    }
  }
  return rv;
}

static lagopus_result_t
del_flow_ipv4_dst(struct flowinfo *self, struct flow *flow) {
  uint32_t ipv4_dst, mask;
  lagopus_result_t rv;

  rv = get_match_ipv4_dst(&flow->match_list, &ipv4_dst, &mask);
  if (rv == LAGOPUS_RESULT_OK) {
    struct flowinfo *flowinfo;

    rv = lagopus_hashmap_find_no_lock(&self->hashmap, (void *)ipv4_dst,
                                      (void *)&flowinfo);
    if (rv == LAGOPUS_RESULT_OK) {
      flowinfo->del_func(flowinfo, flow);
    }
    if (rv == LAGOPUS_RESULT_OK) {
      self->nflow--;
    }
  }
  return rv;
}

static struct flow *
match_flow_ipv4_dst(struct flowinfo *self, struct lagopus_packet *pkt,
                    int32_t *pri) {
  struct flowinfo *flowinfo;
  uint32_t ipv4_dst;
  struct flow *flow;
  lagopus_result_t rv;

  flow = NULL;
  ipv4_dst = (pkt->ipv4->ip_dst.s_addr & (uint32_t)self->userdata);
  rv = lagopus_hashmap_find_no_lock(&self->hashmap, (void *)ipv4_dst,
                                    (void *)&flowinfo);
  if (rv == LAGOPUS_RESULT_OK) {
    flow = flowinfo->match_func(flowinfo, pkt, pri);
  }
  return flow;
}

static struct flow *
find_flow_ipv4_dst(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  uint32_t ipv4_dst, mask;
  lagopus_result_t rv;

  rv = get_match_ipv4_dst(&flow->match_list, &ipv4_dst, &mask);
  if (rv == LAGOPUS_RESULT_OK) {
    rv = lagopus_hashmap_find_no_lock(&self->hashmap, (void *)ipv4_dst,
                                      (void *)&flowinfo);
    if (rv != LAGOPUS_RESULT_OK) {
      return NULL;
    }
    return flowinfo->find_func(flowinfo, flow);
  } else {
    return self->misc->find_func(self->misc, flow);
  }
}
