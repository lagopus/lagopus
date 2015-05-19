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
 *      @file   flowinfo_port.c
 *      @brief  Optimized flow database for datapath, for Port
 */

#include <stdlib.h>

#include "openflow.h"
#include "lagopus/ptree.h"
#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "pktbuf.h"
#include "packet.h"

#include "lagopus/flowinfo.h"

#define OXM_FIELD_TYPE(field) ((field) >> 1)

static lagopus_result_t
add_flow_in_port(struct flowinfo *, struct flow *);
static lagopus_result_t
del_flow_in_port(struct flowinfo *, struct flow *);
static struct flow *
match_flow_in_port(struct flowinfo *, struct lagopus_packet *, int32_t *);
static struct flow *
find_flow_in_port(struct flowinfo *, struct flow *);
static void
destroy_flowinfo_in_port(struct flowinfo *);

static struct match *
get_match_in_port(struct match_list *match_list, uint32_t *port) {
  struct match *match;

  TAILQ_FOREACH(match, match_list, entry) {
    if (OXM_FIELD_TYPE(match->oxm_field) == OFPXMT_OFB_IN_PORT) {
      OS_MEMCPY(port, match->oxm_value, sizeof(*port));
      break;
    }
  }
  return match;
}

struct flowinfo *
new_flowinfo_in_port(void) {
  struct flowinfo *self;

  self = calloc(1, sizeof(struct flowinfo));
  if (self != NULL) {
    self->ptree = ptree_init(32); /* use vector? */
    self->misc = new_flowinfo_vlan_vid();
    self->add_func = add_flow_in_port;
    self->del_func = del_flow_in_port;
    self->match_func = match_flow_in_port;
    self->find_func = find_flow_in_port;
    self->destroy_func = destroy_flowinfo_in_port;
  }
  return self;
}

static void
destroy_flowinfo_in_port(struct flowinfo *self) {
  struct ptree_node *node;
  struct flowinfo *flowinfo;

  node = ptree_top(self->ptree);
  while (node != NULL) {
    flowinfo = node->info;
    flowinfo->destroy_func(flowinfo);
    node = ptree_next(node);
  }
  ptree_free(self->ptree);
  self->misc->destroy_func(self->misc);
  free(self);
}

static lagopus_result_t
add_flow_in_port(struct flowinfo *self, struct flow *flow) {
  struct match *match;
  struct flowinfo *flowinfo;
  struct ptree_node *node;
  uint32_t in_port;

  match = get_match_in_port(&flow->match_list, &in_port);
  if (match != NULL) {
    in_port = OS_NTOHL(in_port);
    node = ptree_node_get(self->ptree, (uint8_t *)&in_port, 32);
    if (node->info == NULL) {
      /* new node. */
      node->info = new_flowinfo_vlan_vid();
    }
    match->except_flag = true;
    flowinfo = node->info;
    return flowinfo->add_func(flowinfo, flow);
  } else {
    return self->misc->add_func(self->misc, flow);
  }

  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
del_flow_in_port(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  struct ptree_node *node;
  uint32_t in_port;

  if (get_match_in_port(&flow->match_list, &in_port) != NULL) {
    in_port = OS_NTOHL(in_port);
    node = ptree_node_get(self->ptree, (uint8_t *)&in_port, 32);
    if (node->info == NULL) {
      return LAGOPUS_RESULT_NOT_FOUND;
    }
    flowinfo = node->info;
    return flowinfo->del_func(flowinfo, flow);
  } else {
    return self->misc->del_func(self->misc, flow);
  }
}

static struct flow *
match_flow_in_port(struct flowinfo *self, struct lagopus_packet *pkt,
                   int32_t *pri) {
  struct flowinfo *flowinfo;
  struct ptree_node *node;
  struct flow *flow, *alt_flow;

  flow = NULL;
  node = ptree_node_lookup(self->ptree,
                           (uint8_t *)&pkt->in_port->ofp_port.port_no, 32);
  if (node != NULL) {
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
find_flow_in_port(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  struct ptree_node *node;
  uint32_t in_port;

  if (get_match_in_port(&flow->match_list, &in_port) != NULL) {
    in_port = OS_NTOHL(in_port);
    node = ptree_node_get(self->ptree, (uint8_t *)&in_port, 32);
    if (node->info == NULL) {
      return NULL;
    }
    flowinfo = node->info;
    return flowinfo->find_func(flowinfo, flow);
  } else {
    return self->misc->find_func(self->misc, flow);
  }
}
