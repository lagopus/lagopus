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
#include "ofp_tlv.h"
#include "ofp_instruction.h"
#include "ofp_action.h"
#include "ofp_padding.h"
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/pbuf.h"

static struct next_table_id *
s_next_table_id_alloc(void) {
  struct next_table_id *next_table_id = NULL;
  next_table_id = (struct next_table_id *) calloc(1,
                  sizeof(struct next_table_id));
  return next_table_id;
}

static void
next_table_id_list_entry_free(struct next_table_id_list *next_table_id_list) {
  struct next_table_id *next_table_id;

  while ((next_table_id = TAILQ_FIRST(next_table_id_list)) != NULL) {
    TAILQ_REMOVE(next_table_id_list, next_table_id, entry);
    free(next_table_id);
  }
}

static lagopus_result_t
s_prop_next_tables_parse(struct pbuf *pbuf,
                         uint64_t byte,
                         struct next_table_id_list *next_table_id_list,
                         struct ofp_error *error) {
  uint64_t read_byte = 0;
  lagopus_result_t ret = LAGOPUS_RESULT_OK;
  struct next_table_id *id = NULL;

  while (read_byte < byte) {
    id = s_next_table_id_alloc();
    if (id == NULL) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      break;
    } else {
      ret = ofp_next_table_id_decode(pbuf, &id->ofp);
      if (ret == LAGOPUS_RESULT_OK) {
        read_byte += sizeof(uint8_t);
        TAILQ_INSERT_TAIL(next_table_id_list, id, entry);
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
        ofp_error_set(error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);
        ret = LAGOPUS_RESULT_OFP_ERROR;
        free(id);
        break;
      }
    }
  }

  if (ret != LAGOPUS_RESULT_OFP_ERROR && read_byte != byte) {
    lagopus_msg_warning("bad length.\n");
    ofp_error_set(error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  }

  return ret;
}

