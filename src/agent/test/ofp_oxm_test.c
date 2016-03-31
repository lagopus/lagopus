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

#include <sys/queue.h>
#include "unity.h"
#include "lagopus_apis.h"
#include "lagopus/pbuf.h"
#include "openflow.h"
#include "openflow13.h"
#include "handler_test_utils.h"
#include "../ofp_oxm.h"

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_ofp_oxm_field_check_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t oxm_field = (OFPXMT_OFB_GTPU_EXTN_SCI << 1) + 0x01;

  ret = ofp_oxm_field_check(oxm_field);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                            "ofp_oxm_field_check error.");
}

void
test_ofp_oxm_field_check_bad_field(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t oxm_field = ((OFPXMT_OFB_GTPU_EXTN_SCI + 1) << 1) + 0x01;

  ret = ofp_oxm_field_check(oxm_field);

  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OXM_ERROR, ret,
                            "ofp_oxm_field_check(bad_field) error.");
}
