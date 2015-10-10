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

#include "unity.h"
#include "lagopus_apis.h"
#include "cmd_test_utils.h"
#include "../datastore_apis.h"
#include "../l2_bridge_cmd_internal.h"
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
static struct event_manager *em = NULL;

void
setUp(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create interp. */
  INTERP_CREATE(ret, "l2-bridge", interp, tbl, ds, em);
}

void
tearDown(void) {
  /* destroy interp. */
  INTERP_DESTROY("l2-bridge", interp, tbl, ds, em, destroy);
}

void
test_l2_bridge_cmd_parse_create_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"l2-bridge", "test_name01", "create",
                        "-expire", "18446744073709551615",
                        "-max-entries", "18446744073709551615",
                        "-tmp-dir", "/hoge/foo",
                        NULL
                       };
  const char test_str[] = "{\"ret\":\"OK\"}";
  const char *argv1[] = {"l2-bridge", "test_name02", "create",
                         "-expire", "18446744073709551615",
                         "-max-entries", "18446744073709551615",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name02", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"l2-bridge", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"expire\":18446744073709551615,\n"
    "\"max-entries\":18446744073709551615,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true},\n"
    "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name01\",\n"
    "\"expire\":18446744073709551615,\n"
    "\"max-entries\":18446744073709551615,\n"
    "\"tmp-dir\":\"\\/hoge\\/foo\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"l2-bridge", "test_name02", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"expire\":18446744073709551615,\n"
    "\"max-entries\":18446744073709551615,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\""DATASTORE_NAMESPACE_DELIMITER"b1\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"l2-bridge", "test_name02", "disable",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *argv6[] = {"l2-bridge", "test_name02", "destroy",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"l2-bridge", "test_name01", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv), argv, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE_FOR_L2B(ret, &interp, state, &tbl, &ds, str,
                             "b1", "1", "test_name02",
                             "cha1", "c1", "i2", "p2", "2");

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* show all cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str4);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str5);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY_FOR_L2B(ret, &interp, state, &tbl, &ds, str,
                              "b1", "cha1", "c1", "i2", "p2");

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str6);
  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str7);
}

void
test_l2_bridge_cmd_parse_create_over_expire(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"l2-bridge", "test_name05", "create",
                        "-expire", "18446744073709551616", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = 18446744073709551616.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, l2_bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, l2_bridge_cmd_update, &ds, str, test_str);
}

void
test_l2_bridge_cmd_parse_create_over_max_entries(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"l2-bridge", "test_name05", "create",
                        "-max-entries", "18446744073709551616", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = 18446744073709551616.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, l2_bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, l2_bridge_cmd_update, &ds, str, test_str);
}

void
test_l2_bridge_cmd_parse_invalid_args(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"l2-bridge", "test_name", "hoge_sub_cmd", NULL};
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"sub_cmd = hoge_sub_cmd.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, l2_bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, l2_bridge_cmd_update, &ds, str, test_str);
}

void
test_l2_bridge_cmd_parse_enable_unused(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"l2-bridge", "test_name09", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name09", "enable",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name09. is not used.\"}";
  const char *argv3[] = {"l2-bridge", "test_name09", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name09\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"l2-bridge", "test_name09", "destroy",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str4);
}

void
test_l2_bridge_cmd_parse_config_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"l2-bridge", "test_name10", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name10", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name10\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"l2-bridge", "test_name10", "config",
                         "-expire", "2",
                         "-max-entries", "2",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"l2-bridge", "test_name10", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name10\",\n"
    "\"expire\":2,\n"
    "\"max-entries\":2,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"l2-bridge", "test_name10", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str5);
}

void
test_l2_bridge_cmd_parse_config_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"l2-bridge", "test_name11", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name11", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"l2-bridge", "test_name11", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\""DATASTORE_NAMESPACE_DELIMITER"b11\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv4[] = {"l2-bridge", "test_name11", "config",
                         "-expire", "2",
                         "-max-entries", "2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"l2-bridge", "test_name11", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"expire\":2,\n"
    "\"max-entries\":2,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\""DATASTORE_NAMESPACE_DELIMITER"b11\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv6[] = {"l2-bridge", "test_name11", "disable",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"l2-bridge", "test_name11", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE_FOR_L2B(ret, &interp, state, &tbl, &ds, str,
                             "b11", "11", "test_name11",
                             "cha11", "c11", "i11", "p11", "11");

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str5);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str6);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY_FOR_L2B(ret, &interp, state, &tbl, &ds, str,
                              "b11", "cha11", "c11", "i11", "p11");

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str7);
}

