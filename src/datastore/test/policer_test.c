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
#include "../policer.c"
#include "../ns_util.h"
#define CAST_UINT64(x) (uint64_t) x

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_policer_initialize_and_finalize(void) {
  policer_initialize();
  policer_finalize();
}

void
test_policer_attr_action_name_exists(void) {
  lagopus_result_t rc;
  bool ret = false;
  policer_attr_t *attr = NULL;
  const char *name1 = "policer3";
  const char *name2 = "invalid_policer";

  policer_initialize();

  rc = policer_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new policer");

  rc = policer_attr_add_action_name(attr, "policer1");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = policer_attr_add_action_name(attr, "policer2");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = policer_attr_add_action_name(attr, "policer3");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of setter
  {
    ret = policer_attr_action_name_exists(attr, name1);
    TEST_ASSERT_TRUE(ret);

    ret = policer_attr_action_name_exists(attr, name2);
    TEST_ASSERT_FALSE(ret);
  }

  // Abnormal case of setter
  {
    ret = policer_attr_action_name_exists(NULL, name1);
    TEST_ASSERT_FALSE(ret);

    ret = policer_attr_action_name_exists(attr, NULL);
    TEST_ASSERT_FALSE(ret);
  }

  policer_attr_destroy(attr);

  policer_finalize();
}

