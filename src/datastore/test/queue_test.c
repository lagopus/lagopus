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
#include "../queue.c"
#define CAST_UINT64(x) (uint64_t) x

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_queue_initialize_and_finalize(void) {
  queue_initialize();
  queue_finalize();
}

void
test_queue_conf_create_and_destroy(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";

  queue_initialize();

  // Normal case
  {
    rc = queue_conf_create(&conf, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

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
    rc = queue_conf_create(NULL, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_conf_create(&conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_conf_destroy(conf);

  queue_finalize();
}

void
test_queue_conf_duplicate(void) {
  lagopus_result_t rc;
  const char *ns1 = "ns1";
  const char *ns2 = "ns2";
  const char *name = "queue";
  char *queue_fullname = NULL;
  bool result = false;
  queue_conf_t *src_conf = NULL;
  queue_conf_t *dst_conf = NULL;
  queue_conf_t *actual_conf = NULL;

  queue_initialize();

  // Normal case1(no namespace)
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &queue_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = queue_conf_create(&src_conf, queue_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new policer_action");
      free(queue_fullname);
      queue_fullname = NULL;
    }

    rc = queue_conf_duplicate(src_conf, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns1, name, &queue_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = queue_conf_create(&actual_conf, queue_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new policer_action");
      free(queue_fullname);
      queue_fullname = NULL;
    }

    result = queue_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    queue_conf_destroy(src_conf);
    src_conf = NULL;
    queue_conf_destroy(dst_conf);
    dst_conf = NULL;
    queue_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Normal case2
  {
    // create src conf
    {
      rc = ns_create_fullname(ns1, name, &queue_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = queue_conf_create(&src_conf, queue_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new policer_action");
      free(queue_fullname);
      queue_fullname = NULL;
    }

    rc = queue_conf_duplicate(src_conf, &dst_conf, ns2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    // create actual conf
    {
      rc = ns_create_fullname(ns2, name, &queue_fullname);
      TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK,
                                rc, "cmd_ns_get_fullname error.");
      rc = queue_conf_create(&actual_conf, queue_fullname);
      TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
      TEST_ASSERT_NOT_NULL_MESSAGE(src_conf,
                                   "conf_create() will create new policer_action");
      free(queue_fullname);
      queue_fullname = NULL;
    }

    result = queue_conf_equals(dst_conf, actual_conf);
    TEST_ASSERT_TRUE(result);

    queue_conf_destroy(src_conf);
    src_conf = NULL;
    queue_conf_destroy(dst_conf);
    dst_conf = NULL;
    queue_conf_destroy(actual_conf);
    actual_conf = NULL;
  }

  // Abnormal case
  {
    rc = queue_conf_duplicate(NULL, &dst_conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_conf_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  queue_conf_t *conf1 = NULL;
  queue_conf_t *conf2 = NULL;
  queue_conf_t *conf3 = NULL;
  const char *fullname1 = "conf1";
  const char *fullname2 = "conf2";
  const char *fullname3 = "conf3";

  queue_initialize();

  rc = queue_conf_create(&conf1, fullname1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new queue");
  rc = queue_conf_add(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = queue_conf_create(&conf2, fullname2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new queue");
  rc = queue_conf_add(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  rc = queue_conf_create(&conf3, fullname3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new queue");
  rc = queue_conf_add(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = queue_set_enabled(fullname3, true);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = queue_conf_equals(conf1, conf2);
    TEST_ASSERT_TRUE(result);

    result = queue_conf_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = queue_conf_equals(conf1, conf3);
    TEST_ASSERT_FALSE(result);

    result = queue_conf_equals(conf2, conf3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = queue_conf_equals(conf1, NULL);
    TEST_ASSERT_FALSE(result);

    result = queue_conf_equals(NULL, conf2);
    TEST_ASSERT_FALSE(result);
  }

  rc = queue_conf_delete(conf1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = queue_conf_delete(conf2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  rc = queue_conf_delete(conf3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  queue_finalize();
}

void
test_queue_conf_add(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  queue_conf_t *actual_conf = NULL;

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  // Normal case
  {
    rc = queue_conf_add(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = queue_conf_add(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = queue_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  queue_finalize();
}

void
test_queue_conf_add_not_initialize(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  queue_conf_destroy(conf);

  queue_finalize();
}

void
test_queue_conf_delete(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  queue_conf_t *actual_conf = NULL;

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    rc = queue_conf_delete(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = queue_conf_delete(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = queue_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, rc);

  queue_finalize();
}

void
test_queue_conf_delete_not_initialize(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_delete(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  queue_conf_destroy(conf);

  queue_finalize();
}

void
test_queue_conf_iterate(void) {
  TEST_IGNORE();
}

void
test_queue_conf_list(void) {
  lagopus_result_t rc;
  queue_conf_t *conf1 = NULL;
  const char *name1 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"queue_name1";
  queue_conf_t *conf2 = NULL;
  const char *name2 = "namespace1"DATASTORE_NAMESPACE_DELIMITER"queue_name2";
  queue_conf_t *conf3 = NULL;
  const char *name3 = "namespace2"DATASTORE_NAMESPACE_DELIMITER"queue_name3";
  queue_conf_t *conf4 = NULL;
  const char *name4 = DATASTORE_NAMESPACE_DELIMITER"queue_name4";
  queue_conf_t *conf5 = NULL;
  const char *name5 = DATASTORE_NAMESPACE_DELIMITER"queue_name5";
  queue_conf_t **actual_list = NULL;
  size_t i;

  queue_initialize();

  // create conf
  {
    rc = queue_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new queue");

    rc = queue_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new queue");

    rc = queue_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new queue");

    rc = queue_conf_create(&conf4, name4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf4, "conf_create() will create new queue");

    rc = queue_conf_create(&conf5, name5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf5, "conf_create() will create new queue");
  }

  // add conf
  {
    rc = queue_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = queue_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = queue_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = queue_conf_add(conf4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = queue_conf_add(conf5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // all
  {
    rc = queue_conf_list(&actual_list, NULL);
    TEST_ASSERT_EQUAL(5, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"queue_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"queue_name2") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace2"DATASTORE_NAMESPACE_DELIMITER"queue_name3") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"queue_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"queue_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // no namespace
  {
    rc = queue_conf_list(&actual_list, "");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"queue_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     DATASTORE_NAMESPACE_DELIMITER"queue_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // only namespace
  {
    rc = queue_conf_list(&actual_list, "namespace1");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"queue_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"DATASTORE_NAMESPACE_DELIMITER"queue_name2") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // Abnormal case
  {
    rc = queue_conf_list(NULL, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_conf_list_not_initialize(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  queue_conf_t ***list = NULL;

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_list(list, NULL);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  queue_conf_destroy(conf);

  queue_finalize();
}

void
test_queue_conf_one_list(void) {
  TEST_IGNORE();
}

void
test_queue_conf_one_list_not_initialize(void) {
  TEST_IGNORE();
}

void
test_queue_find(void) {
  lagopus_result_t rc;
  queue_conf_t *conf1 = NULL;
  const char *name1 = "queue_name1";
  queue_conf_t *conf2 = NULL;
  const char *name2 = "queue_name2";
  queue_conf_t *conf3 = NULL;
  const char *name3 = "queue_name3";
  queue_conf_t *actual_conf = NULL;

  queue_initialize();

  // create conf
  {
    rc = queue_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new queue");

    rc = queue_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new queue");

    rc = queue_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new queue");
  }

  // add conf
  {
    rc = queue_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = queue_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = queue_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Normal case
  {
    rc = queue_find(name1, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name1, actual_conf->name);

    rc = queue_find(name2, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name2, actual_conf->name);

    rc = queue_find(name3, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name3, actual_conf->name);

  }

  // Abnormal case
  {
    rc = queue_find(NULL, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_find(name1, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_find_not_initialize(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  queue_conf_t *actual_conf = NULL;

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  queue_conf_destroy(conf);

  queue_finalize();
}

void
test_queue_attr_create_and_destroy(void) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;
  uint32_t actual_id = 0;
  const uint32_t expected_id = actual_id;
  uint16_t actual_priority = 0;
  const uint16_t expected_priority = actual_priority;
  datastore_queue_type_t actual_type = 0;
  const datastore_queue_type_t expected_type =
    DATASTORE_QUEUE_TYPE_UNKNOWN;
  datastore_queue_color_t actual_color = 0;
  const datastore_queue_color_t expected_color =
    DATASTORE_QUEUE_COLOR_UNKNOWN;
  uint64_t actual_committed_burst_size = 0;
  const uint64_t expected_committed_burst_size =
    DEFAULT_COMMITTED_BURST_SIZE;
  uint64_t actual_committed_information_rate = 0;
  const uint64_t expected_committed_information_rate =
    DEFAULT_COMMITTED_INFORMATION_RATE;
  uint64_t actual_excess_burst_size = 0;
  const uint64_t expected_excess_burst_size =
    DEFAULT_EXCESS_BURST_SIZE;
  uint64_t actual_peak_burst_size = 0;
  const uint64_t expected_peak_burst_size =
    DEFAULT_PEAK_BURST_SIZE;
  uint64_t actual_peak_information_rate = 0;
  const uint64_t expected_peak_information_rate =
    DEFAULT_PEAK_INFORMATION_RATE;

  queue_initialize();

  rc = queue_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new queue");

  // default value
  {
    // id
    rc = queue_get_id(attr, &actual_id);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_id, actual_id);

    // priority
    rc = queue_get_priority(attr, &actual_priority);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_priority, actual_priority);

    // type
    rc = queue_get_type(attr, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_type, actual_type);

    // color
    rc = queue_get_color(attr, &actual_color);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_color, actual_color);

    // committed_burst_size
    rc = queue_get_committed_burst_size(attr,
                                        &actual_committed_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_committed_burst_size,
                             actual_committed_burst_size);

    // committed_information_rate
    rc = queue_get_committed_information_rate(
           attr,
           &actual_committed_information_rate);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_committed_information_rate,
                             actual_committed_information_rate);

    // excess_burst_size
    rc = queue_get_excess_burst_size(attr,
                                     &actual_excess_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_excess_burst_size,
                             actual_excess_burst_size);

    // peak_burst_size
    rc = queue_get_peak_burst_size(attr,
                                   &actual_peak_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_peak_burst_size,
                             actual_peak_burst_size);

    // peak_information_rate
    rc = queue_get_peak_information_rate(attr,
                                         &actual_peak_information_rate);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_peak_information_rate,
                             actual_peak_information_rate);
  }

  queue_attr_destroy(attr);

  queue_finalize();
}

void
test_queue_attr_duplicate(void) {
  lagopus_result_t rc;
  bool result = false;
  queue_attr_t *src_attr = NULL;
  queue_attr_t *dst_attr = NULL;

  queue_initialize();

  rc = queue_attr_create(&src_attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(src_attr, "attr_create() will create new queue");

  // Normal case
  {
    rc = queue_attr_duplicate(src_attr, &dst_attr, "namespace");
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    result = queue_attr_equals(src_attr, dst_attr);
    TEST_ASSERT_TRUE(result);
    queue_attr_destroy(dst_attr);
    dst_attr = NULL;

    rc = queue_attr_duplicate(src_attr, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    result = queue_attr_equals(src_attr, dst_attr);
    TEST_ASSERT_TRUE(result);
    queue_attr_destroy(dst_attr);
    dst_attr = NULL;
  }

  // Abnormal case
  {
    result = queue_attr_equals(src_attr, NULL);
    TEST_ASSERT_FALSE(result);

    rc = queue_attr_duplicate(src_attr, &dst_attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    result = queue_attr_equals(NULL, dst_attr);
    TEST_ASSERT_FALSE(result);
    queue_attr_destroy(dst_attr);
    dst_attr = NULL;
  }

  queue_attr_destroy(src_attr);

  queue_finalize();
}

void
test_queue_attr_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  queue_attr_t *attr1 = NULL;
  queue_attr_t *attr2 = NULL;
  queue_attr_t *attr3 = NULL;

  queue_initialize();

  rc = queue_attr_create(&attr1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr1, "attr_create() will create new queue");

  rc = queue_attr_create(&attr2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr2, "attr_create() will create new queue");

  rc = queue_attr_create(&attr3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr3, "attr_create() will create new queue");
  rc = queue_set_id(attr3, 100);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = queue_attr_equals(attr1, attr2);
    TEST_ASSERT_TRUE(result);

    result = queue_attr_equals(NULL, NULL);
    TEST_ASSERT_TRUE(result);

    result = queue_attr_equals(attr1, attr3);
    TEST_ASSERT_FALSE(result);

    result = queue_attr_equals(attr2, attr3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = queue_attr_equals(attr1, NULL);
    TEST_ASSERT_FALSE(result);

    result = queue_attr_equals(NULL, attr2);
    TEST_ASSERT_FALSE(result);
  }

  queue_attr_destroy(attr1);
  queue_attr_destroy(attr2);
  queue_attr_destroy(attr3);

  queue_finalize();
}

void
test_queue_get_attr(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  queue_attr_t *attr = NULL;
  const char *name = "queue_name";

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = queue_get_attr(name, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = queue_get_attr(name, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(attr);
  }

  // Abnormal case of getter
  {
    rc = queue_get_attr(NULL, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_get_attr(NULL, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_get_attr(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_get_attr(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_conf_private_exists(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name1";
  const char *invalid_name = "invalid_name";

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    TEST_ASSERT_TRUE(queue_exists(name) == true);
    TEST_ASSERT_TRUE(queue_exists(invalid_name) == false);
  }

  // Abnormal case
  {
    TEST_ASSERT_TRUE(queue_exists(NULL) == false);
  }

  queue_finalize();
}

void
test_queue_conf_private_used(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  bool actual_used = false;

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = queue_set_used(name, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_queue_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = queue_set_used(NULL, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_conf_public_used(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  bool actual_used = false;

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_queue_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = datastore_queue_is_used(NULL, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_conf_private_enabled(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  bool actual_enabled = false;

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = queue_set_enabled(name, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_queue_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = queue_set_enabled(NULL, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_conf_public_enabled(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  bool actual_enabled = false;

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_queue_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = datastore_queue_is_enabled(NULL, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_attr_private_id(void) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;
  uint32_t actual_id = 0;
  const uint32_t expected_id = actual_id;
  const uint64_t set_id1 = CAST_UINT64(MAXIMUM_ID);
  const uint64_t set_id2 = CAST_UINT64(MINIMUM_ID);
  const uint64_t set_id3 = CAST_UINT64(MAXIMUM_ID + 1);
  const int set_id4 = -1;
  uint32_t actual_set_id1 = 0;
  uint32_t actual_set_id2 = 0;
  uint32_t actual_set_id3 = 0;
  uint32_t actual_set_id4 = 0;
  const uint32_t expected_set_id1 = MAXIMUM_ID;
  const uint32_t expected_set_id2 = MINIMUM_ID;
  const uint32_t expected_set_id3 = MINIMUM_ID;
  const uint32_t expected_set_id4 = MINIMUM_ID;

  queue_initialize();

  rc = queue_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new queue");

  // Normal case of getter
  {
    rc = queue_get_id(attr, &actual_id);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_id, actual_id);
  }

  // Abnormal case of getter
  {
    rc = queue_get_id(NULL, &actual_id);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_get_id(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = queue_set_id(attr, set_id1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_id(attr, &actual_set_id1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_id1, actual_set_id1);

    rc = queue_set_id(attr, set_id2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_id(attr, &actual_set_id2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_id2, actual_set_id2);
  }

  // Abnormal case of setter
  {
    rc = queue_set_id(NULL, set_id1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_set_id(attr, CAST_UINT64(set_id3));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = queue_get_id(attr, &actual_set_id3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_id3, actual_set_id3);

    rc = queue_set_id(attr, CAST_UINT64(set_id4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = queue_get_id(attr, &actual_set_id4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_id4, actual_set_id4);
  }

  queue_attr_destroy(attr);

  queue_finalize();
}

void
test_queue_attr_public_id(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  uint32_t actual_id = 0;
  const uint32_t expected_id = actual_id;

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_queue_get_id(name, true, &actual_id);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_queue_get_id(name, false, &actual_id);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_id, actual_id);
  }

  // Abnormal case of getter
  {
    rc = datastore_queue_get_id(NULL, true, &actual_id);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_id(NULL, false, &actual_id);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_id(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_id(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_attr_private_priority(void) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;
  uint16_t actual_priority = 0;
  const uint16_t expected_priority = actual_priority;
  const uint64_t set_priority1 = CAST_UINT64(MAXIMUM_PRIORITY);
  const uint64_t set_priority2 = CAST_UINT64(MINIMUM_PRIORITY);
  const uint64_t set_priority3 = CAST_UINT64(MAXIMUM_PRIORITY + 1);
  const int set_priority4 = -1;
  uint16_t actual_set_priority1 = 0;
  uint16_t actual_set_priority2 = 0;
  uint16_t actual_set_priority3 = 0;
  uint16_t actual_set_priority4 = 0;
  const uint16_t expected_set_priority1 = MAXIMUM_PRIORITY;
  const uint16_t expected_set_priority2 = MINIMUM_PRIORITY;
  const uint16_t expected_set_priority3 = MINIMUM_PRIORITY;
  const uint16_t expected_set_priority4 = MINIMUM_PRIORITY;

  queue_initialize();

  rc = queue_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new queue");

  // Normal case of getter
  {
    rc = queue_get_priority(attr, &actual_priority);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_priority, actual_priority);
  }

  // Abnormal case of getter
  {
    rc = queue_get_priority(NULL, &actual_priority);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_get_priority(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = queue_set_priority(attr, set_priority1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_priority(attr, &actual_set_priority1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_priority1, actual_set_priority1);

    rc = queue_set_priority(attr, set_priority2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_priority(attr, &actual_set_priority2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_priority2, actual_set_priority2);
  }

  // Abnormal case of setter
  {
    rc = queue_set_priority(NULL, set_priority1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_set_priority(attr, CAST_UINT64(set_priority3));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = queue_get_priority(attr, &actual_set_priority3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_priority3, actual_set_priority3);

    rc = queue_set_priority(attr, CAST_UINT64(set_priority4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = queue_get_priority(attr, &actual_set_priority4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_set_priority4, actual_set_priority4);
  }

  queue_attr_destroy(attr);

  queue_finalize();
}

void
test_queue_attr_public_priority(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  uint16_t actual_priority = 0;
  const uint16_t expected_priority = actual_priority;

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_queue_get_priority(name, true, &actual_priority);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_queue_get_priority(name, false, &actual_priority);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(expected_priority, actual_priority);
  }

  // Abnormal case of getter
  {
    rc = datastore_queue_get_priority(NULL, true, &actual_priority);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_priority(NULL, false, &actual_priority);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_priority(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_priority(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_attr_private_type(void) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;
  datastore_queue_type_t actual_type = 0;
  datastore_queue_type_t expected_type = DATASTORE_QUEUE_TYPE_UNKNOWN;
  datastore_queue_type_t set_type = DATASTORE_QUEUE_TYPE_SINGLE_RATE;
  datastore_queue_type_t actual_set_type = 0;
  datastore_queue_type_t expected_set_type = set_type;

  queue_initialize();

  rc = queue_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new queue");

  // Normal case of getter
  {
    rc = queue_get_type(attr, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_type, actual_type);
  }

  // Abnormal case of getter
  {
    rc = queue_get_type(NULL, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_get_type(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = queue_set_type(attr, set_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_type(attr, &actual_set_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_type, actual_set_type);
  }

  // Abnormal case of setter
  {
    rc = queue_set_type(NULL, actual_set_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_attr_destroy(attr);

  queue_finalize();
}

void
test_queue_attr_public_type(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  datastore_queue_type_t actual_type = 0;
  datastore_queue_type_t expected_type = DATASTORE_QUEUE_TYPE_UNKNOWN;

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_queue_get_type(name, true, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_queue_get_type(name, false, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_type, actual_type);
  }

  // Abnormal case of getter
  {
    rc = datastore_queue_get_type(NULL, true, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_type(NULL, false, &actual_type);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_type(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_type(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_attr_private_color(void) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;
  datastore_queue_color_t actual_color = 0;
  datastore_queue_color_t expected_color = DATASTORE_QUEUE_COLOR_UNKNOWN;
  datastore_queue_color_t set_color = DATASTORE_QUEUE_COLOR_AWARE;
  datastore_queue_color_t actual_set_color = 0;
  datastore_queue_color_t expected_set_color = set_color;

  queue_initialize();

  rc = queue_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new queue");

  // Normal case of getter
  {
    rc = queue_get_color(attr, &actual_color);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_color, actual_color);
  }

  // Abnormal case of getter
  {
    rc = queue_get_color(NULL, &actual_color);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_get_color(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = queue_set_color(attr, set_color);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_color(attr, &actual_set_color);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_set_color, actual_set_color);
  }

  // Abnormal case of setter
  {
    rc = queue_set_color(NULL, actual_set_color);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_attr_destroy(attr);

  queue_finalize();
}

void
test_queue_attr_public_color(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  datastore_queue_color_t actual_color = 0;
  datastore_queue_color_t expected_color = DATASTORE_QUEUE_COLOR_UNKNOWN;

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_queue_get_color(name, true, &actual_color);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_queue_get_color(name, false, &actual_color);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_color, actual_color);
  }

  // Abnormal case of getter
  {
    rc = datastore_queue_get_color(NULL, true, &actual_color);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_color(NULL, false, &actual_color);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_color(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_color(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_attr_private_committed_burst_size(void) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;
  uint64_t actual_committed_burst_size = 0;
  const uint64_t expected_committed_burst_size = DEFAULT_COMMITTED_BURST_SIZE;
  const uint64_t set_committed_burst_size1 = CAST_UINT64(
        MAXIMUM_COMMITTED_BURST_SIZE);
  const uint64_t set_committed_burst_size2 = CAST_UINT64(
        MINIMUM_COMMITTED_BURST_SIZE);
  const uint64_t set_committed_burst_size3 = CAST_UINT64(
        MAXIMUM_COMMITTED_BURST_SIZE + 1);
  const int set_committed_burst_size4 = -1;
  uint64_t actual_set_committed_burst_size1 = 0;
  uint64_t actual_set_committed_burst_size2 = 0;
  uint64_t actual_set_committed_burst_size3 = 0;
  uint64_t actual_set_committed_burst_size4 = 0;
  const uint64_t expected_set_committed_burst_size1 =
    MAXIMUM_COMMITTED_BURST_SIZE;
  const uint64_t expected_set_committed_burst_size2 =
    MINIMUM_COMMITTED_BURST_SIZE;
  const uint64_t expected_set_committed_burst_size3 =
    MINIMUM_COMMITTED_BURST_SIZE;
  const uint64_t expected_set_committed_burst_size4 =
    MINIMUM_COMMITTED_BURST_SIZE;

  queue_initialize();

  rc = queue_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new queue");

  // Normal case of getter
  {
    rc = queue_get_committed_burst_size(attr, &actual_committed_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_committed_burst_size,
                             actual_committed_burst_size);
  }

  // Abnormal case of getter
  {
    rc = queue_get_committed_burst_size(NULL, &actual_committed_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_get_committed_burst_size(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = queue_set_committed_burst_size(attr, set_committed_burst_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_committed_burst_size(attr, &actual_set_committed_burst_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_committed_burst_size1,
                             actual_set_committed_burst_size1);

    rc = queue_set_committed_burst_size(attr, set_committed_burst_size2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_committed_burst_size(attr, &actual_set_committed_burst_size2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_committed_burst_size2,
                             actual_set_committed_burst_size2);
  }

  // Abnormal case of setter
  {
    rc = queue_set_committed_burst_size(NULL, set_committed_burst_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_set_committed_burst_size(attr,
                                        CAST_UINT64(set_committed_burst_size3));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = queue_get_committed_burst_size(attr, &actual_set_committed_burst_size3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_committed_burst_size3,
                             actual_set_committed_burst_size3);

    rc = queue_set_committed_burst_size(attr,
                                        CAST_UINT64(set_committed_burst_size4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = queue_get_committed_burst_size(attr, &actual_set_committed_burst_size4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_committed_burst_size4,
                             actual_set_committed_burst_size4);
  }

  queue_attr_destroy(attr);

  queue_finalize();
}

void
test_queue_attr_public_committed_burst_size(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  uint64_t actual_committed_burst_size = 0;
  const uint64_t expected_committed_burst_size = DEFAULT_COMMITTED_BURST_SIZE;

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_queue_get_committed_burst_size(name, true,
         &actual_committed_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_queue_get_committed_burst_size(name, false,
         &actual_committed_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_committed_burst_size,
                             actual_committed_burst_size);
  }

  // Abnormal case of getter
  {
    rc = datastore_queue_get_committed_burst_size(NULL, true,
         &actual_committed_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_committed_burst_size(NULL, false,
         &actual_committed_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_committed_burst_size(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_committed_burst_size(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_attr_private_committed_information_rate(void) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;
  uint64_t actual_committed_information_rate = 0;
  const uint64_t expected_committed_information_rate =
    DEFAULT_COMMITTED_INFORMATION_RATE;
  const uint64_t set_committed_information_rate1 = CAST_UINT64(
        MAXIMUM_COMMITTED_INFORMATION_RATE);
  const uint64_t set_committed_information_rate2 = CAST_UINT64(
        MINIMUM_COMMITTED_INFORMATION_RATE);
  const uint64_t set_committed_information_rate3 = CAST_UINT64(
        MAXIMUM_COMMITTED_INFORMATION_RATE + 1);
  const int set_committed_information_rate4 = -1;
  uint64_t actual_set_committed_information_rate1 = 0;
  uint64_t actual_set_committed_information_rate2 = 0;
  uint64_t actual_set_committed_information_rate3 = 0;
  uint64_t actual_set_committed_information_rate4 = 0;
  const uint64_t expected_set_committed_information_rate1 =
    MAXIMUM_COMMITTED_INFORMATION_RATE;
  const uint64_t expected_set_committed_information_rate2 =
    MINIMUM_COMMITTED_INFORMATION_RATE;
  const uint64_t expected_set_committed_information_rate3 =
    MINIMUM_COMMITTED_INFORMATION_RATE;
  const uint64_t expected_set_committed_information_rate4 =
    MINIMUM_COMMITTED_INFORMATION_RATE;

  queue_initialize();

  rc = queue_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new queue");

  // Normal case of getter
  {
    rc = queue_get_committed_information_rate(attr,
         &actual_committed_information_rate);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_committed_information_rate,
                             actual_committed_information_rate);
  }

  // Abnormal case of getter
  {
    rc = queue_get_committed_information_rate(NULL,
         &actual_committed_information_rate);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_get_committed_information_rate(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = queue_set_committed_information_rate(attr,
         set_committed_information_rate1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_committed_information_rate(attr,
         &actual_set_committed_information_rate1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_committed_information_rate1,
                             actual_set_committed_information_rate1);

    rc = queue_set_committed_information_rate(attr,
         set_committed_information_rate2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_committed_information_rate(attr,
         &actual_set_committed_information_rate2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_committed_information_rate2,
                             actual_set_committed_information_rate2);
  }

  // Abnormal case of setter
  {
    rc = queue_set_committed_information_rate(NULL,
         set_committed_information_rate1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_set_committed_information_rate(attr,
         CAST_UINT64(set_committed_information_rate3));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = queue_get_committed_information_rate(attr,
         &actual_set_committed_information_rate3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_committed_information_rate3,
                             actual_set_committed_information_rate3);

    rc = queue_set_committed_information_rate(attr,
         CAST_UINT64(set_committed_information_rate4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = queue_get_committed_information_rate(attr,
         &actual_set_committed_information_rate4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_committed_information_rate4,
                             actual_set_committed_information_rate4);
  }

  queue_attr_destroy(attr);

  queue_finalize();
}

void
test_queue_attr_public_committed_information_rate(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  uint64_t actual_committed_information_rate = 0;
  const uint64_t expected_committed_information_rate =
    DEFAULT_COMMITTED_INFORMATION_RATE;

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_queue_get_committed_information_rate(name, true,
         &actual_committed_information_rate);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_queue_get_committed_information_rate(name, false,
         &actual_committed_information_rate);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_committed_information_rate,
                             actual_committed_information_rate);
  }

  // Abnormal case of getter
  {
    rc = datastore_queue_get_committed_information_rate(NULL, true,
         &actual_committed_information_rate);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_committed_information_rate(NULL, false,
         &actual_committed_information_rate);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_committed_information_rate(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_committed_information_rate(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_attr_private_excess_burst_size(void) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;
  uint64_t actual_excess_burst_size = 0;
  const uint64_t expected_excess_burst_size = DEFAULT_EXCESS_BURST_SIZE;
  const uint64_t set_excess_burst_size1 = CAST_UINT64(MAXIMUM_EXCESS_BURST_SIZE);
  const uint64_t set_excess_burst_size2 = CAST_UINT64(MINIMUM_EXCESS_BURST_SIZE);
  const uint64_t set_excess_burst_size3 = CAST_UINT64(MAXIMUM_EXCESS_BURST_SIZE +
                                          1);
  const int set_excess_burst_size4 = -1;
  uint64_t actual_set_excess_burst_size1 = 0;
  uint64_t actual_set_excess_burst_size2 = 0;
  uint64_t actual_set_excess_burst_size3 = 0;
  uint64_t actual_set_excess_burst_size4 = 0;
  const uint64_t expected_set_excess_burst_size1 = MAXIMUM_EXCESS_BURST_SIZE;
  const uint64_t expected_set_excess_burst_size2 = MINIMUM_EXCESS_BURST_SIZE;
  const uint64_t expected_set_excess_burst_size3 = MINIMUM_EXCESS_BURST_SIZE;
  const uint64_t expected_set_excess_burst_size4 = MINIMUM_EXCESS_BURST_SIZE;

  queue_initialize();

  rc = queue_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new queue");

  // Normal case of getter
  {
    rc = queue_get_excess_burst_size(attr, &actual_excess_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_excess_burst_size, actual_excess_burst_size);
  }

  // Abnormal case of getter
  {
    rc = queue_get_excess_burst_size(NULL, &actual_excess_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_get_excess_burst_size(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = queue_set_excess_burst_size(attr, set_excess_burst_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_excess_burst_size(attr, &actual_set_excess_burst_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_excess_burst_size1,
                             actual_set_excess_burst_size1);

    rc = queue_set_excess_burst_size(attr, set_excess_burst_size2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_excess_burst_size(attr, &actual_set_excess_burst_size2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_excess_burst_size2,
                             actual_set_excess_burst_size2);
  }

  // Abnormal case of setter
  {
    rc = queue_set_excess_burst_size(NULL, set_excess_burst_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_set_excess_burst_size(attr, CAST_UINT64(set_excess_burst_size3));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = queue_get_excess_burst_size(attr, &actual_set_excess_burst_size3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_excess_burst_size3,
                             actual_set_excess_burst_size3);

    rc = queue_set_excess_burst_size(attr, CAST_UINT64(set_excess_burst_size4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = queue_get_excess_burst_size(attr, &actual_set_excess_burst_size4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_excess_burst_size4,
                             actual_set_excess_burst_size4);
  }

  queue_attr_destroy(attr);

  queue_finalize();
}

void
test_queue_attr_public_excess_burst_size(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  uint64_t actual_excess_burst_size = 0;
  const uint64_t expected_excess_burst_size = DEFAULT_EXCESS_BURST_SIZE;

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_queue_get_excess_burst_size(name, true,
         &actual_excess_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_queue_get_excess_burst_size(name, false,
         &actual_excess_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_excess_burst_size, actual_excess_burst_size);
  }

  // Abnormal case of getter
  {
    rc = datastore_queue_get_excess_burst_size(NULL, true,
         &actual_excess_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_excess_burst_size(NULL, false,
         &actual_excess_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_excess_burst_size(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_excess_burst_size(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_attr_private_peak_burst_size(void) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;
  uint64_t actual_peak_burst_size = 0;
  const uint64_t expected_peak_burst_size = DEFAULT_PEAK_BURST_SIZE;
  const uint64_t set_peak_burst_size1 = CAST_UINT64(MAXIMUM_PEAK_BURST_SIZE);
  const uint64_t set_peak_burst_size2 = CAST_UINT64(MINIMUM_PEAK_BURST_SIZE);
  const uint64_t set_peak_burst_size3 = CAST_UINT64(MAXIMUM_PEAK_BURST_SIZE + 1);
  const int set_peak_burst_size4 = -1;
  uint64_t actual_set_peak_burst_size1 = 0;
  uint64_t actual_set_peak_burst_size2 = 0;
  uint64_t actual_set_peak_burst_size3 = 0;
  uint64_t actual_set_peak_burst_size4 = 0;
  const uint64_t expected_set_peak_burst_size1 = MAXIMUM_PEAK_BURST_SIZE;
  const uint64_t expected_set_peak_burst_size2 = MINIMUM_PEAK_BURST_SIZE;
  const uint64_t expected_set_peak_burst_size3 = MINIMUM_PEAK_BURST_SIZE;
  const uint64_t expected_set_peak_burst_size4 = MINIMUM_PEAK_BURST_SIZE;

  queue_initialize();

  rc = queue_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new queue");

  // Normal case of getter
  {
    rc = queue_get_peak_burst_size(attr, &actual_peak_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_peak_burst_size, actual_peak_burst_size);
  }

  // Abnormal case of getter
  {
    rc = queue_get_peak_burst_size(NULL, &actual_peak_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_get_peak_burst_size(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = queue_set_peak_burst_size(attr, set_peak_burst_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_peak_burst_size(attr, &actual_set_peak_burst_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_peak_burst_size1,
                             actual_set_peak_burst_size1);

    rc = queue_set_peak_burst_size(attr, set_peak_burst_size2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_peak_burst_size(attr, &actual_set_peak_burst_size2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_peak_burst_size2,
                             actual_set_peak_burst_size2);
  }

  // Abnormal case of setter
  {
    rc = queue_set_peak_burst_size(NULL, set_peak_burst_size1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_set_peak_burst_size(attr, CAST_UINT64(set_peak_burst_size3));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = queue_get_peak_burst_size(attr, &actual_set_peak_burst_size3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_peak_burst_size3,
                             actual_set_peak_burst_size3);

    rc = queue_set_peak_burst_size(attr, CAST_UINT64(set_peak_burst_size4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = queue_get_peak_burst_size(attr, &actual_set_peak_burst_size4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_peak_burst_size4,
                             actual_set_peak_burst_size4);
  }

  queue_attr_destroy(attr);

  queue_finalize();
}

void
test_queue_attr_public_peak_burst_size(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  uint64_t actual_peak_burst_size = 0;
  const uint64_t expected_peak_burst_size = DEFAULT_PEAK_BURST_SIZE;

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_queue_get_peak_burst_size(name, true, &actual_peak_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_queue_get_peak_burst_size(name, false, &actual_peak_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_peak_burst_size, actual_peak_burst_size);
  }

  // Abnormal case of getter
  {
    rc = datastore_queue_get_peak_burst_size(NULL, true, &actual_peak_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_peak_burst_size(NULL, false, &actual_peak_burst_size);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_peak_burst_size(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_peak_burst_size(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}

void
test_queue_attr_private_peak_information_rate(void) {
  lagopus_result_t rc;
  queue_attr_t *attr = NULL;
  uint64_t actual_peak_information_rate = 0;
  const uint64_t expected_peak_information_rate = DEFAULT_PEAK_INFORMATION_RATE;
  const uint64_t set_peak_information_rate1 = CAST_UINT64(
        MAXIMUM_PEAK_INFORMATION_RATE);
  const uint64_t set_peak_information_rate2 = CAST_UINT64(
        MINIMUM_PEAK_INFORMATION_RATE);
  const uint64_t set_peak_information_rate3 = CAST_UINT64(
        MAXIMUM_PEAK_INFORMATION_RATE + 1);
  const int set_peak_information_rate4 = -1;
  uint64_t actual_set_peak_information_rate1 = 0;
  uint64_t actual_set_peak_information_rate2 = 0;
  uint64_t actual_set_peak_information_rate3 = 0;
  uint64_t actual_set_peak_information_rate4 = 0;
  const uint64_t expected_set_peak_information_rate1 =
    MAXIMUM_PEAK_INFORMATION_RATE;
  const uint64_t expected_set_peak_information_rate2 =
    MINIMUM_PEAK_INFORMATION_RATE;
  const uint64_t expected_set_peak_information_rate3 =
    MINIMUM_PEAK_INFORMATION_RATE;
  const uint64_t expected_set_peak_information_rate4 =
    MINIMUM_PEAK_INFORMATION_RATE;

  queue_initialize();

  rc = queue_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new queue");

  // Normal case of getter
  {
    rc = queue_get_peak_information_rate(attr, &actual_peak_information_rate);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_peak_information_rate,
                             actual_peak_information_rate);
  }

  // Abnormal case of getter
  {
    rc = queue_get_peak_information_rate(NULL, &actual_peak_information_rate);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_get_peak_information_rate(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = queue_set_peak_information_rate(attr, set_peak_information_rate1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_peak_information_rate(attr, &actual_set_peak_information_rate1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_peak_information_rate1,
                             actual_set_peak_information_rate1);

    rc = queue_set_peak_information_rate(attr, set_peak_information_rate2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = queue_get_peak_information_rate(attr, &actual_set_peak_information_rate2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_peak_information_rate2,
                             actual_set_peak_information_rate2);
  }

  // Abnormal case of setter
  {
    rc = queue_set_peak_information_rate(NULL, set_peak_information_rate1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = queue_set_peak_information_rate(attr,
                                         CAST_UINT64(set_peak_information_rate3));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = queue_get_peak_information_rate(attr, &actual_set_peak_information_rate3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_peak_information_rate3,
                             actual_set_peak_information_rate3);

    rc = queue_set_peak_information_rate(attr,
                                         CAST_UINT64(set_peak_information_rate4));
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_SHORT, rc);
    rc = queue_get_peak_information_rate(attr, &actual_set_peak_information_rate4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_peak_information_rate4,
                             actual_set_peak_information_rate4);
  }

  queue_attr_destroy(attr);

  queue_finalize();
}

void
test_queue_attr_public_peak_information_rate(void) {
  lagopus_result_t rc;
  queue_conf_t *conf = NULL;
  const char *name = "queue_name";
  uint64_t actual_peak_information_rate = 0;
  const uint64_t expected_peak_information_rate = DEFAULT_PEAK_INFORMATION_RATE;

  queue_initialize();

  rc = queue_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new queue");

  rc = queue_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_queue_get_peak_information_rate(name, true,
         &actual_peak_information_rate);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_queue_get_peak_information_rate(name, false,
         &actual_peak_information_rate);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_peak_information_rate,
                             actual_peak_information_rate);
  }

  // Abnormal case of getter
  {
    rc = datastore_queue_get_peak_information_rate(NULL, true,
         &actual_peak_information_rate);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_peak_information_rate(NULL, false,
         &actual_peak_information_rate);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_peak_information_rate(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_queue_get_peak_information_rate(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  queue_finalize();
}
