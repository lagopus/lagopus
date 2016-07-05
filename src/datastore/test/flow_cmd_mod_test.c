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
#include "cmd_test_utils.h"
#include "lagopus/flowdb.h"
#include "lagopus/dp_apis.h"
#include "../datastore_apis.h"
#include "../datastore_internal.h"
#include "../flow_cmd.h"
#include "../flow_cmd_internal.h"
#include "../channel_cmd_internal.h"
#include "../controller_cmd_internal.h"
#include "../interface_cmd_internal.h"
#include "../port_cmd_internal.h"
#include "../bridge_cmd_internal.h"

static lagopus_dstring_t ds = NULL;
static lagopus_hashmap_t tbl = NULL;
static datastore_interp_t interp = NULL;
static bool destroy = false;


void
setUp(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;

  /* create interp. */
  INTERP_CREATE(ret, NULL, interp, tbl, ds);

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE(ret, &interp, state, &tbl, &ds, str,
                     "b1", "1", "cha1", "c1", "i1", "p1", "1");
}

void
tearDown(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY(ret, &interp, state, &tbl, &ds, str,
                      "b1", "cha1", "c1", "i1", "p1");

  /* destroy interp. */
  INTERP_DESTROY(NULL, interp, tbl, ds, destroy);
}

static void
create_buckets(struct bucket_list *bucket_list) {
  TAILQ_INIT(bucket_list);
}

static void
create_bands(struct meter_band_list *band_list) {
  TAILQ_INIT(band_list);
}

