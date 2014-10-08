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

#include "lagopus/dpmgr.h"
#include "lagopus/bridge.h"
#include "lagopus/flowdb.h"

extern struct dpmgr *dpmgr;

static void
show_bridge_domains_flowcache(struct confsys *confsys,
                              struct bridge *bridge) {
  struct ofcachestat st;
  struct flowdb *flowdb;

  flowdb = bridge->flowdb;
  flowdb_rdlock(flowdb);

  show(confsys, "Bridge: %s\n", bridge->name);
  get_flowcache_statistics(bridge, &st);
  show(confsys, "  nentries: %d\n", st.nentries);
  show(confsys, "  hit:      %" PRIu64 "\n", st.hit);
  show(confsys, "  miss:     %" PRIu64 "\n", st.miss);

  flowdb_rdunlock(flowdb);
}

CALLBACK(show_flowcache_func) {
  struct bridge *bridge;
  ARG_USED();

  TAILQ_FOREACH(bridge, &dpmgr->bridge_list, entry) {
    show_bridge_domains_flowcache(confsys, bridge);
  }
  return 0;
}
