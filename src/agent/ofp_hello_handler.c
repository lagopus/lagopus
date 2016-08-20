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
#include <sys/queue.h>
#include "lagopus_apis.h"
#include "openflow.h"
#include "openflow13packet.h"
#include "ofp_apis.h"
#include "ofp_tlv.h"
#include "ofp_element.h"
#include "lagopus/ofp_handler.h"
#include "lagopus/bridge.h"
#include "lagopus/ofp_pdump.h"

#define BITMAP_TL_SIZE 4
#define BITMAP_SIZE 32

static void
ofp_hello_tarce(struct ofp_hello *hello) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_HELLO)) {
    lagopus_msg_pdump(TRACE_OFPT_HELLO, PDUMP_OFP_HEADER,
                      hello->header, "");
  }
}

/* RECV */
static bool
check_versions(uint8_t bridge_ofp_version,
               uint8_t *support_version,
               struct element_list *element_list) {
  struct element *element;
  struct bitmap *bitmap;
  uint32_t bit = 1;
  int i;

  TAILQ_FOREACH(element, element_list, entry) {
    if (element->type == OFPHET_VERSIONBITMAP) {
      i = 1;
      TAILQ_FOREACH(bitmap, &element->bitmap_list, entry) {
        if (bridge_ofp_version < (BITMAP_SIZE * i)) {
          if ((bitmap->bitmap & (bit << bridge_ofp_version)) != 0) {
            *support_version = bridge_ofp_version;
            return true;
          }
          i++;
        }
      }
    }
  }
  return false;
}