void
test_flow_cmd_mod_add_bad_cmd(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "hoge=0",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"NOT_FOUND\",\n"
      "\"data\":\"Not found cmd (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_not_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/*
 * add flow.
 */

void
test_flow_cmd_mod_add_ofp_error(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=0",
                         "in_port=1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OFP_ERROR\",\n"
      "\"data\":\"Can't flow mod (ADD), "
      "ofp_error[type = OFPET_BAD_MATCH, code = OFPBMC_DUP_FIELD].\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/*
 * match.
 */

/* in_port. */
void
test_flow_cmd_mod_add_match_in_port_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_in_port_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=4294967295",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":\"any\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_in_port_03(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=controller",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":\"controller\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_in_port_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=4294967296",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (4294967296).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_in_port_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* in_phy_port. */
void
test_flow_cmd_mod_add_match_in_phy_port_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=0",
                         "in_phy_port=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":0,\n"
      "\"in_phy_port\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_in_phy_port_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=0",
                         "in_phy_port=4294967295",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":0,\n"
      "\"in_phy_port\":\"any\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_in_phy_port_03(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=0",
                         "in_phy_port=controller",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":0,\n"
      "\"in_phy_port\":\"controller\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_in_phy_port_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=0",
                         "in_phy_port=4294967296",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (4294967296).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_in_phy_port_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=0",
                         "in_phy_port=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* metadata. */
void
test_flow_cmd_mod_add_match_metadata_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "metadata=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"metadata\":\"0\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_metadata_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "metadata=18446744073709551615",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"metadata\":\"18446744073709551615\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_metadata_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "metadata=1/0x1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"metadata\":\"1\\/0x0000000000000001\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_metadata_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "metadata=18446744073709551616",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OUT_OF_RANGE\",\n"
      "\"data\":\"Bad value (18446744073709551616).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_metadata_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "metadata=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* dl_dst. */
void
test_flow_cmd_mod_add_match_dl_dst_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_dst=01:02:03:04:05:06",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_dst\":\"01:02:03:04:05:06\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_dl_dst_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_dst=00:00:00:00:00:01/00:00:00:00:00:01",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_dst\":\"00:00:00:00:00:01\\/00:00:00:00:00:01\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_dl_dst_bad(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_dst=01:02:03:04:05:0",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OUT_OF_RANGE\",\n"
      "\"data\":\"Bad value (01:02:03:04:05:0).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_dl_dst_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_dst=01:02:03:04:05:06/ff:fe:fX:fc:fb:fa",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OUT_OF_RANGE\",\n"
      "\"data\":\"Bad value (ff:fe:fX:fc:fb:fa).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* dl_src. */
void
test_flow_cmd_mod_add_match_dl_src_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_src=01:02:03:04:05:06",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_src\":\"01:02:03:04:05:06\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_dl_src_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_src=00:00:00:00:00:01/00:00:00:00:00:01",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_src\":\"00:00:00:00:00:01\\/00:00:00:00:00:01\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_dl_src_bad(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_src=01:02:03:04:05:0",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OUT_OF_RANGE\",\n"
      "\"data\":\"Bad value (01:02:03:04:05:0).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_dl_src_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_src=01:02:03:04:05:06/ff:fe:fX:fc:fb:fa",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OUT_OF_RANGE\",\n"
      "\"data\":\"Bad value (ff:fe:fX:fc:fb:fa).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* dl_type. */
void
test_flow_cmd_mod_add_match_dl_type_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_dl_type_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":65535,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_dl_type_03(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=vlan",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"vlan\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_dl_type_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_dl_type_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* vlan_vid. */
void
test_flow_cmd_mod_add_match_vlan_vid_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vlan_vid=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"vlan_vid\":\"4096\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_vlan_vid_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vlan_vid=65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"vlan_vid\":\"65535\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_vlan_vid_set_flags(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vlan_vid=4097",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"vlan_vid\":\"4097\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_vlan_vid_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vlan_vid=1/0x1001",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"vlan_vid\":\"4097\\/0x1001\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_vlan_vid_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vlan_vid=1/0x123V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x123V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_vlan_vid_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vlan_vid=65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_vlan_vid_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vlan_vid=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* vlan_pcp. */
void
test_flow_cmd_mod_add_match_vlan_pcp_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vlan_vid=0",
                         "vlan_pcp=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"vlan_vid\":\"4096\",\n"
      "\"vlan_pcp\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_vlan_pcp_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vlan_vid=0",
                         "vlan_pcp=7",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"vlan_vid\":\"4096\",\n"
      "\"vlan_pcp\":7,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_vlan_pcp_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vlan_vid=0",
                         "vlan_pcp=1/0x1234",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_vlan_pcp_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vlan_vid=0",
                         "vlan_pcp=8",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (8).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_vlan_pcp_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vlan_vid=0",
                         "vlan_pcp=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* ip_dscp. */
void
test_flow_cmd_mod_add_match_ip_dscp_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "ip_dscp=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"ip_dscp\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_ip_dscp_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "ip_dscp=63",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"ip_dscp\":63,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_ip_dscp_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "ip_dscp=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_ip_dscp_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "ip_dscp=64",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (64).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_ip_dscp_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "ip_dscp=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* nw_ecn. */
void
test_flow_cmd_mod_add_match_nw_ecn_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_ecn=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_ecn\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_nw_ecn_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_ecn=3",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_ecn\":3,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_nw_ecn_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_ecn=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_nw_ecn_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_ecn=4",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (4).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_nw_ecn_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_ecn=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* nw_proto. */
void
test_flow_cmd_mod_add_match_nw_proto_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_nw_proto_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=255",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":255,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_nw_proto_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_nw_proto_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=256",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (256).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_nw_proto_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* nw_src. */
void
test_flow_cmd_mod_add_match_nw_src_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_src=127.0.0.1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_src\":\"127.0.0.1\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_nw_src_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_src=127.0.0.1/127.0.0.1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_src\":\"127.0.0.1\\/127.0.0.1\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_nw_src_bad(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_src=127.0.0.1000",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
      "\"data\":\"Bad value (127.0.0.1000).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_nw_src_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_src=127.0.0.1/127.0.0.1000",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
      "\"data\":\"Bad value (127.0.0.1000).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* nw_dst. */
void
test_flow_cmd_mod_add_match_nw_dst_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_dst=127.0.0.1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_dst\":\"127.0.0.1\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_nw_dst_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_dst=127.0.0.1/127.0.0.1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_dst\":\"127.0.0.1\\/127.0.0.1\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_nw_dst_bad(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_dst=127.0.0.1000",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
      "\"data\":\"Bad value (127.0.0.1000).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_nw_dst_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_dst=127.0.0.1/127.0.0.1000",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
      "\"data\":\"Bad value (127.0.0.1000).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* tcp_src. */
void
test_flow_cmd_mod_add_match_tcp_src_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=6",
                         "tcp_src=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":6,\n"
      "\"tcp_src\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_tcp_src_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=6",
                         "tcp_src=65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":6,\n"
      "\"tcp_src\":65535,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_tcp_src_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=6",
                         "tcp_src=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_tcp_src_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=6",
                         "tcp_src=65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_tcp_src_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=6",
                         "tcp_src=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* tcp_dst. */
void
test_flow_cmd_mod_add_match_tcp_dst_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=6",
                         "tcp_dst=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":6,\n"
      "\"tcp_dst\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_tcp_dst_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=6",
                         "tcp_dst=65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":6,\n"
      "\"tcp_dst\":65535,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* ldump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_tcp_dst_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=6",
                         "tcp_dst=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_tcp_dst_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=6",
                         "tcp_dst=65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_tcp_dst_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=6",
                         "tcp_dst=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* udp_src. */
void
test_flow_cmd_mod_add_match_udp_src_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=17",
                         "udp_src=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":17,\n"
      "\"udp_src\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_udp_src_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=17",
                         "udp_src=65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":17,\n"
      "\"udp_src\":65535,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_udp_src_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=17",
                         "udp_src=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_udp_src_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=17",
                         "udp_src=65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_udp_src_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=17",
                         "udp_src=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* udp_dst. */
void
test_flow_cmd_mod_add_match_udp_dst_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=17",
                         "udp_dst=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":17,\n"
      "\"udp_dst\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_udp_dst_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=17",
                         "udp_dst=65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":17,\n"
      "\"udp_dst\":65535,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* ldump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_udp_dst_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=17",
                         "udp_dst=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_udp_dst_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=17",
                         "udp_dst=65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_udp_dst_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=17",
                         "udp_dst=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* sctp_src. */
void
test_flow_cmd_mod_add_match_sctp_src_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=132",
                         "sctp_src=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":132,\n"
      "\"sctp_src\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_sctp_src_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=132",
                         "sctp_src=65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":132,\n"
      "\"sctp_src\":65535,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_sctp_src_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=132",
                         "sctp_src=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_sctp_src_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=132",
                         "sctp_src=65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_sctp_src_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=132",
                         "sctp_src=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* sctp_dst. */
void
test_flow_cmd_mod_add_match_sctp_dst_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=132",
                         "sctp_dst=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":132,\n"
      "\"sctp_dst\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_sctp_dst_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=132",
                         "sctp_dst=65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":132,\n"
      "\"sctp_dst\":65535,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* ldump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_sctp_dst_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=132",
                         "sctp_dst=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_sctp_dst_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=132",
                         "sctp_dst=65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_sctp_dst_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=132",
                         "sctp_dst=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* icmp_type. */
void
test_flow_cmd_mod_add_match_icmp_type_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=1",
                         "icmp_type=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":1,\n"
      "\"icmp_type\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_icmp_type_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=1",
                         "icmp_type=255",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":1,\n"
      "\"icmp_type\":255,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_icmp_type_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=1",
                         "icmp_type=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_icmp_type_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=1",
                         "icmp_type=256",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (256).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_icmp_type_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=1",
                         "icmp_type=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* icmp_code. */
void
test_flow_cmd_mod_add_match_icmp_code_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=1",
                         "icmp_code=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":1,\n"
      "\"icmp_code\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_icmp_code_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=1",
                         "icmp_code=255",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":1,\n"
      "\"icmp_code\":255,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_icmp_code_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=1",
                         "icmp_code=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_icmp_code_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=1",
                         "icmp_code=256",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (256).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_icmp_code_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=1",
                         "icmp_code=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* arp_op. */
void
test_flow_cmd_mod_add_match_arp_op_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_op=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"arp\",\n"
      "\"arp_op\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_arp_op_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_op=65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"arp\",\n"
      "\"arp_op\":65535,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_arp_op_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_op=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_arp_op_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_op=65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_arp_op_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_op=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* arp_spa. */
void
test_flow_cmd_mod_add_match_arp_spa_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_spa=127.0.0.1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"arp\",\n"
      "\"arp_spa\":\"127.0.0.1\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_arp_spa_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_spa=127.0.0.1/127.0.0.1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"arp\",\n"
      "\"arp_spa\":\"127.0.0.1\\/127.0.0.1\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_arp_spa_bad(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_spa=127.0.0.1000",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
      "\"data\":\"Bad value (127.0.0.1000).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_arp_spa_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_spa=127.0.0.1/127.0.0.1000",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
      "\"data\":\"Bad value (127.0.0.1000).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* arp_tpa. */
void
test_flow_cmd_mod_add_match_arp_tpa_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_tpa=127.0.0.1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"arp\",\n"
      "\"arp_tpa\":\"127.0.0.1\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_arp_tpa_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_tpa=127.0.0.1/127.0.0.1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"arp\",\n"
      "\"arp_tpa\":\"127.0.0.1\\/127.0.0.1\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_arp_tpa_bad(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_tpa=127.0.0.1000",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
      "\"data\":\"Bad value (127.0.0.1000).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_arp_tpa_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_tpa=127.0.0.1/127.0.0.1000",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
      "\"data\":\"Bad value (127.0.0.1000).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* arp_sha. */
void
test_flow_cmd_mod_add_match_arp_sha_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_sha=01:02:03:04:05:06",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"arp\",\n"
      "\"arp_sha\":\"01:02:03:04:05:06\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_arp_sha_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_sha=00:00:00:00:00:01/00:00:00:00:00:01",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"arp\",\n"
      "\"arp_sha\":\"00:00:00:00:00:01\\/00:00:00:00:00:01\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_arp_sha_bad(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_sha=01:02:03:04:05:0",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OUT_OF_RANGE\",\n"
      "\"data\":\"Bad value (01:02:03:04:05:0).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_arp_sha_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_sha=01:02:03:04:05:06/ff:fe:fX:fc:fb:fa",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OUT_OF_RANGE\",\n"
      "\"data\":\"Bad value (ff:fe:fX:fc:fb:fa).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* arp_tha. */
void
test_flow_cmd_mod_add_match_arp_tha_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_tha=01:02:03:04:05:06",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"arp\",\n"
      "\"arp_tha\":\"01:02:03:04:05:06\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_arp_tha_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_tha=00:00:00:00:00:01/00:00:00:00:00:01",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"arp\",\n"
      "\"arp_tha\":\"00:00:00:00:00:01\\/00:00:00:00:00:01\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_arp_tha_bad(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_tha=01:02:03:04:05:0",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OUT_OF_RANGE\",\n"
      "\"data\":\"Bad value (01:02:03:04:05:0).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_arp_tha_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2054",
                         "arp_tha=01:02:03:04:05:06/ff:fe:fX:fc:fb:fa",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OUT_OF_RANGE\",\n"
      "\"data\":\"Bad value (ff:fe:fX:fc:fb:fa).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* ipv6_src. */
void
test_flow_cmd_mod_add_match_ipv6_src_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_src=0:0:0:0:0:0:0:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ipv6\",\n"
      "\"ipv6_src\":\"::1\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_ipv6_src_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_src=::1/::1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ipv6\",\n"
      "\"ipv6_src\":\"::1\\/::1\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_ipv6_src_bad(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_src=0:0:0:0:0:0:0:122222",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
      "\"data\":\"Bad value (0:0:0:0:0:0:0:122222).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_ipv6_src_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_src=0:0:0:0:0:0:0:1/ff:fe:fd:fc:fb:fa:f9:fx",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
      "\"data\":\"Bad value (ff:fe:fd:fc:fb:fa:f9:fx).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* ipv6_dst. */
void
test_flow_cmd_mod_add_match_ipv6_dst_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_dst=0:0:0:0:0:0:0:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ipv6\",\n"
      "\"ipv6_dst\":\"::1\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_ipv6_dst_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_dst=::1/::1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ipv6\",\n"
      "\"ipv6_dst\":\"::1\\/::1\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_ipv6_dst_bad(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_dst=0:0:0:0:0:0:0:122222",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
      "\"data\":\"Bad value (0:0:0:0:0:0:0:122222).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_ipv6_dst_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_dst=0:0:0:0:0:0:0:1/ff:fe:fd:fc:fb:fa:f9:fx",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
      "\"data\":\"Bad value (ff:fe:fd:fc:fb:fa:f9:fx).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* ipv6_label. */
void
test_flow_cmd_mod_add_match_ipv6_label_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_label=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ipv6\",\n"
      "\"ipv6_label\":\"0\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_ipv6_label_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_label=1048575",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ipv6\",\n"
      "\"ipv6_label\":\"1048575\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_ipv6_label_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_label=1/0x1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ipv6\",\n"
      "\"ipv6_label\":\"1\\/0x00000001\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_ipv6_label_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_label=1048576",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (1048576).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_ipv6_label_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_label=1234/0x567V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x567V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_ipv6_label_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_label=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* icmpv6_type. */
void
test_flow_cmd_mod_add_match_icmpv6_type_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=58",
                         "icmpv6_type=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":58,\n"
      "\"icmpv6_type\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_icmpv6_type_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=58",
                         "icmpv6_type=255",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":58,\n"
      "\"icmpv6_type\":255,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_icmpv6_type_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=58",
                         "icmpv6_type=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_icmpv6_type_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=58",
                         "icmpv6_type=256",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (256).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_icmpv6_type_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=58",
                         "icmpv6_type=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* icmpv6_code. */
void
test_flow_cmd_mod_add_match_icmpv6_code_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=58",
                         "icmpv6_code=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":58,\n"
      "\"icmpv6_code\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_icmpv6_code_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=58",
                         "icmpv6_code=255",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ip\",\n"
      "\"nw_proto\":58,\n"
      "\"icmpv6_code\":255,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_icmpv6_code_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=58",
                         "icmpv6_code=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_icmpv6_code_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=58",
                         "icmpv6_code=256",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (256).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_icmpv6_code_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=2048",
                         "nw_proto=58",
                         "icmpv6_code=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* nd_target. */
void
test_flow_cmd_mod_add_match_nd_target_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "nw_proto=58",
                         "icmpv6_type=135",
                         "nd_target=0:0:0:0:0:0:0:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ipv6\",\n"
      "\"nw_proto\":58,\n"
      "\"icmpv6_type\":135,\n"
      "\"nd_target\":\"::1\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_nd_target_bad(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "nw_proto=58",
                         "icmpv6_type=135",
                         "nd_target=0:0:0:0:0:0:0:122222",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
      "\"data\":\"Bad value (0:0:0:0:0:0:0:122222).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_nd_target_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "nw_proto=58",
                         "icmpv6_type=135",
                         "nd_target=0:0:0:0:0:0:0:1/ff:fe:fd:fc:fb:fa:f9:f0",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* nd_sll. */
void
test_flow_cmd_mod_add_match_nd_sll_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "nw_proto=58",
                         "icmpv6_type=135",
                         "nd_sll=01:02:03:04:05:06",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ipv6\",\n"
      "\"nw_proto\":58,\n"
      "\"icmpv6_type\":135,\n"
      "\"nd_sll\":\"01:02:03:04:05:06\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_nd_sll_bad(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "nw_proto=58",
                         "icmpv6_type=135",
                         "nd_sll=01:02:03:04:05:0",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OUT_OF_RANGE\",\n"
      "\"data\":\"Bad value (01:02:03:04:05:0).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_nd_sll_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "nw_proto=58",
                         "icmpv6_type=135",
                         "nd_sll=01:02:03:04:05:06/ff:fe:fX:fc:fb:f0",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* nd_tll. */
void
test_flow_cmd_mod_add_match_nd_tll_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "nw_proto=58",
                         "icmpv6_type=135",
                         "nd_tll=01:02:03:04:05:06",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ipv6\",\n"
      "\"nw_proto\":58,\n"
      "\"icmpv6_type\":135,\n"
      "\"nd_tll\":\"01:02:03:04:05:06\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_nd_tll_bad(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "nw_proto=58",
                         "icmpv6_type=135",
                         "nd_tll=01:02:03:04:05:0",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OUT_OF_RANGE\",\n"
      "\"data\":\"Bad value (01:02:03:04:05:0).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_nd_tll_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "nw_proto=58",
                         "icmpv6_type=135",
                         "nd_tll=01:02:03:04:05:06/ff:fe:fX:fc:fb:f0",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_label. */
void
test_flow_cmd_mod_add_match_mpls_label_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34887",
                         "mpls_label=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"mpls\",\n"
      "\"mpls_label\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_label_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34887",
                         "mpls_label=1048575",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"mpls\",\n"
      "\"mpls_label\":1048575,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_label_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34887",
                         "mpls_label=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_label_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34887",
                         "mpls_label=1048576",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (1048576).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_label_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34887",
                         "mpls_label=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_tc. */
void
test_flow_cmd_mod_add_match_mpls_tc_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34887",
                         "mpls_tc=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"mpls\",\n"
      "\"mpls_tc\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_tc_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34887",
                         "mpls_tc=7",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"mpls\",\n"
      "\"mpls_tc\":7,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_tc_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34887",
                         "mpls_tc=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_tc_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34887",
                         "mpls_tc=8",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (8).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_tc_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34887",
                         "mpls_tc=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_bos. */
void
test_flow_cmd_mod_add_match_mpls_bos_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34887",
                         "mpls_bos=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"mpls\",\n"
      "\"mpls_bos\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_bos_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34887",
                         "mpls_bos=1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"mpls\",\n"
      "\"mpls_bos\":1,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_bos_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34887",
                         "mpls_bos=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_bos_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34887",
                         "mpls_bos=2",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (2).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_bos_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34887",
                         "mpls_bos=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* pbb_isid. */
void
test_flow_cmd_mod_add_match_pbb_isid_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=35047",
                         "pbb_isid=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"pbb\",\n"
      "\"pbb_isid\":\"0\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_pbb_isid_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=35047",
                         "pbb_isid=16777215",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"pbb\",\n"
      "\"pbb_isid\":\"16777215\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_pbb_isid_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=35047",
                         "pbb_isid=1/0x1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"pbb\",\n"
      "\"pbb_isid\":\"1\\/0x000001\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_pbb_isid_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=35047",
                         "pbb_isid=16777216",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (16777216).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_pbb_isid_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=35047",
                         "pbb_isid=1234/0x567V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x567V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_pbb_isid_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=35047",
                         "pbb_isid=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* tunnel_id. */
void
test_flow_cmd_mod_add_match_tunnel_id_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "tunnel_id=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"tunnel_id\":\"0\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_tunnel_id_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "tunnel_id=18446744073709551615",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"tunnel_id\":\"18446744073709551615\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_tunnel_id_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "tunnel_id=1/0x1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"tunnel_id\":\"1\\/0x0000000000000001\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_tunnel_id_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "tunnel_id=1234/0x567V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x567V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_tunnel_id_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "tunnel_id=18446744073709551616",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OUT_OF_RANGE\",\n"
      "\"data\":\"Bad value (18446744073709551616).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_tunnel_id_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "tunnel_id=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* ipv6_exthdr. */
void
test_flow_cmd_mod_add_match_ipv6_exthdr_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_exthdr=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ipv6\",\n"
      "\"ipv6_exthdr\":\"0\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_ipv6_exthdr_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_exthdr=511",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ipv6\",\n"
      "\"ipv6_exthdr\":\"511\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_ipv6_exthdr_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_exthdr=1/0x1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"dl_type\":\"ipv6\",\n"
      "\"ipv6_exthdr\":\"1\\/0x01\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_ipv6_exthdr_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_exthdr=512",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (512).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_ipv6_exthdr_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_exthdr=123/0x56V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x56V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_ipv6_exthdr_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "dl_type=34525",
                         "ipv6_exthdr=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* pbb_uca. */
void
test_flow_cmd_mod_add_match_pbb_uca_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "pbb_uca=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"pbb_uca\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_pbb_uca_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "pbb_uca=1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"pbb_uca\":1,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_pbb_uca_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "pbb_uca=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_pbb_uca_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "pbb_uca=2",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (2).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_pbb_uca_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "pbb_uca=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* packet_type. */
void
test_flow_cmd_mod_add_match_packet_type_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "packet_type=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"packet_type\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_packet_type_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "packet_type=4294967295",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"packet_type\":4294967295,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_packet_type_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "packet_type=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_packet_type_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "packet_type=4294967296",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (4294967296).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_packet_type_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "packet_type=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* gre_flags. */
void
test_flow_cmd_mod_add_match_gre_flags_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_flags=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"gre_flags\":\"0\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_gre_flags_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_flags=8191",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"gre_flags\":\"8191\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_gre_flags_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_flags=1/0x1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"gre_flags\":\"1\\/0x01\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_gre_flags_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_flags=1/0x123V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x123V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_gre_flags_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_flags=8192",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (8192).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_gre_flags_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_flags=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* gre_ver. */
void
test_flow_cmd_mod_add_match_gre_ver_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_ver=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"gre_ver\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_gre_ver_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_ver=7",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"gre_ver\":7,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_gre_ver_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_ver=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_gre_ver_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_ver=8",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (8).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_gre_ver_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_ver=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* gre_protocol. */
void
test_flow_cmd_mod_add_match_gre_protocol_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_protocol=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"gre_protocol\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_gre_protocol_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_protocol=65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"gre_protocol\":65535,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_gre_protocol_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_protocol=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_gre_protocol_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_protocol=65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_gre_protocol_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_protocol=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* gre_key. */
void
test_flow_cmd_mod_add_match_gre_key_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_key=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"gre_key\":\"0\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_gre_key_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_key=4294967295",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"gre_key\":\"4294967295\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_gre_key_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_key=1/0x1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"gre_key\":\"1\\/0x00000001\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_gre_key_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_key=4294967296",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (4294967296).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_gre_key_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_key=1234/0x567V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x567V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_gre_key_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_key=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* gre_seqnum. */
void
test_flow_cmd_mod_add_match_gre_seqnum_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_seqnum=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"gre_seqnum\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_gre_seqnum_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_seqnum=4294967295",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"gre_seqnum\":4294967295,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_gre_seqnum_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_seqnum=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_gre_seqnum_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_seqnum=4294967296",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (4294967296).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_gre_seqnum_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "gre_seqnum=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* lisp_flags. */
void
test_flow_cmd_mod_add_match_lisp_flags_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_flags=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"lisp_flags\":\"0\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_lisp_flags_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_flags=255",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"lisp_flags\":\"255\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_lisp_flags_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_flags=1/0x1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"lisp_flags\":\"1\\/0x01\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_lisp_flags_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_flags=256",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (256).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_lisp_flags_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_flags=123/0x56V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x56V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_lisp_flags_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_flags=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* lisp_nonce. */
void
test_flow_cmd_mod_add_match_lisp_nonce_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_nonce=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"lisp_nonce\":\"0\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_lisp_nonce_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_nonce=16777215",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"lisp_nonce\":\"16777215\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_lisp_nonce_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_nonce=1/0x1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"lisp_nonce\":\"1\\/0x000001\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_lisp_nonce_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_nonce=16777216",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (16777216).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_lisp_nonce_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_nonce=1234/0x567V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x567V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_lisp_nonce_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_nonce=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* lisp_id. */
void
test_flow_cmd_mod_add_match_lisp_id_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_id=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"lisp_id\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_lisp_id_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_id=4294967295",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"lisp_id\":4294967295,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_lisp_id_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_id=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_lisp_id_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_id=4294967296",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (4294967296).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_lisp_id_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "lisp_id=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* vxlan_flags. */
void
test_flow_cmd_mod_add_match_vxlan_flags_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vxlan_flags=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"vxlan_flags\":\"0\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_vxlan_flags_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vxlan_flags=255",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"vxlan_flags\":\"255\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_vxlan_flags_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vxlan_flags=1/0x1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"vxlan_flags\":\"1\\/0x01\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_vxlan_flags_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vxlan_flags=256",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (256).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_vxlan_flags_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vxlan_flags=123/0x56V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x56V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_vxlan_flags_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vxlan_flags=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* vxlan_vni. */
void
test_flow_cmd_mod_add_match_vxlan_vni_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vxlan_vni=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"vxlan_vni\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_vxlan_vni_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vxlan_vni=16777215",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"vxlan_vni\":16777215,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_vxlan_vni_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vxlan_vni=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_vxlan_vni_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vxlan_vni=16777216",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (16777216).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_vxlan_vni_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "vxlan_vni=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_data_first_nibble. */
void
test_flow_cmd_mod_add_match_mpls_data_first_nibble_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_data_first_nibble=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_data_first_nibble\":\"0\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_data_first_nibble_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_data_first_nibble=15",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_data_first_nibble\":\"15\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_data_first_nibble_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_data_first_nibble=2/0x3",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_data_first_nibble\":\"2\\/0x03\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_data_first_nibble_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_data_first_nibble=16",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (16).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_data_first_nibble_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_data_first_nibble=2/0x3V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x3V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_data_first_nibble_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_data_first_nibble=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_ach_version. */
void
test_flow_cmd_mod_add_match_mpls_ach_version_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_ach_version=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_ach_version\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_ach_version_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_ach_version=15",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_ach_version\":15,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_ach_version_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_ach_version=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_ach_version_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_ach_version=16",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (16).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_ach_version_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_ach_version=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_ach_channel. */
void
test_flow_cmd_mod_add_match_mpls_ach_channel_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_ach_channel=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_ach_channel\":\"0\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_ach_channel_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_ach_channel=65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_ach_channel\":\"65535\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_ach_channel_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_ach_channel=1/0x1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_ach_channel\":\"1\\/0x01\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_ach_channel_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_ach_channel=1/0x123V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x123V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_ach_channel_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_ach_channel=65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_ach_channel_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_ach_channel=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_pw_metadata. */
void
test_flow_cmd_mod_add_match_mpls_pw_metadata_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_pw_metadata=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_pw_metadata\":\"0\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_pw_metadata_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_pw_metadata=1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_pw_metadata\":\"1\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_pw_metadata_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_pw_metadata=0/0x1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_pw_metadata\":\"0\\/0x01\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_pw_metadata_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_pw_metadata=2",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (2).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_pw_metadata_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_pw_metadata=0/0x1V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x1V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_pw_metadata_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_pw_metadata=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_cw_flags. */
void
test_flow_cmd_mod_add_match_mpls_cw_flags_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_flags=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_cw_flags\":\"0\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_cw_flags_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_flags=15",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_cw_flags\":\"15\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_cw_flags_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_flags=2/0x3",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_cw_flags\":\"2\\/0x03\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_cw_flags_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_flags=16",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (16).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_cw_flags_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_flags=2/0x3V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x3V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_cw_flags_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_flags=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_cw_frag. */
void
test_flow_cmd_mod_add_match_mpls_cw_frag_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_frag=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_cw_frag\":\"0\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_cw_frag_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_frag=3",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_cw_frag\":\"3\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_cw_frag_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_frag=2/0x3",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_cw_frag\":\"2\\/0x03\",\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_cw_frag_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_frag=4",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (4).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_cw_frag_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_frag=2/0x3V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x3V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_cw_frag_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_frag=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_cw_len. */
void
test_flow_cmd_mod_add_match_mpls_cw_len_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_len=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_cw_len\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_cw_len_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_len=63",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_cw_len\":63,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_cw_len_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_len=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_cw_len_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_len=64",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (64).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_cw_len_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_len=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_cw_seq_num. */
void
test_flow_cmd_mod_add_match_mpls_cw_seq_num_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_seq_num=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_cw_seq_num\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_cw_seq_num_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_seq_num=65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"mpls_cw_seq_num\":65535,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_mpls_cw_seq_num_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_seq_num=1/0x12",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_cw_seq_num_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_seq_num=65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_mpls_cw_seq_num_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "mpls_cw_seq_num=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/*
 * apply actions.
 */

/*
 * set field.
 */

/* in_port. */
void
test_flow_cmd_mod_add_apply_actions_in_port_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=in_port:1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"NOT_FOUND\",\n"
      "\"data\":\"Not found cmd (in_port).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* in_phy_port. */
void
test_flow_cmd_mod_add_apply_actions_in_phy_port_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=in_phy_port:1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"NOT_FOUND\",\n"
      "\"data\":\"Not found cmd (in_phy_port).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* metadata. */
void
test_flow_cmd_mod_add_apply_actions_metadata_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=metadata:1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"NOT_FOUND\",\n"
      "\"data\":\"Not found cmd (metadata).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* dl_dst. */
void
test_flow_cmd_mod_add_apply_actions_dl_dst_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=dl_dst:01:02:03:04:05:06",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"dl_dst\":\"01:02:03:04:05:06\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_dl_dst_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=dl_dst:01:02:03:04:05:06/ff:fe:fd:fc:fb:fa",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* dl_src. */
void
test_flow_cmd_mod_add_apply_actions_dl_src_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=dl_src:01:02:03:04:05:06",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"dl_src\":\"01:02:03:04:05:06\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_dl_src_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=dl_src:01:02:03:04:05:06/ff:fe:fd:fc:fb:fa",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* dl_type. */
void
test_flow_cmd_mod_add_apply_actions_dl_type_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=dl_type:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"dl_type\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_dl_type_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=dl_type:vlan",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"dl_type\":\"vlan\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_dl_type_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=dl_type:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* vlan_vid. */
void
test_flow_cmd_mod_add_apply_actions_vlan_vid_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=vlan_vid:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"vlan_vid\":4097}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_vlan_vid_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=vlan_vid:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* vlan_pcp. */
void
test_flow_cmd_mod_add_apply_actions_vlan_pcp_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=vlan_pcp:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"vlan_pcp\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_vlan_pcp_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=vlan_pcp:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* ip_dscp. */
void
test_flow_cmd_mod_add_apply_actions_ip_dscp_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=ip_dscp:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"ip_dscp\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_ip_dscp_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=ip_dscp:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* nw_ecn. */
void
test_flow_cmd_mod_add_apply_actions_nw_ecn_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=nw_ecn:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"nw_ecn\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_nw_ecn_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=nw_ecn:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* nw_proto. */
void
test_flow_cmd_mod_add_apply_actions_nw_proto_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=nw_proto:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"nw_proto\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_nw_proto_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=nw_proto:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* nw_src. */
void
test_flow_cmd_mod_add_apply_actions_nw_src_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=nw_src:127.0.0.1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"nw_src\":\"127.0.0.1\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_nw_src_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=nw_src:127.0.0.1/255:254:253:252",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* nw_dst. */
void
test_flow_cmd_mod_add_apply_actions_nw_dst_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=nw_dst:127.0.0.1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"nw_dst\":\"127.0.0.1\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_nw_dst_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=nw_dst:127.0.0.1/255:254:253:252",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* tcp_src. */
void
test_flow_cmd_mod_add_apply_actions_tcp_src_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=tcp_src:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"tcp_src\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_tcp_src_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=tcp_src:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* tcp_dst. */
void
test_flow_cmd_mod_add_apply_actions_tcp_dst_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=tcp_dst:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"tcp_dst\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_tcp_dst_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=tcp_dst:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* udp_src. */
void
test_flow_cmd_mod_add_apply_actions_udp_src_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=udp_src:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"udp_src\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_udp_src_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=udp_src:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* udp_dst. */
void
test_flow_cmd_mod_add_apply_actions_udp_dst_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=udp_dst:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"udp_dst\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_udp_dst_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=udp_dst:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* sctp_src. */
void
test_flow_cmd_mod_add_apply_actions_sctp_src_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=sctp_src:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"sctp_src\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_sctp_src_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=sctp_src:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* sctp_dst. */
void
test_flow_cmd_mod_add_apply_actions_sctp_dst_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=sctp_dst:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"sctp_dst\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_sctp_dst_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=sctp_dst:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* icmp_type. */
void
test_flow_cmd_mod_add_apply_actions_icmp_type_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=icmp_type:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"icmp_type\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_icmp_type_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=icmp_type:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* icmp_code. */
void
test_flow_cmd_mod_add_apply_actions_icmp_code_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=icmp_code:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"icmp_code\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_icmp_code_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=icmp_code:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* arp_op. */
void
test_flow_cmd_mod_add_apply_actions_arp_op_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=arp_op:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"arp_op\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_arp_op_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=arp_op:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* arp_spa. */
void
test_flow_cmd_mod_add_apply_actions_arp_spa_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=arp_spa:127.0.0.1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"arp_spa\":\"127.0.0.1\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_arp_spa_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=arp_spa:127.0.0.1/255:254:253:252",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* arp_tpa. */
void
test_flow_cmd_mod_add_apply_actions_arp_tpa_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=arp_tpa:127.0.0.1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"arp_tpa\":\"127.0.0.1\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_arp_tpa_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=arp_tpa:127.0.0.1/255:254:253:252",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* arp_sha. */
void
test_flow_cmd_mod_add_apply_actions_arp_sha_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=arp_sha:01:02:03:04:05:06",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"arp_sha\":\"01:02:03:04:05:06\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_arp_sha_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=arp_sha:01:02:03:04:05:06/ff:fe:fd:fc:fb:fa",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* arp_tha. */
void
test_flow_cmd_mod_add_apply_actions_arp_tha_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=arp_tha:01:02:03:04:05:06",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"arp_tha\":\"01:02:03:04:05:06\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_arp_tha_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=arp_tha:01:02:03:04:05:06/ff:fe:fd:fc:fb:fa",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* ipv6_src. */
void
test_flow_cmd_mod_add_apply_actions_ipv6_src_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=ipv6_src:0:0:0:0:0:0:0:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"ipv6_src\":\"::1\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_ipv6_src_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=ipv6_src:0:0:0:0:0:0:0:1/ff:fe:fd:fc:fb:fa:f9:f8",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* ipv6_dst. */
void
test_flow_cmd_mod_add_apply_actions_ipv6_dst_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=ipv6_dst:0:0:0:0:0:0:0:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"ipv6_dst\":\"::1\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_ipv6_dst_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=ipv6_dst:0:0:0:0:0:0:0:1/ff:fe:fd:fc:fb:fa:f9:f8",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* ipv6_label. */
void
test_flow_cmd_mod_add_apply_actions_ipv6_label_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=ipv6_label:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"ipv6_label\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_ipv6_label_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=ipv6_label:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* icmpv6_type. */
void
test_flow_cmd_mod_add_apply_actions_icmpv6_type_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=icmpv6_type:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"icmpv6_type\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_icmpv6_type_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=icmpv6_type:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* icmpv6_code. */
void
test_flow_cmd_mod_add_apply_actions_icmpv6_code_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=icmpv6_code:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"icmpv6_code\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_icmpv6_code_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=icmpv6_code:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* nd_target. */
void
test_flow_cmd_mod_add_apply_actions_nd_target_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=nd_target:0:0:0:0:0:0:0:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"nd_target\":\"::1\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_nd_target_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=nd_target:0:0:0:0:0:0:0:1/ff:fe:fd:fc:fb:fa:f9:f8",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* nd_sll. */
void
test_flow_cmd_mod_add_apply_actions_nd_sll_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=nd_sll:01:02:03:04:05:06",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"nd_sll\":\"01:02:03:04:05:06\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_nd_sll_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=nd_sll:01:02:03:04:05:06/ff:fe:fd:fc:fb:fa",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* nd_tll. */
void
test_flow_cmd_mod_add_apply_actions_nd_tll_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=nd_tll:01:02:03:04:05:06",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"nd_tll\":\"01:02:03:04:05:06\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_nd_tll_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=nd_tll:01:02:03:04:05:06/ff:fe:fd:fc:fb:fa",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_label. */
void
test_flow_cmd_mod_add_apply_actions_mpls_label_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_label:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"mpls_label\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_mpls_label_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_label:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_tc. */
void
test_flow_cmd_mod_add_apply_actions_mpls_tc_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_tc:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"mpls_tc\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_mpls_tc_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_tc:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_bos. */
void
test_flow_cmd_mod_add_apply_actions_mpls_bos_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_bos:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"mpls_bos\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_mpls_bos_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_bos:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* pbb_isid. */
void
test_flow_cmd_mod_add_apply_actions_pbb_isid_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=pbb_isid:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"pbb_isid\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_pbb_isid_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=pbb_isid:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* tunnel_id. */
void
test_flow_cmd_mod_add_apply_actions_tunnel_id_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=tunnel_id:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"tunnel_id\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_tunnel_id_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=tunnel_id:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* ipv6_exthdr. */
void
test_flow_cmd_mod_add_apply_actions_ipv6_exthdr_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=ipv6_exthdr:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"ipv6_exthdr\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_ipv6_exthdr_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=ipv6_exthdr:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* pbb_uca. */
void
test_flow_cmd_mod_add_apply_actions_pbb_uca_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=pbb_uca:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"pbb_uca\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_pbb_uca_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=pbb_uca:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* packet_type. */
void
test_flow_cmd_mod_add_apply_actions_packet_type_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=packet_type:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"packet_type\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_packet_type_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=packet_type:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* gre_flags. */
void
test_flow_cmd_mod_add_apply_actions_gre_flags_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=gre_flags:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"gre_flags\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_gre_flags_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=gre_flags:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* gre_ver. */
void
test_flow_cmd_mod_add_apply_actions_gre_ver_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=gre_ver:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"gre_ver\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_gre_ver_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=gre_ver:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* gre_protocol. */
void
test_flow_cmd_mod_add_apply_actions_gre_protocol_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=gre_protocol:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"gre_protocol\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_gre_protocol_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=gre_protocol:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* gre_key. */
void
test_flow_cmd_mod_add_apply_actions_gre_key_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=gre_key:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"gre_key\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_gre_key_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=gre_key:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* gre_seqnum. */
void
test_flow_cmd_mod_add_apply_actions_gre_seqnum_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=gre_seqnum:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"gre_seqnum\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_gre_seqnum_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=gre_seqnum:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* lisp_flags. */
void
test_flow_cmd_mod_add_apply_actions_lisp_flags_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=lisp_flags:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"lisp_flags\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_lisp_flags_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=lisp_flags:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* lisp_nonce. */
void
test_flow_cmd_mod_add_apply_actions_lisp_nonce_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=lisp_nonce:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"lisp_nonce\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_lisp_nonce_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=lisp_nonce:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* lisp_id. */
void
test_flow_cmd_mod_add_apply_actions_lisp_id_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=lisp_id:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"lisp_id\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_lisp_id_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=lisp_id:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* vxlan_flags. */
void
test_flow_cmd_mod_add_apply_actions_vxlan_flags_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=vxlan_flags:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"vxlan_flags\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_vxlan_flags_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=vxlan_flags:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* vxlan_vni. */
void
test_flow_cmd_mod_add_apply_actions_vxlan_vni_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=vxlan_vni:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"vxlan_vni\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_vxlan_vni_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=vxlan_vni:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_data_first_nibble. */
void
test_flow_cmd_mod_add_apply_actions_mpls_data_first_nibble_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_data_first_nibble:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"mpls_data_first_nibble\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_mpls_data_first_nibble_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_data_first_nibble:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_ach_version. */
void
test_flow_cmd_mod_add_apply_actions_mpls_ach_version_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_ach_version:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"mpls_ach_version\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_mpls_ach_version_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_ach_version:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_ach_channel. */
void
test_flow_cmd_mod_add_apply_actions_mpls_ach_channel_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_ach_channel:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"mpls_ach_channel\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_mpls_ach_channel_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_ach_channel:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_pw_metadata. */
void
test_flow_cmd_mod_add_apply_actions_mpls_pw_metadata_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_pw_metadata:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"mpls_pw_metadata\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_mpls_pw_metadata_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_pw_metadata:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_cw_flags. */
void
test_flow_cmd_mod_add_apply_actions_mpls_cw_flags_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_cw_flags:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"mpls_cw_flags\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_mpls_cw_flags_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_cw_flags:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_cw_frag. */
void
test_flow_cmd_mod_add_apply_actions_mpls_cw_frag_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_cw_frag:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"mpls_cw_frag\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_mpls_cw_frag_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_cw_frag:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_cw_len. */
void
test_flow_cmd_mod_add_apply_actions_mpls_cw_len_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_cw_len:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"mpls_cw_len\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_mpls_cw_len_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_cw_len:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* mpls_cw_seq_num. */
void
test_flow_cmd_mod_add_apply_actions_mpls_cw_seq_num_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_cw_seq_num:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"mpls_cw_seq_num\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_mpls_cw_seq_num_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=mpls_cw_seq_num:0/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* output. */
void
test_flow_cmd_mod_add_apply_actions_output_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=output:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"output\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_output_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=output:controller",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"output\":\"controller\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_output_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=output:1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_output_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=output:4294967296",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (4294967296).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_output_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=output:hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* copy_ttl_out. */
void
test_flow_cmd_mod_add_apply_actions_copy_ttl_out_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=copy_ttl_out",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"copy_ttl_out\":null}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

/* copy_ttl_in. */
void
test_flow_cmd_mod_add_apply_actions_copy_ttl_in_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=copy_ttl_in",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"copy_ttl_in\":null}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

/* set_mpls_ttl. */
void
test_flow_cmd_mod_add_apply_actions_set_mpls_ttl_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=set_mpls_ttl:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"set_mpls_ttl\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_set_mpls_ttl_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=set_mpls_ttl:255",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"set_mpls_ttl\":255}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}


void
test_flow_cmd_mod_add_apply_actions_set_mpls_ttl_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=set_mpls_ttl:1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_set_mpls_ttl_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=set_mpls_ttl:256",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (256).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_set_mpls_ttl_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=set_mpls_ttl:hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* dec_mpls_ttl. */
void
test_flow_cmd_mod_add_apply_actions_dec_mpls_ttl_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=dec_mpls_ttl",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"dec_mpls_ttl\":null}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

/* push_vlan. */
void
test_flow_cmd_mod_add_apply_actions_push_vlan_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=push_vlan:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"push_vlan\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_push_vlan_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=push_vlan:65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"push_vlan\":65535}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_push_vlan_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=push_vlan:1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_push_vlan_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=push_vlan:65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_push_vlan_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=push_vlan:hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* strip_vlan. */
void
test_flow_cmd_mod_add_apply_actions_strip_vlan_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=strip_vlan",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"strip_vlan\":null}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

/* push_mpls. */
void
test_flow_cmd_mod_add_apply_actions_push_mpls_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=push_mpls:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"push_mpls\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_push_mpls_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=push_mpls:65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"push_mpls\":65535}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_push_mpls_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=push_mpls:1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_push_mpls_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=push_mpls:65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_push_mpls_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=push_mpls:hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* pop_mpls. */
void
test_flow_cmd_mod_add_apply_actions_pop_mpls_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=pop_mpls:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"pop_mpls\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_pop_mpls_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=pop_mpls:65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"pop_mpls\":65535}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_pop_mpls_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=pop_mpls:1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_pop_mpls_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=pop_mpls:65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_pop_mpls_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=pop_mpls:hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* set_queue. */
void
test_flow_cmd_mod_add_apply_actions_set_queue_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=set_queue:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"set_queue\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_set_queue_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=set_queue:4294967295",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"set_queue\":4294967295}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_set_queue_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=set_queue:1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_set_queue_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=set_queue:4294967296",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (4294967296).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_set_queue_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=set_queue:hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* group. */
void
test_flow_cmd_mod_add_apply_actions_group_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  uint64_t dpid = 1;
  struct bucket_list bucket_list;
  struct ofp_group_mod group_mod;
  struct ofp_error error;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=group:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"group\":1}]}]}]}]}";

  /* group_mod (ADD). */
  create_buckets(&bucket_list);
  memset(&group_mod, 0, sizeof(group_mod));
  group_mod.group_id = 1;
  group_mod.type = OFPGT_ALL;
  TEST_ASSERT_GROUP_ADD(ret, dpid, &group_mod,
                        &bucket_list, &error);

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);

  /* group_mod (DEL). */
  TEST_ASSERT_GROUP_DEL(ret, dpid, &group_mod, &error);
}

