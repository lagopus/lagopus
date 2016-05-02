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
#include "lagopus/flowdb.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/ofp_pdump.h"
#include "ofp_action.h"
#include "ofp_tlv.h"
#include "ofp_header_handler.h"
#include "ofp_padding.h"
#include "ofp_oxm.h"

#define OXM_LENGTH_INDEX 3
#define OXM_VALUE_INDEX 4

struct ed_prop *
ed_prop_alloc(void) {
  struct ed_prop *ed_prop;

  ed_prop = (struct ed_prop *) calloc(1, sizeof(struct ed_prop));

  return ed_prop;
}

void
ed_prop_list_free(struct ed_prop_list *ed_prop_list) {
  struct ed_prop *ed_prop;

  while (TAILQ_EMPTY(ed_prop_list) == false) {
    ed_prop = TAILQ_FIRST(ed_prop_list);
    TAILQ_REMOVE(ed_prop_list, ed_prop, entry);
    free(ed_prop);
  }
}

struct action *
action_alloc(uint16_t size) {
  struct action *action;

  action = (struct action *)calloc(1, sizeof(struct action) + size);
  if (action != NULL) {
    TAILQ_INIT(&action->ed_prop_list);
  }

  return action;
}

void
action_free(struct action *action) {
  if (action != NULL) {
    ed_prop_list_free(&action->ed_prop_list);
  }
  free(action);
}

void
ofp_action_list_trace(uint32_t flags,
                      struct action_list *action_list) {
  struct action *action;

  TAILQ_FOREACH(action, action_list, entry) {
    switch (action->ofpat.type) {
      case OFPAT_OUTPUT:
        lagopus_msg_pdump(flags, PDUMP_OFP_ACTION_OUTPUT,
                          *((struct ofp_action_output *)&action->ofpat), "");
        break;
      case OFPAT_COPY_TTL_OUT:
      case OFPAT_COPY_TTL_IN:
      case OFPAT_DEC_MPLS_TTL:
      case OFPAT_POP_VLAN:
      case OFPAT_DEC_NW_TTL:
      case OFPAT_POP_PBB:
        lagopus_msg_pdump(flags, PDUMP_OFP_ACTION_HEADER,
                          *((struct ofp_action_header *)&action->ofpat), "");
        break;
      case OFPAT_SET_MPLS_TTL:
        lagopus_msg_pdump(flags, PDUMP_OFP_ACTION_MPLS_TTL,
                          *((struct ofp_action_mpls_ttl *)&action->ofpat), "");
        break;
      case OFPAT_PUSH_VLAN:
      case OFPAT_PUSH_MPLS:
      case OFPAT_PUSH_PBB:
        lagopus_msg_pdump(flags, PDUMP_OFP_ACTION_PUSH,
                          *((struct ofp_action_push *)&action->ofpat), "");
        break;
      case OFPAT_POP_MPLS:
        lagopus_msg_pdump(flags, PDUMP_OFP_ACTION_POP_MPLS,
                          *((struct ofp_action_pop_mpls *)&action->ofpat), "");
        break;
      case OFPAT_SET_QUEUE:
        lagopus_msg_pdump(flags, PDUMP_OFP_ACTION_SET_QUEUE,
                          *((struct ofp_action_set_queue *)&action->ofpat), "");
        break;
      case OFPAT_GROUP:
        lagopus_msg_pdump(flags, PDUMP_OFP_ACTION_GROUP,
                          *((struct ofp_action_group *)&action->ofpat), "");
        break;
      case OFPAT_SET_NW_TTL:
        lagopus_msg_pdump(flags, PDUMP_OFP_ACTION_NW_TTL,
                          *((struct ofp_action_nw_ttl *)&action->ofpat), "");
        break;
      case OFPAT_SET_FIELD:
        lagopus_msg_pdump(flags, PDUMP_OFP_ACTION_SET_FIELD,
                          *((struct ofp_action_set_field *)&action->ofpat), "");
        break;
      case OFPAT_EXPERIMENTER:
        lagopus_msg_pdump(flags, PDUMP_OFP_ACTION_EXPERIMENTER_HEADER,
                          *((struct ofp_action_experimenter_header *)&action->ofpat), "");
        break;
      default:
        break;
    }
  }
}

/* Action header parse.  Used by OFPAT_COPY_TTL_OUT,
 * OFPAT_COPY_TTL_IN, OFPAT_DEC_MPLS_TTL, OFPAT_POP_VLAN,
 * OFPAT_DEC_NW_TTL, OFPAT_POP_PBB. */
static lagopus_result_t
ofp_action_header_parse_internal(struct action *action,
                                 struct pbuf *pbuf,
                                 struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_action_header *action_header;

  /* Set pointer to the action payload. */
  action_header = &action->ofpat;

  /* Decode the packet. */
  ret = ofp_action_header_decode(pbuf, action_header);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("bad action length.\n");
    ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
    return LAGOPUS_RESULT_OFP_ERROR;
  }

  /* Success. */
  return LAGOPUS_RESULT_OK;
}