void
test_l2_bridge_cmd_parse_config_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"l2-bridge", "test_name12", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name12", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name12\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"l2-bridge", "test_name12", "config",
                         "-expire", "2",
                         "-max-entries", "2",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"l2-bridge", "test_name12", "config",
                         "-expire", NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name12\",\n"
    "\"expire\":2}]}";
  const char *argv5[] = {"l2-bridge", "test_name12", "config",
                         "-max-entries", NULL
                        };
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name12\",\n"
    "\"max-entries\":2}]}";
  const char *argv6[] = {"l2-bridge", "test_name12", "destroy",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);

  /* config(show) cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str4);

  /* config(show) cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str5);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str6);
}

void
test_l2_bridge_cmd_parse_destroy_used(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"l2-bridge", "test_name13", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name13", "disable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"l2-bridge", "test_name13", "destroy",
                         NULL
                        };
  const char test_str3[] = {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                            "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name13: is used.\"}"
                           };
  const char *argv4[] = {"l2-bridge", "test_name13", "destroy",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE_FOR_L2B(ret, &interp, state, &tbl, &ds, str,
                             "b13", "13", "test_name13",
                             "cha13", "c13", "i13", "p13", "13");

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, l2_bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                 l2_bridge_cmd_update, &ds, str, test_str3);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY_FOR_L2B(ret, &interp, state, &tbl, &ds, str,
                              "b13", "cha13", "c13", "i13", "p13");

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv4), argv4, &tbl,
                 l2_bridge_cmd_update, &ds, str, test_str4);
}

void
test_l2_bridge_cmd_parse_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  char *str = NULL;
  const char *argv1[] = {"l2-bridge", "test_name14", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name14", "current",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name14\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"l2-bridge", "test_name14", "modified",
                         NULL
                        };
  const char test_str3[] =  {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                             "\"data\":\"Not set modified.\"}"
                            };
  const char *argv4[] = {"l2-bridge", "test_name14", "config",
                         "-expire", "2",
                         "-max-entries", "2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"l2-bridge", "test_name14", "modified",
                         NULL
                        };
  const char test_str5[] =  {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                             "\"data\":\"Not set modified.\"}"
                            };
  const char *argv6[] = {"l2-bridge", "test_name14", "config",
                         "-expire", "3",
                         "-max-entries", "3",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"l2-bridge", "test_name14", "modified",
                         NULL
                        };
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name14\",\n"
    "\"expire\":3,\n"
    "\"max-entries\":3,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"l2-bridge", "test_name14", "destroy",
                         NULL
                        };
  const char test_str8[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, l2_bridge_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv3), argv3, &tbl,
                 l2_bridge_cmd_update, &ds, str, test_str3);

  /* config cmd (AUTO COMMIT mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (modified : AUTO COMMIT mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, l2_bridge_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv5), argv5, &tbl,
                 l2_bridge_cmd_update, &ds, str, test_str5);

  /* config cmd (COMMITING mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv6), argv6, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str6);

  /* show cmd (modified : COMMITING mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv7), argv7, &tbl,
                 l2_bridge_cmd_update, &ds, str, test_str7);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv8), argv8, &tbl,
                 l2_bridge_cmd_update, &ds, str, test_str8);
}

void
test_l2_bridge_cmd_serialize_default_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* l2 bridge create cmd str. */
  const char *argv1[] = {"l2-bridge", "test_name27", "create", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* l2 bridge destroy cmd str. */
  const char *argv2[] = {"l2-bridge", "test_name27", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] =
    "l2-bridge "DATASTORE_NAMESPACE_DELIMITER"test_name27 create "
    "-expire 300 "
    "-max-entries 1000000 "
    "-tmp-dir /tmp\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name27", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 l2_bridge_cmd_update, &ds, str, test_str2);
}

void
test_l2_bridge_cmd_serialize_default_opt_escape(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* l2 bridge create cmd str. */
  const char *argv1[] = {"l2-bridge", "test_\"name28", "create", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* l2 bridge destroy cmd str. */
  const char *argv2[] = {"l2-bridge", "test_\"name28", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] =
    "l2-bridge \""DATASTORE_NAMESPACE_DELIMITER"test_\\\"name28\" create "
    "-expire 300 "
    "-max-entries 1000000 "
    "-tmp-dir /tmp\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_\"name28", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 l2_bridge_cmd_update, &ds, str, test_str2);
}

void
test_l2_bridge_cmd_serialize_default_opt_escape_white_space(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* l2 bridge create cmd str. */
  const char *argv1[] = {"l2-bridge", "test name29", "create", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* l2 bridge destroy cmd str. */
  const char *argv2[] = {"l2-bridge", "test name29", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] =
    "l2-bridge \""DATASTORE_NAMESPACE_DELIMITER"test name29\" create "
    "-expire 300 "
    "-max-entries 1000000 "
    "-tmp-dir /tmp\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test name29", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 l2_bridge_cmd_update, &ds, str, test_str2);
}

