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
#include "../bridge.c"
#include "../ns_util.h"
#define CAST_UINT64(x) (uint64_t) x

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_bridge_table_initialize_and_finalize(void) {
  bridge_initialize();
  bridge_finalize();
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
test_bridge_conf_create_and_destroy(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";

  bridge_initialize();

  // Normal case
  {
    rc = bridge_conf_create(&conf, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

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
    rc = bridge_conf_create(NULL, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_conf_create(&conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_conf_destroy(conf);

  bridge_finalize();
}

void
test_bridge_conf_duplicate(void) {
  lagopus_result_t rc;
  const char *ns1 = "ns1";
  const char *ns2 = "ns2";
  const char *name = "bridge";
  char *bridge_fullname = NULL;
  bool result = false;
  bridge_conf_t *src_conf = NULL;
  bridge_conf_t *dst_conf = NULL;
  bridge_conf_t *actual_conf = NULL;

  bridge_initialize();

  // Normal case1(no namespace)
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &bridge_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = bridge_conf_create(&src_conf, bridge_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new bridge_action");
      free(bridge_fullname);
      bridge_fullname = NULL;
    }

    rc = bridge_conf_duplicate(src_conf, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns1, name, &bridge_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = bridge_conf_create(&actual_conf, bridge_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new bridge_action");
      free(bridge_fullname);
      bridge_fullname = NULL;
    }

    result = bridge_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    bridge_conf_destroy(src_conf);
    src_conf = NULL;
    bridge_conf_destroy(dst_conf);
    dst_conf = NULL;
    bridge_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Normal case2
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &bridge_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = bridge_conf_create(&src_conf, bridge_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new bridge_action");
      free(bridge_fullname);
      bridge_fullname = NULL;
    }

    rc = bridge_conf_duplicate(src_conf, &dst_conf, ns2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns2, name, &bridge_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = bridge_conf_create(&actual_conf, bridge_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new bridge_action");
      free(bridge_fullname);
      bridge_fullname = NULL;
    }

    result = bridge_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    bridge_conf_destroy(src_conf);
    src_conf = NULL;
    bridge_conf_destroy(dst_conf);
    dst_conf = NULL;
    bridge_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Abnormal case
  {
    rc = bridge_conf_duplicate(NULL, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_conf_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  bridge_conf_t *conf1 = NULL;
  bridge_conf_t *conf2 = NULL;
  bridge_conf_t *conf3 = NULL;
  const char *fullname1 = "conf1";
  const char *fullname2 = "conf2";
  const char *fullname3 = "conf3";

  bridge_initialize();

  rc = bridge_conf_create(&conf1, fullname1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new bridge");
  rc = bridge_conf_add(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = bridge_conf_create(&conf2, fullname2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new bridge");
  rc = bridge_conf_add(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = bridge_conf_create(&conf3, fullname3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new bridge");
  rc = bridge_conf_add(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = bridge_set_enabled(fullname3, true);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = bridge_conf_equals(conf1, conf2);
    TEST_ASSERT_TRUE(result);

    result = bridge_conf_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = bridge_conf_equals(conf1, conf3);
    TEST_ASSERT_FALSE(result);

    result = bridge_conf_equals(conf2, conf3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = bridge_conf_equals(conf1, NULL);
    TEST_ASSERT_FALSE(result);

    result = bridge_conf_equals(NULL, conf2);
    TEST_ASSERT_FALSE(result);
  }

  rc = bridge_conf_delete(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = bridge_conf_delete(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = bridge_conf_delete(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  bridge_finalize();
}

void
test_bridge_conf_add(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "interface_name";
  bridge_conf_t *actual_conf = NULL;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  // Normal case
  {
    rc = bridge_conf_add(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = bridge_conf_add(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = bridge_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  bridge_finalize();
}

void
test_bridge_conf_add_not_initialize(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  bridge_conf_destroy(conf);

  bridge_finalize();
}

void
test_bridge_conf_delete(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  bridge_conf_t *actual_conf = NULL;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    rc = bridge_conf_delete(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = bridge_conf_delete(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = bridge_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, rc);

  bridge_finalize();
}

void
test_bridge_conf_delete_not_initialize(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_delete(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  bridge_conf_destroy(conf);

  bridge_finalize();
}

void
test_bridge_conf_iterate(void) {
  TEST_IGNORE();
}

void
test_bridge_conf_list(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf1 = NULL;
  const char *name1 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"bridge_name1";
  bridge_conf_t *conf2 = NULL;
  const char *name2 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"bridge_name2";
  bridge_conf_t *conf3 = NULL;
  const char *name3 = "namespace2"DATASTORE_NAMESPACE_DELIMITER"bridge_name3";
  bridge_conf_t *conf4 = NULL;
  const char *name4 = DATASTORE_NAMESPACE_DELIMITER"bridge_name4";
  bridge_conf_t *conf5 = NULL;
  const char *name5 = DATASTORE_NAMESPACE_DELIMITER"bridge_name5";
  bridge_conf_t **actual_list = NULL;
  size_t i;

  bridge_initialize();

  // create conf
  {
    rc = bridge_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new bridge");

    rc = bridge_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new bridge");

    rc = bridge_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new bridge");

    rc = bridge_conf_create(&conf4, name4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf4, "conf_create() will create new bridge");

    rc = bridge_conf_create(&conf5, name5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf5, "conf_create() will create new bridge");
  }

  // add conf
  {
    rc = bridge_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = bridge_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = bridge_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = bridge_conf_add(conf4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = bridge_conf_add(conf5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // all
  {
    rc = bridge_conf_list(&actual_list, NULL);
    TEST_ASSERT_EQUAL(5, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"bridge_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"bridge_name2") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace2"DATASTORE_NAMESPACE_DELIMITER"bridge_name3") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"bridge_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"bridge_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // no namespace
  {
    rc = bridge_conf_list(&actual_list, "");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"bridge_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"bridge_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // only namespace
  {
    rc = bridge_conf_list(&actual_list, "namespace1");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"bridge_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"bridge_name2") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // Abnormal case
  {
    rc = bridge_conf_list(NULL, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_conf_list_not_initialize(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  bridge_conf_t ***list = NULL;

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_list(list, NULL);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  bridge_conf_destroy(conf);

  bridge_finalize();
}

void
test_bridge_find(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf1 = NULL;
  const char *name1 = "bridge_name1";
  bridge_conf_t *conf2 = NULL;
  const char *name2 = "bridge_name2";
  bridge_conf_t *conf3 = NULL;
  const char *name3 = "bridge_name3";
  bridge_conf_t *actual_conf = NULL;

  bridge_initialize();

  // create conf
  {
    rc = bridge_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new bridge");

    rc = bridge_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new bridge");

    rc = bridge_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new bridge");
  }

  // add conf
  {
    rc = bridge_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = bridge_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = bridge_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Normal case
  {
    rc = bridge_find(name1, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name1, actual_conf->name);

    rc = bridge_find(name2, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name2, actual_conf->name);

    rc = bridge_find(name3, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name3, actual_conf->name);
  }

  // Abnormal case
  {
    rc = bridge_find(NULL, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_find(name1, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_find_not_initialize(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  bridge_conf_t *actual_conf = NULL;

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  bridge_conf_destroy(conf);

  bridge_finalize();
}

void
test_bridge_attr_controller_name_exists(void) {
  lagopus_result_t rc;
  bool ret = false;
  bridge_attr_t *attr = NULL;
  const char *name1 = "bridge3";
  const char *name2 = "invalid_bridge";

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  rc = bridge_attr_add_controller_name(attr, "bridge1");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = bridge_attr_add_controller_name(attr, "bridge2");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = bridge_attr_add_controller_name(attr, "bridge3");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of setter
  {
    ret = bridge_attr_controller_name_exists(attr, name1);
    TEST_ASSERT_TRUE(ret);

    ret = bridge_attr_controller_name_exists(attr, name2);
    TEST_ASSERT_FALSE(ret);
  }

  // Abnormal case of setter
  {
    ret = bridge_attr_controller_name_exists(NULL, name1);
    TEST_ASSERT_FALSE(ret);

    ret = bridge_attr_controller_name_exists(attr, NULL);
    TEST_ASSERT_FALSE(ret);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_port_name_exists(void) {
  lagopus_result_t rc;
  bool ret = false;
  bridge_attr_t *attr = NULL;
  const char *name1 = "bridge3";
  const char *name2 = "invalid_bridge";

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  rc = bridge_attr_add_port_name(attr, "bridge1");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = bridge_attr_add_port_name(attr, "bridge2");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = bridge_attr_add_port_name(attr, "bridge3");
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of setter
  {
    ret = bridge_attr_port_name_exists(attr, name1);
    TEST_ASSERT_TRUE(ret);

    ret = bridge_attr_port_name_exists(attr, name2);
    TEST_ASSERT_FALSE(ret);
  }

  // Abnormal case of setter
  {
    ret = bridge_attr_port_name_exists(NULL, name1);
    TEST_ASSERT_FALSE(ret);

    ret = bridge_attr_port_name_exists(attr, NULL);
    TEST_ASSERT_FALSE(ret);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_create_and_destroy(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint64_t actual_dpid = 0;
  const uint64_t expected_dpid = 0;
  datastore_name_info_t *actual_controller_names = NULL;
  struct datastore_name_entry *expected_controller_entry = NULL;
  datastore_name_info_t *actual_port_names = NULL;
  struct datastore_name_entry *expected_port_entry = NULL;
  datastore_bridge_fail_mode_t actual_fail_mode = 0;
  const datastore_bridge_fail_mode_t expected_fail_mode =
    DATASTORE_BRIDGE_FAIL_MODE_UNKNOWN;
  bool actual_flow_statistics = false;
  bool actual_group_statistics = false;
  bool actual_port_statistics = false;
  bool actual_queue_statistics = false;
  bool actual_table_statistics = false;
  bool actual_reassemble_ip_fragments = true;
  uint32_t actual_max_buffered_packets = 0;
  const uint32_t expected_max_buffered_packets = 65535;
  uint16_t actual_max_ports = 0;
  const uint16_t expected_max_ports = 255;
  uint8_t actual_max_tables = 0;
  const uint8_t expected_max_tables = 255;
  bool actual_block_looping_ports = true;
  uint64_t actual_action_types = 0;
  const uint64_t expected_action_types = UINT64_MAX;
  uint64_t actual_instruction_types = 0;
  const uint64_t expected_instruction_types = UINT64_MAX;
  uint32_t actual_max_flows = 0;
  const uint32_t expected_max_flows = MAXIMUM_FLOWS;
  uint64_t actual_reserved_port_types = 0;
  const uint64_t expected_reserved_port_types = UINT64_MAX;
  uint64_t actual_group_types = 0;
  const uint64_t expected_group_types = UINT64_MAX;
  uint64_t actual_group_capabilities = 0;
  const uint64_t expected_group_capabilities = UINT64_MAX;
  uint16_t actual_packet_inq_size = 0;
  const uint16_t expected_packet_inq_size = 1000;
  uint16_t actual_packet_inq_max_batches = 0;
  const uint16_t expected_packet_inq_max_batches = 1000;
  uint16_t actual_up_streamq_size = 0;
  const uint16_t expected_up_streamq_size = 1000;
  uint16_t actual_up_streamq_max_batches = 0;
  const uint16_t expected_up_streamq_max_batches = 1000;
  uint16_t actual_down_streamq_size = 0;
  const uint16_t expected_down_streamq_size = 1000;
  uint16_t actual_down_streamq_max_batches = 0;
  const uint16_t expected_down_streamq_max_batches = 1000;

  bridge_initialize();

  // Normal case
  {
    rc = bridge_attr_create(&attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

    // dpid
    rc = bridge_get_dpid(attr, &actual_dpid);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_dpid, actual_dpid);

    // controller_names
    rc = bridge_get_controller_names(attr, &actual_controller_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(0, actual_controller_names->size);
    expected_controller_entry = TAILQ_FIRST(&(actual_controller_names->head));
    TEST_ASSERT_NULL(expected_controller_entry);

    // port_names
    rc = bridge_get_port_names(attr, &actual_port_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(0, actual_port_names->size);
    expected_port_entry = TAILQ_FIRST(&(actual_port_names->head));
    TEST_ASSERT_NULL(expected_port_entry);

    // fail_mode
    rc = bridge_get_fail_mode(attr, &actual_fail_mode);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_fail_mode, actual_fail_mode);

    // flow_statistics
    rc = bridge_is_flow_statistics(attr, &actual_flow_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_flow_statistics);

    // group_statistics
    rc = bridge_is_group_statistics(attr, &actual_group_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_group_statistics);

    // port_statistics
    rc = bridge_is_port_statistics(attr, &actual_port_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_port_statistics);

    // queue_statistics
    rc = bridge_is_queue_statistics(attr, &actual_queue_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_queue_statistics);

    // table_statistics
    rc = bridge_is_table_statistics(attr, &actual_table_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_table_statistics);

    // reassemble_ip_fragments
    rc = bridge_is_reassemble_ip_fragments(attr, &actual_reassemble_ip_fragments);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_reassemble_ip_fragments);

    // max_buffered_packets
    rc = bridge_get_max_buffered_packets(attr, &actual_max_buffered_packets);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_buffered_packets,
                             actual_max_buffered_packets);

    // max_ports
    rc = bridge_get_max_ports(attr, &actual_max_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_ports, actual_max_ports);

    // max_tables
    rc = bridge_get_max_tables(attr, &actual_max_tables);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_tables, actual_max_tables);

    // block_looping_ports
    rc = bridge_is_block_looping_ports(attr, &actual_block_looping_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_block_looping_ports);

    // action_types
    rc = bridge_get_action_types(attr, &actual_action_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_action_types, actual_action_types);

    // instruction_types
    rc = bridge_get_instruction_types(attr, &actual_instruction_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_instruction_types, actual_instruction_types);

    // max_flows
    rc = bridge_get_max_flows(attr, &actual_max_flows);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_flows, actual_max_flows);

    // reserved_port_types
    rc = bridge_get_reserved_port_types(attr, &actual_reserved_port_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_reserved_port_types,
                             actual_reserved_port_types);

    // group_types
    rc = bridge_get_group_types(attr, &actual_group_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_group_types, actual_group_types);

    // group_capabilities
    rc = bridge_get_group_capabilities(attr, &actual_group_capabilities);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_group_capabilities,
                             actual_group_capabilities);

    // packet_inq_size
    rc = bridge_get_packet_inq_size(attr,
                                    &actual_packet_inq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_packet_inq_size,
                             actual_packet_inq_size);

    // packet_inq_max_batches
    rc = bridge_get_packet_inq_max_batches(attr,
                                           &actual_packet_inq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_packet_inq_max_batches,
                             actual_packet_inq_max_batches);

    // up_streamq_size
    rc = bridge_get_up_streamq_size(attr,
                                    &actual_up_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_up_streamq_size,
                             actual_up_streamq_size);

    // up_streamq_max_batches
    rc = bridge_get_up_streamq_max_batches(attr,
                                           &actual_up_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_up_streamq_max_batches,
                             actual_up_streamq_max_batches);

    // down_streamq_size
    rc = bridge_get_down_streamq_size(attr,
                                      &actual_down_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_down_streamq_size,
                             actual_down_streamq_size);

    // down_streamq_max_batches
    rc = bridge_get_down_streamq_max_batches(attr,
         &actual_down_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_down_streamq_max_batches,
                             actual_down_streamq_max_batches);

  }

  bridge_attr_destroy(attr);
  datastore_names_destroy(actual_controller_names);
  datastore_names_destroy(actual_port_names);

  bridge_finalize();
}

void
test_bridge_attr_create_and_destroy_hybrid(void) {
#ifdef HYBRID
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint64_t actual_dpid = 0;
  const uint64_t expected_dpid = 0;
  datastore_name_info_t *actual_controller_names = NULL;
  struct datastore_name_entry *expected_controller_entry = NULL;
  datastore_name_info_t *actual_port_names = NULL;
  struct datastore_name_entry *expected_port_entry = NULL;
  datastore_bridge_fail_mode_t actual_fail_mode = 0;
  const datastore_bridge_fail_mode_t expected_fail_mode =
    DATASTORE_BRIDGE_FAIL_MODE_UNKNOWN;
  bool actual_flow_statistics = false;
  bool actual_group_statistics = false;
  bool actual_port_statistics = false;
  bool actual_queue_statistics = false;
  bool actual_table_statistics = false;
  bool actual_reassemble_ip_fragments = true;
  uint32_t actual_max_buffered_packets = 0;
  const uint32_t expected_max_buffered_packets = 65535;
  uint16_t actual_max_ports = 0;
  const uint16_t expected_max_ports = 255;
  uint8_t actual_max_tables = 0;
  const uint8_t expected_max_tables = 255;
  bool actual_block_looping_ports = true;
  uint64_t actual_action_types = 0;
  const uint64_t expected_action_types = UINT64_MAX;
  uint64_t actual_instruction_types = 0;
  const uint64_t expected_instruction_types = UINT64_MAX;
  uint32_t actual_max_flows = 0;
  const uint32_t expected_max_flows = MAXIMUM_FLOWS;
  uint64_t actual_reserved_port_types = 0;
  const uint64_t expected_reserved_port_types = UINT64_MAX;
  uint64_t actual_group_types = 0;
  const uint64_t expected_group_types = UINT64_MAX;
  uint64_t actual_group_capabilities = 0;
  const uint64_t expected_group_capabilities = UINT64_MAX;
  uint16_t actual_packet_inq_size = 0;
  const uint16_t expected_packet_inq_size = 1000;
  uint16_t actual_packet_inq_max_batches = 0;
  const uint16_t expected_packet_inq_max_batches = 1000;
  uint16_t actual_up_streamq_size = 0;
  const uint16_t expected_up_streamq_size = 1000;
  uint16_t actual_up_streamq_max_batches = 0;
  const uint16_t expected_up_streamq_max_batches = 1000;
  uint16_t actual_down_streamq_size = 0;
  const uint16_t expected_down_streamq_size = 1000;
  uint16_t actual_down_streamq_max_batches = 0;
  const uint16_t expected_down_streamq_max_batches = 1000;
  uint32_t actual_mactable_max_entries = 0;
  const uint32_t expected_mactable_max_entries = 8192;
  uint32_t actual_mactable_ageing_time = 0;
  const uint32_t expected_mactable_ageing_time = 300;

  bridge_initialize();

  // Normal case
  {
    rc = bridge_attr_create(&attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new interface");

    // dpid
    rc = bridge_get_dpid(attr, &actual_dpid);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_dpid, actual_dpid);

    // controller_names
    rc = bridge_get_controller_names(attr, &actual_controller_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(0, actual_controller_names->size);
    expected_controller_entry = TAILQ_FIRST(&(actual_controller_names->head));
    TEST_ASSERT_NULL(expected_controller_entry);

    // port_names
    rc = bridge_get_port_names(attr, &actual_port_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(0, actual_port_names->size);
    expected_port_entry = TAILQ_FIRST(&(actual_port_names->head));
    TEST_ASSERT_NULL(expected_port_entry);

    // fail_mode
    rc = bridge_get_fail_mode(attr, &actual_fail_mode);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_fail_mode, actual_fail_mode);

    // flow_statistics
    rc = bridge_is_flow_statistics(attr, &actual_flow_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_flow_statistics);

    // group_statistics
    rc = bridge_is_group_statistics(attr, &actual_group_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_group_statistics);

    // port_statistics
    rc = bridge_is_port_statistics(attr, &actual_port_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_port_statistics);

    // queue_statistics
    rc = bridge_is_queue_statistics(attr, &actual_queue_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_queue_statistics);

    // table_statistics
    rc = bridge_is_table_statistics(attr, &actual_table_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_table_statistics);

    // reassemble_ip_fragments
    rc = bridge_is_reassemble_ip_fragments(attr, &actual_reassemble_ip_fragments);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_reassemble_ip_fragments);

    // max_buffered_packets
    rc = bridge_get_max_buffered_packets(attr, &actual_max_buffered_packets);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_buffered_packets,
                             actual_max_buffered_packets);

    // max_ports
    rc = bridge_get_max_ports(attr, &actual_max_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_ports, actual_max_ports);

    // max_tables
    rc = bridge_get_max_tables(attr, &actual_max_tables);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_tables, actual_max_tables);

    // block_looping_ports
    rc = bridge_is_block_looping_ports(attr, &actual_block_looping_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_block_looping_ports);

    // action_types
    rc = bridge_get_action_types(attr, &actual_action_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_action_types, actual_action_types);

    // instruction_types
    rc = bridge_get_instruction_types(attr, &actual_instruction_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_instruction_types, actual_instruction_types);

    // max_flows
    rc = bridge_get_max_flows(attr, &actual_max_flows);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_flows, actual_max_flows);

    // reserved_port_types
    rc = bridge_get_reserved_port_types(attr, &actual_reserved_port_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_reserved_port_types,
                             actual_reserved_port_types);

    // group_types
    rc = bridge_get_group_types(attr, &actual_group_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_group_types, actual_group_types);

    // group_capabilities
    rc = bridge_get_group_capabilities(attr, &actual_group_capabilities);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_group_capabilities,
                             actual_group_capabilities);

    // packet_inq_size
    rc = bridge_get_packet_inq_size(attr,
                                    &actual_packet_inq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_packet_inq_size,
                             actual_packet_inq_size);

    // packet_inq_max_batches
    rc = bridge_get_packet_inq_max_batches(attr,
                                           &actual_packet_inq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_packet_inq_max_batches,
                             actual_packet_inq_max_batches);

    // up_streamq_size
    rc = bridge_get_up_streamq_size(attr,
                                    &actual_up_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_up_streamq_size,
                             actual_up_streamq_size);

    // up_streamq_max_batches
    rc = bridge_get_up_streamq_max_batches(attr,
                                           &actual_up_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_up_streamq_max_batches,
                             actual_up_streamq_max_batches);

    // down_streamq_size
    rc = bridge_get_down_streamq_size(attr,
                                      &actual_down_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_down_streamq_size,
                             actual_down_streamq_size);

    // down_streamq_max_batches
    rc = bridge_get_down_streamq_max_batches(attr,
         &actual_down_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_down_streamq_max_batches,
                             actual_down_streamq_max_batches);

    // mactable_entries
    TEST_ASSERT_NOT_NULL(attr->mactable_entries);

    // l2_bridge
    TEST_ASSERT_FALSE(attr->l2_bridge);

    // mactable_max_entries
    rc = bridge_get_mactable_max_entries(attr,
         &actual_mactable_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(expected_mactable_max_entries,
                      actual_mactable_max_entries);

    // mactable_ageing_time
    rc = bridge_get_mactable_ageing_time(attr,
         &actual_mactable_ageing_time);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(expected_mactable_ageing_time,
                      actual_mactable_ageing_time);
  }

  bridge_attr_destroy(attr);
  datastore_names_destroy(actual_controller_names);
  datastore_names_destroy(actual_port_names);

  bridge_finalize();
#else // HYBRID
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif // HYBRID
}

void
test_datastore_names_duplicate(void) {
  lagopus_result_t rc;
  const char *ns1 = "ns1";
  const char *ns2 = "ns2";
  const char *name1 = "name1";
  const char *name2 = "name2";
  const char *name3 = "name3";
  char *fullname1 = NULL;
  char *fullname2 = NULL;
  char *fullname3 = NULL;

  datastore_name_info_t *names = NULL;
  datastore_name_info_t *dup_names = NULL;
  datastore_name_info_t *actual_names = NULL;

  // Normal case1
  {
    rc = ns_create_fullname(ns1, name1, &fullname1);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              rc, "cmd_ns_get_fullname error.");

    rc = ns_create_fullname(ns1, name2, &fullname2);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              rc, "cmd_ns_get_fullname error.");

    rc = ns_create_fullname(ns1, name3, &fullname3);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              rc, "cmd_ns_get_fullname error.");

    rc = datastore_names_create(&names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_add_names(names, fullname1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_add_names(names, fullname2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_add_names(names, fullname3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = datastore_names_duplicate(names, &dup_names, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(3, dup_names->size);

    free(fullname1);
    free(fullname2);
    free(fullname3);

    rc = ns_create_fullname(ns1, name1, &fullname1);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              rc, "cmd_ns_get_fullname error.");

    rc = ns_create_fullname(ns1, name2, &fullname2);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              rc, "cmd_ns_get_fullname error.");

    rc = ns_create_fullname(ns1, name3, &fullname3);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              rc, "cmd_ns_get_fullname error.");

    rc = datastore_names_create(&actual_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_add_names(actual_names, fullname1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_add_names(actual_names, fullname2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_add_names(actual_names, fullname3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    TEST_ASSERT_TRUE(datastore_names_equals(dup_names, actual_names));

    free(fullname1);
    free(fullname2);
    free(fullname3);

    datastore_names_destroy(names);
    datastore_names_destroy(dup_names);
    datastore_names_destroy(actual_names);

    names = NULL;
    dup_names = NULL;
    actual_names = NULL;
  }

  // Normal case2
  {
    rc = ns_create_fullname(ns1, name1, &fullname1);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              rc, "cmd_ns_get_fullname error.");

    rc = ns_create_fullname(ns1, name2, &fullname2);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              rc, "cmd_ns_get_fullname error.");

    rc = ns_create_fullname(ns1, name3, &fullname3);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              rc, "cmd_ns_get_fullname error.");

    rc = datastore_names_create(&names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_add_names(names, fullname1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_add_names(names, fullname2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_add_names(names, fullname3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = datastore_names_duplicate(names, &dup_names, ns2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(3, dup_names->size);

    free(fullname1);
    free(fullname2);
    free(fullname3);

    rc = ns_create_fullname(ns2, name1, &fullname1);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              rc, "cmd_ns_get_fullname error.");

    rc = ns_create_fullname(ns2, name2, &fullname2);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              rc, "cmd_ns_get_fullname error.");

    rc = ns_create_fullname(ns2, name3, &fullname3);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                              rc, "cmd_ns_get_fullname error.");

    rc = datastore_names_create(&actual_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_add_names(actual_names, fullname1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_add_names(actual_names, fullname2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_add_names(actual_names, fullname3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    TEST_ASSERT_TRUE(datastore_names_equals(dup_names, actual_names));

    free(fullname1);
    free(fullname2);
    free(fullname3);

    datastore_names_destroy(names);
    datastore_names_destroy(dup_names);
    datastore_names_destroy(actual_names);

    names = NULL;
    dup_names = NULL;
    actual_names = NULL;
  }

  // Abnormal case
  {
    rc = datastore_names_duplicate(NULL, &dup_names, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_names_duplicate(names, NULL, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    datastore_names_destroy(names);
    datastore_names_destroy(dup_names);
    datastore_names_destroy(actual_names);

    names = NULL;
    dup_names = NULL;
    actual_names = NULL;
  }
}

void
test_interface_attr_duplicate(void) {
  lagopus_result_t rc;
  const char *ns1 = "ns1";
  const char *ns2 = "ns2";
  const char *port_name = "port";
  const char *controller_name = "controller";
  char *port_fullname = NULL;
  char *controller_fullname = NULL;
  bool result = false;
  bridge_attr_t *src_attr = NULL;
  bridge_attr_t *dst_attr = NULL;
  bridge_attr_t *actual_attr = NULL;

  bridge_initialize();

  // Normal case1(no namespace)
  {
    // create src attribute
    {
      rc = bridge_attr_create(&src_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_attr,
                                   "attr_create() will create new policer");
      rc = ns_create_fullname(ns1, port_name, &port_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = bridge_attr_add_port_name(src_attr, port_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(port_fullname);
      port_fullname = NULL;
      rc = ns_create_fullname(ns1, controller_name, &controller_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = bridge_attr_add_controller_name(src_attr, controller_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(controller_fullname);
      controller_fullname = NULL;
    }

    // duplicate
    rc = bridge_attr_duplicate(src_attr, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual attribute
    {
      rc = bridge_attr_create(&actual_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(actual_attr,
                                   "attr_create() will create new policer");
      rc = ns_create_fullname(ns1, port_name, &port_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = bridge_attr_add_port_name(actual_attr, port_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(port_fullname);
      port_fullname = NULL;
      rc = ns_create_fullname(ns1, controller_name, &controller_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = bridge_attr_add_controller_name(actual_attr, controller_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(controller_fullname);
      controller_fullname = NULL;
    }

    result = bridge_attr_equals(dst_attr, actual_attr);
    TEST_ASSERT_TRUE(result);

    bridge_attr_destroy(src_attr);
    src_attr = NULL;
    bridge_attr_destroy(dst_attr);
    dst_attr = NULL;
    bridge_attr_destroy(actual_attr);
    actual_attr = NULL;
  }

  // Normal case2
  {
    // create src attribute
    {
      rc = bridge_attr_create(&src_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_attr,
                                   "attr_create() will create new policer");
      rc = ns_create_fullname(ns1, port_name, &port_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = bridge_attr_add_port_name(src_attr, port_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(port_fullname);
      port_fullname = NULL;
      rc = ns_create_fullname(ns1, controller_name, &controller_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = bridge_attr_add_controller_name(src_attr, controller_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(controller_fullname);
      controller_fullname = NULL;
    }

    // duplicate
    rc = bridge_attr_duplicate(src_attr, &dst_attr, ns2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual attribute
    {
      rc = bridge_attr_create(&actual_attr);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(actual_attr,
                                   "attr_create() will create new policer");
      rc = ns_create_fullname(ns2, port_name, &port_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = bridge_attr_add_port_name(actual_attr, port_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(port_fullname);
      port_fullname = NULL;
      rc = ns_create_fullname(ns2, controller_name, &controller_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = bridge_attr_add_controller_name(actual_attr, controller_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      free(controller_fullname);
      controller_fullname = NULL;
    }

    result = bridge_attr_equals(dst_attr, actual_attr);
    TEST_ASSERT_TRUE(result);

    bridge_attr_destroy(src_attr);
    src_attr = NULL;
    bridge_attr_destroy(dst_attr);
    dst_attr = NULL;
    bridge_attr_destroy(actual_attr);
    actual_attr = NULL;
  }

  // Abnormal case
  {
    rc = bridge_attr_duplicate(NULL, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  bridge_attr_t *attr1 = NULL;
  bridge_attr_t *attr2 = NULL;
  bridge_attr_t *attr3 = NULL;

  bridge_initialize();

  rc = bridge_attr_create(&attr1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr1, "attr_create() will create new bridge");

  rc = bridge_attr_create(&attr2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr2, "attr_create() will create new bridge");

  rc = bridge_attr_create(&attr3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr3, "attr_create() will create new bridge");
  rc = bridge_set_fail_mode(attr3, DATASTORE_BRIDGE_FAIL_MODE_STANDALONE);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = bridge_attr_equals(attr1, attr2);
    TEST_ASSERT_TRUE(result);

    result = bridge_attr_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = bridge_attr_equals(attr1, attr3);
    TEST_ASSERT_FALSE(result);

    result = bridge_attr_equals(attr2, attr3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = bridge_attr_equals(attr1, NULL);
    TEST_ASSERT_FALSE(result);

    result = bridge_attr_equals(NULL, attr2);
    TEST_ASSERT_FALSE(result);
  }

  bridge_attr_destroy(attr1);
  bridge_attr_destroy(attr2);
  bridge_attr_destroy(attr3);

  bridge_finalize();
}

void
test_bridge_mactable_entry_create_and_destroy(void) {
#ifdef HYBRID
  lagopus_result_t rc;
  bridge_mactable_info_t *entries = NULL;

  // Normal case
  {
    rc = bridge_mactable_entry_create(&entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(entries);
  }

  // Abnormal case
  {
    rc = bridge_mactable_entry_create(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case
  {
    rc = bridge_mactable_entry_destroy(entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = bridge_mactable_entry_destroy(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
#else // HYBRID
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif // HYBRID
}

void
test_bridge_mactable_add_entries(void) {
#ifdef HYBRID
  lagopus_result_t rc;
  bridge_mactable_info_t *entries = NULL;
  const uint32_t port_no = 1;
  mac_address_t addr;

  addr[0] = 0xaa;
  addr[1] = 0xbb;
  addr[2] = 0xcc;
  addr[3] = 0xdd;
  addr[4] = 0xee;
  addr[5] = 0xff;

  bridge_mactable_entry_create(&entries);

  // Normal case
  {
    rc = bridge_mactable_add_entries(entries, addr, port_no);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = bridge_mactable_add_entries(NULL, addr, port_no);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
    rc = bridge_mactable_add_entries(entries, NULL, port_no);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
    rc = bridge_mactable_add_entries(entries, addr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_mactable_entry_destroy(entries);
#else // HYBRID
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif // HYBRID
}

void
test_bridge_mactable_entry_exists(void) {
#ifdef HYBRID
  lagopus_result_t rc;
  bridge_mactable_info_t *entries1 = NULL;
  bridge_mactable_info_t *entries2 = NULL;
  struct bridge_mactable_entry *entry = NULL;
  const uint32_t port_no1 = 1;
  const uint32_t port_no2 = 2;
  mac_address_t addr;

  addr[0] = 0xaa;
  addr[1] = 0xbb;
  addr[2] = 0xcc;
  addr[3] = 0xdd;
  addr[4] = 0xee;
  addr[5] = 0xff;

  bridge_mactable_entry_create(&entries1);
  bridge_mactable_entry_create(&entries2);

  // Normal case 1
  {
    bridge_mactable_add_entries(entries1, addr, port_no1);
    entry = TAILQ_FIRST(&(entries1->head));
    TEST_ASSERT_TRUE(bridge_mactable_entry_exists(entries1, entry) == true);
  }

  // Normal case 2
  {
    bridge_mactable_add_entries(entries2, addr, port_no2);
    TEST_ASSERT_TRUE(bridge_mactable_entry_exists(entries2, entry) == false);
  }

  // Abnormal case
  {
    TEST_ASSERT_TRUE(bridge_mactable_entry_exists(NULL, entry) == false);
    TEST_ASSERT_TRUE(bridge_mactable_entry_exists(entries1, NULL) == false);
  }

  bridge_mactable_entry_destroy(entries1);
  bridge_mactable_entry_destroy(entries2);
#else // HYBRID
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif //HYBRID
}

void
test_bridge_mactable_entry_equals(void) {
#ifdef HYBRID
  bridge_mactable_info_t *entries1 = NULL;
  bridge_mactable_info_t *entries2 = NULL;
  bridge_mactable_info_t *entries3 = NULL;
  const uint32_t port_no1 = 1;
  const uint32_t port_no2 = 2;
  mac_address_t addr1;
  mac_address_t addr2;

  addr1[0] = 0xaa;
  addr1[1] = 0xbb;
  addr1[2] = 0xcc;
  addr1[3] = 0xdd;
  addr1[4] = 0xee;

  addr2[0] = 0x11;
  addr2[1] = 0x22;
  addr2[2] = 0x33;
  addr2[3] = 0x44;
  addr2[4] = 0x55;

  bridge_mactable_entry_create(&entries1);
  bridge_mactable_entry_create(&entries2);
  bridge_mactable_entry_create(&entries3);

  bridge_mactable_add_entries(entries1, addr1, port_no1);
  bridge_mactable_add_entries(entries2, addr1, port_no1);
  bridge_mactable_add_entries(entries3, addr2, port_no2);

  // Normal case
  {
    TEST_ASSERT_TRUE(bridge_mactable_entry_equals(entries1, entries2) == true);
    TEST_ASSERT_TRUE(bridge_mactable_entry_equals(entries1, entries3) == false);
  }

  // Abnormal case
  {
    TEST_ASSERT_TRUE(bridge_mactable_entry_equals(entries1, NULL) == false);
    TEST_ASSERT_TRUE(bridge_mactable_entry_equals(NULL, entries2) == false);
  }

  bridge_mactable_entry_destroy(entries1);
  bridge_mactable_entry_destroy(entries2);
  bridge_mactable_entry_destroy(entries3);
#else // HYBRID
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif // HYBRID
}

void
test_bridge_mactable_entry_duplicate(void) {
#ifdef HYBRID
  lagopus_result_t rc;
  bridge_mactable_info_t *src_entries = NULL;
  bridge_mactable_info_t *dst_entries = NULL;
  const uint32_t port_no = 1;
  mac_address_t addr;

  addr[0] = 0xaa;
  addr[1] = 0xbb;
  addr[2] = 0xcc;
  addr[3] = 0xdd;
  addr[4] = 0xee;

  bridge_mactable_entry_create(&src_entries);
  bridge_mactable_entry_create(&dst_entries);

  bridge_mactable_add_entries(src_entries, addr, port_no);

  // Normal case
  {
    rc = bridge_mactable_entry_duplicate(src_entries, &dst_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(bridge_mactable_entry_equals(src_entries,
                                                  dst_entries) == true);
  }

  // Abnormal case
  {
    rc = bridge_mactable_entry_duplicate(NULL, &dst_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
    rc = bridge_mactable_entry_duplicate(src_entries, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_mactable_entry_destroy(src_entries);
  bridge_mactable_entry_destroy(dst_entries);
#else // HYBRID
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif // HYBRID
}

void
test_bridge_mactable_remove_entry(void) {
#ifdef HYBRID
  lagopus_result_t rc;
  bridge_mactable_info_t *entries = NULL;
  struct bridge_mactable_entry *entry = NULL;
  const uint32_t port_no = 1;
  mac_address_t addr;

  addr[0] = 0xaa;
  addr[1] = 0xbb;
  addr[2] = 0xcc;
  addr[3] = 0xdd;
  addr[4] = 0xee;
  addr[5] = 0xff;

  bridge_mactable_entry_create(&entries);

  // Normal case
  {
    bridge_mactable_add_entries(entries, addr, port_no);
    entry = TAILQ_FIRST(&(entries->head));
    rc = bridge_mactable_remove_entry(entries, addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(bridge_mactable_entry_exists(entries, entry) == false);
  }

  // Abnormal case
  {
    bridge_mactable_add_entries(entries, addr, port_no);
    rc = bridge_mactable_remove_entry(NULL, addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
    rc = bridge_mactable_remove_entry(entries, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_mactable_entry_destroy(entries);
#else // HYBRID
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif // HYBRID
}

void
test_bridge_mactable_remove_all_entries(void) {
#ifdef HYBRID
  lagopus_result_t rc;
  bridge_mactable_info_t *entries1 = NULL;
  bridge_mactable_info_t *entries2 = NULL;
  const uint32_t port_no1 = 1;
  const uint32_t port_no2 = 2;
  mac_address_t addr1;
  mac_address_t addr2;

  addr1[0] = 0xaa;
  addr1[1] = 0xbb;
  addr1[2] = 0xcc;
  addr1[3] = 0xdd;
  addr1[4] = 0xee;
  addr1[5] = 0xff;

  addr2[0] = 0x11;
  addr2[1] = 0x22;
  addr2[2] = 0x33;
  addr2[3] = 0x44;
  addr2[4] = 0x55;
  addr2[5] = 0x66;

  bridge_mactable_entry_create(&entries1);
  bridge_mactable_entry_create(&entries2);

  bridge_mactable_add_entries(entries1, addr1, port_no1);
  bridge_mactable_add_entries(entries1, addr2, port_no2);
  bridge_mactable_add_entries(entries2, addr1, port_no1);
  bridge_mactable_add_entries(entries2, addr2, port_no2);
  TEST_ASSERT_TRUE(bridge_mactable_entry_equals(entries1, entries2) == true);

  // Normal case
  {
    rc = bridge_mactable_remove_all_entries(entries1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(bridge_mactable_entry_equals(entries1, entries2) == false);
  }

  bridge_mactable_entry_destroy(entries1);
  bridge_mactable_entry_destroy(entries2);
#else // HYBRID
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif // HYBRID
}

void
test_bridge_fail_mode_to_str(void) {
  lagopus_result_t rc;
  const char *actual_fail_mode_str;

  // Normal case
  {
    rc = bridge_fail_mode_to_str(DATASTORE_BRIDGE_FAIL_MODE_UNKNOWN,
                                 &actual_fail_mode_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_fail_mode_str);

    rc = bridge_fail_mode_to_str(DATASTORE_BRIDGE_FAIL_MODE_SECURE,
                                 &actual_fail_mode_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("secure", actual_fail_mode_str);

    rc = bridge_fail_mode_to_str(DATASTORE_BRIDGE_FAIL_MODE_STANDALONE,
                                 &actual_fail_mode_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("standalone", actual_fail_mode_str);

    rc = bridge_fail_mode_to_str(DATASTORE_BRIDGE_FAIL_MODE_MIN,
                                 &actual_fail_mode_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_fail_mode_str);

    rc = bridge_fail_mode_to_str(DATASTORE_BRIDGE_FAIL_MODE_MAX,
                                 &actual_fail_mode_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("standalone", actual_fail_mode_str);
  }

  // Abnormal case
  {
    rc = bridge_fail_mode_to_str(DATASTORE_BRIDGE_FAIL_MODE_SECURE, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_bridge_fail_mode_to_enum(void) {
  lagopus_result_t rc;
  datastore_bridge_fail_mode_t actual_fail_mode;

  // Normal case
  {
    rc = bridge_fail_mode_to_enum("unknown", &actual_fail_mode);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_FAIL_MODE_UNKNOWN, actual_fail_mode);

    rc = bridge_fail_mode_to_enum("secure", &actual_fail_mode);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_FAIL_MODE_SECURE, actual_fail_mode);

    rc = bridge_fail_mode_to_enum("standalone", &actual_fail_mode);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_FAIL_MODE_STANDALONE,
                             actual_fail_mode);

    rc = bridge_fail_mode_to_enum("unknown", &actual_fail_mode);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_FAIL_MODE_MIN, actual_fail_mode);

    rc = bridge_fail_mode_to_enum("standalone", &actual_fail_mode);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_FAIL_MODE_MAX, actual_fail_mode);

    rc = bridge_fail_mode_to_enum("Secure", &actual_fail_mode);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_fail_mode_to_enum("SECURE", &actual_fail_mode);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case
  {
    rc = bridge_fail_mode_to_enum("invalid_args", &actual_fail_mode);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_fail_mode_to_enum(NULL, &actual_fail_mode);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_fail_mode_to_enum("secure", NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_bridge_action_type_to_str(void) {
  lagopus_result_t rc;
  const char *actual_action_type_str;

  // Normal case
  {
    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_UNKNOWN,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_action_type_str);

    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_COPY_TTL_OUT,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("copy-ttl-out", actual_action_type_str);

    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_COPY_TTL_IN,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("copy-ttl-in", actual_action_type_str);

    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_SET_MPLS_TTL,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("set-mpls-ttl", actual_action_type_str);

    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_DEC_MPLS_TTL,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("dec-mpls-ttl", actual_action_type_str);

    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_PUSH_VLAN,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("push-vlan", actual_action_type_str);

    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_POP_VLAN,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("pop-vlan", actual_action_type_str);

    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_PUSH_MPLS,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("push-mpls", actual_action_type_str);

    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_POP_MPLS,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("pop-mpls", actual_action_type_str);

    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_SET_QUEUE,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("set-queue", actual_action_type_str);

    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_GROUP,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("group", actual_action_type_str);

    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_SET_NW_TTL,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("set-nw-ttl", actual_action_type_str);

    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_DEC_NW_TTL,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("dec-nw-ttl", actual_action_type_str);

    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_SET_FIELD,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("set-field", actual_action_type_str);

    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_MIN,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_action_type_str);

    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_MAX,
                                   &actual_action_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("set-field", actual_action_type_str);
  }

  // Abnormal case
  {
    rc = bridge_action_type_to_str(DATASTORE_BRIDGE_ACTION_TYPE_SET_FIELD, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_bridge_action_type_to_enum(void) {
  lagopus_result_t rc;
  datastore_bridge_action_type_t actual_action_type;

  // Normal case
  {
    rc = bridge_action_type_to_enum("unknown", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_UNKNOWN,
                             actual_action_type);

    rc = bridge_action_type_to_enum("copy-ttl-out", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_COPY_TTL_OUT,
                             actual_action_type);

    rc = bridge_action_type_to_enum("copy-ttl-in", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_COPY_TTL_IN,
                             actual_action_type);

    rc = bridge_action_type_to_enum("set-mpls-ttl", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_SET_MPLS_TTL,
                             actual_action_type);

    rc = bridge_action_type_to_enum("dec-mpls-ttl", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_DEC_MPLS_TTL,
                             actual_action_type);

    rc = bridge_action_type_to_enum("push-vlan", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_PUSH_VLAN,
                             actual_action_type);

    rc = bridge_action_type_to_enum("pop-vlan", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_POP_VLAN,
                             actual_action_type);

    rc = bridge_action_type_to_enum("push-mpls", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_PUSH_MPLS,
                             actual_action_type);

    rc = bridge_action_type_to_enum("pop-mpls", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_POP_MPLS,
                             actual_action_type);

    rc = bridge_action_type_to_enum("set-queue", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_SET_QUEUE,
                             actual_action_type);

    rc = bridge_action_type_to_enum("group", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_GROUP,
                             actual_action_type);

    rc = bridge_action_type_to_enum("set-nw-ttl", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_SET_NW_TTL,
                             actual_action_type);

    rc = bridge_action_type_to_enum("dec-nw-ttl", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_DEC_NW_TTL,
                             actual_action_type);

    rc = bridge_action_type_to_enum("set-field", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_SET_FIELD,
                             actual_action_type);

    rc = bridge_action_type_to_enum("unknown", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_MIN, actual_action_type);

    rc = bridge_action_type_to_enum("set-field", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_ACTION_TYPE_MAX, actual_action_type);

    rc = bridge_action_type_to_enum("Set-Field", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_action_type_to_enum("SET-FIELD", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case
  {
    rc = bridge_action_type_to_enum("invalid_args", &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_action_type_to_enum(NULL, &actual_action_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_action_type_to_enum("set-field", NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_bridge_instruction_type_to_str(void) {
  lagopus_result_t rc;
  const char *actual_instruction_type_str;

  // Normal case
  {
    rc = bridge_instruction_type_to_str(DATASTORE_BRIDGE_INSTRUCTION_TYPE_UNKNOWN,
                                        &actual_instruction_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_instruction_type_str);

    rc = bridge_instruction_type_to_str(
           DATASTORE_BRIDGE_INSTRUCTION_TYPE_APPLY_ACTIONS,
           &actual_instruction_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("apply-actions", actual_instruction_type_str);

    rc = bridge_instruction_type_to_str(
           DATASTORE_BRIDGE_INSTRUCTION_TYPE_CLEAR_ACTIONS,
           &actual_instruction_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("clear-actions", actual_instruction_type_str);

    rc = bridge_instruction_type_to_str(
           DATASTORE_BRIDGE_INSTRUCTION_TYPE_WRITE_ACTIONS,
           &actual_instruction_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("write-actions", actual_instruction_type_str);

    rc = bridge_instruction_type_to_str(
           DATASTORE_BRIDGE_INSTRUCTION_TYPE_WRITE_METADATA,
           &actual_instruction_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("write-metadata", actual_instruction_type_str);

    rc = bridge_instruction_type_to_str(
           DATASTORE_BRIDGE_INSTRUCTION_TYPE_GOTO_TABLE,
           &actual_instruction_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("goto-table", actual_instruction_type_str);

    rc = bridge_instruction_type_to_str(DATASTORE_BRIDGE_INSTRUCTION_TYPE_MIN,
                                        &actual_instruction_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_instruction_type_str);

    rc = bridge_instruction_type_to_str(DATASTORE_BRIDGE_INSTRUCTION_TYPE_MAX,
                                        &actual_instruction_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("goto-table", actual_instruction_type_str);
  }

  // Abnormal case
  {
    rc = bridge_instruction_type_to_str(
           DATASTORE_BRIDGE_INSTRUCTION_TYPE_GOTO_TABLE, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_bridge_instruction_type_to_enum(void) {
  lagopus_result_t rc;
  datastore_bridge_instruction_type_t actual_instruction_type;

  // Normal case
  {
    rc = bridge_instruction_type_to_enum("unknown", &actual_instruction_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_INSTRUCTION_TYPE_UNKNOWN,
                             actual_instruction_type);

    rc = bridge_instruction_type_to_enum("apply-actions",
                                         &actual_instruction_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_INSTRUCTION_TYPE_APPLY_ACTIONS,
                             actual_instruction_type);

    rc = bridge_instruction_type_to_enum("clear-actions",
                                         &actual_instruction_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_INSTRUCTION_TYPE_CLEAR_ACTIONS,
                             actual_instruction_type);

    rc = bridge_instruction_type_to_enum("write-actions",
                                         &actual_instruction_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_INSTRUCTION_TYPE_WRITE_ACTIONS,
                             actual_instruction_type);

    rc = bridge_instruction_type_to_enum("write-metadata",
                                         &actual_instruction_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_INSTRUCTION_TYPE_WRITE_METADATA,
                             actual_instruction_type);

    rc = bridge_instruction_type_to_enum("goto-table", &actual_instruction_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_INSTRUCTION_TYPE_GOTO_TABLE,
                             actual_instruction_type);

    rc = bridge_instruction_type_to_enum("unknown", &actual_instruction_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_INSTRUCTION_TYPE_MIN,
                             actual_instruction_type);

    rc = bridge_instruction_type_to_enum("goto-table", &actual_instruction_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_INSTRUCTION_TYPE_MAX,
                             actual_instruction_type);

    rc = bridge_instruction_type_to_enum("Set-Field", &actual_instruction_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_instruction_type_to_enum("SET-FIELD", &actual_instruction_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case
  {
    rc = bridge_instruction_type_to_enum("invalid_args", &actual_instruction_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_instruction_type_to_enum(NULL, &actual_instruction_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_instruction_type_to_enum("set-field", NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_bridge_reserved_port_type_to_str(void) {
  lagopus_result_t rc;
  const char *actual_reserved_port_type_str;

  // Normal case
  {
    rc = bridge_reserved_port_type_to_str(
           DATASTORE_BRIDGE_RESERVED_PORT_TYPE_UNKNOWN,
           &actual_reserved_port_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_reserved_port_type_str);

    rc = bridge_reserved_port_type_to_str(DATASTORE_BRIDGE_RESERVED_PORT_TYPE_ALL,
                                          &actual_reserved_port_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("all", actual_reserved_port_type_str);

    rc = bridge_reserved_port_type_to_str(
           DATASTORE_BRIDGE_RESERVED_PORT_TYPE_CONTROLLER,
           &actual_reserved_port_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("controller", actual_reserved_port_type_str);

    rc = bridge_reserved_port_type_to_str(
           DATASTORE_BRIDGE_RESERVED_PORT_TYPE_TABLE,
           &actual_reserved_port_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("table", actual_reserved_port_type_str);

    rc = bridge_reserved_port_type_to_str(
           DATASTORE_BRIDGE_RESERVED_PORT_TYPE_INPORT,
           &actual_reserved_port_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("inport", actual_reserved_port_type_str);

    rc = bridge_reserved_port_type_to_str(DATASTORE_BRIDGE_RESERVED_PORT_TYPE_ANY,
                                          &actual_reserved_port_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("any", actual_reserved_port_type_str);

    rc = bridge_reserved_port_type_to_str(
           DATASTORE_BRIDGE_RESERVED_PORT_TYPE_NORMAL,
           &actual_reserved_port_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("normal", actual_reserved_port_type_str);

    rc = bridge_reserved_port_type_to_str(
           DATASTORE_BRIDGE_RESERVED_PORT_TYPE_FLOOD,
           &actual_reserved_port_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("flood", actual_reserved_port_type_str);

    rc = bridge_reserved_port_type_to_str(DATASTORE_BRIDGE_RESERVED_PORT_TYPE_MIN,
                                          &actual_reserved_port_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_reserved_port_type_str);

    rc = bridge_reserved_port_type_to_str(DATASTORE_BRIDGE_RESERVED_PORT_TYPE_MAX,
                                          &actual_reserved_port_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("flood", actual_reserved_port_type_str);
  }

  // Abnormal case
  {
    rc = bridge_reserved_port_type_to_str(DATASTORE_BRIDGE_RESERVED_PORT_TYPE_ALL,
                                          NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_bridge_reserved_port_type_to_enum(void) {
  lagopus_result_t rc;
  datastore_bridge_reserved_port_type_t actual_reserved_port_type;

  // Normal case
  {
    rc = bridge_reserved_port_type_to_enum("unknown", &actual_reserved_port_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_RESERVED_PORT_TYPE_UNKNOWN,
                             actual_reserved_port_type);

    rc = bridge_reserved_port_type_to_enum("all", &actual_reserved_port_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_RESERVED_PORT_TYPE_ALL,
                             actual_reserved_port_type);

    rc = bridge_reserved_port_type_to_enum("controller",
                                           &actual_reserved_port_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_RESERVED_PORT_TYPE_CONTROLLER,
                             actual_reserved_port_type);

    rc = bridge_reserved_port_type_to_enum("table", &actual_reserved_port_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_RESERVED_PORT_TYPE_TABLE,
                             actual_reserved_port_type);

    rc = bridge_reserved_port_type_to_enum("inport", &actual_reserved_port_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_RESERVED_PORT_TYPE_INPORT,
                             actual_reserved_port_type);

    rc = bridge_reserved_port_type_to_enum("any", &actual_reserved_port_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_RESERVED_PORT_TYPE_ANY,
                             actual_reserved_port_type);

    rc = bridge_reserved_port_type_to_enum("normal", &actual_reserved_port_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_RESERVED_PORT_TYPE_NORMAL,
                             actual_reserved_port_type);

    rc = bridge_reserved_port_type_to_enum("flood", &actual_reserved_port_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_RESERVED_PORT_TYPE_FLOOD,
                             actual_reserved_port_type);

    rc = bridge_reserved_port_type_to_enum("unknown", &actual_reserved_port_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_RESERVED_PORT_TYPE_MIN,
                             actual_reserved_port_type);

    rc = bridge_reserved_port_type_to_enum("flood", &actual_reserved_port_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_RESERVED_PORT_TYPE_MAX,
                             actual_reserved_port_type);

    rc = bridge_reserved_port_type_to_enum("All", &actual_reserved_port_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_reserved_port_type_to_enum("ALL", &actual_reserved_port_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case
  {
    rc = bridge_reserved_port_type_to_enum("invalid_args",
                                           &actual_reserved_port_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_reserved_port_type_to_enum(NULL, &actual_reserved_port_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_reserved_port_type_to_enum("set-field", NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_bridge_group_type_to_str(void) {
  lagopus_result_t rc;
  const char *actual_group_type_str;

  // Normal case
  {
    rc = bridge_group_type_to_str(DATASTORE_BRIDGE_GROUP_TYPE_UNKNOWN,
                                  &actual_group_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_group_type_str);

    rc = bridge_group_type_to_str(DATASTORE_BRIDGE_GROUP_TYPE_ALL,
                                  &actual_group_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("all", actual_group_type_str);

    rc = bridge_group_type_to_str(DATASTORE_BRIDGE_GROUP_TYPE_SELECT,
                                  &actual_group_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("select", actual_group_type_str);

    rc = bridge_group_type_to_str(DATASTORE_BRIDGE_GROUP_TYPE_INDIRECT,
                                  &actual_group_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("indirect", actual_group_type_str);

    rc = bridge_group_type_to_str(DATASTORE_BRIDGE_GROUP_TYPE_FAST_FAILOVER,
                                  &actual_group_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("fast-failover", actual_group_type_str);

    rc = bridge_group_type_to_str(DATASTORE_BRIDGE_GROUP_TYPE_MIN,
                                  &actual_group_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_group_type_str);

    rc = bridge_group_type_to_str(DATASTORE_BRIDGE_GROUP_TYPE_MAX,
                                  &actual_group_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("fast-failover", actual_group_type_str);
  }

  // Abnormal case
  {
    rc = bridge_group_type_to_str(DATASTORE_BRIDGE_GROUP_TYPE_ALL, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_bridge_group_type_to_enum(void) {
  lagopus_result_t rc;
  datastore_bridge_group_type_t actual_group_type;

  // Normal case
  {
    rc = bridge_group_type_to_enum("unknown", &actual_group_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_GROUP_TYPE_UNKNOWN,
                             actual_group_type);

    rc = bridge_group_type_to_enum("all", &actual_group_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_GROUP_TYPE_ALL, actual_group_type);

    rc = bridge_group_type_to_enum("select", &actual_group_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_GROUP_TYPE_SELECT,
                             actual_group_type);

    rc = bridge_group_type_to_enum("indirect", &actual_group_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_GROUP_TYPE_INDIRECT,
                             actual_group_type);

    rc = bridge_group_type_to_enum("fast-failover", &actual_group_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_GROUP_TYPE_FAST_FAILOVER,
                             actual_group_type);

    rc = bridge_group_type_to_enum("unknown", &actual_group_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_GROUP_TYPE_MIN, actual_group_type);

    rc = bridge_group_type_to_enum("fast-failover", &actual_group_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_GROUP_TYPE_MAX, actual_group_type);

    rc = bridge_group_type_to_enum("All", &actual_group_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_group_type_to_enum("ALL", &actual_group_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case
  {
    rc = bridge_group_type_to_enum("invalid_args", &actual_group_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_group_type_to_enum(NULL, &actual_group_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_group_type_to_enum("all", NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_bridge_group_capability_to_str(void) {
  lagopus_result_t rc;
  const char *actual_group_capability_str;

  // Normal case
  {
    rc = bridge_group_capability_to_str(DATASTORE_BRIDGE_GROUP_CAPABILITY_UNKNOWN,
                                        &actual_group_capability_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_group_capability_str);

    rc = bridge_group_capability_to_str(
           DATASTORE_BRIDGE_GROUP_CAPABILITY_SELECT_WEIGHT,
           &actual_group_capability_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("select-weight", actual_group_capability_str);

    rc = bridge_group_capability_to_str(
           DATASTORE_BRIDGE_GROUP_CAPABILITY_SELECT_LIVENESS,
           &actual_group_capability_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("select-liveness", actual_group_capability_str);

    rc = bridge_group_capability_to_str(DATASTORE_BRIDGE_GROUP_CAPABILITY_CHAINING,
                                        &actual_group_capability_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("chaining", actual_group_capability_str);

    rc = bridge_group_capability_to_str(
           DATASTORE_BRIDGE_GROUP_CAPABILITY_CHAINING_CHECK,
           &actual_group_capability_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("chaining-check", actual_group_capability_str);

    rc = bridge_group_capability_to_str(DATASTORE_BRIDGE_GROUP_CAPABILITY_MIN,
                                        &actual_group_capability_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_group_capability_str);

    rc = bridge_group_capability_to_str(DATASTORE_BRIDGE_GROUP_CAPABILITY_MAX,
                                        &actual_group_capability_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("chaining-check", actual_group_capability_str);
  }

  // Abnormal case
  {
    rc = bridge_group_capability_to_str(
           DATASTORE_BRIDGE_GROUP_CAPABILITY_SELECT_WEIGHT, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_bridge_group_capability_to_enum(void) {
  lagopus_result_t rc;
  datastore_bridge_group_capability_t actual_group_capability;

  // Normal case
  {
    rc = bridge_group_capability_to_enum("unknown", &actual_group_capability);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_GROUP_CAPABILITY_UNKNOWN,
                             actual_group_capability);

    rc = bridge_group_capability_to_enum("select-weight",
                                         &actual_group_capability);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_GROUP_CAPABILITY_SELECT_WEIGHT,
                             actual_group_capability);

    rc = bridge_group_capability_to_enum("select-liveness",
                                         &actual_group_capability);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_GROUP_CAPABILITY_SELECT_LIVENESS,
                             actual_group_capability);

    rc = bridge_group_capability_to_enum("chaining", &actual_group_capability);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_GROUP_CAPABILITY_CHAINING,
                             actual_group_capability);

    rc = bridge_group_capability_to_enum("chaining-check",
                                         &actual_group_capability);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_GROUP_CAPABILITY_CHAINING_CHECK,
                             actual_group_capability);

    rc = bridge_group_capability_to_enum("unknown", &actual_group_capability);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_GROUP_CAPABILITY_MIN,
                             actual_group_capability);

    rc = bridge_group_capability_to_enum("chaining-check",
                                         &actual_group_capability);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_BRIDGE_GROUP_CAPABILITY_MAX,
                             actual_group_capability);

    rc = bridge_group_capability_to_enum("All", &actual_group_capability);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_group_capability_to_enum("ALL", &actual_group_capability);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case
  {
    rc = bridge_group_capability_to_enum("invalid_args", &actual_group_capability);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_group_capability_to_enum(NULL, &actual_group_capability);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_group_capability_to_enum("select-weight", NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_bridge_conf_private_exists(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name1";
  const char *invalid_name = "invalid_name";

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    TEST_ASSERT_TRUE(bridge_exists(name) == true);
    TEST_ASSERT_TRUE(bridge_exists(invalid_name) == false);
  }

  // Abnormal case
  {
    TEST_ASSERT_TRUE(bridge_exists(NULL) == false);
  }

  bridge_finalize();
}

void
test_bridge_conf_private_used(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  bool actual_used = false;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = bridge_set_used(name, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_bridge_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = bridge_set_used(NULL, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_conf_public_used(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  bool actual_used = false;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_is_used(NULL, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_conf_private_enabled(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  bool actual_enabled = false;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = bridge_set_enabled(name, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_bridge_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = bridge_set_enabled(NULL, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_conf_public_enabled(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  bool actual_enabled = false;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_is_enabled(NULL, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_dpid(void) {
  lagopus_result_t rc;
  const char *name = "bridge_name";
  bridge_conf_t *conf = NULL;
  uint64_t actual_dpid = 0;
  const uint64_t expected_dpid = 0;
  uint64_t set_dpid = 10;
  uint64_t actual_set_dpid = 0;
  const uint64_t expected_set_dpid = set_dpid;
  uint64_t exists_dpid = 30;

  bridge_initialize();

  //rc = bridge_attr_create(&attr);
  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL(conf->modified_attr);

  // Normal case of getter
  {
    rc = bridge_get_dpid(conf->modified_attr,
                         &actual_dpid);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_dpid, actual_dpid);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_dpid(NULL, &actual_dpid);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_get_dpid(conf->modified_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_dpid(conf->modified_attr,
                         set_dpid);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_dpid(conf->modified_attr,
                         &actual_set_dpid);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_dpid, actual_set_dpid);
  }

  // Normal case : exists check.
  {
    rc = bridge_set_dpid(conf->modified_attr,
                         exists_dpid);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_set_dpid(conf->modified_attr,
                         exists_dpid);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ALREADY_EXISTS, rc);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_dpid(NULL, set_dpid);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_public_dpid(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint64_t actual_dpid = 0;
  const uint64_t expected_dpid = 0;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_dpid(name, true, &actual_dpid);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_dpid(name, false, &actual_dpid);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_dpid, actual_dpid);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_dpid(NULL, true, &actual_dpid);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_dpid(NULL, false, &actual_dpid);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_dpid(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_dpid(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_controller_names(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  datastore_name_info_t *actual_controller_names = NULL;
  struct datastore_name_entry *expected_controller_entry = NULL;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of add, getter, remove, remove all
  {
    // add
    rc = bridge_attr_add_controller_name(attr, "bridge1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_attr_add_controller_name(attr, "bridge2");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_attr_add_controller_name(attr, "bridge3");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // getter
    rc = bridge_get_controller_names(attr, &actual_controller_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(3, actual_controller_names->size);
    TAILQ_FOREACH(expected_controller_entry, &(actual_controller_names->head),
                  name_entries) {
      if ((strcmp("bridge1", expected_controller_entry->str) != 0) &&
          (strcmp("bridge2", expected_controller_entry->str) != 0) &&
          (strcmp("bridge3", expected_controller_entry->str) != 0)) {
        TEST_FAIL();
      }
    }
    rc = datastore_names_destroy(actual_controller_names);
    actual_controller_names = NULL;
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // remove
    rc = bridge_attr_remove_controller_name(attr, "bridge1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_attr_remove_controller_name(attr, "bridge3");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_controller_names(attr, &actual_controller_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(1, actual_controller_names->size);
    expected_controller_entry = TAILQ_FIRST(&(actual_controller_names->head));
    TEST_ASSERT_NOT_NULL(expected_controller_entry);
    TEST_ASSERT_EQUAL_STRING("bridge2", expected_controller_entry->str);
    rc = datastore_names_destroy(actual_controller_names);
    actual_controller_names = NULL;
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // remove all
    rc = bridge_attr_remove_all_controller_name(attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_controller_names(attr, &actual_controller_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(0, actual_controller_names->size);
    expected_controller_entry = TAILQ_FIRST(&(actual_controller_names->head));
    TEST_ASSERT_NULL(expected_controller_entry);
    rc = datastore_names_destroy(actual_controller_names);
    actual_controller_names = NULL;
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_controller_names(NULL, &actual_controller_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_get_controller_names(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case of add
  {
    rc = bridge_attr_add_controller_name(NULL, "bridge1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_attr_add_controller_name(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case of remove
  {
    rc = bridge_attr_remove_controller_name(NULL, "bridge1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_attr_remove_controller_name(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case of remove all
  {
    rc = bridge_attr_remove_all_controller_name(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_controller_names(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  datastore_name_info_t *actual_controller_names = NULL;
  struct datastore_name_entry *expected_controller_entry = NULL;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_controller_names(name, true,
         &actual_controller_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    // controller_names
    rc = datastore_bridge_get_controller_names(name, false,
         &actual_controller_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(0, actual_controller_names->size);
    expected_controller_entry = TAILQ_FIRST(&(actual_controller_names->head));
    TEST_ASSERT_NULL(expected_controller_entry);
    rc = datastore_names_destroy(actual_controller_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_controller_names(NULL, true,
         &actual_controller_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_controller_names(NULL, false,
         &actual_controller_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_controller_names(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_controller_names(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_port_names(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  datastore_name_info_t *actual_port_names = NULL;
  struct datastore_name_entry *expected_port_entry = NULL;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of add, getter, remove, remove all
  {
    // add
    rc = bridge_attr_add_port_name(attr, "bridge1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_attr_add_port_name(attr, "bridge2");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_attr_add_port_name(attr, "bridge3");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // getter
    rc = bridge_get_port_names(attr, &actual_port_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(3, actual_port_names->size);
    TAILQ_FOREACH(expected_port_entry, &(actual_port_names->head), name_entries) {
      if ((strcmp("bridge1", expected_port_entry->str) != 0) &&
          (strcmp("bridge2", expected_port_entry->str) != 0) &&
          (strcmp("bridge3", expected_port_entry->str) != 0)) {
        TEST_FAIL();
      }
    }
    rc = datastore_names_destroy(actual_port_names);
    actual_port_names = NULL;
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // remove
    rc = bridge_attr_remove_port_name(attr, "bridge1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_attr_remove_port_name(attr, "bridge3");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_port_names(attr, &actual_port_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(1, actual_port_names->size);
    expected_port_entry = TAILQ_FIRST(&(actual_port_names->head));
    TEST_ASSERT_NOT_NULL(expected_port_entry);
    TEST_ASSERT_EQUAL_STRING("bridge2", expected_port_entry->str);
    rc = datastore_names_destroy(actual_port_names);
    actual_port_names = NULL;
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // remove all
    rc = bridge_attr_remove_all_port_name(attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_port_names(attr, &actual_port_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(0, actual_port_names->size);
    expected_port_entry = TAILQ_FIRST(&(actual_port_names->head));
    TEST_ASSERT_NULL(expected_port_entry);
    rc = datastore_names_destroy(actual_port_names);
    actual_port_names = NULL;
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_port_names(NULL, &actual_port_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_get_port_names(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case of add
  {
    rc = bridge_attr_add_port_name(NULL, "bridge1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_attr_add_port_name(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case of remove
  {
    rc = bridge_attr_remove_port_name(NULL, "bridge1");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_attr_remove_port_name(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case of remove all
  {
    rc = bridge_attr_remove_all_port_name(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_port_names(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  datastore_name_info_t *actual_port_names = NULL;
  struct datastore_name_entry *expected_port_entry = NULL;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_port_names(name, true, &actual_port_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    // controller_names
    rc = datastore_bridge_get_port_names(name, false, &actual_port_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(0, actual_port_names->size);
    expected_port_entry = TAILQ_FIRST(&(actual_port_names->head));
    TEST_ASSERT_NULL(expected_port_entry);
    rc = datastore_names_destroy(actual_port_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_port_names(NULL, true, &actual_port_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_port_names(NULL, false, &actual_port_names);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_port_names(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_port_names(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_flow_statistics(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  bool actual_flow_statistics = false;
  const bool set_flow_statistics = true;
  bool actual_set_flow_statistics = true;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_is_flow_statistics(attr, &actual_flow_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_flow_statistics);
  }

  // Abnormal case of getter
  {
    rc = bridge_is_flow_statistics(NULL, &actual_flow_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_is_flow_statistics(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_flow_statistics(attr, set_flow_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_is_flow_statistics(attr, &actual_set_flow_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_set_flow_statistics);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_flow_statistics(NULL, set_flow_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_flow_statistics(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  bool actual_flow_statistics = false;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_is_flow_statistics(name, true, &actual_flow_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_is_flow_statistics(name, false, &actual_flow_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_flow_statistics);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_is_flow_statistics(NULL, true, &actual_flow_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_flow_statistics(NULL, false, &actual_flow_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_flow_statistics(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_flow_statistics(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_group_statistics(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  bool actual_group_statistics = false;
  const bool set_group_statistics = true;
  bool actual_set_group_statistics = true;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_is_group_statistics(attr, &actual_group_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_group_statistics);
  }

  // Abnormal case of getter
  {
    rc = bridge_is_group_statistics(NULL, &actual_group_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_is_group_statistics(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_group_statistics(attr, set_group_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_is_group_statistics(attr, &actual_set_group_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_set_group_statistics);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_group_statistics(NULL, set_group_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_group_statistics(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  bool actual_group_statistics = false;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_is_group_statistics(name, true,
         &actual_group_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_is_group_statistics(name, false,
         &actual_group_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_group_statistics);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_is_group_statistics(NULL, true,
         &actual_group_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_group_statistics(NULL, false,
         &actual_group_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_group_statistics(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_group_statistics(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_port_statistics(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  bool actual_port_statistics = false;
  const bool set_port_statistics = true;
  bool actual_set_port_statistics = true;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_is_port_statistics(attr, &actual_port_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_port_statistics);
  }

  // Abnormal case of getter
  {
    rc = bridge_is_port_statistics(NULL, &actual_port_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_is_port_statistics(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_port_statistics(attr, set_port_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_is_port_statistics(attr, &actual_set_port_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_set_port_statistics);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_port_statistics(NULL, set_port_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_port_statistics(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  bool actual_port_statistics = false;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_is_port_statistics(name, true, &actual_port_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_is_port_statistics(name, false, &actual_port_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_port_statistics);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_is_port_statistics(NULL, true, &actual_port_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_port_statistics(NULL, false, &actual_port_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_port_statistics(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_port_statistics(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_queue_statistics(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  bool actual_queue_statistics = false;
  const bool set_queue_statistics = true;
  bool actual_set_queue_statistics = true;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_is_queue_statistics(attr, &actual_queue_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_queue_statistics);
  }

  // Abnormal case of getter
  {
    rc = bridge_is_queue_statistics(NULL, &actual_queue_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_is_queue_statistics(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_queue_statistics(attr, set_queue_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_is_queue_statistics(attr, &actual_set_queue_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_set_queue_statistics);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_queue_statistics(NULL, set_queue_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_queue_statistics(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  bool actual_queue_statistics = false;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_is_queue_statistics(name, true,
         &actual_queue_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_is_queue_statistics(name, false,
         &actual_queue_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_queue_statistics);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_is_queue_statistics(NULL, true,
         &actual_queue_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_queue_statistics(NULL, false,
         &actual_queue_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_queue_statistics(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_queue_statistics(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_table_statistics(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  bool actual_table_statistics = false;
  const bool set_table_statistics = true;
  bool actual_set_table_statistics = true;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_is_table_statistics(attr, &actual_table_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_table_statistics);
  }

  // Abnormal case of getter
  {
    rc = bridge_is_table_statistics(NULL, &actual_table_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_is_table_statistics(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_table_statistics(attr, set_table_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_is_table_statistics(attr, &actual_set_table_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_set_table_statistics);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_table_statistics(NULL, set_table_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_table_statistics(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  bool actual_table_statistics = false;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_is_table_statistics(name, true,
         &actual_table_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_is_table_statistics(name, false,
         &actual_table_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_table_statistics);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_is_table_statistics(NULL, true,
         &actual_table_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_table_statistics(NULL, false,
         &actual_table_statistics);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_table_statistics(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_table_statistics(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_reassemble_ip_fragments(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  bool actual_reassemble_ip_fragments = true;
  const bool set_reassemble_ip_fragments = true;
  bool actual_set_reassemble_ip_fragments = true;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_is_reassemble_ip_fragments(attr, &actual_reassemble_ip_fragments);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_reassemble_ip_fragments);
  }

  // Abnormal case of getter
  {
    rc = bridge_is_reassemble_ip_fragments(NULL, &actual_reassemble_ip_fragments);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_is_reassemble_ip_fragments(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_reassemble_ip_fragments(attr, set_reassemble_ip_fragments);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_is_reassemble_ip_fragments(attr,
                                           &actual_set_reassemble_ip_fragments);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_set_reassemble_ip_fragments);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_reassemble_ip_fragments(NULL, set_reassemble_ip_fragments);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_reassemble_ip_fragments(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  bool actual_reassemble_ip_fragments = false;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_is_reassemble_ip_fragments(name, true,
         &actual_reassemble_ip_fragments);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_is_reassemble_ip_fragments(name, false,
         &actual_reassemble_ip_fragments);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_reassemble_ip_fragments);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_is_reassemble_ip_fragments(NULL, true,
         &actual_reassemble_ip_fragments);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_reassemble_ip_fragments(NULL, false,
         &actual_reassemble_ip_fragments);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_reassemble_ip_fragments(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_reassemble_ip_fragments(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_max_buffered_packets(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint32_t actual_max_buffered_packets = MAXIMUM_BUFFERED_PACKETS;
  const uint32_t expected_max_buffered_packets = actual_max_buffered_packets;
  uint64_t set_max_buffered_packets1 = MAXIMUM_BUFFERED_PACKETS;
  uint64_t set_max_buffered_packets2 = MINIMUM_BUFFERED_PACKETS;
  uint64_t set_max_buffered_packets3 = CAST_UINT64(MAXIMUM_BUFFERED_PACKETS + 1);
  int set_max_buffered_packets4 = -1;
  uint32_t actual_set_max_buffered_packets1 = 0;
  uint32_t actual_set_max_buffered_packets2 = 0;
  uint32_t actual_set_max_buffered_packets3 = 0;
  uint32_t actual_set_max_buffered_packets4 = 0;
  const uint32_t expected_set_max_buffered_packets1 = MAXIMUM_BUFFERED_PACKETS;
  const uint32_t expected_set_max_buffered_packets2 = MINIMUM_BUFFERED_PACKETS;
  const uint32_t expected_set_max_buffered_packets3 = MINIMUM_BUFFERED_PACKETS;
  const uint32_t expected_set_max_buffered_packets4 = MINIMUM_BUFFERED_PACKETS;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_max_buffered_packets(attr, &actual_max_buffered_packets);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_buffered_packets,
                             actual_max_buffered_packets);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_max_buffered_packets(NULL, &actual_max_buffered_packets);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_get_max_buffered_packets(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_max_buffered_packets(attr, set_max_buffered_packets1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_max_buffered_packets(attr,
                                         &actual_set_max_buffered_packets1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_buffered_packets1,
                             actual_set_max_buffered_packets1);

    rc = bridge_set_max_buffered_packets(attr, set_max_buffered_packets2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_max_buffered_packets(attr,
                                         &actual_set_max_buffered_packets2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_buffered_packets2,
                             actual_set_max_buffered_packets2);

    rc = bridge_set_max_buffered_packets(attr, set_max_buffered_packets3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = bridge_get_max_buffered_packets(attr, &actual_set_max_buffered_packets3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_buffered_packets3,
                             actual_set_max_buffered_packets3);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_max_buffered_packets(NULL, set_max_buffered_packets1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_set_max_buffered_packets(attr,
                                         CAST_UINT64(set_max_buffered_packets4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = bridge_get_max_buffered_packets(attr,
                                         &actual_set_max_buffered_packets4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_buffered_packets4,
                             actual_set_max_buffered_packets4);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_max_buffered_packets(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint32_t actual_max_buffered_packets = 0;
  const uint32_t expected_max_buffered_packets = 65535;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_max_buffered_packets(name, true,
         &actual_max_buffered_packets);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_max_buffered_packets(name, false,
         &actual_max_buffered_packets);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_buffered_packets,
                             actual_max_buffered_packets);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_max_buffered_packets(NULL, true,
         &actual_max_buffered_packets);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_max_buffered_packets(NULL, false,
         &actual_max_buffered_packets);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_max_buffered_packets(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_max_buffered_packets(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_mactable_max_entries(void) {
#ifdef HYBRID
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint32_t actual_mactable_max_entries = 0;
  uint64_t set_mactable_max_entries1 = MAXIMUM_MACTABLE_MAX_ENTRIES;
  uint64_t set_mactable_max_entries2 = MINIMUM_MACTABLE_MAX_ENTRIES;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_mactable_max_entries(attr, &actual_mactable_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DEFAULT_MACTABLE_MAX_ENTRIES,
                             actual_mactable_max_entries);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_mactable_max_entries(NULL, &actual_mactable_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
    rc = bridge_get_mactable_max_entries(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_mactable_max_entries(attr, set_mactable_max_entries1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_mactable_max_entries(attr, &actual_mactable_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(MAXIMUM_MACTABLE_MAX_ENTRIES,
                             actual_mactable_max_entries);

    rc = bridge_set_mactable_max_entries(attr, set_mactable_max_entries2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_mactable_max_entries(attr, &actual_mactable_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(MINIMUM_MACTABLE_MAX_ENTRIES,
                             actual_mactable_max_entries);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_mactable_max_entries(NULL, set_mactable_max_entries1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_set_mactable_max_entries(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);

    rc = bridge_set_mactable_max_entries(attr,
                                        (MINIMUM_MACTABLE_MAX_ENTRIES - 1));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);

    rc = bridge_set_mactable_max_entries(attr,
                                        (MAXIMUM_MACTABLE_MAX_ENTRIES + 1));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
  }
#else // HYBRID
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif // HYBRID
}

void
test_bridge_attr_public_mactable_max_entries(void) {
#ifdef HYBRID
  lagopus_result_t rc;
  bridge_conf_t * conf = NULL;
  const char *name = "bridge_name";
  uint32_t actual_mactable_max_entries = 0;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_mactable_max_entries(name, true,
                                          &actual_mactable_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_mactable_max_entries(name, false,
                                          &actual_mactable_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DEFAULT_MACTABLE_MAX_ENTRIES,
                             actual_mactable_max_entries);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_mactable_max_entries(NULL, true,
                                          &actual_mactable_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_mactable_max_entries(NULL, false,
                                          &actual_mactable_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_mactable_max_entries(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_mactable_max_entries(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
#else // HYBRID
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif // HYBRID
}

void
test_bridge_attr_private_mactable_ageing_time(void) {
#ifdef HYBRID
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint32_t actual_mactable_ageing_time = 0;
  uint64_t set_mactable_ageing_time1 = MAXIMUM_MACTABLE_AGEING_TIME;
  uint64_t set_mactable_ageing_time2 = MINIMUM_MACTABLE_AGEING_TIME;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_mactable_ageing_time(attr, &actual_mactable_ageing_time);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DEFAULT_MACTABLE_AGEING_TIME,
                             actual_mactable_ageing_time);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_mactable_ageing_time(NULL, &actual_mactable_ageing_time);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
    rc = bridge_get_mactable_ageing_time(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_mactable_ageing_time(attr, set_mactable_ageing_time1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_mactable_ageing_time(attr, &actual_mactable_ageing_time);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(MAXIMUM_MACTABLE_AGEING_TIME,
                             actual_mactable_ageing_time);

    rc = bridge_set_mactable_ageing_time(attr, set_mactable_ageing_time2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_mactable_ageing_time(attr, &actual_mactable_ageing_time);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(MINIMUM_MACTABLE_AGEING_TIME,
                             actual_mactable_ageing_time);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_mactable_ageing_time(NULL, set_mactable_ageing_time1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_set_mactable_ageing_time(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);

    rc = bridge_set_mactable_ageing_time(attr,
                                         (MINIMUM_MACTABLE_AGEING_TIME - 1));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);

    rc = bridge_set_mactable_ageing_time(attr,
                                        (MAXIMUM_MACTABLE_AGEING_TIME + 1));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
  }
#else // HYBRID
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif // HYBRID
}

void
test_bridge_attr_public_mactable_ageing_time(void) {
#ifdef HYBRID
  lagopus_result_t rc;
  bridge_conf_t * conf = NULL;
  const char *name = "bridge_name";
  uint32_t actual_mactable_ageing_time = 0;
  const uint32_t expected_mactable_ageing_time = 0;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_mactable_ageing_time(name, true,
                                          &actual_mactable_ageing_time);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_mactable_ageing_time(name, false,
                                          &actual_mactable_ageing_time);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DEFAULT_MACTABLE_AGEING_TIME,
                             actual_mactable_ageing_time);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_mactable_ageing_time(NULL, true,
                                          &actual_mactable_ageing_time);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_mactable_ageing_time(NULL, false,
                                          &actual_mactable_ageing_time);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_mactable_ageing_time(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_mactable_ageing_time(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
#else // HYBRID
  TEST_IGNORE_MESSAGE("HYBRID is not defined.");
#endif // HYBRID
}

void
test_bridge_attr_private_max_ports(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint16_t actual_max_ports = 0;
  const uint16_t expected_max_ports = 255;
  uint64_t set_max_ports1 = MAXIMUM_PORTS;
  uint64_t set_max_ports2 = MINIMUM_PORTS;
  uint64_t set_max_ports3 = CAST_UINT64(MAXIMUM_PORTS + 1);
  uint64_t set_max_ports4 = CAST_UINT64(MINIMUM_PORTS - 1);
  int set_max_ports5 = -1;
  uint16_t actual_set_max_ports1 = 0;
  uint16_t actual_set_max_ports2 = 0;
  uint16_t actual_set_max_ports3 = 0;
  uint16_t actual_set_max_ports4 = 0;
  uint16_t actual_set_max_ports5 = 0;
  const uint16_t expected_set_max_ports1 = MAXIMUM_PORTS;
  const uint16_t expected_set_max_ports2 = MINIMUM_PORTS;
  const uint16_t expected_set_max_ports3 = MINIMUM_PORTS;
  const uint16_t expected_set_max_ports4 = MINIMUM_PORTS;
  const uint16_t expected_set_max_ports5 = MINIMUM_PORTS;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_max_ports(attr, &actual_max_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_ports, actual_max_ports);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_max_ports(NULL, &actual_max_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_get_max_ports(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_max_ports(attr, set_max_ports1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_max_ports(attr, &actual_set_max_ports1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_ports1, actual_set_max_ports1);

    rc = bridge_set_max_ports(attr, set_max_ports2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_max_ports(attr, &actual_set_max_ports2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_ports2, actual_set_max_ports2);

    rc = bridge_set_max_ports(attr, set_max_ports3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = bridge_get_max_ports(attr, &actual_set_max_ports3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_ports3, actual_set_max_ports3);

    rc = bridge_set_max_ports(attr, set_max_ports4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = bridge_get_max_ports(attr, &actual_set_max_ports4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_ports4, actual_set_max_ports4);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_max_ports(NULL, set_max_ports1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_set_max_ports(attr, CAST_UINT64(set_max_ports5));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = bridge_get_max_ports(attr, &actual_set_max_ports5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_ports5, actual_set_max_ports5);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_max_ports(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint16_t actual_max_ports = 0;
  const uint16_t expected_max_ports = 255;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_max_ports(name, true, &actual_max_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_max_ports(name, false, &actual_max_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_ports, actual_max_ports);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_max_ports(NULL, true, &actual_max_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_max_ports(NULL, false, &actual_max_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_max_ports(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_max_ports(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_max_tables(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint8_t actual_max_tables = 0;
  const uint8_t expected_max_tables = 255;
  uint64_t set_max_tables1 = MAXIMUM_TABLES;
  uint64_t set_max_tables2 = MINIMUM_TABLES;
  uint64_t set_max_tables3 = CAST_UINT64(MAXIMUM_TABLES + 1);
  uint64_t set_max_tables4 = CAST_UINT64(MINIMUM_TABLES - 1);
  int set_max_tables5 = -1;
  uint8_t actual_set_max_tables1 = 0;
  uint8_t actual_set_max_tables2 = 0;
  uint8_t actual_set_max_tables3 = 0;
  uint8_t actual_set_max_tables4 = 0;
  uint8_t actual_set_max_tables5 = 0;
  const uint8_t expected_set_max_tables1 = MAXIMUM_TABLES;
  const uint8_t expected_set_max_tables2 = MINIMUM_TABLES;
  const uint8_t expected_set_max_tables3 = MINIMUM_TABLES;
  const uint8_t expected_set_max_tables4 = MINIMUM_TABLES;
  const uint8_t expected_set_max_tables5 = MINIMUM_TABLES;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_max_tables(attr, &actual_max_tables);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_tables, actual_max_tables);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_max_tables(NULL, &actual_max_tables);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_get_max_tables(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_max_tables(attr, set_max_tables1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_max_tables(attr, &actual_set_max_tables1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_tables1, actual_set_max_tables1);

    rc = bridge_set_max_tables(attr, set_max_tables2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_max_tables(attr, &actual_set_max_tables2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_tables2, actual_set_max_tables2);

    rc = bridge_set_max_tables(attr, set_max_tables3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = bridge_get_max_tables(attr, &actual_set_max_tables3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_tables3, actual_set_max_tables3);

    rc = bridge_set_max_tables(attr, set_max_tables4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = bridge_get_max_tables(attr, &actual_set_max_tables4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_tables4, actual_set_max_tables4);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_max_tables(NULL, set_max_tables1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_set_max_tables(attr, CAST_UINT64(set_max_tables5));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = bridge_get_max_tables(attr, &actual_set_max_tables5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_tables5, actual_set_max_tables5);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_max_tables(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint8_t actual_max_tables = 0;
  const uint8_t expected_max_tables = 255;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_max_tables(name, true, &actual_max_tables);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_max_tables(name, false, &actual_max_tables);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_tables, actual_max_tables);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_max_tables(NULL, true, &actual_max_tables);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_max_tables(NULL, false, &actual_max_tables);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_max_tables(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_max_tables(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_block_looping_ports(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  bool actual_block_looping_ports = true;
  const bool set_block_looping_ports = true;
  bool actual_set_block_looping_ports = true;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_is_block_looping_ports(attr, &actual_block_looping_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_block_looping_ports);
  }

  // Abnormal case of getter
  {
    rc = bridge_is_block_looping_ports(NULL, &actual_block_looping_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_is_block_looping_ports(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_block_looping_ports(attr, set_block_looping_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_is_block_looping_ports(attr, &actual_set_block_looping_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(actual_set_block_looping_ports);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_block_looping_ports(NULL, set_block_looping_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_block_looping_ports(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  bool actual_block_looping_ports = false;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_is_block_looping_ports(name, true,
         &actual_block_looping_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_is_block_looping_ports(name, false,
         &actual_block_looping_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_block_looping_ports);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_is_block_looping_ports(NULL, true,
         &actual_block_looping_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_block_looping_ports(NULL, false,
         &actual_block_looping_ports);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_block_looping_ports(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_is_block_looping_ports(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_action_types(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint64_t actual_action_types = 0;
  const uint64_t expected_action_types = UINT64_MAX;
  uint64_t set_action_types = 10;
  uint64_t actual_set_action_types = 0;
  const uint64_t expected_set_action_types = set_action_types;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_action_types(attr, &actual_action_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_action_types, actual_action_types);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_action_types(NULL, &actual_action_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_get_action_types(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_action_types(attr, set_action_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_action_types(attr, &actual_set_action_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_action_types, actual_set_action_types);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_action_types(NULL, set_action_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_action_types(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint64_t actual_action_types = 0;
  const uint64_t expected_action_types = UINT64_MAX;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_action_types(name, true, &actual_action_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_action_types(name, false, &actual_action_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_action_types, actual_action_types);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_action_types(NULL, true, &actual_action_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_action_types(NULL, false, &actual_action_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_action_types(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_action_types(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_instruction_types(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint64_t actual_instruction_types = 0;
  const uint64_t expected_instruction_types = UINT64_MAX;
  uint64_t set_instruction_types = 10;
  uint64_t actual_set_instruction_types = 0;
  const uint64_t expected_set_instruction_types = set_instruction_types;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_instruction_types(attr, &actual_instruction_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_instruction_types, actual_instruction_types);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_instruction_types(NULL, &actual_instruction_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_instruction_types(attr, set_instruction_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_instruction_types(attr, &actual_set_instruction_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_instruction_types,
                             actual_set_instruction_types);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_instruction_types(NULL, set_instruction_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_instruction_types(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint64_t actual_instruction_types = 0;
  const uint64_t expected_instruction_types = UINT64_MAX;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_instruction_types(name, true,
         &actual_instruction_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_instruction_types(name, false,
         &actual_instruction_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_instruction_types, actual_instruction_types);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_instruction_types(NULL, true,
         &actual_instruction_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_instruction_types(NULL, false,
         &actual_instruction_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_instruction_types(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_instruction_types(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_max_flows(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint32_t actual_max_flows = 0;
  const uint64_t expected_max_flows = MAXIMUM_FLOWS;
  uint32_t set_max_flows1 = MAXIMUM_FLOWS;
  uint32_t set_max_flows2 = MINIMUM_FLOWS;
  uint64_t set_max_flows3 = CAST_UINT64(MAXIMUM_FLOWS + 1);
  int set_max_flows4 = -1;
  uint32_t actual_set_max_flows1 = 0;
  uint32_t actual_set_max_flows2 = 0;
  uint32_t actual_set_max_flows3 = 0;
  uint32_t actual_set_max_flows4 = 0;
  const uint32_t expected_set_max_flows1 = MAXIMUM_FLOWS;
  const uint32_t expected_set_max_flows2 = MINIMUM_FLOWS;
  const uint32_t expected_set_max_flows3 = MINIMUM_FLOWS;
  const uint32_t expected_set_max_flows4 = MINIMUM_FLOWS;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_max_flows(attr, &actual_max_flows);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_flows, actual_max_flows);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_max_flows(NULL, &actual_max_flows);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_max_flows(attr, set_max_flows1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_max_flows(attr, &actual_set_max_flows1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_flows1, actual_set_max_flows1);

    rc = bridge_set_max_flows(attr, set_max_flows2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_max_flows(attr, &actual_set_max_flows2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_flows2, actual_set_max_flows2);

    rc = bridge_set_max_flows(attr, set_max_flows3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = bridge_get_max_flows(attr, &actual_set_max_flows3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_flows3, actual_set_max_flows3);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_max_flows(NULL, set_max_flows1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_set_max_flows(attr, CAST_UINT64(set_max_flows4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = bridge_get_max_flows(attr, &actual_set_max_flows4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_max_flows4, actual_set_max_flows4);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_max_flows(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint32_t actual_max_flows = 0;
  const uint32_t expected_max_flows = MAXIMUM_FLOWS;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_max_flows(name, true, &actual_max_flows);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_max_flows(name, false, &actual_max_flows);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_flows, actual_max_flows);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_max_flows(NULL, true, &actual_max_flows);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_max_flows(NULL, false, &actual_max_flows);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_max_flows(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_max_flows(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_reserved_port_types(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint64_t actual_reserved_port_types = 0;
  const uint64_t expected_reserved_port_types = UINT64_MAX;
  uint64_t set_reserved_port_types = 10;
  uint64_t actual_set_reserved_port_types = 0;
  const uint64_t expected_set_reserved_port_types = set_reserved_port_types;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_reserved_port_types(attr, &actual_reserved_port_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_reserved_port_types,
                             actual_reserved_port_types);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_reserved_port_types(NULL, &actual_reserved_port_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_reserved_port_types(attr, set_reserved_port_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_reserved_port_types(attr, &actual_set_reserved_port_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_reserved_port_types,
                             actual_set_reserved_port_types);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_reserved_port_types(NULL, set_reserved_port_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_reserved_port_types(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint64_t actual_reserved_port_types = 0;
  const uint64_t expected_reserved_port_types = UINT64_MAX;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_reserved_port_types(name, true,
         &actual_reserved_port_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_reserved_port_types(name, false,
         &actual_reserved_port_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_reserved_port_types,
                             actual_reserved_port_types);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_reserved_port_types(NULL, true,
         &actual_reserved_port_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_reserved_port_types(NULL, false,
         &actual_reserved_port_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_reserved_port_types(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_reserved_port_types(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_group_types(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint64_t actual_group_types = 0;
  const uint64_t expected_group_types = UINT64_MAX;
  uint64_t set_group_types = 10;
  uint64_t actual_set_group_types = 0;
  const uint64_t expected_set_group_types = set_group_types;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_group_types(attr, &actual_group_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_group_types, actual_group_types);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_group_types(NULL, &actual_group_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_group_types(attr, set_group_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_group_types(attr, &actual_set_group_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_group_types,
                             actual_set_group_types);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_group_types(NULL, set_group_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_group_types(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint64_t actual_group_types = 0;
  const uint64_t expected_group_types = UINT64_MAX;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_group_types(name, true,
                                          &actual_group_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_group_types(name, false,
                                          &actual_group_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_group_types, actual_group_types);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_group_types(NULL, true, &actual_group_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_group_types(NULL, false, &actual_group_types);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_group_types(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_group_types(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_group_capabilities(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint64_t actual_group_capabilities = 0;
  const uint64_t expected_group_capabilities = UINT64_MAX;
  uint64_t set_group_capabilities = 10;
  uint64_t actual_set_group_capabilities = 0;
  const uint64_t expected_set_group_capabilities = set_group_capabilities;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_group_capabilities(attr, &actual_group_capabilities);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_group_capabilities,
                             actual_group_capabilities);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_group_capabilities(NULL, &actual_group_capabilities);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_group_capabilities(attr, set_group_capabilities);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_group_capabilities(attr, &actual_set_group_capabilities);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_group_capabilities,
                             actual_set_group_capabilities);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_group_capabilities(NULL, set_group_capabilities);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_group_capabilities(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint64_t actual_group_capabilities = 0;
  const uint64_t expected_group_capabilities = UINT64_MAX;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_group_capabilities(name, true,
         &actual_group_capabilities);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_group_capabilities(name, false,
         &actual_group_capabilities);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_group_capabilities,
                             actual_group_capabilities);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_group_capabilities(NULL, true,
         &actual_group_capabilities);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_group_capabilities(NULL, false,
         &actual_group_capabilities);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_group_capabilities(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_group_capabilities(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_packet_inq_size(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint16_t actual_packet_inq_size = 0;
  const uint64_t expected_packet_inq_size = 1000;
  uint16_t set_packet_inq_size1 = MAXIMUM_PACKET_INQ_SIZE;
  uint16_t set_packet_inq_size2 = MINIMUM_PACKET_INQ_SIZE;
  uint64_t set_packet_inq_size3 = CAST_UINT64(MAXIMUM_PACKET_INQ_SIZE + 1);
  int set_packet_inq_size4 = -1;
  uint16_t actual_set_packet_inq_size1 = 0;
  uint16_t actual_set_packet_inq_size2 = 0;
  uint16_t actual_set_packet_inq_size3 = 0;
  uint16_t actual_set_packet_inq_size4 = 0;
  const uint16_t expected_set_packet_inq_size1 = MAXIMUM_PACKET_INQ_SIZE;
  const uint16_t expected_set_packet_inq_size2 = MINIMUM_PACKET_INQ_SIZE;
  const uint16_t expected_set_packet_inq_size3 = MINIMUM_PACKET_INQ_SIZE;
  const uint16_t expected_set_packet_inq_size4 = MINIMUM_PACKET_INQ_SIZE;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_packet_inq_size(attr, &actual_packet_inq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_packet_inq_size, actual_packet_inq_size);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_packet_inq_size(NULL, &actual_packet_inq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_packet_inq_size(attr, set_packet_inq_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_packet_inq_size(attr, &actual_set_packet_inq_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_packet_inq_size1,
                             actual_set_packet_inq_size1);

    rc = bridge_set_packet_inq_size(attr, set_packet_inq_size2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_packet_inq_size(attr, &actual_set_packet_inq_size2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_packet_inq_size2,
                             actual_set_packet_inq_size2);

    rc = bridge_set_packet_inq_size(attr, set_packet_inq_size3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = bridge_get_packet_inq_size(attr, &actual_set_packet_inq_size3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_packet_inq_size3,
                             actual_set_packet_inq_size3);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_packet_inq_size(NULL, set_packet_inq_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_set_packet_inq_size(attr, CAST_UINT64(set_packet_inq_size4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = bridge_get_packet_inq_size(attr, &actual_set_packet_inq_size4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_packet_inq_size4,
                             actual_set_packet_inq_size4);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_packet_inq_size(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint16_t actual_packet_inq_size = 0;
  const uint16_t expected_packet_inq_size = 1000;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_packet_inq_size(name, true,
         &actual_packet_inq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_packet_inq_size(name, false,
         &actual_packet_inq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_packet_inq_size,
                             actual_packet_inq_size);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_packet_inq_size(NULL, true,
         &actual_packet_inq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_packet_inq_size(NULL, false,
         &actual_packet_inq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_packet_inq_size(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_packet_inq_size(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_packet_inq_max_batches(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint16_t actual_packet_inq_max_batches = 0;
  const uint64_t expected_packet_inq_max_batches = 1000;
  uint16_t set_packet_inq_max_batches1 = MAXIMUM_PACKET_INQ_MAX_BATCHES;
  uint16_t set_packet_inq_max_batches2 = MINIMUM_PACKET_INQ_MAX_BATCHES;
  uint64_t set_packet_inq_max_batches3 =CAST_UINT64(
                                          MAXIMUM_PACKET_INQ_MAX_BATCHES + 1);
  int set_packet_inq_max_batches4 = -1;
  uint16_t actual_set_packet_inq_max_batches1 = 0;
  uint16_t actual_set_packet_inq_max_batches2 = 0;
  uint16_t actual_set_packet_inq_max_batches3 = 0;
  uint16_t actual_set_packet_inq_max_batches4 = 0;
  const uint16_t expected_set_packet_inq_max_batches1 =
    MAXIMUM_PACKET_INQ_MAX_BATCHES;
  const uint16_t expected_set_packet_inq_max_batches2 =
    MINIMUM_PACKET_INQ_MAX_BATCHES;
  const uint16_t expected_set_packet_inq_max_batches3 =
    MINIMUM_PACKET_INQ_MAX_BATCHES;
  const uint16_t expected_set_packet_inq_max_batches4 =
    MINIMUM_PACKET_INQ_MAX_BATCHES;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_packet_inq_max_batches(attr, &actual_packet_inq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_packet_inq_max_batches,
                             actual_packet_inq_max_batches);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_packet_inq_max_batches(NULL, &actual_packet_inq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_packet_inq_max_batches(attr, set_packet_inq_max_batches1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_packet_inq_max_batches(attr,
                                           &actual_set_packet_inq_max_batches1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_packet_inq_max_batches1,
                             actual_set_packet_inq_max_batches1);

    rc = bridge_set_packet_inq_max_batches(attr, set_packet_inq_max_batches2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_packet_inq_max_batches(attr,
                                           &actual_set_packet_inq_max_batches2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_packet_inq_max_batches2,
                             actual_set_packet_inq_max_batches2);

    rc = bridge_set_packet_inq_max_batches(attr, set_packet_inq_max_batches3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = bridge_get_packet_inq_max_batches(attr,
                                           &actual_set_packet_inq_max_batches3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_packet_inq_max_batches3,
                             actual_set_packet_inq_max_batches3);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_packet_inq_max_batches(NULL, set_packet_inq_max_batches1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_set_packet_inq_max_batches(attr,
                                           CAST_UINT64(set_packet_inq_max_batches4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = bridge_get_packet_inq_max_batches(attr,
                                           &actual_set_packet_inq_max_batches4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_packet_inq_max_batches4,
                             actual_set_packet_inq_max_batches4);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_packet_inq_max_batches(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint16_t actual_packet_inq_max_batches = 0;
  const uint16_t expected_packet_inq_max_batches = 1000;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_packet_inq_max_batches(name, true,
         &actual_packet_inq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_packet_inq_max_batches(name, false,
         &actual_packet_inq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_packet_inq_max_batches,
                             actual_packet_inq_max_batches);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_packet_inq_max_batches(NULL, true,
         &actual_packet_inq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_packet_inq_max_batches(NULL, false,
         &actual_packet_inq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_packet_inq_max_batches(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_packet_inq_max_batches(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_up_streamq_size(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint16_t actual_up_streamq_size = 0;
  const uint64_t expected_up_streamq_size = 1000;
  uint16_t set_up_streamq_size1 = MAXIMUM_UP_STREAMQ_SIZE;
  uint16_t set_up_streamq_size2 = MINIMUM_UP_STREAMQ_SIZE;
  uint64_t set_up_streamq_size3 = CAST_UINT64(MAXIMUM_UP_STREAMQ_SIZE + 1);
  int set_up_streamq_size4 = -1;
  uint16_t actual_set_up_streamq_size1 = 0;
  uint16_t actual_set_up_streamq_size2 = 0;
  uint16_t actual_set_up_streamq_size3 = 0;
  uint16_t actual_set_up_streamq_size4 = 0;
  const uint16_t expected_set_up_streamq_size1 = MAXIMUM_UP_STREAMQ_SIZE;
  const uint16_t expected_set_up_streamq_size2 = MINIMUM_UP_STREAMQ_SIZE;
  const uint16_t expected_set_up_streamq_size3 = MINIMUM_UP_STREAMQ_SIZE;
  const uint16_t expected_set_up_streamq_size4 = MINIMUM_UP_STREAMQ_SIZE;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_up_streamq_size(attr, &actual_up_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_up_streamq_size, actual_up_streamq_size);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_up_streamq_size(NULL, &actual_up_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_up_streamq_size(attr, set_up_streamq_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_up_streamq_size(attr, &actual_set_up_streamq_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_up_streamq_size1,
                             actual_set_up_streamq_size1);

    rc = bridge_set_up_streamq_size(attr, set_up_streamq_size2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_up_streamq_size(attr, &actual_set_up_streamq_size2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_up_streamq_size2,
                             actual_set_up_streamq_size2);

    rc = bridge_set_up_streamq_size(attr, set_up_streamq_size3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = bridge_get_up_streamq_size(attr, &actual_set_up_streamq_size3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_up_streamq_size3,
                             actual_set_up_streamq_size3);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_up_streamq_size(NULL, set_up_streamq_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_set_up_streamq_size(attr, CAST_UINT64(set_up_streamq_size4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = bridge_get_up_streamq_size(attr, &actual_set_up_streamq_size4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_up_streamq_size4,
                             actual_set_up_streamq_size4);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_up_streamq_size(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint16_t actual_up_streamq_size = 0;
  const uint16_t expected_up_streamq_size = 1000;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_up_streamq_size(name, true, &actual_up_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_up_streamq_size(name, false,
         &actual_up_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_up_streamq_size, actual_up_streamq_size);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_up_streamq_size(NULL, true, &actual_up_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_up_streamq_size(NULL, false,
         &actual_up_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_up_streamq_size(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_up_streamq_size(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_up_streamq_max_batches(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint16_t actual_up_streamq_max_batches = 0;
  const uint64_t expected_up_streamq_max_batches = 1000;
  uint16_t set_up_streamq_max_batches1 = MAXIMUM_UP_STREAMQ_MAX_BATCHES;
  uint16_t set_up_streamq_max_batches2 = MINIMUM_UP_STREAMQ_MAX_BATCHES;
  uint64_t set_up_streamq_max_batches3 = CAST_UINT64(
      MAXIMUM_UP_STREAMQ_MAX_BATCHES + 1);
  int set_up_streamq_max_batches4 = -1;
  uint16_t actual_set_up_streamq_max_batches1 = 0;
  uint16_t actual_set_up_streamq_max_batches2 = 0;
  uint16_t actual_set_up_streamq_max_batches3 = 0;
  uint16_t actual_set_up_streamq_max_batches4 = 0;
  const uint16_t expected_set_up_streamq_max_batches1 =
    MAXIMUM_UP_STREAMQ_MAX_BATCHES;
  const uint16_t expected_set_up_streamq_max_batches2 =
    MINIMUM_UP_STREAMQ_MAX_BATCHES;
  const uint16_t expected_set_up_streamq_max_batches3 =
    MINIMUM_UP_STREAMQ_MAX_BATCHES;
  const uint16_t expected_set_up_streamq_max_batches4 =
    MINIMUM_UP_STREAMQ_MAX_BATCHES;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_up_streamq_max_batches(attr, &actual_up_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_up_streamq_max_batches,
                             actual_up_streamq_max_batches);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_up_streamq_max_batches(NULL, &actual_up_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_up_streamq_max_batches(attr, set_up_streamq_max_batches1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_up_streamq_max_batches(attr,
                                           &actual_set_up_streamq_max_batches1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_up_streamq_max_batches1,
                             actual_set_up_streamq_max_batches1);

    rc = bridge_set_up_streamq_max_batches(attr, set_up_streamq_max_batches2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_up_streamq_max_batches(attr,
                                           &actual_set_up_streamq_max_batches2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_up_streamq_max_batches2,
                             actual_set_up_streamq_max_batches2);

    rc = bridge_set_up_streamq_max_batches(attr, set_up_streamq_max_batches3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = bridge_get_up_streamq_max_batches(attr,
                                           &actual_set_up_streamq_max_batches3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_up_streamq_max_batches3,
                             actual_set_up_streamq_max_batches3);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_up_streamq_max_batches(NULL, set_up_streamq_max_batches1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_set_up_streamq_max_batches(attr,
                                           CAST_UINT64(set_up_streamq_max_batches4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = bridge_get_up_streamq_max_batches(attr,
                                           &actual_set_up_streamq_max_batches4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_up_streamq_max_batches4,
                             actual_set_up_streamq_max_batches4);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_up_streamq_max_batches(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint16_t actual_up_streamq_max_batches = 0;
  const uint16_t expected_up_streamq_max_batches = 1000;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_up_streamq_max_batches(name, true,
         &actual_up_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_up_streamq_max_batches(name, false,
         &actual_up_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_up_streamq_max_batches,
                             actual_up_streamq_max_batches);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_up_streamq_max_batches(NULL, true,
         &actual_up_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_up_streamq_max_batches(NULL, false,
         &actual_up_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_up_streamq_max_batches(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_up_streamq_max_batches(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_down_streamq_size(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint16_t actual_down_streamq_size = 0;
  const uint64_t expected_down_streamq_size = 1000;
  uint16_t set_down_streamq_size1 = MAXIMUM_DOWN_STREAMQ_SIZE;
  uint16_t set_down_streamq_size2 = MINIMUM_DOWN_STREAMQ_SIZE;
  uint64_t set_down_streamq_size3 = CAST_UINT64(MAXIMUM_DOWN_STREAMQ_SIZE + 1);
  int set_down_streamq_size4 = -1;
  uint16_t actual_set_down_streamq_size1 = 0;
  uint16_t actual_set_down_streamq_size2 = 0;
  uint16_t actual_set_down_streamq_size3 = 0;
  uint16_t actual_set_down_streamq_size4 = 0;
  const uint16_t expected_set_down_streamq_size1 = MAXIMUM_DOWN_STREAMQ_SIZE;
  const uint16_t expected_set_down_streamq_size2 = MINIMUM_DOWN_STREAMQ_SIZE;
  const uint16_t expected_set_down_streamq_size3 = MINIMUM_DOWN_STREAMQ_SIZE;
  const uint16_t expected_set_down_streamq_size4 = MINIMUM_DOWN_STREAMQ_SIZE;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_down_streamq_size(attr, &actual_down_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_down_streamq_size, actual_down_streamq_size);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_down_streamq_size(NULL, &actual_down_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_down_streamq_size(attr, set_down_streamq_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_down_streamq_size(attr, &actual_set_down_streamq_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_down_streamq_size1,
                             actual_set_down_streamq_size1);

    rc = bridge_set_down_streamq_size(attr, set_down_streamq_size2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_down_streamq_size(attr, &actual_set_down_streamq_size2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_down_streamq_size2,
                             actual_set_down_streamq_size2);

    rc = bridge_set_down_streamq_size(attr, set_down_streamq_size3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = bridge_get_down_streamq_size(attr, &actual_set_down_streamq_size3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_down_streamq_size3,
                             actual_set_down_streamq_size3);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_down_streamq_size(NULL, set_down_streamq_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_set_down_streamq_size(attr, CAST_UINT64(set_down_streamq_size4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = bridge_get_down_streamq_size(attr, &actual_set_down_streamq_size4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_down_streamq_size4,
                             actual_set_down_streamq_size4);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_down_streamq_size(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint16_t actual_down_streamq_size = 0;
  const uint16_t expected_down_streamq_size = 1000;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_down_streamq_size(name, true,
         &actual_down_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_down_streamq_size(name, false,
         &actual_down_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_down_streamq_size, actual_down_streamq_size);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_down_streamq_size(NULL, true,
         &actual_down_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_down_streamq_size(NULL, false,
         &actual_down_streamq_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_down_streamq_size(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_down_streamq_size(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

void
test_bridge_attr_private_down_streamq_max_batches(void) {
  lagopus_result_t rc;
  bridge_attr_t *attr = NULL;
  uint16_t actual_down_streamq_max_batches = 0;
  const uint64_t expected_down_streamq_max_batches = 1000;
  uint16_t set_down_streamq_max_batches1 = MAXIMUM_DOWN_STREAMQ_MAX_BATCHES;
  uint16_t set_down_streamq_max_batches2 = MINIMUM_DOWN_STREAMQ_MAX_BATCHES;
  uint64_t set_down_streamq_max_batches3 = CAST_UINT64(
        MAXIMUM_DOWN_STREAMQ_MAX_BATCHES + 1);
  int set_down_streamq_max_batches4 = -1;
  uint16_t actual_set_down_streamq_max_batches1 = 0;
  uint16_t actual_set_down_streamq_max_batches2 = 0;
  uint16_t actual_set_down_streamq_max_batches3 = 0;
  uint16_t actual_set_down_streamq_max_batches4 = 0;
  const uint16_t expected_set_down_streamq_max_batches1 =
    MAXIMUM_DOWN_STREAMQ_MAX_BATCHES;
  const uint16_t expected_set_down_streamq_max_batches2 =
    MINIMUM_DOWN_STREAMQ_MAX_BATCHES;
  const uint16_t expected_set_down_streamq_max_batches3 =
    MINIMUM_DOWN_STREAMQ_MAX_BATCHES;
  const uint16_t expected_set_down_streamq_max_batches4 =
    MINIMUM_DOWN_STREAMQ_MAX_BATCHES;

  bridge_initialize();

  rc = bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new bridge");

  // Normal case of getter
  {
    rc = bridge_get_down_streamq_max_batches(attr,
         &actual_down_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_down_streamq_max_batches,
                             actual_down_streamq_max_batches);
  }

  // Abnormal case of getter
  {
    rc = bridge_get_down_streamq_max_batches(NULL,
         &actual_down_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = bridge_set_down_streamq_max_batches(attr, set_down_streamq_max_batches1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_down_streamq_max_batches(attr,
         &actual_set_down_streamq_max_batches1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_down_streamq_max_batches1,
                             actual_set_down_streamq_max_batches1);

    rc = bridge_set_down_streamq_max_batches(attr, set_down_streamq_max_batches2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = bridge_get_down_streamq_max_batches(attr,
         &actual_set_down_streamq_max_batches2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_down_streamq_max_batches2,
                             actual_set_down_streamq_max_batches2);

    rc = bridge_set_down_streamq_max_batches(attr, set_down_streamq_max_batches3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = bridge_get_down_streamq_max_batches(attr,
         &actual_set_down_streamq_max_batches3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_down_streamq_max_batches3,
                             actual_set_down_streamq_max_batches3);
  }

  // Abnormal case of setter
  {
    rc = bridge_set_down_streamq_max_batches(NULL, set_down_streamq_max_batches1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = bridge_set_down_streamq_max_batches(attr,
         CAST_UINT64(set_down_streamq_max_batches4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = bridge_get_down_streamq_max_batches(attr,
         &actual_set_down_streamq_max_batches4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_down_streamq_max_batches4,
                             actual_set_down_streamq_max_batches4);
  }

  bridge_attr_destroy(attr);

  bridge_finalize();
}

void
test_bridge_attr_public_down_streamq_max_batches(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *name = "bridge_name";
  uint16_t actual_down_streamq_max_batches = 0;
  const uint16_t expected_down_streamq_max_batches = 1000;

  bridge_initialize();

  rc = bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

  rc = bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_bridge_get_down_streamq_max_batches(name, true,
         &actual_down_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_bridge_get_down_streamq_max_batches(name, false,
         &actual_down_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_down_streamq_max_batches,
                             actual_down_streamq_max_batches);
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_down_streamq_max_batches(NULL, true,
         &actual_down_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_down_streamq_max_batches(NULL, false,
         &actual_down_streamq_max_batches);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_down_streamq_max_batches(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_down_streamq_max_batches(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}

#define BRIDGE_NAME_TEST_NUM 4

void
test_bridge_conf_public_bridge_name_by_dpid(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  char *name = NULL;
  uint64_t dpid_not_found = 0xff;
  struct {
    const char *name;
    uint64_t dpid;
  } bridges[BRIDGE_NAME_TEST_NUM] =
        {{"bridge_name01", 0x01},
         {"bridge_name02", 0x02},
         {"bridge_name03", 0x03},
         {"bridge_name04", 0x04}};
  size_t i;

  bridge_initialize();

  for (i = 0; i < BRIDGE_NAME_TEST_NUM; i++) {
    rc = bridge_conf_create(&conf, bridges[i].name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

    rc = bridge_attr_duplicate(conf->modified_attr, &conf->current_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    if (bridges[i].dpid % 2 == 0) {
      rc = bridge_set_dpid(conf->modified_attr, bridges[i].dpid);
    } else {
      rc = bridge_set_dpid(conf->current_attr, bridges[i].dpid);
    }
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = bridge_conf_add(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Normal case of getter : case 01
  {
    // found
    for (i = 0; i < BRIDGE_NAME_TEST_NUM; i++) {
      rc = datastore_bridge_get_name_by_dpid(bridges[i].dpid, &name);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, rc,
                                "datastore_bridge_get_name_by_dpid error.");

      TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(bridges[i].name, name),
                                "bad name.");
      // free
      free(name);
      name = NULL;
    }
  }

  // Normal case of getter : case 02
  {
    // not found
    rc = datastore_bridge_get_name_by_dpid(dpid_not_found, &name);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_NOT_FOUND, rc,
                              "datastore_bridge_get_name_by_dpid error.");

    TEST_ASSERT_EQUAL_MESSAGE(NULL, name, "is not NULL.");
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_name_by_dpid(bridges[0].dpid, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}


#define BRIDGE_NAMES_TEST_NUM 3

void
test_bridge_conf_public_bridge_names(void) {
  lagopus_result_t rc;
  bridge_conf_t *conf = NULL;
  const char *bridge_names[BRIDGE_NAMES_TEST_NUM] =
      {"bridge_name01",
       "bridge_name02",
       "bridge_name_is_destroying"};
  datastore_name_info_t *names = NULL;
  struct datastore_name_entry *name = NULL;
  size_t expected_test_name_size_01 = 1;
  size_t expected_test_name_size_02 = 2;
  size_t i;

  bridge_initialize();

  for (i = 0; i < BRIDGE_NAMES_TEST_NUM; i++) {
    rc = bridge_conf_create(&conf, bridge_names[i]);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new bridge");

    if (i == 2) {
      conf->is_destroying = true;
    }
    rc = bridge_conf_add(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Normal case of getter : case 01
  {
    // get list of a bridge name.
    rc = datastore_bridge_get_names(bridge_names[0], &names);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, rc,
                              "datastore_bridge_get_names error.");

    TEST_ASSERT_EQUAL_MESSAGE(expected_test_name_size_01, names->size,
                              "bad size.");

    i = 0;
    TAILQ_FOREACH(name, &names->head, name_entries) {
      TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(bridge_names[i], name->str),
                                "bad name.");
      i++;
    }
    TEST_ASSERT_EQUAL_MESSAGE(expected_test_name_size_01, i,
                              "bad size.");

    // free
    datastore_names_destroy(names);
    names = NULL;
  }

  // Normal case of getter : case 02
  {
    // get list of all bridge name.
    rc = datastore_bridge_get_names(NULL, &names);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, rc,
                              "datastore_bridge_get_names error.");

    TEST_ASSERT_EQUAL_MESSAGE(expected_test_name_size_02, names->size,
                              "bad size.");

    i = 0;
    TAILQ_FOREACH(name, &names->head, name_entries) {
      TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(bridge_names[i], name->str),
                                "bad name.");
      i++;
    }
    TEST_ASSERT_EQUAL_MESSAGE(expected_test_name_size_02, i,
                              "bad size.");

    // free
    datastore_names_destroy(names);
    names = NULL;
  }

  // Normal case of getter : case 03
  {
    // get list of a bridge name (is destroying).
    rc = datastore_bridge_get_names(bridge_names[2], &names);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_NOT_FOUND, rc,
                              "datastore_bridge_get_names error.");

    TEST_ASSERT_EQUAL_MESSAGE(NULL, names, "is not NULL.");
  }

  // Normal case of getter : case 04
  {
    // get list of a bridge name (bad name).
    rc = datastore_bridge_get_names("bad_bridge_name", &names);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_NOT_FOUND, rc,
                              "datastore_bridge_get_names error.");

    TEST_ASSERT_EQUAL_MESSAGE(NULL, names, "is not NULL.");
  }

  // Abnormal case of getter
  {
    rc = datastore_bridge_get_names(bridge_names[0], NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_bridge_get_names(NULL, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  bridge_finalize();
}