void
test_flow_cmd_mod_add_apply_actions_group_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  uint64_t dpid = 1;
  struct bucket_list bucket_list;
  struct ofp_group_mod group_mod;
  struct ofp_error error;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=group:4294967295",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"group\":\"any\"}]}]}]}]}";

  /* group_mod (ADD). */
  create_buckets(&bucket_list);
  memset(&group_mod, 0, sizeof(group_mod));
  group_mod.group_id = 4294967295;
  group_mod.type = OFPGT_ALL;
  TEST_ASSERT_GROUP_ADD(ret, dpid, &group_mod,
                        &bucket_list, &error);

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);

  /* group_mod (DEL). */
  TEST_ASSERT_GROUP_DEL(ret, dpid, &group_mod, &error);
}

void
test_flow_cmd_mod_add_apply_actions_group_03(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  uint64_t dpid = 1;
  struct bucket_list bucket_list;
  struct ofp_group_mod group_mod;
  struct ofp_error error;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=group:all",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"group\":\"all\"}]}]}]}]}";

  /* group_mod (ADD). */
  create_buckets(&bucket_list);
  memset(&group_mod, 0, sizeof(group_mod));
  group_mod.group_id = OFPG_ALL;
  group_mod.type = OFPGT_ALL;
  TEST_ASSERT_GROUP_ADD(ret, dpid, &group_mod,
                        &bucket_list, &error);

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);

  /* group_mod (DEL). */
  TEST_ASSERT_GROUP_DEL(ret, dpid, &group_mod, &error);
}

