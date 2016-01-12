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

/**
 * @file	eventq_data.h
 * @brief	Event Queue Data.
 * @details	Describe event queue data between Agent and Data-Plane.
 */

#ifndef __LAGOPUS_EVENTQ_DATA_H__
#define __LAGOPUS_EVENTQ_DATA_H__

#include "flowdb.h"
#include "lagopus_apis.h"

/**
 * @brief	packet_out
 * @details	Packet out structure for event queue.
 *
 * @details	The \e free() of a list element and data is executed
 * by the Data-Plane side.
 */
struct packet_out {
  struct ofp_packet_out ofp_packet_out;
  struct action_list action_list;
  struct pbuf *data;
  /* for ofp_error. */
  uint64_t channel_id;
  /* for ofp_error. */
  struct pbuf *req;
};

/**
 * @brief	barrier
 * @details	Barrier Request/Reply structure for event queue.
 */
struct barrier {
  /* OFPT_BARRIER_REQUEST or OFPT_BARRIER_REPLY */
  uint8_t type;
  uint32_t xid;
  /* for reply and ofp_error. */
  uint64_t channel_id;
  /* for ofp_error in OFPT_BARRIER_REQUEST. */
  struct pbuf *req;
};

/**
 * @brief	packet_in
 * @details	Packet in structure for event queue.
 *
 * @details	The \e free() of a data is executed
 * by the Agent side.
 */
struct packet_in {
  struct ofp_packet_in ofp_packet_in;
  struct match_list match_list;
  struct pbuf *data;
  uint16_t miss_send_len;
};

/**
 * @brief	flow_removed
 * @details	Flow Removed structure for event queue.
 */
struct flow_removed {
  struct ofp_flow_removed ofp_flow_removed;
  struct match_list match_list;
};

/**
 * @brief	port_status
 * @details	Port status structure for event queue.
 */
struct port_status {
  struct ofp_port_status ofp_port_status;
};

/**
 * @brief	error
 * @details	ofp_error status structure for event queue.
 */
struct error {
  struct ofp_error ofp_error;
  uint32_t xid;
  uint64_t channel_id;
};

/**
 * @brief	eventq_data_type
 * @details	Type of event queue data.
 */
enum eventq_data_type {
  LAGOPUS_EVENTQ_PACKET_OUT,
  LAGOPUS_EVENTQ_PACKET_IN,
  LAGOPUS_EVENTQ_BARRIER_REQUEST,
  LAGOPUS_EVENTQ_BARRIER_REPLY,
  LAGOPUS_EVENTQ_FLOW_REMOVED,
  LAGOPUS_EVENTQ_PORT_STATUS,
  LAGOPUS_EVENTQ_ERROR,
};

/**
 * @brief	eventq_data
 * @details	Data structure for event queue.
 */
struct eventq_data {
  union {
    enum eventq_data_type type;
  };
  /* Free function for eventq_data .*/
  void (*free) (struct eventq_data *data);
  union {
    struct packet_out packet_out;
    struct packet_in packet_in;
    struct barrier barrier;
    struct flow_removed flow_removed;
    struct port_status port_status;
    struct error error;
  };
};

/**
 * @brief	eventq_t
 * @details	Type of event queue.
 */
typedef LAGOPUS_BOUND_BLOCK_Q_DECL(eventq_t, struct eventq_data *) eventq_t;

/**
 * Free packet_out queue data.
 *
 *     @param[in]	data	A pointer to \e eventq_data structure.
 *
 *     @retval	void
 */
void
ofp_packet_out_free(struct eventq_data *data);

/**
 * Free packet_in queue data.
 *
 *     @param[in]	data	A pointer to \e eventq_data structure.
 *
 *     @retval	void
 */
void
ofp_packet_in_free(struct eventq_data *data);

/**
 * Free flow_removed queue data.
 *
 *     @param[in]	data	A pointer to \e eventq_data structure.
 *
 *     @retval	void
 */
void
ofp_flow_removed_free(struct eventq_data *data);

/**
 * Free barrier queue data.
 *
 *     @param[in]	data	A pointer to \e eventq_data structure.
 *
 *     @retval	void
 */
void
ofp_barrier_free(struct eventq_data *data);

#endif /* __LAGOPUS_EVENTQ_DATA_H__ */