void
test_policer_conf_create_and_destroy(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name";

  policer_initialize();

  // Normal case
  {
    rc = policer_conf_create(&conf, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

    // name
    TEST_ASSERT_EQUAL_STRING(name, conf->name);

    // current_attr
    TEST_ASSERT_NULL(conf->current_attr);

    // modified_attr
    TEST_ASSERT_NOT_NULL(conf->modified_attr);

    // enabled
    TEST_ASSERT_FALSE(conf->is_enabled);

    // used
    TEST_ASSERT_FALSE(conf->is_used);
  }

  // Abnormal case
  {
    rc = policer_conf_create(NULL, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_conf_create(&conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_conf_destroy(conf);

  policer_finalize();
}

void
test_policer_conf_duplicate(void) {
  lagopus_result_t rc;
  const char *ns1 = "ns1";
  const char *ns2 = "ns2";
  const char *name = "policer";
  char *policer_fullname = NULL;
  bool result = false;
  policer_conf_t *src_conf = NULL;
  policer_conf_t *dst_conf = NULL;
  policer_conf_t *actual_conf = NULL;

  policer_initialize();

  // Normal case1(no namespace)
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &policer_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = policer_conf_create(&src_conf, policer_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new policer_action");
      free(policer_fullname);
      policer_fullname = NULL;
    }

    rc = policer_conf_duplicate(src_conf, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns1, name, &policer_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = policer_conf_create(&actual_conf, policer_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new policer_action");
      free(policer_fullname);
      policer_fullname = NULL;
    }

    result = policer_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    policer_conf_destroy(src_conf);
    src_conf = NULL;
    policer_conf_destroy(dst_conf);
    dst_conf = NULL;
    policer_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Normal case2
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &policer_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = policer_conf_create(&src_conf, policer_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new policer_action");
      free(policer_fullname);
      policer_fullname = NULL;
    }

    rc = policer_conf_duplicate(src_conf, &dst_conf, ns2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns2, name, &policer_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = policer_conf_create(&actual_conf, policer_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new policer_action");
      free(policer_fullname);
      policer_fullname = NULL;
    }

    result = policer_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    policer_conf_destroy(src_conf);
    src_conf = NULL;
    policer_conf_destroy(dst_conf);
    dst_conf = NULL;
    policer_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Abnormal case
  {
    rc = policer_conf_duplicate(NULL, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_finalize();
}

void
test_policer_conf_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  policer_conf_t *conf1 = NULL;
  policer_conf_t *conf2 = NULL;
  policer_conf_t *conf3 = NULL;
  const char *fullname1 = "conf1";
  const char *fullname2 = "conf2";
  const char *fullname3 = "conf3";

  policer_initialize();

  rc = policer_conf_create(&conf1, fullname1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new policer");
  rc = policer_conf_add(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = policer_conf_create(&conf2, fullname2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new policer");
  rc = policer_conf_add(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = policer_conf_create(&conf3, fullname3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new policer");
  rc = policer_conf_add(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = policer_set_enabled(fullname3, true);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = policer_conf_equals(conf1, conf2);
    TEST_ASSERT_TRUE(result);

    result = policer_conf_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = policer_conf_equals(conf1, conf3);
    TEST_ASSERT_FALSE(result);

    result = policer_conf_equals(conf2, conf3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = policer_conf_equals(conf1, NULL);
    TEST_ASSERT_FALSE(result);

    result = policer_conf_equals(NULL, conf2);
    TEST_ASSERT_FALSE(result);
  }

  rc = policer_conf_delete(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = policer_conf_delete(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = policer_conf_delete(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  policer_finalize();
}

void
test_policer_conf_add(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name";
  policer_conf_t *actual_conf = NULL;

  policer_initialize();

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  // Normal case
  {
    rc = policer_conf_add(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = policer_conf_add(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = policer_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  policer_finalize();
}

void
test_policer_conf_add_not_initialize(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name";

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  rc = policer_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  policer_conf_destroy(conf);

  policer_finalize();
}

void
test_policer_conf_delete(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name";
  policer_conf_t *actual_conf = NULL;

  policer_initialize();

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  rc = policer_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    rc = policer_conf_delete(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = policer_conf_delete(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = policer_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, rc);

  policer_finalize();
}

void
test_policer_conf_delete_not_initialize(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name";

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  rc = policer_conf_delete(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  policer_conf_destroy(conf);

  policer_finalize();
}

void
test_policer_conf_iterate(void) {
  TEST_IGNORE();
}

void
test_policer_conf_list(void) {
  lagopus_result_t rc;
  policer_conf_t *conf1 = NULL;
  const char *name1 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"policer_name1";
  policer_conf_t *conf2 = NULL;
  const char *name2 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"policer_name2";
  policer_conf_t *conf3 = NULL;
  const char *name3 = "namespace2"DATASTORE_NAMESPACE_DELIMITER"policer_name3";
  policer_conf_t *conf4 = NULL;
  const char *name4 = DATASTORE_NAMESPACE_DELIMITER"policer_name4";
  policer_conf_t *conf5 = NULL;
  const char *name5 = DATASTORE_NAMESPACE_DELIMITER"policer_name5";
  policer_conf_t **actual_list = NULL;
  size_t i;

  policer_initialize();

  // create conf
  {
    rc = policer_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new policer");

    rc = policer_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new policer");

    rc = policer_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new policer");

    rc = policer_conf_create(&conf4, name4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf4, "conf_create() will create new policer");

    rc = policer_conf_create(&conf5, name5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf5, "conf_create() will create new policer");
  }

  // add conf
  {
    rc = policer_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = policer_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = policer_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = policer_conf_add(conf4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = policer_conf_add(conf5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // all
  {
    rc = policer_conf_list(&actual_list, NULL);
    TEST_ASSERT_EQUAL(5, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"policer_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"policer_name2") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace2"DATASTORE_NAMESPACE_DELIMITER"policer_name3") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"policer_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"policer_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // no namespace
  {
    rc = policer_conf_list(&actual_list, "");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"policer_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"policer_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // only namespace
  {
    rc = policer_conf_list(&actual_list, "namespace1");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"policer_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"policer_name2") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // Abnormal case
  {
    rc = policer_conf_list(NULL, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_finalize();
}

void
test_policer_conf_list_not_initialize(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name";
  policer_conf_t ***list = NULL;

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  rc = policer_conf_list(list, NULL);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  policer_conf_destroy(conf);

  policer_finalize();
}

void
test_policer_conf_one_list(void) {
  TEST_IGNORE();
}

void
test_policer_conf_one_list_not_initialize(void) {
  TEST_IGNORE();
}

void
test_policer_find(void) {
  lagopus_result_t rc;
  policer_conf_t *conf1 = NULL;
  const char *name1 = "policer_name1";
  policer_conf_t *conf2 = NULL;
  const char *name2 = "policer_name2";
  policer_conf_t *conf3 = NULL;
  const char *name3 = "policer_name3";
  policer_conf_t *actual_conf = NULL;

  policer_initialize();

  // create conf
  {
    rc = policer_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new policer");

    rc = policer_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new policer");

    rc = policer_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new policer");
  }

  // add conf
  {
    rc = policer_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = policer_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = policer_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Normal case
  {
    rc = policer_find(name1, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name1, actual_conf->name);

    rc = policer_find(name2, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name2, actual_conf->name);

    rc = policer_find(name3, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name3, actual_conf->name);

  }

  // Abnormal case
  {
    rc = policer_find(NULL, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_find(name1, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_finalize();
}

void
test_policer_find_not_initialize(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name";
  policer_conf_t *actual_conf = NULL;

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  rc = policer_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  policer_conf_destroy(conf);

  policer_finalize();
}

void
test_policer_attr_create_and_destroy(void) {
  lagopus_result_t rc;
  policer_attr_t *attr = NULL;
  uint64_t actual_bandwidth_limit = 0;
  const uint64_t expected_bandwidth_limit =
    DEFAULT_BANDWIDTH_LIMIT;
  uint64_t actual_burst_size_limit = 0;
  const uint64_t expected_burst_size_limit =
    DEFAULT_BURST_SIZE_LIMIT;
  uint8_t actual_bandwidth_percent = 0;
  const uint64_t expected_bandwidth_percent =
    DEFAULT_BANDWIDTH_PERCENT;
  datastore_name_info_t *actual_action_names = NULL;
  struct datastore_name_entry *expected_action_entry = NULL;

  policer_initialize();

  rc = policer_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new policer");

  // default value
  {
    // bandwidth_limit
    rc = policer_get_bandwidth_limit(attr, &actual_bandwidth_limit);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_bandwidth_limit,
                             actual_bandwidth_limit);

    // burst_size_limit
    rc = policer_get_burst_size_limit(attr, &actual_burst_size_limit);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_burst_size_limit,
                             actual_burst_size_limit);

    // bandwidth_percent
    rc = policer_get_bandwidth_percent(attr, &actual_bandwidth_percent);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(expected_bandwidth_percent,
                            actual_bandwidth_percent);

    // action_names
    rc = policer_get_action_names(attr, &actual_action_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(0, actual_action_names->size);
    expected_action_entry = TAILQ_FIRST(&(actual_action_names->head));
    TEST_ASSERT_NULL(expected_action_entry);
  }

  policer_attr_destroy(attr);
  datastore_names_destroy(actual_action_names);

  policer_finalize();
}

void
test_policer_attr_duplicate(void) {
  lagopus_result_t rc;
  const char *ns1 = "ns1";
  const char *ns2 = "ns2";
  const char *name = "policer-action";
  char *policer_action_fullname = NULL;
  bool result = false;
  policer_attr_t *src_attr = NULL;
  policer_attr_t *dst_attr = NULL;
  policer_attr_t *actual_attr = NULL;

  policer_initialize();

  // Normal case1(no namespace)
  {
    // create src attribute
    {
      rc = policer_attr_create(&src_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_attr,
                                   "attr_create() will create new policer");
      rc = ns_create_fullname(ns1, name, &policer_action_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = policer_attr_add_action_name(src_attr, policer_action_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(policer_action_fullname);
      policer_action_fullname = NULL;
    }

    // duplicate
    rc = policer_attr_duplicate(src_attr, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual attribute
    {
      rc = policer_attr_create(&actual_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(actual_attr,
                                   "attr_create() will create new policer");
      rc = ns_create_fullname(ns1, name, &policer_action_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = policer_attr_add_action_name(actual_attr, policer_action_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(policer_action_fullname);
      policer_action_fullname = NULL;
    }

    result = policer_attr_equals(dst_attr, actual_attr);
    TEST_ASSERT_TRUE(result);

    policer_attr_destroy(src_attr);
    src_attr = NULL;
    policer_attr_destroy(dst_attr);
    dst_attr = NULL;
    policer_attr_destroy(actual_attr);
    actual_attr = NULL;
  }

  // Normal case2
  {
    // create src attribute
    {
      rc = policer_attr_create(&src_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_attr,
                                   "attr_create() will create new policer");
      rc = ns_create_fullname(ns1, name, &policer_action_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = policer_attr_add_action_name(src_attr, policer_action_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(policer_action_fullname);
      policer_action_fullname = NULL;
    }

    // duplicate
    rc = policer_attr_duplicate(src_attr, &dst_attr, ns2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual attribute
    {
      rc = policer_attr_create(&actual_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(actual_attr,
                                   "attr_create() will create new policer");
      rc = ns_create_fullname(ns2, name, &policer_action_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = policer_attr_add_action_name(actual_attr, policer_action_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(policer_action_fullname);
      policer_action_fullname = NULL;
    }

    result = policer_attr_equals(dst_attr, actual_attr);
    TEST_ASSERT_TRUE(result);

    policer_attr_destroy(src_attr);
    src_attr = NULL;
    policer_attr_destroy(dst_attr);
    dst_attr = NULL;
    policer_attr_destroy(actual_attr);
    actual_attr = NULL;
  }

  // Abnormal case
  {
    rc = policer_attr_duplicate(NULL, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_finalize();
}

void
test_policer_attr_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  policer_attr_t *attr1 = NULL;
  policer_attr_t *attr2 = NULL;
  policer_attr_t *attr3 = NULL;

  policer_initialize();

  rc = policer_attr_create(&attr1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr1, "attr_create() will create new policer");

  rc = policer_attr_create(&attr2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr2, "attr_create() will create new policer");

  rc = policer_attr_create(&attr3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr3, "attr_create() will create new policer");
  rc = policer_set_bandwidth_limit(attr3, 1501);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = policer_attr_equals(attr1, attr2);
    TEST_ASSERT_TRUE(result);

    result = policer_attr_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = policer_attr_equals(attr1, attr3);
    TEST_ASSERT_FALSE(result);

    result = policer_attr_equals(attr2, attr3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = policer_attr_equals(attr1, NULL);
    TEST_ASSERT_FALSE(result);

    result = policer_attr_equals(NULL, attr2);
    TEST_ASSERT_FALSE(result);
  }

  policer_attr_destroy(attr1);
  policer_attr_destroy(attr2);
  policer_attr_destroy(attr3);

  policer_finalize();
}

void
test_policer_get_attr(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  policer_attr_t *attr = NULL;
  const char *name = "policer_name";

  policer_initialize();

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  rc = policer_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = policer_get_attr(name, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = policer_get_attr(name, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(attr);
  }

  // Abnormal case of getter
  {
    rc = policer_get_attr(NULL, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_get_attr(NULL, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_get_attr(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_get_attr(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_finalize();
}

void
test_policer_conf_private_exists(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name1";
  const char *invalid_name = "invalid_name";

  policer_initialize();

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  rc = policer_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    TEST_ASSERT_TRUE(policer_exists(name) == true);
    TEST_ASSERT_TRUE(policer_exists(invalid_name) == false);
  }

  // Abnormal case
  {
    TEST_ASSERT_TRUE(policer_exists(NULL) == false);
  }

  policer_finalize();
}

void
test_policer_conf_private_used(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name";
  bool actual_used = false;

  policer_initialize();

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  rc = policer_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = policer_set_used(name, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_policer_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = policer_set_used(NULL, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_finalize();
}

void
test_policer_conf_public_used(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name";
  bool actual_used = false;

  policer_initialize();

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  rc = policer_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_policer_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = datastore_policer_is_used(NULL, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_finalize();
}

void
test_policer_conf_private_enabled(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name";
  bool actual_enabled = false;

  policer_initialize();

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  rc = policer_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = policer_set_enabled(name, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_policer_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = policer_set_enabled(NULL, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_finalize();
}

void
test_policer_conf_public_enabled(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name";
  bool actual_enabled = false;

  policer_initialize();

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  rc = policer_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_policer_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = datastore_policer_is_enabled(NULL, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_finalize();
}

void
test_policer_attr_private_bandwidth_limit(void) {
  lagopus_result_t rc;
  policer_attr_t *attr = NULL;
  uint64_t actual_bandwidth_limit = 0;
  const uint64_t expected_bandwidth_limit = DEFAULT_BANDWIDTH_LIMIT;
  const uint64_t set_bandwidth_limit1 = CAST_UINT64(MAXIMUM_BANDWIDTH_LIMIT);
  const uint64_t set_bandwidth_limit2 = CAST_UINT64(MINIMUM_BANDWIDTH_LIMIT);
  const uint64_t set_bandwidth_limit3 = CAST_UINT64(MAXIMUM_BANDWIDTH_LIMIT + 1);
  const int set_bandwidth_limit4 = -1;
  uint64_t actual_set_bandwidth_limit1 = 0;
  uint64_t actual_set_bandwidth_limit2 = 0;
  uint64_t actual_set_bandwidth_limit3 = 0;
  uint64_t actual_set_bandwidth_limit4 = 0;
  const uint64_t expected_set_bandwidth_limit1 = MAXIMUM_BANDWIDTH_LIMIT;
  const uint64_t expected_set_bandwidth_limit2 = MINIMUM_BANDWIDTH_LIMIT;
  const uint64_t expected_set_bandwidth_limit3 = MINIMUM_BANDWIDTH_LIMIT;
  const uint64_t expected_set_bandwidth_limit4 = MINIMUM_BANDWIDTH_LIMIT;

  policer_initialize();

  rc = policer_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new policer");

  // Normal case of getter
  {
    rc = policer_get_bandwidth_limit(attr, &actual_bandwidth_limit);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_bandwidth_limit, actual_bandwidth_limit);
  }

  // Abnormal case of getter
  {
    rc = policer_get_bandwidth_limit(NULL, &actual_bandwidth_limit);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_get_bandwidth_limit(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = policer_set_bandwidth_limit(attr, set_bandwidth_limit1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = policer_get_bandwidth_limit(attr, &actual_set_bandwidth_limit1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_bandwidth_limit1,
                             actual_set_bandwidth_limit1);

    rc = policer_set_bandwidth_limit(attr, set_bandwidth_limit2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = policer_get_bandwidth_limit(attr, &actual_set_bandwidth_limit2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_bandwidth_limit2,
                             actual_set_bandwidth_limit2);
  }

  // Abnormal case of setter
  {
    rc = policer_set_bandwidth_limit(NULL, set_bandwidth_limit1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_set_bandwidth_limit(attr, CAST_UINT64(set_bandwidth_limit3));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = policer_get_bandwidth_limit(attr, &actual_set_bandwidth_limit3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_bandwidth_limit3,
                             actual_set_bandwidth_limit3);

    rc = policer_set_bandwidth_limit(attr, CAST_UINT64(set_bandwidth_limit4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = policer_get_bandwidth_limit(attr, &actual_set_bandwidth_limit4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_bandwidth_limit4,
                             actual_set_bandwidth_limit4);
  }

  policer_attr_destroy(attr);

  policer_finalize();
}

void
test_policer_attr_public_bandwidth_limit(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name";
  uint64_t actual_bandwidth_limit = 0;
  const uint64_t expected_bandwidth_limit = DEFAULT_BANDWIDTH_LIMIT;

  policer_initialize();

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  rc = policer_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_policer_get_bandwidth_limit(name, true,
         &actual_bandwidth_limit);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_policer_get_bandwidth_limit(name, false,
         &actual_bandwidth_limit);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_bandwidth_limit, actual_bandwidth_limit);
  }

  // Abnormal case of getter
  {
    rc = datastore_policer_get_bandwidth_limit(NULL, true,
         &actual_bandwidth_limit);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_get_bandwidth_limit(NULL, false,
         &actual_bandwidth_limit);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_get_bandwidth_limit(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_get_bandwidth_limit(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_finalize();
}

void
test_policer_attr_private_burst_size_limit(void) {
  lagopus_result_t rc;
  policer_attr_t *attr = NULL;
  uint64_t actual_burst_size_limit = 0;
  const uint64_t expected_burst_size_limit = DEFAULT_BURST_SIZE_LIMIT;
  const uint64_t set_burst_size_limit1 = CAST_UINT64(MAXIMUM_BURST_SIZE_LIMIT);
  const uint64_t set_burst_size_limit2 = CAST_UINT64(MINIMUM_BURST_SIZE_LIMIT);
  const uint64_t set_burst_size_limit3 = CAST_UINT64(MAXIMUM_BURST_SIZE_LIMIT +
                                         1);
  const int set_burst_size_limit4 = -1;
  uint64_t actual_set_burst_size_limit1 = 0;
  uint64_t actual_set_burst_size_limit2 = 0;
  uint64_t actual_set_burst_size_limit3 = 0;
  uint64_t actual_set_burst_size_limit4 = 0;
  const uint64_t expected_set_burst_size_limit1 = MAXIMUM_BURST_SIZE_LIMIT;
  const uint64_t expected_set_burst_size_limit2 = MINIMUM_BURST_SIZE_LIMIT;
  const uint64_t expected_set_burst_size_limit3 = MINIMUM_BURST_SIZE_LIMIT;
  const uint64_t expected_set_burst_size_limit4 = MINIMUM_BURST_SIZE_LIMIT;

  policer_initialize();

  rc = policer_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new policer");

  // Normal case of getter
  {
    rc = policer_get_burst_size_limit(attr, &actual_burst_size_limit);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_burst_size_limit, actual_burst_size_limit);
  }

  // Abnormal case of getter
  {
    rc = policer_get_burst_size_limit(NULL, &actual_burst_size_limit);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_get_burst_size_limit(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = policer_set_burst_size_limit(attr, set_burst_size_limit1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = policer_get_burst_size_limit(attr, &actual_set_burst_size_limit1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_burst_size_limit1,
                             actual_set_burst_size_limit1);

    rc = policer_set_burst_size_limit(attr, set_burst_size_limit2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = policer_get_burst_size_limit(attr, &actual_set_burst_size_limit2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_burst_size_limit2,
                             actual_set_burst_size_limit2);
  }

  // Abnormal case of setter
  {
    rc = policer_set_burst_size_limit(NULL, set_burst_size_limit1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_set_burst_size_limit(attr, CAST_UINT64(set_burst_size_limit3));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = policer_get_burst_size_limit(attr, &actual_set_burst_size_limit3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_burst_size_limit3,
                             actual_set_burst_size_limit3);

    rc = policer_set_burst_size_limit(attr, CAST_UINT64(set_burst_size_limit4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = policer_get_burst_size_limit(attr, &actual_set_burst_size_limit4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_burst_size_limit4,
                             actual_set_burst_size_limit4);
  }

  policer_attr_destroy(attr);

  policer_finalize();
}

void
test_policer_attr_public_burst_size_limit(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name";
  uint64_t actual_burst_size_limit = 0;
  const uint64_t expected_burst_size_limit = DEFAULT_BURST_SIZE_LIMIT;

  policer_initialize();

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  rc = policer_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_policer_get_burst_size_limit(name, true,
         &actual_burst_size_limit);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_policer_get_burst_size_limit(name, false,
         &actual_burst_size_limit);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_burst_size_limit, actual_burst_size_limit);
  }

  // Abnormal case of getter
  {
    rc = datastore_policer_get_burst_size_limit(NULL, true,
         &actual_burst_size_limit);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_get_burst_size_limit(NULL, false,
         &actual_burst_size_limit);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_get_burst_size_limit(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_get_burst_size_limit(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_finalize();
}

void
test_policer_attr_private_bandwidth_percent(void) {
  lagopus_result_t rc;
  policer_attr_t *attr = NULL;
  uint8_t actual_bandwidth_percent = 0;
  const uint8_t expected_bandwidth_percent = DEFAULT_BANDWIDTH_PERCENT;
  const uint64_t set_bandwidth_percent1 = CAST_UINT64(MAXIMUM_BANDWIDTH_PERCENT);
  const uint64_t set_bandwidth_percent2 = CAST_UINT64(MINIMUM_BANDWIDTH_PERCENT);
  const uint64_t set_bandwidth_percent3 = CAST_UINT64(MAXIMUM_BANDWIDTH_PERCENT +
                                          1);
  const int set_bandwidth_percent4 = -1;
  uint8_t actual_set_bandwidth_percent1 = 0;
  uint8_t actual_set_bandwidth_percent2 = 0;
  uint8_t actual_set_bandwidth_percent3 = 0;
  uint8_t actual_set_bandwidth_percent4 = 0;
  const uint8_t expected_set_bandwidth_percent1 = MAXIMUM_BANDWIDTH_PERCENT;
  const uint8_t expected_set_bandwidth_percent2 = MINIMUM_BANDWIDTH_PERCENT;
  const uint8_t expected_set_bandwidth_percent3 = MINIMUM_BANDWIDTH_PERCENT;
  const uint8_t expected_set_bandwidth_percent4 = MINIMUM_BANDWIDTH_PERCENT;

  policer_initialize();

  rc = policer_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new policer");

  // Normal case of getter
  {
    rc = policer_get_bandwidth_percent(attr, &actual_bandwidth_percent);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(expected_bandwidth_percent, actual_bandwidth_percent);
  }

  // Abnormal case of getter
  {
    rc = policer_get_bandwidth_percent(NULL, &actual_bandwidth_percent);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_get_bandwidth_percent(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = policer_set_bandwidth_percent(attr, set_bandwidth_percent1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = policer_get_bandwidth_percent(attr, &actual_set_bandwidth_percent1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(expected_set_bandwidth_percent1,
                            actual_set_bandwidth_percent1);

    rc = policer_set_bandwidth_percent(attr, set_bandwidth_percent2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = policer_get_bandwidth_percent(attr, &actual_set_bandwidth_percent2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(expected_set_bandwidth_percent2,
                            actual_set_bandwidth_percent2);
  }

  // Abnormal case of setter
  {
    rc = policer_set_bandwidth_percent(NULL, set_bandwidth_percent1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_set_bandwidth_percent(attr, CAST_UINT64(set_bandwidth_percent3));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = policer_get_bandwidth_percent(attr, &actual_set_bandwidth_percent3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(expected_set_bandwidth_percent3,
                            actual_set_bandwidth_percent3);

    rc = policer_set_bandwidth_percent(attr, CAST_UINT64(set_bandwidth_percent4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = policer_get_bandwidth_percent(attr, &actual_set_bandwidth_percent4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(expected_set_bandwidth_percent4,
                            actual_set_bandwidth_percent4);
  }

  policer_attr_destroy(attr);

  policer_finalize();
}

void
test_policer_attr_public_bandwidth_percent(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name";
  uint8_t actual_bandwidth_percent = DEFAULT_BANDWIDTH_PERCENT;
  const uint8_t expected_bandwidth_percent = actual_bandwidth_percent;

  policer_initialize();

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  rc = policer_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_policer_get_bandwidth_percent(name, true,
         &actual_bandwidth_percent);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_policer_get_bandwidth_percent(name, false,
         &actual_bandwidth_percent);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(expected_bandwidth_percent, actual_bandwidth_percent);
  }

  // Abnormal case of getter
  {
    rc = datastore_policer_get_bandwidth_percent(NULL, true,
         &actual_bandwidth_percent);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_get_bandwidth_percent(NULL, false,
         &actual_bandwidth_percent);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_get_bandwidth_percent(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_get_bandwidth_percent(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_finalize();
}

void
test_policer_attr_private_action_names(void) {
  lagopus_result_t rc;
  policer_attr_t *attr = NULL;
  datastore_name_info_t *actual_action_names = NULL;
  struct datastore_name_entry *expected_action_entry = NULL;

  policer_initialize();

  rc = policer_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new policer");

  // Normal case of add, getter, remove, remove all
  {
    // add
    rc = policer_attr_add_action_name(attr, "policer1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = policer_attr_add_action_name(attr, "policer2");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = policer_attr_add_action_name(attr, "policer3");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // getter
    rc = policer_get_action_names(attr, &actual_action_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(3, actual_action_names->size);
    TAILQ_FOREACH(expected_action_entry, &(actual_action_names->head),
                  name_entries) {
      if ((strcmp("policer1", expected_action_entry->str) != 0) &&
          (strcmp("policer2", expected_action_entry->str) != 0) &&
          (strcmp("policer3", expected_action_entry->str) != 0)) {
        TEST_FAIL();
      }
    }
    rc = datastore_names_destroy(actual_action_names);
    actual_action_names = NULL;
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // remove
    rc = policer_attr_remove_action_name(attr, "policer1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = policer_attr_remove_action_name(attr, "policer3");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = policer_get_action_names(attr, &actual_action_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(1, actual_action_names->size);
    expected_action_entry = TAILQ_FIRST(&(actual_action_names->head));
    TEST_ASSERT_NOT_NULL(expected_action_entry);
    TEST_ASSERT_EQUAL_STRING("policer2", expected_action_entry->str);
    rc = datastore_names_destroy(actual_action_names);
    actual_action_names = NULL;
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // remove all
    rc = policer_attr_remove_all_action_name(attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = policer_get_action_names(attr, &actual_action_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(0, actual_action_names->size);
    expected_action_entry = TAILQ_FIRST(&(actual_action_names->head));
    TEST_ASSERT_NULL(expected_action_entry);
    rc = datastore_names_destroy(actual_action_names);
    actual_action_names = NULL;
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case of getter
  {
    rc = policer_get_action_names(NULL, &actual_action_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_get_action_names(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case of add
  {
    rc = policer_attr_add_action_name(NULL, "policer1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_attr_add_action_name(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case of remove
  {
    rc = policer_attr_remove_action_name(NULL, "policer1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_attr_remove_action_name(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case of remove all
  {
    rc = policer_attr_remove_all_action_name(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_attr_destroy(attr);

  policer_finalize();
}

void
test_policer_attr_public_action_names(void) {
  lagopus_result_t rc;
  policer_conf_t *conf = NULL;
  const char *name = "policer_name";
  datastore_name_info_t *actual_action_names = NULL;
  struct datastore_name_entry *expected_action_entry = NULL;

  policer_initialize();

  rc = policer_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new policer");

  rc = policer_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_policer_get_action_names(name, true, &actual_action_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    // controller_names
    rc = datastore_policer_get_action_names(name, false, &actual_action_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(0, actual_action_names->size);
    expected_action_entry = TAILQ_FIRST(&(actual_action_names->head));
    TEST_ASSERT_NULL(expected_action_entry);
    rc = datastore_names_destroy(actual_action_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case of getter
  {
    rc = datastore_policer_get_action_names(NULL, true, &actual_action_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_get_action_names(NULL, false, &actual_action_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_get_action_names(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_get_action_names(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_finalize();
}
