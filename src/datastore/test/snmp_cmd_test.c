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
#include "../cmd_common.h"
#include "../snmp_cmd.c"


static lagopus_dstring_t result = NULL;
static lagopus_hashmap_t tbl = NULL;
static datastore_interp_t interp = NULL;
static datastore_update_proc_t proc = NULL;
static bool destroy = false;

void
setUp(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* create interp. */
  INTERP_CREATE(ret, NULL, interp, tbl, result);

  snmp_initialize();
}

void
tearDown(void) {
  snmp_finalize();

  /* destroy interp. */
  INTERP_DESTROY(NULL, interp, tbl, result, destroy);
}

static lagopus_result_t
snmp_json_result_str(const char *option, const char *option_val,
                     char **dst_str) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  if (option != NULL && option_val != NULL) {
    lagopus_dstring_t ds;
    ret = lagopus_dstring_create(&ds);
    if (ret == LAGOPUS_RESULT_OK) {
      char *esc_str = NULL;
      ret = datastore_json_string_escape(option_val, (char **)&esc_str);
      if (ret == LAGOPUS_RESULT_OK) {
        ret = lagopus_dstring_appendf(&ds,
                                      "{\"ret\":\"OK\",\n\"data\":[{"
                                      "\"%s\":\"%s\""
                                      "}]}",
                                      option,
                                      esc_str);
        if (ret == LAGOPUS_RESULT_OK) {
          char *str = NULL;
          ret = lagopus_dstring_str_get(&ds, &str);
          if (ret == LAGOPUS_RESULT_OK && str != NULL) {
            if (*dst_str != NULL) {
              free((void *)*dst_str);
            }
            *dst_str = str;
            lagopus_dstring_destroy(&ds);
            free((void *)esc_str);
            return LAGOPUS_RESULT_OK;
          }
          free((void *)str);
        }
      }
      free((void *)esc_str);
    }
    lagopus_dstring_destroy(&ds);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "test_escaped_result error.");
  return ret;
}

void
test_snmp_json_result_str(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *option1 = "-master-agentx-socket";
  char *esc_str1 = NULL;
  char expected_str1[PATH_MAX];
  char *actual_str1 = NULL;


  // Normal case
  {
    ret = snmp_json_result_str(option1, DEFAULT_SNMPMGR_AGENTX_SOCKET,
                               &actual_str1);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "snmp_json_result_str error.");
    ret = datastore_json_string_escape(DEFAULT_SNMPMGR_AGENTX_SOCKET,
                                       (char **)&esc_str1);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "datastore_json_string_escape error.");
    snprintf(expected_str1, PATH_MAX,
             "{\"ret\":\"OK\",\n\"data\":[{"
             "\"%s\":\"%s\""
             "}]}",
             option1, esc_str1);
    TEST_ASSERT_EQUAL_STRING(expected_str1, actual_str1);
  }

  free((void *)esc_str1);
  free((void *)actual_str1);
}

static lagopus_result_t
snmp_json_result_uint16_t(const char *option, const uint16_t option_val,
                          char **dst_str) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  if (option != NULL && option_val > 0) {
    lagopus_dstring_t ds;
    ret = lagopus_dstring_create(&ds);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = lagopus_dstring_appendf(&ds,
                                    "{\"ret\":\"OK\",\n\"data\":[{"
                                    "\"%s\":%d"
                                    "}]}",
                                    option,
                                    option_val);
      if (ret == LAGOPUS_RESULT_OK) {
        char *str = NULL;
        ret = lagopus_dstring_str_get(&ds, &str);
        if (ret == LAGOPUS_RESULT_OK && str != NULL) {
          if (*dst_str != NULL) {
            free((void *)*dst_str);
          }
          *dst_str = str;
          lagopus_dstring_destroy(&ds);
          return LAGOPUS_RESULT_OK;
        }
        free((void *)str);
      }
    }
    lagopus_dstring_destroy(&ds);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "test_escaped_result error.");
  return ret;
}

