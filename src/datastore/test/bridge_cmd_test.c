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
#include "lagopus/ofp_bridgeq_mgr.h"
#include "lagopus/bridge.h"
#include "../datastore_apis.h"
#include "../channel_cmd_internal.h"
#include "../controller_cmd_internal.h"
#include "../interface_cmd_internal.h"
#include "../port_cmd_internal.h"
#include "../bridge_cmd_internal.h"
#include "../datastore_internal.h"
#include "../../agent/channel_mgr.h"

static lagopus_dstring_t ds = NULL;
static lagopus_hashmap_t tbl = NULL;
static lagopus_hashmap_t tbl_port = NULL;
static datastore_interp_t interp = NULL;
static bool destroy = false;

void
setUp(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create interp. */
  INTERP_CREATE(ret, "bridge", interp, tbl, ds);
  dp_dataq_put_func_register(ofp_handler_dataq_data_put);
  dp_eventq_put_func_register(ofp_handler_eventq_data_put);

  ret = datastore_find_table("port", &tbl_port,
                             NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "datastore_find_table error.");
}

void
tearDown(void) {
  /* destroy interp. */
  INTERP_DESTROY("bridge", interp, tbl, ds, destroy);
}

void
test_bridge_cmd_parse_create_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name01", "create",
                        "-controller", "c1",
                        "-controller", "c2",
                        "-port", "p1", "1",
                        "-port", "p2", "2",
                        "-dpid", "18446744073709551615",
                        "-fail-mode", "secure",
                        "-flow-statistics", "true",
                        "-group-statistics", "true",
                        "-port-statistics", "true",
                        "-queue-statistics", "true",
                        "-table-statistics", "true",
                        "-reassemble-ip-fragments", "true",
                        "-max-buffered-packets", "65535",
                        "-max-ports", "2048",
                        "-max-tables", "255",
                        "-max-flows", "4294967295",
                        "-packet-inq-size", "65535",
                        "-packet-inq-max-batches", "65535",
                        "-up-streamq-size", "65535",
                        "-up-streamq-max-batches", "65535",
                        "-down-streamq-size", "65535",
                        "-down-streamq-max-batches", "65535",
                        "-block-looping-ports", "true",
                        "-action-type", "copy-ttl-out",
                        "-instruction-type", "apply-actions",
                        NULL
                       };
  const char test_str[] = "{\"ret\":\"OK\"}";
  const char *argv1[] = {"bridge", "test_name02", "create",
                         "-controller", "c5",
                         "-port", "p5", "5",
                         "-dpid", "5",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name02", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c5\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p5\":5},\n"
    "\"dpid\":5,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":true},\n"
    "{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name01\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c1\",\""DATASTORE_NAMESPACE_DELIMITER"c2\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p1\":1,\n"
    "\""DATASTORE_NAMESPACE_DELIMITER"p2\":2},\n"
    "\"dpid\":18446744073709551615,\n"
    "\"fail-mode\":\"secure\",\n"
    "\"flow-statistics\":true,\n"
    "\"group-statistics\":true,\n"
    "\"port-statistics\":true,\n"
    "\"queue-statistics\":true,\n"
    "\"table-statistics\":true,\n"
    "\"reassemble-ip-fragments\":true,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":true,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"bridge", "test_name02", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name02\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c5\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p5\":5},\n"
    "\"dpid\":5,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"bridge", "test_name02", "disable",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *argv6[] = {"bridge", "test_name02", "destroy",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"bridge", "test_name01", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *port_argv1[] = {"port", "p5", NULL};
  const char port_test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"p5\",\n"
    "\"port-number\":5,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"i5\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":[],\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *port_argv2[] = {"port", "p5", NULL};
  const char port_test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"p5\",\n"
    "\"port-number\":5,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"i5\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":[],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *ctrler_argv1[] = {"controller", "c5", NULL};
  const char ctrler_test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"c5\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"cha5\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *ctrler_argv2[] = {"controller", "c5", NULL};
  const char ctrler_test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"c5\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"cha5\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";

  TEST_CONTROLLER_CREATE(ret, &interp, state, &tbl, &ds, str, "cha1", "c1");
  TEST_CONTROLLER_CREATE(ret, &interp, state, &tbl, &ds, str, "cha2", "c2");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i1", "p1");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i2", "p2");
  TEST_CONTROLLER_CREATE(ret, &interp, state, &tbl, &ds, str, "cha5", "c5");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i5", "p5");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv), argv, &tbl, bridge_cmd_update,
                 &ds, str, test_str);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* show all cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* port show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv1), port_argv1, &tbl, port_cmd_update,
                 &ds, str, port_test_str1);

  /* controller show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(ctrler_argv1), ctrler_argv1, &tbl, controller_cmd_update,
                 &ds, str, ctrler_test_str1);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, bridge_cmd_update,
                 &ds, str, test_str7);

  /* port show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv2), port_argv2, &tbl, port_cmd_update,
                 &ds, str, port_test_str2);

  /* controller show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(ctrler_argv2), ctrler_argv2, &tbl, controller_cmd_update,
                 &ds, str, ctrler_test_str2);

  TEST_CONTROLLER_DESTROY(ret, &interp, state, &tbl, &ds, str, "cha1", "c1");
  TEST_CONTROLLER_DESTROY(ret, &interp, state, &tbl, &ds, str, "cha2", "c2");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i1", "p1");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i2", "p2");
  TEST_CONTROLLER_DESTROY(ret, &interp, state, &tbl, &ds, str, "cha5", "c5");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i5", "p5");
}

void
test_bridge_cmd_parse_create_bad_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name03", "create", "-hoge", NULL};
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"opt = -hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_not_controller_name_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name04", "create",
                        "-controller", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_not_port_name_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name05", "create",
                        "-port", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_not_port_no_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name05_2", "create",
                        "-port", "p5_2", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i5_2", "p5_2");

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);

  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i5_2", "p5_2");
}

void
test_bridge_cmd_parse_create_not_port_no_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name05_3", "create",
                        "-port", "p5_3", "-dpid", "1", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = -dpid.\"}";

  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i5_3", "p5_3");

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);

  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i5_3", "p5_3");
}

void
test_bridge_cmd_parse_create_not_dpid_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name06", "create",
                        "-dpid", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_dpid_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name07", "create",
                        "-dpid", "18446744073709551616"
                       };
  const char test_str[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = 18446744073709551616.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_bad_dpid(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name08", "create",
                        "-dpid", "hoge"
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_not_fail_mode_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name09", "create",
                        "-fail-mode", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_bad_fail_mode(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name10", "create",
                        "-fail-mode", "hoge"
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_not_flow_statistics_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name11", "create",
                        "-flow-statistics", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_bad_flow_statistics(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name12", "create",
                        "-flow-statistics", "hoge"
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_not_max_buffered_packets_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name13", "create",
                        "-max-buffered-packets", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_max_buffered_packets_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name14", "create",
                         "-max-buffered-packets", "65536", NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"TOO_LONG\",\n"
    "\"data\":\"Can't add max-buffered-packets.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);
}

void
test_bridge_cmd_parse_create_bad_max_buffered_packets(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name15", "create",
                         "-max-buffered-packets", "hoge"
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";
  const char *argv2[] = {"bridge", "test_name15", "create",
                         "-max-buffered-packets", "-1", NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = -1.\"}";
  const char *argv3[] = {"bridge", "test_name15", "create",
                         "-max-buffered-packets", "0.5", NULL
                        };
  const char test_str3[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = 0.5.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3,
                 &tbl, bridge_cmd_update, &ds, str, test_str3);
}

void
test_bridge_cmd_parse_create_not_max_ports_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name16", "create",
                        "-max-ports", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_max_ports_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name17", "create",
                         "-max-ports", "2049", NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"TOO_LONG\",\n"
    "\"data\":\"Can't add max-ports.\"}";
  const char *argv2[] = {"bridge", "test_name17", "create",
                         "-max-ports", "0", NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"TOO_SHORT\",\n"
    "\"data\":\"Can't add max-ports.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);
}

void
test_bridge_cmd_parse_create_bad_max_ports(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name18", "create",
                         "-max-ports", "hoge"
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";
  const char *argv2[] = {"bridge", "test_name18", "create",
                         "-max-ports", "-1", NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = -1.\"}";
  const char *argv3[] = {"bridge", "test_name18", "create",
                         "-max-ports", "0.5", NULL
                        };
  const char test_str3[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = 0.5.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3,
                 &tbl, bridge_cmd_update, &ds, str, test_str3);
}

void
test_bridge_cmd_parse_create_not_max_tables_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name19", "create",
                        "-max-tables", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_max_tables_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name20", "create",
                         "-max-tables", "256", NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"TOO_LONG\",\n"
    "\"data\":\"Can't add max-tables.\"}";
  const char *argv2[] = {"bridge", "test_name20", "create",
                         "-max-tables", "0", NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"TOO_SHORT\",\n"
    "\"data\":\"Can't add max-tables.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);
}

void
test_bridge_cmd_parse_create_bad_max_tables(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name21", "create",
                         "-max-tables", "hoge"
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";
  const char *argv2[] = {"bridge", "test_name21", "create",
                         "-max-tables", "-1", NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = -1.\"}";
  const char *argv3[] = {"bridge", "test_name21", "create",
                         "-max-tables", "0.5", NULL
                        };
  const char test_str3[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = 0.5.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3,
                 &tbl, bridge_cmd_update, &ds, str, test_str3);
}

void
test_bridge_cmd_parse_create_port_not_exists(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name22", "create",
                         "-controller", "c6",
                         "-port", "p6", "6",
                         "-dpid", "18446744073709551615",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "4294967295",
                         "-max-ports", "65535",
                         "-max-tables", "255",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"port name = "DATASTORE_NAMESPACE_DELIMITER"p6.\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state, &tbl, &ds, str, "cha6", "c6");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  TEST_CONTROLLER_DESTROY(ret, &interp, state, &tbl, &ds, str, "cha6", "c6");
}

void
test_bridge_cmd_parse_create_controller_not_exists(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name23", "create",
                         "-controller", "c7",
                         "-port", "p7", "7",
                         "-dpid", "18446744073709551615",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"controller name = "DATASTORE_NAMESPACE_DELIMITER"c7.\"}";

  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i7", "p7");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i7", "p7");
}

void
test_bridge_cmd_enable_destroy_propagation(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name24", "create",
                         "-controller", "c8",
                         "-port", "p8", "8",
                         "-dpid", "18446744073709551615",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name24", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", "test_name24", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *inter_argv1[] = {"interface", "i8", NULL};
  const char inter_test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"i8\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"i8\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *inter_argv2[] = {"interface", "i8", NULL};
  const char inter_test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"i8\",\n"
    "\"type\":\"ethernet-rawsock\",\n"
    "\"device\":\"i8\",\n"
    "\"mtu\":1500,\n"
    "\"ip-addr\":\"127.0.0.1\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *port_argv1[] = {"port", "p8", NULL};
  const char port_test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"p8\",\n"
    "\"port-number\":8,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"i8\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":[],\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *port_argv2[] = {"port", "p8", NULL};
  const char port_test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"p8\",\n"
    "\"port-number\":8,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"i8\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":[],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *cha_argv1[] = {"channel", "cha8", NULL};
  const char cha_test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"cha8\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *cha_argv2[] = {"channel", "cha8", NULL};
  const char cha_test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"cha8\",\n"
    "\"dst-addr\":\"127.0.0.1\",\n"
    "\"dst-port\":3000,\n"
    "\"local-addr\":\"127.0.0.1\",\n"
    "\"local-port\":3000,\n"
    "\"protocol\":\"tcp\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *ctrler_argv1[] = {"controller", "c8", NULL};
  const char ctrler_test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"c8\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"cha8\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";
  const char *ctrler_argv2[] = {"controller", "c8", NULL};
  const char ctrler_test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"c8\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"cha8\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";

  TEST_CONTROLLER_CREATE(ret, &interp, state, &tbl, &ds, str, "cha8", "c8");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i8", "p8");
  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* interface show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv1), inter_argv1, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str1);

  /* port show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv1), port_argv1, &tbl, port_cmd_update,
                 &ds, str, port_test_str1);

  /* channel show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(cha_argv1), cha_argv1, &tbl,
                 channel_cmd_update,
                 &ds, str, cha_test_str1);

  /* controller show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(ctrler_argv1), ctrler_argv1, &tbl, controller_cmd_update,
                 &ds, str, ctrler_test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* interface show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, interface_cmd_parse, &interp, state,
                 ARGV_SIZE(inter_argv2), inter_argv2, &tbl, interface_cmd_update,
                 &ds, str, inter_test_str2);

  /* port show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv2), port_argv2, &tbl, port_cmd_update,
                 &ds, str, port_test_str2);

  /* channel show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, channel_cmd_parse, &interp, state,
                 ARGV_SIZE(cha_argv2), cha_argv2, &tbl,
                 channel_cmd_update,
                 &ds, str, cha_test_str2);

  /* controller show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(ctrler_argv2), ctrler_argv2, &tbl, controller_cmd_update,
                 &ds, str, ctrler_test_str2);

  TEST_CONTROLLER_DESTROY(ret, &interp, state, &tbl, &ds, str, "cha8", "c8");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i8", "p8");
}

void
test_bridge_cmd_parse_config_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name25", "create",
                         "-controller", "c25",
                         "-port", "p25", "25",
                         "-dpid", "18446744073709551615",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name25", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name25\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c25\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p25\":25},\n"
    "\"dpid\":18446744073709551615,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"bridge", "test_name25", "config",
                         "-controller", "c26",
                         "-port", "p26", "26",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"bridge", "test_name25", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name25\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c25\",\""DATASTORE_NAMESPACE_DELIMITER"c26\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p25\":25,\n"
    "\""DATASTORE_NAMESPACE_DELIMITER"p26\":26},\n"
    "\"dpid\":18446744073709551615,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"bridge", "test_name25", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *port_argv1[] = {"port", "p25", NULL};
  const char port_test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"p25\",\n"
    "\"port-number\":25,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"i25\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":[],\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *port_argv2[] = {"port", "p25", NULL};
  const char port_test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"p25\",\n"
    "\"port-number\":25,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"i25\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":[],\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *port_argv3[] = {"port", "p26", NULL};
  const char port_test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"p26\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"i26\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":[],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *port_argv4[] = {"port", "p26", NULL};
  const char port_test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"p26\",\n"
    "\"port-number\":26,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"i26\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":[],\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *ctrler_argv1[] = {"controller", "c25", NULL};
  const char ctrler_test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"c25\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"cha25\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *ctrler_argv2[] = {"controller", "c25", NULL};
  const char ctrler_test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"c25\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"cha25\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";
  const char *ctrler_argv3[] = {"controller", "c26", NULL};
  const char ctrler_test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"c26\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"cha26\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *ctrler_argv4[] = {"controller", "c26", NULL};
  const char ctrler_test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"c26\",\n"
    "\"channel\":\""DATASTORE_NAMESPACE_DELIMITER"cha26\",\n"
    "\"role\":\"equal\",\n"
    "\"connection-type\":\"main\",\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":false}]}";

  TEST_CONTROLLER_CREATE(ret, &interp, state, &tbl, &ds, str, "cha25", "c25");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i25", "p25");
  TEST_CONTROLLER_CREATE(ret, &interp, state, &tbl, &ds, str, "cha26", "c26");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i26", "p26");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* port show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv1), port_argv1, &tbl, port_cmd_update,
                 &ds, str, port_test_str1);

  /* port show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv3), port_argv3, &tbl, port_cmd_update,
                 &ds, str, port_test_str3);

  /* controller show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(ctrler_argv1), ctrler_argv1, &tbl, controller_cmd_update,
                 &ds, str, ctrler_test_str1);

  /* controller show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(ctrler_argv3), ctrler_argv3, &tbl, controller_cmd_update,
                 &ds, str, ctrler_test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* port show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv2), port_argv2, &tbl, port_cmd_update,
                 &ds, str, port_test_str2);

  /* port show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv4), port_argv4, &tbl, port_cmd_update,
                 &ds, str, port_test_str4);

  /* controller show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(ctrler_argv2), ctrler_argv2, &tbl, controller_cmd_update,
                 &ds, str, ctrler_test_str2);

  /* controller show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, controller_cmd_parse, &interp, state,
                 ARGV_SIZE(ctrler_argv4), ctrler_argv4, &tbl, controller_cmd_update,
                 &ds, str, ctrler_test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  TEST_CONTROLLER_DESTROY(ret, &interp, state, &tbl, &ds, str, "cha25", "c25");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i25", "p25");
  TEST_CONTROLLER_DESTROY(ret, &interp, state, &tbl, &ds, str, "cha26", "c26");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i26", "p26");
}

void
test_bridge_cmd_parse_config_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name\"27", "create",
                         "-controller", "c\"27",
                         "-port", "p\"27", "27",
                         "-dpid", "18446744073709551615",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name\"27", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", "test_name\"27", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name\\\"27\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c\\\"27\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p\\\"27\":27},\n"
    "\"dpid\":18446744073709551615,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":true}]}";
  const char *argv4[] = {"bridge", "test_name\"27", "config",
                         "-dpid", "1",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"bridge", "test_name\"27", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name\\\"27\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c\\\"27\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p\\\"27\":27},\n"
    "\"dpid\":1,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":true}]}";
  const char *argv6[] = {"bridge", "test_name\"27", "disable",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"bridge", "test_name\"27", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state, &tbl, &ds, str, "cha27", "c\"27");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i27", "p\"27");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, bridge_cmd_update,
                 &ds, str, test_str7);

  TEST_CONTROLLER_DESTROY(ret, &interp, state, &tbl, &ds, str, "cha27", "c\"27");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i27", "p\"27");
}

void
test_bridge_cmd_parse_config_delete_port_ctrler(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name28", "create",
                         "-controller", "+c28",
                         "-port", "p28", "28",
                         "-dpid", "18446744073709551615",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name28", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", "test_name28", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name28\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c28\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p28\":28},\n"
    "\"dpid\":18446744073709551615,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":true}]}";
  const char *argv4[] = {"bridge", "test_name28", "config",
                         "-controller", "-c28",
                         "-port", "~p28",
                         "-dpid", "1",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"bridge", "test_name28", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name28\",\n"
    "\"controllers\":[],\n"
    "\"ports\":{},\n"
    "\"dpid\":1,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":true}]}";
  const char *argv6[] = {"bridge", "test_name28", "disable",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"bridge", "test_name28", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state, &tbl, &ds, str, "cha28", "c28");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i28", "p28");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, bridge_cmd_update,
                 &ds, str, test_str7);

  TEST_CONTROLLER_DESTROY(ret, &interp, state, &tbl, &ds, str, "cha28", "c28");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i28", "p28");
}

void
test_bridge_cmd_parse_config_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name29", "create",
                         "-controller", "c29",
                         "-port", "p29", "29",
                         "-dpid", "18446744073709551615",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name29", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name29\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c29\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p29\":29},\n"
    "\"dpid\":18446744073709551615,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"bridge", "test_name29", "config",
                         "-dpid", "1",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"bridge", "test_name29", "config",
                         "-dpid", NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name29\",\n"
    "\"dpid\":1}]}";
  const char *argv5[] = {"bridge", "test_name29", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state, &tbl, &ds, str, "cha29", "c29");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i29", "p29");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd (show). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  TEST_CONTROLLER_DESTROY(ret, &interp, state, &tbl, &ds, str, "cha29", "c29");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i29", "p29");
}

void
test_bridge_cmd_show_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name30", "create",
                         "-controller", "c30",
                         "-port", "p30", "30",
                         "-dpid", "18446744073709551615",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name30", "current",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name30\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c30\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p30\":30},\n"
    "\"dpid\":18446744073709551615,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"bridge", "test_name30", "modified",
                         NULL
                        };
  const char test_str3[] = {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                            "\"data\":\"Not set modified.\"}"
                           };
  const char *argv4[] = {"bridge", "test_name30", "config",
                         "-dpid", "1",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"bridge", "test_name30", "modified",
                         NULL
                        };
  const char test_str5[] =  {"{\"ret\":\"NOT_OPERATIONAL\",\n"
                             "\"data\":\"Not set modified.\"}"
                            };
  const char *argv6[] = {"bridge", "test_name30", "destroy",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state1, &tbl, &ds, str, "cha30", "c30");
  TEST_PORT_CREATE(ret, &interp, state1, &tbl, &ds, str, "i30", "p30");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv3), argv3, &tbl,
                 bridge_cmd_update, &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (modified : AUTO COMMIT mode). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state1, ARGV_SIZE(argv5), argv5, &tbl,
                 bridge_cmd_update, &ds, str, test_str5);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  TEST_CONTROLLER_DESTROY(ret, &interp, state1, &tbl, &ds, str, "cha30", "c30");
  TEST_PORT_DESTROY(ret, &interp, state1, &tbl, &ds, str, "i30", "p30");
}

void
test_bridge_cmd_parse_config_port_number_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name31", "create",
                         "-controller", "c31",
                         "-port", "p31", "31",
                         "-dpid", "18446744073709551615",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name31", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", "test_name31", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name31\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c31\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p31\":31},\n"
    "\"dpid\":18446744073709551615,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":true}]}";
  const char *argv4[] = {"bridge", "test_name31", "config",
                         "-port", "~p31",
                         "-port", "p31", "32",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"bridge", "test_name31", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name31\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c31\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p31\":32},\n"
    "\"dpid\":18446744073709551615,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":true}]}";
  const char *argv6[] = {"bridge", "test_name31", "disable",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"bridge", "test_name31", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state, &tbl, &ds, str, "cha31", "c31");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i31", "p31");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, bridge_cmd_update,
                 &ds, str, test_str7);

  TEST_CONTROLLER_DESTROY(ret, &interp, state, &tbl, &ds, str, "cha31", "c31");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i31", "p31");
}

void
test_bridge_cmd_parse_config_port_number_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name32", "create",
                         "-controller", "c32",
                         "-port", "p32", "32",
                         "-dpid", "32",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name32", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", "test_name32", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name32\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c32\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p32\":32},\n"
    "\"dpid\":32,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":true}]}";
  const char *argv4[] = {"bridge", "test_name32", "config",
                         "-port", "~p32",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"bridge", "test_name32", "config",
                         "-port", "p33", "33",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *argv6[] = {"bridge", "test_name32", "disable",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"bridge", "test_name32", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";
  const char *port_argv1[] = {"port", "p32", NULL};
  const char port_test_str1[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"p32\",\n"
    "\"port-number\":0,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"i32\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":[],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *port_argv2[] = {"port", "p33", NULL};
  const char port_test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"p33\",\n"
    "\"port-number\":33,\n"
    "\"interface\":\""DATASTORE_NAMESPACE_DELIMITER"i33\",\n"
    "\"policer\":\"\",\n"
    "\"queues\":[],\n"
    "\"is-used\":true,\n"
    "\"is-enabled\":true}]}";

  TEST_CONTROLLER_CREATE(ret, &interp, state, &tbl, &ds, str, "cha32", "c32");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i32", "p32");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i33", "p33");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd (del port). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* config cmd (add port). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  /* port show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv1), port_argv1, &tbl, port_cmd_update,
                 &ds, str, port_test_str1);

  /* port show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, port_cmd_parse, &interp, state,
                 ARGV_SIZE(port_argv2), port_argv2, &tbl, port_cmd_update,
                 &ds, str, port_test_str2);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, bridge_cmd_update,
                 &ds, str, test_str7);

  TEST_CONTROLLER_DESTROY(ret, &interp, state, &tbl, &ds, str, "cha32", "c32");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i32", "p32");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i33", "p33");
}

void
test_bridge_cmd_parse_config_controller_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name36", "create",
                         "-controller", "c36",
                         "-port", "p36", "36",
                         "-dpid", "36",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name36", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", "test_name36", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name36\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c36\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p36\":36},\n"
    "\"dpid\":36,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":true}]}";
  const char *argv4[] = {"bridge", "test_name36", "config",
                         "-controller", "~c36",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"bridge", "test_name36", "config",
                         "-controller", "c36",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";
  const char *argv6[] = {"bridge", "test_name36", "disable",
                         NULL
                        };
  const char test_str6[] = "{\"ret\":\"OK\"}";
  const char *argv7[] = {"bridge", "test_name36", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state, &tbl, &ds, str, "cha36", "c36");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i36", "p36");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd (del port). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* config cmd (add port). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv7), argv7, &tbl, bridge_cmd_update,
                 &ds, str, test_str7);

  TEST_CONTROLLER_DESTROY(ret, &interp, state, &tbl, &ds, str, "cha36", "c36");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i36", "p36");
}

void
test_bridge_cmd_serialize_default_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* bridge create cmd str. */
  const char *argv1[] = {"bridge", "test_name37", "create",
                         "-dpid", "1", NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"bridge", "test_name37", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "bridge "
                                DATASTORE_NAMESPACE_DELIMITER"test_name37 create "
                                "-dpid 1 "
                                "-fail-mode secure "
                                "-flow-statistics true "
                                "-group-statistics true "
                                "-port-statistics true "
                                "-queue-statistics true "
                                "-table-statistics true "
                                "-reassemble-ip-fragments false "
                                "-max-buffered-packets 65535 "
                                "-max-ports 255 "
                                "-max-tables 255 "
                                "-max-flows 4294967295 "
                                "-packet-inq-size 1000 "
                                "-packet-inq-max-batches 1000 "
                                "-up-streamq-size 1000 "
                                "-up-streamq-max-batches 1000 "
                                "-down-streamq-size 1000 "
                                "-down-streamq-max-batches 1000 "
                                "-block-looping-ports false\n";

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name37", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 bridge_cmd_update, &ds, str, test_str2);
}

