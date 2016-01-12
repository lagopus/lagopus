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
#include "lagopus/ofp_handler.h"
#include "lagopus/ofp_pdump.h"
#include "ofp_apis.h"
#include "channel_mgr.h"

static void
error_msg_trace(struct ofp_error_msg *error) {
  if (lagopus_log_check_trace_flags(TRACE_OFPT_ERROR)) {
    lagopus_msg_pdump(TRACE_OFPT_ERROR, PDUMP_OFP_HEADER,
                      error->header, "");
    lagopus_msg_pdump(TRACE_OFPT_ERROR, PDUMP_OFP_ERROR,
                      *error, "");
  }
}

static void
ofp_error_str_get_internal(uint16_t type, uint16_t code,
                           const char *pre_msg,
                           char *str, size_t max_len) {
  snprintf(str, max_len, "%s %s [type = %s(%d), code = %s(%d)].",
           pre_msg,
           lagopus_error_get_string(LAGOPUS_RESULT_OFP_ERROR),
           ofp_error_type_str_get(type), type,
           ofp_error_code_str_get(type, code), code);
}

/* RECV */
lagopus_result_t
ofp_error_msg_handle(struct channel *channel, struct pbuf *pbuf,
                     struct ofp_header *xid_header,
                     struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct ofp_error_msg error_msg;

  if (channel != NULL && pbuf != NULL) {
    /* Parse packet. */
    ret = ofp_error_msg_decode(pbuf, &error_msg);

    if (ret == LAGOPUS_RESULT_OK) {
      /* dump trace. */
      error_msg_trace(&error_msg);

      /* Recv Hello error. */
      if ((error_msg.type == OFPET_HELLO_FAILED &&
           error_msg.code == OFPHFC_INCOMPATIBLE) ||
          (error_msg.type == OFPET_BAD_REQUEST &&
           error_msg.code == OFPBRC_BAD_VERSION)) {
        lagopus_msg_warning("Close channel.\n");
        channel_disable(channel);
        ret = LAGOPUS_RESULT_OK;
      } else {
        if (ofp_header_version_check(channel, xid_header) == false) {
          ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_VERSION);
          ret = LAGOPUS_RESULT_OFP_ERROR;
        } else {
          ret = LAGOPUS_RESULT_OK;
        }
      }
    } else {
      lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
      ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_LEN);
      ret =  LAGOPUS_RESULT_OFP_ERROR;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* Handle unsupported packet. */
lagopus_result_t
ofp_unsupported_handle(struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (error != NULL) {
    ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_TYPE);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* Handle unsupported experimenter. */
lagopus_result_t
ofp_bad_experimenter_handle(struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (error != NULL) {
    ofp_error_set(error, OFPET_BAD_REQUEST, OFPBRC_BAD_EXPERIMENTER);
    ret = LAGOPUS_RESULT_OFP_ERROR;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

/* SEND */
/* Send error message. */
STATIC lagopus_result_t
ofp_error_msg_create(struct channel *channel, struct ofp_error *error,
                     struct ofp_header *xid_header, struct pbuf **pbuf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  void *data = NULL;
  size_t data_len;
  size_t size;
  struct ofp_error_msg msg;

  if (channel != NULL && error != NULL &&
      xid_header != NULL && pbuf != NULL) {

    /* Calculate message size. */
    size = sizeof(struct ofp_error_msg);
    data_len = 0;

    switch (error->type) {
      case OFPET_HELLO_FAILED:
        if (error->str != NULL) {
          data_len = strlen(error->str);
          /* for trailing null. */
          data_len++;
          size += data_len;
          data = (void *) error->str;
        }
        break;
      default:
        if (error->req != NULL) {
          data_len = pbuf_plen_get(error->req);
          size += data_len;
          data = (void *) error->req->data;
        }
        break;
    }

    /* alloc */
    *pbuf = channel_pbuf_list_get(channel, size);

    if (*pbuf != NULL) {
      /* Fill in header. */
      ofp_header_set(&msg.header, channel_version_get(channel),
                     OFPT_ERROR, (uint16_t) size,
                     xid_header->xid);

      /* Fill in error. */
      msg.type = error->type;
      msg.code = error->code;

      /* Set size to pbuffer size. */
      pbuf_plen_set(*pbuf, size);

      /* Encode message. */
      ret = ofp_error_msg_encode(*pbuf, &msg);

      if (ret == LAGOPUS_RESULT_OK) {
        /* Encode error string or failed request data. */
        if (data != NULL) {
          if (size < data_len) {
            lagopus_msg_warning("FAILED : over error string."
                                "len = %zu, str = %s\n",
                                data_len, error->str);
            ret = LAGOPUS_RESULT_OUT_OF_RANGE;
          } else {
            /* Copy error msg. */
            ret = pbuf_encode(*pbuf, data, data_len);
            if (ret != LAGOPUS_RESULT_OK) {
              lagopus_msg_warning("FAILED (%s).\n",
                                  lagopus_error_get_string(ret));
            }
          }
        } else {
          ret = LAGOPUS_RESULT_OK;
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }

    /* free. */
    if (ret != LAGOPUS_RESULT_OK && *pbuf != NULL) {
      channel_pbuf_list_unget(channel, *pbuf);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_error_msg_send(struct channel *channel,
                   struct ofp_header *xid_header,
                   struct ofp_error *error) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char ebuf[ERROR_MAX_SIZE];
  struct pbuf *send_pbuf = NULL;

  if (channel != NULL && xid_header != NULL &&
      error != NULL) {
    /* put log. */
    ofp_error_str_get_internal(error->type, error->code, "Send",
                               ebuf, ERROR_MAX_SIZE);
    lagopus_msg_warning("%s\n", ebuf);

    ret = ofp_error_msg_create(channel, error,
                               xid_header, &send_pbuf);
    if (ret == LAGOPUS_RESULT_OK) {
      channel_send_packet(channel, send_pbuf);
    } else {
      lagopus_msg_warning("FAILED (%s).\n", lagopus_error_get_string(ret));
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (ret != LAGOPUS_RESULT_OK && send_pbuf != NULL) {
    channel_pbuf_list_unget(channel, send_pbuf);
  }

  return ret;
}

/* Free ofp_error for event queue. */
void
ofp_error_msg_free(struct eventq_data *data) {
  if (data != NULL) {
    if (data->error.ofp_error.req != NULL) {
      pbuf_free(data->error.ofp_error.req);
    }
    free(data);
  }
}

/* RECV from queue. */
lagopus_result_t
ofp_error_msg_from_queue_handle(struct error *error,
                                uint64_t dpid) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  struct channel *channel = NULL;
  struct ofp_header header;

  if (error != NULL) {
    channel_mgr_channel_lookup_by_channel_id(dpid,
        error->channel_id, &channel);
    if (channel != NULL) {
      header.xid = error->xid;
      ret = ofp_error_msg_send(channel, &header, &error->ofp_error);
    } else {
      lagopus_msg_warning("Not found channel.\n");
      ret = LAGOPUS_RESULT_NOT_FOUND;
    }
  } else {
    lagopus_msg_warning("Arg is NULL.\n");
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

const char *
ofp_error_type_str_get(uint16_t type) {
  return ofp_error_type_str(type);
}

const char *
ofp_error_code_str_get(uint16_t type, uint16_t code) {
  switch (type) {
    case OFPET_HELLO_FAILED:
      return ofp_hello_failed_code_str(code);
    case OFPET_BAD_REQUEST:
      return ofp_bad_request_code_str(code);
    case OFPET_BAD_ACTION:
      return ofp_bad_action_code_str(code);
    case OFPET_BAD_INSTRUCTION:
      return ofp_bad_instruction_code_str(code);
    case OFPET_BAD_MATCH:
      return ofp_bad_match_code_str(code);
    case OFPET_FLOW_MOD_FAILED:
      return ofp_flow_mod_failed_code_str(code);
    case OFPET_GROUP_MOD_FAILED:
      return ofp_group_mod_failed_code_str(code);
    case OFPET_PORT_MOD_FAILED:
      return ofp_port_mod_failed_code_str(code);
    case OFPET_TABLE_MOD_FAILED:
      return ofp_table_mod_failed_code_str(code);
    case OFPET_QUEUE_OP_FAILED:
      return ofp_queue_op_failed_code_str(code);
    case OFPET_SWITCH_CONFIG_FAILED:
      return ofp_switch_config_failed_code_str(code);
    case OFPET_ROLE_REQUEST_FAILED:
      return ofp_role_request_failed_code_str(code);
    case OFPET_METER_MOD_FAILED:
      return ofp_meter_mod_failed_code_str(code);
    case OFPET_TABLE_FEATURES_FAILED:
      return ofp_table_features_failed_code_str(code);
    case OFPET_EXPERIMENTER:
      return "Not code";
    default:
      return "Unknown";
  }
}

void
ofp_error_val_set(struct ofp_error *error, uint16_t type, uint16_t code) {
  error->type = type;
  error->code = code;

  switch (type) {
    case OFPET_HELLO_FAILED:
      error->str = NULL;
      break;
    default:
      break;
  }
}

void
ofp_error_str_get(uint16_t type, uint16_t code,
                  char *str, size_t max_len) {
  ofp_error_str_get_internal(type, code, "Set", str, max_len);
}
