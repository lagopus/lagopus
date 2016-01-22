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
#include "../policer_action_cmd_internal.h"
#include "../policer_cmd_internal.h"
#include "../datastore_internal.h"

static lagopus_dstring_t ds = NULL;
static lagopus_hashmap_t tbl = NULL;
static datastore_interp_t interp = NULL;
static bool destroy = false;

void
setUp(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create interp. */
  INTERP_CREATE(ret, "policer-action", interp, tbl, ds);
}

void
tearDown(void) {
  /* destroy interp. */
  INTERP_DESTROY("policer-action", interp, tbl, ds, destroy);
}

void
test_policer_action_cmd_parse_create_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"policer-action", "test_name01", "create",
                        "-type", "discard",
                        NULL
                       };
  const char test_str[] = "{\"ret\":\"OK\"}";
  const char *argv1[] = {"policer-action", "test_name02", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name02", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"policer-action", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true},\n"
    "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name01\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"policer-action", "test_name02", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"policer-action", "test_name02", "disable",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *argv6[] = {"policer-action", "test_name02", "destroy",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"policer-action", "test_name01", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *policer_argv1[] = {"policer", "test_policer01", "create",
                                 "-action", "test_name02",
                                 NULL
                                };
  const char policer_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_argv2[] = {"policer", "test_policer01", "destroy",
                                 NULL
                                };
  const char policer_test_str2[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv), argv, &tbl, policer_action_cmd_update,
                 &ds, str, test_str);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* policer create cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(policer_argv1), policer_argv1, &tbl, policer_cmd_update,
                 &ds, str, policer_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* show all cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_action_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_action_cmd_update,
                 &ds, str, test_str4);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_action_cmd_update,
                 &ds, str, test_str5);

  /* policer destroy cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(policer_argv2), policer_argv2, &tbl, policer_cmd_update,
                 &ds, str, policer_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_action_cmd_update,
                 &ds, str, test_str6);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_action_cmd_update,
                 &ds, str, test_str7);
}

void
test_policer_action_cmd_parse_create_bad_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"policer-action", "test_name03", "create", "-hoge", NULL};
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad required options(-type), opt = -hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, policer_action_cmd_update, &ds, str, test_str);
}

void
test_policer_action_cmd_parse_create_bad_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"policer-action", "test_name04", "create",
                        "-type", "1", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = 1.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, policer_action_cmd_update, &ds, str, test_str);
}

void
test_policer_action_cmd_parse_create_bad_required_opts_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"policer-action", "test_name08", "create",
                        NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad required options(-type).\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, policer_action_cmd_update, &ds, str, test_str);
}

void
test_policer_action_cmd_parse_invalid_args(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"policer-action", "test_name", "hoge_sub_cmd", NULL};
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"sub_cmd = hoge_sub_cmd.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, policer_action_cmd_update, &ds, str, test_str);
}

void
test_policer_action_cmd_parse_enable_unused(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"policer-action", "test_name09", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name09", "enable",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name09. is not used.\"}";
  const char *argv3[] = {"policer-action", "test_name09", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name09\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"policer-action", "test_name09", "destroy",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_action_cmd_update,
                 &ds, str, test_str3);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_action_cmd_update,
                 &ds, str, test_str4);
}

void
test_policer_action_cmd_parse_config_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"policer-action", "test_name10", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name10", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name10\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"policer-action", "test_name10", "config",
                         "-type", "discard",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"policer-action", "test_name10", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name10\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"policer-action", "test_name10", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_action_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_action_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_action_cmd_update,
                 &ds, str, test_str5);
}

void
test_policer_action_cmd_parse_config_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"policer-action", "test_name11", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name11", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"policer-action", "test_name11", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv4[] = {"policer-action", "test_name11", "config",
                         "-type", "discard",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer-action", "test_name11", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv6[] = {"policer-action", "test_name11", "disable",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"policer-action", "test_name11", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *policer_argv1[] = {"policer", "test_policer02", "create",
                                 "-action", "test_name11",
                                 NULL
                                };
  const char policer_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_argv2[] = {"policer", "test_policer02", "destroy",
                                 NULL
                                };
  const char policer_test_str2[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* policer create cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(policer_argv1), policer_argv1, &tbl, policer_cmd_update,
                 &ds, str, policer_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_action_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_action_cmd_update,
                 &ds, str, test_str4);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_action_cmd_update,
                 &ds, str, test_str5);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_action_cmd_update,
                 &ds, str, test_str6);

  /* policer destroy cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(policer_argv2), policer_argv2, &tbl, policer_cmd_update,
                 &ds, str, policer_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_action_cmd_update,
                 &ds, str, test_str7);
}

void
test_policer_action_cmd_parse_config_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"policer-action", "test_name12", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name12", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name12\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"policer-action", "test_name12", "config",
                         "-type", "discard",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"policer-action", "test_name12", "config",
                         "-type", NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name12\",\n"
    "\"type\":\"discard\"}]}";
  const char *argv5[] = {"policer-action", "test_name12", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_action_cmd_update,
                 &ds, str, test_str3);

  /* config(show) cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_action_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_action_cmd_update,
                 &ds, str, test_str5);
}

void
test_policer_action_cmd_parse_destroy_used(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"policer-action", "test_name13", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name13", "disable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"policer-action", "test_name13", "destroy",
                         NULL
                        };
  const char test_str3[] = {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                            "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name13: is used.\"}"
                           };
  const char *policer_argv1[] = {"policer", "test_policer13", "create",
                                 "-action", "test_name13",
                                 NULL
                                };
  const char policer_test_str1[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* policer create cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(policer_argv1), policer_argv1, &tbl, policer_cmd_update,
                 &ds, str, policer_test_str1);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                 policer_action_cmd_update, &ds, str, test_str3);
}

void
test_policer_action_cmd_parse_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  char *str = NULL;
  const char *argv1[] = {"policer-action", "test_name14", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name14", "current",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name14\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"policer-action", "test_name14", "modified",
                         NULL
                        };
  const char test_str3[] =  {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                             "\"data\":\"Not set modified.\"}"
                            };
  const char *argv4[] = {"policer-action", "test_name14", "config",
                         "-type", "discard",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer-action", "test_name14", "modified",
                         NULL
                        };
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name14\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"policer-action", "test_name14", "config",
                         "-type", "discard",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"policer-action", "test_name14", "modified",
                         NULL
                        };
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name14\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"policer-action", "test_name14", "destroy",
                         NULL
                        };
  const char test_str8[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv3), argv3, &tbl,
                 policer_action_cmd_update, &ds, str, test_str3);

  /* config cmd (AUTO COMMIT mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_action_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (modified : AUTO COMMIT mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv5), argv5, &tbl,
                 policer_action_cmd_update, &ds, str, test_str5);

  /* config cmd (COMMITING mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state2,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_action_cmd_update,
                 &ds, str, test_str6);

  /* show cmd (modified : COMMITING mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv7), argv7, &tbl,
                 policer_action_cmd_update, &ds, str, test_str7);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv8), argv8, &tbl,
                 policer_action_cmd_update, &ds, str, test_str8);
}

void
test_policer_action_cmd_serialize_default_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* policer_action create cmd str. */
  const char *argv1[] = {"policer-action", "test_name27", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* policer_action destroy cmd str. */
  const char *argv2[] = {"policer-action", "test_name27", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] =
    "policer-action "DATASTORE_NAMESPACE_DELIMITER"test_name27 create "
    "-type discard\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_serialize, &interp,
                state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name27", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 policer_action_cmd_update, &ds, str, test_str2);
}

