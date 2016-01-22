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

#include <sys/queue.h>
#include "unity.h"

#include "lagopus_apis.h"
#include "lagopus/pbuf.h"
#include "lagopus/meter.h"
#include "openflow13.h"
#include "ofp_band.h"

static struct meter_table *meter_table;

void
setUp(void) {
  meter_table = meter_table_alloc();
  TEST_ASSERT_NOT_NULL(meter_table);
}

void
tearDown(void) {
  meter_table_free(meter_table);
  meter_table = NULL;
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

void
test_meter_add_modify_delete(void) {
  struct ofp_meter_mod meter_mod;
  struct meter_band_list list;
  struct ofp_error error;
  lagopus_result_t rv;

  TAILQ_INIT(&list);
  meter_mod.meter_id = 0;
  meter_mod.flags = 0;

  rv = meter_table_meter_add(meter_table, &meter_mod, &list, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  rv = meter_table_meter_add(meter_table, &meter_mod, &list, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OFP_ERROR);
  rv = meter_table_meter_modify(meter_table, &meter_mod, &list, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
  meter_mod.meter_id = 1;
  rv = meter_table_meter_modify(meter_table, &meter_mod, &list, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OFP_ERROR);
  meter_mod.meter_id = 0;
  rv = meter_table_meter_delete(meter_table, &meter_mod, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}

void
test_meter_lookup(void) {
  struct ofp_meter_mod meter_mod;
  struct meter_band_list list;
  struct ofp_error error;
  struct meter *meter;
  lagopus_result_t rv;

  TAILQ_INIT(&list);
  meter_mod.meter_id = 0;
  meter_mod.flags = 0;

  rv = meter_table_meter_add(meter_table, &meter_mod, &list, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);

  meter = meter_table_lookup(meter_table, 0);
  TEST_ASSERT_NOT_NULL(meter);
  meter = meter_table_lookup(meter_table, 1);
  TEST_ASSERT_NULL(meter);

  rv = meter_table_meter_delete(meter_table, &meter_mod, &error);
  TEST_ASSERT_EQUAL(rv, LAGOPUS_RESULT_OK);
}
