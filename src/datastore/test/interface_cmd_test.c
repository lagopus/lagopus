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
#include "../interface_cmd_internal.h"
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
  INTERP_CREATE(ret, "interface", interp, tbl, ds);
}

void
tearDown(void) {
  /* destroy interp. */
  INTERP_DESTROY("interface", interp, tbl, ds, destroy);
}

void
test_interface_cmd_parse_create_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"interface", "test_name01", "create",
                        "-type", "ethernet-rawsock",
                        "-device", "eth0",
                        NULL
                       };
  const char test_str[] = "{\"ret\":\"OK\"}";
  const char *argv1[] = {"interface", "test_name02", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth1",
                         "-mtu", "1",
                         "-ip-addr", "127.0.0.2",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name02", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"interface", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1,\n"
    "\"ip-addr\":\"127.0.0.2\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true},\n"
    "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name01\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"interface", "test_name02", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1,\n"
    "\"ip-addr\":\"127.0.0.2\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"interface", "test_name02", "disable",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *argv6[] = {"interface", "test_name02", "destroy",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"interface", "test_name01", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *port_argv1[] = {"port", "test_port01", "create",
                              "-interface", "test_name02",
                              NULL
                             };
  const char port_test_str1[] = "{\"ret\":\"OK\"}";
  const char *port_argv2[] = {"port", "test_port01", "destroy",
                              NULL
                             };
  const char port_test_str2[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv), argv, &tbl, interface_cmd_update,
                 &ds, str, test_str);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* port create cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv1), port_argv1, &tbl, port_cmd_update,
                 &ds, str, port_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* show all cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, interface_cmd_update,
                 &ds, str, test_str5);

  /* port destroy cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv2), port_argv2, &tbl, port_cmd_update,
                 &ds, str, port_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, interface_cmd_update,
                 &ds, str, test_str6);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, interface_cmd_update,
                 &ds, str, test_str7);
}

void
test_interface_cmd_parse_create_bad_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"interface", "test_name03", "create", "-hoge", NULL};
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad required options(-type), opt = -hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, interface_cmd_update, &ds, str, test_str);
}

void
test_interface_cmd_parse_create_bad_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"interface", "test_name04", "create",
                        "-type", "1", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = 1.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, interface_cmd_update, &ds, str, test_str);
}

void
test_interface_cmd_parse_create_bad_required_opts_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"interface", "test_name08", "create",
                        "-port-number", "1", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad required options(-type), opt = -port-number.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, interface_cmd_update, &ds, str, test_str);
}

void
test_interface_cmd_parse_invalid_args(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"interface", "test_name", "hoge_sub_cmd", NULL};
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"sub_cmd = hoge_sub_cmd.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, interface_cmd_update, &ds, str, test_str);
}

void
test_interface_cmd_parse_enable_unused(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name09", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name09", "enable",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name09. is not used.\"}";
  const char *argv3[] = {"interface", "test_name09", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name09\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"interface", "test_name09", "destroy",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);
}

void
test_interface_cmd_parse_config_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name10", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name10", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name10\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"interface", "test_name10", "config",
                         "-type", "ethernet-rawsock",
                         "-device", "eth1",
                         "-mtu", "1",
                         "-ip-addr", "127.0.0.2",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"interface", "test_name10", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name10\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1,\n"
    "\"ip-addr\":\"127.0.0.2\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"interface", "test_name10", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, interface_cmd_update,
                 &ds, str, test_str5);
}

void
test_interface_cmd_parse_config_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name11", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name11", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"interface", "test_name11", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv4[] = {"interface", "test_name11", "config",
                         "-type", "ethernet-rawsock",
                         "-device", "eth1",
                         "-mtu", "1",
                         "-ip-addr", "127.0.0.2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"interface", "test_name11", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1,\n"
    "\"ip-addr\":\"127.0.0.2\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv6[] = {"interface", "test_name11", "disable",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"interface", "test_name11", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *port_argv1[] = {"port", "test_port02", "create",
                              "-interface", "test_name11",
                              NULL
                             };
  const char port_test_str1[] = "{\"ret\":\"OK\"}";
  const char *port_argv2[] = {"port", "test_port02", "destroy",
                              NULL
                             };
  const char port_test_str2[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* port create cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv1), port_argv1, &tbl, port_cmd_update,
                 &ds, str, port_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, interface_cmd_update,
                 &ds, str, test_str5);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, interface_cmd_update,
                 &ds, str, test_str6);

  /* port destroy cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv2), port_argv2, &tbl, port_cmd_update,
                 &ds, str, port_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, interface_cmd_update,
                 &ds, str, test_str7);
}

void
test_interface_cmd_parse_config_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name12", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name12", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name12\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"interface", "test_name12", "config",
                         "-type", "ethernet-rawsock",
                         "-device", "eth1",
                         "-mtu", "1",
                         "-ip-addr", "127.0.0.2",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"interface", "test_name12", "config",
                         "-ip-addr", NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name12\",\n"
    "\"ip-addr\":\"127.0.0.2\"}]}";
  const char *argv5[] = {"interface", "test_name12", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* config(show) cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, interface_cmd_update,
                 &ds, str, test_str5);
}

void
test_interface_cmd_parse_destroy_used(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name13", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name13", "disable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"interface", "test_name13", "destroy",
                         NULL
                        };
  const char test_str3[] = {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                            "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name13: is used.\"}"
                           };
  const char *port_argv1[] = {"port", "test_port13", "create",
                              "-interface", "test_name13",
                              NULL
                             };
  const char port_test_str1[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* port create cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv1), port_argv1, &tbl, port_cmd_update,
                 &ds, str, port_test_str1);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                 interface_cmd_update, &ds, str, test_str3);
}

void
test_interface_cmd_parse_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name14", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name14", "current",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name14\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"interface", "test_name14", "modified",
                         NULL
                        };
  const char test_str3[] =  {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                             "\"data\":\"Not set modified.\"}"
                            };
  const char *argv4[] = {"interface", "test_name14", "config",
                         "-type", "ethernet-rawsock",
                         "-mtu", "105",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"interface", "test_name14", "modified",
                         NULL
                        };
  const char test_str5[] =  {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                             "\"data\":\"Not set modified.\"}"
                            };
  const char *argv6[] = {"interface", "test_name14", "config",
                         "-type", "ethernet-rawsock",
                         "-device", "eth1",
                         "-mtu", "1",
                         "-ip-addr", "127.0.0.2",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"interface", "test_name14", "modified",
                         NULL
                        };
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name14\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1,\n"
    "\"ip-addr\":\"127.0.0.2\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"interface", "test_name14", "destroy",
                         NULL
                        };
  const char test_str8[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv3), argv3, &tbl,
                 interface_cmd_update, &ds, str, test_str3);

  /* config cmd (AUTO COMMIT mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (modified : AUTO COMMIT mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv5), argv5, &tbl,
                 interface_cmd_update, &ds, str, test_str5);

  /* config cmd (COMMITING mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv6), argv6, &tbl, interface_cmd_update,
                 &ds, str, test_str6);

  /* show cmd (modified : COMMITING mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv7), argv7, &tbl,
                 interface_cmd_update, &ds, str, test_str7);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv8), argv8, &tbl,
                 interface_cmd_update, &ds, str, test_str8);
}

void
test_interface_cmd_parse_create_gre_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name15", "create",
                         "-type", "gre",
                         "-port-number", "100",
                         "-dst-addr", "127.0.0.1",
                         "-src-addr", "127.0.0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name15", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name15\",\n"
    "\"type\":\"gre\",\n"
    "\"port-number\":100,\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"src-addr\":\"127.0.0.0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"interface", "test_name15", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);
}

void
test_interface_cmd_parse_create_gre_bad_dst_addr(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name16", "create",
                         "-type", "gre",
                         "-port-number", "100",
                         "-dst-addr", "127.0.0.1.1",
                         NULL
                        };
  const char test_str1[] = {"{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
                            "\"data\":\"Can't add dst-addr.\"}"
                           };

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);
}

void
test_interface_cmd_parse_create_gre_over_port(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name17", "create",
                         "-type", "gre",
                         "-port-number", "4294967296",
                         NULL
                        };
  const char test_str1[] = {"{\"ret\":\"OUT_OF_RANGE\",\n"
                            "\"data\":\"Bad opt value = 4294967296.\"}"
                           };

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);
}

void
test_interface_cmd_parse_create_nvgre_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name18", "create",
                         "-type", "nvgre",
                         "-port-number", "100",
                         "-dst-addr", "127.0.0.1",
                         "-src-addr", "127.0.0.2",
                         "-mcast-group", "127.0.0.3",
                         "-ni", "200",
                         "-mtu", "1",
                         "-ip-addr", "127.0.0.2",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name18", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name18\",\n"
    "\"type\":\"nvgre\",\n"
    "\"port-number\":100,\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"src-addr\":\"127.0.0.2\",\n"
    "\"mcast-group\":\"127.0.0.3\",\n"
    "\"ni\":200,\n"
    "\"mtu\":1,\n"
    "\"ip-addr\":\"127.0.0.2\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"interface", "test_name18", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);
}

void
test_interface_cmd_parse_create_nvgre_bad_dst_addr(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name19", "create",
                         "-type", "nvgre",
                         "-port-number", "100",
                         "-dst-addr", "127.0.0.1.1",
                         NULL
                        };
  const char test_str1[] = {"{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
                            "\"data\":\"Can't add dst-addr.\"}"
                           };

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);
}

void
test_interface_cmd_parse_create_nvgre_over_port(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name20", "create",
                         "-type", "nvgre",
                         "-port-number", "4294967296",
                         NULL
                        };
  const char test_str1[] = {"{\"ret\":\"OUT_OF_RANGE\",\n"
                            "\"data\":\"Bad opt value = 4294967296.\"}"
                           };

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);
}

void
test_interface_cmd_parse_create_vxlan_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name21", "create",
                         "-type", "vxlan",
                         "-port-number", "100",
                         "-device", "eth0",
                         "-dst-addr", "127.0.0.1",
                         "-dst-port", "200",
                         "-src-addr", "127.0.0.2",
                         "-src-port", "300",
                         "-mcast-group", "127.0.0.3",
                         "-ni", "400",
                         "-ttl", "5",
                         "-mtu", "1",
                         "-ip-addr", "127.0.0.2",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name21", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name21\",\n"
    "\"type\":\"vxlan\",\n"
    "\"port-number\":100,\n"
    "\"device\":\"eth0\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":200,\n"
    "\"src-addr\":\"127.0.0.2\",\n"
    "\"src-port\":300,\n"
    "\"mcast-group\":\"127.0.0.3\",\n"
    "\"ni\":400,\n"
    "\"ttl\":5,\n"
    "\"mtu\":1,\n"
    "\"ip-addr\":\"127.0.0.2\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"interface", "test_name21", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);
}

void
test_interface_cmd_parse_create_vxlan_bad_dst_addr(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name22", "create",
                         "-type", "vxlan",
                         "-port-number", "100",
                         "-dst-addr", "127.0.0.1.1",
                         NULL
                        };
  const char test_str1[] = {"{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
                            "\"data\":\"Can't add dst-addr.\"}"
                           };

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);
}

void
test_interface_cmd_parse_create_vxlan_over_ttl(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name22", "create",
                         "-type", "vxlan",
                         "-port-number", "100",
                         "-ttl", "256",
                         NULL
                        };
  const char test_str1[] = {"{\"ret\":\"OUT_OF_RANGE\",\n"
                            "\"data\":\"Bad opt value = 256.\"}"
                           };

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);
}

void
test_interface_cmd_parse_create_vxlan_over_port(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name23", "create",
                         "-type", "vxlan",
                         "-port-number", "4294967296",
                         NULL
                        };
  const char test_str1[] = {"{\"ret\":\"OUT_OF_RANGE\",\n"
                            "\"data\":\"Bad opt value = 4294967296.\"}"
                           };

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);
}

void
test_interface_cmd_parse_create_vhost_user_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name24", "create",
                         "-type", "vhost-user",
                         "-port-number", "100",
                         "-dst-hw-addr", "12:34:56:78:90:AB",
                         "-src-hw-addr", "fe:Dc:ba:09:87:65",
                         "-mtu", "1",
                         "-ip-addr", "127.0.0.2",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name24", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name24\",\n"
    "\"type\":\"vhost-user\",\n"
    "\"port-number\":100,\n"
    "\"dst-hw-addr\":\"12:34:56:78:90:ab\",\n"
    "\"src-hw-addr\":\"fe:dc:ba:09:87:65\",\n"
    "\"mtu\":1,\n"
    "\"ip-addr\":\"127.0.0.2\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"interface", "test_name24", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);
}

void
test_interface_cmd_parse_create_vhost_user_bad_hw_addr(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name25", "create",
                         "-type", "vhost-user",
                         "-port-number", "100",
                         "-src-hw-addr", "fe:Dc:ba:09:87:",
                         NULL
                        };
  const char test_str1[] = {"{\"ret\":\"OUT_OF_RANGE\",\n"
                            "\"data\":\"Bad opt value = fe:Dc:ba:09:87:.\"}"
                           };

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);
}

void
test_interface_cmd_parse_create_vhost_user_over_port(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name26", "create",
                         "-type", "vhost-user",
                         "-port-number", "4294967296",
                         NULL
                        };
  const char test_str1[] = {"{\"ret\":\"OUT_OF_RANGE\",\n"
                            "\"data\":\"Bad opt value = 4294967296.\"}"
                           };

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);
}

void
test_interface_cmd_serialize_default_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* interface create cmd str. */
  const char *argv1[] = {"interface", "test_name27", "create", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"interface", "test_name27", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] =
      "interface "DATASTORE_NAMESPACE_DELIMITER"test_name27 create "
      "-type ethernet-rawsock "
      "-mtu 1500 "
      "-ip-addr 127.0.0.1\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name27", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 interface_cmd_update, &ds, str, test_str2);
}

