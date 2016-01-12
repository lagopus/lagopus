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


/*
 * packet queue (Agent/DP API)
 */
lagopus_result_t
ofp_packet_queue_get(uint64_t dpid,
                     struct ofp_queue_get_config_request *queue_request,
                     struct packet_queue_list *packet_queue_list,
                     struct ofp_error *error) {
  (void) dpid;
  (void) queue_request;
  (void) packet_queue_list;
  (void) error;
  return LAGOPUS_RESULT_OK;
}


/*
 * queue_stats (Agent/DP API)
 */
lagopus_result_t
ofp_queue_stats_request_get(uint64_t dpid,
                            struct ofp_queue_stats_request *queue_stats_request,
                            struct queue_stats_list *queue_stats_list,
                            struct ofp_error *error) {
  (void) dpid;
  (void) queue_stats_request;
  (void) queue_stats_list;
  (void) error;
  return LAGOPUS_RESULT_OK;
}
