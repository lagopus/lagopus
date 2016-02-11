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
 *      @file   flowinfo_vlan.c
 *      @brief  Optimized flow database for dataplane, for VLAN
 */

#include <stdlib.h>

#include "openflow.h"
#include "lagopus_apis.h"
#include "lagopus/flowdb.h"
#include "pktbuf.h"
#include "packet.h"

#include "lagopus/flowinfo.h"

#define OXM_FIELD_TYPE(field) ((field) >> 1)
#define OXM_FIELD_MASK(field) ((field) & 1)
#define VLAN_VID_BITLEN       16

static lagopus_result_t
add_flow_vlan_vid(struct flowinfo *, struct flow *);
static lagopus_result_t
del_flow_vlan_vid(struct flowinfo *, struct flow *);
static struct flow *
match_flow_vlan_vid(struct flowinfo *, struct lagopus_packet *, int32_t *);
static struct flow *
find_flow_vlan_vid(struct flowinfo *, struct flow *);
static void
destroy_flowinfo_vlan_vid(struct flowinfo *);

static struct match *
get_match_vlan_vid(struct match_list *match_list, uint16_t *vid) {
  struct match *match;

  TAILQ_FOREACH(match, match_list, entry) {
    if (OXM_FIELD_TYPE(match->oxm_field) == OFPXMT_OFB_VLAN_VID &&
        !OXM_FIELD_MASK(match->oxm_field)) {
      OS_MEMCPY(vid, match->oxm_value, sizeof(*vid));  /* network byte order */
      if (*vid != 0) {
        *vid |= OS_HTONS(OFPVID_PRESENT);
      }
      break;
    }
  }
  return match;
}

static void
freeup_flowinfo(void *val) {
  struct flowinfo *flowinfo;

  flowinfo = val;
  flowinfo->destroy_func(flowinfo);
}

struct flowinfo *
new_flowinfo_vlan_vid(void) {
  struct flowinfo *self;

  self = calloc(1, sizeof(struct flowinfo));
  if (self != NULL) {
    lagopus_hashmap_create(&self->hashmap, LAGOPUS_HASHMAP_TYPE_ONE_WORD,
                           freeup_flowinfo);
    self->misc = new_flowinfo_eth_type();
    self->add_func = add_flow_vlan_vid;
    self->del_func = del_flow_vlan_vid;
    self->match_func = match_flow_vlan_vid;
    self->find_func = find_flow_vlan_vid;
    self->destroy_func = destroy_flowinfo_vlan_vid;
  }
  return self;
}

static void
destroy_flowinfo_vlan_vid(struct flowinfo *self) {
  lagopus_hashmap_destroy(&self->hashmap, true);
  self->misc->destroy_func(self->misc);
  free(self);
}

static lagopus_result_t
add_flow_vlan_vid(struct flowinfo *self, struct flow *flow) {
  struct match *match;
  struct flowinfo *flowinfo;
  uint16_t vlan_vid;  /* network byte order */
  lagopus_result_t rv;

  match = get_match_vlan_vid(&flow->match_list, &vlan_vid);
  if (match != NULL) {
    rv = lagopus_hashmap_find_no_lock(&self->hashmap,
                                      (void *)vlan_vid, (void *)&flowinfo);
    if (rv != LAGOPUS_RESULT_OK) {
      void *val;

      flowinfo = new_flowinfo_eth_type();
      val = flowinfo;
      rv = lagopus_hashmap_add_no_lock(&self->hashmap, (void *)vlan_vid,
                                       (void *)&val, false);
      if (rv != LAGOPUS_RESULT_OK) {
        goto out;
      }
    }
    rv = flowinfo->add_func(flowinfo, flow);
    /*match->except_flag = true;*/ /* XXX vid match with mask */
  } else {
    rv = self->misc->add_func(self->misc, flow);
  }
  if (rv == LAGOPUS_RESULT_OK) {
    self->nflow++;
  }
out:
  return rv;
}

static lagopus_result_t
del_flow_vlan_vid(struct flowinfo *self, struct flow *flow) {
  uint16_t vlan_vid;  /* network byte order */
  lagopus_result_t rv;

  if (get_match_vlan_vid(&flow->match_list, &vlan_vid) != NULL) {
    struct flowinfo *flowinfo;

    rv = lagopus_hashmap_find_no_lock(&self->hashmap, (void *)vlan_vid,
                                      (void *)&flowinfo);
    if (rv == LAGOPUS_RESULT_OK) {
      rv = flowinfo->del_func(flowinfo, flow);
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
match_flow_vlan_vid(struct flowinfo *self, struct lagopus_packet *pkt,
                    int32_t *pri) {
  struct flowinfo *flowinfo;
  struct flow *flow, *alt_flow;
  uint16_t vlan_vid;  /* network byte order */
  lagopus_result_t rv;

  flow = NULL;
  if (pkt->vlan != NULL) {
    vlan_vid =
      (VLAN_TCI(pkt->vlan) & OS_HTONS(0x0fff)) | OS_HTONS(OFPVID_PRESENT);
  } else {
    vlan_vid = 0;
  }
  rv = lagopus_hashmap_find_no_lock(&self->hashmap, (void *)vlan_vid,
                                    (void *)&flowinfo);
  if (rv == LAGOPUS_RESULT_OK) {
    flow = flowinfo->match_func(flowinfo, pkt, pri);
  }
  alt_flow = self->misc->match_func(self->misc, pkt, pri);
  if (alt_flow != NULL) {
    flow = alt_flow;
  }
  return flow;
}

static struct flow *
find_flow_vlan_vid(struct flowinfo *self, struct flow *flow) {
  struct flowinfo *flowinfo;
  uint16_t vlan_vid;  /* network byte order */

  if (get_match_vlan_vid(&flow->match_list, &vlan_vid) != NULL) {
    if (lagopus_hashmap_find_no_lock(&self->hashmap, (void *)vlan_vid,
                                     (void *)&flowinfo) != LAGOPUS_RESULT_OK) {
      return NULL;
    }
    return flowinfo->find_func(flowinfo, flow);
  } else {
    return self->misc->find_func(self->misc, flow);
  }
}
