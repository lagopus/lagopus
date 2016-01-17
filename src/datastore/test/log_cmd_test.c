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
#include "lagopus/datastore.h"
#include "cmd_test_utils.h"
#include "../interface_cmd_internal.h"
#include "../datastore_internal.h"
#include "../log_cmd.c"

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
}

void
tearDown(void) {
  /* destroy interp. */
  INTERP_DESTROY(NULL, interp, tbl, result, destroy);

  unlink("testlog_ident");
  unlink("testlog_ident'\"esc");
  unlink("testlog_file");
  unlink("testlog_file'\"esc");
  unlink("testlog_emit");
  unlink("testlog_emit'\"esc");
}

void
test_log_create_tables(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  ret = log_initialize();
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  ret = log_ofpt_table_add();
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
}

void
test_log_cmd_parse_create(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str1 = NULL;
  char *str2 = NULL;
  const char *argv1[] = {"log", NULL};
  const char test_str1[] = "{\"ret\":\"OK\",\n\"data\":{"
                           "\"debuglevel\":0,\n"
                           "\"packetdump\":[\"\"]"
                           "}}";
  const char *argv_err1[] = {"log", "invalid_arg", NULL};


  // Normal case
  {
    ret = s_parse_log(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_log error.");
    TEST_DSTRING(ret, &result, str1, test_str1, true);

    ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err1), argv_err1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                              "s_parse_log error.");
  }


  // Abnormal case
  {
    ret = s_parse_log(NULL, state, ARGV_SIZE(argv1), argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);


    ret = s_parse_log(&interp, state, ARGV_SIZE(argv1), argv1, NULL,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);


    ret = s_parse_log(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                      NULL, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);


    ret = s_parse_log(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
  }

  free((void *)str1);
  free((void *)str2);
}

void
test_log_cmd_parse_syslog_and_ident(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str1 = NULL;
  char *str2 = NULL;
  char *str3 = NULL;
  char *str4 = NULL;
  const char *argv1[] = {"log", "-syslog", "-ident", "testlog_ident", NULL};
  const char *argv2[] = {"log", NULL};
  const char *argv3[] = {"log", "-syslog", NULL};
  const char *argv4[] = {"log", "-syslog", "-ident", "testlog_ident'\"esc", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] = "{\"ret\":\"OK\",\n\"data\":{"
                           "\"ident\":\"testlog_ident\",\n"
                           "\"debuglevel\":0,\n"
                           "\"packetdump\":[\"\"]"
                           "}}";
  const char test_str3[] = "{\"ret\":\"OK\"}";
  const char test_str4[] = "{\"ret\":\"OK\",\n\"data\":{"
                           "\"ident\":\"testlog_ident'\\\"esc\",\n"
                           "\"debuglevel\":0,\n"
                           "\"packetdump\":[\"\"]"
                           "}}";
  const char *argv_err1[] = {"log", "-syslog", NULL};
  const char *argv_err2[] = {"log", "-ident", NULL};
  const char *argv_err3[] = {"log", "-ident", "testlog_ident", NULL};
  const char *argv_err4[] = {"log", "-ident", "testlog_ident\"", NULL};
  const char *argv_err5[] = {"log", "-syslog", "-IDENT", "testlog_ident", NULL};
  const char *argv_err6[] = {"log", "-SYSLOG", "-ident", "testlog_ident", NULL};


  // Normal case
  {
    // Return error other than Linux.
    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err1), argv_err1, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
#ifdef HAVE_PROCFS_SELF_EXE
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
#else
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);
#endif /* HAVE_PROCFS_SELF_EXE */
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err2), argv_err2, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
#ifdef HAVE_PROCFS_SELF_EXE
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
#else
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);
#endif /* HAVE_PROCFS_SELF_EXE */
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err3), argv_err3, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
#ifdef HAVE_PROCFS_SELF_EXE
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
#else
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);
#endif /* HAVE_PROCFS_SELF_EXE */
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err4), argv_err4, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err5), argv_err5, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err6), argv_err6, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);
    }


    // Return OK
    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str1, test_str1, true);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str2, test_str2, true);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str3, test_str3, true);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv4), argv4, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str4, test_str4, true);
    }
  }

  free((void *)str1);
  free((void *)str2);
  free((void *)str3);
  free((void *)str4);
}