void
test_bridge_cmd_serialize_default_opt_escape(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* bridge create cmd str. */
  const char *argv1[] = {"bridge", "test_\"name38", "create",
                         "-dpid", "1", NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"bridge", "test_\"name38", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "bridge "
                                "\""DATASTORE_NAMESPACE_DELIMITER"test_\\\"name38\" create "
                                "-dpid 1 "
                                "-fail-mode secure "
                                "-flow-statistics true "
                                "-group-statistics true "
                                "-port-statistics true "
                                "-queue-statistics true "
                                "-table-statistics true "
                                "-reassemble-ip-fragments false "
                                "-max-buffered-packets 65535 "
                                "-max-ports 255 "
                                "-max-tables 255 "
                                "-max-flows 4294967295 "
                                "-packet-inq-size 1000 "
                                "-packet-inq-max-batches 1000 "
                                "-up-streamq-size 1000 "
                                "-up-streamq-max-batches 1000 "
                                "-down-streamq-size 1000 "
                                "-down-streamq-max-batches 1000 "
                                "-block-looping-ports false\n";

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_\"name38", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 bridge_cmd_update, &ds, str, test_str2);
}

void
test_bridge_cmd_serialize_default_opt_escape_white_space(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* bridge create cmd str. */
  const char *argv1[] = {"bridge", "test name39", "create",
                         "-dpid", "1", NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"bridge", "test name39", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "bridge "
                                "\""DATASTORE_NAMESPACE_DELIMITER"test name39\" create "
                                "-dpid 1 "
                                "-fail-mode secure "
                                "-flow-statistics true "
                                "-group-statistics true "
                                "-port-statistics true "
                                "-queue-statistics true "
                                "-table-statistics true "
                                "-reassemble-ip-fragments false "
                                "-max-buffered-packets 65535 "
                                "-max-ports 255 "
                                "-max-tables 255 "
                                "-max-flows 4294967295 "
                                "-packet-inq-size 1000 "
                                "-packet-inq-max-batches 1000 "
                                "-up-streamq-size 1000 "
                                "-up-streamq-max-batches 1000 "
                                "-down-streamq-size 1000 "
                                "-down-streamq-max-batches 1000 "
                                "-block-looping-ports false\n";

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test name39", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 bridge_cmd_update, &ds, str, test_str2);
}

void
test_bridge_cmd_serialize_all_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;

  /* bridge create cmd str. */
  const char *argv1[] = {"bridge", "test_name40", "create",
                         "-controller", "c40",
                         "-port", "p40", "40",
                         "-dpid", "4321",
                         "-flow-statistics", "true",
                         "-group-statistics", "true",
                         "-port-statistics", "true",
                         "-queue-statistics", "true",
                         "-table-statistics", "true",
                         "-reassemble-ip-fragments", "true",
                         "-max-buffered-packets", "33333",
                         "-max-ports", "128",
                         "-max-tables", "128",
                         "-max-flows", "128",
                         "-packet-inq-size", "128",
                         "-packet-inq-max-batches", "128",
                         "-up-streamq-size", "128",
                         "-up-streamq-max-batches", "128",
                         "-down-streamq-size", "128",
                         "-down-streamq-max-batches", "128",
                         "-block-looping-ports", "true",
                         "-action-type", "~copy-ttl-out",
                         "-action-type", "~copy-ttl-in",
                         "-instruction-type", "~apply-actions",
                         "-instruction-type", "~clear-actions",
                         "-reserved-port-type", "~all",
                         "-reserved-port-type", "~controller",
                         "-group-type", "~all",
                         "-group-type", "~select",
                         "-group-capability", "~select-weight",
                         "-group-capability", "~select-liveness",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";

  /* interface destroy cmd str. */
  const char *argv2[] = {"bridge", "test_name40", "destroy", NULL};
  const char test_str2[] = "{\"ret\":\"OK\"}";

  /* serialize result str. */
  const char serialize_str1[] = "bridge "
                                DATASTORE_NAMESPACE_DELIMITER"test_name40 create "
                                "-dpid 4321 "
                                "-controller "DATASTORE_NAMESPACE_DELIMITER"c40 "
                                "-port "DATASTORE_NAMESPACE_DELIMITER"p40 40 "
                                "-fail-mode secure "
                                "-flow-statistics true "
                                "-group-statistics true "
                                "-port-statistics true "
                                "-queue-statistics true "
                                "-table-statistics true "
                                "-reassemble-ip-fragments true "
                                "-max-buffered-packets 33333 "
                                "-max-ports 128 "
                                "-max-tables 128 "
                                "-max-flows 128 "
                                "-packet-inq-size 128 "
                                "-packet-inq-max-batches 128 "
                                "-up-streamq-size 128 "
                                "-up-streamq-max-batches 128 "
                                "-down-streamq-size 128 "
                                "-down-streamq-max-batches 128 "
                                "-block-looping-ports true "
                                "-action-type ~copy-ttl-out "
                                "-action-type ~copy-ttl-in "
                                "-instruction-type ~apply-actions "
                                "-instruction-type ~clear-actions "
                                "-reserved-port-type ~all "
                                "-reserved-port-type ~controller "
                                "-group-type ~all "
                                "-group-type ~select "
                                "-group-capability ~select-weight "
                                "-group-capability ~select-liveness\n";

  TEST_CONTROLLER_CREATE(ret, &interp, state, &tbl, &ds, str, "cha40", "c40");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i40", "p40");

  /* bridge create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* TEST : serialize. */
  TEST_CMD_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_serialize, &interp, state,
                &tbl,
                DATASTORE_NAMESPACE_DELIMITER"test_name40", conf, &ds, str, serialize_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                 bridge_cmd_update, &ds, str, test_str2);

  TEST_CONTROLLER_DESTROY(ret, &interp, state, &tbl, &ds, str, "cha40", "c40");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i40", "p40");
}