void
test_flow_cmd_mod_add_apply_actions_group_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=group:1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_group_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=group:4294967296",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (4294967296).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_group_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=group:hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* set_nw_ttl. */
void
test_flow_cmd_mod_add_apply_actions_set_nw_ttl_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=set_nw_ttl:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"set_nw_ttl\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_set_nw_ttl_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=set_nw_ttl:255",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"set_nw_ttl\":255}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_set_nw_ttl_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=set_nw_ttl:1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_set_nw_ttl_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=set_nw_ttl:256",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (256).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_set_nw_ttl_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=set_nw_ttl:hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* dec_nw_ttl. */
void
test_flow_cmd_mod_add_apply_actions_dec_nw_ttl_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=dec_nw_ttl",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"dec_nw_ttl\":null}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

/* push_pbb. */
void
test_flow_cmd_mod_add_apply_actions_push_pbb_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=push_pbb:1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"push_pbb\":1}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_push_pbb_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=push_pbb:65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"push_pbb\":65535}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_push_pbb_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=push_pbb:1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_push_pbb_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=push_pbb:65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_push_pbb_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=push_pbb:hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* pop_pbb. */
void
test_flow_cmd_mod_add_apply_actions_pop_pbb_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "apply_actions=pop_pbb",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"pop_pbb\":null}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