/* Action output parse. */
static lagopus_result_t
ofp_action_output_parse(struct action *action,
                        struct pbuf *pbuf,
                        struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_action_output *action_output;

  /* Set pointer to the action payload. */
  action_output = (struct ofp_action_output *)&action->ofpat;

  /* Decode the packet. */
  ret = ofp_action_output_decode(pbuf, action_output);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("bad action length.\n");
    ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
    return LAGOPUS_RESULT_OFP_ERROR;
  }

  /* Success. */
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
ofp_action_push_parse(struct action *action,
                      struct pbuf *pbuf,
                      struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_action_push *action_push;

  /* Set pointer to the action payload. */
  action_push = (struct ofp_action_push *)&action->ofpat;

  /* Decode the packet. */
  ret = ofp_action_push_decode(pbuf, action_push);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("bad action length.\n");
    ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
    return LAGOPUS_RESULT_OFP_ERROR;
  }

  /* Success. */
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
ofp_action_pop_mpls_parse(struct action *action,
                          struct pbuf *pbuf,
                          struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_action_pop_mpls *action_pop_mpls;

  /* Set pointer to the action payload. */
  action_pop_mpls = (struct ofp_action_pop_mpls *)&action->ofpat;

  /* Decode the packet. */
  ret = ofp_action_pop_mpls_decode(pbuf, action_pop_mpls);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("bad action length.\n");
    ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
    return LAGOPUS_RESULT_OFP_ERROR;
  }

  /* Success. */
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
ofp_action_mpls_ttl_parse(struct action *action,
                          struct pbuf *pbuf,
                          struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_action_mpls_ttl *action_mpls_ttl;

  /* Set pointer to the action payload. */
  action_mpls_ttl = (struct ofp_action_mpls_ttl *)&action->ofpat;

  /* Decode the packet. */
  ret = ofp_action_mpls_ttl_decode(pbuf, action_mpls_ttl);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("bad action length.\n");
    ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
    return LAGOPUS_RESULT_OFP_ERROR;
  }

  /* Success. */
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
ofp_action_set_queue_parse(struct action *action,
                           struct pbuf *pbuf,
                           struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_action_set_queue *action_set_queue;

  /* Set pointer to the action payload. */
  action_set_queue = (struct ofp_action_set_queue *)&action->ofpat;

  /* Decode the packet. */
  ret = ofp_action_set_queue_decode(pbuf, action_set_queue);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("bad action length.\n");
    ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
    return LAGOPUS_RESULT_OFP_ERROR;
  }

  /* Success. */
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
ofp_action_group_parse(struct action *action,
                       struct pbuf *pbuf,
                       struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_action_group *action_group;

  /* Set pointer to the action payload. */
  action_group = (struct ofp_action_group *)&action->ofpat;

  /* Decode the packet. */
  ret = ofp_action_group_decode(pbuf, action_group);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("bad action length.\n");
    ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
    return LAGOPUS_RESULT_OFP_ERROR;
  }

  /* Success. */
  return LAGOPUS_RESULT_OK;
}

static lagopus_result_t
ofp_action_nw_ttl_parse(struct action *action,
                        struct pbuf *pbuf,
                        struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_action_nw_ttl *action_nw_ttl;

  /* Set pointer to the action payload. */
  action_nw_ttl = (struct ofp_action_nw_ttl *)&action->ofpat;

  /* Decode the packet. */
  ret = ofp_action_nw_ttl_decode(pbuf, action_nw_ttl);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("bad action length.\n");
    ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
    return LAGOPUS_RESULT_OFP_ERROR;
  }

  /* Success. */
  return LAGOPUS_RESULT_OK;
}

static bool
set_field_oxm_check(struct ofp_oxm *oxm) {
  bool ret = false;
  uint16_t oxm_class = (uint16_t)(((*((uint8_t *)oxm)) << 8) +
                                  *((uint8_t *)oxm + 1));

  /* check OXM type. */
  ret = ofp_oxm_field_check(oxm->oxm_field);

  /* check OXM class. (supported OFPXMC_OPENFLOW_BASIC.)*/
  if (ret == LAGOPUS_RESULT_OK &&
      oxm_class == OFPXMC_OPENFLOW_BASIC) {
    ret = true;
  } else {
    ret = false;
  }

  return ret;
}

static lagopus_result_t
ofp_action_set_field_check(struct pbuf *pbuf,
                           struct ofp_action_set_field *action_set_field,
                           struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_oxm *oxm;
  uint16_t pad_length;

  oxm = (struct ofp_oxm *) &action_set_field->field;
  pad_length = GET_64BIT_PADDING_LENGTH(oxm->oxm_length);

  /* check length.  */
  if ((oxm->oxm_length + pad_length) ==
      (action_set_field->len -  sizeof(struct ofp_action_set_field))) {
    ret = pbuf_plen_check(pbuf, oxm->oxm_length);

    if (ret == LAGOPUS_RESULT_OK) {
      /* check OXM. */
      if (set_field_oxm_check(oxm) == false) {
        lagopus_msg_warning("bad oxm.\n");
        ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_SET_TYPE);
        ret = LAGOPUS_RESULT_OFP_ERROR;
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
      ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_SET_LEN);
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    lagopus_msg_warning("bad set_field length.\n");
    ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_SET_LEN);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  }

  return ret;
}

