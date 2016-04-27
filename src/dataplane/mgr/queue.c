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
 *      @file   queue.c
 *      @brief  OpenFlow Queue management.
 */

#include "lagopus/ofp_dp_apis.h"
#include "lagopus/dp_apis.h"

#include "lagopus/interface.h"
#include "lagopus/port.h"


static lagopus_result_t
append_queue_config(struct interface *ifp, uint32_t qidx,
    struct packet_queue_list *packet_queue_list) {

  struct packet_queue *queue;
  struct queue_property *property;
  uint64_t queue_rate_in_bps;
  uint32_t port_speed;

  queue = calloc(1, sizeof(struct packet_queue));
  if (!queue) return LAGOPUS_RESULT_NO_MEMORY;

  TAILQ_INSERT_TAIL(packet_queue_list, queue, entry);
  queue->ofp.len = sizeof(struct ofp_packet_queue);
  queue->ofp.queue_id = ifp->ifqueue.queues[qidx]->id;
  queue->ofp.port = ifp->port->ofp_port.port_no;
  TAILQ_INIT(&queue->queue_property_list);

  port_speed = ifp->port->ofp_port.curr_speed;
  if (!port_speed) port_speed = ifp->port->ofp_port.max_speed;
  if (!port_speed) return LAGOPUS_RESULT_OK;

  property = calloc(1, sizeof(struct queue_property));
  if (!property) return LAGOPUS_RESULT_NO_MEMORY;

  TAILQ_INSERT_TAIL(&queue->queue_property_list, property, entry);
  // rate is measured in 1/10 of a percent of a current port bandwidth
  queue_rate_in_bps = ifp->ifqueue.queues[qidx]->committed_information_rate << 3;
  property->rate = (uint16_t)(queue_rate_in_bps / port_speed);
  property->ofp.len = sizeof(struct ofp_queue_prop_min_rate);
  property->ofp.property = OFPQT_MIN_RATE;
  queue->ofp.len += property->ofp.len;

  property = calloc(1, sizeof(struct queue_property));
  if (!property) return LAGOPUS_RESULT_NO_MEMORY;

  TAILQ_INSERT_TAIL(&queue->queue_property_list, property, entry);
  // rate is measured in 1/10 of a percent of a current port bandwidth
  queue_rate_in_bps = ifp->ifqueue.queues[qidx]->peak_information_rate << 3;
  property->rate = (uint16_t)(queue_rate_in_bps / port_speed);
  property->ofp.len = sizeof(struct ofp_queue_prop_max_rate);
  property->ofp.property = OFPQT_MAX_RATE;
  queue->ofp.len += property->ofp.len;

  return LAGOPUS_RESULT_OK;
}


static lagopus_result_t
append_ifp_queue_config(struct interface *ifp, struct packet_queue_list *packet_queue_list) {
  for(uint32_t i = 0; i < (uint32_t)ifp->ifqueue.nqueue; ++i) {
    lagopus_result_t rv = append_queue_config(ifp, i, packet_queue_list);
    if (rv != LAGOPUS_RESULT_OK) return rv;
  }
  return LAGOPUS_RESULT_OK;
}


static bool
iterate_port_queue_config(void *key, void *port,
     lagopus_hashentry_t he, void *packet_queue_list) {

  lagopus_result_t rv;
  (void) key;
  (void) he;

  // skip unsupported ports and continue iteration
  if (!port || !((struct port*)port)->interface) return true;
  rv = append_ifp_queue_config(((struct port*)port)->interface, packet_queue_list);
  return rv == LAGOPUS_RESULT_OK;
}


/*
 * packet queue (Agent/DP API)
 */
lagopus_result_t
ofp_packet_queue_get(uint64_t dpid,
                     struct ofp_queue_get_config_request *queue_request,
                     struct packet_queue_list *packet_queue_list,
                     struct ofp_error *error) {
  struct bridge *bridge;
  lagopus_result_t rv;
  (void) error;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (!bridge) return LAGOPUS_RESULT_NOT_FOUND;
  if (!queue_request) return LAGOPUS_RESULT_INVALID_ARGS;

  if (queue_request->port == OFPP_ANY) {
    rv = lagopus_hashmap_iterate(&bridge->ports, iterate_port_queue_config, packet_queue_list);
  } else {
    struct port *port = port_lookup(&bridge->ports, queue_request->port);
    if (port && port->interface) {
      rv = append_ifp_queue_config(port->interface, packet_queue_list);
    } else rv = LAGOPUS_RESULT_NOT_FOUND;
  }

  return rv;
}


