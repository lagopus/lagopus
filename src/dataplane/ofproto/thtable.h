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

#define THTABLE_INDEX_MAX UINT16_MAX

struct flow;
struct lagopus_packet;
struct htlist;
typedef struct htlist **thtable_t;

thtable_t thtable_alloc(void);
void thtable_free(thtable_t thtable);

lagopus_result_t thtable_add_flow(struct flow *flow, thtable_t thtable);

void thtable_update(struct flow_list *flow_list);

struct flow *thtable_match(struct lagopus_packet *pkt, thtable_t thtable);