/* cookie. */
void
test_flow_cmd_mod_add_match_cookie_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "cookie=0",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_cookie_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "cookie=18446744073709551615",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":18446744073709551615,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_cookie_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "cookie=1234/0x5678",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":1234,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_match_cookie_badmask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "cookie=1234/0x567V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x567V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_cookie_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "cookie=18446744073709551616",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OUT_OF_RANGE\",\n"
      "\"data\":\"Bad value (18446744073709551616).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_match_cookie_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "cookie=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* table. */
void
test_flow_cmd_mod_add_apply_actions_table_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "table=1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[]},\n"
      "{\"table\":1,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_table_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "table=254",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[]},\n"
      "{\"table\":254,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_table_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "table=1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_table_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "table=256",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (256).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_table_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "table=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* idle_timeout. */
void
test_flow_cmd_mod_add_apply_actions_idle_timeout_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "idle_timeout=1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":1,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_idle_timeout_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "idle_timeout=65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":65535,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_idle_timeout_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "idle_timeout=1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_idle_timeout_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "idle_timeout=65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_idle_timeout_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "idle_timeout=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* hard_timeout. */
void
test_flow_cmd_mod_add_apply_actions_hard_timeout_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "hard_timeout=1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":1,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_hard_timeout_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "hard_timeout=65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":65535,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_hard_timeout_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "hard_timeout=1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_hard_timeout_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "hard_timeout=65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_hard_timeout_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "hard_timeout=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* priority. */
void
test_flow_cmd_mod_add_apply_actions_priority_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "priority=1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":1,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_priority_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "priority=65535",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":65535,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_priority_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "priority=1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_priority_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "priority=65536",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (65536).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_priority_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "priority=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* out_port. */
void
test_flow_cmd_mod_add_apply_actions_out_port_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "out_port=1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_out_port_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "out_port=4294967295",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_out_port_03(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "out_port=controller",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_out_port_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "out_port=1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_out_port_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "out_port=4294967296",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (4294967296).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_out_port_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "out_port=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* out_group. */
void
test_flow_cmd_mod_add_apply_actions_out_group_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "out_group=1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_out_group_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "out_group=4294967295",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_out_group_03(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "out_group=all",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_apply_actions_out_group_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "out_group=1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_out_group_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "out_group=4294967296",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (4294967296).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_apply_actions_out_group_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "out_group=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* send_flow_rem. */
void
test_flow_cmd_mod_add_apply_actions_send_flow_rem_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "send_flow_rem",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"send_flow_rem\":null,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

