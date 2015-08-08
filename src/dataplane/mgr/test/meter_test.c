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

#include <sys/queue.h>
#include "unity.h"
#include "lagopus_apis.h"
#include "lagopus/pbuf.h"
#include "lagopus/meter.h"
#include "openflow13.h"
#include "ofp_band.h"

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_ofp_meter_band_list_elem_free(void) {
  int i;
  int max_cnt = 4;
  struct meter_band_list band_list;
  struct meter_band *band;

  TAILQ_INIT(&band_list);

  /* data */
  for (i = 0; i < max_cnt; i++) {
    band = (struct meter_band *)
           malloc(sizeof(struct meter_band));
    TAILQ_INSERT_TAIL(&band_list, band, entry);
  }

  TEST_ASSERT_EQUAL_MESSAGE(TAILQ_EMPTY(&band_list),
                            false, "not band_list error.");
  /* Call func.*/
  ofp_meter_band_list_elem_free(&band_list);

  TEST_ASSERT_EQUAL_MESSAGE(TAILQ_EMPTY(&band_list),
                            true, "band_list error.");
}