void
test_bridge_cmd_parse_create_not_max_flows_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name41", "create",
                        "-max-flows", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_max_flows_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name41", "create",
                         "-max-flows", "4294967296", NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"TOO_LONG\",\n"
    "\"data\":\"Can't add max-flows.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);
}

void
test_bridge_cmd_parse_create_bad_max_flows(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name42", "create",
                         "-max-flows", "hoge"
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";
  const char *argv2[] = {"bridge", "test_name42", "create",
                         "-max-flows", "-1", NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = -1.\"}";
  const char *argv3[] = {"bridge", "test_name42", "create",
                         "-max-flows", "0.5", NULL
                        };
  const char test_str3[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = 0.5.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3,
                 &tbl, bridge_cmd_update, &ds, str, test_str3);
}

void
test_bridge_cmd_parse_atomic_commit(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"bridge", "test_name43", "create",
                         "-controller", "c43",
                         "-port", "p43", "43",
                         "-dpid", "43",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name43", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"bridge", "test_name43", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name43\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c43\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p43\":43},\n"
    "\"dpid\":43,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"bridge", "test_name43", "config",
                         "-controller", "c43_2",
                         "-port", "p43_2", "143",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"bridge", "test_name43", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"bridge", "test_name43", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name43\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c43\",\""DATASTORE_NAMESPACE_DELIMITER"c43_2\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p43\":43,\n"
    "\""DATASTORE_NAMESPACE_DELIMITER"p43_2\":143},\n"
    "\"dpid\":43,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"bridge", "test_name43", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name43\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c43\",\""DATASTORE_NAMESPACE_DELIMITER"c43_2\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p43\":43,\n"
    "\""DATASTORE_NAMESPACE_DELIMITER"p43_2\":143},\n"
    "\"dpid\":43,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"bridge", "test_name43", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"bridge", "test_name43", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state4, &tbl, &ds, str, "cha43", "c43");
  TEST_CONTROLLER_CREATE(ret, &interp, state4, &tbl, &ds, str, "cha43_2",
                         "c43_2");
  TEST_PORT_CREATE(ret, &interp, state4, &tbl_port, &ds, str, "i43", "p43");
  TEST_PORT_CREATE(ret, &interp, state4, &tbl_port, &ds, str, "i43_2", "p43_2");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  /* port commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p43", conf, &ds);

  /* port commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p43_2", conf, &ds);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name43", conf, &ds);

  /* port commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p43", conf, &ds);

  /* port commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p43_2", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name43", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, bridge_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, bridge_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, bridge_cmd_update,
                 &ds, str, test_str9);

  TEST_CONTROLLER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "cha43", "c43");
  TEST_CONTROLLER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "cha43_2",
                          "c43_2");
  TEST_PORT_DESTROY(ret, &interp, state4, &tbl_port, &ds, str, "i43", "p43");
  TEST_PORT_DESTROY(ret, &interp, state4, &tbl_port, &ds, str, "i43_2", "p43_2");
}

void
test_bridge_cmd_parse_atomic_rollback(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ROLLBACKING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ROLLBACKED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"bridge", "test_name44", "create",
                         "-controller", "c44",
                         "-port", "p44", "44",
                         "-dpid", "44",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name44", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"bridge", "test_name44", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name44\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c44\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p44\":44},\n"
    "\"dpid\":44,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"bridge", "test_name44", "config",
                         "-controller", "c44_2",
                         "-port", "p44_2", "144",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"bridge", "test_name44", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"bridge", "test_name44", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name44\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c44\",\""DATASTORE_NAMESPACE_DELIMITER"c44_2\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p44\":44,\n"
    "\""DATASTORE_NAMESPACE_DELIMITER"p44_2\":144},\n"
    "\"dpid\":44,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"bridge", "test_name44", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = :test_name44\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state4, &tbl, &ds, str, "cha44", "c44");
  TEST_CONTROLLER_CREATE(ret, &interp, state4, &tbl, &ds, str, "cha44_2",
                         "c44_2");
  TEST_PORT_CREATE(ret, &interp, state4, &tbl_port, &ds, str, "i44", "p44");
  TEST_PORT_CREATE(ret, &interp, state4, &tbl_port, &ds, str, "i44_2", "p44_2");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  /* port rollbacking. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p44", conf, &ds);

  /* port rollbacking. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p44_2", conf, &ds);

  /* rollbacking. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name44", conf, &ds);

  /* port rollbacked. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p44", conf, &ds);

  /* port rollbacked. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p44_2", conf, &ds);

  /* rollbacked. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name44", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, bridge_cmd_update,
                 &ds, str, test_str7);

  TEST_CONTROLLER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "cha44", "c44");
  TEST_CONTROLLER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "cha44_2",
                          "c44_2");
  TEST_PORT_DESTROY(ret, &interp, state4, &tbl_port, &ds, str, "i44", "p44");
  TEST_PORT_DESTROY(ret, &interp, state4, &tbl_port, &ds, str, "i44_2", "p44_2");
}

void
test_bridge_cmd_parse_atomic_delay_enable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"bridge", "test_name45", "create",
                         "-controller", "c45",
                         "-port", "p45", "45",
                         "-dpid", "45",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name45", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", "test_name45", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv4[] = {"bridge", "test_name45", "modified", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name45\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c45\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p45\":45},\n"
    "\"dpid\":45,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"bridge", "test_name45", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"bridge", "test_name45", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name45\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c45\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p45\":45},\n"
    "\"dpid\":45,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":true}]}";
  const char *argv7[] = {"bridge", "test_name45", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name45\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c45\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p45\":45},\n"
    "\"dpid\":45,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":true}]}";
  const char *argv8[] = {"bridge", "test_name45", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"bridge", "test_name45", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state4, &tbl, &ds, str, "cha45", "c45");
  TEST_PORT_CREATE(ret, &interp, state4, &tbl_port, &ds, str, "i45", "p45");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* port commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p45", conf, &ds);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name45", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  /* port commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p45", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name45", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, bridge_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, bridge_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, bridge_cmd_update,
                 &ds, str, test_str9);

  TEST_CONTROLLER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "cha45", "c45");
  TEST_PORT_DESTROY(ret, &interp, state4, &tbl_port, &ds, str, "i45", "p45");
}

void
test_bridge_cmd_parse_atomic_delay_disable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"bridge", "test_name46", "create",
                         "-controller", "c46",
                         "-port", "p46", "46",
                         "-dpid", "46",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name46", "enable",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", "test_name46", "disable",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"bridge", "test_name46", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name46\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c46\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p46\":46},\n"
    "\"dpid\":46,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":true}]}";
  const char *argv5[] = {"bridge", "test_name46", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name46\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c46\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p46\":46},\n"
    "\"dpid\":46,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"bridge", "test_name46", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name46\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c46\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p46\":46},\n"
    "\"dpid\":46,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"bridge", "test_name46", "destroy",
                         NULL
                        };
  const char test_str7[] = "{\"ret\":\"OK\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state4, &tbl, &ds, str, "cha46", "c46");
  TEST_PORT_CREATE(ret, &interp, state4, &tbl_port, &ds, str, "i46", "p46");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* port commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p46", conf, &ds);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name46", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  /* port commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p46", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name46", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv7), argv7, &tbl, bridge_cmd_update,
                 &ds, str, test_str7);

  TEST_CONTROLLER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "cha46", "c46");
  TEST_PORT_DESTROY(ret, &interp, state4, &tbl_port, &ds, str, "i46", "p46");
}

void
test_bridge_cmd_parse_atomic_delay_destroy(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"bridge", "test_name47", "create",
                         "-controller", "c47",
                         "-port", "p47", "47",
                         "-dpid", "47",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name47", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", "test_name47", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name47\"}";
  const char *argv4[] = {"bridge", "test_name47", "enable",
                         NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name47\"}";
  const char *argv5[] = {"bridge", "test_name47", "disable",
                         NULL
                        };
  const char test_str5[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name47\"}";
  const char *argv6[] = {"bridge", "test_name47", "destroy",
                         NULL
                        };
  const char test_str6[] =
    "{\"ret\":\"INVALID_OBJECT\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name47\"}";
  const char *argv7[] = {"bridge", "test_name47", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name47\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state4, &tbl, &ds, str, "cha47", "c47");
  TEST_PORT_CREATE(ret, &interp, state4, &tbl_port, &ds, str, "i47", "p47");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* enable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* disable cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  /* port commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p47", conf, &ds);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name47", conf, &ds);

  /* port commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p47", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name47", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, bridge_cmd_update,
                 &ds, str, test_str7);

  TEST_CONTROLLER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "cha47", "c47");
  TEST_PORT_DESTROY(ret, &interp, state4, &tbl_port, &ds, str, "i47", "p47");
}

void
test_bridge_cmd_parse_atomic_abort_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"bridge", "test_name48", "create",
                         "-controller", "c48",
                         "-port", "p48", "48",
                         "-dpid", "48",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name48", NULL};
  const char test_str2[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv3[] = {"bridge", "test_name48", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name48\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c48\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p48\":48},\n"
    "\"dpid\":48,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv4[] = {"bridge", "test_name48", "config",
                         "-controller", "c48_2",
                         "-port", "p48_2", "148",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"bridge", "test_name48", NULL};
  const char test_str5[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv6[] = {"bridge", "test_name48", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name48\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c48\",\""DATASTORE_NAMESPACE_DELIMITER"c48_2\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p48\":48,\n"
    "\""DATASTORE_NAMESPACE_DELIMITER"p48_2\":148},\n"
    "\"dpid\":48,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"bridge", "test_name48", NULL};
  const char test_str7[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set current.\"}";
  const char *argv8[] = {"bridge", "test_name48", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name48\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c48\",\""DATASTORE_NAMESPACE_DELIMITER"c48_2\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p48\":48,\n"
    "\""DATASTORE_NAMESPACE_DELIMITER"p48_2\":148},\n"
    "\"dpid\":48,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"bridge", "test_name48", NULL};
  const char test_str9[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name48\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state4, &tbl, &ds, str, "cha48", "c48");
  TEST_CONTROLLER_CREATE(ret, &interp, state4, &tbl, &ds, str, "cha48_2",
                         "c48_2");
  TEST_PORT_CREATE(ret, &interp, state4, &tbl_port, &ds, str, "i48", "p48");
  TEST_PORT_CREATE(ret, &interp, state4, &tbl_port, &ds, str, "i48_2", "p48_2");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  /* port commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p48", conf, &ds);

  /* port commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p48_2", conf, &ds);

  /* aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name48", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, bridge_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, bridge_cmd_update,
                 &ds, str, test_str8);

  /* port commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p48", conf, &ds);

  /* port commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p48_2", conf, &ds);

  /* aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name48", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv9), argv9, &tbl, bridge_cmd_update,
                 &ds, str, test_str9);

  TEST_CONTROLLER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "cha48", "c48");
  TEST_CONTROLLER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "cha48_2",
                          "c48_2");
  TEST_PORT_DESTROY(ret, &interp, state4, &tbl_port, &ds, str, "i48", "p48");
  TEST_PORT_DESTROY(ret, &interp, state4, &tbl_port, &ds, str, "i48_2", "p48_2");

}

void
test_bridge_cmd_parse_atomic_abort_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_ABORTING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_ABORTED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"bridge", "test_name49", "create",
                         "-controller", "c49",
                         "-port", "p49", "49",
                         "-dpid", "49",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name49", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name49\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c49\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p49\":49},\n"
    "\"dpid\":49,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"bridge", "test_name49", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv4[] = {"bridge", "test_name49", "config",
                         "-controller", "c49_2",
                         "-port", "p49_2", "149",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"bridge", "test_name49", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name49\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c49\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p49\":49},\n"
    "\"dpid\":49,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"bridge", "test_name49", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name49\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c49\",\""DATASTORE_NAMESPACE_DELIMITER"c49_2\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p49\":49,\n"
    "\""DATASTORE_NAMESPACE_DELIMITER"p49_2\":149},\n"
    "\"dpid\":49,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"bridge", "test_name49", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name49\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c49\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p49\":49},\n"
    "\"dpid\":49,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"bridge", "test_name49", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name49\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c49\",\""DATASTORE_NAMESPACE_DELIMITER"c49_2\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p49\":49,\n"
    "\""DATASTORE_NAMESPACE_DELIMITER"p49_2\":149},\n"
    "\"dpid\":49,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv9[] = {"bridge", "test_name49", NULL};
  const char test_str9[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name49\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c49\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p49\":49},\n"
    "\"dpid\":49,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv10[] = {"bridge", "test_name49", "modified", NULL};
  const char test_str10[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv11[] = {"bridge", "test_name49", "destroy",
                          NULL
                         };
  const char test_str11[] = "{\"ret\":\"OK\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state4, &tbl, &ds, str, "cha49", "c49");
  TEST_CONTROLLER_CREATE(ret, &interp, state4, &tbl, &ds, str, "cha49_2",
                         "c49_2");
  TEST_PORT_CREATE(ret, &interp, state4, &tbl_port, &ds, str, "i49", "p49");
  TEST_PORT_CREATE(ret, &interp, state4, &tbl_port, &ds, str, "i49_2", "p49_2");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  /* port commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p49", conf, &ds);

  /* port commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p49_2", conf, &ds);

  /* aborting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name49", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv7), argv7, &tbl, bridge_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv8), argv8, &tbl, bridge_cmd_update,
                 &ds, str, test_str8);

  /* port commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p49", conf, &ds);

  /* port commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p49_2", conf, &ds);

  /* aborted. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name49", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv9), argv9, &tbl, bridge_cmd_update,
                 &ds, str, test_str9);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv10), argv10, &tbl, bridge_cmd_update,
                 &ds, str, test_str10);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv11), argv11, &tbl, bridge_cmd_update,
                 &ds, str, test_str11);

  TEST_CONTROLLER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "cha49", "c49");
  TEST_CONTROLLER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "cha49_2",
                          "c49_2");
  TEST_PORT_DESTROY(ret, &interp, state4, &tbl_port, &ds, str, "i49", "p49");
  TEST_PORT_DESTROY(ret, &interp, state4, &tbl_port, &ds, str, "i49_2", "p49_2");
}

void
test_bridge_cmd_parse_atomic_destroy_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_ATOMIC;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_COMMITING;
  datastore_interp_state_t state3 = DATASTORE_INTERP_STATE_COMMITED;
  datastore_interp_state_t state4 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  void *conf = NULL;
  const char *argv1[] = {"bridge", "test_name50", "create",
                         "-controller", "c50",
                         "-port", "p50", "50",
                         "-dpid", "50",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name50", "destroy",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", "test_name50", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_FOUND\",\n"
    "\"data\":\"name = "DATASTORE_NAMESPACE_DELIMITER"test_name50\"}";
  const char *argv4[] = {"bridge", "test_name50", "create",
                         "-controller", "c50_2",
                         "-port", "p50_2", "150",
                         "-dpid", "150",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"bridge", "test_name50", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name50\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c50\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p50\":50},\n"
    "\"dpid\":50,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"bridge", "test_name50", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name50\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c50\",\""DATASTORE_NAMESPACE_DELIMITER"c50_2\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p50\":50,\n"
    "\""DATASTORE_NAMESPACE_DELIMITER"p50_2\":150},\n"
    "\"dpid\":150,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv7[] = {"bridge", "test_name50", NULL};
  const char test_str7[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name50\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c50\",\""DATASTORE_NAMESPACE_DELIMITER"c50_2\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p50\":50,\n"
    "\""DATASTORE_NAMESPACE_DELIMITER"p50_2\":150},\n"
    "\"dpid\":150,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv8[] = {"bridge", "test_name50", "modified", NULL};
  const char test_str8[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv9[] = {"bridge", "test_name50", "destroy",
                         NULL
                        };
  const char test_str9[] = "{\"ret\":\"OK\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state4, &tbl, &ds, str, "cha50", "c50");
  TEST_CONTROLLER_CREATE(ret, &interp, state4, &tbl, &ds, str, "cha50_2",
                         "c50_2");
  TEST_PORT_CREATE(ret, &interp, state4, &tbl_port, &ds, str, "i50", "p50");
  TEST_PORT_CREATE(ret, &interp, state4, &tbl_port, &ds, str, "i50_2", "p50_2");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  /* port commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p50", conf, &ds);

  /* port commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state2, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p50_2", conf, &ds);

  /* commiting. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state2, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name50", conf, &ds);

  /* port commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p50", conf, &ds);

  /* port commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, port_cmd_update, &interp, state3, &tbl_port,
            DATASTORE_NAMESPACE_DELIMITER"p50_2", conf, &ds);

  /* commited. */
  TEST_PROC(ret, LAGOPUS_RESULT_OK, bridge_cmd_update, &interp, state3, &tbl,
            DATASTORE_NAMESPACE_DELIMITER"test_name50", conf, &ds);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv7), argv7, &tbl, bridge_cmd_update,
                 &ds, str, test_str7);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv8), argv8, &tbl, bridge_cmd_update,
                 &ds, str, test_str8);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state4,
                 ARGV_SIZE(argv9), argv9, &tbl, bridge_cmd_update,
                 &ds, str, test_str9);

  TEST_CONTROLLER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "cha50", "c50");
  TEST_CONTROLLER_DESTROY(ret, &interp, state4, &tbl, &ds, str, "cha50_2",
                          "c50_2");
  TEST_PORT_DESTROY(ret, &interp, state4, &tbl_port, &ds, str, "i50", "p50");
  TEST_PORT_DESTROY(ret, &interp, state4, &tbl_port, &ds, str, "i50_2", "p50_2");
}

