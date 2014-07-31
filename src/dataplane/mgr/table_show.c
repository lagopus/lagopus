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


#include <stdio.h>
#include <stdint.h>

#include "confsys.h"
#include "lagopus/ethertype.h"
#include "lagopus/dpmgr.h"

/* We must have better way to access to dpmgr information. */
extern struct dpmgr *dpmgr;

static void
show_table(struct confsys *confsys, struct table *table, uint8_t table_id) {
  int i, nflow;

  show(confsys, " Table id: %d\n", table_id);

  nflow = 0;
  for (i = 0; i < MAX_FLOWS; i++) {
    nflow += table->flows[i].nflow;
  }
  show(confsys, "  flow entries: %d\n", nflow);
  show(confsys, "  lookup count: %" PRIu64 "\n", table->lookup_count);
  show(confsys, "  matched count: %" PRIu64 "\n", table->matched_count);
}

static void
show_bridge_domains_table_flow(struct confsys *confsys,
                               struct bridge *bridge) {
  uint8_t i;
  struct flowdb *flowdb;
  struct table *table;

  flowdb = bridge->flowdb;
  flowdb_rdlock(flowdb);
  show(confsys, "Bridge: %s\n", bridge->name);

  for (i = 0; i < flowdb->table_size; i++) {
    if ((table = flowdb->tables[i]) != NULL) {
      show_table(confsys, table, i);
    }
  }
  flowdb_rdunlock(flowdb);
}

CALLBACK(show_table_func) {
  struct bridge *bridge;
  ARG_USED();

  TAILQ_FOREACH(bridge, &dpmgr->bridge_list, entry) {
    show_bridge_domains_table_flow(confsys, bridge);
  }
  return 0;
}
