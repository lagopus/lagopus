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

#include "thtable.c"

thtable_t thtable;

void
setUp(void) {
  TEST_ASSERT_EQUAL(dp_api_init(), LAGOPUS_RESULT_OK);
  thtable = thtable_alloc();
  TEST_ASSERT_NOT_NULL(thtable);
}

void
tearDown(void) {
  TEST_ASSERT_NOT_NULL(thtable);
  thtable_free(thtable);
  dp_api_fini();
}

void
test_htlist_alloc(void) {
  struct htlist *htlist;
  int i;

  for (i = 32767; i >= 0; i--) {
    htlist = htlist_alloc(i);
    TEST_ASSERT_NOT_NULL(htlist);
    TEST_ASSERT_EQUAL(htlist->priority, i);
    TEST_ASSERT_EQUAL(htlist->ntable, 0);
    htlist_free(htlist);
  }
}

void
test_htable_alloc(void) {
  struct htable *htable;

  htable = htable_alloc();
  TEST_ASSERT_NOT_NULL(htable);
  TEST_ASSERT_NOT_NULL(htable->hashmap);
  htable_free(htable);
}

void
test_thtable_add_htlist(void) {
  struct htlist *htlist;
  int i;

  for (i = 0; i < 32768; i++) {
    htlist = htlist_alloc(i);
    TEST_ASSERT_NOT_NULL(htlist);
    thtable_add_htlist(thtable, htlist);
    TEST_ASSERT_EQUAL(thtable[htlist->priority], htlist);
  }
}