void
test_interface_cmd_serialize_default_opt_escape(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* interface create cmd str. */
  const char *argv1[] = {"interface", "test_\"name28", "create", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"interface", "test_\"name28", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] =
      "interface \""DATASTORE_NAMESPACE_DELIMITER"test_\\\"name28\" create "
      "-type ethernet-rawsock "
      "-mtu 1500 "
      "-ip-addr 127.0.0.1\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_\"name28", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 interface_cmd_update, &ds, str, test_str2);
}

void
test_interface_cmd_serialize_default_opt_escape_white_space(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* interface create cmd str. */
  const char *argv1[] = {"interface", "test name29", "create", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"interface", "test name29", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] =
      "interface \""DATASTORE_NAMESPACE_DELIMITER"test name29\" create "
      "-type ethernet-rawsock "
      "-mtu 1500 "
      "-ip-addr 127.0.0.1\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test name29", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 interface_cmd_update, &ds, str, test_str2);
}

void
test_interface_cmd_serialize_type_ethernet(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* interface create cmd str. */
  const char *argv1[] = {"interface", "test_name30", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "test_device30",
                         "-mtu", "1",
                         "-ip-addr", "127.0.0.2",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"interface", "test_name30", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "interface "
                                DATASTORE_NAMESPACE_DELIMITER"test_name30 create "
                                "-type ethernet-rawsock "
                                "-device test_device30 "
                                "-mtu 1 "
                                "-ip-addr 127.0.0.2\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name30", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 interface_cmd_update, &ds, str, test_str2);
}

