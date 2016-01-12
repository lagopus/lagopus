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
#include "channel.h"
#include "ofp_match.h"
#include "lagopus/flowdb.h"
#include "lagopus/eventq_data.h"

STATIC lagopus_result_t
ofp_port_status_create(struct port_status *port_status,
                       struct pbuf **pbuf) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  uint8_t tmp_version = 0x00;
  uint32_t tmp_xid = 0;

  if (port_status != NULL && pbuf != NULL) {
    /* alloc */
    *pbuf = pbuf_alloc(sizeof(struct ofp_port_status));
    if (pbuf != NULL) {
      pbuf_plen_set(*pbuf, sizeof(struct ofp_port_status));

      /* Fill in header. */
      /* tmp_* is replaced later. */
      ofp_header_set(&port_status->ofp_port_status.header,
                     tmp_version,
                     OFPT_PORT_STATUS,
                     sizeof(struct ofp_port_status),
                     tmp_xid);

      ret = ofp_port_status_encode(*pbuf, &port_status->ofp_port_status);
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("FAILED : ofp_port_status_encode (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("Can't allocate pbuf.\n");
      ret = LAGOPUS_RESULT_NO_MEMORY;
    }

    /* free. */
    if (ret != LAGOPUS_RESULT_OK && *pbuf != NULL) {
      pbuf_free(*pbuf);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

lagopus_result_t
ofp_port_status_handle(struct port_status *port_status, uint64_t dpid) {
  lagopus_result_t ret;
  struct pbuf *send_pbuf = NULL;

  if (port_status != NULL) {
    /* PortStatus. */
    ret = ofp_port_status_create(port_status, &send_pbuf);
    if (ret == LAGOPUS_RESULT_OK) {
      ret = ofp_role_channel_write(send_pbuf, dpid,
                                   OFPT_PORT_STATUS,
                                   port_status->ofp_port_status.reason);
      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_msg_warning("Socket write error (%s).\n",
                            lagopus_error_get_string(ret));
      }
    } else {
      lagopus_msg_warning("FAILED : ofp_port_status_create (%s).\n",
                          lagopus_error_get_string(ret));
    }

    /* free. */
    if (send_pbuf != NULL) {
      pbuf_free(send_pbuf);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