void
test_policer_action_cmd_serialize_default_opt_escape(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* policer_action create cmd str. */
  const char *argv1[] = {"policer-action", "test_\"name28", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* policer_action destroy cmd str. */
  const char *argv2[] = {"policer-action", "test_\"name28", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] =
    "policer-action \""DATASTORE_NAMESPACE_DELIMITER"test_\\\"name28\" create "
    "-type discard\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_serialize, &interp,
                state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_\"name28", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 policer_action_cmd_update, &ds, str, test_str2);
}

void
test_policer_action_cmd_serialize_default_opt_escape_white_space(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* policer_action create cmd str. */
  const char *argv1[] = {"policer-action", "test name29", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* policer_action destroy cmd str. */
  const char *argv2[] = {"policer-action", "test name29", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] =
    "policer-action \""DATASTORE_NAMESPACE_DELIMITER"test name29\" create "
    "-type discard\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_serialize, &interp,
                state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test name29", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 policer_action_cmd_update, &ds, str, test_str2);
}

void
test_policer_action_cmd_parse_atomic_commit(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer-action", "test_name36", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name36", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"policer-action", "test_name36", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name36\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"policer-action", "test_name36", "config",
                         "-type", "discard",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer-action", "test_name36", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"policer-action", "test_name36", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name36\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"policer-action", "test_name36", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name36\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"policer-action", "test_name36", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"policer-action", "test_name36", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_action_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_action_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_action_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_action_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state2,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name36", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state3,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name36", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_action_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, policer_action_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(argv9), argv9, &tbl, policer_action_cmd_update,
                 &ds, str, test_str9);
}

