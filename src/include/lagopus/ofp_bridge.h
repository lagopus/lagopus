/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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
 * @file	ofp_bridge.h
 * @brief	Agent/Data-Plane connection
 * @details	Describe APIs between Agent and Data-Plane.
 */
#ifndef __LAGOPUS_OFP_BRIDGE_H__
#define __LAGOPUS_OFP_BRIDGE_H__

#include "lagopus/eventq_data.h"

#define DATAQ_SIZE		1000LL
#define EVENTQ_SIZE		1000LL
#define EVENT_DATAQ_SIZE	1000LL

#ifdef OFPH_POLL_WRITING
/* decl edq_buffer */
STAILQ_HEAD(edq_buffer, edq_buffer_entry);
struct edq_buffer_entry {
  STAILQ_ENTRY(edq_buffer_entry) qentry;
  struct eventq_data *qdata;
};
#endif  /* OFPH_POLL_WRITING */

struct ofp_bridge {
  uint64_t dpid;

  /* Data Queue */
  eventq_t dataq;
  /* Event Queue */
  eventq_t eventq;
  /* Event/Data Queue */
  eventq_t event_dataq;

#ifdef OFPH_POLL_WRITING
  struct edq_buffer edq_buffer;
#endif  /* OFPH_POLL_WRITING */
};

/**
 * create ofp_bridge
 */
lagopus_result_t
ofp_bridge_create(struct ofp_bridge **retptr,
                  uint64_t dpid);

/**
 * shutdown ofp_bridge
 */
void
ofp_bridge_shutdown(struct ofp_bridge *ptr, bool is_canceled);

/**
 * destroy ofp_bridge
 */
void
ofp_bridge_destroy(struct ofp_bridge *ptr);


#endif /* __LAGOPUS_OFP_BRIDGE_H__ */
