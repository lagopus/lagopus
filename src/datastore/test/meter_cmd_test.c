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

#include "unity.h"
#include "lagopus_apis.h"
#include "lagopus/ofp_meter_mod_apis.h"
#include "lagopus/meter.h"
#include "cmd_test_utils.h"
#include "../datastore_apis.h"
#include "../meter_cmd_internal.h"
#include "../channel_cmd_internal.h"
#include "../controller_cmd_internal.h"
#include "../interface_cmd_internal.h"
#include "../port_cmd_internal.h"
#include "../bridge_cmd_internal.h"
#include "../datastore_internal.h"

static lagopus_dstring_t ds = NULL;
static lagopus_hashmap_t tbl = NULL;
static datastore_interp_t interp = NULL;
static bool destroy = false;

void
setUp(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create interp. */
  INTERP_CREATE(ret, NULL, interp, tbl, ds);
}

void
tearDown(void) {
  /* destroy interp. */
  INTERP_DESTROY(NULL, interp, tbl, ds, destroy)
}

void
meter_create(uint64_t dpid, uint32_t meter_id, size_t band_num) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_meter_mod mod;
  struct meter_band_list band_list;
  struct ofp_error error;
  struct ofp_meter_band_drop band;
  struct meter_band *meter_band;
  size_t i;

  TAILQ_INIT(&band_list);

  mod.command = OFPMC_ADD;
  mod.flags = OFPMF_KBPS | OFPMF_BURST;
  mod.meter_id = meter_id;

  for (i = 0; i < band_num; i++) {
    band.type = OFPMBT_DROP;
    band.len = 16;
    band.rate = (uint32_t) (1 + i);
    band.burst_size = (uint32_t) (2 + i);

    meter_band = meter_band_alloc((struct ofp_meter_band_header *)&band);
    TAILQ_INSERT_TAIL(&band_list, meter_band, entry);
  }

  ret = ofp_meter_mod_add(dpid, &mod, &band_list, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_meter_mod_add error.");
}

void
meter_destroy(uint64_t dpid, uint32_t meter_id) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_meter_mod mod;
  struct ofp_error error;

  mod.command = OFPMC_DELETE;
  mod.flags = OFPMF_KBPS | OFPMF_BURST;
  mod.meter_id = meter_id;

  ret = ofp_meter_mod_delete(dpid, &mod, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_meter_mod_delete error.");
}

void
test_meter_cmd_parse_dump_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"meter", "b1",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
    "\"meters\":[]}]}";

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE(ret, &interp, state, &tbl, &ds, str,
                     "b1", "1", "cha1", "c1", "i1", "p1", "1");

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, meter_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY(ret, &interp, state, &tbl, &ds, str,
                      "b1", "cha1", "c1", "i1", "p1");
}

void
test_meter_cmd_parse_stats_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"meter", "b2", "stats",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b2\",\n"
    "\"meters\":[]}]}";

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE(ret, &interp, state, &tbl, &ds, str,
                     "b2", "2", "cha2", "c2", "i2", "p2", "2");

  /* stats cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, meter_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY(ret, &interp, state, &tbl, &ds, str,
                      "b2", "cha2", "c2", "i2", "p2");
}

void
test_meter_cmd_parse_bat_opt_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"meter", "hoge",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"hoge\"}";

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 meter_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_meter_cmd_parse_bat_opt_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"meter", "br3", "hoge",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"sub_cmd = hoge.\"}";

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE(ret, &interp, state, &tbl, &ds, str,
                     "b3", "3", "cha3", "c3", "i3", "p3", "3");

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 meter_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY(ret, &interp, state, &tbl, &ds, str,
                      "b3", "cha3", "c3", "i3", "p3");
}

void
test_meter_cmd_parse_stats_bat_opt_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"meter", "br4", "stats",
                         "hoge",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"br4\"}";

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE(ret, &interp, state, &tbl, &ds, str,
                     "b4", "4", "cha4", "c4", "i4", "p4", "4");

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 meter_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY(ret, &interp, state, &tbl, &ds, str,
                      "b4", "cha4", "c4", "i4", "p4");
}

void
test_meter_cmd_parse_dump_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  uint64_t dpid = 5;
  uint32_t meter_id = 5;
  size_t band_num = 2;
  char *str = NULL;
  const char *argv1[] = {"meter", "b5",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b5\",\n"
    "\"meters\":[{\"meter-id\":5,\n"
    "\"flags\":[\"kbps\",\n"
    "\"burst\"],\n"
    "\"bands\":[{\"band-id\":0,\n"
    "\"type\":\"drop\",\n"
    "\"rate\":1,\n"
    "\"burst-size\":2},\n"
    "{\"band-id\":1,\n"
    "\"type\":\"drop\",\n"
    "\"rate\":2,\n"
    "\"burst-size\":3}]}]}]}";

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE(ret, &interp, state, &tbl, &ds, str,
                     "b5", "5", "cha5", "c5", "i5", "p5", "5");

  meter_create(dpid, meter_id, band_num);

  /* dump cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, meter_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  meter_destroy(dpid, meter_id);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY(ret, &interp, state, &tbl, &ds, str,
                      "b5", "cha5", "c5", "i5", "p5");
}

void
test_meter_cmd_parse_stats_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  uint64_t dpid = 6;
  uint32_t meter_id = 6;
  size_t band_num = 2;
  char *str = NULL;
  const char *argv1[] = {"meter", "b6", "stats",
                         NULL
                        };

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE(ret, &interp, state, &tbl, &ds, str,
                     "b6", "6", "cha6", "c6", "i6", "p6", "6");

  meter_create(dpid, meter_id, band_num);

  /* stats cmd. */
  /* duration_sec/duration_nsec is undefined value. */
  TEST_CMD_PARSE_WITHOUT_STRCMP(ret, LAGOPUS_RESULT_OK,
                                meter_cmd_parse, &interp, state,
                                ARGV_SIZE(argv1), argv1, &tbl, NULL,
                                &ds, str);

  meter_destroy(dpid, meter_id);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY(ret, &interp, state, &tbl, &ds, str,
                      "b6", "cha6", "c6", "i6", "p6");
}

void
test_destroy(void) {
    destroy = true;
}
