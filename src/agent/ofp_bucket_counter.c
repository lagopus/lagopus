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
#include "ofp_bucket_counter.h"
#include "ofp_tlv.h"

struct bucket_counter *
bucket_counter_alloc(void) {
  struct bucket_counter *bucket_counter;

  bucket_counter = (struct bucket_counter *)
                   calloc(1, sizeof(struct bucket_counter));

  return bucket_counter;
}

void
bucket_counter_list_elem_free(struct bucket_counter_list
                              *bucket_counter_list) {
  struct bucket_counter *bucket_counter;

  while ((bucket_counter = TAILQ_FIRST(bucket_counter_list)) != NULL) {
    TAILQ_REMOVE(bucket_counter_list, bucket_counter, entry);
    free(bucket_counter);
  }
}

lagopus_result_t
ofp_bucket_counter_list_encode(struct pbuf_list *pbuf_list,
                               struct pbuf **pbuf,
                               struct bucket_counter_list *bucket_counter_list,
                               uint16_t *total_length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bucket_counter *bucket_counter;

  if (pbuf != NULL && bucket_counter_list != NULL &&
      total_length != NULL) {
    *total_length = 0;
    if (TAILQ_EMPTY(bucket_counter_list) == false) {
      TAILQ_FOREACH(bucket_counter, bucket_counter_list, entry) {
        ret = ofp_bucket_counter_encode_list(pbuf_list, pbuf,
                                             &bucket_counter->ofp);

        if (ret == LAGOPUS_RESULT_OK) {
          /* Sum length. And check overflow. */
          ret = ofp_tlv_length_sum(total_length,
                                   sizeof(struct ofp_bucket_counter));
          if (ret != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("over bucket_counter length (%s).\n",
                                lagopus_error_get_string(ret));
            break;
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
          break;
        }
      }
    } else {
      /* bucket_counter_list is empty. */
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
