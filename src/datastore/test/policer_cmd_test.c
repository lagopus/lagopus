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
#include "../datastore_apis.h"
#include "../policer_cmd_internal.h"
#include "../policer_action_cmd_internal.h"
#include "../port_cmd_internal.h"
#include "../datastore_internal.h"

static lagopus_dstring_t ds = NULL;
static lagopus_hashmap_t tbl = NULL;
static datastore_interp_t interp = NULL;
static bool destroy = false;

void
setUp(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create interp. */
  INTERP_CREATE(ret, "policer", interp, tbl, ds);
}

void
tearDown(void) {
  /* destroy interp. */
  INTERP_DESTROY("policer", interp, tbl, ds, destroy);
}

void
test_policer_cmd_parse_create_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"policer", "test_name01", "create",
                        "-action", "test_pa01",
                        "-bandwidth-limit", "1501",
                        "-burst-size-limit", "1502",
                        "-bandwidth-percent", "1",
                        NULL
                       };
  const char test_str[] = "{\"ret\":\"OK\"}";
  const char *argv1[] = {"policer", "test_name02", "create",
                         "-action", "test_pa\"02",
                         "-bandwidth-limit", "1601",
                         "-burst-size-limit", "1602",
                         "-bandwidth-percent", "10",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name02", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"policer", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa\\\"02\"],\n"
    "\"bandwidth-limit\":1601,\n"
    "\"burst-size-limit\":1602,\n"
    "\"bandwidth-percent\":10,\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true},\n"
    "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name01\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa01\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"policer", "test_name02", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa\\\"02\"],\n"
    "\"bandwidth-limit\":1601,\n"
    "\"burst-size-limit\":1602,\n"
    "\"bandwidth-percent\":10,\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"policer", "test_name02", "disable",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *argv6[] = {"policer", "test_name02", "destroy",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"policer", "test_name01", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv1[] = {"policer-action", "test_pa01", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa\"02", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv3[] = {"policer-action", "test_pa\"02", NULL};
  const char policer_action_test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_pa\\\"02\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *policer_action_argv4[] = {"policer-action", "test_pa\"02", NULL};
  const char policer_action_test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_pa\\\"02\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *policer_action_argv5[] = {"policer-action", "test_pa\"02", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str5[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv6[] = {"policer-action", "test_pa01", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str6[] = "{\"ret\":\"OK\"}";
  const char *port_argv1[] = {"port", "port_name01", "create",
                              "-policer", "test_name02",
                              NULL
                             };
  const char port_test_str1[] = "{\"ret\":\"OK\"}";
  const char *port_argv2[] = {"port", "port_name01", "destroy",
                              NULL
                             };
  const char port_test_str2[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv), argv, &tbl, policer_cmd_update,
                 &ds, str, test_str);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* port create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv1), port_argv1, &tbl, port_cmd_update,
                 &ds, str, port_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* show all cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_cmd_update,
                 &ds, str, test_str4);

  /* policer_action show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv3), policer_action_argv3, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str3);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_cmd_update,
                 &ds, str, test_str5);

  /* port destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv2), port_argv2, &tbl, port_cmd_update,
                 &ds, str, port_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_cmd_update,
                 &ds, str, test_str6);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_cmd_update,
                 &ds, str, test_str7);

  /* policer_action show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv4), policer_action_argv4, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str4);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv5), policer_action_argv5, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str5);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv6), policer_action_argv6, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str6);
}

void
test_policer_cmd_parse_create_bad_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"policer", "test_name03", "create", "-hoge", NULL};
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"opt = -hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, policer_cmd_update, &ds, str, test_str);
}

void
test_policer_cmd_parse_create_not_policer_action_name_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"policer", "test_name04", "create",
                        "-action", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, policer_cmd_update, &ds, str, test_str);
}

void
test_policer_cmd_parse_create_over_bandwidth_limit(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"policer", "test_name05", "create",
                        "-bandwidth-limit", "18446744073709551616",
                        NULL
                       };
  const char test_str[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = 18446744073709551616.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, policer_cmd_update, &ds, str, test_str);
}

void
test_policer_cmd_parse_create_bad_bandwidth_limit(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"policer", "test_name06", "create",
                        "-bandwidth-limit", "hoge",
                        NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, policer_cmd_update, &ds, str, test_str);
}

void
test_policer_cmd_parse_create_over_burst_size_limit(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"policer", "test_name05", "create",
                        "-burst-size-limit", "18446744073709551616",
                        NULL
                       };
  const char test_str[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = 18446744073709551616.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, policer_cmd_update, &ds, str, test_str);
}

void
test_policer_cmd_parse_create_bad_burst_size_limit(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"policer", "test_name06", "create",
                        "-burst-size-limit", "hoge",
                        NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, policer_cmd_update, &ds, str, test_str);
}

void
test_policer_cmd_parse_create_over_bandwidth_percent(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"policer", "test_name05", "create",
                        "-bandwidth-percent", "256",
                        NULL
                       };
  const char test_str[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = 256.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, policer_cmd_update, &ds, str, test_str);
}

void
test_policer_cmd_parse_create_bad_bandwidth_percent(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"policer", "test_name06", "create",
                        "-bandwidth-percent", "hoge",
                        NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, policer_cmd_update, &ds, str, test_str);
}

void
test_policer_cmd_parse_enable_unused(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"policer", "test_name09", "create",
                         "-action", "test_pa03",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name09", "enable",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name09. is not used.\"}";
  const char *argv3[] = {"policer", "test_name09", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name09\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa03\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"policer", "test_name09", "destroy",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv1[] = {"policer-action", "test_pa03", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa03", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_cmd_update,
                 &ds, str, test_str3);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_cmd_update,
                 &ds, str, test_str4);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);
}

void
test_policer_cmd_parse_create_not_exists(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"policer", "test_name10", "create",
                         "-action", "test_pa100",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"action name = "DATASTORE_NAMESPACE_DELIMITER"test_pa100.\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);
}

void
test_policer_cmd_parse_config_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"policer", "test_name11", "create",
                         "-action", "test_pa04",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name11", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa04\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"policer", "test_name11", "config",
                         "-action", "~test_pa04",
                         "-action", "test_pa05",
                         "-bandwidth-limit", "1601",
                         "-burst-size-limit", "1602",
                         "-bandwidth-percent", "10",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"policer", "test_name11", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa05\"],\n"
    "\"bandwidth-limit\":1601,\n"
    "\"burst-size-limit\":1602,\n"
    "\"bandwidth-percent\":10,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"policer", "test_name11", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv1[] = {"policer-action", "test_pa04", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa04", NULL};
  const char policer_action_test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_pa04\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *policer_action_argv3[] = {"policer-action", "test_pa04", NULL};
  const char policer_action_test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_pa04\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *policer_action_argv4[] = {"policer-action", "test_pa05", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str4[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv5[] = {"policer-action", "test_pa05", NULL};
  const char policer_action_test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_pa05\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *policer_action_argv6[] = {"policer-action", "test_pa05", NULL};
  const char policer_action_test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_pa05\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *policer_action_argv7[] = {"policer-action", "test_pa04", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str7[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv8[] = {"policer-action", "test_pa05", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str8[] = "{\"ret\":\"OK\"}";


  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv4), policer_action_argv4, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str4);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* policer_action show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);

  /* policer_action show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv5), policer_action_argv5, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str5);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_cmd_update,
                 &ds, str, test_str3);

  /* policer_action show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv3), policer_action_argv3, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str3);

  /* policer_action show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv6), policer_action_argv6, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str6);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_cmd_update,
                 &ds, str, test_str5);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv7), policer_action_argv7, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str7);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv8), policer_action_argv8, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str8);
}

void
test_policer_cmd_parse_config_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"policer", "test_name11", "create",
                         "-action", "test_pa06",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name11", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"policer", "test_name11", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa06\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv4[] = {"policer", "test_name11", "config",
                         "-bandwidth-limit", "1601",
                         "-burst-size-limit", "1602",
                         "-bandwidth-percent", "10",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer", "test_name11", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa06\"],\n"
    "\"bandwidth-limit\":1601,\n"
    "\"burst-size-limit\":1602,\n"
    "\"bandwidth-percent\":10,\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv6[] = {"policer", "test_name11", "disable",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"policer", "test_name11", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv1[] = {"policer-action", "test_pa06", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa06", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";
  const char *port_argv1[] = {"port", "port_name02", "create",
                              "-policer", "test_name11",
                              NULL
                             };
  const char port_test_str1[] = "{\"ret\":\"OK\"}";
  const char *port_argv2[] = {"port", "port_name02", "destroy",
                              NULL
                             };
  const char port_test_str2[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* port create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv1), port_argv1, &tbl, port_cmd_update,
                 &ds, str, port_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_cmd_update,
                 &ds, str, test_str4);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_cmd_update,
                 &ds, str, test_str5);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_cmd_update,
                 &ds, str, test_str6);

  /* port destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv2), port_argv2, &tbl, port_cmd_update,
                 &ds, str, port_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_cmd_update,
                 &ds, str, test_str7);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);
}

void
test_policer_cmd_parse_config_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"policer", "test_name13", "create",
                         "-action", "test_pa09",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name13", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name13\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa09\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"policer", "test_name13", "config",
                         "-bandwidth-limit", "1601",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"policer", "test_name13", "config",
                         "-bandwidth-limit", NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name13\",\n"
    "\"bandwidth-limit\":1601}]}";
  const char *argv5[] = {"policer", "test_name13", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv1[] = {"policer-action", "test_pa09", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa09", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_cmd_update,
                 &ds, str, test_str3);

  /* config cmd (show). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_cmd_update,
                 &ds, str, test_str5);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);
}

void
test_policer_cmd_parse_destroy_used(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"policer", "test_name14", "create",
                         "-action", "test_pa14",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name14", "disable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"policer", "test_name14", "destroy",
                         NULL
                        };
  const char test_str3[] = {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                            "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name14: is used.\"}"
                           };
  const char *argv4[] = {"policer", "test_name14", "destroy",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv1[] = {"policer-action", "test_pa14", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa14", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";
  const char *port_argv1[] = {"port", "port_name14", "create",
                              "-policer", "test_name14",
                              NULL
                             };
  const char port_test_str1[] = "{\"ret\":\"OK\"}";
  const char *port_argv2[] = {"port", "port_name14", "destroy",
                              NULL
                             };
  const char port_test_str2[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* port create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv1), port_argv1, &tbl, port_cmd_update,
                 &ds, str, port_test_str1);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                 policer_cmd_update, &ds, str, test_str3);

  /* port destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv2), port_argv2, &tbl, port_cmd_update,
                 &ds, str, port_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv4), argv4, &tbl,
                 policer_cmd_update, &ds, str, test_str4);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);

}

void
test_policer_cmd_parse_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  char *str = NULL;
  const char *argv1[] = {"policer", "test_name15", "create",
                         "-action", "test_pa15",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name15", "current",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name15\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa15\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"policer", "test_name15", "modified",
                         NULL
                        };
  const char test_str3[] =  {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                             "\"data\":\"Not set modified.\"}"
                            };
  const char *argv4[] = {"policer", "test_name15", "config",
                         "-bandwidth-limit", "1601",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer", "test_name15", "modified",
                         NULL
                        };
  const char test_str5[] =  {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                             "\"data\":\"Not set modified.\"}"
                            };
  const char *argv6[] = {"policer", "test_name15", "config",
                         "-bandwidth-limit", "1701",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"policer", "test_name15", "modified",
                         NULL
                        };
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name15\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa15\"],\n"
    "\"bandwidth-limit\":1701,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"policer", "test_name15", "destroy",
                         NULL
                        };
  const char test_str8[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv1[] = {"policer-action", "test_pa15", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa15", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv3), argv3, &tbl,
                 policer_cmd_update, &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (modified : AUTO COMMIT mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv5), argv5, &tbl,
                 policer_cmd_update, &ds, str, test_str5);

  /* config cmd (COMMITING mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_cmd_update,
                 &ds, str, test_str6);

  /* show cmd (modified : COMMITING mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_cmd_update,
                 &ds, str, test_str7);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, policer_cmd_update,
                 &ds, str, test_str8);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);
}

void
test_policer_cmd_serialize_default_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* policer create cmd str. */
  const char *argv1[] = {"policer", "test_name16", "create",
                         "-action", "test_pa16", NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"policer", "test_name16", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "policer "
                                DATASTORE_NAMESPACE_DELIMITER"test_name16 create "
                                "-action "DATASTORE_NAMESPACE_DELIMITER"test_pa16 "
                                "-bandwidth-limit 1500 "
                                "-burst-size-limit 1500 "
                                "-bandwidth-percent 0\n";

  /* policer_action. */
  const char *policer_action_argv1[] = {"policer-action", "test_pa16", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa16", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* policer create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name16", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 policer_cmd_update, &ds, str, test_str2);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);
}

