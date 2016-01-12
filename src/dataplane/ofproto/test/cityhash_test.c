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

#include "lagopus_includes.h"
#include "City.h"
#include "cityhash_test_imported.h"


void
setUp(void) {
  SETUP();
}

void
tearDown(void) {
}

void
test_cityhash(void) {
  TEST_RUN_CITYHASH();

  TEST_ASSERT_EQUAL_INT(0, errors);
}