void
test_interface_cmd_serialize_type_ethernet_escape(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* interface create cmd str. */
  const char *argv1[] = {"interface", "test_name31", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "test_\"device31",
                         "-mtu", "1",
                         "-ip-addr", "127.0.0.2",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"interface", "test_name31", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "interface "
                                DATASTORE_NAMESPACE_DELIMITER"test_name31 create "
                                "-type ethernet-rawsock "
                                "-device \"test_\\\"device31\" "
                                "-mtu 1 "
                                "-ip-addr 127.0.0.2\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name31", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 interface_cmd_update, &ds, str, test_str2);
}

void
test_interface_cmd_serialize_type_ethernet_escape_white_space(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* interface create cmd str. */
  const char *argv1[] = {"interface", "test_name32", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "test device32",
                         "-mtu", "1",
                         "-ip-addr", "127.0.0.2",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"interface", "test_name32", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "interface "
                                DATASTORE_NAMESPACE_DELIMITER"test_name32 create "
                                "-type ethernet-rawsock "
                                "-device \"test device32\" "
                                "-mtu 1 "
                                "-ip-addr 127.0.0.2\n";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name32", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 interface_cmd_update, &ds, str, test_str2);
}

void
test_interface_cmd_parse_stats_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name33", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name33", "stats",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name33\",\n"
    "\"rx-packets\":0,\n"
    "\"rx-bytes\":0,\n"
    "\"rx-dropped\":0,\n"
    "\"rx-errors\":0,\n"
    "\"tx-packets\":0,\n"
    "\"tx-bytes\":0,\n"
    "\"tx-dropped\":0,\n"
    "\"tx-errors\":0}]}";
  const char *argv3[] = {"interface", "test_name33", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* stats cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);
}

