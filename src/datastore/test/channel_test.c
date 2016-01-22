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
#include "../channel.c"
#define CAST_UINT64(x) (uint64_t) x

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_channel_initialize_and_finalize(void) {
  channel_initialize();
  channel_finalize();
}

void
test_channel_conf_create_and_destroy(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";

  channel_initialize();

  // Normal case
  {
    rc = channel_conf_create(&conf, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

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
    rc = channel_conf_create(NULL, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_conf_create(&conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  channel_conf_destroy(conf);

  channel_finalize();
}

void
test_channel_conf_duplicate(void) {
  lagopus_result_t rc;
  const char *ns1 = "ns1";
  const char *ns2 = "ns2";
  const char *name = "channel";
  char *channel_fullname = NULL;
  bool result = false;
  channel_conf_t *src_conf = NULL;
  channel_conf_t *dst_conf = NULL;
  channel_conf_t *actual_conf = NULL;

  channel_initialize();

  // Normal case1(no namespace)
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &channel_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = channel_conf_create(&src_conf, channel_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new channel_action");
      free(channel_fullname);
      channel_fullname = NULL;
    }

    rc = channel_conf_duplicate(src_conf, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns1, name, &channel_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = channel_conf_create(&actual_conf, channel_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new channel_action");
      free(channel_fullname);
      channel_fullname = NULL;
    }

    result = channel_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    channel_conf_destroy(src_conf);
    src_conf = NULL;
    channel_conf_destroy(dst_conf);
    dst_conf = NULL;
    channel_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Normal case2
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &channel_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = channel_conf_create(&src_conf, channel_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new channel_action");
      free(channel_fullname);
      channel_fullname = NULL;
    }

    rc = channel_conf_duplicate(src_conf, &dst_conf, ns2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns2, name, &channel_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = channel_conf_create(&actual_conf, channel_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new channel_action");
      free(channel_fullname);
      channel_fullname = NULL;
    }

    result = channel_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    channel_conf_destroy(src_conf);
    src_conf = NULL;
    channel_conf_destroy(dst_conf);
    dst_conf = NULL;
    channel_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Abnormal case
  {
    rc = channel_conf_duplicate(NULL, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  channel_finalize();
}

void
test_channel_conf_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  channel_conf_t *conf1 = NULL;
  channel_conf_t *conf2 = NULL;
  channel_conf_t *conf3 = NULL;
  const char *fullname1 = "conf1";
  const char *fullname2 = "conf2";
  const char *fullname3 = "conf3";

  channel_initialize();

  rc = channel_conf_create(&conf1, fullname1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new channel");
  rc = channel_conf_add(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = channel_conf_create(&conf2, fullname2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new channel");
  rc = channel_conf_add(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = channel_conf_create(&conf3, fullname3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new channel");
  rc = channel_conf_add(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = channel_set_enabled(fullname3, true);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = channel_conf_equals(conf1, conf2);
    TEST_ASSERT_TRUE(result);

    result = channel_conf_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = channel_conf_equals(conf1, conf3);
    TEST_ASSERT_FALSE(result);

    result = channel_conf_equals(conf2, conf3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = channel_conf_equals(conf1, NULL);
    TEST_ASSERT_FALSE(result);

    result = channel_conf_equals(NULL, conf2);
    TEST_ASSERT_FALSE(result);
  }

  rc = channel_conf_delete(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = channel_conf_delete(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = channel_conf_delete(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  channel_finalize();
}

void
test_channel_conf_add(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";
  channel_conf_t *actual_conf = NULL;

  channel_initialize();

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  // Normal case
  {
    rc = channel_conf_add(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = channel_conf_add(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = channel_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  channel_finalize();
}

void
test_channel_conf_add_not_initialize(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  channel_conf_destroy(conf);

  channel_finalize();
}

void
test_channel_conf_delete(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";
  channel_conf_t *actual_conf = NULL;

  channel_initialize();

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    rc = channel_conf_delete(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = channel_conf_delete(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = channel_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, rc);

  channel_finalize();
}

void
test_channel_conf_delete_not_initialize(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_delete(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  channel_conf_destroy(conf);

  channel_finalize();
}

void
test_channel_conf_iterate(void) {
  TEST_IGNORE();
}

void
test_channel_conf_one_list_not_initialize(void) {
  TEST_IGNORE();
}

void
test_channel_conf_list(void) {
  lagopus_result_t rc;
  channel_conf_t *conf1 = NULL;
  const char *name1 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"channel_name1";
  channel_conf_t *conf2 = NULL;
  const char *name2 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"channel_name2";
  channel_conf_t *conf3 = NULL;
  const char *name3 = "namespace2"DATASTORE_NAMESPACE_DELIMITER"channel_name3";
  channel_conf_t *conf4 = NULL;
  const char *name4 = DATASTORE_NAMESPACE_DELIMITER"channel_name4";
  channel_conf_t *conf5 = NULL;
  const char *name5 = DATASTORE_NAMESPACE_DELIMITER"channel_name5";
  channel_conf_t **actual_list = NULL;
  size_t i;

  channel_initialize();

  // create conf
  {
    rc = channel_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1,
                                 "conf_create() will create new channel");

    rc = channel_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2,
                                 "conf_create() will create new channel");

    rc = channel_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3,
                                 "conf_create() will create new channel");

    rc = channel_conf_create(&conf4, name4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf4,
                                 "conf_create() will create new channel");

    rc = channel_conf_create(&conf5, name5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf5,
                                 "conf_create() will create new channel");
  }

  // add conf
  {
    rc = channel_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = channel_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = channel_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = channel_conf_add(conf4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = channel_conf_add(conf5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // all
  {
    rc = channel_conf_list(&actual_list, NULL);
    TEST_ASSERT_EQUAL(5, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"channel_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"channel_name2") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace2"DATASTORE_NAMESPACE_DELIMITER"channel_name3") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"channel_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"channel_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // no namespace
  {
    rc = channel_conf_list(&actual_list, "");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"channel_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"channel_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // only namespace
  {
    rc = channel_conf_list(&actual_list, "namespace1");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"channel_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"channel_name2") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // Abnormal case
  {
    rc = channel_conf_list(NULL, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  channel_finalize();
}

void
test_channel_conf_list_not_initialize(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";
  channel_conf_t ***list = NULL;

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_list(list, NULL);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  channel_conf_destroy(conf);

  channel_finalize();
}

void
test_channel_find(void) {
  lagopus_result_t rc;
  channel_conf_t *conf1 = NULL;
  const char *name1 = "channel_name1";
  channel_conf_t *conf2 = NULL;
  const char *name2 = "channel_name2";
  channel_conf_t *conf3 = NULL;
  const char *name3 = "channel_name3";
  channel_conf_t *actual_conf = NULL;

  channel_initialize();

  // create conf
  {
    rc = channel_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new channel");

    rc = channel_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new channel");

    rc = channel_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new channel");
  }

  // add conf
  {
    rc = channel_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = channel_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = channel_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Normal case
  {
    rc = channel_find(name1, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name1, actual_conf->name);

    rc = channel_find(name2, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name2, actual_conf->name);

    rc = channel_find(name3, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name3, actual_conf->name);
  }

  // Abnormal case
  {
    rc = channel_find(NULL, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_find(name1, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  channel_finalize();
}

void
test_channel_find_not_initialize(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";
  channel_conf_t *actual_conf = NULL;

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  channel_conf_destroy(conf);

  channel_finalize();
}

void
test_channel_attr_create_and_destroy(void) {
  lagopus_result_t rc;
  channel_attr_t *attr = NULL;
  char *actual_dst_addr = NULL;
  const char *expected_dst_addr = "127.0.0.1";
  uint16_t actual_dst_port = 0;
  const uint16_t expected_dst_port = 6633;
  char *actual_local_addr = NULL;
  const char *expected_local_addr = "0.0.0.0";
  uint16_t actual_local_port = 0;
  const uint16_t expected_local_port = 0;
  datastore_channel_protocol_t actual_protocol
    = DATASTORE_CHANNEL_PROTOCOL_UNKNOWN;
  const datastore_channel_protocol_t expected_protocol
    = DATASTORE_CHANNEL_PROTOCOL_UNKNOWN;

  channel_initialize();

  rc = channel_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new channel");

  // default value
  {
    // dst_addr
    rc = channel_get_dst_addr_str(attr, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_dst_addr, actual_dst_addr);

    // dst_port
    rc = channel_get_dst_port(attr, &actual_dst_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_dst_port, actual_dst_port);

    // local_addr
    rc = channel_get_local_addr_str(attr, &actual_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_local_addr, actual_local_addr);

    // local_port
    rc = channel_get_local_port(attr, &actual_local_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_local_port, actual_local_port);

    // protocol
    rc = channel_get_protocol(attr, &actual_protocol);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_protocol, actual_protocol);
  }

  channel_attr_destroy(attr);
  free((void *)actual_dst_addr);
  free((void *)actual_local_addr);

  channel_finalize();
}

void
test_channel_attr_duplicate(void) {
  lagopus_result_t rc;
  bool result = false;
  channel_attr_t *src_attr = NULL;
  channel_attr_t *dst_attr = NULL;

  channel_initialize();

  rc = channel_attr_create(&src_attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(src_attr,
                               "attr_create() will create new interface");

  // Normal case
  {
    rc = channel_attr_duplicate(src_attr, &dst_attr, "namespace");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    result = channel_attr_equals(src_attr, dst_attr);
    TEST_ASSERT_TRUE(result);
    channel_attr_destroy(dst_attr);
    dst_attr = NULL;

    rc = channel_attr_duplicate(src_attr, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    result = channel_attr_equals(src_attr, dst_attr);
    TEST_ASSERT_TRUE(result);
    channel_attr_destroy(dst_attr);
    dst_attr = NULL;
  }

  // Abnormal case
  {
    result = channel_attr_equals(src_attr, NULL);
    TEST_ASSERT_FALSE(result);

    rc = channel_attr_duplicate(src_attr, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    result = channel_attr_equals(NULL, dst_attr);
    TEST_ASSERT_FALSE(result);
    channel_attr_destroy(dst_attr);
    dst_attr = NULL;
  }

  channel_attr_destroy(src_attr);

  channel_finalize();
}

void
test_channel_attr_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  channel_attr_t *attr1 = NULL;
  channel_attr_t *attr2 = NULL;
  channel_attr_t *attr3 = NULL;
  const uint32_t dst_port = 0;

  channel_initialize();

  rc = channel_attr_create(&attr1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr1, "attr_create() will create new channel");

  rc = channel_attr_create(&attr2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr2, "attr_create() will create new channel");

  rc = channel_attr_create(&attr3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr3, "attr_create() will create new channel");
  rc = channel_set_dst_port(attr3, dst_port);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = channel_attr_equals(attr1, attr2);
    TEST_ASSERT_TRUE(result);

    result = channel_attr_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = channel_attr_equals(attr1, attr3);
    TEST_ASSERT_FALSE(result);

    result = channel_attr_equals(attr2, attr3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = channel_attr_equals(attr1, NULL);
    TEST_ASSERT_FALSE(result);

    result = channel_attr_equals(NULL, attr2);
    TEST_ASSERT_FALSE(result);
  }

  channel_attr_destroy(attr1);
  channel_attr_destroy(attr2);
  channel_attr_destroy(attr3);

  channel_finalize();
}

void
test_channel_get_attr(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  channel_attr_t *attr = NULL;
  const char *name = "channel_name";

  channel_initialize();

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = channel_get_attr(name, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = channel_get_attr(name, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(attr);
  }

  // Abnormal case of getter
  {
    rc = channel_get_attr(NULL, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_get_attr(NULL, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_get_attr(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_get_attr(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  channel_finalize();
}

void
test_channel_protocol_to_str(void) {
  lagopus_result_t rc;
  const char *actual_type_str;

  // Normal case
  {
    rc = channel_protocol_to_str(DATASTORE_CHANNEL_PROTOCOL_UNKNOWN,
                                 &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("unknown", actual_type_str);

    rc = channel_protocol_to_str(DATASTORE_CHANNEL_PROTOCOL_TCP, &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("tcp", actual_type_str);

    rc = channel_protocol_to_str(DATASTORE_CHANNEL_PROTOCOL_TLS, &actual_type_str);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("tls", actual_type_str);
  }

  // Abnormal case
  {
    rc = channel_protocol_to_str(DATASTORE_CHANNEL_PROTOCOL_TLS, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_channel_protocol_to_enum(void) {
  lagopus_result_t rc;
  datastore_channel_protocol_t actual_type;

  // Normal case
  {
    rc = channel_protocol_to_enum("unknown", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_CHANNEL_PROTOCOL_UNKNOWN, actual_type);

    rc = channel_protocol_to_enum("tcp", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_CHANNEL_PROTOCOL_TCP, actual_type);

    rc = channel_protocol_to_enum("tls", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(DATASTORE_CHANNEL_PROTOCOL_TLS, actual_type);

    rc = channel_protocol_to_enum("UNKNOWN", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_protocol_to_enum("TCP", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_protocol_to_enum("TLS", &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Abnormal case
  {
    rc = channel_protocol_to_enum(NULL, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_protocol_to_enum("tls", NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }
}

void
test_channel_conf_private_exists(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name1";
  const char *invalid_name = "invalid_name";

  channel_initialize();

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    TEST_ASSERT_TRUE(channel_exists(name) == true);
    TEST_ASSERT_TRUE(channel_exists(invalid_name) == false);
  }

  // Abnormal case
  {
    TEST_ASSERT_TRUE(channel_exists(NULL) == false);
  }

  channel_finalize();
}

void
test_channel_conf_private_used(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";
  bool actual_used = false;

  channel_initialize();

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = channel_set_used(name, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_channel_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = channel_set_used(NULL, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  channel_finalize();
}

void
test_channel_conf_public_used(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";
  bool actual_used = false;

  channel_initialize();

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_channel_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = datastore_channel_is_used(NULL, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  channel_finalize();
}

void
test_channel_conf_private_enabled(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";
  bool actual_enabled = false;

  channel_initialize();

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = channel_set_enabled(name, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_channel_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = channel_set_enabled(NULL, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  channel_finalize();
}

void
test_channel_conf_public_enabled(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";
  bool actual_enabled = false;

  channel_initialize();

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_channel_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = datastore_channel_is_enabled(NULL, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  channel_finalize();
}

void
test_channel_attr_private_dst_addr(void) {
  lagopus_result_t rc;
  channel_attr_t *attr = NULL;
  lagopus_ip_address_t *actual_dst_addr = NULL;
  const char *expected_dst_addr_str = "127.0.0.1";
  lagopus_ip_address_t *expected_dst_addr = NULL;

  const char *set_dst_addr_str = "192.168.0.1";
  lagopus_ip_address_t *set_dst_addr = NULL;
  lagopus_ip_address_t *actual_set_dst_addr = NULL;
  lagopus_ip_address_t *expected_set_dst_addr = NULL;

  channel_initialize();

  rc = channel_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new channel");

  // Normal case of getter
  {
    rc = channel_get_dst_addr(attr, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(expected_dst_addr_str, true,
                                   &expected_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_dst_addr,
                     actual_dst_addr));
  }

  // Abnormal case of getter
  {
    rc = channel_get_dst_addr(NULL, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = lagopus_ip_address_create(set_dst_addr_str, true, &set_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = channel_set_dst_addr(attr, set_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = channel_get_dst_addr(attr, &actual_set_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(set_dst_addr_str, true, &expected_set_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_set_dst_addr,
                     actual_set_dst_addr));
  }

  // Abnormal case of setter
  {
    rc = channel_set_dst_addr(NULL, actual_set_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_set_dst_addr(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  lagopus_ip_address_destroy(actual_dst_addr);
  lagopus_ip_address_destroy(expected_dst_addr);
  lagopus_ip_address_destroy(set_dst_addr);
  lagopus_ip_address_destroy(actual_set_dst_addr);
  lagopus_ip_address_destroy(expected_set_dst_addr);
  channel_attr_destroy(attr);
  channel_finalize();
}

void
test_channel_attr_public_dst_addr(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";
  lagopus_ip_address_t *actual_dst_addr = NULL;
  const char *expected_dst_addr_str = "127.0.0.1";
  lagopus_ip_address_t *expected_dst_addr = NULL;

  channel_initialize();

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_channel_get_dst_addr(name, true, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_channel_get_dst_addr(name, false, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(expected_dst_addr_str, true,
                                   &expected_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_dst_addr,
                     actual_dst_addr));
  }

  // Abnormal case of getter
  {
    rc = datastore_channel_get_dst_addr(NULL, true, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_dst_addr(NULL, false, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_dst_addr(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_dst_addr(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  lagopus_ip_address_destroy(actual_dst_addr);
  lagopus_ip_address_destroy(expected_dst_addr);
  channel_finalize();
}

void
test_channel_attr_private_dst_addr_str(void) {
  lagopus_result_t rc;
  channel_attr_t *attr = NULL;
  char *actual_dst_addr = NULL;
  const char *expected_dst_addr = "127.0.0.1";
  const char *set_dst_addr = "192.168.0.1";
  char *actual_set_dst_addr = NULL;
  const char *expected_set_dst_addr = set_dst_addr;
  const char *invalid_dst_addr = "256.256.256.256";

  channel_initialize();

  rc = channel_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new channel");

  // Normal case of getter
  {
    rc = channel_get_dst_addr_str(attr, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_dst_addr, actual_dst_addr);
  }

  // Abnormal case of getter
  {
    rc = channel_get_dst_addr_str(NULL, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = channel_set_dst_addr_str(attr, set_dst_addr, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = channel_get_dst_addr_str(attr, &actual_set_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_dst_addr, actual_set_dst_addr);
  }

  // Abnormal case of setter
  {
    rc = channel_set_dst_addr_str(NULL, actual_set_dst_addr, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_set_dst_addr_str(attr, NULL, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_set_dst_addr_str(attr, invalid_dst_addr, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ADDR_RESOLVER_FAILURE, rc);
  }

  channel_attr_destroy(attr);
  free((void *)actual_dst_addr);
  free((void *)actual_set_dst_addr);

  channel_finalize();
}

void
test_channel_attr_public_dst_addr_str(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";
  char *actual_dst_addr = NULL;
  const char *expected_dst_addr = "127.0.0.1";

  channel_initialize();

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_channel_get_dst_addr_str(name, true, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_channel_get_dst_addr_str(name, false, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_dst_addr, actual_dst_addr);
  }

  // Abnormal case of getter
  {
    rc = datastore_channel_get_dst_addr_str(NULL, true, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_dst_addr_str(NULL, false, &actual_dst_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_dst_addr_str(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_dst_addr_str(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  free((void *)actual_dst_addr);
  channel_finalize();
}

void
test_channel_attr_private_dst_port(void) {
  lagopus_result_t rc;
  channel_attr_t *attr = NULL;
  uint16_t actual_dst_port = 0;
  const uint16_t expected_dst_port = 6633;
  const uint64_t set_dst_port1 = CAST_UINT64(MAXIMUM_DST_PORT);
  const uint64_t set_dst_port2 = CAST_UINT64(MINIMUM_DST_PORT);
  const uint64_t set_dst_port3 = CAST_UINT64(MAXIMUM_DST_PORT + 1);
  const int set_dst_port4 = -1;
  uint16_t actual_set_dst_port1 = 0;
  uint16_t actual_set_dst_port2 = 0;
  uint16_t actual_set_dst_port3 = 0;
  uint16_t actual_set_dst_port4 = 0;
  const uint16_t expected_set_dst_port1 = MAXIMUM_DST_PORT;
  const uint16_t expected_set_dst_port2 = MINIMUM_DST_PORT;
  const uint16_t expected_set_dst_port3 = MINIMUM_DST_PORT;
  const uint16_t expected_set_dst_port4 = MINIMUM_DST_PORT;

  channel_initialize();

  rc = channel_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new channel");

  // Normal case of getter
  {
    rc = channel_get_dst_port(attr, &actual_dst_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_dst_port, actual_dst_port);
  }

  // Abnormal case of getter
  {
    rc = channel_get_dst_port(NULL, &actual_dst_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = channel_set_dst_port(attr, set_dst_port1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = channel_get_dst_port(attr, &actual_set_dst_port1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_dst_port1, actual_set_dst_port1);

    rc = channel_set_dst_port(attr, set_dst_port2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = channel_get_dst_port(attr, &actual_set_dst_port2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_dst_port2, actual_set_dst_port2);
  }

  // Abnormal case of setter
  {
    rc = channel_set_dst_port(NULL, actual_set_dst_port1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_set_dst_port(attr, set_dst_port3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = channel_get_dst_port(attr, &actual_set_dst_port3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_dst_port3, actual_set_dst_port3);

    rc = channel_set_dst_port(attr, CAST_UINT64(set_dst_port4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = channel_get_dst_port(attr, &actual_set_dst_port4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_dst_port4, actual_set_dst_port4);
  }

  channel_attr_destroy(attr);

  channel_finalize();
}

void
test_channel_attr_public_dst_port(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";
  uint16_t actual_dst_port = 0;
  const uint16_t expected_dst_port = 6633;

  channel_initialize();

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_channel_get_dst_port(name, true, &actual_dst_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_channel_get_dst_port(name, false, &actual_dst_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_dst_port, actual_dst_port);
  }

  // Abnormal case of getter
  {
    rc = datastore_channel_get_dst_port(NULL, true, &actual_dst_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_dst_port(NULL, false, &actual_dst_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_dst_port(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_dst_port(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  channel_finalize();
}

void
test_channel_attr_private_local_addr(void) {
  lagopus_result_t rc;
  channel_attr_t *attr = NULL;
  lagopus_ip_address_t *actual_local_addr = NULL;
  const char *expected_local_addr_str = "0.0.0.0";
  lagopus_ip_address_t *expected_local_addr = NULL;

  const char *set_local_addr_str = "192.168.0.1";
  lagopus_ip_address_t *set_local_addr = NULL;
  lagopus_ip_address_t *actual_set_local_addr = NULL;
  lagopus_ip_address_t *expected_set_local_addr = NULL;

  channel_initialize();

  rc = channel_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new channel");

  // Normal case of getter
  {
    rc = channel_get_local_addr(attr, &actual_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(expected_local_addr_str, true,
                                   &expected_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_local_addr,
                     actual_local_addr));
  }

  // Abnormal case of getter
  {
    rc = channel_get_local_addr(NULL, &actual_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = lagopus_ip_address_create(set_local_addr_str, true, &set_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = channel_set_local_addr(attr, set_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = channel_get_local_addr(attr, &actual_set_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(set_local_addr_str, true,
                                   &expected_set_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_set_local_addr,
                     actual_set_local_addr));
  }

  // Abnormal case of setter
  {
    rc = channel_set_local_addr(NULL, actual_set_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_set_local_addr(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  lagopus_ip_address_destroy(actual_local_addr);
  lagopus_ip_address_destroy(expected_local_addr);
  lagopus_ip_address_destroy(set_local_addr);
  lagopus_ip_address_destroy(actual_set_local_addr);
  lagopus_ip_address_destroy(expected_set_local_addr);
  channel_attr_destroy(attr);
  channel_finalize();
}

void
test_channel_attr_public_local_addr(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";
  lagopus_ip_address_t *actual_local_addr = NULL;
  const char *expected_local_addr_str = "0.0.0.0";
  lagopus_ip_address_t *expected_local_addr = NULL;

  channel_initialize();

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_channel_get_local_addr(name, true, &actual_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_channel_get_local_addr(name, false, &actual_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = lagopus_ip_address_create(expected_local_addr_str, true,
                                   &expected_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_TRUE(lagopus_ip_address_equals(expected_local_addr,
                     actual_local_addr));
  }

  // Abnormal case of getter
  {
    rc = datastore_channel_get_local_addr(NULL, true, &actual_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_local_addr(NULL, false, &actual_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_local_addr(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_local_addr(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  lagopus_ip_address_destroy(actual_local_addr);
  lagopus_ip_address_destroy(expected_local_addr);
  channel_finalize();
}

void
test_channel_attr_private_local_addr_str(void) {
  lagopus_result_t rc;
  channel_attr_t *attr = NULL;
  char *actual_local_addr = NULL;
  const char *expected_local_addr = "0.0.0.0";
  const char *set_local_addr = "192.168.0.1";
  char *actual_set_local_addr = NULL;
  const char *expected_set_local_addr = set_local_addr;
  const char *invalid_local_addr = "256.256.256.256";

  channel_initialize();

  rc = channel_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new channel");

  // Normal case of getter
  {
    rc = channel_get_local_addr_str(attr, &actual_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_local_addr, actual_local_addr);
  }

  // Abnormal case of getter
  {
    rc = channel_get_local_addr_str(NULL, &actual_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = channel_set_local_addr_str(attr, set_local_addr, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = channel_get_local_addr_str(attr, &actual_set_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_local_addr, actual_set_local_addr);
  }

  // Abnormal case of setter
  {
    rc = channel_set_local_addr_str(NULL, actual_set_local_addr, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_set_local_addr_str(attr, NULL, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_set_local_addr_str(attr, invalid_local_addr, true);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_ADDR_RESOLVER_FAILURE, rc);
  }

  channel_attr_destroy(attr);
  free((void *)actual_local_addr);
  free((void *)actual_set_local_addr);

  channel_finalize();
}

void
test_channel_attr_public_local_addr_str(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";
  char *actual_local_addr = NULL;
  const char *expected_local_addr = "0.0.0.0";

  channel_initialize();

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_channel_get_local_addr_str(name, true, &actual_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_channel_get_local_addr_str(name, false, &actual_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_local_addr, actual_local_addr);
  }

  // Abnormal case of getter
  {
    rc = datastore_channel_get_local_addr_str(NULL, true, &actual_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_local_addr_str(NULL, false, &actual_local_addr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_local_addr_str(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_local_addr_str(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  free((void *)actual_local_addr);
  channel_finalize();
}

void
test_channel_attr_private_local_port(void) {
  lagopus_result_t rc;
  channel_attr_t *attr = NULL;
  uint16_t actual_local_port = 0;
  const uint16_t expected_local_port = actual_local_port;
  const uint64_t set_local_port1 = CAST_UINT64(MAXIMUM_LOCAL_PORT);
  const uint64_t set_local_port2 = CAST_UINT64(MINIMUM_LOCAL_PORT);
  const uint64_t set_local_port3 = CAST_UINT64(MAXIMUM_LOCAL_PORT + 1);
  const int set_local_port4 = -1;
  uint16_t actual_set_local_port1 = 0;
  uint16_t actual_set_local_port2 = 0;
  uint16_t actual_set_local_port3 = 0;
  uint16_t actual_set_local_port4 = 0;
  const uint16_t expected_set_local_port1 = MAXIMUM_LOCAL_PORT;
  const uint16_t expected_set_local_port2 = MINIMUM_LOCAL_PORT;
  const uint16_t expected_set_local_port3 = MINIMUM_LOCAL_PORT;
  const uint16_t expected_set_local_port4 = MINIMUM_LOCAL_PORT;

  channel_initialize();

  rc = channel_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new channel");

  // Normal case of getter
  {
    rc = channel_get_local_port(attr, &actual_local_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_local_port, actual_local_port);
  }

  // Abnormal case of getter
  {
    rc = channel_get_local_port(NULL, &actual_local_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = channel_set_local_port(attr, set_local_port1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = channel_get_local_port(attr, &actual_set_local_port1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_local_port1, actual_set_local_port1);

    rc = channel_set_local_port(attr, set_local_port2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = channel_get_local_port(attr, &actual_set_local_port2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_local_port2, actual_set_local_port2);
  }

  // Abnormal case of setter
  {
    rc = channel_set_local_port(NULL, actual_set_local_port1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = channel_set_local_port(attr, set_local_port3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = channel_get_local_port(attr, &actual_set_local_port3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_local_port3, actual_set_local_port3);

    rc = channel_set_local_port(attr, CAST_UINT64(set_local_port4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = channel_get_local_port(attr, &actual_set_local_port4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_local_port4, actual_set_local_port4);
  }

  channel_attr_destroy(attr);

  channel_finalize();
}

void
test_channel_attr_public_local_port(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";
  uint16_t actual_local_port = 0;
  const uint16_t expected_local_port = actual_local_port;

  channel_initialize();

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new channel");

  rc = channel_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_channel_get_local_port(name, true, &actual_local_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_channel_get_local_port(name, false, &actual_local_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_local_port, actual_local_port);
  }

  // Abnormal case of getter
  {
    rc = datastore_channel_get_local_port(NULL, true, &actual_local_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_local_port(NULL, false, &actual_local_port);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_local_port(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_local_port(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  channel_finalize();
}

void
test_channel_attr_private_protocol(void) {
  lagopus_result_t rc;
  channel_attr_t *attr = NULL;
  datastore_channel_protocol_t actual_protocol
    = DATASTORE_CHANNEL_PROTOCOL_UNKNOWN;
  const datastore_channel_protocol_t expected_protocol = actual_protocol;
  const datastore_channel_protocol_t set_protocol
    = DATASTORE_CHANNEL_PROTOCOL_UNKNOWN;
  datastore_channel_protocol_t actual_set_protocol
    = DATASTORE_CHANNEL_PROTOCOL_UNKNOWN;
  const datastore_channel_protocol_t expected_set_protocol = actual_set_protocol;
  //const datastore_channel_protocol_t invalid_protocol
  //      = DATASTORE_CHANNEL_PROTOCOL_UNKNOWN;

  channel_initialize();

  rc = channel_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new channel");

  // Normal case of getter
  {
    rc = channel_get_protocol(attr, &actual_protocol);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_protocol, actual_protocol);
  }

  // Abnormal case of getter
  {
    rc = channel_get_protocol(NULL, &actual_protocol);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = channel_set_protocol(attr, set_protocol);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = channel_get_protocol(attr, &actual_set_protocol);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_protocol, actual_set_protocol);
  }

  // Abnormal case of setter
  {
    rc = channel_set_protocol(NULL, actual_set_protocol);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    // rc = channel_set_protocol(attr, invalid_protocol);
    // TEST_ASSERT_EQUAL(LAGOPUS_RESULT_POSIX_API_ERROR, rc);
  }

  channel_attr_destroy(attr);

  channel_finalize();
}

void
test_channel_attr_public_protocol(void) {
  lagopus_result_t rc;
  channel_conf_t *conf = NULL;
  const char *name = "channel_name";
  datastore_channel_protocol_t actual_protocol
    = DATASTORE_CHANNEL_PROTOCOL_UNKNOWN;
  const datastore_channel_protocol_t expected_protocol = actual_protocol;

  channel_initialize();

  rc = channel_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "attr_create() will create new channel");

  rc = channel_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_channel_get_protocol(name, true, &actual_protocol);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_channel_get_protocol(name, false, &actual_protocol);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_protocol, actual_protocol);
  }

  // Abnormal case of getter
  {
    rc = datastore_channel_get_protocol(NULL, true, &actual_protocol);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_protocol(NULL, false, &actual_protocol);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_protocol(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_channel_get_protocol(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  channel_finalize();
}