static lagopus_result_t
append_queue_stats(struct interface *ifp, uint32_t qidx,
   struct queue_stats_list *queue_stats_list) {
  
  struct queue_stats *stats;
  struct timespec *created, now;

#ifdef HAVE_DPDK
  struct rte_sched_queue_stats *curr_stats, update;
  uint16_t queue_len;

  uint32_t rte_queue_id = qidx * RTE_SCHED_TRAFFIC_CLASSES_PER_PIPE;
  if (rte_sched_queue_read_stats(ifp->sched_port, rte_queue_id, &update, &queue_len) != 0) {
    return LAGOPUS_RESULT_NOT_FOUND;
  }  

  curr_stats = &ifp->ifqueue.stats[qidx];
  curr_stats->n_bytes += update.n_bytes;
  curr_stats->n_pkts += update.n_pkts;
  curr_stats->n_bytes_dropped += update.n_bytes_dropped;
  curr_stats->n_pkts_dropped += update.n_pkts_dropped;
#endif // HAVE_DPDK

  stats = calloc(1, sizeof(struct queue_stats));
  if (!stats) return LAGOPUS_RESULT_NO_MEMORY;
  TAILQ_INSERT_TAIL(queue_stats_list, stats, entry);

  stats->ofp.port_no = ifp->port->ofp_port.port_no;
  stats->ofp.queue_id = ifp->ifqueue.queues[qidx]->id;

#ifdef HAVE_DPDK
  stats->ofp.tx_bytes = curr_stats->n_bytes;
  stats->ofp.tx_packets = curr_stats->n_pkts;
  stats->ofp.tx_errors = curr_stats->n_pkts_dropped;
#endif // HAVE_DPDK

  created = &ifp->port->create_time;
  now = get_current_time();
  stats->ofp.duration_sec = (uint32_t)(now.tv_sec - created->tv_sec);
  stats->ofp.duration_nsec = (uint32_t)(now.tv_nsec - created->tv_nsec);
  return LAGOPUS_RESULT_OK;
}


struct queue_stats_context {
  struct queue_stats_list *queue_stats_list;
  uint32_t ofp_queue_id;
};


static lagopus_result_t
append_ifp_queue_stats(struct interface *ifp, struct queue_stats_context *queue_stats_context) {
  if (!ifp->sched_port) return LAGOPUS_RESULT_NOT_FOUND;

  if (queue_stats_context->ofp_queue_id == OFPQ_ALL) {
    for(uint32_t i = 0; i < (uint32_t)ifp->ifqueue.nqueue; ++i) {
      lagopus_result_t rv = append_queue_stats(ifp, i, queue_stats_context->queue_stats_list);
      if (rv != LAGOPUS_RESULT_OK) return rv;
    }
    return LAGOPUS_RESULT_OK;
  }

  for(uint32_t i = 0; i < (uint32_t)ifp->ifqueue.nqueue; ++i) {
    if (queue_stats_context->ofp_queue_id == ifp->ifqueue.queues[i]->id) {
      return append_queue_stats(ifp, i, queue_stats_context->queue_stats_list);
    }
  }
  return LAGOPUS_RESULT_NOT_FOUND;
}


static bool
iterate_port_queue_stats(void *key, void *port,
     lagopus_hashentry_t he, void *queue_stats_context) {
  
  (void) key;
  (void) he;

  if (!port || !((struct port*)port)->interface) return true;
  switch (append_ifp_queue_stats(((struct port*)port)->interface, queue_stats_context)) {
    // continue iteration when returned status is
    case LAGOPUS_RESULT_OK: break;
    case LAGOPUS_RESULT_NOT_FOUND: break;
    // skip iteration otherwise
    default: return false;
  }
  return true;
}


/*
 * queue_stats (Agent/DP API)
 */
lagopus_result_t
ofp_queue_stats_request_get(uint64_t dpid,
                            struct ofp_queue_stats_request *queue_stats_request,
                            struct queue_stats_list *queue_stats_list,
                            struct ofp_error *error) {

  struct queue_stats_context queue_stats_context;
  struct bridge *bridge;
  lagopus_result_t rv;
  (void) error;

  bridge = dp_bridge_lookup_by_dpid(dpid);
  if (!bridge) return LAGOPUS_RESULT_NOT_FOUND;
  if (!queue_stats_request) return LAGOPUS_RESULT_INVALID_ARGS; 

  // prepare request context to search queues on multiple ports
  queue_stats_context.queue_stats_list = queue_stats_list;
  queue_stats_context.ofp_queue_id = queue_stats_request->queue_id;

  if (queue_stats_request->port_no == OFPP_ANY) {
    rv = lagopus_hashmap_iterate(&bridge->ports, iterate_port_queue_stats, &queue_stats_context);
  } else {
    struct port *port = port_lookup(&bridge->ports, queue_stats_request->port_no);
    if (!port || !port->interface) return LAGOPUS_RESULT_NOT_FOUND;
    rv = append_ifp_queue_stats(port->interface, &queue_stats_context);
  }

  return rv;
}