void
test_snmp_json_result_uint16_t(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *option1 = "-ping-interval-second";
  char *esc_str1 = NULL;
  char expected_str1[PATH_MAX];
  char *actual_str1 = NULL;


  // Normal case
  {
    ret = snmp_json_result_uint16_t(option1,
                                    DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC,
                                    &actual_str1);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "snmp_json_result_str error.");
    snprintf(expected_str1, PATH_MAX,
             "{\"ret\":\"OK\",\n\"data\":[{"
             "\"%s\":%d"
             "}]}",
             option1, DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC);
    TEST_ASSERT_EQUAL_STRING(expected_str1, actual_str1);
  }

  free((void *)esc_str1);
  free((void *)actual_str1);
}

static lagopus_result_t
snmp_json_result_log(const char *agentx_sock,
                     uint16_t ping_interval,
                     char **ret_str ) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *esc_sock = NULL;
  bool enable = false;

  if (ret_str == NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if ((ret = lagopus_snmp_get_enable(&enable))
      != LAGOPUS_RESULT_OK) {
    return ret;
  }

  ret = datastore_json_string_escape(agentx_sock,
                                     (char **)&esc_sock);

  if (ret == LAGOPUS_RESULT_OK &&
      IS_VALID_STRING(esc_sock) == true &&
      ping_interval > 0
     ) {
    lagopus_dstring_t ds;
    ret = lagopus_dstring_create(&ds);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = lagopus_dstring_appendf(&ds,
                                    "{\"ret\":\"OK\",\n\"data\":[{"
                                    "\"master-agentx-socket\":\"%s\",\n"
                                    "\"ping-interval-second\":%d,\n"
                                    "\"is-enabled\":%s"
                                    "}]}",
                                    esc_sock,
                                    ping_interval,
                                    IS_ENABLED(enable));
      if (ret == LAGOPUS_RESULT_OK) {
        char *str = NULL;
        ret = lagopus_dstring_str_get(&ds, &str);
        if (ret == LAGOPUS_RESULT_OK) {
          *ret_str = str;
        } else {
          free((void *)str);
        }
      }
    }
    lagopus_dstring_destroy(&ds);
  }

  free((void *)esc_sock);
  return ret;
}

static void
compared_strings(const char *expected_str) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *actual_str = NULL;
  TEST_DSTRING(ret, &result, actual_str, expected_str, true);
  free((void *)actual_str);
}

static void
snmp_json_result(const char *agentx_sock,
                 const uint16_t ping_interval) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const char *argv_snmp[] = {"snmp", NULL};
  char *expected_str = NULL;

  lagopus_dstring_clear(&result);
  ret = s_parse_snmp(&interp, state, ARGV_SIZE(argv_snmp), argv_snmp,
                     &tbl, proc, NULL, NULL, NULL,
                     &result);
  if (ret != LAGOPUS_RESULT_OK) {
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_snmp error.");
  }

  ret = snmp_json_result_log(agentx_sock,
                             ping_interval,
                             &expected_str);
  if (ret == LAGOPUS_RESULT_OK) {
    compared_strings(expected_str);
    lagopus_dstring_clear(&result);
    free((void *)expected_str);
  } else {
    free((void *)expected_str);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "snmp_result_json_str error.");
  }
}

