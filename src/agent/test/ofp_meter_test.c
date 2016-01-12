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
#include "../ofp_meter.h"

void
setUp(void) {
}

void
tearDown(void) {
}

#define TEST_METER_ID_NUM 5

void
test_ofp_meter_id_check_normal_pattern(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint32_t meter_ids[TEST_METER_ID_NUM] =
  {1, OFPM_MAX, OFPM_SLOWPATH, OFPM_CONTROLLER, OFPM_ALL};
  int i;
  struct ofp_error error;

  for (i = 0; i < TEST_METER_ID_NUM; i++) {
    ret = ofp_meter_id_check(meter_ids[i], &error);
    TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OK, ret,
                              "ofp_oxm_field_check error.");
  }
}

void
test_ofp_meter_id_check_invalid_meter_id(void) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error expected_error = {0, 0, {NULL}};
  struct ofp_error error;
  uint32_t meter_id;

  ofp_error_set(&expected_error,
                OFPET_METER_MOD_FAILED,
                OFPMMFC_INVALID_METER);
  meter_id = OFPM_MAX + 1;

  ret = ofp_meter_id_check(meter_id, &error);
  TEST_ASSERT_EQUAL_MESSAGE(LAGOPUS_RESULT_OFP_ERROR, ret,
                            "ofp_oxm_field_check(invalid meter id) error.");
  TEST_ASSERT_EQUAL_MESSAGE(expected_error.type, error.type,
                            "the expected error type is not matched.");
  TEST_ASSERT_EQUAL_MESSAGE(expected_error.code, error.code,
                            "the expected error code is not matched.");
}
