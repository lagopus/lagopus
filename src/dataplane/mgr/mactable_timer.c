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
#include "dp_timer.h"
#include "lagopus/mactable.h"

static void
mactable_timer_expire(struct dp_timer *dp_timer) {
  struct mactable *mactable;
  int i;

  lagopus_msg_info("mactable timer expired!\n");
  for (i = 0; i < dp_timer->nentries; i++) {
    mactable = dp_timer->timer_entry[i];
    if (mactable == NULL) {
      continue;
    }
    /* age out */
    mactable_age_out(mactable);

    /* timer reset */
    add_mactable_timer(mactable, MACTABLE_CLEANUP_TIME);
  }
}

lagopus_result_t
add_mactable_timer(struct mactable *mactable, time_t timeout) {
  void *entryp;

  entryp = add_dp_timer(MACTABLE_TIMER, timeout,
                        mactable_timer_expire, mactable);
  if (entryp != NULL) {
    mactable->mactable_timer = entryp;
  }
  return LAGOPUS_RESULT_OK;
}
