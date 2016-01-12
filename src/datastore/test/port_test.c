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
#include "../port.c"
#define CAST_UINT64(x) (uint64_t) x

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_port_initialize_and_finalize(void) {
  port_initialize();
  port_finalize();
}


void
test_port_attr_queue_name_exists(void) {
  lagopus_result_t rc;
  bool ret = false;
  port_attr_t *attr = NULL;
  const char *name1 = "port3";
  const char *name2 = "invalid_port";

  port_initialize();

  rc = port_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new port");

  rc = port_attr_add_queue_name(attr, "port1");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = port_attr_add_queue_name(attr, "port2");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = port_attr_add_queue_name(attr, "port3");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of setter
  {
    ret = port_attr_queue_name_exists(attr, name1);
    TEST_ASSERT_TRUE(ret);

    ret = port_attr_queue_name_exists(attr, name2);
    TEST_ASSERT_FALSE(ret);
  }

  // Abnormal case of setter
  {
    ret = port_attr_queue_name_exists(NULL, name1);
    TEST_ASSERT_FALSE(ret);

    ret = port_attr_queue_name_exists(attr, NULL);
    TEST_ASSERT_FALSE(ret);
  }

  port_attr_destroy(attr);

  port_finalize();
}

void
test_port_conf_create_and_destroy(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name";

  port_initialize();

  // Normal case
  {
    rc = port_conf_create(&conf, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

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
    rc = port_conf_create(NULL, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = port_conf_create(&conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  port_conf_destroy(conf);

  port_finalize();
}

void
test_port_conf_duplicate(void) {
  lagopus_result_t rc;
  const char *ns1 = "ns1";
  const char *ns2 = "ns2";
  const char *name = "port";
  char *port_fullname = NULL;
  bool result = false;
  port_conf_t *src_conf = NULL;
  port_conf_t *dst_conf = NULL;
  port_conf_t *actual_conf = NULL;

  port_initialize();

  // Normal case1(no namespace)
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &port_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_conf_create(&src_conf, port_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new port_action");
      free(port_fullname);
      port_fullname = NULL;
    }

    rc = port_conf_duplicate(src_conf, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns1, name, &port_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_conf_create(&actual_conf, port_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new port_action");
      free(port_fullname);
      port_fullname = NULL;
    }

    result = port_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    port_conf_destroy(src_conf);
    src_conf = NULL;
    port_conf_destroy(dst_conf);
    dst_conf = NULL;
    port_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Normal case2
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &port_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_conf_create(&src_conf, port_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new port_action");
      free(port_fullname);
      port_fullname = NULL;
    }

    rc = port_conf_duplicate(src_conf, &dst_conf, ns2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns2, name, &port_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_conf_create(&actual_conf, port_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new port_action");
      free(port_fullname);
      port_fullname = NULL;
    }

    result = port_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    port_conf_destroy(src_conf);
    src_conf = NULL;
    port_conf_destroy(dst_conf);
    dst_conf = NULL;
    port_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Abnormal case
  {
    rc = port_conf_duplicate(NULL, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  port_finalize();
}

void
test_port_conf_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  port_conf_t *conf1 = NULL;
  port_conf_t *conf2 = NULL;
  port_conf_t *conf3 = NULL;
  const char *fullname1 = "conf1";
  const char *fullname2 = "conf2";
  const char *fullname3 = "conf3";

  port_initialize();

  rc = port_conf_create(&conf1, fullname1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new port");
  rc = port_conf_add(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = port_conf_create(&conf2, fullname2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new port");
  rc = port_conf_add(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = port_conf_create(&conf3, fullname3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new port");
  rc = port_conf_add(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = port_set_enabled(fullname3, true);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = port_conf_equals(conf1, conf2);
    TEST_ASSERT_TRUE(result);

    result = port_conf_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = port_conf_equals(conf1, conf3);
    TEST_ASSERT_FALSE(result);

    result = port_conf_equals(conf2, conf3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = port_conf_equals(conf1, NULL);
    TEST_ASSERT_FALSE(result);

    result = port_conf_equals(NULL, conf2);
    TEST_ASSERT_FALSE(result);
  }

  rc = port_conf_delete(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = port_conf_delete(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = port_conf_delete(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  port_finalize();
}

void
test_port_conf_add(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name";
  port_conf_t *actual_conf = NULL;

  port_initialize();

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  // Normal case
  {
    rc = port_conf_add(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = port_conf_add(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = port_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  port_finalize();
}

void
test_port_conf_add_not_initialize(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name";

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = port_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  port_conf_destroy(conf);

  port_finalize();
}

void
test_port_conf_delete(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name";
  port_conf_t *actual_conf = NULL;

  port_initialize();

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = port_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    rc = port_conf_delete(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = port_conf_delete(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = port_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, rc);

  port_finalize();
}

void
test_port_conf_delete_not_initialize(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name";

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = port_conf_delete(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  port_conf_destroy(conf);

  port_finalize();
}

void
test_port_conf_iterate(void) {
  TEST_IGNORE();
}

void
test_port_conf_list(void) {
  lagopus_result_t rc;
  port_conf_t *conf1 = NULL;
  const char *name1 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"port_name1";
  port_conf_t *conf2 = NULL;
  const char *name2 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"port_name2";
  port_conf_t *conf3 = NULL;
  const char *name3 = "namespace2"DATASTORE_NAMESPACE_DELIMITER"port_name3";
  port_conf_t *conf4 = NULL;
  const char *name4 = DATASTORE_NAMESPACE_DELIMITER"port_name4";
  port_conf_t *conf5 = NULL;
  const char *name5 = DATASTORE_NAMESPACE_DELIMITER"port_name5";
  port_conf_t **actual_list = NULL;
  size_t i;

  port_initialize();

  // create conf
  {
    rc = port_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new port");

    rc = port_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new port");

    rc = port_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new port");

    rc = port_conf_create(&conf4, name4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf4, "conf_create() will create new port");

    rc = port_conf_create(&conf5, name5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf5, "conf_create() will create new port");
  }

  // add conf
  {
    rc = port_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = port_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = port_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = port_conf_add(conf4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = port_conf_add(conf5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // all
  {
    rc = port_conf_list(&actual_list, NULL);
    TEST_ASSERT_EQUAL(5, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"port_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"port_name2") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace2"DATASTORE_NAMESPACE_DELIMITER"port_name3") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"port_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"port_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // no namespace
  {
    rc = port_conf_list(&actual_list, "");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"port_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"port_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // only namespace
  {
    rc = port_conf_list(&actual_list, "namespace1");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"port_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"port_name2") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // Abnormal case
  {
    rc = port_conf_list(NULL, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  port_finalize();
}

void
test_port_conf_list_not_initialize(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name";
  port_conf_t ***list = NULL;

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = port_conf_list(list, NULL);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  port_conf_destroy(conf);

  port_finalize();
}

void
test_port_conf_one_list(void) {
  TEST_IGNORE();
}

void
test_port_conf_one_list_not_initialize(void) {
  TEST_IGNORE();
}

void
test_port_find(void) {
  lagopus_result_t rc;
  port_conf_t *conf1 = NULL;
  const char *name1 = "port_name1";
  port_conf_t *conf2 = NULL;
  const char *name2 = "port_name2";
  port_conf_t *conf3 = NULL;
  const char *name3 = "port_name3";
  port_conf_t *actual_conf = NULL;

  port_initialize();

  // create conf
  {
    rc = port_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new port");

    rc = port_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new port");

    rc = port_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new port");
  }

  // add conf
  {
    rc = port_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = port_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = port_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Normal case
  {
    rc = port_find(name1, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name1, actual_conf->name);

    rc = port_find(name2, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name2, actual_conf->name);

    rc = port_find(name3, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name3, actual_conf->name);

  }

  // Abnormal case
  {
    rc = port_find(NULL, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = port_find(name1, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  port_finalize();
}

void
test_port_find_not_initialize(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name";
  port_conf_t *actual_conf = NULL;

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = port_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  port_conf_destroy(conf);

  port_finalize();
}

void
test_port_attr_create_and_destroy(void) {
  lagopus_result_t rc;
  port_attr_t *attr = NULL;
  uint32_t actual_port_number = 0;
  const uint32_t expected_port_number = actual_port_number;
  char *actual_interface_name = NULL;
  const char *expected_interface_name = "";
  char *actual_policer_name = NULL;
  const char *expected_policer_name = "";
  datastore_name_info_t *actual_queue_names = NULL;
  struct datastore_name_entry *expected_queue_entry = NULL;

  port_initialize();

  rc = port_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new port");

  // default value
  {
    // port_number
    rc = port_get_port_number(attr, &actual_port_number);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_port_number, actual_port_number);

    // interface_name
    rc = port_get_interface_name(attr, &actual_interface_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_interface_name, actual_interface_name);

    // policer_name
    rc = port_get_policer_name(attr, &actual_policer_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_policer_name, actual_policer_name);

    // port_names
    rc = port_get_queue_names(attr, &actual_queue_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(0, actual_queue_names->size);
    expected_queue_entry = TAILQ_FIRST(&(actual_queue_names->head));
    TEST_ASSERT_NULL(expected_queue_entry);
  }

  port_attr_destroy(attr);
  free((void *)actual_interface_name);
  free((void *)actual_policer_name);
  datastore_names_destroy(actual_queue_names);

  port_finalize();
}

void
test_port_attr_duplicate(void) {
  lagopus_result_t rc;
  const char *ns1 = "ns1";
  const char *ns2 = "ns2";
  const char *policer_name = "policer";
  const char *queue_name = "queue";
  const char *interface_name = "interface";
  char *policer_fullname = NULL;
  char *queue_fullname = NULL;
  char *interface_fullname = NULL;
  bool result = false;
  port_attr_t *src_attr = NULL;
  port_attr_t *dst_attr = NULL;
  port_attr_t *actual_attr = NULL;

  port_initialize();

  // Normal case1(no namespace)
  {
    // create src attribute
    {
      rc = port_attr_create(&src_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_attr,
                                   "attr_create() will create new policer");
      rc = ns_create_fullname(ns1, policer_name, &policer_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_set_policer_name(src_attr, policer_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(policer_fullname);
      policer_fullname = NULL;
      rc = ns_create_fullname(ns1, queue_name, &queue_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_attr_add_queue_name(src_attr, queue_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(queue_fullname);
      queue_fullname = NULL;
      rc = ns_create_fullname(ns1, interface_name, &interface_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_set_policer_name(src_attr, interface_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(interface_fullname);
      interface_fullname = NULL;
    }

    // duplicate
    rc = port_attr_duplicate(src_attr, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual attribute
    {
      rc = port_attr_create(&actual_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(actual_attr,
                                   "attr_create() will create new policer");
      rc = ns_create_fullname(ns1, policer_name, &policer_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_set_policer_name(actual_attr, policer_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(policer_fullname);
      policer_fullname = NULL;
      rc = ns_create_fullname(ns1, queue_name, &queue_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_attr_add_queue_name(actual_attr, queue_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(queue_fullname);
      queue_fullname = NULL;
      rc = ns_create_fullname(ns1, interface_name, &interface_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_set_policer_name(actual_attr, interface_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(interface_fullname);
      interface_fullname = NULL;
    }

    result = port_attr_equals(dst_attr, actual_attr);
    TEST_ASSERT_TRUE(result);

    port_attr_destroy(src_attr);
    src_attr = NULL;
    port_attr_destroy(dst_attr);
    dst_attr = NULL;
    port_attr_destroy(actual_attr);
    actual_attr = NULL;
  }

  // Normal case2
  {
    // create src attribute
    {
      rc = port_attr_create(&src_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_attr,
                                   "attr_create() will create new policer");
      rc = ns_create_fullname(ns1, policer_name, &policer_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_set_policer_name(src_attr, policer_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(policer_fullname);
      policer_fullname = NULL;
      rc = ns_create_fullname(ns1, queue_name, &queue_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_attr_add_queue_name(src_attr, queue_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(queue_fullname);
      queue_fullname = NULL;
      rc = ns_create_fullname(ns1, interface_name, &interface_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_set_interface_name(src_attr, interface_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(interface_fullname);
      interface_fullname = NULL;
    }

    // duplicate
    rc = port_attr_duplicate(src_attr, &dst_attr, ns2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual attribute
    {
      rc = port_attr_create(&actual_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(actual_attr,
                                   "attr_create() will create new policer");
      rc = ns_create_fullname(ns2, policer_name, &policer_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_set_policer_name(actual_attr, policer_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(policer_fullname);
      policer_fullname = NULL;
      rc = ns_create_fullname(ns2, queue_name, &queue_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_attr_add_queue_name(actual_attr, queue_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(queue_fullname);
      queue_fullname = NULL;
      rc = ns_create_fullname(ns2, interface_name, &interface_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = port_set_interface_name(actual_attr, interface_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(interface_fullname);
      interface_fullname = NULL;
    }

    result = port_attr_equals(dst_attr, actual_attr);
    TEST_ASSERT_TRUE(result);

    port_attr_destroy(src_attr);
    src_attr = NULL;
    port_attr_destroy(dst_attr);
    dst_attr = NULL;
    port_attr_destroy(actual_attr);
    actual_attr = NULL;
  }

  // Abnormal case
  {
    rc = port_attr_duplicate(NULL, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  port_finalize();
}

void
test_port_attr_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  port_attr_t *attr1 = NULL;
  port_attr_t *attr2 = NULL;
  port_attr_t *attr3 = NULL;

  port_initialize();

  rc = port_attr_create(&attr1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr1, "attr_create() will create new interface");

  rc = port_attr_create(&attr2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr2, "attr_create() will create new interface");

  rc = port_attr_create(&attr3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr3, "attr_create() will create new interface");
  rc = port_set_port_number(attr3, DATASTORE_INTERFACE_TYPE_VXLAN);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = port_attr_equals(attr1, attr2);
    TEST_ASSERT_TRUE(result);

    result = port_attr_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = port_attr_equals(attr1, attr3);
    TEST_ASSERT_FALSE(result);

    result = port_attr_equals(attr2, attr3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = port_attr_equals(attr1, NULL);
    TEST_ASSERT_FALSE(result);

    result = port_attr_equals(NULL, attr2);
    TEST_ASSERT_FALSE(result);
  }

  port_attr_destroy(attr1);
  port_attr_destroy(attr2);
  port_attr_destroy(attr3);

  port_finalize();
}

void
test_port_get_attr(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  port_attr_t *attr = NULL;
  const char *name = "port_name";

  port_initialize();

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = port_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = port_get_attr(name, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = port_get_attr(name, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(attr);
  }

  // Abnormal case of getter
  {
    rc = port_get_attr(NULL, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = port_get_attr(NULL, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = port_get_attr(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = port_get_attr(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  port_finalize();
}


void
test_port_conf_private_exists(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name1";
  const char *invalid_name = "invalid_name";

  port_initialize();

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = port_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    TEST_ASSERT_TRUE(port_exists(name) == true);
    TEST_ASSERT_TRUE(port_exists(invalid_name) == false);
  }

  // Abnormal case
  {
    TEST_ASSERT_TRUE(port_exists(NULL) == false);
  }

  port_finalize();
}

void
test_port_conf_private_used(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name";
  bool actual_used = false;

  port_initialize();

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = port_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = port_set_used(name, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_port_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = port_set_used(NULL, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  port_finalize();
}

void
test_port_conf_public_used(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name";
  bool actual_used = false;

  port_initialize();

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = port_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_port_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = datastore_port_is_used(NULL, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  port_finalize();
}

void
test_port_conf_private_enabled(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name";
  bool actual_enabled = false;

  port_initialize();

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = port_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = port_set_enabled(name, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_port_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = port_set_enabled(NULL, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  port_finalize();
}

void
test_port_conf_public_enabled(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name";
  bool actual_enabled = false;

  port_initialize();

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = port_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_port_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = datastore_port_is_enabled(NULL, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  port_finalize();
}

void
test_port_attr_private_port_number(void) {
  lagopus_result_t rc;
  port_attr_t *attr = NULL;
  uint32_t actual_port_number = 0;
  const uint32_t expected_port_number = actual_port_number;
  const uint64_t set_port_number1 = CAST_UINT64(MAXIMUM_PORT_NUMBER);
  const uint64_t set_port_number2 = CAST_UINT64(MINIMUM_PORT_NUMBER);
  const uint64_t set_port_number3 = CAST_UINT64(MAXIMUM_PORT_NUMBER + 1);
  const int set_port_number4 = -1;
  uint32_t actual_set_port_number1 = 0;
  uint32_t actual_set_port_number2 = 0;
  uint32_t actual_set_port_number3 = 0;
  uint32_t actual_set_port_number4 = 0;
  const uint32_t expected_set_port_number1 = MAXIMUM_PORT_NUMBER;
  const uint32_t expected_set_port_number2 = MINIMUM_PORT_NUMBER;
  const uint32_t expected_set_port_number3 = MINIMUM_PORT_NUMBER;
  const uint32_t expected_set_port_number4 = MINIMUM_PORT_NUMBER;

  port_initialize();

  rc = port_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new port");

  // Normal case of getter
  {
    rc = port_get_port_number(attr, &actual_port_number);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_port_number, actual_port_number);
  }

  // Abnormal case of getter
  {
    rc = port_get_port_number(NULL, &actual_port_number);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = port_get_port_number(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = port_set_port_number(attr, set_port_number1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = port_get_port_number(attr, &actual_set_port_number1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_port_number1, actual_set_port_number1);

    rc = port_set_port_number(attr, set_port_number2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = port_get_port_number(attr, &actual_set_port_number2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_port_number2, actual_set_port_number2);
  }

  // Abnormal case of setter
  {
    rc = port_set_port_number(NULL, set_port_number1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = port_set_port_number(attr, CAST_UINT64(set_port_number3));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = port_get_port_number(attr, &actual_set_port_number3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_port_number3, actual_set_port_number3);

    rc = port_set_port_number(attr, CAST_UINT64(set_port_number4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = port_get_port_number(attr, &actual_set_port_number4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_port_number4, actual_set_port_number4);
  }

  port_attr_destroy(attr);

  port_finalize();
}

void
test_port_attr_public_port_number(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name";
  uint32_t actual_port_number = 0;
  const uint32_t expected_port_number = actual_port_number;

  port_initialize();

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = port_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_port_get_port_number(name, true, &actual_port_number);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_port_get_port_number(name, false, &actual_port_number);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_port_number, actual_port_number);
  }

  // Abnormal case of getter
  {
    rc = datastore_port_get_port_number(NULL, true, &actual_port_number);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_get_port_number(NULL, false, &actual_port_number);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_get_port_number(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_get_port_number(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  port_finalize();
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
test_port_create_str(void) {
  char *actual_str = NULL;
  const char *expected_str = "aaa";

  create_str(&actual_str, strlen(expected_str));
  TEST_ASSERT_EQUAL_STRING(expected_str, actual_str);

  free((void *)actual_str);
}

void
test_port_attr_private_interface_name(void) {
  lagopus_result_t rc;
  port_attr_t *attr = NULL;
  char *actual_interface_name = NULL;
  const char *expected_interface_name = "";
  const char *set_interface_name1 = "interface_name";
  char *set_interface_name2 = NULL;
  char *actual_set_interface_name1 = NULL;
  char *actual_set_interface_name2 = NULL;
  const char *expected_set_interface_name1 = set_interface_name1;
  const char *expected_set_interface_name2 = set_interface_name1;

  port_initialize();

  rc = port_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new port");

  // Normal case of getter
  {
    rc = port_get_interface_name(attr, &actual_interface_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_interface_name, actual_interface_name);
  }

  // Abnormal case of getter
  {
    rc = port_get_interface_name(NULL, &actual_interface_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = port_get_interface_name(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = port_set_interface_name(attr, set_interface_name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = port_get_interface_name(attr, &actual_set_interface_name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_interface_name1,
                             actual_set_interface_name1);

    create_str(&set_interface_name2, DATASTORE_INTERFACE_FULLNAME_MAX + 1);
    rc = port_set_interface_name(attr, set_interface_name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = port_get_interface_name(attr, &actual_set_interface_name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_interface_name2,
                             actual_set_interface_name2);
  }

  // Abnormal case of setter
  {
    rc = port_set_interface_name(NULL, set_interface_name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = port_set_interface_name(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  port_attr_destroy(attr);
  free((void *)actual_interface_name);
  free((void *)set_interface_name2);
  free((void *)actual_set_interface_name1);
  free((void *)actual_set_interface_name2);

  port_finalize();
}

void
test_port_attr_public_interface_name(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name";
  char *actual_interface_name = NULL;
  const char *expected_interface_name = "";

  port_initialize();

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = port_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_port_get_interface_name(name, true, &actual_interface_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_port_get_interface_name(name, false, &actual_interface_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_interface_name, actual_interface_name);
  }

  // Abnormal case of getter
  {
    rc = datastore_port_get_interface_name(NULL, true, &actual_interface_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_get_interface_name(NULL, false, &actual_interface_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_get_interface_name(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_get_interface_name(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  free((void *)actual_interface_name);
  port_finalize();
}

void
test_port_attr_private_policer_name(void) {
  lagopus_result_t rc;
  port_attr_t *attr = NULL;
  char *actual_policer_name = NULL;
  const char *expected_policer_name = "";
  const char *set_policer_name1 = "policer_name";
  char *set_policer_name2 = NULL;
  char *actual_set_policer_name1 = NULL;
  char *actual_set_policer_name2 = NULL;
  const char *expected_set_policer_name1 = set_policer_name1;
  const char *expected_set_policer_name2 = set_policer_name1;

  port_initialize();

  rc = port_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new port");

  // Normal case of getter
  {
    rc = port_get_policer_name(attr, &actual_policer_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_policer_name, actual_policer_name);
  }

  // Abnormal case of getter
  {
    rc = port_get_policer_name(NULL, &actual_policer_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = port_get_policer_name(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = port_set_policer_name(attr, set_policer_name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = port_get_policer_name(attr, &actual_set_policer_name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_policer_name1,
                             actual_set_policer_name1);

    create_str(&set_policer_name2, DATASTORE_POLICER_FULLNAME_MAX + 1);
    rc = port_set_policer_name(attr, set_policer_name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = port_get_policer_name(attr, &actual_set_policer_name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_policer_name2,
                             actual_set_policer_name2);
  }

  // Abnormal case of setter
  {
    rc = port_set_policer_name(NULL, set_policer_name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = port_set_policer_name(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  port_attr_destroy(attr);
  free((void *)actual_policer_name);
  free((void *)set_policer_name2);
  free((void *)actual_set_policer_name1);
  free((void *)actual_set_policer_name2);

  port_finalize();
}

void
test_port_attr_public_policer_name(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name";
  char *actual_policer_name = NULL;
  const char *expected_policer_name = "";

  port_initialize();

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = port_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_port_get_policer_name(name, true, &actual_policer_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_port_get_policer_name(name, false, &actual_policer_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_policer_name, actual_policer_name);
  }

  // Abnormal case of getter
  {
    rc = datastore_port_get_policer_name(NULL, true, &actual_policer_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_get_policer_name(NULL, false, &actual_policer_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_get_policer_name(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_get_policer_name(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  free((void *)actual_policer_name);
  port_finalize();
}

void
test_port_attr_private_queue_names(void) {
  lagopus_result_t rc;
  port_attr_t *attr = NULL;
  datastore_name_info_t *actual_queue_names = NULL;
  struct datastore_name_entry *expected_queue_entry = NULL;

  port_initialize();

  rc = port_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new port");

  // Normal case of add, getter, remove, remove all
  {
    // add
    rc = port_attr_add_queue_name(attr, "port1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = port_attr_add_queue_name(attr, "port2");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = port_attr_add_queue_name(attr, "port3");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // getter
    rc = port_get_queue_names(attr, &actual_queue_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(3, actual_queue_names->size);
    TAILQ_FOREACH(expected_queue_entry, &(actual_queue_names->head),
                  name_entries) {
      if ((strcmp("port1", expected_queue_entry->str) != 0) &&
          (strcmp("port2", expected_queue_entry->str) != 0) &&
          (strcmp("port3", expected_queue_entry->str) != 0)) {
        TEST_FAIL();
      }
    }
    rc = datastore_names_destroy(actual_queue_names);
    actual_queue_names = NULL;
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // remove
    rc = port_attr_remove_queue_name(attr, "port1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = port_attr_remove_queue_name(attr, "port3");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = port_get_queue_names(attr, &actual_queue_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(1, actual_queue_names->size);
    expected_queue_entry = TAILQ_FIRST(&(actual_queue_names->head));
    TEST_ASSERT_NOT_NULL(expected_queue_entry);
    TEST_ASSERT_EQUAL_STRING("port2", expected_queue_entry->str);
    rc = datastore_names_destroy(actual_queue_names);
    actual_queue_names = NULL;
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // remove all
    rc = port_attr_remove_all_queue_name(attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = port_get_queue_names(attr, &actual_queue_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(0, actual_queue_names->size);
    expected_queue_entry = TAILQ_FIRST(&(actual_queue_names->head));
    TEST_ASSERT_NULL(expected_queue_entry);
    rc = datastore_names_destroy(actual_queue_names);
    actual_queue_names = NULL;
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case of getter
  {
    rc = port_get_queue_names(NULL, &actual_queue_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = port_get_queue_names(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case of add
  {
    rc = port_attr_add_queue_name(NULL, "port1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = port_attr_add_queue_name(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case of remove
  {
    rc = port_attr_remove_queue_name(NULL, "port1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = port_attr_remove_queue_name(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case of remove all
  {
    rc = port_attr_remove_all_queue_name(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  port_attr_destroy(attr);

  port_finalize();
}

void
test_port_attr_public_queue_names(void) {
  lagopus_result_t rc;
  port_conf_t *conf = NULL;
  const char *name = "port_name";
  datastore_name_info_t *actual_queue_names = NULL;
  struct datastore_name_entry *expected_queue_entry = NULL;

  port_initialize();

  rc = port_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = port_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_port_get_queue_names(name, true, &actual_queue_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    // controller_names
    rc = datastore_port_get_queue_names(name, false, &actual_queue_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(0, actual_queue_names->size);
    expected_queue_entry = TAILQ_FIRST(&(actual_queue_names->head));
    TEST_ASSERT_NULL(expected_queue_entry);
    rc = datastore_names_destroy(actual_queue_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case of getter
  {
    rc = datastore_port_get_queue_names(NULL, true, &actual_queue_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_get_queue_names(NULL, false, &actual_queue_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_get_queue_names(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_port_get_queue_names(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  port_finalize();
}