void
test_l2_bridge_cmd_serialize_all_opts(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* l2 bridge create cmd str. */
  const char *argv1[] = {"l2-bridge", "test_name30", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         "-tmp-dir", "/hoge/foo",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* l2 bridge destroy cmd str. */
  const char *argv2[] = {"l2-bridge", "test_name30", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "l2-bridge "
                                DATASTORE_NAMESPACE_DELIMITER"test_name30 create "
                                "-expire 1 "
                                "-max-entries 1 "
                                "-tmp-dir /hoge/foo\n";


  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name30", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 l2_bridge_cmd_update, &ds, str, test_str2);
}

void
test_l2_bridge_cmd_parse_stats_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"l2-bridge", "test_name33", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name33", "stats",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name33\",\n"
    "\"entries\":0}]}";
  const char *argv3[] = {"l2-bridge", "test_name33", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* stats cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);
}

void
test_l2_bridge_cmd_parse_stats_invalid_args(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"l2-bridge", "test_name35", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name35", "stats",
                         "hoge",
                         NULL
                        };
  const char test_str2[] = {"{\"ret\":\"INVALID_ARGS\",\n"
                            "\"data\":\"Bad opt = hoge.\"}"
                           };
  const char *argv3[] = {"l2-bridge", "test_name35", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* stats cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, l2_bridge_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);
}

void
test_l2_bridge_cmd_parse_atomic_commit(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"l2-bridge", "test_name36", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name36", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"l2-bridge", "test_name36", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name36\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"l2-bridge", "test_name36", "config",
                         "-expire", "2",
                         "-max-entries", "2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"l2-bridge", "test_name36", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"l2-bridge", "test_name36", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name36\",\n"
    "\"expire\":2,\n"
    "\"max-entries\":2,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"l2-bridge", "test_name36", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name36\",\n"
    "\"expire\":2,\n"
    "\"max-entries\":2,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"l2-bridge", "test_name36", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"l2-bridge", "test_name36", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name36", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name36", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str9);
}