static lagopus_result_t
ofp_action_set_field_parse(struct action *action,
                           struct pbuf *pbuf,
                           struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_action_set_field *action_set_field;
  uint8_t *oxm_value;

  /* Set pointer to the action payload. */
  action_set_field = (struct ofp_action_set_field *)&action->ofpat;

  /* Decode the packet. */
  ret = ofp_action_set_field_decode(pbuf, action_set_field);

  if (ret == LAGOPUS_RESULT_OK) {
    /* Decode remaining OXM payload. */
    /* oxm value. */
    oxm_value = &action_set_field->field[OXM_VALUE_INDEX];
    ret = ofp_action_set_field_check(pbuf,
                                     action_set_field,
                                     error);

    if (ret == LAGOPUS_RESULT_OK) {
      ret = pbuf_plen_check(pbuf, action_set_field->len -
                            sizeof(struct ofp_action_set_field));
      if (ret == LAGOPUS_RESULT_OK) {
        DECODE_GET(oxm_value, (size_t)(action_set_field->len -
                                       sizeof(struct ofp_action_set_field)));
        ret = LAGOPUS_RESULT_OK;
      } else {
        lagopus_msg_warning("bad set field length.\n");
        ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_SET_LEN );
        ret = LAGOPUS_RESULT_OFP_ERROR;
      }
    }
  } else {
    lagopus_msg_warning("bad action length.\n");
    ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  }

  /* Success. */
  return ret;
}

static lagopus_result_t
ofp_action_ed_prop_parse(struct pbuf *pbuf,
                         struct ed_prop_list *ed_prop_list,
                         struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ed_prop *ed_prop = NULL;
  struct ofp_tlv tlv;

  /* Decode TLV. */
  ret = ofp_tlv_decode_sneak(pbuf, &tlv);

  if (ret == LAGOPUS_RESULT_OK) {
    /* check length.*/
    ret = pbuf_plen_check(pbuf, (size_t) tlv.length);

    if (ret == LAGOPUS_RESULT_OK) {
      ed_prop = ed_prop_alloc();
      if (ed_prop != NULL) {
        /* insert */
        TAILQ_INSERT_TAIL(ed_prop_list, ed_prop, entry);

        switch (tlv.type) {
          case OFPPPT_PROP_PORT_NAME:
            ret = ofp_ed_prop_portname_decode(pbuf,
                                              &ed_prop->ofp_ed_prop_portname);
            break;
          case OFPPPT_PROP_NONE:
          case OFPPPT_PROP_EXPERIMENTER:
          default:
            lagopus_msg_warning("unsupported type (%"PRIu16").\n",
                                tlv.type);
            ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_TYPE);
            ret = LAGOPUS_RESULT_OFP_ERROR;
            break;
        }
      } else {
        lagopus_msg_warning("Can't allocate ed_prop.\n");
        ret = LAGOPUS_RESULT_NO_MEMORY;
      }
    } else {
      lagopus_msg_warning("bad ed_prop length.\n");
      ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_OFP_ERROR;
    ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
    lagopus_msg_warning("A ed_prop failed to decode.\n");
  }

  /* free. */
  if (ret != LAGOPUS_RESULT_OK) {
    ed_prop_list_free(ed_prop_list);
  }

  return ret;
}

static lagopus_result_t
ofp_action_encap_parse(struct action *action,
                       struct pbuf *pbuf,
                       struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_action_encap *action_encap;
  uint8_t *ed_prop = NULL;
  uint8_t *ed_prop_end = NULL;

  action_encap = (struct ofp_action_encap *)&action->ofpat;

  /* Decode the packet. */
  ret = ofp_action_encap_decode(pbuf, action_encap);
  if (ret == LAGOPUS_RESULT_OK) {
    ed_prop = pbuf_getp_get(pbuf);
    ed_prop_end = (uint8_t *) (ed_prop + action_encap->len -
                               sizeof(struct ofp_action_encap));
    while (pbuf_getp_get(pbuf) < ed_prop_end) {
      ret = ofp_action_ed_prop_parse(pbuf,
                                     &action->ed_prop_list,
                                     error);
      if (ret != LAGOPUS_RESULT_OK) {
        break;
      }
    }

    if (ret == LAGOPUS_RESULT_OK) {
      if (ed_prop_end != pbuf_getp_get(pbuf)) {
        lagopus_msg_warning("bad action length.\n");
        ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
        ret = LAGOPUS_RESULT_OFP_ERROR;
      }
    }
  } else {
    lagopus_msg_warning("bad action length.\n");
    ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  }

  return ret;
}

