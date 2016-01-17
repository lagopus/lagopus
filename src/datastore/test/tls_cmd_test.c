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
#include "../tls_cmd.c"

static const char *cmd = "tls";
static const char *option1 = "-cert-file";
static const char *option2 = "-private-key";
static const char *option3 = "-certificate-store";
static const char *option4 = "-trust-point-conf";
static const char *option_val1 = "/tmp/option_val1";
static const char *option_val2 = "/tmp/option_val2";

static lagopus_dstring_t result = NULL;
static lagopus_hashmap_t tbl = NULL;
static datastore_interp_t interp = NULL;
static datastore_update_proc_t proc = NULL;
static bool destroy = false;

void
setUp(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  (void)lagopus_session_tls_set_ca_dir(option_val1);
  (void)lagopus_session_tls_set_server_cert(option_val1);
  (void)lagopus_session_tls_set_server_key(option_val1);
  (void)lagopus_session_tls_set_trust_point_conf(option_val1);

  /* create interp. */
  INTERP_CREATE(ret, NULL, interp, tbl, result);
}

void
tearDown(void) {
  /* destroy interp. */
  INTERP_DESTROY(NULL, interp, tbl, result, destroy);
}

static void
test_option_val_length(const lagopus_result_t expected_ret,
                       const char *str) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const char *argv1[] = {cmd, option1, str, NULL};
  const char *argv2[] = {cmd, option2, str, NULL};
  const char *argv3[] = {cmd, option3, str, NULL};
  const char *argv4[] = {cmd, option4, str, NULL};

  ret = s_parse_tls(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(expected_ret, ret,
                            "s_parse_tls error.");


  ret = s_parse_tls(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(expected_ret, ret,
                            "s_parse_tls error.");


  ret = s_parse_tls(&interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(expected_ret, ret,
                            "s_parse_tls error.");


  ret = s_parse_tls(&interp, state, ARGV_SIZE(argv4), argv4, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(expected_ret, ret,
                            "s_parse_tls error.");
}

static void
create_str(char **string, const size_t length) {
  if (string != NULL && *string == NULL && length > 0) {
    size_t i;
    *string = malloc(sizeof(char) * (length + 1));
    **string = '\0';
    for (i = 0; i < length; i++) {
      strncat(*string, "a", 1);
    }
  }
}

void
test_create_str(void) {
  char *actual_str = NULL;
  const char *expected_str = "aaa";

  create_str(&actual_str, strlen(expected_str));
  TEST_ASSERT_EQUAL_STRING(expected_str, actual_str);

  free((void *)actual_str);
}

void
test_tls_cmd_set(void) {
  char *str1 = NULL;
  char *str2 = NULL;

  create_str(&str1, PATH_MAX - 1);
  create_str(&str2, PATH_MAX);

  test_option_val_length(LAGOPUS_RESULT_OK, str1);
  test_option_val_length(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, str2);

  free((void *)str1);
  free((void *)str2);
}


static lagopus_result_t
create_result_json_str(const char *option, const char *option_val,
                       char **dst_str) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_dstring_t ds;
  char *esc_str = NULL;

  if (option == NULL || option_val == NULL || *dst_str != NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if ((ret = datastore_json_string_escape(option_val,
                                          (char **)&esc_str))
      != LAGOPUS_RESULT_OK) {
    goto done;
  }

  ret = lagopus_dstring_create(&ds);
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
      if (ret == LAGOPUS_RESULT_OK) {
        *dst_str = str;
      } else {
        free((void *)str);
      }
    }
  }
  lagopus_dstring_destroy(&ds);

done:
  free((void *)esc_str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "create_result_json_str error.");
  return ret;
}

void
test_create_result_json_str(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *esc_str1 = NULL;
  char *esc_str2 = NULL;
  char *esc_str3 = NULL;
  char *esc_str4 = NULL;
  char expected_str1[PATH_MAX];
  char expected_str2[PATH_MAX];
  char expected_str3[PATH_MAX];
  char expected_str4[PATH_MAX];
  char *actual_str1 = NULL;
  char *actual_str2 = NULL;
  char *actual_str3 = NULL;
  char *actual_str4 = NULL;

  // Normal case
  {
    // option1
    ret = datastore_json_string_escape(DEFAULT_TLS_CERTIFICATE_FILE,
                                       (char **)&esc_str1);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "datastore_json_string_escape error.");
    snprintf(expected_str1, PATH_MAX,
             "{\"ret\":\"OK\",\n\"data\":[{"
             "\"%s\":\"%s\""
             "}]}",
             option1 + 1, esc_str1);
    ret = create_result_json_str(option1 + 1, DEFAULT_TLS_CERTIFICATE_FILE,
                                 &actual_str1);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "create_result_json_str error.");
    TEST_ASSERT_EQUAL_STRING(expected_str1, actual_str1);


    // option2
    ret = datastore_json_string_escape(DEFAULT_TLS_PRIVATE_KEY,
                                       (char **)&esc_str2);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "datastore_json_string_escape error.");
    snprintf(expected_str2, PATH_MAX,
             "{\"ret\":\"OK\",\n\"data\":[{"
             "\"%s\":\"%s\""
             "}]}",
             option2 + 1, esc_str2);
    ret = create_result_json_str(option2 + 1, DEFAULT_TLS_PRIVATE_KEY,
                                 &actual_str2);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "create_result_json_str error.");
    TEST_ASSERT_EQUAL_STRING(expected_str2, actual_str2);


    // option3
    ret = datastore_json_string_escape(DEFAULT_TLS_CERTIFICATE_STORE,
                                       (char **)&esc_str3);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "datastore_json_string_escape error.");
    snprintf(expected_str3, PATH_MAX,
             "{\"ret\":\"OK\",\n\"data\":[{"
             "\"%s\":\"%s\""
             "}]}",
             option3 + 1, esc_str3);
    ret = create_result_json_str(option3 + 1, DEFAULT_TLS_CERTIFICATE_STORE,
                                 &actual_str3);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "create_result_json_str error.");
    TEST_ASSERT_EQUAL_STRING(expected_str3, actual_str3);


    // option4
    ret = datastore_json_string_escape(DEFAULT_TLS_TRUST_POINT_CONF,
                                       (char **)&esc_str4);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "datastore_json_string_escape error.");
    snprintf(expected_str4, PATH_MAX,
             "{\"ret\":\"OK\",\n\"data\":[{"
             "\"%s\":\"%s\""
             "}]}",
             option4 + 1, esc_str4);
    ret = create_result_json_str(option4 + 1, DEFAULT_TLS_TRUST_POINT_CONF,
                                 &actual_str4);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "create_result_json_str error.");
    TEST_ASSERT_EQUAL_STRING(expected_str4, actual_str4);
  }

  free((void *)esc_str1);
  free((void *)esc_str2);
  free((void *)esc_str3);
  free((void *)esc_str4);
  free((void *)actual_str1);
  free((void *)actual_str2);
  free((void *)actual_str3);
  free((void *)actual_str4);
}