void
test_bridge_cmd_parse_dpid_is_already_exists(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name51", "create",
                         "-dpid", "5",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name51_2", "create",
                         "-dpid", "5",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"ALREADY_EXISTS\",\n"
    "\"data\":\"Can't add dpid.\"}";
  const char *argv3[] = {"bridge", "test_name51", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);
}

void
test_bridge_cmd_parse_port_no_already_exists(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name52", "create",
                         "-port", "p52", "52",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name52", "config",
                         "-port", "p52_2", "152",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", "test_name52", "config",
                         "-port", "p52_3", "52",
                         NULL
                        };
  const char test_str3[] =
    "{\"ret\":\"ALREADY_EXISTS\",\n"
    "\"data\":\"port name = :p52_3, port number = 52.\"}";
  const char *argv4[] = {"bridge", "test_name52", "config",
                         "-port", "p52_4", "152",
                         NULL
                        };
  const char test_str4[] =
    "{\"ret\":\"ALREADY_EXISTS\",\n"
    "\"data\":\"port name = :p52_4, port number = 152.\"}";
  const char *argv5[] = {"bridge", "test_name52", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";

  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i52", "p52");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i52_2", "p52_2");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i52_3", "p52_3");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i52_4", "p52_4");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret,  LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret,  LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i52", "p52");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i52_2", "p52_2");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i52_3", "p52_3");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i52_4", "p52_4");
}

