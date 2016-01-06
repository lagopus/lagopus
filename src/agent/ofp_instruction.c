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
#include "lagopus/ofp_dp_apis.h"
#include "lagopus/ofp_pdump.h"
#include "lagopus_apis.h"
#include "openflow.h"
#include "openflow13packet.h"
#include "ofp_instruction.h"
#include "ofp_action.h"
#include "ofp_tlv.h"

struct instruction *
instruction_alloc(void) {
  struct instruction *instruction;

  instruction = (struct instruction *)
                calloc(1, sizeof(struct instruction));
  if (instruction != NULL) {
    TAILQ_INIT(&instruction->action_list);
  }

  return instruction;
}

void
ofp_instruction_list_trace(uint32_t flags,
                           struct instruction_list *instruction_list) {
  struct instruction *instruction = NULL;

  TAILQ_FOREACH(instruction, instruction_list, entry) {
    switch (instruction->ofpit.type) {
      case OFPIT_GOTO_TABLE:
        lagopus_msg_pdump(flags, PDUMP_OFP_INSTRUCTION_GOTO_TABLE,
                          instruction->ofpit_goto_table, "");
        break;
      case OFPIT_WRITE_METADATA:
        lagopus_msg_pdump(flags, PDUMP_OFP_INSTRUCTION_WRITE_METADATA,
                          instruction->ofpit_write_metadata, "");
        break;
      case OFPIT_WRITE_ACTIONS:
      case OFPIT_APPLY_ACTIONS:
      case OFPIT_CLEAR_ACTIONS:
        lagopus_msg_pdump(flags, PDUMP_OFP_INSTRUCTION_ACTIONS,
                          instruction->ofpit_actions, "");
        break;
      case OFPIT_METER:
        lagopus_msg_pdump(flags, PDUMP_OFP_INSTRUCTION_METER,
                          instruction->ofpit_meter, "");
        break;
      case OFPIT_EXPERIMENTER:
        lagopus_msg_pdump(flags, PDUMP_OFP_INSTRUCTION_EXPERIMENTER,
                          instruction->ofpit_experimenter, "");
        break;
      default:
        break;
    }

    ofp_action_list_trace(flags, &instruction->action_list);
  }
}

