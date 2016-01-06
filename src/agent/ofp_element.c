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
#include "ofp_tlv.h"
#include "ofp_padding.h"
#include "ofp_element.h"

struct bitmap *
bitmap_alloc(void) {
  struct bitmap *bitmap;

  bitmap = (struct bitmap *)
           calloc(1, sizeof(struct bitmap));

  return bitmap;
}

void
bitmap_list_elem_free(struct bitmap_list *bitmap_list) {
  struct bitmap *bitmap;

  while (TAILQ_EMPTY(bitmap_list) == false) {
    bitmap = TAILQ_FIRST(bitmap_list);
    TAILQ_REMOVE(bitmap_list, bitmap, entry);
    free(bitmap);
  }
}

struct element *
element_alloc(void) {
  struct element *element;

  element = (struct element *)
            calloc(1, sizeof(struct element));
  TAILQ_INIT(&element->bitmap_list);

  return element;
}

void
element_list_elem_free(struct element_list *element_list) {
  struct element *element;

  while (TAILQ_EMPTY(element_list) == false) {
    element = TAILQ_FIRST(element_list);
    if (TAILQ_EMPTY(&element->bitmap_list) == false) {
      bitmap_list_elem_free(&element->bitmap_list);
    }
    TAILQ_REMOVE(element_list, element, entry);
    free(element);
  }
}

static lagopus_result_t
ofp_vbitmap_decode(struct pbuf *pbuf, struct bitmap *bitmap) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* Size check. */
  ret = pbuf_plen_check(pbuf, sizeof(uint32_t));
  if (ret == LAGOPUS_RESULT_OK) {
    DECODE_GETL(bitmap->bitmap);
  }

  return ret;
}

static lagopus_result_t
ofp_vbitmap_encode(struct pbuf *pbuf, struct bitmap *bitmap) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* Size check. */
  ret = pbuf_plen_check(pbuf, sizeof(uint32_t));
  if (ret == LAGOPUS_RESULT_OK) {
    ENCODE_PUTL(bitmap->bitmap);
  }

  return ret;
}

static lagopus_result_t
versionbitmap_parse(struct pbuf *pbuf,
                    struct element *element,
                    struct ofp_error *error) {
  int32_t length;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_hello_elem_versionbitmap vbitmap;
  struct bitmap *bitmap = NULL;

  ret = ofp_hello_elem_versionbitmap_decode(pbuf, &vbitmap);

  if (ret == LAGOPUS_RESULT_OK) {
    element->type = vbitmap.type;
    element->length = vbitmap.length;
    length = (int32_t) (vbitmap.length - sizeof(struct ofp_hello_elem_header));

    while (length > 0) {
      bitmap = bitmap_alloc();

      if (bitmap != NULL) {
        TAILQ_INSERT_TAIL(&element->bitmap_list, bitmap, entry);
        ret = ofp_vbitmap_decode(pbuf, bitmap);
        if (ret == LAGOPUS_RESULT_OK) {
          length = length - (int32_t) sizeof(uint32_t);
        } else {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
          ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
          ret = LAGOPUS_RESULT_OFP_ERROR;
          break;
        }
      } else {
        ret = LAGOPUS_RESULT_NO_MEMORY;
        break;
      }
    }
  } else {
    lagopus_msg_warning("FAILED (%s).\n",
                        lagopus_error_get_string(ret));
    ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  }

  /* free. */
  if (ret != LAGOPUS_RESULT_OK) {
    bitmap_list_elem_free(&element->bitmap_list);
  }

  return ret;
}

