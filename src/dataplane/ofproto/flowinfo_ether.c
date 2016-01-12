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
 *      @file   flowinfo_ether.c
 *      @brief  Optimized flow database for dataplane, for ethernet
 */

#include <stdlib.h>
#include <stdbool.h>

#include "openflow.h"
#include "lagopus/ethertype.h"
#include "lagopus/flowdb.h"
#include "lagopus/dataplane.h"
#include "pktbuf.h"
#include "packet.h"

#include "lagopus/flowinfo.h"

#define OXM_FIELD_TYPE(field) ((field) >> 1)

static lagopus_result_t
add_flow_eth_type(struct flowinfo *, struct flow *);
static lagopus_result_t
del_flow_eth_type(struct flowinfo *, struct flow *);
static struct flow *
match_flow_eth_type(struct flowinfo *, struct lagopus_packet *, int32_t *);
static struct flow *
find_flow_eth_type(struct flowinfo *, struct flow *);
static void
destroy_flowinfo_eth_type(struct flowinfo *);

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

struct flowinfo *
new_flowinfo_eth_type(void) {
  struct flowinfo *self;

  self = calloc(1, sizeof(struct flowinfo));
  if (self != NULL) {
    self->nnext = 65536;
    self->next = calloc(self->nnext, sizeof(struct flowinfo *));
    if (self->next == NULL) {
      free(self);
      return NULL;
    }
    self->misc = new_flowinfo_basic();
    self->add_func = add_flow_eth_type;
    self->del_func = del_flow_eth_type;
    self->match_func = match_flow_eth_type;
    self->find_func = find_flow_eth_type;
    self->destroy_func = destroy_flowinfo_eth_type;
  }
  return self;
}

static void
destroy_flowinfo_eth_type(struct flowinfo *self) {
  struct flowinfo *flowinfo;
  int i;

  for (i = 0; i < 65536; i++) {
    flowinfo = self->next[i];
    if (flowinfo != NULL) {
      flowinfo->destroy_func(flowinfo);
    }
  }
  free(self->next);
  self->misc->destroy_func(self->misc);
  free(self);
}

static lagopus_result_t
add_flow_eth_type(struct flowinfo *self, struct flow *flow) {
  struct match *match;
  struct flowinfo *flowinfo;
  uint16_t eth_type;
  lagopus_result_t ret;

  match = get_match_eth_type(&flow->match_list, &eth_type);
  if (match != NULL) {
    eth_type = OS_NTOHS(eth_type);
    if (self->next[eth_type] == NULL) {
      switch (eth_type) {
        case ETHERTYPE_IP:
          self->next[eth_type] = new_flowinfo_ipv4_dst_mask();
          break;
        case ETHERTYPE_IPV6:
          self->next[eth_type] = new_flowinfo_ipv6();
          break;
        case ETHERTYPE_MPLS:
        case ETHERTYPE_MPLS_MCAST:
          self->next[eth_type] = new_flowinfo_mpls();
          break;
        case ETHERTYPE_ARP:
        case ETHERTYPE_PBB:
        default:
          self->next[eth_type] = new_flowinfo_basic();
          break;
      }
    }
    match->except_flag = true;
    flowinfo = self->next[eth_type];
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
del_flow_eth_type(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  uint16_t eth_type;
  lagopus_result_t ret;

  if (get_match_eth_type(&flow->match_list, &eth_type) != NULL) {
    eth_type = OS_NTOHS(eth_type);
    if (self->next[eth_type] != NULL) {
      flowinfo = self->next[eth_type];
    } else {
      return LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    flowinfo = self->misc;
  }
  ret = flowinfo->del_func(flowinfo, flow);
  if (LAGOPUS_RESULT_OK == ret) {
    self->nflow--;
  }
  return ret;
}

static struct flow *
match_flow_eth_type(struct flowinfo *self, struct lagopus_packet *pkt,
                    int32_t *pri) {
  struct flowinfo *flowinfo;
  struct flow *flow, *alt_flow;

  flow = NULL;
  flowinfo = self->next[pkt->ether_type];
  if (flowinfo != NULL) {
    flow = flowinfo->match_func(flowinfo, pkt, pri);
  }
  if (pkt->mpls != NULL) {
    flowinfo = self->next[OS_NTOHS(*(((uint16_t *)pkt->mpls) - 1))];
    if (flowinfo != NULL) {
      alt_flow = flowinfo->match_func(flowinfo, pkt, pri);
      if (alt_flow != NULL) {
        flow = alt_flow;
      }
    }
  }
#ifdef PBB_IS_VLAN
  else if (pkt->pbb != NULL) {
    flowinfo = self->next[ETHERTYPE_PBB];
    if (flowinfo != NULL) {
      alt_flow = flowinfo->match_func(flowinfo, pkt, pri);
      if (alt_flow != NULL) {
        flow = alt_flow;
      }
    }
  }
#endif /* PBB_IS_VLAN */
  alt_flow = self->misc->match_func(self->misc, pkt, pri);
  if (alt_flow != NULL) {
    flow = alt_flow;
  }
  return flow;
}

static struct flow *
find_flow_eth_type(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  uint16_t eth_type;

  if (get_match_eth_type(&flow->match_list, &eth_type) != NULL) {
    eth_type = OS_NTOHS(eth_type);
    if (self->next[eth_type] != NULL) {
      flowinfo = self->next[eth_type];
    } else {
      return NULL;
    }
  } else {
    flowinfo = self->misc;
  }
  return flowinfo->find_func(flowinfo, flow);
}