void
test_policer_action_cmd_parse_atomic_rollback(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ROLLBACKING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ROLLBACKED;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer-action", "test_name37", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name37", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"policer-action", "test_name37", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name37\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"policer-action", "test_name37", "config",
                         "-type", "discard",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer-action", "test_name37", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"policer-action", "test_name37", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name37\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"policer-action", "test_name37", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name37\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_action_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_action_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_action_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_action_cmd_update,
                 &ds, str, test_str6);

  /* rollbacking. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state2,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name37", conf, &ds);

  /* rollbacked. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state3,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name37", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_action_cmd_update,
                 &ds, str, test_str7);
}

void
test_policer_action_cmd_parse_atomic_delay_enable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer-action", "test_name38", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name38", "enable", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"policer-action", "test_name38", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv4[] = {"policer-action", "test_name38", "modified", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name38\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"policer-action", "test_name38", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"policer-action", "test_name38", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name38\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv7[] = {"policer-action", "test_name38", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name38\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv8[] = {"policer-action", "test_name38", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"policer-action", "test_name38", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";
  const char *policer_argv1[] = {"policer", "test_policer38", "create",
                                 "-action", "test_name38",
                                 NULL
                                };
  const char policer_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_argv2[] = {"policer", "test_policer38", "destroy",
                                 NULL
                                };
  const char policer_test_str2[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* policer create cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state1,
                 ARGV_SIZE(policer_argv1), policer_argv1, &tbl, policer_cmd_update,
                 &ds, str, policer_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_action_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_action_cmd_update,
                 &ds, str, test_str4);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state2,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name38", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_action_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_action_cmd_update,
                 &ds, str, test_str6);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state3,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name38", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_action_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, policer_action_cmd_update,
                 &ds, str, test_str8);

  /* policer destroy cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state4,
                 ARGV_SIZE(policer_argv2), policer_argv2, &tbl, policer_cmd_update,
                 &ds, str, policer_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(argv9), argv9, &tbl, policer_action_cmd_update,
                 &ds, str, test_str9);
}

void
test_policer_action_cmd_parse_atomic_delay_disable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer-action", "test_name39", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name39", "enable", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"policer-action", "test_name39", "disable", NULL};
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"policer-action", "test_name39", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name39\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"policer-action", "test_name39", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name39\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"policer-action", "test_name39", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name39\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"policer-action", "test_name39", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *policer_argv1[] = {"policer", "test_policer39", "create",
                                 "-action", "test_name39",
                                 NULL
                                };
  const char policer_test_str1[] = "{\"ret\":\"OK\"}";
  const char *policer_argv2[] = {"policer", "test_policer39", "destroy",
                                 NULL
                                };
  const char policer_test_str2[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* policer create cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state4,
                 ARGV_SIZE(policer_argv1), policer_argv1, &tbl, policer_cmd_update,
                 &ds, str, policer_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_action_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_action_cmd_update,
                 &ds, str, test_str4);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state2,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name39", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_action_cmd_update,
                 &ds, str, test_str5);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state3,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name39", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_action_cmd_update,
                 &ds, str, test_str6);

  /* policer destroy cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state4,
                 ARGV_SIZE(policer_argv2), policer_argv2, &tbl, policer_cmd_update,
                 &ds, str, policer_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_action_cmd_update,
                 &ds, str, test_str7);
}

void
test_policer_action_cmd_parse_atomic_delay_destroy(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer-action", "test_name40", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name40", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"policer-action", "test_name40", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name40\"}";
  const char *argv4[] = {"policer-action", "test_name40", "enable",
                         NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name40\"}";
  const char *argv5[] = {"policer-action", "test_name40", "disable",
                         NULL
                        };
  const char test_str5[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name40\"}";
  const char *argv6[] = {"policer-action", "test_name40", "destroy",
                         NULL
                        };
  const char test_str6[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name40\"}";
  const char *argv7[] = {"policer-action", "test_name40", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name40\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_action_cmd_update,
                 &ds, str, test_str3);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_action_cmd_update,
                 &ds, str, test_str4);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_action_cmd_update,
                 &ds, str, test_str5);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_action_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state2,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name40", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state3,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name40", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_action_cmd_update,
                 &ds, str, test_str7);
}

void
test_policer_action_cmd_parse_atomic_abort_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer-action", "test_name41", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name41", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"policer-action", "test_name41", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name41\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"policer-action", "test_name41", "config",
                         "-type", "discard",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer-action", "test_name41", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"policer-action", "test_name41", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name41\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"policer-action", "test_name41", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv8[] = {"policer-action", "test_name41", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name41\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"policer-action", "test_name41", NULL};
  const char test_str9[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name41\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_action_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_action_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_action_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_action_cmd_update,
                 &ds, str, test_str6);

  /* aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state2,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name41", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_action_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv8), argv8, &tbl, policer_action_cmd_update,
                 &ds, str, test_str8);

  /* aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state3,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name41", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv9), argv9, &tbl, policer_action_cmd_update,
                 &ds, str, test_str9);
}

void
test_policer_action_cmd_parse_atomic_abort_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer-action", "test_name42", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name42", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"policer-action", "test_name42", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv4[] = {"policer-action", "test_name42", "config",
                         "-type", "discard",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer-action", "test_name42", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"policer-action", "test_name42", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"policer-action", "test_name42", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"policer-action", "test_name42", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"policer-action", "test_name42", NULL};
  const char test_str9[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv10[] = {"policer-action", "test_name42", "modified", NULL};
  const char test_str10[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv11[] = {"policer-action", "test_name42", "destroy",
                          NULL
                         };
  const char test_str11[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_action_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_action_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_action_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_action_cmd_update,
                 &ds, str, test_str6);

  /*  aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state2,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name42", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_action_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv8), argv8, &tbl, policer_action_cmd_update,
                 &ds, str, test_str8);

  /*  aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state3,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name42", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv9), argv9, &tbl, policer_action_cmd_update,
                 &ds, str, test_str9);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv10), argv10, &tbl, policer_action_cmd_update,
                 &ds, str, test_str10);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(argv11), argv11, &tbl, policer_action_cmd_update,
                 &ds, str, test_str11);

}

void
test_policer_action_cmd_parse_atomic_destroy_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"policer-action", "test_name43", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name43", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"policer-action", "test_name43", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name43\"}";
  const char *argv4[] = {"policer-action", "test_name43", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer-action", "test_name43", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name43\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"policer-action", "test_name43", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name43\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"policer-action", "test_name43", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name43\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"policer-action", "test_name43", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"policer-action", "test_name43", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_action_cmd_update,
                 &ds, str, test_str3);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_action_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_action_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_action_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state2,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name43", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, policer_action_cmd_update, &interp, state3,
            &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name43", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_action_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv8), argv8, &tbl, policer_action_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state4,
                 ARGV_SIZE(argv9), argv9, &tbl, policer_action_cmd_update,
                 &ds, str, test_str9);
}

void
test_policer_action_cmd_parse_dryrun(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_DRYRUN;
  char *str = NULL;
  const char *argv1[] = {"policer-action", "test_name44", "create",
                         "-type", "discard",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"policer-action", "test_name44", "current", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name44\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"policer-action", "test_name44", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv4[] = {"policer-action", "test_name44", "config",
                         "-type", "discard",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"policer-action", "test_name44", "current", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name44\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"policer-action", "test_name44", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv7[] = {"policer-action", "test_name44", "current", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name44\",\n"
    "\"type\":\"discard\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"policer-action", "test_name44", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv1), argv1, &tbl, policer_action_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv2), argv2, &tbl, policer_action_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, policer_action_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state2,
                 ARGV_SIZE(argv4), argv4, &tbl, policer_action_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state2,
                 ARGV_SIZE(argv5), argv5, &tbl, policer_action_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv6), argv6, &tbl, policer_action_cmd_update,
                 &ds, str, test_str6);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_action_cmd_parse, &interp,
                 state1,
                 ARGV_SIZE(argv7), argv7, &tbl, policer_action_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 policer_action_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, policer_action_cmd_update,
                 &ds, str, test_str8);
}

void
test_destroy(void) {
  destroy = true;
}