void
test_interface_cmd_parse_stats_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name34", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name34", "stats",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name34\",\n"
    "\"rx-packets\":0,\n"
    "\"rx-bytes\":0,\n"
    "\"rx-dropped\":0,\n"
    "\"rx-errors\":0,\n"
    "\"tx-packets\":0,\n"
    "\"tx-bytes\":0,\n"
    "\"tx-dropped\":0,\n"
    "\"tx-errors\":0}]}";
  const char *argv3[] = {"interface", "test_name34", "stats",
                         "-clear", NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"interface", "test_name34", "stats",
                         NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name34\",\n"
    "\"rx-packets\":0,\n"
    "\"rx-bytes\":0,\n"
    "\"rx-dropped\":0,\n"
    "\"rx-errors\":0,\n"
    "\"tx-packets\":0,\n"
    "\"tx-bytes\":0,\n"
    "\"tx-dropped\":0,\n"
    "\"tx-errors\":0}]}";
  const char *argv5[] = {"interface", "test_name34", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* stats cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* stats(clear) cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* stats cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, interface_cmd_update,
                 &ds, str, test_str5);
}

void
test_interface_cmd_parse_stats_invalid_args(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name35", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name35", "stats",
                         "hoge",
                         NULL
                        };
  const char test_str2[] = {"{\"ret\":\"INVALID_ARGS\",\n"
                            "\"data\":\"Bad opt = hoge.\"}"
                           };
  const char *argv3[] = {"interface", "test_name35", "stats",
                         "-clear", "hoge",
                         NULL
                        };
  const char test_str3[] = {"{\"ret\":\"INVALID_ARGS\",\n"
                            "\"data\":\"Bad opt = hoge.\"}"
                           };
  const char *argv4[] = {"interface", "test_name35", "destroy",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* stats cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* stats(clear) cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);
}