static void
compared_result_json_str(const char *option, const char *option_val) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *expected_str = NULL;
  char *actual_str = NULL;
  char *str = NULL;

  ret = create_result_json_str(option, option_val,
                               &expected_str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "create_result_json_str error.");
  ret = lagopus_dstring_str_get(&result, &str);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
  free((void *)str);
  TEST_DSTRING(ret, &result, actual_str, expected_str, true);
  lagopus_dstring_clear(&result);
  free((void *)actual_str);
  free((void *)expected_str);
}

void
test_tls_cmd_parse(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const char *argv0[] = {cmd, option1, option_val1,
                         option2, option_val1,
                         option3, option_val1,
                         option4, option_val1, NULL
                        };
  const char *argv1[] = {cmd, option1, NULL};
  const char *argv2[] = {cmd, option2, NULL};
  const char *argv3[] = {cmd, option3, NULL};
  const char *argv4[] = {cmd, option4, NULL};
  const char *err_argv[] = {cmd, "invalid_arg", NULL};

  // Normal case
  {
    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv0), argv0, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");

    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json_str(option1 + 1, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json_str(option2 + 1, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json_str(option3 + 1, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv4), argv4, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json_str(option4 + 1, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(err_argv), err_argv, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_tls error.");
  }
}

static lagopus_result_t
create_result_json_str_all(const char *session_cert,
                           const char *private_key,
                           const char *session_ca_dir,
                           const char *trust_point_conf,
                           char **ret_str) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *esc_certfile = NULL;
  char *esc_pvtkey = NULL;
  char *esc_certstore = NULL;
  char *esc_tpconf = NULL;
  lagopus_dstring_t ds;

  if (ret_str == NULL || *ret_str != NULL) {
    return LAGOPUS_RESULT_INVALID_ARGS;
  }

  if ((ret = datastore_json_string_escape(session_cert,
                                          (char **)&esc_certfile))
      != LAGOPUS_RESULT_OK) {
    goto done;
  }
  if ((ret = datastore_json_string_escape(private_key,
                                          (char **)&esc_pvtkey))
      != LAGOPUS_RESULT_OK) {
    goto done;
  }
  if ((ret = datastore_json_string_escape(session_ca_dir,
                                          (char **)&esc_certstore))
      != LAGOPUS_RESULT_OK) {
    goto done;
  }
  if ((ret = datastore_json_string_escape(trust_point_conf,
                                          (char **)&esc_tpconf))
      != LAGOPUS_RESULT_OK) {
    goto done;
  }

  ret = lagopus_dstring_create(&ds);
  if (ret == LAGOPUS_RESULT_OK) {
    ret = lagopus_dstring_appendf(&ds,
                                  "{\"ret\":\"OK\",\n\"data\":[{"
                                  "\"cert-file\":\"%s\",\n"
                                  "\"private-key\":\"%s\",\n"
                                  "\"certificate-store\":\"%s\",\n"
                                  "\"trust-point-conf\":\"%s\""
                                  "}]}",
                                  esc_certfile,
                                  esc_pvtkey,
                                  esc_certstore,
                                  esc_tpconf);
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

done:
  free((void *)esc_certfile);
  free((void *)esc_pvtkey);
  free((void *)esc_certstore);
  free((void *)esc_tpconf);
  return ret;
}

void
test_create_result_json_all(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char expected_str[PATH_MAX];
  char *actual_str = NULL;

  // Normal case
  {
    char *esc_str = NULL;
    if ((ret = datastore_json_string_escape(option_val1,
                                            (char **)&esc_str))
        != LAGOPUS_RESULT_OK) {
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "datastore_json_string_escape error.");
    }

    snprintf(expected_str, PATH_MAX,
             "{\"ret\":\"OK\",\n\"data\":[{"
             "\"%s\":\"%s\",\n"
             "\"%s\":\"%s\",\n"
             "\"%s\":\"%s\",\n"
             "\"%s\":\"%s\""
             "}]}",
             option1 + 1, esc_str,
             option2 + 1, esc_str,
             option3 + 1, esc_str,
             option4 + 1, esc_str);
    ret = create_result_json_str_all(option_val1,
                                     option_val1,
                                     option_val1,
                                     option_val1,
                                     &actual_str);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "create_result_json_str error.");
    TEST_ASSERT_EQUAL_STRING(expected_str, actual_str);

    free((void *)esc_str);
  }

  free((void *)actual_str);
}

