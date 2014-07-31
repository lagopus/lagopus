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
 *      @file   flowinfo_mpls.c
 *      @brief  Optimized flow database for datapath, for MPLS
 */

#include <stdlib.h>
#include <sys/queue.h>

#include "openflow.h"
#include "lagopus/ethertype.h"
#include "lagopus/ptree.h"
#include "lagopus/flowdb.h"
#include "pktbuf.h"
#include "packet.h"

#include "lagopus/flowinfo.h"

#define OXM_FIELD_TYPE(field) ((field) >> 1)
#define MPLS_LABEL_BITLEN     20

static lagopus_result_t
add_flow_mpls(struct flowinfo *, struct flow *);
static lagopus_result_t
del_flow_mpls(struct flowinfo *, struct flow *);
static struct flow *
match_flow_mpls(struct flowinfo *, struct lagopus_packet *, int32_t *);
static struct flow *
find_flow_mpls(struct flowinfo *, struct flow *);
static void
destroy_flowinfo_mpls(struct flowinfo *);

static struct match *
get_match_mpls_label(struct match_list *match_list, uint32_t *label) {
  struct match *match;

  TAILQ_FOREACH(match, match_list, entry) {
    if (OXM_FIELD_TYPE(match->oxm_field) == OFPXMT_OFB_MPLS_LABEL) {
      OS_MEMCPY(label, match->oxm_value, sizeof(*label));
      break;
    }
  }
  return match;
}

struct flowinfo *
new_flowinfo_mpls(void) {
  struct flowinfo *self;

  self = calloc(1, sizeof(struct flowinfo));
  if (self != NULL) {
    self->ptree = ptree_init(MPLS_LABEL_BITLEN);
    self->misc = new_flowinfo_basic();
    self->add_func = add_flow_mpls;
    self->del_func = del_flow_mpls;
    self->match_func = match_flow_mpls;
    self->find_func = find_flow_mpls;
    self->destroy_func = destroy_flowinfo_mpls;
  }
  return self;
}

static void
destroy_flowinfo_mpls(struct flowinfo *self) {
  struct ptree_node *node;
  struct flowinfo *flowinfo;

  node = ptree_top(self->ptree);
  while (node != NULL) {
    if (node->info != NULL) {
      flowinfo = node->info;
      flowinfo->destroy_func(flowinfo);
    }
    node = ptree_next(node);
  }
  ptree_free(self->ptree);
  self->misc->destroy_func(self->misc);
  free(self);
}

static lagopus_result_t
add_flow_mpls(struct flowinfo *self, struct flow *flow) {
  struct match *match;
  struct flowinfo *flowinfo;
  struct ptree_node *node;
  uint32_t label;
  uint32_t mpls_lse;
  lagopus_result_t ret;

  match = get_match_mpls_label(&flow->match_list, &label);
  if (match != NULL) {
    SET_MPLS_LSE(mpls_lse, OS_NTOHL(label), 0, 0, 0);
    node = ptree_node_get(self->ptree, (uint8_t *)&mpls_lse,
                          MPLS_LABEL_BITLEN);
    if (node == NULL) {
      /* fatal. */
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    if (node->info == NULL) {
      /* new node. */
      node->info = new_flowinfo_basic();
    }
    match->except_flag = true;
    flowinfo = node->info;
    ret = flowinfo->add_func(flowinfo, flow);
  } else {
    ret = self->misc->add_func(self->misc, flow);
  }
  if (LAGOPUS_RESULT_OK == ret) {
    self->nflow++;
  }
  return ret;
}

static lagopus_result_t
del_flow_mpls(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  struct ptree_node *node;
  uint32_t label;
  uint32_t mpls_lse;
  lagopus_result_t ret;

  if (get_match_mpls_label(&flow->match_list, &label) != NULL) {
    SET_MPLS_LSE(mpls_lse, OS_NTOHL(label), 0, 0, 0);
    node = ptree_node_lookup(self->ptree, (uint8_t *)&mpls_lse,
                             MPLS_LABEL_BITLEN);
    if (node == NULL || node->info == NULL) {
      return LAGOPUS_RESULT_NOT_FOUND;
    }
    flowinfo = node->info;
    ret = flowinfo->del_func(flowinfo, flow);
  } else {
    ret = self->misc->del_func(self->misc, flow);
  }
  if (LAGOPUS_RESULT_OK == ret) {
    self->nflow--;
  }
  return ret;
}

static struct flow *
match_flow_mpls(struct flowinfo *self, struct lagopus_packet *pkt,
                int32_t *pri) {
  struct flowinfo *flowinfo;
  struct ptree_node *node;
  struct flow *flow, *alt_flow;

  flow = NULL;
  node = ptree_node_lookup(self->ptree, (uint8_t *)&pkt->mpls->mpls_lse,
                           MPLS_LABEL_BITLEN);
  if (node != NULL && node->info != NULL) {
    flowinfo = node->info;
    flow = flowinfo->match_func(flowinfo, pkt, pri);
    ptree_unlock_node(node);
  }
  alt_flow = self->misc->match_func(self->misc, pkt, pri);
  if (alt_flow != NULL) {
    flow = alt_flow;
  }
  return flow;
}

static struct flow *
find_flow_mpls(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  struct ptree_node *node;
  uint32_t label;
  uint32_t mpls_lse;

  if (get_match_mpls_label(&flow->match_list, &label) != NULL) {
    SET_MPLS_LSE(mpls_lse, OS_NTOHL(label), 0, 0, 0);
    node = ptree_node_get(self->ptree, (uint8_t *)&mpls_lse,
                          MPLS_LABEL_BITLEN);
    if (node == NULL || node->info == NULL) {
      return NULL;
    }
    flowinfo = node->info;
    return flowinfo->find_func(flowinfo, flow);
  } else {
    return self->misc->find_func(self->misc, flow);
  }
}
