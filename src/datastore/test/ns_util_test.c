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
#include "../datastore_apis.h"
#include "../datastore_internal.h"
#include "../ns_util.h"

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_cmd_ns_create_fullname(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  // Normal case1(namespace exists)
  {
    size_t full_len = 0;
    size_t ns_len = 0;
    size_t delim_len = 0;
    size_t name_len = 0;
    char *buf = NULL;

    const char *ns = "ns";
    const char *name = "name";
    char *fullname = NULL;
    ret = ns_create_fullname(ns, name, &fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_fullname error.");

    ns_len = strlen(ns);
    delim_len = strlen(DATASTORE_NAMESPACE_DELIMITER);
    name_len = strlen(name);
    full_len = ns_len + delim_len + name_len;

    buf = (char *) malloc(sizeof(char) * (full_len + 1));
    ret = snprintf(buf, full_len + 1, "%s%s%s", ns, DATASTORE_NAMESPACE_DELIMITER, name);

    TEST_ASSERT_EQUAL_MESSAGE(0,
                              strcmp(buf, fullname), "string compare error.");

    free(fullname);
    free(buf);
  }

  // Normal case2(no namespace)
  {
    size_t full_len = 0;
    size_t delim_len = 0;
    size_t name_len = 0;
    char *buf = NULL;

    const char *ns = "";
    const char *name = "name";
    char *fullname = NULL;
    ret = ns_create_fullname(ns, name, &fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_fullname error.");

    delim_len = strlen(DATASTORE_NAMESPACE_DELIMITER);
    name_len = strlen(name);
    full_len = delim_len + name_len;

    buf = (char *) malloc(sizeof(char) * (full_len + 1));
    ret = snprintf(buf, full_len + 1, "%s%s", DATASTORE_NAMESPACE_DELIMITER, name);

    TEST_ASSERT_EQUAL_MESSAGE(0,
                              strcmp(buf, fullname), "string compare error.");

    free(fullname);
    free(buf);
  }

  // Abnormal case
  {
    const char *ns = "ns";
    const char *name = "name";
    char *fullname = NULL;

    // namespace is NULL
    ret = ns_create_fullname(NULL, name, &fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_create_fullname error.");

    // name is NULL
    ret = ns_create_fullname(ns, NULL, &fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_create_fullname error.");

    // name is empty
    ret = ns_create_fullname(ns, "", &fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_create_fullname error.");

    // fullname is NULL
    ret = ns_create_fullname(ns, name, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_create_fullname error.");
  }
}

void
test_cmd_ns_split_fullname(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  // Normal case1(namespace exists)
  {
    const char *ns = "ns";
    const char *name = "name";
    char *ret_ns = NULL;
    char *ret_name = NULL;
    char *fullname = NULL;
    ret = ns_create_fullname(ns, name, &fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_fullname error.");

    ret = ns_split_fullname(fullname, &ret_ns, &ret_name);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_namespace error.");
    TEST_ASSERT_EQUAL_MESSAGE(0,
                              strcmp(ns, ret_ns), "string compare error.");
    TEST_ASSERT_EQUAL_MESSAGE(0,
                              strcmp(name, ret_name), "string compare error.");

    free(ret_ns);
    free(ret_name);
    free(fullname);
  }

  // Normal case2(no namespace)
  {
    const char *ns = "";
    const char *name = "name";
    char *ret_ns = NULL;
    char *ret_name = NULL;
    char *fullname = NULL;
    ret = ns_create_fullname(ns, name, &fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_fullname error.");

    ret = ns_split_fullname(fullname, &ret_ns, &ret_name);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_namespace error.");
    TEST_ASSERT_EQUAL_MESSAGE(0,
                              strcmp(ns, ret_ns), "string compare error.");
    TEST_ASSERT_EQUAL_MESSAGE(0,
                              strcmp(name, ret_name), "string compare error.");

    free(ret_ns);
    free(ret_name);
    free(fullname);
  }

  // Abnormal case
  {
    const char *ns = "ns";
    const char *name = "name";
    char *ret_ns = NULL;
    char *ret_name = NULL;
    char *fullname = NULL;
    ret = ns_create_fullname(ns, name, &fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_fullname error.");

    // fullname is NULL
    ret = ns_split_fullname(NULL, &ret_ns, &ret_name);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_split_fullname error.");

    // fullname is empty
    ret = ns_split_fullname("", &ret_ns, &ret_name);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_split_fullname error.");

    // namespace is NULL
    ret = ns_split_fullname(fullname, NULL, &ret_name);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_split_fullname error.");

    // name is empty
    ret = ns_split_fullname(fullname, &ret_ns, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_split_fullname error.");

    free(fullname);
  }
}

void
test_cmd_ns_get_namespace(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  // Normal case1(namespace exists)
  {
    const char *ns = "ns";
    const char *name = "name";
    const char *current_ns = "current_ns";
    char *fullname = NULL;
    char *ret_ns = NULL;
    ret = ns_create_fullname(ns, name, &fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_fullname error.");
    ret = ns_get_namespace(fullname, current_ns, &ret_ns);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_namespace error.");
    TEST_ASSERT_EQUAL_MESSAGE(0,
                              strcmp(ns, ret_ns), "string compare error.");

    free(fullname);
    free(ret_ns);
  }

  // Normal case2(namespace exists)
  {
    const char *ns = "";
    const char *name = "name";
    const char *current_ns = "current_ns";
    char *fullname = NULL;
    char *ret_ns = NULL;
    ret = ns_create_fullname(ns, name, &fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_fullname error.");
    ret = ns_get_namespace(fullname, current_ns, &ret_ns);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_namespace error.");
    TEST_ASSERT_EQUAL_MESSAGE(0,
                              strcmp(ns, ret_ns), "string compare error.");

    free(fullname);
    free(ret_ns);
  }

  // Normal case3(no namespace)
  {
    const char *fullname = "name";
    const char *current_ns = "current_ns";
    char *ret_ns = NULL;
    ret = ns_get_namespace(fullname, current_ns, &ret_ns);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_namespace error.");
    TEST_ASSERT_EQUAL_MESSAGE(0,
                              strcmp(current_ns, ret_ns), "string compare error.");

    free(ret_ns);
  }

  // Normal case4(no namespace)
  {
    const char *fullname = "name";
    const char *current_ns = "";
    char *ret_ns = NULL;
    ret = ns_get_namespace(fullname, current_ns, &ret_ns);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_namespace error.");
    TEST_ASSERT_EQUAL_MESSAGE(0,
                              strcmp(current_ns, ret_ns), "string compare error.");

    free(ret_ns);
  }

  // Abnormal case
  {
    const char *current_ns = "current_ns";
    const char *fullname = "ns:name";
    char *ret_ns = NULL;

    // fullname is NULL
    ret = ns_get_namespace(NULL, current_ns, &ret_ns);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_get_namespace error.");

    // fullname is empty
    ret = ns_get_namespace("", current_ns, &ret_ns);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_get_namespace error.");

    // current namespace is NULL
    ret = ns_get_namespace(fullname, NULL, &ret_ns);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_get_namespace error.");

    // namespace is NULL
    ret = ns_get_namespace(fullname, current_ns, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_get_namespace error.");
  }
}

void
test_cmd_ns_replace_namespace(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  // Normal case1
  {
    const char *ns = "ns";
    const char *name = "name";
    const char *new_ns = "new_ns";
    char *fullname = NULL;
    char *new_fullname = NULL;
    char *ret_fullname = NULL;
    ret = ns_create_fullname(ns, name, &fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_fullname error.");
    ret = ns_create_fullname(new_ns, name, &new_fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_fullname error.");

    ret = ns_replace_namespace(fullname, new_ns, &ret_fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_replace_namespace error.");
    TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(new_fullname, ret_fullname),
                              "string compare error.");

    free(fullname);
    free(new_fullname);
    free(ret_fullname);
  }

  // Normal case2
  {
    const char *ns = "";
    const char *name = "name";
    const char *new_ns = "new_ns";
    char *fullname = NULL;
    char *new_fullname = NULL;
    char *ret_fullname = NULL;
    ret = ns_create_fullname(ns, name, &fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_fullname error.");
    ret = ns_create_fullname(new_ns, name, &new_fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_fullname error.");

    ret = ns_replace_namespace(fullname, new_ns, &ret_fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_replace_namespace error.");
    TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(new_fullname, ret_fullname),
                              "string compare error.");

    free(fullname);
    free(new_fullname);
    free(ret_fullname);
  }

  // Normal case3
  {
    const char *ns = "ns";
    const char *name = "name";
    const char *new_ns = "";
    char *fullname = NULL;
    char *new_fullname = NULL;
    char *ret_fullname = NULL;
    ret = ns_create_fullname(ns, name, &fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_fullname error.");
    ret = ns_create_fullname(new_ns, name, &new_fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_fullname error.");

    ret = ns_replace_namespace(fullname, new_ns, &ret_fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_replace_namespace error.");
    TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(new_fullname, ret_fullname),
                              "string compare error.");

    free(fullname);
    free(new_fullname);
    free(ret_fullname);
  }

  // Normal case4
  {
    const char *ns = "";
    const char *name = "name";
    const char *new_ns = "";
    char *fullname = NULL;
    char *new_fullname = NULL;
    char *ret_fullname = NULL;
    ret = ns_create_fullname(ns, name, &fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_fullname error.");
    ret = ns_create_fullname(new_ns, name, &new_fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_get_fullname error.");

    ret = ns_replace_namespace(fullname, new_ns, &ret_fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              ret, "cmd_ns_replace_namespace error.");
    TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(new_fullname, ret_fullname),
                              "string compare error.");

    free(fullname);
    free(new_fullname);
    free(ret_fullname);
  }

  // Abnormal case
  {
    const char *fullname = "ns:name";
    const char *ns = "ns";
    char *new_fullname = NULL;

    // fullname is NULL
    ret = ns_replace_namespace(NULL, ns, &new_fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_get_namespace error.");

    // fullname is empty
    ret = ns_replace_namespace("", ns, &new_fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_get_namespace error.");

    // namespace is NULL
    ret = ns_replace_namespace(fullname, NULL, &new_fullname);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_get_namespace error.");

    // new fullname is empty
    ret = ns_replace_namespace(fullname, ns, NULL);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_INVALID_ARGS,
                              ret, "cmd_ns_get_namespace error.");
  }
}