void
test_log_cmd_parse_file(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str1 = NULL;
  char *str2 = NULL;
  char *str3 = NULL;
  const char *argv1[] = {"log", "-file", "testlog_file", NULL};
  const char *argv2[] = {"log", NULL};
  const char *argv3[] = {"log", "-file", "testlog_file'\"esc", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] = "{\"ret\":\"OK\",\n\"data\":{"
                           "\"file\":\"testlog_file\",\n"
                           "\"debuglevel\":0,\n"
                           "\"packetdump\":[\"\"]"
                           "}}";
  const char test_str3[] = "{\"ret\":\"OK\",\n\"data\":{"
                           "\"file\":\"testlog_file'\\\"esc\",\n"
                           "\"debuglevel\":0,\n"
                           "\"packetdump\":[\"\"]"
                           "}}";
  const char *argv_err1[] = {"log", "-file", NULL};
  const char *argv_err2[] = {"log", "-FILE", "testlog_file", NULL};
  const char *argv_err3[] = {"log", "-file", "testlog_file\"", NULL};

  // Normal case
  {
    // Return error
    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err1), argv_err1, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err2), argv_err2, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err3), argv_err3, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);
    }


    // Return OK
    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str1, test_str1, true);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str2, test_str2, true);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str3, test_str3, true);
    }
  }


  free((void *)str1);
  free((void *)str2);
  free((void *)str3);
}

void
test_log_cmd_parse_debuglevel(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str1 = NULL;
  char *str2 = NULL;
  char *str3 = NULL;
  char *str4 = NULL;
  const char *argv0[] = {"log", "-file", "testlog_file", NULL};
  const char *argv1[] = {"log", "-debuglevel", NULL};
  const char *argv2[] = {"log", NULL};
  const char *argv3[] = {"log", "-debuglevel", "1", NULL};
  const char *argv4[] = {"log", "-debuglevel", "0", NULL};
  const char test_str1[] = "{\"ret\":\"OK\",\n\"data\":{"
                           "\"debuglevel\":0"
                           "}}";
  const char test_str2[] = "{\"ret\":\"OK\",\n\"data\":{"
                           "\"file\":\"testlog_file\",\n"
                           "\"debuglevel\":0,\n"
                           "\"packetdump\":[\"\"]"
                           "}}";
  const char test_str3_1[] = "{\"ret\":\"OK\"}";
  const char test_str3_2[] = "{\"ret\":\"OK\",\n\"data\":{"
                             "\"file\":\"testlog_file\",\n"
                             "\"debuglevel\":1,\n"
                             "\"packetdump\":[\"\"]"
                             "}}";
  const char *argv_err1[] = {"log", "-debuglevel", "-1", NULL};
  const char *argv_err2[] = {"log", "-debuglevel", "65536", NULL};
  const char *argv_err3[] = {"log", "-DEBUGLEVEL", NULL};


  ret = s_parse_log(&interp, state, ARGV_SIZE(argv0), argv0, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  // Normal case
  {
    // Return OK
    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str1, test_str1, true);
    } else {
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str2, test_str2, true);
    } else {
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str3, test_str3_1, true);
    } else {
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
    }


    free((void *) str3);
    str3 = NULL;


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str3, test_str3_2, true);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv4), argv4, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);
    }


    // Return error
    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err1), argv_err1, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err2), argv_err2, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err3), argv_err3, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);
    }
  }

  free((void *) str1);
  free((void *) str2);
  free((void *) str3);
  free((void *) str4);
}