/* Hello packet receive. */
lagopus_result_t
ofp_hello_handle(struct channel *channel, struct pbuf *pbuf,
                 struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t support_ofp_version = OPENFLOW_VERSION_0_0;
  uint8_t bridge_ofp_version;
  uint64_t dpid;
  struct ofp_hello hello;
  struct element_list element_list;

  if (channel != NULL && pbuf != NULL &&
      error != NULL) {

    /* Init list. */
    TAILQ_INIT(&element_list);
    /* Parse packet. */
    ret = ofp_hello_decode(pbuf, &hello);
    if (ret == LAGOPUS_RESULT_OK) {
      while (pbuf_plen_get(pbuf) > 0) {
        ret = ofp_element_parse(pbuf, &element_list, error);
        if (ret != LAGOPUS_RESULT_OK) {
          lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
          break;
        }
      }

      if (ret == LAGOPUS_RESULT_OK) {
        dpid = channel_dpid_get(channel);

        ret = ofp_switch_version_get(dpid, &bridge_ofp_version);

        if (ret == LAGOPUS_RESULT_OK) {
          if (hello.header.version != bridge_ofp_version) {
            if (check_versions(bridge_ofp_version,
                               &support_ofp_version, &element_list) == false) {
              lagopus_msg_warning("unsupported version.\n");
              ret = LAGOPUS_RESULT_OFP_ERROR;
              ofp_error_set(error, OFPET_HELLO_FAILED, OFPHFC_INCOMPATIBLE);
            } else {
              ret = LAGOPUS_RESULT_OK;
            }
          } else {
            support_ofp_version = bridge_ofp_version;
            ret = LAGOPUS_RESULT_OK;
          }

          if (ret == LAGOPUS_RESULT_OK) {
            /* dump trace.*/
            ofp_hello_tarce(&hello);

            /* set ofp version in channel. */
            channel_version_set(channel, support_ofp_version);

            /* Hello is received. */
            channel_hello_received_set(channel);
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
        }
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
      ret = LAGOPUS_RESULT_OFP_ERROR;
      ofp_error_set(error, OFPET_HELLO_FAILED, OFPHFC_INCOMPATIBLE);
    }
    /* free. */
    element_list_elem_free(&element_list);
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* SEND */
static lagopus_result_t
vbitmap_elem_create(uint64_t dpid,
                    struct element_list *element_list) {
  uint16_t tmp_length = 0;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct element *element;
  struct bitmap *bitmap = NULL;

  if (element_list != NULL) {
    element = element_alloc();

    if (element != NULL) {
      TAILQ_INSERT_TAIL(element_list, element, entry);

      element->type = OFPHET_VERSIONBITMAP;
      element->length = tmp_length;

      bitmap = bitmap_alloc();
      if (bitmap != NULL) {
        TAILQ_INSERT_TAIL(&element->bitmap_list, bitmap, entry);
        /* set ofp version. */
        ret = ofp_switch_version_bitmap_get(dpid, &bitmap->bitmap);

        if (ret != LAGOPUS_RESULT_OK) {
          lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
        }
      } else {
        lagopus_msg_warning("Can't alloc bitmap.\n");
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      lagopus_msg_warning("Can't alloc element.\n");
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }

    /* free. */
    if (ret != LAGOPUS_RESULT_OK) {
      element_list_elem_free(element_list);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* SEND */
/* Called with channel locked context */
/* Assumed channed locked. */
STATIC lagopus_result_t
ofp_hello_create(struct channel *channel,
                 struct pbuf **pbuf) {
  uint8_t bridge_ofp_version;
  uint16_t temp_length = 0;
  uint16_t length = 0;
  uint8_t *header_head;
  uint64_t dpid;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_hello hello;
  struct element_list element_list;

  if (channel != NULL && pbuf != NULL) {
    /* Init list. */
    TAILQ_INIT(&element_list);

    dpid = channel_dpid_get_nolock(channel);
    ret = ofp_switch_version_get(dpid, &bridge_ofp_version);

    if (ret == LAGOPUS_RESULT_OK) {
      channel_version_set_nolock(channel, bridge_ofp_version);

      *pbuf = channel_pbuf_list_get_nolock(channel, OFP_PACKET_MAX_SIZE);

      if (*pbuf != NULL) {
        header_head = pbuf_getp_get(*pbuf);
        pbuf_plen_set(*pbuf, OFP_PACKET_MAX_SIZE);

        /* Fill in header. */
        ofp_header_set(&hello.header,
                       channel_version_get_nolock(channel),
                       OFPT_HELLO,
                       temp_length,
                       channel_xid_get_nolock(channel));

        ret = ofp_hello_encode(*pbuf, &hello);

        if (ret == LAGOPUS_RESULT_OK) {
          /* create version bitmap.*/
          ret = vbitmap_elem_create(dpid, &element_list);

          if (ret == LAGOPUS_RESULT_OK) {
            ret = ofp_element_list_encode(*pbuf, &element_list);

            if (ret == LAGOPUS_RESULT_OK) {
              ret = pbuf_length_get(*pbuf, &length);
              if (ret == LAGOPUS_RESULT_OK) {
                ret = ofp_tlv_length_set(header_head, length);
                if (ret == LAGOPUS_RESULT_OK) {
                  pbuf_plen_reset(*pbuf);
                } else {
                  lagopus_msg_warning("FAILED (%s).\n",
                                      lagopus_error_get_string(ret));
                }
              } else {
                lagopus_msg_warning("FAILED (%s).\n",
                                    lagopus_error_get_string(ret));
              }
            } else {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(ret));
            }
          } else {
            lagopus_msg_warning("FAILED (%s).\n",
                                lagopus_error_get_string(ret));
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
        }
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
        lagopus_msg_warning("Can't allocate pbuf.\n");
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
    }

    /* free. */
    element_list_elem_free(&element_list);

    if (ret != LAGOPUS_RESULT_OK && *pbuf != NULL) {
      channel_pbuf_list_unget_nolock(channel, *pbuf);
      *pbuf = NULL;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* Hello packet send. */
/* assume channel locked. */
lagopus_result_t
ofp_hello_send(struct channel *channel) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct pbuf *pbuf = NULL;

  if (channel != NULL) {
    ret = ofp_hello_create(channel, &pbuf);

    if (ret == LAGOPUS_RESULT_OK) {
      /* send. */
      channel_send_packet_by_event_nolock(channel, pbuf);

      /* Success. */
      ret = LAGOPUS_RESULT_OK;
    }

    if (ret != LAGOPUS_RESULT_OK && pbuf != NULL) {
      channel_pbuf_list_unget_nolock(channel, pbuf);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
