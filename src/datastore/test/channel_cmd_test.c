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
#include "../channel_cmd_internal.h"
#include "../controller_cmd_internal.h"
#include "../datastore_internal.h"

static lagopus_dstring_t ds = NULL;
static lagopus_hashmap_t tbl = NULL;
static datastore_interp_t interp = NULL;
static bool destroy = false;

void
setUp(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create interp. */
  INTERP_CREATE(ret, "channel", interp, tbl, ds);
}

void
tearDown(void) {
  /* destroy interp. */
  INTERP_DESTROY("channel", interp, tbl, ds, destroy);
}

void
test_channel_cmd_parse_create_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"channel", "test_name01", "create",
                        "-dst-addr", "127.0.0.1",
                        "-dst-port", "65535",
                        "-local-addr", "127.0.0.1",
                        "-local-port", "65535",
                        "-protocol", "tcp",
                        NULL
                       };
  const char test_str[] = "{\"ret\":\"OK\"}";
  const char *argv1[] = {"channel", "test_name02", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name02", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"channel", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true},\n"
    "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name01\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"channel", "test_name02", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"channel", "test_name02", "disable",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *argv6[] = {"channel", "test_name02", "destroy",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"channel", "test_name01", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *ctrler_argv1[] = {"controller", "c_name01", "create",
                                "-channel", "test_name02",
                                "-role", "equal",
                                "-connection-type", "main",
                                NULL
                               };
  const char ctrler_test_str1[] = "{\"ret\":\"OK\"}";
  const char *ctrler_argv2[] = {"controller", "c_name01", "destroy",
                                NULL
                               };
  const char ctrler_test_str2[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv), argv, &tbl, channel_cmd_update,
                 &ds, str, test_str);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* controller create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(ctrler_argv1), ctrler_argv1, &tbl,
                 controller_cmd_update, &ds, str, ctrler_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* show all cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, channel_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, channel_cmd_update,
                 &ds, str, test_str4);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, channel_cmd_update,
                 &ds, str, test_str5);

  /* controller destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(ctrler_argv2), ctrler_argv2, &tbl,
                 controller_cmd_update, &ds, str, ctrler_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, channel_cmd_update,
                 &ds, str, test_str6);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, channel_cmd_update,
                 &ds, str, test_str7);
}

void
test_channel_cmd_parse_create_bad_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"channel", "test_name03", "create", "-hoge", NULL};
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"opt = -hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, channel_cmd_update, &ds, str, test_str);
}

void
test_channel_cmd_parse_create_over_dst_port(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"channel", "test_name04", "create",
                         "-dst-port", "65536", NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"TOO_LONG\",\n"
    "\"data\":\"Can't add dst-port.\"}";
  const char *argv2[] = {"channel", "test_name04", "create",
                         "-dst-port", "-1", NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = -1.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, channel_cmd_update, &ds, str, test_str1);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, channel_cmd_update, &ds, str, test_str2);
}

void
test_channel_cmd_parse_create_over_local_port(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"channel", "test_name05", "create",
                         "-local-port", "65536", NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"TOO_LONG\",\n"
    "\"data\":\"Can't add local-port.\"}";
  const char *argv2[] = {"channel", "test_name05", "create",
                         "-local-port", "-1", NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = -1.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, channel_cmd_update, &ds, str, test_str1);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, channel_cmd_update, &ds, str, test_str2);
}

void
test_channel_cmd_parse_create_not_dst_port_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"channel", "test_name06", "create",
                        "-dst-port", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, channel_cmd_update, &ds, str, test_str);
}

void
test_channel_cmd_parse_create_not_local_port_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"channel", "test_name07", "create",
                        "-local-port", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, channel_cmd_update, &ds, str, test_str);
}

void
test_channel_cmd_parse_create_not_dst_addr_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"channel", "test_name08", "create",
                        "-dst-addr", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, channel_cmd_update, &ds, str, test_str);
}

void
test_channel_cmd_parse_create_not_local_addr_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"channel", "test_name09", "create",
                        "-local-addr", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, channel_cmd_update, &ds, str, test_str);
}

void
test_channel_cmd_parse_create_bad_protocol(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"channel", "test_name10", "create",
                        "-protocol", "hoge", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, channel_cmd_update, &ds, str, test_str);
}

void
test_channel_cmd_parse_create_no_protocol_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"channel", "test_name11", "create",
                        "-protocol", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, channel_cmd_update, &ds, str, test_str);
}

void
test_channel_cmd_parse_enable_unused(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"channel", "test_name12", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name12", "enable",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name12. is not used.\"}";
  const char *argv3[] = {"channel", "test_name12", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name12\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"channel", "test_name12", "destroy",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, channel_cmd_update,
                 &ds, str, test_str3);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, channel_cmd_update,
                 &ds, str, test_str4);
}

void
test_channel_cmd_parse_config_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"channel", "test_name13", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name13", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name13\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"channel", "test_name13", "config",
                         "-dst-addr", "127.0.0.2",
                         "-dst-port", "3000",
                         "-local-addr", "127.0.0.2",
                         "-local-port", "3000",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"channel", "test_name13", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name13\",\n"
    "\"dst-addr\":\"127.0.0.2\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.2\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"channel", "test_name13", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, channel_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, channel_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, channel_cmd_update,
                 &ds, str, test_str5);
}

void
test_channel_cmd_parse_config_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"channel", "test_name14", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name14", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"channel", "test_name14", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name14\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv4[] = {"channel", "test_name14", "config",
                         "-dst-addr", "127.0.0.2",
                         "-dst-port", "3000",
                         "-local-addr", "127.0.0.2",
                         "-local-port", "3000",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"channel", "test_name14", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name14\",\n"
    "\"dst-addr\":\"127.0.0.2\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.2\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv6[] = {"channel", "test_name14", "disable",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"channel", "test_name14", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *ctrler_argv1[] = {"controller", "c_name02", "create",
                                "-channel", "test_name14",
                                "-role", "equal",
                                "-connection-type", "main",
                                NULL
                               };
  const char ctrler_test_str1[] = "{\"ret\":\"OK\"}";
  const char *ctrler_argv2[] = {"controller", "c_name02", "destroy",
                                NULL
                               };
  const char ctrler_test_str2[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* controller create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(ctrler_argv1), ctrler_argv1, &tbl,
                 controller_cmd_update, &ds, str, ctrler_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, channel_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, channel_cmd_update,
                 &ds, str, test_str4);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, channel_cmd_update,
                 &ds, str, test_str5);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, channel_cmd_update,
                 &ds, str, test_str6);

  /* controller destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(ctrler_argv2), ctrler_argv2, &tbl,
                 controller_cmd_update, &ds, str, ctrler_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, channel_cmd_update,
                 &ds, str, test_str7);
}

void
test_channel_cmd_parse_config_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"channel", "test_name15", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name15", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name15\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"channel", "test_name15", "config",
                         "-dst-port", "3001",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"channel", "test_name15", "config",
                         "-dst-port", NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name15\",\n"
    "\"dst-port\":3001}]}";
  const char *argv5[] = {"channel", "test_name15", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, channel_cmd_update,
                 &ds, str, test_str3);

  /* config cmd (show). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, channel_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, channel_cmd_update,
                 &ds, str, test_str5);
}

void
test_channel_cmd_parse_destroy_used(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"channel", "test_name102", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name102", "disable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"channel", "test_name102", "destroy",
                         NULL
                        };
  const char test_str3[] = {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                            "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name102: is used.\"}"
                           };
  const char *ctrler_argv1[] = {"controller", "c_name101", "create",
                                "-channel", "test_name102",
                                "-role", "equal",
                                "-connection-type", "main",
                                NULL
                               };
  const char ctrler_test_str1[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* controller create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(ctrler_argv1), ctrler_argv1, &tbl,
                 controller_cmd_update, &ds, str, ctrler_test_str1);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                 channel_cmd_update, &ds, str, test_str3);
}

void
test_channel_cmd_parse_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  char *str = NULL;
  const char *argv1[] = {"channel", "test_name16", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name16", "current",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name16\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"channel", "test_name16", "modified",
                         NULL
                        };
  const char test_str3[] =  {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                             "\"data\":\"Not set modified.\"}"
                            };
  const char *argv4[] = {"channel", "test_name16", "config",
                         "-dst-port", "3001",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"channel", "test_name16", "modified",
                         NULL
                        };
  const char test_str5[] =  {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                             "\"data\":\"Not set modified.\"}"
                            };
  const char *argv6[] = {"channel", "test_name16", "config",
                         "-dst-port", "3002",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"channel", "test_name16", "modified",
                         NULL
                        };
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name16\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":3002,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"channel", "test_name16", "destroy",
                         NULL
                        };
  const char test_str8[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, channel_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv3), argv3, &tbl,
                 channel_cmd_update, &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, channel_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (modified : AUTO COMMIT mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, channel_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv5), argv5, &tbl,
                 channel_cmd_update, &ds, str, test_str5);

  /* config cmd (COMMITING mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv6), argv6, &tbl, channel_cmd_update,
                 &ds, str, test_str6);

  /* show cmd (modified : COMMITING mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, channel_cmd_update,
                 &ds, str, test_str7);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, channel_cmd_update,
                 &ds, str, test_str8);
}

void
test_channel_cmd_serialize_default_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* channel create cmd str. */
  const char *argv1[] = {"channel", "test_name17", "create", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* channel destroy cmd str. */
  const char *argv2[] = {"channel", "test_name17", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "channel "
                                DATASTORE_NAMESPACE_DELIMITER"test_name17 create "
                                "-dst-addr 127.0.0.1 "
                                "-dst-port 6633 "
                                "-local-addr 0.0.0.0 "
                                "-local-port 0 "
                                "-protocol tcp\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name17", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 channel_cmd_update, &ds, str, test_str2);
}