void
test_interface_cmd_parse_atomic_commit(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"interface", "test_name36", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name36", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"interface", "test_name36", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name36\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"interface", "test_name36", "config",
                         "-type", "ethernet-rawsock",
                         "-device", "eth1",
                         "-mtu", "1",
                         "-ip-addr", "127.0.0.2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"interface", "test_name36", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"interface", "test_name36", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name36\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1,\n"
    "\"ip-addr\":\"127.0.0.2\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"interface", "test_name36", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name36\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1,\n"
    "\"ip-addr\":\"127.0.0.2\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"interface", "test_name36", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"interface", "test_name36", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, interface_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, interface_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name36", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name36", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, interface_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, interface_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, interface_cmd_update,
                 &ds, str, test_str9);
}

void
test_interface_cmd_parse_atomic_rollback(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ROLLBACKING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ROLLBACKED;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"interface", "test_name37", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name37", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"interface", "test_name37", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name37\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"interface", "test_name37", "config",
                         "-type", "ethernet-rawsock",
                         "-device", "eth1",
                         "-mtu", "1",
                         "-ip-addr", "127.0.0.2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"interface", "test_name37", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"interface", "test_name37", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name37\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1,\n"
    "\"ip-addr\":\"127.0.0.2\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"interface", "test_name37", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name37\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, interface_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, interface_cmd_update,
                 &ds, str, test_str6);

  /* rollbacking. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name37", conf, &ds);

  /* rollbacked. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name37", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, interface_cmd_update,
                 &ds, str, test_str7);
}

void
test_interface_cmd_parse_atomic_delay_enable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"interface", "test_name38", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name38", "enable", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"interface", "test_name38", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv4[] = {"interface", "test_name38", "modified", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name38\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"interface", "test_name38", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"interface", "test_name38", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name38\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv7[] = {"interface", "test_name38", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name38\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv8[] = {"interface", "test_name38", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"interface", "test_name38", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";
  const char *port_argv1[] = {"port", "test_port38", "create",
                              "-interface", "test_name38",
                              NULL
                             };
  const char port_test_str1[] = "{\"ret\":\"OK\"}";
  const char *port_argv2[] = {"port", "test_port38", "destroy",
                              NULL
                             };
  const char port_test_str2[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* port create cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(port_argv1), port_argv1, &tbl, port_cmd_update,
                 &ds, str, port_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name38", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, interface_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, interface_cmd_update,
                 &ds, str, test_str6);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name38", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, interface_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, interface_cmd_update,
                 &ds, str, test_str8);

  /* port destroy cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(port_argv2), port_argv2, &tbl, port_cmd_update,
                 &ds, str, port_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, interface_cmd_update,
                 &ds, str, test_str9);
}

void
test_interface_cmd_parse_atomic_delay_disable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"interface", "test_name39", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name39", "enable", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"interface", "test_name39", "disable", NULL};
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"interface", "test_name39", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name39\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"interface", "test_name39", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name39\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"interface", "test_name39", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name39\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"interface", "test_name39", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *port_argv1[] = {"port", "test_port39", "create",
                              "-interface", "test_name39",
                              NULL
                             };
  const char port_test_str1[] = "{\"ret\":\"OK\"}";
  const char *port_argv2[] = {"port", "test_port39", "destroy",
                              NULL
                             };
  const char port_test_str2[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* port create cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(port_argv1), port_argv1, &tbl, port_cmd_update,
                 &ds, str, port_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name39", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, interface_cmd_update,
                 &ds, str, test_str5);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name39", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, interface_cmd_update,
                 &ds, str, test_str6);

  /* port destroy cmd.*/
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(port_argv2), port_argv2, &tbl, port_cmd_update,
                 &ds, str, port_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv7), argv7, &tbl, interface_cmd_update,
                 &ds, str, test_str7);
}

