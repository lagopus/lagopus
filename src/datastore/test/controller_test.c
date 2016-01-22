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
#include "../controller.c"

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_controller_initialize_and_finalize(void) {
  controller_initialize();
  controller_finalize();
}

void
test_controller_conf_create_and_destroy(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  const char *name = "controller_name";

  controller_initialize();

  // Normal case
  {
    rc = controller_conf_create(&conf, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

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
    rc = controller_conf_create(NULL, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_conf_create(&conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_conf_destroy(conf);

  controller_finalize();
}

void
test_controller_conf_duplicate(void) {
  lagopus_result_t rc;
  const char *ns1 = "ns1";
  const char *ns2 = "ns2";
  const char *name = "controller";
  char *controller_fullname = NULL;
  bool result = false;
  controller_conf_t *src_conf = NULL;
  controller_conf_t *dst_conf = NULL;
  controller_conf_t *actual_conf = NULL;

  controller_initialize();

  // Normal case1(no namespace)
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &controller_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = controller_conf_create(&src_conf, controller_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new controller_action");
      free(controller_fullname);
      controller_fullname = NULL;
    }

    rc = controller_conf_duplicate(src_conf, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns1, name, &controller_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = controller_conf_create(&actual_conf, controller_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new controller_action");
      free(controller_fullname);
      controller_fullname = NULL;
    }

    result = controller_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    controller_conf_destroy(src_conf);
    src_conf = NULL;
    controller_conf_destroy(dst_conf);
    dst_conf = NULL;
    controller_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Normal case2
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &controller_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = controller_conf_create(&src_conf, controller_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new controller_action");
      free(controller_fullname);
      controller_fullname = NULL;
    }

    rc = controller_conf_duplicate(src_conf, &dst_conf, ns2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns2, name, &controller_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = controller_conf_create(&actual_conf, controller_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new controller_action");
      free(controller_fullname);
      controller_fullname = NULL;
    }

    result = controller_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    controller_conf_destroy(src_conf);
    src_conf = NULL;
    controller_conf_destroy(dst_conf);
    dst_conf = NULL;
    controller_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Abnormal case
  {
    rc = controller_conf_duplicate(NULL, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_finalize();
}

void
test_controller_conf_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  controller_conf_t *conf1 = NULL;
  controller_conf_t *conf2 = NULL;
  controller_conf_t *conf3 = NULL;
  const char *fullname1 = "conf1";
  const char *fullname2 = "conf2";
  const char *fullname3 = "conf3";

  controller_initialize();

  rc = controller_conf_create(&conf1, fullname1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new controller");
  rc = controller_conf_add(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = controller_conf_create(&conf2, fullname2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new controller");
  rc = controller_conf_add(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = controller_conf_create(&conf3, fullname3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new controller");
  rc = controller_conf_add(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = controller_set_enabled(fullname3, true);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = controller_conf_equals(conf1, conf2);
    TEST_ASSERT_TRUE(result);

    result = controller_conf_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = controller_conf_equals(conf1, conf3);
    TEST_ASSERT_FALSE(result);

    result = controller_conf_equals(conf2, conf3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = controller_conf_equals(conf1, NULL);
    TEST_ASSERT_FALSE(result);

    result = controller_conf_equals(NULL, conf2);
    TEST_ASSERT_FALSE(result);
  }

  rc = controller_conf_delete(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = controller_conf_delete(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = controller_conf_delete(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  controller_finalize();
}

void
test_controller_conf_add(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  const char *name = "controller_name";
  controller_conf_t *actual_conf = NULL;

  controller_initialize();

  rc = controller_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

  // Normal case
  {
    rc = controller_conf_add(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = controller_conf_add(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = controller_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  controller_finalize();
}

void
test_controller_conf_add_not_initialize(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  const char *name = "controller_name";

  rc = controller_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

  rc = controller_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  controller_conf_destroy(conf);

  controller_finalize();
}

void
test_controller_conf_delete(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  const char *name = "controller_name";
  controller_conf_t *actual_conf = NULL;

  controller_initialize();

  rc = controller_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

  rc = controller_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    rc = controller_conf_delete(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = controller_conf_delete(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = controller_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, rc);

  controller_finalize();
}

void
test_controller_conf_delete_not_initialize(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  const char *name = "controller_name";

  rc = controller_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

  rc = controller_conf_delete(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  controller_conf_destroy(conf);

  controller_finalize();
}

void
test_controller_conf_iterate(void) {
  TEST_IGNORE();
}

void
test_controller_conf_list(void) {
  lagopus_result_t rc;
  controller_conf_t *conf1 = NULL;
  const char *name1 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"controller_name1";
  controller_conf_t *conf2 = NULL;
  const char *name2 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"controller_name2";
  controller_conf_t *conf3 = NULL;
  const char *name3 = "namespace2"DATASTORE_NAMESPACE_DELIMITER"controller_name3";
  controller_conf_t *conf4 = NULL;
  const char *name4 = DATASTORE_NAMESPACE_DELIMITER"controller_name4";
  controller_conf_t *conf5 = NULL;
  const char *name5 = DATASTORE_NAMESPACE_DELIMITER"controller_name5";
  controller_conf_t **actual_list = NULL;
  size_t i;

  controller_initialize();

  // create conf
  {
    rc = controller_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1,
                                 "conf_create() will create new controller");

    rc = controller_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2,
                                 "conf_create() will create new controller");

    rc = controller_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3,
                                 "conf_create() will create new controller");

    rc = controller_conf_create(&conf4, name4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf4,
                                 "conf_create() will create new controller");

    rc = controller_conf_create(&conf5, name5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf5,
                                 "conf_create() will create new controller");
  }

  // add conf
  {
    rc = controller_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = controller_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = controller_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = controller_conf_add(conf4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = controller_conf_add(conf5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // all
  {
    rc = controller_conf_list(&actual_list, NULL);
    TEST_ASSERT_EQUAL(5, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"controller_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"controller_name2") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace2"DATASTORE_NAMESPACE_DELIMITER"controller_name3") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"controller_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"controller_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // no namespace
  {
    rc = controller_conf_list(&actual_list, "");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"controller_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"controller_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // only namespace
  {
    rc = controller_conf_list(&actual_list, "namespace1");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"controller_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"controller_name2") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // Abnormal case
  {
    rc = controller_conf_list(NULL, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_finalize();
}

void
test_controller_conf_list_not_initialize(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  const char *name = "controller_name";
  controller_conf_t ***list = NULL;

  rc = controller_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

  rc = controller_conf_list(list, NULL);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  controller_conf_destroy(conf);

  controller_finalize();
}

void
test_controller_conf_one_list(void) {
  TEST_IGNORE();
}

void
test_controller_conf_one_list_not_initialize(void) {
  TEST_IGNORE();
}

void
test_controller_find(void) {
  lagopus_result_t rc;
  controller_conf_t *conf1 = NULL;
  const char *name1 = "controller_name1";
  controller_conf_t *conf2 = NULL;
  const char *name2 = "controller_name2";
  controller_conf_t *conf3 = NULL;
  const char *name3 = "controller_name3";
  controller_conf_t *actual_conf = NULL;

  controller_initialize();

  // create conf
  {
    rc = controller_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1,
                                 "conf_create() will create new controller");

    rc = controller_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2,
                                 "conf_create() will create new controller");

    rc = controller_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3,
                                 "conf_create() will create new controller");
  }

  // add conf
  {
    rc = controller_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = controller_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = controller_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Normal case
  {
    rc = controller_find(name1, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name1, actual_conf->name);

    rc = controller_find(name2, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name2, actual_conf->name);

    rc = controller_find(name3, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name3, actual_conf->name);

  }

  // Abnormal case
  {
    rc = controller_find(NULL, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_find(name1, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_finalize();
}

void
test_controller_find_not_initialize(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  const char *name = "controller_name";
  controller_conf_t *actual_conf = NULL;

  rc = controller_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

  rc = controller_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  controller_conf_destroy(conf);

  controller_finalize();
}

void
test_controller_attr_create_and_destroy(void) {
  lagopus_result_t rc;
  controller_attr_t *attr = NULL;
  char *actual_channel_name = NULL;
  const char *expected_channel_name = "";
  datastore_controller_role_t actual_role = DATASTORE_CONTROLLER_ROLE_UNKNOWN;
  const datastore_controller_role_t expected_role = actual_role;
  datastore_controller_connection_type_t actual_type
    = DATASTORE_CONTROLLER_CONNECTION_TYPE_UNKNOWN;
  const datastore_controller_connection_type_t expected_type = actual_type;

  controller_initialize();

  rc = controller_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new controller");

  // default value
  {
    // channel_name
    rc = controller_get_channel_name(attr, &actual_channel_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_channel_name, actual_channel_name);

    // role
    rc = controller_get_role(attr, &actual_role);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_role, actual_role);

    // type
    rc = controller_get_connection_type(attr, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_type, actual_type);
  }

  controller_attr_destroy(attr);
  free((void *)actual_channel_name);

  controller_finalize();
}

void
test_controller_attr_duplicate(void) {
  lagopus_result_t rc;
  const char *ns1 = "ns1";
  const char *ns2 = "ns2";
  const char *name = "channel";
  char *controller_fullname = NULL;
  bool result = false;
  controller_attr_t *src_attr = NULL;
  controller_attr_t *dst_attr = NULL;
  controller_attr_t *actual_attr = NULL;

  controller_initialize();

  // Normal case1(no namespace)
  {
    // create src attribute
    {
      rc = controller_attr_create(&src_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_attr,
                                   "attr_create() will create new controller");
      rc = ns_create_fullname(ns1, name, &controller_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = controller_set_channel_name(src_attr, controller_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(controller_fullname);
      controller_fullname = NULL;
    }

    // duplicate
    rc = controller_attr_duplicate(src_attr, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual attribute
    {
      rc = controller_attr_create(&actual_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(actual_attr,
                                   "attr_create() will create new policer");
      rc = ns_create_fullname(ns1, name, &controller_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = controller_set_channel_name(actual_attr, controller_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(controller_fullname);
      controller_fullname = NULL;
    }

    result = controller_attr_equals(dst_attr, actual_attr);
    TEST_ASSERT_TRUE(result);

    controller_attr_destroy(src_attr);
    src_attr = NULL;
    controller_attr_destroy(dst_attr);
    dst_attr = NULL;
    controller_attr_destroy(actual_attr);
    actual_attr = NULL;
  }

  // Normal case2
  {
    // create src attribute
    {
      rc = controller_attr_create(&src_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_attr,
                                   "attr_create() will create new controller");
      rc = ns_create_fullname(ns1, name, &controller_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = controller_set_channel_name(src_attr, controller_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(controller_fullname);
      controller_fullname = NULL;
    }

    // duplicate
    rc = controller_attr_duplicate(src_attr, &dst_attr, ns2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual attribute
    {
      rc = controller_attr_create(&actual_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(actual_attr,
                                   "attr_create() will create new policer");
      rc = ns_create_fullname(ns2, name, &controller_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = controller_set_channel_name(actual_attr, controller_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(controller_fullname);
      controller_fullname = NULL;
    }

    result = controller_attr_equals(dst_attr, actual_attr);
    TEST_ASSERT_TRUE(result);

    controller_attr_destroy(src_attr);
    src_attr = NULL;
    controller_attr_destroy(dst_attr);
    dst_attr = NULL;
    controller_attr_destroy(actual_attr);
    actual_attr = NULL;
  }

  // Abnormal case
  {
    rc = controller_attr_duplicate(NULL, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_finalize();
}

void
test_controller_attr_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  controller_attr_t *attr1 = NULL;
  controller_attr_t *attr2 = NULL;
  controller_attr_t *attr3 = NULL;
  const char *channel_name = "channel_name";

  controller_initialize();

  rc = controller_attr_create(&attr1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr1, "attr_create() will create new controller");

  rc = controller_attr_create(&attr2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr2, "attr_create() will create new controller");

  rc = controller_attr_create(&attr3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr3, "attr_create() will create new controller");
  rc = controller_set_channel_name(attr3, channel_name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = controller_attr_equals(attr1, attr2);
    TEST_ASSERT_TRUE(result);

    result = controller_attr_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = controller_attr_equals(attr1, attr3);
    TEST_ASSERT_FALSE(result);

    result = controller_attr_equals(attr2, attr3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = controller_attr_equals(attr1, NULL);
    TEST_ASSERT_FALSE(result);

    result = controller_attr_equals(NULL, attr2);
    TEST_ASSERT_FALSE(result);
  }

  controller_attr_destroy(attr1);
  controller_attr_destroy(attr2);
  controller_attr_destroy(attr3);

  controller_finalize();
}

void
test_controller_get_attr(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  controller_attr_t *attr = NULL;
  const char *name = "controller_name";

  controller_initialize();

  rc = controller_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

  rc = controller_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = controller_get_attr(name, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = controller_get_attr(name, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(attr);
  }

  // Abnormal case of getter
  {
    rc = controller_get_attr(NULL, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_get_attr(NULL, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_get_attr(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_get_attr(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_finalize();
}

void
test_controller_role_to_str(void) {
  lagopus_result_t rc;
  const char *actual_type_str;

  // Normal case
  {
    rc = controller_role_to_str(DATASTORE_CONTROLLER_ROLE_UNKNOWN,
                                &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_type_str);

    rc = controller_role_to_str(DATASTORE_CONTROLLER_ROLE_MASTER,
                                &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("master", actual_type_str);

    rc = controller_role_to_str(DATASTORE_CONTROLLER_ROLE_SLAVE, &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("slave", actual_type_str);

    rc = controller_role_to_str(DATASTORE_CONTROLLER_ROLE_EQUAL, &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("equal", actual_type_str);
  }

  // Abnormal case
  {
    rc = controller_role_to_str(DATASTORE_CONTROLLER_ROLE_EQUAL, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_controller_role_to_enum(void) {
  lagopus_result_t rc;
  datastore_controller_role_t actual_type;

  // Normal case
  {
    rc = controller_role_to_enum("unknown", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_CONTROLLER_ROLE_UNKNOWN, actual_type);

    rc = controller_role_to_enum("master", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_CONTROLLER_ROLE_MASTER, actual_type);

    rc = controller_role_to_enum("slave", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_CONTROLLER_ROLE_SLAVE, actual_type);

    rc = controller_role_to_enum("equal", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_CONTROLLER_ROLE_EQUAL, actual_type);

    rc = controller_role_to_enum("UNKNWON", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_role_to_enum("MASTER", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_role_to_enum("SLAVE", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_role_to_enum("EQUAL", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case
  {
    rc = controller_role_to_enum(NULL, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_role_to_enum("equal", NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_controller_connection_type_to_str(void) {
  lagopus_result_t rc;
  const char *actual_type_str;

  // Normal case
  {
    rc = controller_connection_type_to_str(
           DATASTORE_CONTROLLER_CONNECTION_TYPE_UNKNOWN, &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_type_str);

    rc = controller_connection_type_to_str(
           DATASTORE_CONTROLLER_CONNECTION_TYPE_MAIN, &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("main", actual_type_str);

    rc = controller_connection_type_to_str(
           DATASTORE_CONTROLLER_CONNECTION_TYPE_AUXILIARY, &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("auxiliary", actual_type_str);
  }

  // Abnormal case
  {
    rc = controller_connection_type_to_str(
           DATASTORE_CONTROLLER_CONNECTION_TYPE_AUXILIARY, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_controller_connection_type_to_enum(void) {
  lagopus_result_t rc;
  datastore_controller_connection_type_t actual_type;

  // Normal case
  {
    rc = controller_connection_type_to_enum("unknown", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_CONTROLLER_CONNECTION_TYPE_UNKNOWN,
                             actual_type);

    rc = controller_connection_type_to_enum("main", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_CONTROLLER_CONNECTION_TYPE_MAIN,
                             actual_type);

    rc = controller_connection_type_to_enum("auxiliary", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_CONTROLLER_CONNECTION_TYPE_AUXILIARY,
                             actual_type);

    rc = controller_connection_type_to_enum("UNKNOWN", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_connection_type_to_enum("MAIN", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_connection_type_to_enum("AUXILIARY", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case
  {
    rc = controller_connection_type_to_enum(NULL, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_connection_type_to_enum("tls", NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_controller_conf_private_exists(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  const char *name = "controller_name1";
  const char *invalid_name = "invalid_name";

  controller_initialize();

  rc = controller_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

  rc = controller_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    TEST_ASSERT_TRUE(controller_exists(name) == true);
    TEST_ASSERT_TRUE(controller_exists(invalid_name) == false);
  }

  // Abnormal case
  {
    TEST_ASSERT_TRUE(controller_exists(NULL) == false);
  }

  controller_finalize();
}

void
test_controller_conf_private_used(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  const char *name = "controller_name";
  bool actual_used = false;

  controller_initialize();

  rc = controller_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

  rc = controller_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = controller_set_used(name, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_controller_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = controller_set_used(NULL, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_controller_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_finalize();
}

void
test_controller_conf_public_used(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  const char *name = "controller_name";
  bool actual_used = false;

  controller_initialize();

  rc = controller_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

  rc = controller_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_controller_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = datastore_controller_is_used(NULL, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_controller_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_finalize();
}

void
test_controller_conf_private_enabled(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  const char *name = "controller_name";
  bool actual_enabled = false;

  controller_initialize();

  rc = controller_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

  rc = controller_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = controller_set_enabled(name, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_controller_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = controller_set_enabled(NULL, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_controller_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_finalize();
}

void
test_controller_conf_public_enabled(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  const char *name = "controller_name";
  bool actual_enabled = false;

  controller_initialize();

  rc = controller_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

  rc = controller_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_controller_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = datastore_controller_is_enabled(NULL, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_controller_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_finalize();
}

static void
create_str(char **string, size_t length) {
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
test_controller_create_str(void) {
  char *actual_str = NULL;
  const char *expected_str = "aaa";

  create_str(&actual_str, strlen(expected_str));
  TEST_ASSERT_EQUAL_STRING(expected_str, actual_str);

  free((void *)actual_str);
}

void
test_controller_attr_private_channel_name(void) {
  lagopus_result_t rc;
  controller_attr_t *attr = NULL;
  char *actual_channel_name = NULL;
  const char *expected_channel_name = "";
  const char *set_channel_name1 = "channel_name";
  char *set_channel_name2 = NULL;
  char *actual_set_channel_name1 = NULL;
  char *actual_set_channel_name2 = NULL;
  const char *expected_set_channel_name1 = set_channel_name1;
  const char *expected_set_channel_name2 = set_channel_name1;

  controller_initialize();

  rc = controller_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new controller");

  // Normal case of getter
  {
    rc = controller_get_channel_name(attr, &actual_channel_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_channel_name, actual_channel_name);
  }

  // Abnormal case of getter
  {
    rc = controller_get_channel_name(NULL, &actual_channel_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_get_channel_name(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = controller_set_channel_name(attr, set_channel_name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = controller_get_channel_name(attr, &actual_set_channel_name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_channel_name1, actual_set_channel_name1);

    create_str(&set_channel_name2, DATASTORE_CHANNEL_FULLNAME_MAX + 1);
    rc = controller_set_channel_name(attr, set_channel_name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = controller_get_channel_name(attr, &actual_set_channel_name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_channel_name2, actual_set_channel_name2);
  }

  // Abnormal case of setter
  {
    rc = controller_set_channel_name(NULL, set_channel_name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_set_channel_name(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_attr_destroy(attr);
  free((void *)actual_channel_name);
  free((void *)set_channel_name2);
  free((void *)actual_set_channel_name1);
  free((void *)actual_set_channel_name2);

  controller_finalize();
}

void
test_controller_attr_public_channel_name(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  const char *name = "controller_name";
  char *actual_channel_name = NULL;
  const char *expected_channel_name = "";

  controller_initialize();

  rc = controller_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

  rc = controller_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_controller_get_channel_name(name, true, &actual_channel_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_controller_get_channel_name(name, false, &actual_channel_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_channel_name, actual_channel_name);
  }

  // Abnormal case of getter
  {
    rc = datastore_controller_get_channel_name(NULL, true, &actual_channel_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_controller_get_channel_name(NULL, false, &actual_channel_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_controller_get_channel_name(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_controller_get_channel_name(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  free((void *)actual_channel_name);
  controller_finalize();
}

void
test_controller_attr_private_role(void) {
  lagopus_result_t rc;
  controller_attr_t *attr = NULL;
  datastore_controller_role_t actual_role = DATASTORE_CONTROLLER_ROLE_UNKNOWN;
  const datastore_controller_role_t expected_role = actual_role;
  datastore_controller_role_t set_role = DATASTORE_CONTROLLER_ROLE_EQUAL;
  datastore_controller_role_t actual_set_role =
    DATASTORE_CONTROLLER_ROLE_UNKNOWN;
  const datastore_controller_role_t expected_set_role = set_role;

  controller_initialize();

  rc = controller_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new controller");

  // Normal case of getter
  {
    rc = controller_get_role(attr, &actual_role);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_role, actual_role);
  }

  // Abnormal case of getter
  {
    rc = controller_get_role(NULL, &actual_role);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_get_role(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = controller_set_role(attr, set_role);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = controller_get_role(attr, &actual_set_role);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_role, actual_set_role);
  }

  // Abnormal case of setter
  {
    rc = controller_set_role(NULL, set_role);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_attr_destroy(attr);

  controller_finalize();
}

void
test_controller_attr_public_role(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  const char *name = "controller_name";
  datastore_controller_role_t actual_role = DATASTORE_CONTROLLER_ROLE_UNKNOWN;
  const datastore_controller_role_t expected_role = actual_role;

  controller_initialize();

  rc = controller_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

  rc = controller_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_controller_get_role(name, true, &actual_role);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_controller_get_role(name, false, &actual_role);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_role, actual_role);
  }

  // Abnormal case of getter
  {
    rc = datastore_controller_get_role(NULL, true, &actual_role);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_controller_get_role(NULL, false, &actual_role);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_controller_get_role(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_controller_get_role(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_finalize();
}

void
test_controller_attr_private_connection_type(void) {
  lagopus_result_t rc;
  controller_attr_t *attr = NULL;
  datastore_controller_connection_type_t actual_connection_type
    = DATASTORE_CONTROLLER_ROLE_UNKNOWN;
  const datastore_controller_connection_type_t expected_connection_type
    = actual_connection_type;
  datastore_controller_connection_type_t set_connection_type
    = DATASTORE_CONTROLLER_ROLE_EQUAL;
  datastore_controller_connection_type_t actual_set_connection_type
    = DATASTORE_CONTROLLER_ROLE_UNKNOWN;
  const datastore_controller_connection_type_t expected_set_connection_type
    = set_connection_type;

  controller_initialize();

  rc = controller_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new controller");

  // Normal case of getter
  {
    rc = controller_get_connection_type(attr, &actual_connection_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_connection_type, actual_connection_type);
  }

  // Abnormal case of getter
  {
    rc = controller_get_connection_type(NULL, &actual_connection_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_get_connection_type(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = controller_set_connection_type(attr, set_connection_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = controller_get_connection_type(attr, &actual_set_connection_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_connection_type,
                             actual_set_connection_type);
  }

  // Abnormal case of setter
  {
    rc = controller_set_connection_type(NULL, set_connection_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_attr_destroy(attr);

  controller_finalize();
}

void
test_controller_attr_public_connection_type(void) {
  lagopus_result_t rc;
  controller_conf_t *conf = NULL;
  const char *name = "controller_name";
  datastore_controller_connection_type_t actual_connection_type
    = DATASTORE_CONTROLLER_ROLE_UNKNOWN;
  const datastore_controller_connection_type_t expected_connection_type
    = actual_connection_type;

  controller_initialize();

  rc = controller_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new controller");

  rc = controller_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_controller_get_connection_type(name, true,
         &actual_connection_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_controller_get_connection_type(name, false,
         &actual_connection_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_connection_type, actual_connection_type);
  }

  // Abnormal case of getter
  {
    rc = datastore_controller_get_connection_type(NULL, true,
         &actual_connection_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_controller_get_connection_type(NULL, false,
         &actual_connection_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_controller_get_connection_type(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_controller_get_connection_type(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_finalize();
}

void
test_controller_get_name_by_channel(void) {
  lagopus_result_t rc = LAGOPUS_RESULT_ANY_FAILURES;
  controller_conf_t *conf1 = NULL;
  controller_conf_t *conf2 = NULL;
  const char *name1 = "controller_name1";
  const char *name2 = "controller_name2";
  const char *channel_name1 = "channel_name1";
  const char *channel_name2 = "channel_name2";
  char *actual_name = NULL;

  controller_initialize();

  rc = controller_conf_create(&conf1, name1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf1,
                               "conf_create() will create new controller");

  rc = controller_conf_create(&conf2, name2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf2,
                               "conf_create() will create new controller");

  rc = controller_conf_add(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = controller_conf_add(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  if (conf1 != NULL) {
    rc = controller_set_channel_name(conf1->modified_attr,
                                     channel_name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  } else {
    TEST_FAIL_MESSAGE("conf is NULL.");
  }

  if (conf2 != NULL) {
    // move attr
    conf2->current_attr = conf2->modified_attr;
    conf2->modified_attr = NULL;

    rc = controller_set_channel_name(conf2->current_attr,
                                     channel_name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  } else {
    TEST_FAIL_MESSAGE("conf is NULL.");
  }

  // Normal case
  {
    // get name in current_attr
    rc = controller_get_name_by_channel(channel_name1,
                                        true,
                                        &actual_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, rc);
    free(actual_name);
    actual_name = NULL;

    rc = controller_get_name_by_channel(channel_name2,
                                        true,
                                        &actual_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    if (actual_name != NULL) {
      TEST_ASSERT_EQUAL_MESSAGE(strcmp(name2, actual_name), 0,
                                "bad channel_name.");
    } else {
      TEST_FAIL_MESSAGE("name is NULL.");
    }
    free(actual_name);
    actual_name = NULL;

    // get name in modifiedt_attr
    rc = controller_get_name_by_channel(channel_name1,
                                        false,
                                        &actual_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    if (actual_name != NULL) {
      TEST_ASSERT_EQUAL_MESSAGE(strcmp(name1, actual_name), 0,
                                "bad channel_name.");
    } else {
      TEST_FAIL_MESSAGE("name is NULL.");
    }
    free(actual_name);
    actual_name = NULL;

    rc = controller_get_name_by_channel(channel_name2,
                                        false,
                                        &actual_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, rc);
    free(actual_name);
    actual_name = NULL;
  }

  // Abnormal case
  {
    rc = controller_get_name_by_channel("",
                                        true,
                                        &actual_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_get_name_by_channel(NULL,
                                        true,
                                        &actual_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = controller_get_name_by_channel(channel_name1,
                                        true,
                                        NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  controller_finalize();
}
