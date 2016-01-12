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

#include <sys/queue.h>
#include <stdbool.h>
#include <stdint.h>
#include "lagopus_apis.h"
#include "openflow.h"
#include "ofp_apis.h"
#include "lagopus/pbuf.h"
#include "channel_mgr.h"
#include "channel.h"
#include "openflow13packet.h"

#define MASK_MAX_NUM 2

void
ofp_role_channel_mask_init(struct ofp_async_config *role_mask) {
  int i;

  if (role_mask == NULL) {
    return;
  }

  memset(&role_mask->header, 0, sizeof(struct ofp_header));
  for (i=0; i < MASK_MAX_NUM; i++) {
    role_mask->packet_in_mask[i] = DEFAULT_REASON_MASK;
    role_mask->port_status_mask[i] = DEFAULT_REASON_MASK;
    role_mask->flow_removed_mask[i] = DEFAULT_REASON_MASK;
  }

  return;
}

static lagopus_result_t
change_master2slave(struct channel *channel, void *val) {
  (void) val;
  if (channel_role_get(channel) == OFPCR_ROLE_MASTER) {
    channel_role_set(channel, OFPCR_ROLE_SLAVE);
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
ofp_role_channel_update(struct channel *channel,
                        struct ofp_role_request *role_request,
                        uint64_t dpid) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint32_t current_role;

  if (channel != NULL && role_request != NULL) {
    current_role = channel_role_get(channel);
    /* Not change. */
    if (role_request->role == current_role ||
        role_request->role == OFPCR_ROLE_NOCHANGE) {
      lagopus_msg_debug(1, "Role not change (%u).\n",
                        role_request->role);
      role_request->role = current_role;
      ret = LAGOPUS_RESULT_OK;
    } else {
      if (current_role == OFPCR_ROLE_MASTER) {
        /* every other master controller to slave. */
        channel_mgr_dpid_iterate(dpid, change_master2slave, NULL);
      }
      channel_role_set(channel, role_request->role);
      ret = LAGOPUS_RESULT_OK;
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

bool
ofp_role_channel_check_mask(struct ofp_async_config *role_mask, uint8_t type,
                            uint8_t reason, uint32_t role) {
  uint32_t bit = 1;
  uint32_t reason_bitmap;
  uint8_t role_num;
  bool ret;

  if (role_mask != NULL) {
    ret = true;
    switch (role) {
      case OFPCR_ROLE_MASTER:
      case OFPCR_ROLE_EQUAL:
        role_num = 0;
        break;
      case OFPCR_ROLE_SLAVE:
        role_num = 1;
        break;
      default:
        return false;
    }

    if (ret == true) {
      reason_bitmap = bit << reason;

      switch (type) {
        case OFPT_PACKET_IN:
          ret = ((reason_bitmap &
                  role_mask->packet_in_mask[role_num]) > 0);
          break;
        case OFPT_FLOW_REMOVED:
          ret = ((reason_bitmap &
                  role_mask->flow_removed_mask[role_num]) > 0);
          break;
        case OFPT_PORT_STATUS:
          ret = ((reason_bitmap &
                  role_mask->port_status_mask[role_num]) > 0);
          break;
        default:
          ret = false;
      }
    }
  } else {
    ret = false;
  }

  return ret;
}

static lagopus_result_t
ofp_write_channel(struct channel *channel,
                  struct pbuf *pbuf) {
  lagopus_result_t ret = LAGOPUS_RESULT_OK;
  struct pbuf *send_pbuf = NULL;

  if (channel != NULL && pbuf != NULL) {
    send_pbuf = channel_pbuf_list_get(channel,
                                      pbuf_plen_get(pbuf));
    if (send_pbuf != NULL) {
      /* Copy pbuf. */
      ret = pbuf_copy(send_pbuf, pbuf);

      if (ret == LAGOPUS_RESULT_OK) {
        ret = ofp_header_packet_set(channel, send_pbuf);

        if (ret == LAGOPUS_RESULT_OK) {
          channel_send_packet(channel, send_pbuf);
          ret = LAGOPUS_RESULT_OK;
        } else {
          lagopus_msg_warning("FAILED (%s).\n",
                              lagopus_error_get_string(ret));
        }
      } else {
        lagopus_msg_warning("FAILED (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("Can't allocate pbuf.\n");
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }

    if (ret != LAGOPUS_RESULT_OK && send_pbuf != NULL) {
      channel_pbuf_list_unget(channel, send_pbuf);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

struct role_write_vars {
  uint8_t type;
  uint8_t reason;
  struct pbuf *pbuf;
  lagopus_result_t ret;
};

static lagopus_result_t
ofp_role_write(struct channel *channel, void *val) {
  struct role_write_vars *v = val;

  if (channel_is_alive(channel) == true) {
    /* packet filtering. */
    if (channel_role_channel_check_mask(channel, v->type, v->reason) == true) {
      v->ret = ofp_write_channel(channel, v->pbuf);
    } else {
      /* Not send packet. */
      v->ret = LAGOPUS_RESULT_OK;
    }
  } else {
    lagopus_msg_info("Channel is not alive, drop asynchronous message(%s)\n",
                     ofp_type_str(v->type));
    /* Not send packet. */
    v->ret = LAGOPUS_RESULT_OK;
  }

  return LAGOPUS_RESULT_OK;
}

lagopus_result_t
ofp_role_channel_write(struct pbuf *pbuf, uint64_t dpid,
                       uint8_t type, uint8_t reason) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  if (pbuf != NULL) {
    struct role_write_vars v;

    v.type = type;
    v.reason = reason;
    v.pbuf = pbuf;
    v.ret = LAGOPUS_RESULT_ANY_FAILURES;

    ret = channel_mgr_dpid_iterate(dpid, ofp_role_write, (void *) &v);

    if (ret != LAGOPUS_RESULT_OK) {
      if (ret == LAGOPUS_RESULT_NOT_OPERATIONAL ||
          ret == LAGOPUS_RESULT_NOT_FOUND) {
        lagopus_msg_info("Channel is not alive, drop asynchronous message(%s)\n",
                         ofp_type_str(v.type));
        /* Not send packet. */
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = v.ret;
      }
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  if (ret == LAGOPUS_RESULT_OK && pbuf != NULL) {
    /* move getp to putp. */
    pbuf_getp_set(pbuf, pbuf_putp_get(pbuf));
  }

  return ret;
}

bool
ofp_role_check(struct channel *channel,
               struct ofp_header *header) {
  bool ret = false;

  if (channel != NULL && header != NULL) {
    if (channel_role_get(channel) == OFPCR_ROLE_SLAVE) {
      switch (header->type) {
        case OFPT_SET_CONFIG:
        case OFPT_TABLE_MOD:
        case OFPT_FLOW_MOD:
        case OFPT_GROUP_MOD:
        case OFPT_PORT_MOD:
        case OFPT_METER_MOD:
        case OFPT_PACKET_OUT:
        case OFPT_PACKET_IN:
        case OFPT_FLOW_REMOVED:
          ret = false;
          break;
        default:
          ret = true;
          break;
      }
    } else {
      ret = true;
    }
  } else {
    ret = false;
  }

  return ret;
}

bool
ofp_role_mp_check(struct channel *channel,
                  struct pbuf *pbuf,
                  struct ofp_multipart_request *multipart) {
  bool ret = false;

  if (channel != NULL && multipart != NULL &&
      pbuf != NULL) {
    if (channel_role_get(channel) == OFPCR_ROLE_SLAVE) {
      switch (multipart->type) {
        case OFPMP_TABLE_FEATURES:
          /* Check length */
          if (pbuf_plen_equal_check(pbuf, 0) == LAGOPUS_RESULT_OK) {
            /* Body is empty. */
            ret = true;
          } else {
            ret = false;
          }
          break;
        default:
          ret = true;
          break;
      }
    } else {
      ret = true;
    }
  } else {
    ret = false;
  }

  return ret;
}

static int64_t
distance(uint64_t generation_id, uint64_t current_generation_id) {
  return (int64_t) (generation_id - current_generation_id);
}

bool
ofp_role_generation_id_check(uint64_t dpid,
                             uint32_t role,
                             uint64_t generation_id) {
  uint64_t gen;
  lagopus_result_t ret;

  if (role != OFPCR_ROLE_MASTER && role != OFPCR_ROLE_SLAVE) {
    /* do not care */
    return true;
  }

  ret = channel_mgr_generation_id_get(dpid, &gen);
  if (ret == LAGOPUS_RESULT_OK && distance(generation_id, gen) < 0) {
    lagopus_msg_warning("generation_id was already updated(old: %ld, new:%ld).\n",
                        gen, generation_id);
    return false;
  } else if (ret != LAGOPUS_RESULT_OK && ret != LAGOPUS_RESULT_NOT_DEFINED) {
    return false;
  }

  ret = channel_mgr_generation_id_set(dpid, generation_id);
  if (ret != LAGOPUS_RESULT_OK) {
    lagopus_msg_warning("generation_id update failed(old: %ld, new:%ld).\n",
                        gen, generation_id);
    return false;
  }

  return true;
}