void
test_bridge_cmd_parse_create_action_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name53", "create",
                         "-dpid", "53",
                         "-action-type", "~copy-ttl-out",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name53", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name53\",\n"
    "\"controllers\":[],\n"
    "\"ports\":{},\n"
    "\"dpid\":53,\n"
    "\"fail-mode\":\"secure\",\n"
    "\"flow-statistics\":true,\n"
    "\"group-statistics\":true,\n"
    "\"port-statistics\":true,\n"
    "\"queue-statistics\":true,\n"
    "\"table-statistics\":true,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":255,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":1000,\n"
    "\"packet-inq-max-batches\":1000,\n"
    "\"up-streamq-size\":1000,\n"
    "\"up-streamq-max-batches\":1000,\n"
    "\"down-streamq-size\":1000,\n"
    "\"down-streamq-max-batches\":1000,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"bridge", "test_name53", "config",
                         "-action-type", "copy-ttl-out",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"bridge", "test_name53", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name53\",\n"
    "\"controllers\":[],\n"
    "\"ports\":{},\n"
    "\"dpid\":53,\n"
    "\"fail-mode\":\"secure\",\n"
    "\"flow-statistics\":true,\n"
    "\"group-statistics\":true,\n"
    "\"port-statistics\":true,\n"
    "\"queue-statistics\":true,\n"
    "\"table-statistics\":true,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":255,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":1000,\n"
    "\"packet-inq-max-batches\":1000,\n"
    "\"up-streamq-size\":1000,\n"
    "\"up-streamq-max-batches\":1000,\n"
    "\"down-streamq-size\":1000,\n"
    "\"down-streamq-max-batches\":1000,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"bridge", "test_name53", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);
}

