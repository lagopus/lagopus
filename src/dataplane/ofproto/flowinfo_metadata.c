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


/**
 *	@file	flowinfo_metadata.c
 *	@brief	Optimized flow database for datapath, for metadata
 */

#include <stdlib.h>

#include "openflow.h"
#include "lagopus/flowdb.h"
#include "lagopus/ptree.h"
#include "pktbuf.h"
#include "packet.h"

#include "lagopus/flowinfo.h"

#define OXM_FIELD_TYPE(field) ((field) >> 1)
#define METADATA_BITLEN (64)

static lagopus_result_t
add_flow_metadata_mask(struct flowinfo *, struct flow *);
static lagopus_result_t
del_flow_metadata_mask(struct flowinfo *, struct flow *);
static struct flow *
match_flow_metadata_mask(struct flowinfo *, struct lagopus_packet *,
                         int32_t *);
static struct flow *
find_flow_metadata_mask(struct flowinfo *, struct flow *);
static void
destroy_flowinfo_metadata_mask(struct flowinfo *);

static lagopus_result_t
add_flow_metadata(struct flowinfo *, struct flow *);
static lagopus_result_t
del_flow_metadata(struct flowinfo *, struct flow *);
static struct flow *
match_flow_metadata(struct flowinfo *, struct lagopus_packet *, int32_t *);
static struct flow *
find_flow_metadata(struct flowinfo *, struct flow *);
static void
destroy_flowinfo_metadata(struct flowinfo *);

static lagopus_result_t
get_match_metadata(const struct match_list *match_list,
                   uint64_t *metadata,
                   uint64_t *mask) {
  const struct match *match;

  TAILQ_FOREACH(match, match_list, entry) {
    if (match->oxm_field == (OFPXMT_OFB_METADATA << 1) + 1) {
      OS_MEMCPY(metadata, match->oxm_value, sizeof(*metadata));
      OS_MEMCPY(mask, &match->oxm_value[8], sizeof(*mask));
      break;
    }
    if (OXM_FIELD_TYPE(match->oxm_field) == OFPXMT_OFB_METADATA) {
      OS_MEMCPY(metadata, match->oxm_value, sizeof(*metadata));
      *mask = 0xffffffffffffffff;
      break;
    }
  }
  if (match == NULL) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }
  return LAGOPUS_RESULT_OK;
}

struct flowinfo *
new_flowinfo_metadata_mask(void) {
  struct flowinfo *self;

  self = calloc(1, sizeof(struct flowinfo));
  if (self != NULL) {
    self->nflow = 0;
    self->nnext = 0;
    self->next = malloc(0);
    self->misc = new_flowinfo_eth_type();
    self->add_func = add_flow_metadata_mask;
    self->del_func = del_flow_metadata_mask;
    self->match_func = match_flow_metadata_mask;
    self->find_func = find_flow_metadata_mask;
    self->destroy_func = destroy_flowinfo_metadata_mask;
  }
  return self;
}

static void
destroy_flowinfo_metadata_mask(struct flowinfo *self) {
  struct flowinfo *flowinfo;
  int i;

  for (i = 0; i < self->nnext; i++) {
    flowinfo = self->next[i];
    flowinfo->destroy_func(flowinfo);
  }
  free(self->next);
  free(self);
}

struct flowinfo *
new_flowinfo_metadata(void) {
  struct flowinfo *self;

  self = calloc(1, sizeof(struct flowinfo));
  if (self != NULL) {
    self->ptree = ptree_init(METADATA_BITLEN);
    /* misc is not used */
    self->add_func = add_flow_metadata;
    self->del_func = del_flow_metadata;
    self->match_func = match_flow_metadata;
    self->find_func = find_flow_metadata;
    self->destroy_func = destroy_flowinfo_metadata;
  }
  return self;
}

static void
destroy_flowinfo_metadata(struct flowinfo *self) {
  struct ptree_node *node;
  struct flowinfo *flowinfo;

  node = ptree_top(self->ptree);
  while (node != NULL) {
    flowinfo = node->info;
    if (flowinfo != NULL) {
      flowinfo->destroy_func(flowinfo);
    }
    node = ptree_next(node);
  }
  ptree_free(self->ptree);
  free(self);
}

static lagopus_result_t
add_flow_metadata_mask(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  uint64_t metadata, mask;
  lagopus_result_t rv;
  int i;

  rv = get_match_metadata(&flow->match_list, &metadata, &mask);
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
      flowinfo = new_flowinfo_metadata();
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
del_flow_metadata_mask(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  uint64_t metadata, mask;
  lagopus_result_t rv;
  int i;

  rv = get_match_metadata(&flow->match_list, &metadata, &mask);
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
match_flow_metadata_mask(struct flowinfo *self, struct lagopus_packet *pkt,
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
    .flow_type = 0,
    .field_bits = 0
  };
  int i;

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
find_flow_metadata_mask(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  uint64_t metadata, mask;
  lagopus_result_t rv;
  int i;

  rv = get_match_metadata(&flow->match_list, &metadata, &mask);
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
add_flow_metadata(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  struct ptree_node *node;
  uint64_t metadata, mask;
  lagopus_result_t rv;

  rv = get_match_metadata(&flow->match_list, &metadata, &mask);
  if (rv == LAGOPUS_RESULT_OK) {
    node = ptree_node_get(self->ptree, (uint8_t *)&metadata,
                          METADATA_BITLEN);
    if (node->info == NULL) {
      /* new node. */
      node->info = new_flowinfo_eth_type();
    }
    flowinfo = node->info;
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
del_flow_metadata(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  struct ptree_node *node;
  uint64_t metadata, mask;
  lagopus_result_t rv;

  rv = get_match_metadata(&flow->match_list, &metadata, &mask);
  if (rv == LAGOPUS_RESULT_OK) {
    node = ptree_node_get(self->ptree, (uint8_t *)&metadata,
                          METADATA_BITLEN);
    if (node->info == NULL) {
      return LAGOPUS_RESULT_NOT_FOUND;
    }
    flowinfo = node->info;
    rv = flowinfo->del_func(flowinfo, flow);
  } else {
    rv = self->misc->del_func(self->misc, flow);
  }
  if (rv == LAGOPUS_RESULT_OK) {
    self->nflow--;
  }
  return rv;
}

static struct flow *
match_flow_metadata(struct flowinfo *self, struct lagopus_packet *pkt,
                    int32_t *pri) {
  struct flowinfo *flowinfo;
  struct ptree_node *node;
  uint64_t metadata;
  struct flow *flow;

  flow = NULL;
  metadata = (pkt->oob_data.metadata & self->userdata);
  node = ptree_node_lookup(self->ptree, (uint8_t *)&metadata, METADATA_BITLEN);
  if (node != NULL) {
    flowinfo = node->info;
    flow = flowinfo->match_func(flowinfo, pkt, pri);
    ptree_unlock_node(node);
  }
  return flow;
}

static struct flow *
find_flow_metadata(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  struct ptree_node *node;
  uint64_t metadata, mask;
  lagopus_result_t rv;

  rv = get_match_metadata(&flow->match_list, &metadata, &mask);
  if (rv == LAGOPUS_RESULT_OK) {
    node = ptree_node_get(self->ptree, (uint8_t *)&metadata,
                          METADATA_BITLEN);
    if (node->info == NULL) {
      return NULL;
    }
    flowinfo = node->info;
  } else {
    flowinfo = self->misc;
  }
  return flowinfo->find_func(flowinfo, flow);
}
