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
 *      @file   flowinfo_ipv6.c
 *      @brief  Optimized flow database for dataplane, for IPv6
 */

#include <stdlib.h>

#include "openflow.h"
#include "lagopus/flowdb.h"
#include "lagopus/dataplane.h"
#include "pktbuf.h"
#include "packet.h"

#include "lagopus/flowinfo.h"

#define OXM_FIELD_TYPE(field) ((field) >> 1)

static lagopus_result_t
add_flow_ipv6(struct flowinfo *, struct flow *);
static lagopus_result_t
del_flow_ipv6(struct flowinfo *, struct flow *);
static struct flow *
match_flow_ipv6(struct flowinfo *, struct lagopus_packet *, int32_t *);
static struct flow *
find_flow_ipv6(struct flowinfo *, struct flow *);
static void
destroy_flowinfo_ipv6(struct flowinfo *);

static struct match *
get_match_ip_proto(struct match_list *match_list, uint8_t *proto) {
  struct match *match;

  TAILQ_FOREACH(match, match_list, entry) {
    if (OXM_FIELD_TYPE(match->oxm_field) == OFPXMT_OFB_IP_PROTO) {
      *proto = *match->oxm_value;
      break;
    }
  }
  return match;
}

struct flowinfo *
new_flowinfo_ipv6(void) {
  struct flowinfo *self;
  int proto;

  self = calloc(1, sizeof(struct flowinfo));
  if (self != NULL) {
    self->next = calloc(256, sizeof(struct flowinfo *));
    if (self->next == NULL) {
      free(self);
      return NULL;
    }
    for (proto = 0; proto < 256; proto++) {
      switch (proto) {
        case IPPROTO_TCP:
        case IPPROTO_UDP:
        case IPPROTO_SCTP:
        case IPPROTO_ICMPV6:
        default:
          self->next[proto] = new_flowinfo_basic();
          break;
      }
    }
    self->misc = new_flowinfo_basic();
    self->add_func = add_flow_ipv6;
    self->del_func = del_flow_ipv6;
    self->match_func = match_flow_ipv6;
    self->find_func = find_flow_ipv6;
    self->destroy_func = destroy_flowinfo_ipv6;
  }
  return self;
}

static void
destroy_flowinfo_ipv6(struct flowinfo *self) {
  struct flowinfo *flowinfo;
  int proto;

  for (proto = 0; proto < 256; proto++) {
    flowinfo = self->next[proto];
    flowinfo->destroy_func(flowinfo);
  }
  free(self->next);
  self->misc->destroy_func(self->misc);
  free(self);
}

static lagopus_result_t
add_flow_ipv6(struct flowinfo *self, struct flow *flow) {
  struct match *match;
  uint8_t proto;
  lagopus_result_t ret;

  match = get_match_ip_proto(&flow->match_list, &proto);
  if (match != NULL) {
    match->except_flag = true;
    ret = self->next[proto]->add_func(self->next[proto], flow);
  } else {
    ret = self->misc->add_func(self->misc, flow);
  }
  if (LAGOPUS_RESULT_OK == ret) {
    self->nflow++;
  }
  return ret;
}

static lagopus_result_t
del_flow_ipv6(struct flowinfo *self, struct flow *flow) {
  uint8_t proto;
  lagopus_result_t ret;

  if (get_match_ip_proto(&flow->match_list, &proto) != NULL) {
    ret = self->next[proto]->del_func(self->next[proto], flow);
  } else {
    ret = self->misc->del_func(self->misc, flow);
  }
  if (LAGOPUS_RESULT_OK == ret) {
    self->nflow--;
  }
  return ret;
}

static struct flow *
match_flow_ipv6(struct flowinfo *self, struct lagopus_packet *pkt,
                int32_t *pri) {
  struct flowinfo *flowinfo;
  struct flow *flow, *alt_flow;

  flowinfo = self->next[*pkt->proto];
  flow = flowinfo->match_func(flowinfo, pkt, pri);
  alt_flow = self->misc->match_func(self->misc, pkt, pri);
  if (alt_flow != NULL) {
    flow = alt_flow;
  }
  return flow;
}

static struct flow *
find_flow_ipv6(struct flowinfo *self, struct flow *flow) {
  uint8_t proto;

  if (get_match_ip_proto(&flow->match_list, &proto) != NULL) {
    return self->next[proto]->find_func(self->next[proto], flow);
  } else {
    return self->misc->find_func(self->misc, flow);
  }
}
