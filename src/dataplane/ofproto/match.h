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
 *      @file   match.h
 *	@brief	Flow match internal APIs
 */

#ifndef SRC_DATAPLANE_OFPROTO_MATCH_H_
#define SRC_DATAPLANE_OFPROTO_MATCH_H_

/**
 * match result.
 */
enum {
  LAGOPUS_MISMATCH,
  LAGOPUS_MATCHED
};

typedef struct flow *
(*find_flow_t)(const struct lagopus_packet *, struct flow_list *, uint16_t *);

/**
 * Check if packet is matched by specified flow entry.
 *
 * @param[in]   pkt		packet.
 * @param[in]   flow		flow within match field.
 *
 * @retval LAGOPUS_MATCHED	flow is matched
 * @retval LAGOPUS_MISMATCH	flow is not matched
 *
 * flow has multiple match field.
 * return LAGOPUS_MATCHED if the packet is matched as all match field.
 */
int
lagopus_match(const struct lagopus_packet *, struct flow *);

#endif /* SRC_DATAPLANE_OFPROTO_MATCH_H_ */
