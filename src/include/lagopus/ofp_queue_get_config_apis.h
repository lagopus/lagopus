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
 * @file	ofp_queue_get_config_apis.h
 * @brief	Agent/Data-Plane APIs for ofp_queue_get_config
 * @details	Describe APIs between Agent and Data-Plane for ofp_queue_get_config.
 */
#ifndef __LAGOPUS_OFP_QUEUE_GET_CONFIG_APIS_H__
#define __LAGOPUS_OFP_QUEUE_GET_CONFIG_APIS_H__

#include "lagopus_apis.h"
#include "openflow.h"
#include "lagopus/bridge.h"
#include "lagopus/pbuf.h"


/* Queue get config */
/**
 * Queue property.
 */
struct queue_property {
  TAILQ_ENTRY(queue_property) entry;
  struct ofp_queue_prop_header ofp;

  /* for ofp_queue_prop_min_rate, ofp_queue_prop_max_rate. */
  uint16_t rate;

  /* for ofp_queue_prop_experimenter. */
  uint32_t experimenter;
  struct pbuf *data;
};

/**
 * Queue property list.
 */
TAILQ_HEAD(queue_property_list, queue_property);

/**
 * Packet queue.
 */
struct packet_queue {
  TAILQ_ENTRY(packet_queue) entry;
  struct ofp_packet_queue ofp;
  struct queue_property_list queue_property_list;
};

/**
 * Packet queue list.
 */
TAILQ_HEAD(packet_queue_list, packet_queue);

/**
 * Get array of queue configuration for \b OFPT_QUEUE_GET_CONFIG_REQUEST/REPLY.
 *
 *     @param[in]	dpid	Datapath id.
 *     @param[in]	queue_request	A pointer to \e ofp_queue_get_config_request
 *     structures.
 *     @param[out]	packet_queue_list	A pointer to list of packet queue.
 *     @param[out]	error	A pointer to \e ofp_error structure.
 *     If errors occur, set filed values.
 *
 *     @retval	LAGOPUS_RESULT_OK	Succeeded.
 *     @retval	LAGOPUS_RESULT_ANY_FAILURES	Failed.
 *
 *     @details	The \e free() of a list element is executed
 *     by the Agent side.
 */
lagopus_result_t
ofp_packet_queue_get(uint64_t dpid,
                     struct ofp_queue_get_config_request *queue_request,
                     struct packet_queue_list *packet_queue_list,
                     struct ofp_error *error);
/* Queue get config END */

#endif /* __LAGOPUS_OFP_QUEUE_GET_CONFIG_APIS_H__ */
