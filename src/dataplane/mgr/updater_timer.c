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
 *      @file   updater_timer.c
 *      @brief  Management timer for table updater(only hybrid mode).
 */

#include <time.h>

#include "lagopus_apis.h"
#include "dp_timer.h"
#include "lagopus/bridge.h"
#include "lagopus/mactable.h"
#include "lagopus/rib.h"
#include "lagopus/updater.h"

/**
 * Callback function is called when the UPDATER timer expires.
 * @param[in] dp_timer The timer object.
 */
static void
updater_timer_expire(struct dp_timer *dp_timer) {
  struct bridge *bridge;
  int i;

  lagopus_msg_info("updater timer expired!\n");
  for (i = 0; i < dp_timer->nentries; i++) {
    bridge = dp_timer->timer_entry[i];
    if (bridge == NULL) {
      continue;
    }

    /* update mactable. */
    mactable_update(&bridge->mactable);

    /* update rib. */
    rib_update(&bridge->rib);

    /* timer reset */
    add_updater_timer(bridge, UPDATER_TABLE_UPDATE_TIME);
  }
}

/**
 * Add UPDATER timer.
 * @param[in] bridge The bridge object with a timer context.
 * @param[in] timeout Timer value.
 */
lagopus_result_t
add_updater_timer(struct bridge *bridge, time_t timeout) {
  void *entryp;

  entryp = add_dp_timer(UPDATER_TIMER, timeout,
                        updater_timer_expire, bridge);
  if (entryp != NULL) {
    bridge->updater_timer = entryp;
  }

  return LAGOPUS_RESULT_OK;
}