void
test_bridge_cmd_parse_create_bad_action_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name54", "create",
                         "-dpid", "54",
                         "-action-type", "hoge",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);
}

void
test_bridge_cmd_parse_create_instruction_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name55", "create",
                         "-dpid", "55",
                         "-instruction-type", "~apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name55", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name55\",\n"
    "\"controllers\":[],\n"
    "\"ports\":{},\n"
    "\"dpid\":55,\n"
    "\"fail-mode\":\"secure\",\n"
    "\"flow-statistics\":true,\n"
    "\"group-statistics\":true,\n"
    "\"port-statistics\":true,\n"
    "\"queue-statistics\":true,\n"
    "\"table-statistics\":true,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":255,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":1000,\n"
    "\"packet-inq-max-batches\":1000,\n"
    "\"up-streamq-size\":1000,\n"
    "\"up-streamq-max-batches\":1000,\n"
    "\"down-streamq-size\":1000,\n"
    "\"down-streamq-max-batches\":1000,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"bridge", "test_name55", "config",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"bridge", "test_name55", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name55\",\n"
    "\"controllers\":[],\n"
    "\"ports\":{},\n"
    "\"dpid\":55,\n"
    "\"fail-mode\":\"secure\",\n"
    "\"flow-statistics\":true,\n"
    "\"group-statistics\":true,\n"
    "\"port-statistics\":true,\n"
    "\"queue-statistics\":true,\n"
    "\"table-statistics\":true,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":255,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":1000,\n"
    "\"packet-inq-max-batches\":1000,\n"
    "\"up-streamq-size\":1000,\n"
    "\"up-streamq-max-batches\":1000,\n"
    "\"down-streamq-size\":1000,\n"
    "\"down-streamq-max-batches\":1000,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"bridge", "test_name55", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);
}

void
test_bridge_cmd_parse_create_bad_instruction_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name56", "create",
                         "-dpid", "56",
                         "-instruction-type", "hoge",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);
}

void
test_bridge_cmd_parse_create_reserved_port_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name57", "create",
                         "-dpid", "57",
                         "-reserved-port-type", "~all",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name57", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name57\",\n"
    "\"controllers\":[],\n"
    "\"ports\":{},\n"
    "\"dpid\":57,\n"
    "\"fail-mode\":\"secure\",\n"
    "\"flow-statistics\":true,\n"
    "\"group-statistics\":true,\n"
    "\"port-statistics\":true,\n"
    "\"queue-statistics\":true,\n"
    "\"table-statistics\":true,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":255,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":1000,\n"
    "\"packet-inq-max-batches\":1000,\n"
    "\"up-streamq-size\":1000,\n"
    "\"up-streamq-max-batches\":1000,\n"
    "\"down-streamq-size\":1000,\n"
    "\"down-streamq-max-batches\":1000,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"bridge", "test_name57", "config",
                         "-reserved-port-type", "all",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"bridge", "test_name57", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name57\",\n"
    "\"controllers\":[],\n"
    "\"ports\":{},\n"
    "\"dpid\":57,\n"
    "\"fail-mode\":\"secure\",\n"
    "\"flow-statistics\":true,\n"
    "\"group-statistics\":true,\n"
    "\"port-statistics\":true,\n"
    "\"queue-statistics\":true,\n"
    "\"table-statistics\":true,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":255,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":1000,\n"
    "\"packet-inq-max-batches\":1000,\n"
    "\"up-streamq-size\":1000,\n"
    "\"up-streamq-max-batches\":1000,\n"
    "\"down-streamq-size\":1000,\n"
    "\"down-streamq-max-batches\":1000,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"bridge", "test_name57", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);
}

void
test_bridge_cmd_parse_create_bad_reserved_port_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name58", "create",
                         "-dpid", "58",
                         "-reserved-port-type", "hoge",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);
}