/* check_overlap. */
void
test_flow_cmd_mod_add_apply_actions_check_overlap_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "send_flow_rem",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"send_flow_rem\":null,\n"
      "\"cookie\":0,\n"
      "\"actions\":[]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

/* goto_table. */
void
test_flow_cmd_mod_add_goto_table_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "goto_table=1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"goto_table\":1}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_goto_table_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "goto_table=254",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"goto_table\":254}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_goto_table_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "goto_table=1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_goto_table_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "goto_table=255",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (255).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_goto_table_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "goto_table=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* clear_actions. */
void
test_flow_cmd_mod_add_clear_actions_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "clear_actions",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"clear_actions\":null}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

/* write_metadata. */
void
test_flow_cmd_mod_add_write_metadata_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "write_metadata=1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"write_metadata\":\"1\"}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_write_metadata_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "write_metadata=18446744073709551615",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"write_metadata\":\"18446744073709551615\"}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_write_metadata_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "write_metadata=1/0x1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"write_metadata\":\"1\\/0x0000000000000001\"}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_write_metadata_bad_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "write_metadata=1/0x123V",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (0x123V).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_write_metadata_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "write_metadata=18446744073709551616",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OUT_OF_RANGE\",\n"
      "\"data\":\"Bad value (18446744073709551616).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_write_metadata_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "write_metadata=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* meter. */
