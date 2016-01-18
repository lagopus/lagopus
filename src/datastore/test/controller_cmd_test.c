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
#include "../controller_cmd_internal.h"
#include "../channel_cmd_internal.h"
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
  INTERP_CREATE(ret, "controller", interp, tbl, ds);
}

void
tearDown(void) {
  /* destroy interp. */
  INTERP_DESTROY("controller", interp, tbl, ds, destroy);
}

void
test_controller_cmd_parse_create_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"controller", "test_name01", "create",
                        "-channel", "test_cha01",
                        "-role", "equal",
                        "-connection-type", "main",
                        NULL
                       };
  const char test_str[] = "{\"ret\":\"OK\"}";
  const char *argv1[] = {"controller", "test_name02", "create",
                         "-channel", "test_cha\"02",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name02", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"controller", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha\\\"02\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true},\n"
    "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name01\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha01\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"controller", "test_name02", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha\\\"02\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"controller", "test_name02", "disable",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *argv6[] = {"controller", "test_name02", "destroy",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"controller", "test_name01", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *channel_argv1[] = {"channel", "test_cha01", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha\"02", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";
  const char *channel_argv3[] = {"channel", "test_cha\"02", NULL};
  const char channel_test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha\\\"02\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *channel_argv4[] = {"channel", "test_cha\"02", NULL};
  const char channel_test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha\\\"02\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *channel_argv5[] = {"channel", "test_cha\"02", "destroy",
                                 NULL
                                };
  const char channel_test_str5[] = "{\"ret\":\"OK\"}";
  const char *channel_argv6[] = {"channel", "test_cha01", "destroy",
                                 NULL
                                };
  const char channel_test_str6[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv1[] = {"bridge", "bridge_name01", "create",
                                "-controller", "test_name02",
                                NULL
                               };
  const char bridge_test_str1[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv2[] = {"bridge", "bridge_name01", "destroy",
                                NULL
                               };
  const char bridge_test_str2[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv), argv, &tbl, controller_cmd_update,
                 &ds, str, test_str);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv1), bridge_argv1, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* show all cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, controller_cmd_update,
                 &ds, str, test_str4);

  /* channel show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv3), channel_argv3, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str3);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, controller_cmd_update,
                 &ds, str, test_str5);

  /* bridge destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv2), bridge_argv2, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, controller_cmd_update,
                 &ds, str, test_str6);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, controller_cmd_update,
                 &ds, str, test_str7);

  /* channel show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv4), channel_argv4, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str4);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv5), channel_argv5, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str5);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv6), channel_argv6, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str6);
}

void
test_controller_cmd_parse_create_bad_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"controller", "test_name03", "create", "-hoge", NULL};
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"opt = -hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, controller_cmd_update, &ds, str, test_str);
}

void
test_controller_cmd_parse_create_not_channel_name_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"controller", "test_name04", "create",
                        "-channel", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, controller_cmd_update, &ds, str, test_str);
}

void
test_controller_cmd_parse_create_not_role_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"controller", "test_name05", "create",
                        "-role", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, controller_cmd_update, &ds, str, test_str);
}

void
test_controller_cmd_parse_create_not_connection_type_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"controller", "test_name06", "create",
                        "-connection-type", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, controller_cmd_update, &ds, str, test_str);
}

void
test_controller_cmd_parse_create_bad_role(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"controller", "test_name07", "create",
                        "-role", "hoge", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, controller_cmd_update, &ds, str, test_str);
}

void
test_controller_cmd_parse_create_bad_connection_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"controller", "test_name08", "create",
                        "-connection-type", "hoge", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, controller_cmd_update, &ds, str, test_str);
}

void
test_controller_cmd_parse_enable_unused(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"controller", "test_name09", "create",
                         "-channel", "test_cha03",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name09", "enable",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name09. is not used.\"}";
  const char *argv3[] = {"controller", "test_name09", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name09\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha03\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"controller", "test_name09", "destroy",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *channel_argv1[] = {"channel", "test_cha03", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha03", "destroy",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, controller_cmd_update,
                 &ds, str, test_str4);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);
}

void
test_controller_cmd_parse_create_not_exists(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"controller", "test_name10", "create",
                         "-channel", "test_cha100",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"channel name = "DATASTORE_NAMESPACE_DELIMITER"test_cha100.\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);
}

void
test_controller_cmd_parse_config_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"controller", "test_name11", "create",
                         "-channel", "test_cha04",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name11", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha04\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"controller", "test_name11", "config",
                         "-channel", "test_cha05",
                         "-role", "master",
                         "-connection-type", "auxiliary",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"controller", "test_name11", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha05\",\n"
    "\"role\":\"master\",\n"
    "\"connection-type\":\"auxiliary\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"controller", "test_name11", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *channel_argv1[] = {"channel", "test_cha04", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha04", NULL};
  const char channel_test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha04\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *channel_argv3[] = {"channel", "test_cha04", NULL};
  const char channel_test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha04\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *channel_argv4[] = {"channel", "test_cha05", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str4[] = "{\"ret\":\"OK\"}";
  const char *channel_argv5[] = {"channel", "test_cha05", NULL};
  const char channel_test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha05\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *channel_argv6[] = {"channel", "test_cha05", NULL};
  const char channel_test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha05\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *channel_argv7[] = {"channel", "test_cha04", "destroy",
                                 NULL
                                };
  const char channel_test_str7[] = "{\"ret\":\"OK\"}";
  const char *channel_argv8[] = {"channel", "test_cha05", "destroy",
                                 NULL
                                };
  const char channel_test_str8[] = "{\"ret\":\"OK\"}";


  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv4), channel_argv4, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str4);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* channel show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);

  /* channel show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv5), channel_argv5, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str5);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* channel show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv3), channel_argv3, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str3);

  /* channel show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv6), channel_argv6, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str6);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, controller_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, controller_cmd_update,
                 &ds, str, test_str5);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv7), channel_argv7, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str7);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv8), channel_argv8, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str8);
}

void
test_controller_cmd_parse_config_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"controller", "test_name11", "create",
                         "-channel", "test_cha06",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name11", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"controller", "test_name11", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha06\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv4[] = {"controller", "test_name11", "config",
                         "-role", "master",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"controller", "test_name11", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha06\",\n"
    "\"role\":\"master\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv6[] = {"controller", "test_name11", "disable",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"controller", "test_name11", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *channel_argv1[] = {"channel", "test_cha06", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha06", "destroy",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv1[] = {"bridge", "bridge_name02", "create",
                                "-controller", "test_name11",
                                NULL
                               };
  const char bridge_test_str1[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv2[] = {"bridge", "bridge_name02", "destroy",
                                NULL
                               };
  const char bridge_test_str2[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv1), bridge_argv1, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, controller_cmd_update,
                 &ds, str, test_str4);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, controller_cmd_update,
                 &ds, str, test_str5);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, controller_cmd_update,
                 &ds, str, test_str6);

  /* bridge destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv2), bridge_argv2, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, controller_cmd_update,
                 &ds, str, test_str7);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);
}

void
test_controller_cmd_parse_config_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"controller", "test_name13", "create",
                         "-channel", "test_cha09",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name13", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name13\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha09\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"controller", "test_name13", "config",
                         "-role", "master",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"controller", "test_name13", "config",
                         "-role", NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name13\",\n"
    "\"role\":\"master\"}]}";
  const char *argv5[] = {"controller", "test_name13", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *channel_argv1[] = {"channel", "test_cha09", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha09", "destroy",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* config cmd (show). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, controller_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, controller_cmd_update,
                 &ds, str, test_str5);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);
}

void
test_controller_cmd_parse_destroy_used(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"controller", "test_name14", "create",
                         "-channel", "test_cha14",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name14", "disable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"controller", "test_name14", "destroy",
                         NULL
                        };
  const char test_str3[] = {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                            "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name14: is used.\"}"
                           };
  const char *argv4[] = {"controller", "test_name14", "destroy",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *channel_argv1[] = {"channel", "test_cha14", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha14", "destroy",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv1[] = {"bridge", "bridge_name14", "create",
                                "-controller", "test_name14",
                                NULL
                               };
  const char bridge_test_str1[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv2[] = {"bridge", "bridge_name14", "destroy",
                                NULL
                               };
  const char bridge_test_str2[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv1), bridge_argv1, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str1);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                 controller_cmd_update, &ds, str, test_str3);

  /* bridge destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv2), bridge_argv2, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse,
                 &interp, state, ARGV_SIZE(argv4), argv4, &tbl,
                 controller_cmd_update, &ds, str, test_str4);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);

}

void
test_controller_cmd_parse_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  char *str = NULL;
  const char *argv1[] = {"controller", "test_name15", "create",
                         "-channel", "test_cha15",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name15", "current",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name15\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha15\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"controller", "test_name15", "modified",
                         NULL
                        };
  const char test_str3[] =  {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                             "\"data\":\"Not set modified.\"}"
                            };
  const char *argv4[] = {"controller", "test_name15", "config",
                         "-role", "master",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"controller", "test_name15", "modified",
                         NULL
                        };
  const char test_str5[] =  {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                             "\"data\":\"Not set modified.\"}"
                            };
  const char *argv6[] = {"controller", "test_name15", "config",
                         "-role", "slave",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"controller", "test_name15", "modified",
                         NULL
                        };
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name15\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha15\",\n"
    "\"role\":\"slave\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"controller", "test_name15", "destroy",
                         NULL
                        };
  const char test_str8[] = "{\"ret\":\"OK\"}";
  const char *channel_argv1[] = {"channel", "test_cha15", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha15", "destroy",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv3), argv3, &tbl,
                 controller_cmd_update, &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, controller_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (modified : AUTO COMMIT mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv5), argv5, &tbl,
                 controller_cmd_update, &ds, str, test_str5);

  /* config cmd (COMMITING mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv6), argv6, &tbl, controller_cmd_update,
                 &ds, str, test_str6);

  /* show cmd (modified : COMMITING mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, controller_cmd_update,
                 &ds, str, test_str7);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, controller_cmd_update,
                 &ds, str, test_str8);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);
}

void
test_controller_cmd_serialize_default_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* controller create cmd str. */
  const char *argv1[] = {"controller", "test_name16", "create",
                         "-channel", "test_cha16", NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"controller", "test_name16", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "controller "
                                DATASTORE_NAMESPACE_DELIMITER"test_name16 create "
                                "-channel "DATASTORE_NAMESPACE_DELIMITER"test_cha16 "
                                "-role equal "
                                "-connection-type main\n";

  /* channel. */
  const char *channel_argv1[] = {"channel", "test_cha16", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha16", "destroy",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* controller create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name16", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 controller_cmd_update, &ds, str, test_str2);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);
}

