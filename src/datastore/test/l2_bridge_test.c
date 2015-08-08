/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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
#include "../l2_bridge.c"
#define CAST_UINT64(x) (uint64_t) x

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_l2_bridge_initialize_and_finalize(void) {
  l2_bridge_initialize();
  l2_bridge_finalize();
}

void
test_l2_bridge_conf_create_and_destroy(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_bridge_name";

  l2_bridge_initialize();

  // Normal case
  {
    rc = l2_bridge_conf_create(&conf, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

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
    rc = l2_bridge_conf_create(NULL, name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = l2_bridge_conf_create(&conf, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  l2_bridge_conf_destroy(conf);

  l2_bridge_finalize();
}

void
test_l2_bridge_conf_add(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_bridge_name";
  l2_bridge_conf_t *actual_conf = NULL;

  l2_bridge_initialize();

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

  // Normal case
  {
    rc = l2_bridge_conf_add(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = l2_bridge_conf_add(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = l2_bridge_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  l2_bridge_finalize();
}

void
test_l2_bridge_conf_add_not_initialize(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_bridge_name";

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

  rc = l2_bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  l2_bridge_conf_destroy(conf);

  l2_bridge_finalize();
}

void
test_l2_bridge_conf_delete(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_bridge_name";
  l2_bridge_conf_t *actual_conf = NULL;

  l2_bridge_initialize();

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

  rc = l2_bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    rc = l2_bridge_conf_delete(conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Abnormal case
  {
    rc = l2_bridge_conf_delete(NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  rc = l2_bridge_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_FOUND, rc);

  l2_bridge_finalize();
}

void
test_l2_bridge_conf_delete_not_initialize(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_bridge_name";

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

  rc = l2_bridge_conf_delete(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  l2_bridge_conf_destroy(conf);

  l2_bridge_finalize();
}

void
test_l2_bridge_conf_iterate(void) {
  TEST_IGNORE();
}

void
test_l2_bridge_conf_list(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf1 = NULL;
  const char *name1 = "namespace1"NAMESPACE_DELIMITER"l2_bridge_name1";
  l2_bridge_conf_t *conf2 = NULL;
  const char *name2 = "namespace1"NAMESPACE_DELIMITER"l2_bridge_name2";
  l2_bridge_conf_t *conf3 = NULL;
  const char *name3 = "namespace2"NAMESPACE_DELIMITER"l2_bridge_name3";
  l2_bridge_conf_t *conf4 = NULL;
  const char *name4 = NAMESPACE_DELIMITER"l2_bridge_name4";
  l2_bridge_conf_t *conf5 = NULL;
  const char *name5 = NAMESPACE_DELIMITER"l2_bridge_name5";
  l2_bridge_conf_t **actual_list = NULL;
  size_t i;

  l2_bridge_initialize();

  // create conf
  {
    rc = l2_bridge_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new l2_bridge");

    rc = l2_bridge_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new l2_bridge");

    rc = l2_bridge_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new l2_bridge");

    rc = l2_bridge_conf_create(&conf4, name4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf4, "conf_create() will create new l2_bridge");

    rc = l2_bridge_conf_create(&conf5, name5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf5, "conf_create() will create new l2_bridge");
  }

  // add conf
  {
    rc = l2_bridge_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = l2_bridge_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = l2_bridge_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = l2_bridge_conf_add(conf4);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = l2_bridge_conf_add(conf5);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // all
  {
    rc = l2_bridge_conf_list(&actual_list, NULL);
    TEST_ASSERT_EQUAL(5, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"NAMESPACE_DELIMITER"l2_bridge_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"NAMESPACE_DELIMITER"l2_bridge_name2") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace2"NAMESPACE_DELIMITER"l2_bridge_name3") != 0 &&
          strcasecmp(actual_list[i]->name,
                     NAMESPACE_DELIMITER"l2_bridge_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     NAMESPACE_DELIMITER"l2_bridge_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // no namespace
  {
    rc = l2_bridge_conf_list(&actual_list, "");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     NAMESPACE_DELIMITER"l2_bridge_name4") != 0 &&
          strcasecmp(actual_list[i]->name,
                     NAMESPACE_DELIMITER"l2_bridge_name5") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // only namespace
  {
    rc = l2_bridge_conf_list(&actual_list, "namespace1");
    TEST_ASSERT_EQUAL(2, rc);

    for (i = 0; i < (size_t) rc; i++) {
      if (strcasecmp(actual_list[i]->name,
                     "namespace1"NAMESPACE_DELIMITER"l2_bridge_name1") != 0 &&
          strcasecmp(actual_list[i]->name,
                     "namespace1"NAMESPACE_DELIMITER"l2_bridge_name2") != 0) {
        TEST_FAIL_MESSAGE("invalid list entry.");
      }
    }

    free((void *) actual_list);
  }

  // Abnormal case
  {
    rc = l2_bridge_conf_list(NULL, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  l2_bridge_finalize();
}

void
test_l2_bridge_conf_list_not_initialize(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_bridge_name";
  l2_bridge_conf_t ***list = NULL;

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

  rc = l2_bridge_conf_list(list, NULL);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  l2_bridge_conf_destroy(conf);

  l2_bridge_finalize();
}

void
test_l2_bridge_conf_one_list(void) {
  TEST_IGNORE();
}

void
test_l2_bridge_conf_one_list_not_initialize(void) {
  TEST_IGNORE();
}

void
test_l2_bridge_find(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf1 = NULL;
  const char *name1 = "l2_bridge_name1";
  l2_bridge_conf_t *conf2 = NULL;
  const char *name2 = "l2_bridge_name2";
  l2_bridge_conf_t *conf3 = NULL;
  const char *name3 = "l2_bridge_name3";
  l2_bridge_conf_t *actual_conf = NULL;

  l2_bridge_initialize();

  // create conf
  {
    rc = l2_bridge_conf_create(&conf1, name1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf1, "conf_create() will create new l2_bridge");

    rc = l2_bridge_conf_create(&conf2, name2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf2, "conf_create() will create new l2_bridge");

    rc = l2_bridge_conf_create(&conf3, name3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL_MESSAGE(conf3, "conf_create() will create new l2_bridge");
  }

  // add conf
  {
    rc = l2_bridge_conf_add(conf1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = l2_bridge_conf_add(conf2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

    rc = l2_bridge_conf_add(conf3);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  }

  // Normal case
  {
    rc = l2_bridge_find(name1, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name1, actual_conf->name);

    rc = l2_bridge_find(name2, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name2, actual_conf->name);

    rc = l2_bridge_find(name3, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(name3, actual_conf->name);

  }

  // Abnormal case
  {
    rc = l2_bridge_find(NULL, &actual_conf);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = l2_bridge_find(name1, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  l2_bridge_finalize();
}

void
test_l2_bridge_find_not_initialize(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_bridge_name";
  l2_bridge_conf_t *actual_conf = NULL;

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

  rc = l2_bridge_find(name, &actual_conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_NOT_STARTED, rc);

  l2_bridge_conf_destroy(conf);

  l2_bridge_finalize();
}

void
test_l2_bridge_attr_create_and_destroy(void) {
  lagopus_result_t rc;
  l2_bridge_attr_t *attr = NULL;
  uint64_t actual_expire = 0;
  const uint64_t expected_expire = 300;
  uint64_t actual_max_entries = 0;
  const uint64_t expected_max_entries = 1000000;
  char *actual_tmp_dir = NULL;
  const char *expected_tmp_dir = DATASTORE_TMP_DIR;

  l2_bridge_initialize();

  rc = l2_bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new l2_bridge");

  // default value
  {
    // expire
    rc = l2_bridge_get_expire(attr, &actual_expire);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_expire, actual_expire);

    // max_entries
    rc = l2_bridge_get_max_entries(attr, &actual_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_max_entries, actual_max_entries);

    // tmp_dir
    rc = l2_bridge_get_tmp_dir(attr, &actual_tmp_dir);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_tmp_dir, actual_tmp_dir);
  }

  l2_bridge_attr_destroy(attr);
  free((void *)actual_tmp_dir);

  l2_bridge_finalize();
}

void
test_l2_bridge_attr_duplicate(void) {
  lagopus_result_t rc;
  bool result = false;
  l2_bridge_attr_t *src_attr = NULL;
  l2_bridge_attr_t *dst_attr = NULL;

  l2_bridge_initialize();

  rc = l2_bridge_attr_create(&src_attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(src_attr, "attr_create() will create new l2_bridge");

  // Normal case
  {
    rc = l2_bridge_attr_duplicate(src_attr, &dst_attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    result = l2_bridge_attr_equals(src_attr, dst_attr);
    TEST_ASSERT_TRUE(result);
  }

  // Abnormal case
  {
    result = l2_bridge_attr_equals(src_attr, NULL);
    TEST_ASSERT_FALSE(result);

    result = l2_bridge_attr_equals(NULL, dst_attr);
    TEST_ASSERT_FALSE(result);
  }

  l2_bridge_attr_destroy(src_attr);
  l2_bridge_attr_destroy(dst_attr);

  l2_bridge_finalize();
}

void
test_l2_bridge_attr_equals(void) {
  lagopus_result_t rc;
  bool result = false;
  l2_bridge_attr_t *attr1 = NULL;
  l2_bridge_attr_t *attr2 = NULL;
  l2_bridge_attr_t *attr3 = NULL;

  l2_bridge_initialize();

  rc = l2_bridge_attr_create(&attr1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr1, "attr_create() will create new l2_bridge");

  rc = l2_bridge_attr_create(&attr2);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr2, "attr_create() will create new l2_bridge");

  rc = l2_bridge_attr_create(&attr3);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr3, "attr_create() will create new l2_bridge");
  rc = l2_bridge_set_expire(attr3, 1);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    result = l2_bridge_attr_equals(attr1, attr2);
    TEST_ASSERT_TRUE(result);

    result = l2_bridge_attr_equals(attr1, attr3);
    TEST_ASSERT_FALSE(result);

    result = l2_bridge_attr_equals(attr2, attr3);
    TEST_ASSERT_FALSE(result);
  }

  // Abnormal case
  {
    result = l2_bridge_attr_equals(attr1, NULL);
    TEST_ASSERT_FALSE(result);

    result = l2_bridge_attr_equals(NULL, attr2);
    TEST_ASSERT_FALSE(result);
  }

  l2_bridge_attr_destroy(attr1);
  l2_bridge_attr_destroy(attr2);
  l2_bridge_attr_destroy(attr3);

  l2_bridge_finalize();
}

void
test_l2_bridge_get_attr(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  l2_bridge_attr_t *attr = NULL;
  const char *name = "l2_bridge_name";

  l2_bridge_initialize();

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

  rc = l2_bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = l2_bridge_get_attr(name, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = l2_bridge_get_attr(name, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(attr);
  }

  // Abnormal case of getter
  {
    rc = l2_bridge_get_attr(NULL, true, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = l2_bridge_get_attr(NULL, false, &attr);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = l2_bridge_get_attr(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = l2_bridge_get_attr(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  l2_bridge_finalize();
}

void
test_l2_bridge_conf_private_exists(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_bridge_name1";
  const char *invalid_name = "invalid_name";

  l2_bridge_initialize();

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

  rc = l2_bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case
  {
    TEST_ASSERT_TRUE(l2_bridge_exists(name) == true);
    TEST_ASSERT_TRUE(l2_bridge_exists(invalid_name) == false);
  }

  // Abnormal case
  {
    TEST_ASSERT_TRUE(l2_bridge_exists(NULL) == false);
  }

  l2_bridge_finalize();
}

void
test_l2_bridge_conf_private_used(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_bridge_name";
  bool actual_used = false;

  l2_bridge_initialize();

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

  rc = l2_bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = l2_bridge_set_used(name, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_l2_bridge_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = l2_bridge_set_used(NULL, actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_l2_bridge_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  l2_bridge_finalize();
}

void
test_l2_bridge_conf_public_used(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_bridge_name";
  bool actual_used = false;

  l2_bridge_initialize();

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

  rc = l2_bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_l2_bridge_is_used(name, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_used);
  }

  // Abnormal case of getter
  {
    rc = datastore_l2_bridge_is_used(NULL, &actual_used);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_l2_bridge_is_used(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  l2_bridge_finalize();
}

void
test_l2_bridge_conf_private_enabled(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_bridge_name";
  bool actual_enabled = false;

  l2_bridge_initialize();

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

  rc = l2_bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = l2_bridge_set_enabled(name, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = datastore_l2_bridge_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = l2_bridge_set_enabled(NULL, actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_l2_bridge_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  l2_bridge_finalize();
}

void
test_l2_bridge_conf_public_enabled(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_bridge_name";
  bool actual_enabled = false;

  l2_bridge_initialize();

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

  rc = l2_bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_l2_bridge_is_enabled(name, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_FALSE(actual_enabled);
  }

  // Abnormal case of getter
  {
    rc = datastore_l2_bridge_is_enabled(NULL, &actual_enabled);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_l2_bridge_is_enabled(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  l2_bridge_finalize();
}

void
test_l2_bridge_attr_private_expire(void) {
  lagopus_result_t rc;
  l2_bridge_attr_t *attr = NULL;
  uint64_t actual_expire = 0;
  const uint64_t expected_expire = 300;
  const uint64_t set_expire1 = UINT64_MAX;
  uint64_t actual_set_expire1 = 0;
  const uint64_t expected_set_expire1 = UINT64_MAX;

  l2_bridge_initialize();

  rc = l2_bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr, "attr_create() will create new l2_bridge");

  // Normal case of getter
  {
    rc = l2_bridge_get_expire(attr, &actual_expire);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_expire, actual_expire);
  }

  // Abnormal case of getter
  {
    rc = l2_bridge_get_expire(NULL, &actual_expire);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = l2_bridge_get_expire(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = l2_bridge_set_expire(attr, set_expire1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = l2_bridge_get_expire(attr, &actual_set_expire1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_expire1, actual_set_expire1);
  }

  // Abnormal case of setter
  {
    rc = l2_bridge_set_expire(NULL, set_expire1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  l2_bridge_attr_destroy(attr);

  l2_bridge_finalize();
}

void
test_l2_bridge_attr_public_expire(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_bridge_name";
  uint64_t actual_expire = 0;
  const uint64_t expected_expire = 300;

  l2_bridge_initialize();

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

  rc = l2_bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_l2_bridge_get_expire(name, true, &actual_expire);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_l2_bridge_get_expire(name, false, &actual_expire);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_expire, actual_expire);
  }

  // Abnormal case of getter
  {
    rc = datastore_l2_bridge_get_expire(NULL, true, &actual_expire);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_l2_bridge_get_expire(NULL, false, &actual_expire);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_l2_bridge_get_expire(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_l2_bridge_get_expire(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  l2_bridge_finalize();
}

void
test_l2_bridge_attr_private_max_entries(void) {
  lagopus_result_t rc;
  l2_bridge_attr_t *attr = NULL;
  uint64_t actual_max_entries = 0;
  const uint64_t expected_max_entries = 1000000;
  const uint64_t set_max_entries1 = UINT64_MAX;
  uint64_t actual_set_max_entries1 = 0;
  const uint64_t expected_set_max_entries1 = UINT64_MAX;

  l2_bridge_initialize();

  rc = l2_bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr,
                               "attr_create() will create new l2_bridge");

  // Normal case of getter
  {
    rc = l2_bridge_get_max_entries(attr, &actual_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_max_entries, actual_max_entries);
  }

  // Abnormal case of getter
  {
    rc = l2_bridge_get_max_entries(NULL, &actual_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = l2_bridge_get_max_entries(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = l2_bridge_set_max_entries(attr, set_max_entries1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = l2_bridge_get_max_entries(attr, &actual_set_max_entries1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(expected_set_max_entries1,
                             actual_set_max_entries1);
  }

  // Abnormal case of setter
  {
    rc = l2_bridge_set_max_entries(NULL, set_max_entries1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  l2_bridge_attr_destroy(attr);

  l2_bridge_finalize();
}

void
test_l2_bridge_attr_public_max_entries(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_bridge_name";
  uint64_t actual_max_entries = 0;
  const uint64_t expected_max_entries = 1000000;

  l2_bridge_initialize();

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf,
                               "conf_create() will create new l2_bridge");

  rc = l2_bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_l2_bridge_get_max_entries(name, true, &actual_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_l2_bridge_get_max_entries(name, false, &actual_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT32(expected_max_entries, actual_max_entries);
  }

  // Abnormal case of getter
  {
    rc = datastore_l2_bridge_get_max_entries(NULL, true, &actual_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_l2_bridge_get_max_entries(NULL, false, &actual_max_entries);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_l2_bridge_get_max_entries(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_l2_bridge_get_max_entries(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  l2_bridge_finalize();
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
test_l2_bridge_create_str(void) {
  char *actual_str = NULL;
  const char *expected_str = "aaa";

  create_str(&actual_str, strlen(expected_str));
  TEST_ASSERT_EQUAL_STRING(expected_str, actual_str);

  free((void *)actual_str);
}

void
test_l2_bridge_attr_public_bridge_name(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_bridge_name";
  char *actual_bridge_name = NULL;
  const char *expected_bridge_name = "";

  l2_bridge_initialize();

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

  rc = l2_bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_l2_bridge_get_bridge_name(name, &actual_bridge_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_bridge_name, actual_bridge_name);
  }

  // Abnormal case of getter
  {
    rc = datastore_l2_bridge_get_bridge_name(NULL, &actual_bridge_name);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_l2_bridge_get_bridge_name(name, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  free((void *)actual_bridge_name);
  l2_bridge_finalize();
}

void
test_l2_bridge_attr_private_tmp_dir(void) {
  lagopus_result_t rc;
  l2_bridge_attr_t *attr = NULL;
  char *actual_tmp_dir = NULL;
  const char *expected_tmp_dir = DATASTORE_TMP_DIR;
  const char *set_tmp_dir1 = "tmp_dir";
  char *set_tmp_dir2 = NULL;
  char *actual_set_tmp_dir1 = NULL;
  char *actual_set_tmp_dir2 = NULL;
  const char *expected_set_tmp_dir1 = set_tmp_dir1;
  const char *expected_set_tmp_dir2 = set_tmp_dir1;

  l2_bridge_initialize();

  rc = l2_bridge_attr_create(&attr);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(attr,
                               "attr_create() will create new l2_bridge");

  // Normal case of getter
  {
    rc = l2_bridge_get_tmp_dir(attr, &actual_tmp_dir);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_tmp_dir, actual_tmp_dir);
  }

  // Abnormal case of getter
  {
    rc = l2_bridge_get_tmp_dir(NULL, &actual_tmp_dir);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = l2_bridge_get_tmp_dir(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  // Normal case of setter
  {
    rc = l2_bridge_set_tmp_dir(attr, set_tmp_dir1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    rc = l2_bridge_get_tmp_dir(attr, &actual_set_tmp_dir1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_tmp_dir1,
                             actual_set_tmp_dir1);

    create_str(&set_tmp_dir2, PATH_MAX);
    rc = l2_bridge_set_tmp_dir(attr, set_tmp_dir2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_TOO_LONG, rc);
    rc = l2_bridge_get_tmp_dir(attr, &actual_set_tmp_dir2);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_set_tmp_dir2,
                             actual_set_tmp_dir2);
  }

  // Abnormal case of setter
  {
    rc = l2_bridge_set_tmp_dir(NULL, set_tmp_dir1);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = l2_bridge_set_tmp_dir(attr, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  l2_bridge_attr_destroy(attr);
  free((void *)actual_tmp_dir);
  free((void *)set_tmp_dir2);
  free((void *)actual_set_tmp_dir1);
  free((void *)actual_set_tmp_dir2);

  l2_bridge_finalize();
}

void
test_l2_bridge_attr_public_tmp_dir(void) {
  lagopus_result_t rc;
  l2_bridge_conf_t *conf = NULL;
  const char *name = "l2_tmp_dir";
  char *actual_tmp_dir = NULL;
  const char *expected_tmp_dir = DATASTORE_TMP_DIR;

  l2_bridge_initialize();

  rc = l2_bridge_conf_create(&conf, name);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
  TEST_ASSERT_NOT_NULL_MESSAGE(conf, "conf_create() will create new l2_bridge");

  rc = l2_bridge_conf_add(conf);
  TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);

  // Normal case of getter
  {
    rc = datastore_l2_bridge_get_tmp_dir(name, true, &actual_tmp_dir);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_OBJECT, rc);

    rc = datastore_l2_bridge_get_tmp_dir(name, false, &actual_tmp_dir);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(expected_tmp_dir, actual_tmp_dir);
  }

  // Abnormal case of getter
  {
    rc = datastore_l2_bridge_get_tmp_dir(NULL, true, &actual_tmp_dir);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_l2_bridge_get_tmp_dir(NULL, false, &actual_tmp_dir);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_l2_bridge_get_tmp_dir(name, true, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);

    rc = datastore_l2_bridge_get_tmp_dir(name, false, NULL);
    TEST_ASSERT_EQUAL(LAGOPUS_RESULT_INVALID_ARGS, rc);
  }

  free((void *)actual_tmp_dir);
  l2_bridge_finalize();
}
