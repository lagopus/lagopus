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
 *      @file   match.c
 *      @brief  Call match function for each type
 */

#include <inttypes.h>
#include <stdbool.h>
#include <sys/types.h>

#include <openflow.h>

#include "lagopus/ethertype.h"
#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
#include "lagopus/flowinfo.h"
#include "pktbuf.h"
#include "packet.h"
#include "match.h"

#undef DEBUG
#ifdef DEBUG
#include <stdio.h>

#define DPRINT(...) printf(__VA_ARGS__)
#define DPRINT_FLOW(flow) flow_dump(flow, stdout)
#else
#define DPRINT(...)
#define DPRINT_FLOW(flow)
#endif

struct flow *
lagopus_find_flow(struct lagopus_packet *pkt, struct table *table) {
  struct flowinfo *flowinfo;
  struct flow *flow;
  int32_t prio;

  table->lookup_count++;
  prio = -1;

  flowinfo = table->userdata;
  if (flowinfo != NULL) {
    flow = flowinfo->match_func(flowinfo, pkt, &prio);
  } else {
    flow = NULL;
  }

  if (flow != NULL) {
    DPRINT("MATCHED\n");
    DPRINT_FLOW(flow);
    if (flow->priority > 0) {
      table->matched_count++;
    }
    if (flow->idle_timeout != 0 || flow->hard_timeout != 0) {
      flow->update_time = get_current_time();
    }
  } else {
    DPRINT("NOT MATCHED\n");
  }
  return flow;
}