void
test_controller_cmd_serialize_default_opt_escape(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* controller create cmd str. */
  const char *argv1[] = {"controller", "test_\"name17", "create",
                         "-channel", "test_cha17", NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"controller", "test_\"name17", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "controller "
                                "\""DATASTORE_NAMESPACE_DELIMITER"test_\\\"name17\" create "
                                "-channel "DATASTORE_NAMESPACE_DELIMITER"test_cha17 "
                                "-role equal "
                                "-connection-type main\n";
  /* channel. */
  const char *channel_argv1[] = {"channel", "test_cha17", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha17", "destroy",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* controller create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_\"name17", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 controller_cmd_update, &ds, str, test_str2);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);
}

void
test_controller_cmd_serialize_default_opt_escape_white_space(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* controller create cmd str. */
  const char *argv1[] = {"controller", "test name18", "create",
                         "-channel", "test_cha18", NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"controller", "test name18", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "controller "
                                "\""DATASTORE_NAMESPACE_DELIMITER"test name18\" create "
                                "-channel "DATASTORE_NAMESPACE_DELIMITER"test_cha18 "
                                "-role equal "
                                "-connection-type main\n";

  /* channel. */
  const char *channel_argv1[] = {"channel", "test_cha18", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha18", "destroy",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test name18", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 controller_cmd_update, &ds, str, test_str2);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);
}