static void
compared_result_json(const char *session_cert,
                     const char *private_key,
                     const char *session_ca_dir,
                     const char *trust_point_conf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const char *argv[] = {cmd, NULL};
  char *expected_str = NULL;
  char *actual_str = NULL;

  ret = create_result_json_str_all(session_cert,
                                   private_key,
                                   session_ca_dir,
                                   trust_point_conf,
                                   &expected_str);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "tls_result_json_str error.");

  lagopus_dstring_clear(&result);
  ret = s_parse_tls(&interp, state, ARGV_SIZE(argv), argv,
                    &tbl, proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "s_parse_tls error.");
  TEST_DSTRING(ret, &result, actual_str, expected_str, true);
  free((void *)actual_str);
  free((void *)expected_str);
  lagopus_dstring_clear(&result);
}

void
test_tls_cmd_parse_session_cert(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const char *argv1[] = {cmd, option1, option1, NULL};
  const char *argv2[] = {cmd, option1, option_val1, option1, NULL};
  const char *argv3[] = {cmd, option1, option_val2, NULL};
  const char *argv4[] = {cmd, option1, option_val2, option2, option_val2, NULL};
  const char *argv5[] = {cmd, option1, option_val2, option3, option_val2, NULL};
  const char *argv6[] = {cmd, option1, option_val2, option4, option_val2, NULL};
  const char *err_argv1[] = {cmd, "-CERT-FILE", NULL};
  const char *err_argv2[] = {cmd, option1 + 1, NULL};

  // Normal case
  {
    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option1, option_val1, option_val1, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json_str(option1 + 1, option_val1);
    compared_result_json(option_val1, option_val1, option_val1, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val2, option_val1, option_val1, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv4), argv4, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val2, option_val2, option_val1, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv5), argv5, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val2, option_val2, option_val2, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv6), argv6, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val2, option_val2, option_val2, option_val2);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(err_argv1), err_argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");


    ret = s_parse_tls(&interp, state, ARGV_SIZE(err_argv2), err_argv2, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");
  }
}

