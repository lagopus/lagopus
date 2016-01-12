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
#include "../interface.c"
#define CAST_UINT64(x) (uint64_t) x

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_interface_initialize_and_finalize(void) {
  interface_initialize();
  interface_finalize();
}

void
test_interface_conf_create_and_destroy(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";

  interface_initialize();

  // Normal case
  {
    rc = interface_conf_create(&conf, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

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
    rc = interface_conf_create(NULL, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_conf_create(&conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_conf_destroy(conf);

  interface_finalize();
}

void
test_interface_conf_duplicate(void) {
  lagopus_result_t rc;
  const char *ns1 = "ns1";
  const char *ns2 = "ns2";
  const char *name = "interface";
  char *interface_fullname = NULL;
  bool result = false;
  interface_conf_t *src_conf = NULL;
  interface_conf_t *dst_conf = NULL;
  interface_conf_t *actual_conf = NULL;

  interface_initialize();

  // Normal case1(no namespace)
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &interface_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = interface_conf_create(&src_conf, interface_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new interface_action");
      free(interface_fullname);
      interface_fullname = NULL;
    }

    rc = interface_conf_duplicate(src_conf, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns1, name, &interface_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = interface_conf_create(&actual_conf, interface_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new interface_action");
      free(interface_fullname);
      interface_fullname = NULL;
    }

    result = interface_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    interface_conf_destroy(src_conf);
    src_conf = NULL;
    interface_conf_destroy(dst_conf);
    dst_conf = NULL;
    interface_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Normal case2
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &interface_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = interface_conf_create(&src_conf, interface_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new interface_action");
      free(interface_fullname);
      interface_fullname = NULL;
    }

    rc = interface_conf_duplicate(src_conf, &dst_conf, ns2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns2, name, &interface_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = interface_conf_create(&actual_conf, interface_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new interface_action");
      free(interface_fullname);
      interface_fullname = NULL;
    }

    result = interface_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    interface_conf_destroy(src_conf);
    src_conf = NULL;
    interface_conf_destroy(dst_conf);
    dst_conf = NULL;
    interface_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Abnormal case
  {
    rc = interface_conf_duplicate(NULL, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_conf_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  interface_conf_t *conf1 = NULL;
  interface_conf_t *conf2 = NULL;
  interface_conf_t *conf3 = NULL;
  const char *fullname1 = "conf1";
  const char *fullname2 = "conf2";
  const char *fullname3 = "conf3";

  interface_initialize();

  rc = interface_conf_create(&conf1, fullname1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new interface");
  rc = interface_conf_add(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = interface_conf_create(&conf2, fullname2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new interface");
  rc = interface_conf_add(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = interface_conf_create(&conf3, fullname3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new interface");
  rc = interface_conf_add(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = interface_set_enabled(fullname3, true);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = interface_conf_equals(conf1, conf2);
    TEST_ASSERT_TRUE(result);

    result = interface_conf_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = interface_conf_equals(conf1, conf3);
    TEST_ASSERT_FALSE(result);

    result = interface_conf_equals(conf2, conf3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = interface_conf_equals(conf1, NULL);
    TEST_ASSERT_FALSE(result);

    result = interface_conf_equals(NULL, conf2);
    TEST_ASSERT_FALSE(result);
  }

  rc = interface_conf_delete(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = interface_conf_delete(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = interface_conf_delete(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  interface_finalize();
}


void
test_interface_conf_add(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  interface_conf_t *actual_conf = NULL;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  // Normal case
  {
    rc = interface_conf_add(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = interface_conf_add(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = interface_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  interface_finalize();
}

void
test_interface_conf_add_not_initialize(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  interface_conf_destroy(conf);

  interface_finalize();
}

void
test_interface_conf_delete(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  interface_conf_t *actual_conf = NULL;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    rc = interface_conf_delete(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = interface_conf_delete(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = interface_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, rc);

  interface_finalize();
}

void
test_interface_conf_delete_not_initialize(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_delete(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  interface_conf_destroy(conf);

  interface_finalize();
}

void
test_interface_conf_iterate(void) {
  TEST_IGNORE();
}

void
test_interface_conf_list(void) {
  lagopus_result_t rc;
  interface_conf_t *conf1 = NULL;
  const char *name1 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"interface_name1";
  interface_conf_t *conf2 = NULL;
  const char *name2 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"interface_name2";
  interface_conf_t *conf3 = NULL;
  const char *name3 = "namespace2"DATASTORE_NAMESPACE_DELIMITER"interface_name3";
  interface_conf_t *conf4 = NULL;
  const char *name4 = DATASTORE_NAMESPACE_DELIMITER"interface_name4";
  interface_conf_t *conf5 = NULL;
  const char *name5 = DATASTORE_NAMESPACE_DELIMITER"interface_name5";
  interface_conf_t **actual_list = NULL;
  size_t i;

  interface_initialize();

  // create conf
  {
    rc = interface_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new interface");

    rc = interface_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new interface");

    rc = interface_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new interface");

    rc = interface_conf_create(&conf4, name4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf4, "conf_create() will create new interface");

    rc = interface_conf_create(&conf5, name5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf5, "conf_create() will create new interface");
  }

  // add conf
  {
    rc = interface_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = interface_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = interface_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = interface_conf_add(conf4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = interface_conf_add(conf5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // all
  {
    rc = interface_conf_list(&actual_list, NULL);
    TEST_ASSERT_EQUAL(5, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"interface_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"interface_name2") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace2"DATASTORE_NAMESPACE_DELIMITER"interface_name3") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"interface_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"interface_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // no namespace
  {
    rc = interface_conf_list(&actual_list, "");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"interface_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"interface_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // only namespace
  {
    rc = interface_conf_list(&actual_list, "namespace1");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"interface_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"interface_name2") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // Abnormal case
  {
    rc = interface_conf_list(NULL, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_conf_list_not_initialize(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  interface_conf_t ***list = NULL;

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_list(list, NULL);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  interface_conf_destroy(conf);

  interface_finalize();
}

void
test_interface_conf_one_list(void) {
  TEST_IGNORE();
}

void
test_interface_conf_one_list_not_initialize(void) {
  TEST_IGNORE();
}

void
test_interface_find(void) {
  lagopus_result_t rc;
  interface_conf_t *conf1 = NULL;
  const char *name1 = "interface_name1";
  interface_conf_t *conf2 = NULL;
  const char *name2 = "interface_name2";
  interface_conf_t *conf3 = NULL;
  const char *name3 = "interface_name3";
  interface_conf_t *actual_conf = NULL;

  interface_initialize();

  // create conf
  {
    rc = interface_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new interface");

    rc = interface_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new interface");

    rc = interface_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new interface");
  }

  // add conf
  {
    rc = interface_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = interface_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = interface_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Normal case
  {
    rc = interface_find(name1, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name1, actual_conf->name);

    rc = interface_find(name2, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name2, actual_conf->name);

    rc = interface_find(name3, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name3, actual_conf->name);
  }

  // Abnormal case
  {
    rc = interface_find(NULL, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_find(name1, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_find_not_initialize(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  interface_conf_t *actual_conf = NULL;

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  interface_conf_destroy(conf);

  interface_finalize();
}

void
test_interface_attr_create_and_destroy(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  uint32_t actual_port_number = 0;
  const uint32_t expected_port_number = actual_port_number;
  char *actual_device = NULL;
  const char *expected_device = "";
  datastore_interface_type_t actual_type = 0;
  const datastore_interface_type_t expected_type =
    DATASTORE_INTERFACE_TYPE_UNKNOWN;
  char *actual_dst_addr = NULL;
  const char *expected_dst_addr = "127.0.0.1";
  uint32_t actual_dst_port = 0;
  const uint32_t expected_dst_port = 0;
  char *actual_mcast_group = NULL;
  const char *expected_mcast_group = "224.0.0.1";
  char *actual_src_addr = NULL;
  const char *expected_src_addr = "127.0.0.1";
  uint32_t actual_src_port = 0;
  const uint32_t expected_src_port = 0;
  uint32_t actual_ni = 0;
  const uint32_t expected_ni = 0;
  uint8_t actual_ttl = 0;
  const uint8_t expected_ttl = 0;
  mac_address_t actual_dst_hw_addr = {0, 0, 0, 0, 0, 0};
  const mac_address_t expected_dst_hw_addr = {0, 0, 0, 0, 0, 0};
  mac_address_t actual_src_hw_addr = {0, 0, 0, 0, 0, 0};
  const mac_address_t expected_src_hw_addr = {0, 0, 0, 0, 0, 0};
  uint16_t actual_mtu = 0;
  const uint16_t expected_mtu = 1500;
  char *actual_ip_addr = NULL;
  const char *expected_ip_addr = "127.0.0.1";

  interface_initialize();

  // Normal case
  {
    rc = interface_attr_create(&attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

    // port_number
    rc = interface_get_port_number(attr, &actual_port_number);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_port_number, actual_port_number);

    // device
    rc = interface_get_device(attr, &actual_device);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_device, actual_device);

    // type
    rc = interface_get_type(attr, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_type, actual_type);

    // dst_addr
    rc = interface_get_dst_addr_str(attr, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(actual_dst_addr, expected_dst_addr);

    // dst_port
    rc = interface_get_dst_port(attr, &actual_dst_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_dst_port, actual_dst_port);

    // mcat_group
    rc = interface_get_mcast_group_str(attr, &actual_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(actual_mcast_group, expected_mcast_group);

    // src_addr
    rc = interface_get_src_addr_str(attr, &actual_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(actual_src_addr, expected_src_addr);

    // src_port
    rc = interface_get_src_port(attr, &actual_src_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_src_port, actual_src_port);

    // ni
    rc = interface_get_ni(attr, &actual_ni);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_ni, actual_ni);

    // ttl
    rc = interface_get_ttl(attr, &actual_ttl);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(expected_ttl, actual_ttl);

    // dst_hw_addr
    rc = interface_get_dst_hw_addr(attr, &actual_dst_hw_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_dst_hw_addr, actual_dst_hw_addr, 6);

    // src_hw_addr
    rc = interface_get_src_hw_addr(attr, &actual_src_hw_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_src_hw_addr, actual_src_hw_addr, 6);

    // mtu
    rc = interface_get_mtu(attr, &actual_mtu);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_mtu, actual_mtu);

    // ip_addr
    rc = interface_get_ip_addr_str(attr, &actual_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(actual_ip_addr, expected_ip_addr);
  }

  interface_attr_destroy(attr);
  free((void *)actual_device);
  free((void *)actual_dst_addr);
  free((void *)actual_mcast_group);
  free((void *)actual_src_addr);
  free((void *)actual_ip_addr);

  interface_finalize();
}

void
test_interface_attr_duplicate(void) {
  lagopus_result_t rc;
  bool result = false;
  interface_attr_t *src_attr = NULL;
  interface_attr_t *dst_attr = NULL;

  interface_initialize();

  rc = interface_attr_create(&src_attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(src_attr,
                               "attr_create() will create new interface");

  // Normal case
  {
    rc = interface_attr_duplicate(src_attr, &dst_attr, "namespace");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    result = interface_attr_equals(src_attr, dst_attr);
    TEST_ASSERT_TRUE(result);
    interface_attr_destroy(dst_attr);
    dst_attr = NULL;

    rc = interface_attr_duplicate(src_attr, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    result = interface_attr_equals(src_attr, dst_attr);
    TEST_ASSERT_TRUE(result);
    interface_attr_destroy(dst_attr);
    dst_attr = NULL;
  }

  // Abnormal case
  {
    result = interface_attr_equals(src_attr, NULL);
    TEST_ASSERT_FALSE(result);

    rc = interface_attr_duplicate(src_attr, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    result = interface_attr_equals(NULL, dst_attr);
    TEST_ASSERT_FALSE(result);
    interface_attr_destroy(dst_attr);
    dst_attr = NULL;
  }

  interface_attr_destroy(src_attr);

  interface_finalize();
}

void
test_interface_attr_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  interface_attr_t *attr1 = NULL;
  interface_attr_t *attr2 = NULL;
  interface_attr_t *attr3 = NULL;

  interface_initialize();

  rc = interface_attr_create(&attr1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr1, "attr_create() will create new interface");

  rc = interface_attr_create(&attr2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr2, "attr_create() will create new interface");

  rc = interface_attr_create(&attr3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr3, "attr_create() will create new interface");
  rc = interface_set_port_number(attr3, 10);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = interface_attr_equals(attr1, attr2);
    TEST_ASSERT_TRUE(result);

    result = interface_attr_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = interface_attr_equals(attr1, attr3);
    TEST_ASSERT_FALSE(result);

    result = interface_attr_equals(attr2, attr3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = interface_attr_equals(attr1, NULL);
    TEST_ASSERT_FALSE(result);

    result = interface_attr_equals(NULL, attr2);
    TEST_ASSERT_FALSE(result);
  }

  interface_attr_destroy(attr1);
  interface_attr_destroy(attr2);
  interface_attr_destroy(attr3);

  interface_finalize();
}

void
test_interface_get_attr(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  interface_attr_t *attr = NULL;
  const char *name = "interface_name";

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = interface_get_attr(name, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = interface_get_attr(name, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(attr);
  }

  // Abnormal case of getter
  {
    rc = interface_get_attr(NULL, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_attr(NULL, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_attr(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_attr(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_type_to_str(void) {
  lagopus_result_t rc;
  const char *actual_type_str;

  // Normal case
  {
    rc = interface_type_to_str(DATASTORE_INTERFACE_TYPE_UNKNOWN, &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_type_str);

    rc = interface_type_to_str(DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY,
                               &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("ethernet-dpdk-phy", actual_type_str);

    rc = interface_type_to_str(DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV,
                               &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("ethernet-dpdk-vdev", actual_type_str);

    rc = interface_type_to_str(DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK,
                               &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("ethernet-rawsock", actual_type_str);

    rc = interface_type_to_str(DATASTORE_INTERFACE_TYPE_GRE, &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("gre", actual_type_str);

    rc = interface_type_to_str(DATASTORE_INTERFACE_TYPE_NVGRE, &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("nvgre", actual_type_str);

    rc = interface_type_to_str(DATASTORE_INTERFACE_TYPE_VXLAN, &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("vxlan", actual_type_str);

    rc = interface_type_to_str(DATASTORE_INTERFACE_TYPE_VHOST_USER,
                               &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("vhost-user", actual_type_str);

    rc = interface_type_to_str(DATASTORE_INTERFACE_TYPE_MIN, &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_type_str);

    rc = interface_type_to_str(DATASTORE_INTERFACE_TYPE_MAX, &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("vhost-user", actual_type_str);
  }

  // Abnormal case
  {
    rc = interface_type_to_str(DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_interface_type_to_enum(void) {
  lagopus_result_t rc;
  datastore_interface_type_t actual_type;

  // Normal case
  {
    rc = interface_type_to_enum("unknown", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_INTERFACE_TYPE_UNKNOWN, actual_type);

    rc = interface_type_to_enum("ethernet-dpdk-phy", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_PHY,
                             actual_type);

    rc = interface_type_to_enum("ethernet-dpdk-vdev", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_INTERFACE_TYPE_ETHERNET_DPDK_VDEV,
                             actual_type);

    rc = interface_type_to_enum("ethernet-rawsock", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_INTERFACE_TYPE_ETHERNET_RAWSOCK,
                             actual_type);

    rc = interface_type_to_enum("gre", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_INTERFACE_TYPE_GRE, actual_type);

    rc = interface_type_to_enum("nvgre", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_INTERFACE_TYPE_NVGRE, actual_type);

    rc = interface_type_to_enum("vxlan", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_INTERFACE_TYPE_VXLAN, actual_type);

    rc = interface_type_to_enum("vhost-user", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_INTERFACE_TYPE_VHOST_USER, actual_type);

    rc = interface_type_to_enum("UNKNOWN", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_type_to_enum("ETHERNET", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_type_to_enum("GRE", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_type_to_enum("NVGRE", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_type_to_enum("VXLAN", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_type_to_enum("VHOST-USER", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case
  {
    rc = interface_type_to_enum(NULL, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_type_to_enum("ethernet", NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_interface_conf_private_exists(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name1";
  const char *invalid_name = "invalid_name";

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    TEST_ASSERT_TRUE(interface_exists(name) == true);
    TEST_ASSERT_TRUE(interface_exists(invalid_name) == false);
  }

  // Abnormal case
  {
    TEST_ASSERT_TRUE(interface_exists(NULL) == false);
  }

  interface_finalize();
}

void
test_interface_conf_private_used(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  bool actual_used = false;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = interface_set_used(name, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_interface_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = interface_set_used(NULL, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_conf_public_used(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  bool actual_used = false;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_is_used(NULL, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_conf_private_enabled(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  bool actual_enabled = false;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = interface_set_enabled(name, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_interface_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = interface_set_enabled(NULL, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_conf_public_enabled(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  bool actual_enabled = false;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_is_enabled(NULL, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_attr_private_port_number(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
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

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

  // Normal case of getter
  {
    rc = interface_get_port_number(attr, &actual_port_number);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_port_number, actual_port_number);
  }

  // Abnormal case of getter
  {
    rc = interface_get_port_number(NULL, &actual_port_number);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_port_number(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = interface_set_port_number(attr, set_port_number1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_port_number(attr, &actual_set_port_number1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_port_number1, actual_set_port_number1);

    rc = interface_set_port_number(attr, set_port_number2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_port_number(attr, &actual_set_port_number2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_port_number2, actual_set_port_number2);
  }

  // Abnormal case of setter
  {
    rc = interface_set_port_number(NULL, set_port_number1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_port_number(attr, set_port_number3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = interface_get_port_number(attr, &actual_set_port_number3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_port_number3, actual_set_port_number3);

    rc = interface_set_port_number(attr, CAST_UINT64(set_port_number4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = interface_get_port_number(attr, &actual_set_port_number4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_port_number4, actual_set_port_number4);
  }

  interface_attr_destroy(attr);

  interface_finalize();
}

void
test_interface_attr_public_port_number(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  uint32_t actual_port_number = 0;
  const uint32_t expected_port_number = actual_port_number;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_port_number(name, true, &actual_port_number);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_port_number(name, false, &actual_port_number);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_port_number, actual_port_number);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_port_number(NULL, true, &actual_port_number);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_port_number(NULL, false, &actual_port_number);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_port_number(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_port_number(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
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
test_interface_create_str(void) {
  char *actual_str = NULL;
  const char *expected_str = "aaa";

  create_str(&actual_str, strlen(expected_str));
  TEST_ASSERT_EQUAL_STRING(expected_str, actual_str);

  free((void *)actual_str);
}

void
test_interface_attr_private_device(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  char *actual_device = NULL;
  const char *expected_device = "";
  const char *set_device1 = "device";
  char *set_device2 = NULL;
  char *actual_set_device1 = NULL;
  char *actual_set_device2 = NULL;
  const char *expected_set_device1 = set_device1;
  const char *expected_set_device2 = set_device1;

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new port");

  // Normal case of getter
  {
    rc = interface_get_device(attr, &actual_device);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_device, actual_device);
  }

  // Abnormal case of getter
  {
    rc = interface_get_device(NULL, &actual_device);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_device(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = interface_set_device(attr, set_device1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_device(attr, &actual_set_device1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_device1,
                             actual_set_device1);

    create_str(&set_device2, DATASTORE_INTERFACE_FULLNAME_MAX + 1);
    rc = interface_set_device(attr, set_device2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = interface_get_device(attr, &actual_set_device2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_device2,
                             actual_set_device2);
  }

  // Abnormal case of setter
  {
    rc = interface_set_device(NULL, set_device1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_device(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_attr_destroy(attr);
  free((void *)actual_device);
  free((void *)set_device2);
  free((void *)actual_set_device1);
  free((void *)actual_set_device2);

  interface_finalize();
}

void
test_interface_attr_public_device(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  char *actual_device = NULL;
  const char *expected_device = "";

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new port");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_device(name, true, &actual_device);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_device(name, false, &actual_device);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_device, actual_device);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_device(NULL, true, &actual_device);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_device(NULL, false, &actual_device);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_device(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_device(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  free((void *)actual_device);
  interface_finalize();
}

void
test_interface_attr_private_type(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  datastore_interface_type_t actual_type = 0;
  datastore_interface_type_t expected_type = DATASTORE_INTERFACE_TYPE_UNKNOWN;
  datastore_interface_type_t set_type = DATASTORE_INTERFACE_TYPE_VHOST_USER;
  datastore_interface_type_t actual_set_type = 0;
  datastore_interface_type_t expected_set_type = set_type;

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

  // Normal case of getter
  {
    rc = interface_get_type(attr, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_type, actual_type);
  }

  // Abnormal case of getter
  {
    rc = interface_get_type(NULL, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_type(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = interface_set_port_number(attr, set_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_port_number(attr, &actual_set_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_type, actual_set_type);
  }

  // Abnormal case of setter
  {
    rc = interface_set_type(NULL, actual_set_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_attr_destroy(attr);

  interface_finalize();
}

void
test_interface_attr_public_type(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  datastore_interface_type_t actual_type = 0;
  datastore_interface_type_t expected_type = DATASTORE_INTERFACE_TYPE_UNKNOWN;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_type(name, true, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_type(name, false, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_type, actual_type);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_type(NULL, true, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_type(NULL, false, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_type(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_type(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_attr_private_dst_addr(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  lagopus_ip_address_t *actual_dst_addr = NULL;
  const char *expected_dst_addr_str = "127.0.0.1";
  lagopus_ip_address_t *expected_dst_addr = NULL;

  const char *set_dst_addr_str = "192.168.0.1";
  lagopus_ip_address_t *set_dst_addr = NULL;
  lagopus_ip_address_t *actual_set_dst_addr = NULL;
  lagopus_ip_address_t *expected_set_dst_addr = NULL;

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new channel");

  // Normal case of getter
  {
    rc = interface_get_dst_addr(attr, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(expected_dst_addr_str, true,
                                   &expected_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_dst_addr,
                     actual_dst_addr));
  }

  // Abnormal case of getter
  {
    rc = interface_get_dst_addr(NULL, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = lagopus_ip_address_create(set_dst_addr_str, true, &set_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_set_dst_addr(attr, set_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_dst_addr(attr, &actual_set_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(set_dst_addr_str, true, &expected_set_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_set_dst_addr,
                     actual_set_dst_addr));
  }

  // Abnormal case of setter
  {
    rc = interface_set_dst_addr(NULL, actual_set_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_dst_addr(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  lagopus_ip_address_destroy(actual_dst_addr);
  lagopus_ip_address_destroy(expected_dst_addr);
  lagopus_ip_address_destroy(set_dst_addr);
  lagopus_ip_address_destroy(actual_set_dst_addr);
  lagopus_ip_address_destroy(expected_set_dst_addr);
  interface_attr_destroy(attr);
  interface_finalize();
}

void
test_interface_attr_public_dst_addr(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  lagopus_ip_address_t *actual_dst_addr = NULL;
  const char *expected_dst_addr_str = "127.0.0.1";
  lagopus_ip_address_t *expected_dst_addr = NULL;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_dst_addr(name, true, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_dst_addr(name, false, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(expected_dst_addr_str, true,
                                   &expected_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_dst_addr,
                     actual_dst_addr));
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_dst_addr(NULL, true, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_dst_addr(NULL, false, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_dst_addr(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_dst_addr(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  lagopus_ip_address_destroy(actual_dst_addr);
  lagopus_ip_address_destroy(expected_dst_addr);
  interface_finalize();
}

void
test_interface_attr_private_dst_addr_str(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  char *actual_dst_addr = NULL;
  const char *expected_dst_addr = "127.0.0.1";
  const char *set_dst_addr = "192.168.0.1";
  char *actual_set_dst_addr = NULL;
  const char *expected_set_dst_addr = set_dst_addr;
  const char *invalid_dst_addr = "256.256.256.256";

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

  // Normal case of getter
  {
    rc = interface_get_dst_addr_str(attr, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_dst_addr, actual_dst_addr);
  }

  // Abnormal case of getter
  {
    rc = interface_get_dst_addr_str(NULL, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_dst_addr_str(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = interface_set_dst_addr_str(attr, set_dst_addr, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_dst_addr_str(attr, &actual_set_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_dst_addr, actual_set_dst_addr);
  }

  // Abnormal case of setter
  {
    rc = interface_set_dst_addr_str(NULL, actual_set_dst_addr, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_dst_addr_str(attr, NULL, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_dst_addr_str(attr, invalid_dst_addr, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ADDR_RESOLVER_FAILURE, rc);
  }

  interface_attr_destroy(attr);
  free((void *)actual_dst_addr);
  free((void *)actual_set_dst_addr);

  interface_finalize();
}

void
test_interface_attr_public_dst_addr_str(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  char *actual_dst_addr = NULL;
  const char *expected_dst_addr = "127.0.0.1";

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_dst_addr_str(name, true, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_src_addr_str(name, false, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_dst_addr, actual_dst_addr);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_dst_addr_str(NULL, true, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_dst_addr_str(NULL, false, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_dst_addr_str(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_dst_addr_str(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  free((void *)actual_dst_addr);
  interface_finalize();
}

void
test_interface_attr_private_dst_port(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  uint32_t actual_dst_port = 0;
  uint32_t expected_dst_port = 0;
  const uint64_t set_dst_port1 = CAST_UINT64(MAXIMUM_DST_PORT);
  const uint64_t set_dst_port2 = CAST_UINT64(MINIMUM_DST_PORT);
  const uint64_t set_dst_port3 = CAST_UINT64(MAXIMUM_DST_PORT + 1);
  const int set_dst_port4 = -1;
  uint32_t actual_set_dst_port1 = 0;
  uint32_t actual_set_dst_port2 = 0;
  uint32_t actual_set_dst_port3 = 0;
  uint32_t actual_set_dst_port4 = 0;
  const uint32_t expected_set_dst_port1 = MAXIMUM_DST_PORT;
  const uint32_t expected_set_dst_port2 = MINIMUM_DST_PORT;
  const uint32_t expected_set_dst_port3 = MINIMUM_DST_PORT;
  const uint32_t expected_set_dst_port4 = MINIMUM_DST_PORT;

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

  // Normal case of getter
  {
    rc = interface_get_dst_port(attr, &actual_dst_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_dst_port, actual_dst_port);
  }

  // Abnormal case of getter
  {
    rc = interface_get_dst_port(NULL, &actual_dst_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_dst_port(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = interface_set_dst_port(attr, set_dst_port1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_dst_port(attr, &actual_set_dst_port1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_dst_port1, actual_set_dst_port1);

    rc = interface_set_dst_port(attr, set_dst_port2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_dst_port(attr, &actual_set_dst_port2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_dst_port2, actual_set_dst_port2);
  }

  // Abnormal case of setter
  {
    rc = interface_set_dst_port(NULL, actual_set_dst_port1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_dst_port(attr, set_dst_port3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = interface_get_dst_port(attr, &actual_set_dst_port3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_dst_port3, actual_set_dst_port3);

    rc = interface_set_dst_port(attr, CAST_UINT64(set_dst_port4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = interface_get_dst_port(attr, &actual_set_dst_port4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_dst_port4, actual_set_dst_port4);
  }

  interface_attr_destroy(attr);

  interface_finalize();
}

void
test_interface_attr_public_dst_port(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  uint32_t actual_dst_port = 0;
  uint32_t expected_dst_port = 0;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_dst_port(name, true, &actual_dst_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_dst_port(name, false, &actual_dst_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_dst_port, actual_dst_port);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_dst_port(NULL, true, &actual_dst_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_dst_port(NULL, false, &actual_dst_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_dst_port(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_dst_port(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_attr_private_mcast_group(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  lagopus_ip_address_t *actual_mcast_group = NULL;
  const char *expected_mcast_group_str = "224.0.0.1";
  lagopus_ip_address_t *expected_mcast_group = NULL;

  const char *set_mcast_group_str = "192.168.0.1";
  lagopus_ip_address_t *set_mcast_group = NULL;
  lagopus_ip_address_t *actual_set_mcast_group = NULL;
  lagopus_ip_address_t *expected_set_mcast_group = NULL;

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new channel");

  // Normal case of getter
  {
    rc = interface_get_mcast_group(attr, &actual_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(expected_mcast_group_str, true,
                                   &expected_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_mcast_group,
                     actual_mcast_group));
  }

  // Abnormal case of getter
  {
    rc = interface_get_mcast_group(NULL, &actual_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = lagopus_ip_address_create(set_mcast_group_str, true, &set_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_set_mcast_group(attr, set_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_mcast_group(attr, &actual_set_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(set_mcast_group_str, true,
                                   &expected_set_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_set_mcast_group,
                     actual_set_mcast_group));
  }

  // Abnormal case of setter
  {
    rc = interface_set_mcast_group(NULL, actual_set_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_mcast_group(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  lagopus_ip_address_destroy(actual_mcast_group);
  lagopus_ip_address_destroy(expected_mcast_group);
  lagopus_ip_address_destroy(set_mcast_group);
  lagopus_ip_address_destroy(actual_set_mcast_group);
  lagopus_ip_address_destroy(expected_set_mcast_group);
  interface_attr_destroy(attr);
  interface_finalize();
}

void
test_interface_attr_public_mcast_group(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  lagopus_ip_address_t *actual_mcast_group = NULL;
  const char *expected_mcast_group_str = "224.0.0.1";
  lagopus_ip_address_t *expected_mcast_group = NULL;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_mcast_group(name, true, &actual_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_mcast_group(name, false, &actual_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(expected_mcast_group_str, true,
                                   &expected_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_mcast_group,
                     actual_mcast_group));
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_mcast_group(NULL, true, &actual_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_mcast_group(NULL, false, &actual_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_mcast_group(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_mcast_group(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  lagopus_ip_address_destroy(actual_mcast_group);
  lagopus_ip_address_destroy(expected_mcast_group);
  interface_finalize();
}

void
test_interface_attr_private_mcast_group_str(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  char *actual_mcast_group = NULL;
  const char *expected_mcast_group = "224.0.0.1";
  const char *set_mcast_group = "192.168.0.1";
  char *actual_set_mcast_group = NULL;
  const char *expected_set_mcast_group = set_mcast_group;
  const char *invalid_mcast_group = "256.256.256.256";

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

  // Normal case of getter
  {
    rc = interface_get_mcast_group_str(attr, &actual_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_mcast_group, actual_mcast_group);
  }

  // Abnormal case of getter
  {
    rc = interface_get_mcast_group_str(NULL, &actual_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_mcast_group_str(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = interface_set_mcast_group_str(attr, set_mcast_group, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_mcast_group_str(attr, &actual_set_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_mcast_group, actual_set_mcast_group);
  }

  // Abnormal case of setter
  {
    rc = interface_set_mcast_group_str(NULL, actual_set_mcast_group, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_mcast_group_str(attr, NULL, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_mcast_group_str(attr, invalid_mcast_group, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ADDR_RESOLVER_FAILURE, rc);
  }

  interface_attr_destroy(attr);
  free((void *)actual_mcast_group);
  free((void *)actual_set_mcast_group);
  interface_finalize();
}

void
test_interface_attr_public_mcast_group_str(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  char *actual_mcast_group = NULL;
  const char *expected_mcast_group = "224.0.0.1";

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_mcast_group_str(name, true, &actual_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_mcast_group_str(name, false, &actual_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_mcast_group, actual_mcast_group);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_mcast_group_str(NULL, true, &actual_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_mcast_group_str(NULL, false, &actual_mcast_group);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_mcast_group_str(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_mcast_group_str(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  free((void *)actual_mcast_group);
  interface_finalize();
}

void
test_interface_attr_private_src_addr(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  lagopus_ip_address_t *actual_src_addr = NULL;
  const char *expected_src_addr_str = "127.0.0.1";
  lagopus_ip_address_t *expected_src_addr = NULL;

  const char *set_src_addr_str = "192.168.0.1";
  lagopus_ip_address_t *set_src_addr = NULL;
  lagopus_ip_address_t *actual_set_src_addr = NULL;
  lagopus_ip_address_t *expected_set_src_addr = NULL;

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new channel");

  // Normal case of getter
  {
    rc = interface_get_src_addr(attr, &actual_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(expected_src_addr_str, true,
                                   &expected_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_src_addr,
                     actual_src_addr));
  }

  // Abnormal case of getter
  {
    rc = interface_get_src_addr(NULL, &actual_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = lagopus_ip_address_create(set_src_addr_str, true, &set_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_set_src_addr(attr, set_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_src_addr(attr, &actual_set_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(set_src_addr_str, true, &expected_set_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_set_src_addr,
                     actual_set_src_addr));
  }

  // Abnormal case of setter
  {
    rc = interface_set_src_addr(NULL, actual_set_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_src_addr(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  lagopus_ip_address_destroy(actual_src_addr);
  lagopus_ip_address_destroy(expected_src_addr);
  lagopus_ip_address_destroy(set_src_addr);
  lagopus_ip_address_destroy(actual_set_src_addr);
  lagopus_ip_address_destroy(expected_set_src_addr);
  interface_attr_destroy(attr);
  interface_finalize();
}

void
test_interface_attr_public_src_addr(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  lagopus_ip_address_t *actual_src_addr = NULL;
  const char *expected_src_addr_str = "127.0.0.1";
  lagopus_ip_address_t *expected_src_addr = NULL;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_src_addr(name, true, &actual_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_src_addr(name, false, &actual_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(expected_src_addr_str, true,
                                   &expected_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_src_addr,
                     actual_src_addr));
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_src_addr(NULL, true, &actual_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_src_addr(NULL, false, &actual_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_src_addr(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_src_addr(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  lagopus_ip_address_destroy(actual_src_addr);
  lagopus_ip_address_destroy(expected_src_addr);
  interface_finalize();
}


void
test_interface_attr_private_src_addr_str(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  char *actual_src_addr = NULL;
  const char *expected_src_addr = "127.0.0.1";
  const char *set_src_addr = "192.168.0.1";
  char *actual_set_src_addr = NULL;
  const char *expected_set_src_addr = set_src_addr;
  const char *invalid_src_addr = "256.256.256.256";

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

  // Normal case of getter
  {
    rc = interface_get_src_addr_str(attr, &actual_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_src_addr, actual_src_addr);
  }

  // Abnormal case of getter
  {
    rc = interface_get_src_addr_str(NULL, &actual_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_src_addr_str(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = interface_set_src_addr_str(attr, set_src_addr, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_src_addr_str(attr, &actual_set_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_src_addr, actual_set_src_addr);
  }

  // Abnormal case of setter
  {
    rc = interface_set_src_addr_str(NULL, actual_set_src_addr, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_src_addr_str(attr, NULL, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_src_addr_str(attr, invalid_src_addr, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ADDR_RESOLVER_FAILURE, rc);
  }

  interface_attr_destroy(attr);
  free((void *)actual_src_addr);
  free((void *)actual_set_src_addr);

  interface_finalize();
}

void
test_interface_attr_public_src_addr_str(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  char *actual_src_addr = NULL;
  const char *expected_src_addr = "127.0.0.1";

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_src_addr_str(name, true, &actual_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_src_addr_str(name, false, &actual_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_src_addr, actual_src_addr);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_src_addr_str(NULL, true, &actual_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_src_addr_str(NULL, false, &actual_src_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_src_addr_str(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_src_addr_str(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  free((void *)actual_src_addr);
  interface_finalize();
}

void
test_interface_attr_private_src_port(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  uint32_t actual_src_port = 0;
  uint32_t expected_src_port = 0;
  const uint64_t set_src_port1 = CAST_UINT64(MAXIMUM_SRC_PORT);
  const uint64_t set_src_port2 = CAST_UINT64(MINIMUM_SRC_PORT);
  const uint64_t set_src_port3 = CAST_UINT64(MAXIMUM_SRC_PORT + 1);
  const int set_src_port4 = -1;
  uint32_t actual_set_src_port1 = 0;
  uint32_t actual_set_src_port2 = 0;
  uint32_t actual_set_src_port3 = 0;
  uint32_t actual_set_src_port4 = 0;
  const uint32_t expected_set_src_port1 = MAXIMUM_SRC_PORT;
  const uint32_t expected_set_src_port2 = MINIMUM_SRC_PORT;
  const uint32_t expected_set_src_port3 = MINIMUM_SRC_PORT;
  const uint32_t expected_set_src_port4 = MINIMUM_SRC_PORT;

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

  // Normal case of getter
  {
    rc = interface_get_src_port(attr, &actual_src_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_src_port, actual_src_port);
  }

  // Abnormal case of getter
  {
    rc = interface_get_src_port(NULL, &actual_src_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_src_port(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = interface_set_src_port(attr, set_src_port1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_src_port(attr, &actual_set_src_port1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_src_port1, actual_set_src_port1);

    rc = interface_set_src_port(attr, set_src_port2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_src_port(attr, &actual_set_src_port2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_src_port2, actual_set_src_port2);
  }

  // Abnormal case of setter
  {
    rc = interface_set_src_port(NULL, actual_set_src_port1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_src_port(attr, set_src_port3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = interface_get_src_port(attr, &actual_set_src_port3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_src_port3, actual_set_src_port3);

    rc = interface_set_src_port(attr, CAST_UINT64(set_src_port4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = interface_get_src_port(attr, &actual_set_src_port4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_src_port4, actual_set_src_port4);
  }

  interface_attr_destroy(attr);

  interface_finalize();
}

void
test_interface_attr_public_src_port(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  uint32_t actual_src_port = 0;
  uint32_t expected_src_port = 0;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_src_port(name, true, &actual_src_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_src_port(name, false, &actual_src_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_src_port, actual_src_port);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_src_port(NULL, true, &actual_src_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_src_port(NULL, false, &actual_src_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_src_port(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_src_port(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_attr_private_ni(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  uint32_t actual_ni = 0;
  uint32_t expected_ni = 0;
  const uint64_t set_ni1 = CAST_UINT64(MAXIMUM_NI);
  const uint64_t set_ni2 = CAST_UINT64(MINIMUM_NI);
  const uint64_t set_ni3 = CAST_UINT64(MAXIMUM_NI + 1);
  const int set_ni4 = -1;
  uint32_t actual_set_ni1 = 0;
  uint32_t actual_set_ni2 = 0;
  uint32_t actual_set_ni3 = 0;
  uint32_t actual_set_ni4 = 0;
  const uint32_t expected_set_ni1 = MAXIMUM_NI;
  const uint32_t expected_set_ni2 = MINIMUM_NI;
  const uint32_t expected_set_ni3 = MINIMUM_NI;
  const uint32_t expected_set_ni4 = MINIMUM_NI;

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

  // Normal case of getter
  {
    rc = interface_get_ni(attr, &actual_ni);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_ni, actual_ni);
  }

  // Abnormal case of getter
  {
    rc = interface_get_ni(NULL, &actual_ni);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_ni(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = interface_set_ni(attr, set_ni1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_ni(attr, &actual_set_ni1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_ni1, actual_set_ni1);

    rc = interface_set_ni(attr, set_ni2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_ni(attr, &actual_set_ni2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_ni2, actual_set_ni2);
  }

  // Abnormal case of setter
  {
    rc = interface_set_ni(NULL, actual_set_ni1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_ni(attr, set_ni3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = interface_get_ni(attr, &actual_set_ni3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_ni3, actual_set_ni3);

    rc = interface_set_ni(attr, CAST_UINT64(set_ni4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = interface_get_ni(attr, &actual_set_ni4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_ni4, actual_set_ni4);
  }

  interface_attr_destroy(attr);

  interface_finalize();
}

void
test_interface_attr_public_ni(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  uint32_t actual_ni = 0;
  uint32_t expected_ni = 0;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_ni(name, true, &actual_ni);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_ni(name, false, &actual_ni);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_ni, actual_ni);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_ni(NULL, true, &actual_ni);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_ni(NULL, false, &actual_ni);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_ni(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_ni(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_attr_private_ttl(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  uint8_t actual_ttl = 0;
  uint8_t expected_ttl = 0;
  const uint64_t set_ttl1 = CAST_UINT64(MAXIMUM_TTL);
  const uint64_t set_ttl2 = CAST_UINT64(MINIMUM_TTL);
  const uint64_t set_ttl3 = CAST_UINT64(MAXIMUM_TTL + 1);
  const int set_ttl4 = -1;
  uint8_t actual_set_ttl1 = 0;
  uint8_t actual_set_ttl2 = 0;
  uint8_t actual_set_ttl3 = 0;
  uint8_t actual_set_ttl4 = 0;
  const uint8_t expected_set_ttl1 = MAXIMUM_TTL;
  const uint8_t expected_set_ttl2 = MINIMUM_TTL;
  const uint8_t expected_set_ttl3 = MINIMUM_TTL;
  const uint8_t expected_set_ttl4 = MINIMUM_TTL;

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

  // Normal case of getter
  {
    rc = interface_get_ttl(attr, &actual_ttl);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(expected_ttl, actual_ttl);
  }

  // Abnormal case of getter
  {
    rc = interface_get_ttl(NULL, &actual_ttl);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_ni(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = interface_set_ttl(attr, set_ttl1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_ttl(attr, &actual_set_ttl1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(expected_set_ttl1, actual_set_ttl1);

    rc = interface_set_ttl(attr, set_ttl2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_ttl(attr, &actual_set_ttl2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(expected_set_ttl2, actual_set_ttl2);
  }

  // Abnormal case of setter
  {
    rc = interface_set_ttl(NULL, actual_set_ttl1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_ttl(attr, set_ttl3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = interface_get_ttl(attr, &actual_set_ttl3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(expected_set_ttl3, actual_set_ttl3);

    rc = interface_set_ttl(attr, CAST_UINT64(set_ttl4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = interface_get_ttl(attr, &actual_set_ttl4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(expected_set_ttl4, actual_set_ttl4);
  }

  interface_attr_destroy(attr);

  interface_finalize();
}

void
test_interface_attr_public_ttl(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  uint8_t actual_ttl = 0;
  uint8_t expected_ttl = 0;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_ttl(name, true, &actual_ttl);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_ttl(name, false, &actual_ttl);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8(expected_ttl, actual_ttl);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_ttl(NULL, true, &actual_ttl);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_ttl(NULL, false, &actual_ttl);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_ttl(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_ttl(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_attr_private_dst_hw_addr(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  mac_address_t actual_dst_hw_addr = {0, 0, 0, 0, 0, 0};
  mac_address_t expected_dst_hw_addr = {0, 0, 0, 0, 0, 0};
  mac_address_t set_dst_hw_addr = {0xff, 0xff, 0xff, 0x99, 0x99, 0x99};
  mac_address_t actual_set_dst_hw_addr = {0, 0, 0, 0, 0, 0};
  mac_address_t expected_set_dst_hw_addr = {0xff, 0xff, 0xff, 0x99, 0x99, 0x99};

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

  // Normal case of getter
  {
    rc = interface_get_dst_hw_addr(attr, &actual_dst_hw_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_dst_hw_addr, actual_dst_hw_addr, 6);
  }

  // Abnormal case of getter
  {
    rc = interface_get_dst_hw_addr(NULL, &actual_dst_hw_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_dst_hw_addr(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = interface_set_dst_hw_addr(attr, set_dst_hw_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_dst_hw_addr(attr, &actual_set_dst_hw_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_set_dst_hw_addr, actual_set_dst_hw_addr,
                                  6);
  }

  // Abnormal case of setter
  {
    rc = interface_set_dst_hw_addr(NULL, set_dst_hw_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_attr_destroy(attr);

  interface_finalize();
}

void
test_interface_attr_public_dst_hw_addr(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  uint8_t actual_dst_hw_addr[6] = {0, 0, 0, 0, 0, 0};
  uint8_t expected_dst_hw_addr[6] = {0, 0, 0, 0, 0, 0};

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_dst_hw_addr(name, true, actual_dst_hw_addr, 6);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_dst_hw_addr(name, false, actual_dst_hw_addr, 6);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_dst_hw_addr, actual_dst_hw_addr, 6);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_dst_hw_addr(NULL, true, actual_dst_hw_addr, 6);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_dst_hw_addr(NULL, false, actual_dst_hw_addr, 6);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_dst_hw_addr(name, true, NULL, 6);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_dst_hw_addr(name, false, NULL, 6);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_dst_hw_addr(name, false, actual_dst_hw_addr, 5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_dst_hw_addr(name, false, actual_dst_hw_addr, 7);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_attr_private_src_hw_addr(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  mac_address_t actual_src_hw_addr = {0, 0, 0, 0, 0, 0};
  mac_address_t expected_src_hw_addr = {0, 0, 0, 0, 0, 0};
  mac_address_t set_src_hw_addr = {0xff, 0xff, 0xff, 0x55, 0x55, 0x55};
  mac_address_t actual_set_src_hw_addr = {0, 0, 0, 0, 0, 0};
  mac_address_t expected_set_src_hw_addr = {0xff, 0xff, 0xff, 0x55, 0x55, 0x55};

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

  // Normal case of getter
  {
    rc = interface_get_src_hw_addr(attr, &actual_src_hw_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_src_hw_addr, actual_src_hw_addr, 6);
  }

  // Abnormal case of getter
  {
    rc = interface_get_src_hw_addr(NULL, &actual_src_hw_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_src_hw_addr(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = interface_set_src_hw_addr(attr, set_src_hw_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_src_hw_addr(attr, &actual_set_src_hw_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_set_src_hw_addr, actual_set_src_hw_addr,
                                  6);
  }

  // Abnormal case of setter
  {
    rc = interface_set_src_hw_addr(NULL, set_src_hw_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_attr_destroy(attr);

  interface_finalize();
}

void
test_interface_attr_public_src_hw_addr(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  uint8_t actual_src_hw_addr[6] = {0, 0, 0, 0, 0, 0};
  uint8_t expected_src_hw_addr[6] = {0, 0, 0, 0, 0, 0};

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_src_hw_addr(name, true, actual_src_hw_addr, 6);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_src_hw_addr(name, false, actual_src_hw_addr, 6);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_src_hw_addr, actual_src_hw_addr, 6);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_src_hw_addr(NULL, true, actual_src_hw_addr, 6);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_src_hw_addr(NULL, false, actual_src_hw_addr, 6);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_src_hw_addr(name, true, NULL, 6);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_src_hw_addr(name, false, NULL, 6);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_src_hw_addr(name, false, actual_src_hw_addr, 5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_src_hw_addr(name, false, actual_src_hw_addr, 7);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_attr_private_mtu(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  uint16_t actual_mtu = 0;
  uint16_t expected_mtu = 1500;
  const uint64_t set_mtu1 = CAST_UINT64(MAXIMUM_MTU);
  const uint64_t set_mtu2 = CAST_UINT64(MINIMUM_MTU);
  const uint64_t set_mtu3 = CAST_UINT64(MAXIMUM_MTU + 1);
  const int set_mtu4 = -1;
  uint16_t actual_set_mtu1 = 0;
  uint16_t actual_set_mtu2 = 0;
  uint16_t actual_set_mtu3 = 0;
  uint16_t actual_set_mtu4 = 0;
  const uint16_t expected_set_mtu1 = MAXIMUM_MTU;
  const uint16_t expected_set_mtu2 = MINIMUM_MTU;
  const uint16_t expected_set_mtu3 = MINIMUM_MTU;
  const uint16_t expected_set_mtu4 = MINIMUM_MTU;

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

  // Normal case of getter
  {
    rc = interface_get_mtu(attr, &actual_mtu);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_mtu, actual_mtu);
  }

  // Abnormal case of getter
  {
    rc = interface_get_mtu(NULL, &actual_mtu);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_ni(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = interface_set_mtu(attr, set_mtu1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_mtu(attr, &actual_set_mtu1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_mtu1, actual_set_mtu1);

    rc = interface_set_mtu(attr, set_mtu2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_mtu(attr, &actual_set_mtu2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_mtu2, actual_set_mtu2);
  }

  // Abnormal case of setter
  {
    rc = interface_set_mtu(NULL, actual_set_mtu1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_mtu(attr, set_mtu3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = interface_get_mtu(attr, &actual_set_mtu3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_mtu3, actual_set_mtu3);

    rc = interface_set_mtu(attr, CAST_UINT64(set_mtu4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = interface_get_mtu(attr, &actual_set_mtu4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_mtu4, actual_set_mtu4);
  }

  interface_attr_destroy(attr);

  interface_finalize();
}

void
test_interface_attr_public_mtu(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  uint16_t actual_mtu = 0;
  uint16_t expected_mtu = 1500;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_mtu(name, true, &actual_mtu);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_mtu(name, false, &actual_mtu);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_mtu, actual_mtu);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_mtu(NULL, true, &actual_mtu);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_mtu(NULL, false, &actual_mtu);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_mtu(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_mtu(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  interface_finalize();
}

void
test_interface_attr_private_ip_addr(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  lagopus_ip_address_t *actual_ip_addr = NULL;
  const char *expected_ip_addr_str = "127.0.0.1";
  lagopus_ip_address_t *expected_ip_addr = NULL;

  const char *set_ip_addr_str = "192.168.0.1";
  lagopus_ip_address_t *set_ip_addr = NULL;
  lagopus_ip_address_t *actual_set_ip_addr = NULL;
  lagopus_ip_address_t *expected_set_ip_addr = NULL;

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new channel");

  // Normal case of getter
  {
    rc = interface_get_ip_addr(attr, &actual_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(expected_ip_addr_str, true,
                                   &expected_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_ip_addr,
                                               actual_ip_addr));
  }

  // Abnormal case of getter
  {
    rc = interface_get_ip_addr(NULL, &actual_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = lagopus_ip_address_create(set_ip_addr_str, true, &set_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_set_ip_addr(attr, set_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_ip_addr(attr, &actual_set_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(set_ip_addr_str, true, &expected_set_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_set_ip_addr,
                                               actual_set_ip_addr));
  }

  // Abnormal case of setter
  {
    rc = interface_set_ip_addr(NULL, actual_set_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_ip_addr(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  lagopus_ip_address_destroy(actual_ip_addr);
  lagopus_ip_address_destroy(expected_ip_addr);
  lagopus_ip_address_destroy(set_ip_addr);
  lagopus_ip_address_destroy(actual_set_ip_addr);
  lagopus_ip_address_destroy(expected_set_ip_addr);
  interface_attr_destroy(attr);
  interface_finalize();
}

void
test_interface_attr_public_ip_addr(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  lagopus_ip_address_t *actual_ip_addr = NULL;
  const char *expected_ip_addr_str = "127.0.0.1";
  lagopus_ip_address_t *expected_ip_addr = NULL;

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_ip_addr(name, true, &actual_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_ip_addr(name, false, &actual_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(expected_ip_addr_str, true,
                                   &expected_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_ip_addr,
                                               actual_ip_addr));
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_ip_addr(NULL, true, &actual_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_ip_addr(NULL, false, &actual_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_ip_addr(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_ip_addr(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  lagopus_ip_address_destroy(actual_ip_addr);
  lagopus_ip_address_destroy(expected_ip_addr);
  interface_finalize();
}

void
test_interface_attr_private_ip_addr_str(void) {
  lagopus_result_t rc;
  interface_attr_t *attr = NULL;
  char *actual_ip_addr = NULL;
  const char *expected_ip_addr = "127.0.0.1";
  const char *set_ip_addr = "192.168.0.1";
  char *actual_set_ip_addr = NULL;
  const char *expected_set_ip_addr = set_ip_addr;
  const char *invalid_ip_addr = "256.256.256.256";

  interface_initialize();

  rc = interface_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

  // Normal case of getter
  {
    rc = interface_get_ip_addr_str(attr, &actual_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_ip_addr, actual_ip_addr);
  }

  // Abnormal case of getter
  {
    rc = interface_get_ip_addr_str(NULL, &actual_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_get_ip_addr_str(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = interface_set_ip_addr_str(attr, set_ip_addr, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = interface_get_ip_addr_str(attr, &actual_set_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_ip_addr, actual_set_ip_addr);
  }

  // Abnormal case of setter
  {
    rc = interface_set_ip_addr_str(NULL, actual_set_ip_addr, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_ip_addr_str(attr, NULL, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = interface_set_ip_addr_str(attr, invalid_ip_addr, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ADDR_RESOLVER_FAILURE, rc);
  }

  interface_attr_destroy(attr);
  free((void *)actual_ip_addr);
  free((void *)actual_set_ip_addr);

  interface_finalize();
}

void
test_interface_attr_public_ip_addr_str(void) {
  lagopus_result_t rc;
  interface_conf_t *conf = NULL;
  const char *name = "interface_name";
  char *actual_ip_addr = NULL;
  const char *expected_ip_addr = "127.0.0.1";

  interface_initialize();

  rc = interface_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new interface");

  rc = interface_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_interface_get_ip_addr_str(name, true, &actual_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_interface_get_ip_addr_str(name, false, &actual_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_ip_addr, actual_ip_addr);
  }

  // Abnormal case of getter
  {
    rc = datastore_interface_get_ip_addr_str(NULL, true, &actual_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_ip_addr_str(NULL, false, &actual_ip_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_ip_addr_str(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_interface_get_ip_addr_str(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  free((void *)actual_ip_addr);
  interface_finalize();
}