lagopus_result_t
ofp_element_parse(struct pbuf *pbuf,
                  struct element_list *element_list,
                  struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_result_t tmp_ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t padding;
  struct element *element = NULL;
  struct ofp_tlv tlv;

  if (pbuf != NULL && element_list != NULL &&
      error != NULL) {
    /* Decode TLV. */
    ret = ofp_tlv_decode_sneak(pbuf, &tlv);

    if (ret == LAGOPUS_RESULT_OK) {
      if (tlv.length == sizeof(struct ofp_tlv)) {
        if (pbuf_plen_get(pbuf) == 0) {
          ret = LAGOPUS_RESULT_OK;
        } else {
          lagopus_msg_warning("FAILED : but tlv length.\n");
          ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
          ret = LAGOPUS_RESULT_OFP_ERROR;
        }
      } else if (tlv.length == 0) {
        lagopus_msg_warning("FAILED : but tlv length.\n");
        ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
        ret = LAGOPUS_RESULT_OFP_ERROR;
      } else {
        padding = GET_64BIT_PADDING_LENGTH(tlv.length);
        switch (tlv.type) {
          case OFPHET_VERSIONBITMAP:
            element = element_alloc();
            if (element != NULL) {
              TAILQ_INSERT_TAIL(element_list, element, entry);
              ret = versionbitmap_parse(pbuf, element, error);
            } else {
              ret = LAGOPUS_RESULT_NO_MEMORY;
            }
            /* Size check. */
            tmp_ret = pbuf_forward(pbuf, padding);
            if (tmp_ret != LAGOPUS_RESULT_OK && ret == LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(tmp_ret));
              if (tmp_ret == LAGOPUS_RESULT_OUT_OF_RANGE) {
                ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
                ret = LAGOPUS_RESULT_OFP_ERROR;
              }
            }
            break;
          default:
            /* Size check. */
            ret = pbuf_forward(pbuf, (size_t) (tlv.length + padding));
            if (ret != LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(ret));
              if (ret == LAGOPUS_RESULT_OUT_OF_RANGE) {
                ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
                ret = LAGOPUS_RESULT_OFP_ERROR;
              }
            }
            break;
        }
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }

    /* free */
    if (ret != LAGOPUS_RESULT_OK) {
      /* Not check return val from pbuf_forward. */
      (void) pbuf_forward(pbuf, pbuf_plen_get(pbuf));
      element_list_elem_free(element_list);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
versionbitmap_list_encode(struct pbuf *pbuf,
                          struct bitmap_list *bitmap_list,
                          uint16_t *total_length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct bitmap *bitmap;

  if (pbuf != NULL && bitmap_list != NULL) {
    *total_length = 0;
    if (TAILQ_EMPTY(bitmap_list) == false) {
      TAILQ_FOREACH(bitmap, bitmap_list, entry) {
        ret = ofp_vbitmap_encode(pbuf, bitmap);
        if (ret == LAGOPUS_RESULT_OK) {
          /* Sum length. And check overflow. */
          ret = ofp_tlv_length_sum(total_length,
                                   sizeof(uint32_t));

          if (ret != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("over versionbitmap length (%s).\n",
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
      /* bitmap_list is empty. */
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_element_list_encode(struct pbuf *pbuf,
                        struct element_list *element_list) {
  uint16_t element_len = 0;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t *element_head;
  struct element *element;
  struct ofp_hello_elem_versionbitmap vbitmap;

  if (pbuf != NULL && element_list != NULL) {
    if (TAILQ_EMPTY(element_list) == false) {
      TAILQ_FOREACH(element, element_list, entry) {
        /* element header pointer. */
        element_head = pbuf_putp_get(pbuf);

        vbitmap.type = element->type;
        vbitmap.length = element->length;
        ret = ofp_hello_elem_versionbitmap_encode(pbuf, &vbitmap);

        if (ret == LAGOPUS_RESULT_OK) {
          switch (element->type) {
            case OFPHET_VERSIONBITMAP:
              ret = versionbitmap_list_encode(pbuf, &element->bitmap_list,
                                              &element_len);
              if (ret == LAGOPUS_RESULT_OK) {
                ret = ofp_tlv_length_sum(&element_len,
                                         sizeof(struct ofp_hello_elem_header));
                if (ret == LAGOPUS_RESULT_OK) {
                  ret = ofp_tlv_length_set(element_head, element_len);
                  if (ret == LAGOPUS_RESULT_OK) {
                    ret = ofp_padding_encode(NULL, &pbuf, &element_len);
                    if (ret != LAGOPUS_RESULT_OK) {
                      lagopus_msg_warning("FAILED (%s).\n",
                                          lagopus_error_get_string(ret));
                      break;
                    }
                  } else {
                    lagopus_msg_warning("FAILED (%s).\n",
                                        lagopus_error_get_string(ret));
                    break;
                  }
                } else {
                  lagopus_msg_warning("FAILED (%s).\n",
                                      lagopus_error_get_string(ret));
                  break;
                }
              } else {
                lagopus_msg_warning("FAILED (%s).\n",
                                    lagopus_error_get_string(ret));
                break;
              }
              break;
            default:
              break;
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
          break;
        }
      }
    } else {
      /* lement_list is empty. */
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