void
test_bridge_cmd_parse_create_group_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name59", "create",
                         "-dpid", "59",
                         "-group-type", "~all",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name59", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name59\",\n"
    "\"controllers\":[],\n"
    "\"ports\":{},\n"
    "\"dpid\":59,\n"
    "\"fail-mode\":\"secure\",\n"
    "\"flow-statistics\":true,\n"
    "\"group-statistics\":true,\n"
    "\"port-statistics\":true,\n"
    "\"queue-statistics\":true,\n"
    "\"table-statistics\":true,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":255,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":1000,\n"
    "\"packet-inq-max-batches\":1000,\n"
    "\"up-streamq-size\":1000,\n"
    "\"up-streamq-max-batches\":1000,\n"
    "\"down-streamq-size\":1000,\n"
    "\"down-streamq-max-batches\":1000,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"bridge", "test_name59", "config",
                         "-group-type", "all",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"bridge", "test_name59", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name59\",\n"
    "\"controllers\":[],\n"
    "\"ports\":{},\n"
    "\"dpid\":59,\n"
    "\"fail-mode\":\"secure\",\n"
    "\"flow-statistics\":true,\n"
    "\"group-statistics\":true,\n"
    "\"port-statistics\":true,\n"
    "\"queue-statistics\":true,\n"
    "\"table-statistics\":true,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":255,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":1000,\n"
    "\"packet-inq-max-batches\":1000,\n"
    "\"up-streamq-size\":1000,\n"
    "\"up-streamq-max-batches\":1000,\n"
    "\"down-streamq-size\":1000,\n"
    "\"down-streamq-max-batches\":1000,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"bridge", "test_name59", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);
}

void
test_bridge_cmd_parse_create_bad_group_type(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name60", "create",
                         "-dpid", "60",
                         "-group-type", "hoge",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);
}

void
test_bridge_cmd_parse_create_group_capability(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name61", "create",
                         "-dpid", "61",
                         "-group-capability", "~select-weight",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name61", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name61\",\n"
    "\"controllers\":[],\n"
    "\"ports\":{},\n"
    "\"dpid\":61,\n"
    "\"fail-mode\":\"secure\",\n"
    "\"flow-statistics\":true,\n"
    "\"group-statistics\":true,\n"
    "\"port-statistics\":true,\n"
    "\"queue-statistics\":true,\n"
    "\"table-statistics\":true,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":255,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":1000,\n"
    "\"packet-inq-max-batches\":1000,\n"
    "\"up-streamq-size\":1000,\n"
    "\"up-streamq-max-batches\":1000,\n"
    "\"down-streamq-size\":1000,\n"
    "\"down-streamq-max-batches\":1000,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-liveness\","
    "\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"bridge", "test_name61", "config",
                         "-group-capability", "select-weight",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char *argv4[] = {"bridge", "test_name61", NULL};
  const char test_str4[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name61\",\n"
    "\"controllers\":[],\n"
    "\"ports\":{},\n"
    "\"dpid\":61,\n"
    "\"fail-mode\":\"secure\",\n"
    "\"flow-statistics\":true,\n"
    "\"group-statistics\":true,\n"
    "\"port-statistics\":true,\n"
    "\"queue-statistics\":true,\n"
    "\"table-statistics\":true,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":255,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":1000,\n"
    "\"packet-inq-max-batches\":1000,\n"
    "\"up-streamq-size\":1000,\n"
    "\"up-streamq-max-batches\":1000,\n"
    "\"down-streamq-size\":1000,\n"
    "\"down-streamq-max-batches\":1000,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv5[] = {"bridge", "test_name61", "destroy",
                         NULL
                        };
  const char test_str5[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* show cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);
}

void
test_bridge_cmd_parse_create_bad_group_capability(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name62", "create",
                         "-dpid", "63",
                         "-group-capability", "hoge",
                         NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);
}

void
test_bridge_cmd_parse_stats_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name63", "create",
                         "-controller", "c63",
                         "-port", "p63", "63",
                         "-dpid", "63",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name63", "stats",
                         NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name63\",\n"
    "\"packet-inq-entries\":0,\n"
    "\"up-streamq-entries\":1,\n"
    "\"down-streamq-entries\":0,\n"
    "\"flowcache-entries\":0,\n"
    "\"flowcache-hit\":0,\n"
    "\"flowcache-miss\":0,\n"
    "\"flow-entries\":0,\n"
    "\"flow-lookup-count\":0,\n"
    "\"flow-matched-count\":0,\n"
    "\"tables\":[{\"table-id\":0,\n"
    "\"flow-entries\":0,\n"
    "\"flow-lookup-count\":0,\n"
    "\"flow-matched-count\":0}]}]}";
  const char *argv3[] = {"bridge", "test_name63", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state, &tbl, &ds, str, "cha63", "c63");
  TEST_PORT_CREATE(ret, &interp, state, &tbl, &ds, str, "i63", "p63");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* stats cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  TEST_CONTROLLER_DESTROY(ret, &interp, state, &tbl, &ds, str, "cha63", "c63");
  TEST_PORT_DESTROY(ret, &interp, state, &tbl, &ds, str, "i63", "p63");
}

void
test_bridge_cmd_parse_create_not_packet_inq_size_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name66", "create",
                        "-packet-inq-size", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_packet_inq_size_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name67", "create",
                         "-packet-inq-size", "4294967296", NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"TOO_LONG\",\n"
    "\"data\":\"Can't add packet-inq-size.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);
}

void
test_bridge_cmd_parse_create_bad_packet_inq_size(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name68", "create",
                         "-packet-inq-size", "hoge"
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";
  const char *argv2[] = {"bridge", "test_name68", "create",
                         "-packet-inq-size", "-1", NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = -1.\"}";
  const char *argv3[] = {"bridge", "test_name68", "create",
                         "-packet-inq-size", "0.5", NULL
                        };
  const char test_str3[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = 0.5.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3,
                 &tbl, bridge_cmd_update, &ds, str, test_str3);
}

void
test_bridge_cmd_parse_create_not_packet_inq_max_batches_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name69", "create",
                        "-packet-inq-max-batches", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_packet_inq_max_batches_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name70", "create",
                         "-packet-inq-max-batches", "4294967296", NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"TOO_LONG\",\n"
    "\"data\":\"Can't add packet-inq-max-batches.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);
}

void
test_bridge_cmd_parse_create_bad_packet_inq_max_batches(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name71", "create",
                         "-packet-inq-max-batches", "hoge"
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";
  const char *argv2[] = {"bridge", "test_name71", "create",
                         "-packet-inq-max-batches", "-1", NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = -1.\"}";
  const char *argv3[] = {"bridge", "test_name71", "create",
                         "-packet-inq-max-batches", "0.5", NULL
                        };
  const char test_str3[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = 0.5.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3,
                 &tbl, bridge_cmd_update, &ds, str, test_str3);
}

void
test_bridge_cmd_parse_create_not_up_streamq_size_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name72", "create",
                        "-up-streamq-size", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_up_streamq_size_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name73", "create",
                         "-up-streamq-size", "4294967296", NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"TOO_LONG\",\n"
    "\"data\":\"Can't add up-streamq-size.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);
}

void
test_bridge_cmd_parse_create_bad_up_streamq_size(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name74", "create",
                         "-up-streamq-size", "hoge"
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";
  const char *argv2[] = {"bridge", "test_name74", "create",
                         "-up-streamq-size", "-1", NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = -1.\"}";
  const char *argv3[] = {"bridge", "test_name74", "create",
                         "-up-streamq-size", "0.5", NULL
                        };
  const char test_str3[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = 0.5.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3,
                 &tbl, bridge_cmd_update, &ds, str, test_str3);
}

void
test_bridge_cmd_parse_create_not_up_streamq_max_batches_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name75", "create",
                        "-up-streamq-max-batches", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_up_streamq_max_batches_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name76", "create",
                         "-up-streamq-max-batches", "4294967296", NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"TOO_LONG\",\n"
    "\"data\":\"Can't add up-streamq-max-batches.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);
}

void
test_bridge_cmd_parse_create_bad_up_streamq_max_batches(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name77", "create",
                         "-up-streamq-max-batches", "hoge"
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";
  const char *argv2[] = {"bridge", "test_name77", "create",
                         "-up-streamq-max-batches", "-1", NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = -1.\"}";
  const char *argv3[] = {"bridge", "test_name77", "create",
                         "-up-streamq-max-batches", "0.5", NULL
                        };
  const char test_str3[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = 0.5.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3,
                 &tbl, bridge_cmd_update, &ds, str, test_str3);
}

void
test_bridge_cmd_parse_create_not_down_streamq_size_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name78", "create",
                        "-down-streamq-size", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_down_streamq_size_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name79", "create",
                         "-down-streamq-size", "4294967296", NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"TOO_LONG\",\n"
    "\"data\":\"Can't add down-streamq-size.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);
}

void
test_bridge_cmd_parse_create_bad_down_streamq_size(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name80", "create",
                         "-down-streamq-size", "hoge"
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";
  const char *argv2[] = {"bridge", "test_name80", "create",
                         "-down-streamq-size", "-1", NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = -1.\"}";
  const char *argv3[] = {"bridge", "test_name80", "create",
                         "-down-streamq-size", "0.5", NULL
                        };
  const char test_str3[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = 0.5.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3,
                 &tbl, bridge_cmd_update, &ds, str, test_str3);
}

void
test_bridge_cmd_parse_create_not_down_streamq_max_batches_val(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv[] = {"bridge", "test_name81", "create",
                        "-down-streamq-max-batches", NULL
                       };
  const char test_str[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv), argv,
                 &tbl, bridge_cmd_update, &ds, str, test_str);
}

