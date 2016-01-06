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
#include "../policer_action.c"
#define CAST_UINT64(x) (uint64_t) x

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_policer_action_initialize_and_finalize(void) {
  policer_action_initialize();
  policer_action_finalize();
}

void
test_policer_action_conf_create_and_destroy(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf = NULL;
  const char *name = "policer_action_name";

  policer_action_initialize();

  // Normal case
  {
    rc = policer_action_conf_create(&conf, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf,
                                 "conf_create() will create new policer_action");

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
    rc = policer_action_conf_create(NULL, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_action_conf_create(&conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_action_conf_destroy(conf);

  policer_action_finalize();
}

void
test_policer_action_conf_duplicate(void) {
  lagopus_result_t rc;
  const char *ns1 = "ns1";
  const char *ns2 = "ns2";
  const char *name = "policer-action";
  char *policer_action_fullname = NULL;
  bool result = false;
  policer_action_conf_t *src_conf = NULL;
  policer_action_conf_t *dst_conf = NULL;
  policer_action_conf_t *actual_conf = NULL;

  policer_action_initialize();

  // Normal case1(no namespace)
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &policer_action_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = policer_action_conf_create(&src_conf, policer_action_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new policer_action");
      free(policer_action_fullname);
      policer_action_fullname = NULL;
    }

    rc = policer_action_conf_duplicate(src_conf, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns1, name, &policer_action_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = policer_action_conf_create(&actual_conf, policer_action_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new policer_action");
      free(policer_action_fullname);
      policer_action_fullname = NULL;
    }

    result = policer_action_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    policer_action_conf_destroy(src_conf);
    src_conf = NULL;
    policer_action_conf_destroy(dst_conf);
    dst_conf = NULL;
    policer_action_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Normal case2
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &policer_action_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = policer_action_conf_create(&src_conf, policer_action_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new policer_action");
      free(policer_action_fullname);
      policer_action_fullname = NULL;
    }

    rc = policer_action_conf_duplicate(src_conf, &dst_conf, ns2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns2, name, &policer_action_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = policer_action_conf_create(&actual_conf, policer_action_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new policer_action");
      free(policer_action_fullname);
      policer_action_fullname = NULL;
    }

    result = policer_action_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    policer_action_conf_destroy(src_conf);
    src_conf = NULL;
    policer_action_conf_destroy(dst_conf);
    dst_conf = NULL;
    policer_action_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Abnormal case
  {
    rc = policer_action_conf_duplicate(NULL, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_action_finalize();
}

void
test_policer_action_conf_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  policer_action_conf_t *conf1 = NULL;
  policer_action_conf_t *conf2 = NULL;
  policer_action_conf_t *conf3 = NULL;
  const char *fullname1 = "conf1";
  const char *fullname2 = "conf2";
  const char *fullname3 = "conf3";

  policer_action_initialize();

  rc = policer_action_conf_create(&conf1, fullname1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf1,
                               "conf_create() will create new policer_action");
  rc = policer_action_conf_add(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = policer_action_conf_create(&conf2, fullname2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf2,
                               "conf_create() will create new policer_action");
  rc = policer_action_conf_add(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = policer_action_conf_create(&conf3, fullname3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf3,
                               "conf_create() will create new policer_action");
  rc = policer_action_conf_add(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = policer_action_set_enabled(fullname3, true);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = policer_action_conf_equals(conf1, conf2);
    TEST_ASSERT_TRUE(result);

    result = policer_action_conf_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = policer_action_conf_equals(conf1, conf3);
    TEST_ASSERT_FALSE(result);

    result = policer_action_conf_equals(conf2, conf3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = policer_action_conf_equals(conf1, NULL);
    TEST_ASSERT_FALSE(result);

    result = policer_action_conf_equals(NULL, conf2);
    TEST_ASSERT_FALSE(result);
  }

  rc = policer_action_conf_delete(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = policer_action_conf_delete(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = policer_action_conf_delete(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  policer_action_finalize();
}

void
test_policer_action_conf_add(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf = NULL;
  const char *name = "policer_action_name";
  policer_action_conf_t *actual_conf = NULL;

  policer_action_initialize();

  rc = policer_action_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf,
                               "conf_create() will create new policer_action");

  // Normal case
  {
    rc = policer_action_conf_add(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = policer_action_conf_add(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = policer_action_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  policer_action_finalize();
}

void
test_policer_action_conf_add_not_initialize(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf = NULL;
  const char *name = "policer_action_name";

  rc = policer_action_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf,
                               "conf_create() will create new policer_action");

  rc = policer_action_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  policer_action_conf_destroy(conf);

  policer_action_finalize();
}

void
test_policer_action_conf_delete(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf = NULL;
  const char *name = "policer_action_name";
  policer_action_conf_t *actual_conf = NULL;

  policer_action_initialize();

  rc = policer_action_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf,
                               "conf_create() will create new policer_action");

  rc = policer_action_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    rc = policer_action_conf_delete(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = policer_action_conf_delete(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = policer_action_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, rc);

  policer_action_finalize();
}

void
test_policer_action_conf_delete_not_initialize(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf = NULL;
  const char *name = "policer_action_name";

  rc = policer_action_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf,
                               "conf_create() will create new policer_action");

  rc = policer_action_conf_delete(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  policer_action_conf_destroy(conf);

  policer_action_finalize();
}

void
test_policer_action_conf_iterate(void) {
  TEST_IGNORE();
}

void
test_policer_action_conf_list(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf1 = NULL;
  const char *name1 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"policer_action_name1";
  policer_action_conf_t *conf2 = NULL;
  const char *name2 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"policer_action_name2";
  policer_action_conf_t *conf3 = NULL;
  const char *name3 = "namespace2"DATASTORE_NAMESPACE_DELIMITER"policer_action_name3";
  policer_action_conf_t *conf4 = NULL;
  const char *name4 = DATASTORE_NAMESPACE_DELIMITER"policer_action_name4";
  policer_action_conf_t *conf5 = NULL;
  const char *name5 = DATASTORE_NAMESPACE_DELIMITER"policer_action_name5";
  policer_action_conf_t **actual_list = NULL;
  size_t i;

  policer_action_initialize();

  // create conf
  {
    rc = policer_action_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1,
                                 "conf_create() will create new policer_action");

    rc = policer_action_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2,
                                 "conf_create() will create new policer_action");

    rc = policer_action_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3,
                                 "conf_create() will create new policer_action");

    rc = policer_action_conf_create(&conf4, name4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf4,
                                 "conf_create() will create new policer_action");

    rc = policer_action_conf_create(&conf5, name5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf5,
                                 "conf_create() will create new policer_action");
  }

  // add conf
  {
    rc = policer_action_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = policer_action_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = policer_action_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = policer_action_conf_add(conf4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = policer_action_conf_add(conf5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // all
  {
    rc = policer_action_conf_list(&actual_list, NULL);
    TEST_ASSERT_EQUAL(5, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"policer_action_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"policer_action_name2") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace2"DATASTORE_NAMESPACE_DELIMITER"policer_action_name3") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"policer_action_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"policer_action_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // no namespace
  {
    rc = policer_action_conf_list(&actual_list, "");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"policer_action_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"policer_action_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // only namespace
  {
    rc = policer_action_conf_list(&actual_list, "namespace1");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"policer_action_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"policer_action_name2") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // Abnormal case
  {
    rc = policer_action_conf_list(NULL, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_action_finalize();
}

void
test_policer_action_conf_list_not_initialize(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf = NULL;
  const char *name = "policer_action_name";
  policer_action_conf_t ***list = NULL;

  rc = policer_action_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf,
                               "conf_create() will create new policer_action");

  rc = policer_action_conf_list(list, NULL);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  policer_action_conf_destroy(conf);

  policer_action_finalize();
}

void
test_policer_action_conf_one_list(void) {
  TEST_IGNORE();
}

void
test_policer_action_conf_one_list_not_initialize(void) {
  TEST_IGNORE();
}

void
test_policer_action_find(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf1 = NULL;
  const char *name1 = "policer_action_name1";
  policer_action_conf_t *conf2 = NULL;
  const char *name2 = "policer_action_name2";
  policer_action_conf_t *conf3 = NULL;
  const char *name3 = "policer_action_name3";
  policer_action_conf_t *actual_conf = NULL;

  policer_action_initialize();

  // create conf
  {
    rc = policer_action_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1,
                                 "conf_create() will create new policer_action");

    rc = policer_action_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2,
                                 "conf_create() will create new policer_action");

    rc = policer_action_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3,
                                 "conf_create() will create new policer_action");
  }

  // add conf
  {
    rc = policer_action_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = policer_action_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = policer_action_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Normal case
  {
    rc = policer_action_find(name1, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name1, actual_conf->name);

    rc = policer_action_find(name2, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name2, actual_conf->name);

    rc = policer_action_find(name3, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name3, actual_conf->name);

  }

  // Abnormal case
  {
    rc = policer_action_find(NULL, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_action_find(name1, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_action_finalize();
}

void
test_policer_action_find_not_initialize(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf = NULL;
  const char *name = "policer_action_name";
  policer_action_conf_t *actual_conf = NULL;

  rc = policer_action_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf,
                               "conf_create() will create new policer_action");

  rc = policer_action_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  policer_action_conf_destroy(conf);

  policer_action_finalize();
}

void
test_policer_action_attr_create_and_destroy(void) {
  lagopus_result_t rc;
  policer_action_attr_t *attr = NULL;
  datastore_policer_action_type_t actual_type = 0;
  const datastore_policer_action_type_t expected_type =
    DATASTORE_POLICER_ACTION_TYPE_UNKNOWN;

  policer_action_initialize();

  rc = policer_action_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr,
                               "attr_create() will create new policer_action");

  // default value
  {
    // type
    rc = policer_action_get_type(attr, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_type, actual_type);
  }

  policer_action_attr_destroy(attr);

  policer_action_finalize();
}

void
test_policer_action_attr_duplicate(void) {
  lagopus_result_t rc;
  bool result = false;
  policer_action_attr_t *src_attr = NULL;
  policer_action_attr_t *dst_attr = NULL;

  policer_action_initialize();

  rc = policer_action_attr_create(&src_attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(src_attr,
                               "attr_create() will create new policer_action");

  // Normal case
  {
    rc = policer_action_attr_duplicate(src_attr, &dst_attr, "namespace");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    result = policer_action_attr_equals(src_attr, dst_attr);
    TEST_ASSERT_TRUE(result);
    policer_action_attr_destroy(dst_attr);
    dst_attr = NULL;

    rc = policer_action_attr_duplicate(src_attr, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    result = policer_action_attr_equals(src_attr, dst_attr);
    TEST_ASSERT_TRUE(result);
    policer_action_attr_destroy(dst_attr);
    dst_attr = NULL;
  }

  // Abnormal case
  {
    result = policer_action_attr_equals(src_attr, NULL);
    TEST_ASSERT_FALSE(result);

    rc = policer_action_attr_duplicate(src_attr, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    result = policer_action_attr_equals(NULL, dst_attr);
    TEST_ASSERT_FALSE(result);
    policer_action_attr_destroy(dst_attr);
    dst_attr = NULL;
  }

  policer_action_attr_destroy(src_attr);

  policer_action_finalize();
}

void
test_policer_action_attr_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  policer_action_attr_t *attr1 = NULL;
  policer_action_attr_t *attr2 = NULL;
  policer_action_attr_t *attr3 = NULL;

  policer_action_initialize();

  rc = policer_action_attr_create(&attr1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr1,
                               "attr_create() will create new policer_action");

  rc = policer_action_attr_create(&attr2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr2,
                               "attr_create() will create new policer_action");

  rc = policer_action_attr_create(&attr3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr3,
                               "attr_create() will create new policer_action");
  rc = policer_action_set_type(attr3, DATASTORE_POLICER_ACTION_TYPE_DISCARD);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = policer_action_attr_equals(attr1, attr2);
    TEST_ASSERT_TRUE(result);

    result = policer_action_attr_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = policer_action_attr_equals(attr1, attr3);
    TEST_ASSERT_FALSE(result);

    result = policer_action_attr_equals(attr2, attr3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = policer_action_attr_equals(attr1, NULL);
    TEST_ASSERT_FALSE(result);

    result = policer_action_attr_equals(NULL, attr2);
    TEST_ASSERT_FALSE(result);
  }

  policer_action_attr_destroy(attr1);
  policer_action_attr_destroy(attr2);
  policer_action_attr_destroy(attr3);

  policer_action_finalize();
}

void
test_policer_action_get_attr(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf = NULL;
  policer_action_attr_t *attr = NULL;
  const char *name = "policer_action_name";

  policer_action_initialize();

  rc = policer_action_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf,
                               "conf_create() will create new policer_action");

  rc = policer_action_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = policer_action_get_attr(name, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = policer_action_get_attr(name, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(attr);
  }

  // Abnormal case of getter
  {
    rc = policer_action_get_attr(NULL, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_action_get_attr(NULL, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_action_get_attr(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_action_get_attr(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_action_finalize();
}

void
test_policer_action_conf_private_exists(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf = NULL;
  const char *name = "policer_action_name1";
  const char *invalid_name = "invalid_name";

  policer_action_initialize();

  rc = policer_action_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf,
                               "conf_create() will create new policer_action");

  rc = policer_action_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    TEST_ASSERT_TRUE(policer_action_exists(name) == true);
    TEST_ASSERT_TRUE(policer_action_exists(invalid_name) == false);
  }

  // Abnormal case
  {
    TEST_ASSERT_TRUE(policer_action_exists(NULL) == false);
  }

  policer_action_finalize();
}

void
test_policer_action_conf_private_used(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf = NULL;
  const char *name = "policer_action_name";
  bool actual_used = false;

  policer_action_initialize();

  rc = policer_action_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf,
                               "conf_create() will create new policer_action");

  rc = policer_action_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = policer_action_set_used(name, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_policer_action_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = policer_action_set_used(NULL, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_action_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_action_finalize();
}

void
test_policer_action_conf_public_used(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf = NULL;
  const char *name = "policer_action_name";
  bool actual_used = false;

  policer_action_initialize();

  rc = policer_action_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf,
                               "conf_create() will create new policer_action");

  rc = policer_action_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_policer_action_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = datastore_policer_action_is_used(NULL, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_action_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_action_finalize();
}

void
test_policer_action_conf_private_enabled(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf = NULL;
  const char *name = "policer_action_name";
  bool actual_enabled = false;

  policer_action_initialize();

  rc = policer_action_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf,
                               "conf_create() will create new policer_action");

  rc = policer_action_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = policer_action_set_enabled(name, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_policer_action_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = policer_action_set_enabled(NULL, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_action_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_action_finalize();
}

void
test_policer_action_conf_public_enabled(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf = NULL;
  const char *name = "policer_action_name";
  bool actual_enabled = false;

  policer_action_initialize();

  rc = policer_action_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf,
                               "conf_create() will create new policer_action");

  rc = policer_action_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_policer_action_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = datastore_policer_action_is_enabled(NULL, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_action_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_action_finalize();
}

void
test_policer_action_attr_private_type(void) {
  lagopus_result_t rc;
  policer_action_attr_t *attr = NULL;
  datastore_policer_action_type_t actual_type = 0;
  datastore_policer_action_type_t expected_type =
    DATASTORE_POLICER_ACTION_TYPE_UNKNOWN;
  datastore_policer_action_type_t set_type =
    DATASTORE_POLICER_ACTION_TYPE_DISCARD;
  datastore_policer_action_type_t actual_set_type = 0;
  datastore_policer_action_type_t expected_set_type = set_type;

  policer_action_initialize();

  rc = policer_action_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr,
                               "attr_create() will create new policer_action");

  // Normal case of getter
  {
    rc = policer_action_get_type(attr, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_type, actual_type);
  }

  // Abnormal case of getter
  {
    rc = policer_action_get_type(NULL, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = policer_action_get_type(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = policer_action_set_type(attr, set_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = policer_action_get_type(attr, &actual_set_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_type, actual_set_type);
  }

  // Abnormal case of setter
  {
    rc = policer_action_set_type(NULL, actual_set_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_action_attr_destroy(attr);

  policer_action_finalize();
}

void
test_policer_action_attr_public_type(void) {
  lagopus_result_t rc;
  policer_action_conf_t *conf = NULL;
  const char *name = "policer_action_name";
  datastore_policer_action_type_t actual_type = 0;
  datastore_policer_action_type_t expected_type =
    DATASTORE_POLICER_ACTION_TYPE_UNKNOWN;

  policer_action_initialize();

  rc = policer_action_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf,
                               "conf_create() will create new policer_action");

  rc = policer_action_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_policer_action_get_type(name, true, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_policer_action_get_type(name, false, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_type, actual_type);
  }

  // Abnormal case of getter
  {
    rc = datastore_policer_action_get_type(NULL, true, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_action_get_type(NULL, false, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_action_get_type(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_policer_action_get_type(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  policer_action_finalize();
}
