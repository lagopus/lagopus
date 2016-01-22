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
 *      @file   flowinfo.c
 *      @brief  Flow database optimized for speed.
 */

#include "lagopus/flowdb.h"
#include "lagopus/flowinfo.h"

static void add_flow(struct flow *, struct table *);
static void del_flow(struct flow *, struct table *);
static struct flow *find_flow(struct flow *, struct table *);

void
flowinfo_init(void) {
  lagopus_add_flow_hook = add_flow;
  lagopus_del_flow_hook = del_flow;
  lagopus_find_flow_hook = find_flow;
}

static void
add_flow(struct flow *flow, struct table *table) {
  struct flowinfo *flowinfo;

  if (table->userdata == NULL) {
    if (table->table_id == 0) {
      /* at first, match by ETH_TYPE for table 0 */
      table->userdata = new_flowinfo_vlan_vid();
    } else {
      /* at first, match by metadata for other table */
      table->userdata = new_flowinfo_metadata_mask();
    }
    if (table->userdata == NULL) {
      return;
    }
  }
  flowinfo = table->userdata;
  flowinfo->add_func(flowinfo, flow);
}

static void
del_flow(struct flow *flow, struct table *table) {
  struct flowinfo *flowinfo;

  if (table->userdata == NULL) {
    /* flows are not exist, nothing to do. */
    return;
  }
  flowinfo = table->userdata;
  flowinfo->del_func(flowinfo, flow);
}

static struct flow *
find_flow(struct flow *flow, struct table *table) {
  struct flowinfo *flowinfo;

  if (table->userdata == NULL) {
    /* flows are not exist, nothing to do. */
    return NULL;
  }
  flowinfo = table->userdata;
  return flowinfo->find_func(flowinfo, flow);
}