void
test_log_cmd_parse_emit(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str1 = NULL;
  char *str2 = NULL;
  char *str3 = NULL;
  const char *argv0[] = {"log", "-file", "testlog_file", NULL};
  const char *argv1[] = {"log", "emit", "testlog_emit", NULL};
  const char *argv2[] = {"log", NULL};
  const char *argv3[] = {"log", "emit", "testlog_emit'\"esc", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] = {"{\"ret\":\"OK\",\n\"data\":{"
                            "\"file\":\"testlog_file\",\n"
                            "\"debuglevel\":0,\n"
                            "\"packetdump\":[\"\"]"
                            "}}"
                           };
  const char test_str3[] = {"{\"ret\":\"OK\",\n\"data\":{"
                            "\"file\":\"testlog_file\",\n"
                            "\"debuglevel\":0,\n"
                            "\"packetdump\":[\"\"]"
                            "}}"
                           };
  const char *argv_err1[] = {"log", "emit", NULL};
  const char *argv_err2[] = {"log", "EMIT", "testlog_emit", NULL};


  ret = s_parse_log(&interp, state, ARGV_SIZE(argv0), argv0, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  // Normal case
  {
    // Return OK
    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str1, test_str1, true);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str2, test_str2, true);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str3, test_str3, true);
    }


    // Return error
    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err1), argv_err1, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err2), argv_err2, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret);
    }
  }

  free((void *) str1);
  free((void *) str2);
  free((void *) str3);
}

void
test_log_cmd_parse_packetdump(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str1 = NULL;
  char *str2 = NULL;
  char *str3 = NULL;
  char *str4 = NULL;
  char *str5 = NULL;
  const char *argv0[] = {"log", "-file", "testlog_file", NULL};
  const char *argv1[] = {"log", "-packetdump",
                         "hello",
                         NULL
                        };
  const char *argv2[] = {"log", "-packetdump",
                         NULL
                        };
  const char *argv3[] = {"log", NULL};
  const char *argv4[] = {"log", "-packetdump",
                         "-hello",
                         NULL
                        };
  const char *argv5[] = {"log", "-packetdump",
                         "hello",
                         "-hello",
                         NULL
                        };
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] = "{\"ret\":\"OK\",\n\"data\":["
                           "\"hello\""
                           "]}";
  const char test_str3[] = "{\"ret\":\"OK\",\n\"data\":{"
                           "\"file\":\"testlog_file\",\n"
                           "\"debuglevel\":0,\n"
                           "\"packetdump\":["
                           "\"hello\""
                           "]}}";
  const char test_str4[] = "{\"ret\":\"OK\",\n\"data\":["
                           "\"\""
                           "]}";
  const char test_str5[] = "{\"ret\":\"OK\",\n\"data\":["
                           "\"\""
                           "]}";
  const char *argv_err1[] = {"log", "-packetdump",
                             "invalid_arg",
                             NULL
                            };
  const char *argv_err3[] = {"log", "-ident", "testlog_ident",
                             "-packetdump",
                             NULL
                            };
  const char *argv_err7[] = {"log", "-PACKETDUMP", NULL};
  const char *argv_err8[] = {"log", "-packetdump",
                             "HELLO",
                             NULL
                            };

  ret = s_parse_log(&interp, state, ARGV_SIZE(argv0), argv0, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, ret);

  // Normal case
  {
    // Return OK
    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str1, test_str1, true);
    } else {
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str2, test_str2, true);
    } else {
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv3), argv3, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str3, test_str3, true);
    } else {
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv4), argv4, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str4, test_str4, true);
    } else {
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
    }


    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv5), argv5, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                                "s_parse_log error.");
      TEST_DSTRING(ret, &result, str5, test_str5, true);
    } else {
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
    }


    // Return error
    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err1), argv_err1, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                                "s_parse_log error.");
    } else {
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
    }

    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err3), argv_err3, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                                "s_parse_log error.");
    } else {
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
    }

    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err7), argv_err7, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                                "s_parse_log error.");
    } else {
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
    }

    if ((ret = lagopus_dstring_clear(&result)) == LAGOPUS_RESULT_OK) {
      ret = s_parse_log(&interp, state, ARGV_SIZE(argv_err8), argv_err8, &tbl,
                        proc, NULL, NULL, NULL,
                        &result);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_DATASTORE_INTERP_ERROR, ret,
                                "s_parse_log error.");
    } else {
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, ret);
    }
  }

  free((void *) str1);
  free((void *) str2);
  free((void *) str3);
  free((void *) str4);
  free((void *) str5);
}

