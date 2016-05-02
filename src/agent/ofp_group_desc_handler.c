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
#include "openflow13.h"
#include "openflow13packet.h"
#include "channel.h"
#include "ofp_apis.h"
#include "ofp_bucket.h"
#include "lagopus/ofp_dp_apis.h"

void
group_desc_list_elem_free(struct group_desc_list *group_desc_list) {
  struct group_desc *group_desc;

  while ((group_desc = TAILQ_FIRST(group_desc_list)) != NULL) {
    ofp_bucket_list_free(&group_desc->bucket_list);
    TAILQ_REMOVE(group_desc_list, group_desc, entry);
    free(group_desc);
  }
}

/* Send */
STATIC lagopus_result_t
ofp_group_desc_reply_create(struct channel *channel,
                            struct pbuf_list **pbuf_list,
                            struct group_desc_list *group_desc_list,
                            struct ofp_header *xid_header) {
  uint16_t tmp_length = 0;
  uint16_t length = 0;
  uint8_t *group_desc_head = NULL;
  uint16_t bucket_total_length = 0;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *pbuf = NULL;
  struct ofp_multipart_reply mp_reply;
  struct group_desc *group_desc = NULL;

  if (channel != NULL && pbuf_list != NULL &&
      group_desc_list != NULL && xid_header != NULL) {

    /* alloc */
    *pbuf_list = pbuf_list_alloc();

    if (*pbuf_list != NULL) {
      pbuf = pbuf_list_last_get(*pbuf_list);

      if (pbuf != NULL) {
        pbuf_plen_set(pbuf, pbuf_size_get(pbuf));

        /* Fill in header. */
        memset(&mp_reply, 0, sizeof(mp_reply));
        ofp_header_set(&mp_reply.header, channel_version_get(channel),
                       OFPT_MULTIPART_REPLY, tmp_length, xid_header->xid);

        mp_reply.type = OFPMP_GROUP_DESC;
        mp_reply.flags = 0;

        /* Encode multipart reply. */
        ret = ofp_multipart_reply_encode(pbuf, &mp_reply);

        if (ret == LAGOPUS_RESULT_OK) {
          if (TAILQ_EMPTY(group_desc_list) == false) {
            TAILQ_FOREACH(group_desc, group_desc_list, entry) {
              ret = ofp_group_desc_encode_list(*pbuf_list, &pbuf,
                                               &group_desc->ofp);
              if (ret == LAGOPUS_RESULT_OK) {
                group_desc_head = pbuf_putp_get(pbuf) - sizeof(struct ofp_group_desc);

                ret = ofp_bucket_list_encode(*pbuf_list, &pbuf,
                                             &group_desc->bucket_list,
                                             &bucket_total_length);
                if (ret != LAGOPUS_RESULT_OK) {
                  lagopus_msg_warning("FAILED (%s).\n",
                                      lagopus_error_get_string(ret));
                  break;
                } else {
                  ret = ofp_multipart_length_set(group_desc_head,
                                                 (uint16_t) (bucket_total_length +
                                                     sizeof(struct ofp_group_desc)));
                  if (ret != LAGOPUS_RESULT_OK) {
                    lagopus_msg_warning("FAILED (%s).\n",
                                        lagopus_error_get_string(ret));
                    break;
                  }
                }
              } else {
                lagopus_msg_warning("FAILED (%s).\n",
                                    lagopus_error_get_string(ret));
                break;
              }
            }
          } else {
            /* group_desc_list is empyt. */
            ret = LAGOPUS_RESULT_OK;
          }

          if (ret == LAGOPUS_RESULT_OK) {
            /* set length for last pbuf. */
            ret = pbuf_length_get(pbuf, &length);
            if (ret == LAGOPUS_RESULT_OK) {
              ret = ofp_header_length_set(pbuf, length);
              if (ret == LAGOPUS_RESULT_OK) {
                pbuf_plen_reset(pbuf);
                ret = LAGOPUS_RESULT_OK;
              } else {
                lagopus_msg_warning("FAILED (%s).\n",
                                    lagopus_error_get_string(ret));
              }
            } else {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(ret));
            }
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
        }
      } else {
        lagopus_msg_warning("Can't allocate pbuf.\n");
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      lagopus_msg_warning("Can't allocate pbuf_list.\n");
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }

    /* free. */
    if (ret != LAGOPUS_RESULT_OK && *pbuf_list != NULL) {
      pbuf_list_free(*pbuf_list);
      *pbuf_list = NULL;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* RECV */
/* Group Description Request packet receive. */
lagopus_result_t
ofp_group_desc_request_handle(struct channel *channel, struct pbuf *pbuf,
                              struct ofp_header *xid_header,
                              struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;
  struct pbuf_list *pbuf_list = NULL;
  struct group_desc_list group_desc_list;

  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    /* init list. */
    TAILQ_INIT(&group_desc_list);

    if (pbuf_plen_equal_check(pbuf, 0) == LAGOPUS_RESULT_OK) {
      dpid = channel_dpid_get(channel);

      ret = ofp_group_desc_get(dpid,
                               &group_desc_list,
                               error);
      if (ret == LAGOPUS_RESULT_OK) {
        ret = ofp_group_desc_reply_create(channel, &pbuf_list,
                                          &group_desc_list,
                                          xid_header);
        if (ret == LAGOPUS_RESULT_OK) {
          /* write packets. */
          ret = channel_send_packet_list(channel, pbuf_list);
          if (ret != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("Can't write\n");
          }
        } else {
          lagopus_msg_warning("FAILED (%s)\n", lagopus_error_get_string(ret));
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("over packet length.\n");
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }

    /* free. */
    group_desc_list_elem_free(&group_desc_list);
    if (pbuf_list != NULL) {
      pbuf_list_free(pbuf_list);
    }

  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