static lagopus_result_t
s_prop_next_tables_list_encode(struct pbuf_list *pbuf_list,
                               struct pbuf **pbuf,
                               const struct next_table_id_list *next_table_id_list,
                               uint16_t *total_length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const struct next_table_id *next_table_id = NULL;

  if (pbuf_list != NULL && pbuf != NULL &&
      next_table_id_list != NULL) {
    *total_length = 0;
    if (TAILQ_EMPTY(next_table_id_list) == false) {
      /* Encode packet. */
      TAILQ_FOREACH(next_table_id, next_table_id_list, entry) {
        ret = ofp_next_table_id_encode_list(pbuf_list, pbuf,
                                            &next_table_id->ofp);

        if (ret == LAGOPUS_RESULT_OK) {
          /* Sum length. And check overflow. */
          ret = ofp_tlv_length_sum(total_length,
                                   sizeof(struct ofp_next_table_id));
          if (ret != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("over next_table_id length (%s).\n",
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
      /* next_table_id_list is empty. */
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static struct oxm_id *
s_oxm_id_alloc(void) {
  struct oxm_id *oxm_id = NULL;
  oxm_id = (struct oxm_id *) calloc(1, sizeof(struct oxm_id));
  return oxm_id;
}

static void
oxm_id_list_entry_free(struct oxm_id_list *oxm_id_list) {
  struct oxm_id *oxm_id;

  while ((oxm_id = TAILQ_FIRST(oxm_id_list)) != NULL) {
    TAILQ_REMOVE(oxm_id_list, oxm_id, entry);
    free(oxm_id);
  }
}

static lagopus_result_t
s_table_feature_prop_oxm_parse(struct pbuf *pbuf,
                               uint64_t byte,
                               struct oxm_id_list *oxm_id_list,
                               struct ofp_error *error) {
  uint64_t read_byte = 0;
  lagopus_result_t ret = LAGOPUS_RESULT_OK;
  struct oxm_id *oxm_id = NULL;

  while (read_byte < byte) {
    oxm_id = s_oxm_id_alloc();
    if (oxm_id == NULL) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      break;
    } else {
      ret = ofp_oxm_id_decode(pbuf, &oxm_id->ofp);
      if (ret == LAGOPUS_RESULT_OK) {
        read_byte += sizeof(oxm_id->ofp.id);
        TAILQ_INSERT_TAIL(oxm_id_list, oxm_id, entry);
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
        ofp_error_set(error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);
        ret = LAGOPUS_RESULT_OFP_ERROR;
        free(oxm_id);
        break;
      }
    }
  }

  if (ret != LAGOPUS_RESULT_OFP_ERROR && read_byte != byte) {
    lagopus_msg_warning("bad length.\n");
    ofp_error_set(error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  }

  return ret;
}

static lagopus_result_t
s_prop_oxm_id_list_encode(struct pbuf_list *pbuf_list,
                          struct pbuf **pbuf,
                          const struct oxm_id_list *oxm_id_list,
                          uint16_t *total_length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const struct oxm_id *oxm_id = NULL;

  if (pbuf_list != NULL && pbuf != NULL &&
      oxm_id_list != NULL) {
    *total_length = 0;
    if (TAILQ_EMPTY(oxm_id_list) == false) {
      /* Encode packet. */
      TAILQ_FOREACH(oxm_id, oxm_id_list, entry) {
        ret = ofp_oxm_id_encode_list(pbuf_list, pbuf, &oxm_id->ofp);

        if (ret == LAGOPUS_RESULT_OK) {
          /* Sum length. And check overflow. */
          ret = ofp_tlv_length_sum(total_length,
                                   sizeof(struct ofp_oxm_id));
          if (ret != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("over oxm_idlength (%s).\n",
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
      /* oxm_id_list is empty. */
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

static void
experimenter_data_list_entry_free(
  struct experimenter_data_list *experimenter_data_list) {
  struct experimenter_data *experimenter_data;

  while ((experimenter_data = TAILQ_FIRST(experimenter_data_list)) != NULL) {
    TAILQ_REMOVE(experimenter_data_list, experimenter_data, entry);
    free(experimenter_data);
  }
}

/* unsupported experimenter. */
#if 0
static struct experimenter_data *
s_experimenter_data_alloc(void) {
  struct experimenter_data *experimenter_data = NULL;
  experimenter_data = (struct experimenter_data *)
                      calloc(1, sizeof(struct experimenter_data));
  return experimenter_data;
}

static lagopus_result_t
s_table_feature_prop_experimenter_parse(struct pbuf *pbuf, uint64_t byte,
                                        struct experimenter_data_list *experimenter_data_list,
                                        struct ofp_error *error) {
  uint64_t read_byte = 0;
  lagopus_result_t ret = LAGOPUS_RESULT_OK;
  struct experimenter_data *experimenter_data = NULL;

  while (read_byte < byte) {
    experimenter_data = s_experimenter_data_alloc();
    if (experimenter_data == NULL) {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      break;
    } else {
      ret = ofp_experimenter_data_decode(pbuf, &experimenter_data->ofp);
      if (ret == LAGOPUS_RESULT_OK) {
        read_byte += sizeof(experimenter_data->ofp.data);
        TAILQ_INSERT_TAIL(experimenter_data_list, experimenter_data, entry);
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
        ofp_error_set(error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);
        ret = LAGOPUS_RESULT_OFP_ERROR;
        free(experimenter_data);
        break;
      }
    }
  }

  if (ret != LAGOPUS_RESULT_OFP_ERROR && read_byte != byte) {
    lagopus_msg_warning("bad length.\n");
    ofp_error_set(error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  }

  return ret;
}

static lagopus_result_t
s_prop_experimenter_data_list_encode(struct pbuf_list *pbuf_list,
                                     struct pbuf **pbuf,
                                     const struct experimenter_data_list *experimenter_data_list,
                                     uint16_t *total_length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const struct experimenter_data *experimenter_data = NULL;

  if (pbuf_list != NULL && pbuf != NULL &&
      experimenter_data_list != NULL) {
    *total_length = 0;
    if (TAILQ_EMPTY(experimenter_data_list) == false) {
      /* Encode packet. */
      TAILQ_FOREACH(experimenter_data, experimenter_data_list, entry) {
        ret = ofp_experimenter_data_encode_list(pbuf_list, pbuf,
                                                &experimenter_data->ofp);

        if (ret == LAGOPUS_RESULT_OK) {
          /* Sum length. And check overflow. */
          ret = ofp_tlv_length_sum(total_length,
                                   sizeof(struct ofp_experimenter_data));
          if (ret != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("over experimenter_data length (%s).\n",
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
      /* experimenter_data is empty. */
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
#endif

static struct table_property *
s_table_property_alloc(void) {
  struct table_property *table_property = NULL;
  table_property = (struct table_property *)
                   calloc(1,
                          sizeof(struct table_property));
  if (table_property != NULL) {
    TAILQ_INIT(&table_property->instruction_list);
    TAILQ_INIT(&table_property->next_table_id_list);
    TAILQ_INIT(&table_property->action_list);
    TAILQ_INIT(&table_property->oxm_id_list);
    TAILQ_INIT(&table_property->experimenter_data_list);
  }
  return table_property;
}

static lagopus_result_t
s_ofp_instruction_header_parse(struct pbuf *pbuf,
                               uint64_t byte,
                               struct instruction_list *instruction_list,
                               struct ofp_error *error) {
  uint64_t read_byte = 0;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  while (read_byte < byte) {
    ret = ofp_instruction_header_parse(pbuf,
                                       instruction_list,
                                       error);
    if (ret == LAGOPUS_RESULT_OK) {
      read_byte += sizeof(struct ofp_instruction);
    } else {
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
      break;
    }
  }

  return ret;
}

static lagopus_result_t
s_ofp_action_header_parse(struct pbuf *pbuf,
                          uint64_t byte,
                          size_t action_length,
                          struct action_list *action_list,
                          struct ofp_error *error) {
  uint64_t read_byte = 0;
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  while (read_byte < byte) {
    ret = ofp_action_header_parse(pbuf,
                                  action_length,
                                  action_list,
                                  error);

    if (ret == LAGOPUS_RESULT_OK) {
      read_byte = (uint64_t) (read_byte + action_length);
    } else {
      lagopus_msg_warning("FAILED (%s).\n",
                          lagopus_error_get_string(ret));
      break;
    }
  }

  return ret;
}

static lagopus_result_t
s_table_property_parse(struct pbuf *pbuf,
                       struct table_features *table_features,
                       struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_OK;
  struct table_property *table_property = NULL;
  uint8_t *property_list_head = NULL;
  uint8_t *property_list_end = NULL;

  property_list_head = pbuf_getp_get(pbuf);
  property_list_end = property_list_head + (table_features->ofp.length -
                      sizeof(struct ofp_table_features));

  do {
    /* Allocate a new table_property. */
    table_property = s_table_property_alloc();
    if (table_property != NULL) {
      TAILQ_INSERT_TAIL(&table_features->table_property_list, table_property, entry);

      /* decode prop header. */
      ret = ofp_table_feature_prop_header_decode(pbuf,
            &table_property->ofp);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = pbuf_plen_check(pbuf, (size_t) table_property->ofp.length -
                              sizeof(struct ofp_tlv));
        if (ret == LAGOPUS_RESULT_OK) {
          /* Table property parse. */
          switch (table_property->ofp.type) {
            case OFPTFPT_INSTRUCTIONS        :
            case OFPTFPT_INSTRUCTIONS_MISS   :
              ret = s_ofp_instruction_header_parse(pbuf,
                                                   table_property->ofp.length -
                                                   sizeof(struct ofp_tlv),
                                                   &table_property->instruction_list,
                                                   error);
              break;
            case OFPTFPT_NEXT_TABLES         :
            case OFPTFPT_NEXT_TABLES_MISS    : {
              ret = s_prop_next_tables_parse(pbuf,
                                             table_property->ofp.length -
                                             sizeof(struct ofp_tlv),
                                             &table_property->next_table_id_list,
                                             error);
              break;
            }
            case OFPTFPT_WRITE_ACTIONS       :
            case OFPTFPT_WRITE_ACTIONS_MISS  :
            case OFPTFPT_APPLY_ACTIONS       :
            case OFPTFPT_APPLY_ACTIONS_MISS  : {
              struct ofp_tlv tlv;
              ret = ofp_tlv_decode_sneak(pbuf, &tlv);
              if (ret == LAGOPUS_RESULT_OK) {
                ret = s_ofp_action_header_parse(pbuf,
                                                table_property->ofp.length -
                                                sizeof(struct ofp_tlv),
                                                tlv.length,
                                                &table_property->action_list,
                                                error);
              } else {
                lagopus_msg_warning("FAILED (%s).\n",
                                    lagopus_error_get_string(ret));
                ofp_error_set(error, OFPET_TABLE_FEATURES_FAILED,
                              OFPTFFC_BAD_LEN);
                ret = LAGOPUS_RESULT_OFP_ERROR;
              }
              break;
            }
            case OFPTFPT_MATCH               :
            case OFPTFPT_WILDCARDS           :
            case OFPTFPT_WRITE_SETFIELD_MISS :
            case OFPTFPT_APPLY_SETFIELD      :
            case OFPTFPT_WRITE_SETFIELD      :
            case OFPTFPT_APPLY_SETFIELD_MISS :
              ret = s_table_feature_prop_oxm_parse(pbuf,
                                                   table_property->ofp.length - sizeof(struct ofp_tlv),
                                                   &table_property->oxm_id_list, error);
              break;
            case OFPTFPT_EXPERIMENTER        :
            case OFPTFPT_EXPERIMENTER_MISS   :
              lagopus_msg_warning("unsupported OFPTFPT_EXPERIMENTER[_MISS].\n");
              ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_EXPERIMENTER);
              ret = LAGOPUS_RESULT_OFP_ERROR;
              break;
            default:
              lagopus_msg_warning("bad type.\n");
              ofp_error_set(error, OFPET_TABLE_FEATURES_FAILED,
                            OFPTFFC_BAD_TYPE);
              ret = LAGOPUS_RESULT_OFP_ERROR;
              break;
          }

          if (ret == LAGOPUS_RESULT_OK) {
            ret = pbuf_forward(pbuf,
                               GET_64BIT_PADDING_LENGTH(table_property->ofp.length));
            if (ret != LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(ret));
              ofp_error_set(error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);
              ret = LAGOPUS_RESULT_OFP_ERROR;
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
          ofp_error_set(error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);
          ret = LAGOPUS_RESULT_OFP_ERROR;
          break;
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
        ofp_error_set(error, OFPET_TABLE_FEATURES_FAILED,
                      OFPTFFC_BAD_LEN);
        ret = LAGOPUS_RESULT_OFP_ERROR;
        break;
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
      break;
    }
  } while (ret == LAGOPUS_RESULT_OK && pbuf_getp_get(pbuf) < property_list_end);

  /* check length. */
  if (ret == LAGOPUS_RESULT_OK &&
      pbuf_getp_get(pbuf) != property_list_end) {
    ofp_error_set(error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  }

  return ret;
}

static struct table_features *
s_table_features_alloc(void) {
  struct table_features *table_features = NULL;
  table_features = (struct table_features *)
                   calloc(1, sizeof(struct table_features));
  if (table_features != NULL) {
    TAILQ_INIT(&table_features->table_property_list);
  }

  return table_features;
}

static void
table_property_list_free(struct table_property_list *table_property_list) {
  struct table_property *table_property;
  while ((table_property = TAILQ_FIRST(table_property_list)) != NULL) {
    instruction_list_entry_free(&table_property->instruction_list);
    next_table_id_list_entry_free(&table_property->next_table_id_list);
    action_list_entry_free(&table_property->action_list);
    oxm_id_list_entry_free(&table_property->oxm_id_list);
    experimenter_data_list_entry_free(&table_property->experimenter_data_list);
    TAILQ_REMOVE(table_property_list, table_property, entry);
    free(table_property);
  }
}

/* Free table_features list. */
void
table_features_list_elem_free(struct table_features_list
                              *table_features_list) {
  struct table_features *table_features;
  while ((table_features = TAILQ_FIRST(table_features_list)) != NULL) {
    table_property_list_free(&table_features->table_property_list);
    TAILQ_REMOVE(table_features_list, table_features, entry);
    free(table_features);
  }
}

static lagopus_result_t
s_parse_table_property_list(struct pbuf *pbuf,
                            struct table_features *table_features,
                            struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_OK;
  if (TAILQ_EMPTY(&table_features->table_property_list) == true &&
      pbuf_plen_get(pbuf) == 0) {
    res = LAGOPUS_RESULT_OK;    /* table_property_list is empty */
  } else {
    // table_property
    res = s_table_property_parse(pbuf, table_features, error);
    if (res != LAGOPUS_RESULT_OK) {
      lagopus_msg_warning("FAILED : s_table_property_parse (%s).\n",
                          lagopus_error_get_string(res));
    }
  }

  return res;
}

static lagopus_result_t
s_table_property_list_encode(struct pbuf_list *pbuf_list,
                             struct pbuf **pbuf,
                             struct table_property_list *table_property_list,
                             uint16_t *total_length) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t property_len = 0;
  uint8_t *property_head = NULL;
  struct table_property *table_property;

  if (pbuf != NULL &&
      table_property_list != NULL &&
      total_length != NULL) {
    *total_length = 0;
    if (TAILQ_EMPTY(table_property_list) == false) {
      TAILQ_FOREACH(table_property, table_property_list, entry) {
        property_len = 0;
        ret = ofp_table_feature_prop_header_encode_list(pbuf_list,
              pbuf,
              &table_property->ofp);
        if (ret == LAGOPUS_RESULT_OK) {
          property_head = pbuf_putp_get(*pbuf) -
              sizeof(struct ofp_table_feature_prop_header);

          switch (table_property->ofp.type) {
            case OFPTFPT_INSTRUCTIONS        :
            case OFPTFPT_INSTRUCTIONS_MISS   :
              ret =
                ofp_instruction_header_list_encode(pbuf_list,
                                                   pbuf,
                                                   &table_property->instruction_list,
                                                   &property_len);
              break;
            case OFPTFPT_NEXT_TABLES         :
            case OFPTFPT_NEXT_TABLES_MISS    :
              ret =
                s_prop_next_tables_list_encode(pbuf_list,
                                               pbuf,
                                               &table_property->next_table_id_list,
                                               &property_len);
              break;
            case OFPTFPT_WRITE_ACTIONS       :
            case OFPTFPT_WRITE_ACTIONS_MISS  :
            case OFPTFPT_APPLY_ACTIONS       :
            case OFPTFPT_APPLY_ACTIONS_MISS  :
              ret =
                ofp_action_header_list_encode(pbuf_list,
                                              pbuf,
                                              &table_property->action_list,
                                              &property_len);
              break;
            case OFPTFPT_MATCH               :
            case OFPTFPT_WILDCARDS           :
            case OFPTFPT_WRITE_SETFIELD_MISS :
            case OFPTFPT_APPLY_SETFIELD      :
            case OFPTFPT_WRITE_SETFIELD      :
            case OFPTFPT_APPLY_SETFIELD_MISS :
              ret =
                s_prop_oxm_id_list_encode(pbuf_list,
                                          pbuf,
                                          &table_property->oxm_id_list,
                                          &property_len);
              break;
            case OFPTFPT_EXPERIMENTER        :
            case OFPTFPT_EXPERIMENTER_MISS   :
              lagopus_msg_warning("unsupported OFPTFPT_EXPERIMENTER[_MISS].\n");
              ret = LAGOPUS_RESULT_UNSUPPORTED;
              break;
            default:
              ret = LAGOPUS_RESULT_UNSUPPORTED;
              break;
          }

          if (ret == LAGOPUS_RESULT_OK) {
            property_len = (uint16_t) (property_len +
                                       sizeof(struct ofp_table_feature_prop_header));
            /* Set length in ofp_table_feature_prop_header. */
            ret = ofp_tlv_length_set(property_head, property_len);

            if (ret == LAGOPUS_RESULT_OK) {
              ret = ofp_padding_encode(pbuf_list, pbuf,
                                       &property_len);
              if (ret == LAGOPUS_RESULT_OK) {
                /* Sum length. And check overflow. */
                ret = ofp_tlv_length_sum(total_length, property_len);
                if (ret != LAGOPUS_RESULT_OK) {
                  lagopus_msg_warning("FAILED : over property length (%s).\n",
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
        } else {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
          break;
        }
      }
    } else {
      /* list is empty. */
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }
  return ret;
}

STATIC lagopus_result_t
ofp_table_features_reply_create(struct channel *channel,
                                struct pbuf_list **pbuf_list,
                                struct table_features_list *table_features_list,
                                struct ofp_header *xid_header) {
  uint8_t *table_features_head = NULL; // length address
  uint16_t tmp_length = 0;
  uint16_t length = 0;
  uint16_t property_total_length = 0;
  lagopus_result_t ret = LAGOPUS_RESULT_OK;
  struct pbuf *pbuf = NULL;
  struct ofp_multipart_reply mp_reply;
  struct table_features *table_features = NULL;

  if (channel != NULL && pbuf_list != NULL &&
      table_features_list != NULL && xid_header != NULL) {

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

        mp_reply.type = OFPMP_TABLE_FEATURES;
        mp_reply.flags = 0;

        /* Encode multipart reply. */
        ret = ofp_multipart_reply_encode(pbuf, &mp_reply);

        if (ret == LAGOPUS_RESULT_OK) {
          if (TAILQ_EMPTY(table_features_list) == false) {
            TAILQ_FOREACH(table_features, table_features_list, entry) {
              ret = ofp_table_features_encode_list(*pbuf_list,
                                                   &pbuf,
                                                   &table_features->ofp);
              if (ret != LAGOPUS_RESULT_OK) {
                lagopus_msg_warning("Can't allocate pbuf.\n");
                ret = LAGOPUS_RESULT_NO_MEMORY;
                break;
              } else {
                table_features_head = pbuf_putp_get(pbuf) -
                    sizeof(struct ofp_table_features);
                ret = s_table_property_list_encode(*pbuf_list,
                                                   &pbuf,
                                                   &table_features->table_property_list,
                                                   &property_total_length);
                if (ret != LAGOPUS_RESULT_OK) {
                  lagopus_msg_warning("FAILED (%s).\n",
                                      lagopus_error_get_string(ret));
                  break;
                } else {
                  ret = ofp_multipart_length_set(table_features_head,
                                                 (uint16_t) (property_total_length +
                                                     sizeof(struct ofp_table_features)));
                  if (ret != LAGOPUS_RESULT_OK) {
                    lagopus_msg_warning("FAILED ofp_multipart_length_set (%s).\n",
                                        lagopus_error_get_string(ret));
                    break;
                  }
                }
              }
            }
          } else {
            /* table_features_list is empty. */
            ret = LAGOPUS_RESULT_OK;
          }

          if (ret == LAGOPUS_RESULT_OK) {
            /* set length for last pbuf.*/
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
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (ret != LAGOPUS_RESULT_OK && ret != LAGOPUS_RESULT_INVALID_ARGS) {
    if (*pbuf_list != NULL) {
      pbuf_list_free(*pbuf_list);
      *pbuf_list = NULL;
    }
  }

  return ret;
}

lagopus_result_t
ofp_table_features_request_handle(struct channel *channel,
                                  struct pbuf *pbuf,
                                  struct ofp_header *xid_header,
                                  struct ofp_error *error) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint64_t dpid;
  struct table_features *table_features= NULL;
  struct table_features_list table_features_list;
  struct pbuf_list *pbuf_list = NULL;

  TAILQ_INIT(&table_features_list);
  if (channel != NULL && pbuf != NULL &&
      xid_header != NULL && error != NULL) {
    dpid = channel_dpid_get(channel);
    /* ofp_table_features not in request body. */
    if (pbuf_plen_equal_check(pbuf, 0) == LAGOPUS_RESULT_OK) {
      res = ofp_table_features_get(dpid,
                                   &table_features_list,
                                   error);
      if (res == LAGOPUS_RESULT_OK) {
        res = ofp_table_features_reply_create(channel,
                                              &pbuf_list,
                                              &table_features_list,
                                              xid_header);
        if (res == LAGOPUS_RESULT_OK) {
          /* send reply */
          res = channel_send_packet_list(channel, pbuf_list);
          if (res != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("Socket write error (%s).\n",
                                lagopus_error_get_string(res));
          }
        } else {
          lagopus_msg_warning("reply creation failed, (%s).\n",
                              lagopus_error_get_string(res));
        }

        /* free. */
        if (pbuf_list != NULL) {
          pbuf_list_free(pbuf_list);
        }
      } else {
        lagopus_msg_warning("FAILED ofp_talbe_features_get (%s)\n",
                            lagopus_error_get_string(res));
      }
    } else { // ofp_table_features in request body.
      table_features = s_table_features_alloc();
      if (table_features == NULL) {
        res = LAGOPUS_RESULT_NO_MEMORY;
      } else {
        TAILQ_INSERT_TAIL(&table_features_list,
                          table_features,
                          entry);
        while (pbuf_plen_get(pbuf) > 0) {
          /* Decode table_features */
          res = ofp_table_features_decode(pbuf, &table_features->ofp);
          if (res != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("FAILED ofp_table_features_decode (%s)\n",
                                lagopus_error_get_string(res));
            ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
            res = LAGOPUS_RESULT_OFP_ERROR;
          } else {
            res = s_parse_table_property_list(pbuf,
                                              table_features,
                                              error);
            if (res == LAGOPUS_RESULT_OK) {
              res = ofp_table_features_set(dpid,
                                           &table_features_list,
                                           error);
              if (res == LAGOPUS_RESULT_OK) {
                res = ofp_table_features_reply_create(channel,
                                                      &pbuf_list,
                                                      &table_features_list,
                                                      xid_header);
                if (res == LAGOPUS_RESULT_OK) {
                  /* send reply */
                  res = channel_send_packet_list(channel, pbuf_list);
                  if (res != LAGOPUS_RESULT_OK) {
                    lagopus_msg_warning("Socket write error (%s).\n",
                                        lagopus_error_get_string(res));
                  }
                } else {
                  lagopus_msg_warning("reply creation failed, (%s).\n",
                                      lagopus_error_get_string(res));
                }

                /* free. */
                if (pbuf_list != NULL) {
                  pbuf_list_free(pbuf_list);
                }
              }
            } else {
              lagopus_msg_warning("FAILED ofp_table_features_get (%s).\n",
                                  lagopus_error_get_string(res));
            }
          }

          if (res != LAGOPUS_RESULT_OK) {
            break;
          }
        }

        /* check plen. */
        if (res == LAGOPUS_RESULT_OK && pbuf_plen_get(pbuf) != 0) {
          lagopus_msg_warning("packet decode failed. (size over).\n");
          ofp_error_set(error, OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_LEN);
          res = LAGOPUS_RESULT_OFP_ERROR;
        }
      }
    }
  } else {
    res = LAGOPUS_RESULT_INVALID_ARGS;
  }

  /* free. */
  table_features_list_elem_free(&table_features_list);
  return res;
}