void
test_flow_cmd_mod_add_meter_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  uint64_t dpid = 1;
  struct ofp_meter_mod meter_mod;
  struct meter_band_list band_list;
  struct ofp_error error;
  const char *argv1[] = {"flow", "b1", "add",
                         "meter=1",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"meter\":1}]}]}]}";

  /* meter_mod (ADD). */
  create_bands(&band_list);
  memset(&meter_mod, 0, sizeof(meter_mod));
  meter_mod.meter_id = 1;
  meter_mod.command = OFPMC_ADD;
  TEST_ASSERT_METER_ADD(ret, dpid, &meter_mod,
                        &band_list, &error);

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);

  /* meter_mod (DEL). */
  TEST_ASSERT_METER_DEL(ret, dpid, &meter_mod, &error);
}

void
test_flow_cmd_mod_add_meter_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  uint64_t dpid = 1;
  struct ofp_meter_mod meter_mod;
  struct meter_band_list band_list;
  struct ofp_error error;
  const char *argv1[] = {"flow", "b1", "add",
                         "meter=4294967295",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"actions\":[{\"meter\":4294967295}]}]}]}";

  /* meter_mod (ADD). */
  create_bands(&band_list);
  memset(&meter_mod, 0, sizeof(meter_mod));
  meter_mod.meter_id = 4294967295;
  meter_mod.command = OFPMC_ADD;
  TEST_ASSERT_METER_ADD(ret, dpid, &meter_mod,
                        &band_list, &error);

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);

  /* meter_mod (DEL). */
  TEST_ASSERT_METER_DEL(ret, dpid, &meter_mod, &error);
}