static lagopus_result_t
ofp_action_decap_parse(struct action *action,
                       struct pbuf *pbuf,
                       struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_action_decap *action_decap;
  uint8_t *ed_prop = NULL;
  uint8_t *ed_prop_end = NULL;

  action_decap = (struct ofp_action_decap *)&action->ofpat;

  /* Decode the packet. */
  ret = ofp_action_decap_decode(pbuf, action_decap);
  if (ret == LAGOPUS_RESULT_OK) {
    ed_prop = pbuf_getp_get(pbuf);
    ed_prop_end = (uint8_t *) (ed_prop + action_decap->len -
                               sizeof(struct ofp_action_decap));
    while (pbuf_getp_get(pbuf) < ed_prop_end) {
      ret = ofp_action_ed_prop_parse(pbuf,
                                     &action->ed_prop_list,
                                     error);
      if (ret != LAGOPUS_RESULT_OK) {
        break;
      }
    }

    if (ret == LAGOPUS_RESULT_OK) {
      if (ed_prop_end != pbuf_getp_get(pbuf)) {
        lagopus_msg_warning("bad action length.\n");
        ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
        ret = LAGOPUS_RESULT_OFP_ERROR;
      }
    }
  } else {
    lagopus_msg_warning("bad action length.\n");
    ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  }

  return ret;
}