void
test_l2_bridge_cmd_parse_atomic_rollback(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ROLLBACKING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ROLLBACKED;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"l2-bridge", "test_name37", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name37", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"l2-bridge", "test_name37", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name37\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"l2-bridge", "test_name37", "config",
                         "-expire", "2",
                         "-max-entries", "2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"l2-bridge", "test_name37", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"l2-bridge", "test_name37", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name37\",\n"
    "\"expire\":2,\n"
    "\"max-entries\":2,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"l2-bridge", "test_name37", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name37\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str6);

  /* rollbacking. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name37", conf, &ds);

  /* rollbacked. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name37", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str7);
}

void
test_l2_bridge_cmd_parse_atomic_delay_enable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"l2-bridge", "test_name38", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name38", "enable", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"l2-bridge", "test_name38", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv4[] = {"l2-bridge", "test_name38", "modified", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name38\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\""DATASTORE_NAMESPACE_DELIMITER"b38\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"l2-bridge", "test_name38", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"l2-bridge", "test_name38", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name38\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\""DATASTORE_NAMESPACE_DELIMITER"b38\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv7[] = {"l2-bridge", "test_name38", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name38\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\""DATASTORE_NAMESPACE_DELIMITER"b38\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv8[] = {"l2-bridge", "test_name38", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"l2-bridge", "test_name38", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE_FOR_L2B(ret, &interp, state1, &tbl, &ds, str,
                             "b38", "38", "test_name38",
                             "cha38", "c38", "i38", "p38", "38");

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str4);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name38", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str6);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name38", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str8);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY_FOR_L2B(ret, &interp, state4, &tbl, &ds, str,
                              "b38", "cha38", "c38", "i38", "p38");

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str9);
}

void
test_l2_bridge_cmd_parse_atomic_delay_disable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"l2-bridge", "test_name39", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name39", "enable", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"l2-bridge", "test_name39", "disable", NULL};
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"l2-bridge", "test_name39", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name39\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\""DATASTORE_NAMESPACE_DELIMITER"b39\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"l2-bridge", "test_name39", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name39\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\""DATASTORE_NAMESPACE_DELIMITER"b39\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"l2-bridge", "test_name39", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name39\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\""DATASTORE_NAMESPACE_DELIMITER"b39\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"l2-bridge", "test_name39", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE_FOR_L2B(ret, &interp, state4, &tbl, &ds, str,
                             "b39", "39", "test_name39",
                             "cha39", "c39", "i39", "p39", "39");

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str4);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name39", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str5);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name39", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str6);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY_FOR_L2B(ret, &interp, state4, &tbl, &ds, str,
                              "b39", "cha39", "c39", "i39", "p39");

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv7), argv7, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str7);
}

void
test_l2_bridge_cmd_parse_atomic_delay_destroy(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"l2-bridge", "test_name40", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name40", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"l2-bridge", "test_name40", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name40\"}";
  const char *argv4[] = {"l2-bridge", "test_name40", "enable",
                         NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name40\"}";
  const char *argv5[] = {"l2-bridge", "test_name40", "disable",
                         NULL
                        };
  const char test_str5[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name40\"}";
  const char *argv6[] = {"l2-bridge", "test_name40", "destroy",
                         NULL
                        };
  const char test_str6[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name40\"}";
  const char *argv7[] = {"l2-bridge", "test_name40", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name40\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str4);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str5);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name40", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name40", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str7);
}

void
test_l2_bridge_cmd_parse_atomic_abort_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"l2-bridge", "test_name41", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name41", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"l2-bridge", "test_name41", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name41\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"l2-bridge", "test_name41", "config",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"l2-bridge", "test_name41", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"l2-bridge", "test_name41", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name41\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"l2-bridge", "test_name41", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv8[] = {"l2-bridge", "test_name41", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name41\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"l2-bridge", "test_name41", NULL};
  const char test_str9[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name41\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str6);

  /* aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name41", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str8);

  /* aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name41", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv9), argv9, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str9);
}

void
test_l2_bridge_cmd_parse_atomic_abort_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"l2-bridge", "test_name42", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name42", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"l2-bridge", "test_name42", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv4[] = {"l2-bridge", "test_name42", "config",
                         "-expire", "2",
                         "-max-entries", "2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"l2-bridge", "test_name42", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"l2-bridge", "test_name42", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"expire\":2,\n"
    "\"max-entries\":2,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"l2-bridge", "test_name42", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"l2-bridge", "test_name42", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"expire\":2,\n"
    "\"max-entries\":2,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"l2-bridge", "test_name42", NULL};
  const char test_str9[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv10[] = {"l2-bridge", "test_name42", "modified", NULL};
  const char test_str10[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv11[] = {"l2-bridge", "test_name42", "destroy",
                          NULL
                         };
  const char test_str11[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str6);

  /*  aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name42", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str8);

  /*  aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name42", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv9), argv9, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str9);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv10), argv10, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str10);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv11), argv11, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str11);

}

void
test_l2_bridge_cmd_parse_atomic_destroy_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"l2-bridge", "test_name43", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name43", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"l2-bridge", "test_name43", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name43\"}";
  const char *argv4[] = {"l2-bridge", "test_name43", "create",
                         "-expire", "2",
                         "-max-entries", "2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"l2-bridge", "test_name43", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name43\",\n"
    "\"expire\":1,\n"
    "\"max-entries\":1,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"l2-bridge", "test_name43", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name43\",\n"
    "\"expire\":2,\n"
    "\"max-entries\":2,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"l2-bridge", "test_name43", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name43\",\n"
    "\"expire\":2,\n"
    "\"max-entries\":2,\n"
    "\"tmp-dir\":\"\\/tmp\",\n"
    "\"bridge\":\"\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"l2-bridge", "test_name43", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"l2-bridge", "test_name43", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name43", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name43", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv7), argv7, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv8), argv8, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str9);
}

void
test_l2_bridge_cmd_parse_create_bad_tmp_dir(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"l2-bridge", "test_name44", "create",
                        "-tmp-dir", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, l2_bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, l2_bridge_cmd_update, &ds, str, test_str);
}

void
test_l2_bridge_cmd_parse_clear_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"l2-bridge", "test_name45", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name45", "clear",
                         NULL
                        };
  const char test_str2[] ="{\"ret\":\"OK\"}";
  const char *argv3[] = {"l2-bridge", "test_name45", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE_FOR_L2B(ret, &interp, state, &tbl, &ds, str,
                             "b45", "45", "test_name45",
                             "cha45", "c45", "i45", "p45", "45");

  /* clear cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY_FOR_L2B(ret, &interp, state, &tbl, &ds, str,
                              "b45", "cha45", "c45", "i45", "p45");

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);
}

void
test_l2_bridge_cmd_parse_clear_bad_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"l2-bridge", "test_name46", "create",
                         "-expire", "1",
                         "-max-entries", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"l2-bridge", "test_name46", "clear",
                         "hoge",
                         NULL
                        };
  const char test_str2[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"Bad opt = hoge.\"}";
  const char *argv3[] = {"l2-bridge", "test_name46", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_BRIDGE_CREATE_FOR_L2B(ret, &interp, state, &tbl, &ds, str,
                             "b46", "46", "test_name46",
                             "cha46", "c46", "i46", "p46", "46");

  /* clear cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str2);

  /* bridge destroy cmd.*/
  TEST_BRIDGE_DESTROY_FOR_L2B(ret, &interp, state, &tbl, &ds, str,
                              "b46", "cha46", "c46", "i46", "p46");

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, l2_bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, l2_bridge_cmd_update,
                 &ds, str, test_str3);
}

void
test_destroy(void) {
  destroy = true;
}