void
test_interface_cmd_parse_atomic_delay_destroy(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"interface", "test_name40", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name40", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"interface", "test_name40", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name40\"}";
  const char *argv4[] = {"interface", "test_name40", "enable",
                         NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name40\"}";
  const char *argv5[] = {"interface", "test_name40", "disable",
                         NULL
                        };
  const char test_str5[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name40\"}";
  const char *argv6[] = {"interface", "test_name40", "destroy",
                         NULL
                        };
  const char test_str6[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name40\"}";
  const char *argv7[] = {"interface", "test_name40", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name40\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, interface_cmd_update,
                 &ds, str, test_str5);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, interface_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name40", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name40", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, interface_cmd_update,
                 &ds, str, test_str7);
}

void
test_interface_cmd_parse_atomic_abort_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"interface", "test_name41", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name41", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"interface", "test_name41", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name41\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"interface", "test_name41", "config",
                         "-type", "ethernet-rawsock",
                         "-device", "eth1",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"interface", "test_name41", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"interface", "test_name41", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name41\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"interface", "test_name41", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv8[] = {"interface", "test_name41", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name41\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"interface", "test_name41", NULL};
  const char test_str9[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name41\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, interface_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, interface_cmd_update,
                 &ds, str, test_str6);

  /* aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name41", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, interface_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, interface_cmd_update,
                 &ds, str, test_str8);

  /* aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name41", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv9), argv9, &tbl, interface_cmd_update,
                 &ds, str, test_str9);
}

void
test_interface_cmd_parse_atomic_abort_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"interface", "test_name42", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name42", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"interface", "test_name42", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv4[] = {"interface", "test_name42", "config",
                         "-type", "ethernet-rawsock",
                         "-device", "eth1",
                         "-mtu", "1",
                         "-ip-addr", "127.0.0.2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"interface", "test_name42", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"interface", "test_name42", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1,\n"
    "\"ip-addr\":\"127.0.0.2\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"interface", "test_name42", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"interface", "test_name42", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1,\n"
    "\"ip-addr\":\"127.0.0.2\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"interface", "test_name42", NULL};
  const char test_str9[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name42\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv10[] = {"interface", "test_name42", "modified", NULL};
  const char test_str10[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv11[] = {"interface", "test_name42", "destroy",
                          NULL
                         };
  const char test_str11[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, interface_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, interface_cmd_update,
                 &ds, str, test_str6);

  /*  aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name42", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, interface_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, interface_cmd_update,
                 &ds, str, test_str8);

  /*  aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name42", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv9), argv9, &tbl, interface_cmd_update,
                 &ds, str, test_str9);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv10), argv10, &tbl, interface_cmd_update,
                 &ds, str, test_str10);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv11), argv11, &tbl, interface_cmd_update,
                 &ds, str, test_str11);

}

void
test_interface_cmd_parse_atomic_destroy_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"interface", "test_name43", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name43", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"interface", "test_name43", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name43\"}";
  const char *argv4[] = {"interface", "test_name43", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth1",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"interface", "test_name43", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name43\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"interface", "test_name43", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name43\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"interface", "test_name43", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name43\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"interface", "test_name43", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"interface", "test_name43", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, interface_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, interface_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name43", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, interface_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name43", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv7), argv7, &tbl, interface_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv8), argv8, &tbl, interface_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, interface_cmd_update,
                 &ds, str, test_str9);
}

void
test_interface_cmd_parse_create_bad_ip_addr(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name44", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         "-ip-addr", "127.0.0.1.1",
                         NULL
                        };
  const char test_str1[] = {"{\"ret\":\"ADDR_RESOLVER_FAILURE\",\n"
                            "\"data\":\"Can't add ip-addr.\"}"
                           };

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);
}

void
test_interface_cmd_parse_create_over_mtu(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name45", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         "-mtu", "65536",
                         NULL
                        };
  const char test_str1[] = {"{\"ret\":\"OUT_OF_RANGE\",\n"
                            "\"data\":\"Bad opt value = 65536.\"}"
                           };

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, interface_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);
}

void
test_interface_cmd_parse_dryrun(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_DRYRUN;
  char *str = NULL;
  const char *argv1[] = {"interface", "test_name46", "create",
                         "-type", "ethernet-rawsock",
                         "-device", "eth0",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"interface", "test_name46", "current", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name46\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"interface", "test_name46", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv4[] = {"interface", "test_name46", "config",
                         "-type", "ethernet-rawsock",
                         "-device", "eth1",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"interface", "test_name46", "current", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name46\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"interface", "test_name46", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, interface_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, interface_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, interface_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv4), argv4, &tbl, interface_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv5), argv5, &tbl, interface_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 interface_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv6), argv6, &tbl, interface_cmd_update,
                 &ds, str, test_str6);
}

void
test_destroy(void) {
  destroy = true;
}
