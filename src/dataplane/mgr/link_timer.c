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

#include <time.h>

#include "lagopus_apis.h"
#include "lagopus/interface.h"
#include "dp_timer.h"

#define LINK_INTERVAL	1

static void
link_timer_expire(struct dp_timer *dp_timer) {
  struct interface *ifp;
  int i;

  for (i = 0; i < dp_timer->nentries; i++) {
    ifp = dp_timer->timer_entry[i];
    if (ifp == NULL) {
      continue;
    }
    if (ifp->port != NULL) {
      dp_port_update_link_status(ifp->port);
    }
    /* timer reset */
    add_link_timer(ifp);
  }
}

lagopus_result_t
add_link_timer(struct interface *ifp) {
  void *entryp;

  entryp = add_dp_timer(LINK_TIMER, LINK_INTERVAL,
                        link_timer_expire, ifp);
  ifp->link_timer = entryp;
  return LAGOPUS_RESULT_OK;
}
