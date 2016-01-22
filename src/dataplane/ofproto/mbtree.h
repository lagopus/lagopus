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

#ifndef SRC_DATAPLANE_OFPROTO_MBTREE_H_
#define SRC_DATAPLANE_OFPROTO_MBTREE_H_

struct flow;
struct flow_list;
struct lagopus_packet;

void cleanup_mbtree(struct flow_list *flows);
void build_mbtree(struct flow_list *flows);
struct flow *find_mbtree(struct lagopus_packet *pkt, struct flow_list *flows);

#endif /* SRC_DATAPLANE_OFPROTO_MBTREE_H_ */