void
test_log_cmd_parse_set_trace_flags01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str1 = NULL;
  char *str2 = NULL;
  const char *argv1[] = {"log", "-packetdump",
                         "hello",
                         "error",
                         "echo_request",
                         "echo_reply",
                         "experimenter",
                         "features_request",
                         "features_reply",
                         "get_config_request",
                         "get_config_reply",
                         "set_config",
                         "packet_in",
                         "flow_removed",
                         "port_status",
                         "packet_out",
                         "flow_mod",
                         "group_mod",
                         "port_mod",
                         "table_mod",
                         "multipart_request",
                         "multipart_reply",
                         "barrier_request",
                         "barrier_reply",
                         "queue_get_config_request",
                         "queue_get_config_reply",
                         "role_request",
                         "role_reply",
                         "get_async_request",
                         "get_async_reply",
                         "set_async",
                         "meter_mod",
                         NULL
                        };
  const char *argv2[] = {"log", "-packetdump", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] = {"{\"ret\":\"OK\",\n\"data\":["
                            "\"hello\","
                            "\"error\","
                            "\"echo_request\","
                            "\"echo_reply\","
                            "\"experimenter\","
                            "\"features_request\","
                            "\"features_reply\","
                            "\"get_config_request\","
                            "\"get_config_reply\","
                            "\"set_config\","
                            "\"packet_in\","
                            "\"flow_removed\","
                            "\"port_status\","
                            "\"packet_out\","
                            "\"flow_mod\","
                            "\"group_mod\","
                            "\"port_mod\","
                            "\"table_mod\","
                            "\"multipart_request\","
                            "\"multipart_reply\","
                            "\"barrier_request\","
                            "\"barrier_reply\","
                            "\"queue_get_config_request\","
                            "\"queue_get_config_reply\","
                            "\"role_request\","
                            "\"role_reply\","
                            "\"get_async_request\","
                            "\"get_async_reply\","
                            "\"set_async\","
                            "\"meter_mod\""
                            "]}"
                           };

  // Normal case
  {
    // Return OK
    ret = s_parse_log(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_log error.");
    TEST_DSTRING(ret, &result, str1, test_str1, true);


    ret = s_parse_log(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_log error.");
    TEST_DSTRING(ret, &result, str2, test_str2, true);
  }

  free((void *) str1);
  free((void *) str2);
}

void
test_log_cmd_parse_unset_trace_flags_01(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str1 = NULL;
  char *str2 = NULL;
  const char *argv1[] = {"log", "-packetdump",
                         "-hello",
                         "-error",
                         "-echo_request",
                         "-echo_reply",
                         "-experimenter",
                         "-features_request",
                         "-features_reply",
                         "-get_config_request",
                         "-get_config_reply",
                         "-set_config",
                         "-packet_in",
                         "-flow_removed",
                         "-port_status",
                         "-packet_out",
                         "-flow_mod",
                         "-group_mod",
                         "-port_mod",
                         "-table_mod",
                         "-multipart_request",
                         "-multipart_reply",
                         "-barrier_request",
                         "-barrier_reply",
                         "-queue_get_config_request",
                         "-queue_get_config_reply",
                         "-role_request",
                         "-role_reply",
                         "-get_async_request",
                         "-get_async_reply",
                         "-set_async",
                         "-meter_mod",
                         NULL
                        };
  const char *argv2[] = {"log", "-packetdump", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] = "{\"ret\":\"OK\",\n\"data\":["
                           "\"\""
                           "]}";

  // Normal case
  {
    // Return OK
    ret = s_parse_log(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);

    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_log error.");
    TEST_DSTRING(ret, &result, str1, test_str1, true);

    ret = s_parse_log(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);

    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_log error.");
    TEST_DSTRING(ret, &result, str2, test_str2, true);
  }

  free((void *) str1);
  free((void *) str2);
}

void
test_log_cmd_parse_set_trace_flags02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str1 = NULL;
  char *str2 = NULL;
  const char *argv1[] = {"log", "-packetdump",
                         "+hello",
                         "+error",
                         "+echo_request",
                         "+echo_reply",
                         "+experimenter",
                         "+features_request",
                         "+features_reply",
                         "+get_config_request",
                         "+get_config_reply",
                         "+set_config",
                         "+packet_in",
                         "+flow_removed",
                         "+port_status",
                         "+packet_out",
                         "+flow_mod",
                         "+group_mod",
                         "+port_mod",
                         "+table_mod",
                         "+multipart_request",
                         "+multipart_reply",
                         "+barrier_request",
                         "+barrier_reply",
                         "+queue_get_config_request",
                         "+queue_get_config_reply",
                         "+role_request",
                         "+role_reply",
                         "+get_async_request",
                         "+get_async_reply",
                         "+set_async",
                         "+meter_mod",
                         NULL
                        };
  const char *argv2[] = {"log", "-packetdump", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] = {"{\"ret\":\"OK\",\n\"data\":["
                            "\"hello\","
                            "\"error\","
                            "\"echo_request\","
                            "\"echo_reply\","
                            "\"experimenter\","
                            "\"features_request\","
                            "\"features_reply\","
                            "\"get_config_request\","
                            "\"get_config_reply\","
                            "\"set_config\","
                            "\"packet_in\","
                            "\"flow_removed\","
                            "\"port_status\","
                            "\"packet_out\","
                            "\"flow_mod\","
                            "\"group_mod\","
                            "\"port_mod\","
                            "\"table_mod\","
                            "\"multipart_request\","
                            "\"multipart_reply\","
                            "\"barrier_request\","
                            "\"barrier_reply\","
                            "\"queue_get_config_request\","
                            "\"queue_get_config_reply\","
                            "\"role_request\","
                            "\"role_reply\","
                            "\"get_async_request\","
                            "\"get_async_reply\","
                            "\"set_async\","
                            "\"meter_mod\""
                            "]}"
                           };

  // Normal case
  {
    // Return OK
    ret = s_parse_log(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);

    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_log error.");
    TEST_DSTRING(ret, &result, str1, test_str1, true);

    ret = s_parse_log(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);

    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_log error.");
    TEST_DSTRING(ret, &result, str2, test_str2, true);
  }

  free((void *) str1);
  free((void *) str2);
}

