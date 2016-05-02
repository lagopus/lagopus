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
#include "ofp_apis.h"
#include "ofp_bucket.h"
#include "ofp_action.h"
#include "ofp_tlv.h"
#include "lagopus/ofp_pdump.h"

struct bucket *
bucket_alloc(void) {
  struct bucket *bucket;

  bucket = (struct bucket *)
           calloc(1, sizeof(struct bucket));
  if (bucket != NULL) {
    TAILQ_INIT(&bucket->action_list);
  }

  return bucket;
}

void
ofp_bucket_list_trace(uint32_t flags,
                      struct bucket_list *bucket_list) {
  struct bucket *bucket;

  TAILQ_FOREACH(bucket, bucket_list, entry) {
    lagopus_msg_pdump(flags, PDUMP_OFP_BUCKET,
                      bucket->ofp, "");
    ofp_action_list_trace(flags, &bucket->action_list);
  }
}

/* Free bucket list. */
void
ofp_bucket_list_free(struct bucket_list *bucket_list) {
  struct bucket *bucket;

  while (TAILQ_EMPTY(bucket_list) == false) {
    bucket = TAILQ_FIRST(bucket_list);
    if (TAILQ_EMPTY(&bucket->action_list) == false) {
      ofp_action_list_elem_free(&bucket->action_list);
    }
    TAILQ_REMOVE(bucket_list, bucket, entry);
    free(bucket);
  }
}

/* ofp_bucket parser. */
lagopus_result_t
ofp_bucket_parse(struct pbuf *pbuf,
                 struct bucket_list *bucket_list,
                 struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct bucket *bucket = NULL;
  uint8_t *bucket_head = NULL;

  if (pbuf != NULL && bucket_list != NULL &&
      error != NULL) {
    /* Allocate a new bucket. */
    bucket = bucket_alloc();

    if (bucket != NULL) {
      /* insert */
      TAILQ_INSERT_TAIL(bucket_list, bucket, entry);

      bucket_head = pbuf_getp_get(pbuf);

      /* Decode bucket */
      res = ofp_bucket_decode(pbuf, &bucket->ofp);

      if (res == LAGOPUS_RESULT_OK) {
        /* check length. */
        res = pbuf_plen_check(pbuf, bucket->ofp.len -
                              sizeof(struct ofp_bucket));
        if (res == LAGOPUS_RESULT_OK) {
          /* parse actions */
          res = ofp_action_parse(pbuf,
                                 bucket->ofp.len - sizeof(struct ofp_bucket),
                                 &(bucket->action_list),
                                 error);
          if (res == LAGOPUS_RESULT_OK) {
            /* check length. */
            if ((bucket_head + bucket->ofp.len) != pbuf_getp_get(pbuf)) {
              lagopus_msg_warning("bad length.\n");
              ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
              res = LAGOPUS_RESULT_OFP_ERROR;
            }
          } else {
            lagopus_msg_warning("FAILED (%s).\n",
                                lagopus_error_get_string(res));
          }
        } else {
          lagopus_msg_warning("bad length.\n");
          ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
          res = LAGOPUS_RESULT_OFP_ERROR;
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(res));
        ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
        res = LAGOPUS_RESULT_OFP_ERROR;
      }

      /* free */
      if (bucket != NULL && res != LAGOPUS_RESULT_OK) {
        ofp_bucket_list_free(bucket_list);
      }
    } else {
      lagopus_msg_warning("Can't allocate bucket.\n");
      res = LAGOPUS_RESULT_NO_MEMORY;
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return res;
}

lagopus_result_t
ofp_bucket_list_encode(struct pbuf_list *pbuf_list,
                       struct pbuf **pbuf,
                       struct bucket_list *bucket_list,
                       uint16_t *total_length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t bucket_len = 0;
  uint16_t action_total_len = 0;
  uint8_t *bucket_head = NULL;
  struct bucket *bucket;

  if (pbuf != NULL && bucket_list != NULL &&
      total_length != NULL) {
    *total_length = 0;
    if (TAILQ_EMPTY(bucket_list) == false) {
      TAILQ_FOREACH(bucket, bucket_list, entry) {
        ret = ofp_bucket_encode_list(pbuf_list, pbuf, &bucket->ofp);
        if (ret == LAGOPUS_RESULT_OK) {
          /* bucket head pointer. */
          bucket_head = pbuf_putp_get(*pbuf) - sizeof(struct ofp_bucket);

          ret = ofp_action_list_encode(pbuf_list, pbuf, &bucket->action_list,
                                       &action_total_len);
          if (ret == LAGOPUS_RESULT_OK) {
            /* Set bucket length (action_total_len + size of ofp_bucket). */
            /* And check overflow. */
            bucket_len = action_total_len;
            ret = ofp_tlv_length_sum(&bucket_len, sizeof(struct ofp_bucket));

            if (ret == LAGOPUS_RESULT_OK) {
              ret = ofp_multipart_length_set(bucket_head,
                                             (uint16_t) bucket_len);

              if (ret == LAGOPUS_RESULT_OK) {
                /* Sum length. And check overflow. */
                ret = ofp_tlv_length_sum(total_length, bucket_len);
                if (ret != LAGOPUS_RESULT_OK) {
                  lagopus_msg_warning("over bucket length (%s).\n",
                                      lagopus_error_get_string(ret));
                  break;
                }
              } else {
                lagopus_msg_warning("FAILED (%s).\n",
                                    lagopus_error_get_string(ret));
              }
            } else {
              lagopus_msg_warning("over bucket length (%s).\n",
                                  lagopus_error_get_string(ret));
              break;
            }
          } else {
            lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
            break;
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
          break;
        }
      }
    } else {
      /* bucket_list is empty.*/
      ret =  LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