void
test_tls_cmd_parse_private_key(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const char *argv1[] = {cmd, option2, option2, NULL};
  const char *argv2[] = {cmd, option2, option_val1, option2, NULL};
  const char *argv3[] = {cmd, option2, option_val2, NULL};
  const char *argv4[] = {cmd, option2, option_val2, option1, option_val2, NULL};
  const char *argv5[] = {cmd, option2, option_val2, option3, option_val2, NULL};
  const char *argv6[] = {cmd, option2, option_val2, option4, option_val2, NULL};
  const char *err_argv1[] = {cmd, "-PRIVATE_KEY", NULL};
  const char *err_argv2[] = {cmd, option2 + 1, NULL};


  // Normal case
  {
    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val1, option2, option_val1, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json_str(option2 + 1, option_val1);
    compared_result_json(option_val1, option_val1, option_val1, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val1, option_val2, option_val1, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv4), argv4, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val2, option_val2, option_val1, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv5), argv5, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val2, option_val2, option_val2, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv6), argv6, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val2, option_val2, option_val2, option_val2);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(err_argv1), err_argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");


    ret = s_parse_tls(&interp, state, ARGV_SIZE(err_argv2), err_argv2, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");
  }
}


void
test_tls_cmd_parse_certstore(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const char *argv1[] = {cmd, option3, option3, NULL};
  const char *argv2[] = {cmd, option3, option_val1, option3, NULL};
  const char *argv3[] = {cmd, option3, option_val2, NULL};
  const char *argv4[] = {cmd, option3, option_val2, option1, option_val2, NULL};
  const char *argv5[] = {cmd, option3, option_val2, option2, option_val2, NULL};
  const char *argv6[] = {cmd, option3, option_val2, option4, option_val2, NULL};
  const char *err_argv1[] = {cmd, "-CERTIFICATE_STORE", NULL};
  const char *err_argv2[] = {cmd, option3 + 1, NULL};

  // Normal case
  {
    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val1, option_val1, option3, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json_str(option3 + 1, option_val1);
    compared_result_json(option_val1, option_val1, option_val1, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val1, option_val1, option_val2, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv4), argv4, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val2, option_val1, option_val2, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv5), argv5, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val2, option_val2, option_val2, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv6), argv6, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val2, option_val2, option_val2, option_val2);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(err_argv1), err_argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");


    ret = s_parse_tls(&interp, state, ARGV_SIZE(err_argv2), err_argv2, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");
  }
}

void
test_tls_cmd_parse_tpconf(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  const char *argv1[] = {cmd, option4, option4, NULL};
  const char *argv2[] = {cmd, option4, option_val1, option4, NULL};
  const char *argv3[] = {cmd, option4, option_val2, NULL};
  const char *argv4[] = {cmd, option4, option_val2, option1, option_val2, NULL};
  const char *argv5[] = {cmd, option4, option_val2, option2, option_val2, NULL};
  const char *argv6[] = {cmd, option4, option_val2, option3, option_val2, NULL};
  const char *err_argv1[] = {cmd, "-TRUST_POINT_CONF", NULL};
  const char *err_argv2[] = {cmd, option4 + 1, NULL};

  // Normal case
  {
    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val1, option_val1, option_val1, option4);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json_str(option4 + 1, option_val1);
    compared_result_json(option_val1, option_val1, option_val1, option_val1);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val1, option_val1, option_val1, option_val2);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv4), argv4, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val2, option_val1, option_val1, option_val2);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv5), argv5, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val2, option_val2, option_val1, option_val2);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(argv6), argv6, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_tls error.");
    compared_result_json(option_val2, option_val2, option_val2, option_val2);


    ret = s_parse_tls(&interp, state, ARGV_SIZE(err_argv1), err_argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");


    ret = s_parse_tls(&interp, state, ARGV_SIZE(err_argv2), err_argv2, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_snmp error.");
  }
}