/* Parse OFPIT_*_ACTIONS, put each action items into action_list. */
lagopus_result_t
ofp_action_parse(struct pbuf *pbuf, size_t action_length,
                 struct action_list *action_list,
                 struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_OK;
  uint8_t *ofpat;
  uint8_t *ofpat_end;
  uint8_t *action_end = NULL;
  struct action *action;
  (void) error;

  if (pbuf != NULL && action_list != NULL &&
      error != NULL) {
    /* ofp_action start and end pointer.  We are sure that size is big
     * enough to accommodate action_length. */
    ofpat = pbuf_getp_get(pbuf);
    ofpat_end = (uint8_t *)ofpat + action_length;

    /* Parse until pointer reaches to the end. */
    while (ofpat < ofpat_end) {
      /* TLV for ofp_action. */
      struct ofp_tlv tlv;

      /* Sneak preview of ofp_action type and length. */
      ret = ofp_tlv_decode_sneak(pbuf, &tlv);

      if (ret == LAGOPUS_RESULT_OK) {
        /* check action length.*/
        ret = pbuf_plen_check(pbuf, (size_t) tlv.length);
        if (ret == LAGOPUS_RESULT_OK) {
          action_end = pbuf_getp_get(pbuf) + tlv.length;

          /* Allocate a new action. */
          action = action_alloc(tlv.length);
          if (action != NULL) {
            /* Immediately add to linked list so that we can free up all of
             * allocated actions when error occurs. */
            TAILQ_INSERT_TAIL(action_list, action, entry);

            /* Action parse. */
            switch (tlv.type) {
              case OFPAT_OUTPUT:
                ret = ofp_action_output_parse(action, pbuf, error);
                break;
              case OFPAT_COPY_TTL_OUT:
                ret = ofp_action_header_parse_internal(action, pbuf, error);
                break;
              case OFPAT_COPY_TTL_IN:
                ret = ofp_action_header_parse_internal(action, pbuf, error);
                break;
              case OFPAT_SET_MPLS_TTL:
                ret = ofp_action_mpls_ttl_parse(action, pbuf, error);
                break;
              case OFPAT_DEC_MPLS_TTL:
                ret = ofp_action_header_parse_internal(action, pbuf, error);
                break;
              case OFPAT_PUSH_VLAN:
                ret = ofp_action_push_parse(action, pbuf, error);
                break;
              case OFPAT_POP_VLAN:
                ret = ofp_action_header_parse_internal(action, pbuf, error);
                break;
              case OFPAT_PUSH_MPLS:
                ret = ofp_action_push_parse(action, pbuf, error);
                break;
              case OFPAT_POP_MPLS:
                ret = ofp_action_pop_mpls_parse(action, pbuf, error);
                break;
              case OFPAT_SET_QUEUE:
                ret = ofp_action_set_queue_parse(action, pbuf, error);
                break;
              case OFPAT_GROUP:
                ret = ofp_action_group_parse(action, pbuf, error);
                break;
              case OFPAT_SET_NW_TTL:
                ret = ofp_action_nw_ttl_parse(action, pbuf, error);
                break;
              case OFPAT_DEC_NW_TTL:
                ret = ofp_action_header_parse_internal(action, pbuf, error);
                break;
              case OFPAT_SET_FIELD:
                ret = ofp_action_set_field_parse(action, pbuf, error);
                break;
              case OFPAT_PUSH_PBB:
                ret = ofp_action_push_parse(action, pbuf, error);
                break;
              case OFPAT_POP_PBB:
                ret = ofp_action_header_parse_internal(action, pbuf, error);
                break;
              case OFPAT_ENCAP:
                ret = ofp_action_encap_parse(action, pbuf, error);
                break;
              case OFPAT_DECAP:
                ret = ofp_action_decap_parse(action, pbuf, error);
                break;
              case OFPAT_EXPERIMENTER:
                lagopus_msg_warning("unsupported OFPAT_EXPERIMENTER.\n");
                ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_EXPERIMENTER);
                ret = LAGOPUS_RESULT_OFP_ERROR;
                break;
              default:
                lagopus_msg_warning("Not found action type (%"PRIu16").\n",
                                    tlv.type);
                ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_TYPE);
                ret = LAGOPUS_RESULT_OFP_ERROR;
                break;
            }

            if (ret == LAGOPUS_RESULT_OK) {
              if (action_end != NULL && action_end == pbuf_getp_get(pbuf)) {
                ofpat += tlv.length;
              } else {
                lagopus_msg_warning("bad action length.\n");
                ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
                ret = LAGOPUS_RESULT_OFP_ERROR;
                break;
              }
            } else {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(ret));
              break;
            }
          } else {
            ret =  LAGOPUS_RESULT_NO_MEMORY;
            break;
          }
        } else {
          lagopus_msg_warning("bad action length.\n");
          ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
          ret = LAGOPUS_RESULT_OFP_ERROR;
          break;
        }
      } else {
        lagopus_msg_warning("bad action length.\n");
        ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
        ret = LAGOPUS_RESULT_OFP_ERROR;
        break;
      }
    }

    if (ret != LAGOPUS_RESULT_OK) {
      /* When error occurs, free up all of the allocated actions. */
      while ((action = TAILQ_FIRST(action_list)) != NULL) {
        TAILQ_REMOVE(action_list, action, entry);
        action_free(action);
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static lagopus_result_t
ofp_set_field_encode(struct pbuf *pbuf, uint8_t *val, uint16_t length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  /* Size check. */
  ret = pbuf_plen_check(pbuf, length);
  if (ret == LAGOPUS_RESULT_OK) {
    /* Encode packet. */
    ENCODE_PUT(val, length);
  }

  return ret;
}

static lagopus_result_t
ofp_set_field_encode_list(struct pbuf_list *pbuf_list, struct pbuf **pbuf,
                          uint8_t *val, uint16_t length) {
  struct pbuf *before_pbuf;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (pbuf_list == NULL) {
    return ofp_set_field_encode(*pbuf, val, length);
  }

  *pbuf = pbuf_list_last_get(pbuf_list);
  if (*pbuf == NULL) {
    return LAGOPUS_RESULT_NO_MEMORY;
  }

  ret = ofp_set_field_encode(*pbuf, val, length);
  if (ret == LAGOPUS_RESULT_OUT_OF_RANGE) {
    before_pbuf = *pbuf;
    *pbuf = pbuf_alloc(OFP_PACKET_MAX_SIZE);
    if (*pbuf == NULL) {
      return LAGOPUS_RESULT_NO_MEMORY;
    }
    pbuf_plen_set(*pbuf, OFP_PACKET_MAX_SIZE);
    ret = ofp_header_mp_copy(*pbuf, before_pbuf);
    if (ret != LAGOPUS_RESULT_OK) {
      return ret;
    }
    pbuf_list_add(pbuf_list, *pbuf);
    ret = ofp_set_field_encode(*pbuf, val, length);
  }

  return ret;
}

static lagopus_result_t
ofp_action_list_set_field_encode(struct pbuf_list *pbuf_list,
                                 struct pbuf **pbuf,
                                 struct ofp_action_set_field *act_set_field) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t *set_field_head;
  uint8_t *oxm_value;
  uint8_t oxm_len;

  /* action length. */
  oxm_len = act_set_field->field[OXM_LENGTH_INDEX];
  act_set_field->len = (uint16_t) (sizeof(struct ofp_action_set_field) +
                                   oxm_len);
  /* oxm value. */
  oxm_value = &act_set_field->field[OXM_VALUE_INDEX];

  ret = ofp_action_set_field_encode_list(pbuf_list, pbuf, act_set_field);
  if (ret == LAGOPUS_RESULT_OK) {
    /* set_field head pointer. */
    set_field_head = pbuf_putp_get(*pbuf) - sizeof(struct ofp_action_set_field);

    /* Encode remaining OXM payload. */
    ret = ofp_set_field_encode_list(pbuf_list, pbuf, oxm_value,
                                    (uint16_t) (act_set_field->len -
                                        sizeof(struct ofp_action_set_field)));
    if (ret == LAGOPUS_RESULT_OK) {
      ret = ofp_padding_encode(pbuf_list, pbuf, &act_set_field->len);

      if (ret == LAGOPUS_RESULT_OK) {
        /* overwrite action length for padding. */
        ret = ofp_tlv_length_set(set_field_head, act_set_field->len);
        if (ret != LAGOPUS_RESULT_OK) {
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

  return ret;
}

static lagopus_result_t
ofp_action_list_action_ed_prop_encode(struct pbuf_list *pbuf_list,
                                      struct pbuf **pbuf,
                                      struct ed_prop *ed_prop,
                                      uint16_t *total_length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t *prop_head;
  uint16_t length = 0;

  switch (ed_prop->ofp_ed_prop.type) {
    case OFPPPT_PROP_PORT_NAME:
      ret = ofp_ed_prop_portname_encode_list(pbuf_list, pbuf,
                                             &ed_prop->ofp_ed_prop_portname);
      if (ret == LAGOPUS_RESULT_OK) {
        /* head pointer. */
        prop_head = pbuf_putp_get(*pbuf) - sizeof(struct ofp_ed_prop_portname);
        length = sizeof(struct ofp_ed_prop_portname);
      }
      break;
    case OFPPPT_PROP_NONE:
    case OFPPPT_PROP_EXPERIMENTER:
    default:
      ret = LAGOPUS_RESULT_OUT_OF_RANGE;
      break;
  }

  if (ret == LAGOPUS_RESULT_OK) {
    /* Sum length. And check overflow. */
    ret = ofp_tlv_length_sum(total_length, length);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = ofp_tlv_length_set(prop_head, length);
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("bad length (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("over prop_portname length (%s).\n",
                          lagopus_error_get_string(ret));
    }
  }

  return ret;
}

static lagopus_result_t
ofp_action_list_action_encap_encode(struct pbuf_list *pbuf_list,
                                    struct pbuf **pbuf,
                                    struct action *action) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ed_prop *ed_prop = NULL;
  struct ofp_action_encap *act_encap;
  uint16_t ed_prop_length = 0;
  uint8_t *encap_head;

  act_encap = (struct ofp_action_encap *)&(action->ofpat);

  ret = ofp_action_encap_encode_list(pbuf_list, pbuf, act_encap);
  if (ret == LAGOPUS_RESULT_OK) {
    /* head pointer. */
    encap_head = pbuf_putp_get(*pbuf) - sizeof(struct ofp_action_encap);

    if (TAILQ_EMPTY(&action->ed_prop_list) == false) {
      TAILQ_FOREACH(ed_prop, &action->ed_prop_list, entry) {
        ret = ofp_action_list_action_ed_prop_encode(pbuf_list,
                                                    pbuf,
                                                    ed_prop,
                                                    &ed_prop_length);
        if (ret != LAGOPUS_RESULT_OK) {
          lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
          break;
        }
      }
    } else {
      /* ed_prop_list is empty.*/
      ret =  LAGOPUS_RESULT_OK;
    }
  } else {
    lagopus_msg_warning("FAILED (%s).\n",
                        lagopus_error_get_string(ret));
  }

  if (ret == LAGOPUS_RESULT_OK) {
    /* Sum length. And check overflow. */
    ret = ofp_tlv_length_sum(&action->ofpat.len,
                             (uint16_t ) (sizeof(struct ofp_action_encap) +
                                          ed_prop_length));
    if (ret == LAGOPUS_RESULT_OK) {
      ret = ofp_tlv_length_set(encap_head, action->ofpat.len);
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("bad length (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("over action_encap) length (%s).\n",
                          lagopus_error_get_string(ret));
    }
  }

  return ret;
}

static lagopus_result_t
ofp_action_list_action_decap_encode(struct pbuf_list *pbuf_list,
                                    struct pbuf **pbuf,
                                    struct action *action) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ed_prop *ed_prop = NULL;
  struct ofp_action_decap *act_decap;
  uint16_t ed_prop_length = 0;
  uint8_t *decap_head;

  act_decap = (struct ofp_action_decap *)&(action->ofpat);

  ret = ofp_action_decap_encode_list(pbuf_list, pbuf, act_decap);
  if (ret == LAGOPUS_RESULT_OK) {
    /* head pointer. */
    decap_head = pbuf_putp_get(*pbuf) - sizeof(struct ofp_action_decap);

    if (TAILQ_EMPTY(&action->ed_prop_list) == false) {
      TAILQ_FOREACH(ed_prop, &action->ed_prop_list, entry) {
        ret = ofp_action_list_action_ed_prop_encode(pbuf_list,
                                                    pbuf,
                                                    ed_prop,
                                                    &ed_prop_length);
        if (ret != LAGOPUS_RESULT_OK) {
          lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
          break;
        }
      }
    } else {
      /* ed_prop_list is empty.*/
      ret =  LAGOPUS_RESULT_OK;
    }
  } else {
    lagopus_msg_warning("FAILED (%s).\n",
                        lagopus_error_get_string(ret));
  }

  if (ret == LAGOPUS_RESULT_OK) {
    /* Sum length. And check overflow. */
    ret = ofp_tlv_length_sum(&action->ofpat.len,
                             (uint16_t ) (sizeof(struct ofp_action_decap) +
                                          ed_prop_length));
    if (ret == LAGOPUS_RESULT_OK) {
      ret = ofp_tlv_length_set(decap_head, action->ofpat.len);
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("bad length (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("over action_decap) length (%s).\n",
                          lagopus_error_get_string(ret));
    }
  }

  return ret;
}