void
test_policer_cmd_serialize_default_opt_escape(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* policer create cmd str. */
  const char *argv1[] = {"policer", "test_\"name17", "create",
                         "-action", "test_pa17", NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"policer", "test_\"name17", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "policer "
                                "\""DATASTORE_NAMESPACE_DELIMITER"test_\\\"name17\" create "
                                "-action "DATASTORE_NAMESPACE_DELIMITER"test_pa17 "
                                "-bandwidth-limit 1500 "
                                "-burst-size-limit 1500 "
                                "-bandwidth-percent 0\n";

  /* policer_action. */
  const char *policer_action_argv1[] = {"policer-action", "test_pa17", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa17", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* policer create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_\"name17", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 policer_cmd_update, &ds, str, test_str2);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);
}

void
test_policer_cmd_serialize_default_opt_escape_white_space(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* policer create cmd str. */
  const char *argv1[] = {"policer", "test name18", "create",
                         "-action", "test_pa18", NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"policer", "test name18", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "policer "
                                "\""DATASTORE_NAMESPACE_DELIMITER"test name18\" create "
                                "-action "DATASTORE_NAMESPACE_DELIMITER"test_pa18 "
                                "-bandwidth-limit 1500 "
                                "-burst-size-limit 1500 "
                                "-bandwidth-percent 0\n";

  /* policer_action. */
  const char *policer_action_argv1[] = {"policer-action", "test_pa18", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa18", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test name18", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 policer_cmd_update, &ds, str, test_str2);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);
}