void
test_log_cmd_parse_unset_trace_flags_02(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;
  char *str1 = NULL;
  char *str2 = NULL;
  const char *argv1[] = {"log", "-packetdump",
                         "~hello",
                         "~error",
                         "~echo_request",
                         "~echo_reply",
                         "~experimenter",
                         "~features_request",
                         "~features_reply",
                         "~get_config_request",
                         "~get_config_reply",
                         "~set_config",
                         "~packet_in",
                         "~flow_removed",
                         "~port_status",
                         "~packet_out",
                         "~flow_mod",
                         "~group_mod",
                         "~port_mod",
                         "~table_mod",
                         "~multipart_request",
                         "~multipart_reply",
                         "~barrier_request",
                         "~barrier_reply",
                         "~queue_get_config_request",
                         "~queue_get_config_reply",
                         "~role_request",
                         "~role_reply",
                         "~get_async_request",
                         "~get_async_reply",
                         "~set_async",
                         "~meter_mod",
                         NULL
                        };
  const char *argv2[] = {"log", "-packetdump", NULL};
  const char test_str1[] = "{\"ret\":\"OK\"}";
  const char test_str2[] = "{\"ret\":\"OK\",\n\"data\":["
                           "\"\""
                           "]}";

  // Normal case
  {
    // Return OK
    ret = s_parse_log(&interp, state, ARGV_SIZE(argv1), argv1, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);

    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_log error.");
    TEST_DSTRING(ret, &result, str1, test_str1, true);

    ret = s_parse_log(&interp, state, ARGV_SIZE(argv2), argv2, &tbl,
                      proc, NULL, NULL, NULL,
                      &result);

    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "s_parse_log error.");
    TEST_DSTRING(ret, &result, str2, test_str2, true);
  }

  free((void *) str1);
  free((void *) str2);
}

