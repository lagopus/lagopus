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
#include "../port_cmd_internal.h"
#include "../interface_cmd_internal.h"
#include "../queue_cmd_internal.h"
#include "../policer_cmd_internal.h"
#include "../policer_action_cmd_internal.h"
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
  INTERP_CREATE(ret, "port", interp, tbl, ds);
}

void
tearDown(void) {
  /* destroy interp. */
  INTERP_DESTROY("port", interp, tbl, ds, destroy);
}

void
test_port_cmd_parse_create_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"port", "test_name01", "create",
                        "-interface", "test_eth0",
                        "-policer", "p1_1",
                        NULL
                       };
  const char test_str[] = "{\"ret\":\"OK\"}";
  const char *argv1[] = {"port", "test_name02", "create",
                         "-interface", "test_eth\"1",
                         "-policer", "p1_2",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name02", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"port", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"port-number\":65535,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth\\\"1\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p1_2\",\n"
    "\"queues\":{},\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true},\n"
    "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name01\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth0\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p1_1\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"port", "test_name02", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"port-number\":65535,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth\\\"1\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p1_2\",\n"
    "\"queues\":{},\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"port", "test_name02", "disable",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *argv6[] = {"port", "test_name02", "destroy",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"port", "test_name01", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *inter_argv[] = {"interface", "test_eth0", "create",
                              "-type", "ethernet-rawsock",
                              "-device", "eth0",
                              NULL
                             };
  const char inter_test_str[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth\"1", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth\"1", NULL};
  const char inter_test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth\\\"1\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *inter_argv3[] = {"interface", "test_eth\"1", NULL};
  const char inter_test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth\\\"1\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *inter_argv4[] = {"interface", "test_eth\"1", "destroy",
                               NULL
                              };
  const char inter_test_str4[] = "{\"ret\":\"OK\"}";
  const char *inter_argv5[] = {"interface", "test_eth0", "destroy",
                               NULL
                              };
  const char inter_test_str5[] = "{\"ret\":\"OK\"}";
  const char *policer_argv1[] = {"policer", "p1_2", NULL};
  const char policer_test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"p1_2\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"pa1_2\"],\n"
    "\"bandwidth-limit\":1500,\n"
    "\"burst-size-limit\":1500,\n"
    "\"bandwidth-percent\":0,\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *policer_argv2[] = {"policer", "p1_2", NULL};
  const char policer_test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"p1_2\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"pa1_2\"],\n"
    "\"bandwidth-limit\":1500,\n"
    "\"burst-size-limit\":1500,\n"
    "\"bandwidth-percent\":0,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *bridge_argv1[] = {"bridge", "test_bridge01", "create",
                                "-port", "test_name02", "65535",
                                NULL
                               };
  const char bridge_test_str1[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv2[] = {"bridge", "test_bridge01", "destroy",
                                NULL
                               };
  const char bridge_test_str2[] = "{\"ret\":\"OK\"}";

  TEST_POLICER_CREATE(ret, &interp, state, &tbl, &ds, str, "pa1_1", "p1_1");
  TEST_POLICER_CREATE(ret, &interp, state, &tbl, &ds, str, "pa1_2", "p1_2");

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv), inter_argv, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str);

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv), argv, &tbl, port_cmd_update,
                 &ds, str, test_str);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv1), bridge_argv1, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* show all cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* interface show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);


  /* policer show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(policer_argv1), policer_argv1, &tbl, policer_cmd_update,
                 &ds, str, policer_test_str1);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, port_cmd_update,
                 &ds, str, test_str5);

  /* bridge destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv2), bridge_argv2, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, port_cmd_update,
                 &ds, str, test_str6);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, port_cmd_update,
                 &ds, str, test_str7);

  /* interface show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv3), inter_argv3, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str3);

  /* policer show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(policer_argv2), policer_argv2, &tbl, policer_cmd_update,
                 &ds, str, policer_test_str2);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv4), inter_argv4, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str4);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv5), inter_argv5, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str5);

  TEST_POLICER_DESTROY(ret, &interp, state, &tbl, &ds, str, "pa1_1", "p1_1");
  TEST_POLICER_DESTROY(ret, &interp, state, &tbl, &ds, str, "pa1_2", "p1_2");
}

void
test_port_cmd_parse_create_bad_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"port", "test_name03", "create", "-hoge", NULL};
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"opt = -hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, port_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, port_cmd_update, &ds, str, test_str);
}

void
test_port_cmd_parse_create_not_interface_name_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"port", "test_name07", "create",
                        "-interface", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, port_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, port_cmd_update, &ds, str, test_str);
}

void
test_port_cmd_parse_enable_unused(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"port", "test_name08", "create",
                         "-interface", "test_eth2",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name08", "enable",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name08. is not used.\"}";
  const char *argv3[] = {"port", "test_name08", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name08\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth2\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"port", "test_name08", "destroy",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth2", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth2", "destroy",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);
}

void
test_port_cmd_parse_create_not_exists(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"port", "test_name09", "create",
                         "-interface", "test_eth100",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"interface name = "DATASTORE_NAMESPACE_DELIMITER"test_eth100.\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, port_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, port_cmd_update, &ds, str, test_str1);
}

void
test_port_cmd_parse_config_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"port", "test_name10", "create",
                         "-interface", "test_eth3",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name10", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name10\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth3\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"port", "test_name10", "config",
                         "-interface", "test_eth4",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"port", "test_name10", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name10\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth4\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"port", "test_name10", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth3", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth3", NULL};
  const char inter_test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth3\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *inter_argv3[] = {"interface", "test_eth3", NULL};
  const char inter_test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth3\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth0\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *inter_argv4[] = {"interface", "test_eth4", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth1",
                               NULL
                              };
  const char inter_test_str4[] = "{\"ret\":\"OK\"}";
  const char *inter_argv5[] = {"interface", "test_eth4", NULL};
  const char inter_test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth4\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *inter_argv6[] = {"interface", "test_eth4", NULL};
  const char inter_test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth4\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"eth1\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *inter_argv7[] = {"interface", "test_eth3", "destroy",
                               NULL
                              };
  const char inter_test_str7[] = "{\"ret\":\"OK\"}";
  const char *inter_argv8[] = {"interface", "test_eth4", "destroy",
                               NULL
                              };
  const char inter_test_str8[] = "{\"ret\":\"OK\"}";

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv4), inter_argv4, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str4);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* interface show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);

  /* interface show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv5), inter_argv5, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str5);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* interface show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv3), inter_argv3, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str3);

  /* interface show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv6), inter_argv6, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str6);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, port_cmd_update,
                 &ds, str, test_str5);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv7), inter_argv7, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str7);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv8), inter_argv8, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str8);
}

void
test_port_cmd_parse_config_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"port", "test_name11", "create",
                         "-interface", "test_eth5",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name11", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"port", "test_name11", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"port-number\":65535,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth5\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":{},\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv4[] = {"port", "test_name11", "config",
                         "-interface", "test_eth6",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"port", "test_name11", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name11\",\n"
    "\"port-number\":65535,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth6\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":{},\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv6[] = {"port", "test_name11", "disable",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"port", "test_name11", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth5", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth6", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth1",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";
  const char *inter_argv3[] = {"interface", "test_eth5", "destroy",
                               NULL
                              };
  const char inter_test_str3[] = "{\"ret\":\"OK\"}";
  const char *inter_argv4[] = {"interface", "test_eth6", "destroy",
                               NULL
                              };
  const char inter_test_str4[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv1[] = {"bridge", "test_bridge05", "create",
                                "-port", "test_name11", "65535",
                                NULL
                               };
  const char bridge_test_str1[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv2[] = {"bridge", "test_bridge05", "destroy",
                                NULL
                               };
  const char bridge_test_str2[] = "{\"ret\":\"OK\"}";

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv1), bridge_argv1, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, port_cmd_update,
                 &ds, str, test_str5);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, port_cmd_update,
                 &ds, str, test_str6);

  /* bridge destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv2), bridge_argv2, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, port_cmd_update,
                 &ds, str, test_str7);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv3), inter_argv3, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str3);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv4), inter_argv4, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str4);
}

void
test_port_cmd_parse_config_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"port", "test_name12", "create",
                         "-interface", "test_eth7",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name12", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name12\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth7\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"port", "test_name12", "config",
                         "-interface", "test_eth8",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"port", "test_name12", "config",
                         "-interface", NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name12\",\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth8\"}]}";
  const char *argv5[] = {"port", "test_name12", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth7", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth8", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth1",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";
  const char *inter_argv3[] = {"interface", "test_eth7", "destroy",
                               NULL
                              };
  const char inter_test_str3[] = "{\"ret\":\"OK\"}";
  const char *inter_argv4[] = {"interface", "test_eth8", "destroy",
                               NULL
                              };
  const char inter_test_str4[] = "{\"ret\":\"OK\"}";

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* config cmd (show). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, port_cmd_update,
                 &ds, str, test_str5);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv3), inter_argv3, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str3);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv4), inter_argv4, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str4);
}

void
test_port_cmd_parse_destroy_used(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"port", "test_name13", "create",
                         "-interface", "test_eth13",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name13", "disable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"port", "test_name13", "destroy",
                         NULL
                        };
  const char test_str3[] = {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                            "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name13: is used.\"}"
                           };
  const char *argv4[] = {"port", "test_name13", "destroy",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth13", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth13", "destroy",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv1[] = {"bridge", "test_bridge13", "create",
                                "-port", "test_name13", "65535",
                                NULL
                               };
  const char bridge_test_str1[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv2[] = {"bridge", "test_bridge13", "destroy",
                                NULL
                               };
  const char bridge_test_str2[] = "{\"ret\":\"OK\"}";


  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv1), bridge_argv1, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str1);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, port_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                 port_cmd_update, &ds, str, test_str3);

  /* bridge destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv2), bridge_argv2, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse,
                 &interp, state, ARGV_SIZE(argv4), argv4, &tbl,
                 port_cmd_update, &ds, str, test_str4);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);
}

void
test_port_cmd_parse_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  char *str = NULL;
  const char *argv1[] = {"port", "test_name14", "create",
                         "-interface", "test_eth9",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name14", "current",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name14\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth9\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"port", "test_name14", "modified",
                         NULL
                        };
  const char test_str3[] =  {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                             "\"data\":\"Not set modified.\"}"
                            };
  const char *argv4[] = {"port", "test_name14", "config",
                         "-interface", "test_eth10",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"port", "test_name14", "modified",
                         NULL
                        };
  const char test_str5[] =  {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                             "\"data\":\"Not set modified.\"}"
                            };
  const char *argv6[] = {"port", "test_name14", "config",
                         "-interface", "test_eth11",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"port", "test_name14", "modified",
                         NULL
                        };
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name14\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth11\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"port", "test_name14", "destroy",
                         NULL
                        };
  const char test_str8[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth9", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth10", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth1",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";
  const char *inter_argv3[] = {"interface", "test_eth11", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth2",
                               NULL
                              };
  const char inter_test_str3[] = "{\"ret\":\"OK\"}";
  const char *inter_argv4[] = {"interface", "test_eth9", "destroy",
                               NULL
                              };
  const char inter_test_str4[] = "{\"ret\":\"OK\"}";
  const char *inter_argv5[] = {"interface", "test_eth10", "destroy",
                               NULL
                              };
  const char inter_test_str5[] = "{\"ret\":\"OK\"}";
  const char *inter_argv6[] = {"interface", "test_eth11", "destroy",
                               NULL
                              };
  const char inter_test_str6[] = "{\"ret\":\"OK\"}";

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(inter_argv3), inter_argv3, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str3);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, port_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv3), argv3, &tbl,
                 port_cmd_update, &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (modified : AUTO COMMIT mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, port_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv5), argv5, &tbl,
                 port_cmd_update, &ds, str, test_str5);

  /* config cmd (COMMITING mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv6), argv6, &tbl, port_cmd_update,
                 &ds, str, test_str6);

  /* show cmd (modified : COMMITING mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, port_cmd_update,
                 &ds, str, test_str7);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, port_cmd_update,
                 &ds, str, test_str8);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(inter_argv4), inter_argv4, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str4);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(inter_argv5), inter_argv5, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str5);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(inter_argv6), inter_argv6, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str6);
}

void
test_port_cmd_serialize_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char serialize_str1[] =
    "port "DATASTORE_NAMESPACE_DELIMITER"test_name15 create "
    "-interface "DATASTORE_NAMESPACE_DELIMITER"test_eth15 "
    "-policer "DATASTORE_NAMESPACE_DELIMITER"p15_1\n";
  const char *argv1[] = {"port", "test_name15", "create",
                         "-interface", "test_eth15",
                         "-policer", "p15_1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name15", "disable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"port", "test_name15", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth15", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth15", "destroy",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";

  TEST_POLICER_CREATE(ret, &interp, state, &tbl, &ds, str, "pa15_1", "p15_1");

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_serialize, &interp, state, &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name15", conf, &ds, str, serialize_str1);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                 port_cmd_update, &ds, str, test_str3);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);

  TEST_POLICER_DESTROY(ret, &interp, state, &tbl, &ds, str, "pa15_1", "p15_1")
}

void
test_port_cmd_serialize_escape(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char serialize_str1[] =
    "port \""DATASTORE_NAMESPACE_DELIMITER"test_\\\"name16\" create "
    "-interface \""DATASTORE_NAMESPACE_DELIMITER"test_\\\"eth16\"\n";
  const char *argv1[] = {"port", "test_\"name16", "create",
                         "-interface", "test_\"eth16",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_\"name16", "disable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"port", "test_\"name16", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_\"eth16", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_\"eth16", "destroy",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_serialize, &interp, state, &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_\"name16", conf, &ds, str, serialize_str1);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                 port_cmd_update, &ds, str, test_str3);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);
}

void
test_port_cmd_serialize_escape_white_space(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char serialize_str1[] =
    "port \""DATASTORE_NAMESPACE_DELIMITER"test name17\" create "
    "-interface \""DATASTORE_NAMESPACE_DELIMITER"test eth17\"\n";
  const char *argv1[] = {"port", "test name17", "create",
                         "-interface", "test eth17",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test name17", "disable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"port", "test name17", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test eth17", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test eth17", "destroy",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_serialize, &interp, state, &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test name17", conf, &ds, str, serialize_str1);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                 port_cmd_update, &ds, str, test_str3);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);
}

void
test_port_cmd_serialize_none_opts(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char serialize_str1[] =
    "port "DATASTORE_NAMESPACE_DELIMITER"test_name18 create\n";
  const char *argv1[] = {"port", "test_name18", "create",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name18", "disable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"port", "test_name18", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_serialize, &interp, state, &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name18", conf, &ds, str, serialize_str1);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                 port_cmd_update, &ds, str, test_str3);
}

void
test_port_cmd_parse_atomic_commit(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"port", "test_name19", "create",
                         "-interface", "test_eth19",
                         "-policer", "p19_1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name19", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"port", "test_name19", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name19\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth19\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p19_1\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"port", "test_name19", "config",
                         "-interface", "test_eth19_2",
                         "-policer", "p19_2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"port", "test_name19", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"port", "test_name19", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name19\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth19_2\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p19_2\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"port", "test_name19", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name19\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth19_2\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p19_2\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"port", "test_name19", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"port", "test_name19", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth19", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth19_2", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";
  const char *inter_argv3[] = {"interface", "test_eth19", "destroy",
                               NULL
                              };
  const char inter_test_str3[] = "{\"ret\":\"OK\"}";
  const char *inter_argv4[] = {"interface", "test_eth19_2", "destroy",
                               NULL
                              };
  const char inter_test_str4[] = "{\"ret\":\"OK\"}";

  TEST_POLICER_CREATE(ret, &interp, state4, &tbl, &ds, str, "pa19_1", "p19_1");
  TEST_POLICER_CREATE(ret, &interp, state4, &tbl, &ds, str, "pa19_2", "p19_2");

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, port_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, port_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name19", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name19", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, port_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, port_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, port_cmd_update,
                 &ds, str, test_str9);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv3), inter_argv3, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str3);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv4), inter_argv4, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str4);

  TEST_POLICER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "pa19_1", "p19_1");
  TEST_POLICER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "pa19_2", "p19_2");
}

void
test_port_cmd_parse_atomic_rollback(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ROLLBACKING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ROLLBACKED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"port", "test_name20", "create",
                         "-interface", "test_eth20",
                         "-policer", "p20_1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name20", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"port", "test_name20", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name20\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth20\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p20_1\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"port", "test_name20", "config",
                         "-interface", "test_eth20_2",
                         "-policer", "p20_2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"port", "test_name20", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"port", "test_name20", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name20\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth20_2\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p20_2\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"port", "test_name20", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = :test_name20\"}";
  const char *inter_argv1[] = {"interface", "test_eth20", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth20_2", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";
  const char *inter_argv3[] = {"interface", "test_eth20", "destroy",
                               NULL
                              };
  const char inter_test_str3[] = "{\"ret\":\"OK\"}";
  const char *inter_argv4[] = {"interface", "test_eth20_2", "destroy",
                               NULL
                              };
  const char inter_test_str4[] = "{\"ret\":\"OK\"}";

  TEST_POLICER_CREATE(ret, &interp, state4, &tbl, &ds, str, "pa20_1", "p20_1");
  TEST_POLICER_CREATE(ret, &interp, state4, &tbl, &ds, str, "pa20_2", "p20_2");

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret,  LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, port_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, port_cmd_update,
                 &ds, str, test_str6);

  /* rollbacking. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name20", conf, &ds);

  /* rollbacked. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name20", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, port_cmd_update,
                 &ds, str, test_str7);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv3), inter_argv3, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str3);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv4), inter_argv4, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str4);

  TEST_POLICER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "pa20_1", "p20_1");
  TEST_POLICER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "pa20_2", "p20_2");
}

void
test_port_cmd_parse_atomic_delay_enable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"port", "test_name21", "create",
                         "-interface", "test_eth21",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name21", "enable", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"port", "test_name21", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv4[] = {"port", "test_name21", "modified", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name21\",\n"
    "\"port-number\":65535,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth21\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":{},\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"port", "test_name21", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"port", "test_name21", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name21\",\n"
    "\"port-number\":65535,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth21\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":{},\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv7[] = {"port", "test_name21", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name21\",\n"
    "\"port-number\":65535,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth21\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":{},\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv8[] = {"port", "test_name21", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"port", "test_name21", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth21", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth21", "destroy",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv1[] = {"bridge", "test_bridge21", "create",
                                "-port", "test_name21", "65535",
                                NULL
                               };
  const char bridge_test_str1[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv2[] = {"bridge", "test_bridge21", "destroy",
                                NULL
                               };
  const char bridge_test_str2[] = "{\"ret\":\"OK\"}";

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(bridge_argv1), bridge_argv1, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name21", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, port_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, port_cmd_update,
                 &ds, str, test_str6);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name21", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, port_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, port_cmd_update,
                 &ds, str, test_str8);

  /* bridge destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(bridge_argv2), bridge_argv2, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, port_cmd_update,
                 &ds, str, test_str9);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);
}

void
test_port_cmd_parse_atomic_delay_disable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"port", "test_name22", "create",
                         "-interface", "test_eth22",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name22", "enable", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"port", "test_name22", "disable", NULL};
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"port", "test_name22", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name22\",\n"
    "\"port-number\":65535,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth22\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":{},\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"port", "test_name22", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name22\",\n"
    "\"port-number\":65535,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth22\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":{},\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"port", "test_name22", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name22\",\n"
    "\"port-number\":65535,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth22\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":{},\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"port", "test_name22", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth22", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth22", "destroy",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv1[] = {"bridge", "test_bridge22", "create",
                                "-port", "test_name22", "65535",
                                NULL
                               };
  const char bridge_test_str1[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv2[] = {"bridge", "test_bridge22", "destroy",
                                NULL
                               };
  const char bridge_test_str2[] = "{\"ret\":\"OK\"}";

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(bridge_argv1), bridge_argv1, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name22", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, port_cmd_update,
                 &ds, str, test_str5);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name22", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, port_cmd_update,
                 &ds, str, test_str6);

  /* bridge destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(bridge_argv2), bridge_argv2, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv7), argv7, &tbl, port_cmd_update,
                 &ds, str, test_str7);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);
}

void
test_port_cmd_parse_atomic_delay_destroy(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"port", "test_name23", "create",
                         "-interface", "test_eth23",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name23", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"port", "test_name23", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name23\"}";
  const char *argv4[] = {"port", "test_name23", "enable",
                         NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name23\"}";
  const char *argv5[] = {"port", "test_name23", "disable",
                         NULL
                        };
  const char test_str5[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name23\"}";
  const char *argv6[] = {"port", "test_name23", "destroy",
                         NULL
                        };
  const char test_str6[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name23\"}";
  const char *argv7[] = {"port", "test_name23", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name23\"}";
  const char *inter_argv1[] = {"interface", "test_eth23", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth23", "destroy",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, port_cmd_update,
                 &ds, str, test_str5);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, port_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name23", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name23", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, port_cmd_update,
                 &ds, str, test_str7);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);
}

void
test_port_cmd_parse_atomic_abort_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"port", "test_name24", "create",
                         "-interface", "test_eth24",
                         "-policer", "p24_1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name24", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"port", "test_name24", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name24\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth24\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p24_1\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"port", "test_name24", "config",
                         "-interface", "test_eth24_2",
                         "-policer", "p24_2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"port", "test_name24", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"port", "test_name24", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name24\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth24_2\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p24_2\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"port", "test_name24", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv8[] = {"port", "test_name24", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name24\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth24_2\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p24_2\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"port", "test_name24", NULL};
  const char test_str9[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name24\"}";
  const char *inter_argv1[] = {"interface", "test_eth24", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth24_2", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";
  const char *inter_argv3[] = {"interface", "test_eth24", "destroy",
                               NULL
                              };
  const char inter_test_str3[] = "{\"ret\":\"OK\"}";
  const char *inter_argv4[] = {"interface", "test_eth24_2", "destroy",
                               NULL
                              };
  const char inter_test_str4[] = "{\"ret\":\"OK\"}";

  TEST_POLICER_CREATE(ret, &interp, state4, &tbl, &ds, str, "pa24_1", "p24_1");
  TEST_POLICER_CREATE(ret, &interp, state4, &tbl, &ds, str, "pa24_2", "p24_2");

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, port_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, port_cmd_update,
                 &ds, str, test_str6);

  /* aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name24", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, port_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, port_cmd_update,
                 &ds, str, test_str8);

  /* aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name24", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv9), argv9, &tbl, port_cmd_update,
                 &ds, str, test_str9);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv3), inter_argv3, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str3);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv4), inter_argv4, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str4);

  TEST_POLICER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "pa24_1", "p24_1");
  TEST_POLICER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "pa24_2", "p24_2");
}

void
test_port_cmd_parse_atomic_abort_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"port", "test_name25", "create",
                         "-interface", "test_eth25",
                         "-policer", "p25_1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name25", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name25\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth25\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p25_1\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"port", "test_name25", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv4[] = {"port", "test_name25", "config",
                         "-interface", "test_eth25_2",
                         "-policer", "p25_2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"port", "test_name25", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name25\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth25\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p25_1\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"port", "test_name25", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name25\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth25_2\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p25_2\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"port", "test_name25", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name25\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth25\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p25_1\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"port", "test_name25", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name25\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth25_2\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p25_2\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"port", "test_name25", NULL};
  const char test_str9[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name25\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth25\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p25_1\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv10[] = {"port", "test_name25", "modified", NULL};
  const char test_str10[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv11[] = {"port", "test_name25", "destroy",
                          NULL
                         };
  const char test_str11[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth25", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth25_2", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";
  const char *inter_argv3[] = {"interface", "test_eth25", "destroy",
                               NULL
                              };
  const char inter_test_str3[] = "{\"ret\":\"OK\"}";
  const char *inter_argv4[] = {"interface", "test_eth25_2", "destroy",
                               NULL
                              };
  const char inter_test_str4[] = "{\"ret\":\"OK\"}";

  TEST_POLICER_CREATE(ret, &interp, state4, &tbl, &ds, str, "pa25_1", "p25_1");
  TEST_POLICER_CREATE(ret, &interp, state4, &tbl, &ds, str, "pa25_2", "p25_2");

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, port_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, port_cmd_update,
                 &ds, str, test_str6);

  /* aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name25", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, port_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, port_cmd_update,
                 &ds, str, test_str8);

  /* aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name25", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv9), argv9, &tbl, port_cmd_update,
                 &ds, str, test_str9);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv10), argv10, &tbl, port_cmd_update,
                 &ds, str, test_str10);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv11), argv11, &tbl, port_cmd_update,
                 &ds, str, test_str11);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv3), inter_argv3, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str3);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv4), inter_argv4, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str4);

  TEST_POLICER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "pa25_1", "p25_1");
  TEST_POLICER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "pa25_2", "p25_2");
}

void
test_port_cmd_parse_atomic_destroy_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"port", "test_name26", "create",
                         "-interface", "test_eth26",
                         "-policer", "p26_1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name26", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"port", "test_name26", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name26\"}";
  const char *argv4[] = {"port", "test_name26", "create",
                         "-interface", "test_eth26_2",
                         "-policer", "p26_2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"port", "test_name26", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name26\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth26\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p26_1\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"port", "test_name26", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name26\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth26_2\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p26_2\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"port", "test_name26", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name26\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth26_2\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p26_2\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"port", "test_name26", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"port", "test_name26", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth26", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth26_2", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";
  const char *inter_argv3[] = {"interface", "test_eth26", "destroy",
                               NULL
                              };
  const char inter_test_str3[] = "{\"ret\":\"OK\"}";
  const char *inter_argv4[] = {"interface", "test_eth26_2", "destroy",
                               NULL
                              };
  const char inter_test_str4[] = "{\"ret\":\"OK\"}";

  TEST_POLICER_CREATE(ret, &interp, state4, &tbl, &ds, str, "pa26_1", "p26_1");
  TEST_POLICER_CREATE(ret, &interp, state4, &tbl, &ds, str, "pa26_2", "p26_2");

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, port_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, port_cmd_update,
                 &ds, str, test_str6);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name26", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name26", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv7), argv7, &tbl, port_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv8), argv8, &tbl, port_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, port_cmd_update,
                 &ds, str, test_str9);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv3), inter_argv3, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str3);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state4,
                 ARGV_SIZE(inter_argv4), inter_argv4, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str4);

  TEST_POLICER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "pa26_1", "p26_1");
  TEST_POLICER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "pa26_2", "p26_2");
}

void
test_port_cmd_parse_stats_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"port", "test_name27", "create",
                         "-interface", "test_eth27",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name27", "stats",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name27\",\n"
    "\"config\":[\"port-down\"],\n"
    "\"state\":[],\n"
    "\"curr-features\":[],\n"
    "\"supported-features\":[]}]}";
  const char *argv3[] = {"port", "test_name27", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth27", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth27", "destroy",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* stat cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);
}

void
test_port_cmd_parse_stats_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"port", "test_name28", "create",
                         "-interface", "test_eth28",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name28", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"port", "test_name28", "stats",
                         NULL
                        };
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name28\",\n"
    "\"config\":[],\n"
    "\"state\":[],\n"
    "\"curr-features\":[],\n"
    "\"supported-features\":[]}]}";
  const char *argv4[] = {"port", "test_name28", "destroy",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth28", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth28", "destroy",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv1[] = {"bridge", "test_bridge28", "create",
                                "-port", "test_name28", "65535",
                                NULL
                               };
  const char bridge_test_str1[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv2[] = {"bridge", "test_bridge28", "destroy",
                                NULL
                               };
  const char bridge_test_str2[] = "{\"ret\":\"OK\"}";

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv1), bridge_argv1, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* stat cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* bridge destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv2), bridge_argv2, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);
}

void
test_port_cmd_parse_create_not_queue_name_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"port", "test_name29", "create",
                        "-queue", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, port_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, port_cmd_update, &ds, str, test_str);
}

void
test_port_cmd_parse_create_bad_queue_name_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"port", "test_name29", "create",
                        "-queue", "hoge",
                        NULL
                       };
  const char test_str[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"queue name = "DATASTORE_NAMESPACE_DELIMITER"hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, port_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, port_cmd_update, &ds, str, test_str);
}

void
test_port_cmd_parse_create_not_policer_name_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"port", "test_name31", "create",
                        "-policer", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, port_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, port_cmd_update, &ds, str, test_str);
}

void
test_port_cmd_parse_create_bad_policer_name_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"port", "test_name32", "create",
                        "-policer", "hoge",
                        NULL
                       };
  const char test_str[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"policer name = "DATASTORE_NAMESPACE_DELIMITER"hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, port_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, port_cmd_update, &ds, str, test_str);
}

void
test_port_cmd_parse_config_delete_policer(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"port", "test_name33", "create",
                         "-interface", "test_eth33",
                         "-policer", "+p33_1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name33", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"port", "test_name33", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name33\",\n"
    "\"port-number\":65535,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth33\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p33_1\",\n"
    "\"queues\":{},\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv4[] = {"port", "test_name33", "config",
                         "-policer", "~p33_1",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"port", "test_name33", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name33\",\n"
    "\"port-number\":65535,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth33\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":{},\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *argv6[] = {"port", "test_name33", "disable",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"port", "test_name33", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "test_eth33", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth33", "destroy",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";
  const char *policer_argv1[] = {"policer", "p33_1", NULL};
  const char policer_test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"p33_1\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"pa33_1\"],\n"
    "\"bandwidth-limit\":1500,\n"
    "\"burst-size-limit\":1500,\n"
    "\"bandwidth-percent\":0,\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *policer_argv2[] = {"queue", "p33_1", NULL};
  const char policer_test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"p33_1\",\n"
    "\"actions\":[\""DATASTORE_NAMESPACE_DELIMITER"pa33_1\"],\n"
    "\"bandwidth-limit\":1500,\n"
    "\"burst-size-limit\":1500,\n"
    "\"bandwidth-percent\":0,\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *bridge_argv1[] = {"bridge", "test_bridge33", "create",
                                "-port", "test_name33", "65535",
                                NULL
                               };
  const char bridge_test_str1[] = "{\"ret\":\"OK\"}";
  const char *bridge_argv2[] = {"bridge", "test_bridge33", "destroy",
                                NULL
                               };
  const char bridge_test_str2[] = "{\"ret\":\"OK\"}";

  TEST_POLICER_CREATE(ret, &interp, state, &tbl, &ds, str, "pa33_1", "p33_1");

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv1), bridge_argv1, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* policer show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(policer_argv1), policer_argv1, &tbl, policer_cmd_update,
                 &ds, str, policer_test_str1);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, port_cmd_update,
                 &ds, str, test_str5);

  /* policer show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, policer_cmd_parse, &interp, state,
                 ARGV_SIZE(policer_argv2), policer_argv2, &tbl, policer_cmd_update,
                 &ds, str, policer_test_str2);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, port_cmd_update,
                 &ds, str, test_str6);

  /* bridge destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(bridge_argv2), bridge_argv2, &tbl, bridge_cmd_update,
                 &ds, str, bridge_test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, port_cmd_update,
                 &ds, str, test_str7);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);

  TEST_POLICER_DESTROY(ret, &interp, state, &tbl, &ds, str, "pa33_1", "p33_1");
}

void
test_port_cmd_parse_dryrun(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_DRYRUN;
  char *str = NULL;
  const char *argv1[] = {"port", "test_name34", "create",
                         "-interface", "test_eth34",
                         "-policer", "p34_1",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"port", "test_name34", "current", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name34\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth34\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p34_1\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"port", "test_name34", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv4[] = {"port", "test_name34", "config",
                         "-interface", "test_eth34_2",
                         "-policer", "p34_2",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"port", "test_name34", "current", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name34\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"test_eth34_2\",\n"
    "\"policer\":\""DATASTORE_NAMESPACE_DELIMITER"p34_2\",\n"
    "\"queues\":{},\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"port", "test_name34", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";

  const char *inter_argv1[] = {"interface", "test_eth34", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str1[] = "{\"ret\":\"OK\"}";
  const char *inter_argv2[] = {"interface", "test_eth34_2", "create",
                               "-type", "ethernet-rawsock",
                               "-device", "eth0",
                               NULL
                              };
  const char inter_test_str2[] = "{\"ret\":\"OK\"}";
  const char *inter_argv3[] = {"interface", "test_eth34", "destroy",
                               NULL
                              };
  const char inter_test_str3[] = "{\"ret\":\"OK\"}";
  const char *inter_argv4[] = {"interface", "test_eth34_2", "destroy",
                               NULL
                              };
  const char inter_test_str4[] = "{\"ret\":\"OK\"}";

  TEST_POLICER_CREATE(ret, &interp, state1, &tbl, &ds, str, "pa34_1", "p34_1");
  TEST_POLICER_CREATE(ret, &interp, state1, &tbl, &ds, str, "pa34_2", "p34_2");

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* interface create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, port_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, port_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, port_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv4), argv4, &tbl, port_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv5), argv5, &tbl, port_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 port_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv6), argv6, &tbl, port_cmd_update,
                 &ds, str, test_str6);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(inter_argv3), inter_argv3, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str3);

  /* interface destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state1,
                 ARGV_SIZE(inter_argv4), inter_argv4, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str4);

  TEST_PORT_DESTROY(ret, &interp, state1, &tbl, &ds, str, NULL, "test_name34");

  TEST_POLICER_DESTROY(ret, &interp, state1, &tbl, &ds, str, "pa34_1", "p34_1");
  TEST_POLICER_DESTROY(ret, &interp, state1, &tbl, &ds, str, "pa34_2", "p34_2");
}

void
test_destroy(void) {
  destroy = true;
}
