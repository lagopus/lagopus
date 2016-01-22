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
#include "ofp_band_stats.h"
#include "ofp_tlv.h"

struct meter_band_stats *
meter_band_stats_alloc(void) {
  struct meter_band_stats *meter_band_stats;

  meter_band_stats = (struct meter_band_stats *)
                     calloc(1, sizeof(struct meter_band_stats));

  return meter_band_stats;
}

void
meter_band_stats_list_elem_free(struct meter_band_stats_list
                                *band_stats_list) {
  struct meter_band_stats *meter_band_stats;

  while ((meter_band_stats = TAILQ_FIRST(band_stats_list)) != NULL) {
    TAILQ_REMOVE(band_stats_list, meter_band_stats, entry);
    free(meter_band_stats);
  }
}

lagopus_result_t
ofp_meter_band_stats_list_encode(struct pbuf_list *pbuf_list,
                                 struct pbuf **pbuf,
                                 struct meter_band_stats_list *band_stats_list,
                                 uint16_t *total_length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct meter_band_stats *meter_band_stats;

  if (pbuf != NULL && band_stats_list != NULL &&
      total_length != NULL) {
    *total_length = 0;
    if (TAILQ_EMPTY(band_stats_list) == false) {
      TAILQ_FOREACH(meter_band_stats, band_stats_list, entry) {
        ret = ofp_meter_band_stats_encode_list(pbuf_list, pbuf,
                                               &meter_band_stats->ofp);
        if (ret == LAGOPUS_RESULT_OK) {
          /* Sum length. And check overflow. */
          ret = ofp_tlv_length_sum(total_length,
                                   sizeof(struct ofp_meter_band_stats));
          if (ret != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("over meter_band_stats length (%s).\n",
                                lagopus_error_get_string(ret));
            break;
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
          break;
        }
      }
    } else {
      /* band_stats_list is empty. */
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