void
test_log_cmd_serialize_file(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;

  const char *filepath = "/tmp/log_cmd_test";

  const char *ser_argv1[] = {"log", "-file", filepath,
                             "-debuglevel", "0", NULL
                            };

  const char *ser_argv2[] = {"log", "-packetdump",
                             "~hello",
                             "~error",
                             "~echo_request",
                             "~echo_reply",
                             "~experimenter",
                             "~features_request",
                             "~features_reply",
                             "~get_config_request",
                             "~get_config_reply",
                             "~set_config",
                             "~packet_in",
                             "~flow_removed",
                             "~port_status",
                             "~packet_out",
                             "~flow_mod",
                             "~group_mod",
                             "~port_mod",
                             "~table_mod",
                             "~multipart_request",
                             "~multipart_reply",
                             "~barrier_request",
                             "~barrier_reply",
                             "~queue_get_config_request",
                             "~queue_get_config_reply",
                             "~role_request",
                             "~role_reply",
                             "~get_async_request",
                             "~get_async_reply",
                             "~set_async",
                             "~meter_mod",
                             NULL
                            };

  const char test_str1[] = "log -file /tmp/log_cmd_test -debuglevel 0\n"
                           "log -packetdump \"\"\n\n";

  char *str1 = NULL;

  /* set file value. */
  ret = s_parse_log(&interp, state, ARGV_SIZE(ser_argv1), ser_argv1, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_log error.");
  lagopus_dstring_clear(&result);

  /* set packetdump. */
  ret = s_parse_log(&interp, state, ARGV_SIZE(ser_argv2), ser_argv2, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_log error.");
  lagopus_dstring_clear(&result);

  /* TEST : serialize. */
  ret = log_cmd_serialize(&result);
  (void)remove(filepath);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "log_cmd_serialize error.");
  TEST_DSTRING_NO_JSON(ret, &result, str1, test_str1, true);
}

void
test_log_cmd_serialize_syslog(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;

  const char *ser_argv1[] = {"log", "-syslog", "-ident", "test_ident",
                             "-debuglevel", "0", NULL
                            };

  const char *ser_argv2[] = {"log", "-packetdump",
                             "~hello",
                             "~error",
                             "~echo_request",
                             "~echo_reply",
                             "~experimenter",
                             "~features_request",
                             "~features_reply",
                             "~get_config_request",
                             "~get_config_reply",
                             "~set_config",
                             "~packet_in",
                             "~flow_removed",
                             "~port_status",
                             "~packet_out",
                             "~flow_mod",
                             "~group_mod",
                             "~port_mod",
                             "~table_mod",
                             "~multipart_request",
                             "~multipart_reply",
                             "~barrier_request",
                             "~barrier_reply",
                             "~queue_get_config_request",
                             "~queue_get_config_reply",
                             "~role_request",
                             "~role_reply",
                             "~get_async_request",
                             "~get_async_reply",
                             "~set_async",
                             "~meter_mod",
                             NULL
                            };

  const char test_str1[] = "log -syslog -ident test_ident -debuglevel 0\n"
                           "log -packetdump \"\"\n\n";

  char *str1 = NULL;

  /* set syslog value. */
  ret = s_parse_log(&interp, state, ARGV_SIZE(ser_argv1), ser_argv1, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_log error.");
  lagopus_dstring_clear(&result);

  /* set packetdump. */
  ret = s_parse_log(&interp, state, ARGV_SIZE(ser_argv2), ser_argv2, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_log error.");
  lagopus_dstring_clear(&result);

  /* TEST : serialize. */
  ret = log_cmd_serialize(&result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "log_cmd_serialize error.");
  TEST_DSTRING_NO_JSON(ret, &result, str1, test_str1, true);
}

void
test_log_cmd_serialize_syslog_escape(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;

  const char *ser_argv1[] = {"log", "-syslog", "-ident", "test_\"ident",
                             "-debuglevel", "0", NULL
                            };

  const char *ser_argv2[] = {"log", "-packetdump",
                             "~hello",
                             "~error",
                             "~echo_request",
                             "~echo_reply",
                             "~experimenter",
                             "~features_request",
                             "~features_reply",
                             "~get_config_request",
                             "~get_config_reply",
                             "~set_config",
                             "~packet_in",
                             "~flow_removed",
                             "~port_status",
                             "~packet_out",
                             "~flow_mod",
                             "~group_mod",
                             "~port_mod",
                             "~table_mod",
                             "~multipart_request",
                             "~multipart_reply",
                             "~barrier_request",
                             "~barrier_reply",
                             "~queue_get_config_request",
                             "~queue_get_config_reply",
                             "~role_request",
                             "~role_reply",
                             "~get_async_request",
                             "~get_async_reply",
                             "~set_async",
                             "~meter_mod",
                             NULL
                            };

  const char test_str1[] =
    "log -syslog -ident \"test_\\\"ident\" -debuglevel 0\n"
    "log -packetdump \"\"\n\n";

  char *str1 = NULL;

  /* set syslog value. */
  ret = s_parse_log(&interp, state, ARGV_SIZE(ser_argv1), ser_argv1, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_log error.");
  lagopus_dstring_clear(&result);

  /* set packetdump. */
  ret = s_parse_log(&interp, state, ARGV_SIZE(ser_argv2), ser_argv2, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_log error.");
  lagopus_dstring_clear(&result);

  /* TEST : serialize. */
  ret = log_cmd_serialize(&result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "log_cmd_serialize error.");
  TEST_DSTRING_NO_JSON(ret, &result, str1, test_str1, true);
}

void
test_log_cmd_serialize_syslog_escape_white_space(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;

  const char *ser_argv1[] = {"log", "-syslog", "-ident", "test ident",
                             "-debuglevel", "0", NULL
                            };

  const char *ser_argv2[] = {"log", "-packetdump",
                             "~hello",
                             "~error",
                             "~echo_request",
                             "~echo_reply",
                             "~experimenter",
                             "~features_request",
                             "~features_reply",
                             "~get_config_request",
                             "~get_config_reply",
                             "~set_config",
                             "~packet_in",
                             "~flow_removed",
                             "~port_status",
                             "~packet_out",
                             "~flow_mod",
                             "~group_mod",
                             "~port_mod",
                             "~table_mod",
                             "~multipart_request",
                             "~multipart_reply",
                             "~barrier_request",
                             "~barrier_reply",
                             "~queue_get_config_request",
                             "~queue_get_config_reply",
                             "~role_request",
                             "~role_reply",
                             "~get_async_request",
                             "~get_async_reply",
                             "~set_async",
                             "~meter_mod",
                             NULL
                            };

  const char test_str1[] = "log -syslog -ident \"test ident\" -debuglevel 0\n"
                           "log -packetdump \"\"\n\n";

  char *str1 = NULL;

  /* set syslog value. */
  ret = s_parse_log(&interp, state, ARGV_SIZE(ser_argv1), ser_argv1, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_log error.");
  lagopus_dstring_clear(&result);

  /* set packetdump. */
  ret = s_parse_log(&interp, state, ARGV_SIZE(ser_argv2), ser_argv2, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_log error.");
  lagopus_dstring_clear(&result);

  /* TEST : serialize. */
  ret = log_cmd_serialize(&result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "log_cmd_serialize error.");
  TEST_DSTRING_NO_JSON(ret, &result, str1, test_str1, true);
}

void
test_log_cmd_serialize_debuglevel(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;

  const char *filepath = "/tmp/log_cmd_test";

  const char *ser_argv1[] = {"log", "-file", filepath,
                             "-debuglevel", "43210", NULL
                            };

  const char *ser_argv2[] = {"log", "-packetdump",
                             "~hello",
                             "~error",
                             "~echo_request",
                             "~echo_reply",
                             "~experimenter",
                             "~features_request",
                             "~features_reply",
                             "~get_config_request",
                             "~get_config_reply",
                             "~set_config",
                             "~packet_in",
                             "~flow_removed",
                             "~port_status",
                             "~packet_out",
                             "~flow_mod",
                             "~group_mod",
                             "~port_mod",
                             "~table_mod",
                             "~multipart_request",
                             "~multipart_reply",
                             "~barrier_request",
                             "~barrier_reply",
                             "~queue_get_config_request",
                             "~queue_get_config_reply",
                             "~role_request",
                             "~role_reply",
                             "~get_async_request",
                             "~get_async_reply",
                             "~set_async",
                             "~meter_mod",
                             NULL
                            };

  const char test_str1[] = "log -file /tmp/log_cmd_test -debuglevel 43210\n"
                           "log -packetdump \"\"\n\n";

  char *str1 = NULL;

  /* set debuglevel value. */
  ret = s_parse_log(&interp, state, ARGV_SIZE(ser_argv1), ser_argv1, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_log error.");
  lagopus_dstring_clear(&result);

  /* set packetdump. */
  ret = s_parse_log(&interp, state, ARGV_SIZE(ser_argv2), ser_argv2, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_log error.");
  lagopus_dstring_clear(&result);

  /* TEST : serialize. */
  ret = log_cmd_serialize(&result);
  (void)remove(filepath);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "log_cmd_serialize error.");
  TEST_DSTRING_NO_JSON(ret, &result, str1, test_str1, true);
}

void
test_log_cmd_serialize_packetdump(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  datastore_interp_state_t state = DATASTORE_INTERP_STATE_AUTO_COMMIT;

  const char *filepath = "/tmp/log_cmd_test";

  const char *ser_argv1[] = {"log", "-file", filepath,
                             "-debuglevel", "0", NULL
                            };

  const char *ser_argv2[] = {"log", "-packetdump",
                             "+hello",
                             "+error",
                             "+echo_request",
                             "+echo_reply",
                             "+experimenter",
                             "+features_request",
                             "+features_reply",
                             "+get_config_request",
                             "+get_config_reply",
                             "+set_config",
                             "+packet_in",
                             "+flow_removed",
                             "+port_status",
                             "+packet_out",
                             "+flow_mod",
                             "+group_mod",
                             "+port_mod",
                             "+table_mod",
                             "+multipart_request",
                             "+multipart_reply",
                             "+barrier_request",
                             "+barrier_reply",
                             "+queue_get_config_request",
                             "+queue_get_config_reply",
                             "+role_request",
                             "+role_reply",
                             "+get_async_request",
                             "+get_async_reply",
                             "+set_async",
                             "+meter_mod",
                             NULL
                            };

  const char test_str1[] = "log -file /tmp/log_cmd_test -debuglevel 0\n"
                           "log -packetdump \"hello\" \"error\" "
                           "\"echo_request\" \"echo_reply\" \"experimenter\" "
                           "\"features_request\" \"features_reply\" "
                           "\"get_config_request\" \"get_config_reply\" "
                           "\"set_config\" \"packet_in\" "
                           "\"flow_removed\" \"port_status\" "
                           "\"packet_out\" \"flow_mod\" \"group_mod\" "
                           "\"port_mod\" \"table_mod\" \"multipart_request\" "
                           "\"multipart_reply\" \"barrier_request\" "
                           "\"barrier_reply\" \"queue_get_config_request\" "
                           "\"queue_get_config_reply\" \"role_request\" "
                           "\"role_reply\" \"get_async_request\" "
                           "\"get_async_reply\" \"set_async\" \"meter_mod\""
                           "\n\n";

  char *str1 = NULL;

  /* set value. */
  ret = s_parse_log(&interp, state, ARGV_SIZE(ser_argv1), ser_argv1, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_log error.");
  lagopus_dstring_clear(&result);

  /* set packetdump. */
  ret = s_parse_log(&interp, state, ARGV_SIZE(ser_argv2), ser_argv2, &tbl,
                    proc, NULL, NULL, NULL,
                    &result);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "s_parse_log error.");
  lagopus_dstring_clear(&result);

  /* TEST : serialize. */
  ret = log_cmd_serialize(&result);
  (void)remove(filepath);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret, "log_cmd_serialize error.");
  TEST_DSTRING_NO_JSON(ret, &result, str1, test_str1, true);
}

void
test_log_destroy_tables(void) {
  log_finalize();
  TEST_ASSERT_NULL(log_str_to_typ_table);
  TEST_ASSERT_NULL(log_typ_to_str_table);
}

void
test_destroy(void) {
  destroy = true;
}