void
test_tls_cmd_serialize_cert_file(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;

  const char *ser_argv1[] = {"tls", "-cert-file",
                             "/tmp/catls.pem",
                             "-private-key",
                             "/usr/local/etc/lagopus/key.pem",
                             "-certificate-store",
                             "/usr/local/etc/lagopus",
                             "-trust-point-conf",
                             "/usr/local/etc/lagopus/check.conf", NULL
                            };

  const char test_str1[] = "tls -cert-file /tmp/catls.pem "
                           "-private-key /usr/local/etc/lagopus/key.pem "
                           "-certificate-store /usr/local/etc/lagopus "
                           "-trust-point-conf /usr/local/etc/lagopus/check.conf\n\n";

  char *str1 = NULL;

  /* set file value. */
  ret = s_parse_tls(&interp, state, ARGV_SIZE(ser_argv1), ser_argv1, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_tls error.");
  lagopus_dstring_clear(&result);

  /* TEST : serialize. */
  ret = tls_cmd_serialize(&result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "tls_cmd_serialize error.");
  TEST_DSTRING_NO_JSON(ret, &result, str1, test_str1, true);
}

void
test_tls_cmd_serialize_private_key(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;

  const char *ser_argv1[] = {"tls", "-cert-file",
                             "/usr/local/etc/lagopus/catls.pem",
                             "-private-key",
                             "/tmp/key.pem",
                             "-certificate-store",
                             "/usr/local/etc/lagopus",
                             "-trust-point-conf",
                             "/usr/local/etc/lagopus/check.conf", NULL
                            };

  const char test_str1[] = "tls -cert-file /usr/local/etc/lagopus/catls.pem "
                           "-private-key /tmp/key.pem "
                           "-certificate-store /usr/local/etc/lagopus "
                           "-trust-point-conf /usr/local/etc/lagopus/check.conf\n\n";

  char *str1 = NULL;

  /* set file value. */
  ret = s_parse_tls(&interp, state, ARGV_SIZE(ser_argv1), ser_argv1, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_tls error.");
  lagopus_dstring_clear(&result);

  /* TEST : serialize. */
  ret = tls_cmd_serialize(&result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "tls_cmd_serialize error.");
  TEST_DSTRING_NO_JSON(ret, &result, str1, test_str1, true);
}

void
test_tls_cmd_serialize_certificate_store(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;

  const char *ser_argv1[] = {"tls", "-cert-file",
                             "/usr/local/etc/lagopus/catls.pem",
                             "-private-key",
                             "/usr/local/etc/lagopus/key.pem",
                             "-certificate-store",
                             "/tmp/lagopus",
                             "-trust-point-conf",
                             "/usr/local/etc/lagopus/check.conf", NULL
                            };

  const char test_str1[] = "tls -cert-file /usr/local/etc/lagopus/catls.pem "
                           "-private-key /usr/local/etc/lagopus/key.pem "
                           "-certificate-store /tmp/lagopus "
                           "-trust-point-conf /usr/local/etc/lagopus/check.conf\n\n";

  char *str1 = NULL;

  /* set file value. */
  ret = s_parse_tls(&interp, state, ARGV_SIZE(ser_argv1), ser_argv1, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_tls error.");
  lagopus_dstring_clear(&result);

  /* TEST : serialize. */
  ret = tls_cmd_serialize(&result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "tls_cmd_serialize error.");
  TEST_DSTRING_NO_JSON(ret, &result, str1, test_str1, true);
}

void
test_tls_cmd_serialize_trust_point_conf(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;

  const char *ser_argv1[] = {"tls", "-cert-file",
                             "/usr/local/etc/lagopus/catls.pem",
                             "-private-key",
                             "/usr/local/etc/lagopus/key.pem",
                             "-certificate-store",
                             "/usr/local/etc/lagopus",
                             "-trust-point-conf",
                             "/tmp/check.conf", NULL
                            };

  const char test_str1[] = "tls -cert-file /usr/local/etc/lagopus/catls.pem "
                           "-private-key /usr/local/etc/lagopus/key.pem "
                           "-certificate-store /usr/local/etc/lagopus "
                           "-trust-point-conf /tmp/check.conf\n\n";

  char *str1 = NULL;

  /* set file value. */
  ret = s_parse_tls(&interp, state, ARGV_SIZE(ser_argv1), ser_argv1, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_tls error.");
  lagopus_dstring_clear(&result);

  /* TEST : serialize. */
  ret = tls_cmd_serialize(&result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "tls_cmd_serialize error.");
  TEST_DSTRING_NO_JSON(ret, &result, str1, test_str1, true);
}

void
test_destroy(void) {
  destroy = true;
}