/* ofp_instruction parser. */
lagopus_result_t
ofp_instruction_parse(struct pbuf *pbuf,
                      struct instruction_list *instruction_list,
                      struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t action_length;
  struct ofp_tlv tlv;
  struct instruction *instruction = NULL;

  if (pbuf != NULL && instruction_list != NULL &&
      error != NULL) {
    /* Decode TLV. */
    ret = ofp_tlv_decode_sneak(pbuf, &tlv);

    if (ret == LAGOPUS_RESULT_OK) {
      /* check instruction length.*/
      ret = pbuf_plen_check(pbuf, (size_t) tlv.length);
      if (ret == LAGOPUS_RESULT_OK) {
        /* Allocate a new instruction. */
        instruction = instruction_alloc();

        if (instruction != NULL) {
          TAILQ_INSERT_TAIL(instruction_list, instruction, entry);

          switch (tlv.type) {
            case OFPIT_GOTO_TABLE:
              ret = ofp_instruction_goto_table_decode(pbuf,
                                                      &instruction->ofpit_goto_table);
              if (ret != LAGOPUS_RESULT_OK) {
                ret = LAGOPUS_RESULT_OFP_ERROR;
                ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
                lagopus_msg_warning("A instruction failed to decode.\n");
              }
              break;
            case OFPIT_WRITE_METADATA:
              ret = ofp_instruction_write_metadata_decode(pbuf,
                    &instruction->ofpit_write_metadata);
              if (ret != LAGOPUS_RESULT_OK) {
                ret = LAGOPUS_RESULT_OFP_ERROR;
                ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
                lagopus_msg_warning("A instruction failed to decode.\n");
              }
              break;
            case OFPIT_WRITE_ACTIONS:
            case OFPIT_APPLY_ACTIONS:
            case OFPIT_CLEAR_ACTIONS:
              ret = ofp_instruction_actions_decode(pbuf,
                                                   &instruction->ofpit_actions);
              if (ret == LAGOPUS_RESULT_OK) {
                action_length = (instruction->ofpit_actions.len
                                 - sizeof(struct ofp_instruction_actions));
                /* Parse action. */
                ret = ofp_action_parse(pbuf, action_length,
                                       &instruction->action_list,
                                       error);
                if (ret != LAGOPUS_RESULT_OK) {
                  lagopus_msg_warning("FAILED (%s).\n",
                                      lagopus_error_get_string(ret));
                } else if (tlv.type == OFPIT_CLEAR_ACTIONS &&
                           TAILQ_EMPTY(&instruction->action_list) == false) {
                  lagopus_msg_warning("CLEAR_ACTIONS contain any actions.\n");
                  ret = LAGOPUS_RESULT_OFP_ERROR;
                  ofp_error_set(error, OFPET_BAD_INSTRUCTION, OFPBIC_BAD_LEN);
                }
              } else {
                ret = LAGOPUS_RESULT_OFP_ERROR;
                ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
                lagopus_msg_warning("A instruction failed to decode.\n");
              }
              break;
            case OFPIT_METER:
              ret = ofp_instruction_meter_decode(pbuf,
                                                 &instruction->ofpit_meter);
              if (ret != LAGOPUS_RESULT_OK) {
                ret = LAGOPUS_RESULT_OFP_ERROR;
                ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
                lagopus_msg_warning("A instruction failed to decode.\n");
              }
              break;
            case OFPIT_EXPERIMENTER:
              lagopus_msg_warning("unsupported OFPIT_EXPERIMENTER.\n");
              ofp_error_set(error, OFPET_BAD_INSTRUCTION, OFPBIC_UNSUP_INST);
              ret = LAGOPUS_RESULT_OFP_ERROR;
              break;
            default:
              lagopus_msg_warning("Not found instruction type.\n");
              ofp_error_set(error, OFPET_BAD_INSTRUCTION, OFPBIC_UNKNOWN_INST);
              ret = LAGOPUS_RESULT_OFP_ERROR;
              break;
          }
        } else {
          ret = LAGOPUS_RESULT_NO_MEMORY;
        }
      } else {
        lagopus_msg_warning("bad instruction length.\n");
        ofp_error_set(error, OFPET_BAD_INSTRUCTION, OFPBIC_BAD_LEN);
        ret = LAGOPUS_RESULT_OFP_ERROR;
      }
    } else {
      ret = LAGOPUS_RESULT_OFP_ERROR;
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      lagopus_msg_warning("A instruction failed to decode.\n");
    }

    /* free. */
    if (ret != LAGOPUS_RESULT_OK) {
      instruction_list_entry_free(instruction_list);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_instruction_list_encode(struct pbuf_list *pbuf_list,
                            struct pbuf **pbuf,
                            struct instruction_list *instruction_list,
                            uint16_t *total_length) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  uint16_t instruction_len = 0;
  uint16_t action_total_len = 0;
  uint8_t *instruction_head;
  struct instruction *instruction = NULL;

  /* NULL check. pbuf_list allows NULL. */
  if (pbuf != NULL && instruction_list != NULL &&
      total_length != NULL) {
    *total_length = 0;
    if (TAILQ_EMPTY(instruction_list) == false) {
      TAILQ_FOREACH(instruction, instruction_list, entry) {
        /* instruction head pointer. */
        instruction_head = pbuf_putp_get(*pbuf);

        switch (instruction->ofpit.type) {
          case OFPIT_GOTO_TABLE:
            instruction->ofpit.len = (uint16_t)
                                     (sizeof(struct ofp_instruction_goto_table));
            res = ofp_instruction_goto_table_encode_list(
                    pbuf_list, pbuf, &(instruction->ofpit_goto_table));
            break;
          case OFPIT_WRITE_METADATA:
            instruction->ofpit.len = (uint16_t)
                                     (sizeof(struct ofp_instruction_write_metadata));
            res = ofp_instruction_write_metadata_encode_list(
                    pbuf_list, pbuf, &(instruction->ofpit_write_metadata));
            break;
          case OFPIT_WRITE_ACTIONS:
          case OFPIT_APPLY_ACTIONS:
          case OFPIT_CLEAR_ACTIONS:
            instruction->ofpit.len = (uint16_t)
                                     (sizeof(struct ofp_instruction_actions));
            res = ofp_instruction_actions_encode_list(
                    pbuf_list, pbuf, &(instruction->ofpit_actions));
            break;
          case OFPIT_METER:
            instruction->ofpit.len = (uint16_t)
                                     (sizeof(struct ofp_instruction_meter));
            res = ofp_instruction_meter_encode_list(
                    pbuf_list, pbuf, &(instruction->ofpit_meter));
            break;
          case OFPIT_EXPERIMENTER:
            instruction->ofpit.len = (uint16_t)
                                     (sizeof(struct ofp_instruction_experimenter));
            res = ofp_instruction_experimenter_encode_list(
                    pbuf_list, pbuf, &(instruction->ofpit_experimenter));
            break;
          default:
            lagopus_msg_warning("FAILED: unknown instruction (%d)\n",
                                instruction->ofpit.type);
            res = LAGOPUS_RESULT_OUT_OF_RANGE;
            break;
        }

        if (res == LAGOPUS_RESULT_OK) {
          res = ofp_action_list_encode(pbuf_list, pbuf,
                                       &(instruction->action_list),
                                       &action_total_len);
          if (res == LAGOPUS_RESULT_OK) {
            /* Set instruction length (instruction length +  */
            /*                         action total length). */
            /* And check overflow.                           */
            instruction_len = instruction->ofpit.len;
            res = ofp_tlv_length_sum(&instruction_len, action_total_len);

            if (res == LAGOPUS_RESULT_OK) {
              res = ofp_tlv_length_set(instruction_head,
                                       instruction_len);

              if (res == LAGOPUS_RESULT_OK) {
                /* Sum length. And check overflow. */
                res = ofp_tlv_length_sum(total_length, instruction_len);
                if (res != LAGOPUS_RESULT_OK) {
                  lagopus_msg_warning("over instruction length (%s).\n",
                                      lagopus_error_get_string(res));
                  break;
                }
              } else {
                lagopus_msg_warning("FAILED (%s).\n",
                                    lagopus_error_get_string(res));
                break;
              }
            } else {
              lagopus_msg_warning("over instruction length (%s).\n",
                                  lagopus_error_get_string(res));
              break;
            }
          } else {
            lagopus_msg_warning("FAILED : action-list encode error. (%s)\n",
                                lagopus_error_get_string(res));
            break;
          }
        } else {
          lagopus_msg_warning("FAILED : ofp_instruction encode error. (%s)\n",
                              lagopus_error_get_string(res));
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

/* parse ofp_instruction header for ofp_table_feature. */
lagopus_result_t
ofp_instruction_header_parse(struct pbuf *pbuf,
                             struct instruction_list *instruction_list,
                             struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_tlv tlv;
  struct instruction *instruction = NULL;

  if (pbuf != NULL && instruction_list != NULL &&
      error != NULL) {
    /* Decode TLV. */
    ret = ofp_tlv_decode_sneak(pbuf, &tlv);

    if (ret == LAGOPUS_RESULT_OK) {
      /* check instruction length.*/
      ret = pbuf_plen_check(pbuf, (size_t) tlv.length);
      if (ret == LAGOPUS_RESULT_OK) {
        /* Allocate a new instruction. */
        instruction = instruction_alloc();

        if (instruction != NULL) {
          TAILQ_INSERT_TAIL(instruction_list, instruction, entry);

          switch (tlv.type) {
            case OFPIT_GOTO_TABLE:
            case OFPIT_WRITE_METADATA:
            case OFPIT_WRITE_ACTIONS:
            case OFPIT_APPLY_ACTIONS:
            case OFPIT_CLEAR_ACTIONS:
            case OFPIT_METER:
              ret = ofp_instruction_decode(pbuf, &instruction->ofpit);
              if (ret != LAGOPUS_RESULT_OK) {
                lagopus_msg_warning("FAILED (%s)\n",
                                    lagopus_error_get_string(ret));
                ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
                ret = LAGOPUS_RESULT_OFP_ERROR;
              }
              break;
            case OFPIT_EXPERIMENTER:
              lagopus_msg_warning("unsupported OFPIT_EXPERIMENTER.\n");
              ofp_error_set(error, OFPET_BAD_INSTRUCTION, OFPBIC_UNSUP_INST);
              ret = LAGOPUS_RESULT_OFP_ERROR;
              break;
            default:
              lagopus_msg_warning("Not found instruction type.\n");
              ofp_error_set(error, OFPET_BAD_INSTRUCTION, OFPBIC_UNKNOWN_INST);
              ret = LAGOPUS_RESULT_OFP_ERROR;
              break;
          }
        } else {
          lagopus_msg_warning("Can't allocate instruction.\n");
          ret = LAGOPUS_RESULT_NO_MEMORY;
        }
      } else {
        lagopus_msg_warning("bad instruction length.\n");
        ofp_error_set(error, OFPET_BAD_INSTRUCTION, OFPBIC_BAD_LEN);
        ret = LAGOPUS_RESULT_OFP_ERROR;
      }
    } else {
      lagopus_msg_warning("FAILED (%s)\n",
                          lagopus_error_get_string(ret));
      ofp_error_set(error, OFPET_BAD_INSTRUCTION, OFPBIC_BAD_LEN);
      ret = LAGOPUS_RESULT_OFP_ERROR;
    }

    /* free. */
    if (ret != LAGOPUS_RESULT_OK) {
      instruction_list_entry_free(instruction_list);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* encode ofp_instruction header for ofp_table_feature. */
lagopus_result_t
ofp_instruction_header_list_encode(struct pbuf_list *pbuf_list,
                                   struct pbuf **pbuf,
                                   struct instruction_list *instruction_list,
                                   uint16_t *total_length) {
  lagopus_result_t res = LAGOPUS_RESULT_ANY_FAILURES;
  struct instruction *instruction = NULL;

  /* NULL check. pbuf_list allows NULL. */
  if (pbuf != NULL && instruction_list != NULL &&
      total_length != NULL) {
    *total_length = 0;
    if (TAILQ_EMPTY(instruction_list) == false) {
      TAILQ_FOREACH(instruction, instruction_list, entry) {
        switch (instruction->ofpit.type) {
          case OFPIT_GOTO_TABLE:
          case OFPIT_WRITE_METADATA:
          case OFPIT_WRITE_ACTIONS:
          case OFPIT_APPLY_ACTIONS:
          case OFPIT_CLEAR_ACTIONS:
          case OFPIT_METER:
          case OFPIT_EXPERIMENTER:
            instruction->ofpit.len = (uint16_t)
                                     (sizeof(struct ofp_instruction));
            res = ofp_instruction_encode_list(
                    pbuf_list, pbuf, &(instruction->ofpit));
            break;
          default:
            lagopus_msg_warning("FAILED: unknown instruction (%d)\n",
                                instruction->ofpit.type);
            res = LAGOPUS_RESULT_OUT_OF_RANGE;
            break;
        }

        if (res == LAGOPUS_RESULT_OK) {
          /* Sum length. And check overflow. */
          res = ofp_tlv_length_sum(total_length, instruction->ofpit.len);
          if (res != LAGOPUS_RESULT_OK) {
            lagopus_msg_warning("over instruction length (%s).\n",
                                lagopus_error_get_string(res));
            break;
          }
        } else {
          lagopus_msg_warning("FAILED : ofp_instruction encode error. (%s)\n",
                              lagopus_error_get_string(res));
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