void
test_flow_cmd_mod_add_meter_mask(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "meter=1/0x1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad mask.\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_meter_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "meter=4294967296",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"TOO_LONG\",\n"
      "\"data\":\"Bad value (4294967296).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_add_meter_bad_value(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "meter=hoge",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad value (hoge).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/* strict opt. */
void
test_flow_cmd_mod_add_strict_opt_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "-strict",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad opt (-strict).\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/*
 * delete flow.
 */

void
test_flow_cmd_mod_del_ofp_error(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "del",
                         "in_port=0",
                         "in_port=1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OFP_ERROR\",\n"
      "\"data\":\"Can't flow mod (DEL), "
      "ofp_error[type = OFPET_BAD_MATCH, code = OFPBMC_DUP_FIELD].\"}";

  /* mod cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_del_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=1",
                         "vlan_vid=2",
                         "write_metadata=3",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"flow", "b1", "add",
                         "in_port=1",
                         "write_metadata=4",
                         NULL
  };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"flow", "b1", "add",
                         "in_port=2",
                         "dl_type=3",
                         "write_metadata=5",
                         NULL
  };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char test_str4[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"vlan_vid\":\"4098\",\n"
      "\"actions\":[{\"write_metadata\":\"3\"}]},\n"
      "{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"actions\":[{\"write_metadata\":\"4\"}]},\n"
      "{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":2,\n"
      "\"dl_type\":3,\n"
      "\"actions\":[{\"write_metadata\":\"5\"}]}]}]}";
  const char *argv5[] = {"flow", "b1", "del",
                         "in_port=1",
                         NULL
  };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char test_str6[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":2,\n"
      "\"dl_type\":3,\n"
      "\"actions\":[{\"write_metadata\":\"5\"}]}]}]}";


  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, NULL,
                 &ds, str, test_str2);

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, NULL,
                 &ds, str, test_str3);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str4);

  /* delete cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, NULL,
                 &ds, str, test_str5);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str6);
}

void
test_flow_cmd_mod_del_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=1",
                         "vlan_vid=2",
                         "write_metadata=3",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"flow", "b1", "add",
                         "in_port=1",
                         "write_metadata=4",
                         NULL
  };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"flow", "b1", "add",
                         "in_port=1",
                         "dl_type=3",
                         "write_metadata=5",
                         NULL
  };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char test_str4[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"vlan_vid\":\"4098\",\n"
      "\"actions\":[{\"write_metadata\":\"3\"}]},\n"
      "{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"actions\":[{\"write_metadata\":\"4\"}]},\n"
      "{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"dl_type\":3,\n"
      "\"actions\":[{\"write_metadata\":\"5\"}]}]}]}";
  const char *argv5[] = {"flow", "b1", "del",
                         "-strict",
                         "in_port=1",
                         NULL
  };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char test_str6[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"vlan_vid\":\"4098\",\n"
      "\"actions\":[{\"write_metadata\":\"3\"}]},\n"
      "{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"dl_type\":3,\n"
      "\"actions\":[{\"write_metadata\":\"5\"}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, NULL,
                 &ds, str, test_str2);

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, NULL,
                 &ds, str, test_str3);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str4);

  /* delete cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, NULL,
                 &ds, str, test_str5);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str6);
}

void
test_flow_cmd_mod_del_03(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=1",
                         "vlan_vid=2",
                         "write_metadata=3",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"flow", "b1", "add",
                         "in_port=1",
                         "write_metadata=4",
                         NULL
  };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"flow", "b1", "add",
                         "in_port=2",
                         "dl_type=3",
                         "write_metadata=5",
                         NULL
  };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char test_str4[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"vlan_vid\":\"4098\",\n"
      "\"actions\":[{\"write_metadata\":\"3\"}]},\n"
      "{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"actions\":[{\"write_metadata\":\"4\"}]},\n"
      "{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":2,\n"
      "\"dl_type\":3,\n"
      "\"actions\":[{\"write_metadata\":\"5\"}]}]}]}";
  const char *argv5[] = {"flow", "b1", "del",
                         NULL
  };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char test_str6[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, NULL,
                 &ds, str, test_str2);

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, NULL,
                 &ds, str, test_str3);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str4);

  /* delete cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, NULL,
                 &ds, str, test_str5);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str6);
}

/*
 * modify flow.
 */

void
test_flow_cmd_mod_mod_ofp_error(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "mod",
                         "in_port=0",
                         "in_port=1",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"OFP_ERROR\",\n"
      "\"data\":\"Can't flow mod (MOD), "
      "ofp_error[type = OFPET_BAD_MATCH, code = OFPBMC_DUP_FIELD].\"}";

  /* mod cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

void
test_flow_cmd_mod_mod_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=1",
                         "vlan_vid=2",
                         "write_metadata=3",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"flow", "b1", "add",
                         "in_port=1",
                         "write_metadata=4",
                         NULL
  };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"flow", "b1", "add",
                         "in_port=2",
                         "dl_type=3",
                         "write_metadata=5",
                         NULL
  };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char test_str4[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"vlan_vid\":\"4098\",\n"
      "\"actions\":[{\"write_metadata\":\"3\"}]},\n"
      "{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"actions\":[{\"write_metadata\":\"4\"}]},\n"
      "{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":2,\n"
      "\"dl_type\":3,\n"
      "\"actions\":[{\"write_metadata\":\"5\"}]}]}]}";
  const char *argv5[] = {"flow", "b1", "mod",
                         "in_port=1",
                         "goto_table=6",
                         NULL
  };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char test_str6[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"vlan_vid\":\"4098\",\n"
      "\"actions\":[{\"goto_table\":6}]},\n"
      "{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"actions\":[{\"goto_table\":6}]},\n"
      "{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":2,\n"
      "\"dl_type\":3,\n"
      "\"actions\":[{\"write_metadata\":\"5\"}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, NULL,
                 &ds, str, test_str2);

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, NULL,
                 &ds, str, test_str3);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str4);

  /* delete cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, NULL,
                 &ds, str, test_str5);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str6);
}

void
test_flow_cmd_mod_mod_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=1",
                         "vlan_vid=2",
                         "write_metadata=3",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"flow", "b1", "add",
                         "in_port=1",
                         "write_metadata=4",
                         NULL
  };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"flow", "b1", "add",
                         "in_port=2",
                         "dl_type=3",
                         "write_metadata=5",
                         NULL
  };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char test_str4[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"vlan_vid\":\"4098\",\n"
      "\"actions\":[{\"write_metadata\":\"3\"}]},\n"
      "{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"actions\":[{\"write_metadata\":\"4\"}]},\n"
      "{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":2,\n"
      "\"dl_type\":3,\n"
      "\"actions\":[{\"write_metadata\":\"5\"}]}]}]}";
  const char *argv5[] = {"flow", "b1", "mod",
                         "-strict",
                         "in_port=1",
                         "goto_table=6",
                         NULL
  };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char test_str6[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"vlan_vid\":\"4098\",\n"
      "\"actions\":[{\"write_metadata\":\"3\"}]},\n"
      "{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":1,\n"
      "\"actions\":[{\"goto_table\":6}]},\n"
      "{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":2,\n"
      "\"dl_type\":3,\n"
      "\"actions\":[{\"write_metadata\":\"5\"}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, NULL,
                 &ds, str, test_str2);

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, NULL,
                 &ds, str, test_str3);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str4);

  /* delete cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, NULL,
                 &ds, str, test_str5);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str6);
}

void
test_flow_cmd_mod_mod_not_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "mod",
                         NULL
  };
  const char test_str1[] =
      "{\"ret\":\"INVALID_ARGS\"}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);
}

/*  */

void
test_flow_cmd_mod_add_white_space(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port = 0",
                         "apply_actions=dl_type :2, nw_proto : 3",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":0,\n"
      "\"actions\":[{\"apply_actions\":\n"
      "[{\"dl_type\":2},\n"
      "{\"nw_proto\":3}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_flow_cmd_mod_add_many_cmd(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"flow", "b1", "add",
                         "in_port=0",
                         "in_phy_port=1",
                         "write_metadata=3",
                         "apply_actions=dl_type:2,nw_proto:3,output:controller",
                         NULL
  };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] =
      "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
      "\"tables\":[{\"table\":0,\n"
      "\"flows\":[{\"priority\":0,\n"
      "\"idle_timeout\":0,\n"
      "\"hard_timeout\":0,\n"
      "\"cookie\":0,\n"
      "\"in_port\":0,\n"
      "\"in_phy_port\":1,\n"
      "\"actions\":[{\"write_metadata\":\"3\"},\n"
      "{\"apply_actions\":\n"
      "[{\"dl_type\":2},\n"
      "{\"nw_proto\":3},\n"
      "{\"output\":\"controller\"}]}]}]}]}";

  /* add cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,
                 flow_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, NULL,
                 &ds, str, test_str1);

  /* dump cmd. */
  TEST_CMD_FLOW_DUMP(ret, LAGOPUS_RESULT_OK, "b1", OFPTT_ALL,
                     &ds, str, test_str2);
}

void
test_destroy(void) {
  destroy = true;
}