void
test_bridge_cmd_parse_create_down_streamq_max_batches_over(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name82", "create",
                         "-down-streamq-max-batches", "4294967296", NULL
                        };
  const char test_str1[] =
    "{\"ret\":\"TOO_LONG\",\n"
    "\"data\":\"Can't add down-streamq-max-batches.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);
}

void
test_bridge_cmd_parse_create_bad_down_streamq_max_batches(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name83", "create",
                         "-down-streamq-max-batches", "hoge"
                        };
  const char test_str1[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = hoge.\"}";
  const char *argv2[] = {"bridge", "test_name83", "create",
                         "-down-streamq-max-batches", "-1", NULL
                        };
  const char test_str2[] =
    "{\"ret\":\"OUT_OF_RANGE\",\n"
    "\"data\":\"Bad opt value = -1.\"}";
  const char *argv3[] = {"bridge", "test_name83", "create",
                         "-down-streamq-max-batches", "0.5", NULL
                        };
  const char test_str3[] =
    "{\"ret\":\"INVALID_ARGS\",\n"
    "\"data\":\"Bad opt value = 0.5.\"}";

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);

  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3,
                 &tbl, bridge_cmd_update, &ds, str, test_str3);
}

void
test_bridge_cmd_parse_dryrun(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state1 = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  datastore_interp_state_t state2 = DATASTORE_INTERP_STATE_DRYRUN;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name84", "create",
                         "-controller", "c84",
                         "-port", "p84", "84",
                         "-dpid", "84",
                         "-fail-mode", "standalone",
                         "-flow-statistics", "false",
                         "-group-statistics", "false",
                         "-port-statistics", "false",
                         "-queue-statistics", "false",
                         "-table-statistics", "false",
                         "-reassemble-ip-fragments", "false",
                         "-max-buffered-packets", "65535",
                         "-max-ports", "2048",
                         "-max-tables", "255",
                         "-max-flows", "4294967295",
                         "-packet-inq-size", "65535",
                         "-packet-inq-max-batches", "65535",
                         "-up-streamq-size", "65535",
                         "-up-streamq-max-batches", "65535",
                         "-down-streamq-size", "65535",
                         "-down-streamq-max-batches", "65535",
                         "-block-looping-ports", "false",
                         "-action-type", "copy-ttl-out",
                         "-instruction-type", "apply-actions",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name84", "current", NULL};
  const char test_str2[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name84\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c84\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p84\":84},\n"
    "\"dpid\":84,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv3[] = {"bridge", "test_name84", "modified", NULL};
  const char test_str3[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";
  const char *argv4[] = {"bridge", "test_name84", "config",
                         "-controller", "c84_2",
                         "-port", "p84_2", "184",
                         NULL
                        };
  const char test_str4[] = "{\"ret\":\"OK\"}";
  const char *argv5[] = {"bridge", "test_name84", "current", NULL};
  const char test_str5[] =
    "{\"ret\":\"OK\",\n"
    "\"data\":[{\"name\":\""DATASTORE_NAMESPACE_DELIMITER"test_name84\",\n"
    "\"controllers\":[\""DATASTORE_NAMESPACE_DELIMITER"c84\","
                     "\""DATASTORE_NAMESPACE_DELIMITER"c84_2\"],\n"
    "\"ports\":{\""DATASTORE_NAMESPACE_DELIMITER"p84\":84,\n"
               "\""DATASTORE_NAMESPACE_DELIMITER"p84_2\":0},\n"
    "\"dpid\":84,\n"
    "\"fail-mode\":\"standalone\",\n"
    "\"flow-statistics\":false,\n"
    "\"group-statistics\":false,\n"
    "\"port-statistics\":false,\n"
    "\"queue-statistics\":false,\n"
    "\"table-statistics\":false,\n"
    "\"reassemble-ip-fragments\":false,\n"
    "\"max-buffered-packets\":65535,\n"
    "\"max-ports\":2048,\n"
    "\"max-tables\":255,\n"
    "\"max-flows\":4294967295,\n"
    "\"packet-inq-size\":65535,\n"
    "\"packet-inq-max-batches\":65535,\n"
    "\"up-streamq-size\":65535,\n"
    "\"up-streamq-max-batches\":65535,\n"
    "\"down-streamq-size\":65535,\n"
    "\"down-streamq-max-batches\":65535,\n"
    "\"block-looping-ports\":false,\n"
    "\"action-types\":[\"copy-ttl-out\",\"copy-ttl-in\","
    "\"set-mpls-ttl\",\"dec-mpls-ttl\",\"push-vlan\",\"pop-vlan\","
    "\"push-mpls\",\"pop-mpls\",\"set-queue\",\"group\",\"set-nw-ttl\","
    "\"dec-nw-ttl\",\"set-field\"],\n"
    "\"instruction-types\":[\"apply-actions\",\"clear-actions\","
    "\"write-actions\",\"write-metadata\",\"goto-table\"],\n"
    "\"reserved-port-types\":[\"all\",\"controller\",\"table\","
    "\"inport\",\"any\",\"normal\",\"flood\"],\n"
    "\"group-types\":[\"all\",\"select\",\"indirect\",\"fast-failover\"],\n"
    "\"group-capabilities\":[\"select-weight\","
    "\"select-liveness\",\"chaining\",\"chaining-check\"],\n"
    "\"is-used\":false,\n"
    "\"is-enabled\":false}]}";
  const char *argv6[] = {"bridge", "test_name84", "modified", NULL};
  const char test_str6[] =
    "{\"ret\":\"NOT_OPERATIONAL\",\n"
    "\"data\":\"Not set modified.\"}";

  TEST_CONTROLLER_CREATE(ret, &interp, state1, &tbl, &ds, str, "cha84", "c84");
  TEST_CONTROLLER_CREATE(ret, &interp, state1, &tbl, &ds, str, "cha84_2",
                         "c84_2");
  TEST_PORT_CREATE(ret, &interp, state1, &tbl_port, &ds, str, "i84", "p84");
  TEST_PORT_CREATE(ret, &interp, state1, &tbl_port, &ds, str, "i84_2", "p84_2");

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv1), argv1, &tbl, bridge_cmd_update,
                 &ds, str, test_str1);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv2), argv2, &tbl, bridge_cmd_update,
                 &ds, str, test_str2);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state1,
                 ARGV_SIZE(argv3), argv3, &tbl, bridge_cmd_update,
                 &ds, str, test_str3);

  /* config cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv4), argv4, &tbl, bridge_cmd_update,
                 &ds, str, test_str4);

  /* show cmd (current). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv5), argv5, &tbl, bridge_cmd_update,
                 &ds, str, test_str5);

  /* show cmd (modified). */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse, &interp, state2,
                 ARGV_SIZE(argv6), argv6, &tbl, bridge_cmd_update,
                 &ds, str, test_str6);

  TEST_BRIDGE_DESTROY(ret, &interp, state1, &tbl, &ds, str, "test_name84",
                      "cha84", "c84", "i84", "p84")

  TEST_CONTROLLER_DESTROY(ret, &interp, state1, &tbl, &ds, str, "cha84_2",
                          "c84_2");
  TEST_PORT_DESTROY(ret, &interp, state1, &tbl_port, &ds, str, "i84_2", "p84_2");
}

void
test_bridge_cmd_parse_create_clear_queue_packet_inq(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name85", "create",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name85", "clear-queue",
                         "-packet-inq",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", "test_name85", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  /* clear-queue cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3,
                 &tbl, bridge_cmd_update, &ds, str, test_str3);
}

void
test_bridge_cmd_parse_create_clear_queue_up_streamq(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name86", "create",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name86", "clear-queue",
                         "-up-streamq",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", "test_name86", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  /* clear-queue cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3,
                 &tbl, bridge_cmd_update, &ds, str, test_str3);
}

void
test_bridge_cmd_parse_create_clear_queue_down_streamq(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name87", "create",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name87", "clear-queue",
                         "-down-streamq",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", "test_name87", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  /* clear-queue cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3,
                 &tbl, bridge_cmd_update, &ds, str, test_str3);
}

void
test_bridge_cmd_parse_create_clear_queue_all(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name88", "create",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name88", "clear-queue",
                         NULL
                        };
  const char test_str2[] = "{\"ret\":\"OK\"}";
  const char *argv3[] = {"bridge", "test_name88", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  /* clear-queue cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3,
                 &tbl, bridge_cmd_update, &ds, str, test_str3);
}

void
test_bridge_cmd_parse_create_clear_queue_bad_opt(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str = NULL;
  const char *argv1[] = {"bridge", "test_name89", "create",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char *argv2[] = {"bridge", "test_name89", "clear-queue",
                         "-hoge",
                         NULL
                        };
  const char test_str2[] =
      "{\"ret\":\"INVALID_ARGS\",\n"
      "\"data\":\"opt = -hoge.\"}";
  const char *argv3[] = {"bridge", "test_name89", "destroy",
                         NULL
                        };
  const char test_str3[] = "{\"ret\":\"OK\"}";

  /* create cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv1), argv1,
                 &tbl, bridge_cmd_update, &ds, str, test_str1);

  /* clear-queue cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_DATASTORE_INTERP_ERROR,
                 bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv2), argv2,
                 &tbl, bridge_cmd_update, &ds, str, test_str2);

  /* destroy cmd. */
  TEST_CMD_PARSE(ret, LAGOPUS_RESULT_OK, bridge_cmd_parse,
                 &interp, state, ARGV_SIZE(argv3), argv3,
                 &tbl, bridge_cmd_update, &ds, str, test_str3);
}

void
test_destroy(void) {
  destroy = true;
}