void
test_controller_cmd_serialize_all_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* channel create cmd str. */
  const char *channel_argv[] = {"channel", "test_channel19", "create", NULL};
  const char channel_str[] = "{\"ret\":\"OK\"}";

  /* controller create cmd str. */
  const char *argv1[] = {"controller", "test_name19", "create",
                         "-channel", "test_channel19",
                         "-role", "master",
                         "-connection-type", "auxiliary",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* controller create cmd. */
  const char *argv2[] = {"controller", "test_name19", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "controller "
                                DATASTORE_NAMESPACE_DELIMITER"test_name19 create "
                                "-channel "DATASTORE_NAMESPACE_DELIMITER"test_channel19 "
                                "-role master "
                                "-connection-type auxiliary\n";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv), channel_argv, &tbl, channel_cmd_update,
                 &ds, str, channel_str);

  /* controller create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name19", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 controller_cmd_update, &ds, str, test_str2);
}

void
test_controller_cmd_serialize_all_opt_escape(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* channel create cmd str. */
  const char *channel_argv[] = {"channel", "test_\"channel20", "create", NULL};
  const char channel_str[] = "{\"ret\":\"OK\"}";

  /* controller create cmd str. */
  const char *argv1[] = {"controller", "test_name20", "create",
                         "-channel", "test_\"channel20",
                         "-role", "master",
                         "-connection-type", "auxiliary",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* controller create cmd. */
  const char *argv2[] = {"controller", "test_name20", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "controller "
                                DATASTORE_NAMESPACE_DELIMITER"test_name20 create "
                                "-channel \""DATASTORE_NAMESPACE_DELIMITER"test_\\\"channel20\" "
                                "-role master "
                                "-connection-type auxiliary\n";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv), channel_argv, &tbl, channel_cmd_update,
                 &ds, str, channel_str);

  /* controller create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name20", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 controller_cmd_update, &ds, str, test_str2);
}

void
test_controller_cmd_serialize_all_opt_escape_white_space(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* channel create cmd str. */
  const char *channel_argv[] = {"channel", "test channel21", "create", NULL};
  const char channel_str[] = "{\"ret\":\"OK\"}";

  /* interface create cmd str. */
  const char *argv1[] = {"controller", "test_name21", "create",
                         "-channel", "test channel21",
                         "-role", "master",
                         "-connection-type", "auxiliary",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"controller", "test_name21", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "controller "
                                DATASTORE_NAMESPACE_DELIMITER"test_name21 create "
                                "-channel \""DATASTORE_NAMESPACE_DELIMITER"test channel21\" "
                                "-role master "
                                "-connection-type auxiliary\n";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv), channel_argv, &tbl, channel_cmd_update,
                 &ds, str, channel_str);

  /* controller create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name21", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 controller_cmd_update, &ds, str, test_str2);
}

void
test_controller_cmd_parse_stats_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"controller", "test_name22", "create",
                         "-channel", "test_cha22",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name22", "stats",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name22\",\n"
    "\"is-connected\":false,\n"
    "\"supported-versions\":[\"1.3\"],\n"
    "\"role\":\"equal\"}]}";
  const char *argv3[] = {"controller", "test_name22", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *channel_argv1[] = {"channel", "test_cha22", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha22", "destroy",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* stats cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);
}

void
test_controller_cmd_parse_stats_invalid_args(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"controller", "test_name23", "create",
                         "-channel", "test_cha23",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name23", "stats",
                         "hoge",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"INVALID_ARGS\",\n"
                           "\"data\":\"Bad opt = hoge.\"}";
  const char *argv3[] = {"controller", "test_name23", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *channel_argv1[] = {"channel", "test_cha23", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha23", "destroy",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* stats cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);
}

void
test_controller_cmd_parse_atomic_commit(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"controller", "test_name24", "create",
                         "-channel", "test_cha24",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name24", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"controller", "test_name24", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name24\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha24\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"controller", "test_name24", "config",
                         "-channel", "test_cha24_2",
                         "-role", "master",
                         "-connection-type", "auxiliary",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"controller", "test_name24", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"controller", "test_name24", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name24\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha24_2\",\n"
    "\"role\":\"master\",\n"
    "\"connection-type\":\"auxiliary\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"controller", "test_name24", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name24\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha24_2\",\n"
    "\"role\":\"master\",\n"
    "\"connection-type\":\"auxiliary\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"controller", "test_name24", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"controller", "test_name24", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";
  const char *channel_argv1[] = {"channel", "test_cha24", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha24_2", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";
  const char *channel_argv3[] = {"channel", "test_cha24", "destroy",
                                 NULL
                                };
  const char channel_test_str3[] = "{\"ret\":\"OK\"}";
  const char *channel_argv4[] = {"channel", "test_cha24_2", "destroy",
                                 NULL
                                };
  const char channel_test_str4[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, controller_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, controller_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, controller_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name24", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name24", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, controller_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, controller_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, controller_cmd_update,
                 &ds, str, test_str9);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv3), channel_argv3, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str3);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv4), channel_argv4, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str4);
}

void
test_controller_cmd_parse_atomic_rollback(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ROLLBACKING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ROLLBACKED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"controller", "test_name25", "create",
                         "-channel", "test_cha25",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name25", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"controller", "test_name25", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name25\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha25\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"controller", "test_name25", "config",
                         "-channel", "test_cha25_2",
                         "-role", "master",
                         "-connection-type", "auxiliary",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"controller", "test_name25", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"controller", "test_name25", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name25\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha25_2\",\n"
    "\"role\":\"master\",\n"
    "\"connection-type\":\"auxiliary\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"controller", "test_name25", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = :test_name25\"}";
  const char *channel_argv1[] = {"channel", "test_cha25", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha25_2", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";
  const char *channel_argv3[] = {"channel", "test_cha25", "destroy",
                                 NULL
                                };
  const char channel_test_str3[] = "{\"ret\":\"OK\"}";
  const char *channel_argv4[] = {"channel", "test_cha25_2", "destroy",
                                 NULL
                                };
  const char channel_test_str4[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, controller_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, controller_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, controller_cmd_update,
                 &ds, str, test_str6);

  /* rollbacking. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name25", conf, &ds);

  /* rollbacked. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name25", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, controller_cmd_update,
                 &ds, str, test_str7);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv3), channel_argv3, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str3);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv4), channel_argv4, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str4);
}

void
test_controller_cmd_parse_atomic_delay_enable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"controller", "test_name26", "create",
                         "-channel", "test_cha26",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name26", "enable", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"controller", "test_name26", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv4[] = {"controller", "test_name26", "modified", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name26\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha26\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"controller", "test_name26", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"controller", "test_name26", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name26\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha26\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv7[] = {"controller", "test_name26", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name26\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha26\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv8[] = {"controller", "test_name26", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"controller", "test_name26", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";
  const char *channel_argv1[] = {"channel", "test_cha26", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha26", "destroy",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv1[] = {"bridge", "bridge_name26", "create",
                                "-controller", "test_name26",
                                NULL
                               };
  const char bridge_test_str1[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv2[] = {"bridge", "bridge_name26", "destroy",
                                NULL
                               };
  const char bridge_test_str2[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(bridge_argv1), bridge_argv1, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, controller_cmd_update,
                 &ds, str, test_str4);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name26", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, controller_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, controller_cmd_update,
                 &ds, str, test_str6);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name26", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, controller_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, controller_cmd_update,
                 &ds, str, test_str8);

  /* bridge destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(bridge_argv2), bridge_argv2, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str2);


  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, controller_cmd_update,
                 &ds, str, test_str9);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);
}

void
test_controller_cmd_parse_atomic_delay_disable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"controller", "test_name27", "create",
                         "-channel", "test_cha27",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name27", "enable", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"controller", "test_name27", "disable", NULL};
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"controller", "test_name27", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name27\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha27\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"controller", "test_name27", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name27\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha27\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"controller", "test_name27", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name27\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha27\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"controller", "test_name27", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *channel_argv1[] = {"channel", "test_cha27", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha27", "destroy",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv1[] = {"bridge", "bridge_name27", "create",
                                "-controller", "test_name27",
                                NULL
                               };
  const char bridge_test_str1[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv2[] = {"bridge", "bridge_name27", "destroy",
                                NULL
                               };
  const char bridge_test_str2[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(bridge_argv1), bridge_argv1, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, controller_cmd_update,
                 &ds, str, test_str4);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name27", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, controller_cmd_update,
                 &ds, str, test_str5);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name27", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, controller_cmd_update,
                 &ds, str, test_str6);

  /* bridge destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(bridge_argv2), bridge_argv2, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv7), argv7, &tbl, controller_cmd_update,
                 &ds, str, test_str7);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);
}

void
test_controller_cmd_parse_atomic_delay_destroy(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"controller", "test_name28", "create",
                         "-channel", "test_cha28",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name28", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"controller", "test_name28", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name28\"}";
  const char *argv4[] = {"controller", "test_name28", "enable",
                         NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name28\"}";
  const char *argv5[] = {"controller", "test_name28", "disable",
                         NULL
                        };
  const char test_str5[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name28\"}";
  const char *argv6[] = {"controller", "test_name28", "destroy",
                         NULL
                        };
  const char test_str6[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name28\"}";
  const char *argv7[] = {"controller", "test_name28", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name28\"}";
  const char *channel_argv1[] = {"channel", "test_cha28", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha28", "destroy",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, controller_cmd_update,
                 &ds, str, test_str4);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, controller_cmd_update,
                 &ds, str, test_str5);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, controller_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name28", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name28", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, controller_cmd_update,
                 &ds, str, test_str7);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);
}

void
test_controller_cmd_parse_atomic_abort_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"controller", "test_name29", "create",
                         "-channel", "test_cha29",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name29", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"controller", "test_name29", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name29\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha29\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"controller", "test_name29", "config",
                         "-channel", "test_cha29_2",
                         "-role", "master",
                         "-connection-type", "auxiliary",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"controller", "test_name29", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"controller", "test_name29", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name29\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha29_2\",\n"
    "\"role\":\"master\",\n"
    "\"connection-type\":\"auxiliary\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"controller", "test_name29", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv8[] = {"controller", "test_name29", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name29\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha29_2\",\n"
    "\"role\":\"master\",\n"
    "\"connection-type\":\"auxiliary\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"controller", "test_name29", NULL};
  const char test_str9[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name29\"}";
  const char *channel_argv1[] = {"channel", "test_cha29", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha29_2", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";
  const char *channel_argv3[] = {"channel", "test_cha29", "destroy",
                                 NULL
                                };
  const char channel_test_str3[] = "{\"ret\":\"OK\"}";
  const char *channel_argv4[] = {"channel", "test_cha29_2", "destroy",
                                 NULL
                                };
  const char channel_test_str4[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, controller_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, controller_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, controller_cmd_update,
                 &ds, str, test_str6);

  /* aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name29", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, controller_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, controller_cmd_update,
                 &ds, str, test_str8);

  /* aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name29", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv9), argv9, &tbl, controller_cmd_update,
                 &ds, str, test_str9);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv3), channel_argv3, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str3);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv4), channel_argv4, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str4);
}

void
test_controller_cmd_parse_atomic_abort_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"controller", "test_name30", "create",
                         "-channel", "test_cha30",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name30", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name30\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha30\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"controller", "test_name30", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv4[] = {"controller", "test_name30", "config",
                         "-channel", "test_cha30_2",
                         "-role", "master",
                         "-connection-type", "auxiliary",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"controller", "test_name30", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name30\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha30\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"controller", "test_name30", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name30\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha30_2\",\n"
    "\"role\":\"master\",\n"
    "\"connection-type\":\"auxiliary\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"controller", "test_name30", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name30\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha30\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"controller", "test_name30", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name30\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha30_2\",\n"
    "\"role\":\"master\",\n"
    "\"connection-type\":\"auxiliary\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"controller", "test_name30", NULL};
  const char test_str9[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name30\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha30\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv10[] = {"controller", "test_name30", "modified", NULL};
  const char test_str10[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv11[] = {"interface", "test_name30", "destroy",
                          NULL
                         };
  const char test_str11[] = "{\"ret\":\"OK\"}";
  const char *channel_argv1[] = {"channel", "test_cha30", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha30_2", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";
  const char *channel_argv3[] = {"channel", "test_cha30", "destroy",
                                 NULL
                                };
  const char channel_test_str3[] = "{\"ret\":\"OK\"}";
  const char *channel_argv4[] = {"channel", "test_cha30_2", "destroy",
                                 NULL
                                };
  const char channel_test_str4[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, controller_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, controller_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, controller_cmd_update,
                 &ds, str, test_str6);

  /* aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name30", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, controller_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, controller_cmd_update,
                 &ds, str, test_str8);

  /* aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name30", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv9), argv9, &tbl, controller_cmd_update,
                 &ds, str, test_str9);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv10), argv10, &tbl, controller_cmd_update,
                 &ds, str, test_str10);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv11), argv11, &tbl, controller_cmd_update,
                 &ds, str, test_str11);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv3), channel_argv3, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str3);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv4), channel_argv4, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str4);
}

void
test_controller_cmd_parse_atomic_destroy_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"controller", "test_name31", "create",
                         "-channel", "test_cha31",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name31", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"controller", "test_name31", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name31\"}";
  const char *argv4[] = {"controller", "test_name31", "create",
                         "-channel", "test_cha31_2",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"controller", "test_name31", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name31\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha31\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"controller", "test_name31", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name31\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha31_2\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"controller", "test_name31", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name31\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha31_2\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"controller", "test_name31", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"controller", "test_name31", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";
  const char *channel_argv1[] = {"channel", "test_cha31", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha31_2", "create",
                                 "-dst-addr", "127.0.0.2",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.2",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";
  const char *channel_argv3[] = {"channel", "test_cha31", "destroy",
                                 NULL
                                };
  const char channel_test_str3[] = "{\"ret\":\"OK\"}";
  const char *channel_argv4[] = {"channel", "test_cha31_2", "destroy",
                                 NULL
                                };
  const char channel_test_str4[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, controller_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, controller_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, controller_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name31", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, controller_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name31", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv7), argv7, &tbl, controller_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv8), argv8, &tbl, controller_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, controller_cmd_update,
                 &ds, str, test_str9);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv3), channel_argv3, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str3);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(channel_argv4), channel_argv4, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str4);
}

void
test_controller_cmd_parse_dryrun(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_DRYRUN;
  char *str = NULL;
  const char *argv1[] = {"controller", "test_name32", "create",
                         "-channel", "test_cha32",
                         "-role", "equal",
                         "-connection-type", "main",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"controller", "test_name32", "current", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name32\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha32\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"controller", "test_name32", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv4[] = {"controller", "test_name32", "config",
                         "-channel", "test_cha32_2",
                         "-role", "master",
                         "-connection-type", "auxiliary",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"controller", "test_name32", "current", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name32\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"test_cha32_2\",\n"
    "\"role\":\"master\",\n"
    "\"connection-type\":\"auxiliary\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"controller", "test_name32", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";


  const char *channel_argv1[] = {"channel", "test_cha32", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str1[] = "{\"ret\":\"OK\"}";
  const char *channel_argv2[] = {"channel", "test_cha32_2", "create",
                                 "-dst-addr", "127.0.0.1",
                                 "-dst-port", "3000",
                                 "-local-addr", "127.0.0.1",
                                 "-local-port", "3000",
                                 "-protocol", "tcp",
                                 NULL
                                };
  const char channel_test_str2[] = "{\"ret\":\"OK\"}";
  const char *channel_argv3[] = {"channel", "test_cha32", "destroy",
                                 NULL
                                };
  const char channel_test_str3[] = "{\"ret\":\"OK\"}";
  const char *channel_argv4[] = {"channel", "test_cha32_2", "destroy",
                                 NULL
                                };
  const char channel_test_str4[] = "{\"ret\":\"OK\"}";

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(channel_argv1), channel_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str1);

  /* channel create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(channel_argv2), channel_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, controller_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, controller_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, controller_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv4), argv4, &tbl, controller_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv5), argv5, &tbl, controller_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 controller_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv6), argv6, &tbl, controller_cmd_update,
                 &ds, str, test_str6);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(channel_argv3), channel_argv3, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str3);

  /* channel destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(channel_argv4), channel_argv4, &tbl,
                 channel_cmd_update,
                 &ds, str, channel_test_str4);
}

void
test_destroy(void) {
  destroy = true;
}