void
test_snmp_cmd_parse(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const char *argv[] = {"snmp", NULL};
  const char *err_argv[] = {"snmp", "invalid_arg", NULL};

  // Normal case
  {
    ret = s_parse_snmp(&interp, state, ARGV_SIZE(argv), argv, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_snmp error.");
    snmp_json_result(DEFAULT_SNMPMGR_AGENTX_SOCKET,
                     DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC);
  }


  // Abnormal case
  {
    ret = s_parse_snmp(&interp, state, ARGV_SIZE(err_argv), err_argv, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");
    snmp_json_result(DEFAULT_SNMPMGR_AGENTX_SOCKET,
                     DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC);
  }

}


void
test_snmp_cmd_parse_agentx_sock(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const char *arg1 = DEFAULT_SNMPMGR_AGENTX_SOCKET;
  const char *arg2 = "ip:localhost:100";
  const char *option = "-master-agentx-socket";
  const char *argv1[] = {"snmp", option, NULL};
  const char *argv2[] = {"snmp", option, arg2, NULL};
  const char *argv3[] = {"snmp", option, arg1, option, NULL};
  const char *err_argv1[] = {"snmp", "-MASTER-AGENTX-SOCKET", NULL};
  const char *err_argv2[] = {"snmp", option + 1, NULL};
  char *expected_str1 = NULL;


  // Normal case
  {
    ret = s_parse_snmp(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_snmp error.");
    snmp_json_result_str(option + 1, arg1, &expected_str1);
    compared_strings(expected_str1);
    snmp_json_result(arg1,
                     DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC);


    ret = s_parse_snmp(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_snmp error.");
    compared_strings("{\"ret\":\"OK\"}");
    snmp_json_result(arg2,
                     DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC);


    ret = s_parse_snmp(&interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_snmp error.");


    ret = s_parse_snmp(&interp, state, ARGV_SIZE(err_argv1), err_argv1, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");


    ret = s_parse_snmp(&interp, state, ARGV_SIZE(err_argv2), err_argv2, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");
    snmp_json_result(DEFAULT_SNMPMGR_AGENTX_SOCKET,
                     DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC);
  }

  free((void *)expected_str1);
}

void
test_snmp_cmd_parse_ping_interval(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const uint16_t arg1 = DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC;
  const uint16_t arg2 = MAXIMUM_PING_SEC;
  const char *option = "-ping-interval-second";
  const char *argv1[] = {"snmp", option, NULL};
  const char *argv2[] = {"snmp", option, "255", NULL};
  const char *argv3[] = {"snmp", option, "10", option, NULL};
  const char *err_argv1[] = {"snmp", "-PING-INTERVAL-SECOND", NULL};
  const char *err_argv2[] = {"snmp", option + 1, NULL};
  const char *err_argv3[] = {"snmp", option, "0", NULL};
  const char *err_argv4[] = {"snmp", option, "256", NULL};
  char *expected_str1 = NULL;


  // Normal case
  {
    ret = s_parse_snmp(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_snmp error.");
    snmp_json_result_uint16_t(option + 1, arg1, &expected_str1);
    compared_strings(expected_str1);
    snmp_json_result(DEFAULT_SNMPMGR_AGENTX_SOCKET,
                     arg1);


    ret = s_parse_snmp(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_snmp error.");
    compared_strings("{\"ret\":\"OK\"}");
    snmp_json_result(DEFAULT_SNMPMGR_AGENTX_SOCKET,
                     arg2);


    ret = s_parse_snmp(&interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_snmp error.");


    ret = s_parse_snmp(&interp, state, ARGV_SIZE(err_argv1), err_argv1, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");


    ret = s_parse_snmp(&interp, state, ARGV_SIZE(err_argv2), err_argv2, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");
    snmp_json_result(DEFAULT_SNMPMGR_AGENTX_SOCKET,
                     DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC);


    ret = s_parse_snmp(&interp, state, ARGV_SIZE(err_argv3), err_argv3, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");


    ret = s_parse_snmp(&interp, state, ARGV_SIZE(err_argv4), err_argv4, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");
  }

  free((void *)expected_str1);
}

void
test_snmp_cmd_parse_enable(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const char *arg1 = "ip:localhost:100";
  const char *arg2 = "255";
  const char *arg3 = "256";
  const char *sub_cmd1 = "enable";
  const uint16_t arg2_uint16 = 255;
  const char *argv1[] = {"snmp", sub_cmd1, NULL};
  const char *argv2[] = {"snmp", sub_cmd1,
                         "-master-agentx-socket", arg1, NULL
                        };
  const char *argv3[] = {"snmp", sub_cmd1,
                         "-ping-interval-second", arg2, NULL
                        };
  const char *argv4[] = {"snmp", sub_cmd1,
                         "-master-agentx-socket", NULL
                        };
  const char *argv5[] = {"snmp", sub_cmd1,
                         "-ping-interval-second", NULL
                        };
  const char *err_argv1[] = {"snmp", "ENABLE", NULL};
  const char *err_argv2[] = {"snmp", sub_cmd1,
                             "-ping-interval-second", arg3, NULL
                            };

  // Normal case
  {
    conf.is_enabled = false;
    ret = s_parse_snmp(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_snmp error.");
    TEST_ASSERT_TRUE(conf.is_enabled);
    snmp_json_result(DEFAULT_SNMPMGR_AGENTX_SOCKET,
                     DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC);


    conf.is_enabled = false;
    ret = s_parse_snmp(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_snmp error.");
    TEST_ASSERT_TRUE(conf.is_enabled);
    snmp_json_result(arg1, DEFAULT_SNMPMGR_AGENTX_PING_INTERVAL_SEC);


    conf.is_enabled = false;
    ret = s_parse_snmp(&interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_snmp error.");
    TEST_ASSERT_TRUE(conf.is_enabled);
    snmp_json_result(arg1, arg2_uint16);


    ret = s_parse_snmp(&interp, state, ARGV_SIZE(argv4), argv4, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_snmp error.");


    ret = s_parse_snmp(&interp, state, ARGV_SIZE(argv5), argv5, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_snmp error.");


    ret = s_parse_snmp(&interp, state, ARGV_SIZE(err_argv1), err_argv1, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");


    ret = s_parse_snmp(&interp, state, ARGV_SIZE(err_argv2), err_argv2, &tbl,
                       proc, NULL, NULL, NULL,
                       &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");
  }
}

void
test_lagopus_snmp_get_agentx_sock(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  // Normal case
  {
    char *agentx_sock = NULL;
    bool bc = false;
    ret = lagopus_snmp_get_agentx_sock(&agentx_sock);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "lagopus_snmp_get_agentx_sock error.");
    if (strcmp(conf.agentx_sock, agentx_sock) == 0) {
      bc = true;
    }
    TEST_ASSERT_TRUE(bc);
    free((void *)agentx_sock);
  }

  // Abnormal case
  {
    ret = lagopus_snmp_get_agentx_sock(NULL);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                              "lagopus_snmp_get_agentx_sock error.");
  }
}

void
test_lagopus_snmp_get_ping_interval(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  // Normal case
  {
    uint16_t ping_interval = 0;
    bool bc = false;
    ret = lagopus_snmp_get_ping_interval(&ping_interval);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "lagopus_snmp_get_ping_interval error.");
    if (conf.ping_interval == ping_interval) {
      bc = true;
    }
    TEST_ASSERT_TRUE(bc);
  }

  // Abnormal case
  {
    ret = lagopus_snmp_get_ping_interval(NULL);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS, ret,
                              "lagopus_snmp_get_ping_interval error.");
  }
}

void
test_snmp_cmd_serialize_agentx_sock(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;

  const char *ser_argv1[] = {"snmp", "-master-agentx-socket", "tcp:127.0.0.1:705",
                             "-ping-interval-second", "10", NULL
                            };

  const char test_str1[] = "snmp -master-agentx-socket tcp:127.0.0.1:705 "
                           "-ping-interval-second 10\n\n";

  char *str1 = NULL;

  /* set file value. */
  ret = s_parse_snmp(&interp, state, ARGV_SIZE(ser_argv1), ser_argv1, &tbl,
                     proc, NULL, NULL, NULL,
                     &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_snmp error.");
  lagopus_dstring_clear(&result);

  /* TEST : serialize. */
  ret = snmp_cmd_serialize(&result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "snmp_cmd_serialize error.");
  TEST_DSTRING_NO_JSON(ret, &result, str1, test_str1, true);
}

void
test_snmp_cmd_serialize_ping_interval(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;

  const char *ser_argv1[] = {"snmp", "-master-agentx-socket", "tcp:localhost:705",
                             "-ping-interval-second", "55", NULL
                            };

  const char test_str1[] = "snmp -master-agentx-socket tcp:localhost:705 "
                           "-ping-interval-second 55\n\n";

  char *str1 = NULL;

  /* set file value. */
  ret = s_parse_snmp(&interp, state, ARGV_SIZE(ser_argv1), ser_argv1, &tbl,
                     proc, NULL, NULL, NULL,
                     &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_snmp error.");
  lagopus_dstring_clear(&result);

  /* TEST : serialize. */
  ret = snmp_cmd_serialize(&result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "snmp_cmd_serialize error.");
  TEST_DSTRING_NO_JSON(ret, &result, str1, test_str1, true);
}

void
test_destroy(void) {
  destroy = true;
}
