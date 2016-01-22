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
#include "ofp_tlv.h"

lagopus_result_t
ofp_tlv_length_set(uint8_t *tlv_head,
                   uint16_t length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (tlv_head != NULL) {
    /* sneak type in tlv. */
    tlv_head += 2;

    /* set length in tlv. */
    *(tlv_head) = (uint8_t)((length >> 8) & 0xFF);
    *(tlv_head + 1) = (uint8_t)(length & 0xFF);
    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_tlv_length_sum(uint16_t *total_length,
                   uint16_t length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  *total_length = (uint16_t) (*total_length + length);
  if (*total_length < length) {
    ret = LAGOPUS_RESULT_OUT_OF_RANGE;
  } else {
    ret = LAGOPUS_RESULT_OK;
  }

  return ret;
}