lagopus_result_t
ofp_action_list_encode(struct pbuf_list *pbuf_list,
                       struct pbuf **pbuf,
                       struct action_list *action_list,
                       uint16_t *total_length) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct action *action;

  /* NULL check. pbuf_list allows NULL. */
  if (pbuf != NULL && action_list != NULL &&
      total_length != NULL) {
    *total_length = 0;
    if (TAILQ_EMPTY(action_list) == false) {
      TAILQ_FOREACH(action, action_list, entry) {
        switch (action->ofpat.type) {
          case OFPAT_OUTPUT:
            action->ofpat.len = (uint16_t) (sizeof(struct ofp_action_output));
            res = ofp_action_output_encode_list(
                    pbuf_list, pbuf, (struct ofp_action_output *)&(action->ofpat));
            break;
          case OFPAT_COPY_TTL_OUT:
          case OFPAT_COPY_TTL_IN:
          case OFPAT_DEC_MPLS_TTL:
          case OFPAT_POP_VLAN:
          case OFPAT_DEC_NW_TTL:
          case OFPAT_POP_PBB:
            action->ofpat.len = (uint16_t) (sizeof(struct ofp_action_header));
            res = ofp_action_header_encode_list(
                    pbuf_list, pbuf, (struct ofp_action_header *)&(action->ofpat));
            break;
          case OFPAT_SET_MPLS_TTL:
            action->ofpat.len = (uint16_t) (sizeof(struct ofp_action_mpls_ttl));
            res = ofp_action_mpls_ttl_encode_list(
                    pbuf_list, pbuf, (struct ofp_action_mpls_ttl *)&(action->ofpat));
            break;
          case OFPAT_PUSH_VLAN:
          case OFPAT_PUSH_MPLS:
          case OFPAT_PUSH_PBB:
            action->ofpat.len = (uint16_t) (sizeof(struct ofp_action_push));
            res = ofp_action_push_encode_list(
                    pbuf_list, pbuf, (struct ofp_action_push *)&(action->ofpat));
            break;
          case OFPAT_POP_MPLS:
            action->ofpat.len = (uint16_t) (sizeof(struct ofp_action_pop_mpls));
            res = ofp_action_pop_mpls_encode_list(
                    pbuf_list, pbuf, (struct ofp_action_pop_mpls *)&(action->ofpat));
            break;
          case OFPAT_SET_QUEUE:
            action->ofpat.len = (uint16_t) (sizeof(struct ofp_action_set_queue));
            res = ofp_action_set_queue_encode_list(
                    pbuf_list, pbuf, (struct ofp_action_set_queue *)&(action->ofpat));
            break;
          case OFPAT_GROUP:
            action->ofpat.len = (uint16_t) (sizeof(struct ofp_action_group));
            res = ofp_action_group_encode_list(
                    pbuf_list, pbuf, (struct ofp_action_group *)&(action->ofpat));
            break;
          case OFPAT_SET_NW_TTL:
            action->ofpat.len = (uint16_t) (sizeof(struct ofp_action_nw_ttl));
            res = ofp_action_nw_ttl_encode_list(
                    pbuf_list, pbuf, (struct ofp_action_nw_ttl *)&(action->ofpat));
            break;
          case OFPAT_SET_FIELD:
            res = ofp_action_list_set_field_encode(
                    pbuf_list, pbuf, (struct ofp_action_set_field *)&(action->ofpat));
            break;
          case OFPAT_ENCAP:
            action->ofpat.len = 0;
            res = ofp_action_list_action_encap_encode(
                pbuf_list, pbuf, action);
            break;
          case OFPAT_DECAP:
            action->ofpat.len = 0;
            res = ofp_action_list_action_decap_encode(
                pbuf_list, pbuf, action);
            break;
          case OFPAT_EXPERIMENTER:
            break;
          default:
            res = LAGOPUS_RESULT_OUT_OF_RANGE;
            break;
        }

        if (res == LAGOPUS_RESULT_OK) {
          /* Sum length. And check overflow. */
          res = ofp_tlv_length_sum(total_length, action->ofpat.len);
          if (res != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("over action length (%s).\n",
                                lagopus_error_get_string(res));
            break;
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(res));
          break;
        }
      }
    } else {
      res = LAGOPUS_RESULT_OK;
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return res;
}

/* parse action header for ofp_table_feature. */
lagopus_result_t
ofp_action_header_parse(struct pbuf *pbuf, size_t action_length,
                        struct action_list *action_list,
                        struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_OK;
  uint8_t *ofpat;
  uint8_t *ofpat_end;
  struct action *action;
  (void) error;

  if (pbuf != NULL && action_list != NULL &&
      error != NULL) {
    /* ofp_action start and end pointer.  We are sure that size is big
     * enough to accommodate action_length. */
    ofpat = pbuf_getp_get(pbuf);
    ofpat_end = (uint8_t *)ofpat + action_length;

    /* Parse until pointer reaches to the end. */
    while (ofpat < ofpat_end) {
      /* TLV for ofp_action. */
      struct ofp_tlv tlv;

      /* Sneak preview of ofp_action type and length. */
      ret = ofp_tlv_decode_sneak(pbuf, &tlv);

      if (ret == LAGOPUS_RESULT_OK) {
        /* check action length.*/
        ret = pbuf_plen_check(pbuf, (size_t) tlv.length);
        if (ret == LAGOPUS_RESULT_OK) {
          /* Allocate a new action. */
          action = action_alloc(tlv.length);
          if (action != NULL) {
            /* Immediately add to linked list so that we can free up all of
             * allocated actions when error occurs. */
            TAILQ_INSERT_TAIL(action_list, action, entry);

            /* Action parse. */
            switch (tlv.type) {
              case OFPAT_OUTPUT:
              case OFPAT_COPY_TTL_OUT:
              case OFPAT_COPY_TTL_IN:
              case OFPAT_SET_MPLS_TTL:
              case OFPAT_DEC_MPLS_TTL:
              case OFPAT_PUSH_VLAN:
              case OFPAT_POP_VLAN:
              case OFPAT_PUSH_MPLS:
              case OFPAT_POP_MPLS:
              case OFPAT_SET_QUEUE:
              case OFPAT_GROUP:
              case OFPAT_SET_NW_TTL:
              case OFPAT_DEC_NW_TTL:
              case OFPAT_SET_FIELD:
              case OFPAT_PUSH_PBB:
              case OFPAT_POP_PBB:
              case OFPAT_ENCAP:
              case OFPAT_DECAP:
                ret = ofp_action_header_parse_internal(action, pbuf, error);
                break;
              case OFPAT_EXPERIMENTER:
                lagopus_msg_warning("unsupported OFPAT_EXPERIMENTER.\n");
                ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_EXPERIMENTER);
                ret = LAGOPUS_RESULT_OFP_ERROR;
                break;
              default:
                lagopus_msg_warning("Not found action type (%"PRIu16").\n",
                                    tlv.type);
                ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_TYPE);
                ret = LAGOPUS_RESULT_OFP_ERROR;
                break;
            }

            if (ret == LAGOPUS_RESULT_OK) {
              ofpat += tlv.length;
            } else {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(ret));
              break;
            }
          } else {
            ret =  LAGOPUS_RESULT_NO_MEMORY;
            break;
          }
        } else {
          lagopus_msg_warning("bad action length.\n");
          ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
          ret = LAGOPUS_RESULT_OFP_ERROR;
          break;
        }
      } else {
        lagopus_msg_warning("bad action length.\n");
        ofp_error_set(error, OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
        ret = LAGOPUS_RESULT_OFP_ERROR;
        break;
      }
    }

    if (ret != LAGOPUS_RESULT_OK) {
      /* When error occurs, free up all of the allocated actions. */
      while ((action = TAILQ_FIRST(action_list)) != NULL) {
        TAILQ_REMOVE(action_list, action, entry);
        action_free(action);
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* encode action header for ofp_table_feature. */
lagopus_result_t
ofp_action_header_list_encode(struct pbuf_list *pbuf_list,
                              struct pbuf **pbuf,
                              struct action_list *action_list,
                              uint16_t *total_length) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct action *action;

  /* NULL check. pbuf_list allows NULL. */
  if (pbuf != NULL && action_list != NULL &&
      total_length != NULL) {
    *total_length = 0;
    if (TAILQ_EMPTY(action_list) == false) {
      TAILQ_FOREACH(action, action_list, entry) {
        switch (action->ofpat.type) {
          case OFPAT_OUTPUT:
          case OFPAT_COPY_TTL_OUT:
          case OFPAT_COPY_TTL_IN:
          case OFPAT_DEC_MPLS_TTL:
          case OFPAT_POP_VLAN:
          case OFPAT_DEC_NW_TTL:
          case OFPAT_POP_PBB:
          case OFPAT_SET_MPLS_TTL:
          case OFPAT_PUSH_VLAN:
          case OFPAT_PUSH_MPLS:
          case OFPAT_PUSH_PBB:
          case OFPAT_POP_MPLS:
          case OFPAT_SET_QUEUE:
          case OFPAT_GROUP:
          case OFPAT_SET_NW_TTL:
          case OFPAT_SET_FIELD:
          case OFPAT_ENCAP:
          case OFPAT_DECAP:
            action->ofpat.len = (uint16_t) (sizeof(struct ofp_action_header));
            res = ofp_action_header_encode_list(
                    pbuf_list, pbuf, (struct ofp_action_header *)&(action->ofpat));
            break;
          case OFPAT_EXPERIMENTER:
            break;
          default:
            res = LAGOPUS_RESULT_OUT_OF_RANGE;
            break;
        }

        if (res == LAGOPUS_RESULT_OK) {
          /* Sum length. And check overflow. */
          res = ofp_tlv_length_sum(total_length, action->ofpat.len);
          if (res != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("over action length (%s).\n",
                                lagopus_error_get_string(res));
            break;
          }
        } else {
          lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(res));
          break;
        }
      }
    } else {
      res = LAGOPUS_RESULT_OK;
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return res;
}