void
test_policer_cmd_serialize_all_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* policer_action create cmd str. */
  const char *policer_action_argv1[] = {"policer-action", "test_pa19", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa19", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";

  /* policer create cmd str. */
  const char *argv1[] = {"policer", "test_name19", "create",
                         "-action", "test_pa19",
                         "-bandwidth-limit", "1601",
                         "-burst-size-limit", "1602",
                         "-bandwidth-percent", "10",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* policer create cmd. */
  const char *argv2[] = {"policer", "test_name19", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "policer "
                                DATASTORE_NAMESPACE_DELIMITER"test_name19 create "
                                "-action "DATASTORE_NAMESPACE_DELIMITER"test_pa19 "
                                "-bandwidth-limit 1601 "
                                "-burst-size-limit 1602 "
                                "-bandwidth-percent 10\n";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* policer create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name19", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 policer_cmd_update, &ds, str, test_str2);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);
}

void
test_policer_cmd_serialize_all_opt_escape(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* policer_action create cmd str. */
  const char *policer_action_argv1[] = {"policer-action", "test_\"pa20", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_\"pa20", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";

  /* policer create cmd str. */
  const char *argv1[] = {"policer", "test_name20", "create",
                         "-action", "test_\"pa20",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* policer create cmd. */
  const char *argv2[] = {"policer", "test_name20", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "policer "
                                DATASTORE_NAMESPACE_DELIMITER"test_name20 create "
                                "-action \""DATASTORE_NAMESPACE_DELIMITER"test_\\\"pa20\" "
                                "-bandwidth-limit 1501 "
                                "-burst-size-limit 1502 "
                                "-bandwidth-percent 1\n";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* policer create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name20", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 policer_cmd_update, &ds, str, test_str2);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);
}

void
test_policer_cmd_serialize_all_opt_escape_white_space(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* policer_action create cmd str. */
  const char *policer_action_argv1[] = {"policer-action", "test pa21", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test pa21", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";


  /* interface create cmd str. */
  const char *argv1[] = {"policer", "test_name21", "create",
                         "-action", "test pa21",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"policer", "test_name21", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "policer "
                                DATASTORE_NAMESPACE_DELIMITER"test_name21 create "
                                "-action \""DATASTORE_NAMESPACE_DELIMITER"test pa21\" "
                                "-bandwidth-limit 1501 "
                                "-burst-size-limit 1502 "
                                "-bandwidth-percent 1\n";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* policer create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name21", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 policer_cmd_update, &ds, str, test_str2);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);
}

void
test_policer_cmd_parse_atomic_commit(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer", "test_name24", "create",
                         "-action", "test_pa24",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name24", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"policer", "test_name24", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name24\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa24\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"policer", "test_name24", "config",
                         "-action", "~test_pa24",
                         "-action", "test_pa24_2",
                         "-bandwidth-limit", "1601",
                         "-burst-size-limit", "1602",
                         "-bandwidth-percent", "10",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer", "test_name24", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"policer", "test_name24", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name24\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa24_2\"],\n"
    "\"bandwidth-limit\":1601,\n"
    "\"burst-size-limit\":1602,\n"
    "\"bandwidth-percent\":10,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"policer", "test_name24", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name24\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa24_2\"],\n"
    "\"bandwidth-limit\":1601,\n"
    "\"burst-size-limit\":1602,\n"
    "\"bandwidth-percent\":10,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"policer", "test_name24", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"policer", "test_name24", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv1[] = {"policer-action", "test_pa24", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa24_2", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv3[] = {"policer-action", "test_pa24", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str3[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv4[] = {"policer-action", "test_pa24_2", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str4[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name24", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name24", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, policer_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, policer_cmd_update,
                 &ds, str, test_str9);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv3), policer_action_argv3, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str3);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv4), policer_action_argv4, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str4);
}

void
test_policer_cmd_parse_atomic_rollback(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ROLLBACKING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ROLLBACKED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer", "test_name25", "create",
                         "-action", "test_pa25",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name25", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"policer", "test_name25", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name25\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa25\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"policer", "test_name25", "config",
                         "-action", "~test_pa25",
                         "-action", "test_pa25_2",
                         "-bandwidth-limit", "1601",
                         "-burst-size-limit", "1602",
                         "-bandwidth-percent", "10",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer", "test_name25", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"policer", "test_name25", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name25\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa25_2\"],\n"
    "\"bandwidth-limit\":1601,\n"
    "\"burst-size-limit\":1602,\n"
    "\"bandwidth-percent\":10,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"policer", "test_name25", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = :test_name25\"}";
  const char *policer_action_argv1[] = {"policer-action", "test_pa25", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa25_2", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv3[] = {"policer-action", "test_pa25", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str3[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv4[] = {"policer-action", "test_pa25_2", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str4[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_cmd_update,
                 &ds, str, test_str6);

  /* rollbacking. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name25", conf, &ds);

  /* rollbacked. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name25", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_cmd_update,
                 &ds, str, test_str7);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv3), policer_action_argv3, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str3);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv4), policer_action_argv4, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str4);
}

void
test_policer_cmd_parse_atomic_delay_enable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer", "test_name26", "create",
                         "-action", "test_pa26",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name26", "enable", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"policer", "test_name26", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv4[] = {"policer", "test_name26", "modified", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name26\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa26\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"policer", "test_name26", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"policer", "test_name26", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name26\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa26\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv7[] = {"policer", "test_name26", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name26\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa26\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv8[] = {"policer", "test_name26", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"policer", "test_name26", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv1[] = {"policer-action", "test_pa26", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa26", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";
  const char *port_argv1[] = {"port", "port_name26", "create",
                              "-policer", "test_name26",
                              NULL
                             };
  const char port_test_str1[] = "{\"ret\":\"OK\"}";
  const char *port_argv2[] = {"port", "port_name26", "destroy",
                              NULL
                             };
  const char port_test_str2[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* port create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(port_argv1), port_argv1, &tbl, port_cmd_update,
                 &ds, str, port_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_cmd_update,
                 &ds, str, test_str4);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name26", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_cmd_update,
                 &ds, str, test_str6);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name26", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, policer_cmd_update,
                 &ds, str, test_str8);

  /* port destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(port_argv2), port_argv2, &tbl, port_cmd_update,
                 &ds, str, port_test_str2);


  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, policer_cmd_update,
                 &ds, str, test_str9);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);
}

void
test_policer_cmd_parse_atomic_delay_disable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer", "test_name27", "create",
                         "-action", "test_pa27",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name27", "enable", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"policer", "test_name27", "disable", NULL};
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"policer", "test_name27", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name27\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa27\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"policer", "test_name27", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name27\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa27\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"policer", "test_name27", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name27\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa27\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"policer", "test_name27", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv1[] = {"policer-action", "test_pa27", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa27", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";
  const char *port_argv1[] = {"port", "port_name27", "create",
                              "-policer", "test_name27",
                              NULL
                             };
  const char port_test_str1[] = "{\"ret\":\"OK\"}";
  const char *port_argv2[] = {"port", "port_name27", "destroy",
                              NULL
                             };
  const char port_test_str2[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* port create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(port_argv1), port_argv1, &tbl, port_cmd_update,
                 &ds, str, port_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_cmd_update,
                 &ds, str, test_str4);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name27", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_cmd_update,
                 &ds, str, test_str5);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name27", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_cmd_update,
                 &ds, str, test_str6);

  /* port destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(port_argv2), port_argv2, &tbl, port_cmd_update,
                 &ds, str, port_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_cmd_update,
                 &ds, str, test_str7);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);
}

void
test_policer_cmd_parse_atomic_delay_destroy(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer", "test_name28", "create",
                         "-action", "test_pa28",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name28", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"policer", "test_name28", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name28\"}";
  const char *argv4[] = {"policer", "test_name28", "enable",
                         NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name28\"}";
  const char *argv5[] = {"policer", "test_name28", "disable",
                         NULL
                        };
  const char test_str5[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name28\"}";
  const char *argv6[] = {"policer", "test_name28", "destroy",
                         NULL
                        };
  const char test_str6[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name28\"}";
  const char *argv7[] = {"policer", "test_name28", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name28\"}";
  const char *policer_action_argv1[] = {"policer-action", "test_pa28", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa28", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_cmd_update,
                 &ds, str, test_str3);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_cmd_update,
                 &ds, str, test_str4);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_cmd_update,
                 &ds, str, test_str5);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name28", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name28", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_cmd_update,
                 &ds, str, test_str7);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);
}

void
test_policer_cmd_parse_atomic_abort_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer", "test_name29", "create",
                         "-action", "test_pa29",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name29", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"policer", "test_name29", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name29\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa29\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"policer", "test_name29", "config",
                         "-action", "~test_pa29",
                         "-action", "test_pa29_2",
                         "-bandwidth-limit", "1601",
                         "-burst-size-limit", "1602",
                         "-bandwidth-percent", "10",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer", "test_name29", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"policer", "test_name29", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name29\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa29_2\"],\n"
    "\"bandwidth-limit\":1601,\n"
    "\"burst-size-limit\":1602,\n"
    "\"bandwidth-percent\":10,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"policer", "test_name29", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv8[] = {"policer", "test_name29", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name29\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa29_2\"],\n"
    "\"bandwidth-limit\":1601,\n"
    "\"burst-size-limit\":1602,\n"
    "\"bandwidth-percent\":10,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"policer", "test_name29", NULL};
  const char test_str9[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name29\"}";
  const char *policer_action_argv1[] = {"policer-action", "test_pa29", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa29_2", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv3[] = {"policer-action", "test_pa29", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str3[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv4[] = {"policer-action", "test_pa29_2", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str4[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_cmd_update,
                 &ds, str, test_str6);

  /* aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name29", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, policer_cmd_update,
                 &ds, str, test_str8);

  /* aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name29", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv9), argv9, &tbl, policer_cmd_update,
                 &ds, str, test_str9);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv3), policer_action_argv3, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str3);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv4), policer_action_argv4, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str4);
}

void
test_policer_cmd_parse_atomic_abort_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer", "test_name30", "create",
                         "-action", "test_pa30",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name30", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name30\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa30\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"policer", "test_name30", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv4[] = {"policer", "test_name30", "config",
                         "-action", "~test_pa30",
                         "-action", "test_pa30_2",
                         "-bandwidth-limit", "1601",
                         "-burst-size-limit", "1602",
                         "-bandwidth-percent", "10",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer", "test_name30", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name30\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa30\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"policer", "test_name30", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name30\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa30_2\"],\n"
    "\"bandwidth-limit\":1601,\n"
    "\"burst-size-limit\":1602,\n"
    "\"bandwidth-percent\":10,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"policer", "test_name30", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name30\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa30\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"policer", "test_name30", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name30\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa30_2\"],\n"
    "\"bandwidth-limit\":1601,\n"
    "\"burst-size-limit\":1602,\n"
    "\"bandwidth-percent\":10,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"policer", "test_name30", NULL};
  const char test_str9[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name30\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa30\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv10[] = {"policer", "test_name30", "modified", NULL};
  const char test_str10[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv11[] = {"interface", "test_name30", "destroy",
                          NULL
                         };
  const char test_str11[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv1[] = {"policer-action", "test_pa30", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa30_2", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv3[] = {"policer-action", "test_pa30", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str3[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv4[] = {"policer-action", "test_pa30_2", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str4[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_cmd_update,
                 &ds, str, test_str6);

  /* aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name30", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, policer_cmd_update,
                 &ds, str, test_str8);

  /* aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name30", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv9), argv9, &tbl, policer_cmd_update,
                 &ds, str, test_str9);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv10), argv10, &tbl, policer_cmd_update,
                 &ds, str, test_str10);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv11), argv11, &tbl, policer_cmd_update,
                 &ds, str, test_str11);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv3), policer_action_argv3, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str3);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv4), policer_action_argv4, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str4);
}

void
test_policer_cmd_parse_atomic_destroy_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer", "test_name31", "create",
                         "-action", "test_pa31",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name31", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"policer", "test_name31", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name31\"}";
  const char *argv4[] = {"policer", "test_name31", "create",
                         "-action", "~test_pa31",
                         "-action", "test_pa31_2",
                         "-bandwidth-limit", "1601",
                         "-burst-size-limit", "1602",
                         "-bandwidth-percent", "10",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer", "test_name31", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name31\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa31\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"policer", "test_name31", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name31\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa31_2\"],\n"
    "\"bandwidth-limit\":1601,\n"
    "\"burst-size-limit\":1602,\n"
    "\"bandwidth-percent\":10,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"policer", "test_name31", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name31\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa31_2\"],\n"
    "\"bandwidth-limit\":1601,\n"
    "\"burst-size-limit\":1602,\n"
    "\"bandwidth-percent\":10,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"policer", "test_name31", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"policer", "test_name31", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv1[] = {"policer-action", "test_pa31", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa31_2", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv3[] = {"policer-action", "test_pa31", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str3[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv4[] = {"policer-action", "test_pa31_2", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str4[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_cmd_update,
                 &ds, str, test_str3);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name31", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name31", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv8), argv8, &tbl, policer_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, policer_cmd_update,
                 &ds, str, test_str9);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv3), policer_action_argv3, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str3);

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(policer_action_argv4), policer_action_argv4, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str4);
}

void
test_policer_cmd_parse_dryrun(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_DRYRUN;
  char *str = NULL;
  const char *argv1[] = {"policer", "test_name32", "create",
                         "-action", "test_pa32",
                         "-bandwidth-limit", "1501",
                         "-burst-size-limit", "1502",
                         "-bandwidth-percent", "1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer", "test_name32", "current", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name32\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa32\"],\n"
    "\"bandwidth-limit\":1501,\n"
    "\"burst-size-limit\":1502,\n"
    "\"bandwidth-percent\":1,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"policer", "test_name32", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv4[] = {"policer", "test_name32", "config",
                         "-action", "~test_pa32",
                         "-action", "test_pa32_2",
                         "-bandwidth-limit", "1601",
                         "-burst-size-limit", "1602",
                         "-bandwidth-percent", "10",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer", "test_name32", "current", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name32\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"test_pa32_2\"],\n"
    "\"bandwidth-limit\":1601,\n"
    "\"burst-size-limit\":1602,\n"
    "\"bandwidth-percent\":10,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"policer", "test_name32", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";

  const char *policer_action_argv1[] = {"policer-action", "test_pa32", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv2[] = {"policer-action", "test_pa32_2", "create",
                                        "-type", "discard",
                                        NULL
                                       };
  const char policer_action_test_str2[] = "{\"ret\":\"OK\"}";
  const char *policer_action_argv3[] = {"policer-action", "test_pa32_2", "destroy",
                                        NULL
                                       };
  const char policer_action_test_str3[] = "{\"ret\":\"OK\"}";

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(policer_action_argv1), policer_action_argv1, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str1);

  /* policer_action create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(policer_action_argv2), policer_action_argv2, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_cmd_update,
                 &ds, str, test_str6);

  TEST_POLICER_DESTROY(ret, &interp, state1, &tbl, &ds, str, "test_pa32",
                       "test_name32");

  /* policer_action destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(policer_action_argv3), policer_action_argv3, &tbl,
                 policer_action_cmd_update,
                 &ds, str, policer_action_test_str3);
}

void
test_destroy(void) {
  destroy = true;
}
