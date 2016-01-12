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

#include <stdbool.h>
#include <stdint.h>
#include "lagopus_apis.h"
#include "openflow.h"
#include "openflow13packet.h"
#include "ofp_meter.h"
#include "lagopus/ofp_dp_apis.h"

lagopus_result_t
ofp_meter_id_check(uint32_t meter_id, struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_OFP_ERROR;

  if (meter_id > OFPM_MAX &&
      meter_id != OFPM_SLOWPATH &&
      meter_id != OFPM_CONTROLLER &&
      meter_id != OFPM_ALL) {
    lagopus_msg_warning("invalid meter_id.\n");
    ofp_error_set(error, OFPET_METER_MOD_FAILED, OFPMMFC_INVALID_METER);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

  return ret;
}