void
test_channel_cmd_serialize_default_opt_escape(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* channel create cmd str. */
  const char *argv1[] = {"channel", "test_\"name18", "create", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* channel destroy cmd str. */
  const char *argv2[] = {"channel", "test_\"name18", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "channel "
                                "\""DATASTORE_NAMESPACE_DELIMITER"test_\\\"name18\" create "
                                "-dst-addr 127.0.0.1 "
                                "-dst-port 6633 "
                                "-local-addr 0.0.0.0 "
                                "-local-port 0 "
                                "-protocol tcp\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_\"name18", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 channel_cmd_update, &ds, str, test_str2);
}

void
test_channel_cmd_serialize_default_opt_escape_white_space(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* channel create cmd str. */
  const char *argv1[] = {"channel", "test name19", "create", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* channel destroy cmd str. */
  const char *argv2[] = {"channel", "test name19", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "channel "
                                "\""DATASTORE_NAMESPACE_DELIMITER"test name19\" create "
                                "-dst-addr 127.0.0.1 "
                                "-dst-port 6633 "
                                "-local-addr 0.0.0.0 "
                                "-local-port 0 "
                                "-protocol tcp\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test name19", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 channel_cmd_update, &ds, str, test_str2);
}

void
test_channel_cmd_serialize_all_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* channel create cmd str. */
  const char *argv1[] = {"channel", "test_name20", "create",
                         "-dst-addr", "10.0.0.1",
                         "-dst-port", "11111",
                         "-local-addr", "10.0.0.2",
                         "-local-port", "22222",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* channel destroy cmd str. */
  const char *argv2[] = {"channel", "test_name20", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "channel "
                                DATASTORE_NAMESPACE_DELIMITER"test_name20 create "
                                "-dst-addr 10.0.0.1 "
                                "-dst-port 11111 "
                                "-local-addr 10.0.0.2 "
                                "-local-port 22222 "
                                "-protocol tcp\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name20", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 channel_cmd_update, &ds, str, test_str2);
}

void
test_channel_cmd_parse_atomic_commit(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"channel", "test_name21", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name21", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"channel", "test_name21", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name21\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"channel", "test_name21", "config",
                         "-dst-addr", "127.0.0.2",
                         "-dst-port", "3000",
                         "-local-addr", "127.0.0.2",
                         "-local-port", "3000",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"channel", "test_name21", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"channel", "test_name21", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name21\",\n"
    "\"dst-addr\":\"127.0.0.2\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.2\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"channel", "test_name21", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name21\",\n"
    "\"dst-addr\":\"127.0.0.2\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.2\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"channel", "test_name21", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"channel", "test_name21", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, channel_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, channel_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, channel_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, channel_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name21", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name21", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, channel_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, channel_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, channel_cmd_update,
                 &ds, str, test_str9);
}

void
test_channel_cmd_parse_atomic_rollback(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ROLLBACKING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ROLLBACKED;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"channel", "test_name22", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name22", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"channel", "test_name22", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name22\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"channel", "test_name22", "config",
                         "-dst-addr", "127.0.0.2",
                         "-dst-port", "3000",
                         "-local-addr", "127.0.0.2",
                         "-local-port", "3000",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"channel", "test_name22", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"channel", "test_name22", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name22\",\n"
    "\"dst-addr\":\"127.0.0.2\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.2\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"channel", "test_name22", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = :test_name22\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, channel_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, channel_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, channel_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, channel_cmd_update,
                 &ds, str, test_str6);

  /* rollbacking. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name22", conf, &ds);

  /* rollbacked. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name22", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, channel_cmd_update,
                 &ds, str, test_str7);
}

void
test_channel_cmd_parse_atomic_delay_enable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"channel", "test_name23", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name23", "enable", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"channel", "test_name23", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv4[] = {"channel", "test_name23", "modified", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name23\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"channel", "test_name23", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"channel", "test_name23", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name23\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv7[] = {"channel", "test_name23", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name23\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv8[] = {"channel", "test_name23", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"channel", "test_name23", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";
  const char *ctrler_argv1[] = {"controller", "c_name23", "create",
                                "-channel", "test_name23",
                                "-role", "equal",
                                "-connection-type", "main",
                                NULL
                               };
  const char ctrler_test_str1[] = "{\"ret\":\"OK\"}";
  const char *ctrler_argv2[] = {"controller", "c_name23", "destroy",
                                NULL
                               };
  const char ctrler_test_str2[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* controller create cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state1,
                 ARGV_SIZE(ctrler_argv1), ctrler_argv1, &tbl, controller_cmd_update,
                 &ds, str, ctrler_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, channel_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, channel_cmd_update,
                 &ds, str, test_str4);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name23", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, channel_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, channel_cmd_update,
                 &ds, str, test_str6);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name23", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, channel_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, channel_cmd_update,
                 &ds, str, test_str8);

  /* controller destroy cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state4,
                 ARGV_SIZE(ctrler_argv2), ctrler_argv2, &tbl, controller_cmd_update,
                 &ds, str, ctrler_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, channel_cmd_update,
                 &ds, str, test_str9);
}

void
test_channel_cmd_parse_atomic_delay_disable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"channel", "test_name24", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name24", "enable", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"channel", "test_name24", "disable", NULL};
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"channel", "test_name24", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name24\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"channel", "test_name24", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name24\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"channel", "test_name24", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name24\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"channel", "test_name24", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *ctrler_argv1[] = {"controller", "c_name24", "create",
                                "-channel", "test_name24",
                                "-role", "equal",
                                "-connection-type", "main",
                                NULL
                               };
  const char ctrler_test_str1[] = "{\"ret\":\"OK\"}";
  const char *ctrler_argv2[] = {"controller", "c_name24", "destroy",
                                NULL
                               };
  const char ctrler_test_str2[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* controller create cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state4,
                 ARGV_SIZE(ctrler_argv1), ctrler_argv1, &tbl, controller_cmd_update,
                 &ds, str, ctrler_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, channel_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, channel_cmd_update,
                 &ds, str, test_str4);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name24", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, channel_cmd_update,
                 &ds, str, test_str5);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name24", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, channel_cmd_update,
                 &ds, str, test_str6);

  /* controller destroy cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state4,
                 ARGV_SIZE(ctrler_argv2), ctrler_argv2, &tbl, controller_cmd_update,
                 &ds, str, ctrler_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv7), argv7, &tbl, channel_cmd_update,
                 &ds, str, test_str7);
}

void
test_channel_cmd_parse_atomic_delay_destroy(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"channel", "test_name25", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name25", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"channel", "test_name25", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name25\"}";
  const char *argv4[] = {"channel", "test_name25", "enable", NULL};
  const char test_str4[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name25\"}";
  const char *argv5[] = {"channel", "test_name25", "disable", NULL};
  const char test_str5[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name25\"}";
  const char *argv6[] = {"channel", "test_name25", "destroy", NULL};
  const char test_str6[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name25\"}";
  const char *argv7[] = {"channel", "test_name25", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name25\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, channel_cmd_update,
                 &ds, str, test_str3);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, channel_cmd_update,
                 &ds, str, test_str4);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, channel_cmd_update,
                 &ds, str, test_str5);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, channel_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name25", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name25", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, channel_cmd_update,
                 &ds, str, test_str7);
}

void
test_channel_cmd_parse_atomic_abort_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"channel", "test_name26", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name26", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"channel", "test_name26", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name26\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"channel", "test_name26", "config",
                         "-dst-addr", "127.0.0.2",
                         "-dst-port", "3000",
                         "-local-addr", "127.0.0.2",
                         "-local-port", "3000",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"channel", "test_name26", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"channel", "test_name26", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name26\",\n"
    "\"dst-addr\":\"127.0.0.2\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.2\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"channel", "test_name26", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv8[] = {"channel", "test_name26", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name26\",\n"
    "\"dst-addr\":\"127.0.0.2\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.2\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"channel", "test_name26", NULL};
  const char test_str9[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name26\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, channel_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, channel_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, channel_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, channel_cmd_update,
                 &ds, str, test_str6);

  /* aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name26", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, channel_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, channel_cmd_update,
                 &ds, str, test_str8);

  /* aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name26", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv9), argv9, &tbl, channel_cmd_update,
                 &ds, str, test_str9);
}

void
test_channel_cmd_parse_atomic_abort_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"channel", "test_name27", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name27", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name27\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"channel", "test_name27", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv4[] = {"channel", "test_name27", "config",
                         "-dst-addr", "127.0.0.2",
                         "-dst-port", "3000",
                         "-local-addr", "127.0.0.2",
                         "-local-port", "3000",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"channel", "test_name27", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name27\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"channel", "test_name27", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name27\",\n"
    "\"dst-addr\":\"127.0.0.2\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.2\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"channel", "test_name27", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name27\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"channel", "test_name27", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name27\",\n"
    "\"dst-addr\":\"127.0.0.2\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.2\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"channel", "test_name27", NULL};
  const char test_str9[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name27\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv10[] = {"channel", "test_name27", "modified", NULL};
  const char test_str10[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv11[] = {"channel", "test_name27", "destroy",
                          NULL
                         };
  const char test_str11[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, channel_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, channel_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, channel_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, channel_cmd_update,
                 &ds, str, test_str6);

  /* aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name27", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, channel_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, channel_cmd_update,
                 &ds, str, test_str8);

  /* aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name27", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv9), argv9, &tbl, channel_cmd_update,
                 &ds, str, test_str9);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv10), argv10, &tbl, channel_cmd_update,
                 &ds, str, test_str10);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv11), argv11, &tbl, channel_cmd_update,
                 &ds, str, test_str11);
}

void
test_channel_cmd_parse_atomic_destroy_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"channel", "test_name28", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name28", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"channel", "test_name28", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name28\"}";
  const char *argv4[] = {"channel", "test_name28", "create",
                         "-dst-addr", "127.0.0.2",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.2",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"channel", "test_name28", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name28\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"channel", "test_name28", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name28\",\n"
    "\"dst-addr\":\"127.0.0.2\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.2\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"channel", "test_name28", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name28\",\n"
    "\"dst-addr\":\"127.0.0.2\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.2\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"channel", "test_name28", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"channel", "test_name28", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, channel_cmd_update,
                 &ds, str, test_str3);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK,channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, channel_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, channel_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, channel_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name28", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, channel_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name28", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv7), argv7, &tbl, channel_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv8), argv8, &tbl, channel_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, channel_cmd_update,
                 &ds, str, test_str9);
}

void
test_channel_cmd_parse_dryrun(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_DRYRUN;
  char *str = NULL;
  const char *argv1[] = {"channel", "test_name29", "create",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "65535",
                         "-local-addr", "127.0.0.1",
                         "-local-port", "65535",
                         "-protocol", "tcp",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"channel", "test_name29", "current", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name29\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":65535,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":65535,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"channel", "test_name29", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv4[] = {"channel", "test_name29", "config",
                         "-dst-addr", "127.0.0.2",
                         "-dst-port", "3000",
                         "-local-addr", "127.0.0.2",
                         "-local-port", "3000",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"channel", "test_name29", "current", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name29\",\n"
    "\"dst-addr\":\"127.0.0.2\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.2\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"channel", "test_name29", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, channel_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, channel_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, channel_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv4), argv4, &tbl, channel_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv5), argv5, &tbl, channel_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 channel_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv6), argv6, &tbl, channel_cmd_update,
                 &ds, str, test_str6);
}


void
test_destroy(void) {
  destroy = true;
}
